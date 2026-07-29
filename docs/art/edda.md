# The Relickeeper — hero sprite + portrait spec

**Brief:** `../briefs/brief-004-hero-palettes-and-portraits.md` · **Source fiction:** `../narrative/edda.md`
**Palette system:** `hero-palettes.md` (the Gatecamp Family) · **Class language:** CLASSES.md §2 — *mass, blocks, marks*
**Contexts:** player hero among the Awakened · inscribing (Graver work) · Gatecamp workbench

**Naming note:** role-only in every section below, per CLASSES.md's 2026-07-11 reversal of the
named-hero decision. This file keeps its legacy filename (`edda.md`) for cross-reference
stability with `CLASSES.md`, `hero-palettes.md`, and the briefs that link to it by path — only
the prose changed. She/her per `docs/narrative/edda.md`.

---

## 1. Intent

Fiction: unchanged — the last apprentice of the Vault-Tenders, a maintainer catching up on a
thousand-year backlog: stone-dust and banked embers, workshop warmth, not hearth warmth.

Gameplay, revised 2026-07-11 on two axes at once:

1. **Chibi combat register.** She renders at the oversized-head, small-simple-body proportion
   shared by every combat-scale sprite, including her own Awakened retinue and the Still
   Legion. She is *already* the smallest hero in the row under the old proportion system —
   chibi keeps that relative relationship (small figure, big blocks around her) while changing
   what "small" means in absolute silhouette terms.
2. **Shape-only class ID, precedent under review.** Her identity has ridden entirely on the
   **static glyph-dot cluster** — the Relickeeper's reserved bright-carrier shape
   (`hero-palettes.md` §3) — since all four hero classes shared one bright value-role
   (`pale` in `docs/data/art/palette.json`'s key vocabulary; the literal hex that name pointed
   to is retired, see §2). That shared-bright premise was locked under the demichrome
   global-palette gate; the 2026-07-28 colour-gate reversal (`aesthetic-direction.md`) lifts
   the gate and makes hue available again, so whether classes keep one shared bright or regain
   individual hues is an open colour decision, not yet made — flagged, not resolved, at
   handback. Rune marks read as *discrete dots forming marks*, never a contour, never a fill —
   that shape distinction is unaffected either way and stays the thing separating her
   ember-glyphs from a Vanguard banner, a Pathfinder quarry-mark, or a Lampbearer's point+halo.

She must still (a) read as the small deliberate unit among big blocks, (b) make her glyphs
readable through crowds via the reserved-shape rule (not a reserved hue anymore), and (c) keep
her professional distinction from the Crown legible: warm hand-shaped glyph vs. cold Legion
geometry, a value-role and shape distinction now, not a hue one.

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
| `dark` | Vault Dark | outline/recess; cap shadow; face lines; crack-dither on the Awakened |
| `steel` | Patched Steel | the Graver, spectacle rims, apron fittings — **and her white hair** (the warm-family mid now doing double duty as "white," register-style — no longer a cold-grey trick, since Patched Steel is warm across the whole family). Hair reads as *lighter than the anchor, distinct from skin* by adjacency, not by a cold hue anymore |
| `bone` | Kitchen Tin | face, hands, apron, strap, the sealed ink jar — warm channel |
| `pale` | **Gatecamp Bright** | the ONE shared class bright, pending the open colour decision (§1). On her it is spent **only** on glyph and seam pixels: runes on terrain/enemies, the Graver's tip while inscribing, gilded seams on over-mended Awakened. Never a fill, never a halo'd point, **never at rest on her** — the ink stays in the jar |

- **Shape protection (replaces the old hue-based "mark protection"):** no flapping rectangle,
  moving contour, or point+halo bright may appear on her sprite or her marks — only the
  static dot-cluster carrier. While every bright shares one value-role, the dot-cluster
  *shape* is the entire signal that a mark is hers to make (`hero-palettes.md` §3).
