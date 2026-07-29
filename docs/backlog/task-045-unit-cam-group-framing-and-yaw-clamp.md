---
id: 045
title: Frame the majority of the retinue in both centre-column panels and clamp the Unit Cam yaw
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/UI/UnitCamDirector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamDirector.h", "ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamProjector.h", "ELVTR/Source/ELVTR/UI/ViewCamCapture.cpp", "ELVTR/Source/ELVTR/UI/ViewCamCapture.h"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [44]
evidence: A PIE screenshot of the HUD command rectangle at a large retinue count showing both centre-column panels hitting task-044's framing fraction, plus a second screenshot after a hard heading change proving the yaw stays inside the clamped envelope.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: unit-cam-framing
decided: "2026-07-27 done"
---

## Why now
The owner's complaint is direct: the Unit Cam does not show enough of the army, and it
swings left and right too much. Both are live in the current build.

The yaw half is diagnosed. `FUnitCamDirector::Tick` (`UnitCamDirector.cpp:176-203`) drives
`Shot.YawDeg` off a smoothed auto-look that, at the default `AutoLook = 2`, aims at the
centre of mass of whatever brood sits within `CombatScan` (1200uu) of the focus. `LookLerp`
eases it, but nothing bounds it — as the fight shifts around the bearer, the camera follows
all the way around. There is damping but no clamp.

The framing half is a design question first, which is why this depends on task-044.

## Done when
- Both centre-column panels — `ViewFeed` and the `UnitCam` projector
  (`EmberkeepHud.cpp:376-405`) — hit the framing fraction task-044 specifies, at the retinue
  sizes task-044 names.
- Unit Cam yaw is clamped to task-044's envelope off the bearer's heading, and dampened at
  the boundary rather than hitting a hard stop. Existing `Emberkeep.UnitCamProj.AutoLook`
  behavior is preserved *inside* the envelope, not deleted.
- The clamp and the framing target are exposed as `Emberkeep.UnitCamProj.*` CVars with the
  spec'd values as defaults, so they stay tunable in a play session without a rebuild.
- Evidence per `evidence:` above — screenshots from PIE, not a description of a diff.
- No regression in the muster wings: `SyncWingsToCam()` sizes them from
  `GetPanelSizePx()`, so any change to panel sizing must keep the rectangle reading as one
  object.
- **Absorbed from task-040** (parked into this task — same file, would have collided):
  `UnitCamProjector.cpp:857` sets `B.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha)`. The
  panel is UMG, drawn *after* post, so nothing quantizes it and it renders as literal white
  — a fifth value outside the locked ramp. Replace with
  `FLinearColor::FromSRGBColor(FColor(0xE9, 0xEF, 0xEC))` (Demichrome Pale).

