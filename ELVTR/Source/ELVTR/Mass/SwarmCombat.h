#pragma once

#include "MassEntityTypes.h"
#include "SwarmCombat.generated.h"

/**
 * Gate 1 "fun prototype" combat + stance types.
 *
 * Combat is a continuous attrition model, not discrete swings: each unit counts
 * the enemies inside its melee radius (via the shared grid) and bleeds HP at
 * DPS * EnemyCount * dt. No damage events, no random-access writes across
 * entities, so every combat pass stays chunk-local and parallel-safe. Reads
 * correctly for horde combat — a line that is outnumbered 3:1 melts three times
 * as fast — and it is the cheapest thing that can answer the design question.
 */

/** Whole-retinue intent. Mirrors GDD.md §4 "v1 stance set". BlueprintType because the HUD's
 *  FKindledSquad carries one as a BlueprintReadWrite property. */
UENUM(BlueprintType)
enum class ESwarmStance : uint8
{
	Follow	UMETA(DisplayName = "Follow"),
	Charge	UMETA(DisplayName = "Charge"),
	Hold	UMETA(DisplayName = "Hold"),
	Rally	UMETA(DisplayName = "Rally"),
};

inline const TCHAR* LexToString(ESwarmStance Stance)
{
	switch (Stance)
	{
	case ESwarmStance::Charge:	return TEXT("CHARGE");
	case ESwarmStance::Hold:	return TEXT("HOLD");
	case ESwarmStance::Rally:	return TEXT("RALLY");
	default:					return TEXT("FOLLOW");
	}
}

/**
 * v1 typed-unit roster (docs/design/squad-group-system.md §1.1) — exactly two types,
 * deliberately, per Design Law 5. Assigned once at recruit time (USwarmSubsystem::
 * AssignRecruit) and permanent: never changed by combat, promotion, repacking or
 * reinforcement (§1.5). Values match docs/data/unit-types.json's ordering (spearmen
 * first) so a future third type is an additive enum entry, not a renumbering.
 */
UENUM()
enum class EUnitType : uint8
{
	Spearmen	UMETA(DisplayName = "Spearmen"),
	Archers		UMETA(DisplayName = "Archers"),
};

inline constexpr int32 NumUnitTypes = 2;

/**
 * A boss's accreted MARKS (docs/design/castle-layout.md §6.1).
 *
 * A bitmask and not an enum-with-one-value because §6.1's whole claim is that marks
 * COMPOUND — "a boss that took the Great Gate, ate the column that routed from it, and then
 * survived the Works' archers arrives at the Ascent as a gate-breaking, regenerating,
 * ranged-immune problem". Three of the six are implemented here; the other three (Wearing,
 * Unblinded, Column-fed) need war-sim events that do not exist yet, so they are deliberately
 * absent rather than stubbed.
 *
 * NOTHING ACCRETES THESE YET. §6.2's claim — that a boss is a report on a war you were not
 * in — needs the war sim, which is out of this slice's scope. They are set by console
 * command (Kindled.Boss.Marks), which is a prototype surface, not a design position on how a
 * boss earns them.
 *
 * Plain enum rather than a UENUM: nothing Blueprint-facing reads it, and a reflected
 * bitflag enum would add a class-layout dependency to a header the whole sim includes.
 */
enum class EBossMark : uint8
{
	None     = 0,
	Quilled  = 1 << 0,	// armour that scales against RANGED specifically; seeks ranged positions
	Ram      = 1 << 1,	// prefers the objective over bodies; heavy damage to what it is aimed at
	Sated    = 1 << 2,	// regenerates between engagements
};

inline constexpr uint8 operator|(EBossMark A, EBossMark B) { return (uint8)A | (uint8)B; }
inline constexpr bool HasMark(uint8 Marks, EBossMark M) { return (Marks & (uint8)M) != 0; }

inline const TCHAR* LexToString(EUnitType Type)
{
	return Type == EUnitType::Archers ? TEXT("ARCHERS") : TEXT("SPEARMEN");
}

