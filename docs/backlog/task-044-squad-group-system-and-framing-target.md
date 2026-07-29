---
id: 044
title: Spec the squad group system and the Unit Cam group-framing target
status: done
agent: gameplay-director
owns: ["docs/design/squad-group-system.md", "docs/data/squads.json", "docs/data/squads.schema.md", "ELVTR/Content/SwarmControls/**"]
resources: ["unreal-editor"]
depends-on: []
evidence: docs/design/squad-group-system.md — squad-as-entity architecture, the squad manager's command surface, and a numeric "majority of soldiers in frame" target the implementer can code against, plus a Performance requests section.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: squad-group-system
decided: "2026-07-27 done"
---

## Why now
The retinue is currently a flat bag of soldiers. `USwarmSubsystem` carries `MaxSquads = 8`
and `SquadTargetSize = 20`, but squads are *cosmetic* — `SquadStanding[]` is a per-frame
headcount refilled for the muster cards, not a command unit. The owner is about to spin up
a much bigger retinue stage, and at that scale a flat bag means micromanagement the design
cannot carry.

The camera work in task-045 needs a number to hit: "most of the soldiers visible" is not
codeable as stated. What a group *should* read as is a design decision, so it lands here,
ahead of the implementation.

## Done when
- `docs/design/squad-group-system.md` exists and specs:
  - **Squad as its own entity.** The squad is a distinct entity that *contains* simulated
    soldiers — each soldier still gets a particle — managed by a squad manager. State what
    the squad entity owns (cohesion, formation, stance, target) versus what stays
    per-soldier, and how orders flow manager → soldiers.
  - **The micromanagement answer.** What the player commands at squad level that they used
    to command per-unit, and what the player can no longer do.
  - **Scaling behavior.** What happens to squad count and squad size as the retinue grows
    to the bigger stage — more squads, bigger squads, or both — and where the soft caps sit
    (Design law 2: soft caps only, never hard numeric caps).
  - **The framing target.** A number task-045 can code against: what fraction of standing
    retinue must sit inside the frame, for *each* of the two centre-column panels. State it
    as a fraction plus a body floor (e.g. "≥X% of standing retinue, minimum N bodies"), and
    say what the camera sacrifices first when it cannot fit everyone — group cohesion or
    proximity to the bearer.
  - **Yaw discipline.** The owner's constraint: the Unit Cam must not swing left/right, or
    must be clamped hard and dampened. Specify the allowed yaw envelope in degrees off the
    bearer's heading, and the settle behavior. Design intent, not code.
- A `## Performance requests` section addressed to `performance-director`: entity-count
  shape after the change, what the squad manager costs per frame, and what the projection
  loop now has to walk. The owner explicitly asked that the optimization agent be made
  aware of this architecture change.
- `## Simulation notes` per the agent's own rule — or "Not simulated" with the reason.
- Anything implying a `GDD.md` / `CLASSES.md` / `SYSTEMS.md` change is a
  `## Canon proposals` entry, not an edit.

