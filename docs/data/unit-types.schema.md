# unit-types.json — schema

Companion data file to `docs/design/squad-group-system.md` §1/§2/§4. Nested config,
not a flat DataTable — same shape as `squads.json`/`economy.json` (scalar/config
values, not row-per-instance data): there is no per-instance row here, only two
records (`spearmen`, `archers`), each with sub-tables of scalar dials.

**v0.1 (2026-07-27):** new file, introduced alongside the typed-unit rework of
`squad-group-system.md`.

## `types.<type>`

| Field | Type | Notes |
|---|---|---|
| `display_name` | string | player-facing name |
| `role` | enum string | `melee_line` \| `ranged_line` |
| `note` | string | provenance / measurement status |

## `types.<type>.combat`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `max_hp` | float | HP | mirrors `SwarmCombatTuning::RetinueMaxHP()` for Spearmen; a new per-type value for Archers |
| `dps` | float | damage/sec | mirrors `RetinueDPS()` for Spearmen |
| `engage_range` | float | uu | generalizes `MeleeRange()` — for Spearmen this IS `MeleeRange` (95); for Archers it's the new ranged reach |
| `min_engage_range` | float | uu | floor below which this type won't target — Archers' only line-of-fire mitigation (§2.2); 0 = no floor (Spearmen) |
| `targets_per_hit` | int | count (K) | generalizes `RetinueTargetsPerHit()`/`BroodTargetsPerHit()` to per-type; Archers default 1 (precision, no free cleave) |
| `move_speed_scale` | float | multiplier | flavor dial, not load-bearing |

## `types.<type>.formation`

Field names are exactly `SwarmFormation::FParams`' members (`ELVTR/Source/ELVTR/Mass/SwarmFormation.h`) — no parallel vocabulary. Proposed as the shipped defaults for a second, type-namespaced CVar set (`Swarm.Formation.<Type>.*`), mirroring the existing `Swarm.BroodFormation.*` / `Swarm.Formation.*` precedent.

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `shape` | enum string | `Block` \| `Wedge` \| `Arc` \| `Ring` | both v1 types default `Block` |
| `columns` | int | 1-64 | slots per rank |
| `spacing` | float | uu | lateral gap within a rank |
| `rank_spacing` | float | uu | depth gap between ranks |
| `forward` | float | uu | push away from camera/bearer — the load-bearing "who stands where" number (§1.7) |
| `arc_degrees` | float | degrees | unused while `shape` is `Block`; carried for parity with the retinue's own params in case a type moves to `Arc` later |
| `arc_radius` | float | uu | same |

## `types.<type>.growth_source_weight`

Float, 0-1, sums to 1 across types. Fraction of generator-tagged growth/rescue sites
that yield this type (spec §1.4). Not a live economy — a generator-time tag, per GDD
§9's "flags, not simulation" scope guardrail.

## `types.<type>.stance_reflavor`

Four strings (`follow`/`charge`/`hold`/`rally`), citable prose matching
`squad-group-system.md` §1.8's table and (for Spearmen) `CLASSES.md` §1's existing
Vanguard reflavors. Not consumed by code — a design-intent record for whoever
implements the per-(type, stance) `EngageRange`/behavior lookup.

## `allocation`

| Field | Type | Notes |
|---|---|---|
| `unit_claim_order` | array of type keys | order types claim a share of `squads.json`'s `max_squads` budget — spec §4.1's "Spearmen claim first" rule |

## `ranged_combat_model`

Booleans documenting what the v1 minimum-viable model explicitly does NOT include
(spec §2.3's "what is explicitly not v1" list), so a future reader can tell an
intentional omission from an oversight. `note` records the one mitigation that does
exist (`min_engage_range`) and points the VFX ask (the volley visual) at
performance-director rather than claiming it's specced here.
