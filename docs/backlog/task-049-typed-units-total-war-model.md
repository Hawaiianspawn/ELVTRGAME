---
id: 049
title: Rework the squad system as typed Total War-style units, and settle the Unit Cam default view
status: done
agent: gameplay-director
owns: ["docs/design/squad-group-system.md", "docs/data/squads.json", "docs/data/squads.schema.md", "docs/data/unit-types.json", "docs/data/unit-types.schema.md"]
resources: []
depends-on: []
evidence: A reworked docs/design/squad-group-system.md built on typed units, with a recommendation on ranged combat scope, a settled answer on the Unit Cam default view, and a spec task-046 can be rewritten against without further design questions.
score: {gate: 3, risk: 2, cost: 3}
source: user
teammate: typed-units-model
decided: "2026-07-27 done"
---

## Why now
The owner named the model they actually want: **Total War units** — *"One group of small
spearmen and other a group of archers."* The current spec does not describe that, and
task-046 would have compiled the mismatch into the sim.

The specific break: §3.1 defines `SquadTargetSize = ceil(AliveRetinue / MaxSquads)` — squads
absorb growth *evenly*. Under a typed model that is simply wrong. A spearmen unit does not
gain archers when you recruit. Squads-as-even-divisions is a display grouping wearing a
command system's clothes.

Two more decisions have piled onto the same document since it was written, so they get
answered together rather than patched in three passes.

**Verified against the code before filing:** there are **no unit types in the sim at all** —
the only "archetype" is the Mass technical composition. And there is **no ranged combat**:
`Swarm.RetinueDPS` with contact range is the entire model. But `CLASSES.md:21` already makes
Pathfinder the **ranged** class and line 44 lists ranged retinue identities (Huntmaster,
Trailwarden, Falconer, Outrider). Typed units are canon; they are just unbuilt.

## Done when
- **Units are typed.** The unit — not the squad-as-slot-range — is the command object, and
  it has a type that determines how it fights, how it forms up, and what it is for. Spearmen
  and archers are the two the owner named; say whether v1 is exactly those two or a small
  set, and why.
- **Membership, growth and loss are per-unit.** Replace the even-division formula. State how
  a unit is recruited, how it takes casualties, what happens when it is wiped, and whether
  reinforcement refills existing units or adds new ones.
- **Formation is per-type.** A spear block and an archer line are not the same shape and do
  not stand in the same place. Give each type its formation defaults in the existing
  `SwarmFormation` vocabulary.
- **Ranged combat is scoped, not hand-waved.** Archers need it and it does not exist. Spec
  what the minimum viable version is (range bands, volleys vs. projectiles, line-of-fire
  through your own front rank), estimate its size honestly, and **recommend** whether it
  lands in v1 or whether v1 ships spear-only with archers stubbed. The owner decides; give
  them a real recommendation, not a menu.
- **The Unit Cam default view is settled** — §10.5, reopened. The owner has since indicated
  they want the panel to show **mini retinue units by default**, not the ≤8 abstract blocks
  the spec recommended and task-045 built. Reconcile that against §6.2's geometry (units
  render at 6-12px when the camera pulls back far enough to cover the leash radius, at any
  headcount). If "always show mini units" and "show the whole army" are incompatible at one
  zoom, say so plainly and propose which gives way.
- **The wider "map" mode is specced.** The owner wants a zoomed-out mode that exposes more
  options and information than the tight view. What is on it, what it is for, and how it
  relates to Army View as already specced.
- The result is implementable: task-046 gets rewritten against it and must not need to come
  back with design questions.

