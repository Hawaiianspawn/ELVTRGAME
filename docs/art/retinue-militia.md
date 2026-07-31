# Retinue militia — swarm unit sprite

**Subject:** the Liberated militia, the player's mass army unit in the Spike 1 / Gate 1
swarm (`Swarm.SpawnRetinue`) — **not** the Vanguard hero ·
**Cell:** 48×48 (locked, owner decision 2026-07-25) · **Sheet:** 4×1 → 192×48 ·
**Request:** `../data/art/requests/unit-retinue.json` · **Texture:** `T_Unit_Retinue`

**Binds to:** `docs/art/npc-silhouette-brief.md` (a) — the Liberated militia mechanism, and
the three-way disjointness audit · `docs/art/aesthetic-direction.md` (Direction A locked,
global 4-value ramp; chibi ruling 2026-07-11; selective outlining) ·
`docs/narrative/FLAME-FOUNDATION.md` (§1 premise, §3a leash-as-light) ·
`CLASSES.md` §1 "Retinue: the Liberated" (Freed → Militia → Veteran → Bannerman; formation
behaviour is the identity) · `docs/RENDERING-LIGHTING.md` §4a (units are seen from behind,
lit by the flame overhead) · `ELVTR/SETUP-EDITOR.md` §1/§3 (`SwarmSheet` column encoding,
Niagara SubUV, Nearest, Unlit+Masked) · `ELVTR/Source/ELVTR/Mass/SwarmFragments.h`
(`SwarmAnim` bits, `SwarmSheet::CellForBits`) · `docs/data/art/palette.json`.

**Deliberately does NOT bind to:** `docs/art/hero-palettes.md` §1–§2 (void), `WORLD.md`
(superseded in full), or any faction name — current canon names none
(FLAME-FOUNDATION §5).

---

## 1. Intent

**Fiction.** These are the people who came to the light. Freed prisoners, conscripts and
farmers holding whatever they picked up on the way out, standing in a rank because the
rank is inside the circle. `CLASSES.md` §1 calls them "individually weak, strong in
ranks"; FLAME-FOUNDATION calls them a congregation. The sprite has to sell **"this was a
person half an hour ago, and nothing they are wearing is theirs."** Not a soldier. Not a
uniform. A patch job.

**Gameplay.** Three jobs, in priority order:

1. **Read as *yours*, instantly, at 16px, in a press of hundreds.** This is a
   thousand-entity game; the militia is the majority of what is on screen. Its whole
   readability budget goes into being **light-skewing and uneven** against a dark,
   regular, or amorphous enemy register (`npc-silhouette-brief.md` audit).
2. **Read as a *crowd of individuals*, not a texture.** The brief's horde-scale promise is
   "a block of *visibly uneven* small silhouettes, individual and human, holding a line."
   The unevenness has to be inside a single sprite's own outline, because there is only
   one sprite — the sim spawns hundreds of copies of it (mirrored in X, see §6c).
3. **Stay disjoint from the hero it stands next to.** The Vanguard hero is Steel-dominant,
   rigidly geometric, and owns one large Pale rectangle (`docs/art/vanguard.md` §4c). The
   militia is Bone-dominant, ragged, and spends **zero Pale**. "Which one is me" must never
   depend on size alone.

---

## 2. Register ruling — walking sprite, not bust icon

`npc-silhouette-brief.md` carries a **friendly-NPC style lock** (owner, 2026-07-12) putting
friendly NPCs, "Liberated militia included", in a bust-forward icon composition —
oversized round head filling the frame, minimal body, near head-on framing, "not the 3/4
low top-down walking-sprite view". This spec **does not follow that lock**, and that has
to be argued rather than assumed.

**Ruling used here: the bust lock governs the avatar/portrait register. This sprite is in
the gameplay register and is authored as a full-body top-down walking sprite.** Reasons,
strongest first:

1. **The lock's own source is an avatar folder.** It anchors to
   `Artboard/Gameplay Avatars/crops/`. The 2026-07-11 chibi amendment in
   `aesthetic-direction.md` §4 draws exactly this line: *"Chibi is now the combat/gameplay-scale
   register (heroes, retinue, and standard enemies); the higher-resolution non-chibi
   register is reserved for portrait/avatar icons only."* A bust-forward, head-fills-frame
   composition **is** that portrait/avatar register. Read as governing avatars, the two
   owner directives agree; read as governing gameplay sprites, they contradict each other,
   and the later one (2026-07-12) never mentions overturning the earlier.
