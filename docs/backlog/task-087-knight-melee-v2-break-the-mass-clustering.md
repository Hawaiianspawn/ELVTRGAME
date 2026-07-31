---
id: 087
title: Run knight-melee-v2 — break the 1.17x mass clustering and the two-group aspect split that v1 left behind
status: done
agent: claude
model: sonnet
owns: ["RawArt/Renders/knight-melee-v2/**", "docs/data/art/families/knight-melee-v2/**"]
resources: ["pixellab-credits"]
depends-on: []
epic: retinue-identity
evidence: A published Artifact contact sheet of knight-melee-v2 beside v1's five keeps, each variant next to its flat outline, with the variantpipe judge table showing measured aspect/solidity/asymmetry/holes/mass — and specifically whether the combined v1+v2 keep set breaks v1's 1.17x mass spread and its two-group aspect bimodality. A keep-or-reject verdict and reason per variant in the family manifest, the generations actually spent stated against the account balance, and every reject moved to rejected/ rather than deleted.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: knight-melee-v2
decided: "2026-07-29 done"
---

## Why now
Owner, 2026-07-29: *"we can have a variant pipeline test running with the knights to get more
variety testing."* `task-082` closed today with five keeps, and it genuinely worked — but the
measured result has a specific, named weakness that only shows up when you read the whole
table at once rather than variant by variant.

From `docs/data/art/families/knight-melee-v1/manifest.json`:

| variant | aspect | solidity | asymmetry | holes | mass (px) | verdict |
|---|---|---|---|---|---|---|
| v1_narrowguard | 0.70 | 0.70 | 0.08 | 0 | 960 | keep |
| v2_lanceout | 1.05 | 0.53 | 0.39 | 0 | 1081 | keep |
| v3_shieldbreak | 0.95 | 0.61 | 0.74 | 0 | 1124 | keep |
| v4_overhead | 1.05 | 0.52 | 0.17 | 2 | 1012 | keep |
| v5_widecross | 1.09 | 0.53 | 0.54 | 0 | 1215 | **flag** |
| v6_simplecolumn | 0.68 | 0.75 | 0.27 | 0 | 986 | keep |

**Three findings, and they are the brief:**

1. **Mass spread across the five keeps is only 1.17×** (960–1124 px). `task-082`'s own
   findings put **1.3× at the do-not-separate threshold** for aspect. Every kept knight has
   effectively the same pixel mass, which is precisely the property that reads at horde
   density — a hundred of these will look like one blob density, not five unit types.
2. **The aspect spread of 1.53× is really two groups, not five shapes** — two at ~0.69
   (narrowguard, simplecolumn) and three at ~1.0 (lanceout, shieldbreak, overhead). Five
   variants, two silhouette families.
3. **Wide-by-extension underdelivered and topology is nearly unused.** `v5_widecross` aimed at
   aspect 1.60 and landed 1.09 — that is why it flagged. And only `v4_overhead` has any holes
   at all (2). The two axes with the most measured headroom are the two v1 barely moved.

So v2 is not "more knights". It is a targeted pass at the three gaps above.

## Targets — numeric, because the pipeline judges numerically
- Push **mass** deliberately: at least one variant materially below 960 px and one materially
  above 1124, so the combined v1+v2 keep set clears **1.3× mass spread**.
- Land a genuine **wide-by-extension** result where v5 failed — `task-082` measured a level
  spear at **53 px content width**, the widest knight of that session, at solidity 0.45 and
  rotation drift 0.10. That recipe is known to work; v5 just did not reach it.
- Land at least one more **enclosed-hole** topology so holes are a used axis rather than a
  one-off.
- Fill the aspect gap **between 0.70 and 0.95** so the set stops being bimodal.

State the combined v1+v2 spread in the report, not just v2's internal spread. A v2 family that
separates beautifully from itself but sits on top of v1 has achieved nothing.

## The recipe is already measured — do not rediscover it
`task-082`'s findings and `.claude/skills/variants/SKILL.md` carry these at the cost of a full
session each. Re-deriving any of them is waste:

- **A humanoid will not go squat. Stop asking.** "Shieldwall crouch" got 1.02 against a 1.30
  target; "heavy skirt plates" 0.82 against 1.00; "squat immovable heavy" 0.89 against 1.30
  with solidity identical to the base across all eight facings. The same wording made the ooze
  hit 1.56 first try. The skeleton will not compress vertically — **move mass and topology
  instead**, which is exactly what this task needs anyway.
- **What works on a skeleton is narrow ↔ wide-by-extension** — a rigid prop held *out*, never
  an inflated body.
