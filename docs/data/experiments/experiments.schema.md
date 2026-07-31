# `docs/data/experiments/*.json` — schema

Companion data files for `Scripts/sim/runstore.py` (task-075). Follows the
same convention `docs/data/scenarios/scenarios.schema.md` already set: an
experiment is a plain JSON file the owner can hand-edit, not Python — the
harness (specifically `Scripts/sim/runstore.py`) is the only thing that reads
and runs it.

A **scenario** is one fight. An **experiment** is one *question* about a
scenario: which dial to vary, over which values, and why those values. Before
this directory existed, that question lived in a shell-history line nobody
kept — the same throwaway-script pattern `docs/sim/README.md` opens by saying
this harness exists to replace.

Running one:

```powershell
py Scripts/sim/runstore.py capture <name>
```

That writes a versioned run artifact under `docs/sim/runs/`. See
`docs/sim/RUNSTORE.md` for the envelope, the staleness model, and
`report.py`.

## Fields

| Field | Type | Meaning |
|---|---|---|
| `Name` | string | Matches the filename (no `.json`). `runstore.py` errors out if it doesn't. |
| `DisplayName` | string | Human-readable label. |
| `Question` | string | **One sentence: what this experiment answers.** Copied verbatim into every artifact it produces, so a persisted number carries the question it was computed for. If you can't write it in one sentence, the experiment is two experiments. |
| `Scenario` | string | A `docs/data/scenarios/` name (no `.json`). One scenario per experiment. |
| `Axes` | array of strings | `"<file>:<path>=<v1>,<v2>,..."`, the **exact** `--axis` syntax `Scripts/sim/sweep.py` already takes. Parsed by importing `sweep.parse_axis` — not reimplemented, not forked, and the path language is not extended here. See `docs/sim/SWEEPS.md` for the language and the three axis families. Empty/absent = one cell, the scenario exactly as committed. |
| `Trials` | int | **Reserved. Must be `1` or absent.** See below. |
| `Seed` | int \| null | **Reserved. Must be `null` or absent.** See below. |
| `SourceRefs` | array of strings | Where each swept range came from — a doc section or a data file, not invented. Every range needs one. |
| `Notes` | string | Simplifications, and which ranges are exploratory rather than cited. State them, don't hide them. |

## Where the swept values come from — never hand-typed

The rule `scenarios.schema.md` sets for a scenario's `Count`s applies to an
experiment's axis values, and it is the whole reason `SourceRefs` is
mandatory. Three legitimate origins, in descending order of strength:

1. **A committed range field.** `entity-tiers.json`'s
   `brood_elite.SurroundCapRange` (`"15-27, Fermi estimate not measured
   (entity-tiers.md §4)"`) and `combat-model-constants.json`'s
   `EngagedSpacingUU_range` / `MeleeContactFacingFraction_range` exist
   precisely so a sensitivity sweep has cited endpoints instead of invented
   ones. Sweep those endpoints.
2. **A committed default plus a doc-stated alternative.** e.g.
   `MaxAttackersPerUnit` 1/2/4, the values `docs/sim/VALIDATION.md`'s 27-cell
   sweep already ran and `docs/sim/LIMITATIONS.md` §1 already reports on.
3. **Exploratory** — a range chosen to find where a curve bends, tracing to
   nothing but the committed value it starts from. This is allowed, and it
   must be **said in `Notes`**, in those words. `SWEEPS.md`'s family-2 note
   already sets the same rule for `ArrivalSeconds` values not in
   `encounter-budget.json`'s `rank_arrival_timing[]`.

An axis value that traces to none of the three and isn't flagged exploratory
is the failure mode this whole directory exists to prevent.

## `Trials` and `Seed` — reserved, defined now, not implemented yet

Both fields are part of the format today so the schema does not change shape
when task-077 wires them. The seeded variance layer they will drive already
exists and is committed (task-076 — `scenario_runner.run_trial`,
`docs/sim/MODEL.md` §4); what is missing is only the batch driver that runs N
trials per cell and folds them into the run envelope.

`runstore.py capture` **exits non-zero** on any experiment file with
`Trials > 1`. Every experiment committed here has `Trials: 1`. Do not raise it
expecting it to work.

Before quoting any spread that layer eventually produces, read
`docs/sim/LIMITATIONS.md` §6: it is a statement about this harness's
sensitivity to two specific dials, not a confidence interval on the real
game, and it cannot make validation check 3 pass.

## The anti-fitting guard applies to this directory too

`Scripts/sim/sweep.py`'s GUARD, `docs/sim/SWEEPS.md`'s guard section and
`docs/sim/LIMITATIONS.md` §1 all say the same thing, and persisting results
does not weaken it: **no tool in `Scripts/sim/` ranks, sorts, or selects a
"best" or "closest" cell against any target or measured value, for any axis
family.** `runstore.py` stores cells in the literal `itertools.product` order
of this file's `Axes` list; `report.py` shows deltas as given. Neither has a
"closest to 110" capability, and neither is to acquire one.

The corollary for an *experiment author*: do not write an experiment whose
`Question` is "which value of X makes GATE1's 109-111 come out." An
experiment asks how sensitive an outcome is to a dial's own documented
uncertainty band. It does not go shopping for a value.

Any experiment with an axis targeting `constants` is automatically tagged
`diagnostic_family_3` in its artifact and prints the DIAGNOSTIC banner in
both tools. That is detected from the axis's target file, exactly as
`sweep.py` detects it — never from a field an author could omit, so it cannot
be silenced by how the experiment file is written.

## Conventions carried over from sibling tables

- All values are **PROTOTYPE DIALS / illustrative**, the same status as every
  other file in `docs/data/` and every number `Scripts/sim/` produces from
  them — re-run against updated data, never treated as committed balance.
- A wave-attrition experiment inherits `docs/sim/LIMITATIONS.md` §1 whole: the
  model does not reproduce `GATE1-FUN-PROTOTYPE.md`'s measured ~110-of-120
  wave-1 survival at committed defaults. A survivor count out of one of these
  files is illustrative of the mechanism, not a prediction, and a handoff
  citing one says so.
- A `point_target` experiment inherits `docs/sim/LIMITATIONS.md` §3 instead:
  that model is validated against `entity-tiers.md` §7's own table, but every
  result is a **clean-fight lower bound**, not an in-context prediction.

## Committed experiments

| Name | Scenario | Family | Question |
|---|---|---|---|
| `gate1-maxattackers-sensitivity` | `gate1-calibration-wave1` | 3 — **DIAGNOSTIC** | How far does `MaxAttackersPerUnit` alone move wave-1's outcome across the values `VALIDATION.md`'s 27-cell sweep already ran? |
| `elite-surround-cap-band` | `floor2-elite-point-target` | 1 — balance data | How much does the Elite's un-measured `SurroundCapEstimate` Fermi band (15-27) move the point-target TTK? |
| `floor1-retinue-count-vs-wipe` | `floor1-swarm-wave` | 2 — encounter composition | How much Spearmen headcount does it take to stop floor 1's wipe against a fixed 250-strong population? |
