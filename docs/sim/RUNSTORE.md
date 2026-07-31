# `Scripts/sim/runstore.py` + `report.py` — persisted, auditable run artifacts (task-075)

## Why this file exists

The harness is fast and re-runnable, and until now it kept nothing. Every
number went to stdout and died with the terminal: no way to declare an
experiment once and re-run it, no artifact to compare a new result against an
old one, and — the part that matters most — no record of **which data files a
persisted number was computed from**. `docs/sim/README.md` opens by
explaining this harness exists to replace throwaway scratch scripts. A result
pasted into a design doc with no fingerprint of its inputs is a throwaway
scratch script with extra steps: six weeks later nobody can tell whether it
still describes the current `entity-tiers.json`.

This layer closes that. An **experiment** is a committed question; a **run
artifact** is one answer, stamped with a hash of every input that produced
it; `report.py` re-hashes those inputs and tells you, every time, whether the
answer is still current.

```
Scripts/sim/
  runstore.py    -- experiment format + capture/list/show, and the envelope
  report.py      -- tabulate one artifact, delta two or more, staleness verdict

docs/data/experiments/
  experiments.schema.md              -- the experiment file format
  gate1-maxattackers-sensitivity.json
  elite-surround-cap-band.json
  floor1-retinue-count-vs-wipe.json

docs/sim/runs/                       -- GITIGNORED, volatile
  <run_id>.json                        one artifact per capture
  index.json                           compact record per run
  docs/sim/runs/published/           -- COMMITTED, deliberate (see below)
    <experiment>.json
```

## Experiment files

`docs/data/experiments/<name>.json` — one scenario, N sweep axes, a stated
question, source refs. Full field table and the "where the swept values come
from — never hand-typed" rules are in
`docs/data/experiments/experiments.schema.md`.

`Axes` entries use `sweep.py`'s **existing** `--axis` syntax, parsed by
importing `sweep.parse_axis` — not reimplemented, not forked, and the path
language is not extended. Anything `docs/sim/SWEEPS.md` documents about the
path language and the three axis families applies here unchanged.

## Running one

```powershell
py Scripts/sim/runstore.py capture <experiment-name> [--json] [--publish]
py Scripts/sim/runstore.py list [--experiment <name>] [--limit N]
py Scripts/sim/runstore.py show <run-id> [--json]
py Scripts/sim/report.py <run-id-or-path> [<run-id-or-path> ...] [--json]
```

`capture` runs the experiment's full cross product **serially, in one
process**. There is no pool, no threads, and no `--workers` flag, on purpose:
task-077 adds the parallel driver and needs this serial path as the clean
reference implementation its crossover measurement is taken against. Do not
optimise it early.

`show` accepts a run-id, a published experiment name, or a plain path.

## The envelope, field by field

`envelope_version: 1`. This is a **contract**: task-077 extends it for trials
and task-078 builds a watcher on it.

| Field | Meaning |
|---|---|
| `envelope_version` | `1`. task-077 bumps it. `report.py` reads a higher version best-effort and names the keys it cannot interpret rather than crashing. |
| `run_id` | `<UTC yyyymmdd-hhmmss>-<experiment-name>-<6-char hash of invocation>`. The hash covers `argv`, the experiment name and its `Axes`, so `capture X` and `capture X --json` get different ids. |
| `experiment` / `scenario` / `question` | Copied from the experiment file. The question travels with the answer. |
| `created_utc` | ISO 8601, UTC, `Z`-suffixed. |
| `harness` | `{git_commit: <short sha>, dirty: <bool>}` at capture time. |
| `inputs_fingerprint` | `{repo-relative path: first 12 hex of sha256}` — see below. The load-bearing field. |
| `invocation` | `{argv, workers: 1, serial: true, trigger: "cli"}`. task-077 writes different `workers`/`serial`; task-078 writes a different `trigger`. |
| `diagnostic_family_3` / `diagnostic_reason` | True when any axis targets `constants` (`combat-model-constants.json`). Detected from the axis's target file, exactly as `sweep.py` detects it — never from a field the caller could omit. |
| `cells` | `[{overrides, trial, seed, result}]` in canonical order — see below. `result` is the full `scenario_runner` result, `to_json_safe`-serialised (wave-attrition's per-tick `log` and point-target's `breakdown` included). |
| `wall_clock_seconds` | Measured across the cell loop only, not the write. Deliberately outside `cells`, because it varies run to run and `cells` must not. |
| `cell_count` | `len(cells)`. |