- **Simplify the outline, never the interior**, and the floor is hard: simplify only to the
  simplest shape that still keeps a visible head and legs. Square and Column survived that
  test; Dome, Bar and Triangle came back as furniture and were rejected however good their
  numbers were. **If you cannot name the unit from its outline alone, reject it.**
- **The base carries a shield + polearm** (it is `T_Soldier_Knight` state 01) — a large
  dominant prop, which anchors mass in every child. Variants that need to move mass must drop
  or reposition the prop, the way `v1_narrowguard` and `v4_overhead` did.
- **Full colour throughout** per `docs/art/aesthetic-direction.md`'s 2026-07-28 amendment —
  no quantizing, no "fewer colours" request, and **glow banned explicitly in every prompt**.
  Variants colour-match the base by default; the palette flag alone will not hold it if the
  prompt asks for fewer colours.
- **Use `create_character_state`** on the approved base with
  `use_color_palette_from_reference=True`. `standard` mode is the only one with `proportions`
  and it silently dropped the weapon in 3 of 6 generations and added a banned glowing visor.
- **`size` is not the canvas** — asking `size 92` returned 132×132. This family inherits the
  base's 88 px canvas through `create_character_state`, so the trap does not apply here, but
  confirm the returned size on the first variant before queueing the rest.
- **Never verdict from the south frame alone** — judge across all eight rotations.

Base: character id `1c935515-0ea3-459b-8c63-e7f8cf368272`, 88×88, group
`2cc3ab61-4230-4786-98fa-b94da9e99218`. Confirm live with `get_character` before queueing.

## Budget
Account stood at **7,125 of 10,000 generations** on 2026-07-29, Tier 3. A six-state family at
88 px is ~120–240 generations, roughly 2–3% of the balance. **Default for this task is six
states** — same size as v1. Report actual spend against the balance. Credits are real money;
if a variant is clearly going to fail the outline-nameability test, reject it before spending
the rotations rather than after.

## Done when
- `docs/data/art/families/knight-melee-v2/` validates against `task-081`'s family schema,
  naming the base, its confirmed canvas, the axis, the constant held, and a numeric target per
  variant.
- `py Scripts/art/variantpipe.py judge --json` reproduces the recorded verdict set from disk.
- The report states the **combined v1+v2** aspect and mass spread, and says plainly whether
  1.3× mass separation was reached.
- A published Artifact contact sheet shows v2 beside v1's five keeps, each next to its flat
  outline.
- Every reject is moved to `rejected/`, never deleted (`pixellab-retention-rule`).

## Scope fence
- **Do not touch `knight-melee-v1`** or any other existing family directory — v1 is closed and
  recorded. v2 is additive.
- Not the atlas, `SwarmSheet`, the variant bits, the weight table, `brood-variants.json` or
  `swarm-units.json`. Landing knights in-game is `task-085`, which needs the editor
  `task-084` is holding.
- Not `Scripts/art/variantpipe.py` or `silhouette_report.py` — `task-081` closed those. Use
  them as-is; if one is genuinely broken, report it rather than editing it.
- Not the retinue's stat identity — that is `task-086`, running in parallel. Silhouettes only.
- No editor, no PIE, no build, no import. Generation and measurement only.

