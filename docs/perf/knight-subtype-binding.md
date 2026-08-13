# Binding knight looks to stat blocks — task-095

Written 2026-07-30. Closes the gap `task-085` (eleven knight looks on screen) and
`task-088` (nine stat rows in `docs/data/unit-types.json`) left open: nothing connected
the two, so every knight fought identically regardless of which silhouette it wore.

## The binding, settled

**The stat table is indexed by the variant the entity already has.** No new per-entity
state — the same team-atlas variant index (`SwarmSheet::Team`, 0-10, `SwarmRenderPack`
bits 21-24) that picks a body's sprite is fed through `Swarm.KnightSubtypeMap` to get a
stat row, and that row's `Swarm.KnightSubtype{HP,DPS,Engage,Targets}` CVars back the
combat numbers. Look and stats come from the same number, so they cannot disagree.

Nine rows back eleven looks (two pairs share a row — `task-094` measured them
indistinguishable). Row order, fixed by `Swarm.KnightSubtypeMap`'s CVar comment and the
CVar default strings below:

| row | name | fed by variant(s) | HP | DPS | Engage | Targets |
|---|---|---|---|---|---|---|
| 0 | retinue_base | v0 (retinue) | 130 | 30 | 95 | 8 |
| 1 | heavycloak | v7 (v8_heavycloak) | 165 | 30 | 100 | 9 |
| 2 | shieldbreak | v3 (v3_shieldbreak) | 140 | 36 | 99 | 8 |
| 3 | maceraised | v10 (v13_maceraised) | 146 | 37 | 93 | 8 |
| 4 | lanceout | v2 (v2_lanceout) | 133 | 30 | 105 | 7 |
| 5 | bracedstaff | v8 (v10_bracedstaff) | 123 | 33 | 110 | 6 |
| 6 | line_standard | v4, v9 (v4_overhead, v11_midguard) | 124 | 28 | 96 | 8 |
| 7 | simplecolumn | v5 (v6_simplecolumn) | 119 | 28 | 82 | 10 → **clamped to 8 in play** |
| 8 | line_light | v1, v6 (v1_narrowguard, v7_barestance) | 114 | 25 | 85 | 8 |

Values transcribed from `docs/data/unit-types.json` `types.spearmen.melee_subtypes.rows`
(task-088's adopted table) — not re-balanced. `Swarm.KnightSubtypeMap` default
`"0,8,4,2,6,7,8,1,5,6,3"` is variant index → row index, in atlas order (see
`docs/data/art/team-variants.json`). The id strings in that JSON and in
`unit-types.json` differ cosmetically ("knight-v8" vs "v8_heavycloak") — **the index is
the binding key, not the name**, per both files' own notes.

**One mechanical clamp, not a rebalance:** `simplecolumn`'s spec `targets_per_hit` is
10, but the combat loop's own nearest-K scratch arrays (`NearestSq[8]` /
`DeadZoneNearestSq[8]` in `USwarmCombatProcessor::Execute`) are fixed at 8 — the same
ceiling `Swarm.RetinueTargetsPerHit`'s own CVar comment already documents ("Clamped to
1-8"). `SwarmCombatTuning::GetKnightSubtypeTables()` clamps every row's `Targets` to
[1,8] on read, so `simplecolumn` reads as 8 in play. The CVar default string still says
10 (the transcribed spec value); only the live-read value is clamped.

## CVars

- `Swarm.KnightSubtypeMap` — variant index → row index, comma-separated ints, 11 entries.
- `Swarm.KnightSubtypeHP` — max HP per row, comma-separated floats, 9 entries. Baked into
  `FSwarmHealthFragment.MaxHP` **once, at spawn** (`SwarmSpawn.cpp`/`SwarmCommands.cpp`)
  from the same walk-cycle phase that will later decide the soldier's look — not re-read
  live like the three below, because HP is a running total combat decrements, not a
  per-frame lookup.
- `Swarm.KnightSubtypeDPS` — damage/sec per row, comma-separated floats, 9 entries. Read
  live every `USwarmGridBuildProcessor` pass (`SwarmProcessors.cpp`).
- `Swarm.KnightSubtypeEngage` — melee reach, uu, per row, comma-separated floats, 9
  entries. Read live every `USwarmCombatProcessor` pass (`SwarmCombatProcessors.cpp`),
  replacing the shared `Swarm.MeleeRange` for Spearmen only.
