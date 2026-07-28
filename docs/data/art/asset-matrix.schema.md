# asset-matrix.json — schema

Companion data file to the art pipeline (`docs/data/art/sprite-request.schema.json`,
`.claude/skills/sprite/SKILL.md`) and consumed by `Scripts/art/coverage.py` /
`.claude/skills/art-coverage/SKILL.md`. Same house shape as `docs/data/unit-types.json`
and `docs/data/squads.json` — nested config, not a flat DataTable, hand-authored and
committed.

**What this file is, and is not.** A sprite *request*
(`docs/data/art/requests/<id>.json`) describes one thing that was **asked for** —
prompt, palette canon, generation params. This file describes what a character
**needs to exist and be drawn**, independent of whether a request was ever filed for
it. The two are different layers on purpose: a request can be fulfilled and still
leave a character's matrix entry incomplete (the archer proxy has a request-shaped
asset but no `source_request` file at all); a request can exist and be fully honoured
while the matrix entry it feeds is still `unwired` (every soldier variant).

**v1 (2026-07-27):** first cut. Every entry below was derived from one of three
places, and each entry's `derived_from` field says which:

1. **A schema-validated request's own `output` block**
   (`docs/data/art/requests/<id>.json`) — the strongest source, since
   `sprite-request.schema.json` already validates it. Used for every entry that has
   a `source_request`.
2. **The code's own sheet contract** — `ELVTR/Source/ELVTR/Mass/SwarmFragments.h`
   (`SwarmSheet`, the 8-column/4-row grid `T_Swarm_2bit` must match) and
   `ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp` (what it actually loads and how it
   slices cells). Used wherever a request's own geometry needs to be checked against
   what the consumer actually expects, and for `T_Soldier_Archer`, which has no
   request file at all.
3. **A prose spec's acceptance checklist** (`docs/art/*.md`) — for palette/canon
   fields (`value_dominance`, `pale_usage`) where the request doesn't restate them,
   or where the prose and the request disagree and both are recorded rather than
   silently picking one (see `soldier-roster-v1.md`'s stale "4×1 → 192×48" header
   vs. the request's schema-validated `[4, 2]` grid — `open_questions` on the
   `soldier-*` entries name this rather than resolve it).

**Nothing here is invented.** Where deriving a requirement would require a design
decision nobody has made — whether a retinue variant needs a real walk frame per
direction, whether the legacy `T_Unit_Retinue`/`T_Hero_Vanguard` assets are still
required now that the typed-unit model and the single-Bearer decision exist — the
entry says so in `open_questions` instead of picking an answer. The audit still runs
against the entry as filed (usually "required, at whatever geometry the request/code
already commits to"), and its findings should be read with the open question in mind.

## Top level

| Field | Type | Notes |
|---|---|---|
| `version` | string | matrix schema version, not a request's `revision` |
| `updated` | string (date) | |
| `derived_from` | array of strings | every source doc/file consulted to build this matrix, so a future edit can tell what to re-check |
| `excluded` | array of objects | requests/assets deliberately left out of the matrix, each `{id, reason}` — e.g. the `style-soldier-*` batch (a rendering test, not a character) and `protagonist-01/02/03` (rejected prototypes, superseded by `protagonist-04`/`bearer`) |
| `families` | object | groups entries for reporting; `{key: {display_name, note}}` |
| `characters` | array of objects | the actual matrix rows — see below |
| `global_open_questions` | array of strings | design questions that don't belong to one entry |

## `characters[]`

| Field | Type | Notes |
|---|---|---|
| `id` | string | matches the UE texture name (`T_Hero_Bearer`, …) — the operationally meaningful unit, since "does this asset exist / is it wired / is it on-ramp" all key off one texture |
| `character` | string | human label for the roster entity this texture belongs to |
| `family` | string | key into `families` |
| `kind` | enum | `hero` \| `retinue` \| `brood` \| `composite` — mirrors `sprite-request.schema.json`'s `subject.kind` where applicable, plus `composite` for a shared multi-source atlas |
| `required` | bool | is this texture currently asked for by the live game (code or a current spec)? Legacy/contested entries stay `true` with the contest recorded in `open_questions` rather than being silently marked `false` — deciding that is not this file's job |
| `source_request` | string \| null | path to the request JSON this entry's geometry came from, relative to repo root; `null` when no request exists (the archer today) |
| `spec` | string \| null | path to the prose spec, repo-relative |
| `derived_from` | array of strings | which of the three source kinds above this entry's fields came from |
| `texture` | string | UE texture asset name, same as `id` |
| `content_path` | string | expected `/Game/...` package path |
| `disk_sheet` | string | expected path under `RawArt/Sheets/`, repo-relative |
| `sheet` | object | see below |
| `required_frames` | array of strings \| null | explicit frame keys (`"south.idle"`, …) this texture must carry; `null` when the entry instead states a `required_frame_count` because no per-key contract exists yet |
| `required_frame_count` | int \| null | used instead of `required_frames` when only a count is known (the archer: 16 = 8 directions × {idle, walk1}, by analogy with every other hero/retinue sheet, not from any filed request) |
| `palette` | object | `{ref: "demichrome-4", value_dominance, pale_usage}` — copied from the request's `canon` block or the spec's checklist |
| `wiring` | object | `{expected_consumer, check}` — see below |
| `provenance_id` | string | the key this entry should appear under in `docs/data/art/provenance.json`, once that file exists |
| `open_questions` | array of strings | design questions specific to this entry, left open on purpose |
| `notes` | string | free text — measured discrepancies, history, anything that would mislead a reader who only saw the fields |

### `sheet`

| Field | Type | Notes |
|---|---|---|
| `cell` | int | px, locked to 48 for every kind except `ui` (`sprite-request.schema.json`) |
| `grid` | `[cols, rows]` | both powers of two |
| `frame_axes` | array of strings | what varies across the grid's cells for THIS texture — e.g. `["direction"]` (soldier variants: 8 idle poses, one per direction, no walk variation), `["direction", "walk_frame"]` (the Bearer/Vanguard hero sheets and the `SwarmSheet` code contract), `["action"]` (the legacy `T_Unit_Retinue`: walk0/walk1/attack/hit on one direction). Two textures with different `frame_axes` are not drop-in compatible even at the same `cell`/`grid` — this is where the soldier-variant/`SwarmSheet` mismatch shows up mechanically, not just in prose. |
| `total_px` | `[w, h]` | `cell * grid[0]`, `cell * grid[1]` — what the audit measures the PNG against |

### `wiring`

| Field | Type | Notes |
|---|---|---|
| `expected_consumer` | string | plain-language description of what should load this texture (a specific `LoadObject` call, a Niagara renderer, a composite source) |
| `check` | object | `{method: "grep_source", patterns: [...]}` for the generic case, or `{method: "composite_source", composite_request: "swarm-units", prefix: "retinue"}` for a texture that only ever exists baked into another texture (the brood/retinue sources of `T_Swarm_2bit`) |

## Audit categories (for reference — the schema for `coverage.py`'s *output*, not this file)

`missing` · `unwired` · `off-ramp` · `unrecorded` · `incomplete` — defined and checked
by `Scripts/art/coverage.py`; see `.claude/skills/art-coverage/SKILL.md` for exactly
how each is decided and what would fool it.
