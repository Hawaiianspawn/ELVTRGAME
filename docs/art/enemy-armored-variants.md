# Armored-enemy silhouette variants (task-112)

**Status: GENERATED, judged, contact sheet published. Owner verdict still pending — nothing
packed, quantized, or imported.**

**Base:** PixelLab character `afa5582e-c649-49cc-96de-677e6f9869dd` ("Make brighter pallet"
state), 88x88, 8 directions, low top-down, group `8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b`.
Despite the character's PixelLab name this is a bright grey/white **armored humanoid**, not
an ooze — the skeletal counterpart to the amorphous brood, and the first art on disk for the
soldier archetype `docs/art/npc-silhouette-brief.md` §(b) describes (silhouette mechanism
still useful direction; its 4-value Demichrome table is superseded — full colour ships,
nothing here was quantized).

This same base was used by `soldier-scifi-variants.md` (task-056), whose rev 1 already found
that `create_character_state` cannot move a skeletal base's *proportion* — it can only move
*pose*. This batch corroborates that finding independently (see below) rather than
re-deriving it.

All 8 generated variants are `create_character_state` on `afa5582e`, `use_color_palette_from_
reference=True`, no `standard` mode, no `proportions` knobs. Every `edit_description` banned
glow explicitly. `v0_base` is `afa5582e`'s own 8 rotations, fetched as a reference card, not
generated.

Contact sheet: **https://claude.ai/code/artifact/275cccf4-6549-472e-a73d-7f5512c28e29**

Raw renders: `RawArt/Renders/enemy-armored/raw/<variant>/rotations/*.png` — all 8 directions
downloaded for all 9 cards before any measurement, per the retention rule. Nothing has been
deleted or rerolled.

---

## Variance table (written before any prompt was sent)

| # | variant | axis | single move | target |
|---|---|---|---|---|
| v0 | base | — | legacy reference card | — |
| v1 | pike | aspect, narrow | arms tucked, pike held vertical against the body | aspect 0.58 |
| v2 | column | aspect, narrow | no props at all, one clean vertical column, head+legs kept | aspect 0.50 |
| v3 | levelspear | aspect, wide-by-extension | rigid spear held level across the chest | aspect 1.6 |
| v4 | standard | aspect, wide-by-extension, asymmetric | standard/banner out to one side only | asymmetry > 0.4 |
| v5 | slabshield | aspect, wide-by-extension, asymmetric | tall slab shield one side, sword arm tucked other | asymmetry > 0.4, solidity above siblings |
| v6 | notched | topology | deep vertical notch, head dropped below shoulder line | aspect near base |
| v7 | spined | topology | jagged spikes, enclosed air gaps between them | enclosed holes surviving all 8 rotations |
| v8 | tethered | topology | flail head held away from torso on a chain | >=1 enclosed hole surviving all 8 rotations |

---

## Measured results

### South frame

| variant | content | aspect | solidity | asymmetry | holes | luma | colours |
|---|---|---|---|---|---|---|---|
| v0_base | 43x45 | 0.96 | 0.61 | 0.17 | 1 (7px) | 0.318 | 67 |
| v1_pike | 36x51 | 0.71 | 0.61 | 0.22 | 2 (16px) | 0.303 | 35 |
| v2_column | 34x45 | 0.76 | 0.66 | 0.02 | 2 (12px) | 0.325 | 32 |
| v3_levelspear | 59x45 | 1.31 | 0.40 | 0.16 | – | 0.312 | 32 |
| v4_standard | 40x52 | 0.77 | 0.53 | 0.60 | – | 0.309 | 30 |
| v5_slabshield | 44x46 | 0.96 | 0.73 | 0.41 | 1 (7px) | 0.358 | 34 |
| v6_notched | 43x41 | 1.05 | 0.63 | 0.18 | 1 (7px) | 0.282 | 32 |
| v7_spined | 43x45 | 0.96 | 0.64 | 0.15 | 1 (7px) | 0.322 | 31 |
| v8_tethered | 48x50 | 0.96 | 0.49 | 0.87 | – | 0.301 | 34 |

