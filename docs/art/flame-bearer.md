# The Bearer — hero sprite (the flame bearer, shared by all classes)

**Subject:** the protagonist — one flame bearer, played by any of the four classes, not a
per-class hero · **Cell:** 48×48 (locked, owner decision 2026-07-25) · **Sheet:** 4×4 →
192×192 · **Request:** `../data/art/requests/protagonist-04.json` · **Texture:**
`T_Hero_Bearer` · **Winning design:** prototype 04, "The Brazier"
(`docs/art/protagonist-prototypes.md` §9)

**Binds to:** `docs/narrative/FLAME-FOUNDATION.md` (premise, §3a leash-as-light) ·
`docs/art/aesthetic-direction.md` (Direction A locked, global 4-value ramp) ·
`docs/art/protagonist-prototypes.md` (§9 — the source design this spec productionizes,
§2 palette table, §13 canon proposals) · `docs/art/flame-bearer-status.md` (the checkpoint
that framed the decision this spec resolves) · `docs/art/npc-silhouette-brief.md`
(three-way disjointness model) · `docs/data/art/palette.json`
(`shape_carriers`, `dim_shift`) · `ELVTR/SETUP-EDITOR.md` (Nearest, NoMipmaps,
Unlit+Masked, Niagara SubUV).

**Deliberately does NOT bind to:** `docs/art/vanguard.md` (superseded as *the* flame
bearer spec now that prototype 01 did not win — see §10 canon proposal 3),
`docs/art/hero-palettes.md` (void), `WORLD.md` (superseded in full).

---

## 0. The decision this spec records

The owner has decided, relayed via the team lead 2026-07-26: **one flame bearer, not one
per class.** This answers `protagonist-prototypes.md` §13 proposal 1 and
`flame-bearer-status.md` §4 question 1 in the "class-independent protagonist" direction —
prototypes 02, 03 and 04 were candidates for *one* role, not four.

**Why 04 ("The Brazier") over the other three**, checked against the actual quantized
renders in `RawArt/Renders/protagonist-0{1,2,3,4}/r{1,2}/`, not just the prose:

1. **It is the design that visibly carries fire.** Looking at every quantized `south`
   frame in hand (r1 and r2, where a re-roll exists): **01 shows a plain spear with no
   flag at all** — its r2 `concept128/south.png` histogram has **0.0% Pale**, a hard
   contradiction of the one thing the design is supposed to be about. **02's chest device
   reads as a grey plate**, not a lit ember — no amount of squinting recovers a glow from
   it. **03 and 04 are the only two that actually read as fire on sight** — 03's bowl and
   04's ribbed crown both show unambiguous Pale flame shapes in every rotation checked.
   A bearer the player cannot tell is carrying the only light in the world fails
   FLAME-FOUNDATION §1 before any other criterion matters.
2. **Procedural rigging (near-term, owner-stated) wants arms free.** Between the two
   fire-carrying survivors, 04's hands are empty and the brazier mounts to a spine socket;
   03 permanently occupies both hands on a held bowl. A procedural control rig fights a
   permanent two-hand IK lock on everything downstream — weapon draw, gesture, hit-react —
   that 04 does not foreclose. This is the deciding cut between the two designs that
   actually show fire.
3. **Horde-scale legibility.** `protagonist-prototypes.md` §9 rates 04 "second-best horde
   performance" (a countable bright bar at head height) against 03's "middle-performing" —
   worse than 01/04, better than 02 (§8). Between the two remaining fire-visible
   candidates, 04 also reads better at range.

**One correction to the reasoning that came into this round:** the brief characterized
04's rig verdict as "hostile," but `protagonist-prototypes.md` §9 itself says the
opposite — *"Rig: yes, as specced — and this is the reason it is specced this way. A
rigid brazier on a spine socket attaches cleanly and animates for free."* The spine-mount
was chosen specifically because it rigs well; procedural rigging is a second, independent
reason to prefer it, not a reason to overturn a bad verdict that was never actually given.

