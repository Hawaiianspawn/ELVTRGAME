# Soldier roster v1 — six congregation variants, authored as a comparative set

**Subject:** six ordinary-people-who-came-to-a-light soldier variants, designed **as one set**
so that no two of them carry their identity on the same shape ·
**Cell:** 48×48 (locked, owner decision 2026-07-25) · **Sheet:** 4×1 → 192×48 each ·
**Requests:** `../data/art/requests/soldier-01.json` … `soldier-06.json` ·
**Anchors:** all six **authored** — `pixelpipe.py authored` renders §6–§11's pixel maps
directly, 0 generations.

**Binds to:** `docs/art/aesthetic-direction.md` (2026-07-12 RESET — Direction A locked,
global 4-value Demichrome ramp, no fifth value, no palette swaps, no per-faction colour) ·
`docs/narrative/FLAME-FOUNDATION.md` (§1 premise, §3a leash-as-light) ·
`CLASSES.md` §1 (Vanguard / the Liberated ladder: Freed → Militia → Veteran → Bannerman;
C4 "veteran promotion = helmet pixel-tier") and the cross-class silhouette rule at the end
(lines / blocks / darts / points of glow) · `docs/art/npc-silhouette-brief.md` (the
three-way disjointness model: silhouette × value dominance × pale usage) ·
`docs/art/retinue-militia.md` (format precedent; the ragged-militia mechanism) ·
`docs/art/soldier-style-depth-test.md` (the measured 2026-07-25 batch — this spec is a
direct response to its findings) · `docs/art/vanguard.md` (the hero these units stand
beside) · `docs/data/art/palette.json`.

**Deliberately does NOT bind to:** `WORLD.md` (superseded in full), `docs/art/hero-palettes.md`
§1–§2 (void), or any faction, biome or NPC name — current canon names none
(FLAME-FOUNDATION §5) and this spec does not invent one.

> **Revision r2 — 2026-07-25. Variants 01 and 06 only.** The r1 anchors for all six passed
> the stage-D gate and were rotated to eight directions from the authored frame
> (`create_character(mode="v3", reference_image_base64=…)`, 93.3–100% on-palette, zero
> findings). **02, 03, 04 and 05 hold their carriers in all eight directions and are
> untouched by this revision** — including 05, whose caged Pale correctly self-occludes on
> the three rear-facing frames. **01 and 06 regressed under rotation** and their anchors,
> their sections (§6, §11), the 01×04 disjointness entry in §5, and the affected numbers in
> §3/§3a/§5a/§16.4 are revised below. Block order and block indices are unchanged: 01 is
> still index 0 and 06 is still index 5. See §6.1 and §11.1 for the two failure reports.

---

## 1. Intent

**Fiction.** The world is pitch dark and the player carries the only flame. These six are
the congregation: ordinary people who attached themselves to a light because outside it the
dark takes you. They are **not a uniformed army and not an enemy faction** — they are a
crowd that has been standing in someone else's fire long enough to start organising itself.
The set has to sell a *ladder of belonging*: the newest arrival owns nothing, the oldest
hand is wrapped in dead people's iron, and one of them has the job of keeping the fire fed.

**Gameplay.** Under a strict global palette there is no colour channel at all. Class,
faction, tier and role ride on **silhouette, value-pattern, and how the single bright value
is spent** — nothing else. So the deliverable is not six sprites, it is six *mutually
exclusive shape claims*, each of which must survive a 4-value collapse, a 16px crowd, and
partial occlusion by its neighbours.

**Written against measured evidence, not taste.** `soldier-style-depth-test.md` ran six
render treatments on one subject and produced four findings that this spec is built around:

1. **Depth does not survive the ramp.** Every treatment above `flat shading` spent its extra
   pixels on speckle and pushed Dark up (cell 03 hit 60.7% Dark against a Bone-dominance
   spec). No variant here specs shading, and every value edge below is a hard fill boundary.
2. **Text prompting cannot deliver an irregular silhouette.** Across six cells *none* of the
   militia's identity carriers landed; PixelLab regressed every one to a generic uniform
   armoured soldier with a matching helmet. **That is why all six anchors here are authored
   pixel maps, not prompts.** The prompt block in each request exists only to steer the
   later rotation pass, which inherits the authored frame as its reference.
3. **At gameplay scale the helmet dome is the only shape that reads**, and it quantizes to
   Bone, pulling the eye off the face and off the silhouette. The response, stated as a
   rule for this set: **the head is a primary carrier, and only one of the six is allowed to
   keep the plain dome.** Three variants (01, 04, 05) have no dome at all, one (03) squares
   it into a stepped block, one (06) sinks it behind a shield so only the eyes clear, and
   exactly one (02) keeps it — because 02's job is to *be* the baseline the other five
   deviate from.
4. **Dark accumulates under the collapse.** Outline, recess and shadow all land on Dark, so
   every variant below states what must be **absent** (no cast shadow, no doubled outline,
   no filled Dark body mass) as well as what is present.

---

## 2. The palette

Global ramp, no exceptions, no swaps (`aesthetic-direction.md` 2026-07-12 reset; GDD #6
resolved to strict global palette). All six variants draw from exactly this table.

| Hex | Name | Role across this set |
|---|---|---|
| `#211e20` | Demichrome Dark | outline, seam, recess, eye notch, boot, belt, plank seam, vessel rim. **Never a filled body mass on any of the six.** |
| `#555568` | Demichrome Steel | worked metal and hard gear: helm, pauldron, mail skirt, iron banding, apron, firepot walls. The *ladder* value — how much of it you carry is how long you have survived. |
| `#a0a08b` | Demichrome Bone | skin, rag, cloth, bare wood, horn, plank. The *person* value. |
| `#e9efec` | Demichrome Pale | **spent by variant 05 only**, as the fire in its pot. All five others are `pale_usage: none` — the QC pass flags any pale pixel on them. |

Transparency is **not a value**: alpha is binary (0/255), Unlit + Masked, per
`palette.json.mask`. The quantizer hard-thresholds at 128; partial alpha is the most common
silent breakage. Legend used by every code block below (`palette.json.ascii_legend`):
`.` transparent (mask, not a value) · `#` Dark · `S` Steel · `b` Bone · `@` Pale.

**Reservations protected by this set, stated so they can be audited:**

- **The Vanguard's `rectangle_flip`** (banners only) is untouched — no variant carries a
  flag, pennant or cloth rectangle of any size. Variant 04 replaces the standard axis with a
  *sound* carrier precisely to avoid it (§4).
- **The Relickeeper's `dot_cluster`** (rune marks) and **the Pathfinder's `thin_contour`**
  (quarry marks) are drawn *over enemies* by other classes and appear nowhere here, so both
  stay fully available.
- **The Lampbearer's `point_halo`** is the one carrier this set touches, and only variant 05
  touches it. §10 defines a *caged*-versus-*free* distinction to keep the two unforgeable,
  and §16.1 raises it as a canon proposal because it is a change to a registered carrier.
- **Pale eyes are forbidden on all six.** Every eye in this set is a Dark notch. A pale
  eye-dot is the registered tell of the amorphous void register (`npc-silhouette-brief.md`
  (c)); putting one on a friendly is the single worst confusion available in a hue-less game.
- **No faction value.** Current canon names no factions; nothing is reserved for one and
  nothing is invented for one here.

**Light-shifted variants: none emitted** (`light_shift_variant: false` on all six). The
state these subjects need is the *dim* one, and per `palette.json.dim_shift` that is a
4-entry value remap driven by one scalar on the Unlit+Masked material — not a second
texture, and never per-unit material work (mass units are GPU-instanced). See §13.

---

## 3. The set, at a glance

| # | Axis | Carrier (the one shape that IS this variant) | Head signature | `value_dominance` | `pale_usage` | Histogram (D / S / B / @ %) |
|---|---|---|---|---|---|---|
| 01 | Volunteer | **The braced haft** — one *rigid* 3px bar on a hard 45°, Steel-capped at the high end, gripped at the chest, topping out six rows below its own crown | Rag-wrapped soft lump, **no headgear, no brim line** | `bone` | `none` | 29.1 / 2.2 / 68.7 / 0.0 |
| 02 | Line infantry | **The dome-and-vertical** — plain dome helm over one dead-vertical shaft; bilaterally symmetric; nothing leaves the outline sideways | Steel dome + hard Dark brim (the only dome in the set) | `mixed` | `none` | 27.2 / 38.6 / 34.3 / 0.0 |
| 03 | Veteran | **The stepped crown and the lost neck** — two helmet steps where 02 has one, pauldrons flush with the jaw, mail skirt | Squared two-step helm, wide flared brim | `steel` | `none` | 27.9 / 55.0 / 17.0 / 0.0 |
| 04 | Rally caller | **The upswept curve above the head** — a bent horn whose bell clears the crown; the only curve in the roster | Bare shaved skull, no headgear, crown broken by the horn | `bone` | `none` | 31.8 / 5.1 / 63.2 / 0.0 |
| 05 | Flame-tender | **The caged bright** — a Pale core boxed on all four sides inside a hard-rimmed iron pot at chest height | Peaked Steel hood with a deep Dark face cavity | `steel` | `halo` | 36.6 / 43.4 / 17.7 / 2.4 |
| 06 | Shield-heavy | **The sunken head behind the rectangle** — a door-shield wider and taller than the body it hides, carrying **one** iron band across its middle and an iron plate welded to each rim; only the eyes clear the top | Dome sunk below the shield's top edge | `mixed` | `none` | 28.7 / 33.5 / 37.8 / 0.0 |

