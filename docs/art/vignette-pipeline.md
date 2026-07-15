# Chronicle plates — vignette pipeline (system spec, all decision events)

**Brief:** `../briefs/brief-003-vignette-pipeline.md` · **Source decisions:** `aesthetic-direction.md`
§3 (Direction B bundle), §4 decisions 2/3/4/5 (locked 2026-07-10) · **Events:** `WORLD.md` §8 (E1–E8)
**Craft anchors:** `Artboard/Landscape shots/4551f4b0….jpg` (heavy-black ink interior — the plate
tonal target) · `Artboard/Monsters Aesthetic/1853566f….jpg` (ink cage/horde — the prisoner-column
master plate energy, and proof crosshatch density survives as coarse dither) ·
`Artboard/Portrait Styles/e879a99e….jpg` (screen anatomy: framed scene + text box + live HUD) ·
`Artboard/Grounding in the world/05aaaecf….jpg` (blackletter/rubric type register, ruled manuscript
borders)
**Pipeline discipline:** `ELVTR/SETUP-EDITOR.md` §1 import rules · **Companion:**
`portrait-register.md` (the medallion that sits beside these plates)

This is a *system* spec: it governs how every decision-event plate is made, framed, tiered, and
displayed. Individual plates get one-line entries in event/content specs (master, ramp, crop),
not new documents — same rule as portraits.

---

## 1. Intent

Fiction: the scribes' chronicle is the second register of the kingdom's record (canon proposal 1,
adopted). A decision event is the moment an entry is written — the plate is the illustration the
scribe inks *while the party chooses*, and rubrication is the red ink reserved for cost, temptation,
and violation. The register is always: **this is being written down.** Gameplay: plates carry the
weight of choice (E1's tragedy, E3's temptation, E7's private dread) without ever hiding the run —
the HUD and the fight stay visible, because ELVTR is a co-op game and the world does not stop for
one player's conscience. Plates are also the **only render-capable home for Unwitnessed level-3
horror** (decision 3) — the escalation rules in §5 are the fence that keeps that horror inside
this register.

---

## 2. Framed-panel anatomy

### 2.1 Screen footprint

All sizes in **reference pixels** at a 640×360 virtual canvas, integer-scaled to screen (the same
scale law as sprites: one texel = one screen pixel × global integer zoom; the record does not
change gauge). *The 640×360 reference resolution is not yet canon — see Canon proposals.*

```
+--------------------------------------------------------------+
| HUD: floor/torch          (live game, halftone scrim)    HUD |
|            +----------------------------------+              |
|            | +------------------------------+ |              |
|   live     | |                              | |    live      |
|   game     | |     PLATE  384 × 216         | |    game      |
|  visible   | |     (scene art, 4 values)    | |   visible    |
|            | +------------------------------+ |              |
|            |  (O)  The Oath-Stone             |  <- medallion |
|            |  ¶ prose, 2-4 lines              |     overlaps  |
|            |  > Recall the veteran            |     corner    |
|            |  > Release them            (F1↑) |              |
|            +----------------------------------+              |
| HUD: party bars           (live game)             HUD: co-op |
+--------------------------------------------------------------+
```

- **Standard plate assembly (E1, E3, E4, E5, E6, E8):** total footprint **≤ 62% screen width ×
  ≤ 72% screen height, centered** — at reference scale: plate art **384×216**, text box beneath it
  **384×~96** (prose 2–4 lines + 2–3 option lines). A **10% margin band on all four screen edges
  is inviolate**: no plate element may enter it, ever. All HUD lives in that band and renders
  **above** everything, including the scrim.
- **Scrim:** the live game around the assembly dims under a **static 1px halftone scrim** (the
  §1d UI register — pixel-locked, legal at 1px because nothing under it needs parsing at full
  contrast while a tentpole is open). The scrim never covers the HUD band. E2 and E7 get **no
  scrim** (§3, §4).
- **The world keeps rendering.** The plate is an overlay, not a screen. Whether tentpole events
  soft-pause in solo play is a design call, not an art call — this spec guarantees legibility
  either way.

### 2.2 The chronicle frame — distinct from stitch grammar

The frame is a **shared UI asset** (`T_UI_ChronicleFrame.png`), not part of any plate texture —
same auditability rule as the portrait medallion. Register: **ruled manuscript page**, per the
`05aaaecf…` anchor stripped to ink:

- **Outer rule:** 3px solid darkest value (Vault Dark family). **Inner rule:** 1px fine line,
  separated by a 4px gutter of the plate ramp's *third* (parchment/mid-light) value. Corners:
  small drawn fleuron/knotwork ties in ink line — **drawn, continuous, calligraphic**.
- **The distinction from tapestry grammar, enforced:** the stitchers' register (portrait
  medallions, UI sampler borders) is *counted* — discrete stitches, circular hoops, cross-stitch
  motifs. The scribes' register is *drawn* — continuous ruled lines, rectangular plates, pen
  fleurons, blackletter versals. **No stitch motifs, no circular framing, no counted-pattern
  borders on any chronicle element.** A player should be able to tell "the tapestry is showing me
  this" (medallion, roundel) from "the chronicle is recording this" (ruled plate) with zero text.
