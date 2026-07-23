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

USTRUCT()
struct FSwarmHealthFragment : public FMassFragment
{
	GENERATED_BODY()

	float HP = 100.f;
	float MaxHP = 100.f;
};

namespace SwarmCombatTuning
{
	// A retinue soldier must be individually worth several brood, or numbers
	// alone decide every fight and stances stop mattering. At these values one
	// soldier kills a brood in 2s and survives ~2.3s against a full 4-brood
	// swarm — so being surrounded is what kills you, not being outnumbered in
	// aggregate. That is the distinction the stances exist to control.
	constexpr float RetinueMaxHP = 130.f;
	constexpr float RetinueDPS = 30.f;
	constexpr float BroodMaxHP = 60.f;
	constexpr float BroodDPS = 14.f;

	constexpr float MeleeRange = 95.f;
	constexpr float MeleeRangeSq = MeleeRange * MeleeRange;

	/** A single unit can only be meaningfully swarmed by so many at once. */
	constexpr int32 MaxAttackersPerUnit = 4;

	// The hero is tanky but is NOT a win condition on his own. At 120 DPS he
	// could clear a wave solo after the army died, which is exactly the failure
	// GDD §4 "hero relevance" warns about — the hero must matter as a commander,
	// not as the damage. He survives long enough to reposition the line; he
	// cannot replace it.
	constexpr float HeroMaxHP = 500.f;
	constexpr float HeroDPS = 55.f;
	constexpr float HeroMeleeRange = 190.f;
	constexpr float HeroMeleeRangeSq = HeroMeleeRange * HeroMeleeRange;
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