Histograms are counted from the pixel maps in §6–§11 and are what
`pixelpipe.py quantize --stage anchor` must reproduce (an authored anchor is on-palette by
construction, so these should come back unchanged at 100% on-palette).

### 3a. Why value dominance is distributed this way

**Dominance carries the ladder, not the role.** Steel is worked metal, and in this fiction
worked metal is *inherited from the dead* — you get it by surviving, or by taking it off
someone who did not. So the three ladder rungs are a monotone Steel ramp and nothing else:

| Rung | Steel % | Bone % | What the histogram says without a word of UI |
|---|---|---|---|
| 01 Volunteer | 2.2 | 68.7 | owns nothing but a tool cap |
| 02 Line infantry | 38.6 | 34.3 | half-kitted, balanced, ordinary |
| 03 Veteran | 55.0 | 17.0 | more metal than person |

That is a legible three-step tier ladder achieved with **zero colour and zero UI**, which is
the whole problem the strict global palette created. Note the ladder is not merely "more
Steel" — the *person* value shrinks in exact counterpoint (68.7 → 34.3 → 17.0). At horde
scale a mixed rank reads as a gradient of paleness, and the pale ones are the new arrivals.

The three role variants then place themselves **on that same ladder**, which is what makes
their dominance non-arbitrary:

- **04 Rally caller = `bone` (63.2%)** — a caller is drawn from the congregation, not from
  the veterans; they are a volunteer with a horn. It sits one notch off 01 and reads as
  "unarmoured" at a glance, which is correct: a caller who looked armoured would read as an
  officer, and there are no officers in a congregation.
- **05 Flame-tender = `steel` (43.4%)** — this is the only variant where dominance is chosen
  for an *optical* reason rather than a fictional one, and it is worth stating plainly. The
  Pale core has to read at 4px across a crowd. Pale-on-Bone is a luma gap of 0.31; Pale-on-
  Steel is 0.59 and Pale-on-Dark is 0.81. A Bone-bodied tender would swallow its own fire.
  A Dark-bodied one would read as an enemy (below). Steel is the only body value left that
  gives the bright a field to burn against, and the fiction agrees: you do not tend fire in
  cloth.
- **06 Shield-heavy = `mixed` (37.8 Bone / 33.5 Steel / 28.7 Dark)** — the shield is a *door*,
  not issued kit: Bone planks, Steel banding, Dark seams. Its histogram is deliberately the
  flattest in the set, because its identity is 100% silhouette and 0% value; spending a
  dominance claim on it would waste one.

**Zero variants are Dark-dominant, and that is the most load-bearing choice in §3.**
`retinue-militia.md` §11.2 proposes the standing reservation *"Bone-dominant and irregular is
the player's side; enemies take Dark-dominant, or regular, or both."* Current canon names no
enemy (FLAME-FOUNDATION §5, §4.5 open), so that reservation is the only thing protecting
future friend/foe reading, and burning it on a friendly shield unit would be a very cheap
trade for a very expensive channel. **Dark is left unspent across all six on purpose.** The
temptation was real — 06 would read beautifully as a black bar across the rank — and it was
declined; see §16.2.

**Pale is spent once, by one variant, and nowhere else.** The brief's rule is that the
brightest value belongs to honest light first. Variant 05 spends 16 pixels of it (2.4%) on
an actual fire being fed. Compare the hero's banner at 11×7 = 77px (`vanguard.md` §4a): the
tender is a fifth of the hero's bright and it sits inside the crowd line, never above it, so
it can never counterfeit the beacon the player navigates by.

---

## 4. Axis 4 was replaced — the standard carrier is now a sound carrier

The brief's fourth axis was *"a standard / rally carrier — but `hero-vanguard` already owns
the rectangle-and-flag read, so differentiate hard or replace this axis."* **It is replaced.**

The reasoning, on the record because it discards a requested axis:

1. **Two things already own the Pale rectangle, and both outrank a new unit.** The Vanguard
   hero owns the large one (`palette.json.shape_carriers.rectangle_flip`, "banners only";
   `vanguard.md` §2 — the hero is *the only unit-scale sprite in the game permitted a large
   contiguous Pale area*). The Bannerman owns the small one (`CLASSES.md` §1; capped at
   ≤4×3px at head height by `retinue-militia.md` §3). A third flag-bearer would have to fit
   between 4×3 and 11×7 *and* stay disjoint from both in altitude — a gap roughly two pixels
   wide. That is not a design, it is a rounding error.
2. **A non-Pale flag is worse than no flag.** Under the strict global palette a Steel or Bone
   flag is a mid-value rectangle floating over a crowd of mid-value people. It has no
   contrast job to do and reads as a smear at 16px.
3. **The fiction has a better answer available.** Rally in a pitch-dark world does not
   propagate visually — a flag is only visible where the light already reaches, which is
   exactly where you did not need it. **Sound does.** A horn reaches into the dark past the
   circle's edge, which is where FLAME-FOUNDATION §3b says Hold and Charge put people.

So variant 04 is a **horn-caller**, and its carrier is a shape no other sprite in the game
owns: **a thick bent curve that breaks the silhouette above the head.** The Vanguard's whole
grammar is straight lines and rectangles (`CLASSES.md` §1); the Pathfinder owns "the only
curve in the hero row" (their bow arc), and that is a *hero-scale* reservation on a thin
drawn arc — this is a fat, opaque, head-attached tube on a mass unit, and it is flagged
against that reservation in §9 and §16.3 rather than assumed clear.

---

## 5. Disjointness audit — pairwise, all fifteen pairs

The model is `npc-silhouette-brief.md`'s three-axis test: two variants are safely disjoint if
they disagree on **any two** of {silhouette, value dominance, pale usage}, and disambiguate
at horde scale under motion and occlusion if they disagree on all three. With hue gone, this
is the entire readability mechanism, so every pair is checked rather than sampled.

| Pair | Shared axis (the risk) | Resolved by |
|---|---|---|
| **01 × 02** | both humanoid, both hold one straight prop | **Dominance** (bone 68.7 vs mixed) **and prop axis, which is now 45° apart exactly**: 01's haft runs on a hard 45° and stops at the chest, 02's shaft is *dead vertical* and runs the full height of the cell inside the outline. A 45° bar cannot be misread as a vertical one at any zoom; this is why r2 held 01's haft at 45° rather than steepening it to win back the length it lost. Note the pair no longer separates on *length* — r2's haft is 21px and 02's shaft is 39px, so 02 is now the one with the long prop, and 01's claim is angle and rigidity only. 01 has no brim line and no hard headgear; 02's brim is a full-width Dark bar. |
| **01 × 03** | both humanoid | All three axes: 68.7% Bone vs 55.0% Steel; soft rag lump vs squared two-step helm; diagonal prop vs vertical prop; 01's shoulders slope, 03's have no neck at all. Maximum separation in the set. |
| **01 × 04** | **both `bone`-dominant, both break the outline with a held/worn object.** The most dangerous pair — and **the one pair the r1 rotation actually broke** (see §6.1). | **The crown line, plus rigidity — stated as two measurable properties rather than as a description, because the r1 description was true of the anchor and false of three rotated frames.** (a) **Crown line:** 04's carrier *crosses the top of its own head* — the bell clears the crown by 3 rows. 01's carrier *never reaches* the crown line: its topmost pixel sits 6 rows below it, and no rotation is permitted to raise it. Over-the-head versus under-the-head is the whole separation and it survives 12px. (b) **Rigidity:** 04's carrier is a curve *by construction* — 4px thick, head-attached, bending from diagonal to near-vertical. 01's is a straight bar, **3px thick along its entire length and only 21px long (7:1)**, on a single unbroken 45° run with a flat Steel cap at one end and a hard Dark grip at the other. r1's haft was 2px × 31px (15:1) with a 4-row Steel blob on the end; at that aspect ratio the rotator had no rigidity cue, arced it, and handed 01 a curve under the head — i.e. handed it 04's carrier. Thickness and shortness are therefore load-bearing constraints on this pair, not styling. Secondary: 01's head is wrapped and lumpy, 04's is bare and round; 04 carries a Steel baldric diagonal *inside* the torso (an internal value line 01 has nowhere). |
| **01 × 05** | both irregular | Dominance (bone vs steel), pale (none vs halo), head (rag lump vs peaked hood), and 05 holds nothing that leaves the outline upward — its break is a low hump on one side. |
| **01 × 06** | — | Every axis. Thinnest sprite in the set (550 opaque px) vs the heaviest (878). |
| **02 × 03** | **by design — 03 must read as a tier-up of 02, not as a different unit.** | The changes are deliberately *additive and countable*, so the eye reads "same soldier, more of it": **+1 helmet step** (a squared crown block above the brim), **−1 neck row** (pauldrons rise flush with the jaw), **+4px shoulder width**, **+ a mail skirt** where 02 has a belt and bare legs. Same spear, same stance width, same grip height. The dominance shift (mixed → steel, 38.6% → 55.0% Steel) is the value half of the same statement. This is `CLASSES.md` C4 "helmet pixel-tier" rendered literally. |
| **02 × 04** | both humanoid, similar mass | Dominance (mixed vs bone), head (dome + brim vs bare shaved), and the prop axis is *orthogonal*: 02's is vertical and inside the outline, 04's is a curve outside it and above the head. |
| **02 × 05** | — | All three axes. |
| **02 × 06** | **both `mixed`.** | Silhouette is as far apart as the cell allows: 02 is a narrow vertical column (22px wide bbox) with a head fully clear of everything; 06 is a 28px-wide rectangle with its head *sunk behind* that rectangle. At horde scale one is a picket and the other is a wall. |
| **03 × 04** | — | All three axes. |
| **03 × 05** | **both `steel`-dominant.** | **Pale** separates them outright (none vs halo) — 05 is the only sprite in the set with a bright pixel. Silhouette: 03 is a tall sealed column whose widest point is its shoulders; 05 is squat, hooded, holds a wide hard-rimmed box at chest height, and has a bundle hump breaking one side. |
| **03 × 06** | both heavily equipped | Dominance (steel vs mixed) and **head placement**, which is the sharpest single tell in the whole set: 03's head is the *highest* point of the sprite relative to its shoulders, 06's is the *lowest*. One is all neckless shoulder; the other has no visible shoulders at all. |
| **04 × 05** | both break the outline asymmetrically | Dominance (bone vs steel), pale (none vs halo), and the break's altitude: 04's is above the crown, 05's is below the elbow. |
| **04 × 06** | — | All three axes. |
| **05 × 06** | — | All three axes. |

