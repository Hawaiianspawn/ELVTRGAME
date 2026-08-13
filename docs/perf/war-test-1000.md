# 1000-entity war test — 500 garrison vs 500 brood through a chokepoint

**Measured:** 2026-08-13, standalone `-game`, L_Spike1, i7-13700K / RTX 4090,
Development editor build. Two runs, FPS-charted over the whole fight
(`StartFPSChart` t=10s → `StopFPSChart` t=120s).

## The scenario (`Kindled.WarTest`)

One console command, so it fits one `-ExecCmds` line:

```
Kindled.WarTest [perSide=500] [waves=5] [interval=20]
```

- Stands the Gate-1 run state machine down (`Kindled.War.Auto 0`), clears the field.
- **Defenders:** `perSide` garrison on handle 7, anchored Hold at `Kindled.War.Standoff`
  (700uu). Block depth is the reserve — back ranks stand out of reach and the
  formation repack feeds them forward as the front rank dies.
- **Bottleneck:** two wall segments (`USwarmSubsystem::FSwarmWall`, resolved as a
  position-slide clamp in the integrate pass) across the tide's bearing, 150uu in
  front of the line, with a 600uu gate — the garrison stands behind the wall and the
  fight happens in the gate. Author custom layouts with `Swarm.Wall.Add x1 y1 x2 y2
  [halfwidth]` *before* calling WarTest and it keeps yours.
- **Attackers:** `perSide` brood committed progressively — `waves` waves every
  `interval` seconds; the unspawned remainder is their reserve. Spawn is metered
  (never one batch) per GDD §9's 23.46ms/250 spike measurement.

Repro line used for both runs:

```
UnrealEditor.exe ELVTR.uproject L_Spike1 -game -windowed -ResX=1600 -ResY=900
  -ExecCmds="Swarm.RunAfter 3 Kindled.WarTest 500 5 12,
             Swarm.RunAfter 10 StartFPSChart, Swarm.RunAfter 120 StopFPSChart,
             Swarm.RunAfter 130 quit"
```

## Results

| Metric (avg over 110s, all 1000 in from t≈48s) | Run 1 (gate 400uu ahead of line) | Run 2 (gate 150uu, fight in the choke) |
|---|---|---|
| GameThread | 3.00 ms | 3.38 ms |
| RenderThread | 1.54 ms | 1.58 ms |
| GPU | 0.90 ms | 0.86 ms |
| Missed 60fps syncs | 0.00 % | 0.00 % |
| Hitches/min | 3.78 | 1.09 |

- **Sim LOD is doing its job unchanged:** `Swarm.SimLOD.Stride 4` /
  `NearRadius 2200` (docs/perf/one-camera-bench.md) — far marchers steer 1-in-4
  frames, the melee inside the bearer's radius steers every frame. GameThread cost
  tracks the established ~0.75ms/1000 sim line plus fixed game overhead.
- **Headroom:** at 3.4ms game thread for 1000 bodies the 60fps budget holds past
  4x this population before the sim is the wall again; the measured honest ceiling
  from the bench doc (~13k–20k) still stands.
- **The choke fights correctly:** telemetry logged brood ground from 500 → 0 by
  t≈90s against ~7 garrison lost — a 600uu gate in front of a 500-deep block is a
  meat grinder, which is the castle premise (stalemate at a held gate) showing up
  in the cheapest possible test.
- Hitches are the wave-spawn frames (100/batch); run-2's 1.09/min is near noise.

## Walls: what they are and aren't

`FSwarmWall` is a 2D capsule the integrate pass clamps positions out of; the
tangential component of movement survives, so seek naturally slides units along
the wall into the gate. No navmesh, no steering lookahead, no flow field.
`ponytail:` position-slide only — if piles form dead-centre against a long wall
(seek exactly perpendicular), add a steering-side avoidance push. Walls draw as
silver debug boxes (`ASwarmRenderActor::Tick`), invisible to the SceneCapture
debug-shot path, which films only world primitives.

## Not built (deliberately, goal said "for now")

- Grouping is the existing per-type formation detachments; no new grouping code.
- Reserves are emergent (block depth + attacker wave metering), not a system.
- The "global objectives command" (made-up battlefield objectives the player can
  take or ignore) is untouched — that is the war ledger's job
  (castle-layout.md §5.1) and deserves its own slice.
