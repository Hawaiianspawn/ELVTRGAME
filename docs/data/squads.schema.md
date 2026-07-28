# squads.json — schema

Companion data file to `docs/design/squad-group-system.md`. Nested config, not a flat
DataTable — same shape as `economy.json` (scalar/config values, not row-per-record
data), since none of these are per-instance rows a UE DataTable would import.
Per-type stat/formation/growth dials (Spearmen vs Archers) live in `unit-types.json`,
not here — this file is allocation, framing, and camera dials only.

**v0.3 (2026-07-27):** the typed-unit rework. "Squad" and "unit" name the same thing
now (spec §1.0). `squad_manager.squad_target_size_formula` (v0.2, single pool) is
replaced by `squad_manager.typed_allocation` (two pools, Spearmen claim first — see
spec §4.1). Adds `map_mode` (a new third view, spec §5.5) and
`army_view_lens_geometry.orthographic_check` (spec §5.1). `simulation_extrapolation`
gains `typed_allocation_trajectory` alongside the renamed (not removed)
`single_pool_*` fields from v0.2. `view_modes`/`selection_responsiveness`/
`yaw_envelope` carry over from v0.2 with wording updates only (squad → unit) except
where noted.

**v0.2 (2026-07-27):** added the two-mode framing revision. `framing_target` (v0.1)
was replaced by `view_modes.{army_view, unit_squad_view}`; `primary_squad_selection`
(v0.1) was renamed `primary_squad_selection_fallback` and narrowed to a fallback-only
role; `selection_responsiveness` and `army_view_lens_geometry` are new. Nothing else
changed shape.

