# The Vanguard — hero sprite + portrait spec

**Brief:** `../briefs/brief-004-hero-palettes-and-portraits.md` · **Source fiction:** `../narrative/hallam.md`
**Palette system:** `hero-palettes.md` (the Gatecamp Family) · **Class language:** CLASSES.md §1 — *lines & geometry*
**Contexts:** player hero at the line's center · Banner Slam planted · The Muster · Gatecamp (muster board)

**Naming note:** role-only in every section below, per CLASSES.md's 2026-07-11 reversal of the
named-hero decision (playable heroes have no proper name; the role *is* the identity). This file
keeps its legacy filename (`hallam.md`) for cross-reference stability with `CLASSES.md`,
`hero-palettes.md`, and the briefs that link to it by path — only the prose changed.

---

## 1. Intent

Fiction: unchanged — a man who counts people out loud, fighting under a Legion muster-flag
turned inside out and restitched with the names of everyone he brought up alive.

Gameplay, revised 2026-07-11 on two axes at once:

1. **Chibi combat register.** He renders at the oversized-head, small-simple-body proportion
   now shared by every combat-scale sprite in the game — heroes, retinue, and the Still Legion
   alike (`aesthetic-direction.md` §4 decision 6, revised). This is load-bearing, not cosmetic:
   the mirror-fight comparison against the Legion only works if both sides speak the same
   proportion language.
2. **Shape-only class ID.** His identity now rides **entirely** on the flapping-rectangle
   banner — the Vanguard's reserved bright-carrier shape (`hero-palettes.md` §3). There is no
   hue backstop anymore: every hero class renders the exact same bright hex, `#f0c260`
   Gatecamp Bright. A gold rectangle means *Vanguard, and only Vanguard*, purely because
   nothing else in the game is shaped that way.

He must still (a) be findable as the corner-stone of the biggest army on screen, and
(b) refuse a Legion-officer read before targeting resolves.

**What this costs, said loudly:** the old "Bree test" — a deliberately cold Patched Steel that
kept him literally inside the Legion's cold value family — is retired; Patched Steel is warm
now. The mirror-fight read is re-derived on **near-anchor-parity** (Vault Dark `#211210` sits
close enough in *value*, not hue, to Legion Dark `#211e20`) plus **matching chibi proportion**
doing the work the shared cold hue used to do (`hero-palettes.md` §4b). This holds at horde
scale and at distance, where the mirror sequence needs it most; it is weaker than the old system
at close, static zoom. Not an oversight — the replacement mechanism, not a repair of the old one.

---

## 2. Palette table

4 values + transparent mask (mask = background, not a value). Per `hero-palettes.md` §2–§3.

| Hex | Name | Role |
|---|---|---|
| `#211210` | Vault Dark | outline/recess; helm interior shadow; the chain-gall scar ring; hair/beard mass. Shared anchor across the whole world — this is the value doing the mirror-fight's near-anchor-parity job against Legion Dark `#211e20` |
| `#5e2d20` | Patched Steel | armor, tower shield, gorget. **No longer cold** — warm-family now, same as the other two body values. No longer part of the mirror-fight mechanism (see §1) |
| `#c76b2a` | Kitchen Tin | face, straps, tin patches — the friend/foe warm channel (Legion fills this role-slot with cold Legion Steel `#555568`) |
| `#f0c260` | **Gatecamp Bright** | the ONE shared class bright — identical hex on all four hero classes. On him it is spent **only** on banner cloth, thread, and Bannerman mini-flags: the flapping rectangle, per `hero-palettes.md` §3. Never armor, weapon, or skin. At rest: ≤2px knot on his furled banner |

- **Shape protection (replaces the old hue-based "mark protection"):** no dot-cluster, contour,
  or point+halo bright may appear anywhere on his sprite — only the flapping-rectangle carrier.
  Since every bright is the same hex now, shape is the *entire* audit (`hero-palettes.md` §3's
  registry). As a friendly he also carries no quarry-mark or rune-mark shapes.