### 5a. Against everything else already specced

| | Silhouette | Value dominance | Pale usage |
|---|---|---|---|
| **Vanguard hero** (`vanguard.md`) | rigidly geometric — flat shoulder line, rectangle shield held to the side, dead-vertical 2px Dark pole, head fully clear | Steel ≥50% | **one large Pale rectangle**, 11×7px, riding above the crowd |
| **Retinue militia** (`retinue-militia.md`) | ragged *patchwork* — half-capped head, torso split cloth/plate, mismatched legs, short club held clear at the side | Bone 38–44% | none |
| **01 Volunteer** | *destitute*, not patched — nothing matches because nothing exists; one rigid 45° bar, never above the crown | Bone 68.7% | none |
| **02 Line infantry** | regular, symmetric, one vertical Bone shaft | mixed | none |
| **03 Veteran** | stepped, neckless, skirted | Steel 55.0% | none |
| **04 Rally caller** | bare skull + curve above the crown | Bone 63.2% | none |
| **05 Flame-tender** | hooded, squat, caged bright at chest height | Steel 43.4% | **halo, caged** |
| **06 Shield-heavy** | single-banded rectangle wider than its bearer, head sunk | mixed | none |
| *A regular/uniformed enemy register, if one is ever named* | clean, symmetrical, repeated | **Dark-heavy** (reserved, unspent here) | none |
| *An amorphous void register, if one is ever named* | non-geometric, no held object | flat Dark, zero internal modelling | bare eye-dot, no halo |

Three collisions with existing work needed explicit resolution and got it:

- **02 vs the Vanguard hero, on "the one dead-vertical thing."** The hero's vertical is a
  **2px Dark pole carrying a large Pale rectangle**; 02's is a **2px Bone shaft with a small
  Steel head and zero Pale**, and 02 has no shield rectangle, no flat 18px shoulder line, and
  a head roughly half the hero's mass. At horde zoom the hero's vertical is a *dark* line
  under a bright block; 02's is a *light* line with nothing on it. They are inverses, not
  neighbours.
- **06 vs the Vanguard hero, on "the rectangle."** The hero's shield is *smaller than its
  body*, held to the side, with a Pale banner above and the head fully clear. 06's is
  *larger than its body*, held frontally, eats the head, and carries no Pale at all. If these
  two ever start colliding in playtest, the fix is 06's rim thickness — never the hero.
- **01 vs the retinue militia, on "the bone-dominant ragged one."** These are the same rung of
  `CLASSES.md` §1's ladder and they must not both ship. The militia's carrier is
  *patchwork* — four quadrants of mismatched looted gear. 01's carrier is *destitution* — a
  single body value, a rag for a helmet, and a farm tool. Mismatched and unequipped are
  different fictions and different histograms (militia Bone 40.3% / Steel 30.7; 01 Bone 68.7
  / Steel 2.2). One should be picked; see §16.4.

---

## 6. Variant 01 — Volunteer

**Reads as:** *a person who picked up a tool and followed the light - destitute, not equipped.*

**Carrier:** the braced haft. **A rigid 3px-wide Bone bar on a hard 45°**, shouldered up and
out to one side, its high end capped by a flat 3px-wide Steel tip and its low end closed by a
hard 3px Dark grip that **touches the torso outline**.

**Length is no longer the carrier — say so plainly, because r1's was.** The r1 haft spanned
31 rows (~33px along its axis) on a 38px-tall figure: it was identified by being *absurdly
long relative to its owner*, and that is precisely
the property that destroyed it (§6.1). The r2 bar is **21px along its axis** — a little
longer than the torso is deep (11 rows) and a little longer than the head is tall (17 rows),
but no longer a pole that dwarfs its owner. What identifies it now is three things, in this
order:

1. **Angle.** One unbroken 45° run, the only 45° in the roster. 02's shaft is dead vertical;
   04's horn bends. A hard diagonal is a claim neither of them can make.
2. **Rigidity.** 3px thick end to end, 7:1 aspect, hard terminal at each end. It reads as a
   *braced* object — a tool being carried on the shoulder rather than a stick being dragged.
3. **Altitude.** **The bar never reaches the crown line:** its topmost pixel sits six rows
   below the top of the head. 01's silhouette break is unambiguously *beside and below* its
   own head where 04's is *over* it (§5, 01×04).

The fiction survives the change intact. "Destitute" was never carried by the tool's length —
it is carried by 68.7% Bone, a rag for a helmet, an unbelted shirt, bare legs, and twelve
pixels of iron on a figure of 550. A shorter tool is if anything the more honest object: a
hoe, not a pike.

Three properties are non-negotiable and are what r2 exists to enforce:

1. **≥3px thick along its whole length** — a thin line has no rigidity cue and a generator
   will treat it as cord (§6.1).
2. **≤22px long** and on **one unbroken 45° run** — a single slope with no inflection, so
   there is no point at which a bend can be introduced without visibly contradicting the rest
   of the bar.
3. **Hard terminals at both ends** — a flat Steel cap at the high end (a 3px-wide squared-off
   top edge, not a staircase) and a Dark grip fused to the body at the low end. A bar that is
   gripped at one end and squared off at the other is a *held rigid object*; a bar that fades
   out at both ends is a rope.

**Head signature:** a rag-wrapped soft lump with an off-centre knot. No hard headgear, and —
critically — **no brim row.** The full-width Dark brim bar is the single strongest "helmet"
tell at 16px (it is what survives the collapse in `soldier-style-depth-test.md` finding 7);
01 has none, and its head instead carries one horizontal Dark seam *mid-skull* where the rag
is tied, which reads as fabric rather than as a rim.

**Value reasoning.** Bone 68.7%, Steel 2.2%, Dark 29.1% (378 / 12 / 160 of 550 opaque px).
This is still the most extreme value statement in the set and it is doing one job: **at horde
scale a rank of these is a pale, soft field.** The 12 Steel pixels are all on the tool's cap —
literally the only metal this person owns — and they sit at the far end of the diagonal, so
the one hard glint in the sprite is as far from the body as it can get. r1 spent 14 Steel
pixels on a 4×4-ish blob; r2 spends 12 on a squared inline tip instead, because a heavy blob
on the end of a long thin arm is a *flail*, and that is what the r1 rotation delivered. Dark
stays a contour: every Dark pixel is outline, eye notch, rag seam, belt, boot or the haft's
grip. **There is no filled Dark shape anywhere**, which is what stops a Bone-dominant sprite
from collapsing toward the enemy register when it dims outside the leash (§13). Dark rose 1.4
points against r1 (27.7 → 29.1) purely because the torso's lower-right outline is now closed
(the r1 haft ran through it) and the grip is 2px of new Dark; dominance is unchanged and the
margin over Dark is still 39.6 points.

**At 500 units this reads as:** the palest block on the field, with **short stiff bars angled
out of it at one consistent 45°** — a crowd shouldering farm tools, not a rank carrying
spears. This is the one place the r2 bar is genuinely weaker than r1's, and it should be
watched at horde zoom rather than assumed away: a 33px pole broke the crowd's top edge and
was countable from across the field; a 21px bar that stops below the head does not break the
top edge at all, and at 16px it contributes a stubby diagonal nub on one side of a pale blob.
The rank still reads as *tools at a consistent angle* — the angle is what survives the
collapse, not the length — but if playtest shows 01 has stopped being distinguishable from a
plain unarmed body in a mass, the fix is **width, not length**: take the bar to 4px and hold
it at 21px, which buys back visibility without buying back the 15:1 aspect that failed. Do
not lengthen it. When these walk out of the light and take `dim_shift`, Bone→Steel and the
whole block greys wholesale in one step, which is the sympathetic read: the newest people are
the ones the dark erases first.

### 6.1 Why this anchor was revised (r1 → r2, 2026-07-25) — on the record

The r1 anchor drew the haft as a **2px-wide bar stepping one column per ~2.4 rows across 31
rows**, from a 14px Steel tine at top-right down past the hip, with a 2px Dark ferrule. It
quantized perfectly and read correctly in the south anchor. **It failed the rotation pass:**
in **north-east, north-west and west** the rotator bent the haft into a curve. At 48px a
2px line spanning 31 rows (15:1) carries no rigidity information, so a generator treats it as
a flexible cord; the heavy Steel blob on its end completed the read and those three frames
came back looking like a **flail**. That put a *curve* into 01's silhouette, which is exactly
variant 04's assigned carrier, and collapsed the 01×04 pair in the one place §5 had resolved
it.

