#pragma once

#include "CoreMinimal.h"

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

	/** Destroy every tracked swarm entity. */
	void ClearAll(UWorld* World);

	/** Drop stale handles from the subsystem's tracking array after deaths. */
	void CompactTracked(UWorld* World);
}
