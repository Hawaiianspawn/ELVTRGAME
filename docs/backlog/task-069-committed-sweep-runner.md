---
id: 069
title: Build a committed sweep runner with machine-readable output
status: done
agent: sim-director
model: sonnet
owns: ["Scripts/sim/**", "docs/sim/**"]
resources: []
depends-on: [68]
epic: sim-tuning-loop
evidence: A committed sweep command that reproduces VALIDATION.md's 27-cell result from source rather than from a lost script, plus `--json` on both the sweep and the single-scenario runner, demonstrated by one balance-data sweep and one composition sweep with their actual output.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: sweep-runner
decided: "2026-07-29 done"
---

## Why now

`docs/sim/VALIDATION.md` §"Full 27-cell sweep" is load-bearing evidence — it is why
`LIMITATIONS.md` §1's "fails across its entire documented parameter range" was retracted,
and why `task-067` exists. **The script that produced it is not committed.** Verified in
the lead session: `grep -rln "itertools.product\|sweep" Scripts/` returns only
`backlog.py`, `README.md` and `combat_model.py` — no sweep runner anywhere.

That is the exact pattern `task-063` was created to end. `docs/sim/README.md` opens by
saying the harness exists to replace "the pattern of throwaway scratch scripts
`docs/design/scaling-curve.md` §7 and `docs/design/entity-tiers.md` §7 both describe using
and discarding." The harness then reproduced that pattern one layer up: a reusable
per-scenario runner was committed, and the sweep that generated the most consequential
finding in `VALIDATION.md` was thrown away.

Two other measured facts shape this task:

- **Speed is not the problem.** The full validation suite runs in **0.106 seconds**
  (timed in the lead session), against `task-064`'s ~40 minutes of PIE restarts for one
  wave-3 sample. Do not spend any time optimising runtime. What is missing is throughput
  of *variations* and output an agent can consume.
- **Machine-readable output is a small change.** `scenario_runner.run()` already returns a
  dict; `print_result` flattens it to prose. There is no `--json`, so an agent reading
  results today parses formatted text. The structured data already exists internally.

## Done when

- A committed sweep command varies **any** of three axis families, named by the owner:
  1. **Game-balance data** — values in `docs/data/*.json` (tier HP/DPS/armor, unit stats,
     economy, upgrades). The primary use: find where the curve breaks or flattens.
  2. **Encounter composition** — wave sizes, tier mixes, arrival pacing, sourced from
     `docs/data/encounter-budget.json`.
  3. **Harness model constants** — `EngagedSpacingUU`, `MaxAttackersPerUnit`,
     `MeleeContactFacingFraction`. Diagnostic only; see the guard below.
- It **reproduces `VALIDATION.md`'s 27-cell result from committed source.** If the numbers
  come out different from what that file records, that is a finding — report it, do not
  quietly overwrite the file to match.
- `--json` on both the sweep and `scenario_runner.py`, emitting the structured dict that
  already exists rather than a re-derived shape. Note the wave-attrition `log` rows are
  objects with attribute access (`row.t`) and need real serialisation.
- **The anti-tuning guard is built into the tool, not left to discipline.** A sweep over
  axis family 3 must label its output as diagnostic and must never rank cells by "closest
  to the measured GATE1 value" or otherwise present a best-fit cell as a recommendation.
  `LIMITATIONS.md` §1 is explicit that finding a passing off-default combination would be
  worse than failing — a passing check with no citation behind the value that produced it.
  A tool that makes fitting easy is a tool that makes that mistake likely.
- `docs/sim/` documents how to run it and what each axis family is and is not good for.

## Spawn prompt

