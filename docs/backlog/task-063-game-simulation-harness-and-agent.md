---
id: 063
title: Build a committed JSON simulation harness and the agent that drives it
status: done
agent: claude
model: sonnet
owns: ["Scripts/sim/**", "docs/sim/**", "docs/data/scenarios/**", ".claude/agents/sim-director.md"]
resources: []
depends-on: []
epic: ""
evidence: A committed harness that reproduces `entity-tiers.md` §7's stated numbers and GATE1's measured 110-of-120 survival from `docs/data/*.json` alone, plus at least three named wave scenarios run end to end with their output tables, and a written statement of which questions the model is and is not trustworthy for.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: sim-harness
decided: "2026-07-29 done"
---

## Why now

Three separate tasks have each hand-rolled a throwaway simulation and thrown it away.
task-003 says so in its own §7: *"a scratch Python script (session scratchpad, not
committed)"* — and one of the two models it built was **discarded outright** because a
pooled Lanchester attrition sim predicted a 100% retinue wipe on floor 1 where
`GATE1-FUN-PROTOTYPE.md` had *measured* 110 of 120 surviving. That failure was caught
only because someone happened to check it against a measured number; the next scratch
sim has no such guarantee. task-006's evidence bar is a Monte-Carlo of the drop table.
task-004 (score 15.0, the highest-ranked item in the backlog) is blocked on exactly the
concurrent-spawn model 003 said it did not have and could not build.

The data to do this properly is already committed and consistent: `entity-tiers.json`,
`unit-types.json`, `economy.json`, `upgrades.json`, `growth-sites.json`,
`scaling-curve.json`, `squads.json`, `feeding.json`. What is missing is a harness that
reads them, a scenario format, and a validation suite that stops a wrong-in-kind model
being reported as a number.

Nothing breaks in play while this stays undone — it is tooling, and it is scored as
tooling. What it changes is that design answers stop being disposable.

## Done when

- `Scripts/sim/` holds a committed, runnable harness that loads every `docs/data/*.json`
  it depends on, with no numbers hardcoded that already live in those files.
- A **scenario format** exists under `docs/data/scenarios/` with a schema doc, so a
  scenario is data the owner can edit, not Python someone has to rewrite.
- A **validation suite** reproduces at least two already-measured numbers before any new
  result is trusted: `entity-tiers.md` §7's Militia-vs-Fodder TTK of 2.00s and
  Hero(55dps)-vs-Elite of 21.60s, and GATE1's measured 110-of-120 survival. A model that
  cannot reproduce the GATE1 number does not ship as a predictor.
- At least three named wave scenarios run end to end with committed output.
- `.claude/agents/sim-director.md` defines the agent that drives it.
- `docs/sim/` states plainly which questions the model answers and which it does not —
  the arrival-timing and spawn-pacing gap that killed 003's swarm model is a known
  limitation and must be written down, not silently modelled around.

## Spawn prompt

