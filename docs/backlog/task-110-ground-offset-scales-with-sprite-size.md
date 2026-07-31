---
id: 110
title: Scale the sprite ground offset by each unit's own size so feet land on one floor
status: done
agent: claude
model: sonnet
owns:
  - "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp"
  - "ELVTR/Source/ELVTR/Rendering/BloodSubsystem.cpp"
  - "ELVTR/Config/SwarmExecOnPlay.canonical.txt"
  - "ELVTR/Content/Spike1/NS_Swarm.uasset"
resources: ["unreal-editor"]
depends-on: []
epic: ""
evidence: A PIE screenshot at wave-1 density where the biggest and smallest bodies on screen stand on the same floor plane, plus a second capture at `Swarm.BroodSizeJitter 0.6` (worst case) showing the same, and a `Swarm.BroodSizeJitter 0` capture confirming the nominal look is unchanged from today.
score: {feel: 2, risk: 1, cost: 1}
source: user
teammate: ground-offset
decided: "2026-07-30 done"
---

## Why now
Owner-reported 2026-07-30 with a screenshot: soldiers sink into and float above the
floor at different amounts. `SwarmRenderActor.cpp:1741` gives every particle its own
sprite size — `BaseSize × RetinueSizeScale × SizeScale(bits, jitter)` — but `:1748`
shifts every particle down by the same absolute `Swarm.SpriteGroundOffset` (-72uu).
The Sprite Renderer centres on `Particles.Position`, so the correct shift is half that
particle's *own* height. Live jitter is ±0.15 retinue and ±0.4072 brood, so feet miss
the floor by up to ~29uu in either direction, and no single value of the CVar can fix
it. The `-72` the owner tuned on 2026-07-28 is right for a nominal-size body and wrong
for every other one.

## Revised 2026-07-30 — try the pivot first
Owner asked whether a unit-dependent offset is needed at all. It probably is not.
`NiagaraSpriteRendererProperties.h:196` (UE 5.7) exposes `PivotInUVSpace`, default
(0.5, 0.5) and never touched on this asset. Setting it to (0.5, 1.0) anchors the
sprite's BOTTOM edge to `Particles.Position`, which is correct at every size for free
and would delete the CVar, both call sites and the per-particle maths outright.

Two reasons it might not fully hold, and neither is knowable without looking:
1. Pivot moves the sprite in SPRITE space; the CVar moves it in WORLD Z. On a pitched
   camera those are not the same displacement.
2. If the sheet cells carry whitespace below the feet, pivot 1.0 grounds the CELL, and
   a residual (still size-proportional) float survives.

So: set the pivot, look, and keep the C++ scaling only if a residual survives. The C++
half is already written and sitting uncommitted in the working tree.

## Done when
The Z shift is proportional to the particle size that was just written into
`SizeScratch`, so a unit at 1.41× size drops 1.41× as far. The nominal-size look is
byte-for-byte the shipped one (`-72` at `Swarm.SpriteSize 48` = a factor of `-1.5`),
and `Swarm.SpriteSize` / `Swarm.RetinueSizeScale` now carry the offset with them
instead of leaving it stale.

## Spawn prompt

