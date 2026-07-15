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
     The index is already decoded on the CPU in `SwarmRenderActor.cpp`:
     walk frame (bit 0) + 2 × team (bit 3) → 2×2 SubUV cell. Attack tint
     (bit 1) and flip (bit 2) are optional for the spike.
5. Render section — **Sprite Renderer**:
   - Material = `M_Swarm`, **Sub UV = 2 × 2**, Alignment = Unaligned,
     Facing Mode = Face Camera.
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