The lesson, generalised for every future spec in this project: **a rotator infers rigidity
from aspect ratio and terminals, not from the word "haft".** Any held object that must stay
straight through a rotation pass should be specced with (a) a minimum thickness in px, (b) a
maximum length in px, and (c) hard terminals — and the disjointness audit should record those
as *numbers*, so a rotated frame can be checked against them mechanically. The r1 audit
entry described the carrier ("a straight thin line under the head"); a description is not
checkable, and the rotation is what proved it.

Not changed, deliberately: the head, torso, legs, eye notches and rag seam are pixel-identical
to r1, which held in all eight directions. `value_dominance` stays `bone` and `pale_usage`
stays `none`.

*Authored-anchor block index: **0*** — `py Scripts/art/pixelpipe.py authored soldier-01`

```
................................................
................................................
................................................
................................................
................................................
................................................
.....................#bbb#......................
....................#bbbbb#.....................
................#bbbbbbbbbb#....................
..............#bbbbbbbbbbbb#....................
.............#bbbbbbbbbbbbb#....................
.............#bbbbbbbbbbbbb#....................
.............#bbbbbbbbbbbbb#.............SSS....
.............###############.............SSS....
.............#bbbbbbbbbbbbb#............SSS.....
.............#bbbbbbbbbbbbb#...........SSS......
.............#bbb##bbb##bbb#..........bbb.......
.............#bbb##bbb##bbb#.........bbb........
.............#bbbbbbbbbbbbb#........bbb.........
.............#bbbbbbbbbbbbb#.......bbb..........
.............#bbbbbbbbbbbbb#......bbb...........
..............#bbbbbbbbbbb#......bbb............
...............###########......bbb.............
............#bbbbbbbbbbbbbb#...bbb..............
............#bbbbbbbbbbbbbb#..bbb...............
............#bbbbbbbbbbbbbb#.bbb................
............#bbbbbbbbbbbbbb#bbb.................
............#bbbbbbbbbbbbbb###..................
............#bbbbbbbbbbbbbb#....................
............#bbbbbbbbbbbbbb#....................
............#bbbbbbbbbbbbbb#....................
............#bbbbbbbbbbbbbb#....................
............#bbbbbbbbbbbbbb#....................
............################....................
..............#bbb#..#bbb#......................
..............#bbb#..#bbb#......................
..............#bbb#..#bbb#......................
..............#bbb#..#bbb#......................
..............#bbb#..#bbb#......................
..............#bbb#..#bbb#......................
..............#bbb#..#bbb#......................
..............#####..#####......................
..............#####..#####......................
..............#####..#####......................
................................................
................................................
................................................
................................................
```

Content bbox **32 × 38 px** (x12–43, y6–43) inside the 48px cell. `pack` centres content on
one global bbox and **fails hard** rather than scaling, so this margin is required. The bar's
extreme tip is at x43; the global bbox across the set is still driven by 04's bell (x44) and
05's fuel bundle (x7), so packing is unaffected by this revision.

**Reading the bar, row by row, so the rotation can be checked against it:** Steel cap at rows
12–15 (rows 12–13 share the same three columns — that flat two-row top is the hard terminal),
Bone shaft rows 16–26 stepping exactly one column left per row, Dark grip at row 27 landing on
the torso outline. Fifteen rows of travel, fifteen columns of travel: **exactly 45°, no
inflection, no single-pixel wobble.** Highest carrier pixel = row 12; crown = row 6.

---

## 7. Variant 02 — Line infantry

**Reads as:** *the plain soldier the rest of the roster is measured against.*

**Carrier:** the dome-and-vertical. A plain Steel dome helm with a hard full-width Dark brim,
a flat shoulder line, and one dead-vertical Bone shaft with a small Steel head. **Nothing
leaves the outline sideways.** This is the only variant that is allowed to be generic, and
that is its entire function: it is the neutral against which "more" (03), "less" (01),
"other job" (04, 05) and "wider" (06) are legible.

**Head signature:** the dome — the one kept dome in the set. `soldier-style-depth-test.md`
finding 7 notes that a generated dome quantizes to *Bone* and steals the eye; here it is
authored **Steel** with a Dark brim beneath it, which puts the pale-mid value back on the
face where it belongs and turns the dome into a dark cap rather than a highlight.

**Value reasoning.** Steel 38.6%, Bone 34.3%, Dark 27.2%, Pale 0. Declared `mixed`, and
genuinely mixed: no value clears 40% and the top two are 4.3 points apart. This is
deliberate — **the baseline unit should have no dominance claim to spend**, so that both
directions along the ladder (01 down, 03 up) are readable as departures. Steel edges ahead
only because a helmet plus a mail skirt plus a spear head is exactly "half-kitted", which is
the fiction. The torso is Steel with Bone sleeves and the legs are bare below a Steel skirt,
so the sprite alternates value bands vertically — helm, face, jack, skirt, shins, boots — and
that banding is what keeps a symmetric silhouette from reading as a texture in a crowd.

**At 500 units this reads as:** a picket fence. Regular top edge, regular verticals, even
spacing — the one variant whose horde read is *rhythm* rather than shape. That is a real
gameplay signal (this is the rank that is holding), and it is also the read this set most
needs to keep away from any future uniformed enemy register: 02 is regular but **not**
Dark-dominant, and the reservation in §3a says an enemy must be Dark-dominant, or regular,
or both. 02 takes only one of the two.

*Authored-anchor block index: **1*** — `py Scripts/art/pixelpipe.py authored soldier-02 --index 1`

```
................................................
................................................
................................................
................................................
..................................##............
..................................SS............
.................................SSSS...........
....................########.....SSSS...........
...................#SSSSSSSS#.....SS............
..................#SSSSSSSSSS#....##............
.................#SSSSSSSSSSSS#...bb............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
...............#SSSSSSSSSSSSSSSS#.bb............
...............##################.bb............
................#bbbbbbbbbbbbbb#..bb............
................#bbb##bbbb##bbb#..bb............
................#bbb##bbbb##bbb#..bb............
................#bbbbbbbbbbbbbb#..bb............
................#bbbbbbbbbbbbbb#..bb............
.................#bbbbbbbbbbbb#...bb............
..................############....bb............
..............#bbbSSSSSSSSSSSSbbb#bb............
..............#bbbSSSSSSSSSSSSbbb#bb............
...............#bbSSSSSSSSSSSSbb#.bb............
...............#bbSSSSSSSSSSSSbb#.bb............
...............#bbSSSSSSSSSSSSbb#.bb............
...............#bbSSSSSSSSSSSSbb#bbb............
...............#bbSSSSSSSSSSSSbb#bbb............
...............#bbSSSSSSSSSSSSbb#.bb............
...............#bbSSSSSSSSSSSSbb#.bb............
...............##################.bb............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
.................#bbbb#..#bbbb#...bb............
.................#bbbb#..#bbbb#...bb............
.................#bbbb#..#bbbb#...bb............
.................#bbbb#..#bbbb#...bb............
.................#bbbb#..#bbbb#...bb............
.................######..######...bb............
.................######..######...bb............
.................######..######...##............
................................................
................................................
................................................
................................................
```

Content bbox **22 × 40 px** (x14–35, y4–43).

---

## 8. Variant 03 — Veteran

**Reads as:** *the same soldier as the line, with more between them and the dark.*

**Carrier:** the stepped crown and the lost neck. Two helmet steps where 02 has one; a brim
that flares 2px wider on each side; pauldrons that rise flush with the jaw so there is **no
neck row at all**; a mail skirt that widens the hips below the belt. The spear, the grip
height and the stance width are identical to 02's — the tier is expressed only as *added
material at the edges*.

**Head signature:** a squared two-step helm. Where 02's crown curves in a 3-step dome, 03's
crown is a flat-topped rectangle sitting on a second flat-topped rectangle, then a hard
flared brim. At 16px the difference between a curved crown and a stepped one is one visible
notch on each shoulder of the skull — small, but it is the only difference a tier *can*
carry when colour is gone and the silhouette must stay recognisably the same unit.

**Value reasoning.** Steel 55.0%, Dark 27.9%, Bone 17.0%, Pale 0. The Bone collapse is the
point: the veteran's face is a *slot* between brim and pauldrons, not an open oval, so the
person value drops to a third of 02's while the metal value climbs by 17 points. Two things
are deliberately **absent** to protect this from the Dark-accumulation failure mode
(`soldier-style-depth-test.md` finding 1): there is **no cast shadow under the brim beyond a
single row**, and **no internal outlining between plates** — every Steel surface meets its
neighbour on a bare value boundary. Without that discipline a 55%-Steel design quantizes to
a 55%-Dark one.

**Face stays open — a deliberate rejection of the obvious tier signal.** The cheapest way to
say "veteran" would have been a closed visor, and it was rejected: a faceless friendly is one
step from the *regular, no-face, Dark-heavy* mechanism the reservation in §3a is holding for
enemies, and this game cannot afford that confusion. The veteran gets a narrower face, not no
face.

**At 500 units this reads as:** the heavy top edge in a mixed rank. Because 03 has no neck,
its shoulders and head merge into one wide block, so a line of veterans reads as a
*continuous* mid-grey bar where a line of 02s reads as separate heads on separate stalks.
That is a formation-level read achieved per-unit, which is exactly the Vanguard's
"formation shape is the identity" grammar (`CLASSES.md` §1).

*Authored-anchor block index: **2*** — `py Scripts/art/pixelpipe.py authored soldier-03 --index 2`