```
You are fixing a grounding bug in Kindled's Niagara sprite path. Agent type: claude.
You have a shell and you build and PIE this yourself.

THE BUG
ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp, in the per-entity push loop:

  line ~1741  SizeScratch.Add(BaseSize * (bRet ? RetScale : 1.f)
                  * SwarmRenderPack::SizeScale((int32)Bits, bRet ? RetJit : BrdJit));

  line ~1748  PositionScratch.Add(RenderPos[PackIndex] + FVector(0,0, GroundOffset));

Every particle gets its OWN size, but the same absolute -72uu Z shift. NS_Swarm's
Sprite Renderer centres each sprite on Particles.Position, so the shift that grounds
the feet is proportional to that sprite's height. With Swarm.BroodSizeJitter at its
shipped 0.4072 the horde spans 0.59x-1.41x, so bodies land up to ~29uu above or below
the floor. Owner screenshot confirms it.

THE FIX
Hoist the computed size into a local (it is already being built one line above), and
make the Z shift a MULTIPLE of that size rather than an absolute uu value:

  const float Size = BaseSize * (bRet ? RetScale : 1.f)
      * SwarmRenderPack::SizeScale((int32)Bits, bRet ? RetJit : BrdJit);
  SizeScratch.Add(Size);
  ...
  PositionScratch.Add(<render pos> + FVector(0.f, 0.f, GroundScale * Size));

RENAME the CVar Swarm.SpriteGroundOffset -> Swarm.SpriteGroundScale, default -1.5.
The rename is load-bearing, not tidiness: the units change from uu to a fraction of
sprite size, and if the old name survived, a stale `Swarm.SpriteGroundOffset -72` typed
from muscle memory or an old exec file would push units 72*48 = 3456uu underground.
A dead name errors visibly instead. -1.5 x 48 = -72, so a nominal-size unit sits
EXACTLY where the owner tuned it on 2026-07-28 and the shipped look does not move.

Rewrite the CVar's doc comment: it is no longer "uu", it is "how many multiples of the
particle's own sprite size to push down so the centred pivot lands the feet on the
floor". Keep the owner's 2026-07-28 A/B provenance (-24 floated, -100 sank, -72 read
grounded) because that is what -1.5 is derived from.

TWO OTHER CALLERS - both must move with it, do not leave either behind:
1. ELVTR/Source/ELVTR/Rendering/BloodSubsystem.cpp:182-200 looks the CVar up BY NAME
   with FindConsoleVariable and adds it to gore spawn Z. A rename silently makes that
   return nullptr and fall back to 0, which floats every blood decal. Blood has no
   per-entity size to scale by, so use the nominal body: GroundScale * SpriteSize
   (read Swarm.SpriteSize the same way). Say in the comment that gore uses the nominal
   size deliberately rather than the dying unit's roll - it is a splash, not a foot.
2. ELVTR/Config/SwarmExecOnPlay.canonical.txt line 112 sets the old name to -72.
   Update the name and the value and the trailing comment. Then run the /cvars skill
   (or Scripts/ equivalent) so Saved/SwarmExecOnPlay.txt regenerates from canonical -
   do not hand-edit the generated file.

Grep the whole repo for SpriteGroundOffset before you finish. Any hit you have not
touched is a bug you are shipping.

BUILD NOTE
This registers a differently-named TAutoConsoleVariable. Do not trust Live Coding for
it - close the editor, do a full rebuild, relaunch. If a "rebuild ELVTR modules?"
dialog appears on relaunch it is usually a false alarm (check BuildId matches) but it
blocks MCP until dismissed.

EVIDENCE - three PIE captures at wave-1 density, all from the same camera:
  a) Swarm.BroodSizeJitter 0     - proves the nominal look is unchanged from today
  b) shipped jitter (0.4072)     - biggest and smallest bodies share one floor plane
  c) Swarm.BroodSizeJitter 0.6   - worst case, still grounded
Point the camera somewhere the floor plane is legible and say in your handback which
capture is which. The owner reported this from a screenshot; screenshots are what
closes it.

DO NOT TOUCH
- Any .uasset, including NS_Swarm and M_Swarm. This is a C++ and config change only.
- ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp - the Unit Cam never applied a ground
  offset and is disabled in the shipped one-camera build. Leave it.
- Swarm.SpriteSize, Swarm.RetinueSizeScale, either jitter CVar, or any lighting CVar.
  Do not retune anything to compensate; the whole point is that the shipped nominal
  look is preserved and only the off-nominal bodies move.
- GDD.md, SYSTEMS.md, docs/art/**.

Another session may be working in this tree. Several Mass/ files and
Rendering/SwarmRenderActor.h are already dirty on arrival - that is not yours, do not
revert it, do not commit it. Commit only the three files this task owns.

Hand back: the three captures, the diff summary, and the grep result proving no
SpriteGroundOffset reference survives.
```
