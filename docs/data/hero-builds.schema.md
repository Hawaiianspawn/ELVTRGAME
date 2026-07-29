# hero-builds.json — schema

Companion data file to `docs/design/hero-build-variety.md`. Six axis tables
(`chassis`, `weapon_archetypes`, `projectiles`, `modifications`, `abilities`,
`origin_worlds`), a constraint table (each `chassis` row's own `legal_*`
lists), a machine-checkable `synergy_rules` list, and two small worked-data
blocks (`calibration_builds`, `worked_example_builds`). No per-instance row
data — this is a **combinatorial generator's inputs**, not an enumerated
roster. Simulation-space only (owner, 2026-07-29): nothing here is UE
DataTable-imported or runtime-read yet; a Python script is the only consumer
this version targets.

**v0.1 (2026-07-29):** new file, introduced alongside
`docs/design/hero-build-variety.md`.

---

## `reachable_build_count`

Not gameplay data — a computed record of the combinatorics, kept in the file
so the raw-vs-legal claim in the design doc has a citable source.

| Field | Type | Notes |
|---|---|---|
| `raw_product` | int | `chassis x weapon_archetypes x projectiles x (modifications+1) x (abilities+1) x origin_worlds`, no constraints applied |
| `legal_count` | int | Sum over chassis of `legal_weapons x legal_projectiles x (legal_modifications+1) x (legal_abilities+1) x legal_origin_worlds` |
| `per_chassis` | object | `legal_count`'s per-chassis breakdown, `{chassis_id: int}` |

## `chassis.<id>`

A **build's visual/role identity** — what a player recognizes at a glance,
and the axis every other axis is constrained against.

