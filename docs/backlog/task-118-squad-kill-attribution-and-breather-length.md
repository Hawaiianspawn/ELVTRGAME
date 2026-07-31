---
id: 118
title: Credit kills to the squad that landed them, and give the Breather long enough to read
status: done
agent: claude
model: opus
owns:
  - "Source/ELVTR/Mass/SwarmSubsystem.h"
  - "Source/ELVTR/Mass/SwarmCombatProcessors.cpp"
  - "Source/ELVTR/Mass/SwarmProcessors.cpp"
  - "Source/ELVTR/Spike/Spike1GameMode.h"
  - "Source/ELVTR/Spike/Spike1GameMode.cpp"
resources: ["unreal-editor"]
depends-on: []
epic: ui-showcase
evidence: >
  A full editor-closed rebuild, then a PIE run through at least two waves showing
  per-squad and hero kill counters on screen or in the log, with the per-squad sum
  reconciling against the existing run-wide `KilledBrood` total, and the wave
  accumulators visibly zeroing at `BeginWave()` while the run accumulators keep
  climbing. Frame time at the same population must not regress measurably against
  `docs/perf/one-camera-bench.md`.
score: {feel: 2, risk: 3, cost: 2}
source: docs/ui/end-of-wave-showcase.md §5
decided: "2026-07-31 done"
teammate: kill-attribution
---

## Why now
`docs/ui/end-of-wave-showcase.md` (task-116, closed 2026-07-31) specifies a five-row
board at the wave break that ranks who did the work. **Its data does not exist.**
`SwarmTelemetry.h` tracks `KilledBrood` and `KilledRetinue` as run-wide aggregates and
nothing else — there is no per-squad, per-type or per-entity attribution anywhere, so
nothing can be ranked.

That spec's §5 is a finished engineering contract written against the real current code,
including the reason the obvious implementation does not work. This task builds it.

**The Breather half rides along.** `Spike1GameMode.h:66` sets `BreatherSeconds = 2.f`.
Two seconds is not enough time to read a five-row board, so the panel task-116 specified
would flash past unread. That is a one-float change plus a decision about whether the
phase should advance on a dismiss instead of a fixed timer — and it lands in the same
file and the same rebuild window as the attribution work, which is why it is here and not
its own task.

## Done when
- **Attacker squad identity reaches the kill site.** `USwarmSubsystem::FGridEntry` carries
  the attacker's `SquadId` as one `uint8`, populated in `AddToGrid` from the
  `FSwarmAnimFragment::SquadId` that pass already reads.
- **Credit happens in the combat pass, not the death pass.** `USwarmDeathProcessor` is too
  late — it sees `HP <= 0` after `USwarmCombatProcessor` has already summed every
  contributing attacker's damage into one number and discarded who they were. The credit
  goes in the per-victim `QueryNeighbors` visit, at the point the frame's `Damage` takes
  `HP` to `<= 0`.
- **One squad is credited per kill** — the first claimed this frame, by iteration order.
  This reuses the arbitrary tie-break the code already documents for `BlowsClaimed`.
  Do **not** build proportional-damage credit; it needs a second bookkeeping pass this
  model does not otherwise carry, and the spec explicitly rejected it.
- **`USwarmSubsystem` gains `CreditKill(uint8 SquadByte)`** beside `AddKills`, feeding
  `WaveKilledBySquad[MaxSquads]` and `RunKilledBySquad[MaxSquads]` (both `int32`).
  Wave counters reset in `Spike1GameMode::BeginWave()`; run counters in `ResetRunState()`.
- **Hero kills are counted** as `HeroWaveKills` / `HeroRunKills`, incremented in the
  existing `if (bHeroStriking)` branch when that blow is lethal. Same reset cadence.
- **No type-level storage.** The "Types" view is a read-time fold over squads sharing
  `GetSquadType(i)`. Adding a per-type counter is a bug, not a convenience.
- **`BreatherSeconds` is long enough to read the board**, or the Breather advances on an
  explicit dismiss with a timer backstop. Pick one, say why in the code comment.
- **No per-entity attribution.** The spec named it as the expensive piece and gave the
  fallback: squad rows plus the hero row. Stay inside that.

## Spawn prompt

