# Battle hall in real 3D — Sprite3D billboards, depth-tested

Task-158: the battle scene is a `Node3D` with a fixed `Camera3D` and depth-tested `Sprite3D` pixel
billboards. Sorting is per pixel now, so the y-sort problem (scaled sprites in one depth band, and
juggled units whose screen y is shifted by `air_h`, drawing over each other wrongly) is gone.

## Read this first: the class is `Hall3D`, not `World3D`

`World3D` is a built-in engine class (`Viewport.world_3d`) and `class_name World3D` will not
compile. The file is at the path the task named — `godot/scripts/World3D.gd` — but declares
`class_name Hall3D`. **Task-161 must use `Hall3D`.**

## Numbers

FPS on this scene is noisy and, more importantly, it moves with whatever else the machine is doing.
An earlier set of samples here ran 161-237 on the 3D build and 166-186 on the 2D one, interleaved
with `git worktree` checkouts of 8620 files — the disk churn was in the measurement, and the two
builds were not sampled under the same conditions. **Those numbers were thrown out.** What follows
is a single back-to-back run: worktree checked out and imported first, then three probes of the
pre-change build and three of the 3D build with nothing else touching the disk in between.

    before (2D, HEAD 0082e09), godot-run.ps1 -Probe "battle,12":
      PROBE battle fps=209 process_ms=5.7 physics_ms=0.0 units=68 objects=277
      PROBE battle fps=213 process_ms=5.2 physics_ms=0.0 units=66 objects=308
      PROBE battle fps=209 process_ms=5.4 physics_ms=0.0 units=65 objects=311

    after (3D), same probe, immediately after:
      PROBE battle fps=315 process_ms=4.0 physics_ms=0.1 units=63 objects=407
      PROBE battle fps=314 process_ms=3.8 physics_ms=0.0 units=65 objects=433
      PROBE battle fps=317 process_ms=4.6 physics_ms=0.0 units=67 objects=395

**315 / 209 = 151% of baseline.** Not just clearing the 90% floor — the 3D build is faster, and
`process_ms` (5.4 -> 4.0) says why: it is GDScript that got cheaper, not the GPU. Per frame the old
build re-projected every unit (`view.project`, `sprite_scale`, and a `Color` fog lerp each), then
rebuilt the whole hall in `_draw`: 14 depth bands x 7 columns of `draw_polygon` for the floor plus
2 sides x 14 bands x 3 courses for the walls. All of that is now static geometry that moves by one
transform per frame, and units just write a `Vector3`.

The stress probe (851 units) was measured the same way, because 851 `Sprite3D` are 851 draw calls
where the 2D canvas batched them into a handful — the obvious place for 3D to lose:

    before: fps=19 process_ms=54.7 units=851 | fps=19 process_ms=54.2 units=851
    after:  fps=20 process_ms=52.1 units=852 | fps=20 process_ms=54.6 units=854

Same, a hair ahead. `process_ms` at ~53 of a ~52 ms frame says the wall there is the GDScript sim
on both builds; the per-sprite draw calls do not show up even at 851 units.

![before](hall-3d-before.png)
![after](hall-3d-after.png)

## The walk bob (owner follow-up)

Two fixes on top of the port, both in `Unit.gd`:

- `_place()`: `bob` was `-absf(sin(...)) * 3.0`, correct in screen space where -y is up. In 3D that
  dug the sprite into the floor. Sign flipped, so the step lifts. Visible in the probe output —
  `spr=` y is positive now (`spr=(0.0, 5.354254, 0.0)`) where it used to be negative.
- `State.FIGHT`: a unit parked on the line with no foe in reach never set `_moving`, so the front
  rank stood dead still while the ranks behind it bobbed. It now takes `_moving = battle.advancing`
  after `_step`, exactly as `State.RANK` does — the whole company runs in place while the hall
  pushes forward, and stops when `advancing` goes false. Guarded on `_atk_t < 0.0` so a unit
  mid-attack-clip never bobs.

All measurements and self-checks below are from after these two fixes.

