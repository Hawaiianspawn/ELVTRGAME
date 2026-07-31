---
id: 085
title: Split the horde into a team atlas and an enemy atlas, and land the knight family in the team half
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h", "ELVTR/Content/Spike1/**", "ELVTR/SETUP-EDITOR.md", "RawArt/Sheets/T_Swarm*.png", "RawArt/Sheets/T_Team*.png", "RawArt/Sheets/T_Enemy*.png", "docs/data/art/requests/swarm-units.json", "docs/data/art/requests/team-units.json", "docs/data/art/requests/enemy-units.json", "docs/data/art/brood-variants.json", "docs/data/art/team-variants.json", "docs/data/art/provenance.json", "docs/data/art/sprite-request.schema.json", "Scripts/art/pixelpipe.py", "Scripts/art/check_brood_variants.py", "docs/perf/niagara-sprite-path.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
epic: unique-knights
evidence: A PIE capture at gameplay density where the player's own units visibly show several different knight looks at once — and unlike the brood these ARE legible, because retinue art is pale — plus a second capture at a skewed team weight table proving the weights drive it. Draw calls stated as a measured before/after (1 → 2 is expected and acceptable), with a frame-time row at 1k/10k/40k showing the second emitter costs nothing.
score: {feel: 2, risk: 2, cost: 3}
source: user
teammate: team-enemy-atlas
decided: "2026-07-30 done"
model: sonnet
---

## Why now
Owner, 2026-07-29: *"can we implement the knight family into the group. I know we are doing
atlas sprites to save up on material costs I assume. Can we just make a team and enemy atlass
and use that for materials as the enemies will change from time to time."*

Two asks, and they are the same piece of work. Doing the split without populating the team atlas
would land an architecture change with nothing on screen — the exact trap `task-059` fell into,
where the whole mechanism was proven by instrumentation because nobody could photograph it.
Building the team atlas *means* filling it, so the knight family rides in and this task has a
visible result. **One repack instead of two.**

## Correcting the premise, because it changes what to optimise for
The single atlas is not about material cost — it is about **draw calls**. One Niagara sprite
renderer, one material, one texture = one draw call for the entire horde at any count.

**But `task-059` measured the cost and it is nothing:** `draw_ms 0.001`, `game_ms` 11→29 across
1k→40k brood. Rendering is free and the Mass sim is 100% of the frame, consistent with the
earlier 433fps one-camera verdict. **So going 1 → 2 draw calls is acceptable and is not a
regression** — do not contort the design to preserve a single call. This reverses the bar
`task-059` shipped under; that bar was right for a change that added no renderer and is wrong
here. State the before/after count honestly rather than hiding a second renderer.

## What the split actually buys, and the arithmetic behind it
**Churn isolation.** Today, changing enemies means repacking the one texture that also holds the
team, then changing `SwarmSheet::Rows`, then the Sub UV inside `NS_Swarm`. Three things must
agree and a mismatch fails *silently* — `SwarmFragments.h:44` says so explicitly, and it is what
nearly sank `task-059`. Split, and the team atlas goes stable while the enemy atlas churns.

**And the ceiling is closer than it looks.** At 56px cells against an 8192 texture limit, one
shared atlas tops out near **73 variants** (2 rows each, walk0/walk1). Measured on disk right
now, all confirmed fitting the current 56px cell:

| family | variants | side | max content | note |
|---|---|---|---|---|
| `brood-ooze` state00..08 | 9 | enemy | 53×47 | already packed, rows 0-17 |
| `undead-simple` | 22 | enemy | 50×49 | not packed — a whole enemy roster waiting |
| `knight-melee-v1` keeps | 5 | team | 46×44 | judged, kept |
| `knight-melee-v2` keeps | 5 | team | 50×48 | judged, kept |
| retinue (`243c7684`) | 1 | team | 53×44 | currently rows 18-19 |

~42 of 73 already claimed without generating anything. Split: **enemy 31 variants → 62 rows →
448×3472**, **team 11 variants → 22 rows → 448×1232**. Both comfortable, each growing on its own
clock.

### Amended 2026-07-30 — pack the JUDGED knights, not the exploratory ones
This task was written on 2026-07-29 naming `knight-mass` (7), `knight-greatsword` (5) and
`knight-types` (4) as the team sources. **Those are the exploratory families and they were
superseded the same day.** `task-082` and `task-087` ran the codified variant pipeline over the
same base and produced **ten measured, judged keeps** in `knight-melee-v1` and `knight-melee-v2`,
each with a recorded verdict, a full 8-rotation set and an entry in the family manifest:

| family | kept | aspect | solidity | asymmetry | holes | mass |
|---|---|---|---|---|---|---|
| v1 | v1_narrowguard | 0.70 | 0.70 | 0.08 | 0 | 960 |
| v1 | v2_lanceout | 1.05 | 0.53 | 0.39 | 0 | 1081 |
| v1 | v3_shieldbreak | 0.95 | 0.61 | 0.74 | 0 | 1124 |
| v1 | v4_overhead | 1.05 | 0.52 | 0.17 | 2 | 1012 |
| v1 | v6_simplecolumn | 0.68 | 0.75 | 0.27 | 0 | 986 |
| v2 | v7_barestance | 0.75 | 0.65 | 0.06 | 0 | 947 |
| v2 | v8_heavycloak | 0.98 | 0.68 | 0.38 | 0 | 1283 |
| v2 | v10_bracedstaff | 1.14 | 0.46 | 0.54 | 0 | 1012 |
| v2 | v11_midguard | 0.77 | 0.69 | 0.37 | 0 | 1034 |
| v2 | v13_maceraised | 0.85 | 0.59 | 0.79 | 1 | 1162 |

Combined mass spans **947–1283 = 1.36×**, clearing the 1.3× threshold `task-082`'s five failed at
1.17×, and the 0.70→0.95 aspect gap that made v1 bimodal is filled. Every keep fits the 56px cell
(max content 50×48). **This set is the team pack. The dedup-and-measure work the original scope
described is already done, by the pipeline, on disk** — `manifest.json` carries it. Do not
re-judge it, do not pack the exploratory families, and do not generate anything.

## Design — settled, so the teammate does not re-litigate it
**Two emitters, not two renderers on one emitter, and not a two-texture material.**

  - **Two emitters** (one team, one enemy) inside `NS_Swarm`. Each gets its own sprite renderer,
    material instance, texture and **its own Sub UV grid** — which is the whole point: the team
    grid and the enemy grid become independent, so adding enemies never touches the team's rows,
    constants or Sub UV. Two draw calls, measured free.
  - **Rejected: two renderers on one emitter.** Both renderers process every particle, so each
    team pays for the other's count and you need a visibility hack to hide the wrong half.
  - **Rejected: one material sampling two textures via a dynamic parameter.** Keeps one draw
    call, but the renderer's built-in SubUV decodes against ONE grid, so two grids means doing
    the UV maths by hand in the shader and throwing the built-in away. Real shader work to buy
    a draw call that costs 0.001ms.

**The C++ bridge splits by team bit.** `SwarmRenderActor.cpp`'s pack loop currently builds one
set of `Positions`/`SubImages`/`Colors`/`Sizes` and one `Count`. It becomes two sets, partitioned
on `SwarmAnim::TeamBit`, pushed to per-emitter user parameters. Keep the existing scratch arrays
**file-static** — a member is a class-layout change and Live Coding cannot apply those.

**`SwarmSheet` becomes two grids.** One `Rows`/`Columns` pair per side, and `CellFor` needs to
know which sheet it is decoding for. Declare this properly — the "three things must agree"
comment at `SwarmFragments.h:44` now governs *two* sets of three, and that comment must say so.

## Scope, and what to do with the existing brood work
- **Move the nine oozes to the enemy atlas as-is.** They are packed, provenance-recorded and
  proven; this is a re-target, not a redo. `brood-variants.json` and its weight semantics carry
  over unchanged.
- **`undead-simple`'s 22 variants are NOT in scope** — they are the reason the split exists, and
  packing them is a follow-up once the enemy atlas is a thing that can absorb them. Do not
  balloon this task by adding them. Note in the handback how many rows they would need.
- **Pack the ten judged keeps** from `knight-melee-v1` and `knight-melee-v2` (see the 2026-07-30
  amendment above for the list and their measured axes). Separation is already judged by
  **measured silhouette** through `task-081`'s pipeline and recorded in each family's
  `manifest.json` — read it, do not re-derive it and do not judge by eye. A `team-variants.json`
  weight table still ships, because it is how a look gets retired later at `weight: 0` without a
  repack — the same pattern that worked on the oozes. Regenerate nothing.
- **`ranged-roster` is OUT and here is why**, so nobody rediscovers it: six of its eight are
  132px sources with content up to **124px tall** — more than double the 56px cell. `pixelpipe`
  correctly refuses to scale pixel art. Those need a larger cell or their own atlas. Separate
  problem, separate task.

## Done when
- Two atlases exist, two emitters draw them, and each side's Sub UV / `Rows` / `frame_map` agree
  independently of the other's.
- **The player's own units visibly show several different knight looks at gameplay density.**
  This is the real bar and it is reachable here — retinue art is pale grey-green, so it reads
  against the black floor. This task does **not** depend on `task-084`.
- A skewed team weight table visibly changes the knight mix on screen.
- Draw calls stated before/after (1 → 2 expected), with a frame-time row at 1k/10k/40k showing
  the second emitter costs nothing measurable.
- Adding an enemy variant is demonstrably a one-atlas change — say what it now takes, in steps.
- `docs/perf/niagara-sprite-path.md` and `ELVTR/SETUP-EDITOR.md` both record the two-grid layout.
  A stale Sub UV is the silent failure mode of this whole area; there are now two of them.

## Lock note — resolved 2026-07-30
`task-084` held `SwarmRenderActor.cpp` and `docs/perf/niagara-sprite-path.md`, which this task
also owns, so the two serialised. **`task-084` is `done`** — it closed 2026-07-29 having made the
brood visible (the real bug was `M_Swarm` carrying no `ParticleColor` node at all). Its locks are
released and this is now dispatchable. Its result also makes the enemy half of this task
photographable, which is the order that was wanted.

## What comes after this
`task-095` binds the team variant index to a per-sub-type stat block, so the knight you see is
the knight that fights. It depends on this task and on `task-088`. **The variant index this task
lands is therefore about to acquire combat meaning** — declare its semantics clearly.

## Spawn prompt
```
You are splitting Emberkeep's horde rendering into a TEAM atlas and an ENEMY atlas, and landing
the knight family in the team half (C:\Projects\ELVTRGAME). Read
docs/backlog/task-085-split-team-and-enemy-atlases-with-knight-family.md IN FULL FIRST. The
design is SETTLED in it — do not re-litigate the two-emitter choice, and do not redo the
measurements it already contains.

Owner, 2026-07-29: "can we implement the knight family into the group. I know we are doing atlas
sprites to save up on material costs I assume. Can we just make a team and enemy atlass and use
that for materials as the enemies will change from time to time."

FIRST, THE PREMISE CORRECTION THAT CHANGES YOUR TARGET: the single atlas was never about material
cost, it was about DRAW CALLS. But task-059 measured draw_ms at 0.001 with game_ms 11->29 across
1k->40k brood -- rendering is free, the Mass sim is the entire frame. So GOING FROM 1 TO 2 DRAW
CALLS IS FINE AND IS NOT A REGRESSION. task-059 shipped under a "draw calls unchanged" bar; that
bar is explicitly LIFTED here. Report the before/after count honestly instead of contorting the
design to preserve one call.

DESIGN, settled — TWO EMITTERS inside NS_Swarm, one per side, each with its own sprite renderer,
material, texture and OWN SUB UV GRID. That independence is the entire point: adding enemies must
never touch the team's rows, constants or Sub UV. Rejected alternatives and why, so you do not
try them: two renderers on ONE emitter (both process every particle, needs a visibility hack);
one material sampling two textures via a dynamic parameter (the renderer's built-in SubUV decodes
against ONE grid, so two grids means hand-rolling UV maths in the shader to save 0.001ms).

THE C++ BRIDGE SPLITS BY TEAM BIT. SwarmRenderActor.cpp's pack loop builds one set of
Positions/SubImages/Colors/Sizes and one Count; it becomes two sets partitioned on
SwarmAnim::TeamBit, pushed to per-emitter user params. KEEP THE SCRATCH ARRAYS FILE-STATIC -- a
member is a class-layout change and Live Coding cannot apply those. SwarmSheet becomes two
Rows/Columns pairs and CellFor must know which sheet it decodes for. The "three things must
agree" comment at SwarmFragments.h:44 now governs TWO sets of three; update it to say so.

WHAT TO PACK:
  - TEAM: the current retinue character + THE TEN JUDGED KNIGHT KEEPS. Read them from
    docs/data/art/families/knight-melee-v1/manifest.json (v1_narrowguard, v2_lanceout,
    v3_shieldbreak, v4_overhead, v6_simplecolumn) and .../knight-melee-v2/manifest.json
    (v7_barestance, v8_heavycloak, v10_bracedstaff, v11_midguard, v13_maceraised). All have full
    8-rotation sets and all fit the 56px cell (max content 50x48).
    DO NOT PACK knight-mass, knight-greatsword or knight-types. An earlier draft of this task
    named those three -- they are the EXPLORATORY families and task-082/task-087 superseded them
    the same day by running the codified pipeline over the same base and judging the results.
    Separation is ALREADY MEASURED and recorded in each manifest.json. Read it. Do not re-judge
    by eye, do not re-run variantpipe, do not dedup by hand.
  - ENEMY: MOVE the nine already-packed brood-ooze states across as-is. Re-target, not redo --
    brood-variants.json and its weight semantics carry over unchanged.
  - A team-variants.json weight table still ships -- not to judge separation (that is done) but
    because it is how a look gets retired later at weight 0 without a repack. Same pattern that
    worked on the oozes.
  - GENERATE NOTHING. No PixelLab credits. Everything named above is already on disk.

OUT OF SCOPE, explicitly:
  - undead-simple's 22 variants. They are the REASON the split exists, not part of it. Note in
    your handback how many rows they would need, then leave them.
  - ranged-roster. Six of eight are 132px sources with content up to 124px TALL, more than
    double the cell, and pixelpipe correctly refuses to scale pixel art. Separate task.
  - task-084's lighting work. Do not add an additive light term, do not retune flame dials, do
    not touch the nine ooze PNGs.

DONE WHEN:
  - Two atlases, two emitters, each side's Sub UV / Rows / frame_map independent of the other.
  - THE PLAYER'S OWN UNITS VISIBLY SHOW SEVERAL DIFFERENT KNIGHT LOOKS at gameplay density. This
    bar IS reachable, unlike task-059's: retinue art is pale grey-green and reads against the
    black floor. You do NOT need task-084 for this.
  - A skewed team weight table visibly changes the knight mix on screen.
  - Draw calls before/after, plus a frame-time row at 1k/10k/40k showing emitter 2 is free.
  - Adding one enemy variant is demonstrably a one-atlas change -- state what it now takes.
  - docs/perf/niagara-sprite-path.md AND ELVTR/SETUP-EDITOR.md record the two-grid layout. There
    are now TWO Sub UV values that fail silently when stale.

GOTCHAS -- every one of these cost a previous session real time:
  - MCP AddUserVariables CANNOT create an Array DataInterface user parameter. It returns success
    and silently does nothing, because the type needs an instance only the editor UI makes. Add
    it via the User Parameters "+" by hand, then MCP can wire the rest. You will need a full set
    of these for the second emitter.
  - MCP asset edits are IN-MEMORY until save_assets([]). A set_properties returning true is NOT
    proof -- read the value back and compare.
  - Live Coding is unusable on this module (a patch compile re-runs CVar static initialisers and
    crashed in a file the edit never touched). Use Stop-Editor; Build-Editor; Start-Editor;
    Wait-Mcp (Scripts/ue-mcp.ps1), ~7s.
  - pixelpipe validate rejected non-power-of-two ROWS as folklore (SubImageSize is a float ratio)
    and sprite-request.schema.json capped grid items at 16. Both were fixed on 2026-07-29 -- if
    you meet either again, they are the same class of bug, not a real constraint.
  - Swarm.Clear trips the game mode's wave-cleared path and immediately spawns +120
    reinforcements, so you cannot stage a one-team frame that way. Swarm.RunAfter is fine as
    delayed exec.
  - Restore ELVTR/Saved/SwarmExecOnPlay.txt byte-exact if you move any dial for a shot.
  - Do NOT run import_sprites.py with no argument -- it resaves every texture in the project,
    including eight that have nothing to do with this task.
  - The tree is shared with concurrent sessions. Build on uncommitted work you find, do not
    revert it, do not tidy it, and do not attribute it to anyone.
  - DECLARE EVERY PATH YOU WRITE. task-059 wrote four files outside its owns: because the
    declaration was too narrow. If you need a path not in owns:, say so in the handback.

You hold the unreal-editor and mcp-9000 locks. Deliver ON-SCREEN EVIDENCE from a runnable build:
knights visibly varied in the player's army, and the mix visibly changing when a weight goes to
100. Not a diff plus "it works".
```
