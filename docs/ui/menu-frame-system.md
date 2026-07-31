# The Sampler Frame + the Muster — menu shell & central unit-management HUD

**What this is:** the master menu-frame layout system for Kindled, and the first content that fills its central well — an RTS-style company readout with health meters.
**Extends:** `docs/art/aesthetic-direction.md` (locked Direction A + UI register), `docs/art/portrait-register.md` (medallions), `GDD.md` §4 (stances), `docs/RTS-VERTICAL-SLICE.md` §2/§5 (leash, minimal HUD).
**Mockup:** https://claude.ai/code/artifact/d22fcc8c-0b41-4888-a143-76b4776240c1 — *look at the empty middle of the woodcut becoming the company roster; resize narrow for the Steam-Deck collapse.*

---

## 1. Intent

The owner's north-star reference — the Harry-Clarke woodcut border (`Artboard/Grounding in the world/c680cee1…jpg`) — is canon-blessed for this exact use: `aesthetic-direction.md` §2.3 rules it *unbuildable as sprites but buildable as frame / event-card art, quantized to the 4-value ramp with coarse dither for the hatching.* So it becomes **The Sampler Frame**: reusable menu chrome that every full-screen menu inherits. Its signature is a fixed border of made objects — a beast lintel, two flanking guardian panels, corner rosette medallions — around **a central well that is the only part that changes per screen.**

The first well content answers the owner's second ask directly: **RTS-style unit management in the centre, with health meters.** For a *commander* game (not a micromanager game — GDD §4), the retinue is read as **squads, not soldiers** — square Warcraft-format unit cards, where each card's health *is* its **rank-and-file muster** (a grid of pips, one per soldier / per file at scale, lit = standing, dark = fallen). This is also the sim architecture: RTS-slice §5 commits to **aggregation — squad-as-entity, renders as N sprites** — so the game commands groups, and the UI binds to those same squads. Count, formation, health and scale become one read (see §5a).

**Frame regions used by this screen:** all of them (full-dress). **Regions that collapse on the Deck/HUD variant:** the flanking guardian figures drop to plain stitched rails; corner medallions shrink; the central well becomes the whole screen (see §8).

## 2. Layout anatomy

Target **16:9** desktop; **16:10** Steam Deck collapse. The play space is sacred in the live-HUD derivation (§8) — this full-dress version is the *menu / pause muster*, where the frame may own the screen.

```
┌─◉────────────── beast lintel ───────────────◉─┐   ◉ = corner rosette medallion
│            E M B E R K E E P   (crest)         │   header band  (halftone, static)
│            — the record keeps what you save —  │
├────────┬───────────────────────────┬──────────┤
│ GUARD  │            MUSTER          │  ORDERS  │
│ figure │  ┌ Vanguard Company ─────┐ │  figure  │
│        │  │ 99/180 · 85% · 5 squads│ │          │
│ hero   │  └───────────────────────┘ │  stance  │
│medallion  ┌Shield┐┌Shield┐┌Vets─┐   │  wheel   │
│  Vigor │  │▮▮▮▮▮▮││▮▮▮▮▮▯││▮▮▮▯ │   │  + leash │
│  88%   │  │ HOLD ││ HOLD ││CHRG │   │  gauge   │
│        │  └──────┘└──────┘└─────┘   │          │
│        │  ┌Banner┐┌Spearmen ×50──┐  │          │
│        │  │▮▮ FLW││▮▮▮▮▮▮▮ 10×5   │  │          │
│        │  └──────┘└ CHARGE ──────┘  │          │
├─◉──────┴───────────────────────────┴────────◉─┤
│  A Confirm  B Back  ▤ Wheel  ↹ Group   [red]  │   footer input band
└───────────────────────────────────────────────┘
```

- **Left flank** = the commander: guardian figure plate, hero **medallion** (portrait-register), hero Vigor meter. The hero is read as a *person*; the company is read as *mass*.
- **Central well** = the Muster: aggregate company meter on top, then a grid of **square squad cards**.
- **Right flank** = orders: the stance wheel (current stance lit) + the leash gauge with its 80% break warning.

## 3. Component inventory

Each derives from an 8bitcn pattern, **re-skinned to Demichrome — its paint is cut.** All states expressed as value-roles.

