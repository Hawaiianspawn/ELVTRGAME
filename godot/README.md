# Kindled: The Necromancer's Keep (Godot web build)

Godot 4.7 / GDScript. Browser target, single-threaded web export (no COOP/COEP headers needed).

    py Scripts/art/godot_pack.py                       # repack sprites from godot/data/roster.json
    godot --headless --path godot --export-release Web build/web/index.html
    py -m http.server -d build/web 8765                # open http://localhost:8765

Local run/relaunch (no export): `pwsh Scripts\godot-run.ps1` (kills our previous run, launches windowed;
`-Pack` repacks sprites first; `-Probe "battle,12;siege,6"` runs probes and prints the PROBE lines).
A PostToolUse hook in `.claude/settings.json` reruns it after every Edit/Write under `godot/`.

Evidence probe (desktop): `godot --path godot -- --probe=battle,12` prints FPS and saves
`user://probe_battle.png`. Scenes: siege, battle, probe3d.

Data: `data/units.json` (stats + counters), `data/waves.json`, `data/spells.json` (spells + relics),
Sprites: `assets/sprites/<name>.png` = 8 directions in atlas.py column
order, one strip per roster entry.
