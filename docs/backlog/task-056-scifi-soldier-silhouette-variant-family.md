---
id: 056
title: Build a 6-variant silhouette family off the bright-palette sci-fi soldier and publish the contact sheet
status: parked
agent: claude
owns: ["RawArt/Renders/soldier-scifi-variants/**", "docs/art/soldier-scifi-variants.md"]
resources: ["pixellab-credits"]
depends-on: []
epic: ""
evidence: A published Artifact contact sheet showing 6 variants beside their flat outlines, with the silhouette_report.py table (aspect / solidity / asymmetry / holes), the measured aspect spread stated as a number, and a named keep-or-reroll verdict per variant.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: variants-rev2b-run
decided: "2026-07-29 parked"
model: sonnet
---

## Why now
The owner brought a specific base — PixelLab character `afa5582e-c649-49cc-96de-677e6f9869dd`
("Make brighter pallet", 88×88, 8 directions, mannequin template, group
`8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b`) — and asked for ~6 variants that differ in silhouette
**by proportion and topology**, presented as a page.

This base has no variant family yet. `knight-topology` and `knight-primitive` were measured off
the knight base (`1c935515`); `archer-scifi` off the archer. Nothing here is a re-run.

The reason this is worth a task rather than a quick batch: the base is a **skeletal humanoid**,
and `/variants` has measured that states off a skeleton collapse the proportion axis — 2.4×
width spread drops to 1.2×, and three separate attempts to make a humanoid go squat
(1.02 vs 1.30 target, 0.82 vs 1.00, 0.89 vs 1.30) all undershot. Topology works on a skeleton;
proportion fights it. Firing six states at this base and hoping is how the knight group ended up
with four identical 1,093-pixel outlines.

## Rev 2 — owner redirect, 2026-07-28 (supersedes "Done when" below)

Rev 1 shipped six states and **measured 1.7× aspect spread**, with the proportion three failing
outright — `prop-narrow` came back at 0.67 aspect, *wider* than `prop-mid` at 0.64. Owner's
verdict: *"We are too similar."*

**Root cause, and it closes the question permanently.** `create_character_state` is documented
as keeping "the source's identity, body type, and **proportions**". The `proportions` knobs
exist **only** in `create_character` standard mode — pro and v3 both ignore them. There is no
wording that reaches proportion through a state call. Rev 1's brief was not weak; the lever
does not exist on that call. States-only and proportion-variance are mutually exclusive in this
API, and that is now a settled fact rather than a hypothesis.

What rev 1 *did* prove: **topology works.** Asymmetry ran 0.07 → 1.06 (15× range) and width by
extension 29 → 50px. All of the separation the family has came from that axis. Keep it.