```
................................................
................................................
................................................
................................................
..................................##............
..................................SS............
....................########.....SSSS...........
...................#SSSSSSSS#....SSSS...........
...................#SSSSSSSS#.....SS............
................#SSSSSSSSSSSSSS#..##............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
................#SSSSSSSSSSSSSS#..bb............
..............#SSSSSSSSSSSSSSSSSS#bb............
..............####################bb............
.................#bbbbbbbbbbbb#...bb............
.................#bb##bbbb##bb#...bb............
.................#bb##bbbb##bb#...bb............
.................#bbbbbbbbbbbb#...bb............
.................#bbbbbbbbbbbb#...bb............
.................##############...bb............
............#SSSSSSSSSSSSSSSSSSSS#bb............
............#SSSSSSSSSSSSSSSSSSSS#bb............
............#SSSSSSSSSSSSSSSSSSSS#bb............
..............#SSSSSSSSSSSSSSSS#..bb............
..............#SSSSSSSSSSSSSSSS#..bb............
..............#SSSSSSSSSSSSSSSS#..bb............
..............#SSSSSSSSSSSSSSSS#bbbb............
..............#SSSSSSSSSSSSSSSS#bbbb............
..............#SSSSSSSSSSSSSSSS#..bb............
..............##################..bb............
.............#SSSSSSSSSSSSSSSSSS#.bb............
.............#SSSSSSSSSSSSSSSSSS#.bb............
.............#SSSSSSSSSSSSSSSSSS#.bb............
.............#SSSSSSSSSSSSSSSSSS#.bb............
.............#SSSSSSSSSSSSSSSSSS#.bb............
.............####################.bb............
................#SSSS#..#SSSS#....bb............
................#SSSS#..#SSSS#....bb............
................#SSSS#..#SSSS#....bb............
................#SSSS#..#SSSS#....bb............
................######..######....bb............
................######..######....##............
................######..######..................
................................................
................................................
................................................
```

Content bbox **24 × 41 px** (x12–35, y4–44).

---

## 9. Variant 04 — Rally caller

**Reads as:** *a head with a horn growing out of it - the only shape that breaks the crowd's top edge.*

**Carrier:** the upswept curve above the head. A thick bent tube springing from the mouth,
bending from diagonal to near-vertical as it rises, ending in a flared Steel bell **three
rows clear of the crown**. It is the only curve in the roster and the only object that
breaks a silhouette *upward*.

**Head signature:** a bare shaved skull. No headgear, **no brim row**, and the crown itself is
drawn as a hard flat Dark cap rather than a curved highlight — so even though a skull is
technically dome-shaped, it does not read as a helmet, because the helmet read at 16px is
carried by the brim, not by the curve (see §6). The horn's mouthpiece touches the skull's
outline at jaw height, so head and horn read as one silhouette rather than a person holding
a thing.

**Value reasoning.** Bone 63.2%, Dark 31.8%, Steel 5.1%, Pale 0. A caller is congregation,
not command: the body is cloth and skin, and the only worked metal is the bell rim and
mouthpiece — 31 pixels, placed at the two ends of the horn so the curve is bracketed by hard
value at both terminals and reads as a made object rather than as an animal horn. The one
internal value line is a **Steel baldric on a 2px diagonal across the torso**, which does two
jobs: it gives a Bone-dominant torso an internal edge so it does not read as a blank, and it
echoes the horn's diagonal so the sprite has a consistent axis. Dark at 31.8% is the highest
in the set outside 05 and is entirely contour, eye notches, crown cap, belt and boots — again,
**no filled Dark mass**.

**At 500 units this reads as:** hooks in the skyline. A rank of ordinary units has a flat-ish
top edge; every caller in it punches a curved shape 8–10px above that line, and because the
curve is opaque and 4px thick it survives the collapse where a flag would not. In practice a
player should be able to count their callers by scanning the top edge of a crowd without
looking at any individual sprite.

**Risk noted, not waved away:** `CLASSES.md` §3 gives the Pathfinder "the only curve in the
hero row" (their bow arc). This is a mass-unit curve, opaque and head-attached, versus a
hero-scale thin drawn arc held out from the body — different scale, different thickness,
different attachment point. It is nonetheless a nudge at a stated reservation and is raised
in §16.3.

*Authored-anchor block index: **3*** — `py Scripts/art/pixelpipe.py authored soldier-04 --index 3`

```
................................................
................................................
................................................
................................................
................................................
...................................##########...
...................................#SSSSSSSS#...
....................................#SSSSSS#....
..................##########..........#bb#......
................#bbbbbbbbbbbb#........#bb#......
...............#bbbbbbbbbbbbbb#.......#bb#......
...............#bbbbbbbbbbbbbb#......#bb#.......
...............#bbbbbbbbbbbbbb#.....#bb#........
...............#bbbbbbbbbbbbbb#....#bb#.........
...............#bbbbbbbbbbbbbb#...#bb#..........
...............#bbb##bbbb##bbb#..#bb#...........
...............#bbb##bbbb##bbb#.#bb#............
...............#bbbbbbbbbbbbbb##bb#.............
...............#bbbbbbbbbbbbbb##bb#.............
...............#bbbbbbbbbbbbbb#SSS..............
................#bbbbbbbbbbbb#..................
.................#bbbbbbbbbb#...................
..................##########....................
.............#bbbbbbbbbbbbbbbbbb#...............
.............#bbbbbbbbbbbbbbbbbb#...............
..............#bbbbbbbbbbbbSSbb#................
..............#bbbbbbbbbbSSbbbb#................
..............#bbbbbbbbSSbbbbbb#................
..............#bbbbbbSSbbbbbbbb#................
..............#bbbbSSbbbbbbbbbb#................
..............#bbSSbbbbbbbbbbbb#................
..............#SSbbbbbbbbbbbbbb#................
..............##################................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................#bbbb#..#bbbb#..................
................######..######..................
................######..######..................
................######..######..................
................................................
................................................
................................................
................................................
```

Content bbox **32 × 39 px** (x13–44, y5–43).

---

## 10. Variant 05 — Flame-tender

**Reads as:** *the only bright thing on the field that is not the bearer, and it is carried in an iron pot.*

**Carrier:** the caged bright. A 4×4 Pale core sitting inside a hard-rimmed iron firepot at
chest height, **bounded on all four sides**: Bone ember-halo left and right, Bone ember rows
above and below, Dark vessel wall outside that, Dark rim outside that. Sixteen Pale pixels,
2.4% of the sprite, and not one of them touches transparency.

**Head signature:** a peaked Steel hood with a deep Dark face cavity. The peak is a 4px
Dark cap — no dome, no brim. The face is a small Bone patch recessed inside a Dark socket,
which is the only place in the set where Dark appears *inside* a head, and it is what makes
the hood read as a hood rather than as a helmet.

**Why the bright is caged, and why that is a rule and not a detail.**
`palette.json.shape_carriers.point_halo` is registered to the **Lampbearer** — "lamps and
honest light" — and `vanguard.md` canon proposal 2 asks that only the Lampbearer's flame
*glows* while other classes' flames burn as shape. A fire-tending retinue unit is
semantically inside the honest-light channel (it is literally feeding the premise), so it
would be dishonest to give it a mark or a banner instead. The resolution is a shape rule
rather than a value rule:

> **Free light versus caged light.** The Lampbearer's carrier is a *free* point — a bright
> pixel with dither halo and no hard boundary, floating (wisps) or held high (lamp on a
> staff). A flame-tender's bright is *caged* — enclosed by a hard Dark rim on all four sides,
> at or below chest height, never above the head, never detached from a body, and never more
> than 16px.

At 2px that distinction survives: free light has a soft edge that fades into dither, caged
light has a black box around it. Raised as canon proposal §16.1 because it modifies a
registered carrier.

**Value reasoning.** Steel 43.4%, Dark 36.6%, Bone 17.7%, Pale 2.4%. Discussed in §3a: Steel
is chosen optically, to give the Pale core a field with a 0.59 luma gap instead of Bone's
0.31. Dark is unusually high for this set at 36.6% and every pixel of it is accounted for:
the hood's face cavity, the pot's rim and walls, the fuel-bundle seams, the belt and the
boots. It is high *because the bright needs a cage*, and it is still not dominant — 43.4%
Steel beats it by 6.8 points, so the sprite does not read as a Dark body with a light on it
(which would be a void-register read with a lamp, i.e. the exact misread §2 forbids).

**Dither note.** The Bone ember-halo is authored as **2×2 blocks**, not 1px stipple, per
`palette.json.dither.moving_min_block`. The quantizer would convert 1px stipple automatically,
but authoring it correctly means the anchor round-trips unchanged and nothing shimmers on a
moving quad.

**At 500 units this reads as:** the second-brightest thing on screen, always, and always at
waist height inside the crowd. The player's own flame is above and larger; tenders are small
sparks *within* the mass. A crowd with tenders in it looks like a mass with embers scattered
through it — which is the correct picture of a congregation that has learned to carry fire
in more than one hand.

*Authored-anchor block index: **4*** — `py Scripts/art/pixelpipe.py authored soldier-05 --index 4`

