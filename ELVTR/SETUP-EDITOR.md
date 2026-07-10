# Spike 1 — Editor Setup (one-time, ~20 min)

The C++ side is complete. These editor steps create the three assets code can't
author: the Niagara system, its material, and the map. Do them in order.

## 0. Open the project
Double-click `ELVTR.uproject` (UE 5.8). If prompted to rebuild modules, accept.

## 1. Import the texture
1. Content Drawer → make folder `Content/Spike1`.
2. Drag `RawArt/T_Swarm_2bit.png` into it.
3. Open `T_Swarm_2bit`: **Filter = Nearest** (pixel art), Mip Gen Settings =
   NoMipmaps, Compression = UserInterface2D (or Masked), sRGB = on. Save.

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
   - `Positions` — type **Position Array** (or Vector Array)
   - `AnimBits` — type **Int32 Array**
   - `Count` — type **Int32**
3. Emitter Update:
   - **Spawn Burst Instantaneous**: Spawn Count = `User.Count`.
   - Emitter State: Life Cycle Mode = Self, **Loop Behavior = Infinite,
     Loop Duration = 0.0** (re-burst every frame; particles live one frame).
4. Particle Spawn:
   - **Initialize Particle**: Lifetime = 0.0, Sprite Size = (48, 48).
   - Set `Particles.Position` = `User.Positions[Exec Index]`
     (Sample from the array DI by Exec Index).
   - New scratch/int: read `User.AnimBits[Exec Index]` into a particle int
     attribute `AnimBits`.
   - **Sprite SubImage Index** = frame from bits: SubImage layout is
     2 cols × 2 rows → index = (bit0) + 2 * (bit3 team). Attack tint (bit1):
     optional — multiply Color toward red when set. Flip (bit2): negate
     Sprite Size X via Sprite Facing or SubUV mirror — optional for the spike.
5. Render section — **Sprite Renderer**:
   - Material = `M_Swarm`, **Sub UV = 2 × 2**, Alignment = Unaligned,
     Facing Mode = Face Camera.
6. Save. (If array sampling nodes differ in 5.8's UI, the invariant is:
   particle i takes Positions[i] and AnimBits[i], burst count = Count.)

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
- For N in 500, 1000, 2000, 5000, 10000: `Swarm.Clear`, `Swarm.SpawnRetinue 100`,
  `Swarm.SpawnBrood N`, wait for convergence, record Frame/Game/Draw/GPU ms
  from `stat unit` (PIE and Standalone Game).
- One Unreal Insights trace per interesting count (`-trace=default` launch arg,
  or Trace → Start Trace in editor).
