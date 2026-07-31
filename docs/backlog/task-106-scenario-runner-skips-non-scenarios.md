---
id: 106
title: Make scenario_runner --all skip non-scenario files instead of crashing on a bare KeyError
status: proposed
agent: sim-director
model: ""
owns:
  - "Scripts/sim/scenario_runner.py"
  - "Scripts/sim/data_loader.py"
resources: []
depends-on: []
epic: ""
evidence: >
  `py Scripts/sim/scenario_runner.py --all` runs every real scenario in
  `docs/data/scenarios/` to completion, skipping files with no `Kind` field with a
  readable one-line notice rather than raising `KeyError('Kind')`.
score: {feel: 1, risk: 1, cost: 1}
source: task-103 handback, 2026-07-30
teammate: ""
decided: ""
---

## Why now

`scenario_runner.py --all` iterates every `.json` in `docs/data/scenarios/` and does
`scenario["Kind"]` unguarded at `scenario_runner.py:116`. `retinue-subtypes.json` is a
stat-profile table, not a scenario, and has no `Kind` — so `--all` dies with a bare
`KeyError('Kind')`. It sorts alphabetically before `three-act-*`, so it takes the
whole sweep down with it.

Task-103 hit this and had to run its three fixtures individually to verify them. Any
future task whose evidence bar names `--all` hits the same wall. `run-slice-three-wave.json`
already anticipated this exact failure — its own `Notes` field says `"Kind": "run_chain"`
exists *only* so `--all` fails with a readable message instead of a bare `KeyError` —
which means the workaround is already load-bearing and the real fix was deferred.

## Done when

`--all` skips any file without a `Kind` with a one-line notice naming the file, and
completes the rest. Nothing else changes: no new scenario semantics, no schema change,
no relocating `retinue-subtypes.json`.
