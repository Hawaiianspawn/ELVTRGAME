---
id: 096
title: Chain waves into a run — survivor carryover, ember income, and supply degrade
status: done
agent: sim-director
model: sonnet
owns:
  - "Scripts/sim/run_sim.py"
  - "docs/sim/RUN-SIM.md"
  - "docs/data/scenarios/run-*.json"
resources: []
depends-on: []
epic: sim-irons-out-fun
evidence: >
  `py Scripts/sim/run_sim.py --run slice-three-wave --seed 1` prints a per-wave
  table (start count, survivors, brood killed, embers earned, supply demand vs
  capacity, degrade multiplier applied) and a run verdict, chaining each wave's
  survivors into the next instead of restarting at full strength; `--json`
  emits the same; a self-test proves (a) a single-wave run reproduces
  `scenario_runner.py`'s number for that scenario byte-identically, and (b)
  same seed yields identical output; `py Scripts/sim/validate.py` and `py
  Scripts/sim/drift_check.py` both still pass unchanged.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: run-chaining
decided: "2026-07-30 done"
---

## Why now

Every scenario in the harness is one isolated engagement. `docs/sim/LIMITATIONS.md`
§4 names **multi-wave carryover** and **supply/degrade** as two things the harness
does not model at all — and between them they are why the harness cannot say
anything about a *run*, only about a fight. No choice made between waves can be
measured when wave 2 starts from a fresh full-strength retinue regardless.

The data to close both already exists and is cited: `docs/data/economy.json`
carries `supply.start_capacity` 60, `upkeep_per_unit` 1 and the exact degrade
formula `clamp(capacity / demand, 0.4, 1.0)` as a DPS multiplier; its
`embers.income` gives 0.1 per brood killed plus a 10 grant per growth site;
`docs/data/upgrades.json` `tier_ladder` gives the freed/militia/veteran HP and
DPS rows. `combat_model.simulate_wave_attrition()` already returns
`retinue_survivors` and `enemy_survivors`. Nothing here needs new math — it needs
a driver that carries state across calls.

This is the enabling primitive for task-097 (do the growth-site decisions
actually diverge). On its own it is plumbing; it is scored `feel: 2` because the
run-level attrition curve is the first thing this harness can produce that maps
to how a run *feels* rather than how one fight resolves.

## Done when

- `Scripts/sim/run_sim.py` exists and is a **driver only**. It composes
  `combat_model` and `data_loader`'s existing public functions and **does not
  edit `Scripts/sim/combat_model.py`, `scenario_runner.py`, `validate.py`,
  `data_loader.py`, `variety.py`, `differentiation.py`, `sweep.py` or
  `drift_check.py`** — task-076 holds a lock on the first four. Follow
  `differentiation.py`'s precedent: a thin driver over modules it does not own.
  If a needed value is genuinely unreachable through the existing public API,
  **stop and report that in the handback** rather than editing a locked file.
- A run is declared in `docs/data/scenarios/run-<name>.json`: an ordered list of
  existing scenario names to fight in sequence, the retinue's starting
  composition, and the growth-site stops between them (stops may be empty — 096
  does not need to spend embers, only to bank them). Format documented in
  `docs/sim/RUN-SIM.md`. Ship at least one: `run-slice-three-wave.json`, chaining
  `gate1-calibration-wave1/2/3` (or the floor scenarios — say in the doc which
  you chose and why), citing `docs/data/growth-sites.json`'s `slice_placement`
  for where the stops sit.
- **Carryover** — each wave's `retinue_survivors` becomes the next wave's
  starting count. State how fractional survivors and tier composition are carried
  (survivors are a pooled count, not identified units — pick a rule, write it
  down, justify it). Casualties are taken from the weakest tier first unless you
  document a better rule.
- **Supply/degrade** — upkeep demand = alive unit count × `upkeep_per_unit`;
  when demand exceeds capacity, the retinue's DPS is multiplied by
  `clamp(capacity/demand, min_multiplier, 1.0)` straight out of
  `economy.json`. Do not hardcode 0.4 or 60; read them.
- **Ember income** — accrued per wave from brood killed (`enemy_start -
  enemy_survivors`) × `income.per_brood_killed`, plus `growth_site_grant` at each
  stop. Reported, banked, unspent in this task.
- **Self-test** — `py Scripts/sim/run_sim.py --selftest` asserts a one-wave run
  reproduces `scenario_runner.py`'s output for that same scenario exactly (with
  degrade off / demand under capacity, so nothing has been silently applied),
  and that the same seed twice is identical. Non-zero exit on failure.
