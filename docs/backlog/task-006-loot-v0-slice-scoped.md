---
id: 006
title: Design loot v0 — unit orbs, healing, 4-6 stacking items
status: proposed
agent: gameplay-director
owns: ["docs/design/loot-v0.md", "docs/data/loot-v0.json", "docs/data/loot-v0.schema.md"]
resources: []
depends-on: [2]
evidence: Monte-Carlo of the drop table showing whether a median run sees an evolution by floor 3.
score: {gate: 2, risk: 2, cost: 2}
source: docs/GDD-TODO.md:90
decided: ""
---

## Why now
The slice needs *something* droppable to close its reward loop, and GDD Q7 deferred the
full loot system to the gameplay-director by design. This is explicitly the small
version — `docs/RTS-VERTICAL-SLICE.md:75` scopes it as "not the real loot system". The
full system is task-033 and stays parked.

Scope discipline matters here: the gameplay-director's own brief warns that loot's first
design should land as one spec plus a SYSTEMS entry the owner reviews, not a fait
accompli across ten files.

## Done when
- Unit orbs, healing, and four to six stacking items — no more.
- Run-scoped only, no persistent gear (design law 7). Drops feed both hero and retinue;
  the banner-for-all-soldiers vs. weapon-for-hero split is the twist worth preserving.
- Drop sources and weights in `docs/data/loot-v0.json`.
- `## Simulation notes` with the Monte-Carlo result: does a median run see an evolution
  by floor 3?
- Explicitly *not* the full itemization system. If the spec starts growing rarity tiers
  and evolution trees, it has left scope.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Design loot v0 for the vertical slice, per SYSTEMS.md and docs/RTS-VERTICAL-SLICE.md:75.
Scope is deliberately small: unit orbs, healing, and 4-6 stacking items. NOT the full
itemization system — that stays parked as a separate backlog task.

Read first: GDD.md §8 (loot direction) and §12 Q7 (deferred to you by design, with the
note that retinue growth is the real reward loop), docs/design/entity-tiers.md
(task-002 — this depends on it), CLASSES.md.
Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY:
  docs/design/loot-v0.md
  docs/data/loot-v0.json
  docs/data/loot-v0.schema.md
Do not edit SYSTEMS.md, GDD.md, ELVTR/Source, or ELVTR/Content.

Design law: loot is run-scoped, no persistent gear. Drops feed hero AND retinue.
Vampire Survivors-style stacking is the reference frame.

Monte-Carlo the table before publishing weights: does the median run see an evolution by
floor 3? End with `## Simulation notes`.

If you find yourself designing rarity tiers or evolution trees, stop — that is out of
scope and belongs to the parked full-loot task.
```