## Self-checks

    == probe battle,12,swap
    PROBE swap ok: veteran -> halberdier, 46 on field, pool[veteran]=110
    PROBE battle fps=466 process_ms=2.8 physics_ms=0.1 units=72 objects=317
    PROBE rank wd min=116.0 max=287.0 n=42  screen y of min=818.181823730469 fov=73.7 pitch=-3.2 height=200.0
    == probe battle,6,jump
    PROBE jump 1 -> Siege.tscn wave=0
    PROBE jump 2 -> Battle.tscn wave=0
    PROBE jump 3 -> Battle.tscn wave=1
    PROBE jump 4 -> Battle.tscn wave=2
    PROBE jump 5 -> Battle.tscn wave=3
    PROBE battle fps=349 process_ms=3.7 physics_ms=0.1 units=91 objects=394
    == probe battle,8,stress
    PROBE stress units=828
    PROBE battle fps=20 process_ms=51.6 physics_ms=0.0 units=852 objects=1977
    == probe battle,2,turn
    PROBE turn ok: right peaked at 160, back to x=0.0 pitch=-3.18
    PROBE turn ok: stairs peaked at 12, back to x=0.0 pitch=-3.18
    PROBE battle fps=314 process_ms=4.0 physics_ms=0.0 units=76 objects=445
    exit=0

No `SCRIPT ERROR` and no failed assertion in any of them. `turn` is new: nothing exercised
`Battle._turn` (the between-wave lens move) before, and it was the one ported piece with no
coverage, so it got a self-check in the same style as `swap`/`whirl`/`charge`.

Adversarial QA also passes clean, which is the real test of the new airborne-frame check:

    pwsh Scripts\godot-run.ps1 -Adversary battle,60,7
    ADVERSARY start scene=battle secs=60 seed=7
    ADVERSARY done findings=0 report=... (0 findings, 0 engine errors)

