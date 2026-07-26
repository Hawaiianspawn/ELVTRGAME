# Vanguard — hero sprite

**Subject:** the Vanguard hero (playable class avatar), not the retinue ·
**Cell:** 48×48 (locked, owner decision 2026-07-25) · **Sheet:** 4×4 → 192×192 ·
**Request:** `../data/art/requests/hero-vanguard.json` · **Texture:** `T_Hero_Vanguard`

**Binds to:** `docs/narrative/FLAME-FOUNDATION.md` (premise, §3a leash-as-light) ·
`docs/art/aesthetic-direction.md` (Direction A locked, global 4-value ramp) ·
`docs/art/npc-silhouette-brief.md` (three-way disjointness model, chibi ruling) ·
`CLASSES.md` §1 (Vanguard reads as geometry; banner = 2-value flip) ·
`docs/data/art/palette.json` (`shape_carriers.rectangle_flip` → Vanguard, banners only) ·
`ELVTR/SETUP-EDITOR.md` (Nearest, NoMipmaps, Unlit+Masked, Niagara SubUV).

**Deliberately does NOT bind to:** `docs/art/hallam.md`, `docs/narrative/hallam.md`,
`docs/art/hero-palettes.md` §1–§2 (void), `WORLD.md` (superseded in full). See
§10 for what those documents got wrong and what should replace them.

---

## 1. Intent

**Fiction.** The Vanguard is a *bearer*: the only light in a pitch-dark world, and the
thing an army is standing inside. Its flame is not carried in a lantern — it is carried
on a **standard**. The people around it are not troops taking orders, they are a
congregation keeping station in the one bright rectangle on the field. The sprite has to
sell "the light has a face and it is holding a flag," not "here is a knight."

**Gameplay.** Three jobs, in priority order:

1. **Find yourself instantly.** In a 500-unit press, the player must locate their own
   hero in under a glance. The Vanguard hero is the only unit-scale sprite in the game
   permitted a *large contiguous Pale area* — the banner. Everything else spends Pale in
   points, dots, or thin contours.
2. **Read as the class.** `CLASSES.md` §1: the Vanguard is **geometry** — straight lines
   and rectangles. The hero must be the origin of that language, not an exception to it:
   a flat top shoulder line, a squared shield, a dead-vertical pole. When ranks form up
   behind it, the hero's outline should look like the first brick of the wall.
3. **Stay disjoint from its own retinue.** The Liberated militia
   (`npc-silhouette-brief.md` (a)) is ragged, Bone-dominant, zero Pale. The hero must be
   its exact inverse on all three audit axes (see §4c) so that "which one is me" never
   depends on size alone.

---

## 2. Palette

Global ramp, no exceptions, no swaps (`aesthetic-direction.md` 2026-07-12 reset; GDD #6
resolved to strict global palette).

| Hex | Name | Role on this sprite |
|---|---|---|
| `#211e20` | Demichrome Dark | outline, helm rim, eye recesses, belt, boots, **the banner pole** |
| `#555568` | Demichrome Steel | **dominant** — plate, pauldrons, torso, legs, shield field |
| `#a0a08b` | Demichrome Bone | face, hands, one straight band across the shield |
| `#e9efec` | Demichrome Pale | **banner cloth only** — one axis-aligned rectangle, nothing else |

Transparency is **not** a value: alpha is binary (0/255), Unlit+Masked, per
`palette.json.mask`. Anti-aliasing and partial alpha are the single most common silent
breakage — the quantizer hard-thresholds at 128.

**Reservations, stated so they can be audited:**

- **No faction value.** Current canon names no factions (FLAME-FOUNDATION §5), so nothing
  is reserved for one and nothing may be invented for one here.
