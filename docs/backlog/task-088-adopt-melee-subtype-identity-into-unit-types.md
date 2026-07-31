---
id: 088
title: Decide whether the retinue's melee identity axis should split into sub-types, using task-086's measured candidate spread
status: done
agent: gameplay-director
model: sonnet
owns: ["docs/data/unit-types.json", "docs/design/retinue-melee-subtypes.md"]
resources: []
depends-on: [86, 82, 94]
epic: unique-knights
evidence: A stated decision in docs/design/retinue-melee-subtypes.md on whether melee splits into sub-types and HOW MANY ROWS across task-094's ten-way spread — with an explicit variant-to-stat-row mapping naming, for every one of the ten kept knight silhouettes, which row it fights on and why any two share one — resolving the two judgment calls task-086 flagged as open (v3_shieldbreak reading as highest-DPS under the uniform shape formula against the brief's hand-guess, and v4_overhead's "two-handed" read having no axis backing it) — plus, if adopted, the chosen profiles written into docs/data/unit-types.json as versioned dials with their derivation recorded, and a plainly stated reason for any candidate dropped. Reads task-086's tables as input; does not re-run the harness.
score: {feel: 2, risk: 2, cost: 2}
source: sim-director
teammate: melee-subtype-adopt
decided: "2026-07-30 done"
---

## Why this exists
`task-086` (sim-director) tested whether task-082's five kept knight-melee-v1
silhouettes should carry distinct combat identities, or whether the retinue's
current single `spearmen` type (`docs/data/unit-types.json`) is close enough
that splitting it would be invisible noise. **It is not noise.** Fielding all
seven derived candidate profiles together (five silhouettes plus two explicit
alt-hypothesis variants) against an existing scenario's enemy side, unchanged,
shows a robust **~3.5x spread in per-unit damage output** between the top
candidate (`v3_shieldbreak`, 159 HP / 37.5 DPS) and the bottom
(`v1_narrowguard`, 107 HP / 25.5 DPS) — and that RANK ORDER holds identical
across both tested scenarios and 5 randomized headcount-split seeds. Full
method, numbers, and caveats: `docs/sim/SUBTYPE-VARIETY.md`.

**This is not a balance decision and sim-director's charter forbids making
one.** The candidate profiles in `docs/data/scenarios/retinue-subtypes.json`
are derived from a single stated, uniform, literal shape-axis-to-stat formula
— chosen to be testable, not chosen to be right. Two of the derivation's own
disagreements with the intuitive design read are flagged inline in that file
(`v3_shieldbreak` reads as the highest-DPS candidate under the formula, not
the lowest-DPS "braced defender" the shape suggests at a glance; `v4_overhead`'s
"two-handed" read has no numeric axis backing it at all in the primary
formula). Neither should be taken as settled without a real design pass.

## Amended 2026-07-30 — it is ten silhouettes now, not five
This task was filed against `task-086`'s **seven candidates derived from five** kept
`knight-melee-v1` silhouettes. `task-087` closed the same day with **five more keeps** in
`knight-melee-v2`, and the combined set is measurably better than v1 alone: mass spans
947–1283 = **1.36×**, clearing the 1.3× do-not-separate threshold v1's 1.17× failed, and the
0.70→0.95 aspect gap that made v1 bimodal is filled by 0.75 / 0.77 / 0.85.

`task-094` re-runs `task-086`'s derivation across all ten and is now a hard dependency. **Decide
over its ten-way table, not the seven-way one** — the seven-way result stays in
`SUBTYPE-VARIETY.md` as the record of what was measured then, not as this task's input.

Owner, 2026-07-30: *"I would like to see a fair bit of unique knights in the scene… Each unique
character should have its own base stats."* That is a stated preference for **more sub-types
rather than fewer**, and it is the owner's call — but it does not override the numbers.
`task-094` is expected to name pairs that land indistinguishably close, and shipping two stat
rows the player cannot tell apart is worse than shipping one. **Where a pair collapses, say so
and merge it**; two looks sharing a stat row is a legitimate and expected outcome, and
`task-095` is built for a variant→stat-row mapping precisely so that costs nothing.