- **Light-shifted variant:** one step brighter along the value ladder — `dark`→`steel`,
  `steel`→`bone`, `bone`→`pale` (`docs/data/art/palette.json` `light_shift` rule) — no specific
  hex is proposed here anymore; the previously-proposed shift hexes were derived from the now-
  retired base trio. Gatecamp Bright does not shift — her glyphs are inscribed, not emitted;
  the sanctioned flame-core exception (`docs/art/palette-exceptions.md`) applies only to
  lamp/honest-light carriers and does not extend to her.

---

## 3. Silhouette guide — chibi proportion

Coarse mock, 1 char ≈ 2×2 px of the 48×48 cell (`#`=Vault Dark, `s`=Patched Steel,
`t`=Kitchen Tin, `g`=Gatecamp Bright, blank=mask). **Chibi baseline (working ratio, not
owner-locked): head occupies roughly the top half of the sprite's height; body/tools are a
small simple block underneath.** Flat bold fill per material, hard silhouette outline, no
shading gradient — the ramp marks armor vs. skin vs. cloth, not a light-to-dark blend.

```
Walk                        Inscribing (Ward Circle)      Awakened sentinel (contrast, non-chibi
  ,#####,   /s                    ,#####,                  block reads huge next to her)
 ##t###t#/s   <- oversized        ##t###t#                       #ss#
 ##CAP####     round head,        ##CAP###                      #ssss#
  ########     tender's cap       ########                      #ssss#
    #s#s#   <- tiny body,           #s\g   <- Graver tip lit     ##ss##
    #t#t#       Graver on            g g                        #ssss#
                strap                g   g  <- glyph forming     g  g   <- gilded seams
```

Feature rules, silhouette-priority order (chibi-adapted):

1. **Smallest hero, squarest chibi head.** Her round head reads flattest/plainest of the four —
   tender's cap, no ornament — so that at horde scale she is the one small, plain-headed figure
   the big Awakened blocks orbit. Chibi amplifies the size contrast between her and her own
   retinue, which is the point (mirrors the friendly-vs-titan contrast at a smaller scale).
2. **The Graver on the diagonal**, rising above the shoulder — still her one silhouette break
   and the class read (chisel-staff = Relickeeper the way the pole = Vanguard). While working
   it comes down to a two-handed drafting hold.
3. **Tender's cap** — flat, brimless, the clearest head-taxonomy read of the four heroes at
   chibi scale precisely because it adds nothing to the round head silhouette.
4. **Gatecamp Bright appears only where she has just been:** glyphs bloom at the Graver tip
   and stay on the world as static dot-clusters. Her trail of work is her wake.

**Reads as:** *a small, plain-headed stonemason with a chisel-staff, foreman to walking
fortress-blocks, leaving lit letters on the world.*

**Horde-scale check:** at 500 units the Awakened are the biggest, slowest silhouettes on
screen and she is the small round-headed unit they orbit. Gatecamp Bright reads through the
crowd as **static glyph clusters** — dots that do not move with any unit (terrain runes) or
seams that crack across a block (gilded state) — never confusable with the Vanguard's
flapping rectangle, the Pathfinder's moving contour, or the Lampbearer's point+halo, because no
two carriers share a shape (`hero-palettes.md` §3). Against the Crown: her glyphs are
hand-shaped dot-clusters; Legion sigils are cold geometric marks in the Legion's own cold-mid
value — value-role and shape both refuse the confusion, whether or not hue is also carrying
part of that job (§1).

---

## 4. Sheet layout

Structurally unchanged — chibi fits within the existing budget; no cell/grid resize assumed.

- Cell: **48×48 px** · Grid: **8×4** (power-of-two per axis) · Sheet: **384×192 px**
- Import/material per `ELVTR/SETUP-EDITOR.md`: Nearest, NoMipmaps, Unlit Masked,
  RGB→Emissive, A→Opacity Mask. Player hero = full Actor; SubImageIndex by state machine.

