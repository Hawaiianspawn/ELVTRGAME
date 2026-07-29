---
id: 005
title: Tune the Vanguard retinue — growth, attrition, per-floor cap
status: proposed
agent: gameplay-director
owns: ["docs/design/retinue-tuning-vanguard.md", "docs/data/retinue-vanguard.json", "docs/data/retinue-vanguard.schema.md"]
resources: []
depends-on: [2]
evidence: Simulation showing whether a Vanguard's retinue grows, holds, or starves per floor at the spec'd replenishment and death rates.
score: {feel: 3, risk: 2, cost: 2}
source: docs/GDD-TODO.md:89
decided: ""
---

## Why now
Retinue growth *is* the progression axis — GDD Q15 settled that the roguelike run is
primary and retinue growth is how the player advances. The numbers behind the game's
main progression loop do not exist. Gate 1 shipped with a hardcoded 120-unit refill
(`docs/GATE1-FUN-PROTOTYPE.md`), which is a placeholder, not a tuning pass.

## Done when
- Growth rate, attrition rate, replenishment, and per-floor cap for the Vanguard.
- Soft-cost pressure only — no hard numeric caps (design law 2). Ties to the
  degrade-not-die upkeep economy settled in GDD Q16.
- Attrition is designed to be *felt and mourned* (design law 9), with the tuning stated
  in those terms, not just as a decay constant.
- `## Simulation notes` answering plainly: at these rates, does the retinue grow, hold,
  or starve across three floors?

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Tune the Vanguard retinue per SYSTEMS.md §6 and docs/RTS-VERTICAL-SLICE.md:74 — growth
rate, attrition, replenishment, per-floor cap.

Read first: CLASSES.md (Vanguard identity and growth verbs), GDD.md §7 (Q16 settled a
degrade-not-die upkeep economy fed by supply sites) and §4 (stances),
docs/design/entity-tiers.md (task-002 — this depends on it), and
docs/GATE1-FUN-PROTOTYPE.md for the shipped placeholder (120-unit refill between waves).
Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY:
  docs/design/retinue-tuning-vanguard.md
  docs/data/retinue-vanguard.json
  docs/data/retinue-vanguard.schema.md
Do not edit CLASSES.md, SYSTEMS.md, GDD.md, ELVTR/Source, or ELVTR/Content. Vanguard
identity changes are canon proposals, not edits — put them in `## Canon proposals`.

Soft caps only: answer player power with upkeep cost and screen chaos, never a hard
number. Simulate attrition across three floors and state plainly whether the retinue
grows, holds, or starves. End with `## Simulation notes`.
```