/**
 * The four squad-channelled verbs — docs/design/ability-kit.md §1, sourced from CLASSES.md's
 * four hero kits per Q29 = A. Q13 = C's sketch list ("focus fire, reposition, screen, raise")
 * named four words; §1 transcribes which actual class verb each one is:
 *
 *   Focus  = Mark Quarry   (Pathfinder signature) — "the entire pack focus-fires it"
 *   Screen = Ward Circle   (Relickeeper field)    — "allies inside take reduced damage"
 *   Raise  = Kindle        (Lampbearer signature) — "channel onto a unit: heal over time"
 *   Rally  = Banner Slam   (Vanguard signature)   — "retinue in radius gains attack speed
 *                                                    and fights to the death"
 *
 * WHICH FOUR IS NOT A VERDICT ON Q23. Q23 asks whether the kit lives on the player, in the
 * soldiers, or both — a question about STRUCTURE, which this enum deliberately does not
 * encode. The same four values are reached both ways at runtime (Kindled.Ability.Mode), which
 * is the entire point of the build: the owner compares the two addressings against one boss.
 *
 * Reposition (Shield Wall) is absent because the shipped stance set already IS it —
 * ESwarmStance::Hold anchors a unit on the ground it was called on, which is Shield Wall's
 * own mechanical text ("the formation stops tracking the hero and holds that spot").
 * Duplicating it as a fifth verb would have been a second way to press the same button.
 *
 * Plain enum, matching EBossMark's reasoning one struct up: nothing Blueprint-facing reads
 * it, and a reflected enum would put a class-layout dependency in a header the whole sim
 * includes. The UI's own copy is an FText carried on FKindledSquad.
 */
enum class ESquadVerb : uint8
{
	None = 0,
	Focus,
	Screen,
	Raise,
	Rally,
	Num
};

inline constexpr int32 NumSquadVerbs = (int32)ESquadVerb::Num;

inline const TCHAR* LexToString(ESquadVerb Verb)
{
	switch (Verb)
	{
	case ESquadVerb::Focus:		return TEXT("FOCUS");
	case ESquadVerb::Screen:	return TEXT("SCREEN");
	case ESquadVerb::Raise:		return TEXT("RAISE");
	case ESquadVerb::Rally:		return TEXT("RALLY");
	default:					return TEXT("—");
	}
}

/** The CLASSES.md verb this one IS, for a HUD line and a log the owner can trace to canon. */
inline const TCHAR* SquadVerbSource(ESquadVerb Verb)
{
	switch (Verb)
	{
	case ESquadVerb::Focus:		return TEXT("Mark Quarry");
	case ESquadVerb::Screen:	return TEXT("Ward Circle");
	case ESquadVerb::Raise:		return TEXT("Kindle");
	case ESquadVerb::Rally:		return TEXT("Banner Slam");
	default:					return TEXT("none");
	}
}

/** "focus" / "mark" -> ESquadVerb::Focus. ESquadVerb::None on anything unknown. */
inline ESquadVerb SquadVerbFromToken(const FString& Token)
{
	if (Token.Equals(TEXT("focus"), ESearchCase::IgnoreCase) || Token.Equals(TEXT("mark"), ESearchCase::IgnoreCase))
	{
		return ESquadVerb::Focus;
	}
	if (Token.Equals(TEXT("screen"), ESearchCase::IgnoreCase) || Token.Equals(TEXT("ward"), ESearchCase::IgnoreCase))
	{
		return ESquadVerb::Screen;
	}
	if (Token.Equals(TEXT("raise"), ESearchCase::IgnoreCase) || Token.Equals(TEXT("kindle"), ESearchCase::IgnoreCase))
	{
		return ESquadVerb::Raise;
	}
	if (Token.Equals(TEXT("rally"), ESearchCase::IgnoreCase) || Token.Equals(TEXT("banner"), ESearchCase::IgnoreCase))
	{
		return ESquadVerb::Rally;
	}
	return ESquadVerb::None;
}

USTRUCT()
struct FSwarmHealthFragment : public FMassFragment
{
	GENERATED_BODY()

	float HP = 100.f;
	float MaxHP = 100.f;
};

namespace SwarmCombatTuning
{
	// These were compile-time constants; they are now backed by Swarm.* CVars so
	// combat can be tuned live (CVar declarations + these getters' definitions
	// live in SwarmCombatProcessors.cpp). The old `...Sq` helpers were dropped —
	// call sites square the range locally, cached once per processor pass so the
	// hot loop still does zero per-entity CVar reads.