- **Frame states baked as variants:** normal · rubricated (§4) · broken (§5). One asset, three
  states, spec'd here once.

### 2.3 Text box and options

- **Title line:** event name with a **blackletter versal initial** (the `05aaaecf…` type register)
  — the illuminated capital is the one flourish the text layer gets. Body prose: the game's
  standard UI face, 2–4 lines maximum.
- **Options (2–3):** one ruled line each, marked with a **pilcrow (¶)**. Hover/selected state:
  the pilcrow becomes a **manicule (☞)** and the line renders in the ramp's brightest text value.
  On commit, the chosen line gets a 2-frame "ink-in" (text re-renders bolder, one underline rule
  draws left-to-right) — the entry is written. Non-chosen options fade to the mid value; they
  remain in the box until the panel closes (the chronicle shows what was *not* chosen — that is
  the point of a record).
- **Pay-a-cost options render in rubric red** — full line + marker, per §4. Mechanical
  consequence tags (`F1↑`, `run boon`) right-align on the option line in the standard UI face.
- **Party-vote events (E3):** each option line carries up to 4 vote pips (player-slot glyphs) at
  its right end. Pips are UI-register, 1-bit.
- **Portrait medallion (E8 and any named-NPC event):** the character's stitched medallion
  (`portrait-register.md`) sits at the assembly's lower-left, overlapping the frame corner —
  the two registers of the record touching. The medallion keeps its own rules (exact sprite
  palette, stitched hoop); the plate keeps its own. They share no values and never blend.

### 2.4 HUD visibility guarantee (hard rules)

1. No plate element in the 10% edge band, any tier, any state.
2. HUD draws above scrim and plate, always.
3. Total assembly ≤ 62% × 72% (standard), ≤ 40% × 45% (E7), ≤ 30% × 15% (E2 margin note).
4. The scrim dims but never occludes: max density 50% halftone — moving units stay readable
   through it (a Legion push arriving mid-choice must be visible *through* the chronicle moment).

---

## 3. Effort tiers — tiered effort, not coverage

All eight events get plate art (decision 4). The tiers spend *production count*, never quality.

| Tier | Events | Treatment | Unique art cost (v1) |
|---|---|---|---|
| **Tentpole** | E3, E8 | Bespoke full plates, one composition per subject | E3: 1 plate ×3 biome ramps · E8: 5 plates (one per named NPC) |
| **Library** | E1, E4, E5, E6 | Remixable master plates, emitted per biome/crop | ~8 grayscale masters covering 4 templates |
| **Margin note** | E2 | In-combat marginalia strip — no plate, no scrim, no pause | 2–3 stamps (Unlit variants) |

### 3.1 Tentpoles (E3, E8)

