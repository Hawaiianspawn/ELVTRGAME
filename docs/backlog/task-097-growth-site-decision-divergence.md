---
id: 097
title: Measure whether the growth-site allocation is a real decision or theatre
status: done
agent: sim-director
model: sonnet
owns:
  - "Scripts/sim/decisions.py"
  - "docs/sim/DECISIONS.md"
resources: []
depends-on: [96]
epic: sim-irons-out-fun
evidence: >
  `py Scripts/sim/decisions.py --run slice-three-wave --seeds 25` enumerates
  every legal ember allocation at each growth-site stop, runs the identical
  seeded run down each branch, and prints an outcome table plus a named verdict
  per stop — REAL DECISION (branches diverge and no lane dominates), DOMINANT
  LANE (one allocation wins across most seeds, named), or THEATRE (branches
  converge inside noise). `docs/sim/DECISIONS.md` carries the measurement, the
  per-stop verdict, and — where a lane dominates or the choice is inert — the
  specific data-level observation handed to the gameplay director, without
  prescribing a number.
score: {feel: 3, risk: 2, cost: 2}
source: user
teammate: decisions
decided: "2026-07-30 done"
---

## Why now

GDD §48 lists "meaningful decisions on the journey" as a design pillar, and §6
is a whole section on it. `docs/data/growth-sites.json` already encodes the
first concrete instance: five actions (recruit 12 / promote 15 / provision 10 /
item 20 / hero 18 embers) against ~33 embers on arrival, with the file's own
`budget_note` asserting the scarcity *is* the design, and its `growth-A` note
saying the slice currently has a free refill there — "the single highest-leverage
change to add meaningful choice."

That assertion has never been tested. It is exactly the shape of claim the
harness can attack: hold the seed and the enemy side fixed, vary only the
allocation, and read whether the run outcomes actually separate. Three failure
modes are all live and all invisible today — one lane is simply better than the
others (a dominant strategy, not a decision), the lanes are within noise of each
other (theatre), or the ember budget is loose enough that the "you cannot buy
all" premise does not bind.

This is the highest-value fun question the harness can currently reach, because
the answer changes what gets built: a THEATRE verdict means the growth site as
specced does not deserve UI, and a DOMINANT LANE verdict names the number to
change before anyone implements it.

## Done when

- `Scripts/sim/decisions.py` exists as a driver over task-096's `run_sim.py`.
  Same lock discipline as 096: **do not edit `combat_model.py`,
  `scenario_runner.py`, `validate.py`, `data_loader.py`, `variety.py`,
  `differentiation.py`, `sweep.py`, `drift_check.py`, `run_sim.py`, or any
  existing `docs/data/*.json`.** If `run_sim.py` needs a hook it does not
  expose, say so in the handback rather than editing it.
- **Branch enumeration** — at each growth-site stop, enumerate every allocation
  of the banked embers that is legal under `growth-sites.json` (affordable, and
  respecting whatever repeat rules the file implies — state your reading of
  them in the doc, since the file does not say outright whether an action may be
  taken twice). Effects come from the file: `recruit` +10 units at freed tier and
  +10 supply demand, `promote` up to 20 units one tier up
  (`upgrades.json` `tier_ladder`), `provision` +25 supply capacity. `item` and
  `hero` have no modelled effect — **report them as an explicitly unmodelled
  branch, do not silently drop them and do not invent a number for them.** That
  omission is itself a result worth stating: two of the five lanes, including
  both of the file's named "real temptations", are things this harness cannot
  price at all.
- **Divergence measurement** — for each stop, across ≥25 seeds: the spread in
  final outcome between branches (waves survived, final retinue count, total
  brood killed — pick the primary metric, justify it, report the others), how
  often each branch wins, and whether the best and worst branch are separated by
  more than the seed-to-seed variance of a *single fixed* branch. That last
  comparison is the whole test — a difference smaller than the noise of holding
  the choice constant is not a decision.
- **A named verdict per stop**, one of: `REAL DECISION` (branches separate
  cleanly and no single lane wins most seeds), `DOMINANT LANE: <id>` (one wins
  across most seeds — name it and its win rate), or `THEATRE` (branch spread
  sits inside single-branch variance). Where the verdict is not REAL DECISION,
  give the gameplay director the specific data-level observation behind it — the
  cost or effect that is out of line — **without prescribing a replacement
  number.** Same discipline `docs/sim/DIFFERENTIATION.md` used.
