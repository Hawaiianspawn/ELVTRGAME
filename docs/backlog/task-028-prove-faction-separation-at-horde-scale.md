---
id: 028
title: Prove faction separation at horde scale
status: proposed
agent: claude
owns: ["docs/art/faction-separation-proof.md"]
resources: ["unreal-editor"]
depends-on: [27]
evidence: Screen captures at 700 units showing a player can tell friend from foe at gameplay zoom, with the failure cases named.
score: {gate: 3, risk: 3, cost: 2}
source: docs/RTS-VERTICAL-SLICE.md:111
decided: ""
---

## Why now
`docs/RTS-VERTICAL-SLICE.md:111` says this is where **Pillar 4 gets tested** — the slice's
explicit purpose for this item. And design law 6 is unambiguous: readable danger at 500
units, and *"if a player dies to something they couldn't parse, that's your bug."*

**Reframed 2026-07-26.** This was filed as a *palette* test. It isn't one. The palette
question is closed — Direction A, strict global 4-value Demichrome, locked 2026-07-12 —
so there is no faction palette to separate. `RTS-VERTICAL-SLICE.md:111` is phrased in
superseded Direction B vocabulary and should be re-expressed here as what it actually
now is: a **silhouette and value-pattern test**.

That makes it more important, not less. `aesthetic-direction.md:39-46` states the cost of
the lock in its own words: *"there is no more warm/cold friend-foe channel, no
faction-reserved third slot… class, faction, and threat identity now ride on shape and
value-pattern alone."* The four shape carriers in `palette.json` — rectangle-flip,
dot-cluster, thin-contour, point+halo — went from an insurance policy for one bright
pixel to the single load-bearing readability mechanism for the whole game. Nothing has
ever tested them at density.

Scored gate 3 / risk 3 because it can fail in a way that invalidates other work, and it
is the **only task on this board that could legitimately reopen the palette decision**.
If shape-only separation cannot carry 700 units, the answer is either an owner exception
to the locked ramp or a roster silhouette rework — and both are far cheaper before
task-027 produces the real sheets than after.

Sequencing note worth watching: this depends on 027 for real sprites, but 027 is expensive.
If the proof looks doubtful, running a cheap version of this test with placeholder values
*before* 027 is a legitimate reordering to propose.

## Done when
- Captures at 700 units — Gate 1's real top wave — at true gameplay zoom.
- The judgment stated plainly: can a player tell friend from foe fast enough to act?
- Failure cases named specifically: which pairings blur, at what density, at what distance.
- If it fails, the finding is the deliverable. A failed proof that is honestly reported is
  worth more than a passed one that was judged generously.

## Spawn prompt
```
You are testing faction readability at horde scale in Emberkeep (C:\Projects\ELVTRGAME).

docs/RTS-VERTICAL-SLICE.md:111 makes this the test of Pillar 4. GDD design law 6: readable
danger at 500 units — "if a player dies to something they couldn't parse, that's your bug."

Read docs/art/palette-strategy.md (task-016 — settles whether separation comes from hue or
from silhouette and value distribution), docs/art/aesthetic-direction.md,
docs/design/CAMERA-SCALE.md for true gameplay zoom, and docs/GATE1-FUN-PROTOTYPE.md for how
to reach 700 units (wave 3).

You hold the unreal-editor lock. PIE on Content/Spike1/L_Spike1, run to wave 3, capture at
gameplay zoom.

The deliverable is a JUDGMENT with evidence: can a player tell friend from foe fast enough
to act on it? Name the specific failure cases — which pairings blur, at what density, at
what distance.

Report honestly. If separation fails, say it fails and say where. A failed proof reported
straight is worth far more here than a pass judged generously, because the fix (palette
strategy or roster silhouettes) is much cheaper before task-027 produces the real sheets
than after.

If task-027's sprites are not ready, running this with placeholder values is a legitimate
cheaper first pass — say so and label the result as provisional.

Write ONLY docs/art/faction-separation-proof.md. Do not edit source or content.
```
