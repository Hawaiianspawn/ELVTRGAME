---
id: 064
title: Fix the wave-clear stalemate — stray brood that never die stop the run advancing
status: done
agent: claude
model: sonnet
owns: ["ELVTR/Source/ELVTR/Spike/Spike1GameMode.cpp", "ELVTR/Source/ELVTR/Spike/Spike1GameMode.h", "ELVTR/Config/SwarmExecOnPlay.canonical.txt"]
resources: ["unreal-editor"]
depends-on: []
epic: ""
evidence: Ten consecutive PIE runs reaching wave 3 without manual intervention, with the run log showing each wave-clear firing, plus a stated diagnosis of why a handful of brood previously evaded retinue detection.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: wave-clear-stalemate
decided: "2026-07-29 done"
---

## Why now

`task-060` needed one wave-3 frame-time sample in each of two states. Getting them took
**six full PIE attempts and roughly forty minutes**, because five of the six stalemated at
wave 2. `ASpike1GameMode`'s wave-clear gate requires `GetAliveBrood() == 0` and has no
timeout, so when a handful of brood — measured at 1 to 6 out of 450 — evade retinue
detection and never die, the wave never clears and wave 3 never spawns. The run simply
stops advancing, with no failure state and no way out but restarting.

This is not a blood bug and it was correctly out of that task's file boundary. It is a
tax on every future task that needs to observe late-wave behaviour: `task-008` (Play Gate 1,
score 12.0) and `task-030` (CAMERA-SCALE questions) both have to reach the same densities,
and both will pay the same forty minutes unless this closes first.

The teammate also established that the obvious workaround does not exist:
`RestartRun()` in `Spike1GameMode::BeginPlay` runs *after* the render actor's exec file and
wipes it, so the `Swarm.Clear` + `SpawnRetinue` + `SpawnBrood` direct-density trick in
BenchExec cannot be used to skip the wave grind. That was confirmed empirically, not assumed.

## Done when

- The root cause of the evading brood is diagnosed and stated — whether it is pursuit range,
  spatial-grid cell boundaries, a leash interaction, or something else. A timeout alone that
  papers over a real pathing/detection bug is not the finish line, though a timeout may well
  be part of the fix.
- Reaching wave 3 is reliable. Ten consecutive PIE runs advance without manual intervention.
- If a timeout is added, it is a CVar with a prose doc-comment in the house style, tuned
  value in `ELVTR/Config/SwarmExecOnPlay.canonical.txt`.

## Spawn prompt

```
You are executing task-064 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

THE PROBLEM, measured by a previous teammate rather than reported by a player:
ASpike1GameMode's wave-clear gate requires GetAliveBrood() == 0 with no timeout. In practice
1-6 brood out of 450 evade retinue detection and never die, so the gate never fires, wave 3
never spawns, and the run stops advancing with no failure state. Five of six PIE attempts
stalemated at wave 2 this way, costing ~40 minutes to get a single wave-3 sample.

Start by reading ELVTR/Source/ELVTR/Spike/Spike1GameMode.cpp and finding the wave-clear
condition and GetAliveBrood(). Then reproduce it: run PIE, let wave 2 play out, and watch
whether the count stalls at a small non-zero number.

DIAGNOSE BEFORE YOU FIX. The interesting question is WHY a handful of brood become
unreachable. Candidates worth checking, none confirmed:
  - retinue pursuit/engage range vs. how far a brood can drift (unit-types.json has
    engage_range 95uu; the spatial grid was widened for archer range in task-052)
  - spatial-grid cell boundaries leaving a straggler in a cell nothing queries
  - a leash interaction pulling retinue back before they reach a straggler
  - brood that pathed somewhere the retinue will not follow
A timeout that hides a real detection bug is not a fix. A timeout ON TOP of a diagnosed and
addressed root cause is fine, and is probably wanted anyway as a safety valve.

YOU OWN EXACTLY:
  ELVTR/Source/ELVTR/Spike/Spike1GameMode.cpp
  ELVTR/Source/ELVTR/Spike/Spike1GameMode.h

DO NOT WRITE ANYTHING ELSE. In particular do NOT touch, even if the root cause turns out to
live there — report it instead and stop:
  ELVTR/Source/ELVTR/Mass/**            (task-054 territory; SwarmFragments.h also task-059's)
  ELVTR/Source/ELVTR/Rendering/**       (task-059 territory)
  ELVTR/Config/SwarmExecOnPlay.canonical.txt is the ONE exception — you may add a CVar
  default there if your fix introduces one, and nothing else in that file.
If the real fix belongs in the Mass pursuit code, that is a genuinely useful finding: write
it up precisely, with file and line, and hand back rather than reaching across the boundary.

KNOWN DEAD END, do not spend time rediscovering it: the BenchExec direct-density shortcut
(Swarm.Clear + SpawnRetinue + SpawnBrood) does NOT work here. RestartRun() in
Spike1GameMode::BeginPlay runs after the render actor's exec file and wipes it. Confirmed
empirically by the previous teammate.

ENGINE TRAPS:
- Adding a UPROPERTY via Live Coding reports success then crashes the next PIE. Class-layout
  changes need a full editor-closed rebuild. Plan your members up front.
- unreal-mcp asset edits are in memory until save_assets([]).

EVIDENCE REQUIRED: ten consecutive PIE runs reaching wave 3 with no manual intervention, the
run log showing each wave-clear firing, and a stated diagnosis of why brood were evading.
If you cannot get ten clean runs, say how many you got and what the failures looked like —
a partial result reported honestly is worth more than a clean claim.

HAND BACK the diagnosis first and the fix second. State plainly anything you could not do.
Do not run any `py Scripts/backlog.py` commands — the lead session owns backlog transitions.
```
