---
id: 112
title: Build the armored-enemy variant family off the brighter-palette base and publish the contact sheet
status: done
agent: claude
model: sonnet
owns: ["docs/data/art/families/enemy-armored/**", "RawArt/Renders/enemy-armored/**", "docs/art/enemy-armored-variants.md"]
resources: ["pixellab-credits"]
depends-on: []
epic: ""
evidence: A published contact-sheet Artifact showing the base plus 8 armored-enemy variants, each next to its flat outline, with the measured aspect/solidity/asymmetry/hole band per variant and a keep/flag/reject verdict on every card; plus a committed docs/data/art/families/enemy-armored/family.json and a short findings doc naming which variants undershot.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: enemy-armored
decided: "2026-07-30 done"
---

## Why now

The only enemies with art are the nine amorphous brood forms in `T_Enemy_2bit`. There is
no armored, human-shaped enemy anywhere on disk, which is the archetype
`docs/art/npc-silhouette-brief.md` §(b) describes and the one the roster is missing. The
owner has picked a specific base for it — PixelLab character `afa5582e`, the "Make
brighter pallet" state — and wants the options for it laid out before anything gets packed
into an atlas. Nothing breaks while this stays undone; the enemy roster just stays
single-shape.

This is deliberately art-only. It stops at the owner's verdict on the sheet — no packing,
no UE import, no stat blocks, no atlas changes.

## Done when

- `docs/data/art/families/enemy-armored/family.json` validates against
  `docs/data/art/family.schema.json`, records `character_id: afa5582e-…` and the
  **returned** canvas, and names the axis and what is held constant.
- Every rotation of every variant is on disk under
  `RawArt/Renders/enemy-armored/raw/<variant>/rotations/*.png` — downloaded before any
  verdict, per the standing retention rule.
- `py Scripts/art/variantpipe.py judge enemy-armored` runs clean and every variant has a
  keep/flag/reject verdict backed by 8-direction measurements, not a south-frame guess.
- The contact sheet is published as an Artifact and the URL is handed back.
- `docs/art/enemy-armored-variants.md` states the family's aspect spread as a number, and
  names each variant that undershot its stated target or stopped reading as a soldier.

## Spawn prompt

