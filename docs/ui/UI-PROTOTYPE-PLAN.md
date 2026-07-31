# UI Prototype Plan — Kindled UMG build (HUD + menu flow)

**What this is:** the implementation plan for the first real, interactive Kindled UI —
built **natively in Unreal UMG**, covering the full screen flow (main menu → muster →
combat HUD). This is the hand-off doc: a fresh session should be able to start
implementation from here without re-deriving context.

**Decided with owner (2026-07-23):**
- **Platform = UMG-native** (option 3). No web/React prototype. 8bitcn is used as an
  *aesthetic + component reference and an art-asset source*, not as a runtime.
- **Scope of milestone 1 = full flow**, HUD + menu: Sampler Frame main menu → Muster
  (roster) → Combat HUD collapse.
- **8bitcn assets get converted to UE.** Some art featured in 8bitcn (pixel font,
  stepped/9-slice borders, icons) will be pulled and converted to UE-compatible assets.
  We will likely build a **conversion skill or agent** to do this repeatably (§4).
- Owner will **personally revisit the aesthetic** against https://8bitcn.com before/while
  this proceeds — so treat §3 (aesthetic reconciliation) as *proposed direction the owner
  confirms*, not locked.

**Reads / extends (source of truth — do not restate their canon, bind to it):**
- `docs/ui/menu-frame-system.md` — The Sampler Frame + Muster spec. **§3 component
  inventory (8bitcn origin → Demichrome) and §7 UMG translation are the spine of this
  plan.**
- `docs/art/aesthetic-direction.md` — **Direction A LOCKED**: strict global 4-value
  Demichrome palette (`#211e20` Dark / `#555568` Steel / `#a0a08b` Bone / `#e9efec`
  Pale), chibi combat register, import discipline **Nearest / NoMipmaps /
  UserInterface2D**.
- `docs/narrative/FLAME-FOUNDATION.md` — narrative canon (flame-bearer premise).
- Mockups (the visual targets): Sampler Frame + Muster
  https://claude.ai/code/artifact/d22fcc8c-0b41-4888-a143-76b4776240c1 · Combat HUD
  (with Hero/Unit subcamera bookends) https://claude.ai/code/artifact/4eb494b3-c7df-47c0-9231-555465949675
  — local: `docs/ui/mockups/menu-frame-system.html`, `docs/ui/mockups/combat-hud.html`.

---

## 1. Goal & non-goals

**Goal (milestone 1):** a playable-feeling, controller-first UMG prototype of the whole
loop, driven by **mock state**, that answers the design questions the mockups can't:
does the frame→well→HUD collapse feel right, does stance switching land instantly, does
the static muster read at a glance, do the Hero/Unit subcamera cams add or distract.

**Non-goals for milestone 1:**
- Not wired to live gameplay yet — mock data structs only (real bindings are milestone 2,
  §6).
- Not final art — placeholder Demichrome plates + converted 8bitcn assets stand in for
  the pixel-art-director's bespoke Sampler Frame plate set.
- Not the shipping polish pass (animation, audio, reduced-motion variants come after the
  layout/flow is confirmed).

---

## 2. UMG architecture

**Plugin decision (confirm first thing):** use **CommonUI**. Rationale: the target is
controller-first Steam Deck (menu spec §6) — CommonUI gives the screen/activatable-widget
stack, gamepad focus routing, and input-action mapping we'd otherwise hand-roll. If the
owner rejects the dependency, fall back to plain UMG + a custom screen-stack manager
(more work, same shape).

**Content location:** `ELVTR/Content/UI/` (does not exist yet — create it). Suggested
tree:
```
Content/UI/
  Style/        DA_UIStyle (Demichrome tokens), F_UIFont (converted 8bitcn font),
                T_UI_Frame_9slice, T_UI_* brush textures, converted icon atlas
  Common/       shared widgets (built once for menu AND hud)
  Screens/      WBP_MainMenu, WBP_Muster, WBP_CombatHud
  RT/           RT_HeroCam, RT_UnitCam (render targets) + capture rig BP
```

### 2a. Shared widgets — build once, used by both menu and HUD

These come straight from `menu-frame-system.md` §3/§7. The combat HUD is the §8 "collapse"
of the same well content, so **every widget below serves both screens** — do not fork.