### `inputs_fingerprint` — observed reads, never a hardcoded list

The set of hashed files is determined by **watching what `data_loader`
actually reads** during the run (`runstore._recording_reads` wraps
`data_loader._load_json` for the run's duration and restores it in a
`finally` — the same in-process wrap-and-restore pattern `sweep.py`'s
`_make_patched_load_json` established), plus the experiment file itself,
which `runstore.py` reads directly.

It is emphatically **not** a hardcoded list of the files we currently think
get read. Such a list rots silently the first time `data_loader` starts
reading a new one, and a fingerprint that misses an input is worse than no
fingerprint at all — it looks like an audit and isn't.

The recorder nests correctly under `sweep.run_cell`'s own override patch:
`run_cell` captures `data_loader._load_json` at call time, so its
override-applying wrapper calls the recorder as its underlying read, and
overridden cells still fingerprint the real on-disk file.

A capture of `elite-surround-cap-band` records six files: the experiment
file, the scenario, `combat-model-constants.json`, `entity-tiers.json`,
`unit-types.json`, `upgrades.json`.

### Canonical cell order

`cells` is the literal `itertools.product` order over the experiment's `Axes`
value-lists, exactly as given in the file. Two captures of the same
experiment against unchanged data produce **byte-identical `cells`**
(verified: two captures 2s apart, `sha256(json.dumps(cells))` equal, only
`run_id`/`created_utc`/`wall_clock_seconds` differ).

That guarantee is what makes task-077's parallel driver possible: cells may
be computed in any order, in any number of processes, provided they are
sorted back into this order before the envelope is written. `report.py`
matches cells across artifacts by `(overrides, trial)`, not by position, so
it is not itself sensitive to ordering — but the byte-identity guarantee is,
and it is the property that lets a diff of two artifacts mean something.

### Atomic writes

Every write is write-temp-then-`os.replace`, with the temp file created in the
destination directory so the replace is always within one filesystem. A
killed `capture` cannot leave a half-written artifact or a truncated
`index.json` for task-078's watcher to trip over. A corrupt `index.json` that
predates this rule is rebuilt from the current run forward rather than
crashing a capture.

## Staleness — always reported, and it sets the exit code

`report.py` re-hashes every file named in an artifact's
`inputs_fingerprint` as it is on disk **now**:

- **CURRENT** — every recorded input hashes unchanged. Exit 0.
- **STALE** — one or more changed or went missing. Each is named, with the
  recorded digest and the on-disk digest. Exit **1**.

A survivor count computed against an `entity-tiers.json` that has since been
edited is visibly stale, not quietly presented as current. The non-zero exit
makes this usable as a check.

`harness.dirty` and a `git_commit` that no longer matches HEAD are surfaced
as **WARN**, not as staleness. This is a deliberate split: this repo's working
tree carries uncommitted binary assets more often than not, so treating dirty
as a failure would make the exit code meaningless within a week. Changed
**input data** is the thing that invalidates a number, and that is what flips
the exit code; changed harness code is a warning to re-run, and `drift_check.py`
already covers that ground properly.

## Comparing two artifacts

```powershell
py Scripts/sim/report.py <run-a> <run-b>
```

Per-cell delta on the numeric result fields, matched by the cell's
`overrides` (plus `trial`, so the match key does not change shape when
task-077 lands). Non-numeric fields (`result`, `scenario`, `kind`, `target`)
report `same` / `CHANGED`. Nested structures (`log`, `breakdown`) are carried
in the artifact but not tabulated — a per-tick log is not a comparison unit.

