# `docs/data/wave-scaling.json` — schema

Companion data for `docs/design/wave-scaling-three-act.md` (task-102). New shape —
this is the first file to schedule *retinue* size per wave alongside enemy
population; `scaling-curve.json`'s `floor_roster`/`retinue_growth_curve` are two
separate tables because that curve treated retinue growth as a continuous economy
output, not a wave-keyed starting count. Here they're keyed together because the
three-act framing (early/mid/late) is about both sides of the fight moving in lockstep,
not just the enemy side.

## `design_constants`

Citations only — the perf ceiling, the GATE1 calibration ratios, and the entity-tier
references every other table's arithmetic depends on. Not a decision in itself.

## `wave_definitions[]`

One row per wave. The join key every other table hangs off `WaveIndex`.

| Column | Type | Meaning |
|---|---|---|
| `Name` | string | Unique row key, e.g. `wave1_early`. |
| `WaveIndex` | int | 1, 2, or 3 — matches `Spike1GameMode::WaveIndex`. |
| `Label` | string | `Early` \| `Mid` \| `Late`. |
| `RetinueStart` | int | Total retinue at wave start (all `retinue_composition` rows for this `WaveIndex` sum to this). |
| `RetinueMechanism` | string | How the count is reached — see doc §1. Not a claim about which system (`Spike1GameMode` refill vs. `economy.json` growth-site) implements it; both are named as valid. |
| `EnemyPopulation` | int | Total Mass-Entity enemy count (`enemy_composition` rows for this `WaveIndex` sum to this). |
| `PopulationArmyRatio` | float | `EnemyPopulation / RetinueStart` — the same ratio `scaling-curve.md` §3 calibrates against GATE1's measured survival curve. Directional only (`docs/sim/LIMITATIONS.md` §1), never a survivor prediction. |
| `EliteInstances` | int | Promoted-Actor Elite count, this wave. |
| `BossInstances` | int | Promoted-Actor Boss count, this wave. |
| `TotalMassEntities` | int | `RetinueStart + EnemyPopulation` — what actually competes for the measured Mass-sim frame budget. Elite/Boss are `PromotedActor`s and are NOT part of this sum (`entity-tiers.json`'s own `ActorType` column). |
| `HeadroomPctVs34kCeiling` | float | ⚠️ **ARITHMETIC, NOT A MEASUREMENT — corrected 2026-07-31.** `(34000 - TotalMassEntities) / 34000 * 100`. The 34,000 divisor is **one unreproduced standalone run** (`one-camera-bench.md` §8, run 5); four in-editor sweeps on 2026-07-30/31 cross 16.6ms between **~13,000 and ~26,000** entities. Honest working figure is **~13,000–20,000**. Every value in this column is therefore unverified, and `wave3_late`'s 39.41 has never been run. The column name is kept (nothing in code reads it) so the stale figures stay traceable; **task-108** replaces the divisor with a measurement. Do not derive new headroom claims from this until it lands. |
| `Notes` | string | Wave-specific caveats. |

## `retinue_composition[]`

| Column | Type | Meaning |
|---|---|---|
| `Name` | string | Unique row key, e.g. `wave1_early_spearmen`. |
| `WaveIndex` | int | Join key into `wave_definitions`. |
| `UnitType` | string | `spearmen` \| `archers` — key into `unit-types.json`. |
| `Tier` | string | Key into `upgrades.json`'s tier ladder (`militia` throughout this doc — see §7 assumption). |
| `Count` | int | |
| `PctOfRetinue` | float | `Count / RetinueStart * 100` for that wave. |

## `enemy_composition[]`

| Column | Type | Meaning |
|---|---|---|
| `Name` | string | Unique row key, e.g. `wave2_mid_soldier_ranged`. |
| `WaveIndex` | int | Join key. |
| `EntityTier` | string | Key into `entity-tiers.json` (`brood_fodder` \| `brood_soldier_melee` \| `brood_soldier_ranged`). |
| `Count` | int | |
| `PctOfPopulation` | float | `Count / EnemyPopulation * 100` for that wave. |

## `elite_boss_schedule[]`

Same shape as `scaling-curve.json`'s table of the same name.

| Column | Type | Meaning |
|---|---|---|
| `Name` | string | Unique row key. |
| `WaveIndex` | int | Join key. |
| `Tier` | string | `Elite` \| `Boss`. |
| `InstanceIndex` | int | 1-based count within the wave. |
| `EncounterMode` | string | `embedded_in_swarm` \| `isolated_arena` — same vocabulary `scaling-curve.md` §1 already uses. |

## Conventions

- All values PROTOTYPE DIALS / a new proposal, same status as every other file in `docs/data/` — see the design doc's own supersession section before treating any number here as replacing `SYSTEMS.md`.
- `TotalMassEntities` / `HeadroomPctVs34kCeiling` are worst-case, all-at-once figures (the entire wave's population spawned at t=0, matching how `Spike1GameMode::BeginWave()` currently works) — a pulsed arrival schedule (`encounter-budget.md`'s mechanism) would only lower peak concurrency further, never raise it.