| Component | 8bitcn origin | States (value-role) |
|---|---|---|
| **Sampler Frame** (`WBP_SamplerFrame`) | Card / Sidebar chrome | Static furniture. 9-slice frame plate; Steel border; Dark ground; halftone bands. |
| **Company meter** | Health Bar | full = **Pale** pip; lost = **Steel** pip; empty = **Dark** pip w/ Steel outline. No hue. |
| **Square squad card** (`W_SquadCard`) | Item slot + Card | default = Steel border; **focus/selected = jump to Pale** (border + inset); routed-out squad = collapse to Steel, greyed size. Wide variant (`span 2`) for compacted 50-strong squads. |
| **Rank-and-file muster** (`W_MusterGrid`) | Health Bar (re-imagined) | the card's health *and* formation: a grid of pips, **Pale = standing soldier/file, Dark = fallen**. Cols = files. This replaces a linear bar — count, formation, health, and scale in one control. |
| **Stance chip** | Badge / Toggle | inactive = Steel outline / Bone text; **active = Pale fill / Dark text** (the "this is the order" read). |
| **Hero medallion** | Avatar | **reuse `T_UI_MedallionFrame` + portrait-register bust verbatim** — do not invent a second portrait style. |
| **Stance wheel** (`W_StanceWheel`) | Menubar / radial | 4 spokes Follow/Charge/Hold/Rally; current = Pale; leash-warning = Pale pulse on the hub. |
| **Leash gauge** | Progress | fill = Bone; track = Dark/Steel; **80% break marker = Pale tick** (warns before a Hold unit breaks — RTS-slice §2). |
| **Input hint** | Kbd | Bone glyph on Dark; controller-first glyphs. |

## 4. Palette & texture

Strictly the four locked values. Region map: **Dark** = ground/wells/ink; **Steel** = borders/tracks/inactive/spent; **Bone** = surfaces/body text/neutral fill; **Pale** = crest, focus, healthy meter fill, the single glint. **1px halftone** appears on the three static bands only (header, footer, company card) — legal because they are pixel-locked and never scaled (§2.4). **Red is absent** from this screen and confirmed so in the footer legend — a muster is not a cost moment. Rubrication enters only on decision-event cards (separate spec) where the game asks the player to *pay*.

## 5. Data bindings

Grep `ELVTR/Source/ELVTR/` before wiring — the swarm subsystem holds the real state.

| Element | Binding | Status |
|---|---|---|
| Company standing / % | retinue count vs. per-floor cap (`SwarmSubsystem`) | **exists** (verify field names) |
| Per-group count + HP | aggregate per unit-type; needs a **group-health rollup** | **partial → gameplay-director** (is health per-entity or per-squad-aggregate? RTS-slice §5 wants aggregate) |
| Current stance | stance enum, `Follow/Charge/Hold/Rally` (GDD §4) | **exists** |
| Leash % + warning | unit distance vs. `LeashRadius`, warn at 80% (`LeashHysteresis`) | **exists** (RTS-slice §2 tunables) |
| Hero Vigor | hero pawn health | **exists** |

**Threshold question → gameplay-director:** what counts as "company critical" (the point the meter should read as alarm)? The UI reserves a state for it; the number is a balance call, not mine.

## 5a. Squads & scale compaction (the load-bearing idea)

The retinue is never a bag of N individual soldiers to the UI or the sim — it is a small set of **squads**, and squads are the unit of both logic and display. This is the owner's directive ("units become part of a squad rather than micromanaging EVERY unit") and it is already the committed sim architecture (RTS-slice §5: *aggregation — squad-as-entity, renders as N sprites*; GDD §10 entity architecture).

- **A squad has a size and a standing count.** The card shows `standing / size` (e.g. `38/50`); the muster grid shows one pip per soldier, Pale when standing.
- **Compaction scales the pip, not the card.** At small scale one pip = one soldier. As squads grow (20 → 50 → larger), a pip promotes to represent **a file** (a column of the rank), so a 50-strong squad reads as a 10×5 block and a 200-strong mass reads as, say, 20×10 — the card stays a fixed, glanceable object while the thing it represents scales. The mockup's wide Spearmen card shows the 50/10-file case.
- **The UI binds to the squad entity, not its members.** `W_SquadCard` reads squad-level state (size, standing, stance, formation); it must **never** iterate live per-soldier actors. This is what keeps the HUD cheap at horde scale and is the same promise the sim makes.
- **Selection & orders operate on squads.** Cycle/select a card = select a squad; the stance wheel issues to the selected squad(s) or the whole company. No soldier-level selection exists — by design.

This section is a **shared contract with the sim**: the squad is the entity both sides address. It is a §9 handoff to the gameplay- and performance-directors, not a UI-only convention.

## 6. Interaction & input

