# Art pipeline scripts

The local half of the PixelLab sprite pipeline. The orchestration procedure lives in
`.claude/skills/sprite/SKILL.md`; these are the tools it drives.

| Script | Runs where | Does |
|---|---|---|
| `pixelpipe.py` | normal Python (`py`) | validate, compose prompt, fetch, quantize, pack, report, record provenance |
| `import_sprites.py` | **Unreal editor Python only** | import a packed sheet with ELVTR's pixel-art texture settings |
| `roster.py` | normal Python (`py`) | the model: every unit the game has or wants, joined from disk + manifests + atlases, staged |
| `forge.py` | normal Python (`py`), serves a local page | the roster page, plus per-family generate → measure → approve → ship |

## Why the split

PixelLab's generators are MCP tools, so **the agent-driven scripts here never call the
PixelLab API.** Generation happens through `mcp__pixellab__*`, which only Claude can
invoke. `pixelpipe.py` and `variantpipe.py` do local, deterministic work plus downloads
from the public (auth-free) result URLs, which keeps them testable offline and keeps the
skills thin.

`forge.py` is the deliberate exception, and only because it is the one tool with no agent
in it: the owner drives it alone from a browser, so it talks to the same service's v2 REST
API. It is additive — anything forge generates and anything Claude generates land in the
same folder in the same layout, and both show up on the same page.

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

## roster.py

```powershell
py Scripts\art\roster.py                # the whole roster, grouped and staged
py Scripts\art\roster.py --seed         # first-run group assignment
py Scripts\art\roster.py --json
```

The project knew a lot about each unit and none of it in one place. Whether a look has art
is a directory listing; whether it measured well is a family manifest; whether it ships is a
source entry in a composite request; whether it is *actually on screen* is a constant in
`SwarmFragments.h` and a field in a `.uasset`. This joins them.

**Stage is computed, never stored**, so it cannot go stale:

```
concept -> generated -> measured -> APPROVED -> animated -> packed -> LIVE
```

Stage is the *highest rung satisfied*, not the end of an unbroken chain — a look that
shipped before this tooling existed reads `packed` even though nobody ever clicked approve,
and `gate_passed` reports the missing click separately rather than understating the unit.
`live` is its own rung because `ship` cannot reach it: a unit between `packed` and `live` is
invisible in game while looking finished everywhere else.

`docs/data/art/roster.json` stores only what no file in the repo can answer — the group, the
expectation, the notes, and units that are wanted but have no art. The owner's approve/deny
stays in the family manifest where forge writes it; duplicating it here would create exactly
the drift this file exists to prevent.

It also reports the **4-bit ceiling**: each block stops at 16 looks (spearmen 11, archers 13,
brood 9 today), and atlas-vs-C++ drift, which is the thing that decides `live`.

Two bugs it found on its first run, both invisible before the join:
- The nine brood folders are sources of **both** `enemy-units` and the retired `swarm-units`.
  Resolving to the retired sheet made every one read as team, packed-but-not-live. A live
  atlas now always beats a retired one.
- The retinue base look sits at `unit-retinue-colour/raw/rotations/` — flat, in a folder with
  no `family.json` — so it was the one source of `team-units` with no row anywhere.
  Anything a live atlas sources is now discovered whatever its layout.

## forge.py

```powershell
py Scripts\art\forge.py                                 # roster on :8770, opens a browser
py Scripts\art\forge.py --family pathfinder-line        # same, family view preconfigured
py Scripts\art\forge.py --port 8771 --no-open
py Scripts\art\forge.py --selftest
```

Two views. `/` is the roster: every unit grouped, its stage, its walk/attack/death coverage,
an editable group and expectation, and a running note thread. Opening a row plays an
**animated WebP turnaround** — the unit rotating on the spot, because a break in the cycle
announces itself where eight stills have to be compared. Turnarounds are fetched only for
rows you open; 135 looping WebPs decoding at once is not something to ask of a page.

`/family/<name>` is the working view: a prompt panel on the left, the family's front-facing
contact sheet on the right, approve/refine/deny under every card, and one **ship approved**
button. It owns none of the pipeline — measurement is `silhouette_report.py`'s, judging and the family contract are
`variantpipe.py`'s, atlas rows are `atlas.py`'s, packing is `pixelpipe.py`'s, importing is
`import_sprites.py`'s. What is new is an HTTP surface, a REST client, and one field.

**The slug is required, and that is the feature.** `create_character_state` takes a
`state_name`; nothing was sending it, so PixelLab fell back to the prompt and the account
accumulated 158 characters with names like `Can you make a Turre` and five separate
`Reshape this creatur`. Forge demands kebab-case and always sends it.

**The owner's verdict finally has a home.** `variantpipe judge` writes
`variants.<slug>.verdict` — that is a *measurement* verdict and it is rewritten on every
run. Forge writes a sibling `variants.<slug>.owner` block instead:

```json
"owner": { "verdict": "approve", "at": "…", "note": "", "shipped": null }
```

The two are allowed to disagree, which is the useful case: a variant judge calls
mechanically redundant can still be the one that reads best in a crowd. Deny records a
verdict and moves nothing — the retention rule stands.

**Refine is the third button.** It opens the variant's eight rotations inline — the card
shows south, and most defects worth fixing are ones a south-only view cannot show — plus a
box for what needs cleaning up. Sending generates a new state **from that variant's own
character**, not from the family base, so the fix inherits everything already approved and
lands as `<slug>-r2`. The note is written to the parent's `refine_notes[]` either way, so a
described defect survives the child being rejected.

Its limit is measured and stated on the panel: **a state edit changes what the subject IS,
not how one facing looks.** "Taller hood", "drop the arrows" work. "Fix the spear at
north-east" does not — see the 2026-08-08 table in `docs/PIXELLAB-MCP.md`, where the three
named rotations came back byte-identical and the two that moved got worse. Per-facing
geometry needs the anchor-and-rotate path that `/sprite` owns.

**Ship is a batch, not an approve side-effect**, because an atlas row is not a pure data
change. It runs `atlas.py add` per approved variant, then `pixelpipe.py pack`, then the
import through `Scripts/ue-mcp-call.py`, then `atlas.py check --all` — and then prints the
two things it deliberately will not do silently: the `SwarmSheet::` constant in
`SwarmFragments.h` (a recompile) and the Niagara emitter's Sub UV. Batching means one
recompile per session instead of one per unit. If the editor is down the import fails
softly and the packed sheet is still correct.

Defaults come from an optional `forge` block in `docs/data/art/families/<f>/family.json`
(`atlas`, `prefix`, `tail`); `--atlas`/`--prefix` override it. The API key is read from
`PIXELLAB_API_KEY` or from the `pixellab` MCP server in `~/.claude.json`, never from the
repo, and the server binds `127.0.0.1` only.

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
