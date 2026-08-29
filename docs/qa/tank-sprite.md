# task-185 tank sprite QA

`godot-run.ps1 -Probe "battle,12,tank"`:
    PROBE tank sprite ok: tank=tank hero.y=121.0 mount_h=55 saved=.../probe_tank_sprite.png

2 PixelLab generations spent (`create_character` v3 rotation of the owner's turtle_d_tall concept). Owner sent the first pass back: the dome hid the crowd at native size. `TANK_SCALE = 0.55` shrinks the tank Sprite3D (`tank_sprite.scale *= TANK_SCALE`); `TANK_MOUNT_H` retuned proportionally from 100 to 55 sprite px so the hero's feet still land on the dome's (now smaller) peak.

Pre-existing `_gatling_hit_at` target-launch assertion in the same probe fails intermittently on unmodified `master` too (reproduced by stashing this task's changes and re-running) — unrelated to this task, left alone.