**What 04's win closes out:** `protagonist-prototypes.md` §13 proposal 3 (a possible
collision between prototype 03 and retinue unit `soldier-05` Flame-tender, since both are
hooded figures carrying caged fire in a vessel) does not apply — 03 did not win, so
`soldier-05` needs no re-pitch on this account.

---

## 1. Intent

**Fiction.** The bearer is the only light in a pitch-dark world (FLAME-FOUNDATION §1).
The flame is not held out in front like a signal and not hidden inside armour like a
secret — it is **worn**, riding across the shoulders in an iron cage, visible to anyone
standing near enough to be saved by it. The sprite has to sell *"the light rides on this
person's back, and everyone can see it burning"* — not a soldier, not a torch-bearer, a
vessel.

**Gameplay.** Two jobs, in priority order:

1. **Read as fire, on sight, at rest.** This is the criterion the other three prototypes
   failed. Every rotation of this sprite must show Pale flame between the ribs; a frame
   that quantizes to zero Pale is a failed generation, not an acceptable variant (see
   §8).
2. **Find yourself at range without losing the read to occlusion.** The wide horizontal
   bar sits at head height, above most of a crowded field, and it is checkable across the
   field the way no other shape in the game is — it is the widest bright any sprite in the
   game is allowed to spend (`protagonist-prototypes.md` §2 Pale budget table: ≤10%,
   the largest of the four prototypes).

**This character is a near-term procedural-rig target**, per the owner. The empty hands
and spine-socket mount are load-bearing for that, not incidental — see §9.

---

## 2. Palette

Global ramp, no exceptions, no swaps (`aesthetic-direction.md` 2026-07-12 reset; GDD #6
resolved to strict global palette).

| Hex | Name | Role on this sprite |
|---|---|---|
| `#211e20` | Demichrome Dark | outline, contour, **the ribs that divide and cage the fire** (universal caging rule, `protagonist-prototypes.md` §2: "every fire in this set is caged — bounded by a hard Dark rim or rib on all sides"), eye notches, belt line, hem line, boot contour |
| `#555568` | Demichrome Steel | worked iron: hat crown and brim, the brazier's top/bottom frame and end-caps, boot/leg fittings, collar |
| `#a0a08b` | Demichrome Bone | skin, cloth tunic — the *person* value, per `protagonist-prototypes.md` §9's `bone`-and-`skin` value reasoning |
| `#e9efec` | Demichrome Pale | **the flame, and nothing else** — visible only in the gaps between the brazier's ribs |

Transparency is **not** a value: alpha is binary (0/255), Unlit+Masked, per
`palette.json.mask`.

**Reservations, stated so they can be audited:**

- **No faction value.** Current canon names no factions (FLAME-FOUNDATION §5).
- **Pale is spent only between the ribs of the brazier band**, split across the width,
  never one contiguous field (`protagonist-prototypes.md` §2: "split between ribs" for
  this design). No Pale anywhere else — not eyes, not hat, not legs.
- **The caging rule is mechanical, not decorative:** every Pale pixel's four orthogonal
  neighbours must be opaque (never transparent) — `quantize`'s `pale_uncaged` audit
  enforces exactly this. On this sheet the frame rows (Steel, top/bottom of the brazier
  band) and the rib columns (Dark, either side) do that job; no Pale pixel in the design
  below sits at the outer edge of the band.
- **No mark value is sacrificed.** Rune dot-clusters (Relickeeper) and quarry contours
  (Pathfinder) are drawn by other classes on enemies and never appear on this sheet.
- **Value dominance is `mixed` by design**, not by accident: the brazier's own iron frame
  (Steel), the tunic (Bone), the ribs and contour (Dark) and the flame (Pale) are meant to
  stay close to each other rather than let one value take a clean majority. See §8 for the
  hand-counted estimate on the authored anchor and the re-author trigger if a real render
  disagrees.

**Light-shifted variant: none emitted** (`light_shift_variant: false`). A bearer is never
outside its own light — it renders at full ramp always, same reasoning as `vanguard.md`
§7 for the design that previously held this role. The state this sprite *does* need is
the opposite one — see §7.

---

## 3. The south-facing frame — the style contract

