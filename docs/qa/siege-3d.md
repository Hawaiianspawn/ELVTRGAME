# Siege intro in real 3D (task-161)

Ported `Siege.gd` from the hand-rolled 2D pinhole projection (`View.gd`) to a real `Node3D` scene,
reusing `Hall3D` (`godot/scripts/World3D.gd`) and Battle's lens the same way task-158 ported
Battle — see `docs/qa/hall-3d.md` for that scene's port and why the class is `Hall3D`, not
`World3D`.

## What changed

- `Siege.gd` `extends Node3D`. World units 1:1 with the sim: a sprite at `(wx, wd)` sits at
  `Vector3(wx, 0, -wd)`.
- Camera: same lens as Battle, reused via `preload("res://scripts/Battle.gd")`'s `CAM_H`,
  `FOCAL`, `HALF_H`, `HORIZON`, `pitch_for()` rather than duplicated by eye. The fly-in tweens
  `camera.position:z` from `0` to `-1350` over `FLY` (3s) seconds, same sine ease as the old
  `cam_d` tween. Sprites no longer need the `wd - cam_d > 40` visibility hack — the camera's near
  plane handles it, and nothing needs a per-frame position/scale/fog update any more (real 3D
  perspective and `Environment` fog do that automatically every frame).
- Army: 90 `Sprite3D` via `Hall3D.make_sprite3d`, facing north (index 4), random wx/wd as today,
  placed once in `_ready` (no `_process` upkeep needed).
