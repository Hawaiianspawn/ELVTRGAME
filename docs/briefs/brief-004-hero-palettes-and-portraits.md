---
id: 004
title: The four heroes — class palettes + portrait medallions
status: done
from: narrative-director
priority: high
faction: none/friendly (all four player classes)
biome: all (heroes cross every biome; portraits are UI-register)
class-ties: Vanguard | Relickeeper | Pathfinder | Lampbearer
spec: ../art/hero-palettes.md (umbrella) + ../art/hallam.md, ../art/edda.md, ../art/merle.md, ../art/noll.md
---

## Subject

Two deliverables in one brief, because the second is defined by the first
(portrait-register rule: a portrait is the sprite's exact palette at higher
resolution, nothing added):

**(a) The four class palettes**, under the locked Direction B architecture
(`../art/aesthetic-direction.md` §3/§4): every class palette is 4 values —
**shared Vault Dark anchor + shared steel cold-mid + shared Kitchen Tin warm-mid +
one per-class reserved bright signature**. The classes differ *only* in the bright
slot. Each bright is now grounded in a hero's story (below); you own its value,
name, and behavior on-screen.

**(b) The four hero portraits**, per `../art/portrait-register.md` (~96px busts in
128px cells, stitched-medallion framing, exact-sprite-palette rule, expression
ceiling: calm/vacant, never agony). The heroes are now fixed named individuals —
Hallam, Edda, Merle, Noll — people first; hollow helms are Still Legion language
and forbidden on all four. Every hero's face is visible.

## Mood

The heroes sit in Bree's register — lived-in, not shiny; tired hope, not glory —
but each carries a different temperature of it:

- **Hallam (Vanguard):** steadfast warmth under weight. A man who counts people
  out loud. Lamp-warm against Legion cold — and dangerously *close* to Legion, by
  design (see readability).
- **Edda (Relickeeper):** dry, patient, professional. Stone-dust and banked
  embers; the warmth of a workshop, not a hearth.
- **Merle (Pathfinder):** taut, watchful, feral-adjacent. The least warm of the
  four in manner — her warmth is all in the palette and the cord on her wrist.
- **Noll (Lampbearer):** gentle, unhurried, unsettlingly unafraid. The warmest
  light in the game, carried, not owned — lamp-warm against Vesper dark.

## Narrative excerpts (one per hero, anchoring each bright)

**Hallam — bright = the banner ("the Roll"):**
> Every person he has brought out alive is stitched onto the field in lamp-waxed
> thread, one counted name per rescue […] When he plants the banner and the
> Liberated fight to the death beneath it, they are fighting under their own names.

**Edda — bright = the rune-ember ("waking-ink"):**
> Ember-ash bound in wax, the stuff her guild used to feed ward-lines for a
> thousand years. […] "Their marks keep things still. Mine wake things up. Do not
> confuse the two in my hearing."

**Merle — bright = the quarry-mark ("waylight"):**
> The pale lamp-lichen that grows only where honest light burned long […] the mark
> is a promise made in remembered light — *you will not be left wounded in the
> dark* — and she keeps her promises with a drawn bow.

**Noll — bright = the lamp (the Foundling Lamp):**
> Never once allowed to gutter in twenty-two years: fed, trimmed, carried, but
> never relit […] the Crown put out a kingdom's lights, and missed one, and the
> one it missed is coming back down the stairs.

## Readability needs

### Palettes (a)

- **Four-player parse is shape-first, bright-second** (CLASSES.md cross-class
  rule): lines / blocks / darts / glow. The four brights must still be mutually
  distinguishable when all four retinues share a horde fight — they are the only
  slot that differs between classes.
- **Each bright has a canonical carrier** and must read there at gameplay zoom:
  Vanguard — the planted banner + Bannerman mini-flags (2-value flip animation);
  Relickeeper — Graver inscriptions/rune marks, must read *through crowds*
  (reserved-value rule, CLASSES.md §2); Pathfinder — the quarry-mark outline on
  *enemies*, must read through any horde (CLASSES.md §3); Lampbearer — wisp
  points (1px bright + 1px halo dither) and the lamp's palette-shift radius.
- **Scarcity is the grammar:** brights are earned and rare (Direction B). The
  shared three values mean a mixed party reads as one people — that is correct
  and desired; the bright is each hero's *personal* light.
- **⚠ Light-temperature law — position needed on Merle's mark.** Canon proposal 2
  (aesthetic-direction §Canon proposals — still *proposed*): warm bright = honest
  light, cold bright = Crown/deep, never swapped. The quarry-mark is a
  warm-family light **worn by hostiles** — the only bright in the game that sits
  on the enemy. The fiction leans warm (the mark is made of harvested honest
  light; it is a promise, not a taint), but a warm bright on a charging elite may
  counterfeit the safety channel at a glance. Options as we see them: (1) warm
  mark, protected by shape (outline vs. point/area — marks are the only warm
  *outline* in the game); (2) a third temperature for the mark family; (3) rule
  the mark cold and we re-ground the fiction. **The art director owns this call**;
  it likely forces the proposal-2 decision with the owner.
- Hallam's palette must pass the Bree test: Legion silhouette family, but never
  targetable-looking in a Legion fight (brief-001 precedent applies to him and
  his Liberated wholesale).

### Portraits (b)

- One medallion each per portrait-register.md; exact class palette, nothing added.
- **Hallam, 38:** heavy-boned, broken nose set crooked, short beard greying at
  the jaw, chain-gall scar ring at the throat *above the gorget — always visible*;
  Legion-pattern helm with the visor unbolted and gone (open face, on principle).
  Bright: the Roll over his shoulder — stitched names, not a glowing object; obeys
  banner scarcity. Expression: tired, kind, counting.
- **Edda, 64:** small, straight-backed, tender's cap, white hair cropped
  practical, stone-dust in the face lines, round work-spectacles (one lens is
  ward-glass — whether it may carry a bright glint is your scarcity call; fiction
  says she'd deny it). Graver strap visible. Expression: dry, appraising.
- **Merle, 22:** small, wiry, soot-marked, hair cropped with a knife, claw-scar
  through the left eyebrow, hood *down* around the shoulders, knotted token-cord
  on the left wrist in frame. Whether her bust shows any bright depends on the
  mark-temperature ruling — she may legitimately be the one hero whose portrait
  carries none (the mark lives on quarry, not on her). Expression: watchful,
  eyes-first.
- **Noll, 28:** lean, lamplight-pale, head bare ("so the lost can see a face"),
  burn-speckled hands if hands are in frame; the Foundling Lamp lights him from
  below-and-beside. Bright: lamp flame + halo only, per the Bree Watch-Lamp
  precedent. Expression: gentle, unhurried, the calm that unsettles.
- **Flicker exception** (portrait-register §3): Noll qualifies unambiguously
  (lamp). Edda's ember and Hallam's Roll-thread are yours to rule in or out;
  Merle likely static.
- Age/gender/build spread is deliberate — the four medallions side by side must
  read as four different lives, not a uniform hero row.

## Source

`../narrative/hallam.md` · `../narrative/edda.md` · `../narrative/merle.md` ·
`../narrative/noll.md` — plus system specs `../art/portrait-register.md`,
`../art/aesthetic-direction.md` (§3 Direction B, §4 decisions 1/3/5/6, canon
proposal 2), and precedent `brief-001-warden-captain-bree.md`.
