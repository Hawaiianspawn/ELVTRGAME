---
id: 073
title: Close the archer dead zone — brood inside 150uu that nothing can kill
status: done
agent: claude
model: sonnet
owns: ["ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp"]
resources: ["unreal-editor"]
depends-on: []
epic: ""
evidence: Ten PIE runs to wave 3 in which Swarm.WaveClearTimeoutSeconds never fires — i.e. stalls stop happening rather than being cleaned up after — plus a stated reason the chosen fix does not simply reintroduce archers shooting into their own scrum.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: archer-dead-zone
decided: "2026-07-29 done"
---

## Why now

`task-064` diagnosed the wave-clear stalemate precisely and then correctly refused to fix
it, because the root cause is outside the file boundary it was given. This is the fix.

**The mechanism, measured rather than theorised.** `Swarm.ArchersMinEngageRange`
(`SwarmCombatProcessors.cpp:132`, default 150uu) makes an archer refuse to engage anything
closer than 150uu *to itself* — enforced both in the steering gate
(`SwarmProcessors.cpp:792-793`) and in the damage pass (`SwarmCombatProcessors.cpp:340`,
`if (DistSq >= MyRangeSq || DistSq < MyMinRangeSq) return;`). Archers also never close to
melee (`squad-group-system.md` §1.8). Spearmen reach only `Swarm.MeleeRange`, 95uu.

So a brood standing next to an archer is inside that archer's dead zone, and if no spearman
happens to be within 95uu, **nothing in the game can kill it.** The archer cannot shoot it
and will not move away from it. The CVar's own doc-comment describes the gap without
noticing it: *"Just past `Swarm.MeleeRange` (95)."*

Confirmed live, twice, independently: 7 brood held at an exact unchanged count for 135+
continuous seconds, all 18-38uu from their nearest retinue; and across a 10-run batch every
stall's stragglers sat 6-61uu from nearest retinue, almost all under 40uu.

**What is shipped today is a safety valve, not a fix.** `task-064` added
`Swarm.WaveClearTimeoutSeconds` (20s), which force-clears and destroys frozen stragglers so
the run advances. It works — 10/10 runs reached wave 3 — but it papers over units the player
can watch standing there, unkillable, for twenty seconds. Roughly 9 of 10 runs still stall;
about half resolve only because a spearman eventually wanders into range.

## Done when

- The dead zone no longer produces permanently-unkillable brood. The bar is that
  `Swarm.WaveClearTimeoutSeconds` **stops firing** across ten wave-3 runs — stalls prevented,
  not cleaned up after. The timeout stays in as a safety valve; it should simply have nothing
  to do.
- The chosen approach is stated with its tradeoff. `task-064` named three, none obviously
  right and all balance decisions rather than mechanical ones:
  1. Narrow or condition `ArchersMinEngageRange` so it excludes less aggressively.
  2. An escape hatch — if nothing else is engaging this brood, let the archer take it
     regardless of min range.
  3. Give archers a slow reposition/backstep so they are never *permanently* dead-zoned.
- **Whatever is chosen does not simply undo what the min-range exists for.** §2.2's intent is
  "don't shoot into your own scrum". A fix that deletes the dead zone re-creates the problem
  the dead zone was added to solve, and the handback must say why it does not.
- Any new tuning value is a CVar with a prose doc-comment in the house style, defaulted in
  `ELVTR/Config/SwarmExecOnPlay.canonical.txt`.

## Spawn prompt

```
You are executing task-073 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

THIS IS A FIX FOR AN ALREADY-DIAGNOSED BUG. The diagnosis is task-064's, it is measured, and
you should verify it rather than redo it.

THE MECHANISM: Swarm.ArchersMinEngageRange (SwarmCombatProcessors.cpp:132, default 150uu)
makes an archer refuse to engage anything closer than 150uu TO ITSELF — enforced in the
steering gate (SwarmProcessors.cpp:792-793) and in the damage pass
(SwarmCombatProcessors.cpp:340: `if (DistSq >= MyRangeSq || DistSq < MyMinRangeSq) return;`).
Archers never close to melee. Spearmen reach only Swarm.MeleeRange = 95uu. So a brood next to
an archer, with no spearman within 95uu, cannot be killed by anything. Read that CVar's
doc-comment — it says "Just past Swarm.MeleeRange (95)" and describes the gap without
noticing it creates one.

MEASURED EVIDENCE, already gathered — do not spend a session re-establishing it:
  - 7 brood frozen at an EXACT unchanged count for 135+ continuous seconds, 18-38uu from
    nearest retinue.
  - Across a 10-run batch, every stall's stragglers sat 6-61uu from nearest retinue.
Reproduce it once to confirm you are looking at the same thing, then fix it.

READ FIRST:
  ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp  -- the min-range CVar and damage pass
  ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp        -- URetinueFollowProcessor steering gate
  ELVTR/Source/ELVTR/Spike/Spike1GameMode.cpp        -- READ ONLY. task-064's timeout valve
                                                        and its LogStalledBrood dump.
  docs/design/squad-group-system.md §1.8             -- archers never close to melee
  docs/AGENT-TEAMS.md §8a                            -- READ THIS BEFORE ANY PIE TIMING.
      bThrottleCPUWhenNotForeground defaults true and caps the engine to ~3fps when PIE runs
      unfocused. task-064 lost real time to this. Set it before measuring anything, and
      restore it afterwards.

YOU OWN EXACTLY:
  ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp
  ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp
  ELVTR/Config/SwarmExecOnPlay.canonical.txt is the ONE exception — you may add a CVar
  default there if your fix introduces one, and nothing else in that file.

DO NOT WRITE ANYTHING ELSE. In particular do NOT edit Spike1GameMode.cpp/.h — task-064's
timeout stays exactly as it is. It is your MEASUREMENT INSTRUMENT for this task: if your fix
works, that timeout stops firing.

THREE CANDIDATE APPROACHES, from task-064's handback. None is obviously right and all are
balance tradeoffs, so pick deliberately and justify it:
  1. Narrow or condition ArchersMinEngageRange so it excludes less aggressively.
  2. An escape hatch — if nothing else is engaging this brood, let the archer take it
     regardless of min range.
  3. Give archers a slow reposition/backstep so they are never PERMANENTLY dead-zoned.

THE CONSTRAINT THAT MAKES THIS NON-TRIVIAL: the min-range exists for a reason — SYSTEMS.md
§2.2's "don't shoot into your own scrum". A fix that simply deletes or guts the dead zone
recreates the problem it was added to solve, and you will have traded a visible bug for an
invisible one. Your handback MUST say why your approach does not do that.

EVIDENCE REQUIRED: ten PIE runs reaching wave 3 in which Swarm.WaveClearTimeoutSeconds NEVER
FIRES. The bar is that stalls stop happening — not that they get cleaned up, which already
works. Watch the log for the "TIMED OUT ... force-cleared" line; if it appears, you are not
done. If you cannot get ten clean runs, say how many you got and what the failures looked
like. An honest partial beats a clean claim.

ENGINE TRAPS:
- Adding a UPROPERTY via Live Coding reports success then crashes the next PIE. Class-layout
  changes need a full editor-closed rebuild. Plan members up front.
- unreal-mcp asset edits are in memory until save_assets([]).
- Other sessions may share this working tree. If you find a file already modified that you
  did not touch, DO NOT revert it — report it.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: which approach you took and why, why it does not reintroduce shooting-into-scrum,
the ten-run log evidence showing the timeout never fired, and anything you could not do.
```
