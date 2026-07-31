# Spike 1 — Editor Setup (one-time, ~20 min)

The C++ side is complete. These editor steps create the three assets code can't
author: the Niagara system, its material, and the map. Do them in order.

## 0. Open the project
Double-click `ELVTR.uproject` (UE 5.8). If prompted to rebuild modules, accept.

## 1. Import the texture(s)

**task-085 (2026-07-30) SPLIT THIS INTO TWO ATLASES.** `swarm-units.json` /
`T_Swarm_2bit` below are SUPERSEDED and kept only as history — nothing in the shipped
NS_Swarm reads them any more. There are now two independent composite requests, two
textures, two materials and two Niagara emitters, one per side, so that retargeting the
enemy roster never touches the team's rows and vice versa:

| side | request | texture | grid | material |
|---|---|---|---|---|
| team | `docs/data/art/requests/team-units.json` | `T_Team_2bit` | 8×34 | `M_Swarm_Team` |
| enemy | `docs/data/art/requests/enemy-units.json` | `T_Enemy_2bit` | 8×18 | `M_Swarm` |

(`M_Swarm` was the pre-split material; task-085 repointed its texture from
`T_Swarm_2bit` to `T_Enemy_2bit` in place rather than renaming it, so the enemy emitter
needed no rewiring — `M_Swarm_Team` is the new one.)

Full layout, variant ordering and the render-bridge User-parameter names for both sides:
`docs/perf/niagara-sprite-path.md` §1-§2. `Scripts/art/check_brood_variants.py` checks
both sides' request/weights/C++ agreement in one run.

```
py Scripts/art/pixelpipe.py pack team-units
py Scripts/art/pixelpipe.py pack enemy-units
```

Import is scripted — run this in the **editor** console rather than dragging, so
the texture settings are applied *and read back*:

```
py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" team-units
py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" enemy-units
```

**Everything below this point through §3 describes the PRE-SPLIT single-atlas setup.**
Kept as history of how the mechanism was built; do not follow it for a fresh setup —
duplicate the enemy emitter's finished stack instead (task-085's handback has the exact
MCP call sequence that built the Team emitter from a blank `CompletelyEmpty` template,
since `NiagaraToolset_System.AddEmitter` needs a template EMITTER ASSET and neither side
was ever one).

### Original single-atlas notes (superseded)

The sheet was **owned by the `/sprite` pipeline** — do not hand-assemble it. It was built
from `docs/data/art/requests/swarm-units.json` (a *composite* request: one texture,
two subjects) and landed at `RawArt/Sheets/T_Swarm_2bit.png`:

```
py Scripts/art/pixelpipe.py validate swarm-units
py Scripts/art/pixelpipe.py pack     swarm-units
py Scripts/art/pixelpipe.py report   swarm-units
```

Import is also scripted — run this in the **editor** console rather than dragging, so
the texture settings are applied *and read back*:

```
py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" swarm-units
```

It sets Filter = **Nearest**, NoMipmaps, UserInterface2D, sRGB on, then verifies them
and logs `SPRITE import ... [OK]` or `[VERIFY FAILED]`. If you do import by hand
instead, set those four yourself — Nearest is the one that silently ruins pixel art.

**Sheet layout — SUPERSEDED by the task-085 split; kept only to date the cell change.**
The 8×20 / 448×1120 combined `T_Swarm_2bit` sheet this paragraph described is retired.
Live as of task-126: **`T_Enemy_2bit` 448×1008 (8×18)** and **`T_Team_2bit` 448×1904
(8×34)**, per the table at the top of this file. Rows are deliberately NOT a power of two
on either sheet: `SubImageSize` is a float ratio, so 18 or 34 rows decode exactly as well
as 16, and rounding up would cost megabytes of transparent texels. The cell grew because
the retinue character that
replaced the knight measures 41×49 and `pixelpipe pack` refuses to scale pixel art — see
`docs/data/art/requests/swarm-units.json` `output.cell_note`. Only `T_Swarm_2bit` moved;
every other request still packs at 48px, and nothing in C++ reads the cell size.
**The column axis is now FACING, not animation state:**

```
col:    0     1     2     3     4     5     6     7
        S    SE     E    NE     N    NW     W    SW
row 0: brood walk0  (all eight columns hold the same south frame —
row 1: brood walk1   the brood has no rotations yet)
row 2: retinue walk0 (eight real facings)
row 3: retinue walk1
```

