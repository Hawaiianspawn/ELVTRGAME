---
id: 077
title: Build the parallel batch driver — wire trials into the pipeline and measure where the process pool actually pays
status: proposed
agent: sim-director
model: ""
owns:
  - "Scripts/sim/batch.py"
  - "docs/sim/PIPELINE.md"
resources: []
depends-on: [75, 76]
epic: sim-pipeline
evidence: >
  `py Scripts/sim/batch.py <experiment> --workers N` runs an experiment file
  including `Trials > 1` through task-076's variance layer across a process
  pool and lands one artifact carrying per-trial cells and a per-cell
  distribution summary; `cells` is byte-identical at `--workers 1` and
  `--workers 8`; PIPELINE.md carries the MEASURED serial-vs-pool crossover
  table from this machine, with the default worker count set from it.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now

This is the join for the two wave-1 halves. Task-075 defines the experiment
format and the artifact store but refuses `Trials > 1` and runs strictly
serially; task-076 builds the seeded variance layer and the `run_trials` API but
wires it to nothing. Neither is the thing the owner asked for until they are
connected by a driver that can execute a whole experiment — many combinations,
many trials — in one command.

It is also where parallelism finally has something to do. Measured 2026-07-29,
the entire existing suite runs in under a quarter second, so a process pool at
today's scale is a pessimisation (Windows spawn costs ~100-300ms per worker
against a 150ms serial sweep). Trials are what push cell counts into the
thousands: `Trials: 200` over a 27-cell sweep is 5,400 runs. The pool belongs
here, next to the workload that justifies it, and its payoff has to be measured
rather than assumed.

## Done when

1. `batch.py` runs an experiment file's full cross product × `Trials`, across a
   `ProcessPoolExecutor`, and writes exactly one artifact per invocation via
   task-075's `runstore`.
2. Worker count never affects a number: `cells` is byte-identical at
   `--workers 1` and `--workers 8`.
3. The envelope is at version 2 with the delta documented, and task-075's
   `report.py` can still read version 1 artifacts alongside it.
4. PIPELINE.md carries the real measured crossover table, and the default
   worker count is derived from it — including, if that is what the numbers
   say, defaulting to serial.
5. A run-count guard makes an accidentally enormous job impossible to launch
   silently.
6. `validate.py`, `drift_check.py` and `scenario_runner.py --all` all behave
   exactly as they did before this task.

## Spawn prompt