| Cell | Frame |
|---|---|
| 0–3 | Walk (4f — deliberate, low bob) |
| 4–5 | Idle (2f — she checks the strap; craftsman fidget, not soldier stillness) |
| 6–7 | Graver strike (2f — chisel jab, glyph blooms on target on frame B) |
| 8–9 | Mend Stone channel (2f — two-handed hold on a block; ember seam creeps) |
| 10–11 | Ward Circle inscribe (2f — stooped drafting pose, tip lit) |
| 12–13 | Remembrance cast (2f — Graver planted like a survey stake, both hands on it) |
| 14 | Resonance ping (1f — head turn + Graver tip flicks bright for 1 frame; the
  exploration sense made visible) |
| 15 | Stagger (1f) |
| 16–17 | Down (2f — she sets the Graver down *first*; tools don't get dropped) |
| 18–19 | Rise/revived (2f) |
| 20–21 | Workbench idle (2f — Gatecamp: fragment surgery under cloth) |
| 22–31 | Reserved / blank |

Glyphs themselves are **decals/props on separate sheets** (terrain-register), not baked into
her cells — cells 6–13 show only the tip-lit Graver; the world receives the marks.

---

## 5. Animation notes

Relickeeper-family: **mass and marks carry the class; her frames are procedure, not
flourish.** Unaffected by the chibi/palette revision — restated for the redraw pass:

- **Bright is frame-gated:** the Graver tip lights only during strike/channel/inscribe frames
  and the 1f resonance ping. Between jobs she carries an unlit tool — scarcity as animation rule.
- **Crack/gild grammar (her retinue, for reference):** damage = Vault Dark dither cracks on
  Awakened; over-mend = the same crack pattern re-drawn in Gatecamp Bright. One pattern, two
  values, opposite meanings.
- **Walk 4f at sentinel cadence:** she and her blocks share a beat.
- **Down 2f:** Graver set down upright, then her — the tool outlives the fall.

---

## 6. Portrait — medallion (per `portrait-register.md`, unaffected register)

The portrait register stays non-chibi and higher-resolution by design. What changes here is
only the palette citation — the old Waking Ember hex is retired, the medallion spends the
shared `pale`/Gatecamp Bright value-role the same way her sprite does (which, per her
scarcity rule, is: not at all, at rest).

- **Composition:** bust, near-frontal, straight-backed — the smallest face in the hero row
  and the most upright posture. Tender's cap; white hair cropped practical (Patched Steel
  mass); round work-spectacles as thin Patched Steel rims; stone-dust in the face lines
  (Vault Dark cuts + 1px dither along the knuckle-side of the jaw). Graver strap crosses
  the chest; her mother's ink jar sits on it, **sealed, its wax rendered in Kitchen Tin**.
- **Expression:** dry, appraising — direct gaze over the spectacle rims, mouth level.
- **Bright placement:** **none.** The ward-glass lens gets no glint — the fiction says she'd
  deny it, the register bans catchlights, and scarcity says the ink stays in the jar until a
  mark needs making. Hers and the Pathfinder's are the two brightless medallions, and each
  means a different thing (hers: professional restraint).
- **Flicker:** **static** (nothing to flicker). Sheet cell 1 duplicates cell 0.
- **Sheet:** `T_Portrait_Edda.png`, 256×256, 2×2 per register §4 — cell 0 base, cell 1
  duplicate, cells 2–3 reserved. Texture filename kept as-is per the cross-reference-stability
  call.

---

## 7. Depends on

- **#5 (flipbooks vs 3D):** §4–§5 assume **flipbooks on instanced quads**. Said loudly.
- **#6 (global vs per-faction):** assumes **per-faction** (warm dot-cluster vs. cold Legion
  geometry is a palette-role distinction). Under a global palette, glyph-vs-sigil would ride on
  glyph *shape* alone — functional, weaker.

---

## Canon proposals

1. **Gild = ember-seamed cracks (CLASSES.md §2, one line) — unchanged in substance, updated
   value name:** *Awakened damage renders as dark dither cracks; over-mended (gilded) state
   re-draws the same cracks in the shared class-bright value.* Locks the kintsugi read as class
   canon so no future spec invents a second gild grammar.
