# Spike 1 — Editor Setup (one-time, ~20 min)

The C++ side is complete. These editor steps create the three assets code can't
author: the Niagara system, its material, and the map. Do them in order.

## 0. Open the project
Double-click `ELVTR.uproject` (UE 5.8). If prompted to rebuild modules, accept.

## 1. Import the texture

The sheet is **owned by the `/sprite` pipeline** — do not hand-assemble it. It is built
from `docs/data/art/requests/swarm-units.json` (a *composite* request: one texture,
two subjects) and lands at `RawArt/Sheets/T_Swarm_2bit.png`:

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

**Sheet layout (changed 2026-07-26: was 4×2, now 8×4).** 384 × 192, 48px cells.
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
1. New Material in `Content/Spike1`, name `M_Swarm`.
2. Details: Shading Model = **Unlit**, Blend Mode = **Masked**, Two Sided = on.
3. Nodes: `Particle SubUV` texture sample (assign `T_Swarm_2bit`) →
   RGB → **Emissive Color**, A → **Opacity Mask**.
4. Usage flags: check **Used with Niagara Sprites**. Save.

## 3. Niagara system `NS_Swarm`
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
     `SwarmSheet::CellFor` — `col + 8*row`, where the column is the unit's facing
     resolved against the live camera yaw (`SwarmFacing::ColumnFor`) and the row is
     the team bit plus the walk frame.
5. Render section — **Sprite Renderer**:
   - Material = `M_Swarm`, **Sub UV = 8 × 4** (updated 2026-07-26, was 4 × 2, before
     that 2 × 2 — see §1), Alignment = Unaligned, Facing Mode = Face Camera.
   - This field **can** be set from script, but only through the Niagara MCP toolset
     (`NiagaraToolset_System.SetRendererData`), not editor Python — see the sprite
     skill's "Niagara Sub UV" note for the exact calls and the save trap. It was set to
     8 × 4 that way on 2026-07-26 and read back to confirm.
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
- **Automated:** tick `Run Benchmark` on the placed SwarmRenderActor (or launch
  standalone with `-SwarmBench`). It steps through 500/1k/2k/5k/10k brood
  (+100 retinue), waits 8 s to converge, samples 5 s, and logs one
  `SwarmBench: brood=N ... frame/game/draw/gpu ms` line per count, ending with
  `SwarmBench: DONE`. Frame smoothing/VSync are disabled for the run.
- Manual alternative: `Swarm.Clear`, `Swarm.SpawnRetinue 100`,
  `Swarm.SpawnBrood N`, read `stat unit`.
- One Unreal Insights trace per interesting count (`-trace=default` launch arg,
  or Trace → Start Trace in editor).
