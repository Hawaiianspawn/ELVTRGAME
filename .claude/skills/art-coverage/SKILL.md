---
name: art-coverage
description: Audit ELVTR's art asset coverage against docs/data/art/asset-matrix.json — finds art that is required but absent (missing), sitting on disk but not drawn by any code (unwired), off a named palette ramp when checked (off-ramp — informational by default, opt-in with --enforce-palette; see below), missing a provenance record (unrecorded), or short of its required frames (incomplete). Read-only: never generates, imports, edits art, or spends PixelLab credits. Use when the user runs /art-coverage or asks what art a character needs, whether an asset is actually wired in, or for a gap report before planning new sprite work.
---

# art-coverage — what does a character need, and what's actually there?

Reports gaps. It is a layer **above** `.claude/skills/sprite/SKILL.md`, not a
replacement for it — this skill calls nothing in the sprite pipeline and never
touches `mcp__pixellab__*`. It reads `docs/data/art/asset-matrix.json` (what a
character needs, derived from requests/code/specs) and checks that against the real
repo (`RawArt/Sheets/`, `ELVTR/Content/`, `ELVTR/Source/`, provenance records).

## Why this exists

Three real problems this session made the case for it: six finished,
palette-validated soldier variants sat unused in `Content/` while the Unit Cam drew
a single generic row — nobody knew until someone grepped for it. Archers had no art
at all until a design doc named them. An archer proxy shipped with no animation
frames, found only while writing the import task. A coverage check catches all
three mechanically instead of by accident.

## Run it

```
py Scripts/art/coverage.py                       # full report, every matrix entry
py Scripts/art/coverage.py --id T_Soldier_01      # one entry
py Scripts/art/coverage.py --category unwired     # only entries with that finding
py Scripts/art/coverage.py --json                 # machine-readable
py Scripts/art/coverage.py --enforce-palette      # off-ramp becomes a finding again
```

No arguments needed beyond that — it needs no editor, no MCP connection, and makes
no network calls. Safe to run at any time, including mid-generation on another task.

**Palette enforcement is opt-in, not a default (since 2026-07-28).** The owner
reversed the strict global 4-value Demichrome lock — `docs/art/aesthetic-direction.md`'s
AMENDMENT 2026-07-28 — and the game now ships full colour by default
(`Emberkeep.Quantize=0`). Comparing every sheet against `demichrome-4` and calling the
difference a defect would report deliberate full-colour art (the knight/archer
rosters, the retinue character, the brood-ooze family) as broken, so a plain run no
longer counts off-ramp pixels as a finding — it still computes and prints them as an
`i off-ramp ... informational only` line (and always in `--json`), it just doesn't
gate on them or count them in the summary. Pass `--enforce-palette` to check
conformance for anything that has actually opted into a ramp: it uses each matrix
entry's own `palette.ref` when the entry sets one, falling back to `--palette`
(default `demichrome-4`) otherwise. This doesn't invent a new colour standard —
there isn't one written down yet — it only stops assuming demichrome-4 by default.

## The five categories, and exactly how each is decided

