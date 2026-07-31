---
id: 126
title: Make an archer look like an archer on the battlefield — pack the family into the team atlas and branch the render path on unit type
status: approved
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "RawArt/Sheets/T_Team_2bit.png", "ELVTR/Content/Spike1/**", "docs/data/art/requests/team-units.json", "docs/data/art/team-variants.json", "docs/data/art/provenance.json", "ELVTR/SETUP-EDITOR.md", "docs/perf/niagara-sprite-path.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [125]
epic: archers-on-the-field
evidence: A PIE screenshot at gameplay density where the archer line behind the spearmen is visibly a different unit, plus `Swarm.TeamVariantReport` output showing the archer sub-table in use and a frame-time row at 1k/10k/40k against the current baseline.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: ""
decided: "2026-07-31 approved"
---

## Why now
The archer half of the typed-unit model has been fully simulated since `task-049`/`task-046`
and is invisible. `SwarmProcessors.cpp:1190` picks a sprite variant from the jitter phase and
the team bit alone, with no unit-type branch, so an entity that spawns as
`EUnitType::Archers`, marches in the archer formation and shoots from 750uu still draws one
of the eleven knight looks. The player cannot see their own ranged line. `task-125` produces
the art; nothing renders it until this lands.

## Done when
- The archer keeps from `task-125` are packed into `T_Team_2bit` and the atlas grid, the
  Niagara Sub UV, and `SwarmSheet::Team::Rows` all agree — the "three things must agree"
  contract at `SwarmFragments.h:44`.
- **An entity whose `SquadId` resolves to `EUnitType::Archers` draws from the archer rows and
  never from the eleven spearmen rows, and vice versa.**
- **At gameplay density, the archer line reads as a visibly different unit from the spearmen
  line in a PIE screenshot.** This is the real bar. A diff and a claim do not clear it.
- A weight CVar over the archer sub-table exists and skewing it visibly changes the archer
  mix, the same retire-at-weight-0 pattern `Swarm.TeamVariantWeights` already ships.
- `Swarm.TeamVariantReport` reports both sub-tables, or says plainly which one it covers.
- Frame time at 1k/10k/40k stated against the current baseline. No new draw call is expected
  — this is more rows in one existing atlas, not a third emitter.
- `ELVTR/SETUP-EDITOR.md` and `docs/perf/niagara-sprite-path.md` both record the new grid.
  A stale Sub UV is the silent failure mode of this whole area.

## Spawn prompt

