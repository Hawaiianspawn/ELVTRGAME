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
| **A — Look test** ✅ **BUILT 2026-07-23 (§4b.7); G1 still unjudged** | Post-process material on `L_Spike1`: Bayer 4×4 + fixed 4-color LUT + one radial light on the hero (**full spec: §4b**). Luminance-based like the sketch — no index buffer yet. Virtual-res render + nearest upscale. **Mandatory: flicker + world-anchored floor dither** (§4b.1) — without both, the test measures a vignette and returns a false negative. | Does dithered 2-bit light *look right* at gameplay zoom with 10k brood moving? Does it read as a **carried light** rather than a lens effect? Cost vs. Spike 1 baseline. | §4b.5 gates G1–G5. G1 (reads as carried light) is the only one that can kill it |
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

## 4a. Directional unit lighting — the flame overhead (owner, 2026-07-22)

**Owner direction:** the flame rides above the bearer's head and lights the **backs**
of the units around them. Under the new narrative canon
(`docs/narrative/FLAME-FOUNDATION.md`) the hero is the only light source in the world,
so this is not a lighting flourish — it is the game's primary read.

> **BUILT 2026-07-23 (debug-renderer approximation).** The intent below — units lit on
> the flame-facing side, dark on the side turned to the dark — is now live in
> `TickDebugRender`, and confirmed by eye (a stationary hero rings the retinue radially,
> so front/back shows without needing motion). It is *not yet* the SubUV frame-bucket
> mechanism specced below; that waits on the Niagara sprite path being fixed
> (`GATE1-FUN-PROTOTYPE.md` §3a). The stop-gap: each unit is drawn as **two rotated
> half-boxes** split along the direction to the flame — the near half bright, the far
> half `Swarm.UnitBackShade`× dimmer — with brightness also falling off by distance and
> floored at `Swarm.UnitLightFloor` so edge units stay readable silhouettes (this
> incidentally fixes the gate-G5 vanishing noted in §4b.8). Point-light-correct: the
> lit side is always the hemisphere toward the flame, so it is radial, not based on the
> unit's travel facing. Toggle `Swarm.UnitShading`; `0` restores the flat single box.
>
> **Cost / caveats.** Doubles debug draws (~2 boxes/unit, so ~1640 at a 700-brood
> wave) — a debug-renderer cost that evaporates when the real sprite path lands. Two
> known softnesses, both acceptable for now: (1) near the flame the screen-space pool
> lifts *both* halves toward Pale and the split washes out — fine, the hot core reads
> as overexposed anyway; (2) the demichrome pass quantises each half to a single
> palette value, so a unit shows *two* values, not a gradient — which is correct for
> 2-bit and on-style. The values below are the tuning surface; the two-value-per-unit
> look is a property of the palette, not a bug to smooth out.

### 4a.1 The brood exposure window — enemies fade in (owner, 2026-07-26)
The §4a shading above lit both teams through one curve: `Lerp(UnitLightFloor, 1, Atten)`.
Two owner-reported problems, both artefacts of the shared curve rather than of the light:

**Blown out at close.** With the top of the range at `1`, a brood standing in the pool draws
at full albedo, and the flame's screen-space lift then pushes it over `Threshold3` into Pale
— the *enemy* ends up at the brightest value in the frame, next to a hero whose whole premise
is that he is the light. Brood now have a **ceiling** (`Swarm.BroodLightCeil`, 0.7). This is
the world-side twin of the panel's `BroodCeil` (§4d finding 3) and enforces the same rule:
brood sit a step below retinue at *every* distance, so a soldier beside a brood always reads
as the lit one.

**They arrive already visible.** `Swarm.UnitLightFloor` (0.28) is a promise about *your* line
— gate G5 says your soldiers must stay a countable silhouette out at the leash. Applied to
the tide as well, it pinned every distant brood at one flat mid-value, so a brood popped into
being at the spawn ring as a fully-formed shape. Brood now have their own **floor**
(`Swarm.BroodLightFloor`, **0**): at the outer edge of the pool a brood is drawn black, the
demichrome pass quantises that to `Palette[0]` — the same value as the ground — and it is
simply *not there* until the approach lifts it. The tide **fades into existence** out of the
dark instead of crossing a visibility line. Retinue keep the shared floor untouched, so G5 is
unaffected for the team it was written for.

> **Deliberate asymmetry, not an oversight.** The silhouette-rescue rule (§2.5) now applies
> to retinue only. That is the point: what you can and cannot see of the enemy is the
> horror budget, and it is exactly what the flame is *for*. If a playtest says the tide is
> unreadable rather than dreadful, `Swarm.BroodLightFloor` is the single dial back —
> 0.10–0.15 restores a hint of shape at the edge without returning the flat grey crowd.

### Why the post pass alone cannot deliver it
The Phase A/C post pass is screen-space: it knows pixel position and depth, nothing
about **sprite facing**. It produces correct *distance* falloff — a pool of light
around the bearer — but every unit at the same radius is shaded identically. The
"lit from behind" read is a per-unit directional term the full-screen pass has no
data for. Distance and direction are two different mechanisms and both are needed.

### The mechanism: frame selection, not shading
Directional light in pixel art is authored, not computed. The bridge already builds
one float per unit and hands it to Niagara as a SubUV index
(`SwarmRenderActor.cpp`, the `SubImageScratch` loop — currently `Frame + Team`,
values 0–3 only). Extend that index to include a **light bucket**:

