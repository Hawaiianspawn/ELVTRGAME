---
id: 068
title: Teach the wave-attrition model arrival timing, and re-run the GATE1 calibration check
status: done
agent: sim-director
model: sonnet
owns: ["Scripts/sim/**", "docs/sim/**", "docs/data/scenarios/**"]
resources: []
depends-on: [4]
epic: ""
evidence: validate.py's check 3 re-run with per-rank arrival gating active, its actual output committed to VALIDATION.md, and a plain statement of whether the GATE1 110-of-120 gap closed, narrowed, or did not move — with the residual attributed to candidate 2 (MaxAttackersPerUnit transfer) only if the numbers support it.
score: {feel: 1, risk: 3, cost: 2}
source: user
teammate: wave-arrival-timing
decided: "2026-07-29 done"
---

## Why now

`docs/sim/LIMITATIONS.md` §1 names two undisentangled candidates for why the
wave-attrition model predicts a full retinue wipe where GATE1 measured ~110-of-120
surviving. Candidate 1 is arrival/spawn-pacing timing — and §1 states plainly that no
committed data file carries it, and §2 names the encounter-budget table as the deliverable
that would.

**`task-004` just built it, and the data is better than §2 anticipated.** It is not an
estimate: `ELVTR/Source/ELVTR/Mass/SwarmCommands.cpp` spawns brood in *ranks*, and the
shipped CVars are cited by name — `Swarm.BroodSpawnRadiusMin` 2500uu (`:31`),
`Swarm.BroodFormation.RankSpacing` 140uu (`:99`), `Swarm.BroodSpeedJitter` 0.06 (`:120`),
with `:241` confirming rank 0 leads and later ranks step outward. Applied to the harness's
own `gate1-calibration-wave1.json` fixture, no brood reaches melee contact before ~5.85s
and the full population is not in until ~7.6s. `docs/data/encounter-budget.json` carries
this as `rank_arrival_context[]` (7 rows) and `rank_arrival_timing[]` (32 rows), tagged
`DERIVED FROM SHIPPED CODE` to separate it from that task's own judgment calls.

**The harness cannot consume it.** Verified in the lead session:
`simulate_wave_attrition` (`Scripts/sim/combat_model.py:222`) takes ten parameters and none
is an arrival time; the only occurrence of "arrival" in the module is a docstring pointing
at `LIMITATIONS.md`. Every `Composition.Count` is fully alive from t=0 regardless of what a
scenario file says. `task-004` correctly declined to write a scenario, because one written
today would be silently ignored and would reproduce the known wipe while *looking* tested.

So the blocking input now exists and the consumer does not. This is the task that turns the
scaffold `LIMITATIONS.md` §2 describes into a model that has actually been checked against
its one measured baseline.

## Done when

- `simulate_wave_attrition` gates a group's contribution to melee-alive counts, frontage,
  and damage behind `t >= ArrivalSeconds`. `task-004`'s handback proposes splitting each
  `WaveGroup` into per-rank sub-groups carrying `ArrivalSeconds`, with
  `rank_arrival_timing[]` rows already shaped for it — evaluate that shape, adopt or
  improve it, and say which.
- `docs/data/scenarios/scenarios.schema.md` documents the new field, and
  `gate1-calibration-wave1.json` carries the arrival data sized to it.
- `validate.py` check 3 re-runs with gating active and its **actual output** is committed
  to `VALIDATION.md` — not a summary, not a rounding.
- `LIMITATIONS.md` §1 and §2 are rewritten to match whatever actually happened, including
  the case where the gap does not close. §2's "ready scaffold" claim is a prediction this
  task tests; if it was wrong, say so.
- The verdict on candidate 1 vs candidate 2 is stated at the confidence the numbers
  support. If arrival gating closes the gap, candidate 1 was it. If it narrows and stops,
  the residual is evidence for candidate 2 (`MaxAttackersPerUnit`'s pooled-vs-per-entity
  transfer) but **not proof** — that still wants an in-engine measurement, and saying so
  is the correct finish line.

## Spawn prompt