```
................................................
................................................
................................................
................................................
................................................
................................................
......................####......................
....................#SSSSSS#....................
...................#SSSSSSSS#...................
..................#SSSSSSSSSS#..................
.................#SSSSSSSSSSSS#.................
................#SSSSSSSSSSSSSS#................
................#SSS########SSS#................
................#SS##########SS#................
................#SS#bbbbbbbb#SS#................
................#SS#b##bb##b#SS#................
................#SS#b##bb##b#SS#................
................#SS#bbbbbbbb#SS#................
................#SS#bbbbbbbb#SS#................
.................#SSbbbbbbbbSS#.................
..................#SbbbbbbbbS#..................
...................#bbbbbbbb#...................
..................############..................
..............#SSSSSSSSSSSSSSSSSS#..............
.......#bbbb#.#SSSSSSSSSSSSSSSSSS#..............
.......#bbbb##SSSSSSSSSSSSSSSSSSSSSS#...........
.......#b##b##SSSSSSSSSSSSSSSSSSSSSS#...........
.......#b##b##SSSS##############SSSS#...........
.......#bbbb##SSSS###bbbbbbbb###SSSS#...........
.......#bbbb##SSSS###bbbbbbbb###SSSS#...........
.......#b##b##SSSS###bb@@@@bb###SSSS#...........
.......#b##b##SSSS###bb@@@@bb###SSSS#...........
.............#SSSS###bb@@@@bb###SSSS#...........
.............#SSSS###bb@@@@bb###SSSS#...........
.............#SSSS###bbbbbbbb###SSSS#...........
.............#SSSS##############SSSS#...........
...............#SSSSSSSSSSSSSSSS#...............
...............#SSSS#......#SSSS#...............
...............#SSSS#......#SSSS#...............
...............#SSSS#......#SSSS#...............
...............#SSSS#......#SSSS#...............
...............#SSSS#......#SSSS#...............
...............######......######...............
...............######......######...............
................................................
................................................
................................................
................................................
```

Content bbox **30 × 38 px** (x7–36, y6–43).

---

## 11. Variant 06 — Shield-heavy

**Reads as:** *a banded door with a pair of eyes over the top of it.*

**Carrier:** the sunken head behind the rectangle. A 28×25px door-shield held frontally —
wider than the figure and taller than its torso — with the head sunk so far behind it that
only the crown, brow and **eyes** clear the rim. Two splayed feet appear below the shield's
bottom edge and nothing else of the body is visible at all.

**The shield's internal structure — and the rule that governs it: only one thing on this
plane is allowed to float.** The rectangle now carries exactly four internal features, and
three of them are welded to its boundary:

| Feature | Rows | Why it survives rotation |
|---|---|---|
| Dark top rim | 16 | it *is* the rectangle's top edge |
| **Steel edge plate** | 17–19 | 3 rows of full-width Steel directly under the top rim, read as the slab's **thickness seen from a high camera**. This is the depth cue (below). |
| **The single iron band** | 28–31 | 4 rows of full-width Steel, unbroken across the plank seams, centred on the front face. **The only interior feature in the sprite.** |
| Dark bottom rim + Steel bottom rail | 39, 40 | both welded to the bottom edge |
| Two Dark plank seams | 20–27, 32–38 | 2px wide, **vertical** — vertical detail on a plane yaws stably; horizontal detail is what has to be repositioned |

r1 put **three** full-width Steel bands in the field, plus the two rim treatments, and the
rotation pass scrambled them at every intermediate angle (§11.1). r2 keeps the "banded door"
read with a *single* floating band, made **heavier** (4 rows against r1's 3) so that what is
left is unmissable, and moves the remaining Steel to the rims, where it cannot drift because
the rectangle's own edges pin it.

**Depth without tilt.** The 3-row Steel plate under the top rim is a real depth cue: from the
game's high top-down camera you *would* see the top edge of a slab held vertically, and
drawing it means the rotator does not have to invent a thickness when the figure turns. A
literal 3/4 tilt in the anchor was considered and **declined** — 06's identity is an
axis-aligned rectangle (§5, 02×06: "one is a picket and the other is a wall"), §13 requires
the shield to stay level through the walk cycle, and the horde read in §11 depends on ranks of
these stacking into an unbroken horizontal bar. A tilted anchor would trade the carrier to
save the banding. If the single band still scrambles on re-rotation, the tilt is the next
lever and it should be spent then, not now; see §11.1.

**Head signature:** placement, not shape. 06 keeps an ordinary dome, and then *hides two
thirds of it.* This is the inverse of variant 03: 03's head is the highest point of the sprite
relative to its shoulders, 06's is the lowest, and 06 is the only variant in the set with no
visible shoulders, no visible arms and no visible torso. The head is also the second
occlusion cue: it is cut off dead flat by the shield's top rim, which tells the rotator the
plane is *in front of* the body rather than strapped to it.

**Value reasoning.** Bone 37.8%, Steel 33.5%, Dark 28.7%, Pale 0 (332 / 294 / 252 of 878
opaque px) — declared `mixed`, the flattest histogram in the set by design, and **numerically
identical to r1**. That is not a coincidence: r2 rearranges the shield's rows without changing
the rectangle's dimensions or the ratio of Steel rows to plank rows (8 full-Steel rows and 15
plank rows in both revisions), so the dominance claim, the §3 table and the §3a distribution
all stand unchanged.

> ⚠ **Do not use the histogram to check whether this revision landed.** r1 and r2 have byte-
> identical value counts (878 / 252 / 294 / 332) by design, so `quantize` will report the same
> numbers before and after and a histogram diff is a **false negative**. The r1→r2
> discriminator is *row order*: in r1 the first plank row is **18** and the first full-Steel
> band starts at row **21**; in r2 the first plank row is **20** and the first band starts at
> row **28**. Equivalently — r1 has full-Steel runs of lengths 1, 3, 3, 1; **r2 has 3, 4, 1**.
> Count the Steel runs, not the Steel pixels.

06's identity is entirely silhouette, so it spends no dominance claim;
that leaves the dominance axis free for 01/04 (bone) and 03/05 (steel) to use without a fourth
competitor. The shield is a **door**: Bone planks (the wood), Steel ironwork (one band plus
two rim plates), Dark seams (vertical, splitting the field into three planks) and a hard Dark
edge top and bottom. The one-band-plus-vertical-seam pattern is chosen specifically so it
cannot be confused with the Vanguard hero's shield, which carries a single horizontal *Bone*
band on a Steel field and nothing else (`vanguard.md` §4a) — 06 inverts that: a single Steel
band on a Bone field.

**The one thing this variant is not allowed to be: Dark-dominant.** A black slab would read
better as "the line is holding" than anything here does, and it was declined. The reasoning is
in §3a and the trade is stated in §16.2 for an owner to overturn if they want it.

**At 500 units this reads as:** a banded wall. A rank of these produces an unbroken horizontal
striped bar with a row of small pale heads on top of it — the only formation in the game that
reads as *architecture* rather than as people. r2 makes that stronger, not weaker: three thin
stripes per unit blur into texture at 16px, whereas one thick band at a fixed height across
every unit in the rank lines up into **one continuous horizontal line across the whole
formation**. That is the "hold the line" signal, delivered without a HUD, without colour, and
without spending the Dark reservation.

### 11.1 Why this anchor was revised (r1 → r2, 2026-07-25) — on the record