```
You are executing task-063 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

GOAL, from the owner in their own words: "we should create an agent that can create and
run json simulations of the game. Since we have all the information on the game we can
calculate through that. Create scenarios and how to manage the waves."

WHY THIS EXISTS — READ THIS FIRST, IT IS THE WHOLE POINT
Design work on this project keeps building throwaway simulations. docs/design/scaling-curve.md
§7 ("Simulation notes") is the case study, and you must read it before writing any code:

  - It built a pooled two-sided Lanchester attrition model (each side's total DPS applied
    proportionally against the other's total HP pool, armor-mitigated per matchup).
  - That model predicted a 100% retinue wipe on floor 1.
  - docs/GATE1-FUN-PROTOTYPE.md had MEASURED the same population: 110 of 120 survive.
  - The model was not slightly off. It was wrong IN KIND — it applied the enemy's entire
    population's DPS simultaneously with no concurrency or arrival-timing limit.
  - It was discarded rather than reported. That was the right call, and it was only
    possible because a measured number existed to check against.

Your job is to make that check automatic and the harness reusable, NOT to finally "solve"
swarm-vs-swarm attrition. If your model cannot reproduce GATE1's measured number, the
correct output is a harness that says so loudly, not a tuned fudge factor that hits 110.

WHAT YOU ARE BUILDING

1. Scripts/sim/ — a committed Python harness. Plain stdlib, run with `py`, no new deps
   without saying why in the handback. It loads its inputs from docs/data/*.json and
   hardcodes NOTHING that already lives there. If you find yourself typing a stat block,
   stop — read it from entity-tiers.json.

2. docs/data/scenarios/ — a scenario FORMAT plus a schema doc, following the convention
   the repo already uses for data (see docs/data/feeding.json + feeding.schema.md as the
   pattern to match). A scenario is data the owner can edit by hand. Do not make scenarios
   Python. At least three named wave scenarios, and they should cover the cases the design
   docs actually argue about: a floor-1 swarm wave, a floor-2 wave with the ranged threat
   introduced, and a point-target fight (Elite or Boss).

3. A VALIDATION SUITE, and it gates everything else. Before any new result is reported,
   the harness must reproduce these already-known numbers from the committed data:
     - Militia vs Fodder TTK = 2.00s        (docs/design/entity-tiers.md §7)
     - Hero(55dps) vs Elite TTK = 21.60s    (docs/design/entity-tiers.md §7)
     - GATE1's measured survival: 110 of 120 at that doc's stated population
       (docs/GATE1-FUN-PROTOTYPE.md — read it for the exact setup; do not guess it)
   The first two are a closed-form check that scaling-curve.md §7 already passed, so they
   should be straightforward. The GATE1 one is the hard one and it is the one that matters.

4. .claude/agents/sim-director.md — the agent definition that drives this. Match the
   frontmatter shape of the existing agents in .claude/agents/ (read two of them first).
   It needs Read/Glob/Grep/Write/Edit/Bash so it can actually run the harness. Its
   description should make clear it runs and interprets simulations against committed
   data — it is not a second gameplay-director and must not own SYSTEMS.md or docs/design/.

5. docs/sim/ — how to run it, the scenario format, and a plainly-worded section on what
   this model is NOT good for. The arrival-timing / spawn-pacing gap is a known hole:
   scaling-curve.md §3 and §4 both flag it, and the per-floor encounter budget table that
   would carry that data (RTS-VERTICAL-SLICE.md §4) is still unbuilt. Write the limitation
   down. Do not model around it silently.

YOU OWN EXACTLY:
  Scripts/sim/**
  docs/sim/**
  docs/data/scenarios/**
  .claude/agents/sim-director.md

DO NOT WRITE ANYTHING ELSE. Specifically do not touch: SYSTEMS.md, GDD.md, CLASSES.md,
any existing docs/data/*.json (you READ them — they are other tasks' outputs and two of
those tasks are live right now), docs/design/**, ELVTR/Source/**, ELVTR/Content/**.
If the harness reveals that a committed number is wrong, SAY SO IN THE HANDBACK. Do not
edit the file.

CANON WARNINGS
- Do NOT read WORLD.md. It is superseded by docs/narrative/FLAME-FOUNDATION.md
  (total narrative reset 2026-07-22).
- The 4-value colour gate is superseded (2026-07-28); the game ships in full colour.
  Irrelevant to you, but ignore any doc that leans on it.
- docs/perf/niagara-sprite-refactor.md §2/§8.1 carry a retracted GPU-sim claim. Ignore.

HAND BACK: the validation suite's actual output (pass or fail, with the numbers), the three
scenarios' output tables, and a straight answer to "what is this harness not trustworthy
for yet". If the GATE1 reproduction fails, hand back the failure and your read on why —
that is a genuinely useful result and it is much better than a fudged pass.
```
