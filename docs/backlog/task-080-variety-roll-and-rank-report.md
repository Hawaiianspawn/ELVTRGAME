---
id: 080
title: Roll hero builds in the harness and report metrics plus an ASCII top-10 performers table
status: done
agent: sim-director
model: sonnet
owns:
  - "Scripts/sim/variety.py"
  - "Scripts/sim/data_loader.py"
  - "Scripts/sim/combat_model.py"
  - "docs/sim/VARIETY.md"
resources: []
depends-on: [79]
epic: hero-variety
evidence: >
  `py Scripts/sim/variety.py --scenario floor2-ranged-wave --seed 1` samples
  hero builds out of docs/data/hero-builds.json, evaluates task-079's synergy
  rules against the rolled roster, runs the fight, and prints a metrics block
  plus an ASCII top-10 performers table ranked by per-build contribution;
  `--seed 1` twice is byte-identical output and `--seed 2` differs;
  `py Scripts/sim/validate.py` still passes and `py Scripts/sim/drift_check.py`
  still passes against the UNCHANGED docs/sim/baseline.json, proving the
  combat_model.py change is purely additive.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: variety-report
decided: "2026-07-29 done"
---

## Why now

This is the half of the owner's 2026-07-29 ask that makes task-079's build
space visible: *"After the simulation runs it should give a report, basic
metrics, some ascii results of top 10 performers."* Owner chose **individual
unit builds** as the ranking unit (not army compositions) when asked.

The harness cannot do it today, for two concrete reasons checked 2026-07-29:

1. **No randomization of composition.** `combat_model.py` has no RNG at all.
   A scenario is a fixed list of `WaveGroup`s; running one twice gives the same
   answer. Rolling a roster out of a build space is not a thing the harness can
   express.
2. **No per-group attribution.** `simulate_wave_attrition()` returns only
   aggregate `retinue_survivors` / `enemy_survivors` / `elapsed_seconds` and a
   coarse tick log. Outgoing damage *is* computed per group inside the tick
   loop (`group_output = group_slots * per_unit_dps * dt`) and then thrown
   away. Ranking individual builds needs that number kept.

With hundreds of reachable builds and only a handful in any one fight, sampling
plus ranking is the only way to see the space at all — which is exactly the
report the owner asked for.

## Done when

`py Scripts/sim/variety.py --scenario <name> --seed <n>` prints, in one pass:

- **A metrics block.** The owner's message said "basic metrics below" with no
  list attached, so this task **declares the default set** and `VARIETY.md`
  says it is a default open to change: seed, scenario, builds sampled, roster
  size, active synergies, result, elapsed seconds, hero survivors / start,
  enemy survivors / start, total damage dealt by each side.
- **An ASCII top-10 performers table**, one row per hero build, ranked by
  contribution — damage dealt, share of the roster's total, estimated kills,
  and whether that build survived. Plain-text, aligned columns, terminal-safe
  ASCII only (this repo has already been bitten by non-ASCII in console output
  — `scenario_runner.py` currently prints a mojibake em-dash on this machine).
  Show the winning **axis values**, not just an opaque build id: the point of
  the table is seeing *which weapon archetype and origin* did the work.
- **Reproducibility.** Same seed, byte-identical output. Different seed,
  different roll. Seeded off `random.Random(seed)` and nothing global.
- **Synergies actually applied.** Task-079's rules are evaluated against the
  rolled roster and the granted modifiers are folded into the fighter blocks
  before the fight, with the report naming which rules fired. A report that
  lists synergies without applying them is not done.

And the harness stays trustworthy:

- `py Scripts/sim/validate.py` passes as before.
- `py Scripts/sim/drift_check.py` passes against the **unchanged**
  `docs/sim/baseline.json`. The `combat_model.py` edit is additive only.
- `docs/sim/VARIETY.md` carries the caveat, not buried: this is a
  **wave-attrition** result, and `docs/sim/LIMITATIONS.md` §1 says the
  wave-attrition model does not reproduce GATE1's measured 110-of-120 survival.
  So the ranking is a **relative** comparison between builds run through the
  same model, never an absolute claim about how a build performs in the game.

## Spawn prompt

