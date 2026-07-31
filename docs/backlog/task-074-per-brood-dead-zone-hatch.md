---
id: 074
title: Scope the archer dead-zone hatch per-brood, not per-archer
status: proposed
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp"]
resources: ["unreal-editor"]
depends-on: []
epic: ""
evidence: Ten PIE runs to wave 3 with Swarm.WaveClearTimeoutSeconds never firing, on a binary built with the editor closed — plus a stated argument for why a per-brood exception does not make Swarm.ArchersMinEngageRange effectively advisory.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now

`task-073` shipped a real improvement and stopped one step short. This is that step, and
the diagnosis is already done — do not rediscover it.

**What landed in `task-073`.** An escape hatch in the archer targeting path: if an archer has
no in-band target at all, it may engage a brood inside `Swarm.ArchersMinEngageRange` (150uu)
that would otherwise be unkillable. Measured effect: **3 of 3 PIE runs clean** where
`task-064` measured roughly **9 stalls in 10**. The common path is closed and that code is
compiled into the current DLL.

**The gap, found by the `task-073` teammate re-reading its own code.** The hatch is scoped
**per archer**, not per brood. `Swarm.ArchersTargetsPerHit` defaults to **1**, so an archer
that already has a legitimate in-band target 300uu out has no spare slot. A straggler sitting
in that same archer's dead zone stays stuck indefinitely — the archer is standing right next
to it and will never take it. The hatch only helps when the archer has *nothing else to do*.

So the original bug survives in a narrower form: not "no archer can shoot it" but "no *idle*
archer can shoot it."

**One unexplained data point, deliberately not attributed.** A straggler at 108uu from its
nearest retinue — inside the 95-150uu dead zone — stalled 20s and needed `task-064`'s timeout,
on a binary that did contain the hatch (DLL written 12:24:41, editor process opened its log
12:24:50, PIE at 12:26:12). It may be an instance of this gap. It may also be an artifact:
`bThrottleCPUWhenNotForeground` was not disabled until 19:28:56, over two minutes *after* that
stall fired, so the run may have been at ~3fps (`docs/AGENT-TEAMS.md` §8a records two prior
misdiagnoses from exactly this). Treat it as unexplained. This gap stands on its own from the
code reading regardless.

## Done when

- A straggler that **nothing can reach** gets engaged, including when the nearest archer
  already has another in-band target.
- Ten PIE runs reach wave 3 with `Swarm.WaveClearTimeoutSeconds` never firing. It stays in as
  a safety valve; it should have nothing to do.
- **The scrum argument is made explicitly.** A per-brood exception is *wider* than the
  per-archer one, so the case for why `ArchersMinEngageRange` is not now effectively advisory
  matters more, not less. If the honest answer is that it is, say so and let the owner judge —
  do not widen it quietly and call the bug closed.
- Whether `ArchersTargetsPerHit = 1` is the right binding constraint is **named, not changed**.
  A dead-zone straggler not consuming the normal target slot may be the cleaner fix; changing
  that default is a balance decision and belongs to the gameplay director.

## Spawn prompt

```
You are executing task-074 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

DO NOT REDISCOVER THE DIAGNOSIS. It is done, twice over, and both halves are precise.

ROUND 1 (task-064) found the root cause: Swarm.ArchersMinEngageRange (150uu,
SwarmCombatProcessors.cpp) makes an archer refuse anything closer than 150uu to ITSELF —
enforced in the steering gate (SwarmProcessors.cpp, URetinueFollowProcessor::Execute) and the
damage pass (USwarmCombatProcessor::Execute). Archers never close to melee. Spearmen reach
only Swarm.MeleeRange = 95uu. A brood with no spearman within 95uu and only archers within
150uu cannot be killed by anything.

ROUND 2 (task-073) shipped a partial fix and found its own gap:
  - What shipped: an escape hatch — an archer with NO in-band target may take a brood inside
    its min range. Measured: 3 of 3 clean runs, against ~9 stalls in 10 before. It works.
  - The gap: the hatch is PER ARCHER, not per brood. ArchersTargetsPerHit defaults to 1, so
    an archer with a legitimate in-band target 300uu out has no spare slot, and a straggler in
    that same archer's dead zone stays stuck forever. The hatch only fires for an idle archer.

YOUR JOB is exactly that gap: make the hatch key off "nothing can reach this brood" rather
than "this archer has nothing to do."

WHAT IS ALREADY IN THE FILES, and whose it is — do not confuse these:
  task-073's changes, which you are EXTENDING, not replacing:
    SwarmProcessors.cpp        -- new FindNearestEnemyBanded(), plus its call site in
                                  URetinueFollowProcessor::Execute's `// --- auto-fight` block
    SwarmCombatProcessors.cpp  -- the dead-zone shadow-tracking block and its promotion,
                                  inside USwarmCombatProcessor::Execute's per-entity loop,
                                  plus the CVarArchersMinEngageRange doc comment
  NOT task-073's, and NOT yours — pre-existing work from another session in the same file:
    SwarmProcessors.cpp        -- Swarm.SimLOD.Stride / Swarm.SimLOD.NearRadius CVars and
                                  their comments, and every other processor in the file
  Leave that alone. Do not revert, tidy, or "fix" it.

