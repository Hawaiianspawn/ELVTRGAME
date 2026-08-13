# task-059 evidence — read this before judging the screenshots

The four PNGs here are real PIE frames at gameplay density (120 retinue + 250 brood, wave 1).
**You will not be able to see the nine ooze looks in them, and that is the finding, not a
framing mistake.** The ooze body measures RGB 0-35 per channel, the floor of L_Spike1 renders
black, and `M_Swarm` is Unlit — so the brood is black-on-black at every camera distance. What
fills the frame is the retinue, which is pale grey-green.

Three visibility knobs were tried and none of them lift a black sprite, because none of them
is additive:

| knob | result |
|---|---|
| `Swarm.DebugPlainView 1` (post-process off) | floor still black, brood still invisible |
| `Swarm.UnitStencil 0` (drop the units' exemption from the demichrome lift) | no change to the brood |
| `Swarm.FlameRadius 3200` / `CoreRadius 2600` / `Intensity 1.6` / `Falloff 1` | floor still black |

`User.Colors` cannot fix it either: it is a MULTIPLIER, so it can dim a brood or replace it
outright for the hit flash, but it cannot lift RGB 0-35 out of the dark.

Suppressing the retinue for a brood-only shot is also not available: `Swarm.Clear` trips the
game mode's wave-cleared path, which immediately spawns `+120 reinforcements`.

## So the decode is proved by what the sprite renderer was HANDED, per frame

`Swarm.DebugShotAfter` now logs the distinct atlas rows in the same frame as the capture.
Row = Variant*2 + WalkFrame for the brood (0-17); the retinue is 18-19.

| capture | weights | rows the renderer received |
|---|---|---|
| `task059-brood-variants-skewed-crown.png` | `0,0,0,0,0,0,100,0,0` | **12 13** 18 19 — variant 6's pair ONLY |
| `task059-brood-variants-default-mix.png` | `14,40,12,8,10,4,2,6,4` | **0 1 2 3 4 5 6 7 8 9 10 11 13 14 15 16 17** 18 19 — all nine variants at once |

Both frames carry 370 cells (250 brood + 120 retinue), so the brood is drawing. Row 12 is
absent from the mixed frame only because variant 6 is at weight 2 (~3 of 250 bodies) and those
few happened to be on walk frame 1.

Paired histograms from the same frames:

```
250 brood | weights "0,0,0,0,0,0,100,0,0"  | v6=250 (100.0%), all others 0
250 brood | weights "14,40,12,8,10,4,2,6,4" | v0=42 (16.8%) v1=101 (40.4%) v2=30 (12.0%)
                                              v3=28 (11.2%) v4=26 (10.4%) v5=4 (1.6%)
                                              v6=3 (1.2%) v7=13 (5.2%) v8=3 (1.2%)
```

The retinue drawing correctly is itself part of the proof: it lives on rows 18-19 of a 20-row
sheet, so a wrong `Sub UV` split or a wrong `Rows` constant would garble it, and it does not.

## The two size captures DO show their subject

`task059-sizejitter-off.png` vs `task059-sizejitter-on.png` — `Swarm.RetinueSizeJitter` 0 vs
0.5. Same `User.Sizes` array and same `Uniform Sprite Size` binding the brood uses, shot on the
retinue because the retinue is visible.

## What would make the brood legible

Lighter brood art, or an additive light term on the sprite path. Both are fenced out of
task-059 and neither was attempted here. See docs/perf/niagara-sprite-path.md section 8.
