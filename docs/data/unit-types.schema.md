# unit-types.json — schema

Companion data file to `docs/design/squad-group-system.md` §1/§2/§4. Nested config,
not a flat DataTable — same shape as `squads.json`/`economy.json` (scalar/config
values, not row-per-instance data): there is no per-instance row here, only two
records (`spearmen`, `archers`), each with sub-tables of scalar dials.

**v0.1 (2026-07-27):** new file, introduced alongside the typed-unit rework of
`squad-group-system.md`.

**v0.2 (2026-08-08):** added the `adaptation` block (evolution ladders — see
`docs/design/adaptation.md`), and **documented `types.<type>.melee_subtypes`, which shipped
with task-088 and never had a schema section**. The `melee_subtypes` gap is paid off here
rather than left, because `adaptation` reuses its exact atlas-index binding pattern and is
unreadable without it.

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

## `types.<type>.melee_subtypes`

*(task-088; documented retroactively in v0.2. Present on `spearmen` only — archers have no
sub-type table anywhere, see the gap note under `adaptation` below.)*

A template→variant roster with an override rule and a visual binding. Three keys:

| Field | Type | Notes |
|---|---|---|
| `note` | string | states the override rule; quoted below because it is load-bearing |
| `rows` | dict of row-id → row | 9 rows; each `{ note, combat }` where `combat` is the **same six fields as `types.<type>.combat`** |
| `variant_to_row` | array | 11 entries, `{ index, id, row }` |

**The override rule**, verbatim from the file: *"Each row REPLACES the combat block above…
formation, stance_reflavor, and growth_source_weight are unchanged and shared by every row
(only the combat stat block varies per row — GDD §10 Mass Entity data-cheap-soldier
constraint)."* A row is a **complete** combat block, not a delta.

`variant_to_row[].index` is the **atlas index** and is the binding key. The file says so
outright: the `id` is a convenience label and *must not* be treated as the binding key.

## `adaptation`

*(v0.2, 2026-08-08. Decision home: `docs/design/adaptation.md`. Amends `SYSTEMS.md` §7 and
`GDD.md` Q31.)*

Evolution ladders. A **rung** is the triple `(unit_type, tier, variant_index)` — all three
keys already exist elsewhere and none is restated here.

| Field | Type | Units / range | Meaning |
|---|---|---|---|
| `note` | string | — | job, decision home, trust status. Closes with the PROTOTYPE DIALS clause |
| `handle_rule` | string | — | one command handle per **branch**, not per rung; a captain plus its retinue is one handle |
| `ladders` | array of ladder | — | one entry per ladder |

### `adaptation.ladders[]`

| Field | Type | Units / range | Meaning |
|---|---|---|---|
| `id` | string | kebab-case | ladder identity |
| `side` | enum string | `friendly` \| `enemy` | **the enemy reservation.** Friendly rungs key `tier` into `upgrades.json` `tier_ladder`; enemy rungs would key `entity-tiers.json`. The triple is identical either way, so enemy ladders need no schema change. v1 is friendly-only (owner, 2026-08-08) |
| `unit_type` | key | `spearmen` \| `archers` | keys `types` above |
| `families` | array of string | — | the `docs/data/art/families/<f>` folders this ladder draws from. It inherits their `axis` + `constant` as its identity guarantee. **Usually one.** More than one is only legitimate when the folders share a PixelLab base character — `spearmen-line` spans `knight-melee-v1`/`v2` because both descend from character `1c935515`, so the one-axis rule holds at the **body-group** level even though the folder split does not. A multi-folder ladder MUST carry a `families_note` saying why |
| `rungs` | array of rung | — | **array order IS the rank.** See the anti-rule below |
| `separation_note` | string | — | how this ladder's rungs were shown to read apart, and **at what epistemic grade**. Say `MEASURED`/`CITED` with the numbers, or say plainly that it is qualitative — `archer-scout` carries "QUALITATIVE, NOT MEASURED" because its silhouette deltas are under ~6% and one is negative. A ladder with no honest separation grade is not finished |
| `shop` | object | — | see below |

### `adaptation.ladders[].rungs[]`

