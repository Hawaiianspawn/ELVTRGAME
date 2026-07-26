# PixelLab MCP — local reference

**Source:** https://api.pixellab.ai/mcp/docs (fetched 2026-07-11) + firsthand usage notes.
**Account:** Tier 3 "Pixel Architect" subscription — 10,000 generations/period (balance via `get_balance`).
**Web UIs:** assets made via MCP appear in the Character Creator / Map Workshop on the same account — usable for visual refinement after programmatic generation.

## The pipeline (start here)

Sprite generation is no longer hand-driven. The chain from art spec to imported UE
texture is `.claude/skills/sprite/SKILL.md`, driven by one schema-validated request per
subject:

| Layer | Where |
|---|---|
| The contract | `docs/data/art/requests/<id>.json`, validated against `docs/data/art/sprite-request.schema.json`; the ramp as data in `docs/data/art/palette.json` |
| Local work | `Scripts/art/pixelpipe.py` — validate, compose prompt, fetch, quantize, pack, report |
| UE import | `Scripts/art/import_sprites.py` (editor Python) |
| Orchestration | `.claude/skills/sprite/` — the only layer that calls `mcp__pixellab__*` |

### Three findings that shaped it (verified 2026-07-25 via `agent_help`)

1. **`create_character` accepts no seed.** Confirmed directly: *"Seed/reproducibility: No.
   create_character does not accept a seed parameter."* Reproducibility therefore has to
   come from our side — a deterministically composed prompt plus a manifest recording
   every UUID and cost. This is why the pre-pipeline folders in `RawArt/Renders/` can be
   regenerated but never *iterated on*: nothing records what made them.
2. **Nothing forces a palette.** Confirmed: *"No tool explicitly forces a strict 4-color
   limit."* `create_character_state(use_color_palette_from_reference=True)` is the only
   palette-snapping mechanism, and it only applies to variants of an existing character.
   So the quantize pass is mandatory infrastructure, not cleanup.
3. **v3 + `reference_image_base64` is the strongest style lever available.** It rotates
   your exact sprite into 8 directions — *"Faithfulness: High... preserving identity and
   style"* — and animations inherit the rotated sprites as their base. Hence anchor-first:
   generate one south frame, quantize it to the ramp, feed it back, and the palette and
   silhouette propagate to every rotation and animation frame for free. **Measured on
   hero-vanguard, 2026-07-25: 0% on-palette from generated anchors vs 99.2–100% from a
   quantized reference**, with the class's defining prop retained in all eight directions.
   It costs 1 generation, not 2.

   **The corollary, learned the same day: v3 reference *rotates*, it does not *render*.**
   Handed an ASCII-schematic anchor it reproduced the schematic almost pixel-for-pixel.
   The anchor's artistic quality is the ceiling for the entire sheet — so the cheap
   authored-ASCII route (below) gives a perfect greybox blockout, and shippable art needs
   a real hand-drawn 48px anchor pushed through the same path. Rear-facing rotations
   degrade most; judge a rotation pass on north, not south.

The practical shape of that: **one frame defines the subject.** Spend the effort and the
human attention on the south-facing anchor; everything downstream inherits it, so an
unapproved anchor wastes the whole batch rather than one call.

### The mode trap (learned the expensive way, 2026-07-25)

**Generate the anchor in `standard` mode, not `v3`.** v3 silently ignores `shading`,
`proportions`, `text_guidance_scale` and `n_directions`. A v3 text-to-sprite anchor
therefore throws away "chibi" and "flat shading" without saying so. Measured on
`hero-vanguard` r1: the result was a realistically-proportioned, heavily-shaded metallic
knight — 60% Dark against a spec demanding Steel ≥50%, Pale spilled onto the shield, and
a swallowtail flag despite an explicit negative prompt. It failed the art director's own
written reject criteria on five counts.

`standard` is the only mode that honours those params, and it costs 1 generation instead
of 2. **v3's value is the reference-image path**, where the style comes from pixels you
already approved rather than from params it would have discarded. So:
`standard` → quantize → **v3 + `reference_image_base64`** → animate.

Related: prefer `outline: "selective outline"`. Full black outlining is the biggest single
driver of Dark dominance under a 4-value collapse, and Direction A specifies selective
outlining regardless.

## Retention rule — every generation gets saved

**Every PixelLab creation (character, portrait, tile, tileset, object, UI asset, font) gets downloaded to `RawArt/Renders/` — no exceptions, including rejects and rough drafts.** Nothing gets discarded at generation time.

