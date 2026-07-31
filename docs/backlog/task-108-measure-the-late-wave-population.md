---
id: 108
title: Measure frame time at the three-act populations, and price the Elite/Boss actors
status: proposed
agent: performance-director
model: ""
owns:
  - "docs/perf/three-act-scale.md"
resources: [unreal-editor]
depends-on: []
epic: wave-measurement
evidence: >
  `docs/perf/three-act-scale.md` reporting measured frame/game/draw/GPU time at the
  three populations `docs/data/wave-scaling.json` specifies (notably 20,600), from a
  `-SwarmBench` run, plus a measured per-actor cost for the Elite/Boss
  PromotedActors at that population — replacing task-102's arithmetic headroom claim
  with a measurement, and stating plainly whether the late wave holds 60fps.
score: {feel: 1, risk: 3, cost: 1}
source: user, 2026-07-30 — the scale half of "an effective way to test them"
teammate: ""
decided: ""
---

## Why now

Task-102 specced the late wave at 20,000 brood + 600 retinue and argued **39.4%
headroom** against the ~34,000-entity ceiling in `docs/perf/one-camera-bench.md`.
That is arithmetic against a bench run at a different configuration, not a
measurement of this scenario. Two things in it are explicitly unverified, and
task-102 flagged both rather than assuming them:

1. **Elite/Boss PromotedActors are not counted against the Mass-entity budget.**
   Their per-actor cost is unmeasured. At floor 3 that is 3 Elites + 1 Boss on top
   of 20,600 entities.
2. **The whole claim depends on `Swarm.SimLOD.Stride 4` remaining the shipped
   default.** Against the no-LOD ceiling (~21,000, `one-camera-bench.md:248`) the
   same design has **1.9%** headroom, not 39.4%. That is the difference between a
   comfortable target and a design that falls over if one CVar changes.

Blood is a third unknown: `blood-particles.md` measured it near saturation at 820
entities and argues `Blood.MaxBurstsPerFrame` caps it architecturally — untested at
20,600.

**This does not depend on task-105.** `-SwarmBench` deliberately takes the field
away from the wave logic (`Spike1GameMode.cpp:187-196` — two independent population
schedulers make every run incomparable), so it spawns its own populations and can be
pointed at 20,600 today.

## Done when

`docs/perf/three-act-scale.md` reports, measured:

- Frame/game/draw/GPU time and fps at each of the three-act populations, including
  20,600, via `-SwarmBench` with counts set in `Saved/SwarmBenchConfigs.txt`.
- The measured cost of adding the Elite/Boss PromotedActors at the late population.
- A plain verdict: does the late wave hold 60fps, and how much of the 39.4% claim
  survives contact.
- Whether blood particles stay capped at that population or become a factor.

## Scope notes

**Do not change the design numbers.** If 20,000 does not hold frame rate, say so with
the measurement — proposing a different population is a gameplay-director call, filed
as a follow-up.

**Extend `docs/perf/one-camera-bench.md` by reference; do not rewrite it.** It is the
source of the ceiling numbers this task checks against.