- **Reproducible and honest** — `--seed` fully determines the output, same seed
  twice is identical, and a `--selftest` asserts it. `docs/sim/DECISIONS.md`
  repeats the LIMITATIONS §1 trust caveat inherited through `run_sim.py`: these
  are relative comparisons between branches of one unvalidated wave model, and
  the fact that every branch may end in a wipe does not invalidate the
  *comparison* between branches — but it does mean no absolute claim.
- If every branch wipes identically and the metric cannot separate them at all,
  that is a reportable result — say the measurement could not resolve the
  question at this model's fidelity and name what would be needed. Do not
  manufacture separation by changing a constant.

## Spawn prompt

```
You are the sim-director. Measure whether the growth-site allocation in
docs/data/growth-sites.json is a REAL DECISION, a DOMINANT LANE, or THEATRE.

This depends on task-096, which built Scripts/sim/run_sim.py -- a driver that
chains several wave scenarios into one run, carrying survivors, ember income and
supply/degrade across them. Read that module and docs/sim/RUN-SIM.md first; it
is the thing you are driving.

Then read:
  docs/data/growth-sites.json  -- the five actions, their ember costs, effects,
                                  the budget_note, and slice_placement's two
                                  stops with their estimated ember pools
  docs/data/economy.json       -- supply capacity/demand/degrade, ember income
  docs/data/upgrades.json      -- tier_ladder, what "promote one tier" means
  docs/sim/LIMITATIONS.md      -- §1 especially; the trust caveat you inherit
  docs/sim/DIFFERENTIATION.md  -- the precedent for how to report a verdict of
                                  this shape honestly, including how it named a
                                  data-level observation WITHOUT prescribing a
                                  number

HARD CONSTRAINT -- you write exactly two files:
  Scripts/sim/decisions.py
  docs/sim/DECISIONS.md
Do not edit combat_model.py, scenario_runner.py, validate.py, data_loader.py,
variety.py, differentiation.py, sweep.py, drift_check.py, run_sim.py, or ANY
existing docs/data/*.json. Task-076 holds locks on several of those and
task-096 owns run_sim.py. If run_sim.py lacks a hook you need, STOP and say so
in your handback instead of editing it.

The measurement:

1. At each growth-site stop, enumerate every LEGAL allocation of the banked
   embers under growth-sites.json. The file does not say whether an action may
   be taken twice -- state your reading in the doc and be consistent.

2. Model recruit / promote / provision from the file's own effect fields
   (+10 freed units and +10 supply demand; promote up to 20 units one tier;
   +25 supply capacity). The `item` and `hero` lanes have no representable
   effect in this harness. Report them as an EXPLICITLY UNMODELLED branch.
   Do not drop them silently and do not invent a damage number for them --
   "two of five lanes, including both of the file's named real temptations,
   cannot be priced by this harness" is itself a finding worth stating plainly.

3. Across at least 25 seeds, run the IDENTICAL run down each branch -- same
   seed, same enemy side, only the allocation differs. Report per branch: waves
   survived, final retinue count, total brood killed. Name which is your primary
   metric and why.

4. THE ACTUAL TEST: compare the best-vs-worst branch spread against the
   seed-to-seed variance of ONE FIXED branch. A branch spread smaller than
   single-branch noise is not a decision, it is theatre. Report both numbers
   side by side so a reader can check the comparison themselves.

5. Give each stop one named verdict: REAL DECISION / DOMINANT LANE: <id> (with
   its win rate) / THEATRE. Where it is not REAL DECISION, hand the gameplay
   director the specific data-level observation -- which cost or effect is out
   of line -- WITHOUT prescribing a replacement number. That is not your call.

6. --selftest asserts same-seed reproducibility, non-zero exit on failure.

7. docs/sim/DECISIONS.md carries the whole measurement and repeats the
   LIMITATIONS.md §1 caveat you inherit through run_sim.py: this is a relative
   comparison between branches of one unvalidated wave model. Every branch may
   end in a retinue wipe -- that does not invalidate comparing branches to each
   other, but it does mean you make no absolute claim about a played run.

Do not tune any constant to produce a more interesting answer. If every branch
wipes identically and the measurement cannot separate them, report exactly that
and name what would be needed to resolve it. A null result honestly reported is
the correct deliverable.

Hand back: the commands you ran, the per-stop verdict table, the
spread-vs-noise numbers behind each verdict, and the observation you are handing
to the gameplay director.
```
