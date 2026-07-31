---
id: 094
title: Extend the melee sub-type derivation to all ten kept knight silhouettes and re-run the spread
status: done
agent: sim-director
model: sonnet
owns: ["Scripts/sim/variety.py", "docs/data/scenarios/retinue-subtypes.json", "docs/data/scenarios/retinue-subtypes-v2.json", "docs/sim/SUBTYPE-VARIETY.md"]
resources: []
depends-on: []
epic: unique-knights
evidence: A re-run of `variety.py --mode subtypes` over candidate profiles derived for ALL TEN kept knight silhouettes (knight-melee-v1's five plus knight-melee-v2's five), using the same uniform shape-axis formula already recorded in retinue-subtypes.json, against the same two scenarios with the enemy side unchanged — showing the ranked per-profile separation, whether the ten-way spread still separates or collapses into ties, and which pairs land close enough to be indistinguishable. The LIMITATIONS §1 wave-attrition caveat stated inline, not footnoted. Reproducible from a stated command on a stated seed.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: knight-stat-sheet
decided: "2026-07-30 done"
---

## Why now
Owner, 2026-07-30: *"I would like to see a fair bit of unique knights in the scene. That also
includes their specific spec sheet on the simulation side. Each unique character should have
its own base stats."*

`task-086` answered the sub-type question over **five** silhouettes, because five was all that
existed when it ran. Then `task-087` closed the same day with **five more keeps**. The art half
of the owner's ask is already done — ten measured, judged knight silhouettes sit on disk — but
the stat half covers half of them.

| family | kept | aspect | solidity | asymmetry | holes | mass |
|---|---|---|---|---|---|---|
| v1 | v1_narrowguard | 0.70 | 0.70 | 0.08 | 0 | 960 |
| v1 | v2_lanceout | 1.05 | 0.53 | 0.39 | 0 | 1081 |
| v1 | v3_shieldbreak | 0.95 | 0.61 | 0.74 | 0 | 1124 |
| v1 | v4_overhead | 1.05 | 0.52 | 0.17 | 2 | 1012 |
| v1 | v6_simplecolumn | 0.68 | 0.75 | 0.27 | 0 | 986 |
| **v2** | v7_barestance | 0.75 | 0.65 | 0.06 | 0 | 947 |
| **v2** | v8_heavycloak | 0.98 | 0.68 | 0.38 | 0 | 1283 |
| **v2** | v10_bracedstaff | 1.14 | 0.46 | 0.54 | 0 | 1012 |
| **v2** | v11_midguard | 0.77 | 0.69 | 0.37 | 0 | 1034 |
| **v2** | v13_maceraised | 0.85 | 0.59 | 0.79 | 1 | 1162 |

v2 did what it was filed to do: combined mass now spans **947–1283 = 1.36×**, clearing the
1.3× do-not-separate threshold v1's 1.17× failed, and the 0.70→0.95 aspect gap that made v1
bimodal is filled by 0.75 / 0.77 / 0.85. **The silhouette set is good. It just has no stats.**

## What this task is, and is not
This is `task-086` re-run wider, not a new method. **Use the formula already recorded in
`retinue-subtypes.json`'s `derivation_method` verbatim** — mass→max_hp, solidity→targets_per_hit,
aspect→engage_range, asymmetry→dps, normalised against the `spearmen` baseline (130 HP / 30 DPS /
95 engage / 8 targets). Do not invent a better formula. The formula being uniform and literal is
the entire reason its output is trustworthy as a *comparison*; tuning it per-variant would make
the ranking an opinion.

**It does not decide shipped balance.** `sim-director`'s charter forbids it, and `task-088`
(gameplay-director) is the task that decides. `docs/data/unit-types.json` and `docs/design/` are
off limits here. Candidate profiles are experiment inputs, labelled as such.

## The question that actually matters
`task-086` found a ~3.5× per-unit-damage spread across seven candidates and called it robust.
**Ten candidates is a harder test, and the interesting answer is a negative one:** with ten
profiles drawn from one formula over four axes, some will land on top of each other. Say which,
numerically, and say it plainly.

The report has to answer:

1. **Does the ten-way spread still separate**, or does adding five more candidates compress the
   ranking into a band where adjacent profiles are inside the noise?
2. **Which pairs are indistinguishable** — close enough in `damage_per_unit_committed` that
   shipping both as distinct stat blocks would be a lie the player cannot feel. Name them. This
   is the finding `task-088` most needs, because it is what turns "ten looks" into "N sub-types".
3. **Does the rank order hold** across both scenarios and the randomised headcount splits, the
   way it did at seven? If it stops holding at ten, that is a bigger result than the spread.

Two v1 candidates were explicit alt-hypothesis probes (`v2_lanceout_alt_sweep`,
`v4_overhead_holes_swap`) rather than silhouettes. Keep them, flag them as probes, and rank the
ten real silhouettes separately from them so the ten-way table is not polluted by two synthetic
rows.

