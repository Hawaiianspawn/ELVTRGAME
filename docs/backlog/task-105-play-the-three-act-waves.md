---
id: 105
title: Wire per-wave retinue and composition into Spike1GameMode, and play the three-act curve
status: parked
agent: claude
model: ""
owns:
  - "ELVTR/Source/ELVTR/Spike/**"
  - "docs/design/wave-scaling-three-act-pie.md"
resources: [unreal-editor]
depends-on: []
epic: three-act-waves
evidence: >
  A runnable build in which the three waves differ by retinue size, retinue
  composition and enemy composition, plus a PIE capture per wave and a written
  answer to the one question the sim cannot reach: does early / mid / late read
  as three different fights, and does the late wave hold frame rate at its
  specced population.
score: {feel: 3, risk: 3, cost: 3}
source: task-103 handback, 2026-07-30
teammate: ""
decided: "2026-07-30 parked"
---

## Why now

`docs/design/wave-scaling-three-act.md` and the three fixtures under
`docs/data/scenarios/` are the whole paper answer, and task-103 established that the
paper answer has run out of road: the wave-attrition model predicts a full retinue
wipe on **all three** waves, including wave 1 at ratio 2.00 where GATE1 *measured*
92% survival at the nearly identical ratio 2.08. That is `LIMITATIONS.md` §1 doing
exactly what it says it does. No further simulation will tell us whether this curve
is any good.

Meanwhile the shipped prototype still cannot express the curve at all.
`Spike1GameMode.h:47` holds `WaveBroodCounts = {250, 450, 700}` against a single flat
`StartingRetinue = 120` / `RetinueCap = 120`. There is no per-wave retinue count, no
retinue composition, and no enemy composition — the three things that make early,
mid and late different from each other.

## Done when

- `Spike1GameMode` takes a per-wave descriptor rather than a bare `TArray<int32>`:
  retinue count, retinue composition (Spearmen/Archer split), enemy count, enemy
  composition by tier. The existing 250/450/700 behaviour must remain reachable as a
  config, not be deleted.
- The three waves from `docs/data/wave-scaling.json` are playable end to end.
- A PIE capture per wave, and a measured frame time on the late wave at its specced
  population — the perf claim in task-102 is arithmetic against a bench, not a
  measurement of this scenario.

## Open questions this is expected to answer

- Does mid actually read as "unique units", or does the Archer/Soldier-ranged reveal
  disappear into the mass?
- Does the late wave hold 60fps at 20,600 entities *with* Elites and Boss
  promoted-Actors on top? Task-102 explicitly flagged their per-actor cost as
  unmeasured and did not assume it free.
- Is the ×3.33 → ×50 population jump between mid and late a good curve or a cliff?

## Spawn prompt

```
<draft at dispatch — the three PIE questions above are the deliverable, and the
owner should confirm the arena/procgen sizing question before this starts, since a
20,000-population Arena may not fit the current test level at all>
```
