#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Measurement helpers for answering "is the sim actually doing what it looks
 * like it's doing" without trusting the renderer.
 */
namespace SwarmDebug
{
	/**
	 * Nearest-neighbour distance distribution per team, logged as one line each.
	 * Reads the subsystem's packed render buffers (positions + team bit), so it
	 * measures exactly the data handed to the renderer — if the report says the
	 * units are 110uu apart and the screen shows one blob, the bug is downstream
	 * of the sim.
	 */
	void LogSpacingReport(UWorld* World);
}
