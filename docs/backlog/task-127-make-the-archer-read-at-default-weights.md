---
id: 127
title: Make an archer readable at default weights — the render branch works but the bow disappears into the mass
status: done
agent: claude
model: sonnet
owns: ["ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "docs/perf/evidence/task127/**", "docs/perf/niagara-sprite-path.md"]
resources: ["unreal-editor"]
depends-on: []
epic: archers-on-the-field
evidence: A PIE screenshot at gameplay density, at DEFAULT Swarm.ArcherVariantWeights, in which an archer is findable without being told where to look — plus the same capture at the pre-change setting for comparison, and a frame-time row showing the change costs nothing.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: archer-readable
decided: "2026-07-31 done"
---

## Why now
`task-126` landed the render branch and proved it works: archers resolve to the archer
sub-table, `Swarm.ArcherVariantWeights` visibly drives the mix, and `draw_ms` stays flat
at ~2ms from 1k to 40k, so the atlas grew without a new draw call. The mechanism is not
in question.

What it did not achieve is the thing the feature exists for. In
`docs/perf/evidence/task126/02-skewed-bowextended-vs-midguard.png`, with weights skewed
hard to one look, bows are clearly visible. In
`01-default-weights-archers-among-spearmen.png`, at the default mix, an archer cannot be
found in the frame at all. Archers roll at 20% of every recruit
(`SwarmCommands.cpp:271`) and that 20% then spreads across six archer looks, so any one
look is ~3% of the army — and the tell is a thin dark bow arc against a near-black
frame. The player cannot see their own ranged line, which is the exact problem
`task-126` was filed to solve. The engineering half is done; this is the other half.

## Done when
- **At default `Swarm.ArcherVariantWeights`, an archer is findable in a gameplay-density
  PIE capture without being told where to look.** This is the bar and it is a look call —
  hand the capture over, do not self-certify it.
- A before capture at the current setting is attached beside the after, same camera, same
  density, same seed if the spawn is seedable. One image alone proves nothing here.
- Frame time at 10k and 40k stated against `docs/perf/evidence/task126/SwarmBench-task126.csv`,
  which is the only committed bench and is therefore the baseline. Draw calls unchanged.
- Whatever lever moved is exposed as a CVar in the `Swarm.*` namespace, following the
  existing `retire-at-weight-0` pattern, so the owner can tune it live rather than
  needing a rebuild.
- `docs/perf/niagara-sprite-path.md` records the lever and its default.

## Spawn prompt

```
You are executing task-127. Read docs/backlog/task-127-make-the-archer-read-at-default-weights.md
first, then docs/backlog/task-126-land-archers-in-the-team-atlas.md, which landed the
machinery you are building on and must not be undone.

GOAL
Archers now draw from their own sprite rows and are still effectively invisible in play.
Make one findable at default weights, at gameplay density, without being told where to
look.

LOOK AT THE EVIDENCE FIRST, BEFORE YOU CHANGE ANYTHING
docs/perf/evidence/task126/01-default-weights-archers-among-spearmen.png  (the problem)
docs/perf/evidence/task126/02-skewed-bowextended-vs-midguard.png          (bows visible)
The second image proves the render path is correct. Do not re-debug it. The difference
between the two images is entirely the weight table, which means this is a visibility
problem and not a wiring problem.

WHY IT DOES NOT READ, as far as the evidence shows
Archers are 20% of recruits (SwarmCommands.cpp:271), that 20% spreads across six archer
looks, so each individual look is roughly 3% of the army. The archer tell is a thin,
dark, low-contrast bow arc. The frame is very dark. At a 56px cell in a mass of
hundreds, that tell is below the threshold at which anything reads.

TAKE THE LAZY PATH, and climb the ladder in this order. Stop at the first rung that
clears the bar, capture it, and stop — do not do all four:

1. SIZE. There is already a per-particle size path — User.Sizes / TeamSizes is a
   populated float array and Swarm.BroodSizeJitter already drives it, so a per-unit-type
   size multiplier is very likely a few lines in SwarmRenderActor.cpp's pack loop
   where the archer row offset is already applied. A modest archer size bump may be the
   whole fix. Try this first.
2. CONTRAST. There is already a per-particle colour path and M_Swarm_Team reads
   ParticleColor (see the material-must-read-particle-color history — the material was
   silently discarding it once already, so verify the value actually lands before
   concluding the lever does nothing). A small brightness lift on archers only would
   separate them from the spearmen mass.
3. MIX. If neither reads, the six-way spread may simply be too thin. Raising the default
   archer weight concentration is a one-line default change, but it changes the army's
   composition read, so flag it rather than assuming it.
4. Only if 1-3 all fail: say so and stop. Do NOT regenerate art. Do NOT call PixelLab.
   This task does not hold pixellab-credits and new sprites are a separate decision.

Whatever you land, expose it as a Swarm.* CVar with a sensible default, following the
existing pattern where weight 0 retires a look. The owner tunes this live; a value that
needs a rebuild to change is not finished.

WHAT YOU MUST NOT DO
- Do not undo, re-architect or "improve" task-126's render branch. Do not widen
  SwarmRenderPack::VariantMask. Do not add a third emitter or a third atlas. Do not add
  a fragment field or any other class-layout change: Live Coding reports success on a
  layout change and then crashes the next PIE, so a layout change needs a full
  editor-closed rebuild.
- Do not retune any archer combat stat. Do not edit docs/data/unit-types.json.
- Do not touch the atlas, the sprite sheets, docs/data/art/**, or anything under
  RawArt/. Only the paths in this task's owns: list.
- MCP asset edits are in memory until you call save_assets([]). An unsaved change looks
  correct in your session and is gone on restart.

HAND BACK
- The before and after PIE captures, both at DEFAULT weights, same camera and density,
  in docs/perf/evidence/task127/.
- Which rung of the ladder you stopped at and why the earlier ones did or did not clear.
- The CVar name, its default, and what a useful range looks like.
- Frame time at 10k and 40k against docs/perf/evidence/task126/SwarmBench-task126.csv,
  and the draw-call count before and after.
- Whether you rebuilt with the editor closed or relied on Live Coding.
- Confirmation that every asset edit was followed by save_assets([]).

Do not commit and do not push. The lead handles that at the close. If you are blocked —
the editor is not running, MCP is unreachable, a rebuild is required — say so plainly
rather than declaring success.
```
