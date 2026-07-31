---
id: 082
title: Run a Knight melee variant family through the codified pipeline and report it
status: done
agent: claude
model: sonnet
owns: ["RawArt/Renders/knight-melee-v1/**", "docs/data/art/families/knight-melee-v1/**"]
resources: ["pixellab-credits"]
depends-on: [81]
epic: unit-variant-automation
evidence: A published Artifact contact sheet of the knight-melee-v1 family showing each variant beside its flat outline, the variantpipe judge table with the measured separation across aspect/solidity/asymmetry/holes, a stated keep-or-reject verdict and reason per variant recorded in the family manifest, the generations actually spent, and every reject moved to rejected/ rather than deleted.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: knight-melee
decided: "2026-07-29 done"
---

## Why now
Owner, 2026-07-29, asking for automated per-unit-type variety: *"We have a ton of oozes, and a
decent amount of archers as examples."* The gap is the melee line. `archers` is covered —
`archer-scifi` is six measured states and the owner's call this session was to record its
verdicts rather than regenerate it (`task-081`). The oozes are covered and then some. The
knight families on disk are the ones that **measurably failed**:

- The shipped knight group: states 01, 02, 05 and 06 have an **identical 1,093-pixel
  outline**. Only 03 and 04 move it, and by ~3%. Knight 06 came back byte-identical to 05
  because the axis had run out.
- `knight-topology`: aspect spread 1.5×.
- `knight-primitive`: aspect spread 3.2× — but three of its six (Dome, Bar, Triangle) came
  back as a metal egg, a gold-trimmed plaque and a shield. Furniture, not units.

So the melee line currently has either one silhouette repeated or six shapes half of which
stopped being soldiers. This is the run that fixes it, and it is the first real exercise of
`task-081`'s pipeline on new generations.

Mage is deliberately **not** in this batch — owner's call, 2026-07-29. There is no mage base
character and no `mage` unit type in the sim (`SwarmCombat.h:49` ships only `Spearmen` and
`Archers`). It is filed as a follow-on for once a caster gameplay type exists.

## Findings — the recipe is already measured, do not rediscover it
The knight base is a **skeletal humanoid**, and the skill has three separate measured failures
from pushing it the wrong way:

- **A humanoid will not go squat. Stop asking.** "Shieldwall crouch" got aspect 1.02 against a
  1.30 target; "heavy skirt plates" 0.82 against 1.00; "squat immovable heavy" 0.89 against
  1.30 and came back with solidity identical to the base's across all eight facings. The same
  wording made the ooze hit 1.56 first try. The skeleton will not compress vertically.
- **What works on a skeleton is narrow ↔ wide-by-extension.** Measured on the same base:
  *narrow* — arms tucked, shield dropped, extended upward — reached aspect **0.58 at 28 px**
  and held 0.46–0.58 across every rotation. *Wide* — a rigid prop held out rather than an
  inflated body — a spear held level got **53 px**, the widest knight produced all session, at
  solidity 0.45 and rotation drift of just 0.10.
- **Simplify the outline, never the interior.** Stripping to one nameable primitive spread
  aspect 3.2× against 1.6× for adding gear, and every protrusion added is a new thing to
  foreshorten. But the ceiling is hard: **simplify to the simplest shape that still keeps a
  visible head and legs.** Square and Column survived; Dome, Bar and Triangle did not. If you
  cannot name the unit from its outline alone, reject it however good its aspect number is.
- **Do not seed off a prop-carrying variant.** Four states off the plain Vanguard spread 2.1×;
  four states off its tower-shield variant spread **1.3×**, at the do-not-separate threshold,
  because the ~20 px shield anchors the mass in every child. If the chosen knight base carries
  a large dominant prop, plan to remove the prop in the states that need to move.
- **`size` is not the canvas.** Asking `size 92` returned **132×132**. For a 92 px canvas
  request `size 64`. Confirm the returned size on the first variant before queueing the rest.
- **`standard` mode drops props.** It is the only mode with `proportions`, and it silently
  dropped the weapon in 3 of 6 generations and added a glowing visor that was explicitly
  banned. For a family that must match the roster, use `create_character_state` on the
  approved base with `use_color_palette_from_reference=True`.

Account state 2026-07-29: **7,125 of 10,000 generations remaining**, Tier 3 active. A
six-state family at 88–92 px is ~120–240 generations, so roughly 2–3% of the balance.

## Done when
- `docs/data/art/families/knight-melee-v1/spec.json` validates against `task-081`'s schema,
  naming the base character id, its **confirmed** canvas size, the axis, the constant held
  across the family, and a numeric silhouette target per variant.
- Six knight variants generated as `create_character_state` on the confirmed base with
  `use_color_palette_from_reference=True`, every rotation downloaded to
  `RawArt/Renders/knight-melee-v1/raw/<variant>/rotations/` before anything is judged.
- `variantpipe.py judge` run across all eight directions, with the separation stated as
  numbers — not "they look varied".
- A keep-or-reject verdict and reason recorded per variant in the family manifest, with
  generations spent. Rejects **moved** to `rejected/`, never deleted.
