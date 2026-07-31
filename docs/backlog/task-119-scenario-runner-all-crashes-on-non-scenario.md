---
id: 119
title: Stop scenario_runner.py --all crashing on a docs/data/scenarios file that is not a scenario
status: proposed
agent: sim-director
model: ""
owns: ["Scripts/sim/data_loader.py", "Scripts/sim/scenario_runner.py", "docs/sim/README.md"]
resources: []
depends-on: []
epic: ""
evidence: >
  `py Scripts/sim/scenario_runner.py --all` exits 0 and runs every real scenario,
  with `retinue-subtypes.json` skipped for a stated structural reason rather than a
  second hardcoded filename; `py Scripts/sim/validate.py` and `py Scripts/sim/drift_check.py`
  both still pass unchanged.
score: {feel: 1, risk: 1, cost: 1}
source: task-075 handback
teammate: ""
decided: ""
---

## Why now

`py Scripts/sim/scenario_runner.py --all` crashes partway through:

    File "Scripts\sim\scenario_runner.py", line 236, in run
        kind = scenario["Kind"]
    KeyError: 'Kind'

`data_loader.list_scenarios()` returns every `docs/data/scenarios/*.json` except
`combat-model-constants`. `retinue-subtypes.json` is task-086's candidate file, read
through its own `load_retinue_subtypes()`, and has no `Kind` — its top-level keys at
HEAD are `$schema_note, version, status, baseline_anchor, derivation_method,
candidates`. Verified pre-existing at HEAD by the task-075 teammate, not caused by
that task and not caused by the uncommitted edits sitting on that file.

So `--all` has been broken since `retinue-subtypes.json` landed. It is one of the
four commands every sim task is asked to run before handing back, which means every
future teammate either trips over it or learns to ignore a red exit code.

## Done when

- `--all` exits 0 and runs every actual scenario.
- The skip is **structural, not a second hardcoded name.** A file without `Kind` is
  not a scenario; deciding that from the file's own shape is what stops this
  recurring the next time a non-scenario JSON lands in that directory. A skipped
  file is named on stdout, not silently dropped.
- `validate.py` and `drift_check.py` both still pass, untouched.
- `docs/sim/README.md` says what `list_scenarios()` will and will not pick up.

## Spawn prompt

```
You are executing task-119. You are the sim-director for Kindled
(C:\Projects\ELVTRGAME). Read docs/backlog/task-119-scenario-runner-all-crashes-on-non-scenario.md,
then Scripts/sim/data_loader.py and Scripts/sim/scenario_runner.py.

THE BUG, found by the task-075 teammate and confirmed pre-existing at HEAD:
`py Scripts/sim/scenario_runner.py --all` crashes with KeyError: 'Kind' at
scenario_runner.py:236. Cause: data_loader.list_scenarios() returns every
docs/data/scenarios/*.json except combat-model-constants, and retinue-subtypes.json
(task-086's candidate file, read via its own load_retinue_subtypes()) has no `Kind`
key. Reproduce it first before changing anything, and re-confirm the line number —
briefs on this repo go stale.

FIX IT STRUCTURALLY, NOT BY NAME. Adding "retinue-subtypes" beside
"combat-model-constants" in an exclusion list is the fix to avoid: it works today and
breaks again the next time a non-scenario JSON lands in that directory. A file with
no `Kind` is not a scenario — decide it from the file's own shape. Name every skipped
file on stdout; do not drop one silently.

YOU OWN ONLY: Scripts/sim/data_loader.py, Scripts/sim/scenario_runner.py,
docs/sim/README.md

DO NOT TOUCH: docs/data/scenarios/** (retinue-subtypes.json is not yours to reshape,
and another session has been editing it), Scripts/sim/combat_model.py,
Scripts/sim/sweep.py, Scripts/sim/runstore.py, Scripts/sim/report.py,
docs/sim/baseline.json, or anything under ELVTR/.

Stdlib only. Before handing back, run and report the actual output of:
    py Scripts/sim/scenario_runner.py --all
    py Scripts/sim/validate.py
    py Scripts/sim/drift_check.py
All three must pass. If drift_check.py reports drift you changed behaviour rather
than fixing a crash — find it rather than refreshing the baseline, which is not
yours to refresh.

HAND BACK: those three commands' output, the before/after of the crash, and one
sentence on why your skip rule will not need editing when the next non-scenario file
lands.
```
