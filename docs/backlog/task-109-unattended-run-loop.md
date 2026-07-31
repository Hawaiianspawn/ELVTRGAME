---
id: 109
title: Run a wave N times unattended, so a metric has a spread instead of one sample
status: proposed
agent: claude
model: ""
owns:
  - "ELVTR/Source/ELVTR/Mass/SwarmTelemetry.cpp"
  - "ELVTR/Source/ELVTR/Mass/SwarmTelemetry.h"
  - "ELVTR/Source/ELVTR/Spike/**"
resources: [unreal-editor]
depends-on: [105]
epic: wave-measurement
evidence: >
  A single launch that plays a wave N times zero-input and exits, producing N rows
  in `Saved/SwarmTelemetry/fights.csv` that `Scripts/sim/fight_metrics.py`
  (task-107) can group — demonstrated with a real N=10 capture and the resulting
  spread, not just the code.
score: {feel: 1, risk: 2, cost: 2}
source: user, 2026-07-30 — "an effective way to test them"
teammate: ""
decided: ""
---

## Why now

One launch is one sample. `GATE1-FUN-PROTOTYPE.md`'s measured baseline is a range
(109-111 of 120 across 4 runs, 131-145 brood on wave 3) precisely because a single
run does not tell you whether a wave is survivable or whether you got lucky — and
those 4 runs were done by hand. Any comparison between the three waves needs a
spread, and hand-pressing R for it does not scale to three waves at N runs each.

The pieces already exist. `ASpike1GameMode::RestartRun()` is there and bound to R.
`USwarmTelemetrySubsystem` already auto-detects fight start and end and appends one
summary row per fight. **Nothing joins them into an unattended loop.**

## Done when

A launch flag or CVar (e.g. `Swarm.Fight.Runs N`) makes the game mode re-run
automatically on reaching `Won` or `Lost`, until N runs have completed, then exit —
leaving N attributable rows in `fights.csv`.

Demonstrated with a real N=10 capture whose spread is reported in the handback.

## Scope notes

**Keep it small.** `RestartRun()` and the telemetry auto-detect both already work.
The honest size of this is a counter, a branch in `Tick` on run-over, and a quit —
not a test-harness framework, not a new subsystem, not a scenario description
format. If it grows past that, stop and say why.

**Zero-input, like GATE1.** The runs are unattended: the hero never moves and never
issues a stance, matching `GATE1-FUN-PROTOTYPE.md` §3's baseline convention. That is
a deliberate limitation, not an oversight — it measures the wave, not the player.
State it in the handback so nobody reads these numbers as played-run numbers.

**Do not merge this with `-SwarmBench`.** `Spike1GameMode.cpp:187-196` disables the
wave run under that flag on purpose: two independent population schedulers on one
field make every run incomparable. Balance capture and perf capture stay separate
launches. Task-108 owns the perf half.

**Watch out for the Live Coding trap.** Adding a `UPROPERTY` via Live Coding reports
success and then crashes the next PIE — class-layout changes need a full
editor-closed rebuild.
