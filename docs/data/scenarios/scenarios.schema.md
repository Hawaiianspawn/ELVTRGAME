# `docs/data/scenarios/*.json` — schema

Companion data files for `Scripts/sim/` (task-063). Follows the convention
`docs/data/feeding.json` + `feeding.schema.md` already set: a scenario is a
plain JSON file the owner can hand-edit, not Python — the harness (specifically
`Scripts/sim/scenario_runner.py`) is the only thing that reads and runs it.

Every file in this directory except `combat-model-constants.json` (a shared
constants block, documented in its own `$schema_note`) is a **scenario**: one
named fight, one `Kind`, run end to end by `run_scenario.py <name>`.

## Common fields (every scenario)

| Field | Type | Meaning |
|---|---|---|
| `Name` | string | Matches the filename (no `.json`). |
| `DisplayName` | string | Human-readable label for output tables. |
| `Kind` | enum | `wave_attrition` \| `point_target` — which model in `combat_model.py` runs this scenario. |
| `Description` | string | One or two sentences: what question this scenario answers. |
| `SourceRefs` | array of strings | Where the population/roster numbers came from — a doc section or data file, not invented. Every count in a scenario should trace to one of these. |
| `HeroPresent` | bool | Whether the Hero fighter (`combat-model-constants.json`) is added to the retinue side. |
| `Notes` | string | Simplifications made when translating a design doc's narrative composition into exact counts — state them, don't hide them. |

## `Kind: "wave_attrition"` — pooled two-sided swarm fight

Uses `combat_model.simulate_wave_attrition` (frontage-capped, pooled HP —
see `docs/sim/MODEL.md`).

| Field | Type | Meaning |
|---|---|---|
| `Retinue.Composition` | array of `{UnitType, Tier, Count, ArrivalSeconds?}` | `UnitType` — key into `unit-types.json` (`spearmen`\|`archers`). `Tier` — key into `upgrades.json`'s tier ladder (`freed`\|`militia`\|`veteran`\|`bannerman`). Each row becomes one `WaveGroup`. `ArrivalSeconds` — see below, optional, defaults to `0` (already on the field at t=0) — present for schema symmetry with `Enemy.Composition`; no committed scenario uses a nonzero value on the retinue side yet. |
| `Enemy.Composition` | array of `{EntityTier, Count, Name?, ArrivalSeconds?}` | `EntityTier` — key into `entity-tiers.json` (`brood_fodder`\|`brood_soldier_melee`\|`brood_soldier_ranged`; Elite/Titan/Boss are `point_target`-only, not wave-attrition rows). `Name` — optional, distinguishes multiple rows sharing one `EntityTier` (e.g. per-rank sub-groups) in output/logging; defaults to `EntityTier` if omitted. |
| `TimeLimitSeconds` | number | Simulation stops here even if neither side is wiped (`result: "timed_out"`). |

### `ArrivalSeconds` (task-068) — per-row arrival gating

Optional float, seconds, default `0.0`. A `Composition` row with `ArrivalSeconds >
0` is not in contact range until `t >= ArrivalSeconds`: it still exists (its
HP pool is untouched and it counts toward the population totals the sim uses
to decide the fight isn't over) but contributes to nothing in the per-tick
combat math before then — not `exposed_frontage`, not the incoming/outgoing
melee damage bounds, not the damage-application split, and it can't be
damaged. See `combat_model.simulate_wave_attrition`'s docstring and
`WaveGroup.has_arrived()` for the exact mechanism, and `docs/sim/MODEL.md`
for the write-up.

**Where the numbers come from — never hand-typed.** Values should trace to
`docs/data/encounter-budget.json` `rank_arrival_timing[].ArrivalSecondsNominal`
(closed-form arithmetic over shipped `Swarm.BroodSpawn*`/`BroodFormation.*`/
`BroodSpeed` CVar defaults — see that file's own `design_constants` block for
the formula and citations) — not invented per-scenario. Splitting one
`EntityTier`'s single `Count` into several `Composition` rows, one per rank,
each carrying its own `Name` and `ArrivalSeconds`, is the pattern
`gate1-calibration-wave1.json` uses (5 rows, `brood_fodder_rank0..4`, matching
`rank_arrival_timing[]`'s `gate1_calibration_wave1_rank0..4` rows exactly) —
the row split is bookkeeping, not a second source of truth for the population
total (the 5 `Count`s still sum to the same locked population).

## `Kind: "point_target"` — army vs. one big single-location entity

Uses `combat_model.army_ttk_vs_point_target` — the same closed-form method
`docs/design/entity-tiers.md` §7 already validated (melee capped at the
target's `SurroundCapEstimate`, ranged uncapped, summed to one TTK). No time
axis — it returns a single TTK number plus the per-group DPS breakdown.

| Field | Type | Meaning |
|---|---|---|
| `Target.EntityTier` | string | Key into `entity-tiers.json`. Must have a non-null `SurroundCapEstimate` to be a meaningful point-target fight (Elite/Titan/Boss). |
| `Retinue.Composition` | array of `{UnitType, Tier, Count}` | Same shape as `wave_attrition`. |

## Conventions carried over from sibling tables

- All values are **PROTOTYPE DIALS / illustrative**, same status as every other file in `docs/data/` and every number `Scripts/sim/` produces from them — re-run against updated data, never treated as committed balance.
- A scenario's `Composition` counts should be traceable to a real design doc (`scaling-curve.md`'s floor tables, `economy.json`'s `slice_targets`, `GATE1-FUN-PROTOTYPE.md`'s stated setup) — if a scenario simplifies a mixed-tier narrative into a single tier for editability, say so in `Notes`, per the same "flag the assumption plainly" convention `entity-tiers.md` §7 and `scaling-curve.md` §7 both use.
- `combat-model-constants.json` is the one file in this directory that is NOT a scenario — it's read by every scenario of both kinds and documents itself.
