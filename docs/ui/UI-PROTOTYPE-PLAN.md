# UI Prototype Plan — Kindled UMG build (HUD + pause menu)

**What this is:** the implementation plan for the first real, interactive Kindled UI —
built **natively in Unreal UMG**. **Re-cut by task-115 (owner, 2026-07-30): the game boots
straight into play, no title screen.** The plan below is no longer a sequential launch
flow (main menu → muster → combat HUD) — it's two things that already coexist from the
first frame: the **live combat HUD** (mostly built) and a **pause overlay** summoned over
it (mostly not built). This is the hand-off doc: a fresh session should be able to start
implementation from here without re-deriving context.

**Decided with owner:**
- **2026-07-23 — Platform = UMG-native** (option 3). No web/React prototype. 8bitcn is
  used as an *aesthetic + component reference and an art-asset source*, not as a runtime.
- **2026-07-28 — Colour returns.** The 4-value Demichrome lock is superseded; the game
  ships in full colour by default (`Kindled.Quantize 0`). Palette content in this doc is
  governed by `docs/ui/ui-aesthetic.md`, not restated here.
- **2026-07-30 — Boot straight into play.** No menu gate at launch. The scope below is
  re-ordered around that: build/finish the live HUD first (it's the thing running from
  frame one), the pause overlay second.
- **8bitcn assets get converted to UE.** Some art featured in 8bitcn (pixel font,
  stepped/9-slice borders, icons) will be pulled and converted to UE-compatible assets via
  a **conversion skill or agent** (§4).

**Reads / extends (source of truth — do not restate their canon, bind to it):**
- `docs/ui/ui-aesthetic.md` — **binding palette/type/state system** (Ink/Iron/Parchment/
  Ember + reserved Rubrication). Supersedes this doc's own palette content wherever they
  used to disagree.
- `docs/ui/menu-frame-system.md` — The Sampler Frame + Muster spec, now framed as a
  **pause overlay, not a launch screen**. §4 component inventory and §8 UMG translation
  are the spine of this plan.
- `docs/art/aesthetic-direction.md` — frame/well concept, chibi combat register, import
  discipline **Nearest / NoMipmaps / UserInterface2D**. (Its 4-value palette lock is
  superseded — see `ui-aesthetic.md`.)
- `docs/narrative/FLAME-FOUNDATION.md` — narrative canon (flame-bearer premise,
  single-player first).
- `ELVTR/Source/ELVTR/Spike/Spike1GameMode.h` — the real run structure (`ERunPhase`:
  `Deploying / WaveActive / Breather / Won / Lost`, three waves) the pause overlay has to
  behave correctly across.
- Mockups (the visual targets): pause overlay
  https://claude.ai/code/artifact/aafde786-5fa1-49c7-9d9a-0055903ba0ad · combined
  aesthetic preview https://claude.ai/code/artifact/bc2f4bc0-e1a6-4959-b010-cfdd7fa4b13e ·
  combat HUD (with Hero/Unit subcamera bookends)
  https://claude.ai/code/artifact/4eb494b3-c7df-47c0-9231-555465949675 — local:
  `docs/ui/mockups/menu-frame-system.html`, `docs/ui/mockups/ui-aesthetic-menu-and-wave.html`,
  `docs/ui/mockups/combat-hud.html`.

---

## 1. Goal & non-goals

**Goal (milestone 1):** a playable-feeling, controller-first UMG prototype of the whole
loop, driven by **mock state**, that answers the design questions the mockups can't: does
the live-HUD frame collapse feel right over a moving field, does stance switching land
instantly, does the pause overlay open/close without cost, does the static muster read at
a glance, do the Hero/Unit subcamera cams add or distract.

**Non-goals for milestone 1:**
- Not wired to live gameplay yet — mock data structs only (real bindings are milestone 2,
  §6).
- Not final art — placeholder plates + converted 8bitcn assets stand in for the
  pixel-art-director's bespoke Sampler Frame plate set.
- Not the shipping polish pass (animation, audio, reduced-motion variants come after the
  layout/flow is confirmed).

---

## 2. UMG architecture

