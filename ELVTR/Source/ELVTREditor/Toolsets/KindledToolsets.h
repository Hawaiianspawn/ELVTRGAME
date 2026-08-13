#pragma once

#include "CoreMinimal.h"

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "KindledToolsets.generated.h"

/**
 * Console access for the running editor, and for PIE while it is up.
 *
 * The engine's EditorAppToolset can only *search* console variables. Until this
 * existed, changing one meant editing Saved/SwarmExecOnPlay.txt and relaunching,
 * or asking a human to paste a line into the console — see
 * Scripts/populate_cvar_preset.py, which was the only write path.
 *
 * Commands run against the PIE world when a play session is up, otherwise the
 * editor world, so the same call tunes a live fight or a cold level.
 */
UCLASS(BlueprintType, MinimalAPI)
class UKindledConsoleToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Runs one console command and returns everything it printed.
	 *
	 * Accepts anything the in-editor console accepts: a CVar assignment
	 * ("Swarm.BroodAdd 0.3"), a bare CVar name to print its current value, an exec
	 * command ("Swarm.SpawnBrood 2000", "stat Swarm"), or an engine command.
	 * Prefer SetCVar for assignments — a mistyped CVar name is silently ignored here.
	 *
	 * @param Command The command line to run. Raises a script error if empty.
	 * @return The captured output, or an empty string for a command that prints
	 *   nothing. A command the engine did not recognise is reported in the output.
	 */
	UFUNCTION(meta = (AICallable), Category = "KindledConsoleToolset")
	static FString Exec(const FString& Command);

	/**
	 * Sets a console variable and reads the value back.
	 *
	 * Unlike Exec, this fails loudly on a CVar that does not exist, which is the
	 * usual way a tuning call silently does nothing.
	 *
	 * @param Name Full CVar name, e.g. "Swarm.BroodAdd". Raises a script error if
	 *   no such CVar is registered.
	 * @param Value The value to set, as a string. Parsed by the CVar's own type.
	 * @return The value the CVar holds after the set — compare it against Value to
	 *   catch a clamp or a rejected parse.
	 */
	UFUNCTION(meta = (AICallable), Category = "KindledConsoleToolset")
	static FString SetCVar(const FString& Name, const FString& Value);
};

/**
 * Live read of the Kindled swarm simulation.
 *
 * Answers "what is the sim actually doing right now" without adding a UE_LOG,
 * rebuilding, replaying and grepping the log — which is how every recent swarm
 * bug (archer rows, retinue distance-dim, hit flash, brood visibility) got found.
 *
 * Reads USwarmSubsystem's already-published counters, so it costs nothing and
 * reports exactly the numbers the renderer and the HUD are given.
 */
UCLASS(BlueprintType, MinimalAPI)
class UKindledSwarmToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Returns the current state of the swarm as a JSON object.
	 *
	 * Counts are valid from the end of each frame's integrate pass, so during PIE
	 * they are one frame old at worst. Keys:
	 *
	 *   pie           bool    — whether a play session is running. False means every
	 *                           count below is from a cold editor world and is likely 0.
	 *   worldTime     number  — seconds since the world began.
	 *   alive         object  — retinue, brood, spearmen, archers, leashBroken
	 *   renderCount   number  — entities pushed to the render bridge this frame
	 *   gridCells     number  — occupied spatial-grid cells
	 *   hero          object  — alive, hp, maxHp, striking
	 *   stance        string  — the last whole-retinue order: Follow/Charge/Hold/Rally
	 *   stanceAnchor  [x,y,z] — where that order was aimed
	 *   attractor     [x,y,z] — the point the brood steers at
	 *   kills         object  — retinue, brood (run totals), heroWave, heroRun
	 *   damage        object  — toRetinue, toBrood, hero (run totals, HP)
	 *   squads        array   — one entry per CLAIMED unit slot only:
	 *                           index, type, standing, stance, centroid, waveKills, runKills
	 *   fight         object  — present only while the telemetry recorder is running:
	 *                           index, duration, killedRetinue, killedBrood, exchangeRate
	 *
	 * Raises a script error if there is no world at all.
	 */
	UFUNCTION(meta = (AICallable), Category = "KindledSwarmToolset")
	static FString Snapshot();
};