| Field | Type | Units / range | Meaning |
|---|---|---|---|
| `tier` | key | `freed`\|`militia`\|`veteran`\|`bannerman` | keys `upgrades.json` `tier_ladder.tiers[]` — **the stat spine. No HP/DPS appears in this file.** |
| `variant_index` | int | 0-23 team atlas | the atlas index, same binding key `melee_subtypes.variant_to_row[].index` uses |
| `id` | string | kebab-case | rung identity, a label — never a binding key |
| `captain` | bool | — | present and `true` on the top rung only |
| `captain_grammar` | string | — | captain rungs only: **why this look reads as a leader**. Three grammars are in use and they are not interchangeable — `bulk` (largest look in the branch), `salience` (highest-contrast/most-enclosed, a "point of glow" marker), and `pose` (reads as an individual rather than as an object). Per-branch, not one rule for all. `pose` is the weakest of the three |
| `captain_flag` | string | — | optional. A captain assignment the art pass itself does not trust. `archer-siege`'s `rocketshoulder` carries one: it is the best look **on disk**, not the right look, and taking it cost that branch its monotonic growth at the top rung. **A named weak pick beats a silent one** — and where the flag names the art that would actually solve it, that is the system's own art brief |
| `retinue` | object | — | captain rungs only: `{ size, unit_type, tier, variant_index }` |
| `retinue_size_note` | string | — | `JUDGMENT CALL` — ≤ 8, half `TypeLegibilityCeiling` (`SwarmSubsystem.h:59`). No measurement gives a captain retinue size |
| `upkeep_per_retinue_body` | int \| **null** | Supply demand | **`null` = OPEN.** Whether captain retinue draws Supply was never put to the owner |

**Two anti-rules, stated because both are easy to get backwards:**

> 1. **`variant_index` is NOT the rank.** Atlas order is arbitrary-by-history and frozen —
>    `Scripts/art/atlas.py:306-309` warns that inserting anywhere but the end silently
>    re-points every later stat row. Rank is `rungs[]` array order, the same convention
>    `Scripts/sim/decisions.py:77-78` already reads off `tier_ladder` key order.
> 2. **Every `variant_index` must resolve to an atlas row that already exists**, or sit inside
>    free 4-bit capacity (3 archer / 5 spearman slots). `py Scripts/art/atlas.py check --all`
>    must come back byte-identical. Adaptation adds zero repack debt — an acceptance
>    criterion, not a preference.

### `adaptation.ladders[].shop`

| Field | Type | Units / range | Meaning |
|---|---|---|---|
| `price_gold` | int \| **null** | gold | **`null` = OPEN (O8), blocked on O4.** Gold's drop rate and sources are undecided (`GDD.md` Q26); pricing before that is invention |
| `price_note` | string | — | carries the O4/Q26 citation |
| `stock_rule` | string | — | the rule that needs no number: *the shop offers rungs on branches the player's pick did not grant* |
| `captain_note` | string | — | `bannerman` is not purchasable — `upgrades.json`'s own trait says item/event reward only |

**Known gap, deliberate:** archers have **no per-tier row** anywhere in `docs/data/`.
`Scripts/sim/data_loader.py:126-136` synthesises archer tier stats by scaling the spearmen
ratio with a stamped `ASSUMED` note, and `:149` hardcodes `armor: 0.0`. Adding real archer
tier rows would silently move every existing archer number in the repo. Not done here;
recorded so the next reader does not mistake it for an oversight.

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

## Conventions

Carried over from sibling tables (`entity-tiers.schema.md`, `wave-scaling.schema.md`,
`scenarios.schema.md`), and added in v0.2 because this file never carried the section:

- **All values are PROTOTYPE DIALS** (first pass, unmeasured) unless a `*_note` says `CITED`
  and names the source — same status as `economy.json` and `squads.json`. `CVAR-SHIPPED` in a
  note means the value mirrors a live default in `ELVTR/Source/**` and moving it here does
  **not** move the game.
- **Reuse a column, don't invent a second stat.** `combat` is the one per-unit stat block;
  `melee_subtypes.rows[].combat` and any future ladder override use those exact six fields.
  The friendly HP/DPS ladder lives in `upgrades.json` `tier_ladder` and is never restated
  here — `adaptation` keys into it.
- **An unanswered number is an explicit `null` plus a sibling `*_note` naming the open
  question**, never a plausible guess and never an omitted key. `price_gold` and
  `upkeep_per_retinue_body` are the two in this file.
- **Markers are all-caps inline in the note prose**, not separate fields: `CITED`,
  `UNVERIFIED`, `JUDGMENT CALL`, `ASSUMED`.
- **Atlas indices are load-bearing identity.** Any field typed "atlas index" binds a sprite
  row, a combat stat row and a 4-bit render field at once. Reordering the atlas breaks all
  three silently — see `ELVTR/Source/ELVTR/Mass/SwarmFragments.h:37-118` for the four-things-
  must-agree contract.
