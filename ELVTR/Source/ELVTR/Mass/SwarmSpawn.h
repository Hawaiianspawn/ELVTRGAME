#pragma once

#include "CoreMinimal.h"
#include "SwarmCombat.h" // EUnitType

class UWorld;

/**
 * Shared spawn entry points. Implemented in SwarmCommands.cpp alongside the
 * console commands so there is exactly one definition of what a brood or
 * retinue entity is composed of.
 */
namespace SwarmSpawn
{
	/** Ring of brood well outside the play area, so the tide visibly converges. */
	void SpawnBrood(UWorld* World, int32 Count);

	/**
	 * Retinue in concentric formation slots around the hero. SlotBase continues
	 * the ring packing from an existing retinue so mid-run reinforcements land
	 * in fresh slots instead of stacking on the ones already occupied.
	 */
	void SpawnRetinue(UWorld* World, int32 Count, int32 SlotBase = 0);

	// --- the pivot's two populations (docs/design/slice-a7.md) ---------------------------
	// Both are SpawnRetinue with the recruitment policy overridden, not a second kind of
	// soldier: same archetype, same fragments, same steering, same combat. What differs is
	// only which command handle the body lands on, and that is one field.

	/**
	 * The autonomous war. Every body joins the single garrison handle
	 * (USwarmSubsystem::GarrisonUnit), which is never swept by a broadcast order and is exempt
	 * from the leash — so it holds the line it was given whether or not the bearer is there.
	 * Types still roll against Swarm.ArcherGrowthWeight, so the line is mixed the way the war
	 * should be; the handle's own recorded type is therefore only cosmetic (see slice-a7.md).
	 */
	void SpawnGarrison(UWorld* World, int32 Count);

	/**
	 * ONE ARMY for the Battleground testbed (docs/design/battleground.md §1.2, §2.2) —
	 * SpearmenCount Spearmen and ArcherCount Archers, deliberately spawned as two separate
	 * FIXED-TYPE batches (not one weighted-roll batch) so the resulting unit COUNT is
	 * deterministic rather than a matter of how the RNG happened to land — the Build scope
	 * evidence this level owes is an exact handle count, not an expected one.
	 *
	 * Both batches land on TeamId's own claimed units via AssignRecruit's TeamId-partitioned
	 * search (USwarmSubsystem::AssignRecruit), at wherever USwarmSubsystem's Attractor
	 * currently is — SpawnRetinue's own "Center = GetAttractor()" convention, reused
	 * unchanged (SwarmCommands.cpp). The caller sets the Attractor to a deployment zone
	 * before calling this, once per team.
	 */
	void SpawnArmy(UWorld* World, int32 SpearmenCount, int32 ArcherCount, uint8 TeamId);

	/**
	 * ONE named soldier, on command handle UnitIndex (0..NamedSoldiers-1), with an explicit
	 * type and an HP multiplier over whatever rung the handle already carries.
	 *
	 * The multiplier is a prototype expedient and nothing else: the seven have no stat block
	 * anywhere in canon (Q2 and Q14 are both open), and at a plain veteran rung a named
	 * soldier dies to the tide in seconds — which makes it impossible to judge whether
	 * commanding them individually feels like anything. Assign the handle's look and rung with
	 * USwarmSubsystem::SetSquadRung BEFORE calling this, since HP is baked from it at spawn.
	 */
	void SpawnNamed(UWorld* World, int32 UnitIndex, EUnitType Type, float HPScale);

	/**
	 * A world point Distance uu from the bearer along the bearing the tide arrives on —
	 * Swarm.BroodSpawnArcCenter plus, while Swarm.BroodSpawnFaceCamera is set, the tracked
	 * camera yaw.
	 *
	 * Lives here because those two dials are owned by this translation unit, and because two
	 * separate things now need to agree with the tide about where "the front" is: the
	 * garrison's held line, and where the boss walks in. Deriving that bearing twice is how
	 * they end up on opposite sides of the bearer the first time anyone touches the arc.
	 */
	FVector TideBearingPoint(UWorld* World, float Distance);

	/** Destroy every tracked swarm entity. */
	void ClearAll(UWorld* World);

	/** Drop stale handles from the subsystem's tracking array after deaths. */
	void CompactTracked(UWorld* World);
}
