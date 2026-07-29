---
id: 015
title: Settle GDD Q5 — flipbooks vs. flat-shaded 3D — with an actual art test
status: parked
agent: pixel-art-director
owns: ["docs/art/flipbook-vs-3d-test.md"]
resources: []
depends-on: []
evidence: A side-by-side comparison at gameplay zoom and horde density, with a recommendation and the cost/risk of each path stated.
score: {feel: 3, risk: 3, cost: 2}
source: GDD.md:429
decided: "2026-07-28 parked"
---

## Why now
`GDD.md` §12 Q5 is marked **"Needs art test"** and has been since 2026-07-09. Its lean is
flipbooks on instanced quads. `GDD-TODO.md:95` states the consequence plainly: art-test
decisions **gate all sprite production**. Every sprite task on this board is downstream
of an answer nobody has produced.

It also entangles a live thread: `docs/art/protagonist-prototypes.md` treats "may be
rigged, may become a 3D model" as an open option, and `docs/art/flame-bearer-status.md` §4
lists "is a 3D-rigged protagonist near-term or a someday option?" as one of three
consolidated blockers. Same fork, asked twice in two places.

## Done when
- A real comparison, not a written argument: the same unit rendered both ways at gameplay
  zoom and at horde density.
- Judged against the locked Demichrome register — a path that cannot hold four values at
  density fails regardless of how good it looks in isolation.
- Cost and risk of each path stated, including what each does to the Mass render bridge.
- One recommendation. The owner decides; this task produces the evidence to decide on.
- Written up in `docs/art/flipbook-vs-3d-test.md` with a `## Canon proposals` section
  proposing the GDD §12 Q5 edit — this task does not edit `GDD.md` itself.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

GDD.md §12 Q5 (line ~429) is "Needs art test" and has been open since 2026-07-09:
sprite flipbooks vs. flat-shaded 3D for the 2-bit look. Current lean is flipbooks on
instanced quads. This decision gates ALL sprite production (docs/GDD-TODO.md:95), so it
is worth real evidence rather than another written argument.

Read: GDD.md §10 (entity architecture) and §12, docs/art/aesthetic-direction.md
(Direction A LOCKED: strict global 4-value Demichrome), docs/art/protagonist-prototypes.md
and docs/art/flame-bearer-status.md §4 (both treat a 3D-rigged protagonist as an open
option — same fork asked twice), docs/perf/niagara-sprite-refactor.md for what the sprite
render path actually costs, and docs/design/CAMERA-SCALE.md for the gameplay zoom you
must judge at.

Note: docs/perf/niagara-sprite-refactor.md §2 and §8.1 are STALE — the sprite emitter's
zero-draw cause was GPUComputeSim vs CPUSim and is already fixed. Read the corrections at
the top of that file.

Produce a real comparison at gameplay zoom AND at horde density, judged against the
locked 4-value register. State cost and risk of each path, including the effect on the
Mass render bridge. Give ONE recommendation.

Write ONLY docs/art/flipbook-vs-3d-test.md. Do NOT edit GDD.md — put the proposed §12 Q5
resolution in a `## Canon proposals` section. Written specs only; no mcp__pixellab__*
generation calls without the owner approving a credits spend.
```
