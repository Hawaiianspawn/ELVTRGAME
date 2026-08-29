# task-185 tank sprite QA

`godot-run.ps1 -Probe "battle,8,tank"`:
    PROBE tank sprite ok: tank=tank hero.y=121.0 mount_h=55 saved=.../probe_tank_sprite.png

2 PixelLab generations spent (`create_character` v3 rotation of the owner's turtle_d_tall concept). Owner round 3: the hero was full-unit scale on a 0.55 tank, so `hero_sprite.scale *= TANK_SCALE` too (set once at creation, clip playback never touches `.scale`). `separation()`'s hero shove widened from a flat 34 to `TANK_FOOTPRINT_R = 75` (the dome's on-screen radius at 0.55) so ranks stand clear of the tank body. `_muzzle()`'s rise above the mount scales too (`20 * TANK_SCALE` ~= 11 sprite px). hero_turret still turns to face the cursor every frame (`Game.facing_from`) whenever no attack/hurt clip is playing.

Owner round 4, two corrections: "back" meant toward the camera, not deeper down the hall -- `hero_wd` scrapped from the round-3 335 back to 245 (the original depth; at TANK_SCALE=0.55 it now fits inside frame with no clipping, unlike the old full-size dome). And the muzzle flash sprite is gone from both weapons (`_burst(_muzzle(), ...)` deleted from `_gatling_hit_at` and `_fire_cannon`) -- kept: the `_impact` star/X on a gatling hit, the small burst on a floor miss, the tracer line, the cannon blast. `TANK_MOUNT_H` stays 55 (scale-relative, not depth-relative, so the round-3 retune already covers this).

Pre-existing `_gatling_hit_at` target-launch assertion in the same probe fails intermittently on unmodified `master` too (reproduced by stashing this task's changes and re-running) — unrelated to this task, left alone.
