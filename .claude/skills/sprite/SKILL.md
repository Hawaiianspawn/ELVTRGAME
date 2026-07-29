---
name: sprite
description: Generate ELVTR sprites through PixelLab and land them in Unreal on whichever palette the request names. Drives the chain from a schema-validated request in docs/data/art/requests/ through anchor generation, palette enforcement, rotation, animation, SubUV sheet packing and UE import, recording every generation in a provenance manifest. Use when the user runs /sprite or asks to generate, regenerate, quantize, pack or import a character sprite, sprite sheet or portrait.
---

# sprite — spec to imported texture

Turns one JSON request into an imported UE sprite sheet that provably holds
whichever palette its `canon.palette` names. Generation calls are made here via
`mcp__pixellab__*`; every local step is `Scripts/art/pixelpipe.py`.

**2026-07-28: the strict global 4-value Demichrome lock is superseded** (see
`docs/art/aesthetic-direction.md`'s AMENDMENT and `docs/data/art/palette.json`'s
`palettes.demichrome-4.superseded`). The game ships full colour by default now.
`demichrome-4` is kept in full and remains available — every request below still
works exactly as documented, because every request in the repo already names its
palette explicitly (`canon.palette` is schema-required) — but `pixelpipe.py` no
longer *assumes* demichrome-4 for anything that omits it: a missing/malformed
`canon.palette` now fails loudly instead of silently quantizing to a ramp nobody
asked for. This doesn't tell you what a full-colour request should look like —
that's an open owner decision, not something this skill or `pixelpipe.py` decides —
it only stops the tooling from forcing the retired ramp by default.

## Hard constraints (learned 2026-07-25 — do not re-discover)

Verified against PixelLab's docs and their own docs agent. These shape the whole design:

- **No tool forces a palette.** Hexes in a description are soft guidance and nothing
  more. Off-ramp output is guaranteed. `agent_help` confirms: *"No tool explicitly forces
  a strict 4-color limit."* The quantize pass is mandatory, not polish.
- **`create_character` accepts no seed.** There is no reproducibility from PixelLab's
  side. Ours comes from two things: a **deterministically composed prompt** (never
  hand-type a description) and the **manifest** that records UUIDs and costs. Losing the
  manifest means losing the ability to iterate on a sprite.
- **`create_character(mode="v3", reference_image_base64=...)` is the strongest style
  lever that exists.** It rotates *your exact sprite* into 8 directions at high fidelity,
  and animations then inherit that style. This is why the pipeline is anchor-first:
  quantize one frame, feed it back, and the ramp propagates for free. Measured on
  hero-vanguard: generated anchors came back **0% on-palette**; rotations from a quantized
  reference came back **99.2–100%**, with the class's defining prop intact in all eight
  directions. It also costs only **1 generation**, not 2.

- **But v3 reference is a faithful *rotator*, not a *renderer*. The anchor's artistic
  quality is the hard ceiling for the whole sheet.** It reproduced an ASCII-schematic
  anchor almost pixel-for-pixel rather than rendering it up — it preserves, it does not
  embellish. Consequences worth planning around:
  - The authored-ASCII route buys perfect palette compliance and perfect prop retention,
    but yields a **greybox blockout**, not shippable art. Good for proving a sheet, wiring
    up the material, and blocking in a class; not for shipping.
  - Shippable art needs a **real hand-drawn 48px anchor** (a pixel artist's frame, or a
    generated one cleaned by hand) fed in through the same `authored`/reference path.
  - Rear-facing rotations degrade most — with the reference's front detail unavailable,
    tall props flatten into slabs. Check north/north-east/north-west first when judging a
    rotation pass, not south.
- **`create_character_state(use_color_palette_from_reference=True)`** is the only
  palette-snapping mechanism on offer. Use it for variants (gear, damage, states) of an
  already-approved character — it preserves the *same individual*. For a different
  character in the same style, use `create_character` with matching params + a v3
  reference image instead.