| Widget | Role | 8bitcn origin (re-skinned to Demichrome) |
|---|---|---|
| `WBP_SamplerFrame` | 9-slice menu chrome (lintel, L/R flanks, corners, footer) + a `NamedSlot` "Well" every screen fills | Card / Sidebar chrome |
| `W_StitchMeter` | segmented meter (`UImage` pips, not `UProgressBar`); `FullSegments`/`Total` | Health Bar |
| `W_SquadCard` | one square card bound to **one squad entity**; default Steel border, selected→Pale; `wide` variant spans 2 | Item slot + Card |
| `W_MusterGrid` | `UUniformGridPanel` of pips inside the card; Pale=standing, Dark=fallen; cols=files; pip→file compaction at scale | Health Bar (re-imagined) |
| `W_StanceChip` | inactive=Steel outline/Bone; active=Pale fill/Dark | Badge / Toggle |
| `W_StanceWheel` | 4 spokes Follow/Charge/Hold/Rally; current=Pale; leash-warn=Pale hub pulse | Menubar / radial |
| `W_LeashGauge` | fill=Bone, track=Dark/Steel, 80% break=Pale tick | Progress |
| `W_Medallion` | hero portrait via `T_UI_MedallionFrame` + portrait-register bust (do NOT invent a 2nd portrait style) | Avatar |
| `W_InputHint` | Bone glyph on Dark, controller-first | Kbd |

### 2b. New for the HUD — the subcamera cams

The combat-HUD mockup adds two live **subcamera feeds** bookending the command band
(Hero Cam flush-left, Unit Cam flush-right; both squared, Unit Cam boxier). These are new
vs. the menu spec:

- `WBP_HeroCam`, `WBP_UnitCam` — a framed `UImage` whose brush is a **RenderTarget2D**
  (`RT_HeroCam` / `RT_UnitCam`), overlaid with the cinematic chrome (corner brackets,
  focus reticle, scanline, live tag, flame-lit rim) as child widgets on top of the RT.
- Capture rig — a `SceneCaptureComponent2D` per cam: one rigged to a socket on the hero,
  one to the **selected squad's representative** (§5a: never a per-soldier loop — pick a
  squad-designated proxy actor).
- **Cam frame ≠ portrait medallion.** The menu uses the static `W_Medallion`; the HUD uses
  the live cam. Keep both; they are different registers (menu = embroidered record, HUD =
  live feed).

### 2c. Screens & flow

`WBP_MainMenu` (full-dress Sampler Frame) → `WBP_Muster` (roster in the well) →
`WBP_CombatHud` (the §8 collapse: guardians/lintel drop to stitched rails, well content
docks to the bottom command band). CommonUI activatable-widget stack drives the
transitions. The muster **band is static/compressed** (fixed footprint, no scroll) so the
cam bookends never shift — matches the current mockup.

---

## 3. Aesthetic reconciliation — 8bitcn × Demichrome (owner to confirm)

The tension the owner is resolving: 8bitcn ships a bright, NES-ish, "Press Start 2P" look;
our locked direction is the strict 4-value Demichrome sampler. The menu spec already
committed the resolution — **"each component derives from an 8bitcn pattern, re-skinned to
Demichrome — its paint is cut"** (§3). Concretely:

- **Adopt from 8bitcn:** the *component inventory and interaction patterns* (stepped/pixel
  borders, chunky focus states, the badge/toggle/card/progress vocabulary), and its
  *art assets* (pixel font, border sprites, icon set) as raw material.
