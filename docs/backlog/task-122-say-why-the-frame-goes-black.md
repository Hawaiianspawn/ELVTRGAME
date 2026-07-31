---
id: 122
title: Measure why the frame reads black and fix the value hierarchy it inverted
status: in-progress
agent: claude
model: opus
owns:
  - "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp"
  - "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h"
  - "ELVTR/Config/SwarmExecOnPlay.canonical.txt"
  - "ELVTR/Content/PostProcess/M_PP_Demichrome.uasset"
  - "docs/RENDERING-LIGHTING.md"
  - "docs/perf/evidence/black-frame/**"
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
epic: ""
evidence: A measured luminance table for the shipped framing — upper frame, mid floor, near floor, retinue body, brood body — with the current numbers beside the proposed ones, plus before/after PIE captures at identical framing showing units reading ABOVE the floor they stand on and the upper frame no longer at literal 0/255.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: black-frame
decided: "2026-07-31 in-progress"
---

## Why now

Owner screenshot, 2026-07-31: the shipped frame is a bright dithered floor with **pure
black paper cut-outs** standing on it, under a **hard horizontal edge above which the
frame is literal black** for the top third.

Both halves have a documented cause and neither has been measured against the current
`Kindled.Quantize 0` look:

1. **The two teams are lit by opposite models.** The floor takes the full-screen additive
   lift from `M_PP_Demichrome`'s flame pool. Units are stencil-*exempt* from that lift
   (`Swarm.UnitStencil 1`, owner call 2026-07-28) and instead get a per-particle term that
   **can only darken** — `Lit = Lerp(floor, ceil, Atten)` multiplying already-dark art
   (`SwarmRenderActor.cpp:1660-1720`). Live values are `Swarm.UnitLightFloor 0.28` and
   `Swarm.BroodLightFloor 0` / `BroodAdd 0.05`. So the floor climbs toward white while every
   character multiplies toward black: the value hierarchy the art was authored for is
   inverted, and the brightest thing in frame is the ground.
2. **The upper frame is absolutely black.** `Swarm.FlameRadius 900` with
   `Swarm.FlameFalloff 2` ends the pool well inside the camera's depth, and
   `RENDERING-LIGHTING.md:254` states the rule this breaks — *"Never pure black at the outer
   edge. `Palette[0]` is heavy midnight (`#211e20`), not black. The world outside the light
   is dark, not absent."* That floor came from the 4-value quantizer, which the game no
   longer runs (`Kindled.Quantize 0`), so nothing enforces it now. §4e already measured the
   consequence for the brood: they cross **64-78% of their approach in absolute blackness**
   (`RENDERING-LIGHTING.md:823`).

Nothing is broken in the "it crashed" sense — every dial is doing what it was told. What is
missing is a measurement saying which of them is responsible for how much of the black, so
the fix is a number and not another look call taken blind.

## Done when

- `docs/perf/evidence/black-frame/` holds a luminance measurement of the shipped framing
  broken out by region — upper frame, mid floor, near floor, retinue body pixels, brood body
  pixels — for the **current** settings, stated as mean and p99 per region.
- The same table exists for the proposed settings, and it shows: no region at literal 0/255,
  a **retinue body brighter than the floor it stands on**, and brood still a step below the
  retinue (the deliberate gap `Swarm.BroodLightCeil` exists to hold).
- Before/after PIE captures at identical camera framing and identical wave state.
- `docs/RENDERING-LIGHTING.md` records the numbers and retires or restates the
  `Palette[0]` heavy-midnight rule for `Kindled.Quantize 0` — it is currently canon that the
  build silently violates.
- Whatever changed is in `SwarmExecOnPlay.canonical.txt` **and** the working
  `Saved/SwarmExecOnPlay.txt`, diffable against each other.

## Spawn prompt

