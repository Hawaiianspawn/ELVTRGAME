---
name: playtest
description: Run Kindled in PIE, hold a scenario for a set number of seconds, and come back with a screenshot plus a live swarm snapshot. Composes the MCP tools (Kindled.Console, Kindled.Swarm, EditorAppToolset PIE + capture) into one pass so tuning a CVar and seeing the result is a single step. Use when the user runs /playtest or asks to play, run, soak, or look at a fight, to check what a CVar change does on screen, or to grab evidence of a swarm change working.
---

# playtest — one pass from CVar to screenshot

Turns "does this change actually look right" into one round of tool calls instead of
six. Everything here uses tools that already exist; this skill is the ordering.

## Prerequisites

The running editor must expose `KindledConsoleToolset` and `KindledSwarmToolset`.
Check with `list_toolsets`. If they are missing the editor is on a binary built
before they landed — rebuild with `pwsh Scripts\ue-iterate.ps1 -Force Relaunch` and
say so rather than falling back to the old paste-a-line-into-the-console workflow.

## The pass

1. **Set the scenario, cold.** Before PIE, apply the CVars under test with
   `KindledConsoleToolset.SetCVar` (one call each — it errors on a name that does not
   exist, which is the failure this replaces). Batch three or more through
   `ProgrammaticToolset.execute_tool_script` to save round-trips.

   CVars set before PIE survive into the play session. `Saved/SwarmExecOnPlay.txt`
   still runs at BeginPlay, so anything that file also sets will overwrite you —
   set those *after* PIE is up instead, or edit the file (see `/cvars`).

2. **Start and soak.** `EditorAppToolset.StartPIE` with
   `warmupSeconds` = however long the fight needs to develop. This is the only
   correct way to wait: the MCP call runs on the game thread, so sleeping inside a
   `ProgrammaticToolset` script freezes the world instead of letting it tick.

   Typical soaks: 2s to see spawn, 8-15s to see a line meet a brood wave, 30s+ for
   attrition. Populate the fight with `Kindled.Console.Exec("Swarm.SpawnBrood 2000")`
   or the level's own spawner.

3. **Read the sim.** `KindledSwarmToolset.Snapshot()`. Do this *before* the
   screenshot — it is the cheap, unambiguous answer, and half the time it settles the
   question without anyone looking at a picture. Check `pie: true` first; if it is
   false the session died and every count below it is meaningless.

4. **Look.** This step is the weak one — read the caveat before promising a picture.

   `EditorAppToolset.CaptureViewport` and `Exec("HighResShot 1920x1080")` both came
   back with the **editor level viewport**, gizmos and PlayerStart icon and all, while
   `IsPIERunning` said true (measured 2026-07-31, `PlayMode_InViewPort`). Neither is a
   gameplay frame. `CaptureViewport` with a `captureTransform` is an offscreen render
   of the editor world; passing `null` did not change the result.
   `CaptureEditorImage` — the one that grabs the app as a human sees it — killed the
   MCP connection outright.

   So: **do not report a capture as "the game" without checking it for gizmos.** Until
   this is sorted, a real gameplay frame still comes from a human alt-tabbing to the
   editor. `HighResShot` writes to `ELVTR/Saved/Screenshots/WindowsEditor/`; read the
   newest file from disk rather than through the tool result, which is a ~2 MB base64
   blob that blows the context limit.

   Both capture tools also require `captureTransform` and `annotations` to be present
   in the arguments even though the schema marks them optional — pass `null` for both.

5. **Stop.** `EditorAppToolset.StopPIE`. Always, including on a failed pass — a
   leftover PIE session blocks the next `StartPIE` and blocks `FocusOnActors`.

## Reporting

Lead with the snapshot numbers that answer the question, then the image. A
screenshot alone is not evidence for a count, a rate, or an exchange ratio —
`Snapshot()` is. The image is evidence for how it *looks*, which is the owner's call
(see the "show a build for review" standing rule: big changes get a runnable build or
on-screen evidence, never a written "it works").

## Do not

- **Do not send two MCP tool calls in one message.** They share one HTTP connection to
  the editor and the second closes the socket ("The socket connection was closed
  unexpectedly"). Every call in this skill goes one at a time. Batch through
  `ProgrammaticToolset` instead when you want several in one round-trip.
- Do not relaunch the editor to apply a CVar. That was the old loop; `SetCVar` is why
  this skill exists.
- Do not restart or close a shared editor to run a pass — another session may be
  using it. Ask first.
- Do not report a number you read off a screenshot when `Snapshot()` publishes it.