- **Cut from 8bitcn:** its palette. Every converted asset is quantized to the four
  Demichrome hexes; the value-role map from menu spec §4 governs (Dark=ground/ink,
  Steel=borders/inactive, Bone=surfaces/body text, Pale=focus/healthy/the single glint).
  **Red stays absent** from menu/HUD (it's reserved for cost/decision-event rubrication).
- **Open aesthetic decisions for the owner:** (a) which pixel font — 8bitcn's default vs. a
  Demichrome-friendlier face vs. our own; (b) how stepped/chunky the borders read against
  the woodcut Sampler Frame (8bitcn's hard-step corner vs. the beast-lintel plate); (c)
  whether 8bitcn's icon language coexists with the "items = 1-bit glyph" rule from
  aesthetic-direction §1f. These are owner calls, flagged not decided.

---

## 4. The 8bitcn → UE asset-conversion pipeline (skill/agent)

8bitcn is open-source (shadcn-style React + Tailwind); most of its "8-bit" look is CSS
(stepped box-shadow borders) plus a pixel font and icon set. "Converting art assets to UE"
means turning the reusable *visual* pieces into UE-ready assets with correct import
settings. Proposed repeatable tool — **build as a skill or subagent** (owner: "we may make
a conversion skill or agent for it"):

**Name (proposed):** `8bitcn-to-ue`.

**Inputs → outputs:**
| 8bitcn source | Conversion | UE asset |
|---|---|---|
| Pixel font (TTF) | Import, or offline-rasterize to a bitmap font for crisp integer sizes | UE `Font` asset (`F_UIFont`) |
| Stepped/pixel border (CSS) | Author the stepped corner as a small PNG, palette-map to Demichrome, define 9-slice margins | `T_UI_Frame_9slice` + Slate border brush |
| Icon set (SVG / lucide-pixel) | Rasterize at target px → quantize to 4 values → pack | UI icon `Texture2D` / atlas |
| Component styling (colors, radii, states) | Translate to Slate style tokens | entries in `DA_UIStyle` |

**Hard rules the tool must enforce (from aesthetic-direction + menu spec §7):**
- Palette-quantize every raster to the **four Demichrome hexes only** — no fifth value.
- Import settings: **Nearest filter, NoMipmaps, UserInterface2D** (mask textures: no sRGB).
- Author at integer pixel sizes; never let UE resample a UI texture at non-integer zoom.
- Emit the asset **plus** its `DA_UIStyle`/brush wiring so a converted asset is drop-in.

**Behavior porting is manual.** The tool converts *art*; the *widget behavior/layout* is
hand-built UMG (§2). Don't try to auto-generate widgets from React.

---

## 5. Milestones

**M1 — Full-flow prototype (this plan's scope), mock-data driven:**
1. Confirm CommonUI; scaffold `Content/UI/` + `DA_UIStyle` with the 4 Demichrome tokens.
2. Stand up the `8bitcn-to-ue` skill; convert first assets (font + one 9-slice border + a
   handful of icons) as the vertical slice of the pipeline.
3. Build shared widgets (§2a) against mock data.
4. Build the three screens + CommonUI flow (§2c); wire the static/compressed muster.
5. Add the Hero/Unit subcamera cams (§2b) with placeholder capture targets.
6. Controller-first focus pass: stance = one-press radial, instant; A-confirm/B-back.

**M2 — Live bindings:** replace mock structs with real state. **Grep
`ELVTR/Source/ELVTR/` first** — `SwarmSubsystem` holds retinue count / stance / leash /
hero vigor (menu spec §5 says these *exist*, verify field names). The **squad-health
rollup** and **"company critical" threshold** are open → gameplay-director. Bind
`W_SquadCard`/`W_MusterGrid` to **squad-level state only**, never per-soldier actors
(§5a contract).

**M3 — Art + polish:** swap placeholder plates for the pixel-art-director's Sampler Frame
plate set (menu spec §9 briefs a–c); readability pass over the live swarm; motion +
reduced-motion; audio.

---

## 6. Open decisions / handoffs (carry into the new session)

- **CommonUI yes/no** — owner/eng call; blocks scaffold. (Recommend yes.)
- **Aesthetic** — §3 open items (font, border chunkiness, icon coexistence) — **owner**,
  against 8bitcn.
- **Subcamera cost** — two `SceneCaptureComponent2D` per frame over the Mass swarm isn't
  free → **performance-director**: capture resolution + update-rate (cap fps / capture only
  on selection change?), and whether the Unit Cam follows the selected squad or hovers.
- **Squad-health rollup + "company critical" threshold + squad-composition rules** →
  **gameplay-director** (menu spec §5/§5a).
- **HUD spec doc** — `docs/ui/combat-hud.md` is referenced by menu spec §8 but not yet
  written; the current HTML mockup is the de-facto spec. Formalize it (incl. the new cam
  bookends) during M1.

---

## 7. New-session bootstrap (do this first)

1. Read this file, then `docs/ui/menu-frame-system.md` (§3, §5a, §7) and
   `docs/art/aesthetic-direction.md` (Direction A / import discipline).
2. Open both mockup artifacts (URLs above) as the visual target.
3. Confirm the **CommonUI** decision with the owner.
4. `grep` `ELVTR/Source/ELVTR/` for `SwarmSubsystem` fields (retinue/stance/leash/vigor)
   to know what M2 will bind to — but M1 uses mock data.
5. Create `Content/UI/` + `DA_UIStyle`, then start the `8bitcn-to-ue` skill (§4) with the
   font + one border as the pipeline's first pass.

> **Live-Coding caution (project memory):** adding a `UPROPERTY` via Live Coding reports
> success then crashes the next PIE — class-layout changes need a full editor-closed
> rebuild. Plan C++ widget-base changes accordingly.
