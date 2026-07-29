---
id: 021
title: Measure the per-entity cost and design the group-as-cost-boundary (squad aggregation)
status: done
agent: performance-director
owns: ["docs/perf/squad-aggregation.md"]
resources: ["unreal-editor"]
depends-on: []
evidence: docs/perf/squad-aggregation.md carrying MEASURED per-entity render+sim cost at slice density (not estimated), the LOD swap threshold with the numbers behind it, and a recommendation that may be "don't build this".
score: {feel: 2, risk: 3, cost: 3}
source: docs/RTS-VERTICAL-SLICE.md:95
teammate: cost-boundary
decided: "2026-07-27 done"
---

## Why now
The owner settled what grouping is actually *for*, 2026-07-27:

> *"What I am trying to do is manage and update expense for the amount of units in the game.
> The grouping is what helps reduce runtime costs when we reach extreme scale."*

That reframes the whole squad line of work. `docs/design/squad-group-system.md` (task-049,
done) specs grouping as a **command surface** — typed Total War units you give orders to.
That is real and stays. But it is the *rider*, not the driver. The driver is **cost**: the
group is the boundary at which the sim stops paying per soldier.

This task defines that boundary, and it is the thing task-046 (the sim implementation) has
to be built against. Building the command layer without knowing where the cost boundary
sits means putting it in the wrong place and rebuilding.

**The dependency on task-007 was dropped deliberately** (owner's call, 2026-07-27): rather
than run all of Spike 1 first, this task measures exactly the thing its own design turns on.
`SPIKE1-RESULTS.md` stays empty and task-007 remains open.

**Measurement is now actually possible for a director with no MCP tools** — task-048 (done)
built an agent-drivable capture and PIE path documented in `docs/AGENT-TEAMS.md` §8, driven
over plain PowerShell via `Scripts/ue-mcp.ps1`.

## Done when
- **Measured**, not estimated: current per-entity render + sim cost at slice density. State
  the machine, the method, and the counts.
- **The cost boundary is defined**: at what scale a group stops being N simulated soldiers
  and becomes one entity that *represents* N. What the group entity holds, how it expands to
  N sprites, what happens on partial casualties, and how combat addresses members across the
  swap.
- **The swap is specified as a threshold with numbers behind it**, not a vibe — what triggers
  it, whether it is per-group or global, and whether it can flip back.
- **The Unit Cam constraint is respected**: the owner wants the panel to show *real units*
  almost always. Say what the aggregation does to that — at what point the panel is showing a
  proxy rather than soldiers, and whether the player can tell.
- **An honest recommendation, including "don't build this"** if the per-entity path holds at
  target density. Design law 5 cuts both ways: cheap shared archetypes are the point, and an
  aggregation layer that reintroduces per-unit special-casing is a broken design.

## Spawn prompt
```
You are the performance-director for Emberkeep (C:\Projects\ELVTRGAME), on the
flame-spotlight branch.

THE OWNER SETTLED WHAT GROUPING IS FOR, 2026-07-27, in their words:

  "What I am trying to do is manage and update expense for the amount of units in the game.
   The grouping is what helps reduce runtime costs when we reach extreme scale."

And separately: "The retinue units should almost always be their unit in the unit cam."

So: the group is a COST BOUNDARY first. Commanding units is a benefit that rides on the same
grouping, not the reason for it. Your job is to define that boundary with measured numbers.

READ FIRST:
- docs/design/squad-group-system.md (task-049, done) — the typed-unit design: v1 is exactly
  Spearmen and Archers, units are permanent and typed at recruit time, MaxSquads 8. Its §8
  "Performance requests" was written FOR YOU. Read it and answer it. Note it specs grouping
  as a command surface; that is not wrong, it is just not the driver. Do not rewrite it —
  you own docs/perf/, not docs/design/.
- GDD.md §10 (entity architecture — only elites/titans/bosses promote to full Actors).
- docs/perf/BUDGETS.md.
- The Mass processor sources under ELVTR/Source/ELVTR/Mass/ and the render bridge in
  ELVTR/Source/ELVTR/Rendering/.

MEASURE FIRST, DESIGN SECOND. Your evidence bar is measured numbers, not estimates. You hold
the unreal-editor lock. You have NO MCP tools, but you do NOT need them — task-048 just built
an agent-drivable path, documented in docs/AGENT-TEAMS.md §8: drive PIE and capture over
plain HTTP via Scripts/ue-mcp.ps1's Invoke-McpTool (StartPIE / StopPIE / GetLogEntries), and
Swarm.DebugShotAfter now renders at a fixed 1920x1080 without needing window focus. Read §8
before you start — it will save you hours, and it documents the traps (DebugRender must be 0
or captures show no units; SlateInspectorToolset ejects PIE on click-in).

Useful existing instrumentation: there is a -SwarmBench path (a clean A/B was run
2026-07-26 measuring UnitShading's two-box split at 2.21x draw cost at 2000 brood), and
Swarm.SpawnBrood / Swarm.SpawnRetinue console commands. Note L_Spike1 runs its own auto-fight
harness at BeginPlay that overrides spawn counts set via Saved/SwarmExecOnPlay.txt.

THEN DESIGN THE BOUNDARY:
1. At what scale does a group stop being N simulated soldiers and become one entity that
   REPRESENTS N? Give the threshold and the numbers behind it. Per-group or global? Can it
   flip back as casualties reduce the count?
2. What does the group entity hold, how does it expand to N sprites, what happens on partial
   casualties, and how does combat address individual members across the swap?
3. THE UNIT CAM CONSTRAINT — the owner wants the panel showing REAL units almost always.
   task-045 shipped an Army View that draws <=8 aggregate blocks; task-049 measured that a
   405-837px panel cannot show legible individual soldiers across a leash-bound army at any
   zoom (~6-13px sprites). Say what your aggregation does to that: at what point is the panel
   showing a proxy rather than soldiers, and can the player tell? This is where cost and the
   owner's stated preference actually collide — do not paper over it.
4. What this costs in complexity, honestly. Design law 5: an aggregation layer that
   reintroduces per-unit special-casing at horde scale is a broken design.

RECOMMEND, including "DO NOT BUILD THIS" if your measurements say the per-entity path holds
at target density. That is a legitimate and valuable outcome — aggregation bought without
need is pure complexity, and you are the only one positioned to say so with numbers.

Write ONLY docs/perf/squad-aggregation.md.

DO NOT TOUCH: ELVTR/Source/**, ELVTR/Content/**, docs/design/** (task-049's territory),
GDD.md, CLASSES.md, SYSTEMS.md, or any docs/backlog/ file. Implementation is a separate task
the owner approves after reading yours. If you change Saved/SwarmExecOnPlay.txt to drive a
measurement, RESTORE IT — several values there are owner-tuned and deliberate.

CANON WARNINGS:
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 are STALE: they claim the swarm emitter
  draws zero particles. The cause was GPUComputeSim vs CPUSim and it is fixed. Do not build
  on those two sections.
- WORLD.md is superseded by the 2026-07-22 reset; current canon is
  docs/narrative/FLAME-FOUNDATION.md.

STATE YOUR ASSUMPTIONS. If you did not measure something, say you did not measure it — this
project has been bitten repeatedly by confident claims that turned out to be untested.

HAND BACK: your measured numbers with machine and method, the cost boundary and its
threshold, what happens to the Unit Cam's real-units-by-default preference, your complexity
assessment, and your recommendation.
```