- **Pale is spent as a rectangle, and only as a rectangle.** This is the Vanguard's
  registered shape-carrier (`palette.json.shape_carriers.rectangle_flip` — "banners
  only"). No Pale on armour, skin, weapon, or eyes. No halo dither anywhere on this
  sprite — halo-dither point-glow belongs to the Lampbearer and would misread as a second
  light source standing next to the first.
- **No mark value is sacrificed.** Rune dot-clusters (Relickeeper) and quarry contours
  (Pathfinder) are drawn *over* enemies by other classes and never appear on this sheet;
  they remain fully available because the Vanguard's own bright is shape-disjoint from
  both.
- **Bone is rationed.** Face, hands and one shield band only. Bone is what the militia is
  made of; letting it creep onto the hero's plate collapses §4c row 2.

**Light-shifted variant: not emitted.** A bearer is never outside its own light, so the
brighter-by-one sheet (`light_shift.map`) has no state to represent — the request sets
`light_shift_variant: false`. The state the Vanguard *does* need is the opposite one; see
§7 and canon proposal 3.

---

## 3. The south-facing frame — the style contract

Generation is anchor-first: one south-facing sprite is produced, quantized to the four
values, and fed back as the reference that generates all eight rotations *and* every
animation frame. **Everything on this sheet inherits this one frame.** So the following
are not preferences, they are the contract. If the south frame fails any of these at the
stage-D gate, re-roll the anchor; do not proceed to the rotation pass.

Must be true in the south frame:

1. **The shoulder line is flat and horizontal**, at least 18px wide, unbroken by neck,
   collar or cloth. This single edge is the class. If the anchor comes back with sloped
   or rounded shoulders, the whole sheet is a different class.
2. **The shield is a rectangle with a flat top edge** — axis-aligned, four straight
   sides, no kite point, no round boss breaking the outline. It reads as a held wall.
3. **The banner pole is dead vertical, 2px, Demichrome Dark**, running from above the
   flag down to a gripping hand at hip height. Vertical, not shouldered at an angle — a
   diagonal pole reads as a spear and the class loses its only unambiguous vertical.
4. **The flag is one axis-aligned Pale rectangle**, roughly 11×7px, clear of the head,
   attached along the pole. Four straight edges. No swallowtail, no torn hem, no motion
   curve, no device stitched on it — at 48px a device becomes noise and at horde zoom it
   becomes a grey smudge, which costs the beacon.
5. **The face is open and visible** — Bone, no visor, no crest, no plume. Eyes are two
   Dark 2×2 recesses, *never* Pale. (A bearer with bright eyes reads as a Quiet creature
   from `npc-silhouette-brief.md` (c), which is the worst possible confusion in this game.)
6. **Steel dominates.** Target histogram of opaque pixels: Steel ≥ 50%, Dark ~25%,
   Bone ≤ 12%, Pale ≤ 9%. The QC pass checks this against `value_dominance: steel`.
7. **No silhouette-breaking soft geometry**: no cape, no cloak, no tassels, no fringe, no
   loose straps. Everything that leaves the body outline must be a straight edge.
8. **Bilateral bulk.** The figure must read as symmetric mass with two asymmetric
   attachments (shield left, pole right). A silhouette whose read depends on a profile
   detail will not survive the rotation pass — north especially, where the face is gone
   and only the shoulder line, shield edge and flag remain.

**Dither:** none is required on this sprite, and none should be invented. If the
quantizer produces dither at a Steel/Bone boundary it must land as **2×2 blocks minimum**
(`palette.json.dither.moving_min_block`); 1px stipple shimmers on a moving quad. 1px
detail that is not stipple — the eye recesses, the pole, the shield band — is preserved
by design and is where the detail budget goes.

---

## 4. Silhouette guide

### 4a. South frame at cell scale (48×48)

Legend: `.` transparent (mask, not a value) · `#` Demichrome Dark · `S` Demichrome Steel ·
`b` Demichrome Bone · `@` Demichrome Pale

```
................................................
................................................
................................................
.................................##.............
.................................##.............
.................................############...
.................................##@@@@@@@@@@#..
.................................##@@@@@@@@@@#..
.................................##@@@@@@@@@@#..
.................................##@@@@@@@@@@#..
.................................##@@@@@@@@@@#..
.................................##@@@@@@@@@@#..
.................................##@@@@@@@@@@#..
..................#############..############...
.................#SSSSSSSSSSSSS#.##.............
.................#SSSSSSSSSSSSS#.##.............
.................#SSSSSSSSSSSSS#.##.............
.................#SSSSSSSSSSSSS#.##.............
.................#SSSSSSSSSSSSS#.##.............
.................#SSSSSSSSSSSSS#.##.............
.................##############.##..............
.................#SbbbbbbbbbbbS#.##.............
.................#SbbbbbbbbbbbS#.##.............
.................#Sbb##bbb##bbS#.##.............
.................#Sbb##bbb##bbS#.##.............
.................#SbbbbbbbbbbbS#.##.............
.................#SbbbbbbbbbbbS#.##.............
..............#####################.............
..............#SSSSSSSSSSSSSSSSSS##.............
.....###############SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSbb##.............
.....#bbbbbbbbbbbbb#SSSSSSSSSSSbb##.............
.....#bbbbbbbbbbbbb#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS##.............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS...............
.....#SSSSSSSSSSSSS#SSSSSSSSSSSSS...............
.....#SSSSSSSSSSSSS##############...............
.....#SSSSSSSSSSSSS#.SSSS..SSSS.................
.....#SSSSSSSSSSSSS#.SSSS..SSSS.................
.....#SSSSSSSSSSSSS#.SSSS..SSSS.................
.....###############.####..####.................
................................................
................................................
```

Content bounding box ≈ 42×43px inside the 48px cell — the packer centres content and
**fails hard** if any frame exceeds 48px rather than scaling it, so the anchor must leave
this margin.

Reading the block, top to bottom: floating Pale rectangle → 2px vertical Dark pole →
big helmed head with an open Bone face and two Dark eye recesses → **flat horizontal
shoulder line** → torso, with the held shield rectangle overlapping it on the left and a
single Bone band crossing the shield → belt line → two short square legs on Dark boots.

### 4b. Horde-zoom check (~12px, what the player actually sees at gameplay zoom)

```
....@@@@@...
....@@@@@...
.......#....
...####.#...
...#bb#.#...
.######.#...
.#S##SSS#...
.#S##SSS#...
.#b##SSS#...
.#S##SSS#...
.####S#S#...
............
```

**Reads as:** *a walking rectangle under a pale flag — the one straight-edged figure in
the crowd.*

**At 500 units this reads as:** the single bright rectangle floating above a crowd line of
ragged Bone-and-Steel militia; below it, a dark-edged block whose top edge is the only
horizontal straight line on screen. Even when the body is fully occluded by the retinue,
the flag clears the crowd and stays a hard-edged Pale rectangle — no other unit-scale
sprite in the game is allowed that shape at that size, so the read cannot be counterfeited
by an enemy or by another player's class.

### 4c. Disjointness audit

Per the three-axis model in `npc-silhouette-brief.md`. The hero must be disjoint from its
*own* retinue, which is the hardest case — same class, same fiction, same ramp.

| | Silhouette | Value dominance | Pale usage |
|---|---|---|---|
| **Vanguard hero** | rigidly geometric: flat shoulder line, rectangle shield, vertical pole | **Steel** (≥50%), Bone rationed to face/hands/one band | **one large Pale rectangle** (banner), no other Pale pixel |
| Liberated militia (its retinue) | ragged/irregular humanoid | Bone+Steel (light) | none at rest; Bannerman flag-flip only |
| Quiet-type void creature (if the dark gets bodies) | amorphous, non-geometric | flat Dark, zero internal modelling | bare eye-dot, no halo |
| Lampbearer hero | point-and-halo carrier | (its own spec) | point + halo dither |

All three axes disagree against the militia — that is the bar the brief sets for
readability under motion blur and partial occlusion. Against the Bannerman (the one
retinue unit that also spends a Pale rectangle) the separation is **area and altitude**:
the Bannerman's flag is a mini-flag, ≤4×3px, at head height and inside the crowd line; the
hero's is 11×7px and rides above it. If those two ever start colliding in playtest, shrink
the Bannerman — never the hero.

---

## 5. Sheet layout

- **Cell:** 48×48px. **Grid:** 4 columns × 4 rows (both powers of two, per the Niagara
  SubUV rule in `ELVTR/SETUP-EDITOR.md`). **Texture:** 192×192, `T_Hero_Vanguard`,
  imported Nearest / NoMipmaps / sRGB on, material Unlit+Masked, RGB→Emissive,
  A→Opacity Mask.
- **Encoding: `SubImageIndex = direction × 2 + walkBit`.** Walk is **bit 0** and direction
  is bits 1–3, which mirrors the reference `T_Swarm_2bit` encoding (walk frame = bit 0).
  Direction indices follow the pipeline's canonical order.

| Cell | Frame key | Cell | Frame key |
|---|---|---|---|
| 0 | `south.idle` | 8 | `north.idle` |
| 1 | `south.walk1` | 9 | `north.walk1` |
| 2 | `south-west.idle` | 10 | `north-east.idle` |
| 3 | `south-west.walk1` | 11 | `north-east.walk1` |
| 4 | `west.idle` | 12 | `east.idle` |
| 5 | `west.walk1` | 13 | `east.walk1` |
| 6 | `north-west.idle` | 14 | `south-east.idle` |
| 7 | `north-west.walk1` | 15 | `south-east.walk1` |

Row layout that produces: row 0 = south + south-west, row 1 = west + north-west,
row 2 = north + north-east, row 3 = east + south-east. Every cell is filled; no
transparent filler cells.

**Why all eight directions get a walk, instead of four directions getting a full stride.**
Sixteen cells buys exactly one of these:

- *(A)* 8 idles + a 2-frame stride on the four cardinals — diagonals fall back to the
  nearest cardinal and the player's own avatar faces 45° wrong whenever they move
  diagonally, which is most of the time in a twin-stick top-down game.
- *(B, chosen)* 8 idles + one walk pose per direction, played as a 2-frame flip against
  the idle. Facing is always correct; the cost is that the same leg leads every step.

At gameplay zoom the leading leg is 2–3px and the dominant motion cue is the whole-body
bob and the shield edge rising; a 45° facing error on the player's own hero is visible
from across the room. (B) also lands the class's own grammar: **the Vanguard's motion
language is a 2-frame flip** — the same device `CLASSES.md` §1 already assigns to the
Bannerman's flag. The hero marches on a two-beat.

**Upgrade path, costing zero extra generations.** The template produces `walk1`–`walk4`
per direction; this sheet packs `walk1` only, and the other three frames stay in
`RawArt/Renders/hero-vanguard/r1/` under the retention rule. A later revision can repack
the *same* generations as an 8×4 grid (8 cols × 4 rows = 384×192, both powers of two) for
a full 4-frame stride in all eight directions, with `SubImageIndex = direction + 8 × frame`.
Bump `revision`, edit `output.grid` and `frame_map`, re-pack. No re-generation.

---

## 6. Animation notes

| Action | Frames | Source | What the motion communicates |
|---|---|---|---|
| Idle | 1 per direction (8 total) | v3 rotation pass | **Stillness is the default state.** The bearer is the thing everyone is standing in; the hero stands and the army moves around it (FLAME-FOUNDATION §3c). No breathing loop — a hero that bobs at rest fights the "fixed point of light" read and costs 8 cells we do not have. |
| Walk | 2 per direction (idle ↔ `walk1`), 8 dirs | `walking-4-frames` template, 1 generation/direction | A **march**, not a jog: a hard two-beat with visible ground contact. Even cadence, no anticipation frames, no lean. The Vanguard advances a line; it does not dart (that is the Pathfinder's budget). |

**Playback:** ~4–5 fps on the walk flip (≈2 steps/sec) — slow enough that the two poses
read as deliberate footfalls rather than a vibration. Do not interpolate; SubUV index
selection is a hard cut, which is correct here.

**Banner behaviour.** The flag is a fixed Pale rectangle on this sheet — no per-frame
flap. `CLASSES.md` §1 promises a 2-value flip on Vanguard banners, and it is honoured
here as the *idle↔walk* body flip carrying the flag with it, not as a separate cloth
animation. A true flag flip (Pale rectangle alternating against a Dark rectangle) is worth
having, but it needs its own frame budget; it belongs to the **planted banner prop** — a
static world object dropped by Banner Slam — which should be its own request with its own
2×2 sheet, not smuggled into the hero's 16 cells.

**What deliberately gets no frames:** attack, hit-react, death. The hero's 55-DPS problem
is being redesigned (FLAME-FOUNDATION §3c) and animating a swing now would spend
generations on a kit that may not exist. Revisit after the prototype answers §4.1.

---

## 7. Light: full value inside, dimmed one value outside

FLAME-FOUNDATION §3a makes the leash visible: inside the circle, lit floor and units at
full value; outside, the dark ground state and units dimmed one value down the ramp.

**For this sprite the relationship is inverted, and that inversion is the point.** The
Vanguard *is* the source. It is never outside its own light, so it renders at **full
ramp, always** — it is the calibration reference for every other sprite on screen. When
the player looks at their own hero they are looking at what full value means; every dimmed
unit is legible as dimmed by comparison to it.

Two states this sheet must support without a second texture:

1. **Everything else dims, the hero does not.** Retinue outside the leash shift one value
   *down* (Bone→Steel, Steel→Dark, Pale→Bone). The hero stays put. Net effect at horde
   scale: as units drift out of the circle they lose contrast and the hero's Pale
   rectangle becomes progressively *more* the only bright thing on screen. The visual
   punishment for over-extending is that your army greys out around your flag. This is a
   renderer-side value remap — a 4-entry lookup driven by one scalar instance parameter on
   the Unlit+Masked material, not a second sheet and not per-unit material work.
2. **Guttering flame (upkeep failure).** If the fire is unfed (GDD §7 upkeep, "degrade,
   don't die"), apply that same one-step-down remap **to the hero itself**. The banner
   drops Pale→Bone and the player instantly loses the beacon they navigate by — the single
   clearest bad-news signal available in a 4-value game, and it costs one scalar. Needs
   the inverse map added to `palette.json`; see canon proposal 3.

Neither state requires the `light_shift_variant` brighter sheet, so the request sets it
false.

---

## 8. Buildability checklist (stage-D gate)

Reject the anchor and re-roll if any of these fail — they are cheap to check by eye
against §4a and they are what the QC pass asserts:

- [ ] Exactly four values present after quantization; alpha strictly 0 or 255.
- [ ] Pale appears **only** inside one contiguous axis-aligned rectangle. Any Pale pixel
      on skin, plate, weapon or eyes = reject (`pale_usage: banner`).
- [ ] Steel is the largest bucket (`value_dominance: steel`).
- [ ] Shoulder line is flat and horizontal for ≥18px.
- [ ] Shield has four straight sides and a flat top edge.
- [ ] Pole is vertical, ≤2px wide, Dark.
- [ ] Face is open (no visor), eyes are Dark recesses.
- [ ] No dither block smaller than 2×2 anywhere.
- [ ] Content bbox ≤ 48×48.

If the anchor is good but the rotation pass loses the flag on the north-facing frame
(a known failure mode — the pole tends to migrate behind the body), keep the anchor and
re-roll the rotation pass only; do not re-roll the anchor, since everything else already
inherits it.

---

## 9. Depends on

- **GDD #5 (sprite flipbooks vs. flat-shaded 3D): assumes flipbooks on instanced quads.**
  This spec is loudly dependent on that side. The entire deliverable is a SubUV flipbook
  sheet with a hand-authored frame-to-cell map and an 8-direction rotation set baked as
  pixels; under flat-shaded 3D the rotation set becomes free and this sheet becomes
  meaningless, while the value-remap trick in §7 would move from a material lookup to a
  per-object palette constant (still cheap, but a different implementation). The
  *silhouette* rules in §3 and §4 survive either answer; the *sheet* does not.
  Recommendation unchanged: flipbooks — the 2-frame march flip in §6 is only defensible
  as a deliberate style choice in a flipbook world, and a 4-value ramp on a 3D mesh will
  fight the "no shading" constraint the moment anything rotates.
- **GDD #6:** resolved (strict global palette, 2026-07-12). Not re-litigated here.
  Note that `GDD.md` §12 row 6 still reads "Per-faction | Open" — see canon proposal 4.

---

## 10. Canon proposals

**1. `CLASSES.md` §1 "The hero" block — replace entirely.** The current block is stale on
three independent axes: it cites a bright hex retired by the 2026-07-12 palette reset (the
"Roll-Gold" entry — the hex itself is deliberately not restated here, since
`pixelpipe.py validate` rejects any spec that cites a retired hex; it is recorded in
`palette.json.retired_hexes` with `still_cited_in: CLASSES.md:78`); it is written around a
named individual ("Hallam"), reversing the 2026-07-11 role-only ruling stated eleven lines
earlier in the same file; and it leans on the Still Legion's armour pattern and the
Gatecamp, both discarded by the 2026-07-22 narrative reset. Proposed replacement:

> ### The hero
>
> *Sprite spec: `docs/art/vanguard.md` · palette: `docs/data/art/palette.json`*
>
> - **Who:** no name — the role is the identity (see "Hero identities", above). A
>   bearer who chose to carry the flame where it is most needed rather than where it is
>   safest, and who counts the people who came back.
> - **The face:** open. Heavy-boned, deliberate, visible in every frame including the
>   portrait; an open-faced helm with no visor and no crest — *a banner needs a face
>   under it.*
> - **The light — the Standard:** this class's flame is carried on a **standard**, not
>   in a lamp. The banner cloth is the one large bright rectangle on the field, and it
>   is the only large bright area any unit-scale sprite in the game is permitted.
> - **Reads as:** *a walking rectangle under a pale flag — the one straight-edged figure
>   in the crowd.* Widest hero; flat horizontal shoulder line; dead-vertical 2px pole
>   (never reads as a spear); Steel-dominant plate against the Bone-dominant, ragged
>   militia it leads.
> - **2-bit readability:** geometry — straight lines and rectangles. The hero is the
>   first brick of the wall its retinue forms.

**2. The bearer's flame is expressed in each class's registered carrier shape — new
canon, needed by every hero spec, not just this one.** FLAME-FOUNDATION makes all four
classes bearers, but `palette.json.shape_carriers` assigns point-and-halo glow to the
Lampbearer alone. Left unresolved, every hero spec will want a glow pixel and the carrier
registry collapses. Proposed rule: *a bearer's flame is rendered in that class's own
registered carrier shape and no other — the Vanguard's flame is its banner rectangle, the
Relickeeper's is its rune cluster, the Pathfinder's is its mark contour, the Lampbearer's
is the point-and-halo. Only the Lampbearer's flame glows; the others burn as shape.* This
keeps the four brights mutually unforgeable at 1–2px and gives the Lampbearer a real
identity under a premise that otherwise makes every class a light-carrier. Owner decision
needed — this is fiction territory as much as art.

**3. Add a `dim_shift` inverse map to `docs/data/art/palette.json`.** `light_shift` maps
one value brighter (dark→steel→bone→pale→pale) and is the mechanism for lamp radius. The
premise now needs the opposite at least as often: everything outside the leash, and any
guttering flame, shifts one value *darker*. Proposed:
`{"pale": "bone", "bone": "steel", "steel": "dark", "dark": "dark"}`, same rule ("light is
a step along this ladder, never a new hex"). Without it, §7's two states have no canonical
data form and each spec will invent its own.

**4. `GDD.md` §12 row 6 is out of date.** It still reads *"Strict 4-color global palette
vs. per-faction palettes | Per-faction | Open"*. This was resolved to **strict global
palette** by the owner on 2026-07-12 (`aesthetic-direction.md` top banner) and every art
doc and the pipeline validator now assume it. Row 6 should read: *"Strict global 4-value
palette (2-bit Demichrome) | ✅ Decided 2026-07-12"*. Also worth a line in §10: the
per-faction palette-swap approach described there is no longer the plan.

**5. Retire the four named-hero art/narrative files.** `docs/art/hallam.md`,
`docs/narrative/hallam.md` (and their `edda`/`merle`/`noll` siblings) are stale on the
palette reset, the role-only naming reversal, *and* the WORLD.md supersession. Recommend
the project convention already in use elsewhere: keep them unedited, add a superseded
banner pointing at `docs/art/vanguard.md` and its future siblings. They should not be
deleted — but nothing should cite them, and `CLASSES.md` §1 currently does.