- Once we decide we like a result, move it out of `RawArt/Renders/` into its proper home (e.g. `RawArt/Portraits/`, `docs/art/`, engine content folders).
- Anything still in `RawArt/Renders/` is undecided — keep it until a call is made, don't prune it to "clean up."
- Reason: candidate frames, style tests, and rejected drafts are cheap to generate but expensive to reproduce exactly (seeds/prompts drift); keeping every result preserves the option to revisit a "reject" once the eye adjusts to the style.

## Architecture

- **Everything is async/non-blocking:** creation tools return a UUID job/asset ID immediately; poll the matching `get_*` tool. Typical times: characters/objects 2–4 min, isometric tiles 10–20 s, animations 30–60 s per direction, UI assets 30–90 s.
- **Batch pattern:** submit all creations first (collect IDs), then poll in parallel.
- **Animations can be queued immediately after `create_character`** — no need to wait for the character to finish.
- **Downloads need no auth** — the UUID in the URL is the access key. Treat download links as public.
- **Auto-cleanup gotcha:** `create_map_object` results are deleted after 8 hours unless retained.

## Tools we care about (ELVTR)

### Characters & portraits

| Tool | What | Notes |
|---|---|---|
| `create_character` | text → sprite w/ 4 or 8 directional views | `size` = canvas px, **character occupies ~60% of canvas height**; `view`: `low top-down` (classic 3/4 RPG — our camera), `high top-down`, `side`, `oblique` (beta). Modes: `standard` 1 gen (style params soft guidance) · `v3` 2–9 gens, highest quality, always 8-dir, ignores shading/proportions (outline+detail still soft hints) · `pro` 20–40 gens, ignores all style params. **Firsthand: v3 at 48px = 2 generations, ~3–5 min.** |
| `create_character_state` | variant of existing character (identity preserved) | `edit_description` applied across all rotations; auto-waits ≤30 s for source. Good for: light-shifted palette variants, gear states. |
| `animate_character` | animation on existing character | `template_animation_id` or free-text `action_description`; `frame_count` default 8; one job per direction; `confirm_cost` flag exists. |
| `create_portrait_character` | **converter**, portrait PNG ↔ character sprite | NOT text-to-image. `direction`: `character_to_portrait` gives a bust from a full-body sprite. `image` = base64 PNG in. `result_size`: 16–160; **128/160 render at 2K for extra detail (cost more)**. Poll `get_portrait_character(job_id)`. |
| `get_character` | status / rotation URLs / download | Branches: processing (progress+ETA) / failed (error+retry) / completed. |

**Portrait pipeline for ELVTR:** no direct text→portrait tool exists. Path: `create_character` (48px, low top-down, v3) → download a rotation PNG → base64 → `create_portrait_character(direction="character_to_portrait", result_size=128)`. Output is a **draft for the hand-quantize pass** — PixelLab will not hold a strict 4-value palette; exact-palette enforcement (portrait-register.md §2.2) is always manual cleanup.

### Tilesets & tiles

| Tool | What | Notes |
|---|---|---|
| `create_topdown_tileset` | 16-tile Wang (corner) autotile set; 25 tiles at `transition_size=1.0` (cliff/wall fills) | `lower_description` + `upper_description`; `transition_size` 0=sharp/0.25/0.5/1.0; `view` high=RTS, low=RPG. **Chaining:** pass a finished set's `base_tile_id` as `lower_base_tile_id` for seamless biome chains (ocean→beach→grass…); base IDs return immediately, chain without waiting. |
| `create_sidescroller_tileset` | 16-tile platformer set | same chaining idea via `base_tile_id`. |
| `create_isometric_tile` | single iso tile | 10–20 s; ≥24px recommended (32 best); `tile_shape` thin/thick/block; **same `seed` across tiles = consistent style**. |
| `create_tiles_pro` | pro tile tool, style-image support | takes `style_images` — the lever for matching our locked ramp aesthetic. |

### Other

- `create_map_object` / `create_1_direction_object` / `create_8_direction_object` — props; multi-candidate objects enter a **review state**: `get_object` shows candidate frames → `select_object_frames(indices)` to keep, `dismiss_review` to discard.
- `create_ui_asset` — pixel UI panels (default 256×256, rounded-rect if `pieces` omitted; takes `color_palette`). Candidate for medallion-frame / chronicle-frame drafts.
- `create_font` — pixel font (pro): `glyph_px` default 16. Candidate for the blackletter-versal/UI type register drafts.
- `agent_help(question)` — searches PixelLab's own docs; use before guessing.
- `agent_feedback` — report tool issues upstream.
- Sandbox + chat + deployed-agent tools exist (Node.js sandboxes on `*.dev.pixellab.run`, project git URLs via `list_projects`) — not part of our pipeline; UE project stays local.

