---
id: 021
title: Build Mass squad-as-entity aggregation (renders as N sprites)
status: proposed
agent: performance-director
owns: ["docs/perf/squad-aggregation.md"]
resources: []
depends-on: [7]
evidence: A design doc with the measured cost of the current per-entity path at slice density, and the projected saving, before any code is written.
score: {gate: 2, risk: 3, cost: 3}
source: docs/RTS-VERTICAL-SLICE.md:95
decided: ""
---

## Why now
`docs/RTS-VERTICAL-SLICE.md:95` lists aggregation — one squad entity rendering as N
sprites — as a slice build item. It is the main lever for pushing entity counts past what
the per-entity path sustains, and it is also the one with the widest blast radius: it
changes the render bridge, the spatial grid's contents, and what combat can address.

Depends on task-007 because the honest answer might be "not needed". If Spike 1 says the
per-entity path holds at target density, aggregation is complexity bought for nothing.
Building it before measuring is exactly the mistake the spike exists to prevent.

This task is **design and measurement only** — the implementation is a follow-up the
owner approves separately, after seeing what it costs and what it buys.

## Done when
- Current per-entity render + sim cost at slice density, measured, not estimated.
- The aggregation design: what a squad entity holds, how it expands to N sprites, what
  happens when a squad takes partial casualties, and how combat addresses members.
- Projected saving with the assumptions stated.
- An honest recommendation, including "don't build this" if Spike 1 says the simple path
  holds. Design law 5 cuts both ways: cheap shared archetypes are the point, and an
  aggregation layer that reintroduces per-unit special-casing is a broken design.

## Spawn prompt
```
You are the performance-director for Emberkeep (C:\Projects\ELVTRGAME).

Design (do not implement) Mass squad-as-entity aggregation, per
docs/RTS-VERTICAL-SLICE.md:95 — one squad entity that renders as N sprites.

Read docs/SPIKE1-RESULTS.md FIRST (task-007 fills it; this task depends on it). If the
per-entity path already holds at target density, the correct recommendation may be "do not
build this", and you should say so plainly — aggregation bought without need is pure
complexity.

Then read: GDD.md §10 (entity architecture — only elites/titans/bosses promote to full
Actors), docs/perf/BUDGETS.md, docs/perf/niagara-sprite-refactor.md (NOTE: §2 and §8.1 are
stale in the body; the emitter zero-draw cause was GPUComputeSim vs CPUSim and is fixed),
and the Mass processor sources under ELVTR/Source.

Measure the current per-entity render + sim cost at slice density — measured, not
estimated. Then design: what a squad entity holds, how it expands to N sprites, what
happens on partial casualties, how combat addresses individual members.

Write ONLY docs/perf/squad-aggregation.md. Do NOT edit ELVTR/Source or ELVTR/Content —
implementation is a separate task the owner approves after reading this. Do not launch or
drive the Unreal editor without saying so; you hold no editor lock on this task.

State your assumptions. If you did not measure something, say you did not measure it.
```
