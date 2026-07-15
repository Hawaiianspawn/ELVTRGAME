# The Pathfinder — hero sprite + portrait spec

**Brief:** `../briefs/brief-004-hero-palettes-and-portraits.md` · **Source fiction:** `../narrative/merle.md`
**Palette system:** `hero-palettes.md` (the Gatecamp Family; §4a = the quarry-mark grammar this spec binds to)
**Class language:** CLASSES.md §3 — *motion, darts, marks* · **Contexts:** ahead of the party · marking · Gatecamp wall-top

**Naming note:** role-only in every section below, per CLASSES.md's 2026-07-11 reversal of the
named-hero decision. This file keeps its legacy filename (`merle.md`) for cross-reference
stability with `CLASSES.md`, `hero-palettes.md`, and the briefs that link to it by path — only
the prose changed. They/them per `docs/narrative/merle.md` (deliberate — see the face, §3 below).

---

## 1. Intent

Fiction: unchanged — the trapper who keeps the hunt-law like scripture, *nothing you mark is
ever left wounded in the dark*, painting promises in harvested honest light on the things that
must die.

Gameplay, revised 2026-07-11 on two axes at once:

1. **Chibi combat register.** They render at the oversized-head, small-simple-body proportion
   shared by every combat-scale sprite. The Pathfinder's whole identity was already about
   being the smallest, quickest silhouette in the row; chibi keeps that relationship while
   resetting what "small" looks like — the bow arc stays the one curve in an otherwise
   round-headed hero row.
2. **Shape-only class ID, mark grammar re-derived.** Their identity rides entirely on the
   **thin moving contour** — the Pathfinder's reserved bright-carrier shape (`hero-palettes.md`
   §3). Because every hero bright is now the literal same hex, the quarry-mark's old
   belt-and-suspenders insurance — a hue (`#d9f0b8`, pale green) deliberately separated from
   the lamp's hue — is **gone** (`hero-palettes.md` §4a). What survives: the grammar argument
   that a warm contour riding an enemy silhouette shares zero shape-signals with the
   point+halo/stillness safety channel. That argument still holds; it just carries the whole
   load alone now, with no hue backstop if a 1px read ever gets ambiguous. Flagging this here
   per the doc's own instruction not to let a future artist rediscover the loss by shipping a
   confusable asset.

They must still (a) read as motion — the one friendly sprite that never marches in step, and
(b) carry no bright of their own at rest — their light is spent entirely on others.

---

## 2. Palette table

4 values + transparent mask (mask = background, not a value). Per `hero-palettes.md` §2–§3.

| Hex | Name | Role |
|---|---|---|
| `#211210` | Vault Dark | outline/recess; cropped hair; soot marks; the claw-scar; bowstring |
| `#5e2d20` | Patched Steel | bow limbs, knife, arrowheads, buckles |
| `#c76b2a` | Kitchen Tin | face, hands, hood (worn down), quiver leather, the token-cord — warm channel |
| `#f0c260` | **Gatecamp Bright** | the ONE shared class bright — same literal hex as the other three classes, no longer a distinct pale-green Waylight. On them it is spent **only** as a contour and pip: 1px outline hugging a marked enemy's silhouette; snare-line anchor pips; the 2f marking-cast tip. Never a fill, never a halo'd point, **never on any friendly sprite — including this hero at rest** |

- **Shape protection (replaces the old hue-based "mark protection"):** no flapping rectangle,
  static dot-cluster, or point+halo bright may appear anywhere in their kit — only the thin
  moving contour and its anchor pips. This is now the *only* thing distinguishing a quarry-mark
  from a Lampbearer's point+halo at 1px: contour rides a silhouette and always moves with the
  target; point+halo is circular, haloed, and still-or-drifting independent of any enemy. See
  §1 for what was lost when the hue insurance retired.
- **Light-shifted variant:** `#211210→#35211c`, `#5e2d20→#7c4630`, `#c76b2a→#e08c46` (proposed,
  `hero-palettes.md` §2). Gatecamp Bright does not shift — the mark is a claim, not emitted
  light; the `#fff6dd` flame exception does not extend to it.

---

## 3. Silhouette guide — chibi proportion

Coarse mock, 1 char ≈ 2×2 px of the 48×48 cell (`#`=Vault Dark, `s`=Patched Steel,
`t`=Kitchen Tin, `g`=Gatecamp Bright, blank=mask). **Chibi baseline (working ratio, not
owner-locked): head occupies roughly the top half of the sprite's height; body/limbs are a
small simple block underneath.** Flat bold fill, hard silhouette outline, no shading gradient.

```
Draw                        Marked quarry (any enemy)        Snare line
   ,####,                        gggg
  ##t##t#(   <- oversized       g####g    <- 1px bright        g........g
  ##HOOD##(     round head,    g##ss##g      contour riding    ^ anchor pips only;
   ######        bow arc on    g##ss##g      the enemy's own      the line itself is
    #s#(         left side      g####g       silhouette            Vault Dark dither
    #t#(                         g  g
```

Feature rules, silhouette-priority order (chibi-adapted):

1. **Smallest, lightest, fastest — round head, no ornament.** The bow arc on their left is the
   only curve in the hero row (everyone else is rectangles and verticals); at chibi scale that
   curve reads even more clearly against four round-headed silhouettes.
2. **Hood down.** A soft bump at the shoulders, face open — hood-*up* is forbidden on their
   friendly read (a cowled dart shape drifts toward Quiet language). The head-taxonomy slot:
   open helm / cap / **hood-down** / bare.
3. **Motion is the identity** (CLASSES.md §3): they get the highest hero frame count and the
   only 6f run. At rest they perch; they never stand in rank.
