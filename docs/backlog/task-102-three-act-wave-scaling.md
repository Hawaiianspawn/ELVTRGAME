---
id: 102
title: Spec the three waves as early / mid / late game — retinue size, unique units, and the scale ceiling
status: done
agent: gameplay-director
model: sonnet
owns:
  - "docs/design/wave-scaling-three-act.md"
  - "docs/data/wave-scaling.json"
resources: []
depends-on: []
epic: three-act-waves
evidence: >
  `docs/design/wave-scaling-three-act.md` plus a schema-conformant
  `docs/data/wave-scaling.json`, together naming for each of the three waves:
  starting retinue count, retinue composition (spearmen/archer split and tier),
  total enemy population, enemy composition by `entity-tiers.json` tier, ranged
  share on both sides, and the promoted-Actor (Elite) instance count. The late
  wave's population must be justified against the measured ~34,000-entity 60fps
  ceiling in `docs/perf/one-camera-bench.md` §"60fps ceilings", with the chosen
  number and the headroom left stated explicitly. Every number carries a citation
  or is flagged as a new proposal.
score: {feel: 3, risk: 2, cost: 2}
source: user
teammate: wave-scaling
decided: "2026-07-30 done"
---

## Why now

The owner wants the three waves to read as **early / mid / late game** — a small
retinue early, unique units in the middle, and a late wave at "as much scale as we
can push." The shipped prototype cannot express any of that. `Spike1GameMode.h:47`
holds `WaveBroodCounts = {250, 450, 700}` with a single flat `StartingRetinue = 120`
and a refill-to-cap between waves: one retinue size for the whole run, one enemy
type per wave, no composition axis at all.

The scale ambition also collides with canon. `SYSTEMS.md:45` **locks** 250/450/700
as the slice curve (decision record dated 2026-07-24). Meanwhile
`docs/perf/one-camera-bench.md` measured the real ceiling at **~34,000 entities at
60fps** with `Swarm.SimLOD.Stride 4`. Canon's biggest wave is 2% of what the engine
demonstrably carries. That gap is the whole content of "as far as we can push it,"
and nobody has written down what the curve should be if the perf budget rather than
the gate-1 placeholder sets it.

## Done when

`docs/design/wave-scaling-three-act.md` exists and answers, per wave, with citations:

1. **Retinue** — starting count and composition. Early is explicitly *smaller* than
   today's flat 120. Say what the retinue is at each wave and whether it grows by
   carryover, by growth-site purchase, or by refill.
2. **Enemy population and composition** — total count and the split across
   `docs/data/entity-tiers.json` tiers, extending (not rewriting)
   `docs/design/scaling-curve.md` §1's existing 85/15 → 55/25/20 → 45/25/30 table.
3. **What makes mid "unique units"** — which specific types appear at wave 2 that
   are absent at wave 1, drawn from `docs/data/unit-types.json` (retinue side, e.g.
   the Archer split and the knight subtypes) and `entity-tiers.json` (brood side).
   This is the wave's identity, not a stat bump.
4. **The late-wave number**, argued against the measured ceiling, with headroom
   stated. If the answer is "20,000, leaving 40% headroom for the Elites, the
   retinue and blood particles," say that and show the arithmetic.
5. **Ranged share on both sides** — see the scope note below.

`docs/data/wave-scaling.json` carries the same numbers machine-readably, with a
`design_constants` block citing sources the way `scaling-curve.json` and
`encounter-budget.json` already do, and a sibling `.schema.md` if the shape is new.

## Scope notes — read these before starting

**On projectiles.** The owner mentioned projectiles as part of this test. There is
**no projectile system in the engine** — `grep -r Projectile ELVTR/Source` returns
nothing. What exists is *ranged combat*: `EUnitType::Archers` in
`ELVTR/Source/ELVTR/Mass/SwarmCombat.h:49` with an engage range and instant damage
application, and `brood_soldier_ranged` in `entity-tiers.json`, exercised by
`docs/data/scenarios/floor2-ranged-wave.json`. **Spec the ranged share of each wave
using those existing types.** Do not design a projectile-actor system, travel time,
or a miss model — that is a separate mechanic and it is not this task. If the spec
surfaces a question that only real projectiles answer, write it in an "Open" section
and leave it.

**Do not edit `SYSTEMS.md`.** The 250/450/700 lock is a committed decision record and
superseding it is the owner's call, made at handback with your proposal in front of
them. Write the new curve as a **proposal** in your own doc, and include a short
section stating plainly which `SYSTEMS.md` line it would supersede and why. The same
goes for `docs/design/scaling-curve.md` — extend it by reference, do not rewrite it.

**Do not edit `docs/data/economy.json` or `growth-sites.json`.** Task-101 owns the
supply-capacity reconciliation and is unresolved. Relevant to you: `economy.json`
sets `supply.start_capacity` to 60 while every wave scenario assumes a 120 retinue,
so the retinue currently fights degraded from tick zero. **A smaller early retinue
may dissolve that collision on its own** — if your early number lands at or under 60,
say so in the doc, because it materially changes task-101's premise. Do not act on
task-101 yourself.

**Do not author scenario files.** `docs/data/scenarios/**` belongs to task-103, which
turns your numbers into runnable fixtures. Write the numbers; it runs them.