- Ground: `Hall3D._build_floor(600.0)` (the same floor builder Battle's hall uses), no walls — an
  open battlefield, not a hall. The old sky gradient became a `WorldEnvironment` with
  `background_color = #1a221c` and the same depth-fog range (`Hall3D.FOG_START`/`FOG_END`,
  380..980) the old `View.fog` lerp used.
- Facade: a `Sprite3D` (`pixel_size` scaled) standing at depth `GATE_D = 2000`,
  `FACADE_WORLD_W x FACADE_WORLD_H` world units, `facade.png`, nearest filter, alpha discard. The
  arch (`ARCH_PX` in facade pixels) maps to a rectangle on that quad; the `hall_glow.png` quad
  sits 4 units further back (alpha `0.35 + 0.55 * _gate` is exactly what the old draw did for
  alpha; the 3D version drives `modulate.a` via `_update_gate()` — wait, alpha is fixed 0.35, see
  note below); the two door `Sprite3D`s sit 4 units nearer the camera and squash toward their
  hinge (the arch's outer edges) as `_gate` goes 0..1, reproducing the old
  `draw_texture_rect`-into-a-shrinking-rect stretch with a fixed-height `pixel_size` plus a
  per-frame `scale.x` and reposition.
- Green smoke puffs: kept as a 2D `CanvasLayer` overlay (`smoke_layer`, parity with the old
  `_draw`), unprojected each frame via `Hall3D.unproject`/`screen_scale` so it tracks the facade's
  screen position as the lens flies in.
- Caption `CanvasLayer` + Label unchanged. `_run()` changes only its tween target (camera
  `position:z` instead of `view.cam_d`); the gate tween, `_unhandled_input`, `Game.goto('battle')`
  unchanged.
- Deleted `godot/scripts/View.gd` and `godot/scripts/View.gd.uid`. `git grep --untracked -nP
  '\bView\b' -- godot` is clean (one comment in `Siege.gd` was reworded to not contain the bare
  word after the first pass flagged it).

`_glow.modulate.a` is driven from `_update_gate()` each frame as `0.35 + 0.55 * _gate`, matching
the old draw's brightening as the doors swing open (caught and fixed while writing this doc — the
first pass set it once at construction and left it static).

## Before / after PROBE (`pwsh Scripts/godot-run.ps1 -Probe "siege,6"`)

This is the probe the task literally named. Both runs land on `Battle`, not `Siege`, by the 6s
mark (`Siege._run` auto-advances to Battle at ~5.1s: 3s fly + 1.6s gate + 0.5s wait) — **these two
lines measure the Battle wave-1 frame after the siege-to-battle handoff, not the siege scene
itself.**

    before (2D View.gd, pre-port), measures Battle after the siege->battle handoff:
      PROBE siege fps=272 process_ms=4.8 physics_ms=0.0 units=48 objects=269

    after (3D), measures Battle after the siege->battle handoff:
      PROBE siege fps=1467 process_ms=13.8 physics_ms=0.0 units=48 objects=269

No ratio is drawn from these two: `process_ms=13.8` at `fps=1467` is internally inconsistent
(13.8ms of process time caps a frame well under 1467fps), which means this is a transition-frame
artefact — the fade/scene-swap `change_scene_to_file` triggers mid-sample — not a clean steady-
state measurement. Treat both lines as "the task-specified probe ran clean, no errors, same
`units`/`objects` both sides" evidence, not as a performance comparison.

The actual siege-scene-itself measurement is below.

## `siege,4` PROBE — the siege scene itself, after the port

    PROBE siege fps=1422 process_ms=1.2 physics_ms=0.0 units=0 objects=437

Self-consistent (1.2ms process time comfortably clears a 1422fps frame budget). No matching
same-methodology "before" number exists — the 2D build's `siege,6` probe also landed past the
handoff onto Battle (same issue as above, just true for the old build too), so there is no clean
before/after ratio to report for the siege scene specifically. What this number does say: at
1.2ms of process time for 90 static `Sprite3D`s, a static floor and one facade quad, the siege
scene is nowhere near a frame-budget problem either before or after the port.

![before](siege-3d-before.png)
![after](siege-3d-after.png)

`siege-3d-before.png` is Battle wave 1 (2D build, per the transition-timing note above).
`siege-3d-after.png` is `siege,4` on the 3D build — the facade, arch and doors mid-swing with the
green glow spilling through, confirmed by eye. It does **not** show the besieging ranks: by
design, `_run()` finishes the 3s camera fly (which passes almost the entire army, seeded at
`wd` 120..1500 against a camera that stops at depth 1350) *before* the gate starts opening, so by
the time the gate is visibly ajar only the ~10% of the army still ahead of the camera (`wd` >
1350, and only within the near-field's narrower FOV cone) could be on screen at all — this run's
RNG seed didn't happen to put any of them in frame. This is the flipside of the near-plane
visibility fix noted above (sprites correctly drop out once the camera passes them) rather than a
bug: the old 2D pinhole never physically passed a sprite, so every "before" screenshot could show
the whole depth range at once; the 3D fly-through can't do both at the same instant, because the
gate only starts opening once the fly is over. A separate ad hoc `siege,2.5` pull (not saved,
reproducible with `godot-run.ps1 -Probe "siege,2.5"`) does show the facade and several besieging
sprites together, mid-flight, with the gate still shut — confirming the army itself renders
correctly, just not at the same instant as the open gate.

## `siege,6,jump` self-check

    == probe siege,6,jump
    PROBE jump 1 -> Siege.tscn wave=0
    PROBE jump 2 -> Battle.tscn wave=0
    PROBE jump 3 -> Battle.tscn wave=1
    PROBE jump 4 -> Battle.tscn wave=2
    PROBE jump 5 -> Battle.tscn wave=3
    PROBE siege fps=372 process_ms=3.6 physics_ms=0.0 units=90 objects=400
    PROBE rank wd min=115.0 max=287.0 n=42  screen y of min=822.687194824219 fov=73.7 pitch=-3.2 height=200.0
    PROBE unit wd=286.939758300781 pos=(83.3102, 0.0, -286.9398) spr=(-0.0, 6.444544, -0.0) screen=(580.7812, 492.3151)
    PROBE unit wd=115.084060668945 pos=(-46.39436, 0.0, -115.0841) spr=(0.0, 3.061319, 0.0) screen=(347.4456, 822.3058)
    PROBE saved ...

No `SCRIPT ERROR` in any run (`siege,6`, `siege,6,jump`, `siege,4`, and the `siege,2`/`siege,2.5`/
`siege,3.2`/`siege,3.5` ad hoc visual pulls used to find the before/after screenshots above). All
5 jumper phases (siege + 4 hall waves) load clean.

## `View` grep

    git grep --untracked -nP '\bView\b' -- godot
    (no output)

## Web export

    Godot_v4.7.2-stable_win64_console.exe --headless --path godot --export-release Web <repo>/build/web/index.html
    EXIT=0
    index.wasm 39,514,754  index.pck 958,272  index.js 279,815  index.html 5,461

No Vulkan-only features, no compute, still `gl_compatibility`; `godot/project.godot` untouched.
(First attempt with a project-relative `build/web/index.html` failed with "Target folder does not
exist" — the export target path resolves relative to the Godot project root passed to `--path`,
not the invoking shell's cwd. An absolute path fixed it; not a code issue.)

## Files touched

- `godot/scripts/Siege.gd` — ported to `Node3D`
- `godot/scenes/siege/Siege.tscn` — root retyped to `Node3D`
- `godot/scripts/View.gd`, `godot/scripts/View.gd.uid` — deleted
- `docs/qa/siege-3d.md` (this file), `docs/qa/siege-3d-before.png`, `docs/qa/siege-3d-after.png`

Not touched: `Battle.gd`, `Unit.gd`, `World3D.gd`, `Game.gd`, `Battle.tscn`, `Probe.gd`,
`Adversary.gd`, `ArmyPanel.gd`, `godot/data/**`, `godot/assets/**`, `Scripts/**`, `ELVTR/**`.
