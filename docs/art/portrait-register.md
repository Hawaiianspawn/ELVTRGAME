# Portrait register — system spec

**Source decisions:** `aesthetic-direction.md` §4 decisions 1, 3, 5, 6 (locked 2026-07-10)
**Craft anchors:** `Artboard/Portrait Styles/02210b4f….jpg` (dark-ground roundel busts — composition,
framing, expression ceiling) · `Artboard/Portrait Styles/c72ab868….jpg` (posterized helmet grid —
proof that ~4 values on black carry real armor; the quantize fast path) ·
`Artboard/Portrait Styles/e879a99e….jpg` (dialogue anatomy: framed scene + text box + HUD)
**Pipeline:** `ELVTR/SETUP-EDITOR.md` import discipline (Nearest, NoMipmaps, Unlit Masked)

This is a *system* spec: it governs every named-character portrait. Individual portraits get
one-line entries in their character specs (palette + expression note), not new documents.

---

## 1. Intent

Fiction: a portrait is a **medallion stitched into the kingdom's record** — to be stitched in is
to be witnessed (two-register record, canon proposal 1). Meeting a named character is the moment
the record acquires them. Gameplay: portraits anchor decision events and dialogue — the player
must instantly bind the bust to the sprite they've been fighting beside. The binding mechanism is
total palette identity: **the portrait is the sprite's palette at higher resolution, nothing
added.** Bree's bust is Vault Dark / Patched Steel / Kitchen Tin / Watch-Lamp — lamp pixel
included — and could never be mistaken for a Legion officer's, because her palette already
refuses that read.

---

## 2. Canvas and rules

### 2.1 Canvas

- **Bust art: ~96×96 px, inside a 128×128 px cell.** Transparent mask outside the medallion
  circle (mask does not count as a value).
- Rationale: 128 is power-of-two (sheet- and pipeline-friendly); a ~96px bust is **2× the 48×48
  sprite cell** — exactly one fidelity register up, per the §1c board read. At 96px, eyes are
  3–5px events and a face is carved from value masses — which is what 4 values can actually
  deliver. Bigger canvases start demanding line detail and half-tones the palette cannot pay
  for (the roundel anchor already shows quantization noise at its scale; we stay under it).
- **Display at the same screen-pixel scale as gameplay sprites** (one stitch = one pixel is a
  global law — the record does not change gauge). In the `e879a99e…` panel anatomy the medallion
  sits beside the text box at native integer scale.

### 2.2 The exact-sprite-palette rule

**A portrait uses only its character's sprite palette: 4 values + mask. No portrait-only
values, ever.**

Sprite palettes were built for armor and cloth, so faces work like this:

- **Darkest value (e.g. Vault Dark):** eye sockets, mouth line, hair mass, the shadow side of
  the face. Features are **negative space cut into the mid values** — never drawn as outlines
  on top.
- **Cold mid (e.g. Patched Steel):** armor, and the *turning plane* of the face — the boundary
  band between lit skin and shadow. This is the trick that makes 4 values enough: the cold mid
  does double duty as steel and as half-tone.
- **Warm mid (e.g. Kitchen Tin):** lit skin, and whatever the sprite already uses it for
  (straps, scarf). On friendlies this is the charm channel (decision 6b) — warmth comes from
  the palette, not from the expression.
- **Bright value:** obeys the **sprite's own scarcity rule at portrait scale**. Bree's
  Watch-Lamp appears on her lamp + halo only — *not* as eye catchlights or skin highlights.
  If a character's sprite has no bright (Still Legion palettes have none), the portrait has
  none. Scarcity is what makes the light read; the portrait must not counterfeit it.
- **Dither:** portraits are static, pixel-locked UI — so **1px dither is legal here**
  (§2.4 of the direction doc reserves fine dither for exactly this register). Use it for skin
  turning, hair texture, beard mass. The 2×2 minimum applies only to things that move.

**Craft path:** hand-drawn-first is the standard. Paint-or-photobash-then-quantize to the exact
4 values is the sanctioned fast path (`c72ab868…` is the proof it works) — but the quantize
output must be cleaned by hand: stray isolated pixels killed, dither regularized, the bright
value re-audited against the scarcity rule. Quantizers do not know which value is reserved.

### 2.3 The stitched-medallion frame

- The circular border is a **separate shared UI asset**, not part of the portrait texture —
  this keeps the exact-palette rule auditable (the portrait PNG contains 4 values + mask, full
  stop).
- Frame register: static UI (1-bit + halftone language of §1d), drawn as **counted-stitch
  sampler border** — running-stitch ring, corner motifs. The frame is a *made object*, so
  stitch grammar is legal on it (decision 5 fence); the face inside is a living person and
  must never read as fabric. The stitching is the record; the person is witnessed *through* it.