**Plugin decision (confirm first thing) — recommend CommonUI, but flag that the case for
it is smaller now.** The original rationale was a controller-first Steam Deck **screen
sequence** (title → muster → HUD) needing CommonUI's activatable-widget stack and gamepad
focus routing. That sequence no longer exists — there is exactly one always-on screen (the
combat HUD) and one toggleable overlay (the pause menu), not a multi-screen stack. What
CommonUI still buys: gamepad focus routing *within* the pause overlay (tab strip → card
grid → stance wheel, menu-frame-system.md §7) and input-action mapping shared with the
live HUD's own controller input, which is real, ongoing value regardless of screen count.
What it no longer needs to justify: a screen-stack manager, since the overlay is a single
toggle, not a sequence with a back-stack. **Recommendation: still yes, for the focus
routing and input-action layer — but scope it as "one screen + one overlay," not the
three-screen flow the original plan assumed, and it's fair for the owner to ask whether a
single toggle needs a screen-stack framework at all.** Owner/engineering call either way;
plain UMG + a bound visibility toggle is a real fallback that costs less now than it would
have under the old flow.

**Content location:** `ELVTR/Content/UI/` (does not exist yet — create it). Suggested
tree:
```
Content/UI/
  Style/        DA_UIStyle (Ink/Iron/Parchment/Ember tokens), F_UIFont (converted 8bitcn
                font), T_UI_Frame_9slice, T_UI_* brush textures, converted icon atlas
  Common/       shared widgets (built once for the HUD AND the pause overlay)
  Screens/      WBP_CombatHud (always on), WBP_SamplerFrame (the pause overlay)
  RT/           RT_HeroCam, RT_UnitCam (render targets) + capture rig BP
```

### 2a. Shared widgets — build once, used by both the HUD and the overlay

These come straight from `menu-frame-system.md` §4/§8. The combat HUD is the live
collapse of the same well content the pause overlay shows full-dress, so **every widget
below serves both** — do not fork. Status reflects what's actually in
`ELVTR/Source/ELVTR/UI/` today, not the original plan's assumption that this was all
future work.

| Widget | Role | Status |
|---|---|---|
| `UKindledHud` | Live combat-HUD band, splits into wings around the Unit Cam | **Built** (`KindledHud.h/.cpp`) |
| `UMusterPanel` | Company meter + squad cards, feeds both the HUD wings and the pause overlay's well | **Built** (`MusterPanel.h/.cpp`) |
| `USquadCard` | One square card bound to **one squad entity**; default Iron border, selected → Ember + 2px | **Built** (`SquadCard.h/.cpp`) |
| `UMusterGrid` | `UUniformGridPanel` of pips inside the card; Ember=standing, Ink=fallen; cols=files | **Built** (`MusterGrid.h/.cpp`) |
| `UStitchMeter` | segmented meter (`UImage` pips, not `UProgressBar`); `FullSegments`/`Total` | **Built** (`StitchMeter.h/.cpp`) |
| `UKindledWidget` | Shared `UCommonUserWidget` base for everything above | **Built** (`KindledWidget.h/.cpp`) |
| `WBP_SamplerFrame` | 9-slice pause-overlay chrome (lintel, L/R flanks, corners, footer) + `NamedSlot` "Well" + the Muster/Settings tab strip | **Not built** — the actual scoped work for this milestone |
| `W_MenuTab` | Muster/Settings toggle (Badge/Toggle pattern, no new component family) | **Not built**, small |
| `W_StanceWheel` | 4 spokes Follow/Charge/Hold/Rally; current=Ember; leash-warn=Ember hub pulse | **Not built** as a standalone asset |
| `W_LeashGauge` | fill=Parchment, track=Ink/Iron, 80% break=Ember tick | **Not built** as a standalone asset |
| `W_Medallion` | hero portrait via `T_UI_MedallionFrame` + portrait-register bust | **Not built** |
| `W_InputHint` | Parchment glyph on Ink, controller-first; also carries the one-shot first-launch toast (menu spec §3) | **Not built** |

### 2b. Subcamera cams (live HUD only)

The combat-HUD mockup adds two live **subcamera feeds** bookending the command band (Hero
Cam flush-left, Unit Cam flush-right; both squared, Unit Cam boxier). No palette content,
unaffected by this task's changes:

- `WBP_HeroCam`, `WBP_UnitCam` — a framed `UImage` whose brush is a **RenderTarget2D**
  (`RT_HeroCam` / `RT_UnitCam`), overlaid with cinematic chrome (corner brackets, focus
  reticle, scanline, live tag, flame-lit rim) as child widgets on top of the RT.
- Capture rig — a `SceneCaptureComponent2D` per cam: one rigged to a socket on the hero,
  one to the **selected squad's representative** (menu spec §6a: never a per-soldier loop
  — pick a squad-designated proxy actor). `UUnitCamProjector`/`UnitCamDirector` already
  exist in code (`ELVTR/Source/ELVTR/UI/`) — verify against this section before treating
  the cams as unbuilt.
