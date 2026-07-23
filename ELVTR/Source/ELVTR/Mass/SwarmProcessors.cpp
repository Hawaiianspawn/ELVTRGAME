#include "SwarmProcessors.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "SwarmCombat.h"
#include "SwarmFragments.h"
#include "SwarmStats.h"
#include "SwarmSubsystem.h"

namespace SwarmTuning
{
	constexpr float BroodSpeed = 320.f;
	constexpr float RetinueSpeed = 450.f;
	constexpr float ChargeSpeedMul = 1.25f;
	constexpr float SeparationRadius = 60.f;
	constexpr float SeparationRadiusSq = SeparationRadius * SeparationRadius;
	constexpr float SeparationWeight = 1.4f;
	constexpr int32 SeparationNeighborCap = 6;
	constexpr float BroodContactRange = 120.f;

	/** How far a unit will step off its anchor to hit something, per stance. */
	constexpr float FollowEngageRange = 250.f;
	constexpr float ChargeEngageRange = 400.f;
	constexpr float LineEngageRange = 160.f;	// Hold / Rally — keep the line

	constexpr float SlotArriveRadius = 40.f;
	constexpr float RallySlotScale = 0.45f;		// collapse tight onto the hero
	constexpr float WalkAnimHz = 6.f;
}

namespace
{
	FORCEINLINE FVector SeparationForce(const USwarmSubsystem& Swarm, const FVector& Location)
	{
		FVector Push = FVector::ZeroVector;
		int32 Considered = 0;
		Swarm.QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (Considered >= SwarmTuning::SeparationNeighborCap)
			{
				return;
			}
			const FVector Delta = Location - Entry.Location;
			const float DistSq = Delta.SizeSquared2D();
			if (DistSq > UE_KINDA_SMALL_NUMBER && DistSq < SwarmTuning::SeparationRadiusSq)
			{
				Push += Delta.GetSafeNormal2D() * (1.f - FMath::Sqrt(DistSq) / SwarmTuning::SeparationRadius);
				++Considered;
			}
		});
		return Push;
	}

	/** Nearest entry of the opposing team inside the 3x3 grid neighbourhood. */
	FORCEINLINE bool FindNearestEnemy(const USwarmSubsystem& Swarm, const FVector& Location, bool bWantRetinue, FVector& OutLocation, float& OutDistSq)
	{
		bool bFound = false;
		float BestSq = TNumericLimits<float>::Max();
		FVector Best = FVector::ZeroVector;

		Swarm.QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (Entry.bRetinue != bWantRetinue)
			{
				return;
			}
			const float DistSq = FVector::DistSquared2D(Entry.Location, Location);
			if (DistSq < BestSq)
			{
				BestSq = DistSq;
				Best = Entry.Location;
				bFound = true;
			}
		});

		OutLocation = Best;
		OutDistSq = BestSq;
		return bFound;
	}

	FORCEINLINE void UpdateAnimBits(uint8& Bits, const FVector& Velocity, float TimeSeconds, float Phase, bool bAttacking)
	{
		const bool bMoving = Velocity.SizeSquared2D() > 100.f;
		const bool bFrame = bMoving && (FMath::Fmod((TimeSeconds + Phase) * SwarmTuning::WalkAnimHz, 2.f) >= 1.f);

		// Preserve team + leash warning (owned by the steering pass); rebuild the rest.
		Bits &= (SwarmAnim::TeamBit | SwarmAnim::LeashWarnBit);
		Bits |= bFrame ? SwarmAnim::FrameBit : 0;
		Bits |= bAttacking ? SwarmAnim::AttackBit : 0;
		Bits |= (Velocity.X < 0.f) ? SwarmAnim::FlipBit : 0;
	}
}

//----------------------------------------------------------------------
// Grid build
//----------------------------------------------------------------------
USwarmGridBuildProcessor::USwarmGridBuildProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmGrid"));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmGridBuildProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmGridBuildProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmGridBuild, SwarmGridBuild);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	const int32 ExpectedCount = Swarm->GetTrackedEntities().Num();
	Swarm->ResetGrid(ExpectedCount);
	Swarm->ResetRenderBuffers(ExpectedCount);

	EntityQuery.ForEachEntityChunk(Context, [Swarm](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetFragmentView<FSwarmAnimFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			Swarm->AddToGrid(Transforms[i].GetTransform().GetLocation(), (Anim[i].Bits & SwarmAnim::TeamBit) != 0);
		}
	});

	// Occupancy is the grid's own health metric: cells vs. entities tells you
	// whether GridCellSize is bucketing sensibly or degenerating toward a list.
	SET_DWORD_STAT(STAT_SwarmGridCells, Swarm->GetGridCellCount());
}

