# 2-Bit Dithered Lighting — Rendering Plan

**Version:** 0.1 (draft from owner's shader concept, 2026-07-10) · Companion to `GDD.md` §10
**Status:** proposed — Phase A is the next art test

## 1. The idea

Dynamic light casting in a strict 2-bit world: no alpha blending, no smooth falloff.
Light attenuation is simulated with **ordered (Bayer) dithering** — as light intensity
drops with distance, more pixels of the dither matrix flip to the darker palette value,
so light "scatters" into darkness through pure 2-bit patterns. Shadows come from
raymarching a 2D occluder mask. The whole thing runs as a **full-screen post pass**,
which is exactly right for ELVTR: post-process cost is per-pixel and flat — it does
not grow with entity count. The horde pillar and this lighting model don't fight.

Source concept: owner-provided GLSL sketch (radial light → `pow(x,2)` falloff →
Bayer 4×4 threshold → 4-color quantization), archived in §7 notes below where adapted.

## 2. What changes for ELVTR (vs. the sketch)

### 2.1 Quantize palette *indices*, not gray values
The sketch averages RGB to gray and thresholds into 4 colors. That destroys the two
things our art system is built on: **faction-reserved values** and per-class color
identity (CLASSES.md cross-class rules). Instead:

- Sprites/tiles are authored as **value indices 0–3** plus a **palette ID**
  (faction/biome). The scene renders unlit, emissive-encoding `(value index, palette ID)`
  into two channels with wide separation (quantize on read — survives the pipeline).
- The post pass computes per-pixel light, converts it to a **value-step shift**
  (light +1/+2 steps, darkness −1/−2 steps), with the Bayer threshold deciding
  rounding at each pixel — that's the dithered falloff.
- A final **LUT texture** maps `(palette ID, shifted value index) → RGB`.

Payoffs:
- **The Lampbearer's mechanic falls out for free.** CLASSES.md already promises
  "rooms shift one value brighter inside lamp radius" — that *is* the +1 value step.
  Daybreak = whole-screen LUT row swap for a few seconds. Trivial.
- **GDD §12 #6 (global vs. per-faction palette) stops being an architecture decision.**
  Per-faction palettes are LUT rows; a strict global palette is a LUT with one row.
  The code is identical; the choice becomes art data the pixel-art-director owns.
- Reserved values (quarry marks, rune marks, faction channel) can be declared
  **light-exempt** in the LUT — they read through darkness by construction (pillar 4).

### 2.2 Lock the dither to the virtual pixel grid
Screen-space Bayer on a native-res render makes dither pixels smaller than art
pixels — it reads as noise, not craft. Decide a **virtual resolution** (art test:
try 640×360-ish so 48px sprites keep presence), render scene color at that res,
run the entire lighting/dither/LUT pass there, then **nearest-upscale** to native.
Bonus: the raymarch and post pass run at 1/9th the pixels or less.

### 2.3 Shadows: we already know where the walls are
The sketch offers raymarching a collision buffer (A) or engine 2D shadow maps (B).
B doesn't exist for us — everything is unlit, and this is 3D UE, not Godot 2D.
A fits unusually well because **the floor is procedurally generated**: the generator
can bake a 1-bit occluder mask per floor directly into a texture — no SceneCapture,
no per-frame cost. Post pass raymarches from pixel toward each light through that
mask; a hit zeroes the light before dithering, so shadow edges dither into darkness
automatically.

Budget: ≤ 8–16 active lights on screen, 16–32 raymarch steps, computed at the
virtual res (optionally a half-res light buffer upsampled under the dither).
Light data (position, radius, flicker seed) via a Material Parameter Collection
or a small data texture updated from gameplay.

### 2.4 Lighting is a gameplay system wearing a shader
Light sources are already fiction and mechanics, not decoration: braziers and
relit routes (world flag S7), Sanctuary/Vigil, the hero's lamp, titan-fire.
The Quiet *extinguish* lights (WORLD.md §3c) — so the light list must be driven by
gameplay state, and the darkness budget per floor (WORLD.md §2) becomes a real,
tunable number: ambient value-step per biome (Highgates −0, Vesper Halls −2).

### 2.5 Keep the sketch's soul
Adopt unchanged: exponential/inverse-square falloff ("the dark feels heavier"),
flicker via time-noise on radius (torch claustrophobia for pennies), PALETTE[0]
as heavy midnight — never pure black, and the **silhouette rescue rule**: units
whose exposure drops below threshold render a 1px value-1 outline instead of
vanishing (readability floor for the horde; implement as an exposure-gated pass
or LUT clamp for entity pixels).

## 3. Phased plan

| Phase | Build | Proves | Gate |
|---|---|---|---|
| **A — Look test** | Post-process material on `L_Spike1`: Bayer 4×4 + fixed 4-color LUT + one radial light on the hero. Luminance-based like the sketch — no index buffer yet. Virtual-res render + nearest upscale. | Does dithered 2-bit light *look right* at gameplay zoom with 10k brood moving? Cost vs. Spike 1 baseline. | Kill/keep on look + ≤ ~1 ms GPU |
| **B — Index pipeline** | `M_Swarm` outputs `(value index, palette ID)`; `T_Swarm_2bit` re-authored as index ramp; LUT texture; light = value-step shift. | Per-faction palettes + lamp-radius value shift + light-exempt reserved values. Resolves GDD #6 as data. | Faction readability survives darkness |
| **C — Occlusion & real lights** | Procgen-baked occluder mask; raymarched shadows; N gameplay-driven lights (brazier/Sanctuary/flicker); per-biome ambient value-step. | The Vesper Halls fantasy; the Quiet's extinguish mechanic has teeth. | 60fps at Spike-1 counts with 8+ lights |
| **D — Polish** | Below-exposure silhouette outlines; Daybreak LUT swap; titan-scale light interaction (W6 camera pullback). | Pillar 4 holds in the worst case: dark room, full horde, 4 players. | Playtest readability |

Phase A doubles as the **GDD §12 #5 art test vehicle** (flipbooks vs. flat 3D both
render through this pipeline unchanged — test them under it).

