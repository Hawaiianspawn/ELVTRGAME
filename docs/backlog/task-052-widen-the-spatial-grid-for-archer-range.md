---
id: 052
title: Widen the spatial grid so archers can actually reach 750uu, and measure what it costs
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.h", "ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp", "docs/perf/grid-cell-size.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A measured before/after frame-cost comparison at matched entity counts showing what the wider grid costs, taken with bThrottleCPUWhenNotForeground disabled, plus proof that a 750uu engagement actually resolves where it previously could not.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: grid-reach
decided: "2026-07-27 done"
---

## Why now
task-049 specced Archers with `EngageRange` 750uu as the thing that distinguishes them from
Spearmen. task-021 then found that **750uu does not work**: the spatial grid is
`GridCellSize = 200` (`SwarmSubsystem.h:28`), and a 3×3 neighbourhood search therefore reaches
only ~600uu. `Swarm.BroodAggroRange`'s own doc-comment already says so —
*"Capped in practice by the 3x3 grid reach (~600uu at GridCellSize 200), so values beyond that
read the same. [0..600]"* — and it is clamped to `[0..600]` for exactly this reason.

So an archer set to 750uu would silently behave as if set to 600, and the design's headline
distinction would be quietly smaller than specced.

The owner chose to **widen the grid** rather than cap archers at 600.

**The cost is the open question, and it is the one thing task-021 did not measure.** Its
sweep found frame time flat from 120 to 814 entities at the *current* cell size. A bigger cell
means more entities per cell, and every neighbour query tests against all of them — so this
change moves a cost that measurement did not cover. That is the whole point of this task: not
just to make 750 work, but to find out what it costs.

## Done when
- The 3×3 neighbourhood reach is **≥750uu**, so an archer at that range genuinely engages.
  `GridCellSize 250` gives exactly 750; justify whatever you pick.
- **Measured before/after** frame cost at matched entity counts, using task-021's methodology
  and its numbers as the baseline (120 / 370 / 570 / 814 entities; 8.34 / 8.42 / 8.33 / 8.71ms
  avg; 283 draw calls flat). Same method, same waves, so the comparison is real.
- `Swarm.BroodAggroRange`'s doc-comment and its `[0..600]` clamp are **corrected** — both are
  now wrong, and the clamp will actively prevent the new reach from being used.
- A short `docs/perf/grid-cell-size.md` recording the tradeoff: reach vs. entities-per-cell,
  the measured cost, and the point at which widening stops being worth it.
- **An honest verdict.** If the cost turns out to be material, say so plainly and recommend
  capping archers at 600 instead — the owner picked widening believing it was affordable, and
  they would rather learn it is not than have it shipped quietly.

