---
id: 091
title: Measure whether the build space actually differentiates, or whether cleave is the only answer
status: done
agent: sim-director
model: sonnet
owns: ["docs/sim/DIFFERENTIATION.md", "Scripts/sim/differentiation.py", "docs/data/scenarios/hero-build-differentiation-*.json"]
resources: []
depends-on: [90]
epic: build-space-differentiates
evidence: docs/sim/DIFFERENTIATION.md — a multi-seed, multi-scenario measurement of how much of the build space is competitive, with a named verdict on whether Cleave Melee Sweep's dominance is a data problem or a correct answer to a swarm scenario
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: differentiation
decided: "2026-07-29 done"
---

## Why now
The owner's stated reason for the whole build-variety layer is that **characters will be
unique in the game**. Measured against that goal on the committed data, the space does
not currently deliver: rolling 20 builds against `floor1-swarm-wave` at seeds 1, 3, 7, 11
and 42, **Cleave Melee Sweep took rank 1 on all five**, carrying 24–48% of total roster
damage. It is the only weapon in the roster with `targets_per_shot: 8`; every other weapon
sits at 1–4. Against a 250-enemy wave, an 8-target cleave is worth roughly 2–8× anything
else on that axis alone.

Two very different explanations, and nobody has separated them:

1. **The space is broken** — 999 builds collapse to "did you roll cleave", and 998 of them
   are strictly worse. If so, wiring this into the game ships a monoculture.
2. **The scenario is the whole story** — cleave *should* dominate a 250-enemy melee blob,
   and the space differentiates fine once the fight isn't a swarm. If so, the finding is
   about scenario coverage, not the build data.