```
You are executing task-112. Read docs/backlog/task-112-enemy-armored-variant-family.md
first, then follow .claude/skills/variants/SKILL.md — this task IS that skill's loop, run
once on a new family called `enemy-armored`. Also read .claude/skills/sprite/SKILL.md
§"Hard constraints" for the PixelLab mode rules it depends on.

THE BASE — already confirmed, do not re-litigate it:
  character_id  afa5582e-c649-49cc-96de-677e6f9869dd
  name/state    "A black ghost with sharp" / "Make brighter pallet"
  group         8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b (43 states)
  canvas        88x88 RETURNED (confirm with get_character before queueing anything)
  view          low top-down, 8 directions
Despite the character's name this state is an ARMORED HUMANOID in a bright grey/white
palette, not an ooze. It is a skeletal base.

THE AXIS — decided by the owner, 2026-07-30, do not substitute your own:
  create_character_state on this base ONLY. No `standard` mode, no `proportions` knobs.
  Always use_color_palette_from_reference=True. Always ban glow explicitly in the
  edit_description — aesthetic-direction.md reserves light for the flame.
  On a skeletal base the axis is narrow <-> wide-by-extension. SKILL.md records three
  measured failures trying to make a humanoid go squat. Do not ask for squat. Do not ask
  for tall-and-heavy. Narrow means arms tucked and prop dropped; wide means something
  rigid held OUT from the body, not an inflated torso.
  Aspect is spent first, then topology, per SKILL.md's order of axes. Never interior/kit.

THE BATCH — base + 8 generated variants:
  v0_base        source: legacy. Do NOT generate. Fetch afa5582e's own 8 rotations with
                 `variantpipe.py fetch enemy-armored v0_base --url ...` so the sheet has a
                 reference card for free.
  v1_pike        narrow. Arms tucked to the sides, weapon held vertical against the body,
                 nothing protruding. Target aspect 0.58.
  v2_column      narrower still, and simpler. No prop at all, limbs tucked, one clean
                 vertical column — but it MUST keep a visible head and legs. Target 0.50.
  v3_levelspear  wide by extension. A rigid weapon held level straight across the body.
                 Body mass unchanged. Target aspect 1.6.
  v4_standard    wide by extension, asymmetric. A long standard/pole held out to ONE side
                 only. The two sides must be clearly different, not mirrored.
                 Target: asymmetry above 0.4.
  v5_slabshield  wide by extension, the other asymmetry. A tall slab shield held out to
                 one side, sword arm tucked on the other. Target: asymmetry above 0.4 and
                 a solidity clearly above its siblings.
  v6_notched     topology. A deep notch splitting the body mass, head dropped below the
                 shoulder line so the top edge is uneven. Aspect stays near the base.
  v7_spined      topology. A jagged top edge with AIR GAPS between the spikes. Read
                 SKILL.md's note that a notch is not a hole: for a gap to count the
                 background must not be able to flow into it, and on an 88px canvas a hole
                 under ~15px closes at panel scale. Aim for gaps that are closed on every
                 side and generous.
  v8_tethered    topology, and the one most likely to produce a real hole. Something rigid
                 held AWAY from the body on an arm or haft, with clear space between it and
                 the torso on every side — SKILL.md's detached-drone precedent. Target: at
                 least one enclosed hole surviving all 8 rotations.

SIZE TRAP: the returned canvas comes back ~40% larger than the requested `size`. This
family must return 88px to sit next to the brood. Check the canvas of the FIRST state that
comes back before you queue the other seven. If it is not 88, stop and report it.

COST: create_character_state at 88px is ~20-40 generations each, so 8 variants is ~160-320.
Balance at drafting time was 6725 of 10000 remaining, so this fits — but pixellab-credits
is real money and this task holds the lock. State the running total in your handback. If a
variant comes back unusable, report it rather than silently rerolling it more than once.

CANON WARNINGS — these are live traps, verify before you rely on any of them:
- docs/art/npc-silhouette-brief.md is the only doc describing an armored enemy (§(b) "Still
  Legion soldier": clean symmetrical outline, dark-dominant mass). Its SILHOUETTE mechanism
  is still useful direction. Its 4-value Demichrome palette table is SUPERSEDED — the game
  ships in full colour as of 2026-07-28 (docs/art/aesthetic-direction.md, Quantize 0). Do
  not quantize anything and do not enforce a 4-value lock.
- The enemy faction has NO settled name (task-033 is still open on it). The family is called
  `enemy-armored`. Do not name it Legion, Quiet, or Unwitnessed anywhere.
- WORLD.md is superseded by docs/narrative/FLAME-FOUNDATION.md (2026-07-22 reset).
- The game is called Kindled, not Emberkeep, wherever you write prose.

JUDGING — SKILL.md §Judging governs. Specifically:
- Measure with `py Scripts/art/silhouette_report.py ... --all-directions`. A single frame
  will lie to you; SKILL.md's own worked example is a variant that measured 0.86 from the
  south and 2.60 from the east and nearly got rerolled for free.
- Identical opaque counts between two variants is an automatic fail — that is the exact
  failure this whole skill exists to prevent, and both the knight and brood groups hit it.
- Report the aspect spread as a NUMBER. Do not write "they look varied". A humanoid base is
  expected to spread less than the brood's 3.1x; if it lands under 1.3x say so plainly,
  because that is the finding, not a thing to soften.
- A variant that gained distinctiveness by ceasing to read as a soldier is a REJECT however
  good its numbers. If you cannot name the unit from its outline alone, it went too far.

YOU OWN, and may write, only:
  docs/data/art/families/enemy-armored/**
  RawArt/Renders/enemy-armored/**
  docs/art/enemy-armored-variants.md

DO NOT TOUCH: docs/data/art/requests/*.json (especially enemy-units.json and
swarm-units.json), any T_*.uasset, any existing family under docs/data/art/families/,
Scripts/art/*.py, GDD.md, SYSTEMS.md, docs/data/art/brood-variants.json, or anything in
ELVTR/Source/. Do not pack a sheet, do not quantize, do not import into Unreal, do not open
the editor — /variants ends at the owner's verdict and /sprite owns everything after it.
Do not delete any generation from RawArt/Renders/, including ones you judge as rejects.

HAND BACK: the published Artifact URL for the contact sheet, the family's measured aspect
spread as a number, a one-line per-variant verdict (keep/flag/reject + why), the generation
count spent, and any variant you think should be rerolled with a different brief. Name the
failures explicitly — a variant that missed its target is a finding, not something to omit.
```
