---
id: 058
title: Tighten the HUD chrome, and let UMG follow the live ramp instead of hardcoding four hexes
status: parked
agent: claude
owns: ["ELVTR/Source/ELVTR/UI/EmberkeepHud.cpp", "ELVTR/Source/ELVTR/UI/EmberkeepHud.h", "ELVTR/Source/ELVTR/UI/EmberkeepPalette.h", "ELVTR/Source/ELVTR/UI/MusterPanel.cpp", "ELVTR/Source/ELVTR/UI/MusterPanel.h", "ELVTR/Source/ELVTR/UI/MusterGrid.cpp", "ELVTR/Source/ELVTR/UI/MusterGrid.h", "ELVTR/Source/ELVTR/UI/SquadCard.cpp", "ELVTR/Source/ELVTR/UI/SquadCard.h", "ELVTR/Source/ELVTR/UI/StitchMeter.cpp", "ELVTR/Source/ELVTR/UI/StitchMeter.h", "ELVTR/Source/ELVTR/UI/EmberkeepUITypes.h", "docs/ui/hud-chrome.md"]
resources: ["unreal-editor"]
depends-on: [57]
epic: "scene-tightening"
evidence: A PIE session where the HUD reads as one deliberate system at the shipping camera — and where dragging Emberkeep.Palette recolours the muster panels, cards and meters along with the world instead of leaving them stranded on demichrome.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: ""
decided: "2026-07-28 parked"
---

## Why now
Owner goal, 2026-07-28: *"tighten up the UI and visuals in the scene."* This is the UI half;
`task-057` is the visuals half.

There is a concrete, visible defect to fix rather than a vague polish ask. `EmberkeepPalette.h`
hardcodes the four Demichrome hexes and UMG draws **after** post-processing, so the palette
dial `task-043` shipped recolours the entire world and every unit — and leaves the HUD
stranded on the old ramp. Flip to eulbink and the world goes cyan while the muster panels
stay olive. `task-043` recorded this honestly as an accepted v0 gap. `task-057` makes it
worse by adding a bypass and 2-8 value ramps, so the gap widens from "one palette" to "every
exploration state". Closing it is what makes the exploration rig usable on a whole screen.

The second half is the shelf itself. The layout is mid-transition: the game is now one
camera (`Emberkeep.UI.Cams 0`), the two-wing layout that sized the muster panels against the
Unit Cam is gone, and the shelf is held up by a single `Emberkeep.UI.BandHeight` number with
down-only scaling — a fallback, not a design. Spacing, border weight and type scale were
never passed over as a set.

## Done when
- **UMG follows the live ramp.** `EmberkeepPalette.h` resolves its four (or N) values from the
  same preset the post pass is using, rather than returning literals. Dragging
  `Emberkeep.Palette` recolours HUD and world together. Where the ramp has more than four
  values, the four *roles* (Dark = ground/ink, Steel = borders/inactive, Bone = surfaces/body
  text, Pale = focus/the single glint) map onto it sensibly and are documented.
- **A chrome pass over the shelf as one system** — consistent margins, border weights, and
  type scale across `MusterPanel`, `SquadCard`, `StitchMeter` and the company meter, judged
  at the shipping camera and not in isolation.
- **The knobs are Breadboard rows, not constants.** Whatever gets tuned (padding, border
  weight, type scale, shelf height) lands as `Emberkeep.UI.*` CVars in the `/cvars` canonical
  set with range hints, so the owner can dial the look live rather than accept the
  teammate's taste. This is the "toggleable state in the breadboard" part of the goal.
- `docs/ui/hud-chrome.md` records the resulting scale and the role mapping.
- **Proof is a runnable build**, not a diff.

## Scope fence
- **Not** the Unit Cam. `UnitCamProjector.*` and `ViewCamCapture.*` stay untouched — the Unit
  Cam is disabled, not deleted, and `task-055` has history there.
- **Not** the menu → muster → combat flow. That is `task-023`, still proposed, and it is a
  much bigger piece of work. This task tightens what is on screen now.
