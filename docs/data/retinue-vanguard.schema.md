# `retinue-vanguard.json` — schema

Companion data file for `docs/design/retinue-tuning-vanguard.md`. Vanguard-
specific retinue tuning (growth rate, attrition, replenishment, per-floor
cap) that layers on top of the generic Supply/Embers economy
(`docs/data/growth-sites.json` / `docs/data/economy.json`, unchanged) —
not a replacement for it. Friendly retinue tier stats (Freed/Militia/
Veteran/Bannerman HP/DPS) are a separate table (`docs/data/upgrades.json`),
not duplicated here.

## `design_constants`

| Field | Type | Meaning |
|---|---|---|
| `starting_headcount` | int | Retinue count entering floor 1. Sourced from `economy.json` (see `starting_headcount_source`/`_status`) — the one input in this file not traced to `encounter-budget.json`/`entity-tiers.json`. |
| `rescue_conversion_rate` | float, 0–1 | Proposed `Vanguard.RescueConversionRate`. Fraction of a cleared Risk Room's `BonusFodderCount` converted to rescued Freed-tier retinue. PROTOTYPE DIAL, judgment call, not derived. |
| `calibration_anchor` | object | The one measured retinue-vs-swarm data point this file's loss model is calibrated against — GATE1's zero-input baseline. `population_n`/`retinue_n` cite `encounter-budget.json`'s own `rank_arrival_context[gate1_calibration_wave1]`; `measured_losses_*` cite `GATE1-FUN-PROTOTYPE.md` §3b directly (required reading, not re-simulated — see `docs/sim/LIMITATIONS.md` §1 for why the harness can't reproduce it). |
| `expected_losses_formula` | string | The closed-form arithmetic §3 of the design doc uses — reproduced here so the formula and the data live next to each other. |
| `expected_losses_caveat` | string | States plainly that the model scales losses by floor population×danger, not by defending headcount — a limitation of having one calibration point, not an oversight. Cross-reference: design doc §8 (bimodal caveat). |

## `danger_index[]` row columns

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | DataTable row key. |
| `Floor` | int, 1–3 | Which floor. |
| `Fodder` / `SoldierMelee` / `SoldierRanged` | int | Summed population per tier across that floor's `encounter-budget.json pulse_schedule[]` rows. |
| `Population` | int | Floor total — must equal `Fodder+SoldierMelee+SoldierRanged` and match the floor's locked total in `encounter-budget.json room_types[]`. |
| `DangerIndex` | float | Population-weighted mean enemy DPS (`entity-tiers.json tiers[].DPS`), see `design_constants.expected_losses_formula`. |
| `Sources` | string | Exact cells this row's numbers trace to. |

## `expected_losses[]` row columns

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | DataTable row key. |
| `Floor` | int | Which floor. |
| `Population` / `DangerIndex` | — | Copied from `danger_index[]` for this floor, kept alongside the derived figures rather than requiring a join. |
| `RawLosses` | float | Unrounded output of the calibrated formula. |
| `ExpectedLosses` | int | `RawLosses`, rounded to nearest whole unit — the figure the ledger uses. |

## `rescue_and_rally[]` row columns

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | DataTable row key. |
| `Floor` | int | Which floor. |
| `RiskRoomBonusFodder` | int | `encounter-budget.json risk_room_budget[].BonusFodderCount` for that floor — the cell this row converts. |
| `Source` | string | Exact cell cited. |
| `ConversionRate` | float | Copy of `design_constants.rescue_conversion_rate`, kept per-row for a self-contained calculation. |
| `RawRescued` / `RescuedFreed` | float / int | Unrounded and rounded (nearest whole) rescued Freed-tier count. |

## `three_floor_ledger[]` row columns — the doc's main deliverable

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | DataTable row key. |
| `Floor` | int | Which floor. |
| `EnteringHeadcount` | int | Retinue count at the start of this floor (floor 1: `starting_headcount`; floors 2–3: prior row's `ExitingHeadcount`). Rescue-only — does **not** include any growth-site Recruit spend (see `growth_site_context`). |
| `RescueReplenishment` | int | This floor's `rescue_and_rally[].RescuedFreed`. |
| `HeadcountIntoArena` | int | `EnteringHeadcount + RescueReplenishment` — the population that actually faces this floor's Arena, since the Risk Room resolves before the Arena starts. |
| `ExpectedLosses` | int | This floor's `expected_losses[].ExpectedLosses`. |
| `ExitingHeadcount` | int | `HeadcountIntoArena − ExpectedLosses` — feeds the next row's `EnteringHeadcount`. |

`three_floor_ledger_verdict` (string, top-level): the grow/hold/starve
reading of the table above, stated once rather than re-derived per row —
see design doc §6.

## `growth_site_context` (out of the strictly-sourced boundary)

Explicitly *not* part of the two-file evidence chain (`encounter-budget.
json`/`entity-tiers.json`) — sourced from `docs/data/growth-sites.json`,
unmodified, shown so the Rescue-only ledger above isn't mistaken for the
whole picture. `breadth_max_recruit_added[]` states the Ember arrival and
Recruit-action math per growth site; `combined_ledger_for_comparison[]`
re-runs the same row shape as `three_floor_ledger[]` with that lane added.
Never substitute this into `three_floor_ledger[]` — they answer different
questions (design doc §6 vs §6b).

## `per_floor_soft_cap`

| Field | Meaning |
|---|---|
| `rule` | No hard cap (Design Law 2) — the existing Supply/upkeep degrade mechanic (`economy.json`) is the governor. States why Rescue & Rally specifically sharpens that governor's relevance (free Embers, same upkeep demand as a paid recruit). |
| `worked_example` | The slice-total Supply-demand cost (28 units, all three floors' rescues) of never spending Embers on Provision. |

## Conventions carried over from sibling tables

- All values are **PROTOTYPE DIALS**, same status as every other file in
  `docs/data/` — re-tune in play, not committed balance.
- Flat, typed columns within each row array — importable as UE DataTables
  keyed on `Name`, one table per array (`danger_index`, `expected_losses`,
  `rescue_and_rally`, `three_floor_ledger`).
- `growth_site_context` is a nested comparison block, not a DataTable row
  array — it's reference context for design review, not meant to import
  directly the way the four sibling arrays are.