2. **A bust cannot express the states the sim already ships.** `SwarmFragments.h` defines
   `FrameBit`, `AttackBit`/`SwingBit` and `HitFlashBit`, and `SwarmSheet::CellForBits`
   demands four columns: `walk0, walk1, ATTACK, HIT`. A bust has no legs to alternate, no
   arm to swing and no body to recoil. Under the bust reading, three of four columns
   become undrawable and the sim's animation bits become dead code.
3. **The same paragraph of the brief contradicts the lock.** Its horde-scale line is "a
   block of visibly uneven small silhouettes, *individual and human, holding a line*" — a
   line is held by bodies with feet on the ground. The (a) mechanism it is layered onto
   spends its raggedness on *gear that breaks the outline*: "a bandaged arm, an improvised
   club instead of a spear, an open or missing helmet." Two of those three are below the
   neck. A bust deletes the mechanism the brief itself assigns to this subject.
4. **The canonical on-screen view of a militia unit is its back.**
   `docs/RENDERING-LIGHTING.md` §4a, owner direction 2026-07-22: the flame is overhead and
   behind the army, units face outward at what they are fighting, and "the player almost
   never sees their units' faces — only rim-lit backs turned away, toward the dark," which
   the doc calls "the deity/congregation premise rendered directly." A head-on bust is the
   one framing the game's primary lighting read never shows.
5. **At 16px a bust is a dot.** The camera is a straight-down ortho and these read at
   roughly a third of their cell in a crowd. A bust composition spends the entire sprite on
   a head, i.e. on one uniform blob — which is precisely the *texture* failure mode job 2
   above exists to prevent.

**Flagged for owner ruling.** The lock names militia explicitly, so this is an
interpretation of *scope*, not a loophole. It is restated as a canon proposal in §10.1.
**If the owner rules the other way, this spec is void, not amendable** — a bust cannot walk,
swing or recoil, so the swarm sheet would collapse to a single column and
`SwarmSheet::CellForBits` would need rewriting. Say so before generating, not after.

---

## 3. Palette

Global ramp, no exceptions, no swaps (`aesthetic-direction.md` 2026-07-12 reset; GDD #6
resolved to strict global palette).

| Hex | Name | Role on this sprite |
|---|---|---|
| `#211e20` | Demichrome Dark | **outline and recess only** — contour where sprite meets sprite, helmet-brim shadow, two eye recesses, belt, boot soles, the seams that separate arm from torso and leg from leg. Never a body-filling mass. |
| `#555568` | Demichrome Steel | the scavenged hard gear: a battered skullcap over **one** side of the head, **one** bulky pauldron and its sleeve, **one** booted leg, the iron-bound head of the club |
| `#a0a08b` | Demichrome Bone | **dominant** — skin, scalp, face, bandage, patched cloth jerkin, bare wrapped leg, the club's wooden shaft |
| `#e9efec` | Demichrome Pale | **not used. Zero Pale pixels at rest.** |

Transparency is **not a value**: alpha is binary (0/255), Unlit+Masked, per
`palette.json.mask`. The quantizer hard-thresholds at 128; partial alpha is the most
common silent breakage.

**This sprite therefore uses three of the four values.** That is deliberate and it is
exactly the floor the QC pass tolerates (`pixelpipe.py` flags `values_used < 3`). Do not
"fix" it by adding a highlight.

**Reservations, stated so they can be audited:**

- **Pale is spent by no one here.** `pale_usage: none` in the request, which makes the QC
  report flag *any* pale pixel — intended. The bright value is the game's scarcest
  resource; a rescued farmer is not entitled to it. Concretely: the Bannerman (the one
  Liberated unit that ever spends Pale, per `CLASSES.md` §1 and
  `palette.json.shape_carriers.rectangle_flip`) is a **separate subject with its own
  request**, and its flag must stay ≤4×3px at head height so it cannot be confused with
  the hero's 11×7px banner (`vanguard.md` §4c).
- **Eyes are Dark recesses, never Pale.** Two 2×2 Dark notches. A pale eye-dot is the
  registered tell of the void register in `npc-silhouette-brief.md` (c) — putting one on a
  friendly unit is the single worst confusion available in a hue-less game.
- **No mark value is sacrificed.** Rune dot-clusters (Relickeeper) and quarry contours
  (Pathfinder) are drawn over *enemies* and never appear on this sheet, so both stay fully
  available.
- **No faction value.** Current canon names no factions (FLAME-FOUNDATION §5); nothing is
  reserved for one and nothing is invented for one here.

**Light-shifted variant: not emitted** (`light_shift_variant: false`). See §7 — the state
this subject needs is the *dim* one, and that is a material remap, not a second texture.

---

## 4. The south-facing frame — the style contract

