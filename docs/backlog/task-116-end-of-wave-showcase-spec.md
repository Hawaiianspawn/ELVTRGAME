---
id: 116
title: Spec the end-of-wave showcase that names the five units doing the work
status: done
agent: ui-director
model: sonnet
owns: ["docs/ui/end-of-wave-showcase.md", "docs/ui/mockups/end-of-wave-showcase.html"]
resources: []
depends-on: [114]
epic: ui-showcase
evidence: docs/ui/end-of-wave-showcase.md with a published Artifact mockup of the panel at three states — a clean wave, a bloodbath, and the run's final wave — plus a data-contract section naming every field the Mass side must expose, written as a spec an engine task can be filed from without asking a follow-up question
score: {feel: 2, risk: 2, cost: 2}
source: user
decided: "2026-07-31 done"
teammate: wave-showcase
---

## Why now
The owner wants the wave break to answer "who did the work" — a top-five board of hero
types, unit types and standout individual units with their kill counts, filterable more
than one way.

The wave break already exists to hang it on: `ELVTR/Source/ELVTR/Spike/Spike1GameMode.h`
defines `ERunPhase { Deploying, WaveActive, Breather, Won, Lost }`, and **`Breather` is
the beat between waves** where reinforcements arrive. That is the panel's moment, and it
needs no new game state to trigger.

**The data does not exist yet, and that is the point of specifying this first.**
`ELVTR/Source/ELVTR/Mass/SwarmTelemetry.h` tracks `KilledBrood` and `KilledRetinue` as
run-wide aggregates and nothing else — no per-type counters, no per-entity attribution,
nothing that can rank anything. A leaderboard needs kill credit recorded at the moment of
the kill in the Mass combat processors, and how that is stored is a real cost decision at
swarm scale.

So this task writes the **UI spec and the data contract together**, and the engine work
gets filed from it afterwards as its own task with its own rebuild window. Specifying the
panel first is what stops the attribution being built to the wrong shape — a per-entity
tally and a per-type tally are very different costs, and only the UI knows which one the
screen actually needs.

Depends on **task-114** for the aesthetic, and it is 114's densest customer: a ranked
table of numerals is the one surface the existing UI language never covered.

## Done when
- **`docs/ui/end-of-wave-showcase.md` exists** and specifies:
  - **The panel at the `Breather` beat** — how it arrives, whether it holds the wave break
    open or shares it with the reinforcement arrival, and how it is dismissed.
  - **The board itself.** Five rows. What a row shows and what makes rank 1 obvious
    without animation. Ties, and what a row looks like for a unit that died earning its
    place — attrition is constant, so the dead-leader case is the normal case, not an edge
    one.
  - **Types and individuals in one board.** The owner asked for both. Say which is the
    default view and how the other is reached — drill-down, toggle, or two stacked
    sections — and pick one rather than listing options.
  - **The filter set.** Name the actual axes ("many ways to filter it" is the ask, so
    enumerate them): by wave vs. run-cumulative, by unit type, by squad, by hero class,
    by alive-vs-fallen. State which are v1 and which are named-but-later.
  - **What it looks like with nothing to show** — wave one, no meaningful leader, or a
    wipe. A leaderboard that only looks good on a good wave is a leaderboard that lies.
  - **Controller-first navigation** — filters and drill-down have to be reachable on a
    gamepad without a cursor.
- **A data contract section**, written for an engine task to be filed from directly:
  every field the UI needs, its granularity (per type / per entity / per squad), when it
  resets (per wave, per run, both), and — explicitly — **what the panel would lose if
  per-entity attribution proves too expensive at swarm scale**, so the engine task has a
  stated fallback instead of inventing one.
- **A published Artifact mockup** at **three states**: a clean wave, a bloodbath where
  most of the board is fallen, and the run's final wave. Local copy at
  `docs/ui/mockups/end-of-wave-showcase.html`.
- The spec does **not** specify C++, Mass fragments, or processor changes. It states what
  it needs and stops.

## Spawn prompt