## The binding constraint is VALUE SPREAD, not resolution (measured 2026-07-25)

The question "how do we make the units look better" has a measurable answer, and it is not
size, detail level, or prompt length. Under a 4-value ramp, **a sprite is only as good as
the spread of its source luminance across the four buckets.** Rec.601 luma of every opaque
pixel of the south rotation, bucketed to the nearest Demichrome entry:

| Subject | Dark | Steel | Bone | Pale | src colours | result in the sheet |
|---|---|---|---|---|---|---|
| `brood-ooze` 4a75b4ac | **89%** | 7% | 3% | 1% | 16 | featureless blob |
| `unit-knight` 1c935515 | 38% | 37% | 21% | 3% | 871 | holds its structure |
| `unit-retinue` (authored anchor) | 29% | 31% | 40% | 0% | 3 | holds its structure |

The ooze is 89% inside a single bucket, so quantizing it produces a silhouette with no
interior — every body pixel is the same value. That is why brood cells 0–2 in
`T_Swarm_2bit` measure **93% flat Steel** and cell 3 measures **91% flat Bone**, and why
its hit frame had to become a whole-body value flip: a flat mass cannot express a pose.
**No pipeline stage can recover contrast that the source does not contain in value terms.**

The knight, generated from the same account with no palette guidance at all, lands near the
ideal distribution by accident of subject — lit plate has real mid-tones, and a 3% Pale
accent falls on the helm rim rather than smearing across the body.

**So the lever is the prompt, not the pipeline.** Asking for a "black ooze" gets black
pixels, and black is one value. Ask instead for a subject with **internal material
contrast lit from one side** — the thing being described has to have light and dark parts
before the quantizer sees it. Check a candidate by bucketing its luma before spending any
generations on rotations or animations.

### API facts confirmed 2026-07-25 (`agent_help`)

1. **Generate at the size you will ship.** *"Generating at size=48 is recommended. The model
   is optimized for native sizes; downscaling an 88px generation often introduces artifacts
   or blurriness that ruins pixel-perfect clarity."* Note this is a **future** lever, not a
   current defect: our pack normalises by bbox and does not resample (raw ooze bbox 40×44 →
   29×44 in a 48px cell). Both owner-supplied characters are 88px because they came from the
   web Character Creator, which is fine — but pipeline-generated subjects should ask for 48.
2. **Nothing renders to an external palette.** `create_character_state(use_color_palette_from_reference=True)`
   snaps a variant to *its own source character's* palette; there is no way to target
   Demichrome. The local quantize pass is permanent infrastructure.
3. **v3 animation does not preserve palette.** *"It is a generative process and does not
   guarantee bit-perfect palette preservation."* Every animation frame must be re-quantized,
   never trusted because its reference frame was on-ramp.

### The bigger win nobody has spent yet

Eight rotations are generated and **one** is used. `SwarmAnim::FlipBit` only mirrors in X, so
every unit faces the camera no matter which way it is walking. Wiring the directional buckets
(`docs/RENDERING-LIGHTING.md` §4a) costs no generations — the art is already on disk — and
changes the read of the whole field more than any per-sprite quality work would.

## Style-control reality check (for our 2-bit direction)

- Palette hexes in the `description` are **soft guidance only** — nothing enforces a 4-value ramp. Expect off-palette output; the quantize pass is automated now (`pixelpipe.py quantize`), so the hand-clean is optional polish rather than a mandatory step.
- **Measured, 2026-07-25:** the existing renders in `RawArt/Renders/` are **0% on-palette**, including the folder named `npc-demichrome` — it holds 8 distinct off-ramp colours, and `hero-rev1-grayscale` isn't even grayscale (it contains `#a39cea`). Naming a palette in the prompt did nothing. Treat this as the expected outcome, not a bad run.
- `outline`/`detail` are hints in v3; `shading`/`proportions` are ignored in v3. For strict-style needs try `create_tiles_pro` with `style_images`, or `standard` mode with `text_guidance_scale` raised (default 8, max 20).
- Useful phrasing that helped: "muted dark limited palette" + explicit hexes + "somber dark fantasy"; `outline: selective outline`, `detail: low detail` at 48px.

## Docs & resources

- Platform docs: `pixellab://docs/overview` (MCP resources), incl. engine guides (Godot/Unity/Python for tilesets). No Unreal guide — our import discipline is `ELVTR/SETUP-EDITOR.md`.
- API v2 reference: https://api.pixellab.ai/v2/llms.txt · Setup: https://pixellab.ai/vibe-coding
- Principle from the docs: these are MCP tools, not REST endpoints — don't curl the API.