	// A retinue soldier must be individually worth several brood, or numbers
	// alone decide every fight and stances stop mattering. At the defaults one
	// soldier kills a brood in 2s and survives ~2.3s against a full 4-brood
	// swarm — so being surrounded is what kills you, not being outnumbered in
	// aggregate. That is the distinction the stances exist to control.
	float RetinueMaxHP();
	float RetinueDPS();
	float BroodMaxHP();
	float BroodDPS();

	float MeleeRange();

	/**
	 * Hard SAFETY clamp on blows counted against one unit in a single frame.
	 *
	 * No longer the rate limiter it used to be. It capped the first N enemies found in
	 * grid iteration order, and since the grid is rebuilt every frame as units move,
	 * that set churned — over one swing interval far more than N attackers each landed
	 * a blow, and the error grew with the interval (measured 2026-07-25: wave-1 retinue
	 * survivors 97-103 -> 60-62). Incoming damage is now bounded by geometry instead:
	 * a striker hits only its TargetsPerHit nearest enemies. This survives purely to
	 * stop a pathological pile-up producing an absurd single-frame spike.
	 */
	int32 MaxAttackersPerUnit();

	/**
	 * How many enemies one blow lands on — the attacker's K nearest. Per team, because
	 * the pre-2026-07-25 continuous model was implicitly infinite-cleave on output while
	 * capping intake, and that asymmetry is what let an outnumbered line hold. A single
	 * shared K cannot express it: raising it lifts both sides equally.
	 * Retinue cleave; brood commit to one target. The retinue dial is the cleave powerup.
	 */
	int32 RetinueTargetsPerHit();
	int32 BroodTargetsPerHit();

	// --- typed units (docs/design/squad-group-system.md §1, §2, §4.1) ------------------
	// Spearmen ARE today's retinue (RetinueMaxHP/RetinueDPS/MeleeRange/RetinueTargetsPerHit
	// above, unchanged names — no churn on the shipped, owner-tuned Gate 1 defaults).
	// Archers are new: minimum-viable ranged combat, §2.2 — reuses the existing
	// StrikeReachSq/BlowsClaimed grid mechanism with a larger EngageRange, no new pass.

	/** Archer HP/DPS — docs/data/unit-types.json types.archers.combat, unmeasured (§7.5). */
	float ArchersMaxHP();
	float ArchersDPS();

	/**
	 * How far an archer's blow reaches, uu — the load-bearing ranged-combat number (§2.2).
	 * Reaches through the SAME grid Spearmen/brood melee already uses; nothing about the
	 * grid, the victim-pull rule, or BlowsClaimed's conservation-of-damage guarantee
	 * assumes a small radius. Comfortably inside the 3x3 grid's 750uu reach at
	 * USwarmSubsystem::GridCellSize 250 (task-052) — raising this past 750 would silently
	 * behave as 750 regardless of what the CVar says, same caveat as Swarm.BroodAggroRange.
	 */
	float ArchersEngageRange();

	/**
	 * An archer will not engage anything closer than this to ITSELF, uu (§2.2's line-of-fire
	 * mitigation: "won't visibly shoot into its own scrum"). Cheap, local approximation —
	 * NOT true line-of-sight against a specific ally (Design Law 5 rules out per-pair
	 * occlusion at horde scale) — just a band [MinEngageRange, EngageRange] on the archer's
	 * own reach instead of [0, EngageRange]. Archers stand well behind the spear line
	 * (Swarm.Formation.Archers.Forward), so in practice this only bites if the line breaks.
	 */
	float ArchersMinEngageRange();

	/** Archer cleave — 1 by design (precise single-target volleys, not free cleave a mass
	 *  archer line hasn't earned through positioning the way Spearmen's K=8 has). */
	int32 ArchersTargetsPerHit();

	/** Archer march speed, as a multiple of SwarmTuning::RetinueSpeed (SwarmProcessors.cpp). */
	float ArchersMoveSpeedScale();

