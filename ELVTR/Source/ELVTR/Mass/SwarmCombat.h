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

/** Whole-retinue intent. Mirrors GDD.md §4 "v1 stance set". */
UENUM()
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

inline const TCHAR* LexToString(EUnitType Type)
{
	return Type == EUnitType::Archers ? TEXT("ARCHERS") : TEXT("SPEARMEN");
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
	float SwingStrikeAt();
	float SwingLunge();
	float HitFlashTime();

	/** How far a struck unit is shoved, in uu, and over how long. */
	float KnockbackDistance();
	float KnockbackTime();

	// The hero is tanky but is NOT a win condition on his own. At high DPS he
	// could clear a wave solo after the army died, which is exactly the failure
	// GDD §4 "hero relevance" warns about — the hero must matter as a commander,
	// not as the damage. He survives long enough to reposition the line; he
	// cannot replace it.
	float HeroMaxHP();
	float HeroDPS();
	float HeroMeleeRange();
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
