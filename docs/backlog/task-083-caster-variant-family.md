---
id: 083
title: Run a caster variant family — once the roster decides where a caster-silhouette retinue actually lives
status: proposed
agent: claude
model: ""
owns: ["RawArt/Renders/caster-v1/**", "docs/data/art/families/caster-v1/**"]
resources: ["pixellab-credits"]
depends-on: [81]
epic: ""
evidence: A published Artifact contact sheet of the caster-v1 family beside their flat outlines, the variantpipe judge table with separation stated across aspect/solidity/asymmetry/holes, a keep-or-reject verdict and reason per variant in the family manifest, generations spent, and rejects moved to rejected/ — plus, named in the report, which roster class the family was built to serve.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now
Filed as the follow-on the owner dropped from `unit-variant-automation` on 2026-07-29, when
the ask was variant families for Knight, Archer and Mage. Knight became `task-082`, Archer was
covered by recording `archer-scifi`'s verdicts, and **Mage was cut because it has nowhere to
live.**

Stating the blocker precisely, because it is not the one it looks like:

- **There is no `mage` unit type in the sim.** `SwarmCombat.h:49` ships exactly two:
  `Spearmen` and `Archers`. A caster is a third combat behaviour with no existing type to
  borrow — unlike the knight art, which ships against `Spearmen`, or the ooze/retinue art,
  which ships against the two that exist.
- **There is no caster class in the roster either, and that was deliberate.** `CLASSES.md`'s
  v1 roster is Vanguard (melee), Relickeeper (fortifier), Pathfinder (ranged), Lampbearer
  (healer). The support role was *split on purpose* — Relickeeper is prevention (walls, wards,
  damage reduction), Lampbearer is restoration (healing, revives, vision). "Mage" is not a
  deferred fifth slot; it is a word canon does not use.

So this task cannot start on a sprite decision. **It needs an owner call on which existing
class a caster-silhouette retinue belongs to**, and the two candidates are not close:

| Candidate home | The retinue, per CLASSES.md | What that means for this task |
|---|---|---|
| **Lampbearer** | "Guided souls & light-wisps" | **The easy case.** Wisps are *amorphous*, and `/variants` measured that an amorphous base gives states the **full** silhouette range *and* the inherited surface — 3.1× aspect spread on the brood, correct palette, correct canvas. No trade to make. Also the best fit for FLAME-FOUNDATION canon, where light is the premise. |
| **Relickeeper** | "Awakened ancient guardians" | **The hard case.** Robed humanoid casters are a skeletal base, which is the knight problem again: states can only pad the body with gear, the proportion axis collapses 2.4× → 1.2×, and three measured attempts to make a humanoid go squat all undershot. Workable via narrow ↔ wide-by-extension plus topology, but it is a `task-082`-shaped job, not a cheap one. |

That table is the whole reason this is filed rather than built: **the same task is easy or hard
depending on an answer nobody has given.** Nothing breaks while it stays undone — no code
references a caster, no sprite is missing from `asset-matrix.json` on its account.

Do not resolve this by inventing a fifth class. If the honest answer is that neither home fits,
the right outcome is rejecting this task, not generating sprites for a unit the game cannot
spawn.

## Done when
- The owner has named the home — Lampbearer's wisps, Relickeeper's Awakened, or neither.
  **If neither, this task is rejected and no generations are spent.**
- `docs/data/art/families/caster-v1/spec.json` validates against `task-081`'s schema, naming
  the base character id, its **confirmed** returned canvas size, the axis, the constant held
  across the family, and a numeric silhouette target per variant. The report names which class
  the family serves.
- Six variants generated as `create_character_state` on the confirmed base with
  `use_color_palette_from_reference=True`, every rotation downloaded before anything is judged.
- `variantpipe.py judge` run across all eight directions, separation stated as numbers.
- Verdict and reason per variant in the manifest, generations spent recorded, rejects **moved**
  to `rejected/` and never deleted.
- A published Artifact contact sheet, each variant beside its flat outline.

## Spawn prompt