```
You are executing task-118 in the Kindled repo (C:\Projects\ELVTRGAME).

GOAL: give the Mass combat pass squad-level kill attribution, so the end-of-wave board
specified in docs/ui/end-of-wave-showcase.md has data to rank. Plus one adjacent fix in
the same file: the Breather phase is too short to read that board.

READ FIRST — docs/ui/end-of-wave-showcase.md §5 IS YOUR SPEC. It is a finished
engineering contract written against the real current code by the ui-director, closed
and approved 2026-07-31. §5.3 in particular tells you WHERE the credit has to happen and
WHY the obvious place does not work. Read §5.1 through §5.4 in full before writing code.
Then read Source/ELVTR/Mass/SwarmSubsystem.h (FGridEntry ~line 194, AddKills ~line 131,
ResetRunState ~line 548), Source/ELVTR/Mass/SwarmCombatProcessors.cpp (the per-victim
QueryNeighbors loop, ~line 565), and Source/ELVTR/Spike/Spike1GameMode.h.

THE SHAPE, from §5.3 — do not redesign it:
  1. Add the attacker's SquadId as one uint8 on USwarmSubsystem::FGridEntry, populated in
     AddToGrid from FSwarmAnimFragment::SquadId (that pass already has it in hand).
     FGridEntry is rebuilt from scratch every frame, so this is a wider transient struct,
     NOT a layout change on a hot persistent fragment.
  2. USwarmDeathProcessor is TOO LATE. It only sees HP <= 0 after the combat pass summed
     every contributing attacker's Damage into one number and threw away who they were.
     Credit must happen INSIDE USwarmCombatProcessor's per-victim visit, where Damage is
     computed and Health[i].HP -= Damage is applied.
  3. Credit exactly ONE squad per kill — the first claimed this frame, by iteration
     order. This is the same arbitrary, already-documented tie-break the code uses for
     BlowsClaimed. DO NOT build proportional-damage credit. The spec considered it and
     rejected it because it needs a second bookkeeping pass this model does not carry.
  4. Add USwarmSubsystem::CreditKill(uint8 SquadByte) beside AddKills, incrementing
     WaveKilledBySquad[MaxSquads] and RunKilledBySquad[MaxSquads] (int32 each).
  5. Hero: the existing `if (bHeroStriking)` branch already isolates the hero's blow. One
     more increment there when the blow is lethal gives you HeroWaveKills/HeroRunKills.
  6. Resets: wave accumulators in Spike1GameMode::BeginWave(), run accumulators in
     USwarmSubsystem::ResetRunState().

DO NOT add per-type storage. The "Types" view is a read-time fold over squads sharing
GetSquadType(i). A per-type counter is a bug.

DO NOT add per-entity ("this individual soldier had 12 kills") attribution. §5.4 names it
as the expensive piece and gives the shipped fallback: squad rows plus the hero row. It
needs a promotion-system design decision that does not exist yet. Stay inside the
fallback.

THE BREATHER FIX, same rebuild window: Spike1GameMode.h:66 has BreatherSeconds = 2.f.
Two seconds cannot be read. Either raise it to something a five-row board survives, or
advance the phase on an explicit dismiss with a timer backstop. PICK ONE, implement it,
and say why in a code comment. task-115's menu spec hard-pauses the sim and freezes
PhaseTimer while the pause menu is open — whatever you build must respect that freeze.

BUILD DISCIPLINE — this matters and has bitten before:
  - You are changing class layout on a UObject. DO NOT rely on Live Coding. Adding a
    UPROPERTY via Live Coding reports success and then crashes the next PIE. Close the
    editor, do a FULL rebuild, then reopen.
  - You hold the `unreal-editor` resource lock for this task. Another session may share
    this working tree — never restart or close the editor on someone else's behalf, and
    if you find unexpected modified files that are not yours, leave them alone and say so.

EVIDENCE — a written "it works" is not accepted on this repo. Produce:
  - confirmation of a full editor-closed rebuild
  - a PIE run through at least TWO waves, with per-squad and hero counters visible on
    screen or in the log
  - the per-squad sum reconciled against the existing run-wide KilledBrood total (they
    should agree; if they do not, explain the gap rather than papering over it)
  - the wave accumulators visibly zeroing at BeginWave() while run accumulators climb
  - a frame-time sanity check at the same population against docs/perf/one-camera-bench.md
    (measured 433fps at 1,000 units, 2026-07-28). A measurable regression is a finding,
    not something to absorb quietly.

YOU OWN ONLY: Source/ELVTR/Mass/SwarmSubsystem.h,
Source/ELVTR/Mass/SwarmCombatProcessors.cpp, Source/ELVTR/Spike/Spike1GameMode.h,
Source/ELVTR/Spike/Spike1GameMode.cpp

DO NOT TOUCH: docs/ui/** (the specs are closed), Scripts/sim/** and docs/sim/**
(a teammate is live in there right now), docs/backlog/**, GDD.md, SYSTEMS.md, any
.uasset, or any Mass file not listed above. If the work genuinely needs a file outside
your list, STOP and hand back saying which and why — do not widen your own scope.

CANON: the game is KINDLED. CVars are Kindled.* — the Swarm./Emberkeep. prefixes were
renamed 2026-07-31 (tasks 092/093). Never reintroduce them.

HAND BACK: the evidence items above, the tie-break you implemented, which Breather answer
you chose and why, and anything in §5's contract that turned out to be wrong about the
real code — the spec was written by reading, not by compiling, so a mismatch is expected
and worth reporting rather than silently working around.
```
