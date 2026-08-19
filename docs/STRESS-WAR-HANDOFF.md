# StressWar handoff — 5000 v 5000 field battle + war manager

Read this, then `docs/design/stress-war.md`. Plan file (owner-approved, Phase A + B):
`C:\Users\Hawaiian_spawn\.claude\plans\silly-hopping-biscuit.md`.

## 0. State of the tree (2026-08-19)

- **Committed** (`d239d01`): Phase A — the game mode, the side/company war manager on the old
  8-handle budget, reinforcement, CSV, the steering fix, the doc, the first cost curve.
- **Phase B**: 32 handles + company leads, plus three asymmetry fixes found while verifying it
  (per-army formation grid, pinned `Swarm.KnightSubtypeMap`, one formation row per side). See
  `docs/design/stress-war.md` §3–§5 — each is written up there with its measurement. Verified
  in PIE: `A 2593 / B 2562` at t=121s from 5000 + 2500 a side.
- **Regression, verified**: the castle still puts the seven on handles 0–6 and the garrison on
  handle 7 (`Seven: the war — garrison unit 7 holding 110 bodies`), so the
  `GarrisonUnit = NamedSoldiers` pin holds. Battleground's own `BeginPlay` line was NOT
  re-checked after the last rebuild — do that first.
- **Not done: the level asset.** `/Game/StressWar/L_StressWar` does not exist. Owner step, by
  hand in the editor: right-click `Content/Spike1/L_Spike1` → Duplicate → move to
  `Content/StressWar/L_StressWar`, then run `py "C:/Projects/ELVTRGAME/Scripts/stresswar_level.py"`
  from the editor console (pins the game mode, saves). Editor Python cannot do the duplicate
  and the load in one pass — `EditorServer.cpp:2544` "World Memory Leaks" is fatal.
  Until then: `open L_Spike1?game=/Script/ELVTR.StressWarGameMode?PerSide=5000?MaxSeconds=120`.

## 1. Open threads

- **No per-army formation yaw.** Both armies face world +X; the mirror is stats and grid only.
  This is the next real piece of sim work if the level grows.
- **Archer end-game standoff.** When both sides' melee is spent the surviving archer blocks
  park ~4400uu apart and attrition stops. Charge only walks a block to its ordered anchor.
- **Company lead topology vs `adaptation.md` §6** — logged as adaptation item 7 in
  `docs/OPEN-DECISIONS.md`, not a canon call.
- Q24 (front ledger authoritative vs derived, `docs/design/war-ledger.md`) still open; the war
  manager here is a testbed shape, not canon.
- `FAbilityState::FocusUnits` (uint8 over the seven) only needs widening when a company must be
  focus-addressed. `FSwarmAnimFragment::SquadId` stays uint8 up to 64 handles.
- The muster HUD shows one card per claimed handle in one row — 24–32 won't fit. StressWar has
  no HUD, so it doesn't bite yet.

## 2. Working notes (don't re-learn)

- **Never Live Coding** for anything in this area: new UCLASS + class-layout changes crashed the
  editor twice (reports success, crashes on reload). Use `Scripts/ue-relaunch.ps1` — and note
  `pwsh` is not installed on this machine, run it under `powershell.exe`.
- Field-battle Charge is slot-tethered (`BlockEngageRange*2` off `anchor + slot`), so a block
  that reaches its aim PARKS there — hence `ChargeOvershoot`, and hence every formation offset
  showing up as a permanent gap between the armies.
- Every handle (and every look) is its own detachment. A lead handle of one body is a whole
  detachment row unless `GroupsPerRow` puts it abreast.
- CSV `frame_ms` rows of 333.33 = the editor's unfocused 3fps throttle, not the sim (the project
  already sets `bThrottleCPUWhenNotForeground=False`; it throttles anyway). `render_ms` often
  reads 0 in PIE (`GRenderThreadTime` isn't sampled there); `gpu_ms` is real.
- Cost: ~30ms game thread at 10k formed bodies, ~14ms at 5–6k. The game thread is the frame.
- Editor Python `duplicate_asset` + `load_level` in one pass is fatal. Duplicate levels by hand.
- The Kindled MCP `Exec` returns `""` for `py` scripts; read `ELVTR/Saved/Logs/ELVTR.log`
  (`LogPython:`). Driver: `py Scripts/ue-mcp-call.py call <Tool> '<json>' <Toolset>` — tool names
  are bare (`Exec`, `StartPIE`), with the toolset as the third argument.
- `unreal.EditorPerformanceSettings` is not exposed to editor Python.