Generation is anchor-first, and here the anchor is **authored, not generated**: §5's pixel
map is rendered directly by `pixelpipe.py authored` (zero generations, on-palette by
construction), quantized, and fed back as the v3 reference that produces the rotation and
every animation frame. Everything on this sheet inherits that one frame.

Must be true in the south frame:

1. **The shoulder line is uneven.** One side carries a bulky pauldron that starts a full
   row higher and juts 6px past the torso; the other side is a thin bandaged arm that
   starts lower and is half the width. **This is the militia's primary horde-scale tell**
   — a jagged top edge across a whole block of units, against the hero's flat horizontal
   shoulder and the enemy register's repeated symmetry.
2. **The head is value-split down the middle.** A Steel skullcap covers roughly the
   sprite's left half of the crown; the right half is bare Bone scalp. Half the head dark,
   half light. At 16px this survives when nothing else does, and it is the cheapest
   possible rendering of "an open or missing helmet."
3. **The torso is value-split the *other* way.** Patched Bone cloth on the sprite's left,
   a scavenged Steel plate on the right — the inverse of the head. The resulting
   four-quadrant **patchwork read** is unique to this subject: the hero is uniform Steel,
   the dark register is uniform Dark, the void register is flat Dark with no internal
   modelling at all.
4. **The club is off-vertical and leaves the body outline.** A Bone shaft with a heavier
   Steel head, held low and angled up and away, its far end clear of the head. It must
   read as a *diagonal* — the hero's pole is dead vertical and a vertical stick here
   would read as a spear, i.e. as a regular soldier, i.e. as the wrong faction.
5. **The legs are mismatched.** One booted Steel leg, one bare Bone leg. Two values below
   the belt, not one.
6. **The face is open, and the eyes are two Dark 2×2 recesses.** No visor, no closed helm,
   no crest, no plume, and no bright eyes.
7. **Bone dominates.** Target histogram of opaque pixels: **Bone 38–44% (largest), Steel
   28–34%, Dark 24–30%, Pale exactly 0%.** §5's map lands at Bone 40.3 / Steel 30.7 /
   Dark 28.9 / Pale 0.0. This is how the brief's "Bone and Steel roughly even, Dark only
   for recess/outline" is honoured in countable pixels: Bone and Steel together are 71% of
   the sprite, neither is scarce, Bone leads because skin, cloth and wood outweigh two
   pieces of looted plate — and Bone *must* lead, or the dominance axis of the
   disjointness audit collapses into the hero's (Steel) and the enemy register's (Dark).
8. **Dark stays a contour, never a mass.** Every Dark pixel in §5 is one of: outer
   contour, sprite-meets-sprite seam, brim shadow, eye recess, belt, boot sole, crotch
   cap. There is no filled Dark shape anywhere. Outlining is **selective**
   (`aesthetic-direction.md`): the contour exists because at horde scale sprite meets
   sprite constantly, but it is 1px and it is never doubled.

**Dither: none, and none should be invented.** If the rotation or animation pass produces
dither at a Bone/Steel boundary it must land as **2×2 blocks minimum**
(`palette.json.dither.moving_min_block`); the quantizer converts 1px stipple
automatically, but a frame that comes back *mostly* stipple has been shaded rather than
flat-filled and should be re-rolled. 1px non-stipple detail — the eye recesses, the seams,
the belt — is preserved by design and is where the detail budget goes.

---

## 5. Silhouette guide

### 5a. South frame at cell scale (48×48) — this block IS the anchor

Legend (`palette.json.ascii_legend`): `.` transparent (mask, not a value) ·
`#` Demichrome Dark · `S` Demichrome Steel · `b` Demichrome Bone · `@` Demichrome Pale
(unused here).

