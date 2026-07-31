---
id: 075
title: Make simulation results durable — a declarative experiment format, a versioned run-artifact store, and an artifact comparison tool
status: done
agent: sim-director
model: opus
owns:
  - "Scripts/sim/runstore.py"
  - "Scripts/sim/report.py"
  - "docs/data/experiments/**"
  - "docs/sim/RUNSTORE.md"
  - ".gitignore"
resources: []
depends-on: []
epic: sim-pipeline
evidence: >
  `py Scripts/sim/runstore.py capture <experiment>` runs a committed experiment
  file and writes a versioned JSON artifact under docs/sim/runs/ with an index
  entry and an inputs fingerprint; `py Scripts/sim/report.py <run-a> <run-b>`
  prints the per-cell delta between two artifacts and flags either as stale
  when the data it was computed from has since changed on disk.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: runstore
decided: "2026-07-31 done"
---

## Why now

The harness landed (tasks 063/068/069/070/071) and is fast — the whole
11-scenario suite runs in 0.166s, the 27-cell VALIDATION.md sweep in 0.150s,
the 7-sweep drift check in 0.216s, all measured 2026-07-29. What it cannot do
is *keep* anything: every result goes to stdout and dies with the terminal.
There is no way to declare an experiment once and re-run it, no artifact to
compare a new result against an old one, and no consolidated JSON to read
numbers back out of without re-running and re-reading a table by eye. That
persistence gap — not compute — is what stops the harness being used to
experiment with the system.

This task is deliberately the serial, single-process half. It defines the
experiment format and the artifact envelope that tasks 077 and 078 build the
parallel driver and the watcher on top of, and its serial capture path becomes
the reference implementation task-077 measures its process pool against.

## Done when

1. **`docs/data/experiments/<name>.json`** is a documented, committed format —
   one scenario, N sweep axes, a stated question, source refs — reusing
   `sweep.py`'s existing axis-string syntax verbatim. At least three real
   experiment files are committed, each answering a question already open in
   the repo.
2. **`Scripts/sim/runstore.py`** owns the versioned run-artifact envelope
   (write, read, list, `index.json`) and a `capture` CLI that runs one
   experiment file serially and persists exactly one artifact.
3. **`Scripts/sim/report.py`** reads one or more artifacts, tabulates a single
   run, deltas two or more, and always reports a staleness verdict computed
   from the artifact's recorded input fingerprints.
4. **`docs/sim/RUNSTORE.md`** documents the format, the envelope, and both
   commands.
5. `py Scripts/sim/validate.py` and `py Scripts/sim/drift_check.py` both still
   pass, untouched — this task adds files, it does not change behaviour.

## Spawn prompt