- Bucket = angle between the unit's facing and the direction to the bearer,
  quantized to **4 buckets** (lit-from-behind / front / left / right).
- One `atan2` per unit inside a loop that already runs every tick. **No shader work,
  no GPU cost, no new array in the bridge** — only a wider index and a wider sheet.
- Facing is already computed and currently thrown away: `SwarmAnim::FlipBit` is set
  from `Velocity.X < 0` in `SwarmProcessors.cpp` and **is not decoded by the SubUV
  bridge**. Widening the index picks up that dangling thread.

**Sheet cost is lopsided in our favour.** Units face outward at what they are
fighting and the flame is always behind them at the bearer's position, so the
*back-lit* bucket is on screen the overwhelming majority of the time. Author that
bucket to finish quality; rough in the other three.

**Narrative payoff (canon-relevant):** the player almost never sees their units'
faces — only rim-lit backs turned away, toward the dark. This is the deity/congregation
premise rendered directly, at zero extra cost. Treat it as intended, not incidental.

### Distance and direction compose
| Channel | Mechanism | Cost |
|---|---|---|
| **Distance** from the flame (the pool of light, the leash made visible) | Phase A post pass — Bayer-dithered value-step falloff | Per-pixel, flat, entity-count-independent |
| **Direction** (backs lit, faces dark) | Per-unit SubUV bucket, CPU-side | One `atan2` per unit |
| **Leash break** (unit outside the light) | Value-step floor + `bLeashBroken` / `LeashWarnBit`, already in the sim | Free — data already exists |

## 4b. The bearer's spotlight — spec (owner, 2026-07-22)

**Owner direction:** the main spotlight follows the character. This is the primary
light in the game and, under `docs/narrative/FLAME-FOUNDATION.md`, the *only* one —
everything visible is visible because the bearer is standing there.

**Scope of this section: spec only.** Nothing here is built. The base dither layer
already exists as a post-process on `L_Spike1` (the demichrome pass that
`Swarm.DebugPlainView 1` disables); this spec describes the light that composites
with it.

### 4b.1 The trap: a hero-locked camera turns a spotlight into a vignette

`ASpikeHeroPawn` owns its `UCameraComponent` directly at `CameraHeight = 1200` —
**no spring arm, no lag, no offset.** The hero is therefore pinned to the exact
centre of the screen at all times.

A radial light centred on the hero is consequently **stationary in screen space**.
A stationary, centred, unchanging radial gradient is not read by the eye as a light
source. It is read as a **lens vignette** — a property of the camera, not of the
world. The single biggest risk in this feature is spending the whole implementation
and shipping something that looks like a post-process filter someone left on.

Light reads as light when it **has a relationship with the world it falls on**. Five
mechanisms break the vignette read; the spec calls for **at least two**, and the
first two are the cheapest and most effective:

| # | Mechanism | Why it works | Cost |
|---|---|---|---|
| **1** | **World-anchored floor dither** (§4c) | The dither pattern scrolls under a stationary gradient as the bearer walks. Proves the world is moving *through* the light rather than the light being painted on the lens. | Free — it is an anchoring choice, not extra work |
| **2** | **Flicker** — time-noise on radius and intensity | A vignette is perfectly steady. A flame is not. Sells "carried fire" for almost nothing. | ~2 lines of shader |
| 3 | **Occlusion / cast shadows** (Phase C) | Light that is *interrupted* by walls is unambiguously cast. Strongest possible cue. | Expensive — deferred |
| 4 | **A second light in frame** | Any other light source (brazier, another bearer, a titan) gives the eye a comparison and both instantly read as sources. | Depends on content existing |
| 5 | **Camera lead / offset** so the hero is not dead-centre | Breaks the symmetry that makes it look like a lens effect. | Small, but changes game feel — decide deliberately |

**Mechanisms 1 and 2 are the Phase A minimum.** If the look test runs without them it
is not testing this feature, it is testing a vignette, and it will produce a false
negative.

### 4b.2 What it is, mechanically

- **One radial light**, positioned at the bearer, rendered as a full-screen post pass.
  Cost is per-pixel and flat — **it does not scale with entity count**, which is why
  this approach and the horde pillar do not fight (§1).
- **Radius = `SwarmLeash::Radius` (2000uu).** Deliberate, not a coincidence to be
  tuned away: the edge of the light is the edge of the leash. A unit that passes out
  of the light is a unit that breaks stance and comes home. If these two numbers ever
  drift apart, the rules and the picture are telling the player different stories.
  Wire the light radius **from the leash constant**, don't retype the number.
- **Falloff:** exponential / inverse-square-ish (`pow`, start at 2.0) so the dark
  feels heavy and the pool edge arrives with some suddenness rather than fading out
  politely. From the owner's original shader sketch (§2.5).
- **Attenuation → value-step, dithered.** Per §2.1, light does not multiply colour —
  it shifts a **palette index**, and the Bayer threshold decides rounding per pixel.
  That is what makes falloff dither into darkness instead of banding.
- **Never pure black at the outer edge.** `Palette[0]` is heavy midnight
  (`#211e20`), not black (§2.5). The world outside the light is *dark*, not absent.

### 4b.3 Driving data

Hero position → **Material Parameter Collection** → post-process material, written
once per frame from the hero pawn's tick (or `ASwarmRenderActor`, which already ticks
in `TG_PostUpdateWork` after Mass and already reads the subsystem).

