# Aesthetic Direction — Artboard analysis

**Source:** `Artboard/` (92 images across 7 category folders, all viewed) · **Canon checked against:** `GDD.md` §1/§2/§10/§12, `WORLD.md`, existing specs `docs/art/warden-captain-bree.md`, `docs/art/brees-stairwell.md` · **Pipeline:** 48×48 cells, SubUV sheets, Nearest, Unlit Masked (`ELVTR/SETUP-EDITOR.md`)

This is an analysis + direction document, not a sprite spec. It exists to narrow the board
into one buildable style before more per-asset specs are written.

> **RESET 2026-07-12: Direction B ("Lamplit Ledger") is SUPERSEDED. Direction A
> ("The Sampler Kingdom") is now LOCKED, hex-locked to 2-bit Demichrome.** Owner
> directive, verbatim: *"I want to use the 2bit Demichrome as the entire color
> palette, no other variations unless I explicitly overwrite it. I want to start
> again on NPCs and iteration."* This is a full reversal of everything Direction B
> built this session and before it — the per-faction warm/cold split, the Gatecamp
> Family, the Still Legion family, the Unwitnessed's exempt Oil 6 register — all of
> it collapses into **one single global 4-value palette for the entire game**, no
> exceptions until the owner names one. §3 already scoped this exact outcome as
> "the fallback if #6 resolves to a strict global palette" — that fallback has now
> triggered. Direction B stays below as history (same pattern as the chibi/palette
> amendments); do not delete it, do not cite its hexes or its per-faction structure
> in any new spec. **GDD dependency #6 is now RESOLVED: strict global palette**,
> reversing the prior per-faction lean everywhere it appears in this doc, in
> `hero-palettes.md`, and in any future spec. The locked global palette:
>
> | Hex | Name | Role |
> |---|---|---|
> | `#211e20` | Demichrome Dark | darkest — outline, void, recess, ground state |
> | `#555568` | Demichrome Steel | dark-mid — the game's only "cold/armor/shadow" value |
> | `#a0a08b` | Demichrome Bone | pale-mid — the game's only "warm/skin/highlight" value |
> | `#e9efec` | Demichrome Pale | brightest — the game's only bright: lamps, eyes, marks, UI |
>
> These four hexes (formerly the "Still Legion family" in `hero-palettes.md`) are
> now used by **every sprite in the game** — heroes, retinue, Still Legion, the
> Quiet, the Unwitnessed, UI, portraits, vignettes. No sprite may introduce a fifth
> value or a palette-swapped variant of these four without an explicit owner
> exception. §3 Direction A is rewritten below with these hexes; §4's decision log
> and §3 Direction B are kept verbatim as history and are no longer binding on new
> work. `hero-palettes.md` is separately reset — see its own top banner.
>
> **What this costs, said loudly:** there is no more warm/cold friend-foe channel,
> no faction-reserved third slot, and no separate richer register for the
> Unwitnessed's horror. Class, faction, and threat identity now ride on **shape and
> value-*pattern* alone** — this makes the shape-only precedent from
> `hero-palettes.md` §3 (previously a fallback insurance policy for the bright
> pixel only) the single load-bearing readability mechanism for the entire game,
> not just for telling classes apart. The chibi ruling (2026-07-11) is unaffected —
> it was never faction-specific and stands as-is under strict global palette.
>
> **Status 2026-07-10 (superseded 2026-07-12, kept for history): Direction B
> ("Lamplit Ledger") was LOCKED.** All six §4 open
> questions are resolved — §4 is now the decision log. New specs bind to the §3
> Direction B bundle as refined there.
>
> **Amendment 2026-07-11: decision 6 (Charm dial) REVISED — binding.** The owner
> overturned the "no chibi split" ruling. Chibi is now the combat/gameplay-scale
> register (heroes, retinue, and standard enemies including the Still Legion); the
> higher-resolution non-chibi register is reserved for portrait/avatar icons only;
> scale/complexity contrast between chibi friendlies and non-chibi Unwitnessed
> titans is an intentional horror tool. See revised §4 decision 6. Direction B
> itself, and decisions 1–5, are unaffected.
>
> **Amendment 2026-07-11 (later same day): hero palette system REVISED — binding,
> "accept shape-only."** The four-bright Gatecamp system (Roll-Gold / Waking
> Ember / Waylight / Watch-Lamp, on the `#1a1c2c`/`#4e5a66`/`#b08b5e` trio) is
> **retired**. All four hero classes now share one literal bright hex,
> **Gatecamp Bright `#f0c260`**, differentiated by carrier shape alone (never
> hue). The Gatecamp trio itself is re-hexed to `#211210` Vault Dark / `#5e2d20`
> Patched Steel (**no longer cold** — this breaks the old mirror-fight
> mechanism, re-derived on anchor-parity + chibi-proportion-parity instead) /
> `#c76b2a` Kitchen Tin, opposite a disjoint four-value Still Legion family
> (`2-bit Demichrome`-sourced: `#211e20` / `#555568` / `#a0a08b` / `#e9efec`).
> The Unwitnessed are exempt from this combat budget entirely and use `Oil 6`
> for vignette/set-piece art. Any hex or bright-name cited below this line that
> predates 2026-07-11 is superseded — see `hero-palettes.md` §0 for the full
> decision log and rationale. Direction B's structural argument (shared dark
> anchor, per-faction third slot, scarce reserved brights) is unaffected; only
> the specific hex values and the bright-count are revised.

