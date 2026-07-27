---
id: 007
title: Run Spike 1 and fill SPIKE1-RESULTS.md with measured numbers and a verdict
status: proposed
agent: performance-director
owns: ["docs/SPIKE1-RESULTS.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: SPIKE1-RESULTS.md with every table cell filled from a real -SwarmBench run, machine specs recorded, and exactly one of GO / ADJUST / KILL ticked.
score: {gate: 3, risk: 3, cost: 2}
source: docs/SPIKE1-RESULTS.md:31
decided: ""
---

## Why now
`GDD-TODO.md:93` calls Spike 1 "the project's biggest technical risk", and the results
doc is completely blank — no machine, no numbers, no verdict. Seven measurement rows are
empty and the verdict is three unticked boxes. The whole entity-count premise of the
game is currently unevidenced, and `docs/RTS-VERTICAL-SLICE.md` gates Spike 2
(networking) behind this answer.

This is the highest-risk-retiring task on the board. The harness already exists —
`-SwarmBench`, `SwarmRenderActor.cpp:448-527` — so this is a measurement run, not a
build.

## Done when
- Machine (CPU/GPU/RAM) and build date recorded.
- All seven rows measured: 500 / 1,000 / 2,000 / 5,000 / 10,000 entities, PIE and
  Standalone where the table asks for both.
- Observations and the top three game-thread costs from Unreal Insights at 5k.
- Exactly one verdict ticked: GO, ADJUST, or KILL / RETHINK — with the reasoning.
- Numbers reconciled against `docs/perf/BUDGETS.md`; if they disagree, say which is right.

## Spawn prompt
```
You are the performance-director for Emberkeep (C:\Projects\ELVTRGAME).

Run Spike 1 and fill in docs/SPIKE1-RESULTS.md, which is currently an empty template.

The benchmark harness already exists: the -SwarmBench command line and
ELVTR/Source/.../Rendering/SwarmRenderActor.cpp:448-527. Swarm.* console commands work.
Map is Content/Spike1/L_Spike1.

Read first: docs/SPIKE1-RESULTS.md (the template you are filling),
docs/perf/BUDGETS.md (the only existing measured numbers), docs/GATE1-FUN-PROTOTYPE.md,
and docs/perf/niagara-sprite-refactor.md — but note that file's §2 and §8.1 are STALE
and corrected at the top; the sprite emitter's zero-draw root cause was
GPUComputeSim vs CPUSim, already fixed.

You own the Unreal editor for this task — no other task may drive it concurrently.

Measure all seven rows (500/1k/2k/5k/10k, PIE and Standalone as the table asks). Record
machine specs. Capture the top three game-thread costs at 5k from Unreal Insights. Then
tick exactly ONE verdict — GO, ADJUST, or KILL/RETHINK — and justify it.

Write ONLY docs/SPIKE1-RESULTS.md. Do not edit source, content, GDD.md, or SYSTEMS.md.
If your numbers contradict docs/perf/BUDGETS.md, say so explicitly and state which
measurement you trust and why. Do not report a verdict you did not measure.
```
