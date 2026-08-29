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
| 5 screens + props | ~215 | `09e6114` | title_bg, card_win, card_lose, 10 castle props; retained not wired: door backdrop ×2, 4 wall variants |
| 6 boss + tropes | ~140 | `8282689` | necromancer (96 px pro), ghoul / wraith / bone_knight / plague_priest states, 15 clips; placeholder stats in units.json + waves.json |
| 7 boss hurt clips take1 | 7 | (rejected) | animate_image v3 text prompts on the south rotation only varied the staff flame, not the body; kept at raw/necromancer/hurtN_south_take1, not packed |
| 7 boss hurt clips take2 | 8 | `1534b8a`+ | animate_character template mode (taking-punch/leg-sweep/cross-punch/crouching/surprise-uppercut/hurricane-kick/getting-up) gives real torso/leg displacement; 7 necromancer hurt clips (4-7 frames each), combo playback in Unit.gd |
| 8 stone tank concepts | 6 | (uncommitted) | 6 rear 3/4 stone-tank concepts (turtle-dome, ziggurat, gothic, war-cart, obelisk, golem-borne) for owner pick; no retries needed |
| 7 hurt2/hurt5 regen | 2 | (pending) | owner sent back: hurt2 leg-sweep dropped the staff -> lead-jab keeps both hands (3f, old take at hurt2_south_take2); hurt5 surprise-uppercut never lifted the feet -> two-footed-jump (7f, old take at hurt5_south_take2) - still no clean liftoff, best of what templates gave |
| 9 turtle-dome variants | 6 | (uncommitted) | owner picked turtle-dome; 6 axis variants (heavy, mossy, wide, tall, treads, ember) for owner pick; no retries needed |
| 10 tall revision | 2 | (uncommitted) | d-tall: wider furnace mouth, bronze top ornament removed; take1 rendered blank/grey (kept as reject), take2 clean, sheet cell swapped to turtle_d_tall_r2 |
| 11 golem-borne variants | 6 | (uncommitted) | owner switched direction to 06 golem-borne; 6 axis variants (base, heavy, lean, runic, armoured, walking) round-body knuckle-walker read, flat top mount; no retries; arms/legs read more biped than knuckle-walk in most takes, flagged for owner |

**Balance after bucket 11: 5,646 generations remaining** (4,354 used this cycle). Expires 2026-09-10.
| 12 turtle_d_tall cupola removal | 20 | (uncommitted) | owner picked job 9706f739 (turtle_d_tall); inpaint_image masked to the top ornament, closed with plain stone cap; clean on first try, rest pixel-identical. Note: inpaint_image billed ~20 gens for this 128px edit, not the ~1-3 assumed going in — flagged, budget for future single-region edits accordingly |

**Balance after bucket 12: 5,626 generations remaining** (4,374 used this cycle). Expires 2026-09-10.
| 13 task-185 tank rotation | 2 | (uncommitted) | `create_character(mode="v3", reference_image_url=...)` on the owner's turtle_d_tall edit_01 concept — `create_8_direction_object`'s reference_image_base64 has no url variant and truncated the 13 KB inline payload twice (a known transport bug, not this request); v3 mode's reference_image_url sidestepped it cleanly. 8 clean rotations, no retries; north reads as the plain dome back + gun-mount nub, landed in game as the hero_turret's mount |

**Balance after bucket 13: 5,624 generations remaining** (4,376 used this cycle). Expires 2026-09-10.
| 14 stone tank takes | 9 | (uncommitted) | owner: "regular stone tank", not the turtle dome. 3 text-only `create_character` v3 128px takes (stone_a_plain, stone_b_heavy, stone_c_landship), all clean first try, land at `tank-concepts/raw/stone_*`; owner pick: stone_a_plain |

**Balance after bucket 14: 5,615 generations remaining** (4,385 used this cycle). Expires 2026-09-10.
| 15 cannon explosion | 3 | (uncommitted) | pixflux 96px fireball first frame (1) + `animate_image` 12 frames (2): white-hot mushroom burning down to smoke, static stem, starts at peak. Landed at `fx-slash/raw/explosion_pl` (packs as `fx_explosion_pl`); the zero-credit `fx_library.py explosion` (flash + swell + debris + hollowing smoke) is the default in `Battle.EXPLOSION_FX` |

**Balance after bucket 15: 5,612 generations remaining** (4,388 used this cycle). Expires 2026-09-10.

## Reserve menu (owner picks)

- Re-roll seeds: every unit clip ×2 more seeds for choice (~120), UI kits ×3 (~120), portraits ×1 (~175).
- More halls: `create_tiles_pro` floor/wall pairs for halls 2–4 (~100) — needs `World3D` per-wave texture swap.
- More props ×10 (~200), more tropes ×4 (~120), 2–3 new hero looks (~100 + 3 clips each).
- Siege screen: gate-open clip from `assets/env/castle/door_*.png` (~2), green smoke uses `fx_smoke_green` (0).
- Pro re-rolls of the weakest existing sprites (`create_character` pro, ~25 each).

Rejected / retained only: `army_sel_a` (baked checkerboard), `relic_tray` (opaque), `meters` v1
(auto-placed), Body 8 A/B (illegible), Title 32 A (rounded, not blackletter), door backdrops
(hall end is fully fogged), wall variants (too subtle at 32 px).

Balance after bucket 5: see the last `get_balance` line in the session; remaining allowance is
owner-directed (plan §6–7: necromancer boss, new enemy tropes, more halls/props, re-rolls).