| Field | Type | Notes |
|---|---|---|
| `display_name` | string | player-facing name |
| `role` | string | free-text role tag (not consumed by anything, a reading aid) |
| `note` | string | provenance; `"CALIBRATION CHASSIS"` marks the two rows whose `base_stats` are an exact carry-over of a `unit-types.json` row |
| `base_stats.max_hp` | float | HP |
| `base_stats.armor` | float | flat armor points (`entity-tiers.md` §2.2's mechanism) |
| `base_stats.move_speed_scale` | float | multiplier, flavor/non-load-bearing (matches `unit-types.schema.md`'s own field) |
| `legal_weapons` | array of `weapon_archetypes` ids | the chassis-coherence constraint (owner: "somewhat themed consistently to their character view") |
| `legal_projectiles` | array of `projectiles` ids | same |
| `legal_modifications` | array of `modifications` ids | same; **`"none"` (no modification picked) is always additionally legal for every chassis and is NOT listed here** — the `(legal_modifications+1)` term in `reachable_build_count` accounts for it |
| `legal_abilities` | array of `abilities` ids | same; **`"none"` always additionally legal**, same convention |
| `legal_origin_worlds` | array of `origin_worlds` ids | which of the six worlds can plausibly produce this chassis; NOT constrained by weapon/projectile choice — origin-world is orthogonal to the weapon axes by design (see design doc §"why origin-world isn't cross-constrained") |

A build = one pick from each of `legal_weapons` / `legal_projectiles` /
(`legal_modifications` ∪ none) / (`legal_abilities` ∪ none) /
`legal_origin_worlds`, for a chosen chassis.

## `weapon_archetypes.<id>`

Every field the sim needs to compute a `build_stat_block` — see that section
below for the derivation. Two rows (`precision_longbow`, `cleave_melee_sweep`)
are CALIBRATION rows, tuned to reproduce `unit-types.json`'s Archers/Spearmen
exactly; every other row is a PROTOTYPE DIAL.

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `display_name` | string | | |
| `note` | string | | provenance — cites the exact source number for the two calibration rows |
| `rate_of_fire` | float | shots/sec | `swing_interval = 1 / rate_of_fire` |
| `range` | float | uu | maps to `engage_range` |
| `min_range` | float | uu | maps to `min_engage_range` |
| `damage_per_shot` | float | HP | pre-accuracy, pre-modification |
| `targets_per_shot` | int | count | maps to `targets_per_hit`, before any `projectiles.<id>.resolve_type == "area"` conversion |
| `aoe_radius` | float | uu, 0 if none | only read when the rolled `projectiles` row has `resolve_type: "area"` — see the area-conversion formula below |
| `accuracy` | float | 0-1 | hit-chance multiplier, combined with the projectile's `accuracy_modifier` |
| `flavor_names` | array of strings | | player-facing skins from different origin-worlds for the SAME numeric row — this is how "an RPG archer" and "a laser cannon" can be two flavors of what is mechanically one archetype family, or (as in the worked example) two entirely different archetype rows under one chassis; either is a legal way to express the owner's ask |

## `projectiles.<id>`

Its own axis (owner: "their projectile can be different") — what travels, how,
and how it resolves. Split explicitly into sim-consumable vs. presentation-only
fields.

| Field | Type | Units / range | Sim status | Notes |
|---|---|---|---|---|
| `display_name` | string | | — | |
| `travel_type` | enum string | `hitscan` \| `ballistic_arc` \| `physical_straight` \| `melee_instant` \| `homing` | **PRESENTATION-ONLY** | no projectile-travel-time primitive exists in either combat model (`docs/sim/LIMITATIONS.md` §4) |
| `resolve_type` | enum string | `point` \| `pierce` \| `area` | **SIM-CONSUMABLE** | governs the `targets_per_hit` derivation — see below |
| `travel_speed_uu_s` | float or `"instant"` | uu/sec | **PRESENTATION-ONLY** | same reason as `travel_type` |
| `accuracy_modifier` | float | additive, roughly -0.10 to +0.15 | **SIM-CONSUMABLE** | added to the weapon's `accuracy`, then clamped `[0,1]`, before computing `dps` |
| `flavor_names` | array of strings | | — | |

**`resolve_type` -> `targets_per_hit` derivation** (part of the `build_stat_block`
pipeline below):

```
targets_per_shot_effective = weapon.targets_per_shot + (modification.targets_per_shot_add or 0)
if projectile.resolve_type == "area" and weapon.aoe_radius > 0:
    targets_per_hit = max(targets_per_shot_effective, round(weapon.aoe_radius / 40))
else:
    targets_per_hit = targets_per_shot_effective
```

The `aoe_radius / 40` conversion is a circle-packing Fermi estimate (40uu
per body — the same spirit as `entity-tiers.json`'s `SurroundCapEstimate`,
not a measurement) converting a splash radius into an equivalent discrete
cleave count so the flat `targets_per_hit` primitive can consume an area
weapon at all. `pierce` and `point` need no conversion — `weapon.targets_per_shot`
already IS the chain/pierce count for a `pierce` row.

## `modifications.<id>`

**A MODIFICATION alters the build's own numbers** (owner distinction, §2c of
the task brief) — a multiplicative or additive delta applied once, at build
resolution, never re-evaluated mid-fight.

| Field | Type | Notes |
|---|---|---|
| `display_name` | string | |
| `kind` | const `"modification"` | disambiguates from `abilities` rows sharing a rules engine later |
| `effect` | object | one or more of `range_mult`, `rate_of_fire_mult`, `damage_per_shot_mult`, `max_hp_mult`, `move_speed_scale_mult`, `targets_per_shot_add`, `armor_penetration_flat` — see `build_stat_block` derivation for how each is applied |
| `note` | string | flags `piercing_rounds` as needing a one-line `EffectiveBlow` extension the sim doesn't have yet (see `docs/design/hero-build-variety.md`) |

## `abilities.<id>`

**An ABILITY is a conditional or periodic effect** (owner distinction, §2c) —
state- or time-dependent, not a flat number folded into the build's steady-state
stat block. Every row states its own representability plainly rather than
encoding a number the sim would silently misuse.

| Field | Type | Notes |
|---|---|---|
| `display_name` | string | |
| `kind` | const `"ability"` | |
| `representable` | enum string | `"approximate"` \| `"no"` \| `"partial"` — whether either combat model (`Scripts/sim/combat_model.py`) can consume this ability at all |
| `representable_note` | string | the specific reason, and (for `"approximate"`/`"partial"`) exactly what approximation would be needed and which model(s) it's limited to |

**None of these six abilities write a field into `build_stat_block` directly.**
A consumer (task-080) that wants to use `rally_cry` or `chain_reactor` has to
implement the approximation described in `representable_note` itself — this
file states the effect and its honest limits, not a pre-computed number.

## `origin_worlds.<id>`

The synergy key (owner: cross-world merge is current narrative canon,
`docs/narrative/FLAME-FOUNDATION.md` §4.4 — `WORLD.md` is SUPERSEDED and not
cited for any of these names).

| Field | Type | Notes |
|---|---|---|
| `display_name` | string | |
| `working_name_only` | const `true` | same convention `entity-tiers.json`'s `brood_*` rows use — no world names are canon yet |
| `flavor_note` | string | one-line genre/flavor description |

## `synergy_rules.rules[]`

Machine-checkable, evaluated against a **rolled roster** — a list of resolved
builds (each an axis-pick tuple plus its `build_stat_block`). This is the
concrete shape task-080's evaluator should target.

| Field | Type | Notes |
|---|---|---|
| `id` | string | unique |
| `kind` | enum string | `"bonus"` \| `"anti_synergy"` — a reading aid; the `effect.value` (>1 vs <1, or the stat it targets) is what actually determines direction |
| `description` | string | |
| `condition` | object | see condition types below |
| `scope` | enum string | which units in the roster the `effect` applies to — `"units_matching_condition_origin"` (only the majority-world units), `"units_matching_origin_in_pair"` (only units whose `origin_world` is one of `condition.worlds`), `"whole_roster"` (every unit, regardless of what triggered the condition) |
| `effect` | object | `{ "stat": <build_stat_block field name, or "accuracy" pre-dps>, "op": "multiply", "value": <float> }` |

**Condition types:**

| `condition.type` | Fields | Evaluates to true when... |
|---|---|---|
| `origin_world_share` | `op`, `threshold` (0-1) | the roster's single most-common `origin_world` (by unit count, not soldier headcount) makes up `>= threshold` of all units in the roster |
| `origin_world_pair_present` | `worlds` (array of 2 ids) | at least one unit of EACH named world exists in the roster |
| `ability_id_count` | `ability_id`, `op`, `threshold` | count of units in the roster whose `ability` equals `ability_id` meets the threshold |
| `distinct_origin_world_count` | `op`, `threshold` | the number of distinct `origin_world` values present in the roster meets the threshold |

Every rule's `effect.stat` must be a field task-080 can actually multiply —
`dps`, `rate_of_fire` (pre-`swing_interval`-inversion), and `accuracy`
(pre-`dps`-computation) are the three used here; extending to any other
`build_stat_block` field is a straightforward addition, not a new shape.

## `calibration_builds` / `worked_example_builds`

Fully-resolved example builds: axis picks (`chassis`/`weapon`/`projectile`/
`modification`/`ability`/`origin_world`, any of the last four may be `null`
for "none") plus the resulting `build_stat_block`. Not a data table to grow —
illustrative fixtures, verified against the derivation script in
`docs/design/hero-build-variety.md`'s Simulation notes.

## The `build_stat_block` — the one comparable measurement per build

Every unique type in this space resolves to exactly these eight fields, in
these units — the same flat shape `Scripts/sim/data_loader.py`'s
`retinue_fighter()` / `enemy_fighter()` already produce, so a build here can
be dropped into the existing sim harness with no translation layer:

| Field | Type | Units | Derivation |
|---|---|---|---|
| `max_hp` | float | HP | `chassis.base_stats.max_hp x (modification.max_hp_mult or 1)` |
| `armor` | float | flat mitigation points | `chassis.base_stats.armor` (modifications don't touch the wielder's own armor in v0.1 — `piercing_rounds` touches the VICTIM's armor at resolution time, a separate mechanism, see `modifications.piercing_rounds.note`) |
| `dps` | float | damage/sec, steady-state | `effective_damage_per_shot x effective_rate_of_fire x effective_accuracy` — see full derivation in `docs/design/hero-build-variety.md` |
| `swing_interval` | float | sec | `1 / effective_rate_of_fire` |
| `engage_range` | float | uu | `weapon.range x (modification.range_mult or 1)` |
| `min_engage_range` | float | uu | `weapon.min_range` (no modification currently touches this) |
| `targets_per_hit` | int | count | see the `resolve_type` derivation above |
| `move_speed_scale` | float | multiplier | `chassis.base_stats.move_speed_scale x (modification.move_speed_scale_mult or 1)` |

`effective_rate_of_fire = weapon.rate_of_fire x (modification.rate_of_fire_mult or 1)`,
`effective_damage_per_shot = weapon.damage_per_shot x (modification.damage_per_shot_mult or 1)`,
`effective_accuracy = clamp(weapon.accuracy + projectile.accuracy_modifier, 0, 1)`.