**Controller-first (Steam Deck is the target).** Focus order: company card → group rows (↹ cycles) → stance wheel. Stance is a **one-press radial** (`▤` / bumper), instant mid-combat — it must never be gated behind a menu; the wheel here is the *indicator*, the wheel-open is a HUD overlay. A-confirm / B-back. Mouse & keyboard are second-class but supported (row focus-within, click). Motion: transitions are stitch-wipe/ink-bleed *in spirit*, snappy in practice; a **baked Pale flicker** on glints only (portrait-register §3), disabled under `prefers-reduced-motion`. **Nothing animates on the path between pressing a stance and the order landing.**

## 7. UMG translation

- `WBP_SamplerFrame` — a reusable UserWidget: `UImage` 9-slice frame plates in named slots (lintel, L/R flank, corners, footer), a `UNamedSlot` "Well" that every screen fills. This is the one asset that makes "menu = well + frame" real.
- Meters — `UProgressBar` is too smooth; build a **segmented meter widget** (`W_StitchMeter`) as a horizontal box of `UImage` pips for the company/hero readouts. Bind `FullSegments`/`Total`.
- Squad cards — `W_SquadCard` (a `UUserWidget` bound to one squad entity) containing `W_MusterGrid`: a `UUniformGridPanel` of `UImage` pips, `Columns` = files, each pip Standing/Fallen. The grid reads squad-level state only (§5a) — never a per-soldier loop. A `UWrapBox`/`UUniformGridPanel` hosts the cards; the wide variant spans two columns.
- Stance wheel — custom `W_StanceWheel` (Slate `SRadialBox`-style or 4 anchored buttons); the in-combat open is a separate HUD overlay widget, not this indicator.
- Medallion — `UImage` with the portrait material (RGB→Emissive, A→mask), UV-offset for flicker frame; **no per-portrait material work**.
- Import discipline for all frame/medallion/meter art: **Nearest, NoMipmaps, UserInterface2D** (`SETUP-EDITOR.md`).

## 8. The Deck / live-HUD collapse (noted, spec'd next)

The full-dress frame is the **menu / pause muster**. In live combat the same well content survives but the frame yields the screen to the battle (hard constraint §5): guardians and lintel drop, corner medallions shrink, the company meter + stance indicator dock to a thin bottom band, group rows become optional (opened on the stance wheel). That derivation is its own spec (`docs/ui/combat-hud.md`) — flagged here so the shared widgets (`W_StitchMeter`, `W_StanceWheel`, `W_SquadCard`/`W_MusterGrid`, medallion) are built once for both.

**First-look mockup, over real gameplay:** https://claude.ai/code/artifact/4eb494b3-c7df-47c0-9231-555465949675 — the HUD composited onto the current build's actual capture (`ELVTR/Saved/Screenshots/WindowsEditor/SwarmDebugShot00000.png`), with the ops panel replacing the programmer debug text in-place and the HUD state matched to the screenshot (Follow / retinue 120 / hero 500·500 / wave cleared). Note this is a *screenshot composite*, not a live viewport — it validates placement and the "does the HUD leave the swarm readable?" question, which is the real §Readability check to run against the pixel-art-director once it's in UMG over a moving field.

## 9. Handoffs

**Art briefs raised** (to pixel-art-director — write as `brief-<id>-*.md` when actioned): (a) **Sampler Frame plate set** — beast lintel, L/R guardian panels, corner rosettes, footer band, all quantized to the 4 values with coarse dither for the woodcut hatching; (b) **blackletter crest treatment** for the title (the "Old Master" illuminated register, `05aaaecf…`); (c) **stance-wheel + meter end-cap icons**. The mockup's dragon and guardians are placeholders standing in for (a).

**Gameplay-director:** the squad-health rollup binding and the "company critical" threshold (§5); the squad-composition rules (how many soldiers per squad, when a new squad forms vs. an existing one refills — §5a).

**Performance-director:** confirm the **squad-as-entity** boundary (§5a) so the HUD binds to squad state, not per-soldier actors — the pip→file compaction rides on the same aggregation you own (RTS-slice §5). Flag the entity-count at which a pip must stop meaning one soldier and start meaning a file.

## Readability check

*(to pixel-art-director — this is the live-HUD bar, not a brief)* The combat derivation (§8) puts the company meter + stance indicator over the play space. I assumed: a single thin bottom band, Pale used only for the meter fill and the active stance, no tint or bloom over the field, and fodder never getting enemy bars (only elites/boss, RTS-slice §5). **Please confirm this holds at 500 moving units** — specifically that a Pale meter fill at screen-bottom doesn't compete with the Lampbearer's honest-light glint or the Pathfinder's quarry mark for the eye's "brightest thing" read.

## Canon proposals

None — this spec sits inside locked canon. (The one thing it *leans on* that isn't yet written: a `T_UI_MedallionFrame` shared asset, already anticipated by `portrait-register.md` §4.)