```
You are executing task-122. Read docs/backlog/task-122-say-why-the-frame-goes-black.md
first, then docs/RENDERING-LIGHTING.md sections 4b, 4d and 4e in full before touching
anything. This is a MEASUREMENT task with a tuning pass attached, not a rewrite.

The owner's report: the game frame is a bright dithered floor with pure black character
cut-outs on it, under a hard horizontal edge above which the top third of the frame is
literal black.

What you own (write nothing else):
  ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp
  ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h
  ELVTR/Config/SwarmExecOnPlay.canonical.txt
  ELVTR/Content/PostProcess/M_PP_Demichrome.uasset
  docs/RENDERING-LIGHTING.md
  docs/perf/evidence/black-frame/**

DO NOT TOUCH, under any circumstances:
  ELVTR/Content/Spike1/M_Swarm.uasset, T_Swarm_2bit.uasset, NS_Swarm.uasset
  ELVTR/Content/Sprites/**   (T_Soldier_0*, T_Hero_Vanguard, T_Unit_Retinue)
These are ALREADY MODIFIED in the shared working tree by someone else. Another Claude
session and the owner share this tree and this editor. If a change of yours seems to
need one of them, STOP and hand back saying so. Do not revert them, do not stage them,
do not restart the editor without asking.

Phase 1 - MEASURE FIRST. Change nothing until the current frame has numbers.
  Reproduce the owner's framing in PIE on L_Spike1 at wave 1, capture, and measure mean
  and p99 luminance per region: upper frame (the black band), mid floor, near floor,
  retinue body pixels, brood body pixels. Scripts/ already has image tooling used for the
  RENDERING-LIGHTING measurements - reuse it rather than writing a new analyser.
  Then attribute the black. Run the A/Bs the CVars already expose and measure each:
    Swarm.Flame 0                (pool off - what the level's own material gives)
    Swarm.UnitStencil 0          (units back inside the post lift - the old washed look)
    Swarm.FlameRadius up         (does the black band retreat, and at what cost near the hero)
    Swarm.FlameFalloff 1         (linear - a softer pool edge)
    Swarm.UnitLightFloor up      (retinue floor - the multiply's minimum)
    Swarm.BroodAdd up            (the additive brood lift; see 4e on why additive not multiply)
  Report which dial owns which share of the black. A hypothesis without a number is not
  an answer here.

Phase 2 - FIX, preferring dials over code.
  Ladder: an existing CVar value > a new CVar > a code change > a material change. Only
  add a dial if the measurement proves no existing one reaches the problem, and say which
  one you tried first. The target read:
    - no region of the frame sits at literal 0/255
    - retinue bodies measure BRIGHTER than the floor they stand on (today it is inverted)
    - brood stay a step below retinue - that gap is deliberate, see Swarm.BroodLightCeil
    - the pure-white core still reads as the focusing point near the hero, not a blowout
  Two known traps, both already paid for once, both documented in 4e:
    - a MULTIPLIER on near-black brood art expands its range and blows the teeth out. The
      additive term is the one that works. Do not re-litigate this, it was measured.
    - Swarm.RawNear and the flame Atten run along the SAME axis. Multiplying by both makes
      a band where a brood lights up mid-approach and goes out again. One ramp only.
  RENDERING-LIGHTING.md:254 says the world outside the light must be heavy midnight
  (#211e20), never pure black. That rule came from the 4-value quantizer, which is off
  (Kindled.Quantize 0). Either restore a floor that honours it or write down that it is
  retired - do not leave canon stating a rule the build breaks.

Constraints:
  - Adding a UPROPERTY or any member to ASwarmRenderActor is a class-layout change: Live
    Coding reports success and then crashes the next PIE. Use a CVar, or take a full
    editor-closed rebuild deliberately.
  - Kindled.Quantize is 0 and the game ships in full colour. Any reasoning that depends on
    binning into a 4-value ramp is stale - several comments in SwarmRenderActor.cpp say so
    about themselves.
  - Keep ELVTR/Saved/SwarmExecOnPlay.txt and ELVTR/Config/SwarmExecOnPlay.canonical.txt in
    sync and diff them at the end. Saved/ is gitignored, so "git diff is clean" proves
    nothing about it.
  - Do not change Kindled.Cam.* framing to make a capture look better. Same framing before
    and after or the comparison is worthless.

Hand back: the two measurement tables side by side, the before/after captures, the list of
dials you moved with the value and the reason for each, and anything you found that the
measurement contradicts in RENDERING-LIGHTING.md. If the honest conclusion is that some of
this is a look call rather than a fix, say which part and give the owner the measured
options instead of picking one.
```
