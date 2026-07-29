---
id: 025
title: Spec the procgen room-graph generator over the prefab library
status: proposed
agent: gameplay-director
owns: ["docs/design/procgen-room-graph.md", "docs/data/room-types.json", "docs/data/room-types.schema.md"]
resources: []
depends-on: [4, 24]
evidence: A generator spec plus a room-type data file, with a worked example floor showing arena sizing that fits the encounter budget.
score: {feel: 2, risk: 2, cost: 3}
source: docs/RTS-VERTICAL-SLICE.md:102
decided: ""
---

## Why now
Lower priority than it looks, and the score says so. `docs/RTS-VERTICAL-SLICE.md:102`
wants a small room-graph generator over a prefab library — but the prefab library does not
exist yet, the encounter budget it consumes is task-004, and the floor structure it
generates into is task-024. Building the generator first means building it against three
unknowns.

Worth filing now because GDD §9's procgen constraints are already written and the
gameplay half is explicitly the gameplay-director's: arena sizing for horde fights,
encounter-budget spend per room type, decision-event and risk-room placement.

## Done when
- The constraint rules the generator consumes, in the form it will read them.
- Arena sizing rules for horde fights — a room too small for 700 units is a bug the
  generator should be incapable of producing.
- Encounter-budget spend per room type, referencing task-004's table rather than
  restating it.
- Decision-event and risk-room placement rules.
- A worked example floor showing the rules producing something legal.
- Honest about the prefab library's absence: state what the generator assumes about
  prefabs and flag it as a dependency, do not invent a library.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the procgen room-graph generator per docs/RTS-VERTICAL-SLICE.md:102 and GDD.md §9.
You own the GAMEPLAY half: arena sizing for horde fights, encounter-budget spend per room
type, decision-event and risk-room placement rules.

Read: GDD.md §9, docs/design/encounter-budget.md (task-004 — dependency; reference its
table, do not restate the numbers), docs/design/run-structure.md (task-024 — dependency),
docs/perf/BUDGETS.md for the real entity ceiling, and docs/design/CAMERA-SCALE.md for the
gameplay zoom that determines what "big enough arena" means.
Do NOT read WORLD.md — superseded by docs/narrative/FLAME-FOUNDATION.md.

Be honest about a real gap: the prefab library this generates over DOES NOT EXIST yet.
State what your rules assume about prefabs and flag it as a dependency. Do not invent a
library and spec against your own invention.

Hard rule worth designing in: a room too small to hold the wave it is budgeted for is a
bug the generator should be structurally incapable of producing. Make arena sizing a
constraint, not a guideline.

Write ONLY docs/design/procgen-room-graph.md, docs/data/room-types.json, and
docs/data/room-types.schema.md. Do not edit SYSTEMS.md, GDD.md, ELVTR/Source, or
ELVTR/Content. Include a worked example floor.
```
