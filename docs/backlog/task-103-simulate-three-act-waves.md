---
id: 103
title: Turn the three-act curve into runnable scenarios and report what the harness actually says
status: done
agent: sim-director
model: sonnet
owns:
  - "docs/data/scenarios/three-act-early.json"
  - "docs/data/scenarios/three-act-mid.json"
  - "docs/data/scenarios/three-act-late.json"
  - "docs/sim/THREE-ACT-WAVES.md"
resources: []
depends-on: [102]
epic: three-act-waves
evidence: >
  Three schema-conformant scenarios under `docs/data/scenarios/` reproducing
  task-102's early/mid/late curve, all passing `py Scripts/sim/scenario_runner.py
  --all` and `py Scripts/sim/validate.py` with no new validation failures, plus
  `docs/sim/THREE-ACT-WAVES.md` reporting the run with the LIMITATIONS §1 caveat
  attached to every survivor number. The report must state which of the three waves
  the harness can say something trustworthy about and which it cannot, and must name
  any place task-102's numbers contradict a committed data file.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: three-act-sim
decided: "2026-07-30 done"
---

## Why now

Task-102 produces a curve on paper. Committing it as three runnable fixtures is what
makes it checkable later, and running it now surfaces the contradictions early —
`task-096` only found the supply-capacity collision because someone finally ran two
independently-written documents through the same model.

The scoring is deliberately `feel: 1`. This task does not change how anything plays;
it turns a design doc into fixtures and a report. The gameplay change lives in
task-102, and the fun question is answered in PIE, not here.

## Done when

- Three scenarios exist under `docs/data/scenarios/`, conforming to
  `scenarios.schema.md`, reproducing task-102's numbers exactly with `SourceRefs`
  pointing at `docs/design/wave-scaling-three-act.md` and `docs/data/wave-scaling.json`.
- `py Scripts/sim/scenario_runner.py --all` and `py Scripts/sim/validate.py` both run
  clean — no new failures, and the existing baseline in `docs/sim/baseline.json` is
  unchanged by these additions.
- `docs/sim/THREE-ACT-WAVES.md` reports what came out, with every wave-attrition
  survivor count carrying the `LIMITATIONS.md` §1 caveat inline, not in a footnote.

## Scope notes

**You are not a second gameplay director.** Task-102's numbers are the input. If one
looks wrong, **report it, do not fix it.** Do not edit `docs/design/**`, `SYSTEMS.md`,
`docs/data/wave-scaling.json`, or any tier stat block. Do not tune a constant in
`combat-model-constants.json` to make a wave land somewhere nicer —
`LIMITATIONS.md` §1 is explicit that this is the one thing not to do.

**Expect the late wave to be outside the model's range and say so.** The harness has
never been run at anything near the population task-102 is likely to name. If the
pooled attrition model degenerates, produces nonsense, or takes unreasonably long at
that scale, that is a **finding worth reporting**, not a failure to hide. Report the
scale at which it stops meaning anything.

**Ranged, not projectiles.** Model the ranged share with the existing
`brood_soldier_ranged` tier and the Archer retinue type, the way
`floor2-ranged-wave.json` already does. Nothing in the harness models projectile
travel or misses, and you are not adding it.

**Do not chain the waves unless task-102 says to.** `run-slice-three-wave.json`
already exists as the carryover chain over the GATE1 fixtures. If task-102 specifies
retinue carryover between waves, note that a chained companion run is the natural
follow-up and **file it as a question in the report** — do not build it here.

## Spawn prompt

```
You are the sim-director. Task-102 has landed a three-act wave curve (early / mid /
late) for Kindled. Turn it into runnable fixtures and report what the harness says.

READ FIRST — task-102's output is your input:
  docs/design/wave-scaling-three-act.md
  docs/data/wave-scaling.json
Then:
  docs/data/scenarios/scenarios.schema.md      — the shape you must conform to
  docs/data/scenarios/gate1-calibration-wave1.json — the closest existing model,
      including how ArrivalSeconds per-rank rows are cited
  docs/data/scenarios/floor2-ranged-wave.json  — how ranged enemies are expressed
  docs/sim/LIMITATIONS.md                      — READ ALL OF IT before reporting a
      single number. §1 in particular.
  docs/sim/VALIDATION.md, docs/sim/README.md

WHAT YOU OWN — write only these four files:
  docs/data/scenarios/three-act-early.json
  docs/data/scenarios/three-act-mid.json
  docs/data/scenarios/three-act-late.json
  docs/sim/THREE-ACT-WAVES.md

WHAT YOU MUST NOT TOUCH:
  docs/design/**, SYSTEMS.md, docs/data/wave-scaling.json — task-102 owns the design.
      If one of its numbers looks wrong, REPORT it, do not fix it.
  docs/data/entity-tiers.json, unit-types.json, economy.json — not yours.
  docs/data/scenarios/combat-model-constants.json — do NOT tune a constant to make a
      wave land somewhere nicer. LIMITATIONS.md §1 is explicit about this: every one
      of those values is a documented/cited/midpoint number, and picking one because
      it produces a pleasing result trades one bad number for a worse one.

DO:
  1. Author the three scenarios from task-102's numbers exactly. SourceRefs must
     point at docs/design/wave-scaling-three-act.md and docs/data/wave-scaling.json.
  2. Run: py Scripts/sim/scenario_runner.py --all   and   py Scripts/sim/validate.py
     Both must come back clean — no NEW failures, and docs/sim/baseline.json must not
     be perturbed by these additions. If a pre-existing failure is already on record
     in VALIDATION.md, say which and move on.
  3. Write docs/sim/THREE-ACT-WAVES.md reporting the result.

THE REPORT MUST:
  - Attach the LIMITATIONS.md §1 caveat INLINE to every wave-attrition survivor
    count — not in a footnote at the bottom. The wave model does not reproduce its
    one measured baseline (it predicts a wipe where GATE1 measured 109-of-120
    surviving), so survivor numbers here are DIRECTIONAL, not predictive. A reader
    who skims must not come away thinking these are predictions.
  - State plainly which of the three waves the harness can say something trustworthy
    about and which it cannot.
  - Report the scale at which the pooled model stops meaning anything. The late wave
    is very likely far beyond any population this harness has ever been run at. If it
    degenerates, hangs, or produces nonsense, that IS the finding — report the
    population where it breaks down and why, do not hide it or work around it.
  - Name every place task-102's numbers contradict a committed data file. This is the
    highest-value thing you can produce here: task-096 found the supply-capacity
    collision purely by being the first thing to run two independently-written
    documents through one model.

ON RANGED: model the ranged share with the existing brood_soldier_ranged tier and the
Archer retinue type, as floor2-ranged-wave.json already does. Nothing in the harness
models projectile travel time or misses; do not add it.

ON CHAINING: run-slice-three-wave.json already chains the GATE1 fixtures with
carryover. If task-102 specifies retinue carryover between waves, note in the report
that a chained companion run is the natural follow-up — but do NOT build it here.

HAND BACK: the four file paths, the three-wave result table, which waves you consider
trustworthy and which you do not, the population at which the model breaks down, and
every contradiction you found between task-102's numbers and committed data.
```
