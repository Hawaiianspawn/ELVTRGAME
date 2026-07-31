---
id: 072
title: Reconcile scaling-curve §4's scratch TTK table against entity-tiers' committed numbers
status: done
agent: gameplay-director
model: sonnet
owns: ["docs/design/scaling-curve.md"]
resources: []
depends-on: []
epic: ""
evidence: scaling-curve.md §4's table either corrected to match entity-tiers.md §7 and the harness, or its divergence explained with a stated reason — plus a verdict on whether recruit-max actually loses the Elite matchup, since the two docs currently disagree on the direction.
score: {feel: 2, risk: 1, cost: 1}
source: user
teammate: curve-reconcile
decided: "2026-07-29 done"
---

## Why now

**Two committed design docs disagree on the same number, and the disagreement flips a
design conclusion.**

`docs/design/entity-tiers.md:373` (§7's table) gives the balanced Floor-2 Elite matchup a
TTK of **2.13s**. `docs/design/scaling-curve.md:246` (§4's table) gives the same matchup
**2.43s**. Verified in the lead session, both lines read directly.

The simulation harness computes **2.13s** — matching `entity-tiers.md`. That is not a
coincidence or a tie-break: `validate.py`'s bonus check independently reproduces
`entity-tiers.md` §7's table exactly (1.848s against its published 1.85s), so the harness is
demonstrably calibrated to that document. The per-unit arithmetic reconciles too:
`entity-tiers.md:91` defines Blow as DPS × 0.9s, and `:94` gives Militia's blow of 27.0
reduced to 15.0 against Elite Armor 12 — 15.0 / 0.9 = 16.667 DPS per unit, which is exactly
what the harness uses. `scaling-curve.md` §4's numbers imply 14.665 instead, which traces to
no committed table.

**The consequence is a reversed conclusion, not a rounding difference.**
`scaling-curve.md:253-260` states the recruit-max/balanced split "validates the triangle's
intent exactly" and has recruit-max losing every point-target matchup (Floor-2 Elite: 2.43s
balanced vs 2.58s recruit-max). The harness computes the opposite direction for that
matchup — recruit-max **wins** at 2.09s vs 2.13s. If the harness is right, a stated design
conclusion rests on a number that does not match canon.

`scaling-curve.md` §7 records that its own numbers came from a scratch script that was
written once and discarded. That is precisely the provenance failure `task-063` built the
harness to end, and this is the first time the harness has caught it happening.

Filed as a **design** task, not a doc-correction task, because the interesting question is
not which number to type — it is whether the triangle's intended balance actually holds once
the committed numbers are used.

## Done when

- The 2.43s vs 2.13s discrepancy is resolved with a stated reason: `scaling-curve.md`
  corrected, or its divergence explained (a different assumption, a different army shape) and
  that assumption written down.
- The recruit-max verdict for the Floor-2 Elite matchup is restated at whatever direction the
  committed numbers actually support. If recruit-max wins that matchup, `§4`'s "validates the
  triangle's intent exactly" needs revising rather than quietly leaving a claim the numbers
  no longer carry.
- The Floor-3 Boss row is handled honestly. `task-070` flagged a real confound there: the
  harness has no first-class degrade input, so its non-degraded recruit-max run (8.01s) is an
  optimistic bound. Hand-applying the doc's own 0.944 multiplier gives ~8.46s, which is
  slower than balanced and preserves §4's direction. Say which reading is taken.
- Any number kept in `§4` traces to a committed source, or is marked plainly as an estimate.

## Spawn prompt

```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME), executing task-072.

TWO COMMITTED DESIGN DOCS DISAGREE, and the disagreement reverses a design conclusion.

docs/design/entity-tiers.md:373  -- balanced Floor-2 Elite TTK = 2.13s
docs/design/scaling-curve.md:246 -- same matchup = 2.43s

The simulation harness computes 2.13s, matching entity-tiers.md. Check this yourself:
  py Scripts/sim/scenario_runner.py floor2-elite-point-target
  py Scripts/sim/scenario_runner.py floor2-elite-point-target-recruitmax
  py Scripts/sim/validate.py     (its bonus check reproduces entity-tiers §7 exactly)

The harness is not a third opinion — it is calibrated to entity-tiers.md and reproduces that
table exactly. The per-unit math reconciles: entity-tiers.md:91 defines Blow as DPS x 0.9s,
:94 gives Militia blow 27.0 -> 15.0 vs Elite Armor 12, and 15.0/0.9 = 16.667 DPS per unit,
which is what the harness uses. scaling-curve.md §4 implies 14.665, tracing to no committed
table. scaling-curve.md §7 records that its numbers came from a discarded scratch script.

THE CONSEQUENCE, which is the actual work: scaling-curve.md:253-260 concludes the
balanced/recruit-max split "validates the triangle's intent exactly," with recruit-max losing
every point-target matchup. The harness reverses that for Floor-2 Elite — recruit-max WINS,
2.09s vs 2.13s. If the committed numbers are right, that stated conclusion rests on a number
canon does not support.

READ FIRST:
  docs/design/scaling-curve.md   -- §4 and §7 especially. §7 is where it admits the scratch
                                    script provenance.
  docs/design/entity-tiers.md    -- §2.2 (Armor/Blow), §7 (the TTK table), §4 (surround caps)
  docs/sim/LIMITATIONS.md §3     -- what the point-target model IS trusted for, and its
                                    stated assumptions. This matters: it is a LOWER BOUND,
                                    fought clean, and that caveat may itself explain part of
                                    the gap.
  docs/sim/SWEEPS.md             -- py Scripts/sim/sweep.py exists now; use it rather than
                                    writing a scratch script. Writing a throwaway script to
                                    settle this would repeat the exact mistake that caused it.

YOU OWN EXACTLY ONE FILE:
  docs/design/scaling-curve.md

DO NOT WRITE ANYTHING ELSE. In particular do NOT edit docs/design/entity-tiers.md,
docs/data/*.json, Scripts/sim/** or docs/sim/** — a sim-director task may be running in the
sim tree. If you conclude entity-tiers.md is the wrong one, DO NOT change it: say so, with
your reasoning, and hand back. That is a bigger decision than this task.

THE FLOOR-3 BOSS ROW HAS A REAL CONFOUND — do not treat it as a second instance of the same
finding. The harness has no first-class degrade input, so its non-degraded recruit-max run
(8.01s) is an optimistic bound. Hand-applying the doc's own 0.944 degrade multiplier to the
retinue-only DPS gives ~8.46s, slower than balanced, which PRESERVES §4's direction. State
which reading you take and why. The Elite row is the clean finding; the Boss row is not.

ALSO WORTH YOUR ATTENTION, from a sweep run during task-069 and not yet investigated: on
floor1-swarm-wave, spearmen headcount must reach ~200 before the retinue wins, against a
population that is 15% Soldier-melee — over 5x floor 1's actual 32. The same retinue already
wins at actual headcount against pure Fodder. Soldier-melee's Armor 6 appears to matter far
more than population-ratio heuristics suggest. Reproduce with:
  py Scripts/sim/sweep.py floor1-swarm-wave --axis "scenario:Retinue.Composition[UnitType=spearmen].Count=32,160,200,250"
Read it as MECHANISM, not as a survivor-count prediction — LIMITATIONS.md §1 applies to every
wave-attrition number. Mention it in your handback if it bears on §4; do not let it become
the task.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: which number is right and why, what §4 now says, your verdict on whether
recruit-max actually loses the Elite matchup, and anything you could not resolve.
```