```
................................................
................................................
................................................
................................................
.................#######........................
................#SSSSSSS#.......................
...............#SSSSSSSSS######.................
..............#SSSSSSSSSS#bbbbb#................
..............#SSSSSSSSSS#bbbbbb#...............
..............#SSSSSSSSSS#bbbbbb#...............
..............#SSSSSSSSSS#bbbbbb#...............
.............#SSSSSSSSSSS#bbbbbb#...............
.............#############bbbbbb#...............
..............#bbbbbbbbbbbbbbbbb#...............
..............#bbbbbbbbbbbbbbbbb#...............
..............#bbb##bbbbbb##bbbb#...............
.....###......#bbb##bbbbbb##bbbb#...............
....#SSS#.....#bbbbbbbbbbbbbbbbb#...............
...#SSSSS#....#bbbbbbbbbbbbbbbbb#...............
...#SSSSS#....#bbbbbbbbbbbbbbbbb#...............
....#bbb#......#bbbbbbbbbbbbbbb#................
....#bbb#.......###############.................
.....#bbb#.......bbbbbSSSSSSSS#######...........
.....#bbb#......#bbbbbSSSSSSSS#SSSSSS#..........
......#bbb#.....#bbbbbSSSSSSSS#SSSSSS#..........
......#bbb#.#bbb#bbbbbSSSSSSSS#SSSSSS#..........
.......#bbb##bbb#bbbbbSSSSSSSS#SSSSS#...........
.......#bbb##bbb#bbbbbSSSSSSSS#SSSS#............
........#bbb#bbb#bbbbbSSSSSSSS#SSSS#............
........#bbb#bbb#bbbbbSSSSSSSS#SSSS#............
...........#bbbb#bbbbbSSSSSSSS#bbb#.............
...........#bbbb#bbbbbSSSSSSSS#bbb#.............
...........#bbbb#bbbbbSSSSSSSS####..............
............#####b###########S#.................
................#bbbbbSSSSSSSS#.................
.................#SSS#####bbb#..................
.................#SSS#...#bbb#..................
.................#SSS#...#bbb#..................
.................#SSS#...#bbb#..................
.................#SSS#...#bbb#..................
.................#SSS#...#bbb#..................
................#SSSS#...#bbbb#.................
................######...######.................
................................................
................................................
................................................
................................................
................................................
```

Content bounding box **35 × 39 px** inside the 48px cell (x 3–37, y 4–42). `pack` centres
content on one global bbox and **fails hard** rather than scaling, so this margin is
required, not decorative.

Measured histogram of the block: **761 opaque px — Bone 307 (40.3%), Steel 234 (30.7%),
Dark 220 (28.9%), Pale 0.** Matches §4.7. Three values used, no dither, alpha binary.

Reading it top to bottom: a Steel skullcap sitting low over one side of an oversized head
→ bare Bone scalp on the other side, so the crown's top edge steps → brim shadow → open
Bone face with two Dark eye recesses → jaw line → a bulky Steel pauldron on one shoulder
starting a row *above* the torso, and a thin bandaged Bone arm on the other starting a row
*below* it → torso split Bone-cloth / Steel-plate, the inverse of the head → belt → one
Steel booted leg, one bare Bone leg → and out to the side, a Bone club shaft on a diagonal
with an iron-bound Steel head, breaking the outline entirely.

### 5b. Horde-zoom check (~12px wide, hand-reduced impression of what the player sees)

```
.....####...
....#SSbb#..
....#SSbb#..
.##.#bbbbb#.
.#S.#b#b#b#.
..S.#bbbbb#.
..b.#######.
..b.bbSSS#S.
...bbbSSS#S.
...#bbSSS#b.
....#bSSS##.
....#S#bb#..
....##.##...
```

**Reads as:** *a patched, uneven person pressed into a rank — motley, never uniform.*

**At 500 units this reads as:** a pale, restless, visibly *lumpy* mass. No two adjacent
silhouettes share a top edge; sticks poke out of the block at odd angles; every unit is
half-light-half-dark but the halves are on different sides because the mirror bit flips
half the crowd (§6c). Against the flame-lit floor the block reads as a **light** field, and
the moment a unit leaves the circle it drops one value and visibly greys out (§7) while the
mass stays bright — the leash made legible without a HUD. The failure mode this guards
against is the block reading as a single uniform tone, i.e. as *terrain*; the four-quadrant
patchwork plus the jagged top edge is what keeps it reading as *people*.

### 5c. Disjointness audit

Per the three-axis model in `npc-silhouette-brief.md`, extended with the hero it fights
beside — the hardest case, since they share the ramp, the fiction and the proportions.

| | Silhouette | Value dominance | Pale usage |
|---|---|---|---|
| **Retinue militia (this spec)** | **ragged/irregular** — uneven shoulders, half-capped head, off-vertical club leaving the outline, mismatched legs | **Bone** 38–44%, Steel 28–34%, Dark contour-only | **none** — zero Pale pixels, ever, at rest or moving |
| Vanguard hero (`vanguard.md`) | rigidly geometric — flat shoulder line, rectangle shield, dead-vertical pole | Steel ≥50% | one large Pale rectangle (banner) |
| Bannerman (separate request) | militia silhouette + one small held rectangle | Bone (as militia) | ≤4×3px Pale rectangle-flip, at head height |
| A regular/uniformed enemy register, if one is named | clean, symmetrical, repeated | Dark-heavy, thin Steel rim | none |
| An amorphous void register, if one is named | non-geometric, no held object | flat Dark, zero internal modelling | bare eye-dot, no halo |

All three axes disagree in every pairing. Note the enemy rows are described by *mechanism
only*: the factions those mechanisms belonged to were discarded by the 2026-07-22 reset and
current canon names none. See §10.2.

