# The Sampler Frame + the Muster — a menu summoned over a live run

**What this is:** the master menu-frame layout system for Kindled, and the first content
that fills its central well — an RTS-style company readout with health meters. **Reframed
by task-115 (owner, 2026-07-30): the game boots straight into play. There is no title
screen and this doc no longer describes one.** The Sampler Frame is chrome the player
*summons* over a live run — a pause overlay, not a launch gate.
**Extends:** `docs/art/aesthetic-direction.md` (frame/well concept, UI register),
`docs/ui/ui-aesthetic.md` (palette, type, state rules — **binding**, see §4),
`docs/art/portrait-register.md` (medallions), `GDD.md` §4 (stances),
`docs/RTS-VERTICAL-SLICE.md` §2/§5 (leash, minimal HUD).
**Mockup:** https://claude.ai/code/artifact/aafde786-5fa1-49c7-9d9a-0055903ba0ad — *the
frame summoned mid-Wave-2, not a launch screen; a scrim over a frozen battlefield sells
"paused," not "gone." Local copy: `docs/ui/mockups/menu-frame-system.html`.*

---

## 1. Intent

The owner's north-star reference — the Harry-Clarke woodcut border (`Artboard/Grounding
in the world/c680cee1…jpg`) — is canon-blessed for this exact use: `aesthetic-direction.md`
§2.3 rules it *unbuildable as sprites but buildable as frame / event-card art, quantized
to a colour system with coarse dither for the hatching.* So it becomes **The Sampler
Frame**: reusable menu chrome, summoned over gameplay rather than gating entry to it. Its
signature is a fixed border of made objects — a beast lintel, two flanking guardian
panels, corner rosette medallions — around **a central well that is the only part that
changes per screen.**

The well holds two things now, switched by a tab strip (§2, §3): **Muster** (the roster —
this doc's original and still-primary content, unchanged in substance) and **Settings**
(a new, small surface — options + Quit, see §3a). Neither is a step in a sequence; both
are destinations reached the same way, from the same summon.

The Muster content answers the owner's original ask: **RTS-style unit management in the
centre, with health meters.** For a *commander* game (not a micromanager game — GDD §4),
the retinue is read as **squads, not soldiers** — square Warcraft-format unit cards, where
each card's health *is* its **rank-and-file muster** (a grid of pips, one per soldier /
per file at scale, lit = standing, dark = fallen). This is also the sim architecture:
RTS-slice §5 commits to **aggregation — squad-as-entity, renders as N sprites** — so the
game commands groups, and the UI binds to those same squads. Count, formation, health and
scale become one read (see §6).

**Frame regions used by this screen:** all of them (full-dress) — the pause takes the
screen back from the battle on purpose, so there is no play-space budget to protect while
it's open (see §5). **Regions that collapse on the live-combat HUD variant:** the flanking
guardian figures drop to plain stitched rails; corner medallions shrink; the central well
becomes a thin bottom band. That derivation is `docs/ui/combat-hud.md` (§9 of this doc),
already implemented in code as `UKindledHud` — see §8.

## 2. Layout anatomy

Target **16:9** desktop; **16:10** Steam Deck collapse. This full-dress layout is what
appears **over** a paused run — see §3 for the summon/dismiss mechanic and §3a for what
lives under each tab.

```
┌─◉────────────── beast lintel ───────────────◉─┐   ◉ = corner rosette medallion
│                K I N D L E D   (crest)         │   header band  (halftone, static)
│            — the record keeps what you save —  │
│                [ MUSTER ]  Settings             │   tab strip — new, see §3
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
│ ☰ Resume  A Confirm  B Back  ▤ Wheel   [red]   │   footer input band
└───────────────────────────────────────────────┘
```

- **Left flank** = the commander: guardian figure plate, hero **medallion**
  (portrait-register), hero Vigor meter. The hero is read as a *person*; the company is
  read as *mass*.
- **Central well** = whichever tab is active — Muster shown above (§3a for Settings).
- **Right flank** = orders: the stance wheel (current stance lit) + the leash gauge with
  its 80% break warning. Present on the Muster tab; not meaningful on Settings, which
  drops the right flank to a plain stitched rail (§3a).

## 3. Summon and dismiss — the pause, not the boot

**Input, controller-first:** the Menu/Start button (`☰` in the footer hint) opens the
overlay from any run phase; the same button, or `B`/`Esc` on keyboard, dismisses it and
resumes exactly where the run stood. This is a toggle, not a stack of screens — there is
nothing "behind" the Muster tab to back out of except Settings, and switching tabs never
counts as a navigation step worth an input-hint entry of its own (a bumper cycles tabs,
same convention as squad-row cycling in §6).

**What happens to the run:** summoning **hard-pauses** the sim. Kindled is single-player
(`docs/narrative/FLAME-FOUNDATION.md` — one bearer, one army, one fire; co-op is a later
multiplier, not this system's concern) — there is no other player's clock to protect, so
there is no reason to leave the battle running under the frame. This is also why the frame
is allowed to take the **full screen**: the hard constraint that a live HUD must never
cost the battle its screen (§5) governs *rendering during combat*, and combat has stopped.
A paused frame isn't competing with anything for the player's eye.

**Phase behaviour** (`ERunPhase`, `Spike1GameMode.h`) — the summon must say what it does
in each:

| Phase | Behaviour |
|---|---|
| `Deploying` | Opens the same as any other phase. Nothing is at stake yet, but there's no reason to special-case it — the pause is free. |
| `WaveActive` | The primary case, and the one the mockup shows. Sim ticks and hero/unit input stop; the battle freezes exactly where it stood; dismiss resumes mid-swing. |
| `Breather` | Same hard pause, **and it also freezes `PhaseTimer`.** Reviewing squads must never burn down the breather clock and force the next wave to start while the player is still in Muster. |
| `Won` / `Lost` | The summon still opens (Quit and roster review are legitimate here), but it is **not** how the player reads results — that's task-116's end-of-wave board, shown automatically as its own screen, not reached through this summon. This doc doesn't own that hand-off; flagged so the two systems aren't confused for one another. |

**First launch is not a different screen.** Because there's no title screen to teach the
summon exists, the game shows the `☰ Resume` footer hint (already present on every open of
the overlay) as a **one-shot toast** during the first `Deploying` beat — the same glyph,
surfaced once before the player has had a reason to look for it, then never shown again.
This is a toast, not a modal: it doesn't pause anything and it doesn't block the first
wave from starting on schedule. Every other launch is identical to every other summon —
there is no other first-run difference.

### 3a. What's under each tab

The menu is for exactly three things now that it doesn't start the game: **reviewing the
roster** (Muster — squads, stances, leash, hero Vigor; unchanged from this doc's original
content, §4 of the table below), **options** (Settings — audio/video/controls, whatever
set the owner wants; not specified further here, out of this doc's scope), and **quitting**
(a Settings action, not its own tab). Two tabs cover that whole set:

- **Muster** (default tab, opens first every time) — the layout in §2.
- **Settings** — the same Sampler Frame chrome, but the well becomes a plain vertical list
  (audio / video / controls / **Quit**) and the right-flank stance wheel + leash gauge drop
  to a plain stitched rail, since neither means anything outside a squad context. The left
  flank (hero medallion) may stay or drop to a rail too — a small owner call, not decided
  here, since it doesn't affect anything load-bearing.

Settings' own content (what options exist, Quit's confirm-or-not) is not specced by this
doc — it's flagged as new scope, not designed, because nothing about it is a palette,
layout, or frame-system question yet.

## 4. Component inventory

Each derives from an 8bitcn pattern, **re-skinned per `ui-aesthetic.md` §1 — its paint is
cut, not its shape.** All states expressed as value-roles (Ink/Iron/Parchment/Ember, not
the old Dark/Steel/Bone/Pale names — see §5).

| Component | 8bitcn origin | States (value-role) |
|---|---|---|
| **Sampler Frame** (`WBP_SamplerFrame`) | Card / Sidebar chrome | Static furniture. 9-slice frame plate; Iron border; Ink ground; halftone bands. |
| **Menu tab** (`W_MenuTab`) — new, §3a | Tabs (re-derived from Badge/Toggle, same pattern as the stance chip — no new component family) | inactive = Iron outline / Iron-dim text; **active = Ember fill / Ink text**, same "this is in effect" read as an active stance chip. |
| **Company meter** | Health Bar | full = **Ember** pip; lost = **Iron** pip; empty = **Ink** pip w/ Iron outline. No hue beyond Ember. |
| **Square squad card** (`USquadCard`, built) | Item slot + Card | default = Iron border; **focus/selected = 2px Ember border** (width change, not colour-only — `ui-aesthetic.md` §4); routed-out squad = collapse to Iron, greyed size. Wide variant (`span 2`) for compacted 50-strong squads. |
| **Rank-and-file muster** (`UMusterGrid`, built) | Health Bar (re-imagined) | the card's health *and* formation: a grid of pips, **Ember = standing soldier/file, Ink = fallen**. Cols = files. Replaces a linear bar — count, formation, health, and scale in one control. |
| **Stance chip** | Badge / Toggle | inactive = Iron outline / Parchment text; **active = Ember fill / Ink text.** |
| **Hero medallion** | Avatar | **reuse `T_UI_MedallionFrame` + portrait-register bust verbatim** — do not invent a second portrait style. |
| **Stance wheel** (`W_StanceWheel`) | Menubar / radial | 4 spokes Follow/Charge/Hold/Rally; current = Ember; leash-warning = Ember pulse on the hub. |
| **Leash gauge** | Progress | fill = Parchment; track = Ink/Iron; **80% break marker = Ember tick** (warns before a Hold unit breaks — RTS-slice §2). |
| **Input hint** | Kbd | Parchment glyph on Ink; controller-first glyphs. |

## 5. Palette & texture

**Superseded — see `docs/ui/ui-aesthetic.md` §1, §2, §4 for the binding system.** This
section previously specified "strictly the four locked values" (Dark/Steel/Bone/Pale);
that lock is superseded (owner, 2026-07-28 — the game ships in full colour). The shape
survives exactly — a small closed set of named roles, one warm colour reserved for what
matters, red held out entirely — only the values and names changed: **Ink / Iron /
Parchment / Ember**, plus reserved **Rubrication**. Every rule in §4 above already uses
the current names. Do not reintroduce Dark/Steel/Bone/Pale anywhere in new work — they are
stale token names, not the current system.

**Red is absent from this screen**, confirmed in the footer legend, both tabs — a pause
menu is not a cost moment. Rubrication enters only on decision-event cards (separate
spec) where the game asks the player to *pay*.

## 6. Data bindings

Grep `ELVTR/Source/ELVTR/` before wiring — the swarm subsystem holds the real state.

| Element | Binding | Status |
|---|---|---|
| Run phase (gates pause behaviour, §3) | `ERunPhase` (`Spike1GameMode.h`) — `Deploying / WaveActive / Breather / Won / Lost` | **exists**; no UI-side mirror yet, needed to drive the phase table in §3 |
| Company standing / % | retinue count vs. per-floor cap (`SwarmSubsystem`) | **exists** (verify field names) |
| Per-group count + HP | aggregate per unit-type; needs a **group-health rollup** | **partial → gameplay-director** (is health per-entity or per-squad-aggregate? RTS-slice §5 wants aggregate) |
| Current stance | stance enum, `Follow/Charge/Hold/Rally` (GDD §4) | **exists** |
| Leash % + warning | unit distance vs. `LeashRadius`, warn at 80% (`LeashHysteresis`) | **exists** (RTS-slice §2 tunables) |
| Hero Vigor | hero pawn health | **exists** |
| First-launch toast flag (§3) | a single persisted bool ("has the player ever seen the summon hint") | **doesn't exist** — smallest possible new state, a `SaveGame`/`GameInstance` flag, not a new system |

**Threshold question → gameplay-director:** what counts as "company critical" (the point
the meter should read as alarm)? The UI reserves a state for it; the number is a balance
call, not mine.

## 6a. Squads & scale compaction (the load-bearing idea)

The retinue is never a bag of N individual soldiers to the UI or the sim — it is a small
set of **squads**, and squads are the unit of both logic and display. This is the owner's
directive ("units become part of a squad rather than micromanaging EVERY unit") and it is
already the committed sim architecture (RTS-slice §5: *aggregation — squad-as-entity,
renders as N sprites*; GDD §10 entity architecture).

- **A squad has a size and a standing count.** The card shows `standing / size` (e.g.
  `38/50`); the muster grid shows one pip per soldier, Ember when standing.
- **Compaction scales the pip, not the card.** At small scale one pip = one soldier. As
  squads grow (20 → 50 → larger), a pip promotes to represent **a file** (a column of the
  rank), so a 50-strong squad reads as a 10×5 block and a 200-strong mass reads as, say,
  20×10 — the card stays a fixed, glanceable object while the thing it represents scales.
  The mockup's wide Spearmen card shows the 50/10-file case.
- **The UI binds to the squad entity, not its members.** `W_SquadCard` reads squad-level
  state (size, standing, stance, formation); it must **never** iterate live per-soldier
  actors. This is what keeps the HUD cheap at horde scale and is the same promise the sim
  makes.
- **Selection & orders operate on squads.** Cycle/select a card = select a squad; the
  stance wheel issues to the selected squad(s) or the whole company. No soldier-level
  selection exists — by design.

This section is a **shared contract with the sim**: the squad is the entity both sides
address. It is a §9 handoff to the gameplay- and performance-directors, not a UI-only
convention.

## 7. Interaction & input

**Controller-first (Steam Deck is the target).** Summon/dismiss: `☰`/Start toggles the
overlay from any phase (§3); `B`/`Esc` also dismisses. Tab switch: a bumper cycles
Muster ↔ Settings (mirrors the squad-row cycle convention below — one input class for
"cycle a small closed set," never a menu of its own). Within Muster, focus order: company
card → group rows (`↹` cycles) → stance wheel. Stance is a **one-press radial** (`▤` /
bumper), instant mid-combat when the overlay is closed — it must never be gated behind
this menu; the wheel shown here is the *indicator*, the wheel-open is a separate live-HUD
overlay (§8). A-confirm / B-back. Mouse & keyboard are second-class but supported (row
focus-within, click). Motion: transitions are stitch-wipe/ink-bleed *in spirit*, snappy in
practice; a **baked Ember flicker** on glints only (portrait-register §3), disabled under
`prefers-reduced-motion`. **Nothing animates on the path between pressing a stance and the
order landing**, and nothing animates on the path between pressing summon and the pause
landing — the toggle must feel immediate both ways.

## 8. UMG translation

The UI is further along than this doc's prior draft assumed — real UMG C++ already exists
under `ELVTR/Source/ELVTR/UI/`: `UKindledHud`, `UMusterPanel`, `USquadCard`,
`UMusterGrid`, `UStitchMeter`, all `UKindledWidget`-based (a `UCommonUserWidget` subclass).
This section now points at what's built rather than describing it as future work.

- `WBP_SamplerFrame` — **not yet built.** A reusable UserWidget: `UImage` 9-slice frame
  plates in named slots (lintel, L/R flank, corners, footer), a `UNamedSlot` "Well" that
  either tab fills, and the new tab strip (§3a) as a header child. This is the one asset
  that makes "pause menu = well + frame, summoned not booted" real; the live-HUD collapse
  (§9) already stands on its own without it, so this is scoped work, not a prerequisite
  for anything already shipped.
- Meters — `UStitchMeter` (built). `Full` segments render Ember, empty segments render
  Iron-toned. No structural change from this doc.
- Squad cards — `USquadCard` (built). `Refresh()`'s border-colour swap should read
  `bSelected ? Ember : Iron`; per `ui-aesthetic.md` §4 the selected state should also grow
  the border from 1px to 2px (`Frame->SetPadding` is already the lever for this).
- Muster grid — `UMusterGrid` (built), a `UUniformGridPanel` of pips, squad-level state
  only (§6a) — never a per-soldier loop.
- Stance wheel — custom `W_StanceWheel` (not yet built as its own asset outside the HUD
  band) — Slate `SRadialBox`-style or 4 anchored buttons; the in-combat open is the
  separate HUD overlay widget already described by `combat-hud.md`/§9, not this indicator.
- Menu tab strip — `W_MenuTab` (new, small): two `UCheckBox`-as-toggle buttons styled per
  §4, driving which named-slot content the Well shows. Trivial enough not to need a
  dedicated tab-manager widget class.
- Pause wiring — **not yet built.** An input action (`IA_SummonMenu`) bound at the
  `PlayerController` level, toggling `WBP_SamplerFrame` visibility and calling
  `SetGamePaused(true/false)` on the world — the smallest possible implementation; no
  CommonUI activatable-widget stack is required just to toggle one overlay (see
  `UI-PROTOTYPE-PLAN.md` §2 for the fuller CommonUI recommendation, which still applies to
  focus routing inside the overlay even though the *toggle* itself doesn't need it).
- Medallion — `UImage` with the portrait material (RGB→Emissive, A→mask), UV-offset for
  flicker frame; **no per-portrait material work**.
- Import discipline for all frame/medallion/meter art: **Nearest, NoMipmaps,
  UserInterface2D** (`SETUP-EDITOR.md`).

## 9. The live-combat HUD (already built, not this doc's layout)

The full-dress frame in §2 is the **paused overlay only.** In live combat the well content
survives (the same `W_SquadCard`/`W_MusterGrid`/`UStitchMeter` widgets) but the frame
yields the screen to the battle: guardians and lintel drop, corner medallions shrink, the
company meter + stance indicator dock to a thin bottom band. This is **already
implemented** — `UKindledHud::Setup(bool bWithCams)` and `RebuildBand()` build exactly
this collapse today, live and mock-fed. This doc doesn't re-derive that layout; it exists
in code ahead of its own written spec (`docs/ui/combat-hud.md` is still unwritten — the
running `UKindledHud` implementation is the de-facto spec until someone formalizes it).

**First-look mockup, over real gameplay:**
https://claude.ai/code/artifact/4eb494b3-c7df-47c0-9231-555465949675 — the HUD composited
onto the current build's actual capture, with the HUD state matched to the screenshot.
Still useful as a placement reference; predates this doc's palette/tab changes.

## 10. Handoffs

**Art briefs raised** (to pixel-art-director — write as `brief-<id>-*.md` when actioned):
(a) **Sampler Frame plate set** — beast lintel, L/R guardian panels, corner rosettes,
footer band, a tab-strip treatment (§3a), quantized per `ui-aesthetic.md`'s role system
with coarse dither for the woodcut hatching; (b) **blackletter crest treatment** for the
title ("KINDLED," the "Old Master" illuminated register, `05aaaecf…`); (c) stance-wheel +
meter end-cap icons. The mockup's dragon and guardians are placeholders standing in for
(a).

**Gameplay-director:** the squad-health rollup binding and the "company critical"
threshold (§6); the squad-composition rules (how many soldiers per squad, when a new
squad forms vs. an existing one refills — §6a); whether `Won`/`Lost` should suppress the
summon or let it coexist with task-116's results board (§3, phase table).

**Performance-director:** confirm the **squad-as-entity** boundary (§6a) so the HUD binds
to squad state, not per-soldier actors — the pip→file compaction rides on the same
aggregation you own (RTS-slice §5). Flag the entity-count at which a pip must stop meaning
one soldier and start meaning a file.

**Engineering (no owner named yet):** the pause/summon input wiring (§8) and the
first-launch toast flag (§6) are both small, real, unclaimed work.

## Readability check

*(to pixel-art-director — the live-HUD bar, not a brief)* Unchanged from the prior draft
of this doc, since the live-combat HUD (§9) is untouched by this task: I assumed a single
thin bottom band, Ember used only for the meter fill and the active stance, no tint or
bloom over the field, and fodder never getting enemy bars (only elites/boss, RTS-slice
§5). **Please confirm this holds at 500 moving units** — specifically that an Ember meter
fill at screen-bottom doesn't compete with the Lampbearer's honest-light glint or the
Pathfinder's quarry mark for the eye's "brightest thing" read. Not re-asking this for the
paused overlay (§2) — the battle is frozen while it's open, so there's no live-readability
question there, only the frozen-scrim treatment shown in the mockup.

## Canon proposals

None. This doc operates entirely inside decisions already made by the owner (2026-07-28
colour return, 2026-07-30 boot-into-play). It proposes nothing new to canon.
