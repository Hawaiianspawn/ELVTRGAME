---
id: 099
title: Reconcile targets_per_hit against the engine's fixed 8-target ceiling, which flattens the axis for seven of nine melee rows
status: done
agent: gameplay-director
model: sonnet
owns: ["docs/data/unit-types.json", "docs/design/retinue-melee-subtypes.md"]
resources: []
depends-on: []
epic: ""
evidence: A stated resolution in docs/design/retinue-melee-subtypes.md — either the two over-ceiling rows are respecced to values the engine can honour, or the axis is explicitly declared capped with the design consequence written down, or the ceiling itself is raised as a separate follow-on. Whichever way it lands, docs/data/unit-types.json must stop carrying numbers the shipped combat loop silently discards.
score: {feel: 2, risk: 1, cost: 1}
source: lead
teammate: targets-ceiling
decided: "2026-07-31 done"
---

## What surfaced
`task-095` wired the nine melee stat rows into Mass and hit a pre-existing constraint: the
combat loop's nearest-target array is **fixed at 8**, and every `TargetsPerHit` accessor clamps
to `1..8` before use — `RetinueTargetsPerHit`, `BroodTargetsPerHit` and `ArchersTargetsPerHit`
have all done this since before this epic (`SwarmCombatProcessors.cpp:253-260`).

`task-088` specced two rows above that ceiling:

| row | spec | actual in play |
|---|---|---|
| `heavycloak` | 9 | 8 |
| `simplecolumn` | 10 | 8 |

Confirmed live in `task-095`'s own `Swarm.KnightSubtypeReport` output — `row1[v7] … Targets=8`
where `unit-types.json` says 9.

## Why it matters more than two clamped numbers
`targets_per_hit` is the axis `task-094` derived from **solidity**, one of only four axes
separating the sub-types. With both over-ceiling rows pinned to 8, the shipped spread is:

    8, 8, 8, 8, 8, 8, 8, 7, 6

**Seven of nine rows sit at the same value.** The axis is very nearly flat in play while the
data file and the sim both still treat it as a live differentiator — so `retinue-subtypes.json`'s
rankings partly rest on a distinction the game does not make.

## What to decide
1. Respec the two rows within `1..8`, or
2. Declare the axis capped, record what that costs the sub-type spread, and lean the identity
   onto hp/dps/engage instead, or
3. Raise the ceiling — a real change to the combat loop's fixed array and a separate task with
   its own perf question, not something to fold in here.

Whichever way: `unit-types.json` must not keep numbers the engine silently discards. That is the
class of mismatch this codebase has been bitten by repeatedly (`SwarmFragments.h:44`'s "three
things must agree" comment exists for the same reason).

Worth noting for whoever picks this up: **the sim harness has no 8-target ceiling**, so
`Scripts/sim/` will keep ranking a 10-target candidate above an 8-target one. If the ceiling
stays, that is a divergence between the model and the game worth stating in `docs/sim/`.

## Scope fence
- Not the C++ (that is option 3, and a separate task).
- Not `Scripts/sim/**` or `docs/sim/**` — sim-director's. Flag the divergence, don't edit it.

## Spawn prompt

```
You are executing task-099. You are the gameplay-director for Kindled
(C:\Projects\ELVTRGAME). Read docs/backlog/task-099-targets-per-hit-ceiling.md first —
it contains the full finding and the three options. Then read
docs/design/retinue-melee-subtypes.md and docs/data/unit-types.json.

THE PROBLEM: `targets_per_hit` is one of only four axes separating the nine melee
sub-types, and the shipped combat loop clamps it to 1..8. task-088 specced two rows above
that ceiling — `heavycloak` at 9 and `simplecolumn` at 10 — so both land at 8 in play.
The shipped spread is therefore 8,8,8,8,8,8,8,7,6. Seven of nine rows sit at the same
value while the data file and the sim both still treat the axis as a live differentiator.

THE CEILING IS CONFIRMED, re-checked by the lead 2026-07-31 at HEAD:
ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp:253-260 clamps every accessor
(Swarm.RetinueTargetsPerHit, Swarm.BroodTargetsPerHit, Swarm.ArchersTargetsPerHit) to
1..8, and line 275 clamps the per-row Targets array to 1..8 a second time on read. The
CVar help text at lines 199-200 states the reason outright: "the combat loop's own
nearest-K arrays are fixed at 8". task-118 has since landed and been committed, so that
file is no longer being written by anyone — read it freely, but still do not write it.

DECIDE ONE OF THREE, and commit to it:
  1. Respec the two over-ceiling rows within 1..8.
  2. Declare the axis capped. Record what that costs the sub-type spread and lean the
     sub-type identity onto hp/dps/engage instead.
  3. Recommend raising the ceiling — but do NOT implement it. That is a real change to
     the combat loop's fixed-size nearest-target array with its own perf question, and it
     is a separate task. If you pick this, say precisely what the follow-on task must do.

Whichever way it lands, docs/data/unit-types.json MUST STOP carrying numbers the shipped
combat loop silently discards. That class of mismatch has bitten this codebase
repeatedly — SwarmFragments.h:44's "three things must agree" comment exists for the same
reason. Write the resolution and its reasoning into
docs/design/retinue-melee-subtypes.md, not just the data change.

FLAG BUT DO NOT FIX: the sim harness has no 8-target ceiling, so Scripts/sim/ will keep
ranking a 10-target candidate above an 8-target one. If the ceiling stays, that is a real
model-vs-game divergence. State it in your handback as a finding for the sim-director —
docs/sim/** and Scripts/sim/** are not yours and another teammate may be live in them.

YOU OWN ONLY: docs/data/unit-types.json, docs/design/retinue-melee-subtypes.md

DO NOT TOUCH: any C++, Scripts/sim/**, docs/sim/**, docs/backlog/**, GDD.md, SYSTEMS.md,
any .uasset. You have a shell but this task needs no build and no PIE — do not open,
close or rebuild the editor. This task holds no editor lock.

CANON: the game is KINDLED, never Emberkeep. The 4-value colour gate is superseded — not
that it bears on stat rows, but do not "fix" anything toward it.

HAND BACK: which of the three options you chose and why, the exact unit-types.json rows
you changed, what the sub-type spread looks like after your decision, and the sim
divergence stated plainly.
```
