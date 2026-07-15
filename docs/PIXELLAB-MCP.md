# PixelLab MCP — local reference

**Source:** https://api.pixellab.ai/mcp/docs (fetched 2026-07-11) + firsthand usage notes.
**Account:** Tier 3 "Pixel Architect" subscription — 10,000 generations/period (balance via `get_balance`).
**Web UIs:** assets made via MCP appear in the Character Creator / Map Workshop on the same account — usable for visual refinement after programmatic generation.

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

## Style-control reality check (for our 2-bit direction)

- Palette hexes in the `description` are **soft guidance only** — nothing enforces a 4-value ramp. Expect off-palette output; plan the quantize+hand-clean pass every time (aesthetic-direction §2.4 / vignette-pipeline discipline).
- `outline`/`detail` are hints in v3; `shading`/`proportions` are ignored in v3. For strict-style needs try `create_tiles_pro` with `style_images`, or `standard` mode with `text_guidance_scale` raised (default 8, max 20).
- Useful phrasing that helped: "muted dark limited palette" + explicit hexes + "somber dark fantasy"; `outline: selective outline`, `detail: low detail` at 48px.

## Docs & resources

- Platform docs: `pixellab://docs/overview` (MCP resources), incl. engine guides (Godot/Unity/Python for tilesets). No Unreal guide — our import discipline is `ELVTR/SETUP-EDITOR.md`.
- API v2 reference: https://api.pixellab.ai/v2/llms.txt · Setup: https://pixellab.ai/vibe-coding
- Principle from the docs: these are MCP tools, not REST endpoints — don't curl the API.
