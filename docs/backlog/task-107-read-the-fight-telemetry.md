---
id: 107
title: Read fights.csv — turn the telemetry nobody opens into per-wave metrics with spread
status: done
agent: sim-director
model: sonnet
owns:
  - "Scripts/sim/fight_metrics.py"
  - "docs/sim/FIGHT-METRICS.md"
resources: []
depends-on: []
epic: wave-measurement
evidence: >
  `py Scripts/sim/fight_metrics.py` reads `Saved/SwarmTelemetry/fights.csv` and
  prints, per group of runs, the median and spread of exchange rate, retinue
  survivors, enemy survivors, fight duration, and the outcome mix — plus
  `docs/sim/FIGHT-METRICS.md` documenting what each column means, which are
  measured versus derived, and demonstrated against at least one real capture.
score: {feel: 1, risk: 1, cost: 1}
source: user, 2026-07-30 — "I need an effective way to test them and record metrics"
teammate: fight-metrics
decided: "2026-07-30 done"
---

## Why now

`USwarmTelemetrySubsystem` has been writing `Saved/SwarmTelemetry/fights.csv` this
whole time and **nothing reads it.** Grepped the repo: zero consumers, no Python, no
doc. It is the one instrument in this project that measures balance rather than
predicting it, and its output goes into a file no tool opens.

This is also the answer to the gap task-103 hit. The Python wave-attrition model
cannot be trusted (`LIMITATIONS.md` §1) and no amount of work on it will fix that.
Real captured fight data does not have that problem. The reader is what makes the
data usable.

**This does not depend on task-105.** `fights.csv` can be generated today by playing
the current 250/450/700 waves — the reader does not care what the wave counts were,
and the CSV carries the tuning constants in every row precisely so old rows stay
attributable.

## Done when

`py Scripts/sim/fight_metrics.py` reports, over a set of fight rows:

- **Exchange rate** — brood killed per retinue lost. `SwarmTelemetry.h` calls this
  "the one number balance lives or dies on," and it is the direct test of the GDD's
  "more enemies, not spongier enemies" law. Median and spread, never just a mean.
- Retinue survivors, enemy survivors, duration, time-to-first-blood.
- **Outcome mix** — how many of N runs were `BroodCleared` vs `RetinueWiped` vs
  `HeroDown` vs `Stalemate`. A wave that wipes 3 times in 10 is a different design
  problem from one that wipes 10 in 10, and a mean hides that completely.
- Grouped so runs at different tuning constants do not silently pool together. The
  CSV embeds those constants per row for exactly this reason — use them.

## Scope notes

**Report spread, not just central tendency.** A single median across runs with real
variance is the same mistake as a single-point sim estimate. If there is only one
run in a group, say "n=1" rather than printing a median as if it meant something.

**Do not modify the C++ telemetry.** If a column you want is missing, report that as
a finding — the recorder is owned by whoever takes the N-run loop task.

**Do not put captured CSVs in the repo** unless they are small and clearly labelled
as a demonstration fixture. `Saved/` is build output.
