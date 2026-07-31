---
id: 081
title: Codify the variant-family workflow into a driver — declarative family spec, machine-readable judging, auto-culled rejects
status: done
agent: claude
model: sonnet
owns: ["Scripts/art/variantpipe.py", "Scripts/art/silhouette_report.py", "docs/data/art/family.schema.json", "docs/data/art/families/archer-scifi/**", "docs/data/art/families/archer-proxy/**", "docs/data/art/families/knight-topology/**", "docs/data/art/families/knight-primitive/**", "docs/data/art/families/knight-mass/**", "docs/data/art/families/knight-types/**", "docs/data/art/families/brood-ooze/**", "docs/data/art/families/brood-ooze-colour/**", ".claude/skills/variants/SKILL.md", "RawArt/Renders/archer-scifi/**", "RawArt/Renders/archer-proxy/**", "RawArt/Renders/knight-topology/**", "RawArt/Renders/knight-primitive/**", "RawArt/Renders/knight-mass/**", "RawArt/Renders/knight-types/**", "RawArt/Renders/brood-ooze/**", "RawArt/Renders/brood-ooze-colour/**"]
resources: []
depends-on: []
epic: unit-variant-automation
evidence: A published Artifact contact sheet for the existing families showing each variant beside its flat outline with the verdict table, every one of archer-scifi's six states carrying a recorded keep-or-reject verdict and a stated reason in its family manifest, a named cull list of what moved to rejected/, and `py Scripts/art/variantpipe.py judge --json` reproducing that identical verdict set on a re-run from disk.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: variant-pipeline
decided: "2026-07-29 done"
---

## Why now
Owner, 2026-07-29: *"I want to automate the variant characters we can create per unit
type... We will have to cut out some bad data that might get made but we have refined this
workflow pretty effectively for the ranged. I would like reports on new ones we make."*

The workflow is genuinely refined — but it is refined **in prose**, inside
`.claude/skills/variants/SKILL.md`, and re-executed by hand every time. Three things make
that unrepeatable:

- **Nothing on disk is machine-readable.** `silhouette_report.py` prints a table and an HTML
  sheet; it has no `--json`. Every keep/reject verdict is therefore hand-made in a
  conversation and lost when the session ends.
- **No family has a manifest.** `RawArt/Renders/archer-scifi/raw/state0*/rotations/` exists
  with no record of which base character or state id produced it, what its silhouette target
  was, or how many generations it cost. `docs/data/art/provenance.json` has five entries and
  every one is empty. That is the bad data — you cannot tell a rejected reroll from a keeper
  by looking at the folder.
- **The axis lives in the transcript.** The variance table the skill demands ("which variable
  each variant moves, what every variant holds constant") is written once into chat and never
  onto disk, so the next family starts from zero.

Nothing breaks while this stays undone; families just keep costing a full session of
hand-driving each and leaving no record. `task-082` is the first run that depends on it.

## Findings — measured this session, so the teammate does not redo them

**1. Aspect spread alone would fail the family the owner calls the good one.** Measured
2026-07-29, `py Scripts/art/silhouette_report.py RawArt/Renders/archer-scifi/raw`:

```
state01_marksman     41x46   aspect 0.89  solid 0.52  asym 0.81   holes -          luma 0.182
state02_voltaic      30x46   aspect 0.65  solid 0.71  asym 0.30   holes -          luma 0.188
state03_harpooner    45x46   aspect 0.98  solid 0.54  asym 0.46   holes -          luma 0.173
state04_volley       43x48   aspect 0.90  solid 0.65  asym 0.49   holes 1 (46px)   luma 0.193
state05_kite         46x46   aspect 1.00  solid 0.50  asym 0.92   holes -          luma 0.178
state06_sharpshooter 43x46   aspect 0.93  solid 0.52  asym 0.67   holes -          luma 0.178

aspect spread 0.65-1.00 (1.5x) across 6 variants
```

1.5× is *below* the skill's "2.5×+ is good" line. This family reads apart anyway, because it
separates on **asymmetry 0.30–0.92** — a 3× spread on the axis the skill says marks a form
with a front and a back. **A judge that scores aspect spread alone would reject the reference
family.** Score separation across aspect, solidity, asymmetry and hole count together; no
single axis is the gate.

**2. A single frame will lie, and the skill has the measured case.** A variant briefed as a
long low ridge measured aspect **0.86** from the south and was nearly rerolled; across its
rotations it runs **0.59 to 2.60** and was the best in its batch. `--all-directions` exists
for exactly this. **The judge must never auto-reject from one direction.**

**3. So split the verdict into two kinds, and only automate one.** Auto-reject only the
unambiguous, direction-independent fails:
  - an opaque-pixel count identical to a sibling's (the failure the script already flags —
    two variants with one outline are one variant)
  - a returned canvas that does not match the family's declared size (`size 92` returns
    132×132; the trap is documented in the skill)
  - missing rotations, or a variant whose whole band across all eight directions sits inside
    a sibling's band on every axis