The attack and hit columns are **gone**, on the owner's call (2026-07-26). Attack still
reads — it was always also a lunge, and that lunge is now the whole tell. Hit does
**not** read on the sprite path any more: the debug renderer still flashes, but Niagara
has no per-particle colour array here, so restoring it needs a new array plus a graph
edit. `SwingBit`/`HitFlashBit` are still written and still used by the debug renderer —
they are simply no longer decoded into a cell.

The same grid is declared in C++ as `SwarmSheet` (`Mass/SwarmFragments.h`) and is
**not** readable by the Niagara asset — if you change it, change §3.5's Sub UV too.
Getting them out of step is silent and looks like every unit wearing the wrong frame.

Column order is south-first, counter-clockwise, matching the order PixelLab returns
rotations in and the `frame_map` in `docs/data/art/requests/swarm-units.json`.

`report swarm-units` prints a **PLACEHOLDER** verdict while any cell is unmanaged art.
As of 2026-07-26 all 32 are: the brood has no prose spec to generate against (canon has
no antagonist yet) and the retinue is owner-supplied art from the web Character Creator
rather than a managed request. That verdict is the signal — treat the sprites as a
stand-in until it reads PASS.

**Known gap:** only the *south* column has true walk frames. The other seven duplicate
their idle rotation into both walk rows, so a unit walking south bobs and one walking
north is static. One 8-direction template walk (~8 generations) fills them in with no
layout, decode or Sub UV change.

## 2. Material `M_Swarm`

**task-085: there are now two, `M_Swarm` (enemy, samples `T_Enemy_2bit`) and
`M_Swarm_Team` (team, samples `T_Team_2bit`) — `M_Swarm_Team` is a straight
`AssetTools.duplicate` of `M_Swarm` with the `ParticleSubUV` node's `texture` property
repointed. Same graph, same Emissive/Opacity wiring, otherwise.**

1. New Material in `Content/Spike1`, name `M_Swarm`.
2. Details: Shading Model = **Unlit**, Blend Mode = **Masked**, Two Sided = on.
3. Nodes: `Particle SubUV` texture sample (assign `T_Swarm_2bit`) →
   RGB → **Emissive Color**, A → **Opacity Mask**.
4. Usage flags: check **Used with Niagara Sprites**. Save.

## 3. Niagara system `NS_Swarm`

**task-085: NS_Swarm now holds TWO emitters, `Swarm` (enemy — the original, unrenamed)
and `Team` (new). Every step below happens ONCE PER EMITTER, with its own User
parameter names (`User.Positions/SubImages/Colors/Sizes/Count` for `Swarm`,
`User.TeamPositions/TeamSubImages/TeamColors/TeamSizes/TeamCount` for `Team`) and its
own Sub UV (8×18 vs 8×34 — the team grid grew from 8×22 in task-126, which appended the
six archer looks as rows 22-33). See docs/perf/niagara-sprite-path.md §1-§5 for the full
table and the exact parameter/dynamic-input chain each field needs.**

**CPUSim is not negotiable on either emitter** — GPUComputeSim was tried and drew
nothing (docs/perf/niagara-sprite-path.md §6).

1. New → Niagara System → **Empty** system, name `NS_Swarm`, add one **empty
   emitter** (GPU: in Emitter Properties set **Sim Target = GPUCompute Sim**,
   Calculate Bounds Mode = Fixed, generous fixed bounds e.g. ±100000).
2. **User parameters** (system level — names must match `SwarmRenderActor.cpp`):
   - `Positions` — type **Position Array**
   - `SubImages` — type **Float Array** (SubUV frame index, decoded on the CPU)
   - `Count` — type **Int32**
3. Emitter Update:
   - **Spawn Burst Instantaneous**: Spawn Count = `User.Count`.
   - Emitter State: Life Cycle Mode = Self, **Loop Behavior = Infinite,
     Loop Duration = 0.0** (re-burst every frame; particles live one frame).
4. Particle Spawn:
   - **Initialize Particle**: Lifetime = 0.0, Sprite Size = (48, 48).
   - Set `Particles.Position` = `User.Positions[Exec Index]`
     (Select Position From Array, Direct Set by `Engine.ExecIndex`).
   - Set `Particles.SubImageIndex` = `User.SubImages[Exec Index]`
     (Select Float From Array, Direct Set by `Engine.ExecIndex`).
     The index is already decoded on the CPU in `SwarmRenderActor.cpp` via
     `SwarmSheet::Enemy::CellFor` / `::Team::CellFor` — `col + 8*row`, where the column is
     the unit's facing resolved against the live camera yaw (`SwarmFacing::ColumnFor`) and
     the row is `Variant*2 + WalkFrame`. Post-split the retinue is NOT rows 18/19 of this
     sheet — it is variant 0 of the team sheet, on the other emitter.
   - Set `Particles.Color` = `User.Colors[Exec Index]` and `Uniform Sprite Size` =
     `User.Sizes[Exec Index]`, both on `InitializeParticle`, both the same Direct Set /
     `Engine.ExecIndex` pattern (added 2026-07-29 — before that the hit flash, the distance
     gradient and `Swarm.BroodSizeJitter` all did nothing in the world view).
