# The Lampbearer — hero sprite + portrait spec

**Brief:** `../briefs/brief-004-hero-palettes-and-portraits.md` · **Source fiction:** `../narrative/noll.md`
**Palette system:** `hero-palettes.md` (the Gatecamp Family; §4c = the one-flame rule, now trivially true)
**Class language:** CLASSES.md §4 — *points of glow; the room itself brightens* · **Contexts:** center of the constellation · Sanctuary · alone in the dark, on purpose

**Naming note:** role-only in every section below, per CLASSES.md's 2026-07-11 reversal of the
named-hero decision. This file keeps its legacy filename (`noll.md`) for cross-reference
stability with `CLASSES.md`, `hero-palettes.md`, and the briefs that link to it by path — only
the prose changed. **Pronoun correction (this pass): she/her**, matching the current
`docs/narrative/noll.md` (2026-07-11 origin rewrite) and CLASSES.md §4, which this spec's
previous he/him draft predates and had not been reconciled with until now.

---

## 1. Intent

Fiction: unchanged in substance, corrected in pronoun — the Lampbearer does not carry her own
lamp; she carries her mentor's, the Vesper Halls tender who trained her and died with it in
reach. Head bare *so the lost can see a face.*

Gameplay, revised 2026-07-11 on two axes at once:

1. **Chibi combat register.** She renders at the oversized-head, small-simple-body proportion
   shared by every combat-scale sprite. Bare-headed chibi is the cheapest possible silhouette
   in the game — no helm, cap, or hood geometry to draw at any scale — which dovetails with her
   existing "cheapest hero to render richly" design goal (CLASSES.md §4).
2. **Shape-only class ID, one-flame rule now trivial, precedent under review.** Her identity has
   ridden entirely on the **point+halo** — the Lampbearer's reserved bright-carrier shape
   (`hero-palettes.md` §3). The old "her lamp and the Warden-Captain's lamp share a hex on
   purpose" canon proposal used to be a specific, arguable claim; under the shared-bright lock
   it became true **by construction** — every bright in the game held one value-role, so the
   point+halo *shape* was what was actually reserved to lamps, not a specific hex
   (`hero-palettes.md` §4c). The diegetic "every honest flame descends from the Foundling Lamp"
   reading is still nice to have in WORLD.md but was no longer doing mechanical work under that
   lock. The 2026-07-28 colour-gate reversal (`aesthetic-direction.md`) lifts the lock and makes
   hue available again — whether lamps keep a shared value-role or regain a dedicated hue is an
   open colour decision, not yet made; the point+halo shape carries the class either way.

She must still (a) read as the warm point that moves like a person, distinguishable at any
zoom from the Warden-Captain (point locked in a marching rectangle) and from safe rooms (still
points on architecture); (b) be the cheapest hero to render richly — her constellation and her
palette-shift radius do the visual work; (c) own the game's warmest register without ever
spending the shared bright on anything but flame, halo, and wisps.

---

## 2. Palette table

4 values + transparent mask (mask = background, not a value). Cited by
`docs/data/art/palette.json` key, not by hex — the literal hexes this table used to carry are
retired (see `docs/data/art/palette.json` `retired_hexes` for the exact list), and no
replacement hex is locked; that is an open colour decision, not resolved here (see handback).
Names and role-slots are kept as the working vocabulary for *which* value is which, decoupled
from any specific hex, per `hero-palettes.md` §2–§3.

| Key | Name | Role |
|---|---|---|
| `dark` | Vault Dark | outline/recess; hair; burn-speckles on hands/forearms; halo dither ground |
| `steel` | Patched Steel | the Lantern-Staff, lamp housing, belt fittings |
| `bone` | Kitchen Tin | face, hands, long coat, wick-bag — warm channel; she is the most Kitchen-Tin hero (least armor in the row) |
| `pale` | **Gatecamp Bright** | the ONE shared class bright, pending the open colour decision (§1). On her it is spent **only** on flame pixel(s), 1px halo dither, and Guided wisp points. Never coat, skin, or eyes. She is the one carrier allowed the sanctioned flame-core exception (`docs/art/palette-exceptions.md`): source pixels may render brighter than `pale` inside their own radius — a flapping rectangle, a glyph-dot cluster, and a contour reflect remembered light; she and honest-light fixtures are the only carriers that *emit* it |

- **Shape protection (replaces the old hue-based "mark protection"):** no flapping rectangle,
  static dot-cluster, or moving contour may appear anywhere on her sprite or her wisps — only
  point+halo. While every bright shares one value-role, point+halo is the only shape reserved
  to lamps and honest-light fixtures.