//----------------------------------------------------------------------
// Brood steering: seek attractor + separation
//----------------------------------------------------------------------
UBroodSteeringProcessor::UBroodSteeringProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmSteering"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmGrid")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UBroodSteeringProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FBroodTag>(EMassFragmentPresence::All);
}

void UBroodSteeringProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmBroodSteering, SwarmBroodSteering);

	const USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();

			// Bite whatever is in front of you; otherwise keep marching on the hero.
			// This is what makes a front line form instead of the tide flowing past.
			FVector EnemyLocation;
			float EnemyDistSq = 0.f;
			const FVector Target = FindNearestEnemy(*Swarm, Location, /*bWantRetinue=*/true, EnemyLocation, EnemyDistSq)
				? EnemyLocation
				: Attractor;

			const FVector Seek = (Target - Location).GetSafeNormal2D();
			const FVector Push = SeparationForce(*Swarm, Location) * SwarmTuning::SeparationWeight;

			FVector Desired = Seek + Push;
			Desired.Z = 0.f;
			Velocities[i].Value = Desired.GetSafeNormal2D() * SwarmTuning::BroodSpeed * Jitter[i].SpeedScale;
		}
	});
}

//----------------------------------------------------------------------
// Retinue: hold formation slot around the hero
//----------------------------------------------------------------------
URetinueFollowProcessor::URetinueFollowProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmSteering"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmGrid")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void URetinueFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRetinueFollowFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRetinueTag>(EMassFragmentPresence::All);
}

void URetinueFollowProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmRetinueFollow, SwarmRetinueFollow);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();
	const ESwarmStance GlobalStance = Swarm->GetStance();
	const FVector StanceAnchor = Swarm->GetStanceAnchor();

	int32 BrokenThisFrame = 0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor, GlobalStance, StanceAnchor, &BrokenThisFrame](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetMutableFragmentView<FRetinueFollowFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const FVector2D Slot = Follow[i].SlotOffset;
			const float HeroDistSq = FVector::DistSquared2D(Location, Attractor);

			// --- leash (docs/RTS-VERTICAL-SLICE.md §2) -------------------
			// Latch out past Radius, latch back in only inside ReanchorRadius,
			// so a unit sitting exactly on the boundary can't flicker.
			if (Follow[i].bLeashBroken)
			{
				if (HeroDistSq < SwarmLeash::ReanchorRadiusSq)
				{
					Follow[i].bLeashBroken = false;
				}
			}
			else if (HeroDistSq > SwarmLeash::RadiusSq)
			{
				Follow[i].bLeashBroken = true;
			}

			// A broken unit drops to Follow and paths back — it never detaches.
			const ESwarmStance Stance = Follow[i].bLeashBroken ? ESwarmStance::Follow : GlobalStance;

			if (Follow[i].bLeashBroken)
			{
				++BrokenThisFrame;
			}

			// Warn before breaking: breaking stance must never feel random.
			const bool bWarn = !Follow[i].bLeashBroken && HeroDistSq > SwarmLeash::WarnRadiusSq;
			Anim[i].Bits = bWarn ? (Anim[i].Bits | SwarmAnim::LeashWarnBit)
								 : (Anim[i].Bits & ~SwarmAnim::LeashWarnBit);

			// --- where this stance wants the unit to stand ---------------
			FVector Anchor;
			float EngageRange;
			float SpeedMul = 1.f;

			switch (Stance)
			{
			case ESwarmStance::Hold:
				// Anchored to the world point where the order was issued.
				Anchor = StanceAnchor + FVector(Slot.X, Slot.Y, 0.f);
				EngageRange = SwarmTuning::LineEngageRange;
				break;

			case ESwarmStance::Charge:
				Anchor = StanceAnchor;
				EngageRange = SwarmTuning::ChargeEngageRange;
				SpeedMul = SwarmTuning::ChargeSpeedMul;
				break;

			case ESwarmStance::Rally:
				Anchor = Attractor + FVector(Slot.X, Slot.Y, 0.f) * SwarmTuning::RallySlotScale;
				EngageRange = SwarmTuning::LineEngageRange;
				break;

			case ESwarmStance::Follow:
			default:
				Anchor = Attractor + FVector(Slot.X, Slot.Y, 0.f);
				EngageRange = SwarmTuning::FollowEngageRange;
				break;
			}

			// --- auto-fight: step off the anchor for anything in reach ----
			FVector EnemyLocation;
			float EnemyDistSq = 0.f;
			const bool bEnemyFound = FindNearestEnemy(*Swarm, Location, /*bWantRetinue=*/false, EnemyLocation, EnemyDistSq);
			const bool bEngaging = bEnemyFound && EnemyDistSq < FMath::Square(EngageRange);

			const FVector Target = bEngaging ? EnemyLocation : Anchor;
			const FVector ToTarget = Target - Location;
			const float Dist = ToTarget.Size2D();

			// Arrive: full speed far out, ease onto the anchor.
			const float Arrive = FMath::Clamp(Dist / SwarmTuning::SlotArriveRadius, 0.f, 1.f);
			const FVector Push = SeparationForce(*Swarm, Location) * SwarmTuning::SeparationWeight;

			// Don't renormalise: the blended vector's own length is the throttle.
			// Sitting on the anchor with no crowding => zero, so the line settles
			// instead of vibrating between seek and separation.
			FVector Desired = ToTarget.GetSafeNormal2D() * Arrive + Push;
			Desired.Z = 0.f;
			const float Throttle = FMath::Min(Desired.Size2D(), 1.f);
			Velocities[i].Value = Desired.GetSafeNormal2D()
				* SwarmTuning::RetinueSpeed * SpeedMul * Throttle * Jitter[i].SpeedScale;

			if (bEngaging)
			{
				Anim[i].Bits |= SwarmAnim::AttackBit;
			}
		}
	});

	Swarm->AddLeashBroken(BrokenThisFrame);
}

