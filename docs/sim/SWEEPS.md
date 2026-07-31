# `Scripts/sim/sweep.py` — the committed sweep runner (task-069)

## Why this file exists

`docs/sim/VALIDATION.md`'s most consequential finding — that the frontage
model, once the cleave-coupling bug was fixed, actually has the retinue
**winning** in 15 of a 27-cell sweep across `EngagedSpacingUU` x
`MaxAttackersPerUnit` x `MeleeContactFacingFraction` — was produced by a
script that was never committed. `docs/sim/README.md` opens by explaining
this whole harness exists to replace exactly that pattern
(`docs/design/scaling-curve.md` §7's discarded pooled-attrition model). The
sweep behind the harness's own most-cited number was itself a throwaway.
`Scripts/sim/sweep.py` closes that: every sweep from here forward is one
command, re-runnable by anyone, against the current committed data files.

## Running it

```powershell
py Scripts/sim/sweep.py <scenario-name> --axis "<file>:<path>=<v1>,<v2>,..." [--axis ...] [--json]
```

Each `--axis` is one dial and a comma-separated list of values. Multiple
`--axis` flags cross-product (N axes with k1..kN values run k1*k2*...*kN
cells). Every cell is a full, independent `scenario_runner.run()` call with
that cell's overrides applied to an **in-memory copy** of the relevant JSON —
nothing is ever written back to `docs/data/*.json` or any scenario file.
`--json` emits the full cell list (scenario result objects included,
serialized the same way `scenario_runner.py --json` does — see below) instead
of the human-readable table.

`<file>` picks which structure the path walks:

| `file` | Reads | Family |
|---|---|---|
| `entity-tiers` | `docs/data/entity-tiers.json` | 1 — balance data |
| `unit-types` | `docs/data/unit-types.json` | 1 — balance data |
| `upgrades` | `docs/data/upgrades.json` | 1 — balance data |
| `scenario` | the scenario file being run itself | 2 — encounter composition |
| `constants` | `docs/data/scenarios/combat-model-constants.json` | 3 — harness model constants, **DIAGNOSTIC ONLY** |

`economy.json` and `encounter-budget.json` are deliberately **not** offered
as axis files: `data_loader.py` never reads either at run time (a human
copies a number from `slice_targets` or `rank_arrival_timing[]` into a
scenario file by hand — `scenarios.schema.md`'s own convention). An axis
targeting either would silently do nothing to any run's output, which is a
worse trap than not offering it.

### The path language

Dot-separated keys; any segment except the last may carry a
`[field=value]` filter to select one row out of a list by a field's value
instead of a fragile integer index, so paths survive rows being reordered:

```
entity-tiers:tiers[Name=brood_fodder].DPS=30,35,40,45
unit-types:types.spearmen.combat.targets_per_hit=4,8,12
upgrades:tier_ladder.tiers[id=militia].hp=110,130,150
constants:wave_attrition_model.MaxAttackersPerUnit=1,2,4,8
scenario:Retinue.Composition[UnitType=spearmen].Count=32,64,96,128
scenario:Enemy.Composition[Name=brood_fodder_rank0].ArrivalSeconds=0,3,5.85
```

The last segment must be a plain field name (you're always setting a scalar,
never replacing a whole row). Implementation: `sweep.py`'s
`resolve_and_set()`, ~25 lines, no dependency — it is not a general JSONPath
library, just enough to address every field this harness's own data files
actually have.

## The three axis families — what each is and isn't good for

1. **GAME-BALANCE DATA** (`entity-tiers` / `unit-types` / `upgrades`) — the
   gameplay director's committed numbers. Use to find where the scaling
   curve breaks or flattens. Still **read-only** in every sense that matters
   outside one sweep run: a finding here is a report to the gameplay
   director, never a reason to edit the real data file from this tool.
2. **ENCOUNTER COMPOSITION** (`scenario`) — wave sizes, tier mixes, per-row
   `ArrivalSeconds`, `TimeLimitSeconds`. Use to find headcount/ratio
   breakpoints. New `ArrivalSeconds` values swept here that aren't already in
   `encounter-budget.json`'s `rank_arrival_timing[]` are exploratory, not
   newly-cited data — say so in any handoff that uses them.
3. **HARNESS MODEL CONSTANTS** (`constants`) — `EngagedSpacingUU`,
   `MaxAttackersPerUnit`, `MeleeContactFacingFraction`. **DIAGNOSTIC ONLY.**
   Good for sensitivity analysis of the harness's own Fermi/measured-midpoint
   estimates. Not good for, and structurally incapable of, producing a
   "recommended" value — see the guard below.

## The guard against fitting (a hard requirement, not a style choice)

`sweep.py` **never ranks, sorts, or selects a "best" cell by proximity to any
target/measured value, for any axis family, anywhere in the file.** Every
cell's result is printed/emitted in the literal `itertools.product` order of
the `--axis` lists as given on the command line, and nothing is computed
*from* that set of rows except the unranked list itself — no `argmin` against
GATE1's 109-111, no "closest cell," no sort call over results anywhere in
`sweep.py`. `docs/sim/LIMITATIONS.md` §1 states plainly that some untested or
off-default combination of the family-3 constants could technically be found
that makes `validate.py` check 3 pass, and that finding one would be *worse*
than the current honest failure — a passing check with no citation behind the
value that produced it. Making "which cell is closest to 110" one flag away
is exactly the shortcut this guard exists to keep out of reach, so the
capability doesn't exist in this file, for any family, period.

Enforcement is automatic, not a flag a caller could forget: whenever any
`--axis` targets `constants`, `sweep.py` detects it from the axis's `file`
field and prints a mandatory `DIAGNOSTIC ONLY` banner before the table (and
sets `"diagnostic_family_3": true` in `--json` output) — it cannot be
silenced by how the sweep is invoked or labeled.

## `scenario_runner.py --json`

Also added by task-069. `scenario_runner.run()` already returned a
structured dict; `--json` serializes it instead of printing the human table.
The one non-trivial part: wave-attrition's `log` is a list of
`combat_model.WaveTickRecord` dataclass instances (accessed elsewhere as
`row.t` / `row.retinue_alive`), which `json.dumps` cannot handle directly.
`to_json_safe()` walks the result recursively and calls `dataclasses.asdict`
wherever it finds one — real serialization, not a naive `json.dumps(result)`
that would throw.

```powershell
py Scripts/sim/scenario_runner.py gate1-calibration-wave1 --json
py Scripts/sim/scenario_runner.py --all --json      # emits a JSON array
```

## Reproducing `VALIDATION.md`'s 27-cell sweep from committed source

```powershell
py Scripts/sim/sweep.py gate1-calibration-wave1 `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank0].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank1].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank2].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank3].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank4].ArrivalSeconds=0" `
  --axis "constants:wave_attrition_model.EngagedSpacingUU=25,45,51" `
  --axis "constants:wave_attrition_model.MaxAttackersPerUnit=1,2,4" `
  --axis "constants:wave_attrition_model.MeleeContactFacingFraction=0.25,0.5,1.0"
