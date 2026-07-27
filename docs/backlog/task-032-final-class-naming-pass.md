---
id: 032
title: Final naming pass on the four classes (GDD Q11 / CLASSES C1)
status: proposed
agent: narrative-director
owns: ["docs/narrative/class-naming-pass.md"]
resources: []
depends-on: []
evidence: A candidate set per class with the reasoning, presented for the owner to choose from — not a decision made on the owner's behalf.
score: {gate: 1, risk: 1, cost: 1}
source: GDD.md:437
decided: ""
---

## Why now
`GDD.md` §12 Q11 records the class names as working names, status "Naming pass later".
Vanguard / Relickeeper / Pathfinder / Lampbearer are placeholders that have quietly become
load-bearing — they appear in `CLASSES.md`, every art spec, the brief titles, and the
narrative docs.

The longer they stay, the more expensive the rename gets. That is the actual argument for
doing it soon, and it is a modest one, hence the low score.

One constraint that already exists and must be respected: classes are identified **by role
only**. The named-hero approach was reversed because attrition makes names hard to
remember. Companions keep proper names; the four player classes do not. A "naming pass"
here means better role-nouns, not christening people.

## Done when
- Candidates per class with reasoning, drawn from current canon rather than invented fresh.
- The role-only constraint respected — no personal names for the four classes.
- Each candidate checked against the flame canon: this is a world where you carry the only
  light and bearers are treated as gods, and the names should sit inside that.
- Presented as a choice for the owner. This task does not rename anything, and does not
  edit `GDD.md` or `CLASSES.md`.

## Spawn prompt
```
You are the narrative-director for Emberkeep (C:\Projects\ELVTRGAME).

GDD.md §12 Q11 (line ~437) marks the four class names as working names pending a naming
pass: Vanguard, Relickeeper, Pathfinder, Lampbearer.

Read: CLASSES.md, docs/narrative/FLAME-FOUNDATION.md (the current canon — you bear the only
flame in a pitch-dark world, your army needs your light, bearers are treated as gods,
uniting flames is the goal), and the class art specs in docs/art/.
Do NOT read WORLD.md — it is SUPERSEDED by FLAME-FOUNDATION.md.

HARD CONSTRAINT: classes are identified BY ROLE ONLY. The fixed-named-hero approach was
reversed by the owner because attrition makes names hard to remember. Companions keep
proper names; the four player classes do not. A naming pass here means better role-nouns,
not christening characters.

Propose candidates per class with your reasoning, drawn from current canon rather than
invented from scratch. Check each against the flame canon — these names should sit inside
a world of light against dark, not beside it.

Write ONLY docs/narrative/class-naming-pass.md. Do NOT edit GDD.md, CLASSES.md, or any art
spec — renaming touches a dozen files and is the owner's call to trigger. Present a choice,
do not make it.
```
