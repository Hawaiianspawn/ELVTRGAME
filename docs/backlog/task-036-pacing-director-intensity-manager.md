---
id: 036
title: Design the runtime pacing director (L4D-style intensity manager)
status: proposed
agent: gameplay-director
owns: ["docs/design/pacing-director.md"]
resources: []
depends-on: [4, 24]
evidence: A pacing spec reading party state and world flags to drive spawn pressure, breathers, and ambushes.
score: {gate: 1, risk: 2, cost: 3}
source: docs/GDD-TODO.md:115
decided: ""
---

## Why now
**Recommend parking — but this one is closer to ready than the others in this group, so
it is worth a second look rather than an automatic park.**

`SYSTEMS.md` §5 calls for it and it is squarely in the gameplay-director's scope. The
argument for parking is sequencing, not merit: a pacing director is a system that
*modulates* an encounter budget and a floor structure. Neither exists yet (tasks 004 and
024). Built first, it would be a controller with nothing to control, and its parameters
would be invented rather than derived.

There is also a cheap partial already shipping: Gate 1's fixed wave/breather rhythm is a
hardcoded pacing curve. That may be enough for the slice, and finding out is task-008's
playtest, not a design project.

## Done when
Nothing yet. When tasks 004 and 024 land, and if the fixed rhythm proves insufficient:
- Spawn pressure, breather, and ambush rules.
- How it reads party state and world flags.
- Explicitly subordinate to the encounter budget rather than a second authority over
  density — two systems both deciding how many enemies exist is a bug factory.

## Recommended verdict
`py Scripts/backlog.py park 36 -r "Modulates an encounter budget (task-004) and floor
structure (task-024) that don't exist yet; Gate 1's fixed wave rhythm may suffice for the
slice. Revisit after task-008 says whether pacing actually feels flat."`