`USwarmSubsystem::GetAttractor()` is **already the hero's published world position**
and is already updated every tick — the light should read that rather than
introducing a second source of truth for "where the bearer is."

MPC contents for Phase A:

| Param | Source | Notes |
|---|---|---|
| `FlamePosition` | `USwarmSubsystem::GetAttractor()` | World space; the pass projects it |
| `FlameRadius` | `SwarmLeash::Radius` | Wired from the constant, not duplicated |
| `FlameIntensity` | 1.0 baseline | The channel that upkeep/fuel would later drive |
| `FlameFlickerSeed` | per-run random | So two sessions don't flicker identically |

**Note for later, not for Phase A:** `FlameIntensity` and `FlameRadius` are the
natural hooks for the upkeep/fuel economy (GDD §7) and for the "unite the flames"
co-op fantasy. Exposing them as parameters now costs nothing and keeps that door open.

### 4b.4 Composition order with the base dither layer

This is the part that goes wrong silently. There is already a demichrome post-process
on `L_Spike1`. Order is load-bearing:

```
scene colour (flat unlit sprites, authored palette indices)
      ↓
  [ LIGHT ]   attenuation → value-step shift, per pixel
      ↓
  [ DITHER ]  Bayer threshold decides the rounding of that shift
      ↓
  [ LUT ]     index → Demichrome RGB
      ↓
  nearest upscale to native
```

**The dither must resolve the light's value shift, not be applied after it.** If the
existing demichrome pass runs *after* an already-quantised lit image, the light
produces hard banded rings and the dither has nothing left to dissolve — it will just
add texture on top of the bands. Phase A must therefore either fold the light into the
existing pass or explicitly order the light **before** it. Verify by eye: correct
looks like the light *scattering* into the dark; wrong looks like concentric rings.

### 4b.5 Test plan

Run on `L_Spike1` at Gate 1's wave counts, with `Swarm.DebugRender 1` (the Niagara
sprite path renders the whole army stacked on one point and is still unfixed —
`GATE1-FUN-PROTOTYPE.md` §3a — so the debug boxes are the honest read).

| Gate | Question | Pass |
|---|---|---|
| **G1** | Does it read as a **carried light** or as a lens vignette? | A player who was not told says "he's carrying a light." If anyone says "filter" or "vignette", it failed — go back to §4b.1 |
| **G2** | Does the pool edge read as the leash? | Player can predict which units are about to break stance, without the HUD counter |
| **G3** | Does falloff dither or band? | No concentric rings at any radius (§4b.4) |
| **G4** | Cost | ≤ ~1ms GPU delta vs. the Spike-1 baseline, measured with the existing `-SwarmBench` harness at 700 brood |
| **G5** | Readability floor | At 700 brood, units at the pool edge are still individually distinguishable (silhouette-rescue rule, §2.5) |

G1 is the only one that can kill the feature. G3–G4 are fixable; G1 is a design failure.

> **G5 does not currently hold for the SPRITE renderer — measured 2026-07-25.**
> `Swarm.UnitLightFloor` (0.28) is the silhouette-rescue mechanism, but it is applied by
> multiplying the debug renderer's box colour in `TickDebugRender`. `M_Swarm` is
> **Unlit + Masked and samples `T_Swarm_2bit` directly**, so on the sprite path there is
> no floor at all: whatever value the texture holds is what draws, everywhere in the pool.
>
> This bites exactly the subject the art direction asks for. `npc-silhouette-brief.md`
> (c) specifies an enemy whose "entire body is flat Demichrome Dark" — and Demichrome Dark
> is also the world's ground state, so such a sprite is **invisible**, not merely dim.
> Verified with the owner-supplied brood art (PixelLab `4a75b4ac…`, 94% Dark): against the
> Dark floor only its Pale teeth read; the silhouette vanished completely.
>
> Worked around in the sheet rather than the shader, by **light-shifting the brood one
> step up the `palette.json` ladder** (Dark→Steel) so the body is 92–93% Steel. That keeps
> it strictly on-palette, keeps it value-disjoint from the Bone-dominant retinue, costs no
> generations — and means **the shipped brood is not Dark-dominant**, which is a deliberate
> deviation from (c) and wants an owner ruling. Its hit frame is shifted twice
> (→91% Bone / 7% Pale) because a flat body cannot express pose: all nine frames of its
> recoil animation are visually identical, so the hit has to be a value event.
>
> The real fixes, if the (c) mechanism is to be honoured literally: give `M_Swarm` its own
> value floor (a max() against a minimum ramp step before output), or reserve the darkest
> value for the world and never author a unit body at Dark. Neither is done.

### 4b.7 Build results — **BUILT 2026-07-23**

Phase A is implemented and running on `L_Spike1`. Five things the build measured
that the spec got wrong or did not know.

**1. Light must LIFT the value, not scale it.** The spec implied attenuation
multiplies scene luminance. That is wrong here: this level is near-black by design,
so a multiplicative light has nothing to reveal. Measured — at
`Swarm.FlameIntensity 50`, `saturate(atten * 50)` is 1 almost everywhere, and the
floor *still* quantised to palette 0 with only the self-lit debug boxes visible.
The shader now does `lum = saturate(lum + atten * FlameIntensity)`, which is also
what the index-shift model of §2.1 does, arriving one phase early.

