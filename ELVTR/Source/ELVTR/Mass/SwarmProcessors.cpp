#include "SwarmProcessors.h"

#include "HAL/IConsoleManager.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "Algo/BinarySearch.h"
#include "SwarmCombat.h"
#include "SwarmFormation.h"
#include "SwarmFragments.h"
#include "SwarmStats.h"
#include "SwarmSubsystem.h"

namespace SwarmTuning
{
	// Retinue-side steering. Still compile-time: these describe YOUR line, and the
	// stance system is the thing that's meant to move it. The brood equivalents below
	// are CVars because the horde is what gets re-shaped between test runs.
	constexpr float RetinueSpeed = 450.f;
	constexpr float ChargeSpeedMul = 1.25f;
	constexpr float SeparationRadius = 60.f;
	constexpr float SeparationWeight = 1.4f;
	constexpr int32 SeparationNeighborCap = 6;

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
	// --- horde dials (live via console) ----------------------------------
	// The brood's movement was constexpr until 2026-07-26, which meant the only thing
	// tunable about the tide was how hard it hit. Everything that decides how it LOOKS
	// arriving — pace, packing, what it goes for, where it comes from — is here now.
	// Read once per processor pass, never per entity.

	TAutoConsoleVariable<float> CVarBroodSpeed(
		TEXT("Swarm.BroodSpeed"), 320.f,
		TEXT("March speed of a brood, uu/s. Against RetinueSpeed 450 this is what decides\n")
		TEXT("whether the tide can be outrun or has to be fought. [0..1200]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSeparation(
		TEXT("Swarm.BroodSeparation"), 60.f,
		TEXT("Personal space of a brood, uu — the radius it pushes neighbours out of.\n")
		TEXT("The density dial: low packs the tide into a solid mass that arrives all at\n")
		TEXT("once, high spreads it into a loose skirmish line. [0..300]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSeparationWeight(
		TEXT("Swarm.BroodSeparationWeight"), 1.4f,
		TEXT("How hard the separation push competes with the urge to close. Above ~2 the\n")
		TEXT("horde mills instead of charging; at 0 it stacks into one column. [0..4]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarBroodSeparationCap(
		TEXT("Swarm.BroodSeparationCap"), 6,
		TEXT("Neighbours one brood will push against per frame. Also the cost dial for the\n")
		TEXT("steering pass — this bounds its inner loop. [1..16]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodAggroRange(
		TEXT("Swarm.BroodAggroRange"), 600.f,
		TEXT("How far a brood will divert from the bearer to bite a soldier, uu. THE\n")
		TEXT("horde-behaviour dial: high = the tide is stopped by your line and a front\n")
		TEXT("forms; low = it flows past the line and dives for the flame, and holding\n")
		TEXT("ground stops being enough. Capped in practice by the 3x3 grid reach\n")
		TEXT("(~600uu at GridCellSize 200), so values beyond that read the same. [0..600]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodContactRange(
		TEXT("Swarm.BroodContactRange"), 120.f,
		TEXT("Radius, uu, inside which a brood counts as mobbing the bearer. Telemetry and\n")
		TEXT("HUD only — damage is Swarm.HeroMeleeRange. [0..600]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodWalkHz(
		TEXT("Swarm.BroodWalkHz"), 6.f,
		TEXT("Brood walk-cycle rate, full frames/sec. Independent of the retinue's so the\n")
		TEXT("two teams read as different creatures at a glance. [0..20]"), ECVF_Default);

	// --- facing (2026-07-26) ---------------------------------------------
	// "They face outwards from you but sometimes look back at you." The resting posture
	// of your army is a ring staring into the dark; the glance is what keeps the flame
	// premise legible — the light is the thing they are all here for, so their eyes keep
	// returning to it.

	TAutoConsoleVariable<float> CVarFacingMoveSpeed(
		TEXT("Swarm.FacingMoveSpeed"), 40.f,
		TEXT("Speed, uu/s, above which a unit faces where it is GOING rather than outward.\n")
		TEXT("Below it the unit is 'at rest' and turns its back to the bearer to watch the\n")
		TEXT("dark. Set very high to force the outward ring on at all times; set to 0 to\n")
		TEXT("get pure velocity facing and no ring at all. [0..600]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarFacingTurnHysteresis(
		TEXT("Swarm.FacingTurnHysteresis"), 1.4f,
		TEXT("How far, in world facing steps (32 per turn), the desired angle must move\n")
		TEXT("before a unit commits to it. This is the anti-strobe dial: at 0 a unit\n")
		TEXT("steering along a column boundary flickers between two sprites every tick.\n")
		TEXT("Above ~4 turning visibly lags the movement. [0..8]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarGlancePeriod(
		TEXT("Swarm.GlancePeriod"), 6.5f,
		TEXT("Average seconds between one unit glancing back at the bearer. Per-unit and\n")
		TEXT("de-synced by the spawn jitter phase, so the army never turns in unison.\n")
		TEXT("[1..30]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarGlanceDuration(
		TEXT("Swarm.GlanceDuration"), 1.1f,
		TEXT("Seconds a glance back is held. Long enough to read at gameplay zoom, short\n")
		TEXT("enough that the ring still reads as facing outward. [0..5]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarGlanceEnabled(
		TEXT("Swarm.Glance"), 1,
		TEXT("Master switch for the look-back-at-the-flame tic. 0 freezes every resting\n")
		TEXT("unit facing outward."), ECVF_Default);

	/** Everything the facing pass reads, snapshotted once per processor pass. */
	struct FFacingParams
	{
		float MoveSpeedSq = 0.f;
		float Hysteresis = 0.f;
		float GlancePeriod = 0.f;
		float GlanceDuration = 0.f;
		bool bGlanceEnabled = false;
	};

	/**
	 * A unit's world facing step for this frame.
	 *
	 * The rule, in priority order:
	 *   1. MOVING  — face where you are going. A soldier crossing the field looking
	 *      sideways reads as broken no matter how good the fiction is.
	 *   2. GLANCING — a resting soldier periodically turns and looks at the bearer.
	 *   3. AT REST — face radially AWAY from the bearer. The army standing in the only
	 *      light in the world forms a ring watching the dark it came out of.
	 *
	 * Outward-and-glance are retinue-only. The brood is not gathered around the flame,
	 * it is coming for it, so its facing is simply its heading and it holds the last one
	 * when it stops. (Its sheet row ignores the column today anyway — every brood
	 * direction packs the same frame — but the value is computed honestly so the day it
	 * gains rotations nothing here changes.)
	 */
	FORCEINLINE uint8 ResolveFacing(uint8 Current, const FVector& Location, const FVector& Velocity,
		const FVector& HeroLocation, bool bRetinue, float Phase, float TimeSeconds,
		const FFacingParams& P)
	{
		const FVector2f Vel2((float)Velocity.X, (float)Velocity.Y);
		FVector2f Dir = Vel2;

		if (Vel2.SizeSquared() <= P.MoveSpeedSq && bRetinue)
		{
			const FVector2f Outward((float)(Location.X - HeroLocation.X),
									(float)(Location.Y - HeroLocation.Y));

			// Standing ON the bearer has no outward direction; keep the last facing
			// rather than snapping to an arbitrary one.
			if (Outward.IsNearlyZero())
			{
				return Current;
			}

			// De-synced from the spawn jitter phase, and scaled by a different irrational
			// than the size roll (SwarmRenderPack uses 0.618) so a unit's glance clock
			// does not correlate with its size or its walk cycle. Without this the whole
			// line turns its head on the same frame, which reads as a cutscene.
			bool bGlancing = false;
			if (P.bGlanceEnabled && P.GlanceDuration > 0.f)
			{
				const float Cycle = FMath::Max(P.GlancePeriod, 0.1f);
				const float Offset = FMath::Frac(Phase * 0.3819660113f) * Cycle;
				bGlancing = FMath::Fmod(TimeSeconds + Offset, Cycle) < P.GlanceDuration;
			}

			Dir = bGlancing ? -Outward : Outward;
		}

		if (Dir.IsNearlyZero())
		{
			return Current;
		}

		const int32 Desired = SwarmFacing::IndexFromDir(Dir);

		// Shortest signed step delta, wrapped. Steps is a power of two, so the mask does
		// the wrap for free and for negatives too.
		constexpr int32 Half = SwarmFacing::Steps / 2;
		const int32 Delta = ((Desired - (int32)Current + Half) & SwarmFacing::StepMask) - Half;

		return (FMath::Abs(Delta) > P.Hysteresis) ? (uint8)Desired : Current;
	}

	FORCEINLINE FVector SeparationForce(const USwarmSubsystem& Swarm, const FVector& Location,
		float Radius, int32 NeighborCap)
	{
		const float RadiusSq = Radius * Radius;
		FVector Push = FVector::ZeroVector;
		int32 Considered = 0;
		Swarm.QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (Considered >= NeighborCap)
			{
				return;
			}
			const FVector Delta = Location - Entry.Location;
			const float DistSq = Delta.SizeSquared2D();
			if (DistSq > UE_KINDA_SMALL_NUMBER && DistSq < RadiusSq)
			{
				Push += Delta.GetSafeNormal2D() * (1.f - FMath::Sqrt(DistSq) / Radius);
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

	FORCEINLINE void UpdateAnimBits(uint8& Bits, const FVector& Velocity, float TimeSeconds, float Phase, bool bAttacking, float WalkHz)
	{
		const bool bMoving = Velocity.SizeSquared2D() > 100.f;
		const bool bFrame = bMoving && (FMath::Fmod((TimeSeconds + Phase) * WalkHz, 2.f) >= 1.f);

		// Preserve the bits other passes own (team, leash warning, swing, hit flash);
		// rebuild the rest. Anything left off SwarmAnim::PreservedBits is wiped here
		// every single tick, which is a silent way to lose a feature.
		Bits &= SwarmAnim::PreservedBits;
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
	EntityQuery.AddRequirement<FSwarmStrikeFragment>(EMassFragmentAccess::ReadOnly);
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

	// Each entity's own K (how many blows it may hand out this swing) rides into the
	// grid alongside its reach, so the combat pass can cap claims against it instead
	// of trusting the radius alone — see FGridEntry::BlowsClaimed.
	const int32 RetinueTargets = SwarmCombatTuning::RetinueTargetsPerHit();
	const int32 BroodTargets = SwarmCombatTuning::BroodTargetsPerHit();

	EntityQuery.ForEachEntityChunk(Context, [Swarm, RetinueTargets, BroodTargets](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetFragmentView<FSwarmAnimFragment>();
		const TConstArrayView<FSwarmStrikeFragment> Strike = ChunkContext.GetFragmentView<FSwarmStrikeFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			// Strike state rides along with position so the combat pass can tell
			// which neighbours are actually connecting this frame. Published here,
			// at the top of the frame, because the swing clocks were advanced at the
			// end of the last one — so what combat reads is current, not stale.
			const bool bRetinue = (Anim[i].Bits & SwarmAnim::TeamBit) != 0;
			Swarm->AddToGrid(
				Transforms[i].GetTransform().GetLocation(),
				bRetinue,
				Strike[i].bStrikeFrame,
				Strike[i].StrikeReachSq,
				bRetinue ? RetinueTargets : BroodTargets);
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

	// Snapshotted once per pass — the hot loop does zero CVar reads.
	const float Speed = FMath::Max(CVarBroodSpeed.GetValueOnAnyThread(), 0.f);
	const float SepRadius = FMath::Max(CVarBroodSeparation.GetValueOnAnyThread(), 0.f);
	const float SepWeight = CVarBroodSeparationWeight.GetValueOnAnyThread();
	const int32 SepCap = FMath::Clamp(CVarBroodSeparationCap.GetValueOnAnyThread(), 1, 16);
	const float AggroSq = FMath::Square(FMath::Max(CVarBroodAggroRange.GetValueOnAnyThread(), 0.f));

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor, Speed, SepRadius, SepWeight, SepCap, AggroSq](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();

			// Bite whatever is in front of you; otherwise keep marching on the hero.
			// Whether a front line forms at all is decided here: a brood only diverts to
			// a soldier inside AggroRange, so shrinking it lets the tide flow PAST your
			// line toward the flame instead of piling against it.
			FVector EnemyLocation;
			float EnemyDistSq = 0.f;
			const bool bDivert = FindNearestEnemy(*Swarm, Location, /*bWantRetinue=*/true, EnemyLocation, EnemyDistSq)
				&& EnemyDistSq <= AggroSq;
			const FVector Target = bDivert ? EnemyLocation : Attractor;

			const FVector Seek = (Target - Location).GetSafeNormal2D();
			const FVector Push = SeparationForce(*Swarm, Location, SepRadius, SepCap) * SepWeight;

			FVector Desired = Seek + Push;
			Desired.Z = 0.f;
			Velocities[i].Value = Desired.GetSafeNormal2D() * Speed * Jitter[i].SpeedScale;
		}
	});
}

//----------------------------------------------------------------------
// Retinue: close ranks over the dead
//----------------------------------------------------------------------
URetinueFormationProcessor::URetinueFormationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmSteering"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmGrid")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	// Deliberately NOT ordered against RetinueFollow. If the follow pass runs first it
	// steers one frame against the pre-repack slots, which is a single frame of a line
	// that was about to re-form anyway — not worth a serialisation point on the group.
}

void URetinueFormationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRetinueFollowFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRetinueTag>(EMassFragmentPresence::All);
}

void URetinueFormationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmRetinueFormation, SwarmRetinueFormation);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm || !Swarm->NeedsFormationRepack() || !SwarmFormation::ReadParams().bCompact)
	{
		return;
	}

	// Compaction is a RANKING, not a reassignment: gather every live slot index, sort,
	// and each unit's new index is where its old one lands in that order. Nobody swaps
	// places with anybody — the survivors keep their relative positions and simply step
	// inward over the gaps, which is what closing ranks looks like. Doing it any other
	// way (say, handing out indices in chunk order) would reshuffle the whole army
	// every time one soldier died.
	TArray<int32> Live;
	Live.Reserve(FMath::Max(Swarm->GetAliveRetinue(), 64));

	EntityQuery.ForEachEntityChunk(Context, [&Live](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetFragmentView<FRetinueFollowFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			Live.Add(Follow[i].SlotIndex);
		}
	});

	if (Live.Num() == 0)
	{
		Swarm->MarkFormationPacked();
		return;
	}
	Live.Sort();

	EntityQuery.ForEachEntityChunk(Context, [&Live](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetMutableFragmentView<FRetinueFollowFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			Follow[i].SlotIndex = Algo::LowerBound(Live, Follow[i].SlotIndex);
		}
	});

	// Mark against the count the repack was DERIVED from, not the one we just produced,
	// so a death landing mid-pass is picked up next frame instead of being swallowed.
	Swarm->MarkFormationPacked();
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

	// Shape, spacing and bearing, read once per pass rather than per unit. Resolved here
	// rather than baked at spawn so every dial in SwarmFormation.h re-forms the standing
	// army the instant it moves.
	const SwarmFormation::FParams Formation = SwarmFormation::ReadParams();

	int32 BrokenThisFrame = 0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor, GlobalStance, StanceAnchor, Formation, &BrokenThisFrame](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetMutableFragmentView<FRetinueFollowFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const FVector2D Slot = SwarmFormation::SlotOffset(Follow[i].SlotIndex, Formation);
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
			const FVector Push = SeparationForce(*Swarm, Location,
					SwarmTuning::SeparationRadius, SwarmTuning::SeparationNeighborCap) * SwarmTuning::SeparationWeight;

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
	EntityQuery.AddRequirement<FSwarmStrikeFragment>(EMassFragmentAccess::ReadWrite);
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

	// Swing / hit-reaction tunables, snapshotted once per pass.
	const float SwingInterval = SwarmCombatTuning::SwingInterval();
	const float StrikeAt = SwingInterval * SwarmCombatTuning::SwingStrikeAt();
	const float Lunge = SwarmCombatTuning::SwingLunge();

	// The attack POSE window, not the whole interval. A single attack frame held for a
	// short beat either side of the blow reads as a jab — lean in, connect, snap back.
	// Holding it for the full interval would just look like a unit permanently leaning.
	const float PoseStart = StrikeAt * 0.5f;
	const float PoseEnd = FMath::Min(StrikeAt + SwingInterval * 0.18f, SwingInterval);

	// Knockback decays exponentially with time constant KnockbackTime, which is what
	// makes the total displacement exactly Speed x KnockbackTime (see the combat pass)
	// and lets repeated hits stack additively without any special-casing. One Exp per
	// pass, not per entity.
	const float KnockDecay = FMath::Exp(-DeltaTime / SwarmCombatTuning::KnockbackTime());

	// Walk rate is per team so the tide and the line don't step in lockstep. Picked off
	// TeamBit below, which the entity already carries — no extra query or branch on tags.
	const float BroodWalkHz = FMath::Max(CVarBroodWalkHz.GetValueOnAnyThread(), 0.f);

	// Facing dials, snapshotted once. The hero position is the same one the retinue
	// already anchors to, so the ring cannot drift from the formation it is standing in.
	FFacingParams Facing;
	Facing.MoveSpeedSq = FMath::Square(FMath::Max(CVarFacingMoveSpeed.GetValueOnAnyThread(), 0.f));
	Facing.Hysteresis = FMath::Max(CVarFacingTurnHysteresis.GetValueOnAnyThread(), 0.f);
	Facing.GlancePeriod = CVarGlancePeriod.GetValueOnAnyThread();
	Facing.GlanceDuration = CVarGlanceDuration.GetValueOnAnyThread();
	Facing.bGlanceEnabled = CVarGlanceEnabled.GetValueOnAnyThread() != 0;
	const FVector HeroLocation = Swarm->GetAttractor();

	EntityQuery.ForEachEntityChunk(Context, [Swarm, DeltaTime, TimeSeconds, SwingInterval, StrikeAt, PoseStart, PoseEnd, Lunge, KnockDecay, BroodWalkHz, Facing, HeroLocation](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FSwarmStrikeFragment> Strike = ChunkContext.GetMutableFragmentView<FSwarmStrikeFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			FTransform& Transform = Transforms[i].GetMutableTransform();
			const FVector Velocity = Velocities[i].Value;
			FSwarmStrikeFragment& S = Strike[i];

			// --- knockback ------------------------------------------------
			// Added to steering velocity here rather than written into it, because
			// steering OVERWRITES FMassVelocityFragment outright every frame — an
			// impulse stored there would be gone before it moved anything.
			const FVector Push(S.Impulse.X, S.Impulse.Y, 0.f);
			Transform.SetTranslation(Transform.GetTranslation() + (Velocity + Push) * DeltaTime);
			S.Impulse *= KnockDecay;

			// --- swing clock ----------------------------------------------
			// Advances only while something is in reach (AttackBit, set by the combat
			// pass and by retinue steering), so idle units don't swing at air and a
			// unit that just made contact starts its windup from where it left off.
			const bool bWasAttacking = (Anim[i].Bits & SwarmAnim::AttackBit) != 0;
			bool bStrike = false;
			bool bSwinging = false;

			if (bWasAttacking)
			{
				const float Previous = S.SwingTime;
				S.SwingTime += DeltaTime;

				// Edge-triggered: true on exactly the one frame the clock crosses the
				// strike point, so a blow lands once no matter the frame rate.
				bStrike = (Previous < StrikeAt && S.SwingTime >= StrikeAt);

				bSwinging = (S.SwingTime >= PoseStart && S.SwingTime < PoseEnd);

				if (S.SwingTime >= SwingInterval)
				{
					S.SwingTime = FMath::Fmod(S.SwingTime, SwingInterval);
				}
			}
			else
			{
				S.SwingTime = 0.f;
			}
			S.bStrikeFrame = bStrike;

			// --- flash ----------------------------------------------------
			S.FlashTime = FMath::Max(S.FlashTime - DeltaTime, 0.f);

			const float WalkHz = (Anim[i].Bits & SwarmAnim::TeamBit) ? SwarmTuning::WalkAnimHz : BroodWalkHz;
				UpdateAnimBits(Anim[i].Bits, Velocity, TimeSeconds, Jitter[i].Phase, bWasAttacking, WalkHz);

			// Owned here, so they have to survive UpdateAnimBits — see
			// SwarmAnim::PreservedBits.
			Anim[i].Bits = bSwinging ? (Anim[i].Bits | SwarmAnim::SwingBit)
									 : (Anim[i].Bits & ~SwarmAnim::SwingBit);
			Anim[i].Bits = (S.FlashTime > 0.f) ? (Anim[i].Bits | SwarmAnim::HitFlashBit)
											   : (Anim[i].Bits & ~SwarmAnim::HitFlashBit);

			// The lunge is applied to the PUBLISHED position, not the transform: it is
			// a pose, and the sim must not be able to reach further or shove a neighbour
			// because a unit happens to be mid-swing. So the render position leads the
			// true position by a few uu during the pose, and nothing else notices.
			// Deliberately keeps the renderers dumb — they have no idea what a unit is
			// fighting, and now they don't need to.
			FVector Published = Transform.GetTranslation();
			if (bSwinging)
			{
				Published += FVector(S.Facing.X, S.Facing.Y, 0.f) * Lunge;
			}

			// Facing is resolved from the TRUE velocity and the TRUE position, not the
			// published ones: the lunge is a pose offset, and letting it feed back into
			// the direction would make a unit appear to turn every time it swung.
			Anim[i].Facing = ResolveFacing(Anim[i].Facing, Transform.GetTranslation(),
				Velocity, HeroLocation, (Anim[i].Bits & SwarmAnim::TeamBit) != 0,
				Jitter[i].Phase, TimeSeconds, Facing);

			// Size roll rides along in the same int32. Derived from the jitter phase
				// rather than stored, so no fragment grows a field — see SwarmRenderPack.
				Swarm->PushRenderEntry(Published, Anim[i].Bits, Anim[i].SquadId,
					SwarmRenderPack::BucketFromPhase(Jitter[i].Phase), Anim[i].Facing);

			// Consume AttackBit: it is an observation made THIS frame by the steering
			// and combat passes, and it has to be re-observed next frame. Left set it
			// would latch permanently — UpdateAnimBits re-derives the bit from its own
			// previous value, so a unit that engaged once would read as attacking
			// forever. Harmless while nothing decoded the bit; not harmless now that it
			// gates the swing clock, which would leave veterans swinging at nothing.
			Anim[i].Bits &= ~SwarmAnim::AttackBit;
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
	const float ContactRangeSq = FMath::Square(FMath::Max(CVarBroodContactRange.GetValueOnAnyThread(), 0.f));
	int32 Contacts = 0;

	EntityQuery.ForEachEntityChunk(Context, [&Contacts, Attractor, ContactRangeSq](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			if (FVector::DistSquared2D(Transforms[i].GetTransform().GetLocation(), Attractor) < ContactRangeSq)
			{
				++Contacts;
			}
		}
	});

	Swarm->AddHeroContacts(Contacts);
}
