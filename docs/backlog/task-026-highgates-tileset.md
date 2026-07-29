---
id: 026
title: Produce the Highgates tileset
status: proposed
agent: pixel-art-director
owns: ["docs/art/highgates-tileset.md"]
resources: []
depends-on: [15, 39]
evidence: A tileset spec that holds the locked value register, with tiles readable under a 700-unit swarm at gameplay zoom.
score: {feel: 2, risk: 2, cost: 3}
source: docs/RTS-VERTICAL-SLICE.md:109
decided: ""
---

## Why now
`docs/RTS-VERTICAL-SLICE.md:109` marks it explicitly **"(post art-test)"** — the slice
plan itself says this waits on the flipbooks-vs-3D decision. It also waits on the palette
strategy, since a tileset is the largest area of screen and therefore the biggest
consumer of whatever value budget the resolution of task-016 leaves for the floor.

Filed now so it is visible as blocked rather than forgotten, which is exactly what
happened to the three pending briefs.

## Done when
- Tile inventory for Highgates: floor, wall, edge, and the props the biome needs.
- Every tile holds the locked register, with the floor deliberately ceding value range to
  units — the ground must not compete with the things standing on it.
- Readability answer at gameplay zoom **under a 700-unit swarm**, which is the density
  Gate 1 already reaches. A tileset that reads on an empty floor is not evidence.
- Dither rules consistent with the unit specs (no block smaller than 2×2), and explicitly
  reconciled with the known unit dither artifact — two-half-box unit shading dropping the
  dim back-half into the dither band is a real, observed problem, and the floor sits
  directly behind every unit.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the Highgates tileset per docs/RTS-VERTICAL-SLICE.md:109. That line marks it
"(post art-test)" — it depends on task-015 (flipbooks vs 3D) and task-016 (palette
strategy). Read both outputs first; if either is unresolved, say so and state your
assumption rather than deciding it yourself.

Read: docs/art/aesthetic-direction.md (Direction A LOCKED: strict 4-value Demichrome),
docs/data/art/palette.json, docs/art/palette-exceptions.md, docs/design/CAMERA-SCALE.md
for the gameplay zoom, WORLD-superseding canon in docs/narrative/FLAME-FOUNDATION.md for
what Highgates IS, and docs/art/retinue-militia.md for the unit dither rules you must not
contradict.
Do NOT read WORLD.md — superseded.

Design principle worth stating explicitly in the spec: the floor is the largest area of
screen and must CEDE value range to the units standing on it. A tileset that competes with
units for contrast fails at horde density regardless of how it looks alone.

Give a readability answer at gameplay zoom UNDER A 700-UNIT SWARM — that is the density
Gate 1 already reaches. Judging on an empty floor is not evidence.

Reconcile with the known unit dither artifact: two-half-box unit shading drops the dim
back-half into the dither band, and the floor sits directly behind every unit.

Write ONLY docs/art/highgates-tileset.md. Written specs only — no image files, no
mcp__pixellab__* calls (this task holds no credits lock).
```
