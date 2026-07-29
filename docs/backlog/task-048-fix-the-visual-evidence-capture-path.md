---
id: 048
title: Fix the visual-evidence capture path so agent-driven PIE can prove what it built
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "docs/AGENT-TEAMS.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A single documented command an agent can run that produces a sharp, game-camera screenshot of the swarm from an unfocused PIE session, demonstrated by capturing one.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: evidence-capture
decided: "2026-07-27 done"
---

## Why now
Every visual task in this repo hands back on-screen evidence, and the capture path is
broken in three separate ways. This is now measured, not suspected — two consecutive
teammates hit it independently:

- **`Swarm.DebugShotAfter` does not fire from an unfocused PIE window.** task-047 confirmed
  via `GetLogEntries` that sim time was genuinely advancing (spawn logs, a full auto-fight
  running to completion) while the screenshot never landed. So it is not a frozen-sim
  problem; the capture path itself needs viewport focus/paint.
- **`CaptureViewport` renders the flame and the post-process but ZERO swarm sprites** at any
  height tried. Whatever draws the swarm is tied to the actual game camera, not the generic
  editor viewport capture. Only `CaptureEditorImage` (desktop screenshot) sees units.
- **Desktop capture is too low-resolution to certify detail.** task-047 could prove the
  spawn arc and the facing-tracking, but explicitly could not certify whether the brood
  formed legible *ranks* — the whole point of the task — because units land at roughly
  8-16px in a PIE window squeezed into part of the desktop.

The cost is concrete: task-045 could not capture a moving camera or a 0.6s hit-flash at all
and closed with its headline fix unverified; task-047 closed with its ranks half unproven.
Both spent substantial time discovering the same walls. Every future task that has to *show*
something pays this again.

There is also a documented landmine worth capturing while in here: clicking into the PIE
viewport via `SlateInspectorToolset` **ejects the PIE session outright** (task-047).

## Done when
- An agent can get a **sharp, game-camera screenshot of the swarm** from a PIE session it is
  driving, without the window needing OS focus. One documented command or short recipe.
- Whichever of the three walls is cheapest to fix is fixed; the others are documented with
  the reason they were not. Do not fix all three by brute force if one good path exists.
- `Swarm.DebugShotAfter` either works unfocused or its doc-comment says plainly that it does
  not and points at what does.
- The working recipe is written into `docs/AGENT-TEAMS.md` so spawn prompts can reference it
  instead of each teammate rediscovering it.
- The `SlateInspectorToolset`-ejects-PIE landmine is documented in the same place.

## Spawn prompt
```
You are executing task-048 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight
branch.

PROBLEM: agents cannot reliably screenshot what they build. This repo's standard is on-screen
evidence — "a diff plus it works" is explicitly not accepted — and the capture path is broken
three ways. Two teammates hit all three independently, so this is measured, not theoretical.

THE THREE WALLS, with what is already known:
1. Swarm.DebugShotAfter (defined ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp:95) does
   NOT fire from an unfocused PIE window. task-047 confirmed via GetLogEntries that sim time
   was genuinely advancing — spawn logs, a full auto-fight running to completion — while no
   screenshot ever landed. So the sim is not frozen; the capture path needs viewport
   focus/paint. Find out what it actually requires.
2. CaptureViewport (level viewport with an overridden transform) shows the flame pool and the
   demichrome post-process but ZERO swarm sprites, at any height tried. The swarm appears tied
   to the real game camera rather than the generic editor viewport. CaptureEditorImage
   (desktop screenshot) DOES show units. Understand why before choosing a fix — this is
   probably the most informative of the three.
3. Desktop capture resolution is too low to certify detail. Units land at ~8-16px in a PIE
   window occupying part of the desktop, which is why task-047 could prove its spawn arc but
   could NOT certify whether the brood formed legible ranks — the actual point of that task.

WHAT SUCCESS LOOKS LIKE: one documented command or short recipe that gets an agent a sharp,
game-camera screenshot of the swarm from a PIE session it is driving, with no OS focus
required. That is the deliverable. Fix the cheapest wall that gets you there — do NOT
brute-force all three if one good path exists. Document the ones you did not fix and why.

Ideas worth evaluating, not prescriptions: making the DebugShotAfter path use a capture that
does not need paint; widening Emberkeep.Cam.OrthoWidth during capture so units are not
sub-pixel; rendering the shot at a fixed high resolution independent of window size; or a
dedicated SceneCapture that films the game camera on demand. Pick on merit and say why.

ALSO DOCUMENT, in docs/AGENT-TEAMS.md: the working recipe (so spawn prompts can point at it
rather than each teammate rediscovering these walls), and this landmine from task-047 —
clicking into the PIE viewport via SlateInspectorToolset EJECTS the PIE session outright.

CONTEXT WORTH HAVING: L_Spike1 runs its own auto-fight harness at BeginPlay (Spike1GameMode
territory) that overrides spawn counts set via Saved/SwarmExecOnPlay.txt a moment after that
file's ACTIONS section executes. That surprised task-047. If your recipe involves driving
specific counts, account for it or document the workaround.

CONSTRAINTS:
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS AND THEN CRASHES THE NEXT PIE. Use
  `pwsh Scripts/ue-relaunch.ps1` for layout changes; Scripts/ue-iterate.ps1 picks the path.
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are in-memory until save_assets([]).
- If you change Saved/SwarmExecOnPlay.txt to test, restore it. Do not leave tuning dirty —
  the owner's values there are deliberate (several are marked owner-tuned).

YOU OWN: ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp and docs/AGENT-TEAMS.md.
If the fix genuinely needs a file outside that, STOP and say so in your handback rather than
widening scope — other tasks hold locks on the UI and Mass layers.

DO NOT TOUCH: ELVTR/Source/ELVTR/Mass/**, ELVTR/Source/ELVTR/UI/**, ELVTR/Content/**,
GDD.md, CLASSES.md, SYSTEMS.md, docs/design/**, or any docs/backlog/ file.

EVIDENCE: demonstrate the recipe by using it — capture one sharp game-camera screenshot of
the swarm from an unfocused PIE session and hand it back. That single image IS the proof this
task worked. If you cannot get there, say so plainly and hand back what you learned about
each of the three walls; a well-documented negative result is worth more here than a
workaround nobody can repeat.

HAND BACK: which wall you fixed and why that one, the recipe exactly as another agent should
run it, your demonstration screenshot, and what you found out about the walls you left alone.
```
