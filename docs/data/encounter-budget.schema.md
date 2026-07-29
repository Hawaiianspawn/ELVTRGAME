# `encounter-budget.json` — schema

Companion data file for `docs/design/encounter-budget.md`. Seven tables. Per-floor
population totals are **locked in `SYSTEMS.md` §2 / `economy.json`** and per-floor
composition/Elite-Boss scheduling are **locked in `scaling-curve.json`**
(task-003) — neither is re-declared as a new number here. This file adds
(a) how each floor's locked population is spent across procgen room types,
(b) the arrival timing (spike/lull pacing) within the main-arena room, which
neither prior file specified, and (c) **per-rank arrival-timing data derived
from shipped `Swarm.BroodSpawn*`/`BroodFormation.*`/`BroodSpeed` CVar
defaults** — the arrival/spawn-pacing data `docs/sim/LIMITATIONS.md` §1–§2
names as missing from every committed file, needed to test that harness's
wave-attrition model's candidate #1 gap.

**Epistemic status differs sharply by table — read `design_constants`'s
`*_status` fields before trusting a number.** `pulse_lull_seconds` and
`elite_lead_seconds` are this doc's own first-pass judgment calls (readability
dials, not derived from anything shipped). `rank_arrival_context` and
`rank_arrival_timing` are the opposite: closed-form arithmetic over five
already-shipped CVar *defaults*, cited exactly — not measured in-engine, but
not invented either. Do not conflate the two categories.

## `design_constants`

| Field | Type | Meaning |
|---|---|---|
| `entity_ceiling_niagara_measured` | int | Top of the measured Niagara sweep in `docs/perf/BUDGETS.md` (2026-07-28, standalone `-game`) — 15.90ms at 20,000 sprites, GPU-bound, never the frame constraint in that data. |
| `entity_ceiling_60fps_simlod4` | int | The 60fps ceiling stated in `docs/perf/BUDGETS.md`'s supersession note once `Swarm.SimLOD.Stride 4` is applied (~34,000). Used as the stricter of the two ceilings for the `peak_concurrency_check` percentage columns. |
| `pulse_lull_seconds` | float, seconds | Proposed `Swarm.PulseLullSeconds` CVar default. Gap between arrival pulses **within** a single floor's main arena — a new, smaller-grain beat below the existing between-floor growth-site breather (`SYSTEMS.md` §4/§7, unchanged by this file). Named `Lull`, not `Breather`, specifically to not collide with `ERunPhase::Breather` in `Spike1GameMode.h`, which already means "between waves, growth site." |
| `elite_lead_seconds` | float, seconds | Proposed `Swarm.EliteLeadSeconds` CVar default. How long before the rest of its pulse's swarm bodies an embedded Elite's own spawn triggers — gives its 1.8s swing telegraph (`entity-tiers.json`) a moment to read clearly before it's buried in fodder (Design Law 6). |
| `risk_room_access_note` | string | States the access-window rule (see `room_types[]` below) — kept here rather than per-row since it's a single floor-agnostic rule. |

