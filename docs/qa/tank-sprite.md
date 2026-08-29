# task-185 tank sprite QA

`godot-run.ps1 -Probe "battle,8,tank"`:
    PROBE tank sprite ok: tank=tank hero.y=121.0 mount_h=55 saved=.../probe_tank_sprite.png
    PROBE tank ok: siege removed, gatling/cannon/chest all landed

2 PixelLab generations spent (`create_character` v3 rotation of the owner's turtle_d_tall concept). Owner round 2: pushed `hero_wd` 245 -> 300 (between the rank block's RANK_D0=285 and FRONT_D=320) so the tank sits further into the frame; scale stays `TANK_SCALE = 0.55`, still reads a good size at the new depth so no nudge needed. `TANK_MOUNT_H` unchanged at 55 (scale-relative, not depth-relative). No rank/dome overlap observed at the new depth, so `separation()`'s hero shove radius was left alone. hero_turret now turns to face the cursor every frame (`Game.facing_from`, same convention Unit.gd uses for its own targets) whenever no attack/hurt clip is playing.

Pre-existing `_gatling_hit_at` target-launch assertion in the same probe fails intermittently on unmodified `master` too (reproduced by stashing this task's changes and re-running) — unrelated to this task, left alone.