```

**Result: matches every one of `VALIDATION.md`'s 27 rows exactly** (spot
checked all 27; e.g. ES=25/MA=1/FF=0.25 -> 29.80 retinue survivors,
0.00 enemy, `enemy_wiped`, 300.6s; ES=51/MA=4/FF=0.25 -> 0.00 retinue,
89.51 enemy, `retinue_wiped`, 3.6s — both byte-identical to the committed
table).

**One thing to flag, not a regression:** the five `scenario:...ArrivalSeconds=0`
axes above are necessary. Running the sweep directly against the *current*
`gate1-calibration-wave1.json` (task-068 already gave its 5 rows real
nonzero `ArrivalSeconds`, 5.85-7.60s) gives **different** numbers — e.g.
ES=45/MA=4/FF=0.5 comes back as 19.24 enemy survivors, not the table's 19.73.
This is expected: the *fixture's own data* changed under task-068
(single 250-count row -> 5 timed per-rank rows), not the harness math. The
27-cell table predates that change. Zeroing all five `ArrivalSeconds` above
reconstructs the exact pre-task-068 fixture in memory for this one
comparison. `docs/sim/VALIDATION.md`'s task-068 section already documents
the arrival-gated numbers (`0.00` / `19.24`) as the current fixture's actual
behavior — this sweep's un-gated reproduction is a deliberate apples-to-apples
check against the *older* table, not a second, conflicting source of truth
for what `gate1-calibration-wave1` currently does.

## Demonstration 1 — family 1 (game-balance data)

**Refreshed 2026-07-31 (task-120).** The table below is what the harness
prints today; it moved from an earlier printing because the 2026-07-29 armor
fix (`docs/sim/LIMITATIONS.md` §5) stopped hardcoding `victim_armor=0.0` in
`simulate_wave_attrition()`, so `brood_soldier_melee`'s Armor 6 now actually
reduces the retinue's damage output in this scenario's mixed enemy pool. Every
cell below is byte-identical to `docs/sim/baseline.json`'s
`A-floor1-militia-hp-breakpoint` entry, which is regenerated by
`drift_check.py --refresh` and not by hand — treat that file, not this one, as
the thing to re-diff against if a future harness change moves these numbers
again.

**Question:** where does floor 1's outcome (40 retinue vs 250 population,
currently a full wipe — `docs/sim/LIMITATIONS.md` §1) flip, if Militia's
tier-ladder HP is the thing that moves? (Archers scale proportionally off the
same tier ratio per `data_loader.retinue_fighter`'s own documented
assumption, so this also moves them.)

```powershell
py Scripts/sim/sweep.py floor1-swarm-wave --axis "upgrades:tier_ladder.tiers[id=militia].hp=130,200,300,400,600,900"
```

```
=== sweep: floor1-swarm-wave ===
  axis [1-balance-data] upgrades:tier_ladder.tiers[id=militia].hp = [130, 200, 300, 400, 600, 900]