- **Use `standard` for the anchor, `v3` only for the rotation. This is not a preference —
  it is the whole reason the anchor stage exists.** v3 **silently ignores `shading`,
  `proportions`, `text_guidance_scale` and `n_directions`**; `pro` ignores nearly all
  style params. So a v3 text-to-sprite anchor discards "chibi" and "flat shading"
  entirely and returns a realistically-proportioned, heavily-shaded figure that no amount
  of quantizing will rescue — **measured on hero-vanguard r1, 2026-07-25:** 60% Dark
  against a spec demanding Steel ≥50%, Pale spilled onto the shield, and a swallowtail
  flag despite an explicit negative prompt.

  `standard` is the *only* mode that honours those params, and it costs 1 generation
  instead of 2. v3's real strength is the reference-image path, where style comes from
  the pixels you hand it rather than from params it would have ignored anyway.

  Corollary: prefer `outline: "selective outline"` over `"single color black outline"`.
  Full black outlining is the single biggest driver of Dark dominance, and
  `aesthetic-direction.md` specifies selective outlining anyway ("no outline against dark
  floors; Dark outline only where sprite meets sprite").

- **Rendered depth does not survive the 4-value ramp — measured, don't retest casually.**
  A 6-cell controlled batch (`docs/art/soldier-style-depth-test.md`, 2026-07-25) held one
  composed description byte-identical and varied only shading/detail/outline. Every cell
  above `flat shading` spent its extra pixels on speckle, not form, and pushed mass into
  **Dark** rather than producing mid-tones (`medium shading` hit 60.7% Dark; the flat
  control was the most balanced at 49.6/24.7/25.7 and had the cleanest silhouette).
  `single color black outline` was second-worst at 55.2% Dark, re-confirming the outline
  corollary below. `lineless` was the only treatment to push mass into Steel (35.9%) — at
  the cost of collapsing Bone to 11.7%, so the skin/face channel nearly vanished.
  Direction A's flat rule is confirmed by measurement.

  The lever for re-asking this cheaply is `prompt.style_mode` (`flat` default / `depth`).
  The composer otherwise **hardcodes** the flat clause and the
  gradients/anti-aliasing/soft-shading negatives into every prompt, so a `detailed shading`
  param without it measures a prompt-vs-param contradiction and nothing else. `depth` is
  experiment-only — never on a shipping asset without an owner exception.

- **Text prompting cannot deliver an irregular silhouette.** In that same batch, none of
  the militia's `must_include` carriers — uneven shoulders, half skullcap, split
  cloth/plate torso, mismatched legs, off-vertical club — landed in *any* of the six
  cells. PixelLab returned six broadly uniform armoured soldiers with matching helmets,
  near the opposite of a "motley, never uniform" brief, with the club surviving only as
  detached speckle. Deliberately asymmetric or mismatched subjects must go through the
  **authored** anchor route; the generated route regresses them to a generic soldier.
  Related: at 1x–2x the helmet dome is the only shape that reads, and it quantizes to
  *Bone* — so it pulls the eye off the face and silhouette.

- **PixelLab caps the description at 2000 chars, and it is the *composed* prompt that
  counts.** `prompt.description` is schema-capped at 600, but the composer then appends
  `must_include`, `reads_as`, the ramp clause and every `must_avoid` entry — so a request
  can pass `validate` and still be refused by the API. Measured 2026-07-25: soldier-01 and
  soldier-06 validated at 582/585-char descriptions and were rejected at **2413 and 2458
  composed**. Nothing is spent on a rejection, but it costs a round trip.
  `validate` now checks composed length for both stages (hard error >2000, warning >1900).
  When trimming, cut `must_avoid`/`must_include` before the description, and cut the
  description's duplication of `must_include` first — the two overlap heavily and only
  `must_include` is echoed onto the stage-D review checklist.

- **The quantizer cannot see canon rules, only pixels — a clean QC is not a clean sprite.**
  Palette/alpha/dither checks passed soldier-05's rotation at 100% on-palette with zero
  findings while **east and west each carried a 6px uncaged bright bar**: pale pixels
  touching transparency, i.e. *free* light, which is the Lampbearer's `point_halo` carrier
  that the owner's 2026-07-25 caged-light ruling exists to keep unforgeable. A caged-light
  audit now runs inside `quantize` (`stats["pale_uncaged"]`) and flags any pale pixel with
  a transparent orthogonal neighbour. The general lesson stands beyond this one rule:
  **when a carrier is defined by a canon rule, add the check — the palette pass will
  happily certify a frame that breaks it.**

- `pixelpipe.py prompt` drops whichever params the chosen mode ignores — pass its output
  through unchanged rather than re-deriving. If a param you set does not appear in its
  output, the mode is discarding it, and that is worth noticing before you spend.
- **Downloads need no auth** — the UUID in the URL is the access key.
- **MCP CAN import a texture — use it.** `TextureTools.import_file` imports a PNG from
  disk directly, and `ObjectTools.set_properties` applies the pixel-art settings. Verified
  end-to-end 2026-07-25. This is narrower than the `cvars` skill's finding that MCP cannot
  *create an asset from a class*: a dedicated import tool exists for textures, so the
  editor-Python round trip is the fallback here, not the only route.

  Three call-shape traps in these tools, all of which fail loudly with the schema echoed
  back — read the error, it tells you the right parameter name:
  - `ObjectTools.set_properties` takes **`values` as a JSON *string***, not a dict.
  - `ObjectTools.get_properties` takes **`properties`**, not `names`.
  - `AssetTools.save_assets` takes **`asset_paths`** — plain path strings, not `refPath`
    objects, and no `.Object` suffix.

  `set_properties` returns `true` even when it silently changes nothing, so **always read
  the values back** and compare before believing an import took.
- **Stage H can run fully headless — no MCP, no editor session, no owner typing.** The
  PythonScript commandlet works and is the most reliable route found so far:

  ```
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
      "C:\Projects\ELVTRGAME\ELVTR\ELVTR.uproject" -run=pythonscript `
      -script="C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py <id> [<id> ...]" `
      -unattended -nopause -nosplash -stdout
  ```

  **Close the editor first** (asset write contention), and **read the verdict out of
  `ELVTR/Saved/Logs/ELVTR.log`, not stdout** — the commandlet's stdout dropped every
  `SPRITE import` line on one run while the log file had them all. Grep for
  `SPRITE import` and require `[OK]` per texture plus `imported N, skipped 0`.

- **Niagara Sub UV IS fully scriptable — via the Niagara MCP toolset, not editor Python.**
  Corrected twice: 2026-07-25 (wrongly "entirely unreachable"), then **2026-07-26**
  (wrongly "scriptable to change, manual to commit"). Both earlier attempts failed because
  they went through editor Python. `NiagaraToolsets.NiagaraToolset_System` does the whole
  job from a live editor session, and it is verified end-to-end:

  ```powershell
  . "C:\Projects\ELVTRGAME\Scripts\ue-mcp.ps1"; $null = Test-Mcp
  # rendererIndex 0 = the sprite renderer. All six ref fields are REQUIRED even when empty.
  $ref = @{ system=@{refPath="/Game/Spike1/NS_Swarm.NS_Swarm"}; emitterName="Swarm";
            scriptName=""; moduleName=""; rendererIndex=0; inputNameStack=@() }

  # 1. read (note: GetRendererData's parameter is `rendererRef`, Set's is `renderer`)
  Invoke-McpTool -Toolset "NiagaraToolsets.NiagaraToolset_System" -ToolName "GetRendererData" `
    -Arguments @{ rendererRef=$ref }
  # 2. write — a PARTIAL propertyValues JSON string is fine, it merges
  Invoke-McpTool -Toolset "NiagaraToolsets.NiagaraToolset_System" -ToolName "SetRendererData" `
    -Arguments @{ renderer=$ref; rendererData=@{ propertyValues='{"SubImageSize":{"x":8,"y":4}}' } }
  # 3. save — see the trap below
  Invoke-McpTool -Toolset "editor_toolset.toolsets.asset.AssetTools" -ToolName "save_assets" `
    -Arguments @{ asset_paths=@() }
  ```

  **The save trap:** `save_assets` with the explicit path returns *"Asset does not exist:
  /Game/Spike1/NS_Swarm"* — for both the package path and the `.NS_Swarm` object path —
  even though `find_assets` returns that exact string. `exists` also reports false for it.
  The path resolution and the asset registry disagree. What works is the documented
  **empty list = "save all dirty assets"** form, which wrote the .uasset and returned true.

  Because that form is a blunt instrument, **snapshot `LastWriteTime` across
  `ELVTR/Content` before and after and diff it**, so you can state which assets it
  actually touched rather than hoping it was only yours. On the 2026-07-26 run the diff
  was exactly one file, `Spike1\NS_Swarm.uasset`.

  `SetRendererData` returns `{"returnValue":null}` on success — that is not a failure, but
  it is also not confirmation, so **always read back with `GetRendererData`** before
  believing it. A stale Sub UV is silent and just draws the wrong frame on every unit.

  (For the record, the editor-Python route and why it fails: `unreal.NiagaraSystem` exposes
  no emitter members at all and `renderer_properties` is deprecated, though both objects do
  resolve by name — `unreal.find_object(sys, "Swarm_0")`, then
  `unreal.find_object(em, "NiagaraSpriteRendererProperties_0")` — and `set_editor_property`
  works. It is only the *save* that dies: every save API returns `False` in a
  `-run=pythonscript` commandlet, and blocks forever on a modal under `-ExecCmds="py"`.
  Don't spend time there again — use the MCP toolset.)

- **Everything is async.** Creation returns a UUID immediately; poll `get_character`.
  Characters take 2–4 min, animations 30–60 s per direction. Submit the whole batch, then
  poll — do not serialize.

## Retention rule — never delete a generation

Every result is downloaded to `RawArt/Renders/<id>/r<rev>/raw/` and **`raw/` is never
modified or deleted**, rejects included. The quantizer only ever writes a sibling
`quantized/` directory. Anything still in `RawArt/Renders/` is undecided; when a result
is chosen it gets *copied* onward, not moved out of raw. Candidate frames are cheap to
generate and impossible to reproduce exactly — there is no seed.

## The chain

Invoked as `/sprite <request-id>`, e.g. `/sprite hero-vanguard`.

| # | Stage | Who | Cost |
|---|---|---|---|
| A | prose spec + request JSON | `pixel-art-director` | — |
| B | validate + compose prompt | script | — |
| C | anchor: render the spec's pixel map, or generate | script **(0)** / MCP (1 standard) |
| D | quantize the anchor → **human gate** | script + owner | — |
| E | rotate from the approved anchor | MCP | 1 |
| F | animate | MCP | 1/direction |
| G | quantize, pack, report | script | — |
| H | import into Unreal | editor Python | — |

### A — the spec
If `docs/data/art/requests/<id>.json` does not exist, stop and hand off to the
`pixel-art-director` agent; it owns both the prose spec and the request. Do not author a
request yourself — the `canon` block is only meaningful if it restates a real spec.

### B — validate and compose
```
py Scripts/art/pixelpipe.py validate <id>
py Scripts/art/pixelpipe.py prompt   <id>
```
`validate` must pass before anything is spent. It catches retired hexes (in the request
*and* in the spec it links to), non-power-of-two grids, a cell size other than 48, and an
estimate that exceeds the request's budget.

`prompt` prints the exact tool name and kwargs. **Pass them through verbatim.** Editing
the description here breaks the only reproducibility the pipeline has; if the prompt is
wrong, fix the request and bump `revision`.

Check `mcp__pixellab__get_balance` before spending, and never exceed
`budget.max_generations`.

### C — the anchor

**Prefer the authored route when the spec has a silhouette guide:**
```
py Scripts/art/pixelpipe.py authored <id>
```
This renders the prose spec's own ASCII pixel map straight to a PNG — **zero
generations**, on-palette by construction, correct proportions, and it cannot come back
missing the subject's defining prop. An art spec's silhouette guide is not a sketch of
the sprite; it *is* the sprite. Set `anchor.strategy: "authored"`.

Measured on hero-vanguard, 2026-07-25: the authored anchor came out 100% on-palette with
Steel at 52% against a spec demanding ≥50% — the quantizer was a no-op — after two
generated anchors had failed. Its cost was nothing. The tradeoff is that it looks
schematic; the v3 rotation pass is what renders it up.

**Otherwise**, call `mcp__pixellab__create_character` with the kwargs from stage B
(`standard` mode — see the constraint above). Record it
immediately, before polling — a lost UUID is a lost generation:
```
py Scripts/art/pixelpipe.py manifest <id> --stage anchor --set character_id=<uuid> --set generations=2
```
Poll `get_character(character_id)` until completed, then fetch:
```
py Scripts/art/pixelpipe.py fetch <id> --stage anchor --url <rotation or download url>
```

### D — enforce, then STOP
```
py Scripts/art/pixelpipe.py quantize <id> --stage anchor
```
Read the findings. Re-roll rather than proceeding if you see:
- **under 3 values used** — the generation came back too low-contrast; it will read flat.
- **a dominant value contradicting `canon.value_dominance`** — the disjointness audit is
  broken and this subject will not separate from its neighbours at gameplay zoom.
- **pale pixels where `canon.pale_usage` is `none`** — the bright value is the scarcest
  thing in the game; a subject that isn't entitled to it must not spend it.

Then **show the quantized south frame to the owner and get an explicit yes.** Everything
downstream inherits this frame — an unapproved anchor wastes the entire batch, not one
call. On approval set `anchor.approved: true` in the request.

This is the only human gate in the chain. Do not skip it and do not infer approval.

### E — rotate from the anchor
```
py Scripts/art/pixelpipe.py prompt <id> --stage rotation
```
**Prefer `reference_image_url` over `reference_image_base64` for this call.** PixelLab's
`create_character` docs recommend the URL form and warn that inline base64 is often cut
off mid-string by MCP clients, which silently corrupts the reference image — a failure
mode that would not show up as an error, just as a rotation pass that quietly drifted
from the approved anchor. The docs put the crossover around ~32×32; our anchors are 48px
minimum and the concept pass runs up to 256×256, so this call sits well inside the range
where the corruption risk is real, not a footnote.

Use the **quantized** anchor (`quantized/anchor/<source_direction>.png`), not the raw
one, regardless of which transport is used — feeding back the raw frame throws away the
whole point of the anchor stage. If `reference_image_url` needs the file reachable at an
actual URL and no upload path is wired up yet, check `agent_help` for how PixelLab expects
that image to be hosted before falling back to base64 — don't default to base64 out of
habit now that the safer path is documented. If base64 is genuinely the only option
available, treat the transport as a known corruption risk on this call specifically and
sanity-check the fetched rotation against the approved anchor before proceeding to F.

Record, poll, fetch to `--stage rotation`.

### F — animate
**Template animations can queue while the character is still generating; v3 and pro
cannot.** Measured 2026-07-25 on unit-retinue: a template walk queued fine seconds after
`create_character` returned, while both v3 animations were rejected with *"character is
still being created — v3 animations need the finished rotation images."* So the batching
advice is mode-dependent: fire the template ones immediately, then poll `get_character`
to `completed` before the v3 ones. Do not read the rejection as a failed generation —
nothing is spent, it just has to be re-called.
Template mode is 1 generation per direction and by far the cheapest — prefer it. Escalate
only if a result is poor, and `delete_animation` first: template → v3 → pro. `pro`
requires calling once *without* `confirm_cost`, showing the owner the price, and
re-calling only after they agree.

Fetch each animation to `--stage anim`, into a per-animation subdirectory named after the
animation (`raw/anim/walk/south_01.png`), which is how `pack` resolves frame keys.

`get_character` does **not** expose per-frame animation URLs — only the character's
`download` endpoint, which returns a zip. That endpoint **returns HTTP 423 until every
queued job completes**, so poll `get_character` for `pending jobs` before fetching.
`fetch` detects a zip by magic bytes and extracts the PNGs itself.

### G — quantize, pack, report
```
py Scripts/art/pixelpipe.py quantize <id> --stage rotation
py Scripts/art/pixelpipe.py quantize <id> --stage anim
py Scripts/art/pixelpipe.py pack     <id>
py Scripts/art/pixelpipe.py report   <id>
```
`pack` computes **one global alpha bbox across every packed frame** and centres that in
each cell, so a walk cycle doesn't wobble. It refuses to scale: if the sprite is larger
than the 48px cell, lower `pixellab.size` and regenerate rather than resampling — pixel
art must never be resampled.

### H — import

**Preferred: straight through MCP** (verified working 2026-07-25, editor must be running):

```powershell
. "C:\Projects\ELVTRGAME\Scripts\ue-mcp.ps1"; $null = Test-Mcp
# 1. import
Invoke-McpTool -Toolset "editor_toolset.toolsets.texture.TextureTools" -ToolName "import_file" `
  -Arguments @{ folder_path="/Game/Sprites/Heroes"; asset_name="T_Hero_Vanguard";
                source_file="C:/Projects/ELVTRGAME/RawArt/Sheets/T_Hero_Vanguard.png" }
# 2. apply the ELVTR/SETUP-EDITOR.md settings  (values is a JSON STRING)
$tex  = @{ refPath = "/Game/Sprites/Heroes/T_Hero_Vanguard.T_Hero_Vanguard" }
$vals = '{"filter":"TF_Nearest","mipGenSettings":"TMGS_NoMipmaps","compressionSettings":"TC_EditorIcon","sRGB":true}'
Invoke-McpTool -Toolset "editor_toolset.toolsets.object.ObjectTools" -ToolName "set_properties" `
  -Arguments @{ instance=$tex; values=$vals }
# 3. READ BACK -- set_properties returns true even when it did nothing
Invoke-McpTool -Toolset "editor_toolset.toolsets.object.ObjectTools" -ToolName "get_properties" `
  -Arguments @{ instance=$tex; properties=@("filter","mipGenSettings","compressionSettings","sRGB") }
# 4. save, then confirm it is no longer dirty
Invoke-McpTool -Toolset "editor_toolset.toolsets.asset.AssetTools" -ToolName "save_assets" `
  -Arguments @{ asset_paths=@("/Game/Sprites/Heroes/T_Hero_Vanguard") }
```
`TC_EditorIcon` is the enum behind the "UserInterface2D (RGBA)" dropdown entry.
The toolset name is `SlateInspectorToolset.SlateInspectorToolset`, not `SlateInspector` —
list them with `describe_toolset` (parameter: **`toolset_name`**) if a call 404s.

**Fallback: editor Python**, when the editor's MCP server is down or a batch is wanted:
```
py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" <id>
```
Typed into the editor console. Same settings, reads them back, and logs
`SPRITE import ... [OK]` or `[VERIFY FAILED]`. With no argument it imports every request
that has a packed sheet.

## Composite requests — one texture, several subjects

Added 2026-07-25. A **subject** request describes one thing to generate. A **composite**
request generates nothing: it packs frames that other requests already produced into a
single shared texture. It exists because `T_Swarm_2bit` holds both swarm teams in one
sheet — the Niagara emitter has a single sprite renderer with a single material, so one
texture per subject would need a renderer change and would lose the single-index SubUV
decode in `SwarmSheet::CellForBits`.

A composite has a `composite.sources[]` block and **no** `prompt`/`pixellab`/`anchor`/
`canon`/`budget` (the schema's root `required` is an if/then on this). Each source has a
`prefix` and exactly one of:

- **`request`** — a pipeline-managed subject id. Its quantized frames are pulled in via
  `collect_frames`, so a composite can never see a frame the pipeline did not quantize.
- **`frames`** — an explicit frame-key → repo-relative PNG map for **unmanaged
  placeholder art**. Tagged `placeholder` in the manifest, and `report` prints a
  **PLACEHOLDER** verdict instead of PASS for as long as any such cell is packed.

`frame_map` keys become `<prefix>:<direction>.<state>`.

```
py Scripts/art/pixelpipe.py validate swarm-units
py Scripts/art/pixelpipe.py pack     swarm-units
py Scripts/art/pixelpipe.py report   swarm-units
```

Only those three apply — `prompt`, `authored`, `fetch`, `quantize` and `manifest` refuse
a composite and tell you to run against a source instead.

Two behaviours worth knowing:
- **The anti-wobble bbox is per source, not per sheet.** Sharing one bbox across
  subjects would let whichever has the longest reach dictate everyone's centring, so a
  big weapon swing on one team would shove the other team off-centre in every cell.
- **`pack` enforces the ramp itself** on any source with `quantize: true`, using the same
  `quantize_array` pass, and prints which frames it had to repair. Unmanaged art has not
  been through the quantize stage, so this is where it gets snapped; managed frames make
  it a measured no-op.

The placeholder route is a deliberate escape hatch, not a shortcut: it lets a sheet ship
with a stand-in for a subject that has no prose spec yet (e.g. an enemy whose canon is
still open) without the manifest ever claiming that art was generated here.

## Costs

| Operation | Generations |
|---|---|
| `create_character` standard | 1 |
| `create_character` v3 @ ≤48px | 2 (9 at larger sizes) |
| `create_character` pro | 20–40 |
| animation, template | 1 / direction |
| animation, v3 | ~1 / direction ≤96px; 128px≈2, 160px≈4, 256px≈8 |
| animation, pro | 20–40 / direction |
| `create_portrait_character` | 20 (≤64px) · 25 (128/160, rendered at 2K) |

A typical 48px hero with an 8-direction template walk is ~12–16 including a re-roll.

## Variants without regenerating

Two things are free or near-free and are often reached for wrongly:

- **Lamp-radius brightening** is not a generation. `quantize --light-shift` shifts every
  value one step up the same ladder, per `palette.json`'s `light_shift.map`. Light in
  this game is a palette shift, never a new hex.
- **A different individual in the same style** is `create_character` + a v3 reference
  image, *not* `create_character_state`. State always preserves the same person's
  identity and face, even for edits like "elderly" or "wounded" — it makes a variant OF
  someone, not someone new.

## Reporting

State plainly what actually happened: generations spent vs. budget, the QC verdict from
`report.json`, and whether the texture was imported *and verified* in the editor. Never
report a sprite as done because `pack` succeeded — packing a sheet and importing it are
different things, and only the read-back in stage H proves the import settings took.

If the anchor was never approved by a human, say so.
