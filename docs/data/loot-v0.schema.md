# `loot-v0.json` — schema

Companion data file for `docs/design/loot-v0.md`. Covers the **three new**
battle-drop tables (`drop_types[]`, `drop_sources[]`) — units (Unit Orb),
healing (Kindling Ember), and buffs (Rally Ember) — that answer
`RTS-VERTICAL-SLICE.md` §4's "unit orbs + healing" line and its own
follow-up request to add a buffs category. The "4-6 stacking items" fourth
component of that same checklist line is a **reference block only**
(`stacking_items_reference`), pointing at `docs/data/upgrades.json`, which
already owns those numbers (`SYSTEMS.md` §3, decided 2026-07-24).

## `design_constants`

| Field | Type | Meaning |
|---|---|---|
| `pickup_radius` | float, uu | How close a friendly entity must incidentally pass to collect a drop. Reuses `feeding-distraction.md`'s `Swarm.Feeding.ClaimRadius` value (150uu) and its exact justification. |
| `heal_burst_radius` | float, uu | Kindling Ember only — AoE radius of the heal burst on pickup. One `GridCellSize` (250uu), so the burst query reuses the existing neighbour scan. |
| `drop_despawn_time` | float, seconds | How long an uncollected drop persists before removal. All drops are also cleared at floor transition regardless of this timer (no cross-floor carry-over). |
| `max_active_drops` | int | Shared safety-valve population cap across both drop types combined. Not expected to bind at simulated volumes (see `simulation_summary`). |
| `assumed_pickup_rate` | float, 0–1 | **Not measured** — the fraction of spawned drops assumed collected before despawn/floor-end, used only for this file's Monte Carlo. Re-derive once a real sim exists. |
| `resolution_pattern_note` | string | Points at the shared "drop-centric, notice-don't-seek" resolution pattern this file deliberately reuses from `feeding-distraction.md` §5.2, rather than inventing a second one. |
| `buff_duration` | float, seconds | `rally_ember_working` only — how long one application's stack(s) last. A pickup while a buff is already active refreshes this single global clock rather than starting a second one. |
| `buff_per_stack_pct` | float, 0–1 | `rally_ember_working` only — DPS multiplier granted per stack, applied identically to hero and retinue (one global multiplier, not a per-unit fragment). |
| `buff_max_stacks` | int | `rally_ember_working` only — hard cap on concurrent stacks. A diminishing-returns clamp on this one pickup type, not a claim about the power curve's own cap (Supply/upkeep remains the real governor, `SYSTEMS.md` §7). |

## `drop_types[]` row columns

One row per battle-drop type. Flat, typed, DataTable-importable on `Name`.

| Column | Type | Meaning |
|---|---|---|
| `Name` | string (row key) | Snake_case working ID. `_working` suffix mirrors `entity-tiers.json`'s `WorkingNameOnly` convention — not a shippable name. |
| `DisplayName` | string | Editor/debug label. Still a working name. |
| `WorkingNameOnly` | bool | Always `true` — narrative hasn't named anything in current canon (`FLAME-FOUNDATION.md`); see loot-v0.md's Narrative requests. |
| `Kind` | enum | `unit_recruit` \| `heal_burst` \| `temp_buff` — the effect family. Only three exist in v0; do not add a fourth without a design doc update (rarity/evolution is explicitly out of scope). |
| `CollectedBy` | string | Who triggers pickup and how the pass is resolved. |
| `Effect` | string | Full mechanical description — the source of truth for what the drop does. |
| `HeroHealAmount` / `RetinueHealAmount` | float, HP | `kindling_ember_working` only. Flat HP restored to the hero / to each retinue unit inside `heal_burst_radius` at the moment of pickup. |
| `Notes` | string | Rationale and provenance, same role as `entity-tiers.json`'s `Notes` column. |

`rally_ember_working`'s numeric tuning (duration, per-stack %, stack cap)
lives in `design_constants` rather than as row-local fields, since those
three values are shared constants a single drop type reads, not a per-row
table — same reasoning `entity-tiers.json`'s `design_constants` block already
uses for `armor_chip_floor`/`reference_blow_value`.

## `drop_sources[]` row columns

One row per enemy tier that can produce a drop. Joins to
`docs/data/entity-tiers.json`'s `tiers[].Tier` values by name (`Fodder`,
`Soldier_Melee`, `Soldier_Ranged`, `Elite`, `Boss` — `Soldier_Melee`/
`Soldier_Ranged` split matches `scaling-curve.json`'s `floor_roster[].Tier`
naming, not `entity-tiers.json`'s single `Soldier` tier value, since drop
chance differs by sub-type reach even though both share the `Soldier` GDD
taxonomy tier).

| Column | Type | Meaning |
|---|---|---|
| `Name` | string (row key) | DataTable row key. |
| `Tier` | enum | Which enemy tier this row's chances apply to. |
| `UnitOrbChance` / `KindlingEmberChance` / `BattleBuffChance` | float, 0–1 | Per-kill drop probability for that tier, one column per drop type. `0.0` where the tier never drops that type via the probabilistic channel (e.g. Fodder never rolls a Unit Orb or a Rally Ember, only a Kindling Ember). |
| `UnitOrbGuaranteed` / `KindlingEmberGuaranteed` / `BattleBuffGuaranteed` | int | A fixed count granted on that tier's death, independent of the chance columns. Elite/Boss only (single, named-instance kills, not part of the Mass-Entity population roll). |

## `stacking_items_reference`

Not a row table — a pointer block. `catalog_ids` lists the `Name` values
already present in `docs/data/upgrades.json`'s `items.catalog`, purely so
this file is a complete index of "what Loot v0 is" without re-declaring
numbers that file already owns. **Do not add stat fields here** — if the
stacking-item catalog ever needs to change, that change belongs in
`upgrades.json`, per `SYSTEMS.md` §3's existing decision. Note that
`whetstone` in that catalog (a permanent, Ember-bought +DPS stack) and this
file's `rally_ember_working` (a free, temporary, battle-dropped +DPS stack)
are deliberately two different mechanisms toward a similar-shaped effect —
see `loot-v0.md` §6 for why they coexist rather than one superseding the
other.

## `simulation_summary`

Not a tuning input — a cached headline result of the Monte Carlo in
`docs/design/loot-v0.md`'s Simulation notes, kept here for quick reference
only. The design doc is the source of truth for method and assumptions; if
the two ever disagree, the design doc wins and this block should be
regenerated from it.

## Conventions carried over from sibling tables

- All values are **PROTOTYPE DIALS** — re-tune in play, not committed balance.
- Flat, typed columns within each array; no nesting a DataTable can't hold.
- Every dial gets an implied `Swarm.Loot.*` CVar name (noted inline in
  `design_constants`), following the same "every dial gets a console
  variable" convention `feeding.json`/`SwarmCombatProcessors.cpp` already use.
