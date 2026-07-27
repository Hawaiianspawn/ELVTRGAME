---
id: 010
title: Run the retinue militia acceptance checklist against the packed sheet
status: proposed
agent: pixel-art-director
owns: ["docs/art/retinue-militia.md"]
resources: []
depends-on: []
evidence: All eleven checklist items at docs/art/retinue-militia.md:426-436 ticked or failed, each backed by a pixelpipe quantize report or a stated pixel measurement.
score: {gate: 2, risk: 1, cost: 1}
source: docs/art/retinue-militia.md:426
decided: ""
---

## Why now
Eleven unrun acceptance items — three values only with zero Pale pixels, Bone dominant
at 38–44%, Dark ≤30%, deliberately uneven shoulders, value-split head and torso, club
off-vertical, mismatched legs, Dark eye recesses, no dither under 2×2, bbox ≤48×48.

The militia is the unit the player sees a hundred of at once, so its readability rules
are the ones that matter most at horde scale — and they are exactly the rules nobody has
checked. Same shape as task-009; both are cheap and both are pure verification.

## Done when
- Every item at `docs/art/retinue-militia.md:426-436` ticked with its measurement, or
  marked failed with what was measured.
- The zero-Pale rule and the Bone 38–44% / Dark ≤30% bands come from a quantize report.
- The silhouette items (uneven shoulders, value splits, mismatched legs, club clear of
  the outline) are judged against the frames and stated plainly.
- Failures are written up, not fixed.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Run the acceptance checklist at docs/art/retinue-militia.md:426-436 against the packed
militia sheet. This is a verification pass — you are NOT regenerating art.

Read docs/art/retinue-militia.md in full, plus docs/art/aesthetic-direction.md
(Direction A LOCKED, strict 4-value Demichrome) and docs/data/art/palette.json. Note this
unit's rule is stricter than the global one: THREE values, zero Pale pixels, any Pale
pixel is a reject.

Use Scripts/art/pixelpipe.py quantize for on-palette %, value dominance, and alpha
strictness (see Scripts/art/README.md). Do NOT call any mcp__pixellab__* generation
tool — this task holds no pixellab-credits lock and generating costs real money.

Tick each of the eleven items with the measurement that justifies it, or mark it FAILED
with what you measured. The silhouette items are judgment calls — state what you saw.
Do not tick anything you did not verify.

Write ONLY docs/art/retinue-militia.md. Write findings for failures; do not fix them.
```
