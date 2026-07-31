---
id: 121
title: Repack the militia hit frame from the eight already-generated frames the sheet never used
status: proposed
agent: claude
model: ""
owns: ["docs/data/art/requests/unit-retinue.json", "RawArt/Sheets/T_Unit_Retinue.png", "docs/art/retinue-militia.md"]
resources: []
depends-on: []
epic: ""
evidence: >
  A repacked `RawArt/Sheets/T_Unit_Retinue.png` whose hit cell passes the five
  acceptance items it currently fails — dark/steel/bone within 30/28-34/38-44, plus
  head value-split, torso value-split, mismatched legs and Dark eye recesses — shown
  as a before/after `pixelpipe.py quantize` histogram per cell, with the other three
  cells unchanged and still passing all eleven. No PixelLab call, no credits spent.
score: {feel: 2, risk: 1, cost: 1}
source: task-010 handback
teammate: ""
decided: ""
---

## Why now

`task-010` measured the packed militia sheet against its own acceptance checklist:
six items pass sheet-wide, and **five fail in the hit cell alone**.

| cell | dark/steel/bone | verdict |
|---|---|---|
| idle | 28/32/40 | on spec |
| walk | 29/31/40 | on spec |
| attack | 29/30/41 | on spec |
| **hit** | **27/6/67** | Bone 23 points over the 44% ceiling, Steel collapsed from its 28–34% band to 6% |

Losing the Steel took the silhouette rules down with it: the head is one
undifferentiated Bone mass with the skullcap detached as a floating fragment, the
torso split is gone, both legs render Bone, and there are zero Dark pixels in the
head so the eye recesses are absent.

The hit frame is the one `docs/art/retinue-militia.md` says has to read *hardest* as a
shape change — it is the only feedback the player gets that a unit in a hundred-unit
mass took damage. It is the worst cell to have flattened.

**This is cheap and spends nothing.** `unit-retinue.json`'s `frame_map` packs only 3
of the 11 hit/attack frames already sitting under `RawArt/Renders/unit-retinue/r1/`.
Eight generated frames have never been looked at. Trying them costs no PixelLab
credits; only a fresh v3 generation would.

## Done when

- The hit cell passes items 3, 5, 6, 8 and 9 of the checklist at
  `docs/art/retinue-militia.md:426-436`, each backed by a `pixelpipe.py quantize`
  report or a stated pixel measurement.
- The other three cells are byte-identical or re-measured and still passing all eleven.
- `unit-retinue.json`'s `frame_map` records which frame index was chosen.
- **If none of the eight unused frames clears the bar, that is the answer.** Report
  which came closest with its histogram and stop — do not generate a replacement, and
  do not relax the checklist's bands to make a frame fit.

## Spawn prompt

```
You are executing task-121. You are standing in for the pixel-art-director on Kindled
(C:\Projects\ELVTRGAME) — that agent has no shell and this is a measurement and
repacking pass, so it routes to you. Apply the art director's standards; run the tools
yourself.

Read docs/backlog/task-121-repack-militia-hit-frame.md, then
docs/art/retinue-militia.md in full (especially the acceptance checklist around lines
426-436 and task-010's recorded measurements), docs/data/art/requests/unit-retinue.json,
and Scripts/art/README.md.

THE PROBLEM, measured by task-010: the packed sheet's hit cell fails five of the
eleven acceptance items while idle/walk/attack all pass cleanly. Its histogram is
dark/steel/bone 27/6/67 against a spec of roughly 29/31/40 — Steel has collapsed to
6%, and with it the head value-split, the torso value-split, the leg mismatch and the
Dark eye recesses. The skullcap has detached into a floating fragment.

THE CHEAP FIX: unit-retinue.json's frame_map packs only 3 of the 11 hit/attack frames
already generated under RawArt/Renders/unit-retinue/r1/. Eight frames have never been
evaluated. Measure them, pick one that clears the bar, repack the sheet.

DO NOT CALL ANY mcp__pixellab__* GENERATION TOOL. This task holds no pixellab-credits
lock and generating costs real money. Everything you need is already on disk. If none
of the eight frames clears the bar, say so and stop — that is a legitimate outcome and
it is the owner's call what happens next.

DO NOT relax the checklist's bands to make a frame fit, and do not re-judge this asset
against full colour. demichrome-4 is no longer the game-wide default, but
unit-retinue.json's own `canon` block names it explicitly with value_dominance "bone"
and pale_usage "none", so this asset is still fully held to it — three values, zero
Pale pixels, any Pale pixel is a reject.

Preserve the other three cells. Re-measure them after repacking and confirm they still
pass all eleven items.

YOU OWN ONLY: docs/data/art/requests/unit-retinue.json, RawArt/Sheets/T_Unit_Retinue.png,
docs/art/retinue-militia.md

DO NOT TOUCH: any .uasset (the UE import is a separate step and another session may
hold the editor), RawArt/Renders/** (read only — never delete a generation, per the
retention rule), any other file under docs/data/art/, or anything under ELVTR/.

HAND BACK: a before/after pixelpipe quantize histogram for all four cells, which frame
index you chose and why, the five previously-failing items with their new
measurements, and confirmation that the other three cells still pass.
```