- **Not** new widgets or new HUD features. Chrome and colour only.
- The literal-white hit flash is `task-040`, parked. Leave it.

## Why this depends on 057
Two reasons, both real. UMG cannot follow an N-value ramp until an N-value ramp exists. And
both tasks need `unreal-editor`, which is a hard mutex — this is a queue of two, not two
threads running side by side.

## Spawn prompt
```
You are tightening Emberkeep's HUD chrome and making UMG follow the live palette ramp
(C:\Projects\ELVTRGAME).

Owner goal, 2026-07-28: "tighten up the UI and visuals in the scene ... If we can throw it
in a toggleable state in the breadboard that would be good." You are the UI half. task-057
did the visuals half and has already landed — read
docs/backlog/task-057-scene-look-rig-bypass-the-colour-gate.md and its handback before you
start, because it defines the ramp you are about to follow.

THE DEFECT THAT MOTIVATES THIS — do not re-derive it: ELVTR/Source/ELVTR/UI/EmberkeepPalette.h
hardcodes the four Demichrome hexes, and UMG draws AFTER post-processing. So the
Emberkeep.Palette dial recolours the whole world and every unit, and leaves the HUD stranded
on the old ramp — flip to eulbink and the world goes cyan while the muster panels stay olive.
task-057 widened that gap from one palette to a bypass plus 2-8 value ramps. Close it.

BUILD:
  1. EmberkeepPalette.h resolves its values from the SAME preset the post pass is using
     instead of returning literals. Dragging Emberkeep.Palette must recolour HUD and world
     together. Where the ramp has more than four values, map the four ROLES onto it and
     document the mapping: Dark = ground/ink, Steel = borders/inactive/spent, Bone =
     surfaces/body text, Pale = focus/healthy/the single glint.
  2. A chrome pass over the shelf AS ONE SYSTEM — consistent margins, border weights and
     type scale across MusterPanel, SquadCard, StitchMeter and the company meter. Judge it
     at the shipping camera, not widget-by-widget in isolation.
  3. EVERY value you tune becomes an Emberkeep.UI.* CVar in the /cvars canonical set with a
     range hint — padding, border weight, type scale, shelf height. The owner wants to dial
     the look live rather than accept your taste. Constants in the source are a failure of
     this task, not a shortcut.
  4. Record the resulting scale and the role mapping in docs/ui/hud-chrome.md.

CONTEXT ON THE CURRENT LAYOUT: the game is now ONE camera (Emberkeep.UI.Cams 0). The
two-wing layout that sized muster panels against the Unit Cam is gone, and the shelf is held
up by a single Emberkeep.UI.BandHeight number with down-only scaling. That is a fallback, not
a design — spacing, border weight and type scale have never been passed over as a set.

GOTCHAS:
  - Live Coding CANNOT add a UPROPERTY — it reports success then crashes the next PIE. New
    UPROPERTYs on any UUserWidget subclass need a full editor-closed rebuild. CVars are fine.
  - Collapsed / zero-sized UMG widgets stop ticking. If something you expect to update goes
    still, check its visibility before hunting the logic.
  - The tree may carry uncommitted work from a concurrent session in EmberkeepHud.cpp and
    EmberkeepUIDebug.cpp. Build on top of it. Do not revert, stash or tidy it, and do not
    attribute it to anyone.

DO NOT TOUCH:
  - UnitCamProjector.* or ViewCamCapture.* — the Unit Cam is disabled, not deleted, and
    task-055 has history there.
  - The menu -> muster -> combat flow. That is task-023, still proposed, and much larger.
  - The literal-white hit flash (task-040, parked).
  - ELVTR/Content/PostProcess/** or ELVTR/Source/ELVTR/Rendering/** — task-057's territory.
  - No new widgets, no new HUD features. Chrome and colour only.

You hold the unreal-editor lock. Deliver ON-SCREEN EVIDENCE from a runnable build — PIE,
with a capture showing the HUD recolouring alongside the world as the palette row is dragged.
Not a diff plus "it works". The owner launches the editor for review; you launch it for
testing.
```
