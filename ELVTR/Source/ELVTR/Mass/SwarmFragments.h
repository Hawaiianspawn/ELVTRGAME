#pragma once

#include "MassEntityTypes.h"
#include "SwarmFragments.generated.h"

// Anim byte layout (mirrored in NS_Swarm / M_Swarm):
//   bit 0: walk frame (0/1)
//   bit 1: attacking
//   bit 2: flip X
//   bit 3: team (0 = brood, 1 = retinue)
//   bit 4: leash warning (retinue only, debug/HUD — not decoded by the SubUV bridge)
namespace SwarmAnim
{
	constexpr uint8 FrameBit = 1 << 0;
	constexpr uint8 AttackBit = 1 << 1;
	constexpr uint8 FlipBit = 1 << 2;
	constexpr uint8 TeamBit = 1 << 3;
	constexpr uint8 LeashWarnBit = 1 << 4;
}

USTRUCT()
struct FSwarmAnimFragment : public FMassFragment
{
	GENERATED_BODY()

	uint8 Bits = 0;
};

/** Per-entity variation so the brood doesn't move as one rigid mass. */
USTRUCT()
struct FSwarmJitterFragment : public FMassFragment
{
	GENERATED_BODY()

	float SpeedScale = 1.f;
	float Phase = 0.f; // walk-cycle offset, seconds
};

/** Retinue only: fixed formation slot, relative to the hero. */
USTRUCT()
struct FRetinueFollowFragment : public FMassFragment
{
	GENERATED_BODY()

	FVector2D SlotOffset = FVector2D::ZeroVector;

	/**
	 * Leash hysteresis latch (docs/RTS-VERTICAL-SLICE.md §2). Set when the unit
	 * exceeds LeashRadius, cleared only once it is well back inside
	 * ReanchorRadius. While set the unit ignores the global stance and behaves
	 * as Follow — "you must stay in the fight with your troops".
	 */
	bool bLeashBroken = false;
};

USTRUCT()
struct FSwarmTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FBroodTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FRetinueTag : public FMassTag { GENERATED_BODY() };