## Spawn prompt
```
You are the gameplay-director working in Emberkeep (C:\Projects\ELVTRGAME), on the
flame-spotlight branch. You are reworking a design you did not write but which is yours to
own: docs/design/squad-group-system.md. READ IT IN FULL FIRST.

WHY IT IS BEING REWORKED. The owner named the model they actually want, in their words:

  "I want to bring up the reason why I asked for retinue to be turned into simpler units,
   much like how Total War uses and controls units. One group of small spearmen and other
   a group of archers."

The existing spec does not describe that. Its §3.1 defines SquadTargetSize =
ceil(AliveRetinue / MaxSquads) — squads absorb growth EVENLY. Under a typed model that is
wrong: a spearmen unit does not gain archers when you recruit. Squads-as-even-divisions is a
display grouping wearing a command system's clothes. That premise is what you are replacing.

VERIFIED IN THE CODE, so you do not have to re-derive it:
- There are NO unit types in the sim. The only "archetype" is the Mass technical composition
  (SwarmCommands.cpp), not a game concept. One undifferentiated retinue.
- There is NO ranged combat. Swarm.RetinueDPS with contact range is the whole model. There is
  no retinue range CVar at all.
- But CLASSES.md:21 already makes Pathfinder the RANGED class, and line 44 lists ranged
  retinue identities — Huntmaster, Trailwarden, Falconer, Outrider. Typed units and ranged
  troops are canon. They are unbuilt, not unwanted.
- USwarmSubsystem: MaxSquads 8, SquadTargetSize 20, SquadStanding[] refilled per frame,
  SquadId derived from the dense formation-repack slot index.

WHAT SHIPPED SINCE YOUR SPEC — build on it, do not contradict it silently:
- task-045 built the Unit Cam framing and yaw clamp: Emberkeep.UnitCamProj.YawClampDeg 30,
  LookLerp 1.5, SelectedSquad, SelectSpeed 10, FrameFraction 0.6, FrameFloor 6, plus Army
  View as <=8 aggregate blocks on a PLACEHOLDER RING (real centroids do not exist yet).
- task-047 built the brood side: Swarm.BroodSpawnArc 120, BroodSpawnFaceCamera, and a brood
  formation (BroodFormation.Columns 60, RankSpacing 140) reusing SwarmFormation's vocabulary.
  Note that under a typed model, "the brood arrive in ranks" probably becomes "a Legion unit
  of a type advances in ranks" — say whether the brood should be typed too, or whether that
  asymmetry is deliberate.
- task-046 (the sim implementation of YOUR original spec) was drafted but deliberately NOT
  started, because this rework invalidates its premise. It will be rewritten against whatever
  you produce. That is the bar: task-046's author must not have to come back to you with a
  design question.

THREE THINGS TO SETTLE, all in the one document:

1. TYPED UNITS. The unit is the command object and has a TYPE that determines how it fights,
   how it forms up, and what it is for. Spearmen and archers are what the owner named — say
   whether v1 is exactly those two or a small set, and why. Then: how a unit is recruited,
   how it takes casualties, what happens when it is wiped, whether reinforcement refills
   existing units or adds new ones, and what per-type formation defaults are (a spear block
   and an archer line are not the same shape and do not stand in the same place). Use the
   existing SwarmFormation vocabulary — Shape/Columns/Spacing/RankSpacing/Forward/Arc — do
   not invent a parallel one.

2. RANGED COMBAT SCOPE. Archers need it; it does not exist; it is plausibly bigger than
   everything else here combined. Spec the minimum viable version — range bands, volleys vs.
   individual projectiles, what happens when the line of fire crosses your own front rank —
   estimate its size honestly, and RECOMMEND whether it belongs in v1 or whether v1 ships
   spear-only with archers stubbed. Give a recommendation, not a menu. The owner decides, but
   they asked for your judgment.

3. THE UNIT CAM DEFAULT VIEW — your §10.5, reopened. You flagged it yourself: "if the owner
   pictured the latter when asking for a whole army view option, this is the point to say
   so." They have now effectively said so — they want the panel to show MINI RETINUE UNITS by
   default, not the <=8 abstract blocks you recommended and task-045 built. Reconcile that
   against your OWN §6.2 geometry: LeashRadius ~2000uu bounds squad spread to a ~4000uu
   diameter regardless of headcount, covering it at Fov 40 needs the camera at ~5500uu, and a
   sprite there draws 6-12px. If "always show mini units" and "show the whole army" are
   incompatible at one zoom, say that plainly and propose which gives way. Do not quietly
   reverse yourself, and do not quietly dig in — the geometry was good work; the question is
   what to do given the owner wants something it says is hard.

ALSO SPEC: the owner wants a wider "map" mode that exposes more options and information than
the tight Unit Cam view. What is on it, what it is for, and how it relates to Army View as you
already specced it.

FILES YOU MAY WRITE, and nothing else: docs/design/squad-group-system.md (rework in place),
docs/data/squads.json + .schema.md, docs/data/unit-types.json + .schema.md (new, if you
publish per-type numbers).

You have NO editor and NO MCP tools this session, and you hold no resource lock — another
task is using the editor. This is a pure design pass. Do not plan around driving PIE.

KEEP from the existing spec, both verified against the code by the lead: §1.3's sticky-SquadId
finding (SquadId derives from the dense repack index and NeedsFormationRepack fires on ANY
casualty, so membership silently drifts — still true and still load-bearing) and §5's yaw
root-cause diagnosis (Outward is zero in Hero focus, so yaw chases an unstable enemy
centroid). Carry both forward into the new model.

END WITH: a "## Performance requests" section for performance-director (typed units change
the entity-count shape and add per-type behaviour — say what that costs), "## Canon
proposals" for anything implying a GDD.md/CLASSES.md/SYSTEMS.md change, "## Simulation notes"
per your own rule, and "## Assumptions the owner should confirm".

DO NOT TOUCH: ELVTR/Source/**, ELVTR/Content/**, GDD.md, CLASSES.md, SYSTEMS.md,
docs/design/CAMERA-SCALE*.md, or any docs/backlog/ file.

CANON WARNINGS:
- WORLD.md is SUPERSEDED by the 2026-07-22 reset. Current canon is
  docs/narrative/FLAME-FOUNDATION.md. Your own agent definition still lists WORLD.md as a
  source of truth — that line is stale (task-018 exists to fix it).
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 carry a RETRACTED claim that the swarm
  emitter draws zero particles — the cause was GPUComputeSim vs CPUSim and it is fixed.

HAND BACK: the unit types and what makes each distinct, how recruitment/attrition/wipe work,
your ranged-combat recommendation with its size estimate, your answer on mini-units vs blocks
and what gives way, what the map mode carries, and anything in the old spec that did not
survive the change.
```