(The `docs/qa/adversary_7.*` files that run overwrote were restored with `git checkout` — they
belong to an earlier task's evidence.)

## Web export

    Godot_v4.7.2-stable_win64_console.exe --headless --path godot --export-release Web build/web/index.html
    exit=0, no errors
    index.wasm 39,514,754  index.pck 960,120  index.js 279,815  index.html 5,461

No Vulkan-only features, no compute, still `gl_compatibility`. `godot/project.godot` was **not
edited** — nothing about the 3D scene needed a rendering setting the project did not already have.

## What moved, what stayed

**Sprite3D (in the 3D world):** every unit and its 8-direction frames, the hero, the 8 gib chunks,
the `make_fx` clips (slash and the whirl vortex field). Units and gibs are
`ALPHA_CUT_DISCARD` + `alpha_scissor_threshold 0.35` — opaque, depth-writing, sorted per pixel.
The fx clips are `ALPHA_CUT_DISABLED` (blended): they are drawn at 20% alpha and a scissor would
erase them whole.

**2D overlay (`CanvasLayer`, unprojected with `camera.unproject_position`):** everything that is a
draw call — impacts, arrows and the volley arrow streaks, spell/ability flashes, motes, the hero
flame, the wall sconces, the far green glow and its flicker line. `fx.z_index = 5` and
`hud.z_index = 10` are unchanged; the Labels/`ArmyPanel` layer is untouched.

**Geometry:** floor = 17x7 shared-mesh quads on 14 shared unshaded materials, all under one node so
the treadmill is a single transform per frame plus one material reshuffle per 64 units of scroll
(the deterministic `_hash2` variant grid shifts with the tiles, so no tile changes texture under
you). One flat quad covers past 1216 depth, where fog is already solid. Walls = one `ArrayMesh`,
two quads per side, `uv1_offset.x` driven by `scroll / 64`; the top 40% of the last course fades to
black through vertex colour, so there is no ceiling edge. Fog is `Environment` depth fog,
black, 380..980 — the same range `View.fog` used.

**Sim:** untouched. `wx`/`wd` and every constant keep their meaning; a point is
`Vector3(wx, h, -wd)`. Heights, lunges and fx radii stay in *sprite pixels* (the unit every tuned
number in `Battle.gd`/`Unit.gd` was already in) and `Hall3D.PIXEL = 2.2` converts at render time —
so `GRAV`, `POP`, `launch()` impulses and the juggle feel are numerically identical.

**Camera:** `Vector3(0, 200, 0)`, fov `2*atan(270/360) = 73.7 deg`, `KEEP_HEIGHT`, pitch
`-atan((270 - horizon)/360)`. `_turn` tweens `camera.position:x` +/-160 and
`camera.rotation_degrees:x` between the 250 and 170 horizons, same durations and easing.

## Not carried over — read these

1. **The OutRun bend is gone.** `curve_a`, `curve_l`, `curve_dx`, `CURVE_A_BY_WAVE`,
   `CURVE_L_BY_WAVE` deleted, as the task specified. `SCROLL_CREEP_BY_WAVE` and the scroll
   acceleration are kept. The bend returns later on a `Path3D`.
2. **Corpse and gib fades cut out at 35% opacity** instead of reaching zero. `ALPHA_CUT_DISCARD`
   is a threshold, not a blend; the death tween still dims the sprite to 0.4 grey and it then pops
   out over the last third of the fade. Fixing it means putting dying sprites back into
   transparency sorting, which is the thing this task exists to remove, so it was left.
3. **Sconces and the far glow now draw over the world, not under it.** They were `draw_circle`
   calls on `Battle._draw()`, behind everything; on the overlay they are in front. A sconce glow
   can now sit on top of a unit standing against the wall. `fx`/`hud` were already on top, so only
   these two moved layer.
4. **`_recoil` scales with depth now.** It used to be a flat 6 screen px at any distance (it was
   divided by the node scale); it is now 6 sprite pixels, so a distant unit recoils less on screen.
   `_lunge` is unchanged in behaviour.
5. **Gib spray ignores the depth half of `push`.** Gibs fly in the camera plane, so only `push.x`
   steers them; the old code added the screen-y component of the push to their upward velocity,
   which has no meaning once y is a real world axis.
6. **The near field sits slightly higher on screen.** The old projection put the horizon at 250 by
   translating the whole image; a real pitched camera cannot do that, it rotates. Mid-field matches
   closely (front rank feet at y 468 vs 475), but the nearest rank at `wd = 88` lands at y 979
   instead of 1068 — both far below the 540-tall frame either way, so nothing visible changed. The
   camera was left at the task's starting values; nothing needed tuning by eye.
7. **Dead scenery code deleted.** `scenery`, `decor`, `_scenery_sprite` and `_build_scenery` were
   never populated (`_build_scenery` was `pass`), so the per-frame re-projection loop over
   `scenery` was iterating an always-empty array. Gone rather than ported.
8. **`Army.block(host, parent, ...)` now takes `Node`, not `Node2D`** (the `stress` probe passes
   the battle and its world, which are `Node3D` now). Nothing else in `Army.gd` changed, and
   `Siege.gd`, `View.gd`, `ArmyPanel.gd`, `godot/data/**` and `godot/assets/**` were not touched.
9. **`probe3d` is gone**: `godot/scenes/probe3d/`, `godot/scripts/Probe3D.gd`, the `Game.SCENES`
   entry, the `KEY_P` branch and the "P 3D" hint in `Jump.gd` and `Main.gd`.

## Files

- `godot/scripts/World3D.gd` — new, `class_name Hall3D`
- `godot/scripts/Battle.gd`, `godot/scripts/Unit.gd` — `Node3D`
- `godot/scenes/battle/Battle.tscn` — root retyped to `Node3D`
- `godot/scripts/Probe.gd` (camera-based rank print + the new `turn` check),
  `godot/scripts/Adversary.gd` (airborne check via `Hall3D.sprite_top_screen`),
  `godot/scripts/Army.gd`, `godot/scripts/Game.gd`, `godot/scripts/Jump.gd`,
  `godot/scripts/Main.gd`, `godot/README.md`
