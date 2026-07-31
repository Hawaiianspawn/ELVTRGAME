---
id: 005
title: Tune the Vanguard retinue — growth, attrition, per-floor cap
status: done
agent: gameplay-director
model: sonnet
owns: ["docs/design/retinue-tuning-vanguard.md", "docs/data/retinue-vanguard.json", "docs/data/retinue-vanguard.schema.md"]
resources: []
depends-on: [2]
evidence: "A tuning spec plus schema-valid retinue-vanguard.json, carrying a three-floor headcount ledger worked as explicit arithmetic off docs/data/encounter-budget.json and entity-tiers.json — losses per floor, replenishment per floor, running headcount, per row — so the grow/hold/starve answer is auditable line by line rather than asserted. Not a wave-attrition harness run; per docs/sim/LIMITATIONS.md §1 those survivor counts do not reproduce GATE1's measured 110-of-120, so a harness figure is a scaffold with that caveat attached, never the bar."
score: {feel: 3, risk: 2, cost: 2}
source: docs/GDD-TODO.md:89
decided: "2026-07-29 done"
teammate: retinue-tuning
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
- A `## Three-floor ledger` table — per floor: expected losses, replenishment earned,
  running headcount — with every number traced to a cell in `docs/data/encounter-budget.json`
  or `docs/data/entity-tiers.json`. The grow/hold/starve answer falls out of the table.
- Where the arithmetic needs an input nobody has measured (stance effects, knockback,
  multi-wave carryover — `docs/sim/LIMITATIONS.md` §4 models none of them), the assumption
  is named in the row rather than buried in a constant.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Tune the Vanguard retinue per SYSTEMS.md §6 and docs/RTS-VERTICAL-SLICE.md:74 — growth
rate, attrition, replenishment, per-floor cap.

Read first: CLASSES.md (Vanguard identity and growth verbs), GDD.md §7 (Q16 settled a
degrade-not-die upkeep economy fed by supply sites) and §4 (stances),
docs/design/entity-tiers.md + docs/data/entity-tiers.json (task-002 — this depends on it),
docs/design/encounter-budget.md + docs/data/encounter-budget.json (task-004, landed — this is
where your per-floor loss numbers come from), docs/design/run-structure.md (the three-floor
shape), and docs/GATE1-FUN-PROTOTYPE.md for the shipped placeholder (120-unit refill between
waves). Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY:
  docs/design/retinue-tuning-vanguard.md
  docs/data/retinue-vanguard.json
  docs/data/retinue-vanguard.schema.md
Do not edit CLASSES.md, SYSTEMS.md, GDD.md, ELVTR/Source, or ELVTR/Content. Vanguard
identity changes are canon proposals, not edits — put them in `## Canon proposals`.

Soft caps only: answer player power with upkeep cost and screen chaos, never a hard
number.

EVIDENCE BAR — read this carefully, it is the part most likely to go wrong. End with a
`## Three-floor ledger` table: one row per floor, columns for expected losses,
replenishment earned, and running headcount, and EVERY number traced to a named cell in
docs/data/encounter-budget.json or docs/data/entity-tiers.json. Show the arithmetic. The
grow/hold/starve verdict is whatever the last column says — do not assert it separately.

Do NOT run Scripts/sim/ wave-attrition scenarios and present the survivor count as your
answer. docs/sim/LIMITATIONS.md §1 is explicit that the wave model does not reproduce the
one measured baseline it is checked against (GATE1's ~110-of-120 — it predicts a full wipe
at committed defaults). A harness run is a scaffold you may cite WITH that caveat attached,
never the evidence. §4 also models no stances, knockback, positioning or multi-wave
carryover: where your arithmetic needs one of those, name the assumption in the row instead
of hiding it in a constant.

The tree is shared with concurrent sessions. Build on uncommitted work you find, do not
revert it, do not attribute it. HANDBACK to the lead when done; do not change the task's
status yourself.
```
