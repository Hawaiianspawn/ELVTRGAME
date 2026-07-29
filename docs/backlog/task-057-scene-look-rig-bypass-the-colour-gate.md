---
id: 057
title: Take the colour gate off — a bypass and an N-value quantizer, live on the Breadboard
status: done
agent: claude
owns: ["ELVTR/Content/PostProcess/**", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h", "ELVTR/Saved/SwarmExecOnPlay.txt", "docs/RENDERING-LIGHTING.md", ".claude/skills/cvars/SKILL.md", "docs/data/art/palette.json"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
epic: "scene-tightening"
evidence: A PIE session where one Breadboard row drops the posterize entirely and the raw lit scene appears, a second row walks the value count 2→8 live, and setting steps back to 4 with quantize on returns the scene to today's look with no visible difference.
score: {feel: 2, risk: 3, cost: 2}
source: user
teammate: colour-gate
decided: "2026-07-28 done"
---

## Why now
Owner goal, 2026-07-28: *"tighten up the UI and visuals in the scene. We can explore options
without the color gate for now. If we can throw it in a toggleable state in the breadboard
that would be good."*

The colour gate is the thing in the way. `task-043` put a palette **preset** dial on the
Breadboard and it works — but its scope fence was explicit: *"FOUR-value presets only.
Threshold1/2/3 gives exactly four buckets; a 5+ value palette needs more thresholds and is a
follow-up, not v0."* That follow-up is this task, plus the harder ask the fence never
covered: turning the quantizer **off** so the raw lit scene can be judged on its own.

Right now every visual decision is made through a 4-value posterize that was locked in
2026-07-12. That lock is defensible as a shipping look and indefensible as an *exploration*
tool — you cannot tell whether a lighting change is good or whether the quantizer is hiding
it. This gives back the raw signal, on a toggle, without unlocking anything permanently.

## The shape, and why it is cheap
`M_PP_Demichrome` quantizes scene **luminance** into buckets via `Threshold1/2/3` and maps
each bucket to `Palette0..3`, all of which already ride `MPC_Flame` and are pushed per-tick
from `SwarmRenderActor.cpp` (~lines 1061-1081). Sprites render through that pass, so the
world and every unit follow automatically — no sheet is regenerated to try a look. That is
`task-043`'s finding and it holds here; do not re-derive it.

So this is three additions to machinery that already exists:

1. **`Emberkeep.Quantize`** — `0` = bypass, the pass outputs the raw lit scene; `1` = today's
   posterize. Implemented as a lerp/branch at the end of the Custom node, not by disabling
   the post-process volume, so the flame lighting and dither anchoring stay in the picture
   and only the value-collapse goes away.
2. **`Emberkeep.PaletteSteps`** — `[2..8]`, how many values the posterize outputs.
3. **Thresholds and palette entries generalized to N.** `MPC_Flame` gains `Palette4..7`;
   the preset table carries up to 8 entries; the Custom node loops instead of unrolling
   three comparisons.

## The regression guard — non-negotiable
**At `Quantize 1` + `PaletteSteps 4`, the output must be indistinguishable from today.**
`Threshold1/2/3` are owner-tuned (`0.4 / 0.5 / 0.75`) and `DitherBandWidth 0.5` and
`WorldDitherScale 8` are marked `(owner-tuned)` in the exec file — the current look is a
tuned artifact, not a default, and losing it costs real time to recover. Keep
`Threshold1/2/3` authoritative at N=4 and derive evenly-spaced thresholds only for N≠4.
Capture a before/after A/B at N=4 and put it in the handback.

## Done when
- `Emberkeep.Quantize` and `Emberkeep.PaletteSteps` exist as CVars, pushed to `MPC_Flame` in
  the per-tick block that already pushes `Threshold1/2/3` and `Palette0..3`.
- `M_PP_Demichrome` honours both — bypass gives the raw lit scene, steps drives the bucket
  count over `[2..8]`.
- Both rows are in the `/cvars` canonical set with range hints, so they land in
  `SwarmExecOnPlay.txt` and render as bounded Breadboard sliders.
- **`task-043`'s cheapness requirement survives**: adding a new candidate palette is still
  ONE row in `GSwarmPalettePresets` plus one entry in `palette.json`. Presets shorter than
  the current step count pad or clamp gracefully rather than erroring.
- The N=4 A/B capture proving no regression.
- **Proof is a runnable build**, not a diff: PIE, drag the rows, the scene changes.

## Scope fence
The quantizer and its tuning surface only. Explicitly **not** in scope:

- **The per-unit distance layer.** Units are now exempted from the flame lift via
  `Swarm.UnitStencil` (in flight in the working tree, 2026-07-28), and the accepted
  consequence is that they no longer fade with distance. That is a real open visual defect
  and it is *not* this task — do not start it.
- UMG. `EmberkeepPalette.h` hardcodes the four hexes and draws after post, so UI will not
  follow the ramp. That is `task-058`'s job. Say so plainly in the handback.
- Hue separation / the index-buffer path — that is `task-041` (Phase B) and stays parked.

## Working-tree hazard — read this before touching anything
At drafting time the tree carries **~468 lines of uncommitted work** in the exact files this
task owns: `SwarmRenderActor.cpp` (+176), `SwarmRenderActor.h` (+51),
`ELVTR/Content/PostProcess/M_PP_Demichrome.uasset` (modified), and
`ELVTR/Config/DefaultEngine.ini` (`r.CustomDepth=3`).

That work is the `Swarm.UnitStencil` unit-exemption plus a `SwarmBench` config harness. It is
**not yours and it is not stale** — it is the same day's work, and another session may share
this tree. Build on top of it. Do not revert it, do not stash it, do not "clean up" the
diff, and do not attribute it to anyone. If it appears to conflict with what you need to
change, stop and report rather than resolving it unilaterally.

## Spawn prompt
```
You are taking the 4-value colour gate off Emberkeep's demichrome pass, on a toggle
(C:\Projects\ELVTRGAME).

Owner goal, 2026-07-28: "tighten up the UI and visuals in the scene. We can explore options
without the color gate for now. If we can throw it in a toggleable state in the breadboard
that would be good." When asked what "without the color gate" meant, the owner chose:
BYPASS QUANTIZATION ENTIRELY — not just more values, the ability to see the raw lit scene.

WHAT ALREADY EXISTS — do not re-derive any of this:
  - M_PP_Demichrome quantizes scene LUMINANCE into 4 buckets via Threshold1/2/3 and maps
    them to Palette0..3. All seven values ride MPC_Flame and are pushed per-tick from
    SwarmRenderActor.cpp around lines 1061-1081. Copy that pattern.
  - Sprites render THROUGH that pass, so the world and every unit recolour from the post
    pass alone. No sprite sheet needs regenerating to try a look.
  - task-043 built the Emberkeep.Palette preset dial. Read
    docs/backlog/task-043-live-palette-dial-on-the-breadboard.md first — it documents the
    material wiring you are extending.
  - The Breadboard supports Bool/Int/Float rows ONLY (EBreadboardValueType in
    ELVTR/Source/ELVTREditor/Breadboard/BreadboardModel.h). Int and float rows with range
    hints, not colour pickers.

BUILD:
  1. Emberkeep.Quantize  [0..1] — 0 bypasses the posterize and outputs the raw lit scene.
     Do this INSIDE the Custom node (branch/lerp at the end), NOT by disabling the
     post-process volume — the flame lighting and world-anchored dither must stay in the
     picture so only the value-collapse goes away.
  2. Emberkeep.PaletteSteps [2..8] — how many values the posterize outputs.
  3. Generalize to N: MPC_Flame gains Palette4..7, the preset table carries up to 8
     entries, and the Custom node loops instead of unrolling three comparisons.
  4. Add both CVars to the /cvars canonical set with range hints so they land in
     SwarmExecOnPlay.txt and render as bounded Breadboard sliders.

REGRESSION GUARD — NON-NEGOTIABLE: at Quantize 1 + PaletteSteps 4 the output must be
indistinguishable from today. Threshold1/2/3 (0.4/0.5/0.75), DitherBandWidth 0.5 and
WorldDitherScale 8 are marked "(owner-tuned)" in the exec file — the current look is a
tuned artifact, not a default. Keep Threshold1/2/3 authoritative at N=4; derive evenly
spaced thresholds only for N != 4. Capture a before/after A/B at N=4 and include it.

PRESERVE task-043's cheapness requirement: adding a new candidate palette must stay ONE
row in GSwarmPalettePresets plus one entry in docs/data/art/palette.json. Presets with
fewer entries than the current step count must pad or clamp gracefully, never error.

GOTCHAS THAT WILL BITE YOU:
  - MCP set_properties on a Custom node's `code` SILENTLY NO-OPS AND RETURNS TRUE. You are
    editing that exact node's code, extensively. ALWAYS read the code back and compare
    length before recompiling or saving. This has burned this project before.
  - MCP asset edits are in-memory until save_assets([]).
  - Live Coding CANNOT add a UPROPERTY — it reports success then crashes the next PIE.
    CVars and static tables are fine; a new UPROPERTY on ASwarmRenderActor is not, and
    needs a full editor-closed rebuild.
  - ue-iterate compares against the git working tree, not against what is compiled, so its
    Relaunch verdict can be over-cautious when someone has already built.

WORKING-TREE HAZARD — READ BEFORE TOUCHING ANYTHING: the tree carries ~468 lines of
UNCOMMITTED work in the exact files you own — SwarmRenderActor.cpp (+176),
SwarmRenderActor.h (+51), M_PP_Demichrome.uasset, and DefaultEngine.ini (r.CustomDepth=3).
That is the Swarm.UnitStencil unit-exemption plus a SwarmBench harness, from the same day.
Another session may share this tree. BUILD ON TOP OF IT. Do not revert, stash, or tidy it,
and do not attribute it to anyone. If it genuinely blocks you, stop and report.

DO NOT TOUCH:
  - ELVTR/Source/ELVTR/UI/** — UMG draws after post and will NOT follow the ramp. That gap
    is task-058's job. State it plainly in your handback.
  - The per-unit distance layer. Units are exempted from the flame lift via
    Swarm.UnitStencil and consequently no longer fade with distance. Real defect, not
    yours, do not start it.
  - The index-buffer / hue-separation path (task-041, parked).
  - Anything under ELVTR/Source/ELVTR/Mass/**.

You hold the unreal-editor and mcp-9000 locks. Deliver ON-SCREEN EVIDENCE from a runnable
build — PIE, drag the rows, the scene changes. Not a diff plus "it works". The owner
launches the editor for review; you launch it for testing.
```