## What to decide
0. **How many sub-types**, over the ten-way spread — and for every look that does *not* get its
   own row, which row it shares and why.
1. **Is a melee sub-type split worth shipping at all**, given the measured
   separation is real and stable in this harness? (Read the caveat below
   before answering — the harness's underlying wave-attrition model has its
   own unresolved gap, see `docs/sim/LIMITATIONS.md` §1.)
2. If yes: which of the two disagreements above (v3's dps direction, v4's
   holes-driven hp/dps trade) should resolve toward the shape-derived formula
   vs. the design intuition — this is a genuine judgment call, not something
   the sim harness can adjudicate.
3. If adopted, the actual numbers belong in `docs/data/unit-types.json` as
   new `types` entries (or a sub-type extension of `spearmen` — implementation
   shape is gameplay-director's call), NOT copied verbatim from
   `retinue-subtypes.json`, which is explicitly a candidate/experiment file.

## The caveat that must travel with this decision
Every number above comes from `Scripts/sim/variety.py --mode subtypes`, a
WAVE-ATTRITION result. `docs/sim/LIMITATIONS.md` §1 states this harness does
NOT reproduce GATE1's measured 109-111-of-120 survival at its committed
defaults (it predicts a full wipe instead, and both scenarios this task ran
against also end in a full retinue wipe). **The ~3.5x spread is a RELATIVE
finding inside one shared, imperfect model — a measured comparison of these
seven profiles against each other, not a prediction of how any of them would
play in the real game.** It is real enough to be worth a design look; it is
not evidence any specific number is correct.

## Spawn prompt
```
Read docs/backlog/task-088-adopt-melee-subtype-identity-into-unit-types.md IN
FULL FIRST -- including its 2026-07-30 amendment, which changes what you are
deciding over. Then docs/sim/SUBTYPE-VARIETY.md and
docs/data/scenarios/retinue-subtypes*.json in full.

DECIDE OVER THE TEN-WAY TABLE, NOT THE SEVEN-WAY ONE. task-086 measured a
~3.5x per-unit-damage spread across 7 candidates derived from FIVE kept
knight silhouettes. task-087 then landed FIVE MORE KEEPS, and task-094
re-ran the same uniform derivation across all ten. The seven-way result stays
in SUBTYPE-VARIETY.md as the record of what was measured then -- it is NOT
your input. task-094's ten-way table is.

Owner, 2026-07-30: "I would like to see a fair bit of unique knights in the
scene. That also includes their specific spec sheet on the simulation side.
Each unique character should have its own base stats." That is a stated
preference for MORE sub-types rather than fewer and it is the owner's call --
but it does not override the numbers. task-094 names pairs that land
indistinguishably close, and shipping two stat rows the player cannot tell
apart is worse than shipping one. WHERE A PAIR COLLAPSES, SAY SO AND MERGE IT.
Two looks sharing a stat row is legitimate and expected; task-095 is built
around a variant->stat-row mapping precisely so that costs nothing. For every
look that does not get its own row, name the row it shares and why.

Decide: is this worth shipping as a real melee sub-type split in
docs/data/unit-types.json, and at how many rows? The candidate file flags two
places where its
literal shape-derived formula disagrees with an intuitive design read
(v3_shieldbreak's dps direction, v4_overhead's two-handed hp/dps trade) --
those need a real judgment call, not a re-run of the sim. The LIMITATIONS.md
section 1 caveat travels with every number here: this is a RELATIVE ranking
inside one imperfect model, not a prediction of real-game feel.

docs/data/scenarios/retinue-subtypes.json and docs/sim/SUBTYPE-VARIETY.md are
sim-director-owned reference material -- read them, don't edit them. If you
adopt any of this, the shipped numbers go in docs/data/unit-types.json in
your own words/tuning, not copy-pasted from the candidate file.
```