```
You are executing task-069 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.
You are the sim-director. This is the first task of the sim-tuning-loop epic.

THE FINDING THAT MOTIVATES THIS: docs/sim/VALIDATION.md documents a 27-cell sweep whose
result is load-bearing — it retracted LIMITATIONS.md §1's original claim. The script that
produced it was never committed. Verify this yourself: grep -rln "itertools.product|sweep"
Scripts/ returns only backlog.py, README.md and combat_model.py. docs/sim/README.md opens by
saying this harness exists to replace throwaway scratch scripts; the sweep behind its most
consequential finding was itself a throwaway. You are closing that.

TWO MEASURED FACTS, established in the lead session — do not re-litigate them:
1. `py Scripts/sim/validate.py` runs in 0.106 SECONDS. Runtime is NOT a problem. Spend zero
   effort optimising speed. If a sweep is slow it will be because of cell count, not the
   inner loop.
2. scenario_runner.run() already returns a structured dict; print_result flattens it for
   humans. --json is a small addition, not a rewrite. The wave-attrition "log" rows are
   objects accessed as row.t / row.retinue_alive — they need real serialisation, not a
   naive json.dumps.

READ FIRST:
  Scripts/sim/scenario_runner.py, combat_model.py, validate.py, data_loader.py
  docs/sim/VALIDATION.md   -- especially the 27-cell sweep section and the cleave bug writeup
  docs/sim/LIMITATIONS.md  -- §1's anti-tuning rule is a REQUIREMENT of this task
  docs/sim/MODEL.md
  docs/data/scenarios/scenarios.schema.md
  docs/data/encounter-budget.json  -- READ ONLY, task-004's output

YOU OWN:
  Scripts/sim/**
  docs/sim/**

DO NOT WRITE ANYTHING ELSE. In particular:
  - docs/data/scenarios/** belongs to task-070, running beside you. Do not add or edit
    scenario files. If your tool needs a scenario that does not exist, say so in your
    handback and I will route it.
  - docs/data/*.json (unit-types, entity-tiers, economy, encounter-budget) are INPUTS.
    Read them, never write them. Your sweep varies them IN MEMORY for a run; it never
    edits the file on disk. This is important — a sweep that writes back to a committed
    data file would corrupt the very baseline it is measuring against.
  - ELVTR/Source/** is read-only.

BUILD: a committed sweep command that varies any of three axis families:
  1. GAME-BALANCE DATA — docs/data/*.json values (tier HP/DPS/armor, unit stats, economy,
     upgrades). The primary use: find where the scaling curve breaks or flattens.
  2. ENCOUNTER COMPOSITION — wave sizes, tier mixes, arrival pacing from
     encounter-budget.json.
  3. HARNESS MODEL CONSTANTS — EngagedSpacingUU, MaxAttackersPerUnit,
     MeleeContactFacingFraction. DIAGNOSTIC ONLY.

Add --json to the sweep AND to scenario_runner.py.

THE GUARD, AND IT IS A HARD REQUIREMENT: axis family 3 exists for sensitivity analysis, not
for fitting. Your tool must NOT rank cells by proximity to the measured GATE1 value, must
NOT emit a "best" cell for those constants, and must label family-3 output as diagnostic.
LIMITATIONS.md §1 states that some off-default combination could technically be found that
passes check 3, and that adopting it would be worse than failing — it would trade a bad
number for a passing check with no citation behind the value. A tool that makes fitting one
click away is a tool that will eventually be used that way, probably by an agent in a hurry.
Build the guard in. Say in your handback exactly how you built it in.

REPRODUCE VALIDATION.md's 27-cell result from your new committed source. If your numbers
differ from what that file records, THAT IS A FINDING — report it, do not edit VALIDATION.md
to match your output. A silent overwrite would destroy the only record of the original.

DEMONSTRATE with two real runs: one balance-data sweep (family 1) and one composition sweep
(family 2). Commit their actual output, unrounded.

DOCUMENT in docs/sim/ how to run it and what each axis family is and is not good for.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: the exact commands, the actual output, whether the 27-cell reproduction matched
VALIDATION.md, and how the anti-fitting guard is enforced in code. State plainly anything
you could not do.
```
