# Castle art pass — probe (2026-08-20)

Six `create_image_pixflux` probes for the battle hallway (`godot/scripts/Battle.gd`
`_draw_walls` / `View.gd` `draw_ground`, currently flat-colour rectangles) and the
siege intro keep facade (`godot/scripts/Siege.gd` `_draw`). Dark, run-down, not
lived-in: near-black stone, desaturated grey, sickly green as the only accent.

## Owner pick

**2026-08-20:** facade from #1, gate from #3, walls from #4, UI frame from #6 — **darkened**. Finals must push darker than the probes: near-black stone, no visible sky, grime, collapsed sections, the green glow as the only light. Floor variants: skip #5, generate fresh with `create_tiles_pro` (numbered prompt, style_images = #4).

## Contact sheet

![contact sheet](../../RawArt/Renders/castle-art-pass/contact.png)

## Examples

| # | Subject | Tool | Size | View | Seed | Job ID | File | Verdict |
|---|---|---|---|---|---|---|---|---|
| 1 | Keep facade, gate closed | `create_image_pixflux` | 320x180 | side | 10001 | `ac2c36b9-cd62-4e8a-932c-a2b2493655f9` | `RawArt/Renders/castle-art-pass/01-keep-facade-closed-gate.png` | Good — reads as the target style immediately |
| 2 | Keep facade, gate open/glow | `create_image_pixflux` (img2img on #1) | 320x180 | side | 10001 | `12768c91-c657-4d4b-a003-c809e9824256` | `RawArt/Renders/castle-art-pass/02-keep-facade-open-gate.png` | Partial — glow is stronger and spills further, but the doors don't visibly swing open at `init_image_strength=200`; a lower strength (~50-100) is needed for finals to actually open the gate |
| 3 | Gate close-up | `create_image_pixflux` | 160x200 | side | 10003 | `4e834ec8-d90c-4383-bcff-2f92ed710d69` | `RawArt/Renders/castle-art-pass/03-gate-closeup.png` | Good — broken hinge, moss, green gap-glow all landed on the first try |
| 4 | Hallway wall strip | `create_image_pixflux` (retry) | 256x128 | side | 20004 | `5255310b-4053-4cdd-b9e3-c24e83852f04` | `RawArt/Renders/castle-art-pass/04-wall-strip.png` | Good on retry — see below |
| 5 | Floor tile sheet | `create_image_pixflux` (retry) | 256x64 | high top-down | 20005 | `894e2c13-b8ba-4447-85ed-9d05b13c61e2` | `RawArt/Renders/castle-art-pass/05-floor-tiles.png` | Weakest of the six — see below |
| 6 | UI panel frame | `create_image_pixflux` | 192x96 | (unset) | 10006 | `d17dcf99-ddaa-49ff-a5f1-9b789c176ecf` | `RawArt/Renders/castle-art-pass/06-ui-panel-frame.png` | Good — hollow centre, iron corners, green rune-glow accent, first try |

**Rejected first attempts, kept per retention rule (never deleted):**

| File | Why rejected |
|---|---|
| `RawArt/Renders/castle-art-pass/04-wall-strip-v1.png` | Read as 6 repeated arched alcoves, not 6 distinct block patterns; used blue+gold lighting instead of green-only |
| `RawArt/Renders/castle-art-pass/05-floor-tiles-v1.png` | Rendered as vertical side-view panels, not top-down flagstones at all |

### What worked / what didn't, per example

1. **Facade closed** — first try, no retry needed. `view="side"` + explicit "near-black
   stone, sickly green as the only accent" nailed the palette immediately.
2. **Facade open** — img2img via `init_image_url` (the completed job's public download
   link, no auth needed) kept the exact facade/palette, but `init_image_strength=200`
   preserved the doors' *shape* along with everything else — the description asked for
   open doors and the model just brightened the glow instead of parting them. Lower the
   strength for finals if "open" needs to be structurally different from "closed."
3. **Gate close-up** — first try. Straightforward single-subject prompt, no retry needed.
4. **Wall strip** — first attempt failed because "a run of 4-6 visibly different block
   patterns" read as generic architectural variety (arches) rather than discrete numbered
   segments. Retry fixed it by explicitly numbering each segment 1-6 in the prompt
   (`1 plain cracked stone... 2 mossy...`), naming the palette negatively ("no blue no
   gold no orange"), and raising `text_guidance_scale` to 14. The numbered-list phrasing
   is exactly what `agent_help` recommends for `create_tiles_pro` too (see below) — it
   holds for pixflux as well.
5. **Floor tiles** — same numbering + guidance-scale fix improved it from "vertical
   side-view strips" to an actual top-down floor read, but the seven named variants
   still blur into a repeating moss-and-rubble texture rather than seven cleanly
   separated tiles — moss and debris apply across the whole strip instead of one tile
   each. This is the one example that didn't converge in the 2-generation budget;
   `create_tiles_pro` (below) is the right tool for a clean variant sheet, not pixflux.
6. **UI frame** — first try. No `view` param needed for a flat UI asset; hollow centre
   and iron corner brackets came through as asked.

## Generations spent

`get_balance` before: **8384** generations remaining (1616 used of 10000, Tier 3).
`get_balance` after: **8376** generations remaining.
**8 generations spent** (6 base examples + 1 retry each on wall strip and floor tiles —
at the stated cap of 6 + at most 2 retries).

## Workflow notes (`agent_help`, verbatim)

**Q1 — variant consistency within one tile/wall sheet, matching a chosen style ref:**

> For consistent tile/wall variants, use `create_tiles_pro(description="...", style_images=[...])`.
>
> Workflow:
> 1. Select your best style reference image (e.g., the wall block variant).
> 2. Prepare `style_images` as a JSON array of base64 strings.
> 3. In description, number variants: "1). cracked stone block 2). mossy stone block 3). weathered stone block...".
> 4. The model uses the `style_images` to match palette, outline, and shading.
> 5. Use `outline_mode="segmentation"` for cleaner edges without outline artifacts.
>
> Note: `create_tiles_pro` ignores shape controls when `style_images` are provided.
>
> hint: `create_tiles_pro(description="1). cracked stone block 2). mossy stone block 3). weathered stone block 4). plain stone block 5). stone block with vines 6). broken stone block", style_images="[{\"base64\": \"...\"}]", outline_mode="segmentation")`

**Q2 — carrying one approved style reference across separate calls (facade → tiles → UI asset):**

> To match the style of a reference image across different asset types, use the
> `style_image_url` or `style_image_base64` parameters in each tool. These parameters
> force the generator to adopt the palette, outline, detail, and shading of your
> reference image.
>
> For tiles: use `create_tiles_pro(style_images='[{"url": "...", "width": N, "height": N}]', style_options='{"color_palette": true, "outline": true, "detail": true, "shading": true}')`.
>
> For UI: use `create_ui_asset(style_image_base64="...")`.
>
> For general images: use `create_image_pro(style_image_url="...", style_copy=["color_palette", "outline", "detail", "shading"])`.
>
> Note: `create_tiles_pro` ignores shape controls when `style_images` is provided. Prefer
> URLs over base64 to avoid truncation.

**Cost per tool** (from `docs/PIXELLAB-MCP.md`, this probe used only the 1-generation
tool): `create_image_pixflux` = 1 generation/call (used here, ~10-40s). Finals below use
`create_image_pro` (20-40 generations), `create_tiles_pro` (pro-tier tile tool, cost not
itemized separately from the general pro tier in local docs — confirm with `get_balance`
before/after a real run), and `create_ui_asset` (default 256x256, pro-tier).

### Known rules from `docs/PIXELLAB-MCP.md` that apply here

- **Nothing forces a palette except `color_image_base64`/`color_image_url` on pixflux
  itself.** The "near-black stone, sickly green only" language in every prompt above is
  soft guidance — it worked well on 4 of 6 on the first try, but #4 and #5 needed a
  retry specifically because off-palette blue/gold leaked in despite the prompt. Expect
  the same on finals; budget a quantize/clean pass if these ship as-is rather than
  through `create_tiles_pro`'s `style_images` palette lock.
- **Generate at the size you will ship.** All six probes were generated at their final
  in-engine target size (320x180 facade, 256x128 wall strip, 256x64 floor strip, 192x96
  UI frame) rather than generated large and downscaled, per the documented "downscaling
  introduces artifacts" finding.
- **`outline: selective outline`** was used throughout per the documented preference —
  full black outlining is the biggest driver of flat/dark dominance under a value
  collapse, and none of these six needed a harder outline to read.

## Prompt sheet for finals (paste-ready)

Once the owner picks a direction above, these are the calls to make. Each references
the **chosen probe's download URL** as `style_image_url`/`style_images` per the
`agent_help` answers — swap `<CHOSEN_FACADE_URL>` etc. for the actual pick.

**1. Facade (open + closed), `create_image_pro`:**

```
create_image_pro(
  description="ruined castle keep facade, iron portcullis and ornate iron-banded double doors, crumbling battlements, cracked weathered stone, sickly green light leaking from within, near-black stone, desaturated grey masonry",
  style_image_url="<CHOSEN_FACADE_URL>",
  style_copy=["color_palette", "outline", "detail", "shading"],
  width=320, height=180
)
```
Run twice — once with "doors closed, light leaking through cracks only" and once with
"doors fully open, light and smoke pouring out" — rather than relying on img2img
strength to force the state change (see example 2's finding above).

**2. Hallway wall variants, `create_tiles_pro`:**

```
create_tiles_pro(
  description="1). cracked stone wall block 2). mossy damp stone wall block 3). collapsed rubble wall block 4). bricked-up sealed archway wall block 5). empty rusted sconce mount 6). lit sconce with sickly green torch flame",
  style_images="[{\"url\": \"<CHOSEN_WALL_OR_FACADE_URL>\"}]",
  outline_mode="segmentation"
)
```

**3. Floor tile variants, `create_tiles_pro`:**

```
create_tiles_pro(
  description="1). plain flagstone 2). cracked flagstone 3). wet dark flagstone 4). rubble-strewn flagstone 5). iron floor grate 6). dried bloodstain flagstone 7). mossy flagstone",
  style_images="[{\"url\": \"<CHOSEN_FACADE_URL>\"}]",
  outline_mode="segmentation"
)
```

**4. UI panel frame, `create_ui_asset`:**

```
create_ui_asset(
  description="stone-and-iron dialog frame, hollow centre, riveted iron corner brackets, cracked worn stone, thin sickly green rune-glow inner edge",
  style_image_base64="<CHOSEN_UI_PROBE_BASE64>",
  width=192, height=96
)
```

## Wall + floor finals (2026-08-20, `create_tiles_pro`)

Ran the queued prompt sheet above (walls + floor, both against the #4 wall-strip pick per
the owner's note), with the "push darker" direction folded into both descriptions. 16
variations landed for each (`create_tiles_pro` auto-computes count from tile size — the 6
and 7 named items above are a floor, not a cap; extra slots came back as unlabelled
palette-matched variants, kept rather than discarded per the retention rule).

| | Tool | Job ID | Style ref | Cost | Size | Files |
|---|---|---|---|---|---|---|
| Walls | `create_tiles_pro` | `0892db47-b8c6-4b79-bc67-8e2c10331b0c` | #4 wall-strip (downscaled) | 20-40 gen | 16 x 64x64px | `RawArt/Renders/castle-art-pass/finals/walls/tile_0..15.png` |
| Floor | `create_tiles_pro` | `aaeaba58-9d67-4f53-aaf6-2eb426d06d2e` | #4 wall-strip (downscaled) | 20-40 gen | 16 x 64x64px | `RawArt/Renders/castle-art-pass/finals/floor/tile_0..15.png` |

Contact sheets: `RawArt/Renders/castle-art-pass/finals/walls-contact.png`,
`RawArt/Renders/castle-art-pass/finals/floor-contact.png`.

**Verdict (unreviewed by owner):** on-direction at a glance — near-black stone body,
green survives only as the lit-sconce glow and moss accents, floor set has real material
variety (plain / cracked / wet / rubble / iron grate / bloodstain / moss). Needs an owner
pick pass like the original six before anything moves out of `RawArt/Renders/`.

**Known gap vs. the plan above:** `create_tiles_pro`'s `style_images` only accepts inline
base64, not a URL — the `<CHOSEN_WALL_OR_FACADE_URL>` placeholders in the prompt sheet
don't work as written despite `agent_help`'s Q2 answer suggesting `url` is accepted (schema
validation rejects it: `Extra inputs are not permitted`). The full-resolution #4 PNG's
base64 (256x128, ~36KB encoded) also failed silently as "broken data stream... TRUNCATED in
transit" when passed inline — this MCP transport has a practical limit well under that. The
working path: downscale + palette-quantize the reference PNG locally first (this run used
64x32, 16 colors, ~900 B encoded) before handing it to `style_images`. That's also why the
output landed at 64x64 rather than the doc's 256x128 target — the tool sizes tiles from the
style reference's dimensions. If 256px-wide wall strips are needed for the final ship
asset, the next attempt should feed a reference at that resolution but pre-quantized to a
small palette so the encoded size stays under whatever this transport's real cap is
(untested exactly where it sits, only that 36KB fails and 0.9KB works).

## Files

- Contact sheet: `RawArt/Renders/castle-art-pass/contact.png`
- All 8 generations (6 kept finals + 2 rejected first attempts): `RawArt/Renders/castle-art-pass/`
- Finals (walls + floor, unreviewed): `RawArt/Renders/castle-art-pass/finals/`
- This doc: `docs/art/castle-art-pass.md`

## Wall material experiment (2026-08-20)

Six PixelLab calls, ~105 generations. Everything under `RawArt/Renders/castle-hall/wall-materials/`.

| # | Tool / params | Result | Verdict |
|---|---|---|---|
| A | `create_tiles_pro` square_topdown, `tile_view="side"`, 32px, seed 2001, numbered 6-variant ashlar prompt | `ashlar/` 16 tiles, 32x32 — but only a **12px wall lip** at the bottom of a floor tile (bbox rows 10-22) | wrong shape: "side" view still paints a floor square with a wall edge |
| B | same, brick prompt, seed 2002 | `brick/` — same lip shape, darkest set (mean luma 33) | same |
| C | same, cyclopean prompt, seed 2003 | `cyclopean/` — lip shape, purple-grey drift | same |
| D | same, plaster prompt, seed 2004 | `plaster/` — lip shape, brightest (luma 46), teal mould off-palette | same |
| E | `create_image_pixflux` 384x128, side, `text_guidance_scale=14`, seed 3001, 6 numbered segments | `wall-strip-pixflux.png` — full faces, 6 readable variants (crack, rubble, moss, sconce, arch) | usable, 1 generation; mid-grey, no fade |
| **F** | **`create_tiles_pro` square_topdown, `tile_view_angle=0`, `tile_size=32`, `tile_height=64`, seed 2001, same ashlar prompt** | **`ashlar-face64/` 16 distinct full wall faces, 32x44 usable (crop rows 10-54), near-black, one palette, all 6 asked variants + 10 extra** | **the recipe** — `contact-face64.png`, mock hall in `mock-hall-wall.png` |

![](../../RawArt/Renders/castle-hall/wall-materials/contact-face64.png)
![](../../RawArt/Renders/castle-hall/wall-materials/mock-hall-wall.png)

Findings:
- `tile_view="side"` is NOT a wall face. The tile is always a floor square; "side" only deepens its lip. For faces use `tile_view_angle=0` and a tall `tile_height`.
- One tiles_pro call (20-25 gens) returns 16 variations even when 6 are asked; the extras are free variety.
- Grid seams in the mock are two things: the `outline_mode="outline"` 1px border on every tile, and the baked top ledge repeating each course. Fix next run with `outline_mode="segmentation"`; and trim the top 4px on non-top courses when stacking.
- Material family: ashlar reads best at 32px. Brick turns to noise; cyclopean drifts purple; plaster goes too bright and pulls teal.
- Palette is prompt-driven only (no forced palette on tiles_pro) but the "near-black desaturated grey, sickly green only accent" phrasing held in F.

Recipe for task-155 finals: run F again with `outline_mode="segmentation"` (same seed for comparability) for walls; floors via `create_tiles_pro` `tile_view="top-down"`, 32px, numbered 8-variant flagstone prompt, same palette phrasing.

### Floor tiles, flat (2026-08-20)

| G | `create_tiles_pro` square_topdown, `tile_view="top-down"`, 32px, `outline_mode="segmentation"`, seed 2005, 8 numbered flagstone variants | `floor-flat/` 16 tiles, 32x32 full-bleed, no border, seamless, palette held | **the floor recipe** — `contact-floor-flat.png`, `mock-floor-flat.png` |

`tile_view="top-down"` = zero depth, a pure flat square; `segmentation` removes the outline border so tiles butt together clean. Weight plain tiles ~80% when scattering.
