---
id: 003
title: Spec one scaling curve across the slice's three floors
status: proposed
agent: gameplay-director
owns: ["docs/design/scaling-curve.md", "docs/data/scaling-curve.json", "docs/data/scaling-curve.schema.md"]
resources: []
depends-on: [2]
evidence: A plotted curve across floors 1-3 × party sizes 1-4 in the spec's Simulation notes, showing where TTK spikes or collapses.
score: {gate: 3, risk: 2, cost: 2}
source: docs/GDD-TODO.md:87
decided: ""
---

## Why now
This is the first act of the exponential power fantasy — GDD design law 1 says late-run
must trivialize early-run by orders of magnitude, with breakpoints rather than smooth
growth. Right now that promise has no numbers behind it anywhere. `docs/RTS-VERTICAL-SLICE.md:71`
wants the same curve. It needs task-002's tier stat blocks to have anything to scale.

## Done when
- One curve covering floors 1–3 and party sizes 1–4, expressed as multipliers over the
  tier baselines rather than a second set of absolute numbers.
- Co-op scaling adds bodies and elite seasoning, never health multipliers (design law 3).
- Named breakpoints — the moments the player should *feel* a spike.
- `## Simulation notes` showing the curve tabulated, with the collapse/spike points called out.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the scaling curve called for by SYSTEMS.md §2 and docs/RTS-VERTICAL-SLICE.md:71 —
one curve across the vertical slice's three floors, for party sizes 1 through 4.

Read first: GDD.md §7 (power scaling), the entity tier spec at docs/design/entity-tiers.md
and docs/data/entity-tiers.json (task-002 output — this task depends on it), SYSTEMS.md,
and docs/GATE1-FUN-PROTOTYPE.md for the wave structure already shipped.
Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY:
  docs/design/scaling-curve.md
  docs/data/scaling-curve.json
  docs/data/scaling-curve.schema.md
Do not edit SYSTEMS.md, GDD.md, CLASSES.md, docs/data/entity-tiers.json, ELVTR/Source,
or ELVTR/Content.

Design law that binds you: exponential feel via layered multipliers, soft caps only,
scale by more enemies not spongier ones, co-op adds bodies not HP. Express the curve as
multipliers over the tier baselines — do not restate absolute stats.

Simulate: tabulate floors 1-3 × party 1-4, find where TTK collapses or spikes. End with
`## Simulation notes`.
```
