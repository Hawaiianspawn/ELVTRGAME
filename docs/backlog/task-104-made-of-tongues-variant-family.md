---
id: 104
title: Spend a real silhouette axis on Made of tongues, the best surface in the legacy brood batch
status: done
agent: claude
model: sonnet
owns:
  - "docs/data/art/families/brood-tongues/**"
  - "RawArt/Renders/brood-tongues/**"
  - "docs/data/art/provenance.json"
resources: ["pixellab-credits"]
depends-on: []
epic: ""
evidence: A published contact-sheet Artifact for the `brood-tongues` family — six new variants rendered next to their flat outlines with aspect/solidity/asymmetry/hole counts, all eight rotations checked — plus a written statement per variant of which of the 13 existing `brood-ooze` variants it sits closest to and whether it clears that neighbour's rotation band. `docs/data/art/families/brood-tongues/family.json` records the axis and per-variant targets; every generation is recorded in `docs/data/art/provenance.json` and every rotation is retained under `RawArt/Renders/brood-tongues/raw/`.
score: {feel: 2, risk: 2, cost: 1}
source: user
teammate: brood-tongues
decided: "2026-07-30 done"
---

## Why now

The brood directory holds two rounds that failed and succeeded for one measurable reason.
The eleven legacy auto-named states — `Made_of_tongues` among them — varied **surface**:
marble, pale blue skin, hairy overgrown, tongues. All eleven land inside aspect 0.86–0.91,
and three of them share a byte-identical 1,021-pixel outline. The later numbered
`state00`–`state08` batch varied **shape** and spreads 0.50–1.56, a 3.1× range. That is
`variants` SKILL.md's own rule — *vary the silhouette, not the texture; never open with
interior detail* — with the receipts sitting in the same folder.

