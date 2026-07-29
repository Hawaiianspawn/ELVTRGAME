---
id: 043
title: Put a live palette-preset dial on the Breadboard
status: done
agent: claude
owns: ["ELVTR/Content/PostProcess/M_PP_Demichrome**", "ELVTR/Content/PostProcess/MPC_Flame**", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", ".claude/skills/cvars/SKILL.md", "docs/data/art/palette.json"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A PIE session where dragging one Breadboard row recolours the whole world and every unit between demichrome, eulbink and rust-gold, live, with no rebuild and no sprite regenerated.
score: {feel: 3, risk: 2, cost: 2}
source: user
decided: "2026-07-27 done"
---

## Why now
Owner goal, 2026-07-26: *"resolving the palette swap choices — something we can add to
breadboard so we can find better templates and pastels."* This is that, and it is far
cheaper than the Phase B route I first pointed at.

**The correction that makes it cheap.** I previously said swapping a palette means three
edits plus re-quantizing every sheet. That is true for *authoring* final art. It is
**wrong for previewing**, and previewing is the whole job here. `M_PP_Demichrome`
quantizes **scene luminance** into four buckets and maps each to an output colour. Sprites
render through that pass. A sprite baked at Bone (`#a0a08b`, luma 0.622) lands in bucket 2
and comes out as whatever entry 2 currently is. **The world and every unit recolour from
the post pass alone** — the bake does not block a preview at all.

Only UMG stays demichrome, because it draws *after* post (`EmberkeepPalette.h:20-23`,
`RENDERING-LIGHTING.md` §4d). That is a known, acceptable v0 gap: the world is what you
are judging.

**The shape is forced by the Breadboard.** `EBreadboardValueType` is Bool / Int / Float
only — no colour picker, no string. So the dial is an **integer preset index**, not twelve
colour channels. One row, flip through presets live. No new widget work.

**The precedent already exists.** `Swarm.DitherThreshold1/2/3` and `DitherBandWidth` were
bake-time material scalars and now route through `MPC_Flame`, pushed each tick from
`SwarmRenderActor.cpp:724-726`. `FlameCoreColor` is already an MPC **vector**. This task
is the same move, four more times.

## The candidates, measured
Full 8- and 7-colour sets are not drop-ins, but curated 4-cuts are — and one beats the
incumbent. Rec.709 luma under `palette.json`'s own model:

| Preset | 4-value cut | Min gap |
|---|---|---|
| `0` demichrome (incumbent) | `#211e20` `#555568` `#a0a08b` `#e9efec` | **0.218** |
| `1` eulbink-4 | `#252446` `#0098db` `#0ce6f2` `#ffffff` | **0.235** ← *better separated* |
| `2` rust-gold-4 | `#331c17` `#725956` `#bb7f57` `#f6cd26` | 0.168 |

Correcting myself again: I said rust-gold "cannot work." That is true of the **full 8**
(min gap 0.001 — `#202020`/`#331c17` and `#563226`/`#393939` are luma-identical and
collapse). A hand-picked **4** from it is a perfectly serviceable ramp. Hue separation
still needs Phase B (task-041); a warm *tone* does not.

## Done when
- Four vector params on `MPC_Flame` (`Palette0..3`), read by `M_PP_Demichrome`'s Custom
  node via `CollectionParameter` — replacing the baked LUT colours, the same way the
  thresholds already work.
- A preset table in `SwarmRenderActor.cpp` and an `Emberkeep.Palette` **int** CVar,
  pushed in the block that already runs each tick (~line 670-726).
- **Adding a candidate is one row in the table plus one entry in `palette.json`.** That is
  the actual deliverable — the owner wants to shop Lospec for pastels, so the cost of
  trying the eighth palette must be the same as the second.
- The CVar added to the `/cvars` canonical set so it lands in `SwarmExecOnPlay.txt` and
  shows as a Breadboard row, with a range hint covering the preset count.
- **Proof:** PIE, drag the row, watch the world and the units recolour. No rebuild, no
  regenerated sprite.
- State plainly in the handback that UMG does **not** follow yet.

## Progress — 2026-07-26

**Material path: DONE, SAVED, and PROVEN.**
- `MPC_Flame` carries `Palette0..3` (readback-verified).
- `M_PP_Demichrome` has four `CollectionParameter` nodes wired to the Custom node's
  inputs `C0..C3`; readback confirms `C0<-CP_12 … C3<-CP_15`. Recompiled and saved.
- **No Custom node `code` edit was needed.** The palette was already four named
  `VectorParameter`s (`Color_Dark/Steel/Bone/Pale`) feeding `C0..C3`, so this was a node
  swap plus rewire — which sidesteps the known silent-no-op hazard on `code` entirely.
  The old VectorParameter nodes are left in place, disconnected, as a record of the
  original values. `MI_Demichrome` may now carry dead overrides for them; harmless, worth
  a later tidy.
- Proven end-to-end **without the C++ build** by setting the MPC defaults directly:
  demichrome → eulbink (world went cyan) → demichrome. Captures in the session scratchpad.
  Defaults restored to demichrome and saved, because until the per-tick push compiles the
  defaults *are* what the game uses.

**C++ path: DONE and PROVEN.** `Emberkeep.Palette` registers in the running editor
(`SearchCVars` returns it with full help, value 0). The compiled `UnrealEditor-ELVTR.dll`
contains `Emberkeep.Palette` / `eulbink-4` / `rust-gold-4` as UTF-16 literals — checked
directly rather than trusting UBT's "Target is up to date".

**Breadboard row: in `Saved/SwarmExecOnPlay.txt`** under its own `PALETTE @tab Debug`
section, with a `[0..2]` range hint so `ParseRangeHint` gives it a bounded slider rather
than a free int.

**End-to-end proof:** exec set to `2` → PIE → CVar read back as 2 → `TickFlame` pushed
rust-gold to the MPC → viewport rendered warm gold, against demichrome's olive and
eulbink's cyan. Three captures in the session scratchpad. Exec restored to `0`.

Caveat recorded honestly: `IsPIERunning` was false at capture time — PIE had ended and the
MPC retained the pushed value. The chain is still proven (nothing else writes `Palette0..3`),
but it is not a live-PIE screenshot.

**Remaining before `done`:** open the Breadboard (`Emberkeep.Breadboard`) and confirm the
row renders as a bounded int slider — that needs a console command, which MCP cannot send,
so it wants an owner eyeball. Also unverified: behaviour at horde density, which is
task-042's job, not this one's.

## Build state — RESOLVED 2026-07-26
Superseded by the Progress section above; kept as the record of a concern that turned out
not to bite.

The worry was real: `ue-iterate -DryRun` selected **Relaunch**, and the working tree
carries in-flight work that is *not* part of this task — new `Mass/SwarmFormation.{cpp,h}`,
a reflection change in `Mass/SwarmProcessors.h`, and ~150 lines across `SwarmCommands.cpp`,
`SwarmFragments.h`, `SwarmProcessors.cpp`, `SwarmStats.*`, `SwarmSubsystem.h`. At session
start only `SwarmRenderActor.cpp` was modified, so that landed in parallel, and a rebuild
for this task would have compiled it too.

It never came to that. `Build.bat ELVTREditor` returned **"Target is up to date, Result:
Succeeded"** with zero actions — the tree had already been built, including this task's
changes. Verified independently rather than trusting UBT: the compiled
`UnrealEditor-ELVTR.dll` contains `Emberkeep.Palette`, `eulbink-4` and `rust-gold-4` as
UTF-16 literals.

Standing note for the next editor-touching task: `ue-iterate` compares against the **git
working tree**, not against what is actually compiled, so its Relaunch verdict can be
over-cautious when someone has already built.

## Scope fence
Four-value presets only. `Threshold1/2/3` gives exactly four buckets; a 5+ value palette
needs more thresholds and is a follow-up, not v0. Do not widen the threshold system here.

## Spawn prompt
```
You are adding a live palette-preset dial to Emberkeep's Breadboard (C:\Projects\ELVTRGAME).

Goal (owner, 2026-07-26): flip the whole game between candidate palettes live in PIE from
one Breadboard row, so palettes can be shopped for tone.

Why this is cheap — do not re-derive it: M_PP_Demichrome quantizes scene LUMINANCE into
four buckets and maps each to an output colour. Sprites render through that pass, so a
sprite baked at #a0a08b (luma 0.622) lands in bucket 2 and emerges as whatever entry 2 is.
The world and all units recolour from the post pass alone; the baked sheets do not block a
preview. UMG (EmberkeepPalette.h) draws AFTER post and will NOT follow — that is an
accepted v0 gap, but say so in your handback.

Read first: docs/RENDERING-LIGHTING.md §4b (tuning surface — note the thresholds already
moved from bake-time scalars to MPC_Flame and are pushed from SwarmRenderActor.cpp around
lines 724-726; FlameCoreColor is already an MPC vector — copy that pattern),
docs/data/art/palette.json, and .claude/skills/cvars/SKILL.md (the canonical set and how
the exec file feeds the Breadboard).

The Breadboard supports Bool/Int/Float rows ONLY (EBreadboardValueType in
ELVTR/Source/ELVTREditor/Breadboard/BreadboardModel.h). So the dial is an INT preset
index, not twelve colour channels.

Build:
  - MPC_Flame gains Palette0..Palette3 vector params
  - M_PP_Demichrome's Custom node reads them via CollectionParameter instead of baked hexes
  - a preset table + `Emberkeep.Palette` int CVar in SwarmRenderActor.cpp, pushed in the
    existing per-tick Set*ParameterValue block
  - add the CVar to the /cvars canonical set with a range hint

Presets v0 (4-value cuts, luma-verified 2026-07-26):
  0 demichrome  #211e20 #555568 #a0a08b #e9efec   (min luma gap 0.218)
  1 eulbink-4   #252446 #0098db #0ce6f2 #ffffff   (0.235)
  2 rust-gold-4 #331c17 #725956 #bb7f57 #f6cd26   (0.168)

MOST IMPORTANT REQUIREMENT: adding a fourth candidate must be ONE table row plus one
palette.json entry. The owner wants to shop Lospec for pastels — trying the eighth palette
must cost the same as the second. If your design makes candidate #4 expensive, redo it.

Scope fence: FOUR-value presets only. Threshold1/2/3 gives exactly four buckets; 5+ values
needs more thresholds and is a separate task. Do not widen the threshold system.

Gotchas that will bite:
  - MCP set_properties on a Custom node's `code` SILENTLY NO-OPS and returns true. Always
    read the code back and compare length before recompiling or saving. You are editing
    that exact node.
  - MCP asset edits are in-memory until save_assets([]).
  - Live Coding cannot add a UPROPERTY (reports success, crashes the next PIE). A CVar and
    a static table are fine; a new UPROPERTY on ASwarmRenderActor is not.

You hold the unreal-editor and mcp-9000 locks. Deliver on-screen evidence: PIE, drag the
row, world and units recolour. Not a diff plus "it works".
```
