# PixelLab final cycle — spend log

Last Tier 3 cycle: 7,399 generations at start (2026-08-28), expire 2026-09-10. Plan:
`~/.claude/plans/snug-weaving-fairy.md` (UI → portraits → animations → VFX → screens/props →
boss/roster → reserve). Everything generated lands in `godot/RawArt/Renders/<bucket>/` or
`RawArt/Renders/<family>/raw/` first; keeps are copied by `Scripts/art/ui_land.py` (UI) or
packed by `Scripts/art/godot_pack.py` (sprites, clips, fx).

## Tools that landed

| Script | Job |
|---|---|
| `Scripts/art/pl_fetch.py` | download any MCP result (ui / image / font) by id |
| `Scripts/art/ui_crop.py` | split a PixelLab UI kit sheet into pieces (alpha band split, no scipy) |
| `Scripts/art/ui_land.py` | KEEP table: RawArt → `godot/assets/ui/` (fonts, kit pieces, icons, portraits) |
| `Scripts/art/anim_fetch.py` | JOBS / FX tables: `animate_image` results → clip dirs the pack scans |
| `Scripts/art/obj_fetch.py` | OBJECTS table: 8-direction props → `castle-props/raw/<slug>/rotations` |

## Lessons (cost of finding out)

- `create_ui_asset` with no `pieces` returns a **whole kit sheet** (lintel frame, panels, buttons,
  bar, medallion) for 40 gens — far better value than one panel. `pieces[].label` renders as
  literal text; leave labels empty. `elements:["health_bar"]` auto-places tiny; use `pieces`.
- `style_image_base64` and any base64 over ~2 KB is **randomly truncated in transit** (~30 %
  failure). Use `*_url` params: PixelLab rotation URLs
  (`backblaze.pixellab.ai/file/pixellab-characters/<account>/<character>/rotations/<dir>.png`)
  and `api.pixellab.ai/mcp/images/<job>/download` are public and work as `first_frame_url`.
- `animate_image` on our own raw rotation frame gives clips on the exact pack canvas: 1 gen per
  88 px clip, 2 per 92 px. Returns frame_count + 1 (index 0 = input). 20 concurrent jobs max.
- 8 px pixel fonts are illegible; 16 px Regular (body) + 16 px Bold (headings) + 32 px
  blackletter (title) is the working set. Godot: `.ttf.import` `antialiasing=0`, theme via
  `assets/ui/kindled.tres` + `gui/theme/custom` (a runtime `Window.theme` never won over the
  built-in Label font).
- `create_8_direction_object` with `style_object_id` of an existing prop keeps the prop family
  consistent; one in ten came with a baked white background (keyed out in `obj_fetch` run).
- `godot_pack.py` resolves some units through `selects.json` (`RawArt/Aaron Selects/...`), so
  clip dirs must sit beside *that* rotations dir — `anim_fetch.dest_dir` uses
  `godot_pack.rotations_dir` for this.

## Spend

| Bucket | Gens | Commit | Kept |
|---|---|---|---|
| 0 preview | ~230 | — | 3 fonts, 8 icons, banner, tray (rejected: opaque bg), hotbar, kit sheet |
| 1 UI chrome | ~600 | `2b3db70` | Body 16 / Heading 16 / Title 32 B fonts, 3 kit sheets, army panels idle+sel, meters, chips, cursor, crosshair |
| 2 portraits | ~175 | `2c1bfc8` | 7 hero busts (64 px) |
| 3 animations | ~70 | `76c7021` | 53 clips: 8 enemies × death/attack/walk, 4 allies × death/walk, 7 heroes × attack/hurt/walk |
| 4 VFX | ~20 | `c5a360f` | bolt, bolt_hit, mend, wall_fire, ring, smash, smoke_green, orb, relic, arrow |
| 5 screens + props | ~215 | (this commit) | title_bg, card_win, card_lose, 10 castle props; retained not wired: door backdrop ×2, 4 wall variants |

Rejected / retained only: `army_sel_a` (baked checkerboard), `relic_tray` (opaque), `meters` v1
(auto-placed), Body 8 A/B (illegible), Title 32 A (rounded, not blackletter), door backdrops
(hall end is fully fogged), wall variants (too subtle at 32 px).

Balance after bucket 5: see the last `get_balance` line in the session; remaining allowance is
owner-directed (plan §6–7: necromancer boss, new enemy tropes, more halls/props, re-rolls).
