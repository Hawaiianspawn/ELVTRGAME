#include "SwarmCombatProcessors.h"

#include "HAL/IConsoleManager.h"
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
	TAutoConsoleVariable<float> CVarArcherSwingInterval(
		TEXT("Swarm.ArcherSwingInterval"), 1.5f,
		TEXT("Seconds between ARCHER shots, the ranged counterpart to Swarm.SwingInterval.\n")
		TEXT("A bow is not a spear jab: nocking, drawing and loosing is a longer, heavier\n")
		TEXT("beat than a melee line's, and until this existed archers borrowed the melee\n")
		TEXT("cadence and the whole ranged rank read as a single firing machine.\n")
		TEXT("DPS IS UNCHANGED BY THIS: one arrow removes Swarm.ArchersDPS * this, so a\n")
		TEXT("slower rate simply lands fewer, bigger hits. It is a FEEL dial, not balance."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarSwingIntervalJitter(
		TEXT("Swarm.SwingIntervalJitter"), 0.2f,
		TEXT("Per-unit spread on the swing/fire interval, as a fraction of the type's base.\n")
		TEXT("0.2 means each unit's own cadence sits somewhere in +/-20%% of its type's,\n")
		TEXT("fixed for its lifetime and derived from its spawn phase (no stored state).\n")
		TEXT("This is what stops a rank that engaged on the same frame firing in lockstep\n")
		TEXT("forever. 0 = every unit of a type shares one metronome (the old behaviour).\n")
		TEXT("DPS IS UNCHANGED: each unit's blow is its OWN DPS * its OWN interval."),
		ECVF_Default);
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
		TEXT("An archer PREFERS not to engage anything closer than this to ITSELF, uu -- the\n")
		TEXT("cheap, local approximation of 'don't shoot into your own scrum' (§2.2). Not true\n")
		TEXT("line-of-sight against a specific ally (Design Law 5 rules that out at horde\n")
		TEXT("scale); just a band on the archer's own reach. Just past Swarm.MeleeRange (95).\n")
		TEXT("\n")
		TEXT("'Prefers', not refuses (task-073): this band sits past Swarm.MeleeRange, so a lone\n")
		TEXT("brood inside it used to be unreachable by anyone and froze wave-clear outright\n")
		TEXT("(measured: 7 brood held at an unchanged count for 135+s). The processor now falls\n")
		TEXT("back to the nearest dead-zone target ONLY when the [this, EngageRange) band is\n")
		TEXT("empty — see its escape-hatch comment. [0..750]"),
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
	/**
	 * task-130 rung 3: BALANCE CHANGE, not a render dial. task-127 measured that 20% split
	 * six ways over Swarm.ArcherVariantWeights puts any one archer look at ~3% of the army
	 * (task-128 has since made that thirteen ways, so any one look is ~1.2% -- the spread
	 * argument below is unchanged, only the arithmetic moved) --
	 * too thin, on its own, to read as a line even with task-127's size bump and task-130's
	 * rung 2 colour lift, both of which change how ONE archer looks, not how MANY there are.
	 * 0.4 roughly doubles archer presence so archers stop being a rare find in the mass;
	 * Spearmen still claim the majority (0.6), preserving their CLASSES.md primary-identity
	 * role. This is a real gameplay change: Archers carry Swarm.ArchersMaxHP 70 against a
	 * Spearman's 130 and fight at range instead of melee cleave, so a bigger archer share
	 * measurably softens the line's HP total and reshapes how a wave is fought, not just how
	 * it looks. Flagged for the owner rather than treated as a free render-only lever.
	 */
	TAutoConsoleVariable<float> CVarArcherGrowthWeight(
		TEXT("Swarm.ArcherGrowthWeight"), 0.4f,
		TEXT("Fraction of each new recruit rolled Archer rather than Spearman (docs/data/\n")
		TEXT("unit-types.json growth_source_weight). Spearmen claim the rest (0.6 default) --\n")
		TEXT("still the class's primary identity per CLASSES.md, but archers are no longer a\n")
		TEXT("rare find (task-130 rung 3, up from the task-095-era 0.2 -- BALANCE CHANGE, see\n")
		TEXT("the comment above). v1 has no real growth-site system, so this stands in for a\n")
		TEXT("generator-tagged site (§1.4): every recruit rolls independently against this\n")
		TEXT("weight. [0..1]"), ECVF_Default);

	// --- knight sub-types (task-095) --------------------------------------------------
	// Nine stat rows backing the eleven team-atlas looks (docs/data/art/team-variants.json)
	// -- two pairs share a row because task-094 measured them as indistinguishable. Row
	// order: retinue_base, heavycloak, shieldbreak, maceraised, lanceout, bracedstaff,
	// line_standard, simplecolumn, line_light (docs/data/unit-types.json
	// types.spearmen.melee_subtypes.rows) -- Swarm.KnightSubtypeMap is what ties a variant
	// index to a row in this order, so it is NOT the identity list.
	TAutoConsoleVariable<FString> CVarKnightSubtypeMap(
		TEXT("Swarm.KnightSubtypeMap"), TEXT("0,8,4,2,6,7,8,1,5,6,3"),
		TEXT("Team-atlas variant index (0-10, docs/data/art/team-variants.json) -> row index\n")
		TEXT("into the KnightSubtype* tables below, comma-separated integers, one per variant.\n")
		TEXT("Reordering team-variants.json without updating this list swaps every knight's\n")
		TEXT("stats onto a different skin -- see docs/perf/knight-subtype-binding.md."),
		ECVF_Default);
	TAutoConsoleVariable<FString> CVarKnightSubtypeHP(
		TEXT("Swarm.KnightSubtypeHP"), TEXT("130,165,140,146,133,123,124,119,114"),
		TEXT("Max HP per knight sub-type row (Swarm.KnightSubtypeMap has the row order),\n")
		TEXT("comma-separated floats. Baked into a soldier's FSwarmHealthFragment.MaxHP once,\n")
		TEXT("at spawn (SwarmSpawn.cpp), from the exact look-roll spawn already makes -- unlike\n")
		TEXT("DPS/Engage/Targets below, NOT re-read every pass, because HP is a running total\n")
		TEXT("combat decrements, not a per-frame lookup. docs/data/unit-types.json is the spec\n")
		TEXT("this transcribes."), ECVF_Default);
	TAutoConsoleVariable<FString> CVarKnightSubtypeDPS(
		TEXT("Swarm.KnightSubtypeDPS"), TEXT("30,30,36,37,30,33,28,28,25"),
		TEXT("Damage/sec per knight sub-type row, read live every pass like Swarm.RetinueDPS --\n")
		TEXT("dragging one entry visibly changes only the look(s) mapped to that row. See\n")
		TEXT("Swarm.KnightSubtypeMap for the row order."), ECVF_Default);
	TAutoConsoleVariable<FString> CVarKnightSubtypeEngage(
		TEXT("Swarm.KnightSubtypeEngage"), TEXT("95,100,99,93,105,110,96,82,85"),
		TEXT("Melee engage range, uu, per knight sub-type row -- replaces the shared\n")
		TEXT("Swarm.MeleeRange for Spearmen only (brood and Archers unaffected). See\n")
		TEXT("Swarm.KnightSubtypeMap for the row order."), ECVF_Default);
	TAutoConsoleVariable<FString> CVarKnightSubtypeTargets(
		TEXT("Swarm.KnightSubtypeTargets"), TEXT("8,9,8,8,7,6,8,10,8"),
		TEXT("Cleave (targets per blow) per knight sub-type row -- replaces the shared\n")
		TEXT("Swarm.RetinueTargetsPerHit for Spearmen only. Clamped to 1-8 on read: the combat\n")
		TEXT("loop's own nearest-K arrays are fixed at 8 (Swarm.RetinueTargetsPerHit's own\n")
		TEXT("comment), so simplecolumn's spec value of 10 reads as 8 in play -- a mechanical\n")
		TEXT("ceiling, not a rebalance. See Swarm.KnightSubtypeMap for the row order."),
		ECVF_Default);

	int32 ParseFloatCsv(const FString& Csv, float* Out, int32 MaxNum)
	{
		TArray<FString> Parts;
		Csv.ParseIntoArray(Parts, TEXT(","), false);
		int32 Num = 0;
		for (const FString& Part : Parts)
		{
			if (Num >= MaxNum) { break; }
			Out[Num++] = FCString::Atof(*Part.TrimStartAndEnd());
		}
		return Num;
	}

	int32 ParseIntCsv(const FString& Csv, int32* Out, int32 MaxNum)
	{
		TArray<FString> Parts;
		Csv.ParseIntoArray(Parts, TEXT(","), false);
		int32 Num = 0;
		for (const FString& Part : Parts)
		{
			if (Num >= MaxNum) { break; }
			Out[Num++] = FCString::Atoi(*Part.TrimStartAndEnd());
		}
		return Num;
	}
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
	float ArcherSwingInterval() { return FMath::Max(CVarArcherSwingInterval.GetValueOnAnyThread(), 0.05f); }
	float SwingIntervalJitter() { return FMath::Clamp(CVarSwingIntervalJitter.GetValueOnAnyThread(), 0.f, 0.9f); }
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

	FKnightSubtypeTables GetKnightSubtypeTables()
	{
		FKnightSubtypeTables T;
		T.NumRows = ParseFloatCsv(CVarKnightSubtypeHP.GetValueOnAnyThread(), T.HP, MaxKnightSubtypeRows);
		ParseFloatCsv(CVarKnightSubtypeDPS.GetValueOnAnyThread(), T.DPS, MaxKnightSubtypeRows);
		ParseFloatCsv(CVarKnightSubtypeEngage.GetValueOnAnyThread(), T.Engage, MaxKnightSubtypeRows);
		ParseIntCsv(CVarKnightSubtypeTargets.GetValueOnAnyThread(), T.Targets, MaxKnightSubtypeRows);
		for (int32 i = 0; i < T.NumRows; ++i)
		{
			// Mechanical ceiling, not a balance clamp -- see Swarm.KnightSubtypeTargets'
			// own comment: the combat loop's nearest-K scratch arrays are fixed at 8.
			T.Targets[i] = FMath::Clamp(T.Targets[i], 1, 8);
		}
		T.NumVariants = ParseIntCsv(CVarKnightSubtypeMap.GetValueOnAnyThread(), T.Map, MaxKnightSubtypeVariants);

		// The SAME cumulative Swarm.TeamVariantWeights table the render bridge resolves a
		// look from. That CVar is owned by SwarmProcessors.cpp's translation unit; read by
		// name here rather than redeclared, same idiom SwarmRenderActor.cpp's
		// LogVariantHistogram already uses to log it alongside a variant histogram.
		const IConsoleVariable* WeightsVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Swarm.TeamVariantWeights"));
		TArray<FString> WeightParts;
		(WeightsVar ? WeightsVar->GetString() : FString()).ParseIntoArray(WeightParts, TEXT(","), false);
		int32 Running = 0;
		for (const FString& Part : WeightParts)
		{
			if (T.NumTeamVariants >= MaxKnightSubtypeVariants) { break; }
			Running += FMath::Max(FCString::Atoi(*Part.TrimStartAndEnd()), 0);
			T.TeamVariantCum[T.NumTeamVariants++] = Running;
		}
		return T;
	}

	int32 KnightSubtypeRowFor(const FKnightSubtypeTables& Tables, int32 VariantIndex)
	{
		if (Tables.NumVariants <= 0 || Tables.NumRows <= 0)
		{
			return 0;
		}
		const int32 V = FMath::Clamp(VariantIndex, 0, Tables.NumVariants - 1);
		return FMath::Clamp(Tables.Map[V], 0, Tables.NumRows - 1);
	}
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
	// Read-only, already exists on every swarm entity (SwarmSpawn.cpp) — no class-layout
	// change. A Spearman's own walk-cycle phase is what resolves which knight sub-type row
	// it fights on (task-095), the same phase the render bridge already uses to pick its
	// look, so the two can never disagree.
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
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
	// archer PREFERS not to count anything already in its own melee-range scrum as a
	// target — see the per-entity loop below for the task-073 escape hatch that lets an
	// archer take a dead-zone target anyway when it is the ONLY target it can see).
	// Brood are UNCHANGED — still the shared MeleeRangeSq. Spearmen no longer are: their
	// range and K now come from their own knight sub-type row (task-095) — see KnightTables
	// below and the per-entity lookup in the loop. MeleeRangeSq survives as brood's own
	// flat range.
	const float MeleeRangeSq = FMath::Square(SwarmCombatTuning::MeleeRange());
	const float ArchersRangeSq = FMath::Square(SwarmCombatTuning::ArchersEngageRange());
	const float ArchersMinRangeSq = FMath::Square(SwarmCombatTuning::ArchersMinEngageRange());
	const float HeroMeleeRangeSq = FMath::Square(SwarmCombatTuning::HeroMeleeRange());
	const int32 MaxAttackers = SwarmCombatTuning::MaxAttackersPerUnit();
	const int32 ArchersTargets = SwarmCombatTuning::ArchersTargetsPerHit();
	const int32 BroodTargets = SwarmCombatTuning::BroodTargetsPerHit();

	// Snapshotted once per pass, same as everything above — see FKnightSubtypeTables'
	// own doc comment (SwarmCombat.h) for why this is safe next to a 30k-entity sim.
	const SwarmCombatTuning::FKnightSubtypeTables KnightTables = SwarmCombatTuning::GetKnightSubtypeTables();

	// Damage is now parcelled into blows: one blow removes a whole interval's worth
	// of DPS at once. Average throughput over time is identical to the old per-tick
	// bleed, which is what keeps the Gate 1 balance numbers meaningful — but the HP
	// now comes off in steps you can see, and each step is something to react to.
	//
	// NOBODY in the swarm has a flat blow value any more. Type gave retinue strikers their
	// own (FGridEntry::BlowDamage, published per-attacker at grid-build time), and per-unit
	// swing cadence (SwarmCombatTuning::SwingIntervalFor) now does the same to brood — so
	// the hero-exchange path reads the attacker's published BlowDamage too rather than a
	// team constant. HeroBlow stays flat because the hero is a pawn, not a Mass entity: he
	// has no jitter phase to spread a cadence with and swings on the base interval.
	const float SwingInterval = SwarmCombatTuning::SwingInterval();
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

	// Squad kill attribution (docs/ui/end-of-wave-showcase.md §5.3), same chunk-local
	// shape as the two accumulators above: brood killed this pass, bucketed by the unit
	// that landed the first claimed blow on each victim, plus the hero's own tally.
	// Merged into the subsystem once at the end.
	int32 KilledBySquad[USwarmSubsystem::MaxSquads] = {};
	int32 HeroKilled = 0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, HeroLocation, bHeroAlive, bHeroStriking, MeleeRangeSq, ArchersRangeSq, ArchersMinRangeSq, HeroMeleeRangeSq, HeroBlow, MaxAttackers, ArchersTargets, BroodTargets, KnightTables, FlashTime, KnockSpeed, &HeroDamage, &DamageToRetinue, &DamageToBrood, &KilledBySquad, &HeroKilled](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TArrayView<FSwarmHealthFragment> Health = ChunkContext.GetMutableFragmentView<FSwarmHealthFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();
		const TArrayView<FSwarmStrikeFragment> Strike = ChunkContext.GetMutableFragmentView<FSwarmStrikeFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const bool bRetinue = (Anim[i].Bits & SwarmAnim::TeamBit) != 0;
			const bool bArcher = bRetinue && SwarmSquad::UnitType(Anim[i].SquadId) == EUnitType::Archers;
			const bool bKnight = bRetinue && !bArcher; // a Spearman -- task-095 sub-type binding applies

			// Which stat row THIS Spearman fights on, from the SAME phase + live weights
			// table the render bridge resolves its look from (SwarmProcessors.cpp) -- so
			// this can never disagree with the sprite on screen. Unused (0) for archers/brood.
			const int32 MyKnightRow = bKnight
				? SwarmCombatTuning::KnightSubtypeRowFor(KnightTables, SwarmRenderPack::VariantFromPhase(
					Jitter[i].Phase, KnightTables.TeamVariantCum, KnightTables.NumTeamVariants))
				: 0;

			// MY OWN candidate band this frame — Archers reach much further (§2.2) and
			// won't count anything already inside their own MinEngageRange as a candidate.
			// A Spearman's range now comes from its own knight sub-type row (task-095);
			// brood keep the shared MeleeRangeSq, unchanged.
			const float MyRangeSq = bArcher ? ArchersRangeSq : (bKnight ? FMath::Square(KnightTables.Engage[MyKnightRow]) : MeleeRangeSq);
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

			// Cleave is per team, and per type within retinue: Spearmen cleave (per their own
			// knight sub-type row, task-095), Archers hit one precise target, brood commit
			// to one.
			const int32 MyTargets = bRetinue ? (bArcher ? ArchersTargets : KnightTables.Targets[MyKnightRow]) : BroodTargets;

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

			// Who gets the kill if this frame's Damage finishes me: the FIRST attacker to
			// claim a blow on me, by iteration order — squad byte if that was a soldier,
			// the hero flag if the hero's blow got here first (his branch runs after the
			// neighbour walk below, so a soldier always wins a same-frame tie). Exactly one
			// claimant is credited; see USwarmSubsystem::CreditKills for why the model
			// doesn't do proportional-damage credit.
			int32 CreditSquad = INDEX_NONE;
			bool bCreditHero = false;

			// task-073 escape hatch. MyMinRangeSq is 0 for everyone except Archers, so this
			// whole shadow scan is a no-op (bDeadZoneContact can never go true) for Spearmen
			// and brood — nothing below changes their behaviour.
			//
			// For an Archer, MyMinRangeSq excludes anything closer than Swarm.
			// ArchersMinEngageRange as "too close, don't shoot into your own scrum" (§2.2).
			// But a brood standing that close is ALSO too close for anyone else to reach —
			// Swarm.MeleeRange (95uu) is short of MinEngageRange's default (150uu) — so if
			// it is this archer's ONLY candidate, refusing it forever freezes the wave
			// (measured task-064: 7 brood held at an exact unchanged count for 135+s).
			// Track dead-zone candidates in a separate shadow set and only promote them
			// after the scan if the normal in-band set came up completely empty — a real
			// scrum still has in-band candidates the archer keeps preferring over these, so
			// this never overrides a live target; it only fires when the archer would
			// otherwise stand idle next to something nobody else can reach. Deliberately
			// NOT wired into the Strikers/Damage path below — this only ever widens what an
			// Archer can hit, never what can hit an Archer, so nothing about incoming damage
			// changes.
			float DeadZoneNearestSq[8];
			for (int32 k = 0; k < MyTargets; ++k)
			{
				DeadZoneNearestSq[k] = TNumericLimits<float>::Max();
			}
			int32 DeadZoneInReach = 0;
			bool bDeadZoneContact = false;
			FVector2f DeadZoneToward = FVector2f::ZeroVector;

			Swarm->QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
			{
				if (Entry.bRetinue == bRetinue)
				{
					return;
				}
				const float DistSq = (float)FVector::DistSquared2D(Entry.Location, Location);
				if (DistSq >= MyRangeSq)
				{
					return;
				}
				if (DistSq < MyMinRangeSq)
				{
					// Dead zone — shadow-tracked only, see the escape-hatch comment above.
					if (MyMinRangeSq > 0.f)
					{
						bDeadZoneContact = true;
						++DeadZoneInReach;
						DeadZoneToward += FVector2f(
							(float)(Entry.Location.X - Location.X),
							(float)(Entry.Location.Y - Location.Y)).GetSafeNormal();
						for (int32 k = 0; k < MyTargets; ++k)
						{
							if (DistSq < DeadZoneNearestSq[k])
							{
								for (int32 s = MyTargets - 1; s > k; --s)
								{
									DeadZoneNearestSq[s] = DeadZoneNearestSq[s - 1];
								}
								DeadZoneNearestSq[k] = DistSq;
								break;
							}
						}
					}
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
					if (CreditSquad == INDEX_NONE)
					{
						CreditSquad = Entry.SquadId;
					}
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

			// Promote the dead-zone shadow set only if the in-band scan found nothing at
			// all. This is the only place bDeadZoneContact is read — see the comment above
			// the shadow scan for why this can't fire for anyone but an isolated Archer.
			if (!bContact && bDeadZoneContact)
			{
				bContact = true;
				InReach = DeadZoneInReach;
				Toward = DeadZoneToward;
				for (int32 k = 0; k < MyTargets; ++k)
				{
					NearestSq[k] = DeadZoneNearestSq[k];
				}
			}

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
						bCreditHero = (CreditSquad == INDEX_NONE);
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
							// This brood's OWN blow, not a flat team value: swing intervals
							// are per-unit now (SwingIntervalFor), and blow = DPS * interval.
							// Paying a flat blow while striking on a jittered clock would hand
							// a fast-rolled brood more DPS than its stat block says — against
							// the hero only, which is the worst place to hide it.
							HeroDamage += OwnEntry->BlowDamage;
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

				// The kill lands HERE, not in USwarmDeathProcessor: this is the last point
				// at which who dealt the damage is still known. The HP > 0 test is what
				// makes it exactly-once — an already-dead body taking another blow before
				// the deferred DestroyEntity flushes must not pay out a second time.
				// Retinue deaths are not attributed: brood carry no unit to credit.
				const bool bLethal = !bRetinue && Health[i].HP > 0.f && (Health[i].HP - Damage) <= 0.f;

				Health[i].HP -= Damage;

				if (bLethal)
				{
					if (CreditSquad != INDEX_NONE)
					{
						KilledBySquad[SwarmSquad::UnitIndex((uint8)CreditSquad)]++;
					}
					else if (bCreditHero)
					{
						++HeroKilled;
					}
				}
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
	Swarm->CreditKills(KilledBySquad, HeroKilled);
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

//----------------------------------------------------------------------
// Swarm.KnightSubtypeReport (task-095)
//----------------------------------------------------------------------
namespace
{
	/**
	 * Which knight sub-type ROW each live Spearman is actually fighting on, right now,
	 * printed BESIDE the table it came from — same reasoning as SwarmRenderActor.cpp's
	 * Swarm.BroodVariantReport/TeamVariantReport: a histogram without its table proves
	 * nothing. Nine rows back eleven looks (task-095), so this also prints which
	 * team-atlas variant indices feed each row (Swarm.KnightSubtypeMap).
	 *
	 * Reads the render buffer's already-packed variant + squad byte (SwarmRenderPack),
	 * the exact same numbers the sprite on screen and the combat loop's own per-entity
	 * lookup are built from, rather than re-deriving anything from Phase — so this can
	 * never show a different mix than what is actually fighting.
	 */
	void LogKnightSubtypeHistogram(const UWorld* World)
	{
		const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!Swarm)
		{
			UE_LOG(LogTemp, Warning, TEXT("KnightSubtypeReport: no swarm subsystem (not in play?)"));
			return;
		}

		const SwarmCombatTuning::FKnightSubtypeTables Tables = SwarmCombatTuning::GetKnightSubtypeTables();

		int32 RowCounts[SwarmCombatTuning::MaxKnightSubtypeRows] = {};
		int32 Total = 0;
		for (const int32 Bits : Swarm->GetRenderAnimBits())
		{
			if ((Bits & SwarmAnim::TeamBit) == 0)
			{
				continue; // brood -- sub-types are melee-retinue only
			}
			if (SwarmSquad::UnitType(SwarmRenderPack::Squad(Bits)) == EUnitType::Archers)
			{
				continue; // Archers untouched -- still on their own Archers* tuning
			}
			++Total;
			const int32 Row = SwarmCombatTuning::KnightSubtypeRowFor(Tables, SwarmRenderPack::Variant(Bits));
			RowCounts[FMath::Clamp(Row, 0, SwarmCombatTuning::MaxKnightSubtypeRows - 1)]++;
		}

		FString Line;
		for (int32 Row = 0; Row < Tables.NumRows; ++Row)
		{
			FString Indices;
			for (int32 V = 0; V < Tables.NumVariants; ++V)
			{
				if (Tables.Map[V] == Row)
				{
					Indices += Indices.IsEmpty() ? FString::Printf(TEXT("v%d"), V) : FString::Printf(TEXT(",v%d"), V);
				}
			}
			Line += FString::Printf(TEXT("  row%d[%s] HP=%.0f DPS=%.1f Engage=%.0f Targets=%d count=%d (%.1f%%)"),
				Row, *Indices, Tables.HP[Row], Tables.DPS[Row], Tables.Engage[Row], Tables.Targets[Row],
				RowCounts[Row], Total > 0 ? 100.f * (float)RowCounts[Row] / (float)Total : 0.f);
		}
		UE_LOG(LogTemp, Display, TEXT("KnightSubtypeReport: %d knights | map \"%s\" |%s"),
			Total, *CVarKnightSubtypeMap.GetValueOnAnyThread(), *Line);
	}

	FAutoConsoleCommandWithWorld GCmdKnightSubtypeReport(
		TEXT("Swarm.KnightSubtypeReport"),
		TEXT("Log how many live Spearmen are fighting on each knight sub-type row, beside\n")
		TEXT("the HP/DPS/Engage/Targets table and the variant indices that feed each row."),
		FConsoleCommandWithWorldDelegate::CreateLambda(
			[](UWorld* World) { LogKnightSubtypeHistogram(World); }));
}
