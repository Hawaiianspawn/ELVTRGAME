# UI Aesthetic — full colour

**Status:** current · **Owner:** UI/UX Director · **Task:** task-114
**Supersedes, on palette content only:** `docs/ui/UI-PROTOTYPE-PLAN.md` §3–4 (partial),
`docs/ui/menu-frame-system.md` §4 — see the supersession table (§6). Their layout,
component, and interaction content is not touched by this doc and is not superseded.
**Binds to:** `docs/art/aesthetic-direction.md` (AMENDMENT 2026-07-28 — colour is back,
`Kindled.Quantize 0` default), `docs/narrative/FLAME-FOUNDATION.md` (the flame premise),
`GDD.md` §4 (stances), `docs/RTS-VERTICAL-SLICE.md` §2/§5 (leash, minimal HUD),
`CLASSES.md` (retinue vocabulary), `docs/art/portrait-register.md` (medallions).
**Mockup:** https://claude.ai/code/artifact/bc2f4bc0-e1a6-4959-b010-cfdd7fa4b13e
(local: `docs/ui/mockups/ui-aesthetic-menu-and-wave.html`).

> **Naming:** the game is **Kindled**. Any "Emberkeep" surviving elsewhere in the UI
> docs is a discarded working title (task-092 purge) — not touched here, not reverted.
> **CVar:** `Kindled.Quantize` (renamed by task-093; `Kindled.PaletteSteps` likewise).

---

## 0. What changed and why

The 2026-07-12 lock forced every surface in the game onto one 4-value Demichrome
ramp. That lock is superseded (owner, 2026-07-28): the game ships in full colour by
default. `menu-frame-system.md` §4 and `UI-PROTOTYPE-PLAN.md` §3 still describe the
old lock as binding — they are wrong as of that date, not as of this doc. This spec
replaces their palette content and keeps everything else they built (the layout, the
widget taxonomy, the squad/leash contracts) exactly as it stood, because none of that
was ever a palette claim.

The old system's real contribution wasn't the four hexes — it was the **shape**: a
small, closed set of named roles, each with one job, plus one reserved colour that
never does UI work. That shape survives. Only the values change, and they change from
flat single hexes to small role-ramps, because full colour can afford hover/press
depth that a flat 4-value ramp couldn't.

---

## 1. Palette and value roles in full colour

Four roles, plus one reserved colour that is not a UI role at all.

| Role | Base | States | UI job |
|---|---|---|---|
| **Ink** | `#1c1712` | Recess `#120e0b` | Screen ground, frame ink, recessed wells, text set on Parchment |
| **Iron** | `#585a66` | Hairline `#3c3d47` · Disabled text `#6f7078` | Borders, dividers, unfilled meter track, inactive/disabled, secondary text |
| **Parchment** | `#c9bfa0` | Bright surface `#ddd3b6` | Default surface fill, body text on Ink, neutral meter fill |
| **Ember** | `#e8a23c` | Hot `#ffcf6b` | Titles, focus/selection, active order, healthy meter fill, key values, the lamp glow |
| **Rubrication** *(reserved, not a role)* | `#8f1d1d` | — | Cost / temptation / violation only. Absent from every surface this doc governs. |

**The state mechanic is unchanged from the old system, only retargeted:** *selection,
focus, and "this matters" jump toward Ember; disabled/absent collapses toward
Iron/Ink.* Ink and Iron do not lighten into warmth on hover — only Ember carries
warmth, ever (see §2). Parchment is the one role that gets genuine *depth* in full
colour rather than a single flat value, because it is the surface every reading
session (a card, a panel, a row) sits on, and full colour lets a bright/base pair do
what the old system spent its whole "one value brighter" ladder move on.

Why four roles and not more: every additional role is another thing a component's
state table has to enumerate correctly across rest/hover/focus/selected/disabled (§4).
The old ramp's discipline — a small closed vocabulary, applied everywhere — is a
readability tool, not a budget constraint left over from 2-bit. Full colour did not
remove the reason for it.

---

## 2. Where colour carries meaning vs. decoration