```
You are executing task-116. You are the ui-director for Kindled.

GOAL: spec the END-OF-WAVE SHOWCASE — a top-five board naming the units and unit types
doing the work, with kill counts, filterable — and the data contract the engine side
must satisfy to feed it.

READ FIRST:
  1. docs/ui/ui-aesthetic.md — written by task-114, DONE before you start. Its density
     and table section is written for exactly this panel. Use it; do not invent a second
     visual language.
  2. docs/ui/menu-frame-system.md §8 — the combat-HUD "collapse" idea, the existing
     precedent for how chrome behaves over a live fight.
  3. docs/narrative/FLAME-FOUNDATION.md — narrative canon.

OWNER DECISIONS, already made — build to them, do not re-ask:
  - The board ranks BOTH unit/hero TYPES and INDIVIDUAL standout units. You decide which
    is the default view and how the other is reached, and you commit to one answer.
  - The game BOOTS STRAIGHT INTO PLAY (task-115 covers the menu side).
  - This task is SPEC AND MOCKUP ONLY. No engine work, no C++.

THE ENGINE FACTS, already checked by the lead — do not re-derive, and do not assume more
than this:
  - ELVTR/Source/ELVTR/Spike/Spike1GameMode.h defines
    ERunPhase { Deploying, WaveActive, Breather, Won, Lost }, three waves,
    WaveBroodCounts = {250, 450, 700}. BREATHER is the wave-break beat where
    reinforcements arrive — that is your panel's moment, and it needs no new game state.
  - ELVTR/Source/ELVTR/Mass/SwarmTelemetry.h tracks ONLY run-wide aggregates:
    KilledBrood, KilledRetinue, and an exchange rate derived from them. THERE IS NO
    PER-TYPE AND NO PER-ENTITY KILL ATTRIBUTION ANYWHERE. Your leaderboard's data does
    not exist yet. That is expected — you are specifying what has to be built.
  - Units are typed already (knight subtypes, archers — see ELVTR/Source/ELVTR/Mass/
    SwarmCombat.h) and squads exist as an entity the UI already binds to. Existing UI
    contract: bind to SQUAD-level state, never per-soldier actors. Per-individual
    attribution is the thing that would break that rule, which is why your data contract
    has to be explicit about its cost and its fallback.
  - Kindled runs a Mass Entity swarm at thousands of entities. Per-entity state is not
    free. Say what the panel loses if per-entity attribution turns out too expensive,
    rather than assuming it will be granted.

CANON WARNINGS:
  - The game is named KINDLED; "Emberkeep" is a discarded working title (task-092 may be
    purging it concurrently — never revert someone else's edit).
  - THE 4-VALUE COLOUR GATE IS SUPERSEDED (owner, 2026-07-28) — full colour,
    Kindled.Quantize 0. Any UI doc still calling the 4-value palette LOCKED is stale.
  - docs/WORLD.md is superseded; canon is docs/narrative/FLAME-FOUNDATION.md.
  - Heroes are identified BY ROLE ONLY — Vanguard, Relickeeper, Pathfinder, Lampbearer.
    NEVER invent a named hero; the owner reversed that decision because attrition makes
    names hard to remember. If individual units need identifiers, solve it WITHOUT a name
    roster, and say what you chose.

WRITE docs/ui/end-of-wave-showcase.md covering: the panel at the Breather beat (arrival,
whether it holds the break open, dismissal); the board (five rows, what a row shows, how
rank 1 reads without animation, ties, and the DEAD-LEADER case — attrition is constant,
so a unit that died earning its place is the normal case); types and individuals in one
board with a committed answer for default and drill-down; the FILTER SET enumerated by
name (per-wave vs run-cumulative, unit type, squad, hero class, alive vs fallen — mark
which are v1); the EMPTY AND UGLY states (wave one with no real leader, and a wipe);
controller-first navigation with no cursor.

Then a DATA CONTRACT section, written so an engine task can be filed straight from it:
every field, its granularity (per type / per entity / per squad), when it resets (per
wave, per run, or both), and the explicit fallback if per-entity attribution is too
expensive.

THE MOCKUP — Artifact tool, THREE states in one page: a clean wave, a bloodbath where
most of the board is fallen, and the run's final wave. Self-contained HTML, theme-aware.
Local copy at docs/ui/mockups/end-of-wave-showcase.html.

YOU OWN ONLY: docs/ui/end-of-wave-showcase.md, docs/ui/mockups/end-of-wave-showcase.html

DO NOT TOUCH: docs/ui/ui-aesthetic.md (task-114's), docs/ui/menu-frame-system.md and
docs/ui/UI-PROTOTYPE-PLAN.md (task-115 owns them and may be running beside you),
docs/art/**, docs/backlog/**, ELVTR/** (no engine work at all), GDD.md, SYSTEMS.md.

You have no shell. Do not build, PIE, or run scripts.

HAND BACK: the filter set you committed to and which axes you deferred, the data contract
verbatim (an engine task gets filed from it), the Artifact URL, your answer to the
individual-unit identity problem given there is no name roster, and the fallback you named
for expensive per-entity attribution.
```
