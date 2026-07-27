---
id: 017
title: Bring the three consolidated flame-bearer questions to a decision
status: proposed
agent: claude
owns: ["docs/art/flame-bearer-status.md"]
resources: []
depends-on: [15]
evidence: Each of the three questions in flame-bearer-status.md §4 answered by the owner and recorded, with the downstream canon proposals it unblocks listed.
score: {gate: 2, risk: 2, cost: 1}
source: docs/art/flame-bearer-status.md:85
decided: ""
---

## Why now
`docs/art/flame-bearer-status.md` §4 did the hard part already: it pulled a fork that was
scattered across three documents — `vanguard.md` canon proposal 2, `protagonist-prototypes.md`
canon proposal 1, and itself — into one place. It also found, in §2, that the pixels in
hand do not match the prose for **three of the four** protagonist renders.

Its own conclusion is the argument for doing this now: *"there is very little left to
scope from scratch — the open work is a decision plus a fidelity pass, not a fresh design
round."* The consolidation work is done and sitting there waiting for an answer.

The doc also flags the ordering that matters: an owner review of the existing directions
should happen **before any re-roll**, because the §2 QC findings might not survive a
decision that some directions are already out of contention. Re-rolling first would spend
PixelLab credits on directions about to be cut.

## Done when
- All three §4 questions put to the owner and answered, recorded in the doc with the date.
- The §2 QC findings re-checked against the answers — findings against a cut direction
  get retired rather than carried.
- The downstream canon proposals each answer unblocks are listed explicitly.
- No re-rolls. This task spends no PixelLab credits; regeneration is a later task.

## Spawn prompt
```
You are working on Emberkeep (C:\Projects\ELVTRGAME).

docs/art/flame-bearer-status.md §4 consolidates three open questions that were previously
scattered across docs/art/vanguard.md (canon proposal 2),
docs/art/protagonist-prototypes.md (canon proposal 1), and that file itself. They block a
protagonist decision. Task-015 (flipbooks vs 3D) answers one of them and must land first.

Read docs/art/flame-bearer-status.md in full — especially §2 (a QC finding that the
quantized renders do not match the prose for three of the four protos) and §4 — plus the
two docs it consolidates, docs/art/aesthetic-direction.md, and
docs/narrative/FLAME-FOUNDATION.md.
Do NOT read WORLD.md — superseded.

Put each of the three questions to the owner plainly, with the evidence and the tradeoff,
and record the answers in the doc with the date. Then re-check the §2 QC findings against
those answers: a finding against a direction that just got cut should be retired, not
carried forward.

Critical ordering, from the doc itself: the owner review happens BEFORE any re-roll. Do
NOT call any mcp__pixellab__* generation tool — this task holds no pixellab-credits lock,
and re-rolling directions that are about to be cut wastes real money.

Write ONLY docs/art/flame-bearer-status.md. List the downstream canon proposals each
answer unblocks so they can be filed as follow-up tasks.
```