The point-target model is validated (it reproduces `entity-tiers.md` §7's table exactly);
the wave-attrition model is not (`docs/sim/LIMITATIONS.md` §1). Both explanations are
testable against the validated half, which is why this is answerable now.

## Done when
- The measurement is driven by a new `Scripts/sim/differentiation.py` that **calls
  `variety.py --json --seed N` in a loop and aggregates**. Do not modify `variety.py` —
  it already exposes exactly the interface needed (`--seed`, `--scenario`, `--json`), and
  task-090 owns that file.
- `docs/sim/DIFFERENTIATION.md` exists and reports, over **at least 20 seeds**, not five:
  - The damage-share distribution of a rolled roster — how concentrated is rank 1, and
    how much of the roster is within some stated band of the best build.
  - How often each weapon archetype takes rank 1, across seeds. If cleave is 5/5 it will
    be near 20/20; the number matters.
  - The same measurement against a **point-target** scenario as well as a wave scenario.
    This is the load-bearing comparison — it is what separates explanation 1 from 2. Use
    the existing `floor2-elite-point-target` / `floor3-boss-point-target` scenarios; add
    new scenario files only if the existing library genuinely cannot express the case.
- A **named verdict**, in plain words: is the build space differentiated, or is it a
  monoculture, or is it scenario-dependent — and which. "It depends" is only acceptable
  if it names what it depends on and shows the numbers either side.
- Every wave-attrition number carries the `LIMITATIONS.md` §1 caveat. Relative rankings
  within one model are fine and are what this task uses; absolute survivor counts are
  not trustworthy and must not be presented as if they were.
- If the verdict is "monoculture", the doc ends with the specific, smallest data change
  that would fix it (a `targets_per_shot` rebalance, a diminishing-returns curve on
  cleave, a scenario-mix requirement — whichever the numbers actually point at). **Name
  it, do not apply it.** Changing the balance data is a gameplay-director decision and a
  separate task.

## Spawn prompt

```
You are executing task-091. You are the sim-director for Kindled (the game is named
Kindled, NOT Emberkeep — Emberkeep is a discarded working title still present in ~97
files including some skill and task files; never take the name from those).

task-090 has closed before you start: weapon×projectile pairings are now constrained in
docs/data/hero-builds.json and Scripts/sim/variety.py rolls against that constraint.
Read what it changed before you begin — your measurement is of the CORRECTED space, and
if you re-derive the old numbers you will be measuring builds that no longer exist.

THE QUESTION, and it is the owner's own: the point of the hero-build variety layer is
that CHARACTERS WILL BE UNIQUE IN THE GAME. Does the build space actually deliver that?

The finding that prompted this, measured on committed data before task-090 landed —
rolling 20 builds against floor1-swarm-wave at seeds 1, 3, 7, 11, 42:

    Cleave Melee Sweep took rank 1 on ALL FIVE seeds, at 24-48% of total roster damage.

Mechanistically obvious: Cleave Melee Sweep is the only weapon with targets_per_shot: 8.
Everything else in hero-builds.json's weapon_archetypes sits at 1-4. Against a 250-enemy
wave that is worth 2-8x on that axis alone.

TWO COMPETING EXPLANATIONS. Separating them is the entire job:

  (1) THE SPACE IS BROKEN. The build space collapses to "did you roll cleave" and the
      other ~998 builds are strictly worse. If true, wiring this into the game would
      ship a monoculture.

  (2) THE SCENARIO IS THE WHOLE STORY. Cleave SHOULD dominate a 250-enemy melee blob.
      The space may differentiate perfectly well once the fight is not a swarm. If true,
      the finding is about scenario coverage, not about the build data at all.

DO NOT ASSUME EITHER. The measurement decides.

HOW TO DRIVE IT — read this before you write code. variety.py ALREADY does everything
you need per-roll: it takes --seed, --scenario, --roster, --count-per-build and --json.
So write Scripts/sim/differentiation.py as a THIN DRIVER that calls it in a loop over
seeds and aggregates the JSON. Do NOT modify Scripts/sim/variety.py — task-090 owns that
file and you would be overwriting its work. Check `py Scripts/sim/variety.py --json`'s
actual output shape first; do not assume the key names.

WHAT TO MEASURE, into docs/sim/DIFFERENTIATION.md:

1. At least 20 seeds, not five. Five was a smell test; it is not a distribution.
2. Damage-share concentration: how dominant is rank 1, and how much of the rolled roster
   is competitive with it. State the band you chose and why.
3. Rank-1 frequency per weapon archetype across all seeds. If cleave is 20/20, say 20/20.
4. THE LOAD-BEARING COMPARISON: run the same measurement against a POINT-TARGET scenario,
   not only a wave scenario. This is what separates explanation (1) from (2) and it is the
   reason this task exists. Use the existing docs/data/scenarios/floor2-elite-point-target
   or floor3-boss-point-target. Add new scenario files ONLY if the existing library truly
   cannot express the case — and if you add any, name them
   docs/data/scenarios/hero-build-differentiation-*.json (that glob is what you own).

MODEL TRUST — get this right, it is the most common way sim work here goes wrong:

  - The POINT-TARGET model is VALIDATED. It reproduces entity-tiers.md §7's table
    exactly. Numbers from it can carry weight.
  - The WAVE-ATTRITION model is NOT. docs/sim/LIMITATIONS.md §1: it does not reproduce
    GATE1's measured 110-of-120 wave-1 survival and predicts a full wipe instead. Every
    wave-attrition number you print MUST carry that caveat. RELATIVE rankings inside one
    model are fine and are exactly what you are using. ABSOLUTE survivor counts are not
    trustworthy and must not be presented as though they were.
  - DO NOT tune docs/data/scenarios/combat-model-constants.json to make anything land
    nicer. LIMITATIONS.md §1 forbids it by name. You do not own that file anyway.

THE DELIVERABLE IS A VERDICT, not a table dump. End with a plain-words answer:
differentiated, monoculture, or scenario-dependent — and if scenario-dependent, name
what it depends on and show the numbers on both sides.

IF THE VERDICT IS MONOCULTURE: end the doc with the single smallest data change that
would fix it — a targets_per_shot rebalance, a diminishing-returns curve on cleave
against large populations, a scenario-mix requirement, whichever the numbers actually
point at. NAME IT, DO NOT APPLY IT. Balance data is a gameplay-director decision and a
separate task. You are measuring, not tuning.

YOU OWN: docs/sim/DIFFERENTIATION.md, Scripts/sim/differentiation.py,
docs/data/scenarios/hero-build-differentiation-*.json

DO NOT TOUCH: Scripts/sim/variety.py (task-090 owns it — drive it via its CLI instead),
docs/data/hero-builds.json (task-090 just settled it — if you believe its
data is wrong, SAY SO in your handback, do not edit it), docs/data/unit-types.json,
docs/data/entity-tiers.json, docs/data/scenarios/combat-model-constants.json, any
existing scenario file, Scripts/sim/combat_model.py, Scripts/sim/validate.py,
docs/sim/LIMITATIONS.md, docs/sim/VALIDATION.md, SYSTEMS.md, GDD.md, or anything
under ELVTR/.

If you need a change in combat_model.py to measure something, DO NOT make it — describe
it in the handback and note what you measured instead. A borrowed edit to a shared model
file is how two sessions overwrite each other here.

HAND BACK: the verdict in one sentence first, then the seed count, the concentration
numbers for both scenario types side by side, the rank-1-frequency table, and — if the
verdict is monoculture — the one named data change you are recommending but did not make.
```