**2. The light radius is bigger than the screen — "the edge of the light is the
leash" cannot currently be seen.** `SwarmLeash::Radius` is 2000uu. The camera is
**orthographic, 2400uu wide** (logged by `SwarmDebug` at BeginPlay), so the visible
half-width is ~1200uu. The entire playable view therefore sits inside the light's
bright core, and the pool boundary is permanently off-camera. At `FlameIntensity 1.0`
the whole screen saturated to Demichrome Pale. Mitigated for now by dropping the
default intensity to **0.7**, which puts a visible ramp inside the view — but the
underlying conflict is real and is **open decision L10**. The design intent
(§4b.2, gate G2) is not satisfied: a player cannot see which units are about to
break leash, because the boundary is off-screen.

**3. Dither texel size had to roughly double.** At ~2.8uu per pixel, the specced
`WorldDitherScale = 5` put a Bayer texel under 2px and the pattern read as noise
rather than craft — the exact failure `aesthetic-direction.md` §2.4 warns about.
Default is now **12uu**, which holds the 2×2-pixel minimum.

**4. The spotlight is confirmed hero-driven, and it needed a real test to prove
it.** Because the camera is hard-locked to the pawn, a following light stays dead
centre and is visually indistinguishable from a static vignette (§4b.1) — moving
the pawn proves nothing. The test that works: park the MPC's *default*
`FlamePosition` far off-map (9000, 9000) and toggle `Swarm.Flame`. Writer off → the
floor is dark, no pool. Writer on → full pool centred on the hero. Keep this
procedure; it is the only cheap way to distinguish "tracking" from "vignette".

**5. Gate G1 is NOT yet answered.** What exists is a correct light with dithered
falloff. Of the two mandatory anti-vignette mechanisms, flicker is implemented
(`Swarm.FlameFlicker`, default 0.06) and world-anchored dither is implemented and
on by default (`Swarm.DitherWorldAnchor 1`) — but neither has been judged by eye in
motion, which is the whole point of G1. **A human has to drive it and say whether
it reads as a carried fire.**

#### 4b.9 Spring-follow — **owner calls 2026-07-23**

The flame is no longer pinned to the bearer. `TickFlame` pulls a smoothed position
toward the hero and the light reads *that*, so it trails and sloshes as the hero moves.

**First pass was `FMath::VInterpTo`** (the `USpringArmComponent` lag). It was replaced
the same day: the owner wanted it *more responsive* and wanted it to **overshoot on a
fast 180**, and VInterpTo is a critically-damped ease — it physically cannot overshoot
at any speed. It has no momentum.

**Now a real damped spring.** Semi-implicit Euler, `accel = k·(target−pos) − c·vel`,
so the light carries velocity: reverse direction quickly and its momentum sails it past
the hero before the spring reels it back. Two dials:

- `Swarm.FlameStiffness` (default 55) — responsiveness. Higher = snappier, catches up
  faster.
- `Swarm.FlameDamping` (default 0.6) — damping *ratio*, as a fraction of critical, so
  it is independent of stiffness (the coefficient `c = ratio · 2·√k` is derived).
  **1.0 = critically damped, never overshoots** (the old VInterpTo feel is still one
  CVar away); **below 1.0 overshoots** on a direction change, lower = bouncier and
  longer to settle.

`dt` is clamped to 1/30 s inside the integrator so a frame spike can't make the
explicit spring blow up. Snaps on the first tick and resets velocity so the pool
doesn't streak in from the world origin.

**Validated numerically** (headless step response, light released 500uu from a
stationary target):

| Damping ratio | Overshoot past target | Reaches target | Settles |
|---|---|---|---|
| 1.0 (critical) | 0% | ~0.53s | ~1.0s |
| **0.6 (default)** | **8%** | **~0.32s** | **~1.0s** |
| 0.4 | 24% | ~0.25s | ~1.7s |
| 0.6 @ stiffness 90 | 8% | ~0.25s | ~0.8s |

So the defaults deliver both asks: reaches the hero faster than the old ease, and
overshoots ~8% on a hard reversal. The integrator is confirmed stable at 60fps.

**Only the light springs.** Steering, targeting, and the leash all still read
`USwarmSubsystem::GetAttractor()` — the true hero position — so retinue behaviour is
unchanged and exact. Deliberate consequence: the lit pool and the leash "home" diverge
while moving; under the current values (900uu pool, 2000uu leash, decoupled per §4b.8)
that is invisible, but if the two are ever re-coupled (L10) the spring makes it a
*soft* relationship — fire trailing the runner, not clamped to them.

**Feel is unjudged.** The physics is validated, but the *feel* — like G1 — can only be
set with a human driving WASD; headless input can't reproduce continuous player
movement, and a stationary hero converges the spring onto itself (a static capture
shows a centred pool at any stiffness). Defaults 55 / 0.6 are a calibrated starting
point; tune both live, no rebuild needed.

#### 4b.8 The focusing core, and radius decoupled — **owner call 2026-07-23**

Two changes on top of the first build.

**The pure-white core.** The flame now has a hard white centre — the focusing point of
the light — drawn *outside* the locked 4-value palette. This is a deliberate owner
exception to `aesthetic-direction.md`'s "no fifth value" rule, and it is scoped to
the flame itself; it is not licence for a fifth value anywhere else. Its edge is cut
with the **same Bayer threshold** as the pool, so it dissolves into the light rather
than ending on a clean vector circle. Colour is `MPC_Flame.FlameCoreColor`
(white by default), size is `Swarm.FlameCoreRadius`.

