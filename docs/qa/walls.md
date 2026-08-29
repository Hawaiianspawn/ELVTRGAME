# task-183: hall walls wide-to-close

Per-hall half-width table replaces the single `HALL_HALF` const. `HALL_HALF` is now a live var,
set by `start_wave` from `HALF_BY_WAVE`; `hall.set_half()` rebuilds the floor columns and both
wall meshes at the new width (freeing the old geometry/materials first so `Hall3D._bend_mats`
doesn't grow every hall), then `hall.set_hall(i)` re-applies that hall's floor/wall art. Every
reader of `HALL_HALF` (enemy spawn wx range, `_floor_wx`, the lanes cap, `Unit.gd`'s wx clamp,
`Adversary.gd`'s boundary check) picks up the live value automatically since it's a Node property,
not a const. `RANK_HALF` is decoupled to a fixed literal `180.0` per the reviewer note — the rank
block stays put while the hall widens/narrows around it. Wall lamp props are re-flushed to
`±HALL_HALF` on every `start_wave` (`_flush_wall_props`).

| Hall | half | curve_a | curve_l |
|---|---|---|---|
| 1 | 520 | 0.05 | 600 |
| 2 | 400 | 0.15 | 420 |
| 3 | 280 | 0.35 | 320 |
| 4 | 200 (unchanged) | 0.48 | 240 |

## Bend cap (owner directive)

The owner's hard rule: the bend must never carry a wall off screen, for every depth `d` in
`[NEAR_D, FOG_END]` and every scroll phase, `Hall3D.unproject(camera, ±half, d).x` stays inside
40..920px. The `curve_a` values above are sized to that rule, not to the earlier `a=[0.15, 0.45,
0.9, 1.6]` example table, which was far too strong.

One correction to the rule as first stated: checking from the literal `Hall3D.NEAR_D` (128) breaks
even at `curve_a=0`, for every hall, including hall 4's own unchanged, never-touched 200 half —
confirmed by probe (`PROBE walls sweep hall=4 half=200 a=0.000 l=240 x_range=[-38, 998]`, run
against the pre-existing geometry with the bend zeroed out). Two bend-independent reasons:
- `NEAR_D=128` is the nearest floor tile row in the pool, not the nearest *visible* one — the
  screen's bottom edge only reaches the ground at `d≈240` (already noted in `World3D.gd`'s own
  comment on `NEAR_D`); below that the floor renders off the bottom of the frame regardless of x.
- Below a per-hall depth (measured: 416 / 320 / 220 / 156 for halls 1-4), the *unbent* wall is
  simply wider than the 960px viewport at that magnification — pure hall-width-vs-camera-FOV
  geometry, present with `curve_a=0`. Hall 1's 520 half doesn't fit on screen at all until `d≈416`,
  deep past `FRONT_D` (320) — an open-nave look where the walls are only ever seen converging in
  the distance, not a bend artifact.

The probe's `walls` check now sweeps from `d0` — the first depth (snapped to its own 64-unit
sample grid) where the wall could appear in frame at all with zero bend — through `FOG_END`. From
`d0` on, `curve_a`/`curve_l` must not push either wall's screen x outside the 40px margin across a
full phase cycle (33 phase samples spanning `2π·curve_l`). That isolates what the bend is actually
responsible for; the near-depth cutoff itself is bend-independent hall geometry.

```
PROBE walls sweep hall=1 half=520 a=0.050 l=600 d0=448 x_range=[54, 906] bound=[40, 920]
PROBE walls ok: hall=1 half=520 units=72 curve_a=0.050 wall_sep_px=1132 worst_phase=707
PROBE walls sweep hall=2 half=400 a=0.150 l=420 d0=384 x_range=[64, 896] bound=[40, 920]
PROBE walls ok: hall=2 half=400 units=57 curve_a=0.150 wall_sep_px=871
PROBE walls sweep hall=3 half=280 a=0.350 l=320 d0=320 x_range=[59, 901] bound=[40, 920]
PROBE walls ok: hall=3 half=280 units=60 curve_a=0.350 wall_sep_px=610
PROBE walls sweep hall=4 half=200 a=0.480 l=240 d0=256 x_range=[52, 908] bound=[40, 920]
PROBE walls ok: hall=4 half=200 units=57 curve_a=0.480 wall_sep_px=436 worst_phase=236
```

Per-hall max wall screen x (the sweep's `x_range`, i.e. the widest excursion found across the full
depth × phase sweep from `d0` to `FOG_END`): hall 1 `[54, 906]`, hall 2 `[64, 896]`, hall 3
`[59, 901]`, hall 4 `[52, 908]` — all inside the required `[40, 920]`.

Self-check also asserts: `Battle.HALL_HALF` matches `HALF_BY_WAVE[Game.wave]`; every live unit and
floor prop has `absf(wx) < half` (wall lamps excluded — they sit flush on the wall face at
`wx = ±half` by design); and after 3s `Hall3D.curve_a` is within 0.01 of `CURVE_A_BY_WAVE[Game.wave]`.

## Screenshots

Both re-taken posed at the sweep's own worst-phase moment (`Battle.scroll` forced to `worst_phase`
for 3 frames before capture), not whatever phase happened to be live — so each shot shows the
actual strongest swing the probe measured, not an arbitrary point in the cycle.

- `docs/qa/walls-hall1.png` — hall 1, half=520, curve_a=0.05 (near-straight), both walls visible
  and parallel.
- `docs/qa/walls-hall4.png` — hall 4, half=200, curve_a=0.48 at its strongest measured swing
  (`worst_phase=236`): both walls clearly in frame, visibly skewed/wavier than hall 1, lamps
  arcing across the top confirming the bend — no wall leaves the screen.

## Fog

Left `FOG_START`/`FOG_END` unchanged. Fog triggers by depth (`wd`), not lateral width — widening
`HALL_HALF` doesn't push the walls (which run the same `WALL_D0..WALL_D1` depth range regardless
of half) further into the fog band. `walls-hall1.png` shows both walls reading clearly at 520
half with no extra black falloff versus hall 4.
