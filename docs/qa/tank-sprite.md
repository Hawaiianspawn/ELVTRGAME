# task-185 tank sprite QA

`godot-run.ps1 -Probe "battle,8,tank"`:
    PROBE tank sprite ok: tank=tank hero.y=121.0 mount_h=55 saved=.../probe_tank_sprite.png

2 PixelLab generations spent (`create_character` v3 rotation of the owner's turtle_d_tall concept). Owner round 3: the hero was full-unit scale on a 0.55 tank, so `hero_sprite.scale *= TANK_SCALE` too (set once at creation, clip playback never touches `.scale`). `hero_wd` pushed 300 -> 335 (enemy hold line is FRONT_D+40=360, so the tank now sits just behind the fighting) so the whole dome clears the rank helmets, not just its top. `TANK_MOUNT_H` stays 55 (a function of `TANK_SCALE`, not depth or the rider's own scale). `separation()`'s hero shove widened from a flat 34 to `TANK_FOOTPRINT_R = 75` (the dome's on-screen radius at 0.55) so ranks stand clear of the tank body, not just the hero's own footprint. `_muzzle()`'s rise above the mount scales with it too (`20 * TANK_SCALE` ~= 11 sprite px). hero_turret still turns to face the cursor every frame (`Game.facing_from`) whenever no attack/hurt clip is playing.

Pre-existing `_gatling_hit_at` target-launch assertion in the same probe fails intermittently on unmodified `master` too (reproduced by stashing this task's changes and re-running) — unrelated to this task, left alone.
