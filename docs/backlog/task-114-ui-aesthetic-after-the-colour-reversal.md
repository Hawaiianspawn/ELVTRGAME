---
id: 114
title: Write the UI aesthetic that replaces the four-value gate the colour reversal killed
status: done
agent: ui-director
model: sonnet
owns: ["docs/ui/ui-aesthetic.md", "docs/ui/mockups/ui-aesthetic-*.html"]
resources: []
depends-on: []
epic: ui-showcase
evidence: docs/ui/ui-aesthetic.md plus a published Artifact mockup showing the language applied to two real surfaces — one menu panel and one end-of-wave results panel — in full colour, with the value roles named per element rather than described in the abstract
score: {feel: 2, risk: 2, cost: 2}
source: user
decided: "2026-07-31 done"
teammate: ui-aesthetic
---

## Why now
Every UI document in this repo binds to a direction that no longer holds.
`docs/ui/UI-PROTOTYPE-PLAN.md:24-27` names **"Direction A LOCKED: strict global 4-value
Demichrome palette"** and `docs/ui/menu-frame-system.md` builds its whole value-role map
on those four hexes. The owner retired the colour gate on **2026-07-28** — the game ships
in full colour, `Kindled.Quantize 0`, and `docs/art/aesthetic-direction.md` is the canon
that says so.

That leaves the UI in the worst possible state: not undecided, but confidently pointed at
a superseded rule. Anyone who builds a screen from those docs today builds it to a dead
spec, and two more UI tasks in this epic are waiting to do exactly that.

This task is the one that has to land first, because the menu spec (task-115) and the
end-of-wave showcase (task-116) are both *applications* of an aesthetic. Writing them
against four greys and re-skinning later is two passes for one result.

## Done when
- **`docs/ui/ui-aesthetic.md` exists** and is written for full colour from the start —
  not as a patch to Demichrome, and not as "Demichrome plus accents".
- It answers, concretely enough that two people building different screens land in the
  same place:
  - **Palette and value roles.** What carries ground, chrome, surface, body text, focus,
    danger and the single glint, now that there are more than four values to spend. The
    old map (Dark=ground, Steel=borders, Bone=surfaces, Pale=focus) is the *shape* worth
    keeping even though its four hexes are gone.
  - **Where colour is allowed to mean something** and where it is decoration. The flame
    is the game's central image and the narrative premise (`docs/narrative/FLAME-FOUNDATION.md`)
    — say whether UI warmth reads as flame or stays neutral so the world owns that colour.
  - **Type.** One face, its sizes at integer pixel steps, and what it does at 1080p vs a
    Steam Deck panel.
  - **Borders, frames and states.** Rest / hover / focus / selected / disabled, described
    as pixel-level rules, not adjectives. Controller-first: focus must be unmistakable at
    a glance from a couch.
  - **Density.** The end-of-wave board is a *table of numbers*, which nothing in the
    current UI language covers. Rank rows, numerals, alignment, how a leader is made
    obvious without animation.
  - **What survives from the existing docs and what is explicitly superseded**, section by
    section, so task-115 knows exactly which parts of `menu-frame-system.md` it is
    rewriting rather than guessing.
- **A published Artifact mockup** applies the language to two surfaces at once: a menu
  panel and an end-of-wave results panel with a five-row leaderboard. Both in one page so
  they can be judged against each other. Local copy under `docs/ui/mockups/`.
- The spec states plainly, in one line near the top, that it **supersedes the 4-value
  Demichrome sections** of `UI-PROTOTYPE-PLAN.md` §3 and `menu-frame-system.md` §4 — and
  does **not** edit those files (task-115 owns them).

## Spawn prompt