- **Cam frame ≠ portrait medallion.** The pause overlay uses the static `W_Medallion`; the
  HUD uses the live cam. Keep both; they are different registers (overlay = embroidered
  record, HUD = live feed).

### 2c. Screens & flow

There is no pre-play sequence. The game boots directly into `ASpike1GameMode`'s
`Deploying` phase with `WBP_CombatHud` already live. `WBP_SamplerFrame` (the pause
overlay, Muster tab default, Settings tab for options + Quit) is summoned over it by an
input action from any `ERunPhase`, hard-pausing the sim, and dismissed back to exactly
where the run stood — see `menu-frame-system.md` §3 for the full phase-by-phase
behaviour table. There is no `WBP_MainMenu` reached at boot; "main menu" content (settings
+ quit) is the overlay's Settings tab, reachable only through the same summon as Muster.
The HUD band is **static/compressed** (fixed footprint, no scroll) so the cam bookends
never shift, whether or not the overlay is open above them — matches the current mockups.

---

## 3. Palette, type, and 8bitcn — see `ui-aesthetic.md`

This section previously reconciled 8bitcn's palette against a locked 4-value Demichrome
ramp and flagged font/border/icon questions as open for the owner. **That reconciliation
is done — `docs/ui/ui-aesthetic.md` is the binding answer.** What survives from the
original premise, unchanged:

- **Adopt from 8bitcn:** the *component inventory and interaction patterns* (stepped/
  pixel borders, chunky focus states, the badge/toggle/card/progress vocabulary), and its
  *art assets* (pixel font, border sprites, icon set) as raw material — this is a pattern
  and asset source, not a paint source (see CLAUDE.md-level project instructions on
  8bitcn's role, unchanged by any of this).
- **Cut from 8bitcn:** its palette, in full. Every converted asset is quantized to
  `ui-aesthetic.md`'s role system (Ink/Iron/Parchment/Ember), not four flat hexes — see
  that doc §1 for why full colour still keeps a small closed vocabulary instead of opening
  up. **Red stays absent** from menu/HUD (reserved for cost/decision-event rubrication).
- **Still open for the owner**, narrowed but not resolved by `ui-aesthetic.md` §3: which
  pixel font (the one hard requirement now is tabular/lining figures, non-negotiable per
  `ui-aesthetic.md` §3 and §5 — a proportional-digit face is disqualified outright); how
  stepped/chunky the borders read against the woodcut Sampler Frame; whether 8bitcn's icon
  language coexists with the "items = 1-bit glyph" rule.

---

## 4. The 8bitcn → UE asset-conversion pipeline (skill/agent)

8bitcn is open-source (shadcn-style React + Tailwind); most of its "8-bit" look is CSS
(stepped box-shadow borders) plus a pixel font and icon set. "Converting art assets to UE"
means turning the reusable *visual* pieces into UE-ready assets with correct import
settings. Proposed repeatable tool — **build as a skill or subagent**:

**Name (proposed):** `8bitcn-to-ue`.

**Inputs → outputs:**
| 8bitcn source | Conversion | UE asset |
|---|---|---|
| Pixel font (TTF) | Import, or offline-rasterize to a bitmap font for crisp integer sizes | UE `Font` asset (`F_UIFont`) |
| Stepped/pixel border (CSS) | Author the stepped corner as a small PNG, palette-map to the role system, define 9-slice margins | `T_UI_Frame_9slice` + Slate border brush |
| Icon set (SVG / lucide-pixel) | Rasterize at target px → quantize to the role system → pack | UI icon `Texture2D` / atlas |
| Component styling (colors, radii, states) | Translate to Slate style tokens | entries in `DA_UIStyle` |

**Hard rules the tool must enforce (from `ui-aesthetic.md` + menu spec §4):**
- Palette-quantize every raster to the **role system in `ui-aesthetic.md` §1** — Ink /
  Iron / Parchment / Ember, plus the reserved (never-used-by-UI) Rubrication. Not the old
  four flat Demichrome hexes.
- Import settings: **Nearest filter, NoMipmaps, UserInterface2D** (mask textures: no
  sRGB).
- Author at integer pixel sizes; never let UE resample a UI texture at non-integer zoom.
- Emit the asset **plus** its `DA_UIStyle`/brush wiring so a converted asset is drop-in.

**Behavior porting is manual.** The tool converts *art*; the *widget behavior/layout* is
hand-built UMG (§2). Don't try to auto-generate widgets from React.

---

## 5. Milestones

**Re-cut honestly against what's already built** (see §2a). "Build the main menu first"
is no longer the first step of anything — there is no main menu to gate on, and the live
HUD side of M1 is largely done already.

**M1a — Live combat HUD (mostly done):**
1. ~~Scaffold `Content/UI/` + `DA_UIStyle`~~ — done in code terms (`UKindledWidget` and
   siblings exist); the `DA_UIStyle` data asset itself and the Ink/Iron/Parchment/Ember
   token names still need creating/renaming (`ui-aesthetic.md` §9).
2. `UKindledHud`/`UMusterPanel`/`USquadCard`/`UMusterGrid`/`UStitchMeter` — **built**,
   mock-fed (`UseMockData()`, `MakeMockSquads`).
3. Hero/Unit subcamera cams (§2b) — capture rig exists (`UnitCamProjector`,
   `UnitCamDirector`); verify against the current mockup for remaining gaps.
4. Controller-first focus pass on the live HUD: stance = one-press radial, instant;
   confirm it never routes through any menu (menu spec §7).

**M1b — Pause overlay (the real remaining scope):**
1. `IA_SummonMenu` input action; toggling `WBP_SamplerFrame` visibility +
   `SetGamePaused`, wired at the `PlayerController` (menu spec §8).
2. Build `WBP_SamplerFrame` — 9-slice frame plates, the Muster/Settings tab strip
   (`W_MenuTab`), reusing `UMusterPanel`/`USquadCard`/`UMusterGrid`/`UStitchMeter`
   unchanged in the Muster tab's well.
3. `W_StanceWheel`, `W_LeashGauge`, `W_Medallion` as standalone assets (currently folded
   into the HUD band inline, not separable widgets yet).
4. First-launch one-shot toast (menu spec §3, §6) — needs the small persisted flag.
5. Settings tab content (audio/video/controls/Quit) — not designed yet, flagged as new
   scope for whoever picks this up.
6. Phase-behaviour pass: confirm pause/dismiss across all five `ERunPhase` values per
   menu spec §3's table, especially that `Breather`'s `PhaseTimer` actually freezes.

---

## 6. Open decisions / handoffs (carry into the new session)

- **CommonUI yes/no, and at what scope** — §2's recommendation stands (yes, for focus
  routing + input mapping) but the screen-stack question genuinely shrank with the
  boot-into-play decision; owner/eng call.