---

## 6. Sheet layout

- **Cell:** 48×48px. **Grid:** 4 columns × 1 row (both powers of two, per the Niagara
  SubUV rule in `ELVTR/SETUP-EDITOR.md` §3.5). **Texture:** 192×48, `T_Unit_Retinue`,
  imported Nearest / NoMipmaps / UserInterface2D / sRGB on; material Unlit+Masked,
  RGB→Emissive, A→Opacity Mask.
- **Column order is not a choice — it is `SwarmSheet::CellForBits`.**
  `ELVTR/Source/ELVTR/Mass/SwarmFragments.h` decodes one cell per unit on the CPU with
  priority **hit > attack > walk-frame**, and `SETUP-EDITOR.md` §1 documents the grid as
  `walk0, walk1, ATTACK, HIT`. This sheet is exactly that row.

| Cell | Frame key | Shown when |
|---|---|---|
| 0 | `south.idle` | nothing happening. `FrameBit` is only set while `Velocity² > 100`, so **cell 0 is what a standing unit shows** — it must be a plausible at-rest pose, which is why the rotation-pass south frame (the anchor, rendered up) goes here rather than a mid-stride frame. |
| 1 | `south.walk3` | `FrameBit` set — the opposite-leg beat, alternating with cell 0 at `SwarmTuning::WalkAnimHz` |
| 2 | `south.attack3` | `AttackBit`/`SwingBit` — the club at extension |
| 3 | `south.hit2` | `HitFlashBit` — recoil, and it wins over everything else |

**One direction only, and that is a sim fact, not a shortcut.** The swarm has no direction
column: `SwarmProcessors.cpp` sets `SwarmAnim::FlipBit` from `Velocity.X < 0` and the
camera is a straight-down ortho with `Facing Mode = Face Camera`. Eight rotations are
generated (v3 always returns 8) and **retained** under the retention rule, but only south
is packed. When the sheet later grows a facing axis, the extra rotations are already paid
for.

**Frame-pick freedom, costing zero generations.** `walking-4-frames` yields
`walk1…walk4`, and the v3 attack/hit yield 4 frames each; only three of the eleven are
packed and the rest stay in `RawArt/Renders/unit-retinue/r1/`. If `walk3` reads too close
to the idle at 16px, or `attack3` catches the club mid-return instead of extended, edit
`frame_map`, bump `revision` and re-pack. No re-generation.

**Forward compatibility, deliberately reserved.** `RENDERING-LIGHTING.md` §4a specs a
future per-unit **light bucket** (4 buckets: lit-from-behind / front / left / right) folded
into the same SubUV index. That expansion is a 4×4 grid — column = animation state exactly
as above, row = light bucket — which is 192×192 and still power-of-two on both axes. The
back-lit bucket is on screen the overwhelming majority of the time, so **this sheet is that
bucket**; it becomes row 0 of the wider sheet without re-authoring. Do not spend
generations on the other three buckets until the Niagara sprite path is fixed.

**Composite note.** This request deliberately does **not** target `T_Swarm_2bit`. That
atlas is shared with a second subject and is assembled by the composite request
`swarm-units.json`; this per-subject 4×1 row is designed to be pulled into it as the
retinue row (row 1 of the 4×2).

**§10.3's 68px conflict is RESOLVED (2026-07-25) — that section is stale.** The shipped
atlas is now **192×96 with 48px cells**, built by `pixelpipe.py pack swarm-units`, so the
48px lock holds and no exception is needed. The fear that cropping to 48 would clip the
attack pose was measured and is unfounded: `pack` crops to the *content* bbox, not the
canvas, and the widest packed frame across both subjects is 36×48 — it fits with room
spare. The 272×136/68px figure came from a hand-built sheet that has since been
superseded (`Scripts/build_swarm_sheet.py`, now marked as such).

---

## 7. Animation notes

| Action | Frames | Source | What the motion communicates |
|---|---|---|---|
| Idle (cell 0) | 1 | v3 rotation pass (the rendered-up anchor) | **Standing in the light.** Not braced, not at attention — a person waiting. No breathing loop: it would cost a cell this sheet does not have, and at 16px a 1px bob is indistinguishable from the walk flip. |
| Walk (cell 1) | 1 packed of 4 | `walking-4-frames` template, 1 generation | A **shuffle**, not a march and not a jog. `CLASSES.md` §1 gives the Vanguard's ranks their identity through *formation shape*, not through per-unit animation budget — that budget belongs to the Pathfinder's pack. Two beats, even cadence, no anticipation, no lean. |
| Attack (cell 2) | 1 packed of 4 | v3, 1 generation | One **downward club swing**, arms committed, body turned into it. It must read as *effort* — this is a farmer hitting something, not a swordsman. At 16px the readable content is the club leaving the silhouette on a new axis, so the packed frame must be the one where the club is furthest from the body. |
| Hit (cell 3) | 1 packed of 4 | v3, 1 generation | **Recoil**, and it out-prioritises the swing by design (`SwarmFragments.h`: "a unit struck in the middle of its own swing should show the recoil, because being hit is the thing the player needs to read"). Head snapped back, club arm thrown wide, whole silhouette displaced — at horde scale a hit must be legible as a *shape change*, since a 1-frame flash of a 16px sprite is not. |

