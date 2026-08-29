# task-185 tank sprite QA

`godot-run.ps1 -Probe "battle,12,tank"`:
    PROBE tank sprite ok: tank=tank hero.y=220.0 mount_h=100 saved=.../probe_tank_sprite.png

2 PixelLab generations spent (`create_character` v3 rotation of the owner's turtle_d_tall concept). Mount height `TANK_MOUNT_H = 100` sprite px, tuned by eye against `north.png` so the hero's feet land on the dome's peak, just below the small vent nub, clear of the body's rounded flank.

Pre-existing `_gatling_hit_at` target-launch assertion in the same probe fails intermittently on unmodified `master` too (reproduced by stashing this task's changes and re-running) — unrelated to this task, left alone.
