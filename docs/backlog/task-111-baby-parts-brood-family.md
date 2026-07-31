---
id: 111
title: Spend the protrusion axis on the Baby face brood — limbs out, not faces on
status: proposed
agent: claude
model: ""
owns:
  - "docs/data/art/families/brood-baby/**"
  - "RawArt/Renders/brood-baby/**"
  - "docs/data/art/provenance.json"
resources: ["pixellab-credits"]
depends-on: [104]
epic: ""
evidence: A published contact-sheet Artifact for the `brood-baby` family — six new variants rendered next to their flat outlines with aspect/solidity/asymmetry/hole counts, all eight rotations measured — plus a written statement per variant naming which of the 13 existing brood variants it sits closest to and whether it clears that neighbour's rotation band, pass or fail. `docs/data/art/families/brood-baby/family.json` records the axis and per-variant targets against `docs/data/art/family.schema.json`; every generation is recorded in `docs/data/art/provenance.json` and every rotation is retained under `RawArt/Renders/brood-baby/raw/`.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: ""
decided: ""
---

## Why now

The owner asked for the next enemy: the `Baby face` brood look, with "more baby parts on
the monster". Measured tonight, that base is the legacy round's failure profile exactly —
`Baby_face_smiling_li` is 38×44, **aspect 0.86, solidity 0.61, asymmetry 0.00**, 62 colours.
A distinctive *surface* on a generic *shape*, the same shape as `Made_of_tongues` (0.86 /
0.62 / 0.02) and the same crowded 0.86–0.91 band that put three legacy states on a
byte-identical 1,021-pixel outline.

"More baby parts" splits two ways and only one of them survives 40px on a dark panel. As
**surface** — faces studded across the skin — the outline never changes and the sprite's
luma 0.149 means the interior never reaches the player: that is the round that already
failed, in this same directory. As **protrusions** — baby arms, legs and heads pushing out
of the mass — it is a real silhouette axis, and it is the one axis the brood has never
spent: of the 13 existing variants only 3 clear asymmetry 0.2 and only 3 have any enclosed
hole at all. Owner picked protrusions, 2026-07-30.

Nothing breaks while this stays undone. It buys six enemy forms that keep the baby surface
and finally vary the thing the player can actually see at horde scale.

## Done when

Six new variants exist as PixelLab states off `Baby face smiling li`, measured across all
eight rotations, culled, and published as a contact sheet the owner can render a verdict
from.

The bar is not "six new blobs". Because these are destined for the existing horde mix
(`Swarm.BroodVariantWeights`), the family must be checked **against the 13 already-shipped
brood variants**, not only against each other: name each new variant's nearest existing
neighbour and state pass/fail on whether it clears that neighbour's rotation band. A new
form that lands inside an existing variant's band is a finding to report, not something to
quietly ship.

The baby surface is the constant. A variant that reaches its silhouette target by losing
the baby flesh has failed, however good its numbers.

Explicitly **out of scope**: no atlas re-cut, no `T_Swarm_2bit` repack, no edit to
`docs/data/art/brood-variants.json` or the `Swarm.BroodVariantWeights` default. Wiring
keepers into the mix is one follow-up task filed after the owner picks — and it cannot
start before the pick anyway.

## Spawn prompt

