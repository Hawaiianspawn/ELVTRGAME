# Hallway bend — OutRun-style curve, camera straight-on

Task-157: the battle hallway now sweeps left/right with depth while the camera stays fixed
behind the army, instead of running dead straight forever.

## Before

    PROBE battle fps=225 process_ms=6.0 physics_ms=0.0 units=67 objects=285

![before](hall-bend-before.png)

## After

    PROBE battle fps=209 process_ms=5.5 physics_ms=0.0 units=67 objects=294

![after](hall-bend-after.png)

209/225 = 93% of baseline — clears the 90% floor.

## What changed

`View.gd` gained `scroll` (Battle writes its `scroll` into it every frame), `curve_a` and
`curve_l`, and a new `curve_dx(d)` that is the closed-form integral of a sinusoidal curvature
`curve_a * sin((u + scroll) / curve_l)` over depth from `cam_d` to `d`:
`curve_dx(d) = curve_a * curve_l * (cos((cam_d + scroll) / curve_l) - cos((d + scroll) / curve_l))`.
It's camera-relative — always 0 at `d == cam_d` — so the near end of the hall never shifts and
everything bends away from the camera with distance. `project()` adds it to world `x` before
scaling by `s(d)`; `x_at()` subtracts it back out, so mouse/cursor mapping (`Battle._cursor_world`)
stays correct under the bend. `depth_at_y`, the sim's `wx`/`wd`, and `HALL_HALF` clamps are
untouched — the curve is purely a render-space shift, the lane/army simulation still runs on a
flat, straight hallway. Because `scroll` feeds the sine's phase, the bend also sweeps over time
as the field creeps forward, the classic OutRun "road turning under you" look, even though the
camera itself never moves or rotates. Sconces and the far-end glow already route through
`view.project`, so they picked up the bend for free — confirmed in the after screenshot (the
far glow and wall opening are visibly off-center rather than pinned to screen center).

Settled on `curve_a = 0.5`, `curve_l = 460.0`. At the far wall (`fog_end * 1.5 = 1470`) the
worst-case lateral shift is `2 * curve_a * curve_l ≈ 460` world units; combined with the wall's
own `±HALL_HALF = 200` and `s(far_d) ≈ 0.245`, that keeps the far wall's screen x inside roughly
[240, 720] of the 960-wide viewport — comfortably on-screen with margin, never clipped. A first
pass at `curve_a = 0.15` was safe but nearly invisible in play (most of the visible crowd sits at
`d` 90–290, close to `cam_d`, where the bend is still small); 0.5 is the smallest amplitude that
reads clearly as a curve at the depths the player actually looks at, while the far end (where the
bend is largest) stays in unremarkable fog anyway.

`rows` in `View.draw_ground` and `segs` in `Battle._draw_walls` went from 12 to 14 — enough
extra depth bands for the curve to look like a curve instead of a kinked polyline, without
tripping the FPS floor. 16, 24 and 36 were tried first and measured 194/225 (86%), 154/225 (68%)
and 119/225 (53%) — all fail the "≥90% of baseline" bar the task set, so backed off to 14.