## Spawn prompt
```
You are working on the Unit Cam in Emberkeep (C:\Projects\ELVTRGAME), on the
flame-spotlight branch.

TWO PROBLEMS, both from the owner, both live in the current build:

1. The Unit Cam does not show enough of the army. The owner wants "the majority of the
   soldier units" visible in BOTH panels of the HUD command rectangle's centre column.
2. The camera swings left and right too much. The owner wants that rotation eliminated, or
   "limited significantly and dampened".

READ FIRST: docs/design/squad-group-system.md, written by the gameplay-director in task-044.
It carries the numeric framing target (what fraction of standing retinue must be in frame in
each panel, plus a body floor, plus what to sacrifice when everything will not fit) and the
yaw envelope in degrees. Those numbers are the spec. Implement them; do not re-derive them.
If the spec is ambiguous where you need precision, say so in your handback rather than
guessing silently.

WHERE THINGS LIVE:
- ELVTR/Source/ELVTR/UI/EmberkeepHud.cpp:376-405 — the centre column is TWO stacked panels.
  ViewFeed (AViewCamCapture — a real SceneCapture mirroring the player camera, or a
  pulled-back minimap mode) on top; the UnitCam projector below. These are the "both screens"
  the owner means. They are different animals: one is a real render, the other is projection
  math over the sim buffers, so they need different framing mechanisms to hit the same target.
  Owner's design intent, treat it as a given: putting the retinue in these panels rather than
  the main viewport is deliberate — it offloads the "where is my army" read onto the command
  rectangle and frees the main view for the primary visual systems. So a framing that looks
  good but shows only a handful of soldiers fails the brief. Breadth of the group beats a
  flattering close-up.
- ELVTR/Source/ELVTR/UI/UnitCamDirector.h/.cpp — the camera manager. Resolves one FUnitCamShot
  per frame (Focus, YawDeg, DistScale). THE YAW BUG IS HERE: lines 176-203. At the default
  Emberkeep.UnitCamProj.AutoLook = 2 the heading chases the centre of mass of brood within
  CombatScan (1200uu) of the focus, eased by LookLerp. Damping exists; a clamp does not.
  This file is also the right home for "fit the group" logic — its own header says the
  director owns direction and the projector owns projection math and paint.
- ELVTR/Source/ELVTR/UI/UnitCamProjector.h/.cpp — per-unit projection, 1/depth billboard
  scaling, and the panel sizing that scales with body count via
  Emberkeep.UnitCamProj.Size{Bodies,RetinueWeight,BroodWeight,Curve}. Existing lens dials:
  Fov, Dist, Height, Pitch, Range, Scale.
- ELVTR/Source/ELVTR/UI/ViewCamCapture.h/.cpp — the mirrored-view capture. Note it is
  rate-limited (Emberkeep.UI.ViewCam.Rate) and rendered at panel resolution on purpose; it
  is a second full scene render and that cost is the reason. Do not make it per-frame.
- ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h — MaxSquads = 8, SquadTargetSize = 20,
  SquadStanding[] live per-squad standing counts, refilled each frame by PushRenderEntry().

CONSTRAINTS:
- Expose the clamp and the framing target as Emberkeep.UnitCamProj.* CVars with the spec'd
  values as defaults, so they stay tunable live. Follow the existing TAutoConsoleVariable
  pattern and the doc-comment style already in those files — every dial there explains what
  it does in prose.
- Preserve AutoLook behavior INSIDE the envelope. The goal is to bound the swing, not to
  remove the camera's ability to look toward the fight.
- Dampen at the boundary. A hard stop at the clamp edge will read worse than the swing does.
- SyncWingsToCam() sizes the muster wings from UUnitCamProjector::GetPanelSizePx(). If you
  change panel sizing, the whole command rectangle must still read as ONE object.
- ADDING A UPROPERTY VIA LIVE CODING CRASHES THE NEXT PIE. Class-layout changes need a full
  editor-closed rebuild: run `pwsh Scripts/ue-relaunch.ps1` (close → build → relaunch →
  wait for MCP). Scripts/ue-iterate.ps1 decides live-coding vs. relaunch automatically.
- unreal-mcp is on PORT 9000, not 8000.
- When editing assets over MCP, changes are in-memory until you call save_assets([]).

DECISION ALREADY MADE, do not reopen: Army View renders as <=8 per-squad AGGREGATE BLOCKS
(centroid position, size proportional to standing/target, tint by stance, live count label)
— NOT individual soldier billboards pulled back to fit. The spec's section 6.2 has the
geometry: LeashRadius ~2000uu bounds worst-case squad spread to a ~4000uu diameter
regardless of headcount, covering that at Fov 40 needs the camera back at ~5500uu, and a
sprite there draws 6-12px — illegible at any retinue size. Implementing Army View as a
pulled-back per-soldier loop is the named failure mode: it stays O(N) AND looks wrong.

ALSO IN SCOPE — absorbed from task-040, which was parked into this task because it edits the
same file. UnitCamProjector.cpp:857 currently sets
  B.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha)
The panel is UMG, drawn AFTER post-processing (docs/RENDERING-LIGHTING.md section 4d), so
nothing quantizes it and it renders as literal pure white — a genuine fifth value outside
the locked 4-value ramp. Replace with:
  FLinearColor::FromSRGBColor(FColor(0xE9, 0xEF, 0xEC))
Read docs/art/palette-exceptions.md for the full adjudication. Do NOT touch
SwarmRenderActor.cpp:388 — that white is quantized to Pale by M_PP_Demichrome and was ruled
functionally correct. Several files hand-roll #e9efec; propose a shared DemichromePale
constant in your handback but do not refactor every call site here.

EVIDENCE — this is the bar, and a diff plus "it works" does not clear it. Hand back:
1. A PIE screenshot of the HUD command rectangle at a LARGE retinue count, showing both
   centre-column panels hitting the framing fraction. Swarm.DebugShotAfter N writes a real
   game-viewport screenshot to Saved/Screenshots/.
2. A second screenshot after a hard heading change, proving yaw stayed inside the envelope.
State the measured fraction you actually achieved in each panel, not the one you aimed for.

DO NOT TOUCH: docs/design/squad-group-system.md (task-044 wrote it — if it is wrong, say so,
do not edit it), docs/design/CAMERA-SCALE.md and CAMERA-SCALE-HANDOFF.md (task-030),
ELVTR/Content/PostProcess/** (tasks 041 and 043), GDD.md, CLASSES.md, SYSTEMS.md, or any
docs/backlog/ file.

CANON WARNINGS:
- docs/UNIT-CAM-HANDOFF.md describes an OLDER capture-based unit cam (AUnitPortraitStage).
  The projector is the current approach. Context only — do not restore that path.
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 carry a RETRACTED claim that the swarm
  emitter draws zero particles. The cause was GPUComputeSim vs CPUSim and it is fixed.
- WORLD.md is superseded by the 2026-07-22 reset; current canon is
  docs/narrative/FLAME-FOUNDATION.md.

HAND BACK: what you changed, the CVars you added with their defaults, the measured framing
fraction per panel, the two screenshots, and anything in task-044's spec that turned out to
be unimplementable as written.
```