- A published Artifact contact sheet, each variant beside its flat outline.
- The family clears the bar the archer line set: no two variants sharing an opaque-pixel
  count, and separation on at least one axis for every pair. Every keeper is nameable as a
  soldier from its outline alone.

## Spawn prompt

```
You are executing task-082. Read docs/backlog/task-082-knight-melee-variant-family.md in
full first. Its "Findings" section is a list of measured failures from previous attempts on
this exact base — every one of them cost real generations, and repeating any of them is the
main way this task goes wrong.

You are the `claude` agent. This task SPENDS REAL MONEY: it holds the pixellab-credits lock
and will spend roughly 120-240 generations of a 7,125 balance. Say your estimate out loud
before queueing a batch.

THE JOB
Build a six-variant knight melee family that provably reads apart at panel scale, driven
through the pipeline task-081 built, and hand back the report.

Read these first:
  .claude/skills/variants/SKILL.md              the workflow and its measured thresholds
  Scripts/art/variantpipe.py                    the driver task-081 built — USE IT, do not
                                                hand-drive the loop or write a parallel script
  docs/data/art/family.schema.json              the spec format your spec.json must validate against
  docs/data/art/families/archer-scifi/          the reference family, judged and verdicted
  docs/art/aesthetic-direction.md               the 2026-07-28 AMENDMENT

YOU OWN, and may write:
  RawArt/Renders/knight-melee-v1/**
  docs/data/art/families/knight-melee-v1/**

DO NOT TOUCH:
  Scripts/art/**                        task-081 owns the tooling. If the driver has a bug,
                                        report it — do not patch it inside this task
  docs/data/art/provenance.json         task-059 is live and holds it
  any other RawArt/Renders/* family     including the existing knight-* families. They are
                                        historical evidence you read, not data you rewrite
  anything under ELVTR/                 this task ENDS AT THE OWNER'S VERDICT. No quantizing,
                                        no SubUV packing, no UE import. /sprite owns packing;
                                        task-059 owns the in-engine atlas state axis
  WORLD.md                              superseded by the 2026-07-22 narrative reset

CANON YOU NEED
- The game ships in FULL COLOUR as of 2026-07-28 (docs/art/aesthetic-direction.md
  AMENDMENT). The old 4-value demichrome gate is SUPERSEDED — do not quantize to four
  values and do not treat off-ramp pixels as a defect.
- Ban glow explicitly in every prompt. aesthetic-direction reserves light for the flame,
  and `standard` mode has already added an unrequested glowing visor once.
- Variants should colour-match the base by default. Do not ask for fewer colours as a way
  of simplifying — "simpler silhouette" means a clean OUTLINE, never stripped interior detail.
- Retention rule: every generation is downloaded and kept. A reject is MOVED to rejected/,
  never deleted, and never before a verdict is recorded.

THE ORDER OF OPERATIONS THAT MATTERS
1. Pick and CONFIRM the base with mcp__pixellab__get_character — canvas size and group_id.
   Prefer a base without a large dominant prop; if the best base carries a big shield, plan
   to remove it in the variants that need to move. Do not queue anything before the returned
   canvas size is confirmed: asking size 92 returns 132x132, and a 132 px sheet is unusable
   next to a 92 px roster. For a 92 px canvas, request size 64.
2. Write the variance table into spec.json BEFORE writing any prompt: which variable each
   variant moves, what all six hold constant, and each one's numeric silhouette target. If
   you cannot say what makes variant 5 different from variant 1 in one measurable phrase,
   do not queue it.
3. The axis on a skeletal humanoid is NARROW <-> WIDE-BY-EXTENSION. Never tall <-> squat —
   three measured attempts all undershot. Narrow = arms tucked, shield dropped, extended
   upward (0.58 at 28 px is the proven return). Wide = a rigid prop held out, not an
   inflated body (a level spear got 53 px). Use topology too — asymmetry did the real work
   in the archer family, and a hole only counts if the background cannot flow into it AND it
   survives downsampling, which in practice means a prop held away from the body.
4. Generate one variant FIRST, confirm its returned canvas and that it still reads as a
   soldier, then queue the rest. Do not queue six and discover the canvas was wrong.
5. Download every rotation before judging anything.
6. Judge across all eight directions, never from the south frame alone. A variant that
   measured aspect 0.86 south ran 0.59-2.60 across its rotations and was the best of its
   batch — read directional drift as the feature it is on a directional form, and as a
   defect only on a form meant to be uniform.

THE REJECT RULE THAT OVERRIDES EVERY METRIC
If you cannot name the unit as a soldier from its outline alone, reject it however good its
aspect number is. Three of six knight-primitive variants measured beautifully and came back
as a metal egg, a plaque and a shield. Distinctiveness bought by destroying the type read is
worth nothing.

HAND BACK
- The published Artifact contact sheet, each variant beside its flat outline.
- The judge table with separation stated as numbers, and the per-variant keep-or-reject
  verdict with its reason.
- Generations actually spent, against your up-front estimate.
- Which variants undershot their stated target, and which you rerolled or dropped. A variant
  that missed is a finding — report it, do not quietly omit it.
- Anything the task-081 driver made harder than hand-driving would have. That is the most
  useful thing this run can produce for the pipeline.
```
