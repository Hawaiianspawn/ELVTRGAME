# Kindled: The Necromancer's Keep (Godot web build)

Godot 4.7 / GDScript. Browser target, single-threaded web export (no COOP/COEP headers needed).

    py Scripts/art/godot_pack.py                       # repack sprites from godot/data/roster.json
    godot --headless --path godot --export-release Web build/web/index.html
    py -m http.server -d build/web 8765                # open http://localhost:8765

Local run/relaunch (no export): `pwsh Scripts\godot-run.ps1` (kills our previous run, launches windowed;
`-Pack` repacks sprites first; `-Probe "battle,12;siege,6"` runs probes and prints the PROBE lines).
A PostToolUse hook in `.claude/settings.json` reruns it after every Edit/Write under `godot/`.
Adversarial QA: `pwsh Scripts\godot-run.ps1 -Adversary battle,60,7` (report + findings in `docs/qa/`).

Evidence probe (desktop): `godot --path godot -- --probe=battle,12` prints FPS and saves
`user://probe_battle.png`. Scenes: siege, battle. `--probe=battle,12,post=N` applies
post-process preset `N` first and saves `user://probe_battle_postN.png` (see
docs/art/post-process-presets.md). In the battle scene, `[` / `]` cycle the
post-process preset live; the HUD readout shows which one is active.

The battle is a real 3D scene: `Node3D` + depth-tested `Sprite3D` pixel billboards under a fixed
`Camera3D`, so the crowd sorts per pixel instead of y-sorting whole sprites. Hall geometry and the
sim->world/screen helpers live in `scripts/World3D.gd` (`class_name Hall3D` — `World3D` is an
engine class and can't be shadowed). Sim coordinates are unchanged: a point at (wx lateral, wd
depth) sits at `Vector3(wx, h, -wd)`. Siege is still 2D on the old pinhole projection.

Data: `data/units.json` (stats + counters), `data/waves.json`, `data/spells.json` (spells + relics),
Sprites: `assets/sprites/<name>.png` = 8 directions in atlas.py column
order, one strip per roster entry. Sound: `data/units.json` "sfx"/"hero_sfx" and `data/spells.json`
"sfx" name files under `assets/sfx/`, played by the `Sound` autoload (`scripts/Sound.gd`); see
docs/art/CHARACTER-PIPELINE.md#sound.
