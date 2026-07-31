---
id: 128
title: Land the twelve Pathfinder looks as a third team sub-table — the owner-supplied group, already measured and inside the cell
status: proposed
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "RawArt/Sheets/T_Team_2bit.png", "ELVTR/Content/Spike1/**", "docs/data/art/requests/team-units.json", "docs/data/art/team-variants.json", "docs/data/art/provenance.json", "docs/data/art/families/pathfinder-line/**", "RawArt/Renders/pathfinder-line/**", "ELVTR/SETUP-EDITOR.md", "docs/perf/evidence/task128/**"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [127]
epic: ""
evidence: A PIE screenshot at gameplay density in which the Pathfinder looks are visibly present alongside the spearmen and archers, plus Swarm.TeamVariantReport showing three sub-tables and a frame-time row at 10k/40k against docs/perf/evidence/task126/SwarmBench-task126.csv with the draw-call count unchanged.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now
The owner supplied a twelve-state PixelLab group (`Merle (Pathfinder)`, group
`e2702eb6-248d-4b6b-b8dc-fedc1290e059`) on 2026-07-31 and chose, when asked, that these
are **massed army looks for the team atlas as a third sub-table** rather than a hero
line. The art is already generated and already downloaded — all 96 PNGs are on disk at
`RawArt/Renders/pathfinder-line/raw/<state>/rotations/`, at zero credit cost, because the
states already existed server-side.

The size gate that killed `ranged-roster` in `task-085` has already been measured and
**all twelve pass**: the worst content bounding box across all 8 facings of all 12 states
is 53x50 against the 56px cell, and the full per-state table is in the `## Measurements`
section below. Twelve looks also fit one 4-bit within-table index (cap 16), so no repack
of the render int32 is needed — the row-offset approach `task-126` landed extends to a
third block without touching `SwarmRenderPack::VariantMask`.

This could not be filed earlier because `task-126` held every one of these paths.

## Measurements
Measured directly on each PNG's alpha bounding box, max across all 8 rotations. Canvas
92x92 for every state. Cell is 56.

| state | max bbox | gate |
|---|---|---|
| Bump_helmet_ontop_of | 53 x 50 | PASS |
| Can_you_make_a_Turre | 45 x 50 | PASS |
| Can_we_have_one_cove | 43 x 47 | PASS |
| Replace_the_bow_with | 45 x 46 | PASS |
| ArtilleryCannons_on | 46 x 46 | PASS |
| dwarf_version_which | 44 x 46 | PASS |
| Merle_Pathfinder | 43 x 46 | PASS |
| One_with_Hollow_blac | 43 x 46 | PASS |
| RPG_launcher | 43 x 46 | PASS |
| Cyborg_with_Crossbow | 42 x 46 | PASS |
| Convert_to_Mage | 40 x 46 | PASS |
| Lets_so_secret_servi | 32 x 46 | PASS |

## Done when
- All twelve are packed into `T_Team_2bit` as a third sub-table, and the atlas grid, the
  Niagara Sub UV and `SwarmSheet::Team` all agree — the "three things must agree"
  contract.
- `SwarmRenderPack::VariantMask` is **unchanged at 0xF**. The third block is addressed by
  a row offset, exactly as `ArcherVariantBase` addresses the second.
- A weight CVar over the new sub-table exists, following the `retire-at-weight-0` pattern
  the other two use.
- `Swarm.TeamVariantReport` reports all three sub-tables, or says plainly which it covers.
- Frame time at 10k and 40k against `docs/perf/evidence/task126/SwarmBench-task126.csv`.
  No new draw call is expected — this is more rows in one existing atlas.
- `ELVTR/SETUP-EDITOR.md` and the Sub UV both record the new grid. A stale Sub UV is the
  documented silent failure mode in this area.
- `docs/data/art/families/pathfinder-line/family.json` and `provenance.json` record where
  this art came from, including that it was owner-supplied and cost no credits.

## Spawn prompt

