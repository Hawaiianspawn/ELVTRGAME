---
id: 089
title: Expose Swarm.BroodAdd and Swarm.RawNear through the /cvars tuning surface so the two dials that finally make the brood visible are actually tunable in play
status: done
agent: claude
model: sonnet
owns: ["ELVTR/Saved/SwarmExecOnPlay.txt", "Scripts/populate_cvar_preset.py", ".claude/skills/cvars/SKILL.md"]
resources: ["unreal-editor"]
depends-on: [84]
epic: ""
evidence: Both dials present in SwarmExecOnPlay.txt with help text, appearing in the in-engine Breadboard panel and the Console Variables Editor preset — shown as a screenshot of the panel with both fields visible and moving the brood on screen when dragged. The 158 existing entries unchanged.
score: {feel: 2, risk: 1, cost: 1}
source: brood-legibility (task-084 handback)
teammate: brood-cvars
decided: "2026-07-29 done"
---

## Why now
`task-084` made the brood visible and the two dials that do it — `Swarm.BroodAdd` (0.05) and
`Swarm.RawNear` (220) — **are not on the tuning surface.** Verified: zero matches for either
across all 158 CVar entries in `ELVTR/Saved/SwarmExecOnPlay.txt`.

Their C++ defaults apply, so the game looks right. But the Breadboard panel does not show them
and the Console Variables Editor preset does not carry them, which means the one dial that
controls whether the horde is legible cannot be tuned while the game runs — exactly the thing
that surface exists for. `BroodAdd` in particular is a look dial the owner will want to feel out
live, not recompile for.

`task-084`'s teammate deliberately did not add them: that surface is `/cvars`-owned and editing
it was outside its fence. Correct call, so this is the follow-up.

## Done when
- `Swarm.BroodAdd` and `Swarm.RawNear` are in `SwarmExecOnPlay.txt` with help text, in the
  section where the other flame/light dials live.
- Both appear in the in-engine Breadboard panel (`Emberkeep.Breadboard`) and in the Console
  Variables Editor preset via `Scripts/populate_cvar_preset.py`.
- A screenshot shows both fields in the panel, and dragging `BroodAdd` visibly changes the brood.
- The 158 existing entries are unchanged.

## Scope fence
- Do not retune any existing value. This is exposure only.
- Not `Swarm.FlameRadius` — whether the pool widens is an open owner look call from `task-084`,
  not a `/cvars` matter.
- `Scripts/populate_cvar_preset.py` must be run with `py` as editor Python; `unreal-mcp` cannot
  set CVars or populate that panel (read-only `SearchCVars`).

## Spawn prompt
```
You are executing task-089 (C:\Projects\ELVTRGAME). Read the task file, then invoke the /cvars
skill and follow it — this is a routine pass on the surface that skill owns.

task-084 shipped two new dials that make the brood visible, and neither is on the tuning
surface: Swarm.BroodAdd (default 0.05, the additive legibility floor) and Swarm.RawNear
(default 220, dissolves the light model back to authored art near the camera). Verified zero
matches for either across all 158 entries in ELVTR/Saved/SwarmExecOnPlay.txt. Add both, with
help text, alongside the existing flame/light dials.

Then refresh the downstream surfaces: the Breadboard panel (Emberkeep.Breadboard) parses that
same file, and the Console Variables Editor preset is populated by Scripts/populate_cvar_preset.py
— run it with `py` as EDITOR Python. unreal-mcp CANNOT set CVars or populate that panel
(SearchCVars is read-only, the sandbox cannot import unreal), so do not try to route it
through MCP.

DONE WHEN both fields show in the Breadboard panel, both are in the CVE preset, a screenshot
shows them, dragging BroodAdd visibly changes the brood on screen, and the 158 existing entries
are untouched.

DO NOT retune any existing value — this is exposure only. Do NOT touch Swarm.FlameRadius:
whether the flame pool widens is an open owner look decision from task-084, not a /cvars matter.
Read docs/RENDERING-LIGHTING.md §4e first so the help text you write matches what the dials
actually do — BroodAdd is ADDITIVE (it rides particle alpha), not another multiplier.

The tree is shared with concurrent sessions. Build on uncommitted work you find, do not revert
it, do not attribute it. HANDBACK to the lead with the screenshot path and the diff summary;
do not change the task's status yourself.
```