	/**
	 * Fraction of each new recruit rolled Archer rather than Spearman (docs/data/
	 * unit-types.json growth_source_weight — 0.8/0.2). v1 recruitment has no real
	 * "growth site" system yet (GDD §9's "flags, not simulation" — see §1.4), so this
	 * stands in for a generator-tagged site: every new soldier rolls its type against
	 * this weight at spawn, independent of any other soldier's roll.
	 */
	float ArcherGrowthWeight();

	// --- swing cadence + hit reaction -----------------------------------
	// The DPS values above are still the design currency; the cadence only decides
	// how that damage is *parcelled out*. One blow removes DPS * SwingInterval, so
	// average throughput is unchanged and the Gate 1 tuning still reads true. What
	// changed is that there is now an instant to react to.
	float SwingInterval();
	float ArcherSwingInterval();
	float SwingIntervalJitter();
	float SwingStrikeAt();
	float SwingLunge();
	float HitFlashTime();

	/**
	 * THIS unit's swing interval — its type's base cadence, spread per-unit by the
	 * jitter phase it was spawned with.
	 *
	 * Two things were wrong before this existed. A whole rank engages on the same frame,
	 * every SwingTime started at exactly 0, and one global interval advanced all of them
	 * by the same DeltaTime — so the line fired as one animal, forever. And a bow is not
	 * a spear jab: archers had no cadence of their own at all.
	 *
	 * CALLERS MUST PAIR THIS WITH THE BLOW. One blow is DPS * interval, so a unit whose
	 * interval is jittered 20% fast and whose blow is not would deal 20% more DPS than
	 * its stat block says. Both the grid publish (SwarmProcessors.cpp) and the swing
	 * clock (the integrate pass) resolve the interval through THIS function and multiply
	 * the type's DPS by the value it returns — that is what keeps the jitter cosmetic.
	 *
	 * Derived, not stored: no fragment grows a field. The multiplier is a different
	 * irrational than the size roll (0.618) and the glance clock (0.382) so a unit's
	 * cadence does not correlate with how big it is or when it turns its head.
	 */
	FORCEINLINE float SwingIntervalFor(bool bArcher, float Phase)
	{
		const float Base = bArcher ? ArcherSwingInterval() : SwingInterval();
		const float Spread = SwingIntervalJitter();
		if (Spread <= 0.f)
		{
			return Base;
		}
		// Frac(Phase * irrational) -> [0,1) -> [-1,+1], stable for this unit's lifetime.
		const float Roll = FMath::Frac(Phase * 0.7548776662f) * 2.f - 1.f;
		return FMath::Max(Base * (1.f + Roll * Spread), 0.05f);
	}

	/** How far a struck unit is shoved, in uu, and over how long. */
	float KnockbackDistance();
	float KnockbackTime();

	// The hero is tanky but is NOT a win condition on his own. At high DPS he
	// could clear a wave solo after the army died, which is exactly the failure
	// GDD §4 "hero relevance" warns about — the hero must matter as a commander,
	// not as the damage. He survives long enough to reposition the line; he
	// cannot replace it.
	ELVTR_API float HeroMaxHP();	// exported: the editor toolset's swarm snapshot reports hp/maxHp
	/**
	 * RETIRED AS THE PLAYER'S DAMAGE, 2026-08-13 — castle-layout.md §6.4, Q13 = C: "the
	 * player's entire output is the seven... you have no independent attack worth using."
	 *
	 * The getter, the CVar, the swing clock on the pawn and the whole grid bridge in
	 * SwarmCombatProcessors.cpp all STAY — §6.4 is explicit that the hero remains a body in
	 * the grid that can be struck, and entity-tiers.md §1 still points at that bridge as the
	 * precedent every promoted Actor reuses (the boss below does exactly that). What changed
	 * is the number it carries: the shipped default is now 0.
	 *
	 * Set it back above 0 to A/B the pre-pivot hero. NOTE Saved/SwarmExecOnPlay.txt overrides
	 * this default at BeginPlay, so the C++ value alone proves nothing — see slice-a7.md.
	 */
	float HeroDPS();
	float HeroMeleeRange();

