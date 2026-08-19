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
| Company | `UBattlegroundCommander` (reused as-is, plus `Order()`) | 1 company lead + 1 Spearmen + 1 Archers handle, live standing/centroid |
| Handle | `USwarmSubsystem` squad handle | the bodies (Mass entities) |

Team 0 takes handles `0..MaxSquads/2-1`, team 1 the rest — `AssignRecruit`'s own TeamId
split. On 32 handles that is 4 companies per side × 3 handles, 24 of 32 claimed, 625 bodies
per troop handle at 5000. A **company lead** is one body on its own handle
(`SwarmSpawn::SpawnNamed`, `LeadHPScale` 10× its rung's HP); the reserve never refills it.

Orders: each side charges **through** the enemy centroid (`ChargeOvershoot`, default 1500uu
past it along the line of advance). Field-battle Charge is slot-tethered (`SwarmProcessors.cpp`,
`BlockEngageRange*2`), so a block that reaches the enemy centroid parks there; overshoot keeps
both blocks moving through each other. When the enemy is gone a side Holds where it stands.

Reinforcement: each side holds `Reserve` bodies (default 2500). On every decision tick a handle
under `ReinforceFloor` (60%) of its start is refilled from the reserve, spawned at the side's
home zone (`SwarmSpawn::SpawnUnit`, same path as the initial muster). No invisible bodies, no
stat multipliers.

Formation dials set at console priority in `BeginPlay`: Block shape, `Columns 40`,
`GroupsPerRow` = every handle of a side abreast (`BlocksAbreast 0` = auto), `GroupRowPitch` =
one detachment's depth + 200, `GroupDepthCap 16`, `FaceCamera 0`, `RetinueSizeJitter 0`.
Looks: side A melee 1, side B melee 7, archers 0 both — and `Swarm.KnightSubtypeMap` pinned
to all-zeros so both looks read the SAME weapon row (§3).

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
- **Detachment rows are numbered per ARMY** (`BuildGroups`, 2026-08-19). The group ordinal is
  a running count over every live body of a type, so a second retinue army's blocks were laid
  out in the rows *behind* the first army's: at 12 handles a side, side B's blocks sat
  9000–14000uu +X of its own anchor and the two armies walked off the map together — on
  screen, one army running from the other. Rows trail an anchor, so restarting the ordinal per
  army is also the mirror each side should have had. Castle and Battleground are unaffected
  (one retinue army each; the brood is not retinue).

`Stress/StressWarSide.cpp`:
- Orders are clamped to the `HomeZone..EnemyHome` corridor (`ClampToCorridor`). Charge is
  slot-tethered and, until a per-army formation yaw exists, every slot offset still points the
  same world way for both teams, so "enemy centroid + overshoot" can feed its own drift back
  in. The clamp bounds it: the aim may lead the enemy, never leave the field.

## 4. Measurements

### 4.1 First measurement — 2026-08-18, in-editor PIE, 5000 v 5000 + 2500 reserve each

`docs/perf/evidence/stress-war-5000v5000-2026-08-18.csv`, 8 handles, 2 companies a side.
Frame/game ms by bodies on the field (rows with the editor unfocused — 333ms frames — dropped):

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

### 4.2 Phase B — 2026-08-19, 32 handles, 4 companies a side (24 handles claimed)

`docs/perf/evidence/stress-war-32h-2026-08-19.csv`, same 5000 v 5000 + 2500 reserve, 625 per
troop handle. In-focus rows only (18 of 56; the run was cut at t=112 when another session took
the editor):

| bodies | frame ms | game ms | rows |
|---|---|---|---|
| 10000–11000 | 36 | 31 | 3 |
| 9000–10000 | 36 | 26 | 1 |
| 8000–9000 | 34 | 32 | 3 |
| 5000–6000 | 15 | 14 | 11 |

Three times the handles at the same body count did not move the cost curve — the frame is the
bodies, not the command structure. (Rows are few because the editor throttles to 3fps whenever
it is not the foreground window; see the observations below.)

**What Phase B actually cost was correctness, not frames.** Three separate asymmetries, each
measured, each fixed:

| symptom | cause | fix |
|---|---|---|
| One army walks off the map, the other chases it | detachment rows numbered across BOTH armies (§3) — side B's blocks sat 9000–14000uu +X of its own anchor | per-army group ordinal |
| Side A loses ~2:1 | `Swarm.KnightSubtypeMap` sends look 1 → row 8 (HP 114 / DPS 25) and look 7 → row 1 (HP 165 / DPS 30) | the level pins the map to all-zeros: one row, different sprites |
| One clash, then a 4200uu standoff at ~0 kills | wrapped formation rows — a block parks at `anchor + slot`, and a row behind the front is a whole `GroupRowPitch` further along | `BlocksAbreast 0` = every handle of a side in ONE row |

Before: `END (time) at t=120s: A 1498 / B 3857`, gap growing the whole match, side A shot in
the back while it chased. After: `END (time) at t=121s: A 2593 / B 2562` — a 1.2% spread from
5000 + 2500 a side, both reserves spent. The 12-abreast run closes to ~1600uu and grinds there
while any melee lives.

Observations to chase, not fixed here:
- **The two armies still both FACE world +X** — `Swarm.Formation.FaceCamera 0`, `Yaw 0`, and no
  per-army yaw dial exists. The mirror here is stats and grid, not facing.
- **The end-game is an archer standoff.** Once both sides' melee is spent the surviving archer
  blocks sit at their slots ~4400uu apart and attrition stops: archers never close, and Charge
  only walks them to the ordered anchor.
- A company lead is one body with 10x HP on its own handle. Nothing crowds it, so it outruns
  its company and fights alone out front — fine for a stress harness, wrong for a game.
- Reinforcements spawn at the home zone and walk in as a loose stream; they arrive piecemeal.
- Frame rows of 333ms are the editor's unfocused throttle, not the sim. The project already
  sets `bThrottleCPUWhenNotForeground=False` and it throttles anyway, so a cost run needs the
  editor as the foreground window.

## 5. Phase B landed — 2026-08-19

`USwarmSubsystem::MaxSquads` 8 → 32 (5 id bits + type + army fit the squad `uint8`, bit 7
spare; the render int32 moved `SquadMask` to 6 bits at shift 17 and `VariantShift` 21 → 23) →
4 companies per side × (1 lead + 2 troop handles), 24 of 32 claimed. `GarrisonUnit` is pinned
to `NamedSoldiers` rather than `MaxSquads - 1`, or the castle garrison would have moved from
handle 7 to 31. Blast-radius survey: plan file `silly-hopping-biscuit.md` §Phase B.

Canon conflict, logged as adaptation item 7 in `docs/OPEN-DECISIONS.md`: `adaptation.md` §6
says "a captain plus its retinue is one handle" — the inverse of the **company lead** here
(one named body on its own handle over several troop handles). Neither is canon yet.

Next, if this level grows: a per-army formation yaw (the facing mirror), and a decision that
keeps a lead with its company instead of ahead of it.
