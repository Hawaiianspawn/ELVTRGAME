---
id: 086
title: Find the fun in retinue sub-types — give the five kept knight silhouettes candidate stat identities and measure where the spread actually separates
status: done
agent: sim-director
model: sonnet
owns: ["Scripts/sim/data_loader.py", "Scripts/sim/variety.py", "docs/data/scenarios/retinue-subtypes.json", "docs/data/scenarios/retinue-subtype-spread.json", "docs/sim/SUBTYPE-VARIETY.md"]
resources: []
depends-on: []
epic: retinue-identity
evidence: A roll-and-rank report over candidate melee sub-type profiles derived from the five kept knight silhouettes, run through the committed harness against an existing scenario's enemy side unchanged, showing measured per-profile separation with the LIMITATIONS §1 wave-attrition caveat stated inline — plus a named recommendation of which profile spread is worth adopting and which candidate axes collapsed to no measurable difference — and reproducible, in that a stated command re-runs it from disk to the same numbers on the same seed.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: retinue-subtypes
decided: "2026-07-29 done"
---

## Why now
Owner, 2026-07-29: *"Now that the retinue the characters will be more unique in their
appearence and stats/attack."* The appearance half just landed — `task-082` closed today with
**five kept knight silhouettes** measured and recorded. The stats half does not exist. The
typed retinue model ships exactly **two** types:

    spearmen   max_hp 130  dps 30  engage 95   targets_per_hit 8
    archers    max_hp  70  dps 18  engage 750  targets_per_hit 1

Both are `"0.1-prototype"` dials in `docs/data/unit-types.json`, self-described as *"first
pass, unmeasured"*. So five visually distinct knights would all fight as the same `spearmen`
row. This task answers the question before anyone commits balance to it: **if melee splits
into five sub-types, does that produce measurably different fights, or does it collapse?**

This runs now because it needs no editor. `task-084` holds `unreal-editor` for the brood
legibility work; this is pure Python against committed data.

## What this task is NOT
**It does not decide shipped balance.** `sim-director`'s charter is explicit — never edits
`SYSTEMS.md`, never edits `docs/design/`, never invents balance decisions. Candidate profiles
here are *experiment inputs*, written to `docs/data/scenarios/retinue-subtypes.json` and
clearly labelled as candidates. `docs/data/unit-types.json` is `gameplay-director`'s file and
is **off limits**. If the result is worth shipping, that is a follow-on task for
`gameplay-director` — file it, do not do it.

## The silhouettes to derive from — already measured, do not re-measure
From `docs/data/art/families/knight-melee-v1/manifest.json`:

| variant | aspect | solidity | asymmetry | holes | mass (px) |
|---|---|---|---|---|---|
| v1_narrowguard | 0.70 | 0.70 | 0.08 | 0 | 960 |
| v2_lanceout | 1.05 | 0.53 | 0.39 | 0 | 1081 |
| v3_shieldbreak | 0.95 | 0.61 | 0.74 | 0 | 1124 |
| v4_overhead | 1.05 | 0.52 | 0.17 | 2 | 1012 |
| v6_simplecolumn | 0.68 | 0.75 | 0.27 | 0 | 986 |

The interesting part is that the *shapes* suggest their own stat identities, and that mapping
is the hypothesis worth testing rather than assuming:

- **v2_lanceout** — widest reach, a rigid prop held level. Longer `engage_range`, fewer
  `targets_per_hit`? Or the reverse — a level lance sweeps a line, so *more* targets?
- **v3_shieldbreak** — asymmetry 0.74, a braced shield-out front. More HP, less DPS.
- **v4_overhead** — two-handed, arms enclosing a hole. High DPS, low HP, slow.
- **v1_narrowguard / v6_simplecolumn** — narrow and dense (solidity 0.70/0.75). Cheap line
  filler, or the durable anchor?

**Test the mapping, do not assume it.** A result saying "three of these five are
indistinguishable at any reachable stat spread" is a genuinely useful answer and should be
reported as one, not tuned away.

## The lazy route first — check before writing code
`Scripts/sim/data_loader.py:140` already carries a generic fallback for a type with no tier
ladder: *"no tier ladder for this type; used unit-types.json flat stats as-is"*. Scenarios
reference retinue by `UnitType` name + `Tier`, so **check whether a candidate sub-type can
already flow through that path with zero loader changes** before adding any. If it needs a
change, keep it to reading the candidate file — do not restructure the loader.

`Scripts/sim/variety.py` already does exactly this shape of work for hero builds (roll a
legal roster, run it as the retinue side of an existing scenario, report an ASCII ranked
table). **Reuse that pattern — a `--subtypes` mode or its equivalent — rather than writing a
second roll-and-rank.** Read its docstring and `docs/sim/VARIETY.md` first.

## The evidence caveat is not optional
Melee retinue versus a swarm is **wave attrition**, and `docs/sim/LIMITATIONS.md` §1 states
the harness does not reproduce GATE1's measured 110-of-120 survival at committed defaults. So:

- A survivor count is **never** a standalone claim. `variety.py`'s own docstring has the
  correct framing: the table ranks profiles **relative to each other inside one shared,
  imperfect model** — never an absolute claim about the real game.
- State that caveat inline in the report, not in a footnote.
- Stances, leash, supply/degrade, items, knockback and positioning are **not modelled at all**
  (§4). If a candidate profile's whole identity rests on one of those, say so and mark it
  unanswerable here rather than producing a number that pretends otherwise.

## Done when
- `docs/data/scenarios/retinue-subtypes.json` holds candidate melee sub-type profiles, one per
  kept silhouette, each with a stated derivation from its measured shape.