**Playback:** the walk flip runs at `SwarmTuning::WalkAnimHz` (sim-owned; ~4–5 fps reads
as footfalls rather than vibration). SubUV index selection is a hard cut — do not
interpolate. Attack and hit are single-pose states held for as long as their bit is set.

**Mirroring is free variation, and this subject is built for it.** `FlipBit` mirrors the
sprite in X, so half the crowd shows the pauldron and skullcap on the opposite side. For a
subject whose entire read is *asymmetry*, the mirror doubles the apparent roster for
nothing — the exact opposite of the hero, where handedness (shield left, pole right) is
part of the class grammar and mirroring is a compromise. No detail here may depend on
which side it lands on.

---

## 8. Light: full value inside, dimmed one value outside

FLAME-FOUNDATION §3a makes the leash visible: inside the circle, lit floor and units at
full value; outside, the dark ground state and units dimmed one value down the ramp.

**This sprite is authored as the inside-the-circle state** — full ramp, Bone-dominant,
which is what §4.7's histogram describes. The outside state is
`palette.json.dim_shift` applied at render time: Bone→Steel, Steel→Dark, Dark→Dark. A unit
that walks out of the light therefore loses its Bone majority and collapses toward Steel
and Dark — it visibly *becomes* the register of the thing that is going to kill it, which
is the correct and sympathetic read of `bLeashBroken` / `LeashWarnBit`. That is a 4-entry
value remap driven by one scalar on the Unlit+Masked material, not a second sheet and never
per-unit material work (mass units are GPU-instanced).

Two consequences worth stating because they are load-bearing:

1. **The 1px Dark contour is what keeps a dimmed unit visible.** Under `dim_shift` a
   dimmed unit is Steel-and-Dark on a Dark floor; the outline is no longer doing
   sprite-vs-floor work and the *silhouette* (§4.1–§4.5) is carrying the entire read. This
   is the horde-scale case that justifies spending 29% of the sprite on contour.
2. **`light_shift_variant` stays false.** A brighter-by-one sheet has no state to
   represent: the militia's brightest legal state is the one authored here. Brightening
   past it would spend Pale, which §3 forbids.

---

## 9. Buildability checklist (stage-D gate)

The anchor is authored, so items 1–3 should pass by construction — check them anyway,
because a hand edit to §5a will silently change them. Items 4–10 are what to check on the
**rotation and animation passes**, which is where a generated frame can drift.

- [x] Exactly three values present (Dark, Steel, Bone); alpha strictly 0 or 255.
      **MEASURED** (`pixelpipe.py quantize`, standalone, `--palette demichrome-4
      --moving`, on the four cells cropped from the packed `T_Unit_Retinue.png`):
      `values_used = 3` and `alpha_partial_before = 0` for all four cells (idle,
      walk, attack, hit); on-palette 100.0% in all four. Sheet-wide, no exceptions.
- [x] **Zero Pale pixels.** Any pale pixel = reject (`pale_usage: none`).
      **MEASURED:** pale = 0.0% in the quantized histogram of all four cells.
      Sheet-wide, no exceptions.
- [ ] Bone is the largest bucket (`value_dominance: bone`), 38–44%; Dark ≤30%.
      **FAILED on cell 3 (hit).** MEASURED per-cell dark/steel/bone histogram %:
      idle 28/32/40, walk 29/31/40, attack 29/30/41 — all on spec, idle matching
      §5a's own 28.9/30.7/40.3 almost exactly. **hit measures 27/6/67** — Bone is
      67%, 23 points past the 44% ceiling, and Steel has collapsed from its
      28–34% band to 6%. See finding below.
- [x] Shoulder line is visibly uneven — one bulky pauldron, one thin arm.
      Visually confirmed in all four cells: idle/walk/attack show the bulky
      Steel pauldron on one side and a thinner limb on the other; hit keeps an
      asymmetric silhouette (arm and club thrown wide on the recoil) even though
      its value read fails below. PASS on shape in all four.