- **Light-shifted variant:** `#211210→#35211c`, `#5e2d20→#7c4630`, `#c76b2a→#e08c46` (proposed
  values, `hero-palettes.md` §2 — not yet separately owner-locked). Gatecamp Bright does not
  shift — he carries no flame-source pixels, so the `#fff6dd` flame exception does not apply to
  him at all; his banner is remembered light, not emitted light.

---

## 3. Silhouette guide — chibi proportion

Coarse mock, 1 char ≈ 2×2 px of the 48×48 cell (`#`=Vault Dark, `s`=Patched Steel,
`t`=Kitchen Tin, `g`=Gatecamp Bright, blank=mask). **Chibi baseline (working ratio, not
owner-locked — `aesthetic-direction.md` §4 decision 6 leaves the exact head:body ratio open):
head occupies roughly the top half of the sprite's height; body/legs are a small simple block
underneath.** Flat bold fill per material region, hard silhouette outline, no shading gradient —
the 3-value ramp marks *armor vs skin vs cloth*, never a smooth light-to-dark transition.

```
March                                   Banner Slam (planted, separate prop)
   g,##########,          <- furled          |gggg
  ###t####t###ss#            knot atop       |gggg   <- gold rectangle,
 ##t##tt##tt#s#s#            oversized       |gg        2f flip vs Vault Dark
  ####OPEN####ss##           round head       |
   ############              (open face,     ###      <- planted base
     s#ss#s#                 visor gone)
      #tt#             <- tiny simple body,
      #ss#                tower shield barely
                           wider than the head
```

Feature rules, silhouette-priority order (chibi-adapted):

1. **The head carries the identity now — read it first.** At chibi scale the helm/hair/face
   mass dominates the sprite; the open-faced Legion-style helm (Kitchen Tin visible where a
   Still Legion helm holds solid Vault Dark) has to read *inside a much smaller overall
   silhouette* than the old proportion gave it. Bolder, flatter fill, no fine detail.
2. **The pole**, off-center-left, thin, gold knot at the tip, rising above the (now
   proportionally bigger) head — this still must never read as an officer crest
   (crest = 2–3px centered on the helm; pole = thin, off-axis, taller — canon proposal 1,
   unaffected by either revision).
3. **Widest hero, tiny body.** Chibi shrinks the body block for everyone, but his tower shield
   keeps him the broadest silhouette in the friendly row relative to his own small body — the
   wall the wall forms on, at any scale.
4. **The scar ring** stays a 1px Vault Dark line across the Kitchen Tin throat in every frame
   including the portrait (portrait register is unaffected by the chibi ruling — see §6).

**Reads as:** *a small round-headed Legion shield with an open face and a rolled-up flag,
standing where the line is thickest.*

**Horde-scale check:** at 500 units his side is warm-value straight lines of small round-headed
figures; he is the wide rectangle at their center with the one off-axis pole. Planted, the
banner is the only flapping rectangle on screen — nothing else in the game spends the bright on
that shape. He cannot be confused with the Relickeeper's static glyph-dot clusters, the
Pathfinder's thin moving contour, or the Lampbearer's point+halo — four disjoint shape classes,
zero hue backstop needed. Against the Legion mirror: same chibi proportion, near-anchor-parity
dark value, warm vs. cold mid — no shared hex anywhere, and that is by design (§1).

---

## 4. Sheet layout

Structurally unchanged — chibi fits within the existing budget without a cell/grid resize
(exact chibi ratio is a working assumption per §3, not yet owner-locked; a resize is not
assumed here).

- Cell: **48×48 px** · Grid: **8×4** (power-of-two per axis) · Sheet: **384×192 px**
- Import/material per `ELVTR/SETUP-EDITOR.md`: Nearest, NoMipmaps, Unlit Masked,
  RGB→Emissive, A→Opacity Mask. Player hero = full Actor; SubImageIndex driven by state
  machine, no bit-packing.