## Spawn prompt
```
You are the gameplay-director working in Emberkeep (C:\Projects\ELVTRGAME), on the
flame-spotlight branch.

GOAL, from the owner, in their words: "I am trying to transform this retinue into a group
system." The architecture direction is theirs and is NOT up for redesign:

  The squad will be an entirely different entity, that just so happens to CONTAIN
  simulated soldiers — each with a particle — but managed by the squad manager. This
  allows us to scale the units while minimizing micromanagement of units.

The owner is also about to spin up a much bigger retinue stage for game design work. Treat
that as a constraint on everything you spec: assume the retinue gets substantially larger
than it is today, and say where your numbers break.

FILES YOU MAY WRITE, and nothing else:
- docs/design/squad-group-system.md   (the spec — the main deliverable)
- docs/data/squads.json + docs/data/squads.schema.md   (only if you publish tuned numbers)
- ELVTR/Content/SwarmControls/**   (CVP_UnitCam / CVP_Combat console-variable presets)

On that last one, know what you are and are not able to do. Those are BINARY .uasset files
and you have no unreal-mcp tools. The only route is editor Python against a running editor
— see Scripts/populate_cvar_preset.py and the /cvars skill for the working pattern. You
hold the unreal-editor lock, so the editor is yours. If that route does not work cleanly,
do NOT fight it: write the intended preset values into your spec as a table and say the
preset was not updated. A spec with the right numbers beats a half-written binary asset.
You cannot author maps — the bigger retinue stage is not yours to build.

WHAT THE CODE ALREADY DOES (do not re-derive; verify only if you doubt it):
- ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h — MaxSquads = 8 (hard cap; slots beyond fold
  into the last squad), SquadTargetSize = 20 formation slots per squad, SquadIdForSlot()
  assigns contiguous chunks. SquadStanding[] is a live per-squad standing count refilled
  every frame by PushRenderEntry(). Squads today are COSMETIC — a grouping for the muster
  cards, not a command unit. That is the thing you are being asked to change.
- ELVTR/Source/ELVTR/UI/UnitCamDirector.h/.cpp — the Unit Cam's camera manager. Resolves one
  FUnitCamShot per frame: Focus (world point), YawDeg (azimuth), DistScale. Focus modes are
  Hero (0) and FollowSoldier (1) via Emberkeep.UnitCamProj.Focus. Yaw comes from a smoothed
  auto-look (Emberkeep.UnitCamProj.AutoLook, default 2 = outward from the hero, biased
  toward the nearest brood cluster within CombatScan 1200uu), eased by LookLerp. There is
  damping but NO CLAMP — that is the gap the owner is complaining about.
- ELVTR/Source/ELVTR/UI/UnitCamProjector.h/.cpp — projects each unit, draws it as a billboard
  scaled by 1/depth. Panel size scales with body count via
  Emberkeep.UnitCamProj.Size{Bodies,RetinueWeight,BroodWeight,Curve}.
- ELVTR/Source/ELVTR/UI/EmberkeepHud.cpp:376-405 — the HUD command rectangle's centre column
  is TWO stacked panels: ViewFeed (AViewCamCapture, a real SceneCapture mirroring the player
  camera, or a pulled-back minimap mode) on top, and the UnitCam projector below. "Both
  screens" in the owner's request means THESE TWO PANELS. Note they are different animals:
  one is a real render of the world, the other is projection math over the sim buffers. Your
  framing target has to be stateable for both.

WHY THE PANELS CARRY THE RETINUE — owner's design intent, treat it as a given:
Showing the retinue in the side/bottom command rectangle rather than in the main viewport is
deliberate. It offloads the "where is my army" read onto the panels, which frees the main
view to focus on the primary visual systems (the flame, the dark, the tide). Your framing
target should serve that division of labour: the panels are where the player reads their
GROUP, so a framing that shows a heroic close-up of three soldiers fails the brief even if
it looks better in isolation.

WHAT TO PRODUCE — full bar in docs/backlog/task-044-squad-group-system-and-framing-target.md
"Done when". Summarised: squad-as-entity architecture and the manager's command surface;
what the player commands at squad level instead of per-unit; how squad count/size scale as
the retinue grows; a NUMERIC framing target (what fraction of standing retinue must be in
frame in each panel, with a body floor, and what the camera sacrifices first when it cannot
fit everyone); and the yaw envelope in degrees plus settle behavior.

Your spec is consumed by task-045, which implements the framing and the yaw clamp in C++.
Write the framing target so a programmer can turn it into code without a follow-up question.
"Most of them" is not a spec; a fraction and a floor is.

END WITH a "## Performance requests" section addressed to performance-director: entity-count
shape after the change, what the squad manager costs per frame, and what the projection loop
now walks. The owner explicitly asked that the optimization agent be made aware of this.

CANON WARNINGS:
- WORLD.md is SUPERSEDED by the 2026-07-22 narrative reset. Current canon is
  docs/narrative/FLAME-FOUNDATION.md. Your own agent definition still lists WORLD.md as a
  source of truth — that line is stale (task-018 exists to fix it). Do not build on WORLD.md.
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 carry a RETRACTED claim that the swarm
  emitter draws zero particles. The real cause was GPUComputeSim vs CPUSim, and it is fixed.
  Ignore those two sections.
- docs/UNIT-CAM-HANDOFF.md describes an OLDER capture-based unit cam (AUnitPortraitStage).
  The projector is the current approach; read that handoff for context only.

DO NOT TOUCH: ELVTR/Source/**, ELVTR/Content/PostProcess/** (tasks 041 and 043 own it),
ELVTR/Content/Spike1/**, GDD.md, CLASSES.md, SYSTEMS.md, docs/design/CAMERA-SCALE.md,
docs/design/CAMERA-SCALE-HANDOFF.md (task-030 owns it), or any docs/backlog/ file. You
design; the main session implements. Anything implying a canon change goes in a
"## Canon proposals" section for the owner to decide.

HAND BACK: the squad architecture you specced, the framing-target numbers and why, the yaw
envelope, your performance requests, and whether the CVP preset update actually landed. Name
any assumption you had to make that the owner should confirm.
```