## 4. Technical notes for Phase A (UE 5.8)

- Post-process material, `Before Tonemapping` blendable on a PostProcessVolume in
  `L_Spike1` (unbound). Sample `PostProcessInput0`.
- Bayer 4×4 as a 4×4 nearest-filtered texture (or pure math from screen-pos mod 4),
  in **virtual-pixel** coordinates, not native.
- Virtual res: start with r.ScreenPercentage or a render-target path; crude is fine
  for the look test — even native-res with a 4×-scaled Bayer cell approximates it.
- Light uniforms via Material Parameter Collection, written from `SwarmRenderActor`
  or the hero pawn tick (position, radius, flicker).
- Measure with the existing `-SwarmBench` harness; add a lighting on/off toggle so
  the delta is one number in `docs/SPIKE1-RESULTS.md`.

## 5. Open decisions

| # | Question | Lean |
|---|---|---|
| L1 | Virtual resolution (and thus dither cell size on screen) | ~640×360 family; decide by eye in Phase A |
| L2 | Index-buffer encoding channels & precision path (survive tonemapper vs. replace it) | Wide-separation encode + quantize-on-read; revisit with a custom pass only if it bands |
| L3 | Light count / raymarch step budget | 8–16 lights, ≤32 steps at virtual res |
| L4 | Does ambient darkness affect the *floor tiles* only, or entities too? | Both, but entities get the silhouette-rescue floor |

## 6. Canon proposals (owner to apply — agents/docs don't edit canon)

1. **GDD §10 "2-bit rendering"**: add this doc as the lighting direction; note that
   the palette question (#6) migrates from architecture to LUT data.
2. **GDD §12**: add a row — "2-bit dynamic lighting (dither attenuation + index LUT)"
   → status: Phase A art test planned; reference `docs/RENDERING-LIGHTING.md`.
3. **CLASSES.md Lampbearer**: annotate that lamp-radius brightening is implemented
   as the LUT value-step shift (no bespoke system needed).

## 7. Divergences from the source sketch (for the record)

- Gray-average quantization → **index-space quantization** (§2.1) — keeps hue,
  factions, and reserved values.
- Screen-space dither → **virtual-pixel-grid dither** (§2.2).
- Engine 2D shadow maps (Option B) → not applicable in unlit UE; **procgen-baked
  occluder mask** replaces the "collision buffer" of Option A, no capture needed.
- The sketch's `if (brightness < threshold) brightness -= …` softening is replaced
  by threshold-rounded **value-step shifts** — same visual role, exact 2-bit output
  by construction.