**The core only reads if the pool around it is darker than Pale.** Demichrome Pale is
`#e9efec` — already ~94% white — so a pure-white core against a field of Pale is
nearly invisible. Measured: at `FlameIntensity 0.7` the body of the pool sat at
`Threshold3` (0.75) and the core vanished into it. At **0.55** the pool body holds at
Demichrome Bone and the core reads as a distinct focal point. That is why the default
moved; the two values are coupled and should be tuned together.

**Radius decoupled from the leash — this resolves L10 toward "decouple".**
`Swarm.FlameRadius` is now its own tunable (default **900uu**), no longer wired to
`SwarmLeash::Radius`. Consequence, stated plainly: **the edge of the light is no
longer the edge of the leash.** With a 900uu pool and a 2000uu leash, units can stand
in darkness while still perfectly obedient, and the leash boundary remains invisible.
The §4b.2 design intent and gate **G2 are now explicitly not satisfied** — by choice,
pending tuning. If the light/leash relationship is still wanted later, the options are
unchanged: raise the light radius and pull the camera back, or lower the leash.

**Core radius raised to 330uu** (from 220) at the owner's request, 2026-07-23.

**New consequence of the smaller pool — gate G5 is now at risk.** With the radius at
900uu, most of a wave's brood are *outside* the light and render as barely-visible
specks against the ground state. The silhouette-rescue rule (§2.5 — a unit below the
exposure threshold should still draw a 1px value-1 outline rather than vanish) is
**specced but not implemented**, and the tighter pool is what exposed it. Right now
you cannot see what is walking toward you. This is the next thing to fix if the pool
stays this small.

**Still open: the pool reads warm, not white.** The owner asked for a white spotlight.
The core is white, but the pool grades through Demichrome Bone (`#a0a08b`), which is
the palette's only warm mid — so the body of the light reads olive/tan. This is the
locked global palette doing exactly what it says, not a bug. Fixing it would need a
second palette exception (a white-biased ramp for lit areas) and is an owner call, not
one to make silently.

#### Tuning surface

| CVar | Default | Note |
|---|---|---|
| `Swarm.Flame` | 1 | 0 freezes the light — the tracking test above |
| `Swarm.FlameRadius` | 900 | Outer edge of the pool. **Decoupled from the leash** (§4b.8) — expect to tune |
| `Swarm.FlameCoreRadius` | 330 | Pure-white focusing core |
| `Swarm.FlameIntensity` | 0.55 | Coupled to the core: higher pushes the pool to Pale and the core stops reading (§4b.8) |
| `Swarm.FlameFalloff` | 2.0 | Higher = heavier dark, more sudden pool edge |
| `Swarm.FlameFlicker` | 0.06 | Anti-vignette mechanism 2 |
| `Swarm.FlameStiffness` | 55 | Spring responsiveness — higher catches up faster (§4b.9). <=0 snaps |
| `Swarm.FlameDamping` | 0.6 | Damping ratio: 1.0 = no overshoot, <1.0 overshoots on a 180 (§4b.9) |
| `Swarm.DitherWorldAnchor` | 1 | Anti-vignette mechanism 1. 0 = screen-locked. This is the L5 A/B |
| `Swarm.WorldDitherScale` | 12 | See finding 3 |
| `Swarm.DitherBandWidth` | 0.326 | Width of the Bayer dither band at each palette step. Wider = softer/grainier steps, narrower = harder banding, 0 = pure posterise |
| `Swarm.DitherThreshold1` | 0.40 | Luminance of the value 0→1 step. Keep 1 < 2 < 3 |
| `Swarm.DitherThreshold2` | 0.50 | Luminance of the value 1→2 step (mid split) |
| `Swarm.DitherThreshold3` | 0.75 | Luminance of the value 2→3 step. Raising it holds the pool at Bone so the white core reads (§4b.8) |
| `Swarm.UnitShading` | 1 | Per-unit flame shading, front-lit/back-dark (§4a build note). 0 = flat single box |
| `Swarm.UnitBackShade` | 0.32 | How dark the dark-facing half is (0 = black back, 1 = no split) |
| `Swarm.UnitLightFloor` | 0.28 | Min brightness at the pool edge so units don't vanish (fixes G5). **Retinue only** since 2026-07-26 (§4a.1) |
| `Swarm.BroodLightFloor` | 0 | Brood-only floor. 0 = brood are black at the pool edge and **fade into existence** on the approach (§4a.1) |
| `Swarm.BroodLightCeil` | 0.7 | Brightest a brood is ever drawn — the anti blow-out dial. Holds brood below retinue at every distance (§4a.1) |

Material-side (edit on `MPC_Flame`, no rebuild needed): `FlameCoreColor` — the core's
colour, pure white by default. The Demichrome thresholds and dither band width
(`Threshold1` 0.40 / `Threshold2` 0.50 / `Threshold3` 0.75, `DitherBandWidth` 0.326) used
to be bake-time scalars on `M_PP_Demichrome`; they now route through `MPC_Flame` and are
driven live by the `Swarm.DitherThreshold1/2/3` and `Swarm.DitherBandWidth` CVars above.
`PixelScale` (screen-space dither pixelation) is still a bare material scalar — not
exposed. The material reads all four via `CollectionParameter` nodes into the demichrome
Custom node.

