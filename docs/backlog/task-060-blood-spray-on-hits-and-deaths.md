---
id: 060
title: Blood — short-lived red pixel spray on every landed blow, built as a decoupled subsystem
status: in-progress
agent: claude
owns: ["ELVTR/Source/ELVTR/Rendering/BloodSubsystem.h", "ELVTR/Source/ELVTR/Rendering/BloodSubsystem.cpp", "ELVTR/Content/Gore/**", "ELVTR/Config/SwarmExecOnPlay.canonical.txt", "docs/perf/blood-particles.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [59]
epic: ""
evidence: A PIE capture of a wave-3 fight with blood on, showing red spray tracking the fighting line, plus a second capture with `Gore.Blood 0` for comparison — and a measured before/after frame time at wave-3 density in docs/perf/blood-particles.md, since spray-on-every-hit is the cost risk this task exists to retire.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: blood-spray
decided: "2026-07-28 in-progress"
---

## Why now

The owner asked for gore, then narrowed it themselves: *"something very simple like red
pixels for blood sim. We dont have to have it live for long."* Combat today is silent
attrition — `docs/GATE1-FUN-PROTOTYPE.md:393` already lists *"dead units vanish with no
death animation"* as a known feel gap. Blood is the cheapest possible answer to it.

Two things make this a good moment. The 2026-07-28 colour-gate change ships the game in
full colour (`Quantize 0`), so red no longer fights a 4-value ramp — this task was not
buildable as specified a week ago. And `SwarmAnim::HitFlashBit` (bit 6, *"struck this
instant"*, `SwarmFragments.h:14`) is **already written by the strike pass and already
packed into the render array** the subsystem publishes. The data this feature needs exists;
nothing in the sim has to change to read it.

That is what keeps the cost at `2`. The `risk: 2` is entirely about volume: the owner chose
spray on hits *and* deaths, and combat is continuous attrition across the whole line, so
wave-3 density could produce a red wash instead of readable feedback. Finding the particle
budget that stays legible is the actual work.

## Done when

- **A blood spray fires on every landed blow.** Driven off `HitFlashBit` in the packed
  render array, read through `SwarmRenderPack`'s symbolic accessors — never by
  reconstructing the bit layout by hand.
- **It is decoupled.** The feature lives in a new `UBloodSubsystem` that reads
  `USwarmSubsystem::GetRenderPositions()` / `GetRenderAnimBits()` (both already public,
  `SwarmSubsystem.h:390-391`) and drives its own Niagara component created at runtime.
  **It must not touch `SwarmRenderActor.{h,cpp}`, `SwarmFragments.h`, `ELVTR/Content/Spike1/**`
  or `L_Spike1.umap`** — `task-059` was parked to free the editor for this task and will
  rewrite all of them when it resumes, so every file left untouched is one less reconcile.
  Creating the component at runtime rather than placing an actor keeps the map out of scope.
- **Particles are short-lived.** The owner's words: it does not have to live for long.
  Sub-second, so the screen clears between exchanges.
- **Volume is bounded and legible at wave-3 density.** A global per-frame cap on spawned
  particles, so the cost has a ceiling that does not scale with kill rate. If the honest
  finding is that spray-on-every-hit reads as a wash at 700 brood, say so, tune it down to
  what *is* legible, and record the number you landed on and why.
- **Every dial is a CVar** with a prose doc-comment in house style — at minimum an on/off,
  a particles-per-hit, a lifetime, and the per-frame cap — with tuned values landing in
  `ELVTR/Config/SwarmExecOnPlay.canonical.txt`. That file is the source the `/cvars` skill
  regenerates `Saved/SwarmExecOnPlay.txt` from; a default set only in C++ will not take
  effect in the owner's sessions.
- **Cost is measured, not asserted.** Frame time at wave-3 density with blood on and off.
- Evidence per `evidence:` above.

**Explicitly out of scope:** a *larger, distinct* death burst. Deaths cannot be located
from the render array — `USwarmDeathProcessor` counts them but never reads Transform
(`SwarmCombatProcessors.cpp:519-545`), and a dying unit is indistinguishable from a hit
unit in the published data. Since the killing blow also sets `HitFlashBit`, deaths *do*
bleed here, just not more than any other hit. The distinct death burst rides in free once
`task-054` spawns persistent corpses, because a corpse is a death event with a position.
Do not add a death-position buffer to the combat processors to work around this —
`task-054` owns those files.

## Spawn prompt

```
You are executing task-060 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

GOAL, from the owner in their own words: "add some gore particles or sprites... something
very simple like red pixels for blood sim. We dont have to have it live for long."

They were asked when blood should spray and chose: ON DEATHS AND ON HITS. They were warned
that hits land continuously across the line so this is the high-volume option, and they
picked it anyway. Build what they asked for, bound the cost, and report honestly what it
looks like at wave-3 density.

WHAT YOU ARE BUILDING
A new UBloodSubsystem (UTickableWorldSubsystem) in ELVTR/Source/ELVTR/Rendering/ that each
frame scans the swarm's published render arrays for units flagged as struck this instant,
and sprays a short-lived burst of red pixel particles at those positions.

The data you need already exists. Do not add anything to the sim:
- USwarmSubsystem::GetRenderPositions() and ::GetRenderAnimBits() are public
  (ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h:390-391).
- The anim byte is PACKED into the int32 render entry alongside size/facing/squad via
  SwarmRenderPack::Pack (SwarmSubsystem.h:371-376). Unpack it with SwarmRenderPack's own
  symbolic helpers in SwarmFragments.h. NEVER hand-roll the shifts and masks — task-059 is
  going to rework that packing when it resumes, and symbolic access is the only thing that
  will survive it.
- SwarmAnim::HitFlashBit (bit 6) means "struck this instant" and is listed in
  SwarmAnim::PreservedBits, so it survives the per-frame walk-cycle rebuild
  (SwarmFragments.h:14-34). That bit is your spawn trigger.

HARD FILE BOUNDARY — STAY DECOUPLED EVEN THOUGH THE LOCK IS GONE
task-059 (Niagara sprite-path rework) was PARKED on 2026-07-28 to free the editor for this
task. It is not running. But it is still WANTED, and when it resumes it will rewrite
SwarmFragments.h, SwarmRenderActor.{h,cpp}, ELVTR/Content/Spike1/** (which includes NS_Swarm,
M_Swarm, T_Swarm_2bit AND L_Spike1.umap), ELVTR/Content/Swarm/**,
ELVTR/Content/Sprites/Swarm/** and docs/perf/niagara-sprite-path.md.

So the boundary stands for a different reason than a lock: every one of those files you
avoid touching is one less thing 059 has to reconcile when it comes back. You may READ all
of them. You may WRITE NONE of them. If you find yourself convinced you need to modify
SwarmRenderActor.cpp, stop and say so in your handback instead — do not just do it.

You own exactly:
  ELVTR/Source/ELVTR/Rendering/BloodSubsystem.h
  ELVTR/Source/ELVTR/Rendering/BloodSubsystem.cpp
  ELVTR/Content/Gore/**            (new folder — put NS_Blood and M_Blood here)
  ELVTR/Config/SwarmExecOnPlay.canonical.txt
  docs/perf/blood-particles.md     (new)

This is why the feature is a subsystem and not an actor: placing an actor would mean
editing L_Spike1.umap, which is task-059's. Create the NiagaraComponent at runtime instead.

ENGINE TRAPS THAT HAVE ALREADY COST THIS PROJECT DAYS — read before you build the asset:
1. The Niagara emitter MUST be CPUSim, NOT GPUComputeSim. NS_Swarm rendered nothing for
   days for exactly this reason and the emitter graph was never at fault. Set it first,
   verify it, and do not debug anything else until you have.
2. Interpolated spawning destroys very short particle lifetimes. Your particles are
   sub-second by design. Turn interpolated spawn OFF.
3. unreal-mcp asset edits are IN MEMORY until you call save_assets([]). An edit you never
   saved will look like it worked and vanish on restart.
4. unreal-mcp set_properties on a material Custom node's `code` silently no-ops and returns
   true. If you write one, read it back and compare length before recompiling.
5. Adding a UPROPERTY via Live Coding reports success and then crashes the next PIE. Class
   layout changes need a full editor-closed rebuild. Plan your members up front.

LOOK
The game ships in FULL COLOUR as of 2026-07-28 (Quantize 0) — see docs/art/aesthetic-direction.md.
The old locked 4-value Demichrome ramp is SUPERSEDED and red is legal now. Do not quantize
the blood or try to fit it to a 4-value palette.

Units are exempted from the demichrome post-process flame lift via CustomStencil
(`Swarm.UnitStencil`, see SwarmRenderActor.cpp around the stencil CVar). Decide deliberately
whether blood matches that exemption or takes the lift, and say which you chose and why —
if you leave it unset the blood will be graded differently from the units it comes out of,
which will read as a bug.

Keep it pixel-scale and unlit to match the sprites. Red on the horde is the whole ask; do
not add smoke, sparks, decals, or ground stains — the owner explicitly asked for something
very simple.

DIALS
Every tunable is a CVar with a prose doc-comment in the house style used throughout
SwarmRenderActor.cpp. At minimum: on/off, particles per hit, lifetime, and a GLOBAL
per-frame spawn cap so cost cannot scale without bound with the kill rate. Tuned values go
in ELVTR/Config/SwarmExecOnPlay.canonical.txt — that is the source file the /cvars skill
regenerates ELVTR/Saved/SwarmExecOnPlay.txt from, and a default set only in C++ will never
take effect in the owner's play sessions. Do not edit the Saved/ file directly.

OUT OF SCOPE — do not work around this
A larger, distinct DEATH burst. Deaths are counted but never positioned:
USwarmDeathProcessor (SwarmCombatProcessors.cpp:519-545) reads Health and Anim, never
Transform. A dying unit is indistinguishable from a struck one in the published render
data. Because the killing blow also sets HitFlashBit, deaths DO bleed under this task —
just no harder than any other hit. The distinct death burst arrives free once task-054
builds persistent corpses, since a corpse is a death event that has a position.
Do NOT add a death-position buffer to SwarmCombatProcessors.cpp or SwarmSubsystem.h to
get around this. Those are task-054's files.

DONE WHEN
- Blood sprays on landed blows, sub-second lifetime, bounded by a per-frame cap.
- Nothing outside your owns list is modified. Run `git status` before you hand back and
  confirm it.
- docs/perf/blood-particles.md records the measured frame time at wave-3 density with
  blood on and off, the particle budget you landed on, and — honestly — whether
  spray-on-every-hit reads as feedback or as a red wash at 700 brood. If it washes out,
  tune it to what works and say what you changed and why. That finding is worth more than
  a clean number.

HAND BACK
A PIE capture of a wave-3 fight with blood on, a second with it off, the measured frame
times, and a one-paragraph verdict on whether the owner's on-every-hit choice survived
contact with horde density. Follow the project's capture recipe in docs/AGENT-TEAMS.md.
State plainly anything you could not do rather than quietly narrowing the scope.
```
