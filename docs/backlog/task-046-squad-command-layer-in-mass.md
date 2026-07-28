---
id: 046
title: Build the squad command layer in Mass — sticky SquadId, per-squad stance, published centroids
status: proposed
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/**", "ELVTR/Source/ELVTR/UI/UnitCamDirector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamDirector.h", "ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamProjector.h"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [45]
evidence: A PIE screenshot where Army View blocks sit at their squads' real world positions and two squads holding different stances tint differently, plus a demonstration that a casualty in one squad does not move any soldier between squads.
score: {gate: 2, risk: 2, cost: 3}
source: docs/design/squad-group-system.md
teammate: ""
decided: ""
---

## Why now
Two things converge here.

**A real defect ships today.** `SquadIdForSlot(Slot) = Slot / SquadTargetSize` derives squad
membership from the dense formation-repack slot index, and `NeedsFormationRepack()` is
`AliveRetinue != PackedRetinueCount` — so *any* casualty anywhere renumbers slots and
silently moves soldiers between squads. Harmless while squads are cosmetic. The moment a
squad owns a stance, a soldier inherits an order it was never given. Verified in
`SwarmSubsystem.h`; documented in `docs/design/squad-group-system.md` §1.3.

**And task-045 shipped a placeholder because this layer doesn't exist.** Army View arranges
its blocks on a *fake ring* around the bearer, and tints every block with the single global
`GetStance()`, because `USwarmSubsystem` exposes only `GetRenderPositions()` and
`GetRenderAnimBits()` — `PushRenderEntry` takes a `SquadId` and throws it away after bumping
`SquadStanding[]`. Block size and count are real; positions and tint are not. This task is
what makes that view mean something.

## Done when
- **`SquadId` is sticky.** Assigned once at recruit time and persistent for that soldier's
  life, independent of formation repacking. A casualty in squad 5 must not move any soldier
  into or out of squad 3.
- **Formation repack becomes per-squad.** Each squad packs its own members densely; a squad
  whose headcount hasn't changed skips its own repack while a neighbour reforms. Per §1.3
  this is also cheaper in aggregate than today's one retinue-wide sort.
- **Stance is per-squad.** The single global `Stance`/`StanceAnchor` becomes
  `SquadStance[MaxSquads]`/`SquadStanceAnchor[MaxSquads]`. Orders carry an address — "all
  squads" (the default, byte-for-byte today's behavior) or "squad N". Per-unit steering reads
  `SquadStance[soldier.SquadId]` instead of the global.
- **`SquadTargetSize` becomes `ceil(AliveRetinue / MaxSquads)`**, recomputed each repack,
  replacing the fixed-20-with-overflow-into-the-last-squad rule that hard-breaks past
  retinue 160 (§3.1).
- **Per-squad centroids are published** — `SquadCentroidSum[]`/count accumulated on the
  existing `PushRenderEntry` pass, with an accessor the UI can read. No second walk over the
  soldier population.
- **Army View is rewired onto the real data**: blocks at real centroids, tinted by real
  per-squad stance. `Emberkeep.UnitCamProj.ArmyRingRadius` and its "FAKE positioning" comment
  are retired.
- Evidence per `evidence:` above.

## Spawn prompt
```
You are executing task-046 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight
branch. You are building the squad command layer in the Mass sim, then rewiring the Unit
Cam's Army View onto it.

READ FIRST: docs/design/squad-group-system.md — the approved design (task-044, closed done).
Sections 1.1 through 1.4 are your specification. Implement them; do not redesign them. If
something in it is unimplementable as written, say so in your handback rather than silently
substituting your own approach.

THE DEFECT THAT MAKES THIS URGENT (spec section 1.3, verified in the code):
SwarmSubsystem.h has SquadIdForSlot(Slot) = FMath::Clamp(Slot / SquadTargetSize, 0,
MaxSquads - 1), where Slot comes from the dense retinue-wide formation repack, and
NeedsFormationRepack() is AliveRetinue != PackedRetinueCount. So ANY casualty ANYWHERE
renumbers the dense slot index and silently reassigns soldiers between squads. Today that is
cosmetic. The moment squads own stances — which this task builds — a soldier starts obeying
orders that were issued to a different squad. Fixing the stickiness is a PREREQUISITE for
per-squad stance, not an optional cleanup; do it first.

WHAT TO BUILD (spec sections 1.1-1.4, 3.1):
1. Sticky SquadId — assigned once at recruit time (round-robin or fill-lowest-first across
   live squads, your call, say which you picked and why), persisting for that soldier's life
   regardless of repacking elsewhere.
2. Per-squad formation repack — each squad densely packs its OWN members; a squad whose
   headcount has not moved skips its repack while a neighbour reforms. Per the spec this is
   cheaper in aggregate than today's single large sort (<=8 small sorts vs one big one).
3. Per-squad stance — promote the single global Stance/StanceAnchor to
   SquadStance[MaxSquads]/SquadStanceAnchor[MaxSquads]. Orders take an address: "all squads"
   (default — must reproduce today's behavior exactly, this is the regression risk) or
   "squad N". Per-unit steering reads SquadStance[soldier.SquadId].
4. SquadTargetSize = ceil(AliveRetinue / MaxSquads), recomputed each repack. Today's fixed 20
   with overflow folding into the last squad is a hard numeric break past retinue 160 and
   violates Design Law 2 (soft caps only).
5. Publish per-squad centroids — accumulate SquadCentroidSum[]/count on the EXISTING
   PushRenderEntry pass (it already receives Location and SquadId per unit for
   SquadStanding[]). Do NOT add a second O(N) walk. Expose an accessor for the UI.

THEN REWIRE ARMY VIEW (this is the visible payoff):
UnitCamProjector.cpp's Army View currently arranges its <=8 blocks on a FAKE ring around the
bearer (Emberkeep.UnitCamProj.ArmyRingRadius) and tints every block with the single global
GetStance(), because none of the above existed. Now it does. Put blocks at their real
centroids, tint by real per-squad stance, and retire ArmyRingRadius and its "FAKE
positioning" comment. Block size and count already read from GetSquadStanding() and are
correct — leave that alone.

CONSTRAINTS:
- "All squads" addressing must be byte-for-byte today's behavior. This is the main
  regression risk in the task: the whole existing retinue must keep behaving exactly as it
  does now when no squad is explicitly addressed.
- Mass Entity constraints are design law (GDD section 10): no per-unit uniqueness, no
  special-casing at horde scale. A soldier carries ONE small integer (the SquadId it already
  carries) and gains no roster reference, no pointer, no new per-unit bookkeeping.
- Everything new that is tunable gets a CVar with a prose doc-comment, matching the
  TAutoConsoleVariable style already in these files.
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS AND THEN CRASHES THE NEXT PIE. This
  task changes class layout for certain. Use `pwsh Scripts/ue-relaunch.ps1` (close, build,
  relaunch, wait for MCP). Scripts/ue-iterate.ps1 picks the right path automatically.
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are in-memory until save_assets([]).
- If you change Saved/SwarmExecOnPlay.txt to drive a test, back it up and restore it.

KNOWN TOOLING TRAP, budget for it: the PIE window driven over MCP freezes or near-freezes
simulation while it lacks OS focus. task-045 could not capture a moving camera or a 0.6s
hit-flash for exactly this reason. If you hit it, say so plainly and hand back what you could
prove — do not substitute a description for a screenshot you did not take, and do not claim
dynamic behavior you could not observe.

YOU OWN: ELVTR/Source/ELVTR/Mass/** and these four UI files —
UnitCamDirector.cpp/.h, UnitCamProjector.cpp/.h.

DO NOT TOUCH: ELVTR/Source/ELVTR/UI/ViewCamCapture.* , ELVTR/Source/ELVTR/Rendering/**
(task-041), ELVTR/Content/**, docs/design/squad-group-system.md (task-044 wrote it — if it
is wrong, say so, do not edit it), docs/design/CAMERA-SCALE*.md (task-030), GDD.md,
CLASSES.md, SYSTEMS.md, or any docs/backlog/ file.

CANON WARNINGS:
- WORLD.md is superseded by the 2026-07-22 reset; current canon is
  docs/narrative/FLAME-FOUNDATION.md.
- docs/perf/niagara-sprite-refactor.md sections 2 and 8.1 carry a RETRACTED claim that the
  swarm emitter draws zero particles — the cause was GPUComputeSim vs CPUSim and it is fixed.
- docs/UNIT-CAM-HANDOFF.md describes an older capture-based unit cam; the projector is the
  current approach.

EVIDENCE — on-screen proof, not a diff plus "it works":
1. A PIE screenshot where Army View blocks sit at their squads' REAL world positions (move
   one squad, show its block move with it).
2. Two squads holding different stances, tinting differently in the panel.
3. A demonstration that a casualty in one squad moves NO soldier between squads — the
   defect this task exists to fix. A logged before/after of per-soldier SquadId across a
   repack is acceptable here if a screenshot cannot show it.

HAND BACK: what you changed, the recruit-time assignment strategy you chose and why, the
CVars you added with defaults, your three pieces of evidence, confirmation that "all squads"
addressing is unchanged from today, and anything in the spec that did not survive contact
with the code.
```