**Rev 2 path, chosen by the owner: anchor-first.** Edit six distinct south-facing anchors, then
`create_character` `mode="v3"` with `reference_image_url` to rotate each into 8 directions. It
is the only route giving *both* full shape freedom (pixels are being edited directly, so no
skeleton constrains the outline) *and* a locked style (v3+reference reproduces the exact sprite).
Shape reference supplied by the owner: **`bb92dd76-98b0-4327-ba62-8747709402ff`** ("Replace the
bow with"), 92×92, group `667961de-4062-4589-ac2d-c27157cc1cb2` — a hooded figure with a rifle
held level: hood-and-cloak outline, long horizontal weapon, clear front/back.

Because a skeleton no longer constrains anything, **the tall ↔ squat axis is back on the table**
— the "a humanoid will not go squat" rule applied to *states*, not to edited pixels. Target a
genuine ≥2.5× aspect spread this time.

Cost: ~6 edits + 6 × (2–9) v3 generations ≈ 20–60 total, well under rev 1's ~120–240.
Caveat carried from `/sprite`: **v3 preserves, it does not embellish** — the six edited anchors
are the hard quality ceiling for the whole family, and rear rotations degrade most, so judge
north / north-east / north-west first, never south.

---

## Done when (rev 1 — historical record)
Six `create_character_state` variants off `afa5582e` have been generated, downloaded, measured,
and published as a page the owner can judge.

**States only.** The owner ruled out the standard-mode proportion prototype explicitly — every
variant is a state on `afa5582e` with `use_color_palette_from_reference=True`, so all six
inherit the base's palette, lighting, 88px canvas and kit. This is the second time the
states-only path has been chosen; it is settled, not a starting position to argue from.

The six split across the two axes the owner asked for, within what a skeleton will actually
give:

- **3 proportion** — on a humanoid the reachable axis is *narrow ↔ wide-by-extension*, never
  tall ↔ squat. Narrow: limbs tucked, prop stowed, extended upward (the Pikeman reached 0.58
  aspect at 28px this way). Wide: something rigid held out level (a levelled spear got 53px,
  the widest knight produced all session). Plus the base's own mass as the midpoint.
- **3 topology** — change the *kind* of outline, not the ratio: an asymmetric front/back form,
  a genuine enclosed hole made by a prop held away from the body, and a notched or split
  outline.

Then: measure the family, check `--all-directions`, publish the contact sheet as an Artifact,
and report the aspect spread as a number alongside a per-variant verdict.

The bar for "this worked", from `/variants` §Judging:
- aspect spread ≥ 2.5× is good; **under 1.3× is a failure and must be reported as one**
- zero pairs sharing an identical opaque-pixel count (automatic fail)
- at least one variant above 0.4 asymmetry
- **every variant still readable as a soldier from its outline alone** — head and legs present.
  A variant that separates by ceasing to be a unit is rejected however good its aspect number.

## Spawn prompt

```
You are building a silhouette variant family for ELVTR off a PixelLab base the owner picked.

FIRST: read `.claude/skills/variants/SKILL.md` end to end, then `.claude/skills/sprite/SKILL.md`
§"Hard constraints". They carry measured numbers from this exact problem and they will save you
a wasted batch. Follow the variants loop (§The loop) step by step. Also read
`docs/art/aesthetic-direction.md` for the locked art direction.

BASE CHARACTER
  id     afa5582e-c649-49cc-96de-677e6f9869dd  ("Make brighter pallet")
  size   88x88, 8 directions, low top-down, mannequin template
  group  8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b  (37 states — a shared grab-bag group,
         it also holds the brood-ooze states; do not assume group siblings are relatives)
  Confirm all of this yourself with get_character before queueing anything.

This base is a SKELETAL HUMANOID (armoured soldier). That fact governs the whole task:
states off a skeleton cap the proportion range at ~1.2x spread, and a humanoid will not go
squat — three measured attempts on the Vanguard all undershot their target. Do not spend
generations re-proving this. On a skeleton the workable proportion axis is
NARROW <-> WIDE-BY-EXTENSION (tuck limbs and extend upward; or hold something rigid out to
the side), never tall <-> squat.

THE OWNER'S DECISION, already made — build it this way, do not re-litigate it:

  STATES ONLY. Every one of the six is a `create_character_state` call on afa5582e with
  use_color_palette_from_reference=True on EVERY call. The owner explicitly ruled out the
  standard-mode proportion prototype. DO NOT call `create_character`. Do not use `proportions`
  knobs. Do not generate a "cheap shape study first" — that path was considered and rejected,
  and proposing it again wastes the owner's time. All six inherit the base's palette, lighting,
  88px canvas and kit, and that cohesion is the point.

  Save every variant to RawArt/Renders/soldier-scifi-variants/<variant>/

  THE SIX, split across the two axes the owner asked for:

  3 x PROPORTION. On a skeleton the reachable axis is NARROW <-> WIDE-BY-EXTENSION. Measured
  precedents to aim at, all from a humanoid base:
    - NARROW: arms tucked in, prop stowed or dropped, body extended upward. The Pikeman
      reached aspect 0.58 at 28px wide and held 0.46-0.58 across every rotation.
    - WIDE: extend outward with something RIGID rather than inflating the body. A spear held
      level got 53px — the widest knight produced in a whole session — at solidity 0.45 with
      only 0.10 rotation drift.
    - MID: the base's own mass, or one step off it, as the anchor the other two are read
      against.
  Do NOT brief a squat/crouched/immovable-heavy variant. Three measured attempts
  (1.02 vs 1.30 target, 0.82 vs 1.00, 0.89 vs 1.30) all undershot, and the last came back with
  solidity identical to the base across all eight facings. The skeleton will not compress.

  3 x TOPOLOGY. Change the KIND of outline, not the ratio:
    - one asymmetric form with a clear front and back (target asymmetry > 0.4; 0.60 was enough
      to separate three Vanguard states that all sat at aspect 1.16-1.18)
    - one with a genuine enclosed hole — a notch is NOT a hole; the gap must be closed on every
      side, and it must be ~19px+ on this 88px canvas to survive downsampling. The reliable way
      is a prop or drone held AWAY from the body, not an armpit (10px armpit holes close to
      ~2px at panel scale and vanish).
    - one with a notched or split outline (jagged top edge with air gaps, or a body divided)

  EXPECTATION-SETTING, so you report honestly rather than defensively: states off a skeletal
  humanoid have historically capped near 1.2-1.5x aspect spread. The Vanguard topology batch
  spread aspect only 1.5x and its variants still read completely apart, separated by asymmetry
  and hole count. So do NOT judge this family on aspect alone — report aspect, asymmetry,
  solidity and hole count together, and say plainly which axis did the separating.

  Before writing any prompt, write the VARIANCE TABLE first: which single variable each variant
  moves, what all six hold constant, and how many the axis supports before they stop separating.
  If you cannot say in one measurable phrase what makes variant 5 different from variant 1,
  do not queue it.

  Ban glow explicitly in every prompt — aesthetic-direction reserves light for the flame.
  Prefer SIMPLIFYING the outline over loading it with gear: measured, stripping to one nameable
  primitive gave a 3.2x aspect spread where adding pauldrons/shields/banners gave only 1.6x.
  BUT there is a hard ceiling — simplify only to the simplest shape that STILL KEEPS A HEAD AND
  LEGS. Of six pure primitives tried, three came back as furniture (a metal egg, a framed
  plaque, an arrowhead). If you cannot name the unit from its outline, reject it however good
  its aspect number.

TRAPS THAT COST REAL GENERATIONS
  - `size` is not the canvas: the canvas returns ~40% larger. For an 88-92px canvas request
    size ~64. Verify the returned size on the FIRST generation before queueing the rest.
  - Download every rotation to disk BEFORE judging anything (standing project rule — an
    upstream delete must not be able to lose the evidence). Nothing generated is deleted until
    the owner gives a keep/reject verdict.
  - Never verdict from the south frame alone. Run `--all-directions`. A variant briefed as a
    long low ridge measured 0.86 from the south and 0.59-2.60 across its rotations — it was the
    best in the batch and nearly got rerolled. On a directional form the drift IS the feature.
  - Say the generation estimate out loud BEFORE queueing the batch. Six states at 88px is
    ~120-240 generations (states run 20-40 each, scaling with canvas). Current balance ~7,594.
    Generate the FIRST state alone, verify its returned size and that the palette actually
    carried over, and only then queue the remaining five.

DELIVERABLE — a published page, which is what the owner asked for
  1. py Scripts/art/silhouette_report.py RawArt/Renders/soldier-scifi-variants --all-directions --out <sheet>.html
  2. Publish it with the `Artifact` tool. Each variant renders next to its FLAT OUTLINE —
     for a small dark unit the outline is what the player actually receives.
  3. Write docs/art/soldier-scifi-variants.md: the variance table, the full measurement table
     (aspect / solidity / asymmetry / holes, plus rotation range per variant), which axis did
     the separating, and a keep / reroll / drop verdict per variant.

REPORT BACK with the numbers, not adjectives. State the measured aspect spread as a number.
Name which variants MISSED their target — an undershoot is a finding to report, never something
to quietly omit. Judge against: aspect spread >= 2.5x good / < 1.3x fail; zero pairs sharing an
identical opaque-pixel count (automatic fail); at least one variant above 0.4 asymmetry; and
every variant still readable as a soldier from its outline alone.

YOU OWN, and may write, only:
  RawArt/Renders/soldier-scifi-variants/**
  docs/art/soldier-scifi-variants.md

YOU MUST NOT TOUCH — task-055 is live in this repo right now and holds these:
  ELVTR/Content/Sprites/Units/**        (no UE import — none at all)
  docs/data/art/provenance.json
  RawArt/Renders/knight/**, RawArt/Renders/archer-proxy/**
  any RawArt/Renders/ folder other than soldier-scifi-variants/
  Do not open the Unreal editor. Do not use unreal-mcp. Do not pack a SubUV sheet, quantize to
  the Demichrome ramp, or import anything. This task ENDS at the owner's verdict on the contact
  sheet — /sprite owns packing and import as a separate follow-up.
  Do not edit GDD.md, SYSTEMS.md, CLASSES.md, or anything in docs/narrative/.

CANON NOTE: WORLD.md is SUPERSEDED (narrative reset 2026-07-22). Current canon is
docs/narrative/FLAME-FOUNDATION.md. Do not cite WORLD.md.
```