	// --- the marked boss (docs/design/entity-tiers.md §3, castle-layout.md §6) ------------
	// Baseline stat block transcribed from entity-tiers.md §3's Boss row, unchanged: 6000 HP,
	// 110 DPS, Armor 14, TargetsPerHit 6, 250uu melee, 2.0s SwingInterval. SurroundCap is §4's
	// own 35-55 estimate taken at its midpoint. Nothing here re-derives or "improves" a number
	// that doc already owns.
	float BossMaxHP();
	float BossDPS();
	float BossArmor();
	float BossArmorChipFloor();
	int32 BossTargetsPerHit();
	float BossMeleeRange();
	float BossSwingInterval();
	int32 BossSurroundCap();
	float BossSpeed();

	/** Extra Armor applied ONLY to blows from Archers, while the Quilled mark is carried. */
	float BossQuilledArmor();
	/** Ram: how its blow scales against a soldier, and against the thing it is actually aimed at. */
	float BossRamBodyScale();
	float BossRamObjectiveScale();
	/** Sated: seconds without taking a blow before regeneration starts, and its rate in HP/s. */
	float BossSatedCalmSeconds();
	float BossSatedRegenPerSecond();

	// --- the squad-channelled ability kit (task-144, docs/design/ability-kit.md) -----------
	// EVERY tuning value here is a CVar and none is a constant, on the owner's dispatch note:
	// cooldowns, ranges, magnitudes and durations all get tweaked against a live fight, and a
	// recompile between two tries is the thing that stops an owner comparing Q23 = A against
	// Q23 = B honestly. Defaults are stated in SwarmCombatProcessors.cpp beside each CVar and
	// mirrored into ELVTR/Config/SwarmExecOnPlay.canonical.txt.

	/** 0 = Q23 A (a fixed kit on the player), 1 = Q23 B (the verb lives in the soldier).
	 *  NOT A VERDICT — the flip is the deliverable. See docs/design/slice-a7.md §10. */
	int32 AbilityMode();

	/** Q23 = A only: how far from the bearer a soldier still counts as "in range" of the kit. */
	float AbilityPlayerRange();

	/** Seconds between casts. Per VERB under Q23 = A; per SOLDIER under Q23 = B. */
	float AbilityCooldown();

	/** Mark Quarry: how long the pack keeps focus-firing the marked enemy. */
	float AbilityFocusSeconds();

	/** Ward Circle: duration, radius, and the multiplier on damage taken inside it. */
	float AbilityScreenSeconds();
	float AbilityScreenRadius();
	float AbilityScreenScale();

	/** Kindle: channel length and HP/s restored to the soldier being raised. */
	float AbilityRaiseSeconds();
	float AbilityRaiseRate();

	/** Banner Slam: duration, radius, and the swing-clock rate multiplier inside it. */
	float AbilityRallySeconds();
	float AbilityRallyRadius();
	float AbilityRallyHaste();

	/** How close the cursor has to be to a thing for Q26 = A (direct target) to mean it. */
	float AbilityPickRadius();

	/** 0 hides the verb wheel and the active-zone rings. They still work — this only draws. */
	int32 AbilityDraw();

	// --- knight sub-types (task-095, docs/design/retinue-melee-subtypes.md) -----------
	// Binds the team-atlas VARIANT INDEX a Spearman already wears (SwarmSheet::Team,
	// task-085) to a stat row, so a knight's look and its fight can never disagree and no
	// new per-entity state is stored — see docs/perf/knight-subtype-binding.md. Archers are
	// untouched; they stay on the Archers* getters above. Brood are untouched too.

	inline constexpr int32 MaxKnightSubtypeRows = 16;      // headroom past today's 9 rows
	inline constexpr int32 MaxKnightSubtypeVariants = 16;  // SwarmRenderPack's 4-bit variant field

	/**
	 * Everything the melee sub-type binding needs, snapshotted ONCE PER PROCESSOR PASS
	 * (see SwarmProcessors.cpp's FVariantTable for why a per-pass Atoi pass is fine next to
	 * a 30k-entity sim and a per-entity one is not) and captured by value into a per-chunk
	 * lambda — small enough (~200 bytes) for that, same idiom the render bridge already
	 * uses for its own variant table.
	 *
	 * TeamVariantCum is the SAME cumulative Swarm.TeamVariantWeights table the render
	 * bridge resolves a look from (owned by SwarmProcessors.cpp's translation unit; read
	 * by name here, not redeclared — the idiom SwarmRenderActor.cpp's LogVariantHistogram
	 * already uses to log it). Feeding it into the SAME SwarmRenderPack::VariantFromPhase
	 * formula with a unit's own Jitter phase is what guarantees a combat lookup here always
	 * agrees with the sprite on screen — nothing about "which knight is this" is decided
	 * twice.
	 */
	struct FKnightSubtypeTables
	{
		float HP[MaxKnightSubtypeRows] = {};
		float DPS[MaxKnightSubtypeRows] = {};
		float Engage[MaxKnightSubtypeRows] = {};
		int32 Targets[MaxKnightSubtypeRows] = {};	// clamped 1-8 at parse time — see the .cpp
		int32 NumRows = 0;

