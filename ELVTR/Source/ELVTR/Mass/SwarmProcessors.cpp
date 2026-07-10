#include "SwarmProcessors.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "SwarmFragments.h"
#include "SwarmSubsystem.h"

namespace SwarmTuning
{
	constexpr float BroodSpeed = 320.f;
	constexpr float RetinueSpeed = 450.f;
	constexpr float SeparationRadius = 60.f;
	constexpr float SeparationRadiusSq = SeparationRadius * SeparationRadius;
	constexpr float SeparationWeight = 1.4f;
	constexpr int32 SeparationNeighborCap = 6;
	constexpr float BroodContactRange = 120.f;
	constexpr float RetinueEngageRange = 250.f;
	constexpr float SlotArriveRadius = 40.f;
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

	FORCEINLINE void UpdateAnimBits(uint8& Bits, const FVector& Velocity, float TimeSeconds, float Phase, bool bAttacking)
	{
		const bool bMoving = Velocity.SizeSquared2D() > 100.f;
		const bool bFrame = bMoving && (FMath::Fmod((TimeSeconds + Phase) * SwarmTuning::WalkAnimHz, 2.f) >= 1.f);

		Bits &= SwarmAnim::TeamBit; // preserve team, rebuild the rest
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
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmGridBuildProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
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
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			Swarm->AddToGrid(Transforms[i].GetTransform().GetLocation());
		}
	});
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
			const FVector Seek = (Attractor - Location).GetSafeNormal2D();
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
	EntityQuery.AddRequirement<FRetinueFollowFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRetinueTag>(EMassFragmentPresence::All);
}

void URetinueFollowProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetFragmentView<FRetinueFollowFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const FVector SlotTarget = Attractor + FVector(Follow[i].SlotOffset.X, Follow[i].SlotOffset.Y, 0.f);
			const FVector ToSlot = SlotTarget - Location;
			const float Dist = ToSlot.Size2D();

			// Arrive: full speed far out, ease into the slot.
			const float SpeedScale = FMath::Clamp(Dist / SwarmTuning::SlotArriveRadius, 0.f, 1.f);
			Velocities[i].Value = ToSlot.GetSafeNormal2D() * SwarmTuning::RetinueSpeed * SpeedScale * Jitter[i].SpeedScale;

			// "Engage": any brood nearby flips the attack pose. No damage in the spike.
			bool bEnemyNear = false;
			Swarm->QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
			{
				if (!bEnemyNear && FVector::DistSquared2D(Entry.Location, Location) < FMath::Square(SwarmTuning::RetinueEngageRange))
				{
					bEnemyNear = true;
				}
			});
			if (bEnemyNear)
			{
				Anim[i].Bits |= SwarmAnim::AttackBit;
			}
		}
	});
}

//----------------------------------------------------------------------
// Integrate + fill render buffers
//----------------------------------------------------------------------
USwarmIntegrateProcessor::USwarmIntegrateProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmIntegrate"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmSteering")));
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
