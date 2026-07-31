---
id: 070
title: Build out the scenario library so the tuning questions have something to run against
status: done
agent: sim-director
model: sonnet
owns: ["docs/data/scenarios/**"]
resources: []
depends-on: [68]
epic: sim-tuning-loop
evidence: New committed scenarios under docs/data/scenarios/, each with SourceRefs and a Notes field admitting any simplification, each runnable by scenario_runner.py today, and each named for the tuning question it answers rather than for a floor number.
score: {feel: 1, risk: 1, cost: 2}
source: user
teammate: scenario-library
decided: "2026-07-29 done"
---

## Why now

The harness ships four scenarios: one validation fixture (`gate1-calibration-wave1`) and
three named by floor (`floor1-swarm-wave`, `floor2-ranged-wave`,
`floor3-boss-point-target`). That is enough to prove the harness runs. It is nowhere near
enough to *tune* with — a sweep runner (`task-069`) with four scenarios to sweep over
answers very few questions.

More importantly the existing set is organised by **where a fight happens**, not by **what
question it answers**. A tuning loop needs the second kind: "does the Vanguard retinue hold
at 2x fodder density", "where does the archer tier stop mattering", "at what elite count
does the hero stop being the win condition". Those are the shapes a sweep is useful across.

`task-004` also just landed real ranked-arrival data derived from shipped CVars, and
`task-068` is teaching the model to consume it. Scenarios written after both land can carry
arrival timing instead of assuming a population fully in contact at t=0 — which the four
existing ones all do.

## Done when

- New scenarios exist covering tuning questions across **both** model kinds — point-target
  (validated, trustworthy per `LIMITATIONS.md` §3) and wave-attrition (scaffold; every
  scenario of this kind carries the §1 caveat in its `Notes`).
- Each is **named for its question**, not its location.
- Every scenario carries `SourceRefs`, and a `Notes` field admitting any simplification —
  the four existing files are the house style and the standard to match.
- Counts and compositions trace to a real design-doc number. Where one does not exist, the
  scenario says so plainly in `Notes` rather than inventing precision. Per the sim-director
  definition: do not invent them — ask, read further, or admit the simplification.
- Every new scenario actually runs under `py Scripts/sim/scenario_runner.py <name>` at the
  time of handback. A committed scenario that errors is worse than one that does not exist.

## Spawn prompt

```
You are executing task-070 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.
You are the sim-director. This is part of the sim-tuning-loop epic.

THE GAP: the harness ships four scenarios — one validation fixture plus three named by
floor. That proves the harness runs; it does not let anyone tune with it. A sibling task
(task-069) is building a sweep runner, and a sweep over four floor-shaped scenarios answers
very few questions.

Scenarios organised by WHERE a fight happens are the wrong axis for tuning. Write them named
for the QUESTION they answer. Shapes worth covering, not exhaustive and not prescriptive —
read the design docs and pick the ones that are actually live:
  - does the retinue hold as fodder density scales
  - where does the archer tier stop earning its cost
  - at what elite/titan count does the hero stop being the win condition
  - where does the scaling curve flatten or break across the three floors

READ FIRST:
  docs/data/scenarios/*.json + scenarios.schema.md  -- the four existing files ARE the house
                                     style. Match their SourceRefs/Notes discipline exactly.
  docs/design/entity-tiers.md, scaling-curve.md, encounter-budget.md
  docs/data/entity-tiers.json, unit-types.json, economy.json, upgrades.json,
    encounter-budget.json
  docs/sim/LIMITATIONS.md  -- §1 (wave-attrition is a scaffold) and §3 (point-target is
                              validated). This determines what caveat each scenario carries.
  docs/sim/MODEL.md

NOTE ON TIMING: task-068 is running right now and is editing scenarios.schema.md and
gate1-calibration-wave1.json to add per-rank arrival timing. Re-read the schema before you
write, and if arrival timing has landed, USE IT — a scenario assuming the whole enemy
population is in contact at t=0 is the assumption the harness is currently being fixed to
stop making. If it has not landed, write without it and say so in Notes.

YOU OWN EXACTLY:
  docs/data/scenarios/**

DO NOT WRITE ANYTHING ELSE. In particular:
  - Scripts/sim/** and docs/sim/** belong to task-069, running beside you. If a scenario
    you want needs a harness capability that does not exist, DO NOT add it — write the
    scenario anyway if it can be expressed, or report the missing capability in your
    handback so I can route it to 069.
  - docs/data/*.json outside docs/data/scenarios/ are INPUTS. Read, never edit. If you find
    an error in one, that is a handoff to the gameplay director — report it.

THE RULE FROM YOUR OWN DEFINITION: if a scenario's population or composition counts do not
trace to a real design-doc number, do not invent them. Ask, read further, or say plainly in
Notes what simplification you made and why. Every scenario gets SourceRefs. Every wave
attrition scenario carries LIMITATIONS.md §1's caveat in its Notes — a survivor count out of
that model is not a prediction.

VERIFY EACH ONE RUNS: `py Scripts/sim/scenario_runner.py <name>` must succeed for every file
you commit, at handback time. A committed scenario that errors is worse than no scenario.
Run `py Scripts/sim/validate.py` first and say if it was already failing when you started
(check 3 is EXPECTED to fail — that is documented, not your problem to fix).

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: each scenario, the question it answers, its actual run output, and which numbers
are cited versus assumed. State plainly anything you could not do.
```