`Made_of_tongues` has the most distinctive surface of the failed round (luma 0.198 and 84
colours against the numbered batch's ~0.07 and ~15) and the least distinctive shape. Nothing
breaks while it stays undone; this is buying six enemy forms that keep that surface and
finally spend the axis the legacy round skipped.

## Done when

Six new variants exist as PixelLab states off `Made_of_tongues`, measured across all eight
rotations, culled, and published as a contact sheet the owner can render a verdict from.
The bar is not "six new blobs": the family must show a real aspect spread **or** separate on
asymmetry and hole count where aspect crowds, and each variant must be checked against the
thirteen already-shipped `brood-ooze` variants — a new form that lands inside an existing
variant's rotation band is a finding to report, not something to quietly ship.

The tongue surface is the constant. A variant that reaches its silhouette target by losing
the tongues has failed, however good its numbers.

Explicitly **out of scope**: no atlas re-cut. Wiring the keepers into `T_Enemy_2bit` is a
follow-up filed after the owner picks.

## Spawn prompt

```
You are executing task-104. Read docs/backlog/task-104-made-of-tongues-variant-family.md
first, then .claude/skills/variants/SKILL.md in full, then .claude/skills/sprite/SKILL.md
§"Hard constraints".

GOAL
Generate SIX new enemy variants as PixelLab states off the existing brood character
"Made of tongues", measure them, and publish a contact sheet for the owner's verdict.
Six. Not more — the point is a verdict, not a roster.

THE BASE
  character id  4daf02e5-178c-454e-b0a6-7b48050ad7a1   ("Made of tongues")
  group_id      8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b
  88x88 canvas, 8 directions, low top-down view, template mannequin
  rotations on disk: RawArt/Renders/brood-ooze/raw/Made_of_tongues/rotations/
  measured south frame: 38x44, aspect 0.86, solidity 0.62, asym 0.02, luma 0.198, 84 colours

Confirm it with mcp__pixellab__get_character before generating anything. Generate with
create_character_state, use_color_palette_from_reference=True. Do NOT use standard mode
with custom `proportions` — the content is amorphous, and SKILL.md's measured finding is
that for amorphous bases states beat proportion knobs outright: they deliver the full range
AND inherit palette, lighting and canvas. `standard` would cost you the palette and the
surface for nothing.

THE AXIS — read this before writing a single edit_description
The eleven legacy brood states (Made_of_tongues is one) varied SURFACE and all eleven landed
inside aspect 0.86-0.91; three share an identical 1,021px outline. The later numbered
state00-state08 batch varied SHAPE and spreads 0.50-1.56. Do not repeat the first mistake.

  HOLD CONSTANT: the tongue surface, the palette, the canvas, the general "mass of tongues"
                 read. A variant that hits its shape target by losing the tongues is a
                 REJECT however good the numbers.
  SPEND:         aspect first, then topology (asymmetry, enclosed holes), interior never.

Run `py Scripts/art/variantpipe.py judge brood-ooze` first and read the full table — those
13 variants are the band you have to clear, not just each other. Suggested six targets, but
name the axis yourself in family.json and justify what you chose:

  1. a tall narrow column of tongues            target aspect ~0.50
  2. a low wide spill, wider than tall           target aspect ~1.6
  3. a very long low form, 2.5x wider than tall  target aspect ~2.5
  4. a form with a clear front and back, not mirrored   target asymmetry >0.5
  5. a form split by a notch closed on every side, so it has real enclosed holes
  6. one squat dense mass — high solidity, unbroken outline

On #5: a notch is NOT a hole. silhouette_report.py counts enclosed background regions, and
only a gap the background cannot flow into changes topology. Measured: a wide straddle and
a held-out shield produced ZERO holes because both gaps opened to the sprite's edge. And a
hole must survive downsampling — a 10px gap closes to ~2px at panel scale and disappears;
~19px is the floor. The reliable trick is an element held AWAY from the body.

On #3: SKILL.md's worked example is this family's own neighbour, state05_ridge. It measured
aspect 0.86 from the south and was nearly written off; across its rotations it runs 0.59 to
2.60 and was the best variant in the batch, because south is the view down its length. Never
verdict a directional form from the south frame.

Ban glow explicitly in every edit_description — docs/art/aesthetic-direction.md reserves
light for the flame. Full colour, unquantized: the 4-value colour gate was SUPERSEDED
2026-07-28 (Quantize 0).

THE PIPELINE — use the committed tooling, do not hand-roll it
1. Write docs/data/art/families/brood-tongues/family.json against
   docs/data/art/family.schema.json — base, named axis, what is held constant, and per
   variant slug / silhouette_target / edit_description.
2. `py Scripts/art/variantpipe.py plan brood-tongues` emits the exact
   mcp__pixellab__create_character_state calls. Run them.
3. `py Scripts/art/variantpipe.py fetch brood-tongues <variant> --url ...` for EVERY
   rotation of every variant, BEFORE judging anything. Standing project rule: everything
   generated lands under RawArt/Renders/ first and is never deleted before a keep/reject
   decision is recorded.
4. `py Scripts/art/variantpipe.py judge brood-tongues`
5. `py Scripts/art/silhouette_report.py RawArt/Renders/brood-tongues/raw --all-directions`
   — all eight facings, on every variant, not just the suspicious ones.
6. CROSS-FAMILY CHECK. For each of the six, name which of the 13 brood-ooze variants it
   sits closest to and whether it clears that neighbour's measured rotation band. If two of
   your own six collide with each other, say so.
7. `py Scripts/art/variantpipe.py report brood-tongues`, then publish the HTML with the
   Artifact tool (favicon 👅, one-sentence description). Project convention: big work is
   handed over as on-screen evidence, never a written "it works".
8. Record every generation in docs/data/art/provenance.json in the shape the existing brood
   entries use, including the generations spent.

YOU OWN, AND MAY WRITE, ONLY
  docs/data/art/families/brood-tongues/**
  RawArt/Renders/brood-tongues/**
  docs/data/art/provenance.json

DO NOT TOUCH — another task does the wiring
  docs/data/art/requests/enemy-units.json, docs/data/art/brood-variants.json
  ELVTR/Source/ELVTR/Mass/SwarmFragments.h
  ELVTR/Content/**  (no atlas repack, no NS_Swarm, no T_Enemy_2bit)
  anything under RawArt/Renders/brood-ooze/ or docs/data/art/families/brood-ooze/ —
    read it all you like, write none of it
Do not launch the Unreal editor. Do not run the sprite packing or import step.

CANON WARNINGS — the repo will lie to you about these
- The game is named KINDLED, not Emberkeep. ~97 files still say Emberkeep, including
  generated report headers. Do not propagate it; do not "fix" it either — that is task-092.
- WORLD.md is SUPERSEDED (narrative reset 2026-07-22). Canon is
  docs/narrative/FLAME-FOUNDATION.md.
- The 4-value colour gate is SUPERSEDED (2026-07-28). Full colour. Any demichrome-4
  instruction you find binds only assets whose own request names it.
- The enemy faction has NO canon name; enemy-units.json calls the brood source an explicit
  PLACEHOLDER. Do not invent lore or rename anything.
- docs/perf/niagara-sprite-refactor.md §2/§8.1 carry a retracted GPU-sim claim. Irrelevant
  here — do not cite it.

HAND BACK
A short report: which variants hit their stated silhouette_target, which undershot, the
closest existing brood neighbour for each, which you would keep and which you would reroll,
and the Artifact URL. Name the failures — a variant that missed is a finding, not something
to omit. Do not change any task status; the lead session closes the task.
```