READ FIRST:
  ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp and SwarmProcessors.cpp
  ELVTR/Source/ELVTR/Spike/Spike1GameMode.cpp  -- READ ONLY. task-064's timeout valve and its
      LogStalledBrood position dump. That timeout is your MEASUREMENT INSTRUMENT: if your fix
      works it never fires.
  docs/design/squad-group-system.md §1.8       -- archers never close to melee
  docs/AGENT-TEAMS.md §8a                      -- READ BEFORE ANY PIE TIMING, see below

*** THE TWO TRAPS THAT COST task-073 MOST OF ITS SESSION ***

1. NEVER USE LIVE CODING ON THESE FILES. Not even for a function body with no new statics.
   The patch DLL sweeps in the WHOLE translation unit, and SwarmCombatProcessors.cpp holds
   dozens of file-static TAutoConsoleVariables at namespace scope. Loading the patch re-runs
   their initialisers against CVars the base module owns. task-073 got:
     EXCEPTION_ACCESS_VIOLATION reading address 0x0
     UnrealEditor_ELVTR_patch_0!SwarmCombatTuning::HeroMaxHP()  SwarmCombatProcessors.cpp:177
   — a one-line accessor nobody had edited. Tell of the mode: the module name carries a
   `_patch_N` suffix. You need a full rebuild with the editor CLOSED:
     Stop-Editor; Build-Editor; Start-Editor; Wait-Mcp   (all in Scripts/ue-mcp.ps1)
   It takes about nine seconds. Do it yourself if you have the shell; if a lead session is
   coordinating, ask rather than colliding with it.

2. SET bThrottleCPUWhenNotForeground FALSE BEFORE MEASURING ANYTHING. It defaults true and
   caps the engine to ~3fps when PIE runs unfocused. A healthy wave then looks frozen for
   minutes — which is EXACTLY the symptom you are hunting, making it a very easy false
   positive. AGENT-TEAMS.md §8a records two prior misdiagnoses from this. task-073 disabled it
   two minutes AFTER its one interesting stall, which is why that data point is unusable.
   Restore the setting when you finish.

YOU OWN EXACTLY:
  ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp
  ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp
  ELVTR/Config/SwarmExecOnPlay.canonical.txt is the ONE exception — a CVar default only, and
  if you add one, sync ELVTR/Saved/SwarmExecOnPlay.txt too since that is the copy that
  actually executes on play. They are currently byte-identical; keep them that way.

DO NOT edit Spike1GameMode.cpp/.h. Its timeout is your instrument, not your target. Note it
currently has DeploySeconds = 1.f and BreatherSeconds = 2.f rather than the 3f/6f the docs
describe — an open question the owner has not settled. Leave it, and state that your run
timings were measured against that pacing.

THE CONSTRAINT THAT MAKES THIS NON-TRIVIAL: the min range exists so archers do not fire into
their own scrum (SYSTEMS.md §2.2). A per-brood exception is WIDER than task-073's per-archer
one, so your argument for why ArchersMinEngageRange is not now effectively advisory matters
MORE. Make that argument explicitly in your handback. If the honest answer is that it IS
advisory now, say so and let the owner judge — do not widen it quietly and call the bug closed.

WORTH CONSIDERING: task-073 identified ArchersTargetsPerHit = 1 as the binding constraint (no
spare slot). "A dead-zone straggler does not consume the normal target slot" may be the
cleaner shape than a wider range exception. Do NOT change that CVar's default — that is a
balance decision belonging to the gameplay director. Name it if it is load-bearing.

EVIDENCE REQUIRED: ten PIE runs reaching wave 3 with Swarm.WaveClearTimeoutSeconds NEVER
firing, on a binary built with the editor closed. Grep the log for "TIMED OUT ...
force-cleared" — if it appears, you are not done. Do not restart the count to make ten clean
runs appear: report N clean of M attempted, honestly. An honest partial beats a clean claim.

IF YOU GET BLOCKED, SAY SO. task-073's single costliest failure was going quiet for 25 minutes
with a clear instruction outstanding. A stated blocker or a stated disagreement is useful and
can be acted on. Silence cannot be told apart from being finished.

HAND BACK: the approach, the scrum argument, the ten-run log evidence, and anything you could
not do. Do not run `py Scripts/backlog.py` — the lead session owns backlog transitions.
```