4. **The mark is never theirs to wear.** No bright pixel exists on their body in any frame;
   during the 2f marking cast the value appears at the drawn arrowhead only, then lives on
   the quarry.

**Reads as:** *the small quick round-headed shape ahead of the army, and the bright contour on
the thing that is about to die.*

**Horde-scale check:** at 500 units they are the one friendly sprite crossing the grain of the
formations. The mark reads as a **moving contour locked to an enemy silhouette** — visible
through the crowd because no other bright is a contour and no friendly is ever outlined in any
bright. Counterfeit audit (per §1/§2 above): a charging marked elite shares zero *shape* signals
with safety — safety is a point+halo that is still or drifts independently; the mark is a
haloless contour that always tracks its target. Against the other brights: not a rectangle, not
a dot-cluster, not a point+halo — shape carries the entire distinction now, since hue no longer
does (the old "pale green-white, the only non-flame bright in the game" line is retired along
with the hue system it described).

---

## 4. Sheet layout

Structurally unchanged — chibi fits within the existing budget; no cell/grid resize assumed.

- Cell: **48×48 px** · Grid: **8×4** (power-of-two per axis) · Sheet: **384×192 px**
- Import/material per `ELVTR/SETUP-EDITOR.md`: Nearest, NoMipmaps, Unlit Masked,
  RGB→Emissive, A→Opacity Mask. Player hero = full Actor; SubImageIndex by state machine.

| Cell | Frame |
|---|---|
| 0–5 | Run (6f — the only 6f cycle in the hero row; class budget spent here) |
| 6–7 | Idle (2f — weight shifts, head checks an exit; never stands still *still*) |
| 8–10 | Draw–loose (3f — draw, anchor, loose) |
| 11–12 | Charged pierce shot (2f — deeper anchor, 1px longer bow flex) |
| 13–14 | Mark Quarry cast (2f — arrowhead tips bright on frame A; quarry contour decal
  spawns on frame B) |
| 15–16 | Snare Line place (2f — kneel, pin; anchor pips are prop decals, not their pixels) |
| 17–18 | The Hunt Is Called (2f — whistle stance, cord-hand raised) |
| 19–20 | Whistle commands (2f — pack stance signals; reused for kennel-whistles) |
| 21–22 | Dodge/slide (2f) |
| 23 | Stagger (1f) |
| 24–25 | Down (2f) |
| 26–27 | Rise/revived (2f) |
| 28–29 | Perch idle (2f — Gatecamp wall-top crouch, cord-hand at the wrist) |
| 30–31 | Reserved / blank |

The quarry contour is a **decal/overlay asset, not baked frames** — it must ride *any* enemy
sheet. Implementation note: a 1px silhouette-edge outline in Gatecamp Bright, rendered as a
duplicate quad behind the marked enemy's own frame (cheap material parameter or second SubUV
draw — never per-unit material work, per pipeline).

---

## 5. Animation notes

Pathfinder-family: **few sprites, high animation budget each — they and the pack dart while
armies march.** Unaffected by the chibi/palette revision — restated for the redraw pass:

- **Run 6f** is the class statement; every other hero gets 4f.
- **Draw–loose 3f** with a held anchor frame: the pause before the loose is the hunt-law made
  visible.
- **Marking cast 2f:** the bright exists on their sprite for exactly one frame (the tipped
  arrowhead), then belongs to the quarry. Scarcity as animation rule.
- **Cord-touch:** in idle and perch cycles the right hand rests on the left-wrist cord for 1
  frame per loop — the token-cord as a tic, not an event.

---

## 6. Portrait — medallion (per `portrait-register.md`, unaffected register)

The portrait register stays non-chibi and higher-resolution by design. Nothing changes here
except that there is no separate Waylight hex to omit — the medallion still carries **no**
bright at all, which is now trivially consistent with the shared-hex system rather than a
special case.

- **Composition:** bust, slight turn, chin a touch down — **eyes-first**: the widest-open
  eyes in the hero row (4px events), gaze direct per the friendly register but reading like
  they are watching *you* watch them. Hair cropped rough with a knife (ragged Vault Dark
  edge); claw-scar splitting the left eyebrow (1px Vault Dark cut breaking the brow line);
  soot marks as sparse Vault Dark dither on the cheekbone. Hood down in soft Kitchen Tin
  folds around the shoulders; the left hand raised to the collarbone so the **knotted
  token-cord** (Kitchen Tin knots, Vault Dark gaps) sits in frame.
- **Expression:** watchful — level mouth, no almost-smile. The warmth is the palette and the
  cord, exactly as briefed.
- **Bright placement:** **none.** The mark lives on quarry, not on them — the one hero whose
  medallion carries no light, and that is the truest portrait of them: their light is spent on
  others. (Contrast with the Relickeeper's brightless medallion, which means restraint; this
  one means promise-kept-elsewhere.)
- **Flicker:** **static.** Sheet cell 1 duplicates cell 0.
- **Sheet:** `T_Portrait_Merle.png`, 256×256, 2×2 per register §4 — cell 0 base, cell 1
  duplicate, cells 2–3 reserved. Texture filename kept as-is per the cross-reference-stability
  call.

---

## 7. Depends on

- **#5 (flipbooks vs 3D):** §4–§5 assume **flipbooks on instanced quads**; the mark-decal note
  assumes quad rendering. Said loudly.
- **#6 (global vs per-faction):** assumes **per-faction**. Under a global palette the mark
  survives *better* than most signatures (it is still a reserved-shape carrier either way), but
  the warm-vs-cold reasoning underneath the retired hue-insurance system no longer applies.

---

## Canon proposals

None beyond what `hero-palettes.md` §4a already documents (the quarry-mark's hue insurance is
gone; this spec is the file that has to live with it day to day, flagged above in §1/§2).