		int32 Map[MaxKnightSubtypeVariants] = {};	// variant index (0-10) -> row index
		int32 NumVariants = 0;

		int32 TeamVariantCum[MaxKnightSubtypeVariants] = {};
		int32 NumTeamVariants = 0;
	};

	FKnightSubtypeTables GetKnightSubtypeTables();

	/** Row for a team-atlas variant index, via Tables.Map — clamped both ways so a short or
	 * malformed CVar list falls back to row 0 instead of reading garbage. */
	int32 KnightSubtypeRowFor(const FKnightSubtypeTables& Tables, int32 VariantIndex);

	// --- Adaptation tiers (docs/design/adaptation.md §2) ------------------------------
	// THE stat spine, and the only one: a rung is (unit_type, tier, variant_index) and
	// `tier` keys docs/data/upgrades.json tier_ladder.tiers[]. Those four rows are
	// TRANSCRIBED into Swarm.TierHP / Swarm.TierDPS below — not re-derived, not rescaled,
	// and no fifth row is invented here (adaptation.md §4 / O6 bans exactly that).
	//
	// What this deliberately does NOT carry is engage range and cleave. The tier ladder
	// has no such columns and inventing them would be a second stat ladder in all but
	// name. An adapted unit keeps taking those from its LOOK — the knight sub-type row
	// for Spearmen, the flat Archers* getters for Archers — which is coherent rather than
	// a compromise: a rung IS a look, so reach and cleave still come from the body you
	// can see, and only HP/DPS move along the tier spine.
	//
	// The bannerman row measures WORSE than veteran (160/35 against 190/45) and that is
	// correct — its value is an aura, and adaptation.md §9 records at length why raising
	// it to flatten the table would delete the captain rung's whole reason to exist.
	// Scripts/sim/drift_check.py guards those four rows; keep these strings equal to them.

	inline constexpr int32 MaxTierRows = 8;	// headroom past today's four

	struct FTierTable
	{
		float HP[MaxTierRows] = {};
		float DPS[MaxTierRows] = {};
		int32 NumRows = 0;
	};

	/** Snapshotted once per processor pass, same contract as GetKnightSubtypeTables. */
	FTierTable GetTierTable();

	/** HP/DPS for a tier index, or the caller's fallback when the unit has not adapted
	 *  (TierIndex < 0) or the CVar list is short. Never reads outside the parsed rows. */
	FORCEINLINE float TierHPOr(const FTierTable& Tiers, int32 TierIndex, float Fallback)
	{
		return (TierIndex >= 0 && TierIndex < Tiers.NumRows) ? Tiers.HP[TierIndex] : Fallback;
	}
	FORCEINLINE float TierDPSOr(const FTierTable& Tiers, int32 TierIndex, float Fallback)
	{
		return (TierIndex >= 0 && TierIndex < Tiers.NumRows) ? Tiers.DPS[TierIndex] : Fallback;
	}
}

namespace SwarmLeash
{
	/** docs/RTS-VERTICAL-SLICE.md §2 working values. */
	constexpr float Radius = 2000.f;
	constexpr float RadiusSq = Radius * Radius;
	constexpr float Hysteresis = 0.15f;					// re-anchor band
	constexpr float ReanchorRadius = Radius * (1.f - Hysteresis);
	constexpr float ReanchorRadiusSq = ReanchorRadius * ReanchorRadius;
	constexpr float WarnFraction = 0.8f;				// flash before breaking
	constexpr float WarnRadiusSq = (Radius * WarnFraction) * (Radius * WarnFraction);
}