A cell present in one run and not the other is reported as **exactly that**
(`PRESENT IN SOME RUNS ONLY — absent from: <run-id>`), never silently
dropped and never zero-filled. Comparing artifacts from two different
experiments is allowed and prints a note; cells match only where the
overrides are identical.

## The anti-fitting guard carries over

`sweep.py`'s GUARD, `docs/sim/SWEEPS.md`'s guard section and
`docs/sim/LIMITATIONS.md` §1 all say the same thing, and persistence does not
weaken it: **neither `runstore.py` nor `report.py` ranks, sorts, argmins, or
selects a "best" or "closest" cell against any target or measured value, for
any axis family, anywhere.** Cells are stored and shown in canonical order;
deltas are shown as given.

`LIMITATIONS.md` §1 states plainly that some untested or off-default
combination of the family-3 constants could technically be found that makes
`validate.py` check 3 pass, and that finding one would be *worse* than the
current honest failure — it trades a documented, cited-default failure for a
passing check with no citation behind the value that produced it. A store of
persisted cells makes "which of these is closest to GATE1's 110" a tempting
one-liner, which is exactly why the capability does not exist in either file.

Family-3 DIAGNOSTIC status is detected at capture time from the axis target
file, stored in the artifact, and its banner printed by both tools. It cannot
be silenced by how an experiment file is written or how the tool is invoked.

## Publishing is a decision, never a default

`docs/sim/runs/` is gitignored. Every `capture` writes a local, uncommitted
artifact; that is the normal case, and most of them are noise.

`capture --publish` additionally writes
`docs/sim/runs/published/<experiment>.json`, which **is** committed and is the
artifact a design conversation may cite. Publishing is a deliberate act: it
means "this number is worth someone else re-checking my inputs against."
Publishing overwrites the previous published artifact for that experiment by
design — the committed record holds the current answer to each committed
question, and git history holds the rest.

One `.gitignore` detail worth knowing before editing it: the block uses
`docs/sim/runs/*` and not `docs/sim/runs/`. Git will not descend into an
excluded **directory**, so the plain form makes the following
`!docs/sim/runs/published/` unreachable and silently ignores published
artifacts too. Excluding the directory's *entries* instead leaves `published/`
re-includable. Verified with `git check-ignore -v`.

## `Trials` and `Seed` are reserved — task-077 wires them

Both fields exist in the experiment schema today so the format does not
change shape when the batch driver lands. Neither is implemented here.

The seeded variance layer they will drive **already exists and is committed**
(task-076 — `scenario_runner.run_trial`, `docs/sim/MODEL.md` §4). What is
missing is only the driver that runs N trials per cell and folds them into
this envelope. `runstore.py capture` exits non-zero on any experiment file
with `Trials > 1`, naming task-077.

Trials were deliberately not wired here even though `run_trial` is importable:
task-077 measures its process pool against this serial single-trial path, and
that reference is worth more than a head start.

Before quoting any spread that layer eventually produces, read
`docs/sim/LIMITATIONS.md` §6. A spread out of it is a statement about this
harness's sensitivity to two specific dials, not a confidence interval on the
real game, and it cannot make validation check 3 pass.

## What an artifact does NOT become by being persisted

A stored number inherits every limitation the number had when it was printed.
Specifically:

- A `wave_attrition` artifact inherits `LIMITATIONS.md` §1 whole. At
  committed defaults this model predicts a full retinue wipe against
  `GATE1-FUN-PROTOTYPE.md`'s measured ~110-of-120 wave-1 survival, and
  validation check 3 fails. Persisting a survivor count does not make it a
  prediction; it makes it a *reproducible illustration of the mechanism*,
  which is the honest and still-useful thing it was before.
- A `point_target` artifact inherits `LIMITATIONS.md` §3. That model is
  validated and reproduces `entity-tiers.md` §7's own table exactly, but
  every result is a clean-fight **lower bound**, not an in-context
  prediction.
- Both inherit §4's list of what this harness does not model at all.

`report.py` tells you whether an artifact is current. It cannot tell you
whether it was trustworthy in the first place — `docs/sim/LIMITATIONS.md`
does that, and it is still the first thing to read.
