---
id: 024
title: Extend the run structure from waves to three floors plus a boss
status: proposed
agent: gameplay-director
owns: ["docs/design/run-structure.md"]
resources: []
depends-on: [3, 4]
evidence: A run-structure spec covering floor transitions, boss gating, and victory/death screens, reconciled against what Gate 1 already ships.
score: {gate: 2, risk: 2, cost: 2}
source: docs/RTS-VERTICAL-SLICE.md:101
decided: ""
---

## Why now
Partly built already, which is why it needs a spec rather than a build ticket. Gate 1
ships a working *wave* structure — 3s deploy → 250 → breather → 450 → breather → 700 →
win, hero death loses. `docs/RTS-VERTICAL-SLICE.md:101` wants start → **3 floors** → boss
→ victory/death screen. Those are different shapes, and the existing one is not a
subset of the target.

Filing it as a design task rather than "implement floors" because the interesting
questions are unanswered: what a floor transition *is* to the player, whether the retinue
persists across floors and at what cost, and how the boss is gated. Those are tuning
decisions that depend on tasks 003 and 004.

## Done when
- Floor transition defined from the player's side — what they see, what carries over,
  what it costs.
- Retinue persistence across floors reconciled with the attrition and upkeep model
  (task-005's domain; this must not contradict it).
- Boss gating: what admits the player to the boss, and what happens on a wipe.
- Victory and death screens specified as states, with what each shows.
- Explicitly reconciled against `docs/GATE1-FUN-PROTOTYPE.md` — say which shipped
  behaviour survives, which is replaced, and which was a placeholder all along.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the full run structure per docs/RTS-VERTICAL-SLICE.md:101 — start → 3 floors → boss →
victory/death screen.

Important: a wave structure ALREADY SHIPS. docs/GATE1-FUN-PROTOTYPE.md documents it: 3s
deploy → wave 1 (250 brood) → breather (6s, retinue refills to 120) → wave 2 (450) →
breather → wave 3 (700) → win; hero death loses. That is a different shape from three
floors plus a boss, and it is not a subset of the target. Read it first and reconcile
explicitly: what survives, what is replaced, what was always a placeholder.

Then read: docs/design/scaling-curve.md and docs/design/encounter-budget.md (tasks 003/004
— this depends on both), GDD.md §3 (core loop) and §9 (procgen), CLASSES.md.
Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

The interesting questions, which are the actual deliverable: what is a floor transition
from the PLAYER's side? Does the retinue persist across floors, and at what cost? What
gates the boss, and what happens on a wipe?

Retinue persistence must not contradict the attrition/upkeep model — that belongs to
task-005 (docs/design/retinue-tuning-vanguard.md). If it is not written yet, state your
assumption explicitly rather than deciding it here.

Write ONLY docs/design/run-structure.md. Do not edit SYSTEMS.md, GDD.md, ELVTR/Source, or
ELVTR/Content.
```