- `docs/sim/RUN-SIM.md` documents the run file format, the carryover rule, the
  degrade application point, and — required — a **trust caveat** section stating
  plainly that every wave-attrition number underneath inherits
  `docs/sim/LIMITATIONS.md` §1 (the wave model does not reproduce GATE1's
  measured ~110-of-120 survival at committed defaults, and predicts wipes), so
  run output is a **relative** comparison between runs of the same model, never
  an absolute claim about a played run. Do **not** edit `LIMITATIONS.md` itself —
  task-076 owns it; note in RUN-SIM.md that §4's "multi-wave carryover /
  supply-degrade are not modelled" lines now need updating and leave it there.
- `validate.py` and `drift_check.py` both still pass, unchanged and un-edited.

## Spawn prompt

```
You are the sim-director. Build Scripts/sim/run_sim.py: a driver that chains
several existing wave scenarios into one RUN, carrying survivors, ember income
and supply/degrade across them.

Read first, in this order:
  docs/sim/README.md            -- the harness layout and how to run it
  docs/sim/LIMITATIONS.md       -- especially §1 (wave model not validated) and
                                   §4 (multi-wave carryover and supply/degrade
                                   are the two gaps you are closing)
  docs/sim/MODEL.md             -- how simulate_wave_attrition actually works
  Scripts/sim/differentiation.py -- the precedent for a thin driver module
  docs/data/economy.json        -- supply.*, degrade formula, embers.income
  docs/data/upgrades.json       -- tier_ladder HP/DPS/upkeep rows
  docs/data/growth-sites.json   -- slice_placement, where the stops sit in a run

HARD CONSTRAINT — files you must NOT edit. Another task (task-076) holds a lock
on Scripts/sim/combat_model.py, scenario_runner.py, validate.py,
docs/sim/MODEL.md, docs/sim/VALIDATION.md, docs/sim/LIMITATIONS.md and
docs/data/scenarios/combat-model-constants.json. Also leave data_loader.py,
variety.py, differentiation.py, sweep.py, drift_check.py and every existing
docs/data/*.json alone. You write exactly three things:
  Scripts/sim/run_sim.py
  docs/sim/RUN-SIM.md
  docs/data/scenarios/run-<name>.json   (at least one)
If a value you need is not reachable through the existing public API of those
modules, STOP and say so in your handback. Do not edit around the lock.

What to build:

1. A run file format, docs/data/scenarios/run-<name>.json: an ordered list of
   existing scenario names, the retinue's starting composition, and the
   growth-site stops between waves (stops may be empty this task). Ship
   run-slice-three-wave.json chaining three real committed scenarios. Every
   number in it traces to a design doc or another docs/data/*.json file --
   cite them in a SourceRefs field, same discipline as the existing scenarios.

2. Survivor carryover: wave N's retinue_survivors is wave N+1's starting count.
   Survivors come back as a pooled count, not identified units, so you must pick
   and DOCUMENT a rule for which tiers take the casualties (weakest-first is the
   default unless you can justify better) and how a fractional survivor count is
   handled.

3. Supply/degrade, read from docs/data/economy.json -- never hardcoded:
   demand = alive units * upkeep_per_unit; when demand > capacity, multiply the
   retinue's DPS by clamp(capacity/demand, min_multiplier, 1.0). Report demand,
   capacity and the applied multiplier per wave.

4. Ember income: (enemy_start - enemy_survivors) * income.per_brood_killed per
   wave, plus growth_site_grant at each stop. Bank and report it. This task does
   NOT spend embers -- task-097 does that.

5. A --selftest that asserts: (a) a single-wave run with demand under capacity
   reproduces scenario_runner.py's numbers for that scenario byte-identically,
   proving you have not silently changed the combat math; (b) the same seed run
   twice is identical. Non-zero exit on failure.

6. docs/sim/RUN-SIM.md: the run file format, the carryover rule and why, where
   degrade is applied in the tick order, and a REQUIRED trust-caveat section
   stating that every number here inherits LIMITATIONS.md §1 -- the wave model
   does not reproduce GATE1's measured ~110-of-120 wave-1 survival at committed
   defaults and predicts full wipes, so run output is a RELATIVE comparison
   between runs of one imperfect model, never an absolute claim about a played
   run. Note in this doc that LIMITATIONS.md §4 now needs updating; do not edit
   LIMITATIONS.md yourself.

Do not tune any constant to make a run come out well. If the chained run wipes
on wave 1 -- which given LIMITATIONS §1 is the likely outcome -- report that as
the result. A wipe faithfully produced is the correct deliverable; a survivable
run produced by picking a friendlier number is not.

Verify before handing back: py Scripts/sim/run_sim.py --selftest,
py Scripts/sim/validate.py, py Scripts/sim/drift_check.py -- all three pass,
and the last two are unedited.

Hand back: the command you ran, the per-wave table it printed, the carryover
rule you chose and why, and anything you had to leave undone because of the
task-076 lock.
```
