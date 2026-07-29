---
id: 035
title: Design the S8 "Silent Bell" world-flag effects
status: proposed
agent: gameplay-director
owns: ["docs/design/world-flag-s8.md"]
resources: []
depends-on: []
evidence: Encounter-table effects for the S8 flag that change what the player meets without leaking power.
score: {feel: 1, risk: 1, cost: 1}
source: docs/GDD-TODO.md:114
decided: ""
---

## Why now
**It is not now — recommend parking, with a caveat worth recording.**

The flag comes from `WORLD.md` §7, whose effects were marked "design later". WORLD.md is
**superseded** by the 2026-07-22 narrative reset. So the honest state is not "this design
is pending" but "we do not currently know whether S8 still exists as canon".

Designing effects for a flag that may have been dissolved by the reset is wasted work, and
worse, it would quietly re-import superseded canon through the back door — which is exactly
the failure task-018 exists to stop.

The caveat: if the 15-flag system did survive the reset in some form, this is small and
cheap. Resolving *that* question is a prerequisite, and it belongs to whoever reconciles
the world-flag system against flame canon — not here.

## Done when
Nothing, until S8's canon status is settled. If it survived:
- Effects that change *what you encounter* — elite seeds, safe rooms, vendors.
- No power inheritance (design law 8): flags never leak starting stats. The encounter
  tables read flags; the flags do not touch numbers.

## Recommended verdict
`py Scripts/backlog.py park 35 -r "S8 originates in WORLD.md §7, superseded by the
2026-07-22 narrative reset. Blocked on whether the 15-flag system survived the reset at
all."`
