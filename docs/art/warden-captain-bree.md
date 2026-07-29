# Warden-Captain Bree — unit sprite spec

**Brief:** `../briefs/brief-001-warden-captain-bree.md` · **Source fiction:** `../narrative/warden-captain-bree.md`
**Contexts:** at her post (NPC, N1 = met) · recruited elite in a Vanguard line · absent (post held / fallen)

**Naming note:** Bree keeps her name. The 2026-07-11 role-only reversal (CLASSES.md) applies to
the four *playable* hero classes only; Bree is a named NPC seed (WORLD.md §5, "Warden-Captain
Bree") and is unaffected by that decision. Only the chibi-proportion and shape-only-palette
revisions apply to this file.

---

## 1. Intent

Fiction: unchanged — "warm the way a kept watch-lamp is warm against Legion administrative-cold;
the same uniform language as the Still Legion, but *lived-in* where theirs is immaculate."

Gameplay, revised 2026-07-11 on two axes at once:

1. **Chibi combat register.** She renders at the oversized-head, small-simple-body proportion
   now shared by every combat-scale sprite, including the Still Legion she stands opposite. This
   is load-bearing for her, not incidental: her entire silhouette argument is a **mirror-fight
   comparison**, and a mirror only works if both sides speak the same proportion language
   (`aesthetic-direction.md` §4 decision 6, revised).
2. **Shape-only class ID, mechanism replaced, precedent under review.** Her identity still rides
   on the reserved point+halo bright (`hero-palettes.md` §3), but the trick that used to protect
   her Legion-mirror read is gone: **Patched Steel is no longer cold.** The old mechanism — a
   deliberately cold armor mid-value that kept her literally inside the Legion's cold value
   family — is retired (`hero-palettes.md` §4b). That retirement, and the single-shared-bright
   premise generally, was locked under the demichrome global-palette gate; the 2026-07-28
   colour-gate reversal (`aesthetic-direction.md`) lifts the gate and makes hue available again,
   so whether a warm/cold split returns is an open colour decision, not yet made — flagged, not
   resolved, at handback (this spec is the strongest argument in the family for bringing it
   back, per §7). She still must (a) read **friendly at gameplay zoom** while wearing the Legion
   silhouette family, (b) stay **findable inside a dense friendly formation** as the wall the
   line forms on, and (c) **never read as a Legion officer.**

**The mirror-fight read, re-derived (not repaired):** with no shared hex or shared cold value
left between the Gatecamp and Legion families, her "a Legion shield with the crown scoured off"
argument now rests on two weaker, non-hex signals instead, per `hero-palettes.md` §4b:

1. **Near-anchor-parity** — Vault Dark (the `dark` value-role) and the Legion's own dark anchor
   sit close enough in *value* (not hue) that both sides still read as "one dark world," not a
   lit-vs-unlit split.
2. **Matching chibi proportion** — she and the Legion she mirrors both render at identical
   chibi proportions, so silhouette-family match does the work the shared cold hue used to do.

This is a conscious downgrade, said loudly: it holds at horde scale and at distance, where the
mirror sequence needs it most (the first-meeting vignette, the N1 stairwell approach), but it is
more vulnerable at close, static zoom than the old system, which made confusion structurally
impossible rather than merely unlikely to matter in play. Her one unchanged trick: **she still
carries the only warm bright pixel in any Legion fight** — the Still Legion palette has no
bright value at all, so her watch-lamp (point + 1px halo dither) is unforgeable at any zoom,
hue system or not.

---

## 2. Palette table

4 values + transparent mask (the mask is used for background and does **not** count as a value).
Cited by `docs/data/art/palette.json` key, not by hex — the literal hexes this table used to
carry are retired (see `docs/data/art/palette.json` `retired_hexes` for the exact list), and no
replacement hex is locked; that is an open colour decision, not resolved here (see handback).
Names and role-slots are kept as the working vocabulary for *which* value is which, decoupled
from any specific hex, per `hero-palettes.md` §2–§3.

| Key | Name | Role |
|---|---|---|
| `dark` | Vault Dark | outline / recess / underside — shared darkest value across the whole world, and the value doing the mirror-fight's near-anchor-parity job against the Legion's own dark anchor |
| `steel` | Patched Steel | armor mid-tone. **No longer cold** under the retired per-class system — warm-family now, same as every Gatecamp sprite. No longer part of the mirror-fight mechanism; see §1 |
| `bone` | Kitchen Tin | warm mid-light: tin patches, straps, face, scarf. **The friendly-reserved value-role** — the Still Legion fills this same role-slot with its own cold mid value; warm-vs-cold in the third slot is the friend/foe channel for all Legion-family sprites, unaffected by this revision |
| `pale` | **Gatecamp Bright** | bright, pending the open colour decision (§1) — same value-role as every other honest lamp in the game (retiring the old separately-named "Watch-Lamp," which is a role description, not a distinct hex). **Lamp pixel + halo dither only — never on armor, cloth, or skin.** Scarcity is what makes the lamp read; spend it anywhere else and she loses her beacon |

- **Shape protection (replaces the old hue-based "mark protection"):** no flapping rectangle,
  static dot-cluster, or moving contour may appear on her sprite — only the point+halo carrier.
  As a friendly she also carries no quarry-mark or rune-mark shapes.
- **Light-shifted variant** (inside lamp radius — her own, or the Lampbearer's): one step
  brighter along the value ladder — `dark`→`steel`, `steel`→`bone`, `bone`→`pale`
  (`docs/data/art/palette.json` `light_shift` rule) — no specific hex is proposed here anymore;
  the previously-proposed shift hexes were derived from the now-retired base trio. Gatecamp
  Bright does not shift on her either, **except** her lamp's own source pixel may render
  brighter than `pale` inside its own radius, per the sanctioned flame-core exception
  (`docs/art/palette-exceptions.md`) — she is a genuine flame-source carrier, same exception
  granted to the Lampbearer and honest-light fixtures.

---

## 3. Silhouette guide — chibi proportion

Coarse mock, 1 char ≈ 2×2 px of the 48×48 cell (`#`=Vault Dark, `s`=Patched Steel,
`t`=Kitchen Tin, `*`=Gatecamp Bright, `.`=halo dither on Vault Dark, blank=mask). **Chibi
baseline (working ratio, not owner-locked): head occupies roughly the top half of the sprite's
height; body/shield is a small simple block underneath.** Flat bold fill, hard silhouette
outline, no shading gradient.

```
Bree (march / brace)              Still Legion officer (contrast, same chibi head ratio)
     ,#####,                          ,####,     <- crest, 2-3px, centered
    ##s###s#                         ##ss##s#
   .*.ss####      <- oversized       #ssss##
   .#tss####         round head,    #s|##|s#  <- crowned shield boss
    #ssssss#         helm blank      #s|##|s#
    #s####s#                         #ssssss#
     #ss#ss#     <- tiny body,        #s##s#
     #t# #t#        tower shield,
                     face BLANK
                     (sigil scoured)
```

Feature rules, in silhouette-priority order (chibi-adapted):

1. **Full-height blank-faced tower shield**, 2px wider than any Liberated unit's — she is the
   widest rectangle in the friendly line relative to her own small chibi body. Legion shields
   carry a crown-boss (2×3px, Legion-family value); hers is scoured flat.
2. **No crest.** Bare rounded helm on the oversized chibi head — the Legion officer signature is
   the crest + crowned boss (canon proposal 1, unaffected by either revision). A Pathfinder
   scanning for crests skips her before color even resolves, and the chibi head makes the
   crest/no-crest read even more immediate (it dominates a bigger share of the silhouette now).
3. **The watch-lamp**: 1 bright pixel + 1px dither halo, hung at her left shoulder-hook, present
   in every frame including death.

**Reads as:** *a small round-headed Legion shield with the crown scoured off and one warm light
kept on it.*

**Horde-scale check:** at 500 units mid Legion-mirror-fight, the Vanguard side is warm-value
lines of round-headed chibi figures, the Legion side is cold-value lines of the *same-proportion*
round-headed figures, and Bree is **the single steady bright point inside the friendly
geometry** — the corner-stone the wall visibly forms on. She cannot be confused with a
Bannerman (flag = 2-value *flip animation*, motion-coded rectangle; her lamp is a static point)
nor with Lampbearer wisps (a drifting constellation of many points; she is one point locked into
a marching rectangle) — shape and motion carry the whole distinction, whether or not hue is
also backing it up (§1).

---

## 4. Sheet layout

- Cell: **48×48 px** · Grid: **4×4** (power-of-two per pipeline) · Sheet: **192×192 px**
- Import/material per `ELVTR/SETUP-EDITOR.md`: Nearest, NoMipmaps, Unlit Masked, RGB→Emissive, A→Opacity Mask.
- Bree is a **promoted elite / full Actor** (GDD §10), so SubImageIndex is driven directly by
  her state machine — no bit-packed encoding needed, unlike the swarm sheet. One sheet, one
  material, still instanced-quad-friendly. Grid/cell size unchanged by the chibi revision (no
  resize assumed — see hallam.md §4 for the same call across the hero family).

| Cell | Frame |
|---|---|
| 0–3 | March cycle (4f) |
| 4–5 | Post idle (2f — breathing, lamp flicker) |
| 6–7 | Shield Wall brace (2f — drop and lock) |
| 8–9 | Lamp-raise (2f — first-meeting vignette / E8 ask) |
| 10 | Stagger (1f) |
| 11–12 | Fall (2f — N1 → fallen) |
| 13–14 | Stand-the-watch kneel (2f — Option B event idle) |
| 15 | Reserved / blank |

---

## 5. Animation notes

She is Vanguard-family: **formation shape carries her, frames stay cheap** — the budget
exception is the lamp. Unaffected by the chibi/palette revision — restated for the redraw pass:

- **Lamp flicker is baked, not extra cells:** in every 2f+ cycle the bright pixel offsets 1px
  and the halo dither pattern rotates between frames. Free warmth on every loop.
- **March 4f** (Liberated get 2f): half-speed cadence, deliberate — she out-disciplines her own
  troops. In a moving line she is the unit that bobs least.
- **Shield Wall brace 2f:** shield drops 1px and locks; frame B holds indefinitely.
- **Fall 2f:** shield stays upright in frame 1, only she goes down; final frame leaves shield
  standing and lamp lit beside her. If N1 → fallen, the world takes the light later, not this
  frame.

---

## 6. Depends on

- **#5 (flipbooks vs 3D):** assumes **flipbooks on instanced quads**. §4 and §5 are meaningless
  under flat-shaded 3D — this spec only works under the flipbook answer. Said loudly.
- **#6 (global vs per-faction palettes):** palette table assumes **per-faction swaps**
  (warm/cold in the third value slot). Under a strict global palette, the silhouette rules
  (blank shield, no crest, lamp scarcity) survive intact, but the warm/cold friend-foe channel
  does not — friend/foe would ride on the lamp + faction value-roles alone. Functional, weaker.

---

## Canon proposals

1. **Legion officer art signature** (for `CLASSES.md` cross-class silhouette rules or
   `WORLD.md` §3b) — unchanged: *Still Legion officers read by helmet crest (2–3px) + crowned
   shield boss; formations decay when the crest falls.* This spec depends on that signature
   existing so her de-crested, scoured silhouette is a legible refusal, not an accident.
2. **The honest-light rule** (extends CLASSES.md §4's lamp-radius palette shift beyond the
   Lampbearer) — unchanged: *named "honest lights" — Bree's watch-lamp, relit braziers,
   safe-room lamps — apply the same one-value-brighter room shift in a small radius.* Brief
   002's held-stairwell state also needs this.