--- cell 0: upgrades:tier_ladder.tiers[id=militia].hp=130 ---
  Retinue: 40 -> 0.00 survivors
  Enemy:   250 -> 142.59 survivors
  Result: retinue_wiped  (elapsed 1.8s)

--- cell 1: upgrades:tier_ladder.tiers[id=militia].hp=200 ---
  Retinue: 40 -> 0.00 survivors
  Enemy:   250 -> 98.27 survivors
  Result: retinue_wiped  (elapsed 4.5s)

--- cell 2: upgrades:tier_ladder.tiers[id=militia].hp=300 ---
  Retinue: 40 -> 0.00 survivors
  Enemy:   250 -> 39.51 survivors
  Result: retinue_wiped  (elapsed 8.1s)

--- cell 3: upgrades:tier_ladder.tiers[id=militia].hp=400 ---
  Retinue: 40 -> 0.00 survivors
  Enemy:   250 -> 7.16 survivors
  Result: retinue_wiped  (elapsed 10.8s)

--- cell 4: upgrades:tier_ladder.tiers[id=militia].hp=600 ---
  Retinue: 40 -> 1.63 survivors
  Enemy:   250 -> 0.00 survivors
  Result: enemy_wiped  (elapsed 300.6s)

--- cell 5: upgrades:tier_ladder.tiers[id=militia].hp=900 ---
  Retinue: 40 -> 10.97 survivors
  Enemy:   250 -> 0.00 survivors
  Result: enemy_wiped  (elapsed 300.6s)