Everything else — undershot a stated target, lost its prop, stopped reading as the unit type
— is **flagged with the number and the reason, for the owner**. Judgement calls that need an
eye stay the owner's; the script's job is to make them cheap, not to pre-empt them. The
`archer-04` and Dome/Bar/Triangle failures in the skill are all "measured fine, not a unit
any more", which no metric catches.

**4. Generation cannot move into the script, and must not.** `Scripts/art/pixelpipe.py`'s
header states it: PixelLab's generators are **MCP tools, not REST endpoints**, so every
generation call is made by Claude via `mcp__pixellab__*`. `variantpipe.py` is the local,
offline half — exactly the same division `pixelpipe.py` already draws. Mirror
`pixelpipe.py:529`, which prints `{"tool": "mcp__pixellab__create_character", "kwargs": {...}}`
for the agent to execute. The automation being asked for is the deterministic work either
side of the generation, not the generation.

**5. Account state, for the cost note in the report:** 7,125 generations remaining of 10,000,
Tier 3 subscription active, measured 2026-07-29. Credits are not the constraint; a six-state
family at 88–92 px is ~120–240 generations.

## Done when
- `docs/data/art/family.schema.json` defines a family spec: base character id and confirmed
  canvas size, the named axis, the constant held across the family, and per-variant a slug, a
  silhouette target stated as a number, and the `edit_description` that was sent.
- `Scripts/art/variantpipe.py` has four subcommands over that spec, all offline:
  `plan` (emit the `mcp__pixellab__*` tool + kwargs per variant, for the agent to execute),
  `fetch` (download every rotation to `raw/<variant>/rotations/`, recording ids and urls),
  `judge` (measure all directions, apply §Findings 3, emit `--json`), and
  `report` (the HTML contact sheet, via the existing `sheet()` in `silhouette_report.py`).
- `silhouette_report.py` gains machine-readable output — the same numbers it tables today,
  plus its identical-outline flag and its `--all-directions` per-direction bands, as JSON.
  Extend it; do not fork its measurement code into `variantpipe.py`.
- Every family judged carries `docs/data/art/families/<family>/manifest.json` recording base
  character id, per-variant state id, prompt sent, generations spent, and the verdict with
  its reason.
- Rejected variants are **moved** to `RawArt/Renders/<family>/rejected/<variant>/`, never
  deleted — the standing retention rule. The move is recorded in the manifest.
- `.claude/skills/variants/SKILL.md` drives the script instead of restating the loop by hand.
  Its measured findings and thresholds are *why* the numbers are what they are — keep them,
  and point each threshold at where the script now enforces it.
- Run over the existing families and hand back the report: `archer-scifi` (the reference —
  all six states get a recorded verdict), `archer-proxy`, `knight-topology`,
  `knight-primitive`, `knight-mass`, `knight-types`, `brood-ooze`, `brood-ooze-colour`.
- One runnable check that fails if the judge breaks — an `assert`-based self-check pinning
  the archer-scifi numbers in §Findings 1 and confirming that family is **not** rejected.

## Spawn prompt