## `squad_manager`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `max_squads` | int | 1-16, currently **8** | fixed cap on command-group count, not power — SHARED across both unit types (`typed_allocation` below), not doubled |
| `typed_allocation.formula` | string (formula) | — | per-type `ceil(pool / squad_size_legibility_ceiling)`, Spearmen claim their share of `max_squads` first, Archers get the remainder (folding overflow into their own units if exhausted) — replaces v0.2's single-pool `squad_target_size_formula` |
| `typed_allocation.claim_order` | array | — | `["spearmen", "archers"]` — a real policy call (spec §4.1), not derived |
| `squad_size_legibility_ceiling` | int | bodies, currently **80** | approximate size past which a unit stops reading as one shape — a Unit/Squad View constraint (spec §4.2/§7), distinct from `army_view_lens_geometry` below; applied per-type independently as of v0.3 |
| `membership_assignment` | string (policy) | — | must be assigned once at recruit time (now: fill-lowest-first WITHIN the soldier's type); must NOT be re-derived from the per-frame formation repack; type is assigned at the same moment and is equally permanent |

## `view_modes.army_view`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `subject` | string | — | the whole standing retinue, as ≤`max_squads` blocks |
| `representation` | enum string | `aggregate` | never individual billboards — see `representation_reason` |
| `fraction_of_retinue` | float | fixed **1.0** | 100% of squads always represented; not a body-count fraction |
| `sacrifices_first` | enum string | `per_soldier_fidelity` | traded up front, not per-frame |
| `block_sizing` | string (formula) | — | `standing / target-size-for-that-type` |
| `block_position` | string | — | squad centroid |
| `block_tint` | string | — | current squad stance |
| `block_type_marker` | string | — | v0.3 NEW — label/icon per block distinguishing Spearmen vs Archers; rendering detail, still O(`max_squads`) draws |
| `yaw` | string | — | none — fixed layout, does not use `yaw_envelope` |

## `map_mode` (v0.3 NEW)

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `representation` | string | — | real per-soldier positions, both types, orthographic top-down — NOT `army_view`'s aggregate blocks |
| `coverage` | string | — | full leash-bound worst-case spread (4000uu), by fixed scale rather than camera pull-back |
| `recommended_panel_height_px` | int | px, **1200** | from `army_view_lens_geometry.orthographic_check`'s target-height table — the floor is ~1000px (15px sprites) |
| `selection` | string | — | same selection action as a muster-card click / hotkey |
| `leash_visualization` | string | — | flagged opportunity (`FLAME-FOUNDATION.md` §3a), not specced or built here |
| `cost_class` | string | — | O(N) draw pass — the first mode to give up the O(`max_squads`) guarantee, see spec §8 |

## `view_modes.unit_squad_view.{unit_cam_panel, view_feed_panel}`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `fraction_of_selected_squad` | float | 0.0-1.0 | 0.60 (UnitCam) / 0.80 (ViewFeed) — read against the **selected** unit's standing, not the whole retinue |
| `body_floor` | int | bodies, currently **6** | overrides the fraction at low unit counts |
| `sacrifices_first` | enum string | `proximity_to_bearer` | what the camera gives up when it can't fit the target |
| `protects` | enum string | `squad_cohesion` | what the camera never crops to make room |

## `selection_responsiveness`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `driver` | string | — | muster card click / hotkey 1-8; same action as command addressing |
| `transition.mode` | enum string | `travel` | not a hard cut |
| `transition.speed` | float | `VInterpTo` speed units, currently **10** | between `FollowSpeed` (6) and `CastFocusSpeed` (12); ~0.3s settle |
| `persistence.mode` | enum string | `latch` | default; NOT idle-decay |
| `persistence.idle_decay_alternative_seconds` | float | seconds, **6** | documented alternative dial only, not the default |
| `resting_state_no_selection` | enum string | `army_view` | camera's neutral default |
| `explicit_vs_auto_fallback` | string (rule) | — | explicit selection always wins; auto-fallback only fires with no prior selection |
| `selected_squad_wiped_mid_fight` | enum string | `army_view` | drop to resting state, never auto-reselect another unit |
| `cast_focus_precedence` | string | — | cast-focus still outranks everything above, unchanged from v0.1 |

## `primary_squad_selection_fallback`

| Field | Type | Notes |
|---|---|---|
| `rule` | string | fallback selection rule, used only when Unit/Squad View has no explicit player selection |
| `tiebreak` | string | applied when multiple squads tie on the `rule` metric |
| `no_combat_fallback` | string | selection rule when no squad has nearby brood |
| `wipe_fallback` | string | selection rule when the retinue is at zero |

## `yaw_envelope`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `scope` | string | — | Unit/Squad View only — Army View has no yaw concept |
| `clamp_deg` | float | degrees, currently **30** | hard envelope off `base_heading`, both directions |
| `base_heading` | string (rule) | — | bearer → selected-squad-centroid direction; falls back to bearer's last movement heading |
| `look_lerp` | float | ease rate, currently **1.5** | lower = lazier pans; was 3.0 pre-clamp |
| `settle_target_deg` | float | degrees, currently **0** | offset the yaw eases toward when nothing is pulling it |
| `cast_focus_override` | string | — | unaffected by this envelope; documented for completeness |

## `army_view_lens_geometry`

| Field | Type | Units / range | Notes |
|---|---|---|---|
| `leash_radius_uu` | float | uu, **2000** | from `RTS-VERTICAL-SLICE.md` §2 |
| `worst_case_spread_diameter_uu` | float | uu, **4000** | 2× leash radius |
| `cam_fwd_to_cover_worst_case_uu` | float | uu, **~5495** | virtual-camera distance needed to guarantee full coverage |
| `sprite_height_px_at_that_distance` | object | px | per panel-size extreme, at the coverage distance |
| `cam_fwd_for_15px_legibility_uu` | object | uu | per panel-size extreme, distance past which sprites drop below ~15px |
| `area_saturation_body_count_at_worst_case_distance` | int | bodies, **~6584** | headcount at which sprite area alone would saturate the panel — far above any modeled retinue size |
| `orthographic_check.*` | object | — | v0.3 NEW (spec §5.1) — re-derives the same coverage math under a fixed-scale (orthographic) projection instead of perspective; `finding` states the headline (projection method doesn't rescue legibility, matches the perspective numbers); `panel_height_needed_for_target_sprite_height_px` is what `map_mode.recommended_panel_height_px` is drawn from |

Estimates from lens-geometry math (`UnitCamProjector.cpp` constants + the leash
radius), not a measured play session. See spec §5.1/§7 for the full derivation and the
caveats on which numbers are robust (the qualitative conclusion) vs. approximate
(the exact uu/px crossings).

## `simulation_extrapolation`

Records the assumptions behind `docs/design/squad-group-system.md`'s growth-trajectory
sims so a future re-run can be checked against real economy data once the bigger
retinue stage exists. `growth_rate_per_site` and the resulting crossing points are
explicitly **not measured** — placeholders pending real data. Distinct from
`army_view_lens_geometry`, which is a separate finding.

`single_pool_*` fields are v0.2's original single-undifferentiated-pool finding,
kept for reference. `typed_allocation_trajectory` (v0.3 NEW, spec §4.2) is the same
growth assumptions re-run against the two-pool Spearmen/Archer split — its `finding`
field is the important one: the typed model breaks *sooner* than the single-pool
model, and for a different reason (cross-type competition for the fixed 8-handle
budget, not single-type growth outpacing it).
