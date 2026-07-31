---
id: 120
title: Refresh SWEEPS.md's two demonstration tables, which disagree with baseline.json after the armor fix
status: proposed
agent: sim-director
model: ""
owns: ["docs/sim/SWEEPS.md", "docs/sim/baseline.json"]
resources: []
depends-on: []
epic: ""
evidence: >
  `docs/sim/SWEEPS.md` Demonstrations 1 and 2 print the numbers the current harness
  actually produces, each cell reproducible from the stated command, and
  `baseline.json`'s `B-floor1-spearmen-count-breakpoint` trust string no longer claims
  an exact reproduction of a table that says something different. `py
  Scripts/sim/drift_check.py` still passes.
score: {feel: 1, risk: 2, cost: 1}
source: task-075 handback
teammate: ""
decided: ""
---

## Why now

`docs/sim/SWEEPS.md`'s two worked demonstrations are stale, and the file that is
supposed to agree with them does not. Measured by the task-075 teammate:

| Count | SWEEPS.md Demo 2 | harness + baseline.json now |
|---|---|---|
| 32 | 138.62 | **142.59** |
| 64 | 102.14 | 107.63 |
| 96 | 84.52 | 90.86 |
| 128 | 36.10 | 41.22 |
| 160 | 5.69 | 14.00 |
| 200 | 32.18 @ 27.0s | 17.63 @ 42.3s |
| 250 | 84.94 | 70.54 |

Every cell differs. Demonstration 1 is stale the same way (hp=900: SWEEPS.md 12.78,
baseline 10.97). The cause is almost certainly the 2026-07-29 armor fix
(`LIMITATIONS.md` §5 — armor stopped being hardcoded to zero in the wave model):
`baseline.json` was refreshed, SWEEPS.md's inline tables were not.

The part that makes this worth a task rather than a note: **`baseline.json`'s own
`B-floor1-spearmen-count-breakpoint` trust string still reads "Exact reproduction of
docs/sim/SWEEPS.md Demonstration 2 — same command, same 7 cells".** That claim is now
false in its printed values, so the file that exists to make results auditable is
itself asserting something untrue. A doc being out of date is ordinary; a provenance
string vouching for a number it no longer matches is the failure this harness exists
to prevent.

Reproduce with:

    py Scripts/sim/sweep.py floor1-swarm-wave --axis "scenario:Retinue.Composition[UnitType=spearmen].Count=32,64,96,128,160,200,250"

## Done when

- Both demonstration tables in `SWEEPS.md` print what the harness prints today, with
  the exact command above each one.
- Each table says which harness change moved it and when, so the next reader can tell
  a refresh from a regression.
- `baseline.json`'s trust string for `B-floor1-spearmen-count-breakpoint` describes
  what it actually reproduces.
- `py Scripts/sim/drift_check.py` still passes. **The baseline numbers themselves are
  not the thing being changed** — if drift appears, that is a real regression and it
  gets reported, not absorbed.

## Spawn prompt

```
You are executing task-120. You are the sim-director for Kindled
(C:\Projects\ELVTRGAME). Read docs/backlog/task-120-sweeps-demo-tables-disagree-with-baseline.md
first, then docs/sim/SWEEPS.md, docs/sim/baseline.json, and docs/sim/LIMITATIONS.md §5.

THE FINDING, from the task-075 teammate: SWEEPS.md's two worked demonstrations print
numbers the harness no longer produces. Demonstration 2's seven cells all differ from
what the current harness and baseline.json both give (32 -> SWEEPS.md 138.62, actual
142.59; 160 -> 5.69 vs 14.00; 200 -> 32.18 @ 27.0s vs 17.63 @ 42.3s). Demonstration 1
is stale the same way. Likely cause is the 2026-07-29 armor fix (LIMITATIONS.md §5).

VERIFY IT YOURSELF FIRST — do not refresh a table on my say-so:
    py Scripts/sim/sweep.py floor1-swarm-wave --axis "scenario:Retinue.Composition[UnitType=spearmen].Count=32,64,96,128,160,200,250"

WHAT MATTERS MOST HERE is not the table. baseline.json's
B-floor1-spearmen-count-breakpoint trust string still claims "Exact reproduction of
docs/sim/SWEEPS.md Demonstration 2 - same command, same 7 cells", which is now false.
Fix that string to describe what it actually reproduces. A provenance record vouching
for a number it no longer matches is worse than a stale table.

DO NOT REFRESH THE BASELINE NUMBERS. You are correcting a doc and a trust string. If
py Scripts/sim/drift_check.py reports drift, that is a real regression — report it as
a finding and stop, do not absorb it into a new baseline.

Above each refreshed table, state the exact command and which harness change moved the
numbers, so the next reader can tell a deliberate refresh from a regression.

YOU OWN ONLY: docs/sim/SWEEPS.md, docs/sim/baseline.json

DO NOT TOUCH: any file under Scripts/sim/, docs/sim/RUNSTORE.md, docs/sim/MODEL.md,
docs/sim/VALIDATION.md, docs/sim/LIMITATIONS.md, docs/data/**, or anything under ELVTR/.

HAND BACK: the sweep command's actual output, the before/after of both tables, the
corrected trust string, and drift_check.py's result.
```