- **Light-shifted variant:** she *is* the shift — the trio steps brighter per the value ladder
  everywhere inside lamp radius, on her, her allies, and the room: `dark`→`steel`,
  `steel`→`bone`, `bone`→`pale` (`docs/data/art/palette.json` `light_shift` rule) — no specific
  hex is proposed here anymore; the previously-proposed shift hexes were derived from the now-
  retired base trio. Her own sprite is almost always rendered in the shifted trio; author both
  variants, the base is for the Flare window (see §5) and doused states.

---

## 3. Silhouette guide — chibi proportion

Coarse mock, 1 char ≈ 2×2 px of the 48×48 cell (`#`=Vault Dark, `s`=Patched Steel,
`t`=Kitchen Tin, `*`=Gatecamp Bright, `.`=halo dither, blank=mask). **Chibi baseline (working
ratio, not owner-locked): head occupies roughly the top half of the sprite's height; body/coat
is a small simple block underneath.** Flat bold fill, hard silhouette outline, no shading
gradient.

```
Walk                          Sanctuary (lamp planted)     vs Warden-Captain (contrast)
   ,####,     <- BARE           ...                            ,##ss,  <- helm
  ##t##t#        oversized     .***.   <- lamp on staff,      .*.ss## <- lamp locked
  ##t##t#        round head,   .....      planted; wisps      .#tss## <- at shoulder,
   ######        only bare      o   o     orbit the rim       #ssssss#   in a marching
    #s#      <- lamp at        o     o                        #s####s#   rectangle
    #.*.#       hand height,                                              (round chibi
     #t#        halo             (o = wisp: 1px * + 1px .)                 head too)
```

Feature rules, silhouette-priority order (chibi-adapted):

1. **Bare head.** The only helmetless, capless, hoodless hero — at chibi scale this is the
   cheapest possible silhouette in the game (a round shape with no added geometry) and reads at
   any distance. Her crown is the absence of one.
2. **The carried point.** One Gatecamp Bright pixel + 1px halo at staff/hand height, present in
   every frame including down. It travels at person-cadence, inside a drifting constellation,
   inside a room one value brighter: three concentric signals, all hers.
3. **Lean vertical, tiny chibi body.** Narrowest hero — a staff-line of a woman beside the
   Vanguard's rectangle, now doubly true once both render round chibi heads on differently-
   proportioned tiny bodies. Long coat, no shield, no pack: nothing about her silhouette says
   *fighter*, which is correct.
4. **The Guided are her geometry:** wisps = 1px bright + 1px halo dither each (cheapest sprites
   in the game, Niagara-native per C6). She is wherever the constellation thins toward a center.

**Reads as:** *one warm light walking like a small round-headed woman, with a sky of small
lights around her and a room that believes her.*

**Horde-scale check:** at 500 units the Lampbearer read is unmistakable: the palette-shift
radius is visible before any sprite resolves. Vs. the Warden-Captain: her point is locked into
a marching shield-rectangle and shifts a room only per the honest-light rule's small radius;
this hero's point moves at hero cadence inside a drifting constellation — same value-role, same
point+halo shape, disambiguated entirely by motion and context (this was already true when both
shared the retired Watch-Lamp hex under the old per-class system; the shape argument does not
depend on which colour answer is live). Vs. safe rooms: their points are still and their rooms
don't move. Vs. the other brights: a haloed point is neither rectangle, dot-cluster, nor
contour. She is also the party's failure-state indicator: when the constellation collapses to a
Huddle, everyone on screen can read *last stand* from the shape alone.

---

## 4. Sheet layout

Structurally unchanged — chibi fits within the existing budget; no cell/grid resize assumed.

- Cell: **48×48 px** · Grid: **8×4** (power-of-two per axis) · Sheet: **384×192 px**
- Import/material per `ELVTR/SETUP-EDITOR.md`: Nearest, NoMipmaps, Unlit Masked,
  RGB→Emissive, A→Opacity Mask. Player hero = full Actor; SubImageIndex by state machine.
  Wisps are **not** on this sheet (Niagara particles per CLASSES.md §4 / C6).

| Cell | Frame |
|---|---|
| 0–3 | Walk (4f — unhurried; she has never once hurried in the dark, and the cycle says so) |
| 4–5 | Idle (2f — lamp flicker baked: bright pixel offsets 1px, halo dither rotates) |
| 6–7 | Lantern-cone sweep (2f — staff arcs, revealing light; the cone itself is a decal) |
| 8–9 | Kindle channel (2f — lamp extended toward the target, head bowed) |
| 10–11 | Sanctuary plant (2f — lamp set down from staff to ground; the parting of woman and
  light is a 2-frame event on purpose) |
