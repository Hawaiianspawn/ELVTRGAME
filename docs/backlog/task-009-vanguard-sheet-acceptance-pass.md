---
id: 009
title: Run the Vanguard sprite acceptance checklist against the packed sheet
status: done
agent: pixel-art-director
owns: ["docs/art/vanguard.md"]
resources: []
depends-on: []
evidence: All nine checklist items at docs/art/vanguard.md:351-360 ticked or failed, each backed by a pixelpipe quantize report or a stated pixel measurement.
score: {feel: 1, risk: 1, cost: 1}
source: docs/art/vanguard.md:351
decided: "2026-07-30 done"
model: sonnet
teammate: vanguard-accept
---

## Why now
`docs/art/vanguard.md` ends with a nine-item acceptance checklist — four values only,
Pale confined to one contiguous rectangle, Steel dominant, shoulder line flat for ≥18px,
no dither block under 2×2, content bbox ≤48×48 — and none of it has been run against the
sheet that exists. An unverified acceptance gate is the same as no gate.

These are mechanical, measurable checks. `Scripts/art/pixelpipe.py quantize` already
reports on-palette percentage, value dominance, and alpha strictness, so most of the
list is a script run plus a look.

## Done when
- Every item at `docs/art/vanguard.md:351-360` is ticked with its measurement, or marked
  failed with what was measured instead.
- Palette compliance and value dominance come from a `pixelpipe.py quantize` report, not
  from eyeballing.
- Failures produce a written finding, not a fix — regeneration is a separate task, and
  regenerating costs PixelLab credits.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Run the acceptance checklist at docs/art/vanguard.md:351-360 against the packed Vanguard
sheet and record the result. This is a verification pass — you are NOT regenerating art.

Read docs/art/vanguard.md in full, plus docs/art/aesthetic-direction.md (Direction A is
LOCKED: strict 4-value Demichrome) and docs/data/art/palette.json.

Use Scripts/art/pixelpipe.py for anything measurable — quantize reports give you
on-palette %, value dominance, and alpha strictness. Read Scripts/art/README.md for
usage. Do NOT call any mcp__pixellab__* generation tool: this task holds no
pixellab-credits lock and generating costs real money.

Tick each of the nine items in docs/art/vanguard.md with the measurement that justifies
it, or mark it FAILED with what you actually measured. Do not tick anything you did not
verify.

Write ONLY docs/art/vanguard.md. If items fail, write the finding — do not fix it.
Regeneration is a separate task the owner approves.
```
