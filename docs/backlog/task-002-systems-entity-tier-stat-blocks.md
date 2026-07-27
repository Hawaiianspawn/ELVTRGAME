---
id: 002
title: Spec the entity tier stat blocks (fodder → soldier → elite → titan → boss)
status: proposed
agent: gameplay-director
owns: ["docs/design/entity-tiers.md", "docs/data/entity-tiers.json", "docs/data/entity-tiers.schema.md"]
resources: []
depends-on: []
evidence: docs/data/entity-tiers.json imports cleanly as a UE DataTable, plus the spec's Simulation notes section showing TTK per tier at the slice's density.
score: {gate: 3, risk: 2, cost: 2}
source: docs/GDD-TODO.md:86
decided: ""
---

## Why now
`SYSTEMS.md` §1 is a skeleton, and it is the first domino: the scaling curve, the
encounter budget, and retinue tuning all read tier stat blocks. `docs/RTS-VERTICAL-SLICE.md:70`
lists the same item as a slice design prerequisite, so this is one piece of work two
docs are both waiting on. Until it exists, every difficulty number in the slice is a
guess.

## Done when
- A tier taxonomy with HP, damage, speed, and behavior archetype per tier, respecting
  design law 3 (scale by more enemies, not spongier ones) and law 5 (only elite/titan/boss
  promote to full Actors).
- Time-to-kill bands per tier stated in seconds, at the hero DPS Gate 1 actually shipped
  (55 DPS, `docs/GATE1-FUN-PROTOTYPE.md:384`).
- Numbers live in `docs/data/entity-tiers.json` with a sibling schema doc; the spec
  references them rather than restating them.
- A `## Simulation notes` section — the gameplay-director contract requires it, or an
  explicit "Not simulated" and why.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the entity tier stat blocks called for by SYSTEMS.md §1 and
docs/RTS-VERTICAL-SLICE.md:70 — fodder, soldier, elite, titan, boss.

Read first: GDD.md (§7 power scaling, §10 entity architecture), SYSTEMS.md,
docs/RTS-VERTICAL-SLICE.md, docs/GATE1-FUN-PROTOTYPE.md (the shipped combat model:
discrete swing cadence, 55 hero DPS, per-attacker blow cap), and ELVTR/Source Mass
combat headers for what the sim can actually express.
Do NOT read WORLD.md — it is superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY these files:
  docs/design/entity-tiers.md
  docs/data/entity-tiers.json
  docs/data/entity-tiers.schema.md
Do not edit SYSTEMS.md, GDD.md, CLASSES.md, ELVTR/Source, or ELVTR/Content. If your
work implies a canon change, end with a `## Canon proposals` section instead.

Simulate before you spec: TTK per tier at the shipped hero DPS and at retinue sizes
50/120/250. End with `## Simulation notes`. If you did not simulate, say so and why.
```