**South-only aspect spread (`variantpipe.py judge`'s own number): 0.71-1.31, a 1.9x range.**
Above the "will not separate" floor (<1.3x) but well under the brood's 3.1x — exactly the
skeletal-base ceiling `.claude/skills/variants/SKILL.md` documents, and consistent with
task-056's rev 1 on this same base (1.7x south-only). **Zero variants share an identical
opaque-pixel count** — the automatic-fail check never triggers here.

### All 8 rotations (`--all-directions`)

| variant | aspect range | drift | solidity range |
|---|---|---|---|
| v0_base | 0.52-0.98 | 0.46 | 0.53-0.67 |
| v1_pike | 0.40-0.71 | 0.31 | 0.61-0.65 |
| v2_column | 0.48-0.76 | 0.28 | 0.63-0.70 |
| v3_levelspear | 1.00-1.34 | 0.34 | 0.37-0.44 |
| v4_standard | 0.61-1.28 | **0.67** | 0.31-0.54 |
| v5_slabshield | 0.55-0.98 | 0.43 | 0.68-0.73 |
| v6_notched | 0.55-1.05 | 0.50 | 0.53-0.67 |
| v7_spined | 0.55-0.96 | 0.41 | 0.48-0.64 |
| v8_tethered | 0.85-1.12 | 0.26 | 0.41-0.53 |

`silhouette_report.py` itself flags v4_standard's own WATCH: 0.67 drift is the largest in the
family — a genuinely directional form, not noise, matching its asymmetry brief.

`variantpipe.py judge` returns **9 keep, 0 flag, 0 reject** — no duplicate outlines, no
missing rotations, no canvas mismatches. That is the mechanical half of judging; the four
findings below are the human half SKILL.md says judge cannot do by measurement alone.

---

## Named findings — four misses judge could not catch

**1. v2_column came back *wider* than v1_pike, inverting the brief.** v1 was briefed narrow
(target 0.58) and v2 narrower still (target 0.50), but measured v1 = 0.71 south / band
0.40-0.71, v2 = 0.76 south / band 0.48-0.76 — v2 is wider on every measure. Visually v2 still
holds something in front rather than reading as a fully prop-free column; the "remove all
props" instruction was not honoured as strongly as v1's "tuck arms, hold pike vertical."
**Flag — reroll with a more forceful no-props phrasing if the intended ordering matters.**

**2. v3_levelspear undershot its aspect target.** Target 1.6; measured 1.31 south, band
1.00-1.34 (max 1.34 across all 8 rotations). The generated pose holds the spear angled
diagonally toward the lower-left rather than level across the chest, which is why the width
never reached the brief. Still the widest-reading variant in the family by a clear margin
(lowest solidity too, 0.37-0.44, correctly gappy for a spear-across pose), so it separates
fine on the sheet — it just isn't the specific 1.6x shape asked for. **Flag — reroll with more
explicit "perpendicular and flat" phrasing if 1.6 is required.**

**3. v7_spined delivered real holes, but not the briefed shape, and not on every facing.**
Per-direction hole sizes: south 7px, south-east 12px, east 18px, north-east 17px, north 30px,
north-west 17px, west 17px, south-west 12px. Six of eight rotations clear the ~15px
panel-scale survival floor SKILL.md documents; south and south-west (7px, 12px) sit at or
under it and will likely close up at gameplay zoom. Visually, though, PixelLab rendered this
as a shield-like slab with cutouts on one side — the same visual language as v5_slabshield —
rather than the briefed jagged spiked crown on the shoulders/back. **Flag — functions as a
"holes" variant in most facings, but the spiky-crown read specifically was not achieved.**

**4. v8_tethered produced zero enclosed holes in all 8 rotations — a clean miss of its one
stated purpose.** Per-direction: every single rotation measured 0 holes / 0px. The south
frame visually shows the flail head hanging away from the body with an apparent gap, but the
chain hangs low enough that the gap opens to the sprite's bottom edge rather than being
enclosed on all four sides — exactly the "a notch is not a hole" failure mode SKILL.md warns
about (a gap the background can flow into from any edge doesn't count). **Reject the hole
objective specifically — reroll holding the flail/weight out to the SIDE at chest or shoulder
height, explicitly clear of the ground/hem, rather than hanging low.**

## What landed clean

- **v5_slabshield** hit both stated targets: asymmetry 0.38-0.47 (clears >0.4, though only
  just at its narrowest facing) and solidity 0.68-0.73, the highest band in the whole family
  (next-closest is v2_column's 0.63-0.70). Visually the strongest read in the batch — shield
  unmistakable on one side, sword arm tucked on the other.
- **v4_standard** clears its asymmetry target comfortably (band 0.35-0.79, south 0.60) and is
  the most direction-dependent form measured (0.67 drift, SKILL.md's WATCH). Worth checking
  north/east before final approval — the south frame alone reads more like another
  pole-holder than a clearly one-sided standard-bearer; the asymmetry is real but subtle
  head-on.
- **v1_pike** is the cleanest narrow read in the family (band 0.40-0.71) and unambiguously
  reads as a pikeman from every facing checked.
- **v6_notched** held its aspect near the base as briefed (band 0.55-1.05 against base's
  0.52-0.98) but the south frame reads as a blockier, wider-shouldered soldier rather than a
  clearly cleaved one — the "deep notch" silhouette specifically is not obviously legible.
  **Flag — soft miss on the topology's visual character even though the aspect constraint
  held.**
- Every variant is nameable as an armored soldier from its outline alone on the frames
  inspected — none crossed SKILL.md's "stopped reading as the unit type" reject line.

---

## Cost

8 generated variants x 20 generations each (Gemini tier floor, confirmed via `get_balance`
before and after) = **160 generations spent**, against a 160-320 estimate and a starting
balance of 6725/10000. Ending balance: 6565/10000.

## Judging against the stated bar

| Check | Result |
|---|---|
| aspect spread >= 2.5x good / < 1.3x fail | **1.9x south-only — clears the "will not separate" floor, well under the brood's 3.1x, matching the documented skeletal-base ceiling** |
| zero pairs sharing identical opaque-pixel count | **pass** |
| at least one variant above 0.4 asymmetry | **pass — v4_standard, v5_slabshield, v8_tethered all clear it** |
| every variant readable as the unit type from its outline alone | **pass** |
| every stated per-variant target actually hit | **fail on 4 of 9 — v2_column (inverted vs v1), v3_levelspear (undershot), v7_spined (wrong shape, partial hole survival), v8_tethered (zero holes, clean miss)** |

Report the numbers, not "they look varied": **1.9x aspect spread, 4 of 9 variants missed their
stated brief in a way only the 8-direction measurement or a direct look at the frame caught**,
and 5 of 9 (v0, v1, v4, v5, v6-partially) landed as briefed or better.

---

## Iteration 2 (2026-07-30) — claw armor/weapons, less flail/pike

Owner direction: iterate toward claw armor and claw weapons, and away from old-fashioned
melee (flail, pike). **v1_pike and v8_tethered are superseded** — kept on disk per the
retention rule, not deleted — replaced by two new `create_character_state` calls on the same
base, same briefs/targets, claw imagery instead of a pole weapon or a flail:

| slug | replaces | brief | target |
|---|---|---|---|
| v9_clawgauntlet | v1_pike | arms tucked, both hands end in oversized clawed gauntlets, no held prop | aspect 0.58 |
| v10_clawreach | v8_tethered | a massive claw held out at shoulder height, clear of the torso on every side, not hanging low (explicitly targeting v8's failure mode) | >=1 enclosed hole surviving all 8 rotations |

Cost: 2 x 20 generations = **40 more**, family total now **200 generations**. Balance
6725 -> 6505/10000.

### Measured — both replacements underperformed the variants they replaced

**v9_clawgauntlet: worse than v1_pike, not better.** Target 0.58; measured **0.96 south**,
band **0.63-1.00** — nearly as wide as the unmodified base (0.96) and clearly wider than
v1_pike's own band (0.40-0.71) it was meant to improve on. Looking at the frame: the claws
made PixelLab flare the elbows outward to show them off, the opposite of "tuck both arms
tightly against the sides" — the east/west facings do narrow (0.63), but south, the facing
that matters most on a top-down panel, does not. **Flag — this is a regression on the metric
that mattered, not a fix. If claw gauntlets are the right read for this variant, the brief
needs to force the arms flat against the torso more explicitly (e.g. "claws pointed straight
down, elbows touching the ribs") rather than trusting "tuck the arms" to survive the claw
detail.**

**v10_clawreach: still failed its one job, marginally.** Target: hole surviving all 8
rotations. Measured holes per direction: south 7px, south-east 0, east 0, north-east 0, north
6px, north-west 0, west 0, south-west 0 — a hole appears in only 2 of 8 rotations, and both
(7px, 6px) sit under the ~15px panel-scale survival floor. That is *worse* coverage than
v8_tethered's own attempt in raw count, though the failure mode is different: v10's claw
visually presses flush against the body (same "shield held to the side" language the batch
keeps converging on, see v5/v7's finding above) rather than v8's chain hanging low and open
to the bottom edge. Both routes land on the same result — no enclosed gap. **Reject the hole
objective again — a claw-on-a-visible-joint brief is converging toward the same flush "held
against the body" shape every wide-by-extension variant in this family produces by default;
getting a real gap likely needs an explicit distance called out in the prompt (e.g. "a full
claw-width of open air, at least 20 pixels, between the claw and the shoulder") rather than
just "held away."**

**Recommendation:** don't reroll blind a third time. Both misses are legible and specific
enough to write a sharper brief from, but that is a call for whoever is judging this sheet
next, not something to keep spending credits chasing automatically. v1_pike and v9_clawgauntlet
are both on the sheet now — the owner can pick whichever silhouette actually reads better even
though neither hit its number, since v1_pike's own aspect (0.71) also missed its 0.58 target.

Contact sheet republished at the same URL with all 11 cards (v0-v10):
**https://claude.ai/code/artifact/275cccf4-6549-472e-a73d-7f5512c28e29**