**Be honest about what the sim can check.** `docs/sim/LIMITATIONS.md` §1 states the
wave-attrition model does not reproduce its one measured baseline (predicts a wipe
where GATE1 measured 109-of-120 surviving). Task-103 will run your curve, but the
survivor counts it returns are **directional, not predictive**. Do not write a spec
whose justification depends on a survivor number nobody has yet, and do not tune to
one. Argue from composition, from the perf ceiling, and from the measured GATE1
density curve (250 → ~92% survival, 450 → wipe with 16-24 brood left, 700 → wipe with
131-145 left, `docs/GATE1-FUN-PROTOTYPE.md` §3b).

## Spawn prompt

```
You are the gameplay-director. Spec the three-wave curve as an early / mid / late
game progression for Kindled (the game is named Kindled, not Emberkeep).

THE ASK, from the owner, verbatim in intent: the three waves should represent early,
mid and late game. Early has a SMALLER retinue than today. Mid is middle-scale and
introduces UNIQUE UNITS that wave 1 does not have. Late is massive scale — as far as
the engine can be pushed. Ranged units are part of the test.

WHAT YOU OWN — write only these two files:
  docs/design/wave-scaling-three-act.md
  docs/data/wave-scaling.json   (plus a .schema.md sibling if the shape is new)

WHAT YOU MUST NOT TOUCH:
  SYSTEMS.md                  — the 250/450/700 lock at line 45 is a committed
                                decision record; superseding it is the owner's call.
                                Propose the change in YOUR doc, with a section naming
                                exactly which line it would supersede and why.
  docs/design/scaling-curve.md — extend by reference, never rewrite.
  docs/data/economy.json, docs/data/growth-sites.json — task-101 owns these.
  docs/data/scenarios/**      — task-103 owns these.
  Any file under ELVTR/Source/ — this is a paper pass, no engine changes.

READ FIRST:
  ELVTR/Source/ELVTR/Spike/Spike1GameMode.h  — the shipped wave shape you are
      replacing. WaveBroodCounts = {250,450,700}, StartingRetinue = 120,
      RetinueCap = 120. One retinue size, one enemy type per wave, no composition.
  docs/design/scaling-curve.md §1  — the existing per-floor composition table
      (85/15 -> 55/25/20 -> 45/25/30) and the Elite instance schedule. Extend it.
  docs/design/encounter-budget.md  — pulse/Lull pacing and rank arrival timing.
  docs/data/entity-tiers.json      — brood stat blocks (fodder, soldier_melee,
      soldier_ranged, elite, titan, boss) and their Armor values.
  docs/data/unit-types.json        — retinue side: spearmen, archers, knight subtypes.
  docs/perf/one-camera-bench.md    — THE SCALE CEILING. Section "60fps ceilings":
      ~34,000 entities at 60fps with Swarm.SimLOD.Stride 4, measured, extended to
      40,000 so the ceiling is measured rather than extrapolated. This is the number
      "as far as we can push it" has to be argued against.
  docs/GATE1-FUN-PROTOTYPE.md §3b  — the measured density curve: 250 -> 109-111 of
      120 survive; 450 -> retinue wiped, 16-24 brood left; 700 -> wiped, 131-145 left.
  docs/sim/LIMITATIONS.md §1       — why you may not justify anything with a
      predicted survivor count.

DELIVER, per wave (early / mid / late), each number cited or flagged as a new proposal:
  1. Starting retinue count and composition (spearmen/archer split, tier). Early is
     explicitly smaller than 120. State how the retinue changes between waves —
     carryover, growth-site purchase, or refill.
  2. Total enemy population and its split across entity-tiers.json tiers.
  3. What makes MID distinct: which specific unit types appear at wave 2 that are
     absent at wave 1, on both sides. This is the wave's identity, not a stat bump.
  4. The LATE population, argued against the ~34,000 measured ceiling, with the
     headroom you are leaving and the arithmetic shown. Account for what else is on
     screen: the retinue, Elite promoted-Actors, blood particles.
  5. Ranged share on both sides.

ON PROJECTILES — IMPORTANT. There is no projectile system in the engine; grep -r
Projectile ELVTR/Source returns nothing. What exists is ranged combat: EUnitType::
Archers in ELVTR/Source/ELVTR/Mass/SwarmCombat.h:49 (engage range, instant damage,
TargetsPerHit 1) and brood_soldier_ranged in entity-tiers.json. Spec the ranged share
using those existing types ONLY. Do NOT design projectile actors, travel time, or a
miss model — that is a separate mechanic and out of scope. Questions that only real
projectiles can answer go in an "Open" section, unresolved.

ON TASK-101, which you must not act on: economy.json sets supply.start_capacity to 60
while every wave scenario assumes a 120 retinue, so the retinue currently fights the
whole run at a degrade multiplier from tick zero. If your early retinue number lands
at or under 60, SAY SO EXPLICITLY in the doc — it changes task-101's premise and the
owner needs to see that.

HAND BACK: the two file paths, the three-wave table in your reply, the late-wave
number with its headroom arithmetic, which SYSTEMS.md line your proposal would
supersede, whether your early retinue number affects task-101, and any question you
had to assume past.
```
