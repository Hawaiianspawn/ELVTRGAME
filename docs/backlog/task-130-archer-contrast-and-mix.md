---
id: 130
title: Spend the contrast and mix rungs task-127 left — a visible archer block, not one findable archer
status: proposed
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp", "docs/perf/niagara-sprite-path.md", "docs/perf/evidence/task130/**"]
resources: ["unreal-editor"]
depends-on: []
epic: archers-on-the-field
evidence: A PIE capture at gameplay density and DEFAULT settings in which the archer line reads as a distinct block behind the spearmen, beside the task-127 capture for comparison, plus a frame-time row at 10k and 40k against docs/perf/evidence/task126/SwarmBench-task126.csv.
score: {feel: 2, risk: 1, cost: 1}
source: user
teammate: ""
decided: ""
---

## Why now
`task-127` closed at the first rung of its own ladder — size only, `Swarm.ArcherSizeScale`
at 1.4 — and its commit says so plainly: *"in the frame an archer still reads mostly by
robe colour, and the contrast rung is unspent"*. It was closed as good enough rather than
as the bar fully met. Rungs 2 (contrast) and 3 (mix) are written, costed and untried. The
owner has now asked for a visible archer presence, which is exactly what those two rungs
buy.

## Done when
- **At default settings, at gameplay density, the archer line reads as a distinct block
  behind the spearmen — not one archer you can find if you hunt for it.** Look call: hand
  the capture over, do not self-certify it.
- The `task-127` capture is attached beside the new one, same camera, same density.
- Both levers land as `Swarm.*` CVars with sensible defaults, tunable live without a
  rebuild, following the existing pattern.
- The new archer mix is stated as a number and flagged as an army-composition change —
  more archers means fewer spearmen, which is a balance read the owner may want to argue
  with.
- Frame time at 10k and 40k against `docs/perf/evidence/task126/SwarmBench-task126.csv`.
  Draw calls unchanged.
- `docs/perf/niagara-sprite-path.md` records both levers and their defaults.

## Spawn prompt

```
You are executing task-130. Read docs/backlog/task-130-archer-contrast-and-mix.md first,
then docs/backlog/task-127-make-the-archer-read-at-default-weights.md IN FULL — its spawn
prompt contains the four-rung ladder you are continuing, and rung 1 is already spent.

GOAL
Archers are now 1.4x size and still read mostly by robe colour. Make the archer line read
as a visible block behind the spearmen, at default settings, at gameplay density.

WHAT IS ALREADY TRUE — do not redo any of it
task-126 landed the render branch: archers resolve to their own team-atlas sub-table and
Swarm.ArcherVariantWeights drives the mix. task-127 landed rung 1: Swarm.ArcherSizeScale,
default 1.4, in SwarmRenderActor.cpp's pack loop. The mechanism is not in question and
draw_ms stayed flat from 1k to 40k. Do NOT re-debug the render path, do NOT widen
SwarmRenderPack::VariantMask, do NOT add a third emitter or a third atlas.

LOOK AT THE EVIDENCE FIRST
docs/perf/evidence/task127/02-after-archersizescale-1.4.png   (where it stands now)
docs/perf/evidence/task127/03-side-by-side-crop.png
docs/perf/evidence/task126/01-default-weights-archers-among-spearmen.png  (before size)

SPEND EXACTLY THESE TWO RUNGS, in this order
2. CONTRAST. There is a per-particle colour path and M_Swarm_Team reads ParticleColor.
   Give archers only a brightness lift so they separate from the spearmen mass. VERIFY
   THE VALUE ACTUALLY LANDS before concluding the lever does nothing — M_Swarm silently
   discarded every per-particle colour once already because the material had no
   ParticleColor node at all, and that cost a whole investigation. Check the material
   first if the lever looks inert.
3. MIX. Swarm.ArcherGrowthWeight is 0.2 (SwarmCombatProcessors.cpp:156-158) and that 20%
   then spreads across six archer looks, so any one look is ~3% of the army. Raise the
   DEFAULT so there is a real block. This changes army composition — state the new number
   plainly and flag it as a balance change rather than burying it.

Stop when the block reads. If contrast alone clears the bar, say so and leave the mix
alone — that is a better outcome, not a lesser one.

DO NOT climb past rung 3. Do NOT regenerate art, do NOT call PixelLab (this task does not
hold pixellab-credits), do NOT touch the atlas, the sprite sheets, docs/data/art/**, or
anything under RawArt/. Do NOT retune any archer combat stat other than
ArcherGrowthWeight, and do NOT edit docs/data/unit-types.json.

KNOWN TRAPS
- Do not add a fragment field or any other class-layout change. Live Coding reports
  success on a layout change and then crashes the next PIE; a layout change needs a full
  editor-closed rebuild.
- MCP asset edits are in memory until you call save_assets([]). An unsaved change looks
  correct in your session and is gone on restart.
- Setting a material Custom node's `code` via MCP set_properties silently no-ops and
  returns true. Read it back and compare length before recompiling.
- Other work is live in this shared tree. SwarmCombat.h, SwarmCommands.cpp,
  SwarmRenderActor.h and UnitCamProjector.cpp all carry uncommitted changes that are NOT
  yours. Do not revert them, do not tidy them, do not commit them. If a build breaks in
  one of those files, say so and stop rather than fixing it.

DO NOT TOUCH
Anything outside this task's owns: list. Specifically not SwarmProcessors.cpp, not
SwarmCommands.cpp, not SwarmCombat.h, not ELVTR/Content/**, not GDD.md, SYSTEMS.md,
CLASSES.md, or any docs/design/ file.

HAND BACK
- Before and after PIE captures, both at DEFAULT settings, same camera and density, in
  docs/perf/evidence/task130/.
- Which rungs you spent and what each one bought on its own.
- Every CVar name, its new default, and a useful range.
- The new archer share of the army as a number.
- Frame time at 10k and 40k against docs/perf/evidence/task126/SwarmBench-task126.csv,
  and draw calls before and after.
- Whether you rebuilt with the editor closed or relied on Live Coding.

Do not commit and do not push. The lead handles that at the close. If you are blocked —
the editor is not running, a rebuild is required — say so plainly rather than declaring
success.
```
