---
id: 040
title: Replace the literal white hit-flash in the Unit Cam panel with Demichrome Pale
status: parked
agent: claude
owns: ["ELVTR/Source/**/UnitCamProjector.cpp"]
resources: []
depends-on: []
evidence: UnitCamProjector.cpp:857 uses Demichrome Pale, and a screenshot of a hit-flash on the panel shows no value brighter than the ramp's top.
score: {feel: 2, risk: 1, cost: 1}
source: docs/art/palette-exceptions.md:45
decided: "2026-07-27 parked"
---

## Why now
The one place in shipping UI where the locked ramp has a silent hole, and the fix is
already written down.

`docs/art/palette-exceptions.md` traced three claimed exceptions and adjudicated each.
Two came back clean. This one did not: `UnitCamProjector.cpp:857` sets
`B.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha)`, citing the world renderer's
"light-exempt for the same reasons" comment. The ledger found that reasoning **does not
transfer** — the world renderer's white is a reliable-input trick that gets quantized by
`M_PP_Demichrome` to Pale anyway, but the Unit Cam panel is UMG, drawn *after* post
by design (`docs/RENDERING-LIGHTING.md` §4d). Nothing downstream quantizes it, so it
renders as literal unfiltered white: a genuine fifth value.

The ledger's standing rule settles it — sanction a fifth value only when the requirement
is *"brighter than Pale"* (the flame core's real case), not merely *"bright"*. Hit-flash
has nothing to outshine; Pale is already the ramp's designated register for the brightest
thing a unit can be.

Filed as its own task because the ledger's author does not edit `.cpp` and flagged the
exact replacement for whoever does.

## Done when
- `UnitCamProjector.cpp:857` uses `FLinearColor::FromSRGBColor(FColor(0xE9, 0xEF, 0xEC))`.
- Verified on screen: a hit-flash on the panel shows nothing brighter than Pale. At ~92%
  vs 100% luminance over a 0.10s `Swarm.HitFlashTime`, no perceptible difference in play —
  if it looks different, say so rather than reverting silently.
- Worth doing while in there, and called out by the ledger: several files hand-roll
  `#e9efec` independently. A shared `FColor DemichromePale` constant would stop the next
  one drifting. Propose it; do not sprawl the change.
- The world renderer's misleading comment at `SwarmRenderActor.cpp:388` is **not** in
  scope — the ledger ruled it functionally correct, only mis-self-described, and
  `TickDebugRender` is being retired anyway.

## Spawn prompt
```
You are fixing a palette violation in Emberkeep (C:\Projects\ELVTRGAME).

Read docs/art/palette-exceptions.md FIRST — it contains the full adjudication and the
exact fix. Summary: UnitCamProjector.cpp:857 sets
  B.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha)
The Unit Cam panel is UMG, drawn AFTER post-processing (docs/RENDERING-LIGHTING.md §4d),
so nothing quantizes it and this renders as literal pure white — a real fifth value in a
panel that is live today. The locked ramp tops out at Demichrome Pale #e9efec.

Replace it with:
  FLinearColor::FromSRGBColor(FColor(0xE9, 0xEF, 0xEC))

Then verify on screen — PIE, trigger a hit-flash on the panel, confirm nothing renders
brighter than Pale. Hand back a screenshot; this project takes on-screen evidence, not a
diff plus "it works".

Do NOT touch SwarmRenderActor.cpp:388. The ledger ruled that one functionally correct
(its white is quantized to Pale by M_PP_Demichrome regardless) and TickDebugRender is
being retired anyway.

Gotcha: adding a UPROPERTY via Live Coding reports success and then crashes the next PIE.
This change is a value edit inside a function body, so Live Coding is safe —
Scripts/ue-iterate.ps1 will pick the right path.

Several files hand-roll #e9efec independently. Propose a shared FColor DemichromePale
constant in your handback, but do not refactor every call site in this task.
```