```
You are executing task-126. Read docs/backlog/task-126-land-archers-in-the-team-atlas.md
first, then docs/backlog/task-085-split-team-and-enemy-atlases-with-knight-family.md — that
task built the two-atlas split you are extending and its reasoning still governs.
task-125 has already closed and its archer art is on disk; read
docs/data/art/families/archer-medieval/manifest.json for which variants are keeps.

GOAL
Archers are a live unit type that is invisible. Make an archer draw as an archer on the
battlefield.

WHAT IS ALREADY TRUE — do not rebuild any of it
Archers are fully simulated and shipped. EUnitType::Archers is at SwarmCombat.h:46, seven
Swarm.Archers* CVars carry their stats at SwarmCombatProcessors.cpp:119-158, they roll at 20%
of every recruit at SwarmCommands.cpp:271, they read their own engage band at
SwarmCombatProcessors.cpp:436, and they hold their own formation via
SwarmFormation::ReadParamsForType. None of this needs touching. Do NOT retune any archer
stat, do NOT edit docs/data/unit-types.json, and do NOT change the growth weight. The owner
settled on 2026-07-31 that the shipped archer stats are enough as they stand.

THE ACTUAL BUG, in one line
SwarmProcessors.cpp:1190 picks the sprite variant from the jitter phase and the team bit
only. There is no unit-type branch, so archers get the eleven-look knight table.

THE HARD CONSTRAINT — read this before you design anything
SwarmRenderPack::VariantMask is 0xF at SwarmFragments.h:329. Four bits, values 0-15. The
team side already uses 11 of them. Eleven spearmen looks plus the archer keeps will not fit
in one flat 0-15 index space.

TAKE THE LAZY PATH, and do not build the elaborate one:
Keep the 4-bit field as a WITHIN-TABLE index — 0..10 for spearmen, 0..N-1 for archers — and
apply the archer ROW OFFSET where the atlas cell is computed, in SwarmRenderActor.cpp's pack
loop. That loop already has the packed int32, and SwarmRenderPack::Squad() plus
SwarmSquad::UnitType() already resolve the unit type from it — SwarmCombatProcessors.cpp:831
does exactly this lookup today, so copy that pattern. The atlas can then grow past 16 rows
while the bit field never does.

Do NOT widen VariantShift/VariantMask or otherwise repack the render int32. Do NOT add a
third Niagara emitter or a third atlas. Do NOT add a fragment field to store the variant —
variant is DERIVED from the jitter phase by design, and adding a member to a fragment or to
SwarmRenderActor is a class-layout change that Live Coding cannot apply: it reports success
and then crashes the next PIE (see the live-coding-uproperty-crash pattern and the ponytail:
note already in SwarmRenderActor.cpp). Any layout change needs a full editor-closed rebuild.

THE WORK
1. Add the archer keeps as new sources in docs/data/art/requests/team-units.json and extend
   output.grid and output.frame_map. Follow the existing shape exactly: 8 rotations per
   variant, and walk1 aliases the SAME file as walk0 — the shipped atlas has no second
   animation frame, so do not invent one. Cell stays 56. Grid goes from [8, 22] to
   [8, 22 + 2*N].
2. Repack T_Team_2bit via the existing pixelpipe composite path. Regenerate no art.
3. Bump SwarmSheet::Team::Variants / Rows in SwarmFragments.h and update the "three things
   must agree" comment so it names the archer block.
4. Branch the variant pick in SwarmProcessors.cpp on unit type, and add the archer row offset
   in SwarmRenderActor.cpp's pack loop as described above.
5. Add a Swarm.ArcherVariantWeights CVar over the archer sub-table. Reuse ParseVariantTable
   and VariantFromPhase — do not write a second parser.
6. Record the archer block in docs/data/art/team-variants.json and the generation provenance
   in docs/data/art/provenance.json.
7. Update the Niagara Sub UV rows on NS_Swarm's team emitter, and reimport the texture.

STATS BINDING — a trap worth naming
task-095 made the team variant index also select a melee stat row
(SwarmCombatTuning::KnightSubtypeRowFor). Archers already short-circuit that: the bArcher
branches at SwarmProcessors.cpp:548 and SwarmCombatProcessors.cpp:465 route them to the
Archers* CVars and never reach the knight table. Confirm this still holds after your change
and say so in the handback. An archer that starts picking up a knight stat row is a
regression this task would have caused.

MCP NOTES
unreal-mcp is on port 9000 for this project, not 8000. Use the native unreal-mcp plugin, not
the uefn python bridge. MCP asset edits are IN MEMORY until you call save_assets([]) — an
unsaved Sub UV change looks like it worked and is gone on restart. If a material Custom node
is involved at all, set_properties on its `code` field silently no-ops and returns true, so
always read back and compare length before recompiling.

WHAT YOU MUST NOT TOUCH
Only the paths in this task's owns: list. In particular do not touch
docs/data/unit-types.json, SwarmCombat.h, SwarmCombatProcessors.cpp, SwarmCommands.cpp, or
anything under RawArt/Renders/. If you find you genuinely need a file outside the list, stop
and say so in the handback rather than editing it — the tree is shared with other sessions
and unrelated modified assets are already in it.

HAND BACK
- A PIE screenshot at gameplay density showing the archer line behind the spearmen reading as
  a visibly different unit. This is the bar; a diff plus "it works" does not clear it.
- Swarm.TeamVariantReport output showing the archer sub-table in use.
- The result of skewing Swarm.ArcherVariantWeights — what visibly changed.
- Frame time at 1k/10k/40k against the current baseline, and the draw-call count before and
  after.
- Confirmation that archers still route to the Archers* stat CVars and never to a knight
  stat row.
- The new grid dimensions, and confirmation that SETUP-EDITOR.md and
  docs/perf/niagara-sprite-path.md both record them.
```