5. Render section — **Sprite Renderer**:
   - **Two emitters, two Sub UVs, one per sheet — each matches its OWN texture's row
     count, not the row range the C++ addresses.** Emitter `Swarm` (enemy): Material =
     `M_Swarm` → `T_Enemy_2bit` (448×1008), **Sub UV = 8 × 18**. Emitter `Team`: Material
     = `M_Swarm_Team` → `T_Team_2bit` (448×1904), **Sub UV = 8 × 34**. Both Alignment =
     Unaligned, Facing Mode = Face Camera.
   - The old **8 × 20** value recorded here was for the retired combined `T_Swarm_2bit`
     and is no longer live on either emitter (corrected 2026-07-31, task-126). Do not
     "restore" it: 8 × 18 against `T_Enemy_2bit` is exact, because the enemy emitter does
     not sample the 20-row sheet. `SwarmSheet::Enemy::Rows` 18 and the texture's 18 rows
     agree; the 20-row `T_Swarm_2bit` is only `SwarmSheet::Legacy`'s business.
   - This field **can** be set from script, but only through the Niagara MCP toolset
     (`NiagaraToolset_System.SetRendererData`), not editor Python — see the sprite
     skill's "Niagara Sub UV" note for the exact calls and the save trap. It was set to
     8 × 4 that way on 2026-07-26, 8 × 20 on 2026-07-29, and the Team emitter to 8 × 34 on
     2026-07-31, read back to confirm each time.
   - Adding a new User ARRAY parameter is a different story and the MCP route does NOT work
     for it — `AddUserVariables` returns success and silently does nothing for data-interface
     types. Use the `Swarm.NiagaraEnsureArrays` console command instead
     (docs/perf/niagara-sprite-path.md §5).
   - A stale Sub UV is silent and just draws the wrong frame on every unit, so after any
     sheet change, read it back (or open `NS_Swarm` and check by eye) before judging the
     result.
6. Save. (If array sampling nodes differ in 5.8's UI, the invariant is:
   particle i takes Positions[i] and SubImages[i], burst count = Count.)

## 4. Map `L_Spike1`
1. New Level → **Basic** (has floor + light), save as `Content/Spike1/L_Spike1`.
2. Scale the Floor to ~200 × 200 m (X/Y scale ≈ 400). Neutral gray material.
3. Delete or dim anything fancy (sky fog etc. — this is a benchmark).
4. World Settings → GameMode Override = **Spike1GameMode** (also set as project
   default already).
5. Place a **Swarm Render Actor** (search in Place Actors) at origin; on its
   Niagara component assign **NS_Swarm**.
6. Save.

## 5. Smoke test
1. PIE (Play In Editor). WASD moves the cube-hero; camera is top-down.
2. Console (`): `Swarm.SpawnRetinue 100` → light blobs ring the hero and follow.
3. `Swarm.SpawnBrood 1000` → dark tide converges from off-screen.
4. `stat unit` + `stat fps` → confirm 60fps at 1k. Green/yellow HUD lines show
   entity count and cumulative hero contacts.

## 6. Benchmark (fills docs/SPIKE1-RESULTS.md)

**task-085: draw calls went 1 → 2 (one NiagaraSpriteRendererProperties per emitter,
each its own material/texture) — measured as no cost, see
docs/perf/niagara-sprite-path.md §7 for the game/draw/gpu ms table at 1k/10k/40k brood.**

- **Automated:** tick `Run Benchmark` on the placed SwarmRenderActor (or launch
  standalone with `-SwarmBench`). It steps through 500/1k/2k/5k/10k brood
  (+100 retinue), waits 8 s to converge, samples 5 s, and logs one
  `SwarmBench: brood=N ... frame/game/draw/gpu ms` line per count, ending with
  `SwarmBench: DONE`. Frame smoothing/VSync are disabled for the run.
- Manual alternative: `Swarm.Clear`, `Swarm.SpawnRetinue 100`,
  `Swarm.SpawnBrood N`, read `stat unit`.
- One Unreal Insights trace per interesting count (`-trace=default` launch arg,
  or Trace → Start Trace in editor).
