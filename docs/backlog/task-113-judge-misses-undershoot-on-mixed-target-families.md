---
id: 113
title: Fix variantpipe judge silently skipping the undershoot check on mixed-target families
status: proposed
agent: claude
model: ""
owns: ["Scripts/art/variantpipe.py", "Scripts/art/test_variantpipe.py"]
resources: []
depends-on: []
epic: ""
evidence: A regression test in Scripts/art/test_variantpipe.py that fails on today's code and passes after the fix, built from the enemy-armored case below; plus `py Scripts/art/variantpipe.py judge enemy-armored` reporting v3_levelspear as a flag with its band stated.
score: {feel: 1, risk: 1, cost: 1}
source: task-112 handback verification, 2026-07-30
teammate: ""
decided: ""
---

## Why now

`judge` is the gate that decides whether a paid-for variant met its brief, and on
`enemy-armored` it returned **`9 keep, 0 flag`** while one variant had plainly undershot.
The miss was only caught because the executing teammate measured by hand. A gate that
silently passes everything is worse than no gate — the next family's undershoot goes into
the record as a keep.

Measured 2026-07-30, both on the same day and the same code:

| family | targets | v/target | measured band | judge said |
|---|---|---|---|---|
| `brood-tongues` | all numeric | `tongue02_spill` 1.60 | 0.94–1.07 | **flag** ✓ |
| `enemy-armored` | mixed numeric + string | `v3_levelspear` 1.60 | 1.00–1.34 | `keep` ✗ |

`enemy-armored`'s numeric targets that *are* inside their bands (`v1_pike` 0.58 in
0.40–0.71, `v2_column` 0.50 in 0.48–0.76) correctly read `keep`, so the check is not
disabled outright — it is specifically not firing on v3. The schema allows
`silhouette_target` to be a number **or** a string ("a string is a target judge cannot
check numerically and is informational only"), and the mixed case is the untested one.

## Done when

- `Scripts/art/test_variantpipe.py` has a case reproducing the table above — a family whose
  targets mix numbers and strings, with one numeric target outside its band — that fails
  against current `variantpipe.py`.
- `py Scripts/art/variantpipe.py judge enemy-armored` flags `v3_levelspear`, naming its
  1.60 target and its measured 1.00–1.34 band, in the same wording `brood-tongues` gets.
- String targets still pass through as informational and never flag on their own.
- No family's existing verdicts change except the ones that were wrong.

## Spawn prompt

```
You are executing task-113. A bug in Scripts/art/variantpipe.py's `judge`.

REPRODUCE FIRST, before changing anything:
  py Scripts/art/variantpipe.py judge brood-tongues   # tongue02_spill -> flag, correct
  py Scripts/art/variantpipe.py judge enemy-armored   # 9 keep, 0 flag -- WRONG

docs/data/art/families/enemy-armored/family.json gives v3_levelspear a
silhouette_target of 1.6. Its measured band across all 8 rotations is 1.00-1.34
(confirm: py Scripts/art/silhouette_report.py RawArt/Renders/enemy-armored/raw
--all-directions). 1.6 is outside that band, so it must flag exactly the way
tongue02_spill does. It does not.

The difference between the two families is that brood-tongues' silhouette_targets are ALL
numeric, while enemy-armored's are a MIX -- v1/v2/v3 are numbers, v4-v8 are prose strings.
docs/data/art/family.schema.json permits both and says a string is "informational only".
enemy-armored's other numeric targets (v1_pike 0.58 inside 0.40-0.71, v2_column 0.50 inside
0.48-0.76) DO evaluate and correctly return keep -- so the check is not switched off
wholesale. Find the actual cause; do not guess from this description, and do not "fix" it
by making string targets flag.

Read .claude/skills/variants/SKILL.md sections "Judging" and step 6 for what the check is
supposed to mean. Band containment across all 8 rotations is the rule -- never the south
frame alone. SKILL.md's worked example (a variant reading 0.86 south and 2.60 east) is why.

YOU OWN: Scripts/art/variantpipe.py, Scripts/art/test_variantpipe.py

Add the regression case to the existing test file -- assert-based, matching whatever style
is already there. No new test framework, no fixtures directory, no new dependency.

DO NOT TOUCH: any family.json or manifest.json under docs/data/art/families/**, anything
under RawArt/Renders/**, Scripts/art/silhouette_report.py, or any other script. Do not
re-run generation. Do not spend PixelLab credits -- this task holds no credits lock and
needs none; every input it requires is already on disk.

Re-running `judge` on a family REWRITES that family's manifest.json, which you do not own.
Run judge for diagnosis, then `git checkout -- docs/data/art/families/*/manifest.json` to
leave them as you found them, EXCEPT where a verdict legitimately changed because of your
fix -- in which case say so explicitly in the handback rather than restoring it.

HAND BACK: the root cause in one or two sentences, the test you added, judge's before/after
output for both brood-tongues and enemy-armored, and any OTHER family whose verdicts change
once the check fires properly. That last one matters -- a family that was silently passing
may have a miss in its record.
```