**Decision: HUD/menu warmth reads as the flame, on purpose, but only through Ember,
and only there.** Ink, Iron, and Parchment stay strictly neutral — no role but Ember
is allowed to carry saturation. Reasoning:

- The premise (`FLAME-FOUNDATION.md`) is that the flame is the one thing that matters
  mechanically and narratively — the HUD is instrumentation *of that flame*: hero
  Vigor, company standing, the active order, the leaderboard's leader. Reporting on
  the flame's state in a colour that has nothing to do with fire would be lying about
  what the readout means. Ember is warm because what it measures is warm.
- This is deliberately **narrow**, not a warm theme. Structure (frame, chrome, body
  text, table hairlines) stays cold-neutral so that Ember reads as a genuine event
  every time it appears, not as ambient colour. A screen with nothing focused, nothing
  healthy-and-full, nothing leading, should show almost no Ember at all.
- **Ember never outshines the in-world flame.** The in-world fire is the brightest,
  most saturated thing the player can see when both HUD and world are on screen — the
  HUD's Ember is pinned lower-key (its "hot" state, `#ffcf6b`, is the ceiling; nothing
  in the UI goes past it). The hard constraint that the HUD must not cost the battle
  its screen (this doc's charter, §5) extends to colour: Ember competing with the
  Lampbearer's honest-light glint or the world's own flame for "brightest thing in
  frame" would be a bug, not a style choice.
- **Rubrication stays fully reserved**, unchanged from the old rule
  (`aesthetic-direction.md` §4 decision 2): cost/temptation/violation only, appears at
  decision-event/cost surfaces, never in menu or HUD chrome. Neither screen this doc
  specs uses it — both mockup panels carry an explicit "no rubrication on this screen"
  confirmation in the footer, matching the old menu spec's own confirmation pattern
  (`menu-frame-system.md` §4).

**Left open, not decided here:** each hero's own light-artifact already has a colour
in canon prose (Vanguard's Roll-Gold, Relickeeper's Waking Ember, Pathfinder's
Waylight, Lampbearer's Watch-Lamp — `CLASSES.md` §1–4), though that palette layer is
mid-revision and partly stale (`docs/data/art/palette.json` flags Roll-Gold as a
retired hex still cited at `CLASSES.md:78`). A richer version of this system could let
the active hero's own light hue stand in for Ember on that hero's medallion frame or
stance-active glow, instead of one shared amber for every class. That is a real,
attractive idea and it is explicitly **not decided here** — it depends on
`hero-palettes.md` being un-stale first, and it is the same shape as the open
portrait-register tension (memento-mori woodcut vs. medallion) that this doc also
declines to resolve. Ship the single shared Ember first; treat per-class Ember as a
v2 fork for the owner.

**Not this doc's job, flagged for completeness:** the world/sprite render palette
(`Kindled.Quantize`, `Kindled.PaletteSteps`, the `trial_palettes` candidates in
`docs/data/art/palette.json`) is a separate dial the pixel-art-director owns. This
system does not assume or require any particular world palette — it only asks that
Ember stay visually subordinate to whatever the world's own flame is doing, which
holds under any of the trial palettes.

---

## 3. Type

**One face, pixel-locked, integer sizes, no anti-aliasing.** The actual face is still
an open pick — `UI-PROTOTYPE-PLAN.md` §3(a) flagged this and this doc doesn't resolve
it — but it narrows the requirement: whatever face is chosen **must ship tabular
(lining, fixed-width) figures**, because §5's tables are unusable without them. A
proportional-digit pixel font is disqualified regardless of how good it looks.
Current placeholder in code (`SquadCard.cpp`, `MusterPanel.cpp`) is
`FCoreStyle::GetDefaultFontStyle` — a Slate default, not a real pick; every size below
assumes it will be replaced.

| Use | Size (1080p canvas, integer px) | Notes |
|---|---|---|
| Body / label | 10px | Squad names, chip labels, input hints |
| Section header | 12px | "Vanguard Company", column headers (8–9px, see below) |
| Table numerals | 12px, tabular | Rank + kill count — the one place digit alignment is load-bearing |
| Table header caps | 8–9px | Small-caps labels ("RANK", "UNIT", "KILLS") sit lighter than the data they head |
| Leader emphasis | 14px | Rank-1's rank glyph only — one defined step up, never a free size |
| Title / crest | 16–20px | Menu-only; illuminated/blackletter register (`05aaaecf…`); never appears in the live HUD |

**Steam Deck (1280×800, 7" panel, handheld):** render at an integer-scale multiple of
the 1080p canvas so nothing resamples fractionally (same rule as sprite import
discipline). If 10px body text reads too small on the actual device, the fix is a
coarser integer step (e.g. author against a 720p-equivalent virtual canvas and scale
1:1), not a fractional zoom on the 1080p assets. This needs a hardware pass to confirm
— flagged, not resolved by a desktop check.

---

## 4. Borders, frames, and the full state set — pixel rules

Every rule below names a role and a pixel width. No adjectives.

| Component | Rest | Hover (mouse) | Focus (controller/kbd) | Selected | Disabled |
|---|---|---|---|---|---|
| **Squad card frame** | 1px Iron border, Ink fill | 1px Iron border tints toward Parchment (no width change) | **2px Ember border** — width change, not just hue, so it reads from a couch | 2px Ember border + Ink fill unchanged (the ring is the tell, not the ground) | 1px Iron border at reduced contrast (still Iron, not a new value); label text drops to Iron |
| **Meter pip** (company, Vigor) | Empty = Iron pip, 1px Ink outline | — (meters aren't interactive) | — | Full = Ember; a pip that just filled gets one frame of Ember-hot before settling to Ember base | Routed-out squad's pips collapse to Iron regardless of fill fraction |
| **Stance chip** | Iron outline / Parchment text | — | — | **Active = Ember fill / Ink text** — full fill, not an outline change, because this is "the order in effect," the highest-stakes single glyph on the HUD | Inactive-and-unavailable stance: Iron outline, Iron text (both channels drop together) |
| **Stance wheel spoke** | Iron border, Iron-dim text | — | Ember border on the spoke under the stick (open-wheel only) | Current stance = Ember border + Ember text, persists after the wheel closes as the small indicator | — |
| **Leash gauge** | Track Iron, fill Parchment | — | — | 80%-break tick = **1px Ember line** across the track, fixed position, never animated in | Leash gauge doesn't disable; a broken unit's whole card gets the disabled treatment above |
| **Leaderboard row** | Iron hairline divider below, Parchment label, Parchment kills | — | — | Rank-1 row: rank glyph **and** kill count both go Ember (see §5) | — (a results board has no disabled state) |
| **Focused button / input** | Parchment glyph on Ink | — | **Full Ember fill**, Ink glyph — reserved for exactly one focused actionable element at a time, the single largest colour jump in the system | — | Iron glyph on Ink, no border |

**Rules that generalize across the table:**
- A state change that must be readable from a couch changes **width or fill area**,
  never hue alone (2px border, full fill vs. outline). Hue-only changes (the hover
  tint) are for mouse users at arm's reach and are optional — controller flow (§6 of
  `menu-frame-system.md`, unaffected by this doc) never depends on them.
- Disabled always collapses toward Iron/Ink on **every** channel of a component at
  once (border and text together) — a component that's Iron-bordered with
  Parchment text is not a valid disabled state, it's a bug.
- 1px halftone (checker, not gradient) is legal only on static, pixel-locked bands —
  header/footer/company-strip — per the inherited dither rule
  (`aesthetic-direction.md` §2.4, `docs/data/art/palette.json` `dither` block). It
  never appears on a moving or per-frame-rebuilt element (a card, a meter pip).

---

## 5. Density and tables — the leaderboard contract

This is the section the end-of-wave board (task-116) and any future ranked list
depend on. The board is **five fixed rows, no scroll, no variable height** — treat it
as a fixed instrument, not a list component, and most of the usual table complexity
(virtualization, sort, pagination) simply doesn't apply.

### 5.1 Row anatomy

```
RANK   UNIT                                          KILLS
 01    Spearmen ×50                                    142
 02    Vanguard              HERO                       97
 03    Veteran ×12                                      74
 04    Bannerman              STANDOUT                  51
 05    Shield ×24                                       38
```

- **Rank** — fixed 2-digit column, tabular figures, right-flush against the label
  gutter. Iron for ranks 2–5; **rank 1 alone jumps to Ember and one defined size step
  up** (12px → 14px, §3) — never a free/arbitrary size bump.
- **Unit** — left-aligned, single line, mid-truncates if it must (never wraps — a
  wrapped row breaks the fixed row height the whole board depends on).
- **Kills** — right-aligned, tabular figures, fixed digit width so the column doesn't
  jitter row to row. Cap the display at `999+`; the underlying number can exceed it,
  the glyph budget can't. Rank 1's kill count also goes Ember — the leader gets two
  reinforcing touches (rank + count), never a third (no background tint, no icon).

### 5.2 Aggregate rows vs. individual rows — text carries the distinction, not colour or an icon

The board holds two different kinds of subject — a squad/unit-*type* aggregate
(Spearmen, Veteran, Shield) and an individual standout or hero (the Vanguard hero
himself, a named Bannerman) — and it must hold both in the same five rows without a
scoring/sorting mechanic favoring one kind (task-116's job, not this doc's).

**Rule: the row's own text says what it is. No colour and no glyph mark is spent on
this distinction.** Two reasons, both hard constraints from this charter:

1. **Colour is already fully booked** (§2) — spending a second hue on "row kind"
   would mean either diluting Ember's one meaning or introducing a fifth role, and
   this system has exactly one warm colour on purpose.
2. **Every small mark in this game's vocabulary is already owned.** The four
   shape-carriers — rectangle-flip (Vanguard banners), dot-cluster (Relickeeper
   runes), thin-contour (Pathfinder quarry marks), point-halo (Lampbearer honest
   light) — are reserved sprite-scale reads (`docs/data/art/palette.json`
   `shape_carriers`, this charter's constraint 5). A "company" vs. "individual" glyph
   on a UI table would either collide with one of those four or invent a fifth
   mark-language the game doesn't need. The charter's own rule — *the HUD must not
   reuse those reads for chrome* — cuts the other way too: don't invent new chrome
   marks that could later be mistaken for one.

So the differentiation is purely typographic:
- **Aggregate rows** carry a `×N` suffix directly after the label, in Iron (secondary
  weight, so the count reads as metadata on the name, not a second data column).
- **Individual rows** carry no multiplier and instead may carry a small role tag —
  `HERO`, `STANDOUT`, or a Pathfinder pack member's own name with no tag at all, since
  a name is already the individuating signal for that class (`CLASSES.md` §3: *"a
  name is the one thing the Pathfinder still gives away for free"*). The tag sits in
  Iron at rest, Ember only if that row happens to be the leader.
- A squad name and an individual's name are never visually ambiguous in practice
  because the game's own naming discipline already keeps them apart: squad labels are
  role-plural nouns (Spearmen, Veterans, Shield), individuals are either a class-role
  noun (Vanguard, Bannerman) or a proper name reserved to classes whose fiction grants
  one (Pathfinder pack). This spec doesn't invent that discipline, it just declines to
  paper over it with an icon.

### 5.3 Table-wide rules

- Numerals are **always** tabular/lining — this is the one non-negotiable type
  requirement (§3) and the reason the font pick is gated on it.
- Every numeric column right-aligns; every label column left-aligns. Never center a
  data column — centering breaks vertical scan, which is the entire point of a
  ranked board.
- Row separators are 1px Iron hairlines only. **No zebra striping** — an alternating
  row tint would be a fifth value spent on pure decoration, and row height + alignment
  already carry the scan without it.
- The header row (RANK / UNIT / KILLS) is static label text, 8–9px small-caps, Iron —
  it never earns Ember, because the header isn't an event, a row's content is.

---

## 6. Supersession table

Section-by-section verdicts on the two docs this task's sibling (task-115) rewrites.
**Survives** = keep as-is. **Dead** = this doc replaces it outright. **Revise** =
structure is sound, specific claims inside it need retargeting to this doc's system.

### `docs/ui/UI-PROTOTYPE-PLAN.md`

| Section | Verdict | Note |
|---|---|---|
| Header / "Decided with owner" block | Revise | Still true; the `aesthetic-direction.md` citation needs "Direction A LOCKED" replaced with a pointer to this doc |
| §1 Goal & non-goals | Survives | Palette-agnostic |
| §2 UMG architecture (CommonUI decision) | Survives | Architecture is untouched by colour. **Also note:** the widgets it proposes as future work already exist in code — `ELVTR/Source/ELVTR/UI/` has `UMusterPanel`, `USquadCard`, `UStitchMeter`, `UMusterGrid`, `UKindledHud`, all `UCommonUserWidget`-based via `UKindledWidget` |
| §2a shared widget table | Revise | The widget list and roles are correct and largely already built; any Steel/Pale-named states need renaming to Iron/Ember per this doc |
| §2b subcamera cams | Survives | No palette content |
| §2c Screens & flow | Revise | Assumed `WBP_MainMenu → WBP_Muster → WBP_CombatHud` as a pre-play sequence. Owner decision (2026-07-30): **the game boots straight into play, no menu gate at launch.** Muster is reached from a pause, not from a title screen; `WBP_MainMenu` (settings/quit) is reachable only from pause, not the entry point |
| §3 Aesthetic reconciliation (8bitcn × Demichrome) | **Dead** | Entire premise — "cut 8bitcn's palette to the four Demichrome hexes" — is superseded. Replace with a pointer to §1–2 of this doc. The *component-pattern* half of the 8bitcn reference (not its paint) is unaffected and belongs in whatever replaces this section |
| §4 8bitcn→UE pipeline | Revise | Pipeline shape and import-discipline rules (Nearest/NoMipmaps/UserInterface2D) survive untouched. The one hard-rule bullet — *"palette-quantize every raster to the four Demichrome hexes only"* — is dead; replace with "quantize to the role system in `ui-aesthetic.md` §1" |
| §5 Milestones | Revise | Structure survives; M1 step 1's "`DA_UIStyle` with the 4 Demichrome tokens" line needs the token names updated |
| §6 Open decisions | Survives | Font/border-chunkiness/icon-coexistence are still open — this doc narrows the font requirement (tabular figures, §3) but doesn't pick one |
| §7 New-session bootstrap | Revise | Add this doc to the required reading list at step 1 |

### `docs/ui/menu-frame-system.md`

| Section | Verdict | Note |
|---|---|---|
| §1 Intent | Survives | The frame/well concept and the "menu / pause muster" framing (not a title screen) already anticipated the no-launch-gate decision |
| §2 Layout anatomy | Survives | ASCII wireframe and regions hold. The literal `EMBERKEEP (crest)` text in the diagram is a leftover naming instance for task-115 to catch, not a layout problem |
| §3 Component inventory | Revise | The 8bitcn-origin mapping and component list are genuinely reusable; the States column's Steel/Pale references need renaming to Iron/Ember |
| §4 Palette & texture | **Dead** | This is the section the lead flagged as actively stale — "strictly the four locked values" is superseded wholesale. Replace with a pointer to §1, §2, §4 of this doc |
| §5 Data bindings, §5a Squads & scale compaction | Survives | Sim/data content, not a palette claim; the squad-as-entity contract is untouched and already implemented (`FKindledSquad`, `KindledUITypes.h`) |
| §6 Interaction & input | Survives | Input model is colour-agnostic |
| §7 UMG translation | Revise | Mostly correct, but written as future work ("build a segmented meter widget") when the real classes already exist (`UStitchMeter`, `USquadCard`, `UMusterGrid`) — update to point at them and retarget palette references |
| §8 Deck/live-HUD collapse | Survives | Already implemented in `UKindledHud` (`Setup(bool bWithCams)`, `RebuildBand()`) matching this section's description closely |
| §9 Handoffs | Survives | Art briefs and gameplay-director asks are palette-agnostic |
| Readability check | Survives | The ask ("does a Pale/Ember meter fill compete with the Lampbearer's glint") is unaffected by which hex Pale/Ember is |
| Canon proposals | Survives | "None" |

---

## 7. Data bindings

The UI is further along than this task's brief assumed — real UMG C++ already exists
under `ELVTR/Source/ELVTR/UI/`. Not touched by this task (out of scope), cited here so
task-115 and any engineering follow-up don't rediscover it from zero.

| Element | Binding | Status |
|---|---|---|
| Stance enum | `EKindledStance` (`KindledUITypes.h`) — UI-facing mirror of gameplay `ESwarmStance`; M2 maps it via `USwarmSubsystem::GetStance()` | Built, UI-side |
| Squad state | `FKindledSquad` (`DisplayName`, `Size`, `Standing`, `Columns`, `Stance`, `bWide`) | Built, mock-fed (`UMusterPanel::MakeMockSquads`) |
| Company standing / meter | `UMusterPanel::Rebuild()` sums `Squads[].Standing`/`.Size` into a 24-segment `UStitchMeter` | Built |
| Retinue count, leash, hero Vigor (live) | `USwarmSubsystem` — `PeakRetinue`, stance getters exist; `UKindledHud::PushLiveMuster()` is the wiring point, currently a no-op stub pending real units | Partial → gameplay-director for the "company critical" threshold, unchanged from the old spec's open ask |
| **Colour values** | `KindledPalette.h`, namespace `Demichrome` (`Dark()/Steel()/Bone()/Pale()`), used directly by `SquadCard.cpp`, `MusterPanel.cpp` | **Needs an engineering pass to retarget to this doc's roles** — see §9 |

---

## 8. Interaction & input

Unaffected by this doc — controller-first flow, one-press stance radial, A-confirm/
B-back are all specified in `menu-frame-system.md` §6 and already implemented via
`UKindledWidget : public UCommonUserWidget` for CommonUI focus routing. This doc adds
one addition, covered in §4: **a focus change must always be visible as a width or
fill-area change**, not a hue-only tint, specifically because "unmistakable from a
couch" is a pixel-count requirement, not a colour-contrast one — a controller user
sitting several feet back loses subtle hue shifts before they lose a border doubling
in width.

---

## 9. UMG translation

- **Rename/retarget `KindledPalette.h`'s `Demichrome` namespace.** The four static
  accessors (`Dark()/Steel()/Bone()/Pale()`) map directly onto this doc's roles
  (Ink/Iron/Parchment/Ember) — same shape, new values, and ideally new names so a
  future reader isn't hunting for why "Demichrome" ships in full colour. Concretely:
  `KindledPalette::Ink()/Iron()/Parchment()/Ember()`, plus a `Rubrication()` constant
  that stays unused by any menu/HUD widget (comment it as such, the way the header
  comment already documents red's absence). Each accessor should return the *base*
  tone; states (`Hot`, `Recess`, `Hairline`, etc.) are a second small set of
  accessors, not new roles.
- **Meters** (`UStitchMeter`) — no structural change; `Full` segments render Ember
  instead of Pale, empty segments stay Iron-toned instead of Dark-toned. The "one
  frame of hot-then-settle" pip behaviour (§4) is new and needs a tiny timer or
  frame-delay in `UStitchMeter::SetValues`/`Rebuild` — flagged as a small addition,
  not a rebuild.
- **Squad cards** (`USquadCard`) — `Refresh()`'s border-colour swap
  (`bSelected ? Pale : Steel`) becomes `bSelected ? Ember : Iron`, and per §4 the
  selected state should also grow the border from 1px to 2px (currently a colour-only
  swap; `Frame->SetPadding` already exists as the lever, since the 2px "reveal" is
  literally the padding between `Frame` and `Fill`).
- **Leaderboard** — no existing widget covers this; task-116 will need a new
  `UUserWidget` (suggest `W_WaveBoard` or similar, task-116's call) built the same way
  as the existing widgets: pure C++, `UKindledWidget` base, five fixed
  `UHorizontalBox` rows inside a `UVerticalBox`, no `UListView`/virtualization since
  the row count is fixed at five (§5). Tabular-figure alignment is a font-asset
  requirement (§3), not something the widget code can fix on its own.
- **Font** — every widget currently falls back to `FCoreStyle::GetDefaultFontStyle`.
  This doc doesn't resolve the pick, but any resolution must supply tabular figures
  (§3) or the leaderboard ships numerically unreadable regardless of layout code.

---

## 10. Mockup

https://claude.ai/code/artifact/bc2f4bc0-e1a6-4959-b010-cfdd7fa4b13e — local copy
`docs/ui/mockups/ui-aesthetic-menu-and-wave.html`. Two panels side by side: the pause/
muster panel (structure-heavy — medallion, company meter, five squad cards including
the wide 50-strong Spearmen compaction case, stance wheel, leash gauge) and the
end-of-wave board (five fixed rows deliberately mixing three squad-type aggregates
with two individuals — the hero and a named Bannerman standout — to prove §5's
text-only distinction reads at a glance). A role-swatch legend at the foot names all
five colours including the reserved one, and both game panels carry an explicit
"no rubrication on this screen" line so the reservation is confirmed, not just
claimed. Chrome (frame plates, medallion art, corner motifs) is deliberately plain
placeholder geometry — it previews structure and colour, not final pixel art.

---

## 11. Handoffs

**→ pixel-art-director:** no new art brief from this doc — it's a palette/type/table
system, not new drawn assets. Two things worth a heads-up rather than a formal brief:
(a) the per-class Ember fork noted in §2 depends on `hero-palettes.md` being
un-stale; (b) the font pick (§3) is co-owned with whoever runs `8bitcn-to-ue` and the
tabular-figures requirement should be checked before any candidate face is adopted.

**→ gameplay-director:** unchanged from the prior spec — the squad-health rollup and
"company critical" threshold (§7) are still open, and a new one from this doc:
task-116's leaderboard needs to define what makes a unit or hero "standout" enough to
earn an individual row over being folded into its type's aggregate (§5.2 assumes that
selection exists; this doc only specs how it reads once made).

**→ task-115 (menu-frame-system.md / UI-PROTOTYPE-PLAN.md rewrite):** the
supersession table (§6) is written to be applied section-by-section without
re-deriving verdicts. The `EMBERKEEP` crest text still in `menu-frame-system.md` §2's
ASCII diagram is a leftover naming instance, flagged for that pass, not fixed here
(out of scope, per this task's boundaries).

**→ engineering (no owner named yet):** §9's `KindledPalette.h` rename/retarget is
real, scoped work sitting in existing files this doc doesn't touch. Small enough to
be a single task rather than folded into task-115 or task-116, but flagged here so it
doesn't get lost between them.

---

## Readability check

*(to pixel-art-director — the live-HUD bar, not a brief)* I'm carrying forward the
prior spec's assumption unchanged and adding one: Ember used only for the meter fill,
the active stance, and the leaderboard leader — never a tint, never a bloom, never
covering more screen area than the old Pale did. **New assumption to confirm:** Ember
at its "hot" ceiling (`#ffcf6b`) still reads as dimmer/smaller than the in-world
flame and the Lampbearer's honest-light glint at 500 units on screen. If the world's
own light is ever tuned warmer or brighter than this ceiling, the ceiling moves down,
not the world's light — please confirm that ordering holds, since this doc bet on it
without a hardware/PIE check.

## Canon proposals

None. This spec operates entirely inside decisions already made by the owner
(2026-07-28 colour return, 2026-07-30 boot-into-play and mixed leaderboard rows); it
proposes nothing new to canon, only declines to resolve two forks already flagged as
open by prior docs (per-class Ember, §2; portrait register, inherited from
`portrait-register.md`, untouched here).