//----------------------------------------------------------------------
// Integrate + fill render buffers
//----------------------------------------------------------------------
USwarmIntegrateProcessor::USwarmIntegrateProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmIntegrate"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmCombat")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmIntegrateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmIntegrateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmIntegrate, SwarmIntegrate);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	const float TimeSeconds = Context.GetWorld()->GetTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [Swarm, DeltaTime, TimeSeconds](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			FTransform& Transform = Transforms[i].GetMutableTransform();
			const FVector Velocity = Velocities[i].Value;
			Transform.SetTranslation(Transform.GetTranslation() + Velocity * DeltaTime);

			const bool bWasAttacking = (Anim[i].Bits & SwarmAnim::AttackBit) != 0;
			UpdateAnimBits(Anim[i].Bits, Velocity, TimeSeconds, Jitter[i].Phase, bWasAttacking);

			Swarm->PushRenderEntry(Transform.GetTranslation(), Anim[i].Bits);
		}
	});

	// The N every ms number in this group is measured at. Live counts are only
	// valid once integrate has repacked the buffers, so they belong here.
	SET_DWORD_STAT(STAT_SwarmAliveRetinue, Swarm->GetAliveRetinue());
	SET_DWORD_STAT(STAT_SwarmAliveBrood, Swarm->GetAliveBrood());
}

//----------------------------------------------------------------------
// Contact: brood within range of hero
//----------------------------------------------------------------------
USwarmContactProcessor::USwarmContactProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmIntegrate")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmContactProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FBroodTag>(EMassFragmentPresence::All);
}

void USwarmContactProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmContact, SwarmContact);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();
	int32 Contacts = 0;

	EntityQuery.ForEachEntityChunk(Context, [&Contacts, Attractor](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			if (FVector::DistSquared2D(Transforms[i].GetTransform().GetLocation(), Attractor) < FMath::Square(SwarmTuning::BroodContactRange))
			{
				++Contacts;
			}
		}
	});

	Swarm->AddHeroContacts(Contacts);
}