```
You are executing task-068 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.
You are the sim-director. Read your own definition first — and note that
.claude/agents/sim-director.md line ~51 carries a RETRACTED claim ("fails across its entire
documented parameter range"). docs/sim/LIMITATIONS.md §1 is the corrected source of truth:
the corrected 27-cell sweep has the retinue WINNING in 15 of 27 cells. task-067 is filed to
fix that file; it is NOT yours to edit. Work from LIMITATIONS.md, not from your own
definition, wherever the two disagree.

WHAT CHANGED: the arrival-timing data LIMITATIONS.md §1 candidate 1 has always been missing
now exists, and it is derived from shipped code rather than estimated.

READ FIRST, all of these:
  docs/data/encounter-budget.json  -- rank_arrival_context[] (7 rows), rank_arrival_timing[]
                                      (32 rows), design_constants.rank_arrival_source_cvars
  docs/data/encounter-budget.schema.md  -- documents both tables and their epistemic status
  docs/design/encounter-budget.md  -- §2a states plainly what this data does NOT close
  docs/sim/LIMITATIONS.md §1 §2    -- the two candidates, and §2's "ready scaffold" claim
  docs/sim/VALIDATION.md           -- check 3, and the cleave-capacity bug writeup
  ELVTR/Source/ELVTR/Mass/SwarmCommands.cpp  -- READ ONLY. Lines ~30-130 and ~190-250 are
                                      the rank-spawn source. Verify the numbers yourself.

THE GAP TO CLOSE: simulate_wave_attrition (Scripts/sim/combat_model.py:222) has no arrival
parameter. Every Composition.Count is alive from t=0. Verified in the lead session — the
only occurrence of "arrival" in that module is a docstring pointing at LIMITATIONS.md.

YOU OWN:
  Scripts/sim/**
  docs/sim/**
  docs/data/scenarios/**

DO NOT WRITE ANYTHING ELSE. In particular:
  - docs/data/encounter-budget.* is task-004's output and is INPUT to you. Read it, never
    edit it. If you find an error in it, report it — that is a handoff to the gameplay
    director, per your own definition.
  - ELVTR/Source/** is read-only for you. You are reading shipped CVars, not changing them.
  - .claude/agents/sim-director.md belongs to task-067.

THE WORK: gate each group's contribution to enemy_melee_alive, frontage, and damage behind
t >= ArrivalSeconds in the per-tick loop. task-004 proposes splitting each WaveGroup into
per-rank sub-groups carrying ArrivalSeconds; rank_arrival_timing[] is already shaped for
that. Evaluate that shape rather than adopting it unexamined — if a cleaner one exists, take
it and say why. Document the mechanism in docs/sim/MODEL.md with its assumptions, per your
own definition's rule that a combat_model.py extension gets a matching MODEL.md addition.

Then: add the arrival data to gate1-calibration-wave1.json, document the new field in
scenarios.schema.md, and re-run `py Scripts/sim/validate.py`. Commit the ACTUAL output to
VALIDATION.md.

THE ONE RULE THAT MATTERS: do NOT tune EngagedSpacingUU, MaxAttackersPerUnit, or
MeleeContactFacingFraction to make check 3 pass. All three are cited values. LIMITATIONS.md
§1 is explicit that some off-default combination could technically be found that passes, and
that finding one would be worse than failing — a passing check with no citation behind the
value that produced it. Your job is to add a mechanism that is independently justified by
shipped code and then honestly report what it does to the number. If arrival gating does not
close the gap, THAT IS A VALID AND USEFUL RESULT. Report it plainly.

A NEGATIVE RESULT IS NOT A FAILED TASK. LIMITATIONS.md §2 predicts this plugs in "without
restructuring" — that is a claim this task tests, not a guarantee. If the gap narrows and
stops, the residual is evidence for candidate 2 (MaxAttackersPerUnit's pooled-vs-per-entity
transfer) but not proof; that still needs an in-engine measurement, and saying so is the
correct finish line, not a hedge.

Update LIMITATIONS.md §1 and §2 to match what actually happened, whichever way it went.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: the exact command run, the actual output table unrounded, whether the gap closed,
narrowed, or did not move, and your verdict on candidate 1 vs candidate 2 at the confidence
the numbers support. State plainly anything you could not do.
```
