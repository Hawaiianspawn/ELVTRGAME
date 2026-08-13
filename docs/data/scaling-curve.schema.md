# `scaling-curve.json` — schema

Companion data file for `docs/design/scaling-curve.md`. Four tables. Population
totals per floor are **locked in `SYSTEMS.md` §2 / `economy.json`** and are
restated in `design_constants` only as a join key — this file does not
introduce a new population number.

## `design_constants`

| Field | Type | Meaning |
|---|---|---|
| `population_by_floor` | object, floor→int | The locked 250/450/700 from `SYSTEMS.md` §2, keyed by floor for `floor_roster` rows to reference. Not a new decision. |
| `militia_baseline_dps` | float | Reference DPS (`upgrades.json` Militia = 30) that `retinue_growth_curve.ArmyMultiplierVsMilitia` is expressed against. |
| `gate1_calibration` | array | `GATE1-FUN-PROTOTYPE.md`'s measured zero-input survival data (retinue flat-refilled to 120/wave) — the empirical anchor `scaling-curve.md` §3 calibrates against. Each row: `population`, `retinue_n`, `survivors_est`, `survival_rate`. |

## `floor_roster[]` — Mass Entity population composition per floor

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | DataTable row key, `floor{N}_{tier}`. |
| `Floor` | int, 1-3 | Which floor. |
| `Tier` | enum | `Fodder`\|`Soldier_Melee`\|`Soldier_Ranged` — matches `entity-tiers.json` `Tier`/`Role`, Mass-Entity rows only (Elite/Boss are `elite_boss_schedule`, not this table). |
| `PctOfPopulation` | float, 0-100 | Share of that floor's locked population total. Rows for one floor sum to 100. |
| `Count` | int | `PctOfPopulation × population_by_floor[Floor]`, precomputed (not derived at import). |

## `elite_boss_schedule[]` — promoted-Actor instances per floor

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `floor{N}_{tier}_{instance}`. |
| `Floor` | int, 1-3 | Which floor. |
| `Tier` | enum | `Elite`\|`Boss` (`Titan` never appears — out of slice scope per `entity-tiers.md` §1). |
| `InstanceIndex` | int | 1-based instance count within that floor (Floor 3 has 2 Elite instances). |
| `EncounterMode` | enum | `embedded_in_swarm` (spawns inside the floor's live Fodder/Soldier population — all Elite rows) or `isolated_arena` (own room, no concurrent swarm — the Boss row only). Drives the §4 TTK caveat in the spec: `embedded_in_swarm` TTK sim numbers are a lower bound, not a prediction. |

## `retinue_growth_curve[]` — two bounding play-pattern scenarios

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `{scenario}_floor{N}`. |
| `Scenario` | enum | `balanced` (spends across all 3 growth-site triangle lanes) or `recruit_max` (spends every Ember on Recruit + leftover Provision). Both scenarios spend inside the stated Ember arrival estimates in `growth-sites.json`. |
| `Floor` | int, 1-3 | Which floor this is the *start-of-floor* snapshot for. |
| `TotalUnits` | int | Retinue headcount at floor start. |
| `Archers` | int | Subset of `TotalUnits` that are the Archer type (`unit-types.json` 80/20 split carried through Recruit). |
| `ArcherPct` | float | `Archers / TotalUnits × 100`. |
| `SupplyCapacity` | int | Current Supply capacity (`economy.json` model: capacity, not a draining stock). |
| `SupplyDemand` | int | `TotalUnits × upkeep_per_unit(1)`. |
| `DegradeMultiplier` | float, 0.4-1.0 | `clamp(SupplyCapacity/SupplyDemand, 0.4, 1.0)` — 1.0 means no degrade. |
| `ArmyEffectiveDPS` | float | Sum of `unit_count × unit_dps × DegradeMultiplier` across the army's tier/type mix. |
| `ArmyMultiplierVsMilitia` | float | `ArmyEffectiveDPS / militia_baseline_dps` — the Design Law 1 "layered multiplier" reading. |
| `PopulationArmyRatio` | float | `population_by_floor[Floor] / TotalUnits` — the §3 headcount-reality-check number; compare against `gate1_calibration`'s ratios (2.08/3.75/5.83) to read how far outside the measured-safe band a floor sits. |

## `elite_boss_ttk_sim[]` — closed-form single-target TTK results

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `{scenario}_floor{N}_{target}`. |
| `Scenario` | enum | `balanced`\|`recruit_max`, matching `retinue_growth_curve`. |
| `Floor` | int | Which floor's army composition was used. |
| `Target` | enum | `Elite`\|`Boss`. |
| `ArmyN` | int | Total retinue headcount used as the attacking force (== `retinue_growth_curve.TotalUnits` for that scenario/floor). |
| `TTKSeconds` | float | Time to kill **one** instance of `Target`, clean 1-on-1 (see `EncounterMode` caveat above — Elite rows are a lower bound, not a prediction). |
| `TTKSecondsForTwo` | float or absent | Elite-only, Floor 3: `TTKSeconds × 2`, sequential (both instances fought one after another, not simultaneously). |
| `MeleeDPS` | float | Effective melee (Spearmen) DPS contribution, after Armor mitigation, **at the target's `SurroundCapEstimate`** — extra bodies beyond the cap contribute nothing (see `MeleeBodiesCapped`). |
| `MeleeBodiesCapped` | int | How many melee bodies were actually credited (`min(spearman_count, target.SurroundCapEstimate)`). |
| `ArcherDPS` | float | Effective Archer DPS contribution, after Armor mitigation — **uncapped**, scales with `ArcherCount` directly. |
| `ArcherCount` | int | Archers in the army at that floor (== `retinue_growth_curve.Archers`). |
| `HeroDPS` | float or null | Effective hero DPS contribution (55 base DPS, Armor-mitigated). `null` for the two-Elite Floor-3 rows, where the hero's single-target contribution isn't double-counted across sequential fights in this table. |

## Conventions carried over from sibling tables

- All values are **PROTOTYPE DIALS**, same status as every other file in
  `docs/data/` — re-tune in play, not committed balance.
- Flat, typed columns, no nesting inside a row — importable as four separate
  UE DataTables, each keyed on `Name`.
- Population totals and growth-site costs are **not** re-declared as a second
  source of truth — `floor_roster.Count` and `retinue_growth_curve`'s Supply/
  Ember-derived columns are precomputed *from* `SYSTEMS.md` §2 / `economy.json`
  / `growth-sites.json`, not independent numbers. If those source files
  change, this table's precomputed columns go stale and need re-deriving —
  they are not read at runtime as a second economy model.
