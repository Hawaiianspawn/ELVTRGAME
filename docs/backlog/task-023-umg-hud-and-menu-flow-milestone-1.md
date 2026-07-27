---
id: 023
title: Build UMG milestone 1 — main menu → muster → combat HUD
status: proposed
agent: claude
owns: ["ELVTR/Content/UI/**", "docs/ui/UI-PROTOTYPE-PLAN.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A runnable build showing the full flow — Sampler Frame main menu → Muster roster → combat HUD collapse — captured on screen, not described.
score: {gate: 2, risk: 2, cost: 4}
source: docs/RTS-VERTICAL-SLICE.md:99
decided: ""
---

## Why now
The plan is already written and explicitly a handoff doc: `docs/ui/UI-PROTOTYPE-PLAN.md`
says *"a fresh session should be able to start implementation from here without
re-deriving context."* Platform is decided (UMG-native, owner decision 2026-07-23), scope
is decided (full flow, not just HUD), and the component inventory in
`docs/ui/menu-frame-system.md` §3 and §7 is called the spine of the plan.

What exists in game today is the prototype HUD hardcoded in `SpikeHeroPawn.cpp` — phase,
HP, counts, stance as debug text. `docs/RTS-VERTICAL-SLICE.md:99` wants the real one.

Largest task on the board and the only one scored cost 4. It holds both the editor and
MCP locks, so nothing else touching the editor can run alongside it.

## Done when
- The full flow runs: Sampler Frame main menu → Muster roster → combat HUD collapse.
- Built natively in UMG. 8bitcn is an asset source and aesthetic reference, not a runtime.
- The combat HUD covers what the prototype covers — phase, hero HP, retinue/brood counts,
  stance, leash-broken count — without regressing readability.
- Handed over as a **runnable build with on-screen evidence**, per house rule. A diff and
  a written "it works" is not a delivery.
- `UI-PROTOTYPE-PLAN.md` updated with what actually got built vs. what the plan assumed.

## Spawn prompt
```
You are implementing Emberkeep's first real UI (C:\Projects\ELVTRGAME).

docs/ui/UI-PROTOTYPE-PLAN.md is a deliberate handoff doc — start there and follow it. It
records owner decisions from 2026-07-23: platform is UMG-native (no web/React), and
milestone 1 scope is the FULL flow (main menu → muster → combat HUD), not HUD alone.

Also read: docs/ui/menu-frame-system.md §3 (component inventory, 8bitcn origin →
Demichrome) and §7 (UMG translation) — the plan calls these its spine —
docs/art/aesthetic-direction.md (Direction A LOCKED: strict 4-value Demichrome),
docs/data/art/palette.json, and ELVTR/Source/.../Spike/SpikeHeroPawn.cpp for the prototype
HUD you are replacing (phase, hero HP, retinue/brood counts, stance, leash-broken count).

You hold the unreal-editor and mcp-9000 locks for this task — no other task may drive the
editor while this runs.

Two gotchas that will bite: adding a UPROPERTY via Live Coding reports success and then
crashes the next PIE — class-layout changes need a full editor-closed rebuild. And
unreal-mcp asset edits are in-memory until save_assets([]).

Deliver a RUNNABLE BUILD with on-screen evidence of the full flow. A diff plus "it works"
is not a delivery in this project. Update docs/ui/UI-PROTOTYPE-PLAN.md with what was
actually built versus what the plan assumed.

Write only under ELVTR/Content/UI/ and docs/ui/UI-PROTOTYPE-PLAN.md, plus whatever C++ the
HUD genuinely requires — flag any source changes clearly in your handback.
```