```
You are executing task-081. Read docs/backlog/task-081-codify-variant-family-pipeline.md
in full first — its "Findings" section contains measurements taken this session that you
should not spend generations or time rediscovering.

You are the `claude` agent. This task writes Python and runs it. It spends ZERO PixelLab
generations — you are building and proving the local, offline half of the pipeline against
sprite data that is already on disk. Do not call any mcp__pixellab__* generation tool.

THE JOB
Turn the hand-driven variant-family workflow in .claude/skills/variants/SKILL.md into a
declarative spec plus a driver script, then prove it by running it over the families that
already exist in RawArt/Renders/ and recording a keep-or-reject verdict for each variant.

Read these first:
  .claude/skills/variants/SKILL.md      the workflow, its axes, and its measured thresholds
  Scripts/art/silhouette_report.py      the measurement code — extend it, never fork it
  Scripts/art/pixelpipe.py              the house pattern for a local-half driver. Its header
                                        states why generation stays in MCP tools; line ~529
                                        shows how it emits tool+kwargs JSON for the agent
  docs/art/aesthetic-direction.md       the 2026-07-28 AMENDMENT

YOU OWN, and may write:
  Scripts/art/variantpipe.py                    (new)
  Scripts/art/silhouette_report.py              (extend with machine-readable output)
  docs/data/art/family.schema.json              (new)
  docs/data/art/families/{archer-scifi,archer-proxy,knight-topology,knight-primitive,
                          knight-mass,knight-types,brood-ooze,brood-ooze-colour}/**
                                                (new — one spec + manifest + report per family
                                                you judge. Do NOT create other subdirectories
                                                under families/ — task-082 claims knight-melee-v1)
  .claude/skills/variants/SKILL.md
  RawArt/Renders/{archer-scifi,archer-proxy,knight-topology,knight-primitive,knight-mass,
                 knight-types,brood-ooze,brood-ooze-colour}/**

DO NOT TOUCH — these are other tasks' live territory or superseded canon:
  docs/data/art/provenance.json         task-059 is live and holds this file. Family
                                        provenance goes in docs/data/art/families/<f>/manifest.json
  RawArt/Renders/soldier-scifi-variants/**   task-056's territory (parked)
  docs/data/art/requests/**             that is /sprite's single-character format, not families
  Scripts/art/pixelpipe.py              read it, model on it, do not edit it
  anything under ELVTR/                 this task ends at the owner's verdict. No packing,
                                        no quantizing, no UE import. task-059 owns the
                                        in-engine atlas state axis
  WORLD.md                              superseded by the 2026-07-22 narrative reset

CANON YOU NEED
- The game ships in FULL COLOUR as of 2026-07-28 (docs/art/aesthetic-direction.md
  AMENDMENT). The old 4-value demichrome gate is SUPERSEDED. silhouette_report.py's
  off-ramp column is therefore informational only — a full-colour family measuring high
  off-ramp is expected, NOT a defect, and must never be an auto-reject.
- Retention rule: PixelLab output is never deleted until a keep/reject decision is made,
  and a reject is MOVED to rejected/, not removed. Preserve every pixel currently on disk.

THE FOUR THINGS THAT MATTER MOST
1. Do not build an aspect-spread gate. Measured this session, archer-scifi — the family the
   owner names as the refined one — has an aspect spread of only 1.5x and separates on
   asymmetry (0.30 to 0.92) instead. Score separation across aspect, solidity, asymmetry and
   hole count together. The task file has the full table; your self-check must pin those
   numbers and assert that family passes.
2. Never auto-reject from a single direction. A variant that measured aspect 0.86 from the
   south runs 0.59-2.60 across its rotations and was the best of its batch. Judge across all
   eight directions.
3. Auto-reject ONLY the unambiguous fails (identical opaque count to a sibling, wrong
   returned canvas size, missing rotations, a band fully inside a sibling's on every axis).
   Everything else is FLAGGED with its number and reason for the owner to decide. Read
   §Findings 3 of the task file — the failures that matter most ("measured fine, stopped
   being a unit") are ones no metric catches, and pretending otherwise is worse than
   flagging.
4. Generation stays in MCP tools called by an agent. `plan` emits the tool name and kwargs;
   it does not call anything. This is the same split pixelpipe.py already enforces.

BE LAZY, DELIBERATELY
Reuse silhouette_report.py's measure(), variants(), check_rotations(), flat() and sheet()
functions — add a JSON path alongside the existing table, do not reimplement measurement.
Use the stdlib (json, argparse, pathlib, shutil, urllib) and the numpy/PIL already imported.
Add no new dependency. No class hierarchy, no plugin system, no config for a value that
never changes. Four subcommands over one JSON file is the whole design.

HAND BACK
- The verdict table for all eight existing families, and a published Artifact contact sheet
  (use the Artifact tool) showing each variant beside its flat outline. archer-scifi is the
  headline: all six states with a recorded verdict and a stated reason.
- The named cull list: exactly which variant folders moved to rejected/ and why. If nothing
  deserved culling, say that plainly rather than culling something to look productive.
- The one runnable check, and its output.
- Any threshold you could not justify from measured data — name it as a guess rather than
  burying it in the script.
Report what the numbers say, including where a family you expected to pass did not. A
variant that missed is a finding, not something to quietly omit.
```
