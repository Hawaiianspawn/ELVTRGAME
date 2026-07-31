---
id: 115
title: Respec the menu as a front door the game never waits at
status: done
agent: ui-director
model: sonnet
owns: ["docs/ui/menu-frame-system.md", "docs/ui/UI-PROTOTYPE-PLAN.md", "docs/ui/mockups/menu-frame-system.html"]
resources: []
depends-on: [114]
epic: ui-showcase
evidence: A revised menu-frame-system.md and UI-PROTOTYPE-PLAN.md carrying the new aesthetic and a boot-straight-into-play flow, plus a published Artifact mockup of the menu as it appears when summoned mid-run rather than as a launch gate
score: {feel: 2, risk: 1, cost: 2}
source: user
decided: "2026-07-31 done"
teammate: menu-front-door
---

## Why now
The owner's call: **the game boots straight into play.** No menu gate on launch. That is
not a tweak to the existing flow — `docs/ui/UI-PROTOTYPE-PLAN.md` §2c specifies
`WBP_MainMenu → WBP_Muster → WBP_CombatHud` as a linear launch sequence, with the main
menu as the front door you pass through to reach the game. Boot-straight-in inverts it:
the menu becomes something you *summon*, and it has to work as an overlay on a live run
rather than as a full-dress opening screen.

Two things follow that the current spec has no answer for. A menu that appears over a
running fight cannot use the full Sampler Frame chrome without hiding the fight behind
it, and "start a run" stops being a menu action at all — which means the menu's job is
now settings, roster, and quitting, not launching.

The engine agrees with the owner already: `ELVTR/Config/DefaultEngine.ini:3` boots
`L_Spike1` directly and no main menu exists in `ELVTR/Source/`. This task makes the spec
describe what the game actually does, then makes it good.

Depends on **task-114** because the menu's chrome, states and focus treatment are all
applications of the aesthetic, and 114 also produces the supersession table saying which
sections of these two files are dead.

## Done when
- **`docs/ui/menu-frame-system.md` is revised**, not appended to: every section task-114's
  supersession table marks dead is rewritten in the new aesthetic, and the surviving
  component language is kept intact rather than re-derived.
- **`docs/ui/UI-PROTOTYPE-PLAN.md` §2c and §5 describe the real flow** — the run starts
  on launch; the menu is summoned over it. The milestone list is re-cut to match, since
  "build the main menu first" is no longer the first step of anything.
- The spec answers, concretely:
  - **What the menu is for now that it does not start the game.** Settings, roster/muster,
    quit — name the actual set.
  - **How it appears over a live run.** Pause or no pause; how much of the fight stays
    visible; whether the Sampler Frame chrome survives at overlay scale or collapses the
    way the combat HUD does (menu spec §8's collapse is the existing precedent — reuse it
    rather than inventing a second one).
  - **The summon and dismiss inputs**, controller-first, and what happens to a run in
    progress while the menu is up.
  - **First-launch versus every other launch.** Something has to introduce the game once;
    say whether that is a one-time flow, a first-run overlay, or deliberately nothing.
- A **published Artifact mockup** shows the menu as summoned over a live run — not as a
  launch screen — in the task-114 aesthetic. Local copy at
  `docs/ui/mockups/menu-frame-system.html`.
- **The CommonUI question gets a recommendation with a reason**, since the screen stack it
  was chosen for (`UI-PROTOTYPE-PLAN.md` §2) is smaller now that there is no launch flow.
  Recommend; do not decide — it is an engineering dependency, and the owner's call.

## Spawn prompt

