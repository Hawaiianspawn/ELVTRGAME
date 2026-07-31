# The Wave Board — end-of-wave showcase

**Status:** current · **Owner:** UI/UX Director · **Task:** task-116
**Extends:** `docs/ui/ui-aesthetic.md` §5 (the leaderboard contract this board implements),
`docs/ui/menu-frame-system.md` §8 (the live-HUD collapse this panel docks against),
`docs/narrative/FLAME-FOUNDATION.md` (the flame premise).
**Reuses, does not reinvent:** `ui-aesthetic.md`'s role system (Ink/Iron/Parchment/Ember),
type scale, and §5's row anatomy verbatim — this doc is that leaderboard contract's first
real content, not a second visual language.
**Mockup:** https://claude.ai/code/artifact/b5a25360-6913-4309-828e-f46b4ac5d18b
(local: `docs/ui/mockups/end-of-wave-showcase.html`).

> **Naming:** the game is **Kindled**. Heroes are identified by role only (Vanguard,
> Relickeeper, Pathfinder, Lampbearer) — no proper names, ever (`CLASSES.md`, owner
> reversal). The current spike (`Spike1GameMode.h`) runs one unnamed hero and a two-type
> retinue (Spearmen, Archers) — everything below is written against that real roster, not
> the four-class future one.

---

## 1. Intent

At the **Breather** beat (`ERunPhase::Breather` — wave cleared, reinforcements arrive,
`Spike1GameMode.h`), the player gets a report: who did the work. The board answers one
question — *what just carried this wave* — for both the **squads** the player commands
(GDD §10, squad-as-entity) and the **hero**, in the exact five-row instrument
`ui-aesthetic.md` §5 already specced.

This is not a menu. It does not use the full-dress Sampler Frame (lintel, guardian
flanks, corner medallions at full size) from `menu-frame-system.md` — that register is
for the pause/muster screen, reached deliberately. The Breather is a beat *inside* combat
flow, so the board docks as a **compact framed card in the central well of the live,
collapsed HUD** (`menu-frame-system.md` §8): the bottom command band (company meter,
stance indicator) stays visible and unobstructed underneath it, and the standing army is
still visible behind it through a thin Ink scrim — the player is being shown a report
*about* the field they're still standing on, not pulled into a separate screen.

