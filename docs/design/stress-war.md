# StressWar — 5000 v 5000 symmetric field battle

**What this is:** a stress level and its game mode (`AStressWarGameMode`,
`ELVTR/Source/ELVTR/Stress/`) — two identical formed armies on an open field, run by a
two-level war manager, with per-tick cost written to CSV. Not a game level: no hero, no brood,
no director, no upkeep. It answers "what does the Mass sim cost at 2 x N formed bodies, and
where does it fall over", and it is the first place a *side → company → handle* structure
exists in code.

## 1. Play it

- `open L_StressWar` — `/Game/StressWar/L_StressWar`, a duplicate of `L_Spike1` (same
  `SwarmRenderActor`, demichrome post-process, floor) with World Settings pinning the mode.
  Made by hand in the editor (right-click `L_Spike1` → Duplicate, move to `Content/StressWar`);
  `py "Scripts/stresswar_level.py"` from the editor console pins the game mode and saves.
- Or, on any map: `open L_Spike1?game=/Script/ELVTR.StressWarGameMode`.
- URL options: `?PerSide=500?Reserve=0?Companies=2?MaxSeconds=60`.
- Cost: `stat unit`, `stat Swarm`, and `Saved/StressWar.csv`
  (`t,standingA,standingB,reserveA,reserveB,gap_uu,frame_ms,game_ms,render_ms,gpu_ms`, one
  row per decision tick). `Swarm.DebugShotAfter 1` for a raw top-down capture.

## 2. Structure

| Layer | Class | Owns |
|---|---|---|
| Side | `UStressWarSide` (`Stress/StressWarSide.h`) | team id, home zone, reserve pool, its companies; `Decide()` aims every company and refills thinned handles |
| Company | `UBattlegroundCommander` (reused as-is, plus `Order()`) | 1 Spearmen + 1 Archers handle, live standing/centroid |
| Handle | `USwarmSubsystem` squad handle | the bodies (Mass entities) |

Team 0 takes handles `0..MaxSquads/2-1`, team 1 the rest — `AssignRecruit`'s own TeamId
split. On today's 8 handles that is 2 companies per side, 1250 bodies per handle at 5000.

Orders: each side charges **through** the enemy centroid (`ChargeOvershoot`, default 1500uu
past it along the line of advance). Field-battle Charge is slot-tethered (`SwarmProcessors.cpp`,
`BlockEngageRange*2`), so a block that reaches the enemy centroid parks there; overshoot keeps
both blocks moving through each other. When the enemy is gone a side Holds where it stands.

Reinforcement: each side holds `Reserve` bodies (default 2500). On every decision tick a handle
under `ReinforceFloor` (60%) of its start is refilled from the reserve, spawned at the side's
home zone (`SwarmSpawn::SpawnUnit`, same path as the initial muster). No invisible bodies, no
stat multipliers.

Formation dials set at console priority in `BeginPlay`: Block shape, `Columns 40`,
`GroupsPerRow 2` (`BlocksAbreast`), `GroupRowPitch` = one detachment's depth + 200,
`GroupDepthCap 16`, `FaceCamera 0`, `RetinueSizeJitter 0`. Looks: side A melee 1, side B
melee 7, archers 0 both.

## 3. Sim changes this needed (field battle only, castle byte-identical)

`Mass/SwarmProcessors.cpp`:
- `FindNearestEnemyBanded` gained `HostileArmy`: in field battle a retinue body of the OTHER
  army is a steering target. Before this the retinue steering pass only ever looked for brood
  (`bWantRetinue=false`), so two formed teams bumped into each other and stalled at ~2300uu
  with zero kills once their blocks were dense (measured 2026-08-18). The combat pass already
  had the same-side/army test; steering did not.
- Archers under Charge in field battle anchor to `StanceAnchor + slot` (advance with the
  order) instead of `Attractor + slot` (there is no hero; the Attractor is one point for two
  armies).

## 4. First measurement — 2026-08-18, in-editor PIE, 5000 v 5000 + 2500 reserve each

`docs/perf/evidence/stress-war-5000v5000-2026-08-18.csv`. Frame/game ms by bodies on the
field (rows with the editor unfocused — 333ms frames — dropped):

| bodies | frame ms | game ms |
|---|---|---|
| 9000–10000 | ~38 | ~30 |
| 8000–9000 | 36 | 30 |
| 7000–8000 | 32 | 24 |
| 6000–7000 | 27 | 21 |
| 5000–6000 | 24 | 18 |
| 4000–5000 | 18 | 16 |

The game thread is the frame (`render_ms` reads 0 in PIE — `GRenderThreadTime` isn't
sampled there; `gpu_ms` 3–7). ~4500 formed bodies is where this build sits inside 16.6ms;
10k formed bodies is ~2.3x over. Consistent with the one-camera verdict (~13–20k honest
ceiling for *brood*; formed retinue costs more per body — formation repack + neighbour
queries + per-body combat).

Observations to chase, not fixed here:
- Side A loses ~2:1 every run. Not symmetric: A spawns first (type-global slot cursor →
  different detachment geometry), and looks 1 vs 7 map to weapon rows through
  `Swarm.KnightSubtypeMap`, which this level does not pin. Pin both to the same look for a
  true mirror.
- Reinforcements spawn at the home zone and walk in as a loose stream; they arrive piecemeal.
- Frame rows of 333ms are the editor's unfocused throttle, not the sim.

## 5. Phase B (next, own commit): 32 handles + company leads

Widen `USwarmSubsystem::MaxSquads` to 32 (5 id bits + type + army still fit the squad
`uint8`; render int32 has bits 25–31 free) → 4 companies per side × (1 lead + 3 troop
handles); lead = `SwarmSpawn::SpawnNamed`. Checklist and blast radius: plan file
`silly-hopping-biscuit.md` §Phase B (2026-08-18 survey). Load-bearing gotcha:
`GarrisonUnit = MaxSquads - 1` must become `= NamedSoldiers` or the castle garrison silently
moves. Canon conflict: `docs/design/adaptation.md` §6 says "a captain plus its retinue is one
handle" — the opposite topology; call ours **company lead**, log under adaptation O7.