Generation is anchor-first via the **authored** route (§9): the pixel map in §4a below
*is* the anchor, rendered directly by `pixelpipe.py authored`, zero generations. It then
feeds the v3 rotation pass exactly as `vanguard.md`'s authored anchor did. Everything on
this sheet inherits this one frame. Must be true in it:

1. **A wide flat-brimmed hat sits on the head**, straight brim, wider than the head,
   narrower than the brazier band below it.
2. **A wide horizontal brazier band crosses at head/shoulder height**, projecting past
   the body's own shoulder width on both sides — the widest element in the whole
   silhouette, wider than the hat.
3. **Fire is visible only between vertical ribs inside that band**, never as a
   contiguous field, never escaping past the band's top or bottom edge, never touching
   transparency.
4. **Both hands are empty**, visible hanging at the sides of a narrow torso — no held
   prop of any kind.
5. **The body below the band is narrow**: a tunic tapering into a belt line, then two
   separate narrow legs, then boots. Overall shape is a **T** — wide top, narrow stem.
6. **Eyes are Dark notches**, never Pale — a bright-eyed bearer misreads as the
   amorphous-void register (`npc-silhouette-brief.md` (c)), the single worst confusion
   this game can produce.
7. **No cape, no cloak, no loose cloth** breaking the outline anywhere (same rule
   `vanguard.md` §3.7 states for the incumbent, and the rule 01's own renders broke twice).

---

## 4. Silhouette guide

### 4a. South frame at cell scale (48×48)

Legend: `.` transparent (mask, not a value) · `#` Demichrome Dark · `S` Demichrome Steel ·
`b` Demichrome Bone · `@` Demichrome Pale

```
................................................
................................................
................................................
.....................######.....................
....................#SSSSSS#....................
....................#SSSSSS#....................
..............####################..............
..............#SSSSSSSSSSSSSSSSSS#..............
..............####################..............
....................#bbbbbb#....................
...................#bbbbbbbb#...................
...................#b#bbbb#b#...................
...................#bbbbbbbb#...................
....................#bbbbbb#....................
......................#bb#......................
....................#SSSSSS#....................
.....SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS.....
......SSS#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@SSS......
......SSS#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@SSS......
......SSS#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@SSS......
......SSS#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@SSS......
.....SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS.....
................#bbbbbbbbbbbbbb#................
................#bbbbbbbbbbbbbb#................
..............#bbbbbbbbbbbbbbbbbb#..............
..............#bbbbbbbbbbbbbbbbbb#..............
..............#bbbbbbbbbbbbbbbbbb#..............
..............#bbbbbbbbbbbbbbbbbb#..............
..............####################..............
..............#bbbbbbbbbbbbbbbbbb#..............
..............####################..............
..................#bbbbbbbbbb#..................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
...................#SS#..#SS#...................
.................#SSS#....#SSS#.................
.................#SSS#....#SSS#.................
................................................
................................................
................................................
................................................
................................................
................................................
```

**Note on the rib row width above:** the fenced block is authored to exactly 48 lines so
`pixelpipe.py authored` selects it as the exact match over the horde-check block in §4b;
`pipeline.py authored`'s own left-justify pass pads any short line with `.` to the block's
true width, so a stray character short on any one row degrades to extra margin rather than
breaking the render — but **run `pixelpipe.py authored protagonist-04` and read its
opaque-pixel report before treating this as final**, since this map was authored by hand
and not verified by executing the renderer.

Content bounding box ≈ 36×39px inside the 48px cell (brazier band cols 6–41 at its
widest, crown-to-boot rows 3–41 tall) — comfortably inside the cell with margin to spare.

Reading the block top to bottom: small Steel hat crown → wide flat Steel brim → Bone
face with Dark eye notches → short neck → Steel collar → **the brazier band**: a flat
Steel top frame, four rows of alternating Dark rib / Pale flame between Steel end-caps,
a flat Steel bottom frame → Bone shoulder line → wide Bone tunic torso → Dark belt band →
Bone tunic hem → Dark hem edge → narrowing Bone waist → two separate legs (Dark-outlined,
Steel-filled) → two Steel-and-Dark boots.

### 4b. Horde-zoom check (~12px, what the player actually sees at gameplay zoom)

```
.@#@#@#@#@#@.
..##########.
....######...
....#bbbb....
....######...
....#bb#.....
....#SS#.....
....#SS#.....
...#S#.#S#...
....##.##....
.............
```

**Reads as:** *a narrow figure under a wide flickering bar of fire — top-heavy, the light
worn on the shoulders rather than held or hidden.*

**At 500 units this reads as:** a wide bright-and-dark banded bar at head height,
countable across the field, distinct from any single-point glow (the Lampbearer's
`point_halo`) or any large single rectangle (the Vanguard-shaped `rectangle_flip`, which
this bearer does not use — see §10 canon proposal 4) because the bright is *segmented*,
never one shape. Even when the lower body is fully occluded by a crowd, the ribbed bar
clears the crowd line and stays legible as fire specifically — alternating dark-and-pale,
not a solid block — which is a read no other unit-scale sprite in the game produces.

### 4c. Disjointness audit

| | Silhouette | Value dominance | Pale usage |
|---|---|---|---|
| **The Bearer (this sprite)** | wide horizontal ribbed bar over a narrow tapering body (a **T**) | mixed (no clean majority; see §8) | **split, between ribs**, welded to the shoulder line, no gap |
| Liberated militia (retinue) | ragged/irregular humanoid | Bone+Steel (light) | none at rest |
| Vanguard's Bannerman (retinue) | narrow column + small flag | Steel | one small rectangle, above the crown, **detached** by a transparent gap |
| Lampbearer (its own spec) | point-and-halo carrier | (its own spec) | single point + halo dither |
| A Dark-dominant enemy register, if one is ever named | clean, regular, repeated | dark (reserved, unspent) | none |

Against the retinue: all three axes disagree (ragged vs. rigid-bar, light-mixed vs.
mixed-with-fire, none vs. split-fire) — the required bar for a hero against its own
crowd. Against the Vanguard's Bannerman, the two collisions worth naming explicitly are
both resolved by shape: the Bannerman's bright is one small rectangle *detached* above the
head by a gap; this bearer's bright is *segmented into several ribs, welded to the body*,
with no gap anywhere. Split-vs-single and welded-vs-detached both survive to 8px.

---

## 5. Sheet layout

- **Cell:** 48×48px. **Grid:** 4 columns × 4 rows (both powers of two, per the Niagara
  SubUV rule in `ELVTR/SETUP-EDITOR.md`). **Texture:** 192×192, `T_Hero_Bearer`, imported
  Nearest / NoMipmaps / sRGB on, material Unlit+Masked, RGB→Emissive, A→Opacity Mask.
- **Encoding: `SubImageIndex = direction × 2 + walkBit`**, matching `T_Swarm_2bit` and
  `T_Hero_Vanguard`'s own encoding (walk = bit 0).

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

Every cell filled; no transparent filler cells. Same 8-idle + one-walk-flip-per-direction
reasoning as `vanguard.md` §5 (B): facing is always correct, which matters more for the
player's own avatar than a full multi-frame stride would.

---

## 6. Animation notes

| Action | Frames | Source | What the motion communicates |
|---|---|---|---|
| Idle | 1 per direction (8 total) | v3 rotation pass | Stillness is the default read — the bearer stands, the field moves around the light (FLAME-FOUNDATION §3c), same reasoning as `vanguard.md` §6. |
| Walk | 2 per direction (idle ↔ `walk1`), 8 dirs | `walking-4-frames` template, 1 generation/direction | **Arms swing** — unlike every other bearer candidate in this round, both hands are empty, so this is the one design where a natural arm-swing is available at zero extra cost and should be used for the walk read. **The brazier itself must not bob.** `protagonist-prototypes.md` §11 flags this by name: "04's shoulder-mounted mass should not bob, or the bar smears at gameplay zoom — which means 04's walk is the one that needs the most care and probably a locked upper body." Treat the band as rigid on the spine socket; the bob, if any, belongs to the legs and lower torso only. |

**Playback:** ~4–5 fps on the walk flip, same as `vanguard.md` §6 — a hard two-beat, no
interpolation (SubUV index is a hard cut).

**What deliberately gets no frames:** attack, hit-react, death, same reasoning as
`vanguard.md` §6 — the kit is being redesigned and a keyframed swing now would spend
generations against a moveset that may not exist, and (new for this spec) against a rig
approach — procedural — that has not been decided in detail yet.

---

## 7. Light: full value inside, dimmed one value outside

Same relationship as `vanguard.md` §7, restated for this design: the bearer **is** the
source, so it is never outside its own light and renders at full ramp always — the
calibration reference every other sprite on screen dims against.

Two states this sheet must support without a second texture, using
`palette.json.dim_shift` (`{"pale":"bone","bone":"steel","steel":"dark","dark":"dark"}`,
already present in `palette.json` — this is no longer an open canon proposal, it shipped
2026-07-25):

1. **Everything else dims, the bearer does not** — a renderer-side value remap on units
   outside the leash, not a second sheet.
2. **Guttering flame (upkeep failure)** applies the same one-step-down remap **to the
   bearer itself**: the brazier's Pale drops to Bone and the ribs stop reading as fire —
   the clearest bad-news signal available in a 4-value game, and it costs one scalar on
   the Unlit+Masked material, same mechanism `vanguard.md` §7.2 specifies.

---

## 8. Buildability checklist (stage-D gate)

Reject the authored anchor and revise before approving if any of these fail:

- [ ] Exactly four values present after quantization; alpha strictly 0 or 255.
- [ ] Every Pale pixel sits between two ribs inside the brazier band; zero Pale pixels
      have a transparent orthogonal neighbour (`quantize`'s `pale_uncaged` audit) and zero
      Pale pixels appear anywhere else on the sprite (hat, face, legs).
- [ ] **No single value reaches 50% of opaque pixels.** Hand-counted estimate on the
      authored map above: Dark ≈36%, Steel ≈28%, Bone ≈26%, Pale ≈10% — a plurality for
      Dark, not a majority, which is consistent with `value_dominance: mixed` and clear of
      the "no prototype is Dark-dominant" reservation (`protagonist-prototypes.md` §2).
      This is a hand count on an authored ASCII map, not a run of the quantizer — **treat
      it as an estimate to verify, not a guarantee.** If the real render comes back with
      Dark at or above ~45–50%, thin the brazier's rib rows from four to three and drop
      the belt/hem's two full-width Dark rows to single-pixel lines before re-authoring;
      do not add Steel filler to compensate, since the Steel frame is already at its
      minimum useful width.
- [ ] The brazier band is the widest element in the silhouette, wider than the hat brim,
      wider than the shoulders below it.
- [ ] Both hands read empty — no prop anywhere on the sprite.
- [ ] Eyes are Dark notches, never Pale.
- [ ] No dither block smaller than 2×2 anywhere (applies from the rotation pass onward;
      the authored anchor itself uses no dither).
- [ ] Content bbox ≤ 48×48.

Then show the quantized south frame to the owner and get an explicit yes — this is the
only human gate in the chain (`.claude/skills/sprite/SKILL.md` stage D). On approval set
`anchor.approved: true` in `protagonist-04.json`.

---

## 9. Rig note (near-term, procedural)

The owner has confirmed 3D rigging is a near-term goal for this specific character,
animated **procedurally** rather than hand-keyed. That changes what "riggable" means from
the generic verdicts in `protagonist-prototypes.md` §6–§9: a procedural rig wants the
skeleton's hands free for whatever combat/gesture system drives it, not permanently
occupied by a held prop.

This design was already rated "yes, as specced" for a generic rig
(`protagonist-prototypes.md` §9) — a rigid brazier basket on a spine socket, empty hands.
Under the procedural-specific reading it is *more* favourable, not less: nothing about
the flame carrier touches the hand or arm bones at all, so a procedural walk, combat pose,
or gesture system inherits full freedom over both arms with zero conflict. The one
component that does need explicit rig discipline is the brazier's rigidity relative to
the spine — see §6's no-bob note, which is a procedural-animation constraint as much as a
2D flipbook one (a bouncing spine-parented mesh reads exactly as badly in 3D as a smeared
sprite does in 2D).

---

## 10. Depends on

- **GDD #5 (sprite flipbooks vs. flat-shaded 3D): the sheet in §5 assumes flipbooks**,
  same dependency `vanguard.md` §9 states for its own sheet — under flat-shaded 3D the
  8-direction rotation set becomes free and this SubUV sheet becomes unnecessary, while
  the value-remap trick in §7 moves from a material lookup to a per-object palette
  constant. The *silhouette* rules in §3–§4 and the *rig* reasoning in §9 survive either
  answer; the *sheet* does not. Given the owner's near-term procedural-3D intent for this
  specific character (§9), this dependency is closer to being spent than
  `protagonist-prototypes.md` §12 assumed when it wrote "Neither" for the whole round —
  worth flagging rather than silently carrying the flipbook assumption forward.
- **GDD #6:** resolved (strict global palette, 2026-07-12). Not re-litigated.

---

## 11. Canon proposals

**1. `palette.json.shape_carriers` needs a fifth entry for the bearer.** The registry
currently has four entries, one per class (`rectangle_flip` → Vanguard, `dot_cluster` →
Relickeeper, `thin_contour` → Pathfinder, `point_halo` → Lampbearer), and none for the
protagonist — the same gap `protagonist-prototypes.md` §13 proposal 1 already named.
Now that a winner is picked and it is class-independent, propose:
`"caged_bar": {"owner": "Bearer", "use": "the flame the player character carries, caged between ribs, never contiguous"}`.
This is a *new* shape, distinct from all four class carriers, which resolves
`protagonist-prototypes.md`'s open question cleanly: the four class carriers stay
reserved for each class's own *ability/unit* VFX (a Vanguard's banner-slam prop, a
Relickeeper's rune marks on enemies, and so on), and the player's own body uses a fifth,
dedicated shape belonging to nobody else.

**2. `vanguard.md` needs a superseded banner.** It was written as *"the winner of that
round's own argument"* for prototype 01, and 01 did not win. Its sprite spec (banner on a
pole, Steel-dominant, class-tied) is no longer the flame bearer for any class under the
owner's "one bearer" decision. I have not edited `vanguard.md` — that call belongs to
whoever owns reconciling it, since it is a real design loss (the banner-and-standard
fiction is good on its own terms) and not just a stale-doc cleanup. Two live options
worth putting to the owner: (a) mark `vanguard.md` superseded, pointing here, and treat
the Vanguard as visually identical to every other class's bearer; (b) keep the banner
alive as the Vanguard's *ability prop* rather than its hero identity — `vanguard.md` §6
already proposes a "planted banner" static world object dropped by Banner Slam, which
could absorb the rectangle-flip carrier without needing the hero's own sprite to carry it.

**3. `CLASSES.md`'s per-class "hero identity" framing needs the same correction.** Any
language implying each class has its own bearer look (the framing `flame-bearer-status.md`
§4 question 1 and `vanguard.md` canon proposal 2 both flagged as unratified) is superseded
for the *flame carrier* specifically by this decision. Class identity from here on must
be expressed through weapon, ability, and VFX — not through a different fire-carrying
body per class.

**4. Promote free-vs-caged light from a spec section to `palette.json`, now attached to a
shipped design rather than a hypothetical.** Restating `protagonist-prototypes.md` §13
proposal 2, since this spec is the first production hero sprite whose entire buildability
depends on it: `quantize` already computes `stats["pale_uncaged"]` against the rule, and a
rule a script enforces but only a prose file defines will drift. Proposed addition under
`shape_carriers`: `"caged_light": {"rule": "Pale enclosed on all four orthogonal sides by opaque pixels, never adjacent to transparency; available to any subject", "free_light": "Pale adjacent to transparency; reserved to the Lampbearer's point_halo"}`.

**No faction, biome or NPC is named anywhere in this document.** FLAME-FOUNDATION §5
holds.