```
You are executing task-114. You are the ui-director for Kindled.

GOAL: write the UI aesthetic for a game that ships in FULL COLOUR, and prove it with a
mockup. Two sibling tasks build on this one, so it lands first.

THE CANON YOU MUST NOT GET WRONG — this repo will actively mislead you:

  - The game is named KINDLED. "Emberkeep" is a discarded working title. A sibling task
    (task-092) may be renaming it in these same docs while you work; if you see it in a
    file you are reading, ignore it, and NEVER revert someone else's edit.
  - THE 4-VALUE COLOUR GATE IS SUPERSEDED, decided by the owner 2026-07-28. The game
    ships in full colour with the posterisation off (Kindled.Quantize 0). Canon is
    docs/art/aesthetic-direction.md. Read it FIRST.
  - docs/ui/UI-PROTOTYPE-PLAN.md:24-27 and docs/ui/menu-frame-system.md §4 still say the
    strict 4-value Demichrome palette is LOCKED. THAT IS STALE. Read them for their
    component language and their structure — which are good and worth keeping — and
    treat every palette claim in them as dead.
  - docs/WORLD.md is superseded by the 2026-07-22 narrative reset. Narrative canon is
    docs/narrative/FLAME-FOUNDATION.md: you carry the only flame in a pitch-dark world,
    your army needs your light, flame-bearers are treated as gods. Good-guys tone.
  - Heroes are identified BY ROLE ONLY — Vanguard, Relickeeper, Pathfinder, Lampbearer.
    The owner reversed the fixed-name decision because attrition makes names hard to
    remember. Never invent a named hero.
  - There is an unresolved tension the owner has NOT settled: memento-mori woodcut
    portraits vs. the medallion register (drafts in RawArt/Portraits/). Do not resolve
    it yourself. If your aesthetic touches portraits, present it as an open fork.

WHAT TO WRITE — docs/ui/ui-aesthetic.md, covering:
  1. Palette and value roles in full colour. The OLD four-value role map
     (ground / chrome / surface+body / focus-glint) is a good SHAPE — keep the shape,
     replace the four hexes with a real colour system. Say what each role is for.
  2. Where colour carries meaning vs. where it is decoration. The flame is the game's
     central image; decide whether UI warmth reads as flame or stays neutral so the
     world owns that colour, and say why.
  3. Type: one face, integer pixel sizes, behaviour at 1080p and on a Steam Deck panel.
  4. Borders, frames, and the full state set — rest / hover / focus / selected /
     disabled — as PIXEL RULES, not adjectives. Controller-first: focus must be
     unmistakable from a couch. This repo's UI is gamepad-first (menu spec §6).
  5. DENSITY AND TABLES. The end-of-wave board task-116 is specifying is a table of
     numbers with five ranked rows, and NOTHING in the current UI language covers
     numerals, alignment, rank emphasis or how a leader reads at a glance. This section
     is the one your siblings need most — do not shortchange it.
  6. A SUPERSESSION TABLE: which sections of UI-PROTOTYPE-PLAN.md and
     menu-frame-system.md survive, which are dead, section by section. task-115 rewrites
     those files using your table, so be specific.

THE MOCKUP — use your Artifact tool. ONE page showing TWO surfaces so they can be judged
against each other:
  (a) a menu panel, and
  (b) an end-of-wave results panel with a FIVE-ROW leaderboard (rank, unit or type name,
      kill count), because that is the surface the epic exists for.
Self-contained HTML, no external assets, theme-aware. Save a local copy under
docs/ui/mockups/ as ui-aesthetic-*.html.

YOU OWN ONLY: docs/ui/ui-aesthetic.md and docs/ui/mockups/ui-aesthetic-*.html

DO NOT TOUCH: docs/ui/menu-frame-system.md or docs/ui/UI-PROTOTYPE-PLAN.md (task-115
owns them and will apply your supersession table), docs/art/** (pixel-art-director's
canon — bind to it, never edit it), docs/backlog/**, ELVTR/** (anything at all — no
engine work is in scope here), GDD.md, SYSTEMS.md.

You have no shell. Do not attempt to build, PIE, run a script, or generate art.

HAND BACK: the spec's section list, the Artifact URL, the supersession table verbatim
(your siblings depend on it), and any place you deliberately left a fork open for the
owner rather than deciding it yourself.
```
