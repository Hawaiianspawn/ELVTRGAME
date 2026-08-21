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

## Files

- Contact sheet: `RawArt/Renders/castle-art-pass/contact.png`
- All 8 generations (6 kept finals + 2 rejected first attempts): `RawArt/Renders/castle-art-pass/`
- This doc: `docs/art/castle-art-pass.md`
