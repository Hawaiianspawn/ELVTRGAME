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
	// (MaxHeroAttackers, the old "a hero can only be mobbed by 8" counter, was removed
	// 2026-07-25. It capped the first 8 brood found in grid iteration order, and that set
	// churned every frame as the grid rebuilt, so it never bounded a damage RATE. The
	// geometric reach test replaced it — see FGridEntry::StrikeReachSq.)

	// Combat tuning, live via console. These back the SwarmCombatTuning getters
	// declared in SwarmCombat.h. Read once per processor pass (see Execute), never
	// per entity. Defaults match the original compile-time values.
	TAutoConsoleVariable<float> CVarRetinueMaxHP(
		TEXT("Swarm.RetinueMaxHP"), 130.f,
		TEXT("Max HP of a retinue soldier."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRetinueDPS(
		TEXT("Swarm.RetinueDPS"), 30.f,
		TEXT("Damage per second one retinue soldier deals to an adjacent brood."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodMaxHP(
		TEXT("Swarm.BroodMaxHP"), 60.f,
		TEXT("Max HP of a brood (enemy) unit."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodDPS(
		TEXT("Swarm.BroodDPS"), 35.f,
		TEXT("Damage per second one brood deals to the ONE retinue unit (or hero) it is\n")
		TEXT("committed to -- see Swarm.BroodTargetsPerHit.\n")
		TEXT("Raised from 14 on 2026-07-25. 14 was tuned against a model where every unit\n")
		TEXT("implicitly cleaved every enemy in range, so a brood was really dealing 14 to\n")
		TEXT("two or three soldiers at once. Now that a brood commits to a single target,\n")
		TEXT("14 made the horde harmless: zero-input runs took NO wave-1 losses. 35 restores\n")
		TEXT("the measured baseline shape (100-110 of 120 surviving wave 1, vs 97-103 in\n")
		TEXT("docs/GATE1-FUN-PROTOTYPE.md §3)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMeleeRange(
		TEXT("Swarm.MeleeRange"), 95.f,
		TEXT("Unit-to-unit melee reach in uu. Squared once per pass, not per entity."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarMaxAttackersPerUnit(
		TEXT("Swarm.MaxAttackersPerUnit"), 4,
		TEXT("How many enemies can meaningfully swarm one unit at once."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeroMaxHP(
		TEXT("Swarm.HeroMaxHP"), 500.f,
		TEXT("Max HP of the hero/bearer."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeroDPS(
		TEXT("Swarm.HeroDPS"), 55.f,
		TEXT("Damage per second the hero deals to brood within HeroMeleeRange."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeroMeleeRange(
		TEXT("Swarm.HeroMeleeRange"), 190.f,
		TEXT("Hero melee reach in uu."), ECVF_Default);

	// --- swing cadence + hit reaction ------------------------------------
	TAutoConsoleVariable<float> CVarSwingInterval(
		TEXT("Swarm.SwingInterval"), 0.9f,
		TEXT("Seconds between blows. One blow removes DPS * this, so raising it makes\n")
		TEXT("combat slower and heavier-hitting without changing average throughput.\n")
		TEXT("Very short intervals collapse back toward the old continuous bleed."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwingStrikeAt(
		TEXT("Swarm.SwingStrikeAt"), 0.35f,
		TEXT("Fraction of the swing interval at which the blow lands, 0-1. Everything\n")
		TEXT("before it is windup (the tell), everything after is recovery."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwingLunge(
		TEXT("Swarm.SwingLunge"), 12.f,
		TEXT("How far a unit leans toward its target as it swings, in uu. Purely\n")
		TEXT("cosmetic — the renderer offsets the sprite/box, the sim never moves."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitFlashTime(
		TEXT("Swarm.HitFlashTime"), 0.10f,
		TEXT("Seconds a struck unit flashes white. The flash ignores the flame's distance\n")
		TEXT("falloff on purpose, so a hit still reads at the edge of the pool."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKnockbackDistance(
		TEXT("Swarm.KnockbackDistance"), 35.f,
		TEXT("How far a struck unit is shoved, in uu. Sized against measured spacing\n")
		TEXT("(86uu at rest, ~45uu compressed, MeleeRange 95uu): big enough to see, small\n")
		TEXT("enough that the front line doesn't blow apart and the shoved unit can close\n")
		TEXT("again. 0 disables knockback entirely."), ECVF_Default);
	// --- cleave, per team (owner decision 2026-07-25) ---------------------
	// How many enemies one blow lands on: the attacker's K NEAREST inside MeleeRange.
	// Split per team because the ORIGINAL continuous model was implicitly infinite-cleave
	// (every unit dealt full DPS to every enemy in range at once, so a retinue with 4
	// brood on it did 120 DPS, not 30) while victim intake was capped at 4. That
	// asymmetry -- unbounded output, bounded intake -- is precisely why 120 retinue could
	// nearly beat 700 brood. A single shared K cannot reproduce it: raising K lifts both
	// teams equally. Giving the retinue the cleave and leaving the brood at one target
	// encodes the same asymmetry deliberately instead of accidentally.
	TAutoConsoleVariable<int32> CVarRetinueTargetsPerHit(
		TEXT("Swarm.RetinueTargetsPerHit"), 8,
		TEXT("How many brood one retinue blow lands on (its K nearest). The cleave powerup\n")
		TEXT("dial. Damage is conserved -- a striker delivers exactly K blows -- so this\n")
		TEXT("multiplies a soldier's output by K rather than spreading it.\n")
		TEXT("\n")
		TEXT("8 = 'swings at everyone adjacent' (MeleeRange rarely holds more), and that is\n")
		TEXT("deliberate, not a max-it-out. A CONSTANT K removes a stabiliser the old model\n")
		TEXT("had by accident: when output scales with how surrounded you are, a shrinking\n")
		TEXT("force hits proportionally harder, which is negative feedback and keeps runs\n")
		TEXT("landing near a knife edge. Cap it at a constant and attrition runs away, so\n")
		TEXT("fights go bimodal -- the line either holds easily or collapses. Measured\n")
		TEXT("2026-07-25: K=3 lost with ~220 brood left, K=4 WON outright with zero input,\n")
		TEXT("and one integer step should not do that. Clamped to 1-8."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarBroodTargetsPerHit(
		TEXT("Swarm.BroodTargetsPerHit"), 1,
		TEXT("How many retinue one brood blow lands on. 1 = a brood commits to one target,\n")
		TEXT("which is what makes numbers a positioning problem rather than raw throughput.\n")
		TEXT("Raise it for a cleaving elite. Clamped to 1-8."), ECVF_Default);

	TAutoConsoleVariable<float> CVarKnockbackTime(
		TEXT("Swarm.KnockbackTime"), 0.10f,
		TEXT("Seconds over which the shove is spent. Shorter = a sharper, more violent\n")
		TEXT("pop for the same distance; longer = a slide."), ECVF_Default);

	// --- typed units: Archers (docs/design/squad-group-system.md §1, §2, §4.1) ---------
	// Spearmen ARE the CVars above (RetinueMaxHP/RetinueDPS/MeleeRange/RetinueTargetsPerHit) —
	// unchanged names, unchanged shipped defaults, no churn on tuning the owner already set.
	TAutoConsoleVariable<float> CVarArchersMaxHP(
		TEXT("Swarm.ArchersMaxHP"), 70.f,
		TEXT("Max HP of an archer. Lower than a spearman's 130 -- a ranged unit trades\n")
		TEXT("durability for reach. UNMEASURED (docs/data/unit-types.json)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersDPS(
		TEXT("Swarm.ArchersDPS"), 18.f,
		TEXT("Damage/sec one archer deals to whatever it's engaging. UNMEASURED."), ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersEngageRange(
		TEXT("Swarm.ArchersEngageRange"), 750.f,
		TEXT("How far an archer's blow reaches, uu -- the ranged-combat model, squad-group-\n")
		TEXT("system.md §2.2: reuses the SAME grid/BlowsClaimed mechanism Spearmen/brood melee\n")
		TEXT("already use, just a bigger radius. Capped in practice by the 3x3 grid reach\n")
		TEXT("(750uu at USwarmSubsystem::GridCellSize 250, task-052) -- values beyond that read\n")
		TEXT("the same, exactly like Swarm.BroodAggroRange. [0..750]"), ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersMinEngageRange(
		TEXT("Swarm.ArchersMinEngageRange"), 150.f,
		TEXT("An archer won't engage anything closer than this to ITSELF, uu -- the cheap,\n")
		TEXT("local approximation of 'don't shoot into your own scrum' (§2.2). Not true line-\n")
		TEXT("of-sight against a specific ally (Design Law 5 rules that out at horde scale);\n")
		TEXT("just a band on the archer's own reach. Just past Swarm.MeleeRange (95). [0..750]"),
		ECVF_Default);
	TAutoConsoleVariable<int32> CVarArchersTargetsPerHit(
		TEXT("Swarm.ArchersTargetsPerHit"), 1,
		TEXT("How many enemies one archer blow lands on. 1 = precise single-target volleys,\n")
		TEXT("not free cleave -- a mass archer line hasn't earned Swarm.RetinueTargetsPerHit's\n")
		TEXT("cleave (8) through positioning the way Spearmen have. Clamped to 1-8."), ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersMoveSpeedScale(
		TEXT("Swarm.ArchersMoveSpeedScale"), 0.9f,
		TEXT("Archer march speed as a fraction of SwarmTuning::RetinueSpeed (450uu/s). Slightly\n")
		TEXT("slower than Spearmen -- a firing line doesn't need to close distance. [0..2]"),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarArcherGrowthWeight(
		TEXT("Swarm.ArcherGrowthWeight"), 0.2f,
		TEXT("Fraction of each new recruit rolled Archer rather than Spearman (docs/data/\n")
		TEXT("unit-types.json growth_source_weight). Spearmen claim the rest (0.8 default) --\n")
		TEXT("the class's primary identity, per CLASSES.md. v1 has no real growth-site system,\n")
		TEXT("so this stands in for a generator-tagged site (§1.4): every recruit rolls\n")
		TEXT("independently against this weight. [0..1]"), ECVF_Default);
}

namespace SwarmCombatTuning
{
	float RetinueMaxHP()        { return CVarRetinueMaxHP.GetValueOnAnyThread(); }
	float RetinueDPS()          { return CVarRetinueDPS.GetValueOnAnyThread(); }
	float BroodMaxHP()          { return CVarBroodMaxHP.GetValueOnAnyThread(); }
	float BroodDPS()            { return CVarBroodDPS.GetValueOnAnyThread(); }
	float MeleeRange()          { return CVarMeleeRange.GetValueOnAnyThread(); }
	int32 MaxAttackersPerUnit() { return CVarMaxAttackersPerUnit.GetValueOnAnyThread(); }
	float HeroMaxHP()           { return CVarHeroMaxHP.GetValueOnAnyThread(); }
	float HeroDPS()             { return CVarHeroDPS.GetValueOnAnyThread(); }
	float HeroMeleeRange()      { return CVarHeroMeleeRange.GetValueOnAnyThread(); }

	// Floored so a zero or negative interval can't produce a divide-by-zero or a
	// unit that strikes every frame. StrikeAt is clamped inside the interval so the
	// windup and the recovery both always exist.
	float SwingInterval()       { return FMath::Max(CVarSwingInterval.GetValueOnAnyThread(), 0.05f); }
	float SwingStrikeAt()       { return FMath::Clamp(CVarSwingStrikeAt.GetValueOnAnyThread(), 0.f, 0.99f); }
	float SwingLunge()          { return CVarSwingLunge.GetValueOnAnyThread(); }
	float HitFlashTime()        { return FMath::Max(CVarHitFlashTime.GetValueOnAnyThread(), 0.f); }
	float KnockbackDistance()   { return FMath::Max(CVarKnockbackDistance.GetValueOnAnyThread(), 0.f); }
	float KnockbackTime()       { return FMath::Max(CVarKnockbackTime.GetValueOnAnyThread(), 0.01f); }
	int32 RetinueTargetsPerHit() { return FMath::Clamp(CVarRetinueTargetsPerHit.GetValueOnAnyThread(), 1, 8); }
	int32 BroodTargetsPerHit()   { return FMath::Clamp(CVarBroodTargetsPerHit.GetValueOnAnyThread(), 1, 8); }

	float ArchersMaxHP()         { return CVarArchersMaxHP.GetValueOnAnyThread(); }
	float ArchersDPS()           { return CVarArchersDPS.GetValueOnAnyThread(); }
	float ArchersEngageRange()   { return FMath::Max(CVarArchersEngageRange.GetValueOnAnyThread(), 0.f); }
	float ArchersMinEngageRange(){ return FMath::Max(CVarArchersMinEngageRange.GetValueOnAnyThread(), 0.f); }
	int32 ArchersTargetsPerHit() { return FMath::Clamp(CVarArchersTargetsPerHit.GetValueOnAnyThread(), 1, 8); }
	float ArchersMoveSpeedScale(){ return FMath::Max(CVarArchersMoveSpeedScale.GetValueOnAnyThread(), 0.f); }
	float ArcherGrowthWeight()   { return FMath::Clamp(CVarArcherGrowthWeight.GetValueOnAnyThread(), 0.f, 1.f); }
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
	// Reads this unit's own bStrikeFrame (for the hero exchange) and writes its hit
	// reaction — flash timer and knockback impulse. Still only ever writing to self.
	EntityQuery.AddRequirement<FSwarmStrikeFragment>(EMassFragmentAccess::ReadWrite);
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

	// No DeltaTime here any more: damage is no longer a rate integrated per tick, it
	// is a discrete blow applied on the frame it lands.
	const FVector HeroLocation = Swarm->GetAttractor();
	const bool bHeroAlive = Swarm->IsHeroAlive();
	const bool bHeroStriking = Swarm->IsHeroStriking();

	// Snapshot the tunables once per pass so the per-entity loop reads plain
	// locals, not CVars. Ranges are squared here, not inside the loop.
	//
	// Archers read their OWN candidate band instead of the shared MeleeRangeSq (docs/
	// design/squad-group-system.md §2.2's minimum-viable ranged model — same grid, same
	// BlowsClaimed mechanism, just a much bigger radius, plus a MinEngageRange floor so an
	// archer won't count anything already in its own melee-range scrum as a target).
	// Spearmen and brood are UNCHANGED — both still read the shared MeleeRangeSq exactly
	// as before.
	const float MeleeRangeSq = FMath::Square(SwarmCombatTuning::MeleeRange());
	const float ArchersRangeSq = FMath::Square(SwarmCombatTuning::ArchersEngageRange());
	const float ArchersMinRangeSq = FMath::Square(SwarmCombatTuning::ArchersMinEngageRange());
	const float HeroMeleeRangeSq = FMath::Square(SwarmCombatTuning::HeroMeleeRange());
	const int32 MaxAttackers = SwarmCombatTuning::MaxAttackersPerUnit();
	const int32 RetinueTargets = SwarmCombatTuning::RetinueTargetsPerHit(); // Spearmen's K
	const int32 ArchersTargets = SwarmCombatTuning::ArchersTargetsPerHit();
	const int32 BroodTargets = SwarmCombatTuning::BroodTargetsPerHit();

	// Damage is now parcelled into blows: one blow removes a whole interval's worth
	// of DPS at once. Average throughput over time is identical to the old per-tick
	// bleed, which is what keeps the Gate 1 balance numbers meaningful — but the HP
	// now comes off in steps you can see, and each step is something to react to.
	//
	// BroodBlow survives as a flat value (brood aren't typed — §11 Assumption 7) for the
	// hero-exchange path below. The damage a RETINUE striker deals no longer has one flat
	// per-team value — see FGridEntry::BlowDamage, published per-attacker at grid-build
	// time from its own type, and accumulated per-claim in the loop below instead.
	const float SwingInterval = SwarmCombatTuning::SwingInterval();
	const float BroodBlow = SwarmCombatTuning::BroodDPS() * SwingInterval;
	const float HeroBlow = SwarmCombatTuning::HeroDPS() * SwingInterval;

	const float FlashTime = SwarmCombatTuning::HitFlashTime();
	// Impulse speed that spends KnockbackDistance over KnockbackTime under the
	// exponential decay applied in the integrate pass: displacement = V * tau.
	const float KnockSpeed = SwarmCombatTuning::KnockbackDistance() / SwarmCombatTuning::KnockbackTime();

	float HeroDamage = 0.f;

	// Balance telemetry: HP actually removed, bucketed by victim team. Local
	// accumulators merged once at the end, same shape as HeroDamage — so this
	// stays chunk-local and survives the move to ParallelForEachEntityChunk.
	double DamageToRetinue = 0.0;
	double DamageToBrood = 0.0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, HeroLocation, bHeroAlive, bHeroStriking, MeleeRangeSq, ArchersRangeSq, ArchersMinRangeSq, HeroMeleeRangeSq, BroodBlow, HeroBlow, MaxAttackers, RetinueTargets, ArchersTargets, BroodTargets, FlashTime, KnockSpeed, &HeroDamage, &DamageToRetinue, &DamageToBrood](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FSwarmHealthFragment> Health = ChunkContext.GetMutableFragmentView<FSwarmHealthFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();
		const TArrayView<FSwarmStrikeFragment> Strike = ChunkContext.GetMutableFragmentView<FSwarmStrikeFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const bool bRetinue = (Anim[i].Bits & SwarmAnim::TeamBit) != 0;
			const bool bArcher = bRetinue && SwarmSquad::UnitType(Anim[i].SquadId) == EUnitType::Archers;

			// MY OWN candidate band this frame — Archers reach much further (§2.2) and
			// won't count anything already inside their own MinEngageRange as a candidate.
			// Spearmen and brood keep the exact shared MeleeRangeSq, unchanged.
			const float MyRangeSq = bArcher ? ArchersRangeSq : MeleeRangeSq;
			const float MyMinRangeSq = bArcher ? ArchersMinRangeSq : 0.f;

			// One walk of the neighbourhood does three jobs:
			//   bContact  — is any enemy in reach at all? Gates whether this unit's own
			//               swing clock advances, so nobody swings at empty air.
			//   Strikers  — blows landing on ME this frame. A neighbour's blow lands on
			//               me only if I am inside its StrikeReachSq AND its shared
			//               BlowsClaimed counter hasn't already paid out its K blows to
			//               other victims this frame (see FGridEntry::BlowsClaimed). The
			//               radius alone is only a candidate test — it's computed from
			//               last frame's positions, so this frame's live positions can
			//               put more or fewer than K candidates inside it; BlowsClaimed
			//               is what actually keeps a striker's output at exactly K.
			//   NearestSq — my own K nearest enemy distances, which become the reach I
			//               publish for my next blow. K is small (1-8), so an insertion
			//               sort over a fixed array is cheaper than anything smarter.
			bool bContact = false;
			int32 Strikers = 0;
			FVector2f Shove = FVector2f::ZeroVector;
			FVector2f Toward = FVector2f::ZeroVector;

			// Reach published LAST frame is the one the grid handed out, so it is the one
			// my blow uses this frame. Read it before it gets overwritten below.
			const float MyReachSq = Strike[i].StrikeReachSq;

			// Cleave is per team, and per type within retinue: Spearmen cleave, Archers hit
			// one precise target, brood commit to one.
			const int32 MyTargets = bRetinue ? (bArcher ? ArchersTargets : RetinueTargets) : BroodTargets;

			float NearestSq[8];
			for (int32 k = 0; k < MyTargets; ++k)
			{
				NearestSq[k] = TNumericLimits<float>::Max();
			}
			int32 InReach = 0;

			// Accumulates each successful striker's OWN blow value (FGridEntry::BlowDamage)
			// rather than counting strikers and multiplying by one shared team blow — see
			// the doc comment above this pass's tunable snapshot for why: retinue strikers
			// aren't all one type any more.
			float Damage = 0.f;

			Swarm->QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
			{
				if (Entry.bRetinue == bRetinue)
				{
					return;
				}
				const float DistSq = (float)FVector::DistSquared2D(Entry.Location, Location);
				if (DistSq >= MyRangeSq || DistSq < MyMinRangeSq)
				{
					return;
				}
				bContact = true;
				++InReach;

				// Which way this unit is fighting — needed by the swing lunge, and free
				// here. Every enemy in reach votes.
				Toward += FVector2f(
					(float)(Entry.Location.X - Location.X),
					(float)(Entry.Location.Y - Location.Y)).GetSafeNormal();

				// Keep the K smallest distances (insertion into a tiny sorted array).
				for (int32 k = 0; k < MyTargets; ++k)
				{
					if (DistSq < NearestSq[k])
					{
						for (int32 s = MyTargets - 1; s > k; --s)
						{
							NearestSq[s] = NearestSq[s - 1];
						}
						NearestSq[k] = DistSq;
						break;
					}
				}

				// Am I one of this striker's K nearest? StrikeReachSq alone only says
				// I'm a CANDIDATE (see FGridEntry::StrikeReachSq for why a stale radius
				// can admit more or fewer than K by the time anyone tests it); BlowsClaimed
				// is the actual cap — it's shared across every victim that queries this
				// same entry this frame, so only the first K claimants succeed no matter
				// how many candidates the radius currently contains.
				if (Entry.bStriking && DistSq <= Entry.StrikeReachSq
					&& Entry.BlowsClaimed < Entry.TargetsPerHit && Strikers < MaxAttackers)
				{
					++Entry.BlowsClaimed;
					++Strikers;
					// This striker's OWN blow value — a Spearman and an Archer striking the
					// same victim this frame no longer deal the same damage (see above).
					Damage += Entry.BlowDamage;
					// Away from whoever hit you. Summed as unit vectors then normalised
					// once, so being hit from two sides at once mostly cancels rather
					// than launching the victim twice as far.
					const FVector2f Away(
						(float)(Location.X - Entry.Location.X),
						(float)(Location.Y - Entry.Location.Y));
					Shove += Away.GetSafeNormal();
				}
			});

			bool bStruck = Strikers > 0;

			// Brood also trade with the hero directly, on the hero's own cadence.
			if (!bRetinue && bHeroAlive)
			{
				const float HeroDistSq = (float)FVector::DistSquared2D(Location, HeroLocation);
				if (HeroDistSq < HeroMeleeRangeSq)
				{
					bContact = true;
					Toward += FVector2f(
						(float)(HeroLocation.X - Location.X),
						(float)(HeroLocation.Y - Location.Y)).GetSafeNormal();

					// The hero cleaves everything in his reach, which is exactly what the
					// old continuous model did (every brood in range took HeroDPS), so his
					// output per brood is unchanged.
					if (bHeroStriking)
					{
						Damage += HeroBlow;
						bStruck = true;
						const FVector2f Away(
							(float)(Location.X - HeroLocation.X),
							(float)(Location.Y - HeroLocation.Y));
						Shove += Away.GetSafeNormal();
					}

					// This brood's blow lands on the hero only if the hero is one of its K
					// nearest — the same geometric test every other victim gets. The old
					// MaxHeroAttackers counter is gone from this path: it had the identical
					// churn flaw, so "the hero can only be mobbed by 8" was never true as a
					// rate. Now the crowd bounds itself, because a brood mobbing the hero
					// has the hero as its nearest enemy and cannot also be hitting someone
					// else.
					//
					// This still has to go through the SAME BlowsClaimed budget a retinue
					// victim would draw from when it finds this brood via QueryNeighbors —
					// otherwise this brood's one swing could pay the hero AND a retinue
					// victim out of what should be a single K=1 blow (fixed 2026-07-26: was
					// the concrete case of the StrikeReachSq staleness bug — a brood next
					// to both the hero and a soldier could hit both). FindOwnGridEntry does
					// the self-lookup since this check runs on the attacker's own iteration,
					// not via a QueryNeighbors visit that would hand us a shared entry.
					if (Strike[i].bStrikeFrame && HeroDistSq <= MyReachSq)
					{
						USwarmSubsystem::FGridEntry* OwnEntry =
							Swarm->FindOwnGridEntry(Location, /*bRetinue=*/false, Strike[i].bStrikeFrame, MyReachSq);
						if (OwnEntry && OwnEntry->BlowsClaimed < OwnEntry->TargetsPerHit)
						{
							++OwnEntry->BlowsClaimed;
							HeroDamage += BroodBlow;
						}
					}

					// The hero counts as a candidate target for THIS brood's next blow.
					for (int32 k = 0; k < MyTargets; ++k)
					{
						if (HeroDistSq < NearestSq[k])
						{
							for (int32 s = MyTargets - 1; s > k; --s)
							{
								NearestSq[s] = NearestSq[s - 1];
							}
							NearestSq[k] = HeroDistSq;
							++InReach;
							break;
						}
					}
				}
			}

			Strike[i].Facing = Toward.GetSafeNormal();

			// Publish my reach for the next frame: the distance to my Kth nearest enemy,
			// or to my farthest if I have fewer than K in range (so a unit with 2 enemies
			// and K=4 still hits both). Zero when nothing is in range, so an idle unit's
			// blow lands on nobody.
			Strike[i].StrikeReachSq = (InReach <= 0)
				? 0.f
				: NearestSq[FMath::Min(InReach, MyTargets) - 1];

			if (bStruck)
			{
				Strike[i].FlashTime = FlashTime;
				Strike[i].Impulse += Shove.GetSafeNormal() * KnockSpeed;

				// Clamp the recorded amount to HP remaining: overkill is not throughput,
				// and counting it would make a lopsided fight look closer than it was.
				const float Applied = FMath::Min(Damage, FMath::Max(Health[i].HP, 0.f));
				(bRetinue ? DamageToRetinue : DamageToBrood) += Applied;

				Health[i].HP -= Damage;
			}

			if (bContact)
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
