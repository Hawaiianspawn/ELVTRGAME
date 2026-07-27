---
id: 038
title: Fold the settled design specs into SYSTEMS.md as decision records
status: proposed
agent: gameplay-director
owns: ["SYSTEMS.md"]
resources: []
depends-on: [2, 3, 4, 5, 6]
evidence: SYSTEMS.md carrying one dated decision entry per landed spec, each pointing at its spec and data file, with no numbers duplicated.
score: {gate: 2, risk: 1, cost: 1}
source: docs/GDD-TODO.md:85
decided: ""
---

## Why now
This task exists to make tasks 002–006 approvable **in parallel**, which they otherwise
would not be.

`docs/AGENT-TEAMS.md` §3 is blunt about it: two teammates editing the same file overwrite
each other, and only one teammate touches `SYSTEMS.md`. If each of the five design specs
also wrote its own `SYSTEMS.md` entry, they would collide, and `backlog.py approve` would
correctly refuse to let more than one of them be active at a time.

Splitting the write out follows the gameplay-director's own stated contract — specs go to
`docs/design/`, numbers to `docs/data/`, and *"SYSTEMS.md is the index, not the
encyclopedia."* So the five specs own their own files, and exactly one task owns
`SYSTEMS.md`.

## Done when
- One entry per landed spec: the decision, the rationale, the date, and a pointer to the
  spec and data file.
- No numbers restated. If a value appears both here and in `docs/data/`, it will drift —
  the entry cites, it does not copy.
- Sections whose spec has not landed left honestly marked as not yet designed rather than
  quietly filled with the plan's intent.
- The five specs themselves untouched. This task reads them and indexes them.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Fold the settled design specs into SYSTEMS.md as decision records. You are the ONLY task
that writes SYSTEMS.md — that is why this task exists separately from the specs themselves
(docs/AGENT-TEAMS.md §3: two teammates editing one file overwrite each other).

Read whichever of these have landed:
  docs/design/entity-tiers.md            (task-002)
  docs/design/scaling-curve.md           (task-003)
  docs/design/encounter-budget.md        (task-004)
  docs/design/retinue-tuning-vanguard.md (task-005)
  docs/design/loot-v0.md                 (task-006)
plus their docs/data/*.json siblings, and SYSTEMS.md as it stands.

For each landed spec write ONE entry: the decision, the rationale, the date, and a pointer
to the spec and its data file.

Do NOT restate numbers. Your own contract says SYSTEMS.md is the index, not the
encyclopedia — a value that lives in two places will drift. Cite, do not copy.

If a spec has not landed, leave its section honestly marked "not yet designed". Do not
fill it in with what you assume the spec will say.

Write ONLY SYSTEMS.md. Do not edit the design specs, the data files, GDD.md, or CLASSES.md.
```
