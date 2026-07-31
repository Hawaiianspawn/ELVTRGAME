---
id: 125
title: Generate a medieval archer variant family off the shipped knight base, judged on measured silhouette
status: done
agent: claude
model: sonnet
owns: ["RawArt/Renders/archer-medieval/**", "docs/data/art/families/archer-medieval/**", "docs/art/archer-medieval.md"]
resources: ["pixellab-credits"]
depends-on: []
epic: archers-on-the-field
evidence: A published contact sheet plus `variantpipe.py judge --json` output showing at least 4 keeps that separate on measured silhouette, every keep with all 8 rotations on disk at the knight base's canvas and content bbox no larger than 56x56 in any facing.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: archer-family
decided: "2026-07-31 done"
---

## Why now
Archers are already a live unit type — they spawn at 20% of every recruit, hold their own
formation, and fight from their own 750uu engage band — but the renderer has no unit-type
branch, so every archer on the battlefield draws from the eleven-look team atlas and is
pixel-identical to a spearman. There is no medieval archer art anywhere in the repo to fix
that with: `archer-scifi` is a sci-fi register that would clash with the shipped knight line,
`archer-proxy` has no 8-direction rotations on disk, and `ranged-roster` was measured and
ruled out by `task-085` for being more than double the atlas cell. The owner chose to
generate rather than reuse (2026-07-31). This task produces the art; `task-126` lands it.

## Done when
- `docs/data/art/families/archer-medieval/family.json` exists, validating against
  `docs/data/art/family.schema.json`, declaring the knight base and the chosen axis.
- At least **4 variants carry a `keep` verdict** in `manifest.json`, judged by
  `variantpipe.py judge` on measured silhouette — never by eye, and never from the south
  frame alone.
- Every keep has all 8 rotations downloaded to
  `RawArt/Renders/archer-medieval/raw/<variant>/rotations/*.png`, none deleted.
- **Every keep's content bbox is 56x56 or smaller in every one of its 8 facings**, measured
  and reported per variant. This is the hard gate: `task-085` ruled `ranged-roster` out for
  exactly this reason, and a family that misses it cannot be packed at all.
- Every keep reads as a **ranged** unit — a drawn bow, a nocked arrow, a loosing pose or a
  quiver visible in the silhouette, not just a knight holding something.
- A contact sheet is published as an Artifact and its URL is in the handback.
- `docs/art/archer-medieval.md` records the axis, the per-variant measurements, the keeps and
  the culls with reasons.

## Spawn prompt