- [ ] Head is value-split: cap on one side, bare scalp on the other.
      **FAILED on cell 3 (hit).** idle/walk/attack all show the Steel skullcap
      over roughly the left half of the crown and bare Bone scalp on the right,
      matching §5a. **hit renders the head as one undifferentiated Bone mass** —
      the skullcap has detached into a separate small Steel fragment flying
      above/behind the head rather than remaining a value-split cap. See finding.
- [ ] Torso is value-split the other way (cloth vs. plate).
      **FAILED on cell 3 (hit).** idle/walk/attack show a clean Bone-left /
      Steel-right torso split. **hit's torso is entirely Bone** — no Steel
      plate survives the pose. See finding.
- [x] Club present, off-vertical, its far end clear of the body outline.
      Confirmed in all four cells: idle (diagonal down and away, Steel head /
      Bone shaft), walk (held low near the ground), attack (extended to full
      reach, clear of the body), hit (thrown wide on the recoil). PASS.
- [ ] Legs mismatched — one Steel, one Bone.
      **FAILED on cell 3 (hit).** idle/walk/attack all show one Steel booted
      leg and one bare Bone leg. **Both of hit's legs render Bone** — the Steel
      boot is gone. See finding.
- [ ] Eyes are Dark recesses. No visor, no closed helm, no crest.
      **FAILED on cell 3 (hit).** idle/walk/attack all show two small Dark
      eye-recess pixels on the face. **hit has zero Dark pixels anywhere in the
      head** — not a visor or closed helm, just an absence of the eye recesses
      along with everything else the head normally carries. See finding.
- [x] No dither block smaller than 2×2; no frame that is mostly stipple.
      **MEASURED:** `stipple_px_before`/`after` = 0/0 in all four cells; no
      checker field detected in any of them, `dither_enforced = False`
      throughout. Sheet-wide, no exceptions.
- [x] Content bbox ≤ 48×48 in every packed frame.
      **MEASURED** (alpha bbox of each cropped cell): idle 35×39, walk 37×39,
      attack 33×39, hit 42×39. All comfortably inside the 48px cell; idle matches
      §5a's stated 35×39 content box exactly. Sheet-wide, no exceptions.

