---
id: 030
title: Answer CAMERA-SCALE §4's six open questions
status: proposed
agent: claude
owns: ["docs/design/CAMERA-SCALE-HANDOFF.md"]
resources: ["unreal-editor"]
depends-on: []
evidence: Each of the six questions answered from a PIE session with the value tried, especially the flame-pool ratio against OrthoWidth.
score: {gate: 2, risk: 2, cost: 2}
source: docs/design/CAMERA-SCALE-HANDOFF.md:89
decided: ""
---

## Why now
`docs/design/CAMERA-SCALE-HANDOFF.md:89` is explicit: the six open questions in
`CAMERA-SCALE.md` §4 *"are still the agenda and are still genuinely open. Nothing learned
during the emitter fix answers any of them."* This is a handoff that was written to be
picked up and has not been.

Two are re-flagged as newly urgent because the sprite path now touches them — including
§4.5, the flame pool: `Swarm.FlameRadius` is 900uu against an `OrthoWidth` of 2400. That
ratio is a live tuning question the current build is sitting on, and camera scale
underpins every readability judgment in tasks 026 and 028.

Live work on the `flame-spotlight` branch, so it is warm context rather than a cold start.

## Done when
- All six questions from `CAMERA-SCALE.md` §4 answered, each with the value tried and the
  felt result.
- §4.5 specifically: the flame-pool radius against `OrthoWidth` resolved, or the tradeoff
  stated clearly enough for the owner to pick.
- Answers written into the handoff doc, not `CAMERA-SCALE.md` — task-019 owns that file
  and two tasks must not write the same file.
- Anything that turns out to need a canon change flagged as a proposal.

## Spawn prompt
```
You are working on camera scale in Emberkeep (C:\Projects\ELVTRGAME), on the
flame-spotlight branch.

docs/design/CAMERA-SCALE-HANDOFF.md:89 states that CAMERA-SCALE.md §4's six open questions
are still the agenda and still genuinely open — the emitter fix answered none of them.
Answer them.

Read docs/design/CAMERA-SCALE-HANDOFF.md in full first (it is the handoff written for
exactly this pickup), then docs/design/CAMERA-SCALE.md §4. Note the handoff's own reading
guide: CAMERA-SCALE.md is current EXCEPT §5's "emitter draws zero particles", which is
superseded — the cause was GPUComputeSim vs CPUSim and it is fixed. Skip §2 and §8.1 of
docs/perf/niagara-sprite-refactor.md; they are stale in the body.

Pay particular attention to §4.5, the flame pool: Swarm.FlameRadius is 900uu against
OrthoWidth 2400. Resolve that ratio or state the tradeoff clearly enough for the owner to
choose.

You hold the unreal-editor lock. Use CVars to try values — see the /cvars skill and
Saved/SwarmExecOnPlay.txt — rather than editing source.

Write ONLY docs/design/CAMERA-SCALE-HANDOFF.md. Do NOT write docs/design/CAMERA-SCALE.md —
task-019 owns that file, and two tasks writing one file overwrite each other.

Answer from play, with the value tried and the felt result. Flag anything needing a canon
change as a proposal rather than making it.
```