```
You are executing task-083. Read docs/backlog/task-083-caster-variant-family.md in full
first, and read its "Why now" before anything else — this task has a decision gate in front
of it and you must not spend generations before it is answered.

You are the `claude` agent. This task holds the pixellab-credits lock and spends real money
(~120-240 generations of a ~7,000 balance). Say your estimate out loud before queueing.

THE GATE — HANDLE THIS FIRST, DO NOT GENERATE ANYTHING BEFORE IT CLOSES
There is no caster class in CLASSES.md's v1 roster and no caster unit type in the sim
(SwarmCombat.h ships Spearmen and Archers only). Before generating, confirm with the owner
which roster class a caster-silhouette retinue serves:
  - Lampbearer's "guided souls & light-wisps"  -> AMORPHOUS base. States deliver the FULL
    silhouette range with the inherited palette and canvas (3.1x aspect spread measured on
    the brood). For amorphous units, states beat proportion knobs outright — no trade.
  - Relickeeper's "Awakened ancient guardians" -> SKELETAL humanoid base. The proportion
    axis collapses (2.4x -> 1.2x), and a humanoid will NOT go squat — three measured
    attempts undershot (1.02 vs 1.30, 0.82 vs 1.00, 0.89 vs 1.30). The axis here is
    narrow <-> wide-by-extension plus topology, never tall <-> squat. Read task-082's
    Findings section in full if this is the answer; it is the same problem.
  - Neither -> report that and STOP. Recommend rejecting this task. Do not invent a fifth
    class and do not generate art for a unit the game cannot spawn. Stopping here is a
    correct outcome, not a failure.

Read these first:
  CLASSES.md                                    the v1 roster and why support split in two
  docs/narrative/FLAME-FOUNDATION.md            current canon (WORLD.md is SUPERSEDED)
  .claude/skills/variants/SKILL.md              the workflow and its measured thresholds
  Scripts/art/variantpipe.py                    the driver from task-081 — USE IT, do not
                                                hand-drive the loop or write a parallel script
  docs/data/art/family.schema.json              the format your spec.json must validate against
  docs/data/art/families/archer-scifi/          the reference family, judged and verdicted
  docs/backlog/task-082-knight-melee-variant-family.md   its Findings apply verbatim to a
                                                skeletal base — do not rediscover them
  docs/art/aesthetic-direction.md               the 2026-07-28 AMENDMENT

YOU OWN, and may write:
  RawArt/Renders/caster-v1/**
  docs/data/art/families/caster-v1/**
(caster-v1 is a deliberately lore-neutral slug. Keep it whatever the decision names the
unit — the family slug does not need to carry the fiction, and renaming it would mean
editing this task's owns: field.)

DO NOT TOUCH:
  Scripts/art/**                        task-081 owns the tooling. If the driver has a bug,
                                        report it — do not patch it inside this task
  CLASSES.md, GDD.md, SYSTEMS.md        you are not adding a class. If the decision implies
                                        a canon edit, report it for a separate task
  docs/data/art/provenance.json         held by task-059
  any other RawArt/Renders/* family     historical evidence you read, not data you rewrite
  anything under ELVTR/                 this task ENDS AT THE OWNER'S VERDICT. No quantizing,
                                        no SubUV packing, no UE import
  WORLD.md                              superseded by the 2026-07-22 narrative reset

CANON YOU NEED
- The game ships in FULL COLOUR as of 2026-07-28 (docs/art/aesthetic-direction.md
  AMENDMENT). The 4-value demichrome gate is SUPERSEDED — do not quantize to four values,
  and off-ramp pixels are not a defect.
- Ban glow explicitly in every prompt. aesthetic-direction reserves light for the flame.
  This bites hardest here: a caster is exactly the unit a generator wants to make glow, and
  `standard` mode has already added an unrequested glowing visor once. If the family serves
  the Lampbearer, light is the THEME but still not a glow effect on the sprite.
- Variants colour-match the base by default. "Simpler silhouette" means a clean OUTLINE —
  never stripped interior detail, and never asking for fewer colours.
- Retention rule: every generation is downloaded and kept. A reject is MOVED to rejected/,
  never deleted, and never before a verdict is recorded.
- `size` is not the canvas: asking size 92 returned 132x132. For a 92 px canvas request
  size 64, and confirm the returned size on the FIRST variant before queueing the rest.

THE REJECT RULE THAT OVERRIDES EVERY METRIC
If you cannot name the unit from its outline alone, reject it however good its aspect number
is. Three of six knight-primitive variants measured beautifully and came back as a metal
egg, a plaque and a shield. Distinctiveness bought by destroying the type read is worth
nothing. On an amorphous base the equivalent failure is a blob that reads as terrain.

HAND BACK
- Which class the family serves, and the owner exchange that settled it.
- The published Artifact contact sheet, each variant beside its flat outline.
- The judge table with separation as numbers, and the per-variant verdict with its reason.
- Generations spent against your up-front estimate.
- Which variants undershot their stated target. A variant that missed is a finding — report
  it, do not quietly omit it.
```
