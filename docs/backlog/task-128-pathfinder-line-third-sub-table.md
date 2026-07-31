---
id: 128
title: Replace the archer sub-table with the twelve owner-supplied looks — one knight-armored archer survives, the other five retire
status: proposed
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "RawArt/Sheets/T_Team_2bit.png", "ELVTR/Content/Spike1/**", "docs/data/art/requests/team-units.json", "docs/data/art/team-variants.json", "docs/data/art/provenance.json", "docs/data/art/families/pathfinder-line/**", "docs/data/art/families/archer-medieval/**", "RawArt/Renders/pathfinder-line/**", "ELVTR/SETUP-EDITOR.md", "docs/perf/evidence/task128/**"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [130]
epic: ""                  # sequel to the archers-on-the-field epic, not a sibling of it —
                          # it shares SwarmRenderActor.cpp with task-130 and is ordered by
                          # depends-on, which the epic sibling-cut check does not read
evidence: A PIE screenshot at gameplay density in which the archer line is visibly a mix of the twelve owner-supplied looks and reads as a different kind of soldier from the spearmen, plus Swarm.TeamVariantReport showing the resized archer sub-table and a frame-time row at 10k/40k against docs/perf/evidence/task126/SwarmBench-task126.csv with the draw-call count unchanged.
score: {feel: 3, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now
**Revised 2026-07-31 by owner instruction, which inverted this task's original premise.**
It was filed as a *third* sub-table sitting beside the knights and the archers. The owner
looked at the shipped frame and said the opposite: *"I want all these states as archers.
The knight armor ones can be a single variant but not every."*

That is the root cause of the readability problem `task-127` and `task-130` have been
chasing from the render side. All six `archer-medieval` variants — `v1_narrowstrung`
through `v6_slingwhirl` — are **steel-helmeted knights holding a bow**. Against eleven
steel-helmeted knights holding a spear, at a 56px cell in a near-black frame, the bow is
the only difference and it is a thin dark arc. Size (`task-127`) and contrast
(`task-130`) can only push against that; they cannot fix a silhouette that was never
different. Twelve hooded, cloaked and distinctly-kitted looks are.

The art is already generated and already on disk — all 96 PNGs at
`RawArt/Renders/pathfinder-line/raw/<state>/rotations/`, at zero credit cost, because the
states already existed server-side. PixelLab group `e2702eb6-248d-4b6b-b8dc-fedc1290e059`.

## What the archer sub-table becomes
Thirteen rows: the twelve owner-supplied states, plus **exactly one** surviving
knight-armored archer.

- **Keep `v2_bowextended`** as the single knight-armored archer. Default choice, taken so
  the owner does not have to pick one: it is the variant already proven to read in a
  shipped capture (`docs/perf/evidence/task126/02-skewed-bowextended-vs-midguard.png`).
  Say so in the handback; the owner may swap it for another.
- **`v1_narrowstrung`, `v3_loosingarm`, `v4_quiverreach`, `v5_crossbowbrace`,
  `v6_slingwhirl` retire.** Their manifest entries stay on disk as the historical record —
  this is a roster change, not a deletion of evidence.
- Thirteen still fits one 4-bit within-table index (cap 16), so
  `SwarmRenderPack::VariantMask` stays `0xF` and the render int32 needs no repack.

## Measurements
Measured directly on each PNG's alpha bounding box, max across all 8 rotations. Canvas
92x92 for every state. Cell is 56. **All twelve pass** — this gate is what killed
`ranged-roster` in `task-085`, and it has already been run here.

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
- The archer sub-table is the twelve owner-supplied looks plus `v2_bowextended`, and the
  five retired knight-archer looks no longer appear in the army at default weights.
- The atlas grid, the Niagara Sub UV and `SwarmSheet::Team` all agree — the "three things
  must agree" contract at `SwarmFragments.h:44`. A stale Sub UV is the documented silent
  failure mode in this area.
- `SwarmRenderPack::VariantMask` is **unchanged at `0xF`**, and `ArcherVariantBase` still
  addresses the archer block by row offset exactly as `task-126` built it.
- `Swarm.ArcherVariantWeights` covers thirteen entries and still retires a look at weight 0.
- `Swarm.TeamVariantReport` reports the resized table.
- **At gameplay density, at default weights, the archer line reads as a different kind of
  soldier from the spearmen.** This is the real bar and it is a look call — hand the
  capture over, do not self-certify it.
- Frame time at 10k and 40k against `docs/perf/evidence/task126/SwarmBench-task126.csv`.
  No new draw call is expected — this is different rows in one existing atlas.
- `ELVTR/SETUP-EDITOR.md` records the new grid.
- `docs/data/art/families/pathfinder-line/family.json` and `provenance.json` record where
  this art came from, including that it was owner-supplied and cost no credits, and
  `archer-medieval`'s manifest records which five retired and why.

## Spawn prompt

```
You are executing task-128. Read docs/backlog/task-128-pathfinder-line-third-sub-table.md
first, then docs/backlog/task-126-land-archers-in-the-team-atlas.md in full — task-126
built the two-sub-table row-offset design you are working inside, and its reasoning still
governs every decision here.

READ THIS TASK'S "Why now" CAREFULLY. It was rewritten by owner instruction and its
premise is the OPPOSITE of what the title's filename suggests. These twelve looks are not
a third sub-table. They ARE the archer sub-table now.

GOAL
Every current archer look is a steel-helmeted knight holding a bow, which is why an archer
still reads as a spearman at density. Replace the archer sub-table with twelve owner-
supplied looks that are hooded, cloaked and distinctly kitted, keeping exactly one
knight-armored archer.

WHAT THE TABLE BECOMES
Thirteen rows: the twelve states in RawArt/Renders/pathfinder-line/raw/, plus
v2_bowextended from docs/data/art/families/archer-medieval/ as the single surviving
knight-armored archer. v1_narrowstrung, v3_loosingarm, v4_quiverreach, v5_crossbowbrace
and v6_slingwhirl retire from the atlas. Do NOT delete their manifest entries — record
that they retired, with the reason.

Thirteen fits one 4-bit within-table index. Do NOT widen SwarmRenderPack::VariantMask, do
NOT repack the render int32, do NOT add a third emitter or a third atlas. The row-offset
approach task-126 landed is what you are extending.

WHAT IS ALREADY TRUE — do not rebuild any of it
Archers are simulated, formed up, sized (Swarm.ArcherSizeScale, task-127) and now fire a
visible volley cue (task-129, UVolleySubsystem). task-130 has just landed contrast and/or
mix changes on the same render path — read its handback in docs/perf/niagara-sprite-path.md
before you touch SwarmRenderActor.cpp, because its CVar defaults may need retuning once
the sprites themselves stop being knight-shaped. If a contrast lift task-130 added is
redundant after this swap, say so; do not silently revert it.

The size gate is already measured and all twelve pass at the 56px cell — the table is in
this task file. Do not re-measure unless you distrust it, and say so if you do.

NAMING — this needs judgement, not a mechanical copy
The state folder names are truncated prompt fragments: "Can_we_have_one_cove",
"Lets_so_secret_servi", "Bump_helmet_ontop_of", "Can_you_make_a_Turre". They are not
usable as shipped variant names. Give each a real descriptive slug in the same register as
the existing v1_narrowstrung / v2_bowextended naming, based on what the sprite actually
looks like, and record the mapping from folder name to slug in family.json so the
provenance trail survives.

Also note "Merle (Pathfinder)" is a leftover hero name. The owner reversed fixed hero
names — classes are identified by role only, because attrition makes names hard to
remember. Do not propagate "Merle" into any shipped variant name.

THE KIT IS NOT A PROBLEM — do not filter these on tone
Several states carry guns, an RPG launcher, shoulder cannons, a mounted turret, a cyborg
crossbow and a mage staff. The arms-and-kit register was ruled OPEN by the owner on
2026-07-31: guns, turrets and mage kit all ship, and canon never banned them — the ban was
an inference that got cited back as canon. Pack all twelve. If you think one does not
belong, say so in the handback and pack it anyway.

KNOWN TRAPS
- MCP asset edits are IN MEMORY until save_assets([]). unreal-mcp is on port 9000, native
  UE plugin, not the uefn python bridge.
- save_assets([]) saves EVERYTHING dirty in the shared editor, including other sessions'
  work. task-129 hit this and flagged it rather than hiding it. Do the same.
- Path-addressed editor_toolset AssetTools calls (exists, is_dirty, duplicate,
  save_assets(["/Game/..."])) are broken in this editor build and return false for paths
  that find_assets returns verbatim. Only find_assets and save_assets([]) work.
- Do not add a fragment field or any other class-layout change. Live Coding reports
  success on a layout change and then crashes the next PIE.
- EditorPerformanceSettings.bThrottleCPUWhenNotForeground must be disabled on the live CDO
  before any benchmark (docs/AGENT-TEAMS.md §8a). The project ini line claiming it is
  already False does NOT work.
- Never leave the editor launched with -SwarmBench before capturing screenshots; the
  benchmark stays armed and hijacks every later PIE.
- Every capture must have the WHOLE RETINUE IN FRAME. The task-129 captures filled only
  the bottom third with the retinue clipped by the bottom edge and the owner rejected that
  framing. Pull the camera back (Kindled.Cam.Dist / Pitch) and report the values you used.

DO NOT TOUCH
ELVTR/Source/ELVTR/Mass/SwarmCombat.h, SwarmCommands.cpp, SwarmCombatProcessors.cpp,
ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h, UnitCamProjector.cpp, GDD.md, SYSTEMS.md,
CLASSES.md, or any docs/design/ file. Other sessions have live uncommitted work in several
of those exact files — do not revert, tidy or commit anything you did not write. Do not
retune any archer combat stat.

HAND BACK
- The PIE capture at gameplay density and default weights, whole retinue in frame, proving
  the archer line now reads as a different kind of soldier.
- The folder-name to variant-slug mapping.
- Which knight-armored variant survived and confirmation the other five are gone from the
  default mix.
- Swarm.TeamVariantReport output for the resized table.
- Frame time at 10k and 40k against docs/perf/evidence/task126/SwarmBench-task126.csv, and
  draw calls before and after.
- Whether any task-130 CVar default is now redundant, without reverting it yourself.

Do not commit and do not push. The lead handles that at the close.
```