#### 4b.10 The colour gate as a toggle — task-057 (owner 2026-07-28)

Owner goal: *"tighten up the UI and visuals in the scene. We can explore options without
the color gate for now."* Asked what "without the color gate" meant, the owner chose
**bypass quantization entirely**, not just more palette values. Two new CVars, same
per-tick push as everything else in this section:

- `Emberkeep.Quantize` `[0..1]`, default 1. 0 bypasses the posterise inside the Custom
  node (a `lerp` at the very end, not a disabled post-process volume) and shows the raw
  lit scene instead — the flame's additive lift and the world-anchored Bayer dither both
  survive, only the value-collapse goes. `lerp(litCol, outCol, 1) == outCol` exactly, so
  the default is byte-identical to before this task.
- `Emberkeep.PaletteSteps` `[2..8]`, default 4. How many values the posterise collapses
  onto. **At 4, `Threshold1/2/3` and `Palette0..3` are pushed completely unchanged** —
  this is the non-negotiable regression guard task-057 was built under. Away from 4,
  `SwarmRenderActor.cpp` derives `Steps-1` fresh evenly-spaced thresholds
  (`GetEvenThreshold`, never reusing the tuned N=4 numbers at a different count) and
  resamples the active `Emberkeep.Palette` preset's 4 authored colours across the new
  step count (`ResamplePaletteColor`, linear interpolation along the ramp), so no preset
  needs an 8-colour variant to support the full range.