## Spawn prompt
```
You are executing task-052 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

THE PROBLEM. docs/design/squad-group-system.md (task-049) specs Archers with EngageRange 750uu
as the thing that distinguishes them from Spearmen. That number does not work. The spatial grid
is GridCellSize = 200 (ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h:28), so a 3x3 neighbourhood
search reaches only ~600uu. Swarm.BroodAggroRange's own doc-comment
(SwarmProcessors.cpp:64-70) states this and clamps itself to [0..600] because of it. An archer
set to 750 would silently act as if set to 600.

THE OWNER'S DECISION: widen the grid so 750 genuinely works, rather than capping archers at 600.

WHAT TO DO:
1. Widen the grid so the 3x3 reach is >=750uu. GridCellSize 250 gives exactly 750. Justify what
   you pick. Note GridCellSize is a `static constexpr float` on USwarmSubsystem — check whether
   anything else derives from it before changing it, and say what you found.
2. Fix Swarm.BroodAggroRange. Its doc-comment says "~600uu at GridCellSize 200" and its range is
   clamped [0..600]. BOTH are now wrong, and the clamp will actively prevent the new reach from
   being usable. Update the text to match reality and widen the clamp appropriately.
3. MEASURE THE COST. This is the real deliverable, not the cell-size edit.

MEASUREMENT — task-021 (docs/perf/squad-aggregation.md) established the baseline and the method.
Read it first and reuse both so the comparison is apples to apples:
- Method: Spike1GameMode's own auto-fight wave progression as the population driver (120 retinue;
  waves of 250/450/700 brood), stock UE CSV Profiler via `CsvProfile Start` / `CsvProfile Stop`
  console commands, cross-referenced against Swarm.SpacingLogInterval log reports for exact counts.
- Baseline to beat or match, at GridCellSize 200: 120 entities 8.34ms avg / 370 8.42 / 570 8.33 /
  814 8.71. Draw calls flat at 283 throughout.
- Measure the SAME points at your new cell size and compare directly.

**CRITICAL, READ docs/AGENT-TEAMS.md §8a BEFORE MEASURING ANYTHING.**
EditorPerformanceSettings.bThrottleCPUWhenNotForeground defaults to TRUE and caps the engine to
~3fps the instant an unfocused PIE starts — which is every agent-driven PIE run. task-021's first
capture was a flat 333ms/frame with no correlation to entity count; that flat-and-uncorrelated
signature is what a contaminated run looks like. Disable it on the live CDO via
ObjectTools.set_properties (no restart needed), and RESTORE IT when you are done. If you skip
this your numbers are fiction and this whole task is wasted.

Also note: Spike1GameMode::RestartRun() wipes any manual Swarm.SpawnBrood override from
Saved/SwarmExecOnPlay.txt almost immediately, so you cannot drive population that way on
L_Spike1 — use the harness's own waves, as task-021 did.

4. Prove the range actually works now: show a 750uu engagement resolving where it previously
   could not. A log-based demonstration is acceptable here if a screenshot cannot show reach.

5. Write docs/perf/grid-cell-size.md: the reach-vs-entities-per-cell tradeoff, your measured
   cost, and where widening stops being worth it.

BE WILLING TO REPORT THAT THIS WAS A BAD TRADE. The owner chose widening believing it was
affordable. If your measurements say otherwise, say so plainly and recommend capping archers at
600uu instead. A measured "don't do this" is exactly as valuable as a measured "this is fine" —
task-021's headline was "do not build the aggregation layer" and that was its best contribution.

CONSTRAINTS:
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS THEN CRASHES THE NEXT PIE. GridCellSize is a
  static constexpr on a UObject-derived class, so treat this as a layout-affecting change and use
  `pwsh Scripts/ue-relaunch.ps1`. Scripts/ue-iterate.ps1 picks the path automatically.
- unreal-mcp is on PORT 9000, not 8000.
- If you change Saved/SwarmExecOnPlay.txt, RESTORE IT. Several values are owner-tuned and
  deliberate (Swarm.Formation.Spacing 42.4, the UnitCamProj Fov/Height/Pitch block,
  Swarm.UnitShading 0, and the whole HORDE ARRIVAL section from task-047).

YOU OWN: SwarmSubsystem.h, SwarmProcessors.cpp/.h, SwarmCombatProcessors.cpp (all under
ELVTR/Source/ELVTR/Mass/), and docs/perf/grid-cell-size.md.

DO NOT TOUCH: ELVTR/Source/ELVTR/UI/** (task-050 is live in there right now),
ELVTR/Source/ELVTR/Rendering/**, ELVTR/Content/**, docs/design/** (task-049's), 
docs/perf/squad-aggregation.md (task-021's, done — cite it, do not edit it),
docs/data/art/** and .claude/skills/art-coverage/** (task-051 is live in there), GDD.md,
CLASSES.md, SYSTEMS.md, or any docs/backlog/ file.

HAND BACK: the cell size you chose and why, your before/after numbers with the method stated,
confirmation you disabled AND restored the CPU throttle, proof a 750uu engagement resolves, what
you found deriving from GridCellSize, and your honest verdict on whether this was worth it.
```
