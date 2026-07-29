---
id: 004
title: Spec the per-floor encounter budget table
status: done
agent: gameplay-director
owns: ["docs/design/encounter-budget.md", "docs/data/encounter-budget.json", "docs/data/encounter-budget.schema.md"]
resources: []
depends-on: [2, 3]
evidence: docs/data/encounter-budget.json with a spend-per-room-type table, plus a simulated floor walkthrough showing density staying inside the measured entity budget.
score: {feel: 3, risk: 2, cost: 2}
source: docs/GDD-TODO.md:88
decided: "2026-07-29 done"
model: sonnet
teammate: encounter-budget
---

## Why now
The procgen floor generator (task-025) consumes this table as its constraint input — it
cannot be built against nothing. It is also the doc that ties design intent to the
measured entity budget in `docs/perf/BUDGETS.md`, which is currently the only place
real numbers exist.

## Done when
- Density, wave composition, and spike/breather rhythm per floor, spending against a
  stated budget rather than free-listing spawns.
- Encounter-budget spend per room type, in the form the floor generator will read.
- Cross-checked against `docs/perf/BUDGETS.md` — a budget that exceeds the measured
  entity ceiling is a broken budget, and the spec must say what it assumes.
- `## Simulation notes` walking one floor end to end.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the per-floor encounter budget table called for by SYSTEMS.md §4 and
docs/RTS-VERTICAL-SLICE.md:72 — density, wave composition, spike/breather pacing, and
encounter-budget spend per room type.

Read first: docs/design/entity-tiers.md + docs/design/scaling-curve.md (tasks 002/003 —
this depends on both), GDD.md §9 (procgen), docs/perf/BUDGETS.md for the MEASURED entity
ceiling, and docs/GATE1-FUN-PROTOTYPE.md for the wave/breather structure already shipped
(3s deploy → 250 → breather → 450 → breather → 700).
Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY:
  docs/design/encounter-budget.md
  docs/data/encounter-budget.json
  docs/data/encounter-budget.schema.md
Do not edit SYSTEMS.md, GDD.md, ELVTR/Source, ELVTR/Content, or the task-002/003 data files.

Hard constraint: your budget must fit inside the measured entity ceiling in
docs/perf/BUDGETS.md. If it does not, say so explicitly rather than quietly assuming
headroom. End with `## Simulation notes` walking one floor end to end.
```