`MPC_Flame` gained matching `Threshold4..7` and `Palette4..7` parameters (16 → 26
`CollectionParameter` nodes on `M_PP_Demichrome`), and the Custom node's fixed
three-comparison unroll became a loop over `Steps`. Same **judging dial, not canon**
status as `Emberkeep.Palette` (task-043) — Direction A stays locked, and UMG still does
not follow (`EmberkeepPalette.h` draws after post; that gap is task-058's).

#### Two engine gotchas worth remembering

- **Live Coding cannot add a `UPROPERTY`.** Adding `FlameCollection` to
  `ASwarmRenderActor` via a Live Coding compile reported *"Live coding succeeded"*,
  then crashed the editor on the next PIE with
  `Assertion failed: Ret->IsA(T::StaticClass())` during PIE world duplication —
  the placed actor's serialized layout no longer matched the patched class. Any
  change to a `UCLASS`'s reflected members needs a **full editor-closed rebuild**.
  Live Coding remains fine for function bodies and CVar defaults.
- **The MCP `set_properties` write to a Custom node's `code` silently no-ops.** It
  returns `{"returnValue":true}` and stores an empty string, which compiles to
  `'CustomExpression0': function must return a value` and falls back to the default
  material — i.e. the level looks *fine but unfiltered*, which is easy to misread as
  success. It is not length-related (2570 chars stored fine after failing).
  **Always read `code` back and compare lengths before recompiling, and never save
  the asset until the read-back matches.**

### 4b.6 Explicitly out of scope for this spec

Directional/back-lighting of units (§4a) is a **separate mechanism** and is not part
of the spotlight — the spotlight is distance only. Shadows and occlusion are Phase C.
Multiple lights are Phase C. Nothing in this section requires the Niagara sprite path
to be fixed first.

## 4c. The dither decision this forces — **anchoring**

Owner note 2026-07-22: *"the dither may be of other style depending on some decisions
we make."* The style question is downstream of one technical choice that has to be
made first — **what the dither pattern is locked to.** It is not one global answer;
different surfaces want different anchors, and picking wrong produces crawl.

| Anchor | Behaviour | Right for |
|---|---|---|
| **Screen-space** (as §2.2 currently specs) | Pattern fixed to the display grid. Anything that *moves across the screen* crawls through the pattern. | Static UI only |
| **World-space** | Pattern fixed to the ground. Stable as the bearer's light sweeps over it; reads as a property of the floor. | **Floor / terrain** |
| **Sprite-UV / object-space** | Pattern fixed to the sprite. Never crawls regardless of movement; effectively becomes authored art. | **Units** |

`docs/art/aesthetic-direction.md` §2.4 already warns that 1px halftone shimmers on
moving 48×48 quads and mandates **2×2 blocks minimum on movers**. That rule was
written about authored sprite dither; under a *moving light source* it now binds the
lighting pass too, and more tightly — because with the bearer as the only light,
the light moves constantly and every falloff edge in the game is in motion.

**Lean:** world-anchored dither on the floor, sprite-anchored on units, screen-space
reserved for UI. This means unit "shading" is largely authored into the sheet
(§4a) rather than computed — which is the cheaper path *and* the one that keeps
craft control. **Decide in Phase A by eye; this is L5 below.**

## 4d. The Unit Cam, without a second render — projection prototype (2026-07-23)

**Status: BUILT (needs a rebuild to load — new UCLASSes).** Source:
`ELVTR/Source/ELVTR/UI/UnitCamProjector.{h,cpp}`.

### The problem this sidesteps
The Unit Cam close-up was built as a real second camera (`AUnitPortraitStage` →
`SceneCaptureComponent2D`). The capture investigation (`docs/UNIT-CAM-HANDOFF.md`) hit
three walls, two of them architectural: SceneCaptures **cannot see `DrawDebug`
primitives** (so the default debug-box swarm is invisible to it), and the capture films
`SCS_FinalColorLDR` — the demichrome pass — so a close-up **collapses to one flat value**.
The 2-bit look that is right for the main view is wrong for a close-up capture.

### The mechanism (Doom-sprite forced perspective)
The world here is near-black empty space, a flat floor, a light pool, and billboards —
almost nothing a second viewpoint reveals through parallax. So the "camera" is a **pure
math construct, not a render**: define a virtual camera (position, orientation, FOV),
project each in-frame unit through it, scale each by `1/depth`, sort far→near, and blit
them as billboards into a Slate panel. Every input already exists on the CPU each tick —
`USwarmSubsystem::GetRenderPositions()` / `GetRenderAnimBits()` / `GetAttractor()` — the
same buffers the Niagara bridge reads.

Consequences, stated plainly:
- **Independent of the sprite/Niagara path.** It reads sim positions, not rendered
  primitives, so it works in the default `Swarm.DebugRender 1` (debug-box) mode — wall #1
  does not apply.
- **No demichrome capture to flatten** — shading is applied per-billboard here, reusing
  the same flame-distance falloff as `Swarm.UnitShading`, so the panel matches the world
  without the post pass collapsing it (wall #3 does not apply).
- **Cost scales with units-in-frame, not total swarm**, and there is no second scene
  render. This is the claim the performance-director should put a number on before it goes
  past prototype (capture on/off vs. projection, via `-SwarmBench`).

### Prototype scope / not-yet
**Soldiers (retinue) now draw a real sprite** — the `vanguard` grayscale render, runtime-
loaded from `RawArt/Renders/hero-rev1-grayscale/vanguard/south.png` via
`FImageUtils::ImportFileAsTexture2D` (prototype path; a proper Content import replaces it
later), bottom-anchored on the ground point at texture aspect, tinted only by flame
distance. Because the panel is UMG (drawn *after* post-processing), the sprite shows crisp
— it does **not** get demichrome-flattened the way the old capture did (handoff wall #3).
Brood still draw as flat dark-red quads. Still not-yet: **one facing only** (south) — the
per-direction facing bucket (§4a: bucket the unit's facing against the **virtual** camera's
forward, using the 8 `vanguard` directions) is the next layer. Painter's-order overlap is
not true occlusion; fine for a handful of units.

### Relationship to the capture path
It is now the **default Unit Cam**: `UEmberkeepHud::RebuildBand` hosts a
`UUnitCamProjector` in the band's right bookend (where the capture feed's "UNIT CAM"
used to sit), so it shows on Play with the auto-HUD (`Emberkeep.UI.AutoShow 1`) — no
console command. It is embedded in the HUD band, so it cannot be occluded by the HUD the
way a separately-viewport-added panel was. The old `AUnitPortraitStage` unit SceneCapture
is **retired** (`ShowCombatHud` no longer spawns it) — that per-frame full-scene render is
gone; the HUD's "cams mode" signal moved to the hero render target. The hero cam still uses
a capture (separate feature, left as-is).

### Panel shading — the close-up needs its own light model (2026-07-25)

Reusing `Swarm.UnitShading`'s distance falloff verbatim was not enough: brood read as a
**flat grey crowd**, indistinguishable near or far. Three separate causes, each fixed:

**1. No directional term.** The world renderer sells its light by splitting a unit into a
flame-lit half and a `Swarm.UnitBackShade` half. A billboard cannot be split, so the panel
had *only* distance — and distance alone is nearly constant across a close-up, where every
unit in frame is at a similar radius from the bearer. The fix resolves the same geometry
against the **virtual camera** instead: `dot(unit→flame, unit→lens)` in the ground plane is
how much of the lit hemisphere the lens can actually see. `+1` = the flame is behind the
lens and we see the lit face; `−1` = the flame is behind the unit and it is a backlit
silhouette. Because the camera sits behind the bearer looking out, brood advancing on him
resolve toward `+1` and **brighten as they arrive** — the "walking into the light" read
falls out of the geometry rather than being authored per-unit. Shares `Swarm.UnitBackShade`
with the world so the two cannot drift.

**2. The shared light floor was pinning brood at grey.** `Swarm.UnitLightFloor` (0.28)
exists so units never vanish at gameplay zoom (gate G5), but in a close-up it clamps every
distant brood to one flat mid-grey — they read as fog, not as something coming out of the
dark. Brood now get their own far lower floor, so they start near-black at the pool edge
and are *lifted by the approach*. Retinue keep the shared floor: they are yours and must
stay legible out at the leash.

**3. A Slate tint can only darken.** The instinct — overdrive the tint past 1 so the light
*lifts* the value the way §4b.7 finding 1 requires — **does not work here.** Slate packs the
tint to an 8-bit vertex colour (`FSlateElementBatcher::PackVertexColor` → `ToFColor`), which
clamps: the sprite as authored is a hard ceiling. Since the atlas draws both teams as
mid-grey figures, the lever that *does* work is the opposite one — hold the brood **ceiling
down** so they stay below the retinue in value at every distance, and a soldier beside a
brood always reads as the lit one. This is the world's brood rule (`SwarmRenderActor.cpp`:
brood sit low in the value range, the flame lifts them only as they close) applied here.

Finally the light is **banded into discrete tiers** (`LightSteps`). The panel is UMG, drawn
*after* the demichrome pass, so nothing downstream posterises it and a continuous multiplier
smears the 2-bit art through every intermediate grey. Note this steps the **light, not the
pixels** — snapping the sprite itself to palette entries is precisely what collapsed the old
SceneCapture close-up to one flat value (`docs/UNIT-CAM-HANDOFF.md` wall #3), so each body
keeps its internal values and only its lighting tier is quantised.

| CVar | Default | Note |
|---|---|---|
| `Emberkeep.UnitCamProj.DirShade` | 1 | Shade by which side the lens sees. 0 = distance only (the old flat look) |
| `Emberkeep.UnitCamProj.BroodFloor` | 0.05 | Brood-only light floor, replacing `Swarm.UnitLightFloor` in this panel. Lower = brood emerge from deeper dark |
| `Emberkeep.UnitCamProj.BroodCeil` | 0.7 | Brightest a brood is ever drawn. Clamped to ≥ `BroodFloor`. 1 = brood may reach full sprite brightness |
| `Emberkeep.UnitCamProj.LightSteps` | 5 | Discrete lighting tiers. 0/1 = continuous (smooth, off-style); 4–6 reads as 2-bit |

Reused from the world so the panel and the main view stay locked together:
`Swarm.FlameRadius`, `Swarm.FlameFalloff`, `Swarm.UnitBackShade`, `Swarm.UnitLightFloor`.

### Camera manager (seed) + near-plane fade
`FUnitCamDirector` resolves what the camera centres on each frame — the seed of the
flexible manager. `Emberkeep.UnitCamProj.Focus`: `0` = the hero (overview), `1` = **follow
a soldier** (default). Follow uses *nearest-unit continuity* — each frame it locks the
nearest retinue unit to last frame's focus, which stays the same unit as it moves, so the
camera rides along with no per-frame entity handle and survives the render buffers being
rebuilt. Smoothed by `FollowSpeed` (VInterpTo). Camera focus and the flame/shading origin
are split: the cam follows a unit, but lighting still radiates from the bearer.

Units entering near the fake camera's near plane **fade in** rather than pop
(`NearFade`, uu band above the near plane). The open design fork is the *selection model*:
how the followed unit is chosen (auto-nearest today, vs click-select / cycle / auto-pick
most-wounded/threatened) — that's the next layer on top of the director.

### Try it
Just press Play — the bottom-right "UNIT CAM" bookend follows a soldier. Dials (live, no
rebuild): `Emberkeep.UnitCamProj.Focus / FollowSpeed / SoldierScale / NearFade / Fov / Dist
/ Height / Yaw / Range / Scale`, plus the shading set `DirShade / BroodFloor / BroodCeil /
LightSteps`. `SoldierScale` is the soldier framing-size dial; `Focus 0`
drops back to the hero overview. The standalone `Emberkeep.UI.UnitCamProj` toggle still
exists for isolated testing (it lands top-left, outside the HUD).

## 5. Open decisions

| # | Question | Lean |
|---|---|---|
| L1 | Virtual resolution (and thus dither cell size on screen) | ~640×360 family; decide by eye in Phase A |
| L2 | Index-buffer encoding channels & precision path (survive tonemapper vs. replace it) | Wide-separation encode + quantize-on-read; revisit with a custom pass only if it bands |
| L3 | Light count / raymarch step budget | 8–16 lights, ≤32 steps at virtual res |
| L4 | Does ambient darkness affect the *floor tiles* only, or entities too? | Both, but entities get the silhouette-rescue floor |
| L5 | **Dither anchoring** (§4c) — screen / world / sprite-UV | World on floor, sprite-UV on units, screen for UI. Decide by eye in Phase A. **Raised in priority by §4b.1** — world-anchored floor dither is one of the two mechanisms keeping the spotlight from reading as a vignette |
| L8 | **Camera offset** (§4b.1 mechanism 5) — hero dead-centre vs. lead/offset | Dead-centre for now; revisit only if G1 fails on the other mechanisms. Changes game feel, so decide deliberately rather than as a lighting fix |
| L9 | Does the light fold **into** the existing demichrome pass or run as a separate pass before it? (§4b.4) | **RESOLVED 2026-07-23: folded in.** The light lifts luminance inside `M_PP_Demichrome`'s existing Custom node, before the Bayer threshold. One pass, no ordering hazard |
| L10 | **Light radius (2000uu leash) exceeds the visible half-width (~1200uu)** — the pool edge is permanently off-camera, so gate G2 cannot pass (§4b.7 finding 2). Pull the camera back, or decouple light radius from the leash? | **RESOLVED 2026-07-23 — decoupled.** `Swarm.FlameRadius` is its own tunable, default 900uu. Accepted consequence: the edge of the light is no longer the edge of the leash and **G2 is not satisfied** (§4b.8) |
| L11 | **The pool reads warm, not white** — the light grades through Demichrome Bone `#a0a08b`, the palette's only warm mid, so only the core is truly white (§4b.8) | Open — owner call. Fixing it needs a second palette exception (a white-biased ramp for lit areas) on top of the core exception already granted |
| L6 | **Directional bucket count** (§4a) — 4 buckets vs. 8 | 4; raise to 8 only if the rotation reads steppy at gameplay zoom |
| L7 | Does the flame's overhead position change the **sprite pivot / Z-offset** of the light, or is it purely an authoring convention in the sheet? | Authoring convention first — cheapest test |

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
