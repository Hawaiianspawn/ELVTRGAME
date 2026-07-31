---
id: 065
title: Land the hit flash, retire the stale header, and fix the black late-session capture
status: proposed
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h", "ELVTR/Content/Spike1/**"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
epic: ""
evidence: A PIE screenshot showing struck units flashing white on the shipped Niagara path (not the debug-box renderer), a second screenshot taken after t>350s in the same session proving the capture path no longer returns black, and the corrected SwarmFragments.h header.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now

Three findings surfaced during the 2026-07-29 batch, all landing in the same file set. They
are filed as one task because they share `owns:` — cutting them apart would create three
tasks that cannot run beside each other.

**1. The hit flash does not reach the screen.** `SwarmRenderActor.cpp:1508-1513` decodes
`HitFlashBit` into a per-particle white colour and pushes the array at `:1567-1568`. But the
`User.Colors` parameter is never bound in the Niagara graph, so the array is silently
ignored. The code says so about itself at `:1564`: *"Pushing them is harmless before the
emitter reads them — an unbound User array is simply ignored — so the C++ half can land and
be verified independently of the graph edit."* `task-059` owns that graph edit and is
**parked**, so the C++ half has been sitting complete and inert. Consequence, established by
`task-029`: there is currently no on-screen tell anywhere that distinguishes a unit merely
engaged from one that just took a blow.

**2. `SwarmFragments.h:54-57` is actively misleading.** It states *"Niagara has no
per-particle colour array here"* — which was true when written and is now false; the array
exists in C++. Its conclusion (the hit tell is not on the sprite path) is still correct, but
for a different reason than it gives. This comment already cost real work: it led `task-029`
to a wrong finding, and then led the lead session to "correct" a claim that had been right.
This is the same failure mode as the GPU-sim myth that cost this project days.

**3. `Swarm.DebugShotAfter` returns a fully black image late in long PIE sessions.**
Measured in `task-060`: every capture attempted at t>350s came back black, while every
attempt at t<40s worked, including one on a dense mob. Correlates with elapsed `PlayTime`,
not entity density. This blocks on-screen evidence for exactly the long-running sessions
that need it most — it is why `task-060` has no wave-3 screenshot in either state.

## Done when

- `User.Colors` is bound in NS_Swarm and struck units visibly flash white **on the shipped
  Niagara path**, proven by a PIE screenshot — not by the debug-box renderer, which has always
  flashed and is not the thing in question.
- `SwarmFragments.h`'s header reflects what the code actually does.
- The late-session black capture is diagnosed and either fixed or, if the cause is outside
  this file set, precisely located and reported.

## Spawn prompt

```
You are executing task-065 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

Three related findings, all in one file set. Do them in this order — the first is the one
with visible value, the third is the one that might defeat you.

=== 1. BIND User.Colors SO THE HIT FLASH ACTUALLY RENDERS ===

The C++ half is already done and correct. SwarmRenderActor.cpp:1508-1513 sets
FLinearColor::White for any unit with SwarmAnim::HitFlashBit, and :1567-1568 pushes the array
via SetNiagaraArrayColor(NiagaraComponent, "Colors", ColorScratch). Read :1564's comment
first — the code documents its own unfinished state.

What is missing is the Niagara graph edit. task-059 (PARKED) wrote the recipe in its own
"remaining work is mechanical" section — read docs/backlog/task-059-*.md around line 212
before you start, and follow it rather than inventing an approach:
  - add User parameter `Colors` (NiagaraDataInterfaceArrayColor), matching the name C++ pushes
  - add `Particles.Color` to the same spawn Set-Parameters module that already sets
    Position/SubImageIndex, bound to SelectColorFromArray, indexed the same way
  - copy the existing binding pattern; do not invent one
You may also bind `User.Sizes` (NiagaraDataInterfaceArrayFloat, pushed at :1569-1570) by the
same method if it is free to do so — but the hit flash is the deliverable and Sizes is
optional. Do not let it expand scope.

PROVE IT ON THE SHIPPED PATH. The debug-box renderer has ALWAYS flashed white
(SwarmRenderActor.cpp:1349) — a screenshot of that proves nothing. Your evidence must be the
Niagara sprite path with the debug renderer off.

=== 2. RETIRE THE STALE HEADER ===

SwarmFragments.h:54-57 says "Niagara has no per-particle colour array here, so restoring a
hit tell means either a new array + graph edit or reinstating a cell axis." The array now
exists in C++. Once you land step 1 the whole claim is obsolete; if step 1 fails, the claim
is still wrong about the array and right about the outcome.

Rewrite it to say what is true when you are done. Cite the actual mechanism (User parameter
binding), because the specific way this comment misled people was by naming a cause that had
since changed. Keep the "Don't delete them" instruction about SwingBit/HitFlashBit — that
part is still good advice.

This comment has already cost two sessions real work. Treat the rewrite as a deliverable,
not a chore.

=== 3. THE BLACK LATE-SESSION CAPTURE ===

Swarm.DebugShotAfter returns a fully black PNG when fired late in a long PIE session. Measured
in task-060: all attempts at t>350s black; all attempts at t<40s fine, including a dense mob
at t=38s. Correlates with elapsed PlayTime, not density. DebugCaptureComponent lives in
SwarmRenderActor.cpp, which you own.

Reproduce first — run PIE, fire a shot early, confirm it works, then idle to t>350s and fire
again. Do not start fixing before you have seen it. Plausible directions, none confirmed:
render-target lifetime, a component that stops ticking, throttling when the editor loses
focus (note bThrottleCPUWhenNotForeground, which task-060 had to toggle), or a scene-capture
state that goes stale.

If the cause turns out to live outside your owns list, STOP and report it with file and line.
Do not reach across.

YOU OWN EXACTLY:
  ELVTR/Source/ELVTR/Mass/SwarmFragments.h
  ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp
  ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h
  ELVTR/Content/Spike1/**            (this is where NS_Swarm lives)

DO NOT WRITE: ELVTR/Source/ELVTR/Mass/** other than SwarmFragments.h (task-054's),
ELVTR/Source/ELVTR/Rendering/BloodSubsystem.* (task-060's, just closed),
ELVTR/Source/ELVTR/Spike/** (task-064's), docs/design/**, SYSTEMS.md, GDD.md.

RELATIONSHIP TO PARKED task-059: this task is carved out of 059's file set deliberately —
059 is a large sprite-path rework (state axis, per-unit size) that was parked on 2026-07-28,
and this is the small mechanical remainder that has independent value. Do NOT attempt 059's
broader rework. If you find yourself redesigning the atlas or the packing, stop. When 059
resumes it must reconcile with whatever you land here, so keep changes minimal and comment
them clearly.

ENGINE TRAPS THAT HAVE COST THIS PROJECT DAYS:
1. The emitter is CPUSim, NOT GPUComputeSim. NS_Swarm rendered nothing for days for exactly
   this reason and the graph was never at fault. Do not "fix" it back.
2. unreal-mcp asset edits are IN MEMORY until you call save_assets([]). An edit you never
   saved looks like it worked and vanishes on restart. This will bite you on the graph edit
   specifically — save and verify.
3. unreal-mcp set_properties on a material Custom node's `code` silently no-ops and returns
   true. Read back and compare length before recompiling.
4. Adding a UPROPERTY via Live Coding reports success then crashes the next PIE.

EVIDENCE REQUIRED: a PIE screenshot of struck units flashing white on the Niagara path with
debug boxes OFF; a second screenshot taken at t>350s in the same session proving the capture
path works (or a precise diagnosis of why it cannot); and the rewritten header.

Run `git status` before handing back and confirm nothing outside your owns list changed.
State plainly anything you could not do rather than narrowing scope quietly. Do not run any
`py Scripts/backlog.py` commands — the lead session owns backlog transitions.
```
