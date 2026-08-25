# Hallway warp ramp — per-wave OutRun sweep

Task-160: the necromancer speeds up the hallway sweep as the halls get harder. Two
dials now ramp with `Game.wave` (0..3, four halls), set in `Battle.start_wave()`:

| Wave | scroll_creep (u/s) | curve_a | curve_l | far-wall worst-case screen x* |
|------|---------------------|---------|---------|-------------------------------|
| 1    | 84                  | 0.50    | 460     | 642                           |
| 2    | 112                 | 0.65    | 380     | 650                           |
| 3    | 140                 | 0.85    | 320     | 662                           |
| 4    | 170                 | 1.10    | 270     | 674                           |

\* `480 + (HALL_HALF + 2*curve_a*curve_l) * s(fog_end*1.5)`, per `docs/qa/hall-bend.md`'s
derivation — the far wall stays inside the 960-wide viewport at every wave, with more
margin than the original 0.5/460 baseline had.

`curve_a`/`curve_l` tween to their wave's target over 2s (`Tween.TRANS_SINE`,
`EASE_IN_OUT`) so the bend hardens smoothly instead of snapping; `curve_dx(d)` is a
continuous function of both, and is always 0 at `d == cam_d`, so the near end of the
hall never jumps mid-tween.

## A found conflict: CREEP is not purely cosmetic

The task brief said "`scroll` drives scenery only... enemies move on their own speed,
so creep changes nothing in the sim." That's true of `scroll` itself, but
`Unit.gd:279` reads `battle.CREEP` directly to drag enemy `wd` toward the camera every
frame (`wd = maxf(wd - battle.CREEP * delta, battle.ENEMY_MIN_D)`) — that's sim state,
not scenery. Renaming/ramping `CREEP` in place would have changed enemy closing speed,
which is out of scope ("don't touch sim, unit speeds, Unit.gd").

Fix: split the two uses. `CREEP := 42.0` stays exactly as it was, feeding only
`Unit.gd`'s treadmill — untouched. A new `SCROLL_CREEP_BY_WAVE` array feeds only
`Battle.scroll` (which drives `View.scroll` — row lines, sconces, floor tile step,
curve phase). `Unit.gd` was not edited.

## Evidence

    PROBE battle fps=208 process_ms=5.3 physics_ms=0.0 units=66 objects=308

![wave 1](hall-warp-wave1.png)

    PROBE battle fps=168 process_ms=6.6 physics_ms=0.0 units=108 objects=396

![wave 4](hall-warp-wave4.png)

## fps floor note

The task set "wave 4 fps must be >= 90% of 209" (209 is `hall-bend.md`'s wave-1
baseline). Measured wave 4 comes in at 168 (80%), under that bar. Checked whether
this is a regression from this task: reverted `Battle.gd` and re-ran the same
`--probe=battle,12,jump` wave-4 capture against the pre-existing code — fps=171,
units=106. That's within run-to-run noise of the 168 measured with the warp ramp in
(unit count varies run to run because `spawn_queue.shuffle()` is unseeded). Wave 4
was already ~76-82% of the wave-1 baseline before this task touched anything, because
wave 4 has more lanes and reserves by design (`data/waves.json`), not because of the
creep/curve ramp. Flagging this rather than picking a wave-1-shaped floor to hide it.
