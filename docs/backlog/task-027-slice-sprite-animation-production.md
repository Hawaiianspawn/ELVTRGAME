---
id: 027
title: Produce the slice sprite set — Vanguard, retinue, 5 enemies, walk/attack/death each
status: proposed
agent: claude
owns: ["docs/data/art/requests/**", "RawArt/Renders/**", "ELVTR/Content/Sprites/**"]
resources: ["pixellab-credits", "unreal-editor", "mcp-9000"]
depends-on: [9, 11, 15, 39]
evidence: Imported UE sprite sheets that pass the palette checker, plus the provenance manifest recording every generation UUID and its credit cost.
score: {gate: 2, risk: 2, cost: 4}
source: docs/RTS-VERTICAL-SLICE.md:110
decided: ""
---

## Why now
The single largest art-production item and the one that spends real money. It depends on
four things because getting the order wrong is expensive: the flipbook-vs-3D decision
(015) determines whether animation frames are even the right deliverable, the palette
strategy (016) determines the target register, and the two acceptance passes (009, 011)
determine whether the existing anchors are good enough to rotate from.

That last one matters most. The `/sprite` pipeline's measured finding is that v3
reference rotation is a faithful **rotator, not a renderer** — the anchor's quality is the
hard ceiling for the entire sheet. Generating rotations and animations from an anchor that
fails its acceptance checklist burns credits producing a sheet that will be rejected.

Holds the `pixellab-credits` lock: no other generating task runs alongside it.

## Done when
- Vanguard hero and retinue sprites, plus five enemy sprites — walk, attack, and death
  for each.
- Every sheet on the locked register, verified by `pixelpipe.py quantize`, not by eye.
- Every generation recorded in the provenance manifest with its UUID and cost. PixelLab
  gives no seed, so the manifest is the only route back to a sprite for iteration —
  losing it means losing the ability to revise.
- All raw generations retained in `RawArt/Renders/` — nothing deleted before a keep/reject
  decision.
- Imported into UE and visible in game, not just sitting on disk.

## Spawn prompt
```
You are producing Emberkeep's slice sprite set (C:\Projects\ELVTRGAME). This task SPENDS
REAL MONEY — you hold the pixellab-credits lock.

Use the /sprite skill. It owns this chain end to end: schema-validated request in
docs/data/art/requests/ → anchor → palette enforcement → rotation → animation → SubUV sheet
packing → UE import → provenance manifest. Read it before doing anything.

Check your dependencies first and STOP if any is unmet:
- task-015 (flipbooks vs 3D) — determines whether animation frames are the right deliverable
- task-016 (palette strategy) — determines the target register
- task-009 and task-011 (acceptance passes on the Vanguard and soldier anchors)

That last pair is the expensive one to skip. The measured finding in the /sprite skill is
that v3 reference rotation is a faithful ROTATOR, not a renderer: the anchor is the hard
quality ceiling for the whole sheet. Rotating from an anchor that failed its checklist
burns credits on a sheet that will be rejected.

Deliver: Vanguard hero + retinue, plus 5 enemy sprites, with walk/attack/death each
(docs/RTS-VERTICAL-SLICE.md:110).

Non-negotiable house rules: every generation goes to RawArt/Renders/ first and is NEVER
deleted before a keep/reject decision. Every generation is recorded in the provenance
manifest with UUID and cost — PixelLab provides no seed, so the manifest is the only way
back to a sprite for iteration.

Verify the palette with Scripts/art/pixelpipe.py quantize, not by eye. Import into UE and
show them in game. Report total credits spent.
```
