# Scripts/sim — a committed simulation harness (task-063)

**What this is:** a reusable, re-runnable Python harness that computes combat
outcomes straight from `docs/data/*.json`, replacing the pattern of
throwaway scratch scripts `docs/design/scaling-curve.md` §7 and
`docs/design/entity-tiers.md` §7 both describe using and discarding. It
reads its inputs, never hardcodes a stat block that already lives in a data
file, and gates its own trustworthiness with a validation suite before any
new number gets reported.

**Read `docs/sim/LIMITATIONS.md` before trusting a number out of this.** The
short version: the wave-attrition (swarm-vs-swarm) model does **not**
currently reproduce `GATE1-FUN-PROTOTYPE.md`'s measured survival numbers, and
that failure is diagnosed, not hidden — see `docs/sim/VALIDATION.md`. The
point-target (army-vs-Elite/Boss) model **does** pass its checks and matches
`entity-tiers.md` §7's own published table exactly.

## Layout

```
Scripts/sim/
  data_loader.py      -- the ONLY module that touches docs/data/*.json
  combat_model.py      -- the math: point-target TTK + wave-attrition sim
  scenario_runner.py   -- loads + runs one docs/data/scenarios/<name>.json (--json supported)
  validate.py           -- the gate (three required/one bonus check)
  sweep.py               -- committed cross-product parameter sweeps (task-069, see docs/sim/SWEEPS.md)
  drift_check.py           -- baseline sweep results + regression gate (task-071, see docs/sim/DRIFT-CHECK.md)

docs/data/scenarios/
  scenarios.schema.md            -- the scenario file format
  combat-model-constants.json    -- Hero stats + frontage-model constants
                                     (the only dials no gameplay data file owns)
  gate1-calibration-wave1.json   -- validation fixture, not a design scenario
  floor1-swarm-wave.json         -- named scenario 1
  floor2-ranged-wave.json        -- named scenario 2
  floor3-boss-point-target.json  -- named scenario 3

docs/sim/
  README.md        -- this file
  MODEL.md          -- how the two combat models work, full derivations
  VALIDATION.md      -- the actual validation suite output, committed for the record
  LIMITATIONS.md      -- what this harness is not trustworthy for yet
  SWEEPS.md            -- sweep.py: axis families, the anti-fitting guard, worked examples
  DRIFT-CHECK.md         -- drift_check.py: baseline format, threshold, refresh procedure, cadence
  baseline.json            -- committed expected sweep results (task-071)
```

## Running it

Plain stdlib, `py` (Python 3.12 on this machine), no dependencies to install.

```powershell
# Run the validation suite first, always. Non-zero exit = the two required
# closed-form checks failed and NOTHING downstream should be trusted.
py Scripts/sim/validate.py

# Run one named scenario
py Scripts/sim/scenario_runner.py floor1-swarm-wave

# Run every scenario in docs/data/scenarios/
py Scripts/sim/scenario_runner.py --all

# List available scenario names
py Scripts/sim/scenario_runner.py --list
```

`validate.py`'s exit code gates checks 1-2 only (the closed-form TTK
sanity checks). Check 3 (the GATE1 wave reproduction) is reported but does
NOT flip the exit code, per task-063's own instruction: a documented,
honestly-reported failure there is a valid, useful result, not a build
failure to be suppressed.

## Adding a scenario

Write a new `docs/data/scenarios/<name>.json` following
`docs/data/scenarios/scenarios.schema.md` — no code changes needed.
`scenario_runner.py` dispatches purely on the file's `Kind` field
(`wave_attrition` or `point_target`). Every `Count`/tier/entity name in a
scenario should trace to a real number in a design doc or another
`docs/data/*.json` file — put the citation in `SourceRefs`, and put any
simplification you made translating a mixed-tier narrative into a flat
scenario in `Notes`, per the convention the four shipped scenarios already
follow.

## Sweeping a parameter across multiple values

`Scripts/sim/sweep.py` runs one scenario across the full cross product of any
number of `--axis` dials — game-balance data, scenario/encounter composition,
or (diagnostic-only) the harness's own model constants. See
`docs/sim/SWEEPS.md` for the axis-path language, what each of the three axis
families is and isn't good for, and the anti-fitting guard built into the
tool (it never ranks or selects a "best" cell, for any axis, on purpose).
This is now how `docs/sim/VALIDATION.md`'s 27-cell sweep is reproduced —
never hand-roll a one-off sweep script outside `Scripts/sim/`.

## Checking for drift against a committed baseline

`Scripts/sim/drift_check.py` re-runs 7 committed `sweep.py` commands
(`docs/sim/baseline.json`, task-071) and exits non-zero if any cell moves
beyond a stated tolerance, or if a `result` field (win/loss/timed-out)
flips outright. See `docs/sim/DRIFT-CHECK.md` for the baseline format, the
threshold and its justification, how to refresh the baseline deliberately
(`--refresh --yes`, never automatic), and a cadence recommendation.

```powershell
py Scripts/sim/drift_check.py           # CHECK — exits 1 on drift
py Scripts/sim/drift_check.py --refresh # preview a new baseline, never writes
```

## Adding a stat this harness needs but no data file owns

Two things (Hero's DPS/HP/SwingInterval, and the wave-attrition frontage
model's spacing constants) aren't owned by any gameplay-director data file —
see `docs/data/scenarios/combat-model-constants.json`'s own `$schema_note`
for why, and don't duplicate a number that DOES already live in
`entity-tiers.json`/`upgrades.json`/`unit-types.json` into that file instead
of reading it live.