```
You are executing task-128. Read docs/backlog/task-128-pathfinder-line-third-sub-table.md
first, then docs/backlog/task-126-land-archers-in-the-team-atlas.md in full — task-126
built the two-sub-table row-offset design you are extending to a third, and its reasoning
still governs every decision here.

GOAL
Twelve owner-supplied unit looks are on disk and draw nowhere. Pack them into the team
atlas as a third sub-table so they appear in the army.

WHAT IS ALREADY DONE — do not redo any of it
- The art is generated and downloaded: 96 PNGs at
  RawArt/Renders/pathfinder-line/raw/<state>/rotations/<facing>.png, 12 states x 8
  rotations, canvas 92x92. RawArt/Renders/pathfinder-line/source.json records the
  PixelLab group and per-state character ids.
- The size gate is measured and all twelve pass. The table is in the task file. Do NOT
  spend credits, do NOT call PixelLab to regenerate, and do NOT re-measure from scratch —
  though you SHOULD spot-check two or three states against the table before packing,
  because packing on a wrong number is expensive to unwind.
- The owner has ruled these are massed army looks, not a hero line. That question is
  settled; do not reopen it.

REGISTER — read this before you judge any sprite
Several of these carry firearms, artillery, a turret, a cyborg crossbow and mage kit.
That is deliberate and it is canon as of 2026-07-31: see the AMENDMENT at the head of
docs/narrative/FLAME-FOUNDATION.md and the mirror at the top of
docs/art/aesthetic-direction.md. Do NOT cull, substitute or flag a look for being
insufficiently medieval, and do not "fix" one back to a bow. All twelve go in.

THE HARD CONSTRAINT
SwarmRenderPack::VariantMask is 0xF — four bits, values 0-15. It is a WITHIN-TABLE index.
Eleven spearmen, six archers and twelve Pathfinders is 29 looks and will never fit one
flat index space. Twelve DOES fit one sub-table on its own, which is why this works.

TAKE THE LAZY PATH, and do not build the elaborate one:
task-126 already established the pattern — the 4-bit field stays a within-table index and
the block offset is applied where the atlas cell is computed, in SwarmRenderActor.cpp's
pack loop, next to the existing ArcherVariantBase offset. Extend that to a third block.
Follow the shape that is already there rather than generalising it into a table-of-tables
unless the existing code genuinely cannot express a third case.

Do NOT widen VariantShift/VariantMask or repack the render int32. Do NOT add a third
Niagara emitter or a third atlas — this is more rows in the one team atlas. Do NOT add a
fragment field to store the variant: variant is derived from the jitter phase by design,
and adding a member to a fragment or to SwarmRenderActor is a class-layout change that
Live Coding cannot apply. It reports success and then crashes the next PIE. Any layout
change needs a full editor-closed rebuild.

WHICH UNIT TYPE DRAWS THESE
This is the one genuinely open question and it is yours to answer, in one line, in the
handback. The two existing blocks key off EUnitType. If no existing unit type should draw
Pathfinder looks, the cheapest correct answer is likely a weight-driven share of an
existing type rather than inventing a new EUnitType — inventing one reaches into
SwarmCombat.h, unit-types.json and the spawn roll, all of which are outside this task's
owns: list. If you conclude a new unit type is genuinely required, STOP and say so in the
handback instead of editing those files.

THE WORK
1. Write docs/data/art/families/pathfinder-line/family.json against
   docs/data/art/family.schema.json, recording the group, the twelve states and their
   measured bboxes. This is a provenance record for already-existing art, not a
   generation plan.
2. Add the twelve as sources in docs/data/art/requests/team-units.json and extend
   output.grid and output.frame_map. Follow the existing shape exactly: 8 rotations per
   state, walk1 aliases the SAME file as walk0 (the shipped atlas has no second animation
   frame — do not invent one). Cell stays 56. The grid grows from [8, 34] by 2 rows per
   state.
3. Repack T_Team_2bit through the existing pixelpipe composite path. Regenerate no art.
4. Bump the SwarmSheet::Team constants and update the "three things must agree" comment
   so it names the third block.
5. Branch the variant pick and add the third row offset, per the lazy path above.
6. Add a weight CVar over the new sub-table. Reuse ParseVariantTable and VariantFromPhase
   — do not write a second parser.
7. Record the block in docs/data/art/team-variants.json and the provenance in
   docs/data/art/provenance.json, noting the art was owner-supplied at zero credit cost.
8. Update the Niagara Sub UV rows on NS_Swarm's Team emitter and reimport the texture.

MCP NOTES
unreal-mcp is on port 9000 for this project, not 8000. Use the native unreal-mcp plugin,
not the uefn python bridge. MCP asset edits are IN MEMORY until you call save_assets([]) —
an unsaved Sub UV change looks like it worked and is gone on restart. If a material Custom
node is involved, set_properties on its `code` field silently no-ops and returns true, so
read back and compare length before recompiling.

A NOTE ON THE TWO EMITTERS, so you do not chase a ghost
The Swarm emitter samples T_Enemy_2bit (448x1008, 18 rows, SubImageSize {8,18}). The Team
emitter samples T_Team_2bit (448x1904 today, 34 rows, SubImageSize {8,34}) and is the one
you are growing. T_Swarm_2bit (448x1120, 20 rows) is neither — it survives only for the
disabled Legacy UnitCam billboard path. Do not "reconcile" these three; they are already
correct and independent.

WHAT YOU MUST NOT TOUCH
Only the paths in this task's owns: list. In particular do not touch docs/data/unit-types.json,
SwarmCombat.h, SwarmCombatProcessors.cpp, SwarmCommands.cpp, or anything under
RawArt/Renders/ other than pathfinder-line/. The tree is shared with other sessions and
already carries unrelated modified assets: do not revert, stash, checkout, reset or clean
anything, and do not commit or push. The lead handles that at the close.

HAND BACK
- A PIE screenshot at gameplay density showing the Pathfinder looks present in the army,
  in docs/perf/evidence/task128/.
- Swarm.TeamVariantReport output showing all three sub-tables.
- Which unit type draws these, and why that was the cheapest correct answer.
- The new grid dimensions, and the Sub UV value you READ BACK off the live Team emitter
  after saving — state that you read it rather than inferring it from a doc.
- Frame time at 10k and 40k against docs/perf/evidence/task126/SwarmBench-task126.csv,
  plus draw-call count before and after.
- Whether you rebuilt with the editor closed or relied on Live Coding.
- Confirmation that every asset edit was followed by save_assets([]).

If you are blocked — the editor is not running, MCP is unreachable, a rebuild is required
— say so plainly in the handback rather than declaring success.
```