---

## 1. What the board actually says

### 1a. Landscape shots (17)

**The dominant thread is monochrome darkness with one honest light.** The folder is
mostly duotone or full black-and-white, and almost every image stages a single light
source (or light-colored subject) against a dark world:

- **1-bit / duotone dither landscapes:** `5d10f3c6…jpg` (the "Hammerbeam" 1-bit
  landscape card set — pure black/white, dither-only tone), `hazy-night-236645.png`
  (ink + cream, dither clouds), `5d7e46c9…jpg` (dark-teal/pale-green castle,
  Obra-Dinn-style dither), `skull-ring-198563.png` (violet/mint duotone grass hiding a
  skull-faced thing — *terrain that turns out to be a monster*, straight out of the
  Unwitnessed brief).
- **B/W ink interiors at oppressive scale:** `4551f4b0…jpg` and `8e61d2379…jpg`
  (heavy-black dungeon corridors, skulls in masonry), `8ab66eca…jpg` (gothic
  mega-castle, tiny figures on the stairs), and the standout `5db6f8af…jpg` — a hall
  of colossal bells with two tiny figures against the *only* light in the frame. That
  one image is practically the Vesper Halls + Silent Bell (WORLD.md S8) mood target.
- **Light as the subject:** `overflowing-558961.png` (glowing brazier-bowl raining
  sparks), `final-moments-806552.png` (a lamp-creature whose candle is the scene's
  only warmth), `winter-power-pole-725394.png` (cold dark world, one red field —
  temperature as meaning).
- **Color outliers:** `convergence-105282.png` (full-color psychedelia),
  `bossroom_corruptcastellum_540x.png` (high-color magenta demon panorama),
  `praise-the-moon-468178.png` (warm tarot-card palette),
  `metroidvania-biomes-603240.png` (multi-hue). Note that even the metroidvania sheet
  obeys the structural rule: *dark ground state, terrain self-lit in warm amber, cool
  accents* — its palette is rich but its light logic matches the monochrome majority.

**Recurring grammar:** darkness is the default; light is scarce, warm, and means
something; dither carries all atmosphere; scale is expressed by tiny figures against
vast silhouettes.

### 1b. Monsters Aesthetic (36)

Three distinct monster languages coexist on the board, and they map almost 1:1 onto
the three factions:

1. **The void with eyes (→ the Quiet).** Monsters drawn as *negative space* — a black
   mass whose only features are pale eyes/teeth: `kaijune-day-part-3-365094.png` and
   `kaijune-day-part-1-711379.png` (black kaiju silhouettes against dithered pastel
   clouds), `monstro-do-amago…gif` (a girl holding a lit match under a giant dark face
   — the single most on-theme image on the whole board: Lampbearer vs the dark),
   `chutulu…176655.png` (muted mass, two glowing white eyes),
   `c38bd07455…jpg` (1-bit white-on-black monster *catalog* with names — "Wandering
   Soul Mass", "Leech Lamp", "Tattered Wraith" — several entries read like the Quiet's
   roster already), `arrival-164511.png`, `nightmare-579116.png` (dark body, ember
   accents). This language is natively 2-bit: darkest value = body, bright value =
   eyes only.
2. **Human-derived tragic horror in monochrome (→ the Unwitnessed / titan dread).**
   Charcoal and ink, faces where faces shouldn't be, enormous scale:
   `fb1c84eaf…jpg` (charcoal face-bug titans looming over tiny travelers — the
   Unwitnessed titan+brood mood in one frame), `e07de0d39…jpg` (pencil banquet hall,
   many-eyed thing emerging while nobody looks), `b646b736…jpg` (Berserk corridor
   panel: torchlight party vs tunnel-filling mass), `1853566f…jpg` (ink cage crammed
   with a horde — the prisoner-column/rescue-site energy), `f82aedb9…jpg` (fleshy
   bust studies), `5f1fd49a…jpg` (mask grid), `79f44f3f…jpg`, `3ed7a4cb…jpg`
   (spiked knight), `8f28968…jpg` (halftone 1-bit face), `16e48ed4…jpg`
   (cross-stitch-textured skeleton horror *with an RPG dialogue box* — horror
   delivered through UI), `MonsterPortraits.jpg` (1-bit portrait grid, black/white
   plus a reserved red).
3. **Charm-grotesque (→ friendly-side / decision-event flavor).**
   `1d9f9329…jpg` ("Something Vast and Dragon-Like…" — a *smiling* vast thing, old
   newspaper ink), `panaceia-775349.png` (rust/green cartoon grotesque),
   `friend-494949.png` (grayscale girl, eyes in the dark doorway),
   `be-not-afraid-834008.png` (small many-winged angel). A minority voice, but it is
   the one that keeps the board compatible with the good-guys tone.

Also present: **hollow armor as identity** — `59520576…jpg` (six knight-helm
portraits, each an affliction: "Infestation, Petrification, Despair…") — helmets whose
*contents* tell the story. That is the Still Legion: the uniform persists, the person
is gone. `caa0a7964…jpg` and `4d8a73ec…jpg` (muted 4–5 value pixel seraph / veiled
statue — the latter literally includes its 5-value ramp as a swatch) show how the
horror survives quantization: value ramp + silhouette, no line detail.

