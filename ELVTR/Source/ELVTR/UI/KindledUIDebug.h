#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Spawns/clears the M1 combat-HUD preview. Shared by the console commands and the
 *  game mode's auto-show on play. */
namespace KindledUI
{
	/** Add the combat-HUD bottom muster band to the play world's viewport. */
	void ShowCombatHud(UWorld* World);

	/** Remove any HUD these helpers spawned. */
	void ClearHud();

	/** Show the HUD if the Kindled.UI.AutoShow cvar is on (default on). */
	void AutoShowIfEnabled(UWorld* World);
}
