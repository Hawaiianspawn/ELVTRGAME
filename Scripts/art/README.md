# Art pipeline scripts

The local half of the PixelLab sprite pipeline. The orchestration procedure lives in
`.claude/skills/sprite/SKILL.md`; these are the tools it drives.

| Script | Runs where | Does |
|---|---|---|
| `pixelpipe.py` | normal Python (`py`) | validate, compose prompt, fetch, quantize, pack, report, record provenance |
| `import_sprites.py` | **Unreal editor Python only** | import a packed sheet with ELVTR's pixel-art texture settings |

## Why the split

PixelLab's docs are explicit that theirs are MCP tools, not REST endpoints — so
**no script here ever calls the PixelLab API.** Generation happens through
`mcp__pixellab__*`, which only Claude can invoke. `pixelpipe.py` does local, deterministic
work plus downloads from the public (auth-free) result URLs, which keeps it testable
offline and keeps the skill thin.

`import_sprites.py` is the **fallback** import route, for when the editor's MCP server is
down or a whole batch needs importing. The preferred route is straight through MCP:
`TextureTools.import_file` + `ObjectTools.set_properties`, verified working 2026-07-25 and
documented with its call-shape traps in `.claude/skills/sprite/SKILL.md` stage H. (The
`cvars` skill's "MCP cannot create assets" note is narrower than it reads: it holds for
creating an asset *from a class*, but a dedicated texture import tool does exist.)

The script still earns its place because the MCP Python sandbox cannot `import unreal`
(only `re/json/copy/math/datetime/time`), so anything needing the engine API in bulk goes
through editor Python — same pattern as `Scripts/populate_cvar_preset.py`.

## pixelpipe.py

```powershell
py Scripts\art\pixelpipe.py validate hero-vanguard
py Scripts\art\pixelpipe.py authored hero-vanguard          # anchor from the spec, 0 gens
py Scripts\art\pixelpipe.py prompt   hero-vanguard [--stage anchor|rotation]
py Scripts\art\pixelpipe.py fetch    hero-vanguard --stage anchor --url URL
py Scripts\art\pixelpipe.py quantize hero-vanguard --stage anchor
py Scripts\art\pixelpipe.py pack     hero-vanguard
py Scripts\art\pixelpipe.py report   hero-vanguard
py Scripts\art\pixelpipe.py manifest hero-vanguard --stage anchor --set character_id=UUID --set generations=2
```

Standalone quantize, for trying the enforcement pass on any folder of PNGs without a
request (this is how it was validated before any generations were spent):

```powershell
py Scripts\art\pixelpipe.py quantize --in RawArt\Renders\hero-rev1-grayscale\vanguard --out out\ --moving
```

Requires Pillow, numpy, and (for `validate`) jsonschema.

### The authored anchor

`authored` scans the request's linked prose spec for fenced code blocks written purely in
the palette's `ascii_legend` characters (`.` transparent, `#` dark, `S` steel, `b` bone,
`@` pale) and renders one straight to a PNG.

This is usually the right way to get an anchor. An art spec's silhouette guide is already
a complete, on-palette, correctly proportioned drawing — rendering it costs **zero
generations** and cannot come back missing the subject's defining prop. On `hero-vanguard`
it produced a 100% on-palette frame at Steel 52% (spec demanded ≥50%) after two *generated*
anchors had failed: the `v3` one discarded chibi and flat shading and came back 60% Dark,
the `standard` one dropped the banner entirely because template-mode skeleton generation
won't accommodate a pole above the head.

The tradeoff is that a hand-drawn ASCII map looks schematic. The v3 reference pass is what
renders it up while preserving the palette and silhouette.

### What quantize actually does

1. **Binarize alpha** at 128. The material is Unlit + Masked; partial alpha is the most
   common silent breakage and it survives all the way to a soft-edged sprite in game.
2. **Map to the ramp** by nearest luma (Rec.709, gamma space). The four values are
   near-neutral, so luma separates them evenly where RGB distance would not. If the input
   is muddy — under 95% on-palette *and* a luma span under 0.35 — it is contrast-stretched
   first, and the report says `NORMALIZED` so this is never silent.
3. **Enforce 2×2 dither.** Detects genuine 1px checkerboard fields and snaps them to 2×2
   blocks by majority vote with a parity tiebreak, iterating to a fixed point. A perfect
   1px checker becomes a perfect 2×2 checker at the same average tone.

Step 3 only touches blocks that intersect a detected checker. **1px detail that is not
stipple survives** — eye dots, rune marks, thin contours, diagonal and vertical 1px lines
were all verified intact against a synthetic test. The detector requires all four
orthogonal neighbours to be opaque, which is what keeps silhouette-edge pixels from
registering as dither.

Raw downloads are never modified: quantize writes a sibling `quantized/` tree.

### Layout it owns

```
docs/data/art/requests/<id>.json        the request (hand-authored, committed)
RawArt/Renders/<id>/r<rev>/raw/         downloads, NEVER modified (retention rule)
RawArt/Renders/<id>/r<rev>/quantized/   the enforced 4-value output
RawArt/Renders/<id>/r<rev>/manifest.json  provenance: UUIDs, urls, generations spent
RawArt/Renders/<id>/r<rev>/report.json    QC results
RawArt/Sheets/<texture>.png             the packed SubUV sheet
```

The manifest matters more than it looks: `create_character` takes **no seed**, so it is
the only record that makes a sprite reproducible. The four revision folders that predate
this pipeline have no manifest and cannot be iterated on — only regenerated from scratch.

## import_sprites.py

```
py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" hero-vanguard
```

Typed into the editor console (`~`), or via Tools > Execute Python Script (with no
argument it imports every request that has a packed sheet).

Applies the canonical settings from `ELVTR/SETUP-EDITOR.md` — Filter = Nearest,
MipGenSettings = NoMipmaps, Compression = UserInterface2D, sRGB per the request — then
**reads them back** and logs `SPRITE import ... [OK]` or `[VERIFY FAILED]`. Destination
and texture name come from the manifest, so there are no hardcoded paths.