```
You are executing task-125. Read docs/backlog/task-125-medieval-archer-variant-family.md
first, then .claude/skills/variants/SKILL.md in full, then the "Hard constraints" section of
.claude/skills/sprite/SKILL.md.

GOAL
Generate a family of medieval archer looks for Kindled's player army, judged apart by
measured silhouette, ready to be packed into the team sprite atlas by a follow-up task.
Target at least 4 keeps. Budget about 6 generations so there is room to cull.

THE BASE — do not pick your own
Build these as PixelLab character STATES on the already-shipped knight base:

  character_id  1c935515-0ea3-459b-8c63-e7f8cf368272
  canvas        88x88
  group         2cc3ab61-4230-4786-98fa-b94da9e99218

Confirm it live with mcp__pixellab__get_character before you queue anything. This is the
same base every shipped knight look came from (docs/data/art/families/knight-melee-v1/
family.json, knight-melee-v2/family.json). Using it is what makes the archers inherit the
right palette, lighting, canvas and register, so they sit beside the ten knights already in
the atlas instead of looking like a different game. Do NOT call create_character, and do NOT
use the proportions knobs — create_character_state has no size parameter and inherits the
source canvas, which is exactly what this family needs.

The base carries a shield and a polearm as its established silhouette. That is a large
dominant prop, and per the variants SKILL.md's Finding 4 it will anchor mass in every state
unless a prompt explicitly drops or repositions it. Every archer state must drop it — an
archer holding a shield and polearm is not an archer.

REGISTER — this is a medieval fantasy game, not a sci-fi one
The setting is docs/narrative/FLAME-FOUNDATION.md: you carry the only flame in a pitch-dark
world and your army follows your light. Bows, crossbows, longbows, slings, quivers, hoods,
leather. No guns, no energy weapons, no drones, no visors, no glow of any kind. Ban glow
explicitly in every prompt, the way knight-melee-v1 and v2 both did.

Full colour throughout, per docs/art/aesthetic-direction.md's 2026-07-28 amendment. Do not
quantize, do not ask for fewer colours, and do not apply the demichrome-4 palette — the
four-value colour gate is superseded and this family is not one of the assets that named it.

THE AXIS
Spend variance in the order the SKILL.md proves: aspect first, then topology, and interior
or kit last and only as a tiebreaker. Note that the base is a skeletal humanoid, which caps
the aspect axis hard — knight-melee-v1 measured only a 1.53x spread and it split into two
clusters rather than five shapes. Expect to lean on topology: a bow drawn across the body,
an arm raised to loose, a quiver held away from the torso to open a real hole. A hole must
be closed on every side to count, and must be big enough to survive downsampling — hold an
object away from the body rather than relying on an armpit.

THE SIZE GATE — the one that kills a family
Every keep must have a content bounding box of 56x56 or smaller in EVERY ONE of its 8
facings. The team atlas cell is 56px (docs/data/art/requests/team-units.json, output.cell)
and pixelpipe correctly refuses to scale pixel art. task-085 ruled the whole ranged-roster
family out for exactly this: 132px sources with content up to 124px tall. Measure this
yourself per rotation and report it per variant. A variant that busts the cell in one facing
is a cull, not a keep, however good it looks.

THE PIPELINE
Write docs/data/art/families/archer-medieval/family.json against
docs/data/art/family.schema.json — base, named axis, what is held constant, and a slug,
silhouette_target and edit_description per variant. Then:

  py Scripts/art/variantpipe.py plan   archer-medieval
  py Scripts/art/variantpipe.py fetch  archer-medieval <variant> --url URL [--url URL ...]
  py Scripts/art/variantpipe.py judge  archer-medieval --json
  py Scripts/art/variantpipe.py report archer-medieval

Download all 8 rotations for every variant BEFORE judging anything. This is a standing
project rule and it is not optional: judge checks numeric targets against the whole
8-direction band, and a verdict from the south frame alone is wrong often enough that the
skill calls it out by name. Nothing generated is ever deleted, including culls — keep them
on disk with their reason recorded.

Publish the contact sheet from `report` as an Artifact and put the URL in your handback.

WHAT YOU MUST NOT TOUCH
Only these paths:
  RawArt/Renders/archer-medieval/**
  docs/data/art/families/archer-medieval/**
  docs/art/archer-medieval.md

Do NOT touch any of the following — a separate task (task-126) owns every one of them and
will overwrite you or be overwritten:
  RawArt/Sheets/T_Team_2bit.png
  docs/data/art/requests/team-units.json
  docs/data/art/team-variants.json
  docs/data/art/provenance.json
  anything under ELVTR/Source/ or ELVTR/Content/
Do not pack an atlas, do not import anything into Unreal, do not open the editor, and do not
edit any C++.

CREDITS
pixellab-credits is real money and this task holds that lock. Budget about 6 generations.
Do not reroll a variant more than once. If the axis runs out — a state comes back visually
identical to a sibling, which is how the original knight group failed — stop and say so in
the handback rather than spending more.

HAND BACK
- The contact sheet Artifact URL.
- The per-variant measurement table: aspect, solidity, asymmetry, holes, and the max content
  bbox across all 8 rotations.
- Which variants are keeps, which are culls, and why — from the measurements, not from
  looking.
- Whether the size gate passed for every keep, stated explicitly.
- How many generations you actually spent.
```