**Finding — the hit (recoil) cell has lost its Steel value, 2026-07-31 (task-010
verification pass).** Cell 3 of the packed sheet is not a scaled or cropped version
of the same silhouette rules the other three cells hold to — its Steel content has
collapsed to two small isolated fragments (the detached skullcap and the club
head), taking the skullcap/scalp head-split, the cloth/plate torso-split, and the
Steel booted leg down with it, and pushing Bone to 67% of the frame against the
38–44% target. This is the frame `docs/art/retinue-militia.md` §7 says has to carry
the *most* legible shape change ("a hit must be legible as a shape change, since a
1-frame flash of a 16px sprite is not") and it is the one cell where the subject's
entire patchwork read — the mechanism this spec exists to protect — is absent. It
resembles the "generator tidying the sprite up" failure mode §9 already warns
about (a rotation coming back cleaner/more uniform than requested), just showing
up on the animation pass rather than the rotation pass, which is not the case that
prose anticipates. Not fixed here — this is a verification pass, not a regeneration
pass. Re-rolling `attack`/`hit` frame selection or the v3 hit generation itself
(§6's "frame-pick freedom, costing zero generations" — three of eleven generated
hit/attack frames are packed, the other eight already exist under
`RawArt/Renders/unit-retinue/r1/`) is the likely fix and costs no new generations
if one of the other three hit frames holds its Steel value; only a fresh v3
generation would spend budget.

Measured against `docs/data/art/requests/unit-retinue.json`'s `canon.palette:
"demichrome-4"` exactly as written — this checklist does not reconsider whether
that opt-in should change now that demichrome-4 is no longer the game-wide
default (`aesthetic-direction.md` AMENDMENT 2026-07-28); that is an owner call,
not this pass's.

**Known failure modes to watch, in order of likelihood:**

- **Dark dominance.** Shadow, outline and every crevice all land on Dark under a 4-value
  collapse. If the rotation pass comes back Dark-dominant, the cause is heavy internal
  outlining or a cast shadow — both must be *absent*, not merely un-requested. Re-roll the
  rotation; keep the anchor.
- **The generator tidying the sprite up.** Matched armour, a symmetric silhouette, a real
  helmet or a proper spear are all "improvements" that destroy the subject. If the rotation
  returns a neat soldier, re-roll it; do not accept it as "cleaner."
- **Rear-facing rotations flattening the club into a slab** — the documented v3 weakness.
  Irrelevant here (only south is packed) but it is the tell that the reference propagated
  weakly; if south *also* looks flattened, re-roll.

---

## 10. Depends on

- **GDD #5 (sprite flipbooks vs. flat-shaded 3D): assumes flipbooks on instanced quads,
  loudly.** The entire deliverable is a SubUV flipbook whose cell order is dictated by
  `SwarmSheet::CellForBits`, packed for camera-facing Niagara sprite billboards. Under
  flat-shaded 3D this sheet is meaningless: rotation becomes free, the four animation
  columns become skeletal clips, and the `dim_shift` remap in §8 moves from a material
  lookup to a per-object palette constant. **The silhouette and value rules in §4/§5
  survive either answer; the sheet does not.** Recommendation unchanged and reinforced by
  this subject: flipbooks. A 4-value flat ramp on a 3D mesh fights the no-shading
  constraint the instant anything rotates, and at 16px per unit a mesh buys nothing a
  4-cell sheet does not already deliver — while a thousand instanced quads is a solved
  problem the Spike 1 benchmark already measures.
- **GDD #6:** resolved (strict global palette, 2026-07-12). Not re-litigated. Note
  `GDD.md` §12 row 6 still reads "Per-faction | Open" — already raised as canon proposal 4
  in `vanguard.md`.

---

## 11. Canon proposals

**1. Scope the friendly-NPC bust lock to the avatar/portrait register — owner ruling
needed before this sheet is generated.** `npc-silhouette-brief.md`'s style lock
(2026-07-12) names "Liberated militia included" and specifies a bust-forward icon
composition, explicitly "not the 3/4 low top-down walking-sprite view". Taken literally it
applies to the mass gameplay unit, which cannot then walk, swing or recoil — leaving three
of `SwarmSheet`'s four columns undrawable and the `AttackBit`/`HitFlashBit` decode dead.
§2 argues it was meant as the avatar/portrait register, consistent with the 2026-07-11
chibi amendment ("the higher-resolution non-chibi register is reserved for portrait/avatar
icons only"). Proposed edit to the brief's lock paragraph:

> **Friendly-NPC style lock (owner directive, 2026-07-12; scoped 2026-07-25):** every
> friendly NPC's **avatar / portrait / dialogue icon** — Liberated militia included — is
> anchored to the composition of `Artboard/Gameplay Avatars/crops/`: a bust-forward icon,
> oversized round head occupying nearly the whole frame, minimal-to-no visible body,
> single bold outline, flat fill, near head-on framing. **Gameplay-scale sprites are not
> covered by this lock** and use the chibi top-down full-body register (2026-07-11
> amendment), because the sim requires walk / attack / hit poses a bust cannot express.
> The (a) militia mechanism — ragged silhouette, light-dominant values, no Pale at rest —
> governs both registers.

If the owner instead intends the lock to cover gameplay sprites, this spec is void and
`SwarmSheet` needs redesigning; that is a code change, not an art change.

**2. The disjointness audit needs at least one named enemy register to be checkable.**
`npc-silhouette-brief.md`'s audit is a three-way table, but two of its three rows (the
regular/uniformed register and the amorphous void register) were discarded with the
2026-07-22 reset, and current canon names no replacement (FLAME-FOUNDATION §5) — correctly
so, pending §4.5 ("does the dark have monsters, or is the dark itself the enemy?"). The
consequence for art is concrete: **this spec can guarantee the militia is Bone-dominant,
ragged and Pale-free, but it cannot guarantee that is *disjoint* from anything, because
there is nothing to be disjoint from.** §5c is written against mechanisms rather than
names as a stopgap. Proposal: when the prototype answers §4.5, the first thing the answer
must produce is a **value-dominance and silhouette-regularity claim for the primary enemy**
(nothing more — no faction name, no lore), so the audit becomes checkable. Suggested
reservation to protect in the meantime: **Bone-dominant and irregular is the player's
side.** Enemies take Dark-dominant, or regular, or both.

**3. ~~`T_Swarm_2bit`'s 68px cells conflict with the 48px cell lock.~~ RESOLVED
2026-07-25 — no owner call needed.** Resolution (a) was taken and cost nothing: the atlas
is re-packed by `pixelpipe.py pack swarm-units` at **192×96, 48px cells**, so the lock
holds, `SwarmSheet`/Sub UV/`UnitCamProjector` are untouched, and no validator exception
exists.

The premise behind the 68px figure was wrong rather than merely inconvenient. `pack`
crops to each source's **content bbox**, not to PixelLab's canvas, so the ~40% canvas
padding is discarded before cell-fitting ever happens — the widest packed frame across
both subjects measured **36×48**. Nothing clipped. Option (b) (generating at size 32–40)
would have been a real quality loss taken for no reason, which is worth remembering as a
general lesson: measure the bbox before believing a canvas-size argument.