- **The unpicked hole:** the Unwitnessed get **no portraits**. Where an event would show one,
  show the medallion frame with frayed/unpicked inner edge and mask-empty center (canon
  proposal 5). This is a real asset — spec it once, reuse everywhere.
- Quiet-faction named entities (if any are ever portrayed): a dark medallion, body as
  negative space, pale eyes as the only feature — their sprite language at bust scale.

### 2.4 Light-shifted variants — **recommendation: NO**

Portraits never palette-shift with scene light. One variant per character, the base palette,
always. Reasons: (1) diegetically the medallion is the *record*, not the scene — the tapestry
in the Gatecamp doesn't get brighter because you walked in with a lamp; (2) it would double
every portrait asset for a shift the player reads better on sprites; (3) keeping the portrait
fixed makes the *sprite's* light-shift more legible by contrast when both are on screen.
The sprite light-shift variants (Bree spec §2) are unaffected.

### 2.5 Expression guidance

Global ceiling first (decision 3): **calm/vacant, never agony — anywhere a face appears.**
The record is sober. Then by alignment:

| Alignment | Expression register |
|---|---|
| Friendlies / Gatecamp | Direct gaze, lived-in, at most a settled almost-smile. Warmth rides the Kitchen-Tin channel and soft asymmetry (a tilted head, a mended collar), never a grin — coziness, not comedy (decision 6). |
| Named Legion-family (Bree et al.) | Direct, weathered, tired-but-holding. The eyes do the work; the mouth stays level. |
| Still Legion (if ever portrayed) | No face: hollow helm, visor interior solid darkest value. The portrait of an affliction, not a person (`59520576…` energy). |
| The Quiet | Vacant-mournful: pale eyes on dark mass, no mouth unless the fiction demands one. |
| The Unwitnessed | Never portrayed. Unpicked hole (§2.3). Ambiguous almost-faces belong to the *vignette* register only, and even there flirt-never-confirm. |

---

## 3. Animation policy — **recommendation: static, with one sanctioned exception**

**Base rule: portraits are static single frames.** No blink cycles — blinks would cost 2× cells
plus timing logic across every named character, and a record does not blink.

**The exception: baked light-flicker, 2 frames, only for characters whose sprite owns a light.**
Bree's medallion gets the same trick as her sprite (§5 of her spec): bright pixel offsets 1px,
halo dither rotates, alternating at a slow fixed period. The tapestry doesn't move, but the
gold thread catches the light. Cost is honest: +1 cell for lit characters, zero for everyone
else, no new grammar — it's the sprite's baked flicker at bust scale.

---

## 4. File and sheet conventions

- **Texture:** `T_Portrait_<Name>.png` — one **256×256 sheet, 2×2 grid of 128×128 cells**
  (power-of-two, consistent with the SubUV discipline even though portraits are UMG-driven):

| Cell | Content |
|---|---|
| 0 | Base bust |
| 1 | Flicker frame B (lit characters only; duplicate of 0 otherwise) |
| 2 | Reserved — alternate expression / event variant |
| 3 | Reserved — memorial variant (see Canon proposals) |

- **Import:** Filter = **Nearest**, Mip Gen = NoMipmaps, Compression = UserInterface2D,
  sRGB = on — identical discipline to `SETUP-EDITOR.md` §1.
- **Usage:** UMG Image / material with texture RGB → Emissive-equivalent, A → opacity (the
  medallion circle mask lives in the alpha). No per-portrait material work; frame selection is
  a UV offset, same mental model as SubImageIndex.
- **Shared assets:** `T_UI_MedallionFrame.png` (stitched ring, states: normal / unpicked-hole),
  spec'd once under the UI register.

---

## 5. Depends on

- **#5 (flipbooks vs flat-shaded 3D):** **Neither** — portraits are a UI register and survive
  either answer untouched.
- **#6 (global vs per-faction palettes):** assumes **per-faction** (now effectively locked by
  Direction B). The exact-sprite-palette rule inherits whatever the sprite has; under a strict
  global palette the rule still holds trivially — every portrait would share one ramp and the
  binding job would fall entirely on silhouette and the bright-scarcity signature.

---

## Canon proposals

1. **Portraits as witnessed-status (GDD, light mechanical hook):** *a named character's
   medallion is stitched into the record when first met (N-flag); the codex/record UI shows
   unmet characters as empty hoops and the Unwitnessed as the unpicked hole.* Cheap, uses only
   existing flags, and makes canon proposal 1 (two-register record) player-visible.
2. **Memorial stitch (WORLD.md tone / UI):** *when a named character dies permanently, their
   medallion's border is bound off — the running stitch closed with a finishing knot — rather
   than removed.* The record keeps its dead. Cell 3 of every portrait sheet is reserved for
   this; producing the variants can wait until permadeath of named characters is actually in.