**Frame regions used:** a small stitched card frame (border + two corner rosettes,
reused from the Sampler Frame's medallion furniture, not full guardian panels) + header
band (halftone, static) + footer input band. **Regions that don't exist here at all:**
flanking guardian columns, beast lintel, hero medallion — this card is smaller than even
the Deck collapse of the muster panel, because it has one job and five rows to do it in.

---

## 2. Layout anatomy

Target the same 1080p canvas ui-aesthetic.md §3 sizes are authored against; 16:10 Steam
Deck collapse needs no special-case here (see note at end of section) because this panel
never carried the full-dress frame's flanking columns to begin with.

```
                    ┌─◉──────────────────────────◉─┐   ◉ = small corner rosette (reused)
                    │        WAVE 2  CLEARED        │   header band (halftone, static)
                    │      the Highgates · floor 1   │
                    ├────────────────────────────────┤
                    │ RANK   UNIT                KILLS│  column header, 8-9px caps, Iron
                    │  01    Spearmen ×50           142│  ← real lead: rank+kills → Ember
                    │  02    Vanguard      HERO      97│
                    │  03    Veteran ×12              74│
                    │  04    Bannerman  STANDOUT †    51│  ← fallen individual (†, typographic)
                    │  05    Shield ×24                38│
                    ├────────────────────────────────┤
                    │ A Continue   RB Kind: All        │   footer input band
                    │                no rubrication     │
                    └────────────────────────────────┘
        ░░░░ (behind, dimmed by an Ink scrim: the standing army, company meter,
              stance indicator — all still visible, none of it covered) ░░░░
```

Fixed card width sized to the table (§5.1 of `ui-aesthetic.md`: five rows, no scroll, no
variable height) — roughly 420px at the 1080p canvas, centered, floating above the
bottom HUD band rather than replacing it.

**Steam Deck (16:10):** integer-scale the same card; no layout collapse needed since
there's no flanking furniture to drop. If the card's width competes with the live bottom
band's cam projection at 1280×800, shrink the card's own padding first (§4's pixel rules
already require no fractional resampling) before touching the band underneath it — the
board is a guest over the HUD, never the other way around.

---

## 3. Component inventory

Everything below is `ui-aesthetic.md` §4/§5 verbatim, re-declared here only where this
panel adds something new.

| Component | Origin | States (value-role) |
|---|---|---|
| **Card frame** | Sampler Frame corner-rosette furniture, shrunk | Static. 1px Iron border, Ink fill, halftone header/footer bands (legal — static, pixel-locked, §4). |
| **Leaderboard row** | `ui-aesthetic.md` §5, verbatim | default = Iron hairline divider, Parchment label, Parchment kills; **rank-1 (real lead only) = Ember rank + Ember kills**; no other state (a results board has no disabled state). |
| **Fallen mark** | New, typographic only | a single `†` glyph appended directly after an **individual** row's name. Not a role tag, not a colour, not an icon — see §6. Never appears on an aggregate row (a wiped squad already says so via `×0`, §6). |
| **Row-kind chip** | Badge, re-skinned | `All` / `Types` / `Individuals`, Iron outline/Parchment text at rest, **active = Ember fill/Ink text** (same treatment as a stance chip, §4's active-fill rule — this is "the order in effect" for what the board shows). |
| **Scope indicator** | Text, not a control in v1 | "This wave" / "Whole run" — read-only label, auto-selected by `ERunPhase` (§5). No chip, no manual toggle (§7 — deferred). |
| **Ink scrim** | New | flat Ink at reduced opacity behind the card only — never full-screen, never tinting the live scene's own palette (constraint 5, this charter). |

---

## 4. Palette & texture

Strictly the four `ui-aesthetic.md` roles, no fifth value. Ink = card ground + scrim;
Iron = borders, hairlines, header/footer caps, non-leading rows, the `×N` suffix, role
tags at rest; Parchment = row labels, body kills; Ember = **only** the real rank-1's
rank+kills and the active row-kind chip — nothing else on this panel ever goes Ember.
**1px halftone** on the header and footer bands only (static, pixel-locked, per the
inherited dither rule). **Rubrication is absent** and the footer confirms it, matching
the pattern every other panel in `ui-aesthetic.md` §10 already carries — a results
report, win or wipe, is never a cost moment.

---

## 5. Data contract

This is the section an engine task gets filed from directly. Every field below is
written against the **real current code** (`Spike1GameMode.h`, `SwarmSubsystem.h`,
`SwarmCombatProcessors.cpp`, `SwarmFragments.h`) — not the four-hero future.

### 5.1 What exists today (free)

| Field | Source | Granularity | Notes |
|---|---|---|---|
| `GetSquadStanding(i)` / `GetSquadType(i)` | `USwarmSubsystem` | per squad (0..7) | live, always current — feeds the `×N` suffix and the row label |
| `WaveBroodCounts`, `GetPhase()`, `GetWaveIndex()` | `Spike1GameMode` | per run | drives header text (wave number, Breather vs Won) |
| `ESwarmFightOutcome` | `SwarmTelemetry.h` | per fight | `RetinueWiped`/`HeroDown` selects the wipe-state copy (§6) |

### 5.2 What has to be built (the real ask)

| Field | Granularity | Reset cadence | Cost | Notes |
|---|---|---|---|---|
| `WaveKilledBySquad[MaxSquads]` (int32) | per squad instance | `Spike1GameMode::BeginWave()` | **Medium** | The board's primary feed. See §5.3 — needs attacker identity threaded through the combat pass, not just the death pass. |
| `RunKilledBySquad[MaxSquads]` (int32) | per squad instance | `USwarmSubsystem::ResetRunState()` | **Medium** | Same credit site as above, one more accumulator. |
| *(Type-level view)* | per unit type (2 today) | n/a — **not stored** | **Free** | Sum `WaveKilledBySquad[i]` / `RunKilledBySquad[i]` over squads where `GetSquadType(i) == Type`, at read time. No new field — the "Types" row-kind filter is a fold, not a counter. |
| `HeroWaveKills` / `HeroRunKills` (int32) | one singleton individual | `BeginWave()` / `ResetRunState()` | **Low** | The hero's own `HeroDamage` branch in the combat pass already isolates his blow (`SwarmCombatProcessors.cpp`, the `if (bHeroStriking)` block) — crediting a kill there when that blow is lethal is a local, one-branch addition. |

### 5.3 How squad/type credit actually works (the engineering shape)

`USwarmDeathProcessor` (where kills are counted today, `AddKills`) is **too late** — it
only sees `HP <= 0` after the combat pass already summed every contributing attacker's
`Damage` into one number and discarded who they were. Squad-level attribution has to
happen **inside** `USwarmCombatProcessor`'s per-victim loop, at the point it already
computes `Damage` and checks `Health[i].HP -= Damage`:

1. `USwarmSubsystem::FGridEntry` (currently `Location`, `bRetinue`, `bStriking`,
   `StrikeReachSq`, `TargetsPerHit`, `BlowsClaimed`, `BlowDamage`) needs the attacker's
   `SquadId` byte added — one more `uint8`, populated at `AddToGrid` from the same
   `FSwarmAnimFragment::SquadId` the grid-build pass already has in hand. Cheap: no
   class-layout change on a hot fragment, just a wider struct that already gets rebuilt
   from scratch every frame.
2. In the victim's `QueryNeighbors` visit (`SwarmCombatProcessors.cpp` ~line 565), when
   an entry's blow is claimed, note its `SquadId`. When this frame's `Damage` is about
   to take the victim's `HP` to `<= 0`, credit **one** contributing squad —
   the first one claimed this frame, by iteration order. This is the same arbitrary,
   documented tie-break the code already uses for `BlowsClaimed` itself ("whichever
   chunk/entity is iterated first this frame... not a meaningful unfairness over the
   course of a run") — this contract reuses that precedent rather than inventing
   proportional-damage credit, which would need a second pass of bookkeeping this model
   doesn't otherwise carry.
3. `USwarmSubsystem` gets a `CreditKill(uint8 SquadByte)` sibling to `AddKills`,
   incrementing both the wave and run accumulators for that squad's `UnitIndex`.

### 5.4 Individual "standout" rows below the hero — the expensive piece, named honestly

The hero is the **only** individual the engine can show cheaply today, because the hero
pawn is never destroyed — its state is always live-readable. Every other soldier *is*
destroyed on death (`ChunkContext.Defer().DestroyEntity`, `USwarmDeathProcessor`), and
Kindled's combat model deliberately carries **no per-entity gameplay state** for the
general retinue (Design Law 5) — no per-soldier kill tally exists, or is cheap to add,
for the same reason no per-soldier anything else exists.

Building a real "Bannerman had 12 kills" row requires, in order:

1. **A promotion/flag decision** (gameplay-director, not this doc) — which soldier
   becomes "the" trackable standout for a squad. `CLASSES.md`'s Freed→Militia→Veteran→
   Bannerman ladder is the fictional hook, but no promotion system exists in code yet.
2. **Attacker *entity* identity**, not just squad identity, threaded through the same
   combat-pass credit site as §5.3 — a bigger addition than a byte on `FGridEntry`,
   since it means resolving and comparing a `FMassEntityHandle` per credited blow.
3. **A snapshot-on-death step**, *before* `DestroyEntity` fires, copying a standout's
   `{Type, SquadOrigin, Kills, RoleTag}` out of live-entity space into a small capped
   side table (`DeadStandouts`, ring buffer, ~16 entries) on the subsystem — because the
   **normal** case (per this task's brief) is that the standout earning its row died
   doing it, and a destroyed Mass entity has nothing left to read.

**This is real, scoped work, and it is explicitly not built by this task.**

### 5.5 Fallback — what the board loses if §5.4 is judged too expensive

**Ship v1 with squad-aggregate rows + the hero row only (§5.1–5.3).** No other
individual identity exists below the hero without §5.4, and this board must never
fabricate a name or a number to fill a row. Concretely:

- The **"Individuals"** row-kind filter (§7) degrades to **"Hero only"** — it never
  disappears (the chip still means something, still shows real data), it just has one
  possible row instead of several.
- The five-row board is allowed to come up **short** (see §6, wave-1 case) rather than
  invent a subject. A short board — 2 or 3 real rows plus honest placeholders — is a
  correct read of an early or individual-sparse wave, not a bug to paper over.
- If §5.4 later ships, the fallback degrades gracefully in the other direction: the
  "Individuals" filter simply starts having more than one possible row, no layout or
  contract change needed.

---

## 6. States — the empty and the ugly

### 6.1 Wave one, no real leader

Wave 1 has exactly two squads (Spearmen, Archers) plus the hero — at most 3 real
subjects for 5 row slots, and their kill counts are close together early. Two rules,
both already implied by §5's contract rather than special-cased:

- **Never fabricate a row.** An unfilled slot renders rank number (Iron, never Ember),
  an em-dash `—` for the unit column, `—` for kills — not `0`, since `0` claims a real
  subject that fought and scored nothing, which didn't happen.
- **No Ember without a real lead.** Rank-1's Ember treatment requires
  `rank1.kills > rank2.kills`, strictly. A tie at the top (a realistic wave-1 outcome)
  produces an all-Iron, all-Parchment board — which *is* the correct "no leader yet"
  read (`ui-aesthetic.md` §2: "Ember reads as a genuine event... a screen with nothing
  leading should show almost no Ember at all"). No extra state to build — this falls
  out of the existing comparison for free.

### 6.2 A wipe (`ESwarmFightOutcome::RetinueWiped` / `HeroDown`)

`WaveKilledBySquad`/`RunKilledBySquad` are independent accumulators, not derived from
current standing — a wiped squad still shows its real kill count against a `×0`
standing suffix: **`Spearmen ×0 — 89`** is a complete, legible, honestly grim row, and
it needs no memorial mechanism at all (unlike an individual, a squad's counters never
get destroyed with its members). The hero row survives a `HeroDown` outcome the same
way — `HeroWaveKills` lives on the subsystem, not the pawn.

Copy note (flagged, not load-bearing): the header's "Cleared" should not read as an
Ember-word win-state on a loss — reserve that treatment for an actual clear; a wipe's
header reads in plain Parchment. Final narrative copy is not this doc's call.

### 6.3 The run's final wave — Won and Lost alike

Same widget, and **it appears at both `ERunPhase::Won` and `ERunPhase::Lost`** — a
losing run still earns the report; who carried a fight that was ultimately lost is the
same honest record `ui-aesthetic.md`'s "everything is the kingdom's record" register
already commits to, and building one widget that answers both is the lazy, correct
call over inventing a second results screen. `Scope` auto-selects run-cumulative
(`RunKilledBySquad` + `HeroRunKills`, §7) for both. Content is otherwise whichever of
§6.1/§6.2's rules applied to the run's last engagement — a `Lost` board is a wipe board
(§6.2) with the run-cumulative numbers instead of the wave's. The footer drops the
Continue/backstop-timer entirely on both — neither phase has a next wave to advance
into. See §12 for how this coexists with the pause menu at these phases.

---

## 7. Types and individuals in one board — default, drill-down, and the filter set

**Committed default: the unified mixed board**, exactly as `ui-aesthetic.md` §5.2
already locked it — five rows, ranked by kill count, squad-aggregates and individuals
interleaved, the row's own text (the `×N` suffix vs. a role tag) carrying the
distinction, no colour or icon spent on it. This task does not reopen that call; it
implements it.

**How the other view is reached:** a single **row-kind chip** (`All` → `Types` →
`Individuals` → `All`), cycled by one dedicated control (§9), not a second screen and
not a tab bar. `Types` re-ranks using the type-level fold from §5.2; `Individuals`
shows the hero (and, once §5.4 ships, any tracked standouts) only. This *is* the
drill-down — a filter, not a navigation.

### Filter set (enumerated, v1 status marked)

| Filter | v1? | Mechanism |
|---|---|---|
| **Row kind** (All / Types / Individuals) | **v1** | one chip, cycled by a dedicated control (§9) — the "other view" above |
| **Scope** (per-wave / run-cumulative) | **v1**, but automatic | context-driven by `ERunPhase` (§6.3), not a manual toggle — one fewer control to navigate with |
| **Unit type** (Spearmen / Archers / All) | **v1** | narrows rows to one type's squads; cheap, `EUnitType` already exists |
| **Squad** (a specific one of ≤8 unit slots) | **Deferred** | low value at 2 types / ≤8 squads — unit-type already narrows meaningfully; revisit if squad count grows |
| **Hero class** (Vanguard/Relickeeper/Pathfinder/Lampbearer) | **Deferred** | only one hero class exists in the current build; multi-hero/co-op is itself deferred (`FLAME-FOUNDATION.md` §4.4) — nothing to filter yet |
| **Alive vs. Fallen** | **v1, as a status mark, not a hide toggle** | the `†` mark (§6) is always on; an isolate-only-alive/only-fallen hide filter is deferred — a 5-row fixed board rarely needs to hide a row it can just mark |

**Ties** (any scope, any filter): broken by ascending squad/unit-slot index (0..7), hero
sorting last. Deterministic, free, no timestamp bookkeeping needed.

**How rank 1 reads without animation:** the board's numbers are already fully resolved
the instant it's shown (the wave is over) — there is no count-up, no reveal animation on
the data itself, ever. Rank-1's Ember jump (rank digit to 14px + Ember, kills to Ember,
§3) is a static layout fact the instant the panel appears, identical to how a squad
card's selected-state border is a static 2px, not an animated one (`ui-aesthetic.md`
§4's own rule: state changes are width/fill-area facts, not motion). The only permitted
motion on this panel is its own arrival transition (§9) — never the numbers.

---

## 8. Mockup

https://claude.ai/code/artifact/b5a25360-6913-4309-828e-f46b4ac5d18b — local copy
`docs/ui/mockups/end-of-wave-showcase.html`. Three states in one page, sharing the exact
CSS token system `ui-aesthetic-menu-and-wave.html` already established (same variable
names, same values — no second visual language):

1. **A clean wave** — a real lead (Spearmen ×50, Ember rank+kills), one hero row, one
   fallen-and-still-ranked Bannerman (`†`), ordinary aggregate rows filling the rest.
2. **A bloodbath** — most rows show `×0` standing against real kill counts, the header
   reads a plain-Parchment "The Line Fell," and the hero row survives at `HeroDown`.
3. **The run's final wave** — `Scope: Whole run`, no Continue/timer in the footer, a
   run-cumulative kill count visibly larger than any single-wave number.

---

## 9. Interaction & input

**Controller-first, no cursor** — there is nothing to move a highlight between. Exactly
three bound controls, each with its own dedicated input, never a shared focus ring:

- **A / Confirm** — dismiss. On a Breather board this also lifts the phase hold (see
  below). On the final-wave board there is nothing to confirm into; A is absent from the
  footer (§6.3).
- **RB / right bumper** — cycles the row-kind chip (`All → Types → Individuals → All`).
- **LB / left bumper** — cycles unit-type filter (`All → Spearmen → Archers → All`).
  Grey/Iron-collapsed (§4's disabled rule: both border and label drop together) when
  `Row kind = Individuals`, since a type filter has nothing to narrow there.

Mouse/keyboard: click the chip directly, same targets. Motion: the panel's own
arrival/dismissal is a stitch-wipe/ink-bleed *in spirit*, snappy in practice — under
~150–200ms, disabled under `prefers-reduced-motion`, matching `menu-frame-system.md` §6.
Nothing about a filter change animates beyond the chip's own state jump (§3, active =
full Ember fill, not a fade).

**Arrival, hold, and dismissal at the Breather beat:**

- **Arrival:** the panel appears the instant `Phase` flips to `Breather`
  (`Spike1GameMode::EnterPhase`). The live scene keeps rendering underneath (army idle,
  brood cleared) — nothing pauses.
- **Does it hold the beat open? Yes, and this is a flagged engine gap, not a UI-only
  decision.** `Spike1GameMode::BreatherSeconds` defaults to **2 seconds** — a fine
  placeholder for a fast internal test loop, but not long enough for a player to read a
  five-row table. This doc asks (→ gameplay-director, §11) that the Breather→WaveActive
  transition gate on **either** the player pressing A **or** a backstop timer generous
  enough to read the board (recommend 6–8s, not this director's number to fix), instead
  of the current fixed 2s regardless of the board being on screen. This backstop is a
  countdown on top of `PhaseTimer`, not a replacement for it, so task-115's freeze of
  `PhaseTimer` while its pause menu is open (§12) freezes this backstop too — summoning
  the menu can't burn down the read-time budget this doc is asking for.
- **Dismissal:** A/Confirm advances immediately, same as pressing it early always could
  — the backstop is a safety net for an unattended controller, not the primary path.
- **The pause menu can also be summoned while this board is up — see §12.**

---

## 10. UMG translation

- **`W_WaveBoard`** — new `UUserWidget`, `UKindledWidget`-based, pure C++, matching
  every existing widget's idiom (`USquadCard`, `UMusterPanel`). Five fixed
  `UHorizontalBox` rows inside a `UVerticalBox` — **no `UListView`**, same reasoning
  `ui-aesthetic.md` §9 already gives (row count is fixed at five, never virtualized).
- **View-model:** a small `FWaveBoardRow` struct (`FText Label`, `FText Tag`, `int32
  Kills`, `bool bFallen`, `bool bIsIndividual`, `bool bLead`) populated by one read
  function on `USwarmSubsystem` (or a thin helper) that folds §5's fields per the
  active Row-kind/Scope/Unit-type filter state — the widget itself does no ranking
  logic, it renders a pre-sorted, pre-filtered array of exactly ≤5 rows (padded with
  placeholder rows per §6.1 if short).
- **Card frame** — reuses whatever 9-slice frame asset `WBP_SamplerFrame` already
  defines for its corner rosettes, at a smaller fixed size; no new frame asset.
- **Fallen mark** — a literal `†` glyph in the same font run as the label — a type
  requirement (tabular figures + this one extra glyph in the pick), not a new icon
  asset.
- **Docking** — the card lives in `UKindledHud`'s existing `Band`/`Overlay` structure
  (`KindledHud.h`), as a new overlay slot above `Rect` (the live command band), shown/
  hidden on `Phase == Breather || Phase == Won`. `UKindledHud::PushLiveMuster()` is the
  existing precedent for a "read `USwarmSubsystem`, push to UI" tick — `W_WaveBoard`
  follows the same shape rather than inventing a second update path.
- **Font** — inherits the still-open pick from `ui-aesthetic.md` §3/§9; the `†` glyph
  and tabular figures both need to survive whichever face is chosen.

---

## 11. Handoffs

**→ gameplay-director:**
1. §5.2/§5.3 — file `WaveKilledBySquad`/`RunKilledBySquad`/`HeroWaveKills`/
   `HeroRunKills` plus the combat-pass credit-site change as a real engine task; this
   section is written to be filed verbatim.
2. §5.4 — the promotion/flag decision (which soldier becomes a trackable standout) is a
   balance/design call this doc explicitly declines to make. Until it's made, §5.5's
   fallback (hero-only individuals) is what ships.
3. §9 — `Spike1GameMode::BreatherSeconds` (currently 2s) needs either a real value
   raise or a dismiss-gated phase transition; the current constant predates this panel
   and is a placeholder engineering value, not a tuned UX number.

**→ pixel-art-director:** no new brief. This panel reuses the Sampler Frame's existing
corner-rosette furniture at a smaller size and the `†` fallen-mark is pure typography —
nothing here needs new drawn pixels. Flagging only: the tabular-figures + one extra
glyph (`†`) requirement rides on whatever font `ui-aesthetic.md` §3/§9 eventually picks.

**→ task-115 (`menu-frame-system.md` / `combat-hud.md`):** this panel docks against the
live-HUD's collapsed bottom band (`menu-frame-system.md` §8, `UKindledHud`). `docs/ui/
combat-hud.md` doesn't exist yet — when it lands, confirm this panel's overlay-in-the-
well placement doesn't collide with whatever that spec puts in the same band.

---

## 12. Coordination with the pause menu (task-115)

task-115's respec makes the summonable pause menu a **hard pause, full screen**, with
its own per-phase behaviour. Two things that respec explicitly left to this doc rather
than deciding for it:

**Does this board block the summon, get dismissed by it, or sit behind it? — dismissed
by it.** A menu summon **hides the board and does not otherwise touch it.** No new
state, no z-order, no dimming-behind-a-dimming-scrim stack: the board's own visibility
is already just `Phase == Breather || Phase == Won || Phase == Lost`, and dismiss-by-
summon adds one more condition, `&& !bPauseMenuOpen`. Nothing about the board's *hold*
logic (§9 — Breather doesn't advance until A or the backstop) changes while the menu is
open, because task-115 already freezes `PhaseTimer` for exactly this reason. On menu
close, the board reappears on its own — the phase never advanced, so there's nothing to
restore. **Blocking the summon** was rejected outright: a report screen that prevents
the player from pausing is a worse bug than any coordination cost of the simple
hide/show rule. **Sitting behind** was rejected as unnecessary complexity: the pause
menu is full-screen and hard-paused, so there is no composited "behind" state a player
could ever see anyway — sitting behind a fully opaque, full-screen widget is
indistinguishable from being dismissed by it, so build the one that needs no z-order.

**Does the board appear at Won and Lost, and is the pause menu suppressed there? —
yes to both appearing (§6.3), no to suppression.** The pause menu stays summonable at
`Won`/`Lost` — a terminal run is exactly when a player wants Restart/Quit-to-title,
which is the pause menu's territory, not this board's. The **same dismiss-by-summon
rule from above applies unchanged**: no special case is needed because a terminal
phase doesn't advance or expire on its own the way `Breather`'s timer does, so there is
nothing for a menu summon to race or interrupt. One rule covers all three phases this
board can be shown in.

---

## Readability check

*(to pixel-art-director)* This panel is not a play-space HUD element in the RTS-slice
sense — it appears only at Breather/Won, when brood are already cleared, so the "500
moving units" bar doesn't directly apply. What I am assuming and asking confirmed: the
Ink scrim behind the card (§3) stays subtle enough that the standing army and the
company meter underneath remain genuinely readable through it, not just technically
visible — since the whole point of not going full-screen is that the player is still
looking at their army, not a report *instead of* it.

## Canon proposals

None. This spec operates inside decisions already made (single-player, role-only hero
identity, the mixed-board default already locked by `ui-aesthetic.md` §5.2) and flags
implementation gaps (§5.4, `BreatherSeconds`) as engineering/balance handoffs, not new
canon.