```
You are executing task-111. Read docs/backlog/task-111-baby-parts-brood-family.md first,
then .claude/skills/variants/SKILL.md in full, then .claude/skills/sprite/SKILL.md
§"Hard constraints".

GOAL
Generate SIX new enemy variants as PixelLab states off the existing brood character
"Baby face smiling li", measure them, and publish a contact sheet for the owner's verdict.
Six. Not more — the point is a verdict, not a roster.

THE BASE
  character id  98875280-46c4-4a5d-93dc-a53b85b97a62   ("Baby face smiling li")
  group_id      8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b
  88x88 canvas, 8 directions, low top-down view, template mannequin
  rotations on disk: RawArt/Renders/brood-ooze/raw/Baby_face_smiling_li/rotations/
  measured south frame: 38x44, aspect 0.86, solidity 0.61, asym 0.00, luma 0.149, 62 colours

Confirm it with mcp__pixellab__get_character before generating anything. Generate with
create_character_state, use_color_palette_from_reference=True. Do NOT use create_character
standard mode with custom `proportions` — the content is amorphous, and SKILL.md's measured
finding (lines 110-114) is that for amorphous bases states beat proportion knobs outright:
they deliver the full range AND inherit palette, lighting and canvas. `standard` would cost
you the baby surface and the palette for nothing.

THE AXIS — read this before writing a single edit_description
PROTRUSIONS. Baby arms, legs and heads pushing OUT of the mass so the OUTLINE changes.
NOT baby faces studded across the skin. The owner was asked directly and picked protrusions
(2026-07-30) precisely because the interior does not survive 40px on a dark panel: this
sprite's mean luma is 0.149, so the outline is doing all the work.

This is the same trap task-104 was filed against. The eleven legacy brood states — the
baby base among them — varied SURFACE and all eleven landed inside aspect 0.86-0.91, with
three sharing an identical 1,021px outline. The later numbered state00-state08 batch varied
SHAPE and spreads 0.50-1.56. Do not repeat the first mistake.

Aspect is already well spent across the existing 13 (0.50-1.56, 3.1x). This family's job is
the axes that are NOT spent: asymmetry (only state08_slug 0.69, state00_base 0.29,
state07_twin 0.27 clear 0.2) and hole count (only 3 of 13 have any enclosed hole).

THE SIX VARIANTS
Every edit_description keeps the constant clause verbatim, and NO GLOW in any of them —
docs/art/aesthetic-direction.md reserves light for the flame.

  baby01_reach   target asym > 0.5
    Two long baby arms thrown out to one side, reaching well clear of the body, while the
    other side is pulled flat and smooth. A clear front and back that are not mirror
    images.
  baby02_crawl   target solidity < 0.50
    Four splayed baby limbs pushing out low and wide from a small body, leaving big open
    gaps between them — a gappy spidered outline, not a filled mass.
  baby03_crown   target >= 3 distinct bulges on the top edge
    Three or more small baby heads pushing up out of the top of the mass, so the upper
    edge reads as a notched row of heads rather than one smooth shoulder line.
  baby04_hoop    target one enclosed hole >= 19px, closed on every side
    Two baby arms curving out and meeting each other to close a complete ring, with a real
    gap fully surrounded by flesh on every side that does not open out to any edge.
  baby05_tail    target aspect >= 1.6
    Baby limbs trailing out lengthwise behind the body along the ground, the whole form
    much longer than it is tall — a dragged mass, not a standing one.
  baby06_bud     target solidity > 0.72, unbroken outline
    Baby limbs half-absorbed back into a single dense squat mass, only slightly wider than
    tall, with a smooth unbroken outline and no gaps or notches on the edge.

  constant clause (append to every edit_description):
    "Keep the same pale baby-flesh surface and inherited palette. No glow, no light source,
    no new colours."

BEFORE YOU SPEND ANYTHING
task-104's docs/data/art/families/brood-tongues/family.json records that get_character on
this shared group returned 37 states, not the 20 that brood-ooze/family.json tracks — 17
untracked states with shape-exploration names ("Keep this exact tall", "Keep this exact
tria...", "Split this creature", "Keep the body low an..."). Some of this axis may already
be paid for. List them and look before generating. Reading is free.

MEASURE AND JUDGE
  py Scripts/art/silhouette_report.py RawArt/Renders/brood-baby/raw --all-directions
--all-directions catches a variant whose shape drifts as it turns, which a south-only pass
cannot see. Then run the same script over RawArt/Renders/brood-ooze/raw to get the 13
existing variants' numbers, and for EACH new variant name its nearest existing neighbour
and state pass/fail on band clearance. That comparison is the evidence bar, not a bonus.

YOU OWN, AND MAY WRITE, ONLY:
  docs/data/art/families/brood-baby/**
  RawArt/Renders/brood-baby/**
  docs/data/art/provenance.json

DO NOT TOUCH:
  RawArt/Renders/brood-ooze/**        — read-only source; task-104 is live in that tree
  docs/data/art/families/brood-tongues/**  — task-104 owns it
  docs/data/art/brood-variants.json   — the atlas mix, follow-up task
  Content/Spike1/T_Swarm_2bit.uasset, any atlas, any repack, any Swarm CVar
  Any source file
The working tree is shared with other sessions. Never git stash, never revert someone
else's change, never restart the editor.

RETENTION
Every generation lands under RawArt/Renders/brood-baby/raw/ and stays there until the owner
makes a keep/reject call. Delete nothing. Rejects go to RawArt/Renders/brood-baby/rejected/
if you cull, still on disk.

COST
SKILL.md line 250 puts create_character_state at ~20-40 generations at this canvas, so six
is roughly 120-240 generations. Real money. If a variant misses its target, report the miss
rather than burning a third reroll on it.

HAND BACK
  1. The published contact-sheet Artifact URL.
  2. The measured table: six new variants, all eight rotations, aspect/solidity/asymmetry/
     holes/luma/colours.
  3. Per variant: nearest existing brood neighbour, and pass/fail on band clearance.
  4. Any variant that lost the baby surface to hit its target — called out as a failure.
  5. docs/data/art/families/brood-baby/family.json validating against
     docs/data/art/family.schema.json, and the provenance.json entries.
Do not repack the atlas. Do not edit brood-variants.json. Stop at the verdict.
```
