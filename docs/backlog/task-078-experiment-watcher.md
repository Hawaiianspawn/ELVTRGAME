---
id: 078
title: Add the long-running watcher that keeps experiment results current, and fold the pipeline into the harness README
status: proposed
agent: sim-director
model: ""
owns:
  - "Scripts/sim/watcher.py"
  - "docs/sim/WATCHER.md"
  - "docs/sim/README.md"
  - ".claude/agents/sim-director.md"
resources: []
depends-on: [77]
epic: sim-pipeline
evidence: >
  `py Scripts/sim/watcher.py` runs as a standalone long-lived process that
  re-runs only the experiments a changed JSON actually affects — demonstrated
  on both sides, affected re-run and unaffected untouched — and writes a
  heartbeat status file; `py Scripts/sim/watcher.py --status` reports LIVE
  while running and STALE after the process is killed, exiting non-zero when
  not live; README.md documents the pipeline as one workflow.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now

The owner chose a long-running watcher over a fire-and-forget command
(2026-07-29), explicitly accepting its risk: a stale watcher silently serving
old numbers. That makes the liveness and staleness machinery the substance of
this task rather than a nicety bolted on at the end — a dead watcher must never
be indistinguishable from a live one.

It comes last because it drives task-077's batch driver as a subprocess and
resolves which experiments a change affects from task-075's
`inputs_fingerprint`. Neither exists until those close. It also carries the
final fold: README.md and the agent definition still describe a harness with no
persistence, no experiment files and no watcher.

## Done when

1. `watcher.py` is a standalone long-lived process that detects content changes
   in the watched JSON set, resolves which experiments those changes affect, and
   re-runs only those — writing local artifacts, never publishing, never
   committing, never mutating its inputs.
2. The stale-watcher failure mode is closed by construction: a heartbeat status
   file, a `--status` verdict that verifies the recorded pid is alive, and a
   non-zero exit when not live.
3. Watcher-triggered artifacts are attributable — the trigger and the changed
   file that caused them are recorded in the artifact.
4. `WATCHER.md` documents the design and its four hard guarantees; `README.md`
   describes the whole pipeline as one workflow without softening anything it
   currently says about the wave-attrition model; `sim-director.md` names the
   new paths and tools.

## Spawn prompt

