---
id: 046
title: Build the typed-unit command layer in Mass — sticky type, per-type stance, per-type allocation, published centroids
status: in-progress
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/**", "ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamProjector.h", "ELVTR/Source/ELVTR/UI/UnitCamDirector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamDirector.h"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A PIE capture where a knight unit and an archer unit hold DIFFERENT stances visibly, plus proof that a casualty in one unit moves no soldier between units and changes nobody's type — the defect this task exists to fix.
score: {gate: 2, risk: 2, cost: 3}
source: docs/design/squad-group-system.md
teammate: typed-unit-layer
decided: "2026-07-27 in-progress"
---

## Why now
**A real defect ships in the build today.** `SquadIdForSlot(Slot) = Slot / SquadTargetSize`
derives membership from the dense formation-repack slot index, and `NeedsFormationRepack()`
is `AliveRetinue != PackedRetinueCount` — so *any* casualty anywhere renumbers slots and
silently moves soldiers between squads. Harmless while squads are cosmetic. The moment a
squad owns a stance, a soldier inherits an order it was never given. Verified in
`SwarmSubsystem.h`; documented at `docs/design/squad-group-system.md` §1.3.

It already forces a workaround: task-050 had to derive the Unit Cam's per-soldier sprite
choice from `SizeBucket`/`JitterFragment::Phase` precisely *because* `SquadId` was unstable.
That was good engineering around a bug, and it should not have been necessary.

**This task was drafted once and never started, deliberately** — task-049 replaced its whole
premise. The old draft assumed one undifferentiated pool with
`SquadTargetSize = ceil(AliveRetinue / MaxSquads)`, growth spread evenly across 8 squads.
The current spec is typed units with per-type pools and a completely different allocation
rule. This is a rewrite against what the spec actually says now.

**Two measurements have since removed work from this task, not added it.** task-021 measured
frame time flat from 120 to 814 entities and recommended **not** building an aggregation/LOD
layer — so this builds the command layer only, no entity substitution. task-052 widened the
grid so Archers' 750uu `EngageRange` genuinely reaches, at no measured cost.

## Done when
- **Type is a first-class, permanent property.** v1 is exactly Spearmen and Archers. Assigned
  once at recruit time, never changed by combat, promotion, repacking or reinforcement.
- **`SquadId` is sticky.** Assigned once at recruit time and stable for that soldier's life,
  independent of formation repacking. A casualty in one unit must not move any soldier into
  or out of another, and must not change anyone's type.
- **Formation repack becomes per-unit** — each unit densely packs its own members; a unit
  whose headcount has not changed skips its own repack while a neighbour reforms.
- **Stance is per-unit.** The single global `Stance`/`StanceAnchor` becomes per-unit arrays.
  Orders carry an address — "all units" (the default, byte-for-byte today's behaviour) or a
  specific unit. Per-soldier steering reads its own unit's stance.
- **Per-type allocation replaces the old even-split formula** (§4.1):
  `WantedUnits(type) = ceil(Pool(type) / 80)`, Spearmen claim from `MaxSquads` first,
  Archers clamp to the remainder, overflow folds into that type's own units. Recomputed each
  repack. A wiped unit leaves **no ghost slot** — the derived count simply recomputes.
- **Recruitment is fill-lowest-first within type** (§1.4), growth sites tagged by type.
- **Per-type formation defaults** (§1.7) and Archers' 750uu `EngageRange` (§2.2).
- **Per-unit centroids are published**, accumulated on the existing `PushRenderEntry` pass —
  no second walk over the soldier population.
- **The Unit Cam draws by TYPE, not by hash.** Retire task-050's `SizeBucket` workaround:
  a spearman draws the knight sprite because it *is* a spearman. That is the visible proof
  the typed layer works.
- Evidence per `evidence:` above.

## Spawn prompt
```
You are executing task-046 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight
branch. You are building the typed-unit command layer in the Mass sim, then paying it off in
the Unit Cam.

READ FIRST: docs/design/squad-group-system.md — the approved design (task-049, done). It was
REWRITTEN since this task was first drafted; do not work from memory or from any earlier
description of it. Sections 1.0-1.8, 2.2, 3 and 4.1 are your specification. Implement them.
If something is unimplementable as written, say so in your handback rather than silently
substituting your own approach.

THE DEFECT THAT MAKES THIS URGENT (spec §1.3, verified in the code):
SwarmSubsystem.h has SquadIdForSlot(Slot) = FMath::Clamp(Slot / SquadTargetSize, 0,
MaxSquads - 1), where Slot comes from the dense retinue-wide formation repack, and
NeedsFormationRepack() is AliveRetinue != PackedRetinueCount. So ANY casualty ANYWHERE
renumbers the dense slot index and silently reassigns soldiers between squads. Today that is
cosmetic. The moment units own stances — which this task builds — a soldier starts obeying
orders issued to a different unit. Fix the stickiness FIRST; everything else depends on it.
Note task-050 had to route the Unit Cam's sprite choice through SizeBucket/JitterFragment::Phase
specifically to dodge this instability. That workaround is retired at the end of this task.

WHAT TO BUILD:
1. TYPE AS A FIRST-CLASS PROPERTY. v1 is exactly Spearmen and Archers (spec §1.1). Assigned
   once at recruit time, permanent — never changed by combat, promotion, repacking or
   reinforcement (§1.5: "Type never changes through combat"). The spec §8 proposes encoding
   type as a SquadId range partition rather than a new fragment field, specifically to avoid a
   class-layout change on the hot path — evaluate that, and if you deviate say why.
2. STICKY SquadId — assigned once at recruit time, persisting for that soldier's life
   regardless of repacking elsewhere.
3. PER-UNIT FORMATION REPACK — each unit densely packs its OWN members; a unit whose headcount
   has not moved skips its repack while a neighbour reforms. Cheaper in aggregate than one
   large sort.
4. PER-UNIT STANCE — promote the single global Stance/StanceAnchor to per-unit arrays. Orders
   take an address: "all units" (default — MUST reproduce today's behaviour exactly, this is
   the main regression risk) or one named unit. Per-soldier steering reads its own unit's stance.
5. PER-TYPE ALLOCATION (§4.1), replacing the old ceil(AliveRetinue/MaxSquads) even split:
     WantedUnits(type) = ceil(Pool(type) / 80)          [80 = legibility ceiling]
     Units(Spearmen)   = max(1, Wanted) if Pool > 0 else 0
     Units(Archers)    = clamp(max(1, Wanted), 0, MaxSquads - Units(Spearmen)) if Pool > 0 else 0
   Spearmen claim first, deliberately (§4.1 explains why). Overflow folds into that type's own
   units. Recomputed every repack. A wiped unit leaves NO ghost slot — the derived count just
   recomputes smaller (§1.5). Do not add "is this slot empty" bookkeeping; the formula is the
   mechanism.
6. RECRUITMENT (§1.4): new soldiers join the least-full existing unit OF THEIR TYPE
   (fill-lowest-first, scoped per type). Growth sites are tagged with which type they yield —
   docs/data/unit-types.json growth_source_weight, Spearmen 0.8 / Archers 0.2.
   REINFORCEMENT (§1.6): refills within a type's existing units first, and only grows that
   type's unit count once its derived count rises.
7. PER-TYPE FORMATION DEFAULTS (§1.7) and Archers' EngageRange (§2.2, 750uu with
   MinEngageRange 150uu). NOTE: 750uu now genuinely reaches — task-052 widened GridCellSize
   200->250 giving a 3x3 reach of exactly 750, measured to cost nothing. Before that change it
   would have silently behaved as ~600.
8. PUBLISH PER-UNIT CENTROIDS — accumulate on the EXISTING PushRenderEntry pass, which already
   receives Location and SquadId per unit. Do NOT add a second O(N) walk.

THEN PAY IT OFF IN THE UNIT CAM — this is your visible evidence:
UnitCamProjector.cpp currently picks each soldier's sprite by hashing SizeBucket (task-050's
workaround for the unstable SquadId). Retire that: a soldier draws the knight sprite because
it IS a spearman, and the archer sprite because it IS an archer. Keep everything else task-050
built — the 56x60 cells, full-colour sheets, FullColorFloor/FullColorDimStrength lighting,
SoldierAspect, load-by-content-path. You are changing WHICH sprite a body picks and WHY, not
how it is drawn.

DO NOT BUILD AN AGGREGATION OR LOD LAYER. task-021 measured frame time flat from 120 to 814
entities (8.34 -> 8.71ms, draw calls pinned at 283) and recommended explicitly against it. See
docs/perf/squad-aggregation.md. Cite it; do not re-litigate it.

CONSTRAINTS:
- "All units" addressing must be byte-for-byte today's behaviour. The whole existing retinue
  must keep behaving exactly as it does now when no unit is explicitly addressed. This is the
  main way this task can break the game.
- Mass Entity constraints are design law (GDD §10): no per-unit uniqueness, no special-casing
  at horde scale. A soldier carries small integers, not a roster reference or a pointer.
- Every new tunable gets a CVar with a prose doc-comment, matching the style already there.
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS THEN CRASHES THE NEXT PIE. This changes
  class layout for certain. Use `pwsh Scripts/ue-relaunch.ps1`.
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are in-memory until save_assets([]).
  MCP AssetTools delete/move are known-unreliable on existing asset paths; the working
  fallback is a headless -run=pythonscript commandlet (see docs/data/art/provenance.json).
- SAVED/SwarmExecOnPlay.txt IS GITIGNORED — "git diff is empty" proves NOTHING about it. If you
  change it, verify with:  diff ELVTR/Config/SwarmExecOnPlay.canonical.txt ELVTR/Saved/SwarmExecOnPlay.txt
  and update the canonical copy if the change is permanent. See docs/AGENT-TEAMS.md §8c.
  Values marked (owner-tuned) are deliberate and must survive.
- BEFORE ANY TIMED MEASUREMENT: EditorPerformanceSettings.bThrottleCPUWhenNotForeground
  defaults true and caps unfocused PIE to ~3fps. docs/AGENT-TEAMS.md §8a. Disable, then restore.

EVIDENCE — on-screen proof, not a diff plus "it works". Read docs/AGENT-TEAMS.md §8 for the
capture recipe (Swarm.DebugShotAfter, 1920x1080, works unfocused). Hand back:
1. A capture where a knight unit and an archer unit visibly hold DIFFERENT stances — that is
   the whole point of the command layer and it cannot be faked.
2. Proof that a casualty in one unit moves NO soldier between units and changes nobody's type.
   A logged before/after of per-soldier SquadId and type across a repack is acceptable.
3. Confirmation that "all units" addressing is unchanged from today.

YOU OWN: ELVTR/Source/ELVTR/Mass/** and UnitCamProjector.cpp/.h, UnitCamDirector.cpp/.h.

DO NOT TOUCH: ELVTR/Source/ELVTR/UI/ViewCamCapture.*, ELVTR/Source/ELVTR/Rendering/**,
ELVTR/Content/**, docs/design/** (task-049's — if the spec is wrong, say so, do not edit it),
docs/perf/** , GDD.md, CLASSES.md, SYSTEMS.md, or any docs/backlog/ file.

CANON WARNINGS:
- WORLD.md is superseded by the 2026-07-22 reset; current canon is
  docs/narrative/FLAME-FOUNDATION.md.
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 carry a RETRACTED claim that the swarm
  emitter draws zero particles — the cause was GPUComputeSim vs CPUSim and it is fixed.

HAND BACK: how you encoded type and whether you followed §8's range-partition proposal, the
recruit-time assignment strategy, your three pieces of evidence, confirmation "all units" is
unchanged, what the Unit Cam looks like now that sprite choice follows type, and anything in
the spec that did not survive contact with the code.
```