| 12–13 | Daybreak cast (2f — lamp raised two-handed overhead) |
| 14 | Flare cast (1f — the constellation leaves; she holds the empty gesture) |
| 15–16 | Soothe / raise-the-lamp (2f — ritual: lamp lifted to eye level, one slow look;
  reused for the mentor's-soul release beat) |
| 17 | Stagger (1f) |
| 18–19 | Down (2f — the lamp sits upright and lit beside her) |
| 20–21 | Rise/revived (2f) |
| 22–23 | Vigil kneel (2f — Gatecamp last-office idle; also the gate-hook scene) |
| 24–31 | Reserved / blank |

---

## 5. Animation notes

Lampbearer-family: **the cheapest hero — flicker is baked, the constellation and the room do
the motion.** Unaffected by the chibi/palette revision — restated for the redraw pass, pronoun
corrected:

- **Lamp flicker in every 2f+ cycle:** bright pixel offsets 1px, halo dither rotates —
  identical grammar to the Warden-Captain and her stairwell, offset phase when near either so
  honest lights never strobe in sync.
- **Walk 4f, lowest bob in the row after the Warden-Captain:** calm as cadence.
- **Flare window (cell 14):** the one time her sprite drops to the unshifted base trio — the
  constellation and its light have left her. Vulnerability rendered as palette, zero extra
  frames.
- **Down 2f:** she falls, the lamp doesn't. Non-negotiable: the flame has not guttered in
  decades and no death animation gets to be the first.
- **Raise-the-lamp 2f** is deliberately slow-tempo when driven by the soul-release beat: one
  look per soul, and the party waits. The animation is the cost made visible.

---

## 6. Portrait — medallion (per `portrait-register.md`, unaffected register)

The portrait register stays non-chibi and higher-resolution by design. What changes here is
only the palette citation and the pronoun — the old Watch-Lamp hex is retired (same role, the
value-role now cited as `pale`/Gatecamp Bright pending the open colour decision, §1), and every
"he/his" below is corrected to "she/her."

- **Composition:** bust, near-frontal, head bare; the **lamp intrudes at the lower-left frame
  edge** — flame pixels + halo dither, lighting her **from below and beside**: lit planes
  (Kitchen Tin) on the *undersides* of brow, nose, jaw; the Patched Steel turning-band runs
  *above* the lit planes — the register's face logic, inverted vertically. It should be the
  only portrait in the game whose shadow falls upward. If the cradling hand is in frame,
  burn-speckles = sparse Vault Dark dots.
- **Expression:** gentle, unhurried, a settled almost-smile at the ceiling's exact limit — the
  calm that unsettles. Direct gaze; early creases at the eyes in 1px dither.
- **Bright placement:** lamp flame + halo only, per the shared-lamp precedent. **No catchlights
  in the eyes** — the register bans it and the restraint is the point: the light is carried,
  not owned.
- **Flicker:** **YES — the unambiguous qualifier.** 2f baked flicker per register §3: bright
  pixel offsets 1px, halo dither rotates, slow fixed period. Because the lamp lights the face,
  the flicker may also swap **≤4 boundary pixels** between Kitchen Tin and Patched Steel along
  the jaw's turning band (the light breathes on her) — no other pixels move, no new values
  appear.
- **Sheet:** `T_Portrait_Noll.png`, 256×256, 2×2 per register §4 — cell 0 base, **cell 1
  flicker frame B**, cells 2–3 reserved. Texture filename kept as-is per the
  cross-reference-stability call.

---

## 7. Depends on

- **#5 (flipbooks vs 3D):** §4–§5 assume **flipbooks on instanced quads**; wisp notes assume
  Niagara sprites (C6 lean). Said loudly.
- **#6 (global vs per-faction):** assumes **per-faction** — the palette-shift radius *is* a
  palette-swap mechanic; under a strict global palette her entire class read degrades to the
  point-and-halo alone. This spec is the strongest argument in the family for the per-faction
  answer.
- **Pending:** spec-001 canon proposal 2 (honest-light rule) for fixture radii.

---

## Canon proposals

1. **The lamp never falls (CLASSES.md §4 / event art, one line) — unchanged:** *in every state
   including hero-down, the lamp renders lit and upright; no animation, vignette, or death
   frame may show it guttered.* A fence protecting the class's central fact.
