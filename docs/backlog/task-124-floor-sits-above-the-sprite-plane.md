---
id: 124
title: Put the floor back under the sprites — the ground plane sits above where the units draw
status: proposed
agent: claude
model: ""
owns:
  - "ELVTR/Content/Spike1/L_Spike1.umap"
  - "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp"
  - "ELVTR/Config/SwarmExecOnPlay.canonical.txt"
  - "docs/perf/evidence/black-frame/**"
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
epic: ""
evidence: A PIE capture at wave-1 density, same framing as the owner's 2026-07-31 screenshot, where units read as lit sprites standing ON the floor rather than black cut-outs punched into it — plus the measure.py region table before and after, so the change is a number and not just a look.
score: {feel: 2, risk: 1, cost: 1}
source: owner, 2026-07-31 ("it was because the floor was above the meshes")
teammate: ""
decided: ""
---

## Why now

Owner, 2026-07-31, after looking at it directly: **"it was because the floor was above the
meshes."**

This replaces task-122, which was scoped to measure the flame/lift model and tune it. That
task assumed the units were being *lit* wrong. They are being *occluded* — the ground plane
sits above the plane the sprites draw on, so what reads as "black paper cut-outs on a bright
floor" is geometry, not the demichrome pass and not `Swarm.UnitLightFloor`. task-122 is
parked; its measurement harness survives and this task reuses it.

The likely cause is recent and specific. Three changes moved the sprite plane and nothing
moved the floor to match:

- `f6b9261` — anchor sprites at their own feet, delete the offset that faked it
- `3ce84c4` — flip the sprite pivot to the edge that actually grounds the feet
- `task-110` — deleted `Swarm.SpriteGroundOffset` / `GroundScale` outright, on the grounds
  that NS_Swarm's `PivotInUVSpace (0.5, 0.0)` makes `Particles.Position` the foot by
  construction, so no per-particle Z compensation is needed

That reasoning is right about the *sprite*. It says nothing about where the floor is. If the
ground mesh's Z was chosen back when the sprite was centre-anchored and carried a
`SpriteGroundOffset` lift, then deleting the lift lowered the sprites and left the floor
where it was.

## Done when

- The floor's Z and the sprite plane's Z are both stated as measured numbers, with the
  offset between them, before and after.
- The fix is in the smallest place that holds for every unit — not a per-unit nudge, and
  not a new offset CVar re-introducing what task-110 deliberately deleted, unless the
  measurement proves geometry alone cannot reach it.
- A PIE capture at the owner's framing shows units standing on the floor and reading as
  lit sprites.
- `measure.py`'s region table is run before and after, so "the units are no longer black"
  is a number.
- If the fix does NOT also close the black upper third of the frame, say so explicitly.
  Those are two different problems and only one of them is this task.

## Spawn prompt

```
You are executing task-124. Read docs/backlog/task-124-floor-sits-above-the-sprite-plane.md
first, then docs/backlog/task-122-say-why-the-frame-goes-black.md (parked — it is the
context, not your instructions).

THE FINDING IS THE OWNER'S, not a hypothesis to re-open. Owner, 2026-07-31:
"it was because the floor was above the meshes." The units in the shipped frame read as
pure black cut-outs on a bright dithered floor because the ground plane sits above the
plane the sprites draw on. This is OCCLUSION, not lighting. Do not spend time re-deriving
whether the demichrome lift or Swarm.UnitLightFloor is at fault — task-122 went down that
road and it was the wrong road.

Your job: measure the offset, fix it in the smallest place that holds, prove it.

START HERE, before opening the editor:
  docs/perf/evidence/black-frame/measure.py and capture.py already exist. A previous
  teammate built them and was interrupted before producing a table. measure.py breaks a
  frame into regions (upper / mid floor / near floor / retinue / brood) and finds sprites
  by CHROMA — the dithered floor is exactly neutral, so any pixel with chroma is sprite
  art. Read them and reuse them. Do not write a second analyser.
  NOTE: capture.py's SceneCapture2D route was reported as returning mean 2/255 with no
  flame pool visible, i.e. it may not see the post-process at all. The owner's own
  screenshot is ground truth and is on disk:
    C:\Users\Hawaiian_spawn\Pictures\Screenshots\Screenshot 2026-07-31 020921.png
  Use that for the "before" table. It is an editor viewport grab, so crop the HUD and
  viewport chrome and say what you cropped.

THE GEOMETRY:
  Find the ground plane actor in L_Spike1 and read its Z and bounds. Find where the swarm
  sprites actually draw — NS_Swarm's sprite renderer has PivotInUVSpace (0.5, 0.0), so
  Particles.Position IS the foot of the sprite, and SwarmRenderActor.cpp pushes
  RenderPos[] straight into the Positions array with no Z compensation (task-110 deleted
  Swarm.SpriteGroundOffset/GroundScale for exactly that reason — see the comment at
  SwarmRenderActor.cpp:1736). State both numbers and the delta.
  Check the three commits named in the task file. If one of them lowered the sprites and
  left the floor, say which.

THE FIX — smallest place that holds:
  Ladder: move the floor > move the sprite publish plane > anything else. Do NOT
  reintroduce a per-particle Z offset CVar; task-110 deleted that deliberately because it
  only ever grounded the nominal-size body and off-nominal bodies floated or sank by up to
  ~29uu. If you believe geometry alone cannot fix it, prove it with the measurement before
  proposing anything else.

TWO SYMPTOMS, ONE TASK:
  The owner's screenshot has TWO black problems — black character cut-outs, and a hard
  edge with the top third of the frame at literal black. You are fixing the first. The
  second is measured already and is a separate owner look-call: docs/RENDERING-LIGHTING.md
  4e records that FlameRadius 900 dies 55% down the frame and ~42% of frame height is
  ground the flame cannot reach. Do NOT widen Swarm.FlameRadius to make your capture look
  better. If your fix happens to change the upper frame too, report it; do not chase it.

SHARED TREE — READ THIS TWICE:
  Another Claude session and the owner share this working tree and this Unreal editor.
  L_Spike1.umap IS ALREADY MODIFIED in the tree by someone else, and it is a file you own.
  Before editing it: check `git status`, and if it is still dirty, STOP and hand back
  rather than saving over a peer's unsaved level work. A .umap is binary; there is no
  merge.
  Also currently dirty and NOT yours: M_Swarm.uasset, T_Swarm_2bit.uasset,
  Content/Sprites/**, SwarmRenderActor.h, docs/RENDERING-LIGHTING.md. Do not touch,
  revert, or stage any of them.
  Do not restart or close the editor. Do not `git add -A`.
  ELVTR/Saved/SwarmExecOnPlay.txt: restore it after every run and verify by hash. Note it
  currently differs from ELVTR/Config/SwarmExecOnPlay.canonical.txt in 5 values
  (DitherWorldAnchor, DitherBandWidth, DitherThreshold1/2, PaletteSteps, Blood.HeightOffset)
  — that drift is PRE-EXISTING and is the owner's live tuning. Preserve the live file as
  you found it; do not "fix" it toward canonical.

Adding a UPROPERTY or member to ASwarmRenderActor is a class-layout change: Live Coding
reports success and then crashes the next PIE. Use a CVar or take a full editor-closed
rebuild deliberately.

HAND BACK: the two Z numbers and their delta, what you changed and why that was the
smallest place, the measure.py region table before and after, and a PIE capture at the
owner's framing. If L_Spike1.umap was still dirty and you could not touch it, hand back
with the diagnosis and the exact edit you would make.
```
