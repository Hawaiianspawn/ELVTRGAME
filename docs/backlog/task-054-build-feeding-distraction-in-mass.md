---
id: 054
title: Build the feeding-distraction mechanic in Mass — corpses, kill attribution, and the three-slot feed
status: proposed
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/SwarmCombat.h", "ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp", "ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.h", "ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Saved/SwarmExecOnPlay.txt"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [53]
epic: feeding-distraction
evidence: A PIE capture showing a killer stopped on a corpse while the fight continues around it, a second showing three feeders on one body and a fourth attacker walking past it, and a measured before/after frame time at wave-3 density.
score: {gate: 1, risk: 3, cost: 3}
source: user
teammate: ""
decided: ""
---

## Why now

This is the build half of `task-053`. It cannot start until the spec closes, because the
spec settles four things this task would otherwise have to invent: what "null" means, how
three feeders share a body, whether feeding pays, and whether symmetric feeding survives
wave-3 density at all.

It is scored `risk: 3` because it retires a genuine technical unknown. The combat model is
deliberately chunk-local and parallel-safe — *"no damage events, no random-access writes
across entities"* (`SwarmCombat.h:10-14`) — and this feature wants three things that model
does not do: a corpse that outlives the entity's death, attribution of a killing blow, and
attackers claiming one of three slots on a specific other entity. The last one is a
cross-entity write by nature. Finding a formulation that stays inside the model is the work.

## Done when

- **Corpses exist.** `USwarmDeathProcessor` currently destroys the entity the frame HP hits
  zero. A body persists long enough to be fed on, and is cleaned up deterministically —
  including the case where nobody ever feeds on it.
- **A killer is identified** without breaking the parallel-safe model. Say what you chose and
  what it costs.
- **The three-slot claim works and is bounded.** Three feeders per corpse, a fourth attacker
  keeps fighting, and slot claiming does not become per-frame arbitration between arbitrary
  entities. If you had to weaken the exactness of "three" to keep it cheap, say so explicitly
  rather than shipping a silent approximation.
- **Feed duration follows the spec's curve**, keyed to the dead unit's `MaxHP` as the armor
  proxy, with the swap-in point for a real armor stat marked in a comment.
- **Every dial is a CVar** with a prose doc-comment matching the house style, and the tuned
  values land in `ELVTR/Saved/SwarmExecOnPlay.txt` — that file overrides C++ defaults at play
  time, and a default set only in code will not take effect in the owner's sessions.
- **It is visible.** A feeding unit must read as feeding on screen, not merely stop moving.
  Whatever the cheapest legible signal is at horde scale — an anim bit, a pose, a state flag
  the renderer can key off — it has to be there, or the mechanic is invisible and untestable.
- **Cost is measured, not asserted.** Frame time at wave-3 density before and after.
- Evidence per `evidence:` above.

## Spawn prompt