## Spawn prompt
```
You are running Emberkeep's knight-melee-v2 variant family (C:\Projects\ELVTRGAME). Read
docs/backlog/task-087-knight-melee-v2-break-the-mass-clustering.md IN FULL FIRST, then
.claude/skills/variants/SKILL.md. Both carry measured findings that cost a full session each —
re-deriving any of them is pure waste.

WHY V2 EXISTS. task-082 closed today with five kept knights and it genuinely worked, but the
table read as a whole has three named weaknesses, and fixing them IS the brief:
  1. MASS SPREAD ACROSS THE FIVE KEEPS IS ONLY 1.17x (960-1124 px). task-082's own findings put
     1.3x at the do-not-separate threshold. Mass is the property that reads at horde density —
     a hundred of these look like one blob density, not five unit types.
  2. The 1.53x aspect spread is really TWO GROUPS, not five shapes: two at ~0.69 (narrowguard,
     simplecolumn) and three at ~1.0 (lanceout, shieldbreak, overhead).
  3. Wide-by-extension UNDERDELIVERED (v5_widecross aimed at aspect 1.60, landed 1.09 — that is
     why it flagged) and topology is nearly unused (only v4_overhead has holes, and just 2).
     The two axes with the most headroom are the two v1 barely moved.

NUMERIC TARGETS:
  - At least one variant materially below 960 px mass and one materially above 1124, so the
    COMBINED v1+v2 keep set clears 1.3x mass spread.
  - A genuine wide-by-extension result where v5 failed. The recipe is measured: a level spear
    got 53 px content width, the widest knight of that session, solidity 0.45, rotation drift
    0.10. v5 just never reached it.
  - At least one more enclosed-hole topology so holes become a used axis.
  - Fill the aspect gap between 0.70 and 0.95 so the set stops being bimodal.
STATE THE COMBINED v1+v2 SPREAD IN YOUR REPORT, not just v2's internal spread. A family that
separates beautifully from itself but sits on top of v1 has achieved nothing.

THE MEASURED RECIPE — do not rediscover:
  - A HUMANOID WILL NOT GO SQUAT. STOP ASKING. "Shieldwall crouch" got 1.02 against a 1.30
    target; "heavy skirt plates" 0.82 against 1.00; "squat immovable heavy" 0.89 against 1.30
    with solidity identical to the base across all eight facings. The same wording made the
    ooze hit 1.56 first try. Move mass and topology instead — which is what this task wants.
  - What works on a skeleton is narrow <-> WIDE-BY-EXTENSION: a rigid prop held OUT, never an
    inflated body.
  - SIMPLIFY THE OUTLINE, NEVER THE INTERIOR, and the floor is hard — simplify only as far as
    still having a visible head and legs. Square and Column survived; Dome, Bar and Triangle
    came back as a metal egg, a plaque and a shield and were rejected however good their
    numbers were. IF YOU CANNOT NAME THE UNIT FROM ITS OUTLINE ALONE, REJECT IT.
  - The base carries a shield + polearm (T_Soldier_Knight state 01) — a large dominant prop
    that anchors mass in every child. Variants that must move mass have to drop or reposition
    it, the way v1_narrowguard and v4_overhead did.
  - FULL COLOUR throughout (docs/art/aesthetic-direction.md, 2026-07-28 amendment). No
    quantizing, no "fewer colours" request, and GLOW BANNED EXPLICITLY IN EVERY PROMPT.
    Variants colour-match the base by default — the palette flag alone will NOT hold it if the
    prompt asks for fewer colours.
  - Use create_character_state on the approved base with
    use_color_palette_from_reference=True. `standard` mode is the only one with `proportions`
    and it silently dropped the weapon in 3 of 6 generations and added a banned glowing visor.
  - `size` is not the canvas (asking 92 returned 132x132). create_character_state INHERITS the
    base's 88 px canvas so the trap does not apply here — but confirm the returned size on the
    first variant before queueing the rest.
  - NEVER VERDICT FROM THE SOUTH FRAME ALONE. Judge across all eight rotations.

BASE: character id 1c935515-0ea3-459b-8c63-e7f8cf368272, 88x88, group
2cc3ab61-4230-4786-98fa-b94da9e99218. Confirm live with get_character before queueing anything.

BUDGET — CREDITS ARE REAL MONEY. Account stood at 7,125 of 10,000 generations on 2026-07-29,
Tier 3. Six states at 88 px is ~120-240 generations. SIX STATES IS THE DEFAULT for this task,
same size as v1. Report actual spend against the balance. If a variant is clearly going to fail
the outline-nameability test, reject it BEFORE spending the rotations, not after. All
generations save to RawArt/Renders/ first and rejects move to rejected/ — NEVER deleted.

DONE WHEN:
  - docs/data/art/families/knight-melee-v2/ validates against task-081's family schema, naming
    base, confirmed canvas, axis, the constant held, and a numeric target per variant.
  - py Scripts/art/variantpipe.py judge --json reproduces the recorded verdict set from disk.
  - The report states the COMBINED v1+v2 aspect and mass spread and says plainly whether 1.3x
    mass separation was reached.
  - A published Artifact contact sheet shows v2 beside v1's five keeps, each next to its flat
    outline.

DO NOT TOUCH:
  - knight-melee-v1 or any other existing family directory. v1 is closed and recorded; v2 is
    additive.
  - The atlas, SwarmSheet, the variant bits, the weight table, brood-variants.json,
    swarm-units.json. Landing knights in-game is task-085 and it needs the editor.
  - Scripts/art/variantpipe.py or silhouette_report.py — task-081 closed those. Use them
    as-is; if one is genuinely broken, REPORT it rather than editing it.
  - Retinue stat identity — that is task-086, running in parallel. Silhouettes only.
  - The editor. task-084 holds the unreal-editor lock. No PIE, no build, no import.

The tree is shared with concurrent sessions — build on uncommitted work you find, do not revert
it, do not attribute it to anyone.

HANDBACK: report to the lead with (a) the judge table, (b) the combined v1+v2 spread and
whether 1.3x mass was reached, (c) the Artifact URL, (d) generations spent against the balance,
(e) what was rejected and why. Do not change this task's status — the lead owns the closing
transitions.
```