## Scope fence
- Scenarios' **enemy side stays untouched**, same as `task-086`. Melee identity axis only.
- Do not re-run or re-judge the art. `variantpipe.py` and the family manifests are read-only
  input; `task-087` closed them.
- Do not touch `docs/data/unit-types.json`, `docs/design/**`, `SYSTEMS.md`.
- No editor, no PIE, no PixelLab credits.

## Spawn prompt
```
You are extending Kindled's melee sub-type stat derivation from five knight silhouettes to ten
(C:\Projects\ELVTRGAME). Read docs/backlog/task-094-subtype-candidates-all-ten-knights.md IN
FULL FIRST, then docs/sim/SUBTYPE-VARIETY.md and docs/data/scenarios/retinue-subtypes.json.

Owner, 2026-07-30: "I would like to see a fair bit of unique knights in the scene. That also
includes their specific spec sheet on the simulation side. Each unique character should have its
own base stats."

WHY THIS EXISTS. task-086 derived and ranked candidate melee sub-type profiles for the FIVE kept
knight-melee-v1 silhouettes. Later the same day task-087 landed FIVE MORE KEEPS in
knight-melee-v2 (v7_barestance, v8_heavycloak, v10_bracedstaff, v11_midguard, v13_maceraised --
read their measured axes from docs/data/art/families/knight-melee-v2/manifest.json). Ten judged
silhouettes now exist and only five have candidate stats.

USE THE EXISTING FORMULA VERBATIM. retinue-subtypes.json's derivation_method already states it:
mass->max_hp, solidity->targets_per_hit, aspect->engage_range, asymmetry->dps, normalised against
the spearmen baseline of 130 HP / 30 DPS / 95 engage / 8 targets. DO NOT invent a better formula
and DO NOT hand-tune any candidate. The formula being uniform and literal is the ONLY reason its
output is trustworthy as a comparison; tuning per-variant turns the ranking into an opinion.
Where the formula disagrees with an intuitive design read, STATE the disagreement in the data
file and leave it unresolved -- that is what task-086 did with v3_shieldbreak's dps direction and
it was correct.

RUN IT the same way task-086 did: py Scripts/sim/variety.py --mode subtypes, all candidates
fielded at once, against gate1-calibration-wave1 AND floor1-swarm-wave, enemy side UNCHANGED,
every non-melee retinue row carried through unchanged. Primary result is the deterministic equal
split (no --seed); then the randomised-split robustness check across the same 5 seeds.

THE REPORT MUST ANSWER THREE THINGS, and the negative answers are the valuable ones:
  1. Does the TEN-way spread still separate, or does it compress into a band where adjacent
     profiles sit inside the noise?
  2. WHICH PAIRS ARE INDISTINGUISHABLE -- close enough in damage_per_unit_committed that shipping
     both as distinct stat blocks would be a difference the player cannot feel. NAME THEM. This
     is the single finding task-088 needs most, because it is what turns "ten looks" into "N
     sub-types".
  3. Does the rank order hold across both scenarios and all 5 seeds the way it did at seven? If
     it stops holding at ten, say so loudly -- that is a bigger result than the spread itself.

The two synthetic alt-hypothesis probes from task-086 (v2_lanceout_alt_sweep,
v4_overhead_holes_swap) stay, but flag them as probes and rank the TEN REAL SILHOUETTES in their
own table so the ten-way comparison is not polluted by two synthetic rows.

THE CAVEAT TRAVELS INLINE, NOT AS A FOOTNOTE. docs/sim/LIMITATIONS.md section 1: this harness
does NOT reproduce GATE1's measured 109-111-of-120 survival at committed defaults -- it predicts
a full wipe. Every number you produce is a RELATIVE comparison of these profiles against each
other inside one shared imperfect model, NOT a prediction of real-game feel. State that in the
report body where the tables are, the way SUBTYPE-VARIETY.md already does.

SCOPE FENCE, hard:
  - You own ONLY Scripts/sim/variety.py, docs/data/scenarios/retinue-subtypes*.json, and
    docs/sim/SUBTYPE-VARIETY.md.
  - docs/data/unit-types.json, docs/design/**, SYSTEMS.md are OFF LIMITS. Your charter forbids
    balance decisions and task-088 (gameplay-director) makes this one. Candidate profiles are
    EXPERIMENT INPUTS, labelled as such.
  - Do not re-run, re-judge or edit the art. The family manifests and variantpipe.py are
    read-only input; task-087 closed them.
  - No editor, no PIE, no PixelLab credits.

DONE WHEN: a stated command re-runs your result from disk to the same numbers on the same seed,
the ten-way ranked table is in SUBTYPE-VARIETY.md alongside task-086's seven-way one (add, do not
overwrite -- the seven-way result is the record of what was measured then), and the
indistinguishable-pairs finding is stated explicitly rather than left for a reader to infer from
the table.

Hand back with: the ten-way table, the answer to all three questions above, the exact reproduce
command, and anything you had to assume. Do not mark the task done -- the lead does that.
```
