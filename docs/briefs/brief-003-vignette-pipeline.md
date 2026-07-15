```yaml
id: 003
title: Chronicle-plate vignette pipeline (all decision events)
status: done
from: user (via aesthetic-direction §4 decisions, 2026-07-10)
priority: high
faction: all (per-event)
biome: all (plates re-quantize to the local biome ramp)
class-ties: none (E5/E6/E7 are class-flavored; the pipeline must serve all four)
spec: ../art/vignette-pipeline.md
```

## Subject

Not a single asset — the **system spec** for decision-event vignette art, which every
future plate is drawn against. Per the locked aesthetic direction
(`../art/aesthetic-direction.md` §4, decisions 3/4/5): **all** decision events get
framed-panel vignette art — large static compositions in the scribes' register
("chronicle plates"), ink energy quantized to the local biome's 4-value ramp with
coarse dither standing in for hatching. This spec is consumed by everything
downstream: it defines the panel anatomy, the effort tiers, the quantize pipeline,
and the templates the v1 event set (WORLD.md §8, E1–E8) will be drawn into.

## Mood

An entry being written into the kingdom's chronicle as you choose. The plate is the
record witnessing the party's decision — solemn, inked, permanent. Per-event mood
varies (E1 tragic, E3 vast/temptation, E7 intimate dread), but the register itself
is always: *this is being written down.*

## Narrative excerpt

> The Undervault keeps a two-register record: the stitchers' tapestry and the
> scribes' chronicle. To be stitched into the record is to be witnessed.
> Red is rubrication — the chronicle's red ink, reserved for cost, temptation,
> and violation. (canon, adopted 2026-07-10)

## Readability needs

- **Framed-panel anatomy** (per the locked decision): bordered scene above the text
  box, HUD still visible — the plate must never hide the run. Define the panel's
  screen footprint, border treatment (chronicle/manuscript frame, distinct from the
  stitched tapestry grammar), and how 2–3 choice options sit beneath it.
- **Effort tiers, not coverage tiers**: E3/E8 are the tentpoles (bespoke
  compositions); E2 is the constant in-combat texture (cheapest treatment — may need
  a reduced in-combat variant that doesn't stop the fight); E1/E4/E5/E6 mid-tier from
  a **remixable plate library** (same composition re-quantized to a different biome
  ramp / re-cropped must read as a different place). Remixed plates must not feel
  cheap next to bespoke ones — define what varies and what may not.
- **Rubrication**: E7 (Dark Bargain) and any pay-a-cost option carry the reserved
  red as the chronicle's warning ink — frame accents / option text, unmistakable at
  a glance, never decoration. E7 is *private to one player*: the plate composition
  should read as intimate (a whisper, not a proclamation).
- **Horror spikes live here**: this register is the only render-capable home for the
  Unwitnessed level-3 moments (aesthetic-direction decision 3). Define how a plate
  escalates (E3 titan-heart at minimum) while the expression ceiling
  (calm/vacant, never agony; ambiguous faces, never a confirmed victim) holds.
- **Pipeline steps for a solo dev**: ink (digital or scanned) → quantize to the
  local 4-value ramp → coarse dither pass; specify target canvas, dither coarseness
  floor (static art may go finer than the 2×2 sprite rule — say how far), and
  file/import conventions consistent with `ELVTR/SETUP-EDITOR.md`.

## Source

`../art/aesthetic-direction.md` (§3 Direction B locked bundle, §4 decision log) ·
`WORLD.md` §8 (E1–E8 event templates, design rules) · board anchors:
`Artboard/Landscape shots/4551f4b066eb42c11cebfac4702bd794.jpg`,
`Artboard/Monsters Aesthetic/1853566f8eebdeeb95f366dc132fe5fa.jpg`,
`Artboard/Portrait Styles/e879a99e186564da0e977b89229b478d.jpg` (panel anatomy),
`Artboard/Grounding in the world/05aaaecfed1b446dfc43fc576fab1aa3.jpg` (frame/type register).