6 cells run. No 'best' cell is computed or printed -- see this file's module docstring, section THE GUARD.
```

**Reading it:** the break sits between HP=400 (still a total wipe, 7.16 of
250 enemy left) and HP=600 (retinue wins, 1.63 survivors) — roughly
4.6x-4.9x Militia's shipped 130 HP, at fixed headcount (40) and fixed enemy
population (250). This is illustrative-mechanism output, same caveat as
every wave-attrition number in this repo (`LIMITATIONS.md` §1) — it shows
*that* this model's outcome is HP-sensitive and roughly where the knee sits
for this one scenario, not a tuning recommendation for Militia's real HP.

## Demonstration 2 — family 2 (encounter composition)

**Refreshed 2026-07-31 (task-120), same cause as Demonstration 1** — the
2026-07-29 armor fix (`docs/sim/LIMITATIONS.md` §5) moved every cell below.
Byte-identical to `docs/sim/baseline.json`'s
`B-floor1-spearmen-count-breakpoint` entry.

**Question:** holding floor 1's 250-strong population fixed, how much
Spearmen headcount alone (Archers held at the scenario's own 8) does it take
to stop the wipe?

```powershell
py Scripts/sim/sweep.py floor1-swarm-wave --axis "scenario:Retinue.Composition[UnitType=spearmen].Count=32,64,96,128,160,200,250"
```

```
=== sweep: floor1-swarm-wave ===
  axis [2-encounter-composition] scenario:Retinue.Composition[UnitType=spearmen].Count = [32, 64, 96, 128, 160, 200, 250]

--- cell 0: scenario:Retinue.Composition[UnitType=spearmen].Count=32 ---
  Retinue: 40 -> 0.00 survivors
  Enemy:   250 -> 142.59 survivors
  Result: retinue_wiped  (elapsed 1.8s)

--- cell 1: scenario:Retinue.Composition[UnitType=spearmen].Count=64 ---
  Retinue: 72 -> 0.00 survivors
  Enemy:   250 -> 107.63 survivors
  Result: retinue_wiped  (elapsed 1.8s)

--- cell 2: scenario:Retinue.Composition[UnitType=spearmen].Count=96 ---
  Retinue: 104 -> 0.00 survivors
  Enemy:   250 -> 90.86 survivors
  Result: retinue_wiped  (elapsed 2.7s)

--- cell 3: scenario:Retinue.Composition[UnitType=spearmen].Count=128 ---
  Retinue: 136 -> 0.00 survivors
  Enemy:   250 -> 41.22 survivors
  Result: retinue_wiped  (elapsed 3.6s)

--- cell 4: scenario:Retinue.Composition[UnitType=spearmen].Count=160 ---
  Retinue: 168 -> 0.00 survivors
  Enemy:   250 -> 14.00 survivors
  Result: retinue_wiped  (elapsed 7.2s)

--- cell 5: scenario:Retinue.Composition[UnitType=spearmen].Count=200 ---
  Retinue: 208 -> 17.63 survivors
  Enemy:   250 -> 0.00 survivors
  Result: enemy_wiped  (elapsed 42.3s)

--- cell 6: scenario:Retinue.Composition[UnitType=spearmen].Count=250 ---
  Retinue: 258 -> 70.54 survivors
  Enemy:   250 -> 0.00 survivors
  Result: enemy_wiped  (elapsed 27.9s)

7 cells run. No 'best' cell is computed or printed -- see this file's module docstring, section THE GUARD.
```

**Reading it:** the break sits between 160 Spearmen (still a total wipe,
14.00 of 250 enemy left) and 200 (retinue wins, 17.63 survivors) — i.e. even
**5x** the scenario's actual headcount (32) still loses at 160, against this
specific 250-strong 85%-Fodder/15%-Soldier-melee population. Worth
contrasting with the pure-Fodder `gate1-calibration-wave1` fixture, where the
27-cell sweep above has the retinue winning at its *actual* headcount (120)
in 15/27 cells: floor 1's population includes `brood_soldier_melee`
(Armor 6, higher DPS than Fodder), which this comparison shows is
substantially harder for this model's pooled retinue to out-attrit than raw
population count alone would suggest. Same illustrative-mechanism caveat as
demonstration 1.

## Layout addition

```
Scripts/sim/
  sweep.py              -- this file's tool

docs/sim/
  SWEEPS.md              -- this file
```