Full-canvas unique ink compositions at maximum craft. E3 (*The Titan's Heart*) is additionally the
horror-escalation reference plate — fully specified in §5.3. E8 (*The Keeper's Ask*) gets one
bespoke plate per named NPC (N1–N5), composed around their site (Bree's barricade, Maro's forge,
the brazier route, the kennels, the great door) with the NPC's medallion at the corner per §2.3.
Tentpole plates are the only tier allowed multi-figure compositions and full-depth scenes
(the `4551f4b0…` corridor register: heavy black, one aperture of light, skulls in the masonry).

### 3.2 The remixable plate library (E1, E4, E5, E6)

**Masters:** ~8 grayscale ink compositions drawn once at bespoke quality, each owning a subject:

| Master | Serves | Composition seed |
|---|---|---|
| `OathStone` | E1 | Standing stone, kneeling hollow soldier, empty helm at its foot |
| `Column` | E4 | Prisoner column between pike ranks, receding — the `1853566f…` cage energy, marching |
| `Pens` | E4, E5 | Barred arch crammed with the held (`1853566f…` directly), one torch |
| `BreachRoute` | E5 | Broken wall, rope down into dark, tiny figures |
| `QuietRoute` | E5 | Narrow stair, lamp shaded to a sliver, shapes waiting |
| `Shrine` | E6 | Lamp-hall nave, one shrine flame, gathered soul-lights |
| `GateArena` | E1, E5 | Gate plaza, shield line silhouettes, high dark architecture |
| `Cistern` | E4, E5, E6 | Flooded hall, reflections carry the only light |

**What a remix MAY vary (and the first two are mandatory per emission):**

1. **Biome ramp** — the master re-quantizes to the local biome's 4-value ramp (Highgates stone,
   Sunken Works water-cold, Vesper Halls deep-dark). This is the load-bearing variation: the same
   scene in the Vesper ramp *is* a different place, because under Direction B the ramp is the
   place.
2. **Crop window** — masters are authored at 576×324 (1.5× the display plate); the display
   emission is a 384×216 window. Different crops recompose the scene (the column head vs. its
   tail; the shrine near vs. across the nave).
3. **Horizontal mirror.**
4. **Dither seed** — the ordered-dither pattern phase/direction (§6.3) may re-roll, changing the
   hatching texture without touching the ink.

**What a remix may NEVER vary:** the ink itself (no content edits, no element recoloring, no
pasted-in props), the value-role assignments (darkest = ground/ink mass; bright = the honest
light source, one per plate, or absent), the frame, the type register, or the dither coarseness
rules. A remix is the *scribe re-inking the same scene in the local district's ink* — it is
diegetic, not a palette-swap apology.

**Repeat protection:** within one run, no two emissions may share the same (master, ramp, crop)
triple; across a profile, the emitter prefers the least-recently-seen crop per master. A remix
shown next to a tentpole must differ from it in *scale of subject*, not in finish — masters are
drawn at tentpole craft; the tier saves count (8 masters serve 4 templates × 3 biomes × crops ≈
dozens of distinct-feeling plates), not quality.

### 3.3 E2 — the margin note (must not stop the fight)

E2 (*Soothe or Strike*) is constant, in-combat texture. It gets **no plate, no scrim, no text box,
no camera change**. Treatment: a **marginalia strip** docked to the screen-edge side nearest the
soothable Unlit, inside the standard HUD layer (respecting the same edge-band rules as HUD, because
it *is* HUD-tier UI):

- **Anatomy:** 96×48 inked stamp (a tiny chronicle sketch of an Unlit — soul-light in a dark
  shroud, calm) + one option line ("Soothe — hold ¤" / "Strike") + a channel pip when soothing.
  Total footprint ≤ 30% × 15% of screen.
- The stamp is drawn once per Unlit variant (2–3 for v1), quantized on the Vesper ramp regardless
  of floor (the Unlit are the Quiet's; their ink is always the deep ramp — cheap and correct).
- Diegetically these are **margin notes**: the scribe has no time to plate every mercy; E2
  entries are written in the chronicle's margin. Soothing under pressure while the note sits at
  the screen edge *is* the design — the fight stays the subject.
- Frame: single 1px rule, no fleurons, no versal. The margin is humble.

---

## 4. Rubrication — the warning ink

**Value:** `#b13e53 · Rubric Red`. It is **not** a member of any plate ramp — plates stay 4 values.
Rubric red is a **UI/event-layer value** (decision 2's sanctioned scale) living only on the frame
and text layers of this register. Budget and semantics per canon: cost / temptation / violation;
never damage, never enemy-coding, never decoration; never in the scene art itself.

**Where it may appear — exhaustive list:**

1. **Pay-a-cost option lines** (any event): the full option text + its pilcrow/manicule render in
   Rubric Red. Nothing else on the assembly changes. One glance at the box tells you which lines
   cost blood.
2. **E7 frame accent:** the frame's inner fine rule renders in Rubric Red for the whole panel
   (§4.1). Only E7 rubricates the frame — on all other events the frame stays ink, so a
   red-ruled frame *means* bargain.
3. **E7 versal:** the title initial is rubricated — the entry is being written in red.
4. **Consequence echo (optional, post-choice):** when a rubricated option is chosen, the ink-in
   underline draws in Rubric Red and the closed entry's title keeps its red versal in any
   chronicle/codex UI. The record remembers what was paid.

Never: red in plate scene art, red on the scrim, red on medallions, red on E2 margin notes
(soothe/strike is mercy-vs-dps, not a hidden cost — F2 movement is transparent), red anywhere at
sprite scale (protected by decision 2 already).

### 4.1 E7 — The Dark Bargain: intimate composition rules

E7 is private to one player: others see the outcome, never the offer. The panel must read as a
whisper, not a proclamation.

- **Footprint:** ≤ 40% × 45% of screen, **anchored off-center** (lower-right quadrant by default,
  mirrored if it would cover the owning player's character). No scrim — the room does not know
  this is happening. No pause. The fight around it stays fully live.
- **Plate:** 256×144. Composition law: **close crop, single subject, dominant negative space.**
  The whisperer is never shown whole — an aperture in the dark, an offering hand, the offered
  thing itself (a titan-touched shard, a hybrid seed) as the plate's only bright value. No party
  figures, no architecture wider than a doorway. If the offer is titan-touched, §5 escalation
  devices apply at whisper scale (a too-many-jointed hand is legal; a titan is not).
- **Frame:** thin — 1px outer rule only, plus the rubricated inner rule (§4). No fleurons. The
  smallest, quietest, reddest frame in the game.
- **Text:** prose max 2 lines; options max 2; the accepting option is always rubricated, the
  refusing option never is.
- **Diegetic read (canon proposal 3):** bargain entries appear in the chronicle *already
  written*, in red, in a hand no scribe owns. The player isn't making a record — they're finding
  one.

```
Standard plate (centered, ruled, scrim)      E7 (off-center, thin, red rule, no scrim)
+================================+
|  scene 384x216                 |                          +----------------+
|                                |                          | dark aperture, |
+================================+                          | one bright     |
| ¶ option                       |                          | offered thing  |
| ¶ option (cost)      <- rubric |                          +-- red rule ----+
+================================+                          | ¶ accept (red) |
                                                            | ¶ refuse       |
                                                            +----------------+
```

---

## 5. Horror escalation — Unwitnessed level-3 plates

This register is the only home for level-3 body-horror spikes (decision 3), Unwitnessed only.
The ceiling holds everywhere: **calm/vacant, never agony; ambiguous, never a confirmed victim;
no gore, no wounds, no viscera.** Horror is escalated through five sanctioned devices, applied
in order — a plate uses only as many as its moment earns:

1. **Scale against the frame.** The subject exceeds the plate: cropped by the border on at least
   two sides, no full silhouette ever shown. The chronicle cannot fit them — the composition
   admits it. (Board grammar: tiny figures against vast masses, `fb1c84ea…` / `4551f4b0…`.)
2. **Wrong count.** Hatching-direction breaks and dither-field discontinuities where anatomy
   repeats past sense — joints, knuckles, rib-like ranks that don't resolve. The viewer's eye
   loses count; nothing is depicted wrongly enough to point at.
3. **The almost-face.** At most one half-recognized element per plate — a helm shape in the mass,
   a hand the right size — rendered calm/vacant. Flirt, never confirm: no identifiable gear, no
   individual repeated across plates. (The record cannot depict them; these are the scribe's
   failures of nerve, not documentation.)
4. **Inverted light.** Level-3 plates contain **no honest light**: the ramp's bright value is
   spent *inside the thing* (eyes, heart, seams) or not at all. Warm-bright is banned from these
   plates entirely — the one palette rule that flips, and only here.
5. **The frame fails.** The `broken` frame state: the inner rule is unfinished where the subject
   crosses it — the line simply stops, pen-lifted, and resumes on the far side. The chronicle-
   register counterpart of the tapestry's unpicked hole (canon proposal below). Reserved
   exclusively for Unwitnessed level-3 plates; it must stay rare to stay loud.

### 5.3 E3 — The Titan's Heart (reference plate, minimum bar)

The tentpole and the floor for what level-3 means. Composition: the slain titan's mass fills the
plate's upper two-thirds, cropped by the frame on three sides (device 1), its anatomy dissolving
into wrong-count dither passages (device 2), one almost-helm in the mass (device 3). The **heart
is the plate's only bright value** (device 4) — held low, small, genuinely beautiful: the
temptation must be real or the choice is fake. Party figures tiny at the bottom edge, backs to
the viewer. Frame state: broken along the top rule (device 5). Options: *Harvest* is rubricated
(the biggest boon in the game, and the Quiet notices); *Burn at the breach* is not. Three
emissions, one per Sunken Works breach ramp variant — this plate never remixes outside its ramp
family.

---

## 6. Solo-dev pipeline — ink → quantize → coarse dither

### 6.1 Canvases (reference pixels, 1 texel = 1 screen pixel at 640×360)

| Asset | Author at | Display | Notes |
|---|---|---|---|
| Library master | 576×324 grayscale | 384×216 crop window | 1.5× overdraw buys the crop variation |
| Tentpole plate | 384×216 (may author 576×324 if crops are ever wanted) | 384×216 | |
| E7 plate | 256×144 | 256×144 | no remixing, author at display size |
| E2 margin stamp | 96×48 | 96×48 | |

### 6.2 Steps

1. **Ink.** Digital or scanned traditional — line, hatch, and gray wash all legal *at this
   stage*; the quantize will crush them. Work over a 4-band gray underlay from the start
   (thumbnails posterized early) so the composition is proven in 4 values before rendering.
   Keep one honest light source per plate (or zero — see §5 device 4).
2. **Normalize.** Flatten to grayscale, levels so blacks sit at true black; kill paper texture
   (it quantizes to noise).
3. **Quantize.** Posterize to 4 luminance bands → map bands to the target biome ramp
   (dark→`ramp[0]` … light→`ramp[3]`). Per-biome ramps come from the environment palette specs
   (Highgates ramp already established in `brees-stairwell.md`; Sunken Works and Vesper Halls
   ramps to be authored under the same shared-Vault-Dark-anchor law). Library masters stay
   grayscale on disk; ramp mapping happens per emission.
4. **Coarse dither pass.** Replace band transitions with ordered dither (rules below), then
   **hand-clean**: kill stray isolated pixels, regularize pattern edges, re-audit the bright
   value (quantizers don't know which value is reserved — same law as portraits §2.2).
5. **Emit + import** per §6.4.

### 6.3 Dither coarseness floor (static plates)

Plates are static, pixel-locked UI — finer than the 2×2 moving-sprite rule is legal, down to a
hard floor of **1px, ordered patterns only**:

- **Legal:** Bayer 2×2 and 4×4 checker at 1px; **directional line dither** (1px lines, 2px
  spacing, any consistent angle) — the house pattern for plates, because it reads as the ink
  hatching it replaces. Tone steps: 25 / 50 / 75% only.
- **Banned:** error-diffusion (Floyd–Steinberg etc. — reads as photo grain, not pen), random
  noise dither, any pattern finer than 1px via downscaling, and mixed pattern families inside
  one continuous surface (one surface, one hatch direction — direction *changes* are reserved
  for §5 device 2).
- The 2×2 minimum still applies to anything on the plate that animates (nothing should — plates
  are still; the only sanctioned motion is the text-layer ink-in, §2.3, which is type, not art).

### 6.4 Files and import (per `ELVTR/SETUP-EDITOR.md` discipline)

- **Masters (not imported):** `ELVTR/RawArt/Vignettes/Masters/T_PlateMaster_<Subject>.png`
  (grayscale, 576×324).
- **Emissions (imported):** `ELVTR/RawArt/Vignettes/T_Plate_<Subject>_<Biome>.png` → imported to
  `Content/UI/Vignettes/`. Tentpoles: `T_Plate_E3_TitansHeart_SunkenWorks.png`,
  `T_Plate_E8_<NPCName>.png`. E7: `T_Plate_E7_<OfferType>.png`. Stamps:
  `T_Note_E2_<UnlitVariant>.png`.
- **Import settings:** Filter = **Nearest**, Mip Gen = NoMipmaps, Compression =
  **UserInterface2D**, sRGB = on — identical to SETUP-EDITOR §1 / portrait register §4.
- **Usage:** UMG Image (plates are UI, not Niagara); crop windows are UV rects on the master-
  sized emission texture, so one texture serves all its crops — the "remix" at runtime is a UV
  rect + which emission texture, zero material work. RGB→brush color as-is (unlit-equivalent),
  A unused (plates are fully opaque rectangles; only the broken-frame overlay uses alpha).
- **Shared assets:** `T_UI_ChronicleFrame.png` (states: normal / rubricated / broken),
  `T_UI_ChronicleVersals.png` (blackletter initial set, ink + rubric rows) — spec'd here, built
  once, reused by every event.
- Non-power-of-two sizes are deliberate and safe here: these are UMG textures, not SubUV sheets;
  the power-of-two rule binds Niagara flipbooks only.

---

## 7. Depends on

- **#5 (flipbooks vs flat-shaded 3D): Neither.** Plates are a pixel-locked UI register and
  survive either answer untouched (E2's margin note is HUD-docked, not world-anchored).
- **#6 (global vs per-faction palettes): assumes per-faction/per-biome — said loudly.** The
  entire remix tier (§3.2) rides on biome-ramp re-quantization; under a strict global palette,
  remixes collapse to crop+mirror only and the library would need ~2× the masters to avoid
  repetition-fatigue. Direction B has effectively locked the per-faction side, but if #6 ever
  reopens, this spec is the first casualty — flagging it.

---

## Canon proposals

1. **Margin notes (WORLD.md §8 design rules, one line):** *E2 and other in-combat micro-choices
   are recorded as margin notes, not plates — the chronicle's margin is where mercy under
   pressure gets written.* Licenses the no-plate treatment diegetically and fences future
   in-combat events into the cheap tier by canon rather than by budget.
2. **The unfinished entry (extends canon proposal 5, the unpicked hole):** *the chronicle's
   counterpart of the tapestry's unpicked hole — where an entry concerns the Unwitnessed, the
   scribe's rule-lines stop and resume; the entry is never finished.* Justifies the broken frame
   state (§5 device 5) and keeps both registers' treatment of the Unwitnessed consistent.
3. **The red hand (WORLD.md tone / E7):** *Dark Bargain entries appear in the chronicle already
   written, in red, in a hand no scribe owns.* Makes E7's rubricated frame diegetic and gives
   the temptation track a recurring, dread-loaded fiction beat for free.
4. **Reference virtual resolution (GDD §10, tech):** the 640×360 / integer-scale assumption in
   §2.1 and §6.1 needs an owner decision — every canvas in this spec is expressed in reference
   pixels against it. If the game ships at a different virtual resolution, sizes rescale
   proportionally (the ratios, edge bands, and all rules stand).
