# Character art pipeline — one page

How a character gets from an idea to pixels moving in game. Two routes; pick by
whether the character is **new** or a **sibling** of one that already looks right.

```
 idea ──► spec ──► ANCHOR ──► [human gate] ──► rotate 8-way ──► animate
                                                  │
        (new character = /sprite)                 ▼
        (sibling of an approved base = /variants) atlas row ──► pack sheet ──► UE import ──► Sub UV
```

## Route A — new character (`/sprite <id>`)

| Stage | Command / tool | Cost |
|---|---|---|
| A spec | `pixel-art-director` writes prose spec + `docs/data/art/requests/<id>.json` | — |
| B validate | `py Scripts/art/pixelpipe.py validate <id>` then `prompt <id>` | — |
| C anchor | `pixelpipe.py authored <id>` (renders spec's ASCII map) **or** `create_character` mode `standard` | 0 / 1 |
| D enforce + **gate** | `pixelpipe.py quantize <id> --stage anchor`, show owner the south frame, get an explicit yes | — |
| E rotate | `create_character` mode `v3` + `reference_image_url` = the **quantized** anchor | 1 |
| F animate | `animate_character` template mode | 1 / direction |
| G pack | `quantize --stage rotation/anim`, `pack`, `report` | — |
| H import | MCP `TextureTools.import_file` + `ObjectTools.set_properties`, read back | — |

Mode rules that decide everything: `standard` is the only mode honouring `shading`,
`proportions`, `n_directions`; `v3` ignores them but is a near-perfect **rotator** of a
reference image. So: author/generate one frame, quantize it, let v3 propagate the look.
The anchor's quality is the ceiling for the whole sheet — v3 preserves, never embellishes.

Stage D is the only human gate in the chain. Everything downstream inherits that frame.

## Route B — siblings / a family (`/variants <family>`)

For "six knights that read apart", not "one knight". Family written as
`docs/data/art/families/<family>/family.json` (base character, the one axis being varied,
what's held constant, per-variant edit descriptions), then:

```
py Scripts/art/variantpipe.py plan   <family>    # prints the mcp__pixellab__* calls to run
py Scripts/art/variantpipe.py fetch  <family> <variant> --url URL ...
py Scripts/art/variantpipe.py judge  <family> [--cull]
py Scripts/art/variantpipe.py report <family>    # contact sheet for the owner verdict
```

Generation is `create_character_state(use_color_palette_from_reference=True)` on the
approved base — same individual, same palette, same canvas.

Axis order, spend in this order: **aspect** (width/height) → **topology** (holes, notches,
asymmetry) → **interior/kit** last, only as a tiebreaker. Simplify the *outline*, never the
interior. Floor: it must still keep a head and legs — a variant that stops reading as the
unit type is rejected however good its numbers are.

Variety is judged by measurement (`silhouette_report.py`: aspect, solidity, asymmetry,
hole count), never by eye on the south frame.

## Landing it in game (`/atlas`)

Units share composite atlases (`team-units`, `enemy-units`) because the Niagara emitter has
one sprite renderer and one material.

```
py Scripts/art/atlas.py add   team-units --prefix <slug> --rotations RawArt/Renders/<slug>/raw/state00/rotations
py Scripts/art/pixelpipe.py pack team-units        # writes RawArt/Sheets/T_*.png
Exec('py ".../Scripts/art/import_sprites.py" team-units')
# widen the emitter Sub UV in NS_Swarm, then READ IT BACK
py Scripts/art/atlas.py check --all
```

Four things must agree or the horde decodes garbage: the request's `frames` block, its
`frame_map`, `output.grid` = `[8, variants*2]`, `SwarmSheet::Team`/`::Enemy` in
`SwarmFragments.h`, and the emitter's Sub UV. `check` verifies the first three, compares the
fourth, prints the fifth. Never hand-edit `frames`/`frame_map` — `sync` derives both.
Append variants; `--at N` renumbers indices that weights and stat rows are keyed on.

## Files

```
docs/art/<name>.md                       prose spec (silhouette guide = the anchor source)
docs/data/art/requests/<id>.json         subject or composite request (committed)
docs/data/art/families/<f>/family.json   variant family definition
RawArt/Renders/<id>/r<rev>/raw/          downloads — NEVER modified or deleted
RawArt/Renders/<id>/r<rev>/quantized/    palette-enforced output
RawArt/Renders/<id>/r<rev>/manifest.json UUIDs, urls, generations — the only reproducibility
RawArt/Sheets/T_*.png                    packed SubUV sheet (48px cells)
ELVTR/Content/Sprites/Units/*.uasset     imported texture
```

## Rules that cost credits when forgotten

- **No tool forces a palette.** Hexes in a prompt are guidance. The quantize pass is
  mandatory, not polish. Requests name their palette explicitly (`canon.palette`); the
  game ships full colour by default, `demichrome-4` is opt-in per request.
- **`create_character` takes no seed.** The manifest is the only record that makes a sprite
  reproducible — write the UUID before polling.
- **Never delete a generation.** `raw/` is append-only including rejects; choosing a result
  copies it onward.
- **Never hand-type a description or an ASCII grid.** `pixelpipe.py prompt` composes the
  prompt deterministically (and drops params the mode ignores); a drawer script emits grids.
- **Composed prompt caps at 2000 chars**, not the 600-char `description` — `validate` checks it.
- **Never resample.** `pack` refuses to scale; if the sprite overflows the 48px cell, lower
  `pixellab.size` and regenerate.
- **A clean QC is not a clean sprite.** The quantizer sees pixels, not canon — add a check
  when a rule has a pixel carrier (e.g. the caged-light audit).
- **Read back every editor write.** `set_properties` and `SetRendererData` both return
  success when they changed nothing.

Costs: `standard` 1, `v3` 2 (≤48px), template animation 1/direction, portrait 20–25, `pro`
20–40 — a 48px hero with an 8-direction walk lands ~12–16 including one re-roll. Check
`get_balance` before spending.

Deeper detail: `.claude/skills/sprite/SKILL.md` (constraints, per-stage traps),
`.claude/skills/variants/SKILL.md` (family measurements), `.claude/skills/atlas/SKILL.md`,
`Scripts/art/README.md` (what quantize actually does).