- **Font, border chunkiness, icon coexistence** (§3) — narrowed by `ui-aesthetic.md` but
  not picked. Owner, against 8bitcn.
- **Subcamera cost** — two `SceneCaptureComponent2D` per frame over the Mass swarm isn't
  free → **performance-director**: capture resolution + update-rate (cap fps / capture
  only on selection change?), and whether the Unit Cam follows the selected squad or
  hovers.
- **Squad-health rollup + "company critical" threshold + squad-composition rules** →
  **gameplay-director** (menu spec §6/§6a).
- **Settings tab content** — not designed (§5, M1b step 5). Whoever picks up M1b should
  raise this rather than invent it silently.
- **HUD spec doc** — `docs/ui/combat-hud.md` is still referenced but not yet written; the
  running `UKindledHud` implementation is the de-facto spec (menu spec §9). Formalizing it
  is lower urgency now that the code exists and works, but it should still happen so a
  reader doesn't have to reverse-engineer the collapse from C++.

---

## 7. New-session bootstrap (do this first)

1. Read this file, then `docs/ui/ui-aesthetic.md` (palette/type, binding) and
   `docs/ui/menu-frame-system.md` (§3 summon/dismiss, §4 components, §6a squads, §8 UMG
   translation).
2. Open the mockup artifacts (URLs above) as the visual target — especially the pause
   overlay one, which is the actual new scope (§5, M1b).
3. Confirm the **CommonUI** scope with the owner (§2/§6 — smaller than originally scoped,
   still recommended for a narrower reason).
4. `grep` `ELVTR/Source/ELVTR/UI/` first — most of M1a already exists; don't rebuild it.
   Then `grep` for `SwarmSubsystem` fields (retinue/stance/leash/vigor) and
   `Spike1GameMode.h` for `ERunPhase` to know what M1b/M2 bind to.
5. Start on M1b (§5) — the pause overlay is the actual unbuilt milestone-1 work now.

> **Live-Coding caution (project memory):** adding a `UPROPERTY` via Live Coding reports
> success then crashes the next PIE — class-layout changes need a full editor-closed
> rebuild. Plan C++ widget-base changes accordingly.
