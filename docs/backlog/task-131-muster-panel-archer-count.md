---
id: 131
title: Let the muster panel say how many archers you have — the type split exists in the sim and never reaches the HUD
status: proposed
agent: claude
model: ""
owns: ["ELVTR/Source/ELVTR/UI/KindledHud.cpp", "ELVTR/Source/ELVTR/UI/KindledHud.h", "ELVTR/Source/ELVTR/UI/MusterPanel.cpp", "ELVTR/Source/ELVTR/UI/MusterPanel.h", "docs/ui/muster-typed-readout.md"]
resources: ["unreal-editor"]
depends-on: []
epic: archers-on-the-field
evidence: A PIE screenshot in which the muster panel states a live archer count that visibly tracks the army as archers die, beside the current flat retinue readout for comparison.
score: {feel: 2, risk: 1, cost: 1}
source: user
teammate: ""
decided: ""
---

## Why now
`UKindledHud::PushLiveMuster` (`KindledHud.cpp:158`) already refreshes off the sim every
0.15s and reads `USwarmSubsystem::GetAliveRetinue()` — so the panel correctly counts units
the player owns whether or not they are on screen. What it cannot do is say how many of
them are *archers*. `GetAliveByType(EUnitType::Archers)` has existed since the typed-unit
work landed and no UI code calls it; the cards carry five hardcoded cosmetic squad names
(`Shield`, `Vets`, `Spearmen`, `Banner`, `Reserve`) that predate unit types entirely.

Archers are now visually distinct on the field (`task-126`/`127`/`130`) and visibly shoot
(`task-129`). The player can see them and cannot count them. The owner asked what the
manager knows about units off screen — it knows the total, and nothing about the split.

## Done when
- The muster panel states a live archer count, or a spearman/archer split, sourced from
  `GetAliveByType` and not from a hardcoded name list.
- The number tracks attrition — archers dying drops it, in a PIE screenshot.
- The flat total still reads correctly; this adds a breakdown, it does not replace the
  company readout.
- The five cosmetic squad names are either reconciled with unit types or explicitly left
  alone with a one-line note saying why. Do not silently leave two disagreeing models of
  "what a squad is" in the same panel.
- `docs/ui/muster-typed-readout.md` records what the panel now reads and from where.

## Spawn prompt

```
You are executing task-131. Read docs/backlog/task-131-muster-panel-archer-count.md first,
then ELVTR/Source/ELVTR/UI/KindledHud.cpp's PushLiveMuster in full.

GOAL
The muster panel counts the retinue but cannot say how many are archers. Give it the
type split.

WHAT IS ALREADY TRUE — do not rebuild it
PushLiveMuster (KindledHud.cpp:158) already runs off the sim every 0.15s, already handles
the mock-until-first-spawn case, and already rebuilds only when the visible state changed.
That refresh path is correct; you are adding to what it reads, not rewriting how it reads.
USwarmSubsystem::GetAliveByType(EUnitType::Archers) and GetAliveByType(EUnitType::Spearmen)
already exist (SwarmSubsystem.h:502) and are maintained by the sim. Use them.

THE LAZY PATH
Read the two per-type counts in PushLiveMuster alongside the existing GetAliveRetinue()
call and surface them in the panel's existing readout. Do NOT build a new widget class, do
NOT add a second panel, do NOT restructure the card model, and do NOT add a per-unit
iteration — the counts are already maintained, so this is a read, not a tally.

THE ONE JUDGEMENT CALL
The five hardcoded card names (Shield, Vets, Spearmen, Banner, Reserve, KindledHud.cpp:195)
are cosmetic squads and predate unit types. They now sit next to a real type split that
disagrees with them — one of those names is literally "Spearmen" while the sim has an
actual Spearmen type meaning something else. Either reconcile them or leave them and write
one line saying why. Do not leave two disagreeing models of "squad" in the panel silently.

DO NOT TOUCH
ELVTR/Source/ELVTR/Mass/** (the sim is not yours — read GetAliveByType, change nothing),
SwarmRenderActor.cpp, UnitCamProjector.cpp, ELVTR/Content/**, GDD.md, SYSTEMS.md,
CLASSES.md, or any docs/design/ file. Other sessions have live uncommitted work in this
tree; do not revert or tidy anything you did not write.

Note a collapsed or zero-sized UMG widget stops ticking entirely — if your readout never
updates, check the widget's size before suspecting the data path.

HAND BACK
- A PIE screenshot showing the live archer count, beside the current flat readout.
- What you did about the five cosmetic squad names, and why.
- docs/ui/muster-typed-readout.md.

Do not commit and do not push. The lead handles that at the close.
```