```
You are executing task-115. You are the ui-director for Kindled.

GOAL: respec the menu around one owner decision — THE GAME BOOTS STRAIGHT INTO PLAY.
No menu gate at launch. The menu becomes something summoned over a live run.

READ FIRST, IN THIS ORDER:
  1. docs/ui/ui-aesthetic.md — written by task-114, which is DONE before you start. It
     is your aesthetic and it contains a SUPERSESSION TABLE naming which sections of the
     two files you own are dead. Follow that table; do not re-litigate it.
  2. docs/ui/menu-frame-system.md and docs/ui/UI-PROTOTYPE-PLAN.md — the files you are
     revising.
  3. docs/art/aesthetic-direction.md — art canon.

THE CANON YOU MUST NOT GET WRONG:
  - The game is named KINDLED. "Emberkeep" is a discarded working title. task-092 may be
    renaming it in these same files concurrently — never revert someone else's edit.
  - THE 4-VALUE COLOUR GATE IS SUPERSEDED (owner, 2026-07-28). The game ships in full
    colour, Kindled.Quantize 0. Both files you own still claim the 4-value Demichrome
    palette is LOCKED. That is exactly what you are fixing.
  - docs/WORLD.md is superseded by the 2026-07-22 reset; narrative canon is
    docs/narrative/FLAME-FOUNDATION.md.
  - Heroes are identified BY ROLE ONLY (Vanguard, Relickeeper, Pathfinder, Lampbearer).
    Never invent a named hero.

WHAT THE ENGINE ACTUALLY DOES TODAY, already checked — do not re-derive:
  - ELVTR/Config/DefaultEngine.ini:3 boots /Game/Spike1/L_Spike1 directly.
  - There is NO main menu anywhere in ELVTR/Source/. Nothing to preserve compatibility
    with; you are specifying the first one.
  - The run structure is real: ELVTR/Source/ELVTR/Spike/Spike1GameMode.h defines
    ERunPhase { Deploying, WaveActive, Breather, Won, Lost } over three waves
    (WaveBroodCounts = {250, 450, 700}). A summoned menu has to say what it does in each
    of those phases.

WHAT TO WRITE:
  1. REVISE docs/ui/menu-frame-system.md in place. Rewrite what task-114's supersession
     table marks dead. KEEP the component inventory and interaction language that
     survives — it is good work and re-deriving it is how it gets worse.
  2. REVISE docs/ui/UI-PROTOTYPE-PLAN.md §2c (screens and flow) and §5 (milestones) so
     they describe boot-straight-into-play. "Build the main menu first" is no longer the
     first step of anything; re-cut the milestone list honestly.
  3. Answer concretely: what the menu is FOR now that it does not start the game
     (settings, roster/muster, quit — name the real set); how it appears over a live run
     (pause or not, how much of the fight stays visible, whether the Sampler Frame chrome
     survives at overlay scale or collapses — menu spec §8 already specifies a collapse
     for the combat HUD, REUSE that idea rather than inventing a second one); the summon
     and dismiss inputs, controller-first; and what first launch does differently from
     every other launch, if anything.
  4. CommonUI: give a RECOMMENDATION with a reason, not a decision. The screen stack it
     was chosen for is smaller without a launch flow. It is an engineering dependency and
     the owner's call.

THE MOCKUP — use your Artifact tool: the menu SUMMONED OVER A LIVE RUN, not a launch
screen, in the task-114 aesthetic. Self-contained HTML, theme-aware. Local copy at
docs/ui/mockups/menu-frame-system.html.

YOU OWN ONLY: docs/ui/menu-frame-system.md, docs/ui/UI-PROTOTYPE-PLAN.md,
docs/ui/mockups/menu-frame-system.html

DO NOT TOUCH: docs/ui/ui-aesthetic.md (task-114 wrote it — bind to it, never edit it),
docs/ui/end-of-wave-showcase.md (task-116 owns it and may be running beside you),
docs/art/**, docs/backlog/**, ELVTR/** (no engine work is in scope), GDD.md, SYSTEMS.md.

You have no shell. Do not build, PIE, or run scripts.

HAND BACK: what you rewrote vs. kept in each file, the Artifact URL, your CommonUI
recommendation in two sentences, and anything the boot-straight-in decision broke that
you could not resolve inside the spec.
```