- A run over those profiles against an existing scenario's **unchanged** enemy side reports
  measured per-profile separation, with the §1 caveat stated inline.
- The report names which candidate axes produced real separation and which collapsed to noise.
- A stated command reproduces the identical numbers from disk on the same seed.
- `docs/sim/SUBTYPE-VARIETY.md` records the method, the caveat, and the recommendation.
- A follow-on `gameplay-director` task is *filed* (not executed) if the spread is worth
  shipping into `unit-types.json`.

## Scope fence
- **No canon writes.** Not `docs/data/unit-types.json`, not `SYSTEMS.md`, not `docs/design/**`.
- Not `Scripts/sim/combat_model.py`, `scenario_runner.py`, `validate.py`, or
  `docs/data/scenarios/combat-model-constants.json` — `task-076` owns those for the seeded
  variance layer. Do not touch the model constants to make a spread look better.
- Not the existing scenarios. Add new ones; run against an existing enemy side unchanged so
  the comparison is like-for-like.
- No editor, no PIE, no build. `task-084` holds `unreal-editor`.
- No new dependencies. `random.Random(seed)` only, never global random.

## Spawn prompt
```
You are finding the fun in Emberkeep's retinue sub-types (C:\Projects\ELVTRGAME). Read
docs/backlog/task-086-retinue-subtype-stat-identity-sim.md IN FULL FIRST, then
docs/sim/LIMITATIONS.md and docs/sim/VARIETY.md before you trust any number you produce.

THE QUESTION: task-082 closed today with FIVE kept knight silhouettes, all measured. But the
typed retinue model ships exactly two types — spearmen (hp 130 / dps 30 / engage 95 /
targets_per_hit 8) and archers (hp 70 / dps 18 / engage 750 / targets 1) — both self-described
in docs/data/unit-types.json as "0.1-prototype, first pass, unmeasured". So five visually
distinct knights would all fight as the same spearmen row. Does splitting melee into five
sub-types produce measurably different fights, or does it collapse to noise?

THE SILHOUETTES, already measured — read them from
docs/data/art/families/knight-melee-v1/manifest.json, do not re-measure:
  v1_narrowguard   aspect 0.70  solidity 0.70  asym 0.08  holes 0  mass  960
  v2_lanceout      aspect 1.05  solidity 0.53  asym 0.39  holes 0  mass 1081
  v3_shieldbreak   aspect 0.95  solidity 0.61  asym 0.74  holes 0  mass 1124
  v4_overhead      aspect 1.05  solidity 0.52  asym 0.17  holes 2  mass 1012
  v6_simplecolumn  aspect 0.68  solidity 0.75  asym 0.27  holes 0  mass  986
Derive a candidate stat profile per silhouette and STATE the derivation. Then TEST the mapping
rather than assuming it — "three of these five are indistinguishable at any reachable spread"
is a genuinely useful answer and must be reported as one, not tuned away.

DO THE LAZY THING FIRST. Scripts/sim/data_loader.py:140 already has a generic fallback for a
type with no tier ladder ("used unit-types.json flat stats as-is"). Check whether a candidate
sub-type flows through that path with ZERO loader changes before you add any. And
Scripts/sim/variety.py ALREADY does this exact shape of work for hero builds — roll a legal
roster, run it as the retinue side of an existing scenario, print a ranked ASCII table. Reuse
that pattern; do not write a second roll-and-rank.

THE CAVEAT IS NOT OPTIONAL. Melee-vs-swarm is WAVE ATTRITION, and docs/sim/LIMITATIONS.md §1
states this harness does NOT reproduce GATE1's measured 110-of-120 survival at committed
defaults. variety.py's own docstring has the correct framing: the table ranks profiles
RELATIVE TO EACH OTHER inside one shared, imperfect model — never an absolute claim about the
real game. State that inline in your report, not as a footnote. Stances, leash, supply/degrade,
items, knockback and positioning are NOT MODELLED (§4) — if a candidate's whole identity rests
on one of those, mark it unanswerable here instead of producing a number that pretends.

YOU DO NOT DECIDE BALANCE. Your charter forbids it. Candidate profiles go in
docs/data/scenarios/retinue-subtypes.json labelled as candidates. docs/data/unit-types.json is
gameplay-director's file and is OFF LIMITS, as are SYSTEMS.md and docs/design/**. If the spread
is worth shipping, FILE a follow-on gameplay-director task and say so — do not do it yourself.

DO NOT TOUCH:
  - Scripts/sim/combat_model.py, scenario_runner.py, validate.py, or
    docs/data/scenarios/combat-model-constants.json — task-076 owns those. Above all, do not
    touch the model constants to make a spread look better.
  - The existing scenario files. Add new ones, and run against an existing enemy side
    UNCHANGED so the comparison is like-for-like.
  - The editor. task-084 holds the unreal-editor lock — no PIE, no build, no MCP. Pure Python.

DONE WHEN:
  - Candidate profiles exist with a stated derivation from each measured shape.
  - A run reports measured per-profile separation with the §1 caveat inline.
  - The report names which axes separated and which collapsed to noise.
  - A stated command reproduces identical numbers from disk on the same seed.
  - docs/sim/SUBTYPE-VARIETY.md records method, caveat and recommendation.

No new dependencies. random.Random(seed) only, never global random. The tree is shared with
concurrent sessions — build on uncommitted work you find, do not revert it, do not attribute it.

HANDBACK: report to the lead with (a) the ranked separation table, (b) the reproduce command,
(c) which axes collapsed, (d) your recommendation and the follow-on task id you filed. Do not
change this task's status — the lead owns the closing transitions.
```