```
You are executing task-054 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

READ docs/design/feeding-distraction.md FIRST. It is the spec for this feature, written by
task-053, and it is your source of truth for every design decision. This prompt tells you the
engineering constraints; that document tells you what to build. Where they disagree about
DESIGN, the spec wins. Where they disagree about ENGINEERING, this prompt wins. If the spec is
missing an answer you need, say so in your handback rather than inventing one silently.

GOAL, from the owner, in their words: "a monster eating feature that basically if they defeat
a monster they become null and eat the body. Up to three can be distracted, or armor can play
with how long one can be distracted (have to chomp through the damage)."

Settled by the owner already: both sides feed; the cap of three is PER CORPSE; feed duration
proxies off the dead unit's MaxHP because no armor stat exists yet.

THE THREE HARD PROBLEMS — this feature needs three things the combat model does not have, and
this is the actual work:

1. THERE ARE NO CORPSES. ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp:455 —
   USwarmDeathProcessor destroys the entity via ChunkContext.Defer().DestroyEntity() the frame
   HP <= 0. You need a body that outlives that, for a bounded time, that can be found by
   nearby units and cleaned up deterministically INCLUDING when nobody ever feeds on it.
   Decide whether a corpse stays a Mass entity in a corpse archetype or becomes something
   lighter, and justify it on cost — at wave 3 there are 700 brood and a lot of them die.

2. THERE IS NO KILL ATTRIBUTION. SwarmCombat.h:10-14: combat is continuous attrition, each
   unit bleeds HP at DPS * EnemyCount * dt. Nobody "defeats" anybody. A swing cadence was
   added later (SwingInterval / SwingStrikeAt, strikers hit their K nearest per
   RetinueTargetsPerHit / BroodTargetsPerHit) so a killing blow is derivable, but it is not
   tracked. Note SwarmCombatProcessors.cpp:397 already clamps recorded damage to HP remaining
   for the fight log — read that code, it is the closest existing thing to attribution.

3. SLOT CLAIMING IS A CROSS-ENTITY WRITE, and the model forbids exactly that: "no damage
   events, no random-access writes across entities, so every combat pass stays chunk-local and
   parallel-safe". Three-attackers-claim-one-corpse is arbitration between arbitrary entities
   by nature. Find a formulation that stays inside the model — a deterministic slot derived
   from spatial order, a claim resolved in one dedicated non-parallel pass over corpses only
   (there are far fewer corpses than units), or a probabilistic bound that averages to three.
   The spec should flag which of its rules are expensive; follow that. IF YOU HAD TO WEAKEN
   "exactly three" TO KEEP IT CHEAP, SAY SO PLAINLY IN YOUR HANDBACK — a silent approximation
   that usually gives 2-4 feeders is acceptable engineering but unacceptable to hide.

ALSO REQUIRED:
- FEED DURATION from the spec's curve, keyed to the dead unit's MaxHP. Armor does not exist
  yet; task-002 will add it. Mark the swap-in point with a comment so that task has an obvious
  seam.
- IT MUST BE VISIBLE. A feeding unit that merely stops moving is indistinguishable from a
  pathing bug. Give it the cheapest legible on-screen signal available at horde scale — an
  anim bit in the existing SwarmAnim bits, a pose, a state the renderer can key off. Check
  what SwarmAnim already encodes (SwarmCombatProcessors.cpp uses SwarmAnim::TeamBit) and
  extend that vocabulary rather than inventing a parallel channel. Without this, neither you
  nor the owner can tell whether the feature works.
- EVERY DIAL IS A CVAR with a prose doc-comment in the existing style — each dial in these
  files explains what it does and what the tradeoff is, in plain language. Put your tuned
  values in ELVTR/Saved/SwarmExecOnPlay.txt too. THAT FILE OVERRIDES C++ DEFAULTS AT PLAY
  TIME: a default set in code but not there will not take effect in the owner's sessions.
  This exact trap bit task-045 (LookLerp 1.5 in code, overridden by LookLerp 3 in the exec
  file, so the change never ran). Set BOTH, and keep the file's comment style.
- MEASURE THE COST. Frame time at wave-3 density (700 brood, SYSTEMS.md:45) before and after.
  Both sides feeding means a large fraction of the field may be in a new state at peak.

ENGINEERING CONSTRAINTS:
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS AND THEN CRASHES THE NEXT PIE. This
  feature almost certainly adds fragments, which IS a class-layout change. Use
  `pwsh Scripts/ue-relaunch.ps1`; Scripts/ue-iterate.ps1 picks the right path automatically.
  Budget for full rebuilds — do not try to Live Code your way through new fragments.
- Mass Entity constraints are design law (GDD section 10). No per-unit uniqueness, no
  special-casing at horde scale.
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are in-memory until save_assets([]).
- If the "rebuild ELVTR modules?" dialog appears on launch it is usually a false alarm (the
  BuildId matches) but it BLOCKS MCP until dismissed.

YOU OWN, and may write only: ELVTR/Source/ELVTR/Mass/SwarmCombat.h,
SwarmCombatProcessors.cpp, SwarmCombatProcessors.h, SwarmFragments.h, and
ELVTR/Saved/SwarmExecOnPlay.txt.

DO NOT TOUCH: SwarmProcessors.cpp/.h, SwarmSubsystem.h, SwarmCommands.cpp,
SwarmFormation.cpp/.h (task-046 and task-052 claim these and will conflict);
ELVTR/Source/ELVTR/UI/** ; ELVTR/Source/ELVTR/Rendering/** (task-041, task-043);
ELVTR/Content/** ; GDD.md, CLASSES.md, SYSTEMS.md; docs/design/** — INCLUDING the spec you
are reading, which is task-053's and read-only to you; or any docs/backlog/ file.
If you need a dial that lives in a file you do not own, say so in the handback instead of
reaching into it.

CANON WARNINGS:
- WORLD.md IS SUPERSEDED by the 2026-07-22 narrative reset; current canon is
  docs/narrative/FLAME-FOUNDATION.md.
- docs/perf/niagara-sprite-refactor.md sections 2 and 8.1 carry a RETRACTED claim that the
  swarm emitter draws zero particles — the cause was GPUComputeSim vs CPUSim and it is fixed.

KNOWN TOOLING TRAP, budget for it: the PIE window driven over MCP freezes or near-freezes
simulation while it lacks OS focus. task-045 could not capture a moving camera for this
reason. Swarm.DebugShotAfter N writes a real game-viewport screenshot to Saved/Screenshots/.
If the trap bites, say so plainly and hand back what you could actually prove — do not
substitute a description for a screenshot you did not take, and do not claim behaviour you
could not observe.

EVIDENCE — on-screen proof, not a diff plus "it works":
1. A capture showing a killer stopped on a corpse while the fight continues around it.
2. A capture showing three feeders on one body and a fourth attacker declining it and walking
   on. This is the rule most likely to be silently wrong.
3. Frame time at wave-3 density, before and after.

HAND BACK: how you made corpses persist and what they cost, how you attributed the killing
blow, how you solved slot claiming without breaking the parallel-safe model AND whether
"exactly three" survived or became an approximation, the CVar list with defaults, your
captures, the frame-time numbers, and anything in the spec that turned out to be unbuildable
as written.
```
