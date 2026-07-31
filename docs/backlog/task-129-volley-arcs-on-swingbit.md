---
id: 129
title: Show the volley — an arcing cue on an archer's SwingBit so the ranged line visibly shoots
status: done
agent: claude
model: opus
owns: ["ELVTR/Source/ELVTR/Rendering/VolleySubsystem.h", "ELVTR/Source/ELVTR/Rendering/VolleySubsystem.cpp", "ELVTR/Content/Gore/**", "docs/perf/volley-vfx.md", "docs/perf/evidence/task129/**"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
epic: archers-on-the-field
evidence: A PIE capture at gameplay density in which volley arcs are visibly leaving the archer line and travelling toward the brood, plus a frame-time row at 10k and 40k against docs/perf/evidence/task126/SwarmBench-task126.csv showing the cue costs nothing measurable.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: volley-arcs
decided: "2026-07-31 done"
---

## Why now
Archers are fully simulated, formed up and now drawn as archers (`task-125`/`126`/`127`
all closed), but nothing happens on screen when one shoots. A brood 750uu away simply
takes damage and flashes. `docs/design/squad-group-system.md` §2.2 specced the missing
half in one line — *"the visual is a volley (a cheap arcing Niagara trail or ribbon
triggered on `SwingBit`, purely cosmetic, no gameplay state)"* — and it was never built.
This is the last piece that makes the ranged line read as ranged in play.

## Done when
- **A volley cue leaves the archer line and travels toward the brood, visible in a
  gameplay-density PIE capture without being told where to look.** This is the bar and it
  is a look call — hand the capture over, do not self-certify it.
- The cue is driven off `SwarmAnim::SwingBit` on entities whose squad byte resolves to
  `EUnitType::Archers`, read from the already-published render arrays. **No new fragment,
  no new processor, no new render-buffer bit, no gameplay state.**
- Spearmen and brood produce no cue. Only archers.
- A `Volley.MaxPerFrame` style budget cap bounds the cost, the same way
  `Blood.MaxBurstsPerFrame` already does, and a CVar can switch the whole thing off.
- Frame time at 10k and 40k stated against `docs/perf/evidence/task126/SwarmBench-task126.csv`.
  A visible regression at 40k is a result worth reporting, not a reason to hide the number.
- `docs/perf/volley-vfx.md` records what was built, what the cap does at wave-3 density,
  and — honestly — how often one shot produces more than one cue.

## Spawn prompt

```
You are executing task-129. Read docs/backlog/task-129-volley-arcs-on-swingbit.md first,
then ELVTR/Source/ELVTR/Rendering/BloodSubsystem.h and BloodSubsystem.cpp in full. That
subsystem is the pattern you are copying; its header comment explains every design
constraint that applies to you as well.

GOAL
Archers shoot from 750uu and nothing appears on screen. Add a cheap, purely cosmetic
volley cue so the player can see their ranged line firing.

WHAT IS ALREADY TRUE — do not rebuild or retune any of it
Archers are shipped end to end. EUnitType::Archers is at SwarmCombat.h:46. Seven
Swarm.Archers* CVars carry their stats at SwarmCombatProcessors.cpp:119-158 — 750uu
engage range, a 150uu minimum band, cleave 1. They roll at 20% of every recruit at
SwarmCommands.cpp:271, hold their own formation via SwarmFormation::ReadParamsForType,
and draw from their own team-atlas sub-table. None of this needs touching. Do NOT change
any Swarm.Archers* value, do NOT touch the combat pass, and do NOT add per-entity state.

THE LAZY PATH — build this one
Mirror UBloodSubsystem exactly: a UTickableWorldSubsystem that each frame scans
USwarmSubsystem::GetRenderPositions() and GetRenderAnimBits() — the sim's published
output, read-only — and spawns a fire-and-forget Niagara component per cue via
UNiagaraFunctionLibrary::SpawnSystemAtLocation with bAutoDestroy=true.

Identify an archer from the published bits the same way SwarmRenderActor.cpp:1708 already
does: SwarmSquad::UnitType(SwarmRenderPack::Squad(Bits)) == EUnitType::Archers. Trigger on
SwarmAnim::SwingBit (1 << 5, SwarmFragments.h:27 — it is in PreservedBits, so it survives
in the published buffer).

DIRECTION — the part that needs a decision, and the lazy answer is the right one
You do NOT have a per-shot target. The published render buffer has no victim id, and
adding one is out of scope (it is exactly the per-entity uniqueness cost Design Law 5
rules out, and squad-group-system.md §2.2 says so explicitly).

Compute ONE enemy centroid per frame in the same pass you are already making over the
render arrays — average the positions of non-team (brood) entities — and fire every
volley cue from its archer toward that centroid, ending around Swarm.ArchersEngageRange
away. At horde scale this reads correctly because the brood arrive as a mass. Do NOT
build per-shot targeting, do NOT do a nearest-enemy search per archer, and do NOT publish
a target from the combat pass.

THE ASSET
There is no arrow Niagara system. Duplicate ELVTR/Content/Gore/NS_Blood to
ELVTR/Content/Gore/NS_Volley and retune it into a short arcing streak. You own
ELVTR/Content/Gore/** — NS_Blood itself must keep working exactly as it does today, so
duplicate, never repurpose.

KNOWN TRAPS IN THIS AREA — all of these have cost a session before
- A Niagara emitter set to GPUComputeSim draws NOTHING in this project. CPUSim is what
  works. NS_Swarm's "broken graph" was this and nothing else.
- MCP asset edits are IN MEMORY until you call save_assets([]). unreal-mcp is on port
  9000; use the native UE plugin, not the uefn python bridge.
- If a per-particle colour looks inert, check the MATERIAL first — M_Swarm silently
  discarded every per-particle colour because it had no ParticleColor node at all.
- unreal-mcp CANNOT create an Array DataInterface user parameter. If you need one, it has
  to be added by hand through the User Parameters + button first.
- Adding a UPROPERTY via Live Coding reports success and then crashes the next PIE. A new
  UCLASS with UPROPERTYs needs a full editor-closed rebuild.
- If a "rebuild ELVTR modules?" dialog appears on launch it is usually a false alarm race;
  dismiss it, it blocks MCP until you do.

DO NOT TOUCH
ELVTR/Source/ELVTR/Mass/** (all of it — the sim is not yours), SwarmRenderActor.cpp/.h,
UnitCamProjector.cpp, ELVTR/Content/Spike1/**, docs/perf/niagara-sprite-path.md, GDD.md,
SYSTEMS.md, CLASSES.md, or any docs/design/ file. Other work is live in this tree and in
some of those exact files.

HAND BACK
- The PIE capture(s) proving arcs leave the archer line, in docs/perf/evidence/task129/.
- The frame-time rows at 10k and 40k against
  docs/perf/evidence/task126/SwarmBench-task126.csv.
- docs/perf/volley-vfx.md — what was built, what the cap does at density, and how often
  one shot produces more than one cue. Be honest about that last one; BloodSubsystem's
  header is the tone to match.
- One line on whether the centroid direction actually reads at density, or whether it
  looks wrong from some camera angles. That is a real finding either way.
```
