---
id: 100
title: Retire pixelpipe validate's stale 48px cell rule, which rejects every shipped retinue request file
status: proposed
agent: claude
model: ""
owns: ["Scripts/art/pixelpipe.py"]
resources: []
depends-on: []
epic: ""
evidence: A clean run of pixelpipe.py validate passing on swarm-units.json, team-units.json and enemy-units.json without the cell-size rejection, plus a stated check that the rule's removal does not let through a genuinely wrong cell size — the packer's real constraint restated in the code where the folklore one used to be.
score: {feel: 1, risk: 1, cost: 1}
source: lead
teammate: ""
decided: ""
---

## What surfaced
`task-085` reported that `pixelpipe.py validate` fails all three request files on:

    output.cell is 56 -- locked to 48 for kind 'retinue'

and confirmed the same failure **already existed on the original `swarm-units.json`** before the
atlas split, so this is not fallout from that task. The rule predates the 2026-07-28 cell-size
change and never got updated. `pack` and `report` both work fine and are what actually shipped
both atlases — so the validator is the only thing that thinks 56 is wrong.

## Why it is worth an hour
A validator that always fails is a validator nobody reads, and this one sits on the art path that
already has three-things-must-agree failure modes elsewhere. `task-085` also hit two sibling cases
in the same file on 2026-07-29 — a non-power-of-two `rows` rejection (folklore: `SubImageSize` is
a float ratio) and a schema cap of 16 grid items — both fixed then. This is the third of the same
class, left behind because nothing blocked on it.

Check whether the rule should be **deleted or corrected** rather than assuming deletion: if the
packer has a genuine cell constraint, state that one instead of removing the check entirely.

## Scope fence
- Only `Scripts/art/pixelpipe.py`. Not the schema, not the request files, not any atlas.
- No regeneration, no PixelLab credits, no editor.