```
You are the sim-director agent. Build the hero-build variety roll-and-rank
report in the committed Python harness.

You own exactly these paths and must not write anything else:
  Scripts/sim/variety.py        (new — the roll, run and report tool)
  Scripts/sim/data_loader.py    (add a loader for docs/data/hero-builds.json)
  Scripts/sim/combat_model.py   (ADDITIVE per-group attribution ONLY, see below)
  docs/sim/VARIETY.md           (new — how to use it, and the caveats)

Do NOT touch: Scripts/sim/scenario_runner.py, Scripts/sim/validate.py,
Scripts/sim/sweep.py, Scripts/sim/drift_check.py, docs/sim/baseline.json,
docs/sim/MODEL.md, docs/sim/LIMITATIONS.md, docs/sim/VALIDATION.md, anything
under docs/data/ (including hero-builds.json — you READ it, task-079 wrote it),
anything under ELVTR/. Two of those are owned by an unstarted task-076 and
docs/data/ is task-079's; overwriting either is a real collision.

READ FIRST, ALL OF IT, BEFORE WRITING ANY CODE
  docs/sim/README.md            what this harness is and its running rules
  docs/sim/MODEL.md             how both combat models work
  docs/sim/LIMITATIONS.md       §1 especially — read it before you trust a
                                wave-attrition survivor count for anything
  Scripts/sim/combat_model.py   the tick loop you are instrumenting
  Scripts/sim/data_loader.py    the ONLY module allowed to touch docs/data/*.json
  Scripts/sim/scenario_runner.py  the CLI + printing conventions to match
  docs/data/hero-builds.json + docs/data/hero-builds.schema.md
                                task-079's output — the axes, the weapon
                                archetypes, and the synergy rule format you
                                must evaluate. Written by another agent, so
                                read the schema doc, do not assume a shape.

WHAT TO BUILD

`py Scripts/sim/variety.py --scenario <name> --seed <n> [--roster <k>]
 [--json]`

  1. Load docs/data/hero-builds.json through data_loader.py (never open a
     docs/data JSON anywhere else — that module's exclusivity is a stated
     invariant of this harness).
  2. Sample `--roster` hero builds from the combinatorial axis space with
     random.Random(seed). No global random. No numpy — plain stdlib, this
     harness has zero dependencies and keeps it that way.
  3. Resolve each rolled build into the flat fighter dict combat_model.py
     expects — see data_loader.py's existing `retinue_fighter()` for the exact
     shape (max_hp, dps, swing_interval, engage_range, min_engage_range,
     targets_per_hit, armor, role). Map task-079's weapon fields onto it and
     write down the mapping in VARIETY.md; where a field has no home in the
     model (aoe_radius, accuracy), state plainly how you folded it in or that
     you did not, rather than silently dropping it.
  4. Evaluate task-079's synergy rules against the rolled roster and apply the
     granted modifiers to the fighter blocks before the fight. Report which
     rules fired. Do NOT invent, retune or "fix" a synergy rule or a weapon
     stat — you are not a second gameplay director. If a rule is ambiguous or
     unevaluable as written, implement your best reading, flag it in the
     handback, and say so in VARIETY.md.
  5. Take the ENEMY side straight from the named existing scenario in
     docs/data/scenarios/ — the enemy roster stays 6-8 types by owner
     instruction and is not part of this variety layer. One hero build = one
     WaveGroup, which is what gives you per-build resolution for free.
  6. Run simulate_wave_attrition() and print the metrics block and the ASCII
     top-10 table (see "Done when" above for both). --json prints the same
     data as machine-readable output, matching how scenario_runner.py already
     does --json.

THE combat_model.py CHANGE — KEEP IT TINY AND ADDITIVE

  simulate_wave_attrition() already computes each group's outgoing damage per
  tick (`group_output = group_slots * per_unit_dps * dt`, and the ranged
  equivalent) and discards it. Accumulate it into a per-group-name dict and add
  it to the returned dict under a NEW key. Do not rename, reorder, restructure
  or change the value of a single existing return key, and do not change the
  math. `py Scripts/sim/drift_check.py` must pass against the UNCHANGED
  docs/sim/baseline.json — that is the proof the change was additive, and it is
  part of the evidence bar. If you find yourself refactoring the tick loop, you
  have gone too far; that is task-076's territory, not yours.

  Derive "estimated kills" from damage dealt and the enemy tier's max_hp. It is
  an estimate because the model pools HP rather than tracking bodies — label it
  as an estimate in the table header, do not present it as a body count.

CAVEATS THAT MUST LAND IN VARIETY.md, NOT BE BURIED

  This is a WAVE-ATTRITION result. docs/sim/LIMITATIONS.md §1 states the
  wave-attrition model does not reproduce GATE1-FUN-PROTOTYPE.md's measured
  ~110-of-120 wave-1 survival at the harness's committed defaults. Therefore
  the top-10 ranking is a RELATIVE comparison between builds pushed through one
  shared model, and is never an absolute claim about in-game performance. Say
  that in VARIETY.md in plain words, near the top. Also state what the model
  does not represent at all (LIMITATIONS.md §4: stances, leash, positioning,
  knockback, items, multi-wave carryover) so nobody reads a build's rank as a
  verdict on a mechanic the model never simulated.

CONTEXT YOU WOULD NOT OTHERWISE HAVE
  The owner asked for this on 2026-07-29 and chose "individual unit builds" as
  the ranking unit over "whole army compositions" when given the choice. They
  also said, verbatim: "We dont need the editor right now this is all in json
  and simulation space." Nothing here needs Unreal, a build, or PIE.
  "basic metrics below" in the original ask had no list attached — the default
  metric set in "Done when" is this task's declared default, and VARIETY.md
  should say it is a default that is open to change, not a settled spec.

HAND BACK
  - the exact commands you ran and their real output, pasted
  - the top-10 table from at least two different seeds, so the spread is visible
  - proof drift_check.py and validate.py still pass
  - the weapon-field -> fighter-field mapping, and anything you could not map
  - any task-079 synergy rule that was ambiguous, and the reading you took
Then stop. Do not edit any status field in docs/backlog/ — the lead closes it.
```