```
You are executing task-075. Read docs/backlog/task-075-experiment-pipeline-persisted-runs.md
first, then docs/sim/README.md, docs/sim/SWEEPS.md, docs/sim/LIMITATIONS.md,
Scripts/sim/sweep.py's module docstring in full, and
docs/data/scenarios/scenarios.schema.md.

You are the sim-director. Simulation results currently die with the terminal —
everything the harness computes goes to stdout and is gone. Build the layer that
makes a result durable, re-runnable and comparable. You own EXACTLY these paths
and must not write any other file:

    Scripts/sim/runstore.py       (new — envelope + capture CLI)
    Scripts/sim/report.py         (new — tabulate, delta, staleness)
    docs/data/experiments/**      (new directory — schema doc + experiment files)
    docs/sim/RUNSTORE.md          (new)
    .gitignore                    (one added block only)

DO NOT TOUCH, for any reason: Scripts/sim/combat_model.py,
Scripts/sim/scenario_runner.py, Scripts/sim/validate.py,
Scripts/sim/data_loader.py, Scripts/sim/sweep.py, Scripts/sim/drift_check.py,
Scripts/sim/batch.py, Scripts/sim/watcher.py, docs/sim/baseline.json,
docs/sim/README.md, MODEL.md, VALIDATION.md, LIMITATIONS.md, SWEEPS.md,
DRIFT-CHECK.md, PIPELINE.md, docs/data/scenarios/**,
.claude/agents/sim-director.md, or anything under ELVTR/ or docs/design/.

STATUS CORRECTION, checked by the lead 2026-07-31: task-076 has LANDED and is
committed (f6271c0). combat_model.py, scenario_runner.py and validate.py now
carry the seeded variance layer — `scenario_runner.run_trial(name, trial_index,
root_seed, overrides)` exists and is module-level on purpose so a future pool
can pickle it. They are still not yours to edit; read them, import from them.
Tasks 077 (pooled batch driver + trials wiring) and 078 (watcher) are still
`proposed` and unbuilt, so every forward reference in this task holds.
IMPORT what you need from the modules you don't own; never edit them. If you
believe a change to one of them is unavoidable, STOP and hand that back as a
finding instead of making it.

Stdlib only — no numpy, no new dependency. The harness's "plain stdlib, no
install step" property is load-bearing.

## 1 — the experiment file format

Write docs/data/experiments/experiments.schema.md following the style and rigour
of docs/data/scenarios/scenarios.schema.md (read it first — match its tone, its
"where the numbers come from — never hand-typed" discipline, and its table
formatting). The format:

  {
    "Name": "<matches filename, no .json>",
    "DisplayName": "<human label>",
    "Question": "<one sentence: what this experiment answers>",
    "Scenario": "<a docs/data/scenarios/ name, no .json>",
    "Axes": ["<file>:<path>=<v1>,<v2>,...", ...],
    "Trials": 1,
    "Seed": null,
    "SourceRefs": ["<doc section or data file each swept range traces to>"],
    "Notes": "<simplifications, exploratory-vs-cited ranges>"
  }

Hard requirements on the format:

  - `Axes` entries are parsed by REUSING sweep.py's own axis parser and
    `resolve_and_set` path language by importing them (`import sweep` /
    `from sweep import ...`). Do not reimplement, fork, or copy-paste that
    parser, and do not extend the path language. If sweep.py does not export
    what you need as an importable name, import the module and call its existing
    functions; still do not edit it.
  - `Trials` and `Seed` are DEFINED in the schema now but NOT implemented by
    you. `Trials` must be 1 (or absent) in every experiment file you commit, and
    `capture` must exit non-zero with a clear message ("Trials > 1 requires the
    batch driver — task-077, not yet wired") if it sees a value above 1. The
    variance layer itself already exists (task-076), so do NOT say it does not;
    what is missing is only the driver that runs trials. Reserving the fields
    now is deliberate: task-077 wires them, and the schema must not change shape
    when it does. Do not wire trials yourself even though `run_trial` is sitting
    right there — the serial single-trial path is what task-077 measures its
    pool against.
  - THE ANTI-FITTING GUARD CARRIES OVER, and this is a hard requirement, not a
    style note. Read sweep.py's "THE GUARD" section and docs/sim/LIMITATIONS.md
    §1 before writing a line of report.py. Neither tool may rank, sort, argmin,
    or select a "best" or "closest" cell against any target or measured value,
    for any axis family, anywhere. report.py's job is to show A-vs-B deltas as
    given, never to tell the caller which cell is closest to 110. The capability
    must not exist in the code at all. Also carry over sweep.py's automatic
    family-3 DIAGNOSTIC detection: if any of an experiment's axes targets
    `constants`, tag the artifact `"diagnostic_family_3": true` with the reason
    and print the DIAGNOSTIC banner in both tools. Detect it from the axis
    target file exactly as sweep.py does — never from a flag the caller could
    omit.

Commit at least three real experiment files. Draw their questions from what is
already open in the repo rather than inventing them — docs/sim/LIMITATIONS.md
§1's MaxAttackersPerUnit sensitivity band, §3's SurroundCapEstimate Fermi
estimate, and the retinue-count-vs-wipe question SWEEPS.md's family-2 section
names are all good candidates. Every swept range needs a SourceRef, and any
range that is exploratory rather than cited must say so in `Notes` — the rule
scenarios.schema.md already sets.

## 2 — the run artifact envelope (runstore.py)

This envelope is a CONTRACT: task-077 extends it for trials and task-078 builds
a watcher on top of it. Implement it exactly:

  {
    "envelope_version": 1,
    "run_id": "<UTC yyyymmdd-hhmmss>-<experiment-name>-<6-char hash of invocation>",
    "experiment": "<Name>",
    "scenario": "<Scenario>",
    "question": "<Question, copied through>",
    "created_utc": "<ISO 8601, UTC, Z-suffixed>",
    "harness": {"git_commit": "<short sha>", "dirty": <bool>},
    "inputs_fingerprint": {"<repo-relative path>": "<first 12 hex of sha256>"},
    "invocation": {"argv": [...], "workers": 1, "serial": true,
                   "trigger": "cli"},
    "diagnostic_family_3": <bool>,
    "diagnostic_reason": "<string or null>",
    "cells": [{"overrides": {...}, "trial": 0, "seed": null, "result": {...}}],
    "wall_clock_seconds": <float>,
    "cell_count": <int>
  }

`inputs_fingerprint` is the load-bearing field — it makes a persisted number
auditable and is how task-078's watcher avoids silently serving stale results.
It must record a hash of EVERY input file the run actually read: the experiment
file, the scenario file, combat-model-constants.json, and each docs/data/*.json
that data_loader touched. Determine that set by observing actual reads (e.g.
wrap `data_loader._load_json` for the duration of a run — sweep.py's
`_make_patched_load_json` shows the established in-process wrap-and-restore
pattern, always restoring in a `finally`), NOT by hardcoding a file list that
will silently rot the moment data_loader starts reading a new file.

`cells` order must be the deterministic sweep order (itertools.product over the
axis lists exactly as given), and two runs of the same experiment against
unchanged data must produce byte-identical `cells`. Task-077 parallelises this
and will sort results back into canonical order; design the envelope so that is
possible, and say so in RUNSTORE.md.

Storage: docs/sim/runs/<run_id>.json plus docs/sim/runs/index.json (a compact
record per run: run_id, experiment, created_utc, cell_count,
wall_clock_seconds, git_commit, dirty). Writes must be atomic — write to a temp
file and replace — so a killed process cannot leave a half-written artifact or a
corrupt index for the watcher to trip over later.

Gitignore the volatile store: add docs/sim/runs/ but NOT
docs/sim/runs/published/ (`docs/sim/runs/` then `!docs/sim/runs/published/`).
One added block with a comment saying why is your only .gitignore edit.
Promoting a result into the committed record is then a deliberate act:
`--publish` writes a second copy to docs/sim/runs/published/<experiment>.json,
which IS committed and is the artifact a design conversation may cite. Runs
without --publish stay local and uncommitted. Note in RUNSTORE.md that
publishing is a decision, never a default.

CLI:

    py Scripts/sim/runstore.py capture <experiment-name> [--json] [--publish]
    py Scripts/sim/runstore.py list [--experiment <name>] [--limit N]
    py Scripts/sim/runstore.py show <run-id> [--json]

`capture` runs the experiment's full cross product SERIALLY, in one process.
Do not add a process pool, threads, or a --workers flag — that is task-077's
job, and its crossover measurement needs your serial path as its clean
reference implementation. Say so in a comment so nobody "optimises" it early.

## 3 — the report layer (report.py)

    py Scripts/sim/report.py <run-id-or-path> [<run-id-or-path> ...] [--json]

  - One artifact: tabulate its cells.
  - Two or more: per-cell delta on the numeric result fields, matched by the
    `overrides` key. Cells present in one run and not the other are reported as
    exactly that — never silently dropped, never silently zero-filled.
  - ALWAYS print a staleness verdict per artifact: re-hash the files named in
    `inputs_fingerprint` as they are on disk NOW and report any that changed
    since the run, naming them. A number computed against an entity-tiers.json
    that has since been edited must be visibly stale, not quietly presented as
    current. Also surface `harness.dirty` and a `git_commit` that no longer
    matches HEAD. Exit non-zero on a stale artifact so this is usable as a
    check.
  - Read the GUARD requirement in §1 again before writing the comparison code.
    No "closest cell", no ranking, no scoring against a measured value.
  - Design the reader to tolerate a future `envelope_version` above 1 by
    reporting what it cannot interpret rather than crashing — task-077 bumps it.

## 4 — docs and verification

docs/sim/RUNSTORE.md: what an experiment file is, the full envelope spec field
by field, the three capture/list/show commands and report.py, the staleness
model, the atomic-write and canonical-order guarantees, and an explicit
"Trials/Seed are reserved, task-077 wires them" note. Match the existing
docs/sim/*.md register — plain statements, numbers with citations, limitations
stated not hedged. Do not edit README.md to link it; task-078 folds the whole
pipeline into README.

Before handing back, run and report the actual output of:
    py Scripts/sim/validate.py
    py Scripts/sim/drift_check.py
    py Scripts/sim/scenario_runner.py --all
    py Scripts/sim/sweep.py gate1-calibration-wave1 --axis "constants:wave_attrition_model.MaxAttackersPerUnit=1,2,4"
All four must behave exactly as they do today. If drift_check.py reports drift,
you changed something you do not own — find it rather than refreshing the
baseline (refreshing it is not yours to do, and baseline.json is not in your
owned paths).

Hand back: those four commands' output; one experiment captured end to end with
the artifact path and a paste of its envelope header (everything except
`cells`); proof that capturing the same experiment twice yields identical
`cells`; a report.py delta between two artifacts; and a demonstration that the
staleness check fires — edit a docs/data JSON, re-run report.py against the
older artifact, show it flagged and naming the changed file, then revert the
edit and confirm it reads clean again.
```