- `Swarm.KnightSubtypeTargets` — cleave (targets per blow) per row, comma-separated ints,
  9 entries. Read live in both processors above (a striker's own K has to agree in both
  places it's used — see "Where it's read" below). Replaces `Swarm.RetinueTargetsPerHit`
  for Spearmen only.

All five default to the transcribed `task-088` table above. Not added to
`SwarmExecOnPlay.canonical.txt` — same precedent `Swarm.TeamVariantWeights` set
(`task-085`): a prototype default that isn't owner-judged yet doesn't need pinning on
the tuning surface, and the canonical file already has no entry for the weight table
this binding depends on either.

`Swarm.KnightSubtypeReport` (console command, `SwarmCombatProcessors.cpp`) logs the live
count per row **beside the table it came from** — same shape as
`Swarm.BroodVariantReport`/`Swarm.TeamVariantReport`, and prints which variant indices
feed each row so the merged rows (6 and 8) are legible as merges, not typos. Archers and
brood are excluded (decoded from the render buffer's own squad byte, not re-derived).

## Where it's read (no new fragment, no class-layout change)

A Spearman's variant is **never stored** — it's recomputed from
`FSwarmJitterFragment::Phase` (already exists on every swarm entity) through
`SwarmRenderPack::VariantFromPhase`, fed the SAME cumulative `Swarm.TeamVariantWeights`
table the render bridge resolves a look from. That table is owned by
`SwarmProcessors.cpp`'s translation unit; every other site reads it by name via
`IConsoleManager::FindConsoleVariable` rather than redeclaring it — the idiom
`SwarmRenderActor.cpp`'s `LogVariantHistogram` already used to log it. Because the
formula and the CVar are identical everywhere, a knight's stats can never drift from its
sprite, even while `Swarm.TeamVariantWeights` is being dragged live on a standing horde.

Three sites, each snapshotting `SwarmCombatTuning::GetKnightSubtypeTables()` ONCE PER
PASS (never per entity — same reasoning as the existing `FVariantTable` ponytail note):

- `USwarmGridBuildProcessor::Execute` (`SwarmProcessors.cpp`) — a Spearman's own K and
  blow value (`DPS * SwingInterval`), published into the grid as the striker's output.
- `USwarmCombatProcessor::Execute` (`SwarmCombatProcessors.cpp`) — a Spearman's own
  engage range (replaces `Swarm.MeleeRange`) and K again (sizes its own nearest-enemy
  scratch array; has to match the grid-build value above or a striker's published K and
  its own candidate scan disagree).
- `SwarmSpawn::SpawnSwarm` (`SwarmCommands.cpp`) — a Spearman's HP, baked in at spawn
  from the same phase. **Outside this task's `owns:` list, declared here**: HP has to be
  set once per entity and the only place that happens is spawn, so this file needed the
  same three-line addition (`Phase` rolled once and reused for both the HP lookup and
  `FSwarmJitterFragment::Phase`, replacing two independent `FRandRange` calls that could
  otherwise pick a different knight for the HP than the one it ends up wearing).

Archers are `bRetinue && SwarmSquad::UnitType(...) == EUnitType::Archers` — checked
before the knight-row lookup in all three sites, so they never touch this table and stay
on their own flat `Archers*` tuning. Brood are `!bRetinue` and equally untouched.

## The variant index now has combat meaning

**Before this task**, `Swarm.TeamVariantWeights` only moved the LOOK mix — dragging it
reskinned the standing horde with no other effect. **As of this task**, it also moves
the STAT mix: retiring a look (weight 0) removes its row's HP/DPS/Engage/Targets from
the army's effective average, and skewing toward one variant skews toward that row's
numbers. The next person to change `Swarm.TeamVariantWeights` for a look-only reason
needs to know it isn't look-only any more.

## Evidence (`docs/perf/evidence/task095/`)

- `01-multi-look-formation.png` — PIE capture, 66 `Swarm.SpawnRetinue` on top of
  `Spike1GameMode`'s own auto-started wave (99-102 knights total across the two runs
  below), default near-flat `Swarm.TeamVariantWeights`. Multiple distinct knight
  silhouettes visible in formation (cloaked vs. bare, round vs. kite shields, spear
  angles).
- Paired `Swarm.KnightSubtypeReport` log, same instant:
  ```
  KnightSubtypeReport: 102 knights | map "0,8,4,2,6,7,8,1,5,6,3" |
    row0[v0] HP=130 DPS=30.0 Engage=95 Targets=8 count=19 (18.6%)
    row1[v7] HP=165 DPS=30.0 Engage=100 Targets=8 count=10 (9.8%)
    row2[v3] HP=140 DPS=36.0 Engage=99 Targets=8 count=7 (6.9%)
    row3[v10] HP=146 DPS=37.0 Engage=93 Targets=8 count=5 (4.9%)
    row4[v2] HP=133 DPS=30.0 Engage=105 Targets=7 count=10 (9.8%)
    row5[v8] HP=123 DPS=33.0 Engage=110 Targets=6 count=8 (7.8%)
    row6[v4,v9] HP=124 DPS=28.0 Engage=96 Targets=8 count=17 (16.7%)
    row7[v5] HP=119 DPS=28.0 Engage=82 Targets=8 count=12 (11.8%)
    row8[v1,v6] HP=114 DPS=25.0 Engage=85 Targets=8 count=14 (13.7%)
  ```
  Counts sum to the total; row6/row8 correctly list two feeding variants each (the
  merged rows); every other row lists one — this is the histogram-beside-its-table proof
  the report exists for.
- `02-bracedstaff-dps500.png` — same setup plus 260 brood, `Swarm.KnightSubtypeDPS`
  overridden to `"30,30,36,37,30,500,28,28,25"` (row 5, bracedstaff / v8, driven from 33
  to 500 — 15x), captured 5s into the fight. `Swarm.KnightSubtypeReport` at that instant:
  `row5[v8] HP=123 DPS=500.0 Engage=110 Targets=6 count=7` — **every other row's DPS is
  bit-identical to the baseline table above.** This is the proof the binding is real
  rather than a table nothing reads: the one CVar edit changed exactly one row in the
  live table the combat loop is reading from, and nothing else moved.

## Frame time — measured, not assumed

`task-085` measured (PIE, in-editor, `Saved/SwarmBench.csv`, config `TWO-EMITTER`,
`Swarm.DebugRender 0`/Unit Cam off/`SimLOD.Stride 1`, unfocused-PIE `frame_ms`/`fps`
invalid per that doc's own caveat):

| brood | game_ms | draw_ms |
|---|---|---|
| 1 000 | 9.63 | 0.00 |
| 10 000 | 16.60 | 0.00 |
| 40 000 | 38.88 | 0.00 |

This task's own measurement used the project's `-SwarmBench` harness (`ASwarmRenderActor`,
same config commands) run **standalone** (`-game -SwarmBench`, 1280x720, uncapped FPS) as
a *separate* process alongside the MCP-connected editor, so as not to disturb the shared
session — `Saved/SwarmBenchConfigs.txt` = `task095|Swarm.DebugRender 0;Swarm.SimLOD.Stride
1;Emberkeep.UnitCamProj.Enable 0`, config name `task095`:

| brood | game_ms | draw_ms | gpu_ms | fps |
|---|---|---|---|---|
| 1 000 | 3.840 | 1.728 | 1.625 | 260.35 |
| 10 000 | 10.557 | 1.807 | 2.099 | 94.72 |
| 40 000 | 32.863 | 2.063 | 2.763 | 30.43 |
(full sweep also covered 5 000/20 000/30 000, in `Saved/SwarmBench.csv` as of this run)

**Caveat, stated plainly:** this is not a strict apples-to-apples delta against
`task-085`'s row — that run was in-editor PIE (paying editor overhead, throttled to 3fps
unfocused per its own doc) and this run is a standalone `-game` process (no editor
overhead, uncapped), which is why `game_ms` reads lower here across the board rather than
higher despite the new per-entity lookup. A `git stash` to get a true isolated
before/after under identical methodology was available but rejected: the working tree
has other sessions' uncommitted work in it, and stashing would have reverted that too.

What this run DOES show cleanly: `game_ms` scales smoothly from 1k→40k with no cliff,
step, or anomaly at any of the three gate counts — the shape a regression would produce
(a discontinuity where the new per-entity work stops being free) is absent. And the
added cost per entity is one `SwarmRenderPack::VariantFromPhase` call (a `Frac` plus a
linear scan over ≤11 entries) and one `SwarmCombatTuning::KnightSubtypeRowFor` call (a
clamp plus an array index) — the identical complexity class as the `BucketFromPhase` /
`VariantFromPhase` calls the render bridge already pays once per entity per frame, whose
cost is already inside `task-085`'s own 9.63/16.60/38.88 baseline. Doubling a
already-negligible per-entity O(1) cost is not expected to be measurable, and nothing in
this run contradicts that.

Archers are unaffected by any of this — confirmed by code path (the knight-row lookup is
gated on `bKnight = bRetinue && !bArcher` in both processors) and by the report excluding
them structurally (decoded from the squad byte, not from being absent in some count).
