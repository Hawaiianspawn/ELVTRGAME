#pragma once

#include "MassEntityTypes.h"
#include "SwarmFragments.generated.h"

// Anim byte layout (mirrored in NS_Swarm / M_Swarm):
//   bit 0: walk frame (0/1)
//   bit 1: attacking
//   bit 2: flip X
//   bit 3: team (0 = brood, 1 = retinue)
namespace SwarmAnim
{
	constexpr uint8 FrameBit = 1 << 0;
	constexpr uint8 AttackBit = 1 << 1;
	constexpr uint8 FlipBit = 1 << 2;
	constexpr uint8 TeamBit = 1 << 3;
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
};

USTRUCT()
struct FSwarmTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FBroodTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FRetinueTag : public FMassTag { GENERATED_BODY() };