The r1 anchor gave the shield **three full-width Steel bands** (rows 17, 21–23 and 33–35) plus
a bottom rail, over a field of Bone planks. South and north rotated cleanly. **SE, east, NE,
NW and west did not:** the rotator repositioned the bands independently as the plane
foreshortened, they lost their shared alignment, and the flat plane stopped reading as one
surface — the shield came apart into what looked like a **crate**. The carrier ("a banded door
with a pair of eyes over the top of it") did not survive in five of eight frames.

Two causes, both worth generalising:

1. **A large flat frontal plane is the hardest subject in a rotation pass**, because nothing
   in a frontal view tells the generator which way the plane faces or how thick it is. If the
   anchor does not supply a depth cue, the rotator invents one per frame, and per-frame
   invention is exactly what "scrambled" looks like. r2 supplies one (the top edge plate).
2. **Interior detail on that plane scales the damage.** Every free-floating horizontal is
   another thing that can be misplaced, and misplaced horizontals on a rectangle read as
   *segmentation* — the plane splits. The fix is not "draw the bands better", it is **draw
   fewer things that are allowed to float**: one band, everything else pinned to an edge.

Stated as a rule for future specs: **on any large flat plane, count the features that are not
touching the silhouette's boundary. That count is your rotation risk.** r1's count was three;
r2's is one.

Not changed, deliberately: the shield's outer dimensions (28×25), its position, the head, the
eye notches, the feet, the vertical plank seams and the histogram are all identical to r1 —
r1's silhouette was never the problem, only its interior. `value_dominance` stays `mixed` and
`pale_usage` stays `none`.

*Authored-anchor block index: **5*** — `py Scripts/art/pixelpipe.py authored soldier-06 --index 5`

```
................................................
................................................
................................................
................................................
................................................
................................................
......................######....................
....................#SSSSSSSS#..................
...................#SSSSSSSSSS#.................
..................#SSSSSSSSSSSS#................
..................#SSSSSSSSSSSS#................
.................#SSSSSSSSSSSSSS#...............
.................################...............
..................#bbbbbbbbbbbb#................
..................#bb##bbbb##bb#................
..................#bbbbbbbbbbbb#................
..........############################..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........#Sbbbbbb##bbbbbbbb##bbbbbbS#..........
..........############################..........
..........#SSSSSSSSSSSSSSSSSSSSSSSSSS#..........
............########........########............
............########........########............
............########........########............
................................................
................................................
................................................
................................................
```

Content bbox **28 × 38 px** (x10–37, y6–43) — unchanged from r1.

**Row map, so the rotation can be checked against it:** row 16 Dark top rim · rows 17–19
**Steel edge plate** (the depth cue) · rows 20–27 planks · rows **28–31 the single iron band**
(4 rows, full width, crossing both plank seams unbroken) · rows 32–38 planks · row 39 Dark
bottom rim · row 40 Steel bottom rail. Plank seams are the 2px Dark columns at x18–19 and
x28–29 and appear **only** in plank rows. Eight full-width Steel rows and fifteen plank rows,
exactly as in r1 — which is why the histogram is unchanged.

---

## 12. Sheet layout

Identical for all six.

- **Cell:** 48×48px (locked). **Grid:** 4 columns × 1 row — both powers of two, per the
  Niagara SubUV rule in `ELVTR/SETUP-EDITOR.md` §3.5. **Texture:** 192×48, `T_Soldier_01` …
  `T_Soldier_06`, `/Game/Sprites/Units`, imported Nearest / NoMipmaps / sRGB on; material
  Unlit + Masked, RGB→Emissive, A→Opacity Mask.
- **Column order is not a choice — it is `SwarmSheet::CellForBits`.**
  `ELVTR/Source/ELVTR/Mass/SwarmFragments.h` decodes one cell per unit on the CPU with
  priority **hit > attack > walk-frame**, and `SETUP-EDITOR.md` §1 documents the row as
  `walk0, walk1, ATTACK, HIT`.

| Cell | Frame key | Shown when |
|---|---|---|
| 0 | `south.idle` | nothing happening. `FrameBit` is only set while `Velocity² > 100`, so **cell 0 is what a standing unit shows** — which is why the authored anchor (a plausible at-rest pose) goes here. |
| 1 | `south.walk*` | `FrameBit` set — the opposite-leg beat, alternating with cell 0 at `SwarmTuning::WalkAnimHz` |
| 2 | `south.attack*` | `AttackBit` / `SwingBit` |
| 3 | `south.hit*` | `HitFlashBit` — recoil, and it out-prioritises everything else |

**This batch fills cell 0 only.** `output.frame_map` maps `"0": "south.idle"`; cells 1–3 stay
transparent until walk / attack / hit are commissioned. That is deliberate — the stage-D
human gate has not passed on any of these six, and everything downstream inherits the anchor.
Do not spend generations on animation before an anchor is approved
(`anchor.approved: false` on all six).

**One direction only, and that is a sim fact, not a shortcut.** The swarm has no direction
column: `SwarmProcessors.cpp` sets `SwarmAnim::FlipBit` from `Velocity.X < 0` and the camera
is a straight-down ortho with `Facing Mode = Face Camera`. Eight rotations will be generated
(v3 always returns 8) and **retained** under the retention rule; only south is packed.

**Mirroring is free variation for the asymmetric variants.** `FlipBit` mirrors in X, which
doubles the apparent roster for 01, 04 and 05 at no cost, and is a near no-op for 02, 03 and
06. The one thing no variant may do is make its carrier depend on *which* side it lands on —
checked, and none do: 01's haft, 04's horn and 05's pot all read identically mirrored.

**Composite note.** These six deliberately do **not** target `T_Swarm_2bit`. That atlas is
assembled by the composite request `swarm-units.json`; whichever of these six ship as mass
units should be pulled in there as rows, exactly as `retinue-militia.md` §6 describes.

---

## 13. Animation notes, and light

**No animations are requested in this batch** (`animations: []` on all six). The reason is
finding 2 of the depth test: the identity carriers here are unusual shapes that a generator
will "tidy up" given any excuse, and every animation frame inherits the anchor. Approve the
six anchors at the stage-D gate first, then commission motion. When it is commissioned:

| Variant | What its motion must communicate | Frame budget note |
|---|---|---|
| 01 Volunteer | a **shuffle** — no anticipation, no lean. A person keeping up, not marching. | lowest budget in the set; **the haft stays 45°, straight and 3px thick in every frame** — the angle must not change or the carrier flickers, and any bend repeats the r1 rotation failure (§6.1) |
| 02 Line infantry | a **march** — hard two-beat, visible ground contact, even cadence | `CLASSES.md` §1: the Vanguard's ranks get their identity from *formation shape*, not per-unit animation |
| 03 Veteran | the same march, **slower and heavier** — one fewer step per cycle at the same distance | the tier must be legible in cadence as well as silhouette |
| 04 Rally caller | the horn must **not** move relative to the head; the body does the walking | if the bell drifts, the top-edge read (§9) breaks and the variant loses its only carrier |
| 05 Flame-tender | the pot stays **level** through the whole cycle; the body absorbs the motion | a bobbing bright is a Lampbearer wisp read; a level bright is a carried fire |
| 06 Shield-heavy | the shield stays **axis-aligned and level**; only the feet move | any shield tilt reads as a kite or a round shield and destroys the rectangle. **The iron band must stay at the same row in every frame** — a band that rises and falls with the walk turns the rank's continuous horizontal line (§11) into a ripple |

**Playback:** the walk flip runs at `SwarmTuning::WalkAnimHz` (~4–5 fps reads as footfalls,
not vibration). SubUV index selection is a hard cut — do not interpolate.

**Light: full value inside the circle, dimmed one value outside.** FLAME-FOUNDATION §3a makes
the leash visible — inside, lit floor and units at full value; outside, dark ground and units
one value down the ramp. All six are authored as the **inside-the-circle** state, which is
what §3's histograms describe. The outside state is `palette.json.dim_shift`
(bone→steel, steel→dark, dark→dark, pale→bone) applied at render time as a 4-entry lookup
driven by one scalar on the Unlit+Masked material.

Three consequences worth stating because they are load-bearing:

1. **The ladder inverts in the dark, and that is correct.** Under `dim_shift` the Bone-heavy
   volunteer (01) loses 68.7% of its pixels to Steel and collapses hardest; the Steel-heavy
   veteran (03) loses 55% to Dark and nearly vanishes into the ground state. Both are
   sympathetic reads — the new arrival greys out, the veteran disappears — and neither
   requires a second texture.
2. **The 1px Dark contour is what keeps a dimmed unit visible.** Once dimmed, a unit is
   Steel-and-Dark on a Dark floor and the *silhouette* is carrying the entire read. That is
   the horde-scale case that justifies spending 27–37% of every sprite on contour.
3. **05's Pale survives one dim step and then dies.** `dim_shift` maps pale→bone, so a
   flame-tender outside the circle still shows a Bone core against Steel walls — a fire
   guttering, not out. That is a free, correct piece of storytelling from a value remap.
   No variant needs `light_shift_variant`; the brighter-by-one sheet has no state to
   represent for any of them.

---

## 14. Buildability checklist (stage-D gate)

Items 1–4 pass by construction on an authored anchor — check them anyway, because a hand edit
to a pixel map silently changes them. Items 5+ are what to check on the **rotation pass**,
which is where a generated frame drifts.

- [ ] Exactly 48 lines of exactly 48 characters in each block; only `.#Sb@`; nothing else in
      the fence. (`pixelpipe.py authored` warns "no block is exactly 48x48" if this fails.)
- [ ] `quantize --stage anchor` reports **100% on-palette** and alpha strictly 0 or 255.
- [ ] Reported `dominant` matches `canon.value_dominance` for 01, 03, 04, 05. For 02 and 06
      (`mixed`) the check is skipped by design — verify by eye that no value clears 40%.
- [ ] **Zero Pale pixels on 01, 02, 03, 04, 06.** Any pale pixel = reject.
- [ ] 05 has exactly 16 Pale pixels, contiguous, bounded on all four sides, below head height.
- [ ] Every eye in the set is a Dark notch. No pale eyes, anywhere, ever.
- [ ] Each variant's carrier is present and unmistakable (§3 column 3).
- [ ] No dither block smaller than 2×2; no frame that is mostly stipple.
- [ ] No filled Dark body mass on any variant; Dark is contour, seam, recess, rim only.
- [ ] Content bbox ≤ 48×48 in every packed frame.

### 14a. Rotation-pass acceptance, per carrier — added in r2

The r1 rotation produced 48 frames at 93.3–100% on-palette with **zero automated findings**,
and two of the six variants had still lost their carrier. The palette check cannot see a
carrier; these can. Check them **per frame, all eight directions**, not on the anchor.

| Variant | Test | Reject if |
|---|---|---|
| **01** | the haft's centreline is straight | the bar bends, arcs or droops anywhere along its run — measure the centreline against a straight line between its two terminals; **>1px deviation at any point = reject** |
| **01** | the haft's thickness | any span of the bar drops below 3px, or the bar tapers toward either end |
| **01** | the crown line | any pixel of the haft or its cap sits at or above the top of the head. **This is a hard reject**, not a note: it is 04's carrier |
| **01** | the cap | the Steel tip grows into a blob, a blade, an axe head or a ball. A tip wider than ~4px reads as a flail head |
| **04** | the crown line, inverted | the bell stops clearing the crown, or the horn straightens |
| **05** | the cage | any Pale pixel touches transparency, rises above chest height, or detaches from the body |
| **06** | the band count | **more than one** free-floating horizontal band appears on the shield face, in any frame |
| **06** | the band's continuity | the single band breaks into segments, steps, or fails to cross the full width of the visible shield face |
| **06** | the plane | the shield reads as separate boards, slats or a crate rather than one surface; the top edge plate disappears or wraps around a curve |
| **06** | the head | the head rises clear of the rim, or shoulders/arms/torso become visible |
| **all** | value dominance | the rotated frame's dominant value differs from `canon.value_dominance` |

**Known failure modes, in order of likelihood:**

- **A long thin held object being treated as flexible.** *This is the r1 failure on variant 01
  and it is now first on this list because it actually happened.* A rotator infers rigidity
  from **aspect ratio and terminals**, not from the noun in the prompt. Anything above roughly
  10:1 with soft ends will be arced somewhere in the eight frames. Fix in the anchor
  (thicker, shorter, hard terminals), never in the prompt alone. See §6.1.
- **A large flat frontal plane coming apart at intermediate angles.** *This is the r1 failure
  on variant 06.* Frontal planes carry no depth information, so the rotator invents thickness
  per frame, and every free-floating interior feature is another thing it can misplace. Fix in
  the anchor: supply a depth cue, and reduce interior features to one. See §11.1.
- **The generator tidying the sprite up.** Matched armour, a symmetric silhouette, a real
  helmet, a proper spear on 01, a normal round shield on 06 — all are "improvements" that
  destroy the subject. This is finding 2 of the depth test and it will happen. Re-roll the
  rotation; keep the anchor.
- **Dark dominance on the rotation pass.** Shadow, outline and every crevice land on Dark.
  If a rotation comes back Dark-dominant the cause is heavy internal outlining or a cast
  shadow — both must be *absent*, not merely un-requested.
- **The horn flattening into a slab on rear-facing rotations** — the documented v3 weakness
  with protruding geometry. Did **not** occur in the r1 rotation of variant 04, which held in
  all eight. Still worth watching if 04 is ever re-rolled.

**One structural note for the re-rotation.** Both regressions were in the *interior or the
proportions* of a subject whose south anchor was correct, which means the anchor is where the
fix has to live — the prompt edits in `soldier-01.json` / `soldier-06.json` reinforce the
authored geometry, they do not substitute for it. If either carrier fails again on
re-rotation, the next lever for 01 is shortening the bar further (16px, 5:1) and for 06 it is
the authored 3/4 tilt declined in §11 — in that order, and one at a time, so it stays clear
which change bought the result.

---

## 15. Depends on

- **GDD #5 (sprite flipbooks vs. flat-shaded 3D): assumes flipbooks on instanced quads,
  loudly.** Every deliverable here is a SubUV flipbook cell whose order is dictated by
  `SwarmSheet::CellForBits`, packed for camera-facing Niagara sprite billboards. Under
  flat-shaded 3D these six sheets are meaningless: rotation becomes free, the four animation
  columns become skeletal clips, and the `dim_shift` remap in §13 moves from a material
  lookup to a per-object palette constant. **The silhouette, carrier and value rules in
  §3–§11 survive either answer; the sheets do not.** Recommendation unchanged and reinforced
  by this batch: flipbooks. Three of these six (01's haft, 04's horn, 06's oversized shield)
  are silhouettes that only stay disjoint because they are *drawn*, not solved — a 4-value
  flat ramp on a rotating mesh would give the horn a shaded interior and the shield a
  foreshortened rim the instant the camera moved, and both carriers would die.
- **GDD #6:** resolved (strict global palette, 2026-07-12). Not re-litigated. Note `GDD.md`
  §12 row 6 still reads "Per-faction | Open" — already raised as canon proposal 4 in
  `vanguard.md`.

---

## 16. Canon proposals

**1. Split the `point_halo` carrier into *free* and *caged* light — RATIFIED BY OWNER
2026-07-25. Binding.** The owner ruled *"caged light is fine"*: contained fire in a vessel is
permitted for flame-tending retinue units, and free/haloed light remains Lampbearer-only, on
exactly the terms proposed below. Variant 05 stands as designed and was rotated on this
ruling. The blocking question is closed; the text below is now the rule, not a request.

`palette.json.shape_carriers.point_halo` is registered to
the Lampbearer ("lamps and honest light"), and `vanguard.md` canon proposal 2 asks that only
the Lampbearer's flame glows. Variant 05 is a fire-tending retinue unit: it is inside the
honest-light channel by definition, but giving it the Lampbearer's carrier unqualified would
make a mass unit counterfeit a hero class at 2px. Proposed addition to
`palette.json.shape_carriers`:

> `point_halo` (Lampbearer) is **free light**: a bright core with dither halo and no hard
> boundary — floating, or held above the head. `caged_light` is a new sub-carrier available to
> fire-tending retinue units: a bright core **enclosed by a hard Dark rim on all four sides**,
> at or below chest height, never above the head, never detached from a body, ≤16px. Free
> light is a light source; caged light is fuel being carried. They are disjoint at 2px because
> one has a black box around it and the other does not.

*(The alternative — that no non-Lampbearer unit may spend Pale at all, which would have made
variant 05 void rather than amendable — was considered and rejected by the owner.)*

**2. Confirm (or overturn) the "no Dark-dominant friendly" reservation.** `retinue-militia.md`
§11.2 proposes as a stopgap that *Bone-dominant and irregular is the player's side; enemies
take Dark-dominant, or regular, or both.* This spec **honours it in full** — zero of six
variants is Dark-dominant, which cost variant 06 its most natural design (a char-blackened
slab shield, which would read as "the line is holding" better than the banded door does).
That is a real price paid for a reservation that has never been ratified, and it should be
either ratified or released:

- **If ratified:** every future friendly spec is bound by it and 06 stays as drawn.
- **If released:** 06 should be redrawn Dark-dominant, and this spec's §3a dominance
  distribution becomes bone / mixed / steel / bone / steel / **dark**, which is a strictly
  better spread.

This is not resolvable by art. It needs the answer to FLAME-FOUNDATION §4.5 — *does the dark
have monsters, or is the dark itself the enemy?* — because if the dark itself is the enemy
there may be no enemy sprites to be disjoint from, and the reservation is free to release.

**3. The Pathfinder's "only curve" reservation needs a scope.** `CLASSES.md` §3 says the
Pathfinder's bow arc is "the only curve in the hero row." Variant 04's horn is a curve on a
mass unit, which is outside the letter of that reservation and inside its spirit. Proposed
clarification, one line in `CLASSES.md` §3: *the reservation is hero-scale and applies to
thin drawn arcs held clear of the body; opaque head-attached curves on retinue units are
outside it.* If the owner wants the reservation to be absolute, variant 04's carrier must be
rebuilt as an angular bent tube (two straight segments meeting at a hard elbow), which is
weaker but survivable — say so before generating, not after.

**4. Variant 01 and `retinue-militia.md` occupy the same rung and should not both ship.**
Both are the bottom of `CLASSES.md` §1's Liberated ladder, both are Bone-dominant, both are
irregular, and both are the game's highest-population sprite. They differ in fiction —
*mismatched* (militia: four quadrants of looted gear, Bone 40.3 / Steel 30.7) versus
*destitute* (01: one body value, a rag, a farm tool, Bone 68.7 / Steel 2.2) — and the
destitute read serves the flame premise better, because it says these people brought nothing
and came for the light. But that is an owner call, not an art call, and shipping both would
put two near-identical pale ragged silhouettes in the same crowd. Recommended: **01 replaces
`unit-retinue`, and `retinue-militia.md`'s patchwork mechanism is promoted to the *second*
rung** — i.e. it becomes what a volunteer looks like after one floor of scavenging, sitting
between 01 and 02 as a fourth ladder step. That costs one extra sprite and buys a four-rung
ladder with a clean value ramp (Steel 2.2 → 30.7 → 38.6 → 55.0).

**5. Current canon still names no faction for these six, and that is now a measurable gap.**
`subject.faction` is `"none"` on all six, correctly (FLAME-FOUNDATION §5). But §5's
disjointness audit can only guarantee that these six are disjoint *from each other and from
the hero* — it cannot guarantee they are disjoint from an enemy, because there is no enemy.
Repeating `retinue-militia.md` §11.2's ask with one addition: when the prototype answers
FLAME-FOUNDATION §4.5, the first art-facing output should be **a value-dominance claim and a
silhouette-regularity claim for the primary threat** — nothing more, no name, no lore — so
that this table becomes checkable rather than aspirational.

**6. Add two *rotation-survivability* rules to the shared silhouette model — raised in r2.**
`docs/art/npc-silhouette-brief.md` is the live three-way disjointness model (silhouette ×
value dominance × pale usage) and it is entirely a *statement about one frame*. The r1
rotation of this set produced two carriers that were disjoint in the anchor and not disjoint
after rotation, at 100% on-palette with zero automated findings — so a carrier can pass every
existing check and still be lost. Proposed addition to that brief, as two lines any future
spec is audited against:

> **(1) Rigidity.** A carrier that must read as a straight held object is specced with a
> minimum thickness, a maximum length and a hard terminal at each end. Above roughly **10:1**
> length-to-thickness with soft ends, a rotation pass will bend it, and a bent line is a
> *curve* — a different registered carrier. Aspect ratio is part of the carrier, not styling.
>
> **(2) Floating features on a flat plane.** On any large frontal plane, count the interior
> features that do not touch the silhouette's boundary. **That count is the rotation risk**,
> and it should be one. Everything else moves to an edge, and the anchor supplies its own
> depth cue (a visible edge thickness) so the rotator does not invent one per frame.

Also proposed: the three-way disjointness table gains a fourth, cheap column — **"survives
rotation: y/n, checked against which frames"** — because until a carrier has been rotated,
its entry in that table is a hypothesis. This spec's §14a is the working form of that check;
if it holds on the re-rotation it should be promoted out of here and into the brief, so it
is not this document's private discipline.

*(This is a proposal against a document this spec does not own; §14a is written locally in
the meantime so the re-rotation is not blocked on it.)*
