---
id: 047
title: Make the brood arrive as a front, in ranks, instead of surrounding the hero as a mob
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/SwarmCommands.cpp", "ELVTR/Source/ELVTR/Mass/SwarmFormation.cpp", "ELVTR/Source/ELVTR/Mass/SwarmFormation.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.h", "ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Saved/SwarmExecOnPlay.txt"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A PIE screenshot showing the brood arriving as legible ranks from the front while the retinue faces them, and a second showing the spawn front stays in front after the camera/formation facing changes.
score: {gate: 2, risk: 2, cost: 2}
source: user
teammate: brood-front-ranks
decided: "2026-07-27 done"
---

## Why now
The owner tuned `Swarm.Formation.Spacing` to 42.4 and put the *retinue* into readable rows
in the Unit Cam, then saw the obvious next move: the enemy should read the same way.

Half the machinery exists and is switched off. `Swarm.BroodSpawnArc` defaults to **360** —
surrounded on all sides — and the CVar's own comment already argues against that default:
*"An arc is what lets a wave arrive as a FRONT — which is the situation the stances are
actually about, since Hold only means something if there is a direction to hold against."*
Hold and Rally are currently stances without a thing to hold against.

The other half doesn't exist: brood spawn on a ring with speed jitter and walk at the hero
individually. There is no brood formation, so a wave is a mob. `SwarmFormation.cpp` already
solves exactly this problem for the retinue and is the template.

## Done when
- **The brood arrive from the front.** `Swarm.BroodSpawnArc` gets a default that is a front,
  not a full ring. Pick the width from what reads well in play and say why.
- **The front tracks facing.** `Swarm.BroodSpawnArcCenter` is currently a fixed world bearing
  (+X = 0), while the retinue formation faces the camera (`Swarm.Formation.FaceCamera 1`
  tracking `Emberkeep.Cam.Yaw`). A fixed world bearing stops being "the front" the moment the
  facing changes. Make the arc centre follow the same heading the retinue faces, with an
  override for scripted encounters that want a fixed bearing.
- **The brood arrive in ranks.** A brood-side formation so a wave reads as advancing lines
  rather than a crowd — reusing `SwarmFormation`'s vocabulary (shape / columns / spacing /
  rank spacing) rather than inventing a second system.
- **Ranks survive contact plausibly.** State what happens to the formation when the wave
  meets the retinue — does it dissolve into the existing per-unit steering, hold, or degrade
  gradually. A rank structure that snaps rigidly through a melee will read worse than a mob.
- **`Swarm.BroodSpeedJitter` is reconciled.** It defaults to 0.15 explicitly to string the
  tide "into a ragged arrival instead of one rigid wall" — which directly fights rank
  legibility. Decide which wins, tune it, and say what you chose.
- Everything new is a CVar with a prose doc-comment, and the tuned values land in
  `Saved/SwarmExecOnPlay.txt` so they persist across sessions.
- Evidence per `evidence:` above.

