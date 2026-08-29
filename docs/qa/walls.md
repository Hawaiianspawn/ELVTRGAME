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
| 1 | 520 | 0.15 | 600 |
| 2 | 400 | 0.45 | 420 |
| 3 | 280 | 0.9 | 320 |
| 4 | 200 (unchanged) | 1.6 | 240 |

## Probe self-check

`Probe.gd`'s extra comma args now compose: `wave=N` and `post=N` apply wherever they land in the
spec, whatever's left over is the self-check keyword — so `walls` and `wave=3` combine in one
`--probe=` spec instead of only one or the other.

```
PROBE walls ok: hall=1 half=520 units=72 curve_a=0.150 wall_sep_px=1132
PROBE walls ok: hall=4 half=200 units=57 curve_a=1.600 wall_sep_px=436
```

Self-check (`-Probe "battle,8,walls[,wave=3]"`) asserts: `Battle.HALL_HALF` matches
`HALF_BY_WAVE[Game.wave]`; every live unit and floor prop has `absf(wx) < half` (wall lamps are
excluded — they sit flush on the wall face at `wx = ±half` by design, not inside it); and after 3s
`Hall3D.curve_a` is within 0.01 of `CURVE_A_BY_WAVE[Game.wave]` (the 2s tween-in has finished).
`wall_sep_px` is the on-screen gap between the two walls at `FRONT_D` (320), computed with
`Hall3D.unproject` at the same camera the screenshot renders with: 1132px for hall 1 (wider than
the ~960px viewport — the walls run off-frame near the front line, which reads correctly in
`walls-hall1.png` as an open, wall-less foreground), 436px for hall 4 (both walls comfortably
on-screen, matching today's look in `walls-hall4.png`).

## Screenshots

- `docs/qa/walls-hall1.png` — hall 1, half=520, curve_a settled at 0.15 (near-straight).
- `docs/qa/walls-hall4.png` — hall 4, half=200, curve_a settled at 1.6. Caught at a moment of
  strong lateral phase (the bend oscillates continuously with `scroll`, not a one-time skew), so
  one wall swings mostly off-frame in this single freeze-frame — expected at this amplitude, not a
  bug; the effect settles back through zero every cycle during actual play.

## Fog

Left `FOG_START`/`FOG_END` unchanged. Fog triggers by depth (`wd`), not lateral width — widening
`HALL_HALF` doesn't push the walls (which run the same `WALL_D0..WALL_D1` depth range regardless
of half) further into the fog band. `walls-hall1.png` shows both walls reading clearly at 520
half with no extra black falloff versus hall 4.