| Category | Decided by | What would fool it |
|---|---|---|
| **missing** | matrix entry is `required: true` and neither `RawArt/Sheets/<texture>.png` nor `ELVTR/Content/<content_path>/<texture>.uasset` exists | nothing — file existence is unambiguous. But see "content vs. disk" below: a texture generated to `RawArt/Sheets/` but never imported still reports missing, correctly, since the game can't load a `.uasset` that was never created. |
| **unwired** | the texture's Content asset exists, but a literal grep for its name across `ELVTR/Source/**/*.cpp` and `*.h` finds zero hits | **Blueprint graphs, Niagara systems/materials, and DataAssets are binary or editor-only — this check cannot see into them at all.** A texture referenced only from a Blueprint or a Niagara Sprite Renderer (not a C++ `LoadObject` string) is reported unwired even if it is genuinely drawn in-game. A texture loaded via a path built at runtime (string concatenation, a DataTable row, a resolved soft-object-ptr) is also invisible to a literal-string grep. Treat an "unwired" finding as "no C++ evidence of wiring", not "definitely unused" — cross-check against Blueprints/Niagara by hand before concluding an asset is truly dead. |
| **off-ramp** | opaque pixels in the `RawArt/Sheets/` PNG that are not an *exact* RGB match to one of the checked palette's hexes (per-entry `palette.ref` if the matrix entry sets one, else `--palette`, default `demichrome-4`), or alpha that is not strictly 0/255 — **informational only** unless `--enforce-palette` is passed; see "Palette enforcement is opt-in" above | This inspects the **source PNG**, not the imported `.uasset` — there is no read-only way to inspect an imported texture's raw pixels from outside the editor. If someone hand-imports a different PNG than the one in `RawArt/Sheets/`, this check is blind to the drift. It also only checks exact hex membership, not `pixelpipe.py`'s fuller `quantize` pass — it does **not** run the caged-light (`pale_uncaged`) audit, the dither-block-size check, or the `value_dominance`/`pale_usage` cross-check against canon. A sheet whose hexes are all valid but whose *dominant value contradicts its spec* (a real, disclosed case: `T_Soldier_Archer`'s ~69% Dark against the roster's "zero variants are Dark-dominant" rule) will NOT be flagged off-ramp by this tool — that is a canon-dominance violation, not a palette violation, and only `pixelpipe.py quantize`'s `flag_findings` catches it. Read `docs/data/art/provenance.json` and each request's own QC notes for that class of issue. And since 2026-07-28: a report with zero off-ramp *findings* proves nothing about colour conformance unless `--enforce-palette` was passed — check the `i off-ramp` lines (or `--json`'s `detail.off_ramp`) for the informational numbers either way. |
| **unrecorded** | one of two conventions, matching which one the repo actually uses for that entry: an entry with a `source_request` is checked against that request's own `RawArt/Renders/<id>/r<rev>/manifest.json` (the normal `/sprite` pipeline's provenance, keyed off the request's *own* `revision` field so this can't drift from what's actually current); an entry with **no** `source_request` (only `T_Soldier_Archer` today) is checked against `docs/data/art/provenance.json`, which exists specifically for assets that skipped the request pipeline (its own top-of-file note says so) | If `docs/data/art/provenance.json` is ever deleted, every no-request entry reports unrecorded — a real finding, not a bug. If a request's `revision` field is bumped without a corresponding `r<N>/manifest.json` existing (e.g. hand-edited), this reports unrecorded even though earlier revisions might have a manifest — deliberately, since the manifest for the *current* revision is what should exist. |
| **incomplete** | matrix's `required_frames` (exact keys) or `required_frame_count` (a floor) compared against the entry's actual `output.frame_map` (from its `source_request`, if one exists) or, for entries with no request, the count of distinct PNGs under `RawArt/Renders/<provenance_id>/` | For a no-request entry the frame count is a **floor estimate from files on disk**, not a validated frame_map — it can overcount (stray reference/debug PNGs in the renders folder) or undercount (frames that exist only inside a zip that was never extracted). Also: a `required_frame_count` that was itself invented by the matrix (see `T_Soldier_Archer`'s `open_questions` — 16 is asset-matrix.json's own analogy to the hero sheets, not a number anyone filed a request against) will produce a real, honest "incomplete" finding against a requirement nobody has actually signed off on. Read the matrix entry's `open_questions` before treating an incomplete finding as an action item. |

## Reading a report

Every entry prints its `open_questions` from the matrix alongside any findings.
**Read them before acting on a finding** — several entries (the two competing hero
textures, the six soldier variants vs. the legacy `T_Unit_Retinue` strip, the
archer's invented frame-count requirement) carry a live, unresolved design question
that changes what a "gap" actually means. A clean audit run does not mean every
open question is settled; it means the matrix's *stated* requirements are met.

The summary block's `untested categories` line matters: if a category has zero
findings across every entry, say plainly that it's untested rather than implying
the whole art set is clean on that axis. As of this skill's first run (2026-07-27,
right after `task-050` landed), `off-ramp` and `unrecorded` were both genuinely
zero-finding — every current sheet really did pass exact-hex palette membership
and really did have a provenance record under one of the two conventions above.
That was a real, checked state at the time, not an artifact of a broken check —
see this skill's own `coverage.py` module docstring for the synthetic-pixel test
that proves the off-ramp detector isn't trivially passing.

**That off-ramp reading is now stale in a different way, and it's worth being
precise about which.** As of the 2026-07-28 colour reversal, `off-ramp` reads
zero *findings* on a plain run regardless of what's on disk, because the check is
informational by default now (see above) — not because every sheet is on-ramp.
Run with `--enforce-palette` to get the pre-2026-07-28 meaning back: measured the
same day this changed, that turns up 2 (`T_Swarm_2bit`, `T_Soldier_Archer` — both
now genuinely full-colour, which is expected and correct under the current
amendment, not a regression).

## Maintaining `docs/data/art/asset-matrix.json`

Add a character by deriving its requirement from the same three places every
existing entry cites in its `derived_from` field, in priority order:

1. **A schema-validated request's own `output` block** — the strongest source,
   since `sprite-request.schema.json` already validates it.
2. **The code's own sheet contract** — `SwarmFragments.h`'s `SwarmSheet` and
   whatever `UnitCamProjector.cpp` (or its successor) actually `LoadObject`s and how
   it slices cells.
3. **A prose spec's acceptance checklist** (`docs/art/*.md`) — for palette/canon
   fields a request doesn't restate, or where the prose and the request disagree
   (record both, don't silently pick one — see the `soldier-*` entries' stale
   sheet-dimension header for the pattern).

**Never invent a requirement to make the audit report a gap.** If deriving a field
would require a design decision nobody has made, add it to the entry's
`open_questions` (or `global_open_questions` if it spans entries) instead of
picking an answer — see `docs/data/art/asset-matrix.schema.md` for the full field
reference, and every existing entry for the pattern.

This skill and `Scripts/art/coverage.py` never write to the matrix, the requests
directory, `RawArt/`, `ELVTR/Content/`, or `docs/data/art/provenance.json` — editing
the matrix is a manual, deliberate act by whoever is planning the next character's
art, same as editing any other `docs/data/` file.