## Spawn prompt
```
You are executing task-047 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight
branch.

GOAL, from the owner: the enemy should arrive as a FRONT, in ROWS — not as a mob that
surrounds the hero. Their words: "The enemies should mostly come from the front."

This came out of them tuning the RETINUE into readable ranks (Swarm.Formation.Spacing 42.4,
tight rows that read cleanly in the Unit Cam panel) and wanting the tide to read with the
same clarity.

WHAT ALREADY EXISTS — read this before designing anything:
- ELVTR/Source/ELVTR/Mass/SwarmCommands.cpp lines ~27-53. Swarm.BroodSpawnRadiusMin (2500)
  and RadiusMax (4000) define the ring brood spawn in. Swarm.BroodSpawnArc DEFAULTS TO 360
  — surrounded on all sides. Swarm.BroodSpawnArcCenter (0) is the bearing that arc centres
  on, world +X = 0, and is IGNORED while Arc is 360. The comment above those CVars already
  makes the argument for you: "An arc is what lets a wave arrive as a FRONT — which is the
  situation the stances are actually about, since Hold only means something if there is a
  direction to hold against." So half of this is a default that was never turned on.
- ELVTR/Source/ELVTR/Mass/SwarmFormation.cpp — the RETINUE formation system: Shape
  (0=Ring, 1=Block, 2=Wedge, 3=Arc), Columns, Spacing, RankSpacing, Forward, Arc, ArcRadius,
  Yaw, FaceCamera, Compact. This is your template for the brood side. REUSE ITS VOCABULARY —
  do not invent a second, differently-named formation system.
- Swarm.BroodSpeedJitter (0.15, SwarmCommands.cpp) exists explicitly to string the wave "into
  a ragged arrival instead of one rigid wall". That is in direct tension with rank
  legibility. You have to reconcile it — see below.

WHAT TO BUILD:
1. FRONT-FACING SPAWN. Change the BroodSpawnArc default from 360 to a front. Choose the width
   from what actually reads in play (~90 is one flank, ~30 is a column down one approach per
   the existing doc-comment) and justify your pick.
2. THE FRONT MUST TRACK FACING. BroodSpawnArcCenter is a FIXED WORLD BEARING today, but the
   retinue formation faces the camera (Swarm.Formation.FaceCamera 1, tracking
   Emberkeep.Cam.Yaw). A fixed bearing stops being "the front" as soon as facing changes, so
   the owner would see enemies spawning behind them. Make the arc centre follow the same
   heading the retinue faces, and keep a way to override it to a fixed bearing for scripted
   encounters. Mirror how Formation.FaceCamera/Formation.Yaw compose — same idea, same shape.
3. BROOD RANKS. Give the brood a formation so a wave reads as advancing lines. Reuse
   SwarmFormation's shape/columns/spacing/rank-spacing vocabulary. The brood formation should
   be able to differ from the retinue's (a wide shallow line of attackers against a tight
   block of defenders is the readable case), so these are separate CVars, not shared ones.
4. RANKS UNDER CONTACT. Decide and state what happens when the wave meets the retinue.
   Rigidly holding rank through a melee will look worse than a mob — a graceful degrade into
   the existing per-unit steering is probably right, but it is your call and it must be
   deliberate, documented in the CVar comment, and visible in your evidence.
5. RECONCILE BroodSpeedJitter. At 0.15 it deliberately ragged-ifies arrival, which fights the
   rows. Either lower it and say what is lost, or keep it and explain how ranks survive it.
   Do not silently leave the two settings fighting each other.

CONSTRAINTS:
- Mass Entity constraints are design law (GDD section 10): no per-unit uniqueness, no
  special-casing at horde scale. The brood formation must be slot math of the same cost class
  as the retinue's, not per-unit bookkeeping.
- "We are the good guys" (gameplay design law 9): the Still Legion ADMINISTRATES. A tide that
  advances in ranks reads as an implacable institution rather than a zombie mob, which serves
  the fiction — worth getting right rather than merely functional.
- Every new dial is a CVar with a prose doc-comment matching the style already in these files
  (each existing dial explains what it does and what the tradeoff is, in plain language).
- Put your tuned values in ELVTR/Saved/SwarmExecOnPlay.txt so they persist. That file
  OVERRIDES C++ defaults at play time — a default you set in code but not there will not take
  effect in the owner's sessions. (This exact trap just bit task-045: LookLerp 1.5 in code was
  being overridden by LookLerp 3 in the exec file, so the change never ran.) Set BOTH, and keep
  the file's existing comment style — every line documents its dial.
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS AND THEN CRASHES THE NEXT PIE. If you
  change class layout use `pwsh Scripts/ue-relaunch.ps1`. Scripts/ue-iterate.ps1 picks the
  right path automatically.
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are in-memory until save_assets([]).

KNOWN TOOLING TRAP, budget for it: the PIE window driven over MCP freezes or near-freezes
simulation while it lacks OS focus. task-045 could not capture a moving camera for this
reason. If it bites, say so plainly and hand back what you could actually prove — do not
substitute a description for a screenshot you did not take, and do not claim dynamic
behaviour you could not observe.

YOU OWN: SwarmCommands.cpp, SwarmFormation.cpp/.h, SwarmProcessors.cpp/.h, SwarmFragments.h
(all under ELVTR/Source/ELVTR/Mass/), and ELVTR/Saved/SwarmExecOnPlay.txt.

DO NOT TOUCH: ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h and the squad plumbing
(SquadId/SquadStanding/stance arrays) — task-046 owns that work and will conflict.
Also off limits: ELVTR/Source/ELVTR/UI/** (task-045), ELVTR/Source/ELVTR/Rendering/**
(task-041), ELVTR/Content/**, GDD.md, CLASSES.md, SYSTEMS.md, docs/design/**, or any
docs/backlog/ file.

NOTE ON Swarm.Formation.Spacing: the owner just set this to 42.4 (retinue), deliberately
below the ~70 threshold the old comment warned about. Do not "fix" it back. If the retinue
line visibly seethes, the separation force is the thing to lower — say so, do not revert
their tuning.

CANON WARNINGS:
- WORLD.md is superseded by the 2026-07-22 reset; current canon is
  docs/narrative/FLAME-FOUNDATION.md.
- docs/perf/niagara-sprite-refactor.md sections 2 and 8.1 carry a RETRACTED claim that the
  swarm emitter draws zero particles — the cause was GPUComputeSim vs CPUSim and it is fixed.

EVIDENCE — on-screen proof, not a diff plus "it works":
1. A PIE screenshot of the brood arriving as legible ranks from the front, with the retinue
   facing them. Swarm.DebugShotAfter N writes a real game-viewport screenshot to
   Saved/Screenshots/.
2. A second screenshot proving the spawn front is STILL in front after the facing changes —
   this is the part that silently breaks if the arc centre stays a fixed world bearing.
3. Say what the ranks look like at the moment of contact, with a capture if you can get one.

HAND BACK: the arc width and centring approach you chose and why, the brood formation CVars
with defaults, what you did about BroodSpeedJitter, what happens to ranks under contact, your
screenshots, and anything that turned out to fight the existing steering.
```