## `room_types[]` — procgen room budget per floor

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `floor{N}_{room_type}[_{index}]`. |
| `Floor` | int, 1-3 | Which floor. |
| `RoomType` | enum | `Arena`\|`Corridor`\|`DecisionSite`\|`RiskRoom`\|`BossRoom` — the GDD §9 procgen room categories this doc schedules population against. |
| `Count` | int | How many rooms of this type on this floor. `Corridor` is a range, not a fixed count — genuinely a procgen layout concern, not a population-budget one (see Notes). |
| `PopulationSource` | enum | `locked` (draws from the floor's SYSTEMS.md §2 total, see `pulse_schedule[]`) \| `bonus` (additional, not part of the locked total — `RiskRoom` only) \| `none` (zero Mass Entities ever spawn here). |
| `PopulationCount` | int or null | The room type's total population contribution for that floor. `null` for `Corridor`/`DecisionSite` (always 0, stated as null rather than 0 to flag "structurally can't ever be nonzero" vs. "budgeted at zero this floor"). |
| `Notes` | string | Rationale, GDD §9 cross-reference, or an explicit scope flag. |

## `pulse_schedule[]` — arrival timing inside the main Arena room

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `floor{N}_pulse{K}`. |
| `Floor` | int, 1-3 | Which floor. |
| `PulseIndex` | int, 1-based | Order within the floor's Arena encounter. |
| `PulseTotal` | int | Mass Entities arriving in this pulse. Pulses for one floor sum exactly to that floor's locked population (`scaling-curve.json design_constants.population_by_floor`). |
| `PctOfFloorPopulation` | float | `PulseTotal / population_by_floor[Floor] × 100`. |
| `Fodder` / `SoldierMelee` / `SoldierRanged` | int | Per-pulse tier split. Held at the **same ratio as the floor's overall composition** (`scaling-curve.json floor_roster`) — pulses don't reshuffle composition, only timing; the one exception is `EliteTrigger`, which is additive, not drawn from these counts (Elites are `PromotedActor`s, never part of the Mass Entity population — `entity-tiers.md` §1). |
| `EliteTrigger` | int, 0-2 | How many Elite instances spawn at the start of this pulse (`0` most rows). Sourced from `scaling-curve.json elite_boss_schedule`, scheduled onto a specific pulse here for the first time. |
| `EliteLeadSeconds` | float or null | If `EliteTrigger > 0`, how long before this pulse's swarm bodies the Elite(s) spawn (`design_constants.elite_lead_seconds`). `null` otherwise. |
| `LullAfterSeconds` | float | Gap before the *next* pulse (`design_constants.pulse_lull_seconds`, `0` for a floor's final pulse — the floor-clear growth-site breather takes over from there, unchanged). |

## `risk_room_budget[]` — optional bonus pocket per floor

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `floor{N}_risk_room`. |
| `Floor` | int, 1-3 | Which floor. |
| `BonusFodderCount` | int | Additional Mass Entities, **on top of** the locked population — never subtracted from `pulse_schedule`. Pure Fodder only (see spec §4 for why composition is deliberately not varied here). |
| `PctOfLockedPopulation` | float | `BonusFodderCount / population_by_floor[Floor] × 100` — informational, not a formula this file re-derives elsewhere. |
| `RewardNote` | string | What clearing it grants. Points at `docs/data/loot-v0.json` and `economy.json`'s per-kill Ember rate rather than inventing a second reward table. |

## `peak_concurrency_check[]` — the entity-ceiling proof

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `floor{N}_peak` or `floor3_boss_room_peak`. |
| `Floor` | int, 1-3 | Which floor. |
| `SwarmPeak` | int | Worst case: the floor's *entire* locked Mass Entity population alive simultaneously (as if a lull ran short and the prior pulse hadn't been dented at all — deliberately pessimistic, not an expected number). |
| `RetinuePeak` | int | The higher of `scaling-curve.json`'s two growth scenarios (`balanced`/`recruit_max`) at that floor. |
| `EliteCount` / `BossCount` | int | From `scaling-curve.json elite_boss_schedule`. `BossCount` is always `0` in the main-arena row — the Boss is `isolated_arena`, never concurrent with the swarm (see the separate `floor3_boss_room_peak` row). |
| `RiskRoomBonusWorstCase` | int | `risk_room_budget.BonusFodderCount`, added even though the design keeps the Risk Room spatially and temporally separate from the main Arena (§4) — included anyway so the ceiling check doesn't quietly rely on that separation holding. |
| `TotalPeak` | int | Sum of the above. |
| `PctOf20kMeasured` / `PctOf34kSimLOD4Ceiling` | float | `TotalPeak` as a percentage of `design_constants`'s two ceiling figures. |

## `rank_arrival_context[]` — rollup summary, one row per spawn event

The per-rank detail (`rank_arrival_timing[]` below) rolled up to one row per
`Context` (a wave/pulse/scenario spawn event), for readers who want the
headline numbers without walking every rank.

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | The `Context` key `rank_arrival_timing[]` rows join against. |
| `Floor` / `PulseIndex` | int or null | Where this context sits in `pulse_schedule[]`. `null` for `gate1_calibration_wave1`, which isn't a floor-budget row — it's the harness's own committed validation fixture (`docs/data/scenarios/gate1-calibration-wave1.json`), included here because it's the single most useful context to attach this data to (see spec §2a). |
| `PopulationN` | int | Total Mass Entities in this spawn event (matches `pulse_schedule.PulseTotal` where applicable). |
| `RetinueN` | int | Retinue headcount used to compute `retinue_radius` (design_constants formula) — the "target" the brood are closing on. |
| `RankCount` | int | `ceil(PopulationN / Swarm.BroodFormation.Columns)`. |
| `FrontRankArrivalSeconds` / `BackRankArrivalSeconds` | float, seconds | Nominal (no jitter) time for the first/last rank to close from `Swarm.BroodSpawnRadiusMin` to `EngageRange` contact. |
| `SpreadSeconds` | float | `BackRankArrivalSeconds - FrontRankArrivalSeconds` — how long the full population takes to finish arriving once the front rank is already fighting. |
| `SourceRefs` | string | Where `RetinueN` came from for this context — every count here traces to a cited source, per the sibling `scenarios.schema.md` convention. |

## `rank_arrival_timing[]` — per-rank arrival events (the machine-readable arrival curve)

One row per rank per context — the actual sequence a consumer (a wave director
or a future `simulate_wave_attrition` arrival-time parameter) would step
through: at `ArrivalSecondsNominal`, `RankCount` more bodies enter contact
range.

| Column | Type | Meaning |
|---|---|---|
| `Name` | string, unique | `{Context}_rank{RankIndex}`. |
| `Context` | string | Joins `rank_arrival_context.Name`. |
| `RankIndex` | int, 0-based | 0 = front rank (spawns at `Swarm.BroodSpawnRadiusMin`, arrives first). |
| `RankCount` | int | Bodies in this rank — `Swarm.BroodFormation.Columns` (60) for every rank except the last, which absorbs the remainder. |
| `ArrivalSecondsNominal` | float, seconds | Travel time at `Swarm.BroodSpeed` with zero jitter, from `design_constants.rank_arrival_formula`. |
| `ArrivalSecondsFast` / `ArrivalSecondsSlow` | float, seconds | Bracketed by `Swarm.BroodSpeedJitter` (±6%) — the fastest/slowest a real brood in that rank could arrive, same convention as `EngagedSpacingUU_range` in `combat-model-constants.json`. |
| `Pattern` | const string | Always `rank_staggered` here — a discrete staircase (whole ranks arrive together), not a continuous trickle and not instant. Named explicitly so a future arrival model doesn't have to guess the shape from the numbers alone. |

## Conventions carried over from sibling tables

- `room_types`, `pulse_schedule`, `risk_room_budget`, and `peak_concurrency_check`
  are **PROTOTYPE DIALS**, same status as every other file in `docs/data/` —
  re-tune in play, not committed balance. `PulseLullSeconds` and
  `EliteLeadSeconds` in particular are first-pass readability numbers, not
  measured.
- `rank_arrival_context` and `rank_arrival_timing` are a **different
  epistemic category** — closed-form arithmetic over shipped CVar *defaults*
  (`SwarmCommands.cpp` / `SwarmProcessors.cpp`), cited exactly per
  `design_constants.rank_arrival_source_cvars`. Not an in-engine measurement,
  but not this doc's judgment call either — if the underlying CVar defaults
  change, these values go stale and must be recomputed from
  `rank_arrival_formula`, not hand-edited to a new guess.
- Flat, typed columns, no nesting inside a row — importable as seven separate
  UE DataTables, each keyed on `Name`.
- Population totals, composition percentages, and Elite/Boss instance counts
  are **not** re-declared as a second source of truth — `pulse_schedule`'s
  per-pulse tier counts are precomputed *from* `scaling-curve.json`'s
  `floor_roster`/`elite_boss_schedule`, not independent numbers. If those
  source files change, this table's precomputed columns go stale and need
  re-deriving.