| Cell | Frame |
|---|---|
| 0–3 | March (4f) |
| 4–5 | Idle (2f — breathing; furled-banner knot sways 1px) |
| 6–7 | Melee slash (2f) |
| 8–9 | Shield Rush (2f — shield leads, body follows) |
| 10–11 | Shield Wall brace (2f — drop and lock; frame B holds) |
| 12–13 | Banner Slam (2f — raise, plant) |
| 14–15 | Planted banner prop (2f flip — the gold rectangle; separate prop cells) |
| 16–17 | The Muster cast (2f — banner high, off-hand sweep) |
| 18 | Stagger (1f) |
| 19–20 | Down (2f — shield stays upright beside him) |
| 21–22 | Rise/revived (2f) |
| 23–24 | Count-off idle (2f — Gatecamp/muster-board flavor: head turns down the line) |
| 25–31 | Reserved / blank |

---

## 5. Animation notes

Vanguard-family: **formation shape carries him; frames stay cheap. The budget exception is
the banner.** Unaffected by the chibi/palette revision — restated for the redraw pass:

- **Banner flip (cells 14–15):** the class's flair — Gatecamp Bright ↔ Vault Dark 2-value
  flip, ~2 Hz. Bannerman mini-flags reuse the same two values at 1×2px, offset phases so the
  line ripples rather than strobes.
- **March 4f, half-bob:** he bobs least in his own line — discipline as a silhouette property,
  more legible than ever now that the chibi head amplifies any bob.
- **Shield Wall 2f:** drop 1px and lock; the stillness is the statement.
- **Down 2f:** he goes down, the shield doesn't.
- **Count-off idle:** the one personality frameset (charm channel a: Gatecamp only).

---

## 6. Portrait — medallion (per `portrait-register.md`, unaffected register)

The portrait register stays non-chibi and higher-resolution by design (`aesthetic-direction.md`
§4 decision 6: chibi is the combat register only). What changes here is only the palette hex —
Roll-Gold is retired, the medallion now spends the shared Gatecamp Bright the same way his
sprite does.

- **Composition:** bust, three-quarter turn; open Legion helm framing a heavy-boned face —
  broken nose set crooked (Vault Dark cut off-axis), short beard greying at the jaw (beard
  mass Vault Dark, grey = 1px Patched Steel dither into it), **chain-gall scar ring in Vault
  Dark across the Kitchen Tin throat, above the gorget, fully visible**. The banner drapes
  over his far shoulder behind the bust: cloth mass in Patched Steel, **stitched names as
  short Gatecamp-Bright thread-strokes** — a glowing object never, a written one always.
- **Expression:** tired, kind, counting — direct gaze per the friendly row; the eyes do the
  work, the mouth stays level with the faintest settled set.
- **Bright placement:** Gatecamp Bright thread-stitches on the draped cloth only. No
  catchlights, no rim light on armor. Obeys banner scarcity at portrait scale.
- **Flicker:** **static.** Thread is not flame; flicker is reserved for living light (lamps
  only). Sheet cell 1 duplicates cell 0.
- **Sheet:** `T_Portrait_Hallam.png`, 256×256, 2×2 per register §4 — cell 0 base, cell 1
  duplicate, cells 2–3 reserved (event variant / memorial stitch). Texture filename kept as-is
  per the same cross-reference-stability call as the doc filename.

---

## 7. Depends on

- **#5 (flipbooks vs 3D):** §4–§5 assume **flipbooks on instanced quads**; sheet layout is
  meaningless under flat-shaded 3D. Said loudly.
- **#6 (global vs per-faction):** assumes **per-faction** (Gatecamp Family, warm/cold third
  slot). Under a global palette the crest/pole and open-face silhouette rules survive; the
  warm-face channel does not, and shape-only class ID would have to do even more work with an
  even smaller budget.

---

## Canon proposals

1. **Pole ≠ crest (extends spec-001 proposal 1, the Legion officer signature) — unchanged:**
   *officer crests are 2–3px, centered on the helm; standard/banner poles are 1px, off-axis,
   taller than any crest.* One line protecting every future standard-carrier from officer-reads.