### 1c. Portrait Styles (18)

Two incompatible registers sit side by side:

- **Limited-palette / quantized portraits:** `the-sorcerer-457757.png` (GB-green
  4-value bust), `petal-knight-gb-263462.png` (2-value GB knight),
  `ce3ff8760…jpg` ("Dessertry" — a complete GB 4-shade game mockup: portraits, UI,
  monsters, all in one ramp), `5136ed5c…jpg` (1-bit dither cowgirl),
  `c72ab868…jpg` (posterized helmet studies — ~4 values per hue on black; the
  strongest bridge between photo-real armor and 2-bit), `02210b4f…jpg` (muted dark
  portrait roundels, Darkest-Dungeon-adjacent), `papercut-4-680445.png` (tiny 3-value
  faces), `e879a99e…jpg` (SNES-horror dialogue screen — haloed saint, "Floor: 1 /
  Torch: 42%" HUD; the closest thing on the board to ELVTR's actual screen anatomy).
- **High-fidelity painterly/comic portraits:** `2919ac96…jpg` + `892dff86…jpg`
  (Matsuno/FFT portrait sheets — small painterly busts over tiny sprites),
  `a32c81d856…jpg` (painterly pixel swordswoman shown at two scales),
  `5571487575…jpg` ("Ned the Knight" — full-color comic helmet grid, humor carried
  entirely by helmet shape), `memento-mori-691667.png` (colorful woodcut skeleton
  knight in flowers — death + warmth in one frame).
- **Ink figures:** `d6bbefd8…jpg`, `81c3175699…jpg` (hooded swordsmen, heavy
  crosshatch, menace with charm).

**Threads:** helmets/headgear as the entire character read (three separate helmet-grid
references — this is the board voting for the Legion officer-crest silhouette rule in
spec 001); portraits live at a *higher fidelity register* than gameplay sprites; and
dialogue-box framing appears repeatedly (`e879a99e…jpg`, `16e48ed4…jpg`) — portraits
exist to serve decision events.

### 1d. Grounding in the world (11)

This folder answers "why does this world look like pixel art?" and it has a real
answer: **needlework.**

- `85958757…jpg`, `289461eecda…jpg`, `TapestryINWorld.jpg` — monochrome cross-stitch
  samplers: dragons, knights, moons, whole kingdoms rendered in counted stitches.
  Cross-stitch *is* pixel art with a diegetic excuse: one stitch = one pixel, two
  values, dither = weave. A buried kingdom's surviving tapestries would look exactly
  like ELVTR's sprites.
- Taxonomy sheets: `5a79ffa94…jpg` (shield-shape chart), `7793ecb7…jpg` (armor-type
  chart), `dcdc3221…jpg` (pauldron variations sketch — "so many variations… and yet
  you chose this"). The user is collecting *silhouette taxonomies* — exactly the
  discipline the cross-class silhouette rules need.
- `05aaaecf…jpg` ("The Old Master" blackletter specimen + illuminated borders) — the
  typography/frame register for titles and event cards.
- `87858ae5…jpg`, `2e56e2c7…jpg` (1-bit and GB-green dithered photo-collage zines),
  `dc385f03…gif` ("ENTER THE DUNGEON? Hell yeah!" — Playdate-style 1-bit halftone
  scene + terminal text), `5da72291…jpg` (MiniSlate 1-bit title screen) — the
  UI/menu/meta register: heavy halftone, white terminal type on black.

### 1e. Gameplay Avatars (4) — the unanimous folder

All four references are 1-bit, chunky, dark-ground micro-sprites:
`071bdecaf…jpg` (1-bit chibi Star Wars cast — identity in ≤16px via 2–3 px props),
`59df83c0…jpg` (Mother/EarthBound-style 1-bit overworld sprites — dozens of distinct
characters, zero mid-tones), `2e0b9cb1…jpg` (complete 1-bit top-down game mockups
with portrait dialogue boxes), `11f9f1fd…jpg` (2-value + dither top-down party scene
— four little adventurers, readable ground dither, dark border). There is **no
dissent in this folder**: at gameplay scale the user's taste is white-on-dark, hard
silhouettes, personality from headgear/prop pixels, dither only on terrain.

### 1f. Items (3) — also unanimous

Three 1-bit icon sheets (`0372ab07…jpg` "100+ black and white icons",
`378ef8434…jpg`, `30b770be…jpg` — food icons): flat white-on-black glyphs, ~16px,
outline-first, zero color coding. Items read as *glyphs*, not miniatures.

### 1g. Magic (3)

- `e06a4ab91…jpg` — an ember-glowing rune alphabet scratched on black cloth: **warm
  glyph on dark ground**.
- `a2fe5039…jpg` — cold geometric sacred-geometry sigils on near-black.
- `the-scrying-094665.gif` — a dark figure over a glowing blue orb; glow is a halo
  around a bright core, not a particle system.

**Thread:** magic = *mark + glow*, drawn not simulated. This is exactly the reserved-
value rune/quarry-mark grammar from CLASSES.md, and it confirms the "1 bright pixel +
1px halo dither" glow language already used in specs 001/002. The board offers two
magic temperatures (ember vs cold-geometry) — usable as Relickeeper runes (warm,
awakening) vs Crown/Quiet sigils (cold, administrative).

---

## 2. Tensions and contradictions — what must be cut or fenced

1. **Bit-depth schism.** Gameplay Avatars + Items + half of Landscape are 1-bit;
   Monsters/Portraits include high-color pieces (`bossroom_corruptcastellum`,
   `convergence`, `frenzy-flame`, `everopen`, `praise-the-moon`, `memento-mori`,
   `5571487575…` Ned, FFT sheets). These cannot share one screen. Resolution: treat
   the board as **registers, not one style** — gameplay register (strict 2-bit),
   portrait/vignette register (own 4-value palette, higher resolution), UI register
   (1-bit + halftone). The full-color pieces contribute *mood and shape language
   only*; their palettes are cut.
2. **Horror ceiling vs the good-guys GDD tone.** A large slice of the monster folder
   is nihilistic-oppressive (`16e48ed4…` "you made the wrong choices",
   `59520576…` corpse-kingdom afflictions, `3ed7a4cb…` spiked brute,
   `MonsterPortraits.jpg` gore-red accents). WORLD.md §2 is explicit: the kingdom's
   enemies are *tragic, not edgy*, and full horror is reserved for the Unwitnessed
   alone. The board pulls darker than canon. Fence it: void-with-eyes → the Quiet
   (mournable), hollow-helm → Still Legion (tragic), charcoal face-titans → the
   Unwitnessed (the one sanctioned horror). The pure-cruelty references (spike brute,
   "wrong choices" taunt) should be **cut** — they have no faction to live in.
3. **Crosshatch ink cannot survive 48×48.** The ink illustrations (`8ab66eca…`,
   `4551f4b0…`, `1853566f…`, `d6bbefd8…`, `c680cee1…` Harry-Clarke frame) depend
   on line density that aliases into noise at sprite scale under Nearest filtering.
   They are only buildable as **vignette/event-card art** (large, static, quantized to
   a 4-value ramp with coarse dither standing in for hatching) — never as sprites.
4. **Fine halftone dither breaks at gameplay zoom.** The photo-dither zine pieces
   (`87858ae5…`, `8f28968…`, `2e56e2c7…`, `dc385f03…gif`) use 1px checker/halftone
   that will shimmer on moving 48×48 quads and die at any non-integer zoom. Rule:
   **dither on moving sprites = 2×2 blocks minimum; 1px halftone is reserved for
   static UI/menus** where it can be pixel-locked.
5. **Portrait register is unresolved on the board itself.** FFT-painterly vs
   4-value-posterized (`c72ab868…`) vs 1-bit dither (`5136ed5c…`) are three different
   games. One must be chosen (see §4 Q1); the analysis below recommends the
   posterized 4-value register because it shares the sprite ramp discipline.
6. **Two light temperatures compete.** Warm ember light dominates (`final-moments`,
   `overflowing`, `metroidvania`, `e06a4ab91…` runes) and matches the existing
   Bree/stairwell "warm bright = friendly/safe" channel. But several refs use cold
   white/blue light (`5db6f8af…` bells, moon pieces, `the-scrying` orb). Both can
   coexist **only** if temperature is load-bearing: warm bright = honest light
   (players, safety, restoration), cold bright = the Crown's order and the deep
   places. Never mix roles.

---

## 3. Candidate directions

### Direction A — "The Sampler Kingdom" (strict near-1-bit) — **LOCKED 2026-07-12, hex-locked to 2-bit Demichrome**

> **This is now the game's art direction.** Re-hexed and re-confirmed against the
> owner's 2026-07-12 reset (top banner). The bundle description below is the
> original scoping, kept intact where it still holds, with the concrete hexes and
> post-chibi adjustments folded in.

**Bundle:** one global ramp — **`#211e20` Demichrome Dark / `#555568` Demichrome
Steel / `#a0a08b` Demichrome Bone / `#e9efec` Demichrome Pale**, no other hexes,
no palette swaps — everything — world, units, monsters, UI, portraits — drawn in
the cross-stitch/1-bit language of `85958757…jpg`, `5d10f3c6…jpg`,
`59df83c0…jpg`, `c38bd074…jpg`. Flat unlit; light = one-value-brighter palette-shift
pockets only (Demichrome Dark→Steel→Bone→Pale ladder, same four values, no new
hex). Outlines: **none against dark floors** — silhouette against the dark ground
state does the readability work; a Demichrome Dark outline is only used where
sprite meets sprite (horde separation at close ranks), never as a decorative line.
Dither: **2×2 blocks minimum on anything that moves** (per pipeline tension §2.4);
1px halftone reserved for static UI only. Monsters: negative-space + bright eyes
— the Quiet's void-with-eyes language (§1b) is now the *default* monster grammar,
not one of three registers, since there is no separate Unwitnessed palette left to
carry a richer horror register. Portraits: 1-bit dither (`5136ed5c…jpg`), same four
values, no separate portrait ramp. **Anchors:** Gameplay Avatars folder (all),
Items folder (all), `hazy-night`, `monstro-do-amago`.
**Chibi ruling unaffected:** the 2026-07-11 chibi-combat decision (§4 decision 6,
history) was never faction-specific — chibi heroes/retinue/Legion, non-chibi
Unwitnessed titans — and stands as-is under strict global palette; only the *hue*
argument for the contrast is gone, the *scale/complexity* contrast argument was
always independent of color and still fully carries the horror-scale device.
**Class/faction/threat identity now rides on shape and value-*pattern* alone** —
no warm/cold channel, no faction-reserved slot, no exempt richer register for the
Unwitnessed. This raises the stakes on the cross-class silhouette rules
(CLASSES.md) and on `hero-palettes.md` §3's shape-carrier registry (rectangle /
dot-cluster / contour / point+halo) far beyond their original scope: that
mechanism no longer differentiates just the bright pixel, it is the *only*
differentiation mechanism left for anything, at every value.
**Trade-off:** maximum horde readability and the strongest, most ownable identity on
the board — but faction/biome differentiation must ride entirely on silhouette and
value-*role* pattern (which of the four values a shape claims, and how), color
gives nothing beyond that, and one ramp for the whole game risks monotony over a
full playthrough. This is no longer a fallback: **GDD #6 is resolved to strict
global palette**, so this direction is simply correct rather than contingent.

### Direction B — "Lamplit Ledger" (shared dark anchor + per-faction/per-biome ramps + reserved warm bright) — **LOCKED 2026-07-10** (see §4 decision log)

**Bundle:** what specs 001/002 already prototype, now confirmed by the board and
refined by the §4 decisions.
- **Palette philosophy:** every palette is 4 values; all share the same darkest
  anchor (Vault Dark family) so the world's ground state is uniform darkness; the
  third slot is the faction/biome channel (warm for friendlies, cold for Legion,
  sickly for Unwitnessed broods, near-absent for the Quiet); the bright slot is
  scarce and *earned* — lamps, runes, eyes, safety. Anchors:
  `metroidvania-biomes-603240.png` (structure: dark world, warm self-lit pockets,
  cool accents), `winter-power-pole` (temperature as meaning), `4d8a73ec…jpg` and
  `caa0a7964…jpg` (how much dread a muted 4-value ramp can carry),
  `ce3ff8760…jpg` (proof a whole game fits in one 4-shade ramp per screen).
- **Light model:** flat unlit; "light" = one-value-brighter palette shift in a
  radius (CLASSES.md §4 / honest-light rule from spec 001) + single bright pixels
  with 1px halo dither. Warm bright = honest; cold bright = Crown/deep. Anchors:
  `final-moments`, `overflowing`, `monstro-do-amago…gif`, `e06a4ab91…` runes.
- **Outline/dither rules:** sprites are silhouette-first with darkest-value outlines
  only where sprite meets sprite (horde separation); no outlines against dark floors.
  Dither: 2×2 minimum on anything that moves; 1px halftone reserved for static UI
  (`dc385f03…gif` register). Terrain detail = stitch-like patterning
  (`TapestryINWorld.jpg`) so the world itself reads as the kingdom's needlework.
- **Monster treatment:** per-faction languages from §1b — Quiet = void + bright
  eyes (`kaijune-3`, `c38bd074…`); Legion = intact cold silhouettes, crest/boss
  officer signature, hollow helms (`59520576…`, `c72ab868…`), now **chibi** per
  revised decision 6 so the mirror-fight comparison to chibi heroes still holds;
  Unwitnessed titans = multi-tile charcoal-dither masses whose silhouettes read as
  terrain until they move (`fb1c84ea…`, `skull-ring`), shedding 1-bit-style brood,
  and **stay deliberately non-chibi** — the scale/complexity gap against chibi
  friendlies is the horror device (revised decision 6). **Horror registers
  (§4 decision 3):** gameplay sprites hold the face-titan/no-gore baseline;
  level-3 body-horror spikes are Unwitnessed-only and live in large static
  registers (vignettes, breach reveals, multi-tile set-pieces). Faces stay
  deliberately ambiguous — flirt with victim-reads, never confirm; expressions
  calm/vacant, never agony. Titans never offer interaction prompts (mechanical
  fence, canon proposal 6).
- **Portrait treatment (§4 decision 1):** 4-value posterized busts using the
  character's **exact sprite palette** — no separate portrait ramp — inside
  stitched-medallion circular frames (`02210b4f…` roundels) and the `e879a99e…jpg`
  dialogue anatomy (framed scene + text box + HUD). Hand-drawn-first;
  quantize-from-paint/photobash is the fast path. Full spec:
  `docs/art/portrait-register.md`.
- **Vignette treatment (§4 decision 4):** ALL decision events get framed-panel
  vignette art — ink compositions quantized to the local biome 4-value ramp.
  Effort is tiered, not coverage: bespoke plates for signature/faction dilemmas; a
  remixable library (re-quantized ramps, crop variants) for minor events.
- **Reserved red / rubrication (§4 decision 2):** red is a globally reserved
  accent meaning **cost / temptation / violation** — never damage, never
  enemy-coding, never decoration. Lives at the UI/event layer plus at most 1–2
  world-scale set-piece fields per run; never as isolated sprite-scale pixels.
- **The two-register record (§4 decision 5):** the whole art direction is the
  kingdom's own record. Stitchers' tapestry = the pixel registers (world, sprites,
  UI frames, title screen; stitch grammar on made objects + terrain patterning
  only — living creatures and raw stone never read as fabric). Scribes' chronicle
  = the vignette register (inked plates, red rubrication). The Unwitnessed appear
  in neither: their panel is an unpicked hole.
- **Charm channels (§4 decision 6, REVISED 2026-07-11):** combat-scale gameplay
  sprites — heroes, retinue, and standard enemies including the Still Legion — are
  **chibi**; higher-resolution non-chibi proportions are reserved for the
  portrait/avatar register only (unaffected, still `portrait-register.md`). The
  Unwitnessed titans stay non-chibi and terrain-scale on purpose: chibi-friendly
  vs. huge-detailed-horror is an intentional scale-contrast device that makes the
  horror land harder, not a mismatch to fix. On top of that baseline: charm = (a)
  animation personality + hearth props in the Gatecamp only, (b) Kitchen-Tin
  palette warmth everywhere friendly, (c) rescued civilians as soft bundled
  silhouettes with held lamps and sober motion. Friendly-side charm anchors
  (`memento-mori`, `Dessertry`, `1d9f9329…`) contribute coziness, not comedy — the
  Gatecamp is the warmest register in the game.
**Trade-off:** requires palette discipline (every new palette must protect the mark
reservations and the warm/cold channel) and a palette-swap pipeline, but it is the
only direction that serves all three: horde readability, three-biome variety, and
the tragic-but-hopeful tone. Assumes GDD #6 = per-faction (the current lean).
**Note (2026-07-11):** at the time this was written the two existing specs
(`warden-captain-bree.md`, `brees-stairwell.md`) needed no rework; that is no
longer true after the revised decision 6 chibi ruling — see the flagged-file list
below §4.

### Direction C — "Illuminated Depths" (rich per-biome color, charm-forward)

**Bundle:** keep 4 values per sprite but let palettes go saturated and varied per
biome/room, in the direction of `metroidvania-biomes` at full richness,
`praise-the-moon`, `memento-mori`, `bossroom_corruptcastellum` for bosses; portraits
go FFT-painterly (`2919ac96…`, `892dff86…`).
**Trade-off:** the prettiest screenshots and the friendliest tone fit — but at 500
units the hue variety starts colliding with the faction/mark value reservations, the
bright-value-scarcity safety grammar (spec 002) weakens when every biome sparkles,
and it abandons the board's own monochrome majority. Highest art cost, weakest
at-scale readability. Not recommended as the base; salvage its energy as the *boss
arena / late-floor escalation* register if ever needed.

### Recommendation

**Direction B.** Reasons, in pillar order: (1) *Readable at scale* — shared dark
anchor + scarce brights means friend/foe/mark channels survive 500 moving units,
and it is the only direction where the Lampbearer/safe-room glow grammar (already
specced and canon-adjacent) stays unforgeable; (2) it matches the pipeline — flat
ramps + palette swaps are cheap material parameters under Unlit Masked with
RGB→Emissive, and coarse dither survives Nearest at 48×48 where halftone and
crosshatch don't; (3) it serves the fiction — tragic Legion reads cold-but-human,
the Quiet reads as absence, the Unwitnessed get the board's full horror without
leaking it onto the mournable factions, and the good-guys warmth lives in the
friendly third-slot + honest light; (4) it is continuous with `warden-captain-bree.md`
and `brees-stairwell.md` — the board *confirms* those specs rather than overturning
them (`final-moments` is practically Bree's watch-lamp as a creature).

---

## 4. Decision log — resolved 2026-07-10

All six open questions were resolved with the owner, references in hand, one by one.
**Direction B ("Lamplit Ledger") is locked.** The nuances below are binding on all
future specs; refinements are folded into the §3 Direction B bundle.

1. **Portrait register — RESOLVED: posterized 4-value busts, EXACT sprite palette.**
   A named character's portrait uses only that character's sprite palette — Bree's
   bust is Vault Dark / Patched Steel / Kitchen Tin / Watch-Lamp at higher
   resolution, lamp pixel included. **Names/roles unchanged, hexes and the
   "Watch-Lamp" bright-name are superseded by the 2026-07-11 shape-only palette
   revision** (now Vault Dark `#211210` / Patched Steel `#5e2d20` / Kitchen Tin
   `#c76b2a` / Gatecamp Bright `#f0c260` — see `hero-palettes.md` §0); Bree's own
   portrait spec has not been redrawn to match, so treat this line as the rule,
   not the current values. Hand-drawn-first is the craft standard;
   paint-or-photobash-then-quantize (`c72ab868…` proof) is the sanctioned fast path.
   Presentation: **stitched-medallion framing** — busts inside circular
   sampler-style borders (`02210b4f…` roundels), as if each named character is
   embroidered into the kingdom's record (per decision 5). Dialogue anatomy per
   `e879a99e…jpg`: framed scene + text box + HUD. Full spec:
   `docs/art/portrait-register.md`.
2. **Red — RESOLVED: RESERVED, not banned.** Semantic: **cost / temptation /
   violation only** — red appears when the game asks you to pay or warns that
   something was paid (Dark Bargain, blood-price choices). NEVER damage feedback,
   never enemy-coding, never decoration. Scales: the UI/event layer, PLUS rare
   world-scale set-pieces (`winter-power-pole`-style whole-room fields) on a hard
   budget of **~1–2 per run**. Never isolated sprite-scale pixels — this protects
   the honest-light channel from a competing bright. Diegetic name (per decision 5):
   **rubrication** — the chronicle's red ink.
3. **Horror ceiling — RESOLVED (custom): baseline face-titans / no-gore, with
   occasional level-3 body-horror spikes reserved for the Unwitnessed alone.**
   Since viscera cannot render at 48×48/4-value, the spikes live **only in large
   static registers**: event vignettes, breach reveals, multi-tile titan
   set-pieces — never on gameplay sprites. Faces are **deliberately ambiguous in
   the art itself**: occasional half-recognized helm / almost-familiar face is
   allowed; the art flirts with victim-reads but never confirms one (no confirmed
   gear, no repeated individuals). The rescue-instinct risk is mitigated
   mechanically (titans never offer interaction prompts) plus Gatecamp lore debate.
   **Expression ceiling: calm/vacant, never agony** — anywhere a face appears.
4. **Vignettes — RESOLVED: ALL decision events get vignette art** (owner chose
   broader coverage than the recommendation), framed-panel anatomy: bordered scene
   above the text box, HUD visible. Cost mitigation: **tier the effort, not the
   coverage** — bespoke compositions for signature/faction dilemmas; minor events
   draw from a remixable library (same composition re-quantized to a different
   biome ramp, crop variants). Pipeline: ink compositions quantized to the local
   4-value ramp, as fenced in tension §2.3.
5. **Tapestry conceit — RESOLVED: ADOPTED FULLY as canon**, expanded to a
   **two-register record**: the *stitchers* keep the tapestry (world, sprites,
   terrain patterning, UI frames, title screen — one stitch = one pixel); the
   *scribes* keep the chronicle (event vignettes = inked chronicle plates;
   reserved red = rubrication). *To be stitched into the record is to be
   witnessed* — the Unwitnessed have an **unpicked hole** where their panel should
   be, which is what justifies decision 3's ambiguous faces: the record cannot
   depict them. Gatecamp tapestry = V1 flag-mirror centerpiece: one arched panel
   per world flag, stitched in as districts are permanently retaken. Fence against
   tweeness: stitch grammar only on the kingdom's *made objects* + terrain
   patterning; **living creatures and raw stone never read as fabric**.
6. **Charm dial — REVISED 2026-07-11 (supersedes the 2026-07-10 ruling; owner
   directive, binding).**

   **Original 2026-07-10 ruling (superseded, kept for history):** "Proportions
   consistent everywhere — no chibi split, because friendlies must stay in the
   Legion silhouette family for the mirror-fight read (Bree spec)." This reasoning
   is now **wrong** and must not be assumed by any new spec.

   **New ruling, owner's verbatim intent (2026-07-11):** *"Chibi direction is what
   we want for combat with light. The stark contrast between that and the horrors
   is intended. The higher resolution is for avatar icons. Simplistic chibi icons
   also allow us to scale up."*

   Unpacked, binding on all future specs:
   - **Chibi is the combat/gameplay-scale register**, not a separate overworld
     micro-avatar register as the original decision assumed. It applies to heroes,
     retinue, and standard-scale enemies — including the **Still Legion**.
   - **The mirror-fight read is re-derived, not broken.** If heroes went chibi
     while the Legion stayed non-chibi, the silhouette-family trick would collapse
     (different proportion languages can't mirror each other). The consistent
     resolution: **everything at combat scale goes chibi**, so Bree vs her Legion
     mirror reads chibi-to-chibi. The Legion-silhouette-family comparison rule
     (Bree spec, `hero-palettes.md`) still holds — it now operates *within* the
     chibi register instead of arguing against it.
   - **Scale/complexity contrast against horror is now a stated design
     principle, not an accident to fence.** Small, simple, low-detail chibi
     heroes/friendlies standing next to the large, detailed, non-chibi Unwitnessed
     titans (§1b, §3 Direction B "terrain-scale charcoal-dither masses") makes the
     horror land harder by sheer scale and complexity contrast. Do not chibi-fy the
     Unwitnessed titans to "match" — the mismatch is the point.
   - **The portrait/avatar register is the home for higher-resolution, non-chibi
     proportions**, and is otherwise **unaffected** — `portrait-register.md`'s
     posterized medallion busts stay exactly as specced (decision 1).
   - **Practical rationale, on the record:** simpler chibi sprites are cheaper to
     draw and cheaper to render, which directly serves the "500 units on screen"
     horde requirement (GDD §2 pillar 4 / CLASSES.md / spec 002). This is a stated
     design and production rationale, not incidental taste.
   - **Charm channels (a)/(b)/(c) are unchanged** and now layer on top of the
     chibi baseline rather than a realist one: **(a)** animation personality +
     hearth props, Gatecamp only; **(b)** palette warmth (the Kitchen Tin channel),
     everywhere friendly; **(c)** rescued civilians = soft bundled silhouettes,
     held lamps, one warm value, sober motion. Dessertry's *comedy* is still cut;
     its coziness-as-reward is still the Gatecamp register.
   - **Not resolved here:** exact chibi proportion ratio (head:body), whether it
     changes the 48×48 cell size or grid, and which existing specs need a redraw
     pass. Those are scoped as follow-up work, not decided by this amendment.

**Specs flagged stale by this revision (scoping note, not yet actioned):**
`docs/art/hallam.md`, `docs/art/edda.md`, `docs/art/merle.md`, `docs/art/noll.md`,
`docs/art/warden-captain-bree.md`, `docs/art/brees-stairwell.md` — all currently
spec 48×48 sprites with proportions consistent to the Legion silhouette family
under the *original* decision 6 (no chibi split). They need a revision pass to
either redraw at chibi proportions or explicitly declare a transition plan.
`docs/art/portrait-register.md` is **not** stale — the portrait register is
confirmed as the intended home for non-chibi proportions. `CLASSES.md`'s
per-class "2-bit readability" sections should also be checked for
proportion-consistency language once this doc's revision pass begins (this is
canon territory, not this doc's to edit).

> **Update 2026-07-11 (later same day): `docs/art/hero-palettes.md` is no
> longer on this list — it has been fully rewritten under the "accept
> shape-only" palette revision (see the amendment banner at the top of this
> doc and `hero-palettes.md` §0) and is current.** The other six files on this
> list were stale on **two independent axes** — the original chibi-
> proportion ruling AND the palette hex/bright-count revision.
>
> **Update 2026-07-12: all six redrawn, both axes, in one pass — none of
> them are stale anymore.** `docs/art/hallam.md`, `docs/art/edda.md`,
> `docs/art/merle.md`, `docs/art/noll.md`, `docs/art/warden-captain-bree.md`,
> `docs/art/brees-stairwell.md` now render at chibi combat proportion and cite
> the current Gatecamp Bright `#f0c260` shape-only palette throughout; see
> `hero-palettes.md` §5 for the resolution note, including the naming call
> (four hero files stayed role-only in prose per CLASSES.md's 2026-07-11
> reversal; the two Bree files kept her name since she is a named NPC, not a
> playable class).

---

## Depends on

- **#6 (global vs per-faction palettes): RESOLVED 2026-07-12 — strict global
  palette.** Direction A is locked and requires this answer; Direction B required
  the per-faction answer and is now superseded; Direction C was incompatible with
  a global palette entirely and is dead. Any future spec that wants a
  faction-reserved value or a palette swap must get an explicit new owner
  exception — the default is now "no exceptions."
- **#5 (flipbooks vs flat-shaded 3D):** analysis is register-level and survives
  either answer; the dither rules (2×2 minimum on movers) assume **flipbooks on
  instanced quads** and would need re-testing under flat-shaded 3D.

---

## Canon proposals

Status legend: **adopted-by-owner** = decided 2026-07-10, awaiting the actual
WORLD.md/GDD.md text (owner / narrative-director territory — not this doc's to
write); **proposed** = still awaiting a decision.

1. **The needlework frame — ADOPTED-BY-OWNER (pending WORLD.md text), expanded to
   the two-register record (WORLD.md §1 or §6):** *the Undervault keeps a
   two-register record: the stitchers' tapestry (counted-stitch sampler-work — the
   game's pixel art is the kingdom depicting itself) and the scribes' chronicle
   (inked plates — the event vignettes). To be stitched into the record is to be
   witnessed.* Gives the Gatecamp its flag-driven tapestry (one arched panel per
   world flag, stitched in as districts are permanently retaken) and licenses
   stitch-pattern terrain detail. Fence: stitch grammar on made objects + terrain
   patterning only; living creatures and raw stone never read as fabric.
2. **Light-temperature law — proposed (GDD §2 pillar 4 / extends spec-001
   honest-light rule):** *warm bright = honest light (players, safety,
   restoration); cold bright = the Crown's order and the deep dark; the two never
   swap roles.* The board applies this consistently; making it canon protects
   every future palette. Still pending an owner decision as of 2026-07-10.
3. **Faction art-language table — proposed (WORLD.md §3, three lines):** Quiet =
   negative-space silhouettes with bright eyes; Still Legion = intact cold
   uniforms, hollow helms, crest-coded officers; Unwitnessed = terrain-scale
   charcoal-dither masses with human-derived features, sole holder of full horror.
   Fences tension §2.2 permanently.
4. **The rubrication rule — ADOPTED-BY-OWNER (pending GDD/WORLD text):** *red is
   the chronicle's rubric ink — reserved for cost, temptation, and violation; it
   appears when the game asks you to pay or records that something was paid.
   Never damage feedback, never enemy-coding, never decoration; UI/event layer
   plus at most 1–2 world set-pieces per run; never sprite-scale pixels.*
5. **The unpicked panel — ADOPTED-BY-OWNER (pending WORLD.md text):** *the record
   cannot depict the Unwitnessed; where their panel should be there is an unpicked
   hole.* This is the diegetic justification for ambiguous faces (decision 3), the
   Unwitnessed having no portrait medallions, and any codex/tapestry gap the UI
   shows.
6. **Titans never prompt — ADOPTED-BY-OWNER (mechanical fence, GDD territory):**
   *Unwitnessed titans never offer interaction prompts.* Protects players from the
   rescue-instinct trap the ambiguous-face art deliberately creates; pairs with
   Gatecamp lore debate as the narrative-side mitigation.