```
You are executing task-077. Tasks 075 and 076 have both closed. Read their task
files (docs/backlog/task-075-experiment-pipeline-persisted-runs.md and
docs/backlog/task-076-seeded-variance-layer-distributions.md), then read what
they actually built: Scripts/sim/runstore.py, Scripts/sim/report.py,
docs/data/experiments/experiments.schema.md, docs/sim/RUNSTORE.md,
combat_model.py's variance section, scenario_runner.py's run_trials, and
docs/sim/MODEL.md's variance section. Read what they built, not what their task
files predicted — where the two differ the code is the truth, and note the
difference in your handback. Also read docs/sim/LIMITATIONS.md in full and
Scripts/sim/sweep.py's module docstring in full.

You are the sim-director. Build the production driver that runs a whole
experiment — every parameter combination, every trial — in one command, in
parallel, into one durable artifact. You own EXACTLY these paths:

    Scripts/sim/batch.py       (new)
    docs/sim/PIPELINE.md       (new)

DO NOT TOUCH anything else. Specifically not: runstore.py, report.py,
combat_model.py, scenario_runner.py, validate.py, sweep.py, data_loader.py,
drift_check.py, watcher.py, docs/sim/baseline.json, README.md, MODEL.md,
VALIDATION.md, LIMITATIONS.md, SWEEPS.md, DRIFT-CHECK.md, RUNSTORE.md,
docs/data/**, .gitignore, .claude/agents/sim-director.md, or anything under
ELVTR/ or docs/design/. You are connecting two finished pieces, not revising
either. If a sibling left a real defect in a file you do not own, hand it back
as a finding — a follow-up task is the right answer, not reaching in. Task-078
adds the watcher and folds everything into README.md afterwards.

Stdlib only. `concurrent.futures`, `multiprocessing`, `statistics`. No numpy, no
new dependency.

## 1 — the driver

    py Scripts/sim/batch.py <experiment-name> [--workers N] [--serial]
                            [--json] [--publish] [--yes]

Runs the experiment's full axis cross product, `Trials` times per cell, and
writes one artifact through runstore. Reuse runstore's envelope writer and
sweep.py's axis parser by importing them — do not reimplement either, and do not
edit either.

## 2 — WINDOWS PROCESS-POOL CORRECTNESS (read twice; the likeliest way this fails)

  - This machine is Windows, 24 logical cores, Python 3.12 (`py`).
    multiprocessing uses the SPAWN start method, not fork. Every worker
    re-imports your module from scratch.
  - Guard the entry point with `if __name__ == "__main__":`, and make every job
    payload PLAIN PICKLABLE DATA — scenario name as a string, overrides as a
    list of primitive (file, path, value) tuples, trial index and seed as ints.
    Do not send closures, lambdas, `Axis` objects holding functions, or anything
    capturing live module state. Reconstruct the override state inside the
    worker from primitives.
  - sweep.py's override mechanism monkeypatches the module-global
    `data_loader._load_json`. That is process-local, so it is SAFE under process
    parallelism and UNSAFE under threads. Use `ProcessPoolExecutor` — never
    ThreadPoolExecutor, never both. Put the reason in a comment so nobody
    "optimises" it to threads later.
  - Each worker restores the patched global in a `finally`, per cell, exactly as
    sweep.run_cell already does.

## 3 — determinism is the hard requirement

Worker count and completion order must never change a number.

  - Seeding stays task-076's derived scheme — seed derived from (root_seed,
    scenario, overrides, trial_index). Do NOT introduce a second seeding path,
    do not advance a shared RNG stream across cells or trials, and do not let a
    worker's identity enter a seed.
  - Sort results back into canonical order — itertools.product over the axis
    lists as given, then trial index ascending — before writing. Never
    completion order.
  - Verify explicitly and show it in the handback: the same experiment at
    `--workers 1` and `--workers 8` produces byte-identical `cells`.

## 4 — trials

For each cell, call task-076's `run_trials` with the experiment's `Seed` as root
seed, and record per-trial cells plus that cell's distribution summary
(n/mean/median/p5/p95/min/max/stdev over the numeric result fields). Use
task-076's summary implementation rather than recomputing percentiles a second
way — two percentile methods in one codebase will eventually disagree and be
read as a bug in the model.

Carry task-076's `diagnostic_invented_variance` flag up into the artifact and
print its banner. If a spread comes from a magnitude nobody measured, every
consumer of that artifact must be able to see that from the artifact alone.

Envelope: bump `envelope_version` to 2 and document the delta in PIPELINE.md.
report.py must still read version 1 artifacts and compare a v1 against a v2 —
verify that, because the persisted history from task-075 becomes unreadable
otherwise. If report.py cannot do it without modification, hand that back as a
finding; do not edit report.py.

RUN-COUNT GUARD. Trials multiply cell count fast. Before starting, print the
cell count, the trial count, the product, and an ETA from a short calibration
run. Above a documented threshold, refuse without `--yes`. A typo'd extra zero
must not be able to launch a 500,000-run job silently.

## 5 — THE ANTI-FITTING GUARD STILL HOLDS

Read sweep.py's "THE GUARD" section and docs/sim/LIMITATIONS.md §1 before
writing any summary code. No ranking, no sorting by result, no argmin, no
"closest cell", no selection by proximity to a measured value, for any axis
family. And specifically: do NOT add a "does the measured value fall inside this
cell's distribution" verdict. That is the variance-flavoured version of the same
fitting shortcut, and it is the exact way a distribution gets used to quietly
declare LIMITATIONS.md §1's check-3 gap closed. Distributions get reported. They
do not get scored against GATE1's 109-111. Carry over sweep.py's automatic
family-3 DIAGNOSTIC detection from the axis target file, the same way runstore
does.

## 6 — measure the crossover, do not assume it

Serial is currently faster and you must find where that stops being true. Time
task-075's serial `runstore.py capture` path against your pool at a spread of
total run counts — e.g. 27 / 100 / 500 / 2,000 / 10,000 / 50,000 runs, reaching
the large counts through `Trials` rather than absurd axis lists — and put the
real measured table in PIPELINE.md with this machine's core count, the Python
version, and the date.

Set the default worker count from that measurement: serial below the measured
crossover, pooled above it. If the crossover turns out higher than any realistic
experiment, SAY THAT PLAINLY and default to serial. "Parallelism does not pay
below N runs, here are the numbers, here is where it starts to" is a correct and
valuable result, not a failure to deliver. Do not manufacture a speedup by
slowing the serial path, and do not report a speedup you did not measure.

## 7 — docs and verification

docs/sim/PIPELINE.md: the batch command, the trials path, the envelope v2 delta
and the v1-read compatibility statement, the measured crossover table with the
default it implies, the Windows spawn constraints and why processes not threads,
the determinism guarantees, and the run-count guard and its threshold. Match the
existing docs/sim/*.md register — plain statements, numbers with citations,
limitations stated not hedged. Cross-link RUNSTORE.md. Do not edit README.md;
task-078 folds the pipeline in there.

Before handing back, run and report:
    py Scripts/sim/validate.py
    py Scripts/sim/drift_check.py
    py Scripts/sim/scenario_runner.py --all
None may change. drift_check must pass against the unchanged baseline.

Then demonstrate with real pasted output:
  - one experiment at Trials > 1 landing an artifact with per-trial cells and
    per-cell distribution summaries (paste the envelope header and one cell);
  - byte-identical `cells` at --workers 1 vs --workers 8;
  - report.py reading a v1 artifact and a v2 artifact together;
  - the run-count guard refusing an oversized job without --yes;
  - the measured crossover table.

State plainly whether the pool pays at realistic trial counts and at what run
count it starts to, and note any difference between what tasks 075/076 were
specified to build and what they actually built.
```
