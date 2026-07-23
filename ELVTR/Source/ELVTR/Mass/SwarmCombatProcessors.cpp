#include "SwarmCombatProcessors.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "SwarmCombat.h"
#include "SwarmFragments.h"
#include "SwarmStats.h"
#include "SwarmSubsystem.h"

namespace
{
	/** A hero can only be mobbed by so many bodies before the rest are just queueing. */
	constexpr int32 MaxHeroAttackers = 8;
}

//----------------------------------------------------------------------
// Melee attrition
//----------------------------------------------------------------------
USwarmCombatProcessor::USwarmCombatProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmCombat"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmSteering")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmCombatProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmCombatProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmCombat, SwarmCombat);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	const float DeltaTime = Context.GetDeltaTimeSeconds();
	const FVector HeroLocation = Swarm->GetAttractor();
	const bool bHeroAlive = Swarm->IsHeroAlive();

	// Sequential across chunks (ForEachEntityChunk is single-threaded here), so a
	// plain counter is enough to cap how many brood can engage the hero at once.
	int32 HeroAttackers = 0;
	float HeroDamage = 0.f;

	// Balance telemetry: HP actually removed, bucketed by victim team. Local
	// accumulators merged once at the end, same shape as HeroDamage — so this
	// stays chunk-local and survives the move to ParallelForEachEntityChunk.
	double DamageToRetinue = 0.0;
	double DamageToBrood = 0.0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, DeltaTime, HeroLocation, bHeroAlive, &HeroAttackers, &HeroDamage, &DamageToRetinue, &DamageToBrood](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FSwarmHealthFragment> Health = ChunkContext.GetMutableFragmentView<FSwarmHealthFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const bool bRetinue = (Anim[i].Bits & SwarmAnim::TeamBit) != 0;

			int32 Attackers = 0;
			Swarm->QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
			{
				if (Entry.bRetinue == bRetinue || Attackers >= SwarmCombatTuning::MaxAttackersPerUnit)
				{
					return;
				}
				if (FVector::DistSquared2D(Entry.Location, Location) < SwarmCombatTuning::MeleeRangeSq)
				{
					++Attackers;
				}
			});

			// Incoming: whatever the other team's per-unit DPS is.
			const float IncomingDPS = bRetinue ? SwarmCombatTuning::BroodDPS : SwarmCombatTuning::RetinueDPS;
			float Damage = Attackers * IncomingDPS;

			// Brood also trade with the hero directly.
			if (!bRetinue && bHeroAlive
				&& FVector::DistSquared2D(Location, HeroLocation) < SwarmCombatTuning::HeroMeleeRangeSq)
			{
				Damage += SwarmCombatTuning::HeroDPS;
				++Attackers;

				if (HeroAttackers < MaxHeroAttackers)
				{
					++HeroAttackers;
					HeroDamage += SwarmCombatTuning::BroodDPS * DeltaTime;
				}
			}

			// Clamp the recorded amount to HP remaining: overkill is not throughput,
			// and counting it would make a lopsided fight look closer than it was.
			const float Applied = FMath::Min(Damage * DeltaTime, FMath::Max(Health[i].HP, 0.f));
			(bRetinue ? DamageToRetinue : DamageToBrood) += Applied;

			Health[i].HP -= Damage * DeltaTime;

			if (Attackers > 0)
			{
				Anim[i].Bits |= SwarmAnim::AttackBit;
			}
		}
	});

	if (HeroDamage > 0.f)
	{
		Swarm->AddPendingHeroDamage(HeroDamage);
	}

	Swarm->AddDamageDealt(DamageToRetinue, DamageToBrood);
}

//----------------------------------------------------------------------
// Death
//----------------------------------------------------------------------
USwarmDeathProcessor::USwarmDeathProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmDeath"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmIntegrate")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmDeathProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FSwarmHealthFragment>(EMassFragmentAccess::ReadOnly);
	// Read-only, purely so deaths can be bucketed by team for the fight log.
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmDeathProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmDeath, SwarmDeath);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();

	int32 KilledRetinue = 0;
	int32 KilledBrood = 0;

	EntityQuery.ForEachEntityChunk(Context, [&KilledRetinue, &KilledBrood](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FSwarmHealthFragment> Health = ChunkContext.GetFragmentView<FSwarmHealthFragment>();
		const TConstArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetFragmentView<FSwarmAnimFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			if (Health[i].HP <= 0.f)
			{
				((Anim[i].Bits & SwarmAnim::TeamBit) != 0 ? KilledRetinue : KilledBrood)++;
				ChunkContext.Defer().DestroyEntity(ChunkContext.GetEntity(i));
			}
		}
	});

	if (Swarm && (KilledRetinue | KilledBrood) != 0)
	{
		Swarm->AddKills(KilledRetinue, KilledBrood);
	}
}