```
You are executing task-078. Tasks 075, 076 and 077 have all closed. Read their
task files, then read what they actually built: Scripts/sim/runstore.py,
report.py, batch.py, docs/sim/RUNSTORE.md, docs/sim/PIPELINE.md, and
docs/data/experiments/experiments.schema.md. Read what they built, not what their
task files predicted — where the two differ the code is the truth. Also read
docs/sim/README.md and .claude/agents/sim-director.md in full before editing
either.

You are the sim-director. Build the long-running watcher that keeps experiment
results current, and fold the whole pipeline into the harness's front-door docs.
You own EXACTLY these paths:

    Scripts/sim/watcher.py            (new)
    docs/sim/WATCHER.md               (new)
    docs/sim/README.md                (extend)
    .claude/agents/sim-director.md    (extend)

DO NOT TOUCH anything else. Specifically not: batch.py, runstore.py, report.py,
combat_model.py, scenario_runner.py, validate.py, sweep.py, data_loader.py,
drift_check.py, docs/sim/baseline.json, MODEL.md, VALIDATION.md, LIMITATIONS.md,
SWEEPS.md, DRIFT-CHECK.md, RUNSTORE.md, PIPELINE.md, docs/data/**, .gitignore,
or anything under ELVTR/ or docs/design/. If an earlier task left a real defect
in a file you do not own, hand it back as a finding rather than reaching in.

Stdlib only. No `watchdog`, no inotify wrapper — poll.

## 1 — the watcher

    py Scripts/sim/watcher.py [--interval SECONDS] [--once] [--status] [--stop]

A long-lived foreground process the owner starts deliberately. Every interval
(default 5s — justify whatever you choose in WATCHER.md): hash the watched set,
compare against last-seen hashes, and for anything changed, re-run the
experiments that depend on it.

Watched set: docs/data/experiments/*.json, docs/data/scenarios/*.json,
docs/data/*.json.

DETECT CHANGES BY CONTENT HASH, not mtime. mtime moves on a no-op touch, on a
`git checkout`, and whenever this repo's own tooling rewrites a file
identically — re-running the world because git restored a byte-identical file is
exactly the noise that makes a watcher untrustworthy and gets it turned off.

AFFECTED-EXPERIMENT RESOLUTION. Use the `inputs_fingerprint` task-075 records in
every artifact — it names the files a run actually read. An experiment is
affected iff a changed file appears in its most recent artifact's fingerprint,
or its own experiment file changed, or it has no artifact at all. Do not re-run
everything on every change. An experiment with no artifact yet has no
fingerprint to resolve against: run it once to establish one, and log that this
is what you are doing.

Requirements:

  - DEBOUNCE. Coalesce edits inside a short window into one sweep. Saving three
    data files in a row is one change, not three sweeps.
  - RUN IN A SUBPROCESS. Invoke batch.py as a subprocess, not in-process. A
    long-lived daemon must survive a run that crashes or hangs, and this machine
    is Windows/spawn, so nesting a process pool inside a daemon loop is the
    fragile option. Cap concurrent batch subprocesses and apply a per-run
    timeout; a queue that grows without bound while runs pile up is a failure,
    not throughput.
  - NEVER PUBLISH, NEVER COMMIT, NEVER MUTATE INPUTS. Watcher-triggered runs
    write local artifacts to the gitignored run store only. Do not pass
    --publish, do not invoke git, do not write anything under docs/data/**. A
    background process that commits to the repo, or edits the data it is
    watching, is strictly out of bounds. State this in WATCHER.md as a
    guarantee, not a preference.
  - CRASH-ONLY IS FINE; SILENCE IS NOT. Log every sweep, every triggered run and
    every failure to a rotating log in the run store. If a batch subprocess
    fails, record it in the status file and keep watching — do not die quietly,
    and do not retry in a tight loop.
  - CLEAN SHUTDOWN on Ctrl-C / SIGTERM: mark the status file stopped, drain or
    kill children, leave no orphaned subprocesses.

## 2 — closing the stale-watcher failure mode

This is the specific risk the owner accepted in choosing a watcher, so it is the
part that has to be right.

  - Heartbeat status file in the run store (e.g. watcher-status.json), rewritten
    every interval even when nothing changed: pid, started_utc,
    last_heartbeat_utc, last_sweep_utc, sweeps_completed, runs_triggered,
    watched_file_count, pending_queue_depth, last_error, and state
    (running|stopped|failed). Write it atomically — a reader must never catch a
    half-written status file.
  - `--status` reads it and prints a verdict: LIVE, STALE or STOPPED, decided
    from last_heartbeat_utc against the interval with a stated tolerance, AND by
    checking the recorded pid is actually alive. A status file whose process is
    gone must report STALE, never LIVE. Exit non-zero when not live so it is
    usable as a check in a script.
  - Every artifact the watcher produces must be attributable: record the trigger
    in the artifact's `invocation` — `"trigger": "watcher"` plus the changed file
    that caused it — so a number's provenance survives. batch.py owns the
    envelope writer, so pass this through its existing interface; if it has no
    way to accept a trigger, hand that back as a finding rather than editing
    batch.py.
  - WATCHER.md must say plainly: the watcher does not make results trustworthy,
    it makes them CURRENT. report.py's staleness check remains the authority on
    whether a given artifact matches the data on disk right now, and a reader
    should trust that check over the watcher's liveness.

## 3 — the docs fold

WATCHER.md: the design, the polling and debounce rationale with the interval
justified, the content-hash decision and why not mtime, the affected-experiment
resolution rule, the four hard guarantees from §1, the status and staleness
model, and how to run and check on it.

README.md: fold the pipeline into one workflow narrative — author an experiment,
run it, persist it, compare it, keep it current — and cross-link RUNSTORE.md,
PIPELINE.md and WATCHER.md from the existing Layout and Running sections, adding
the new modules to the Layout tree. Two constraints: keep the existing "read
docs/sim/LIMITATIONS.md before trusting a number out of this" framing at the top
and do not soften a word of what README.md currently says about the
wave-attrition model failing to reproduce GATE1's measured survival. A faster,
more convenient pipeline makes that warning MORE load-bearing, not less — it is
now much cheaper to generate a confident-looking untrustworthy number, and the
docs should say so.

sim-director.md: add docs/data/experiments/** to the owned-paths block and add
the new tools (runstore/report/batch/watcher) to the harness description so a
future sim-director session knows they exist. Do not change the agent's
boundaries — it still never edits SYSTEMS.md or docs/design/, and the
LIMITATIONS.md §1 caveat on wave-attrition numbers still applies to every result
this pipeline produces.

## 4 — verification

    py Scripts/sim/validate.py
    py Scripts/sim/drift_check.py
    py Scripts/sim/scenario_runner.py --all
None may change; drift_check must pass against the unchanged baseline.

Then demonstrate with real pasted output:
  - the watcher running; an edit to one docs/data JSON triggering exactly the
    affected experiments and NOT the unaffected ones — show both sides — then
    reverting the edit;
  - a byte-identical rewrite (touch, or write the same content back) triggering
    NOTHING;
  - `--status` reporting LIVE while running, then STALE after the process is
    killed with the status file left behind, with the exit codes;
  - a watcher-produced artifact showing its `trigger` provenance;
  - a failing batch subprocess recorded in the status file with the watcher
    still alive afterwards.

Hand back that output, plus the README.md and sim-director.md diffs, and a plain
statement of anything in the pipeline you found underdocumented or fragile while
wiring the watcher on top of it — you are the first consumer of all three
earlier tasks at once, and that is worth reporting.
```
