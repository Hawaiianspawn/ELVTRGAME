---
id: 011
title: Run the soldier roster v1 acceptance checklist across all six variants
status: proposed
agent: pixel-art-director
owns: ["docs/art/soldier-roster-v1.md"]
resources: []
depends-on: []
evidence: All ten checklist items at docs/art/soldier-roster-v1.md:1093-1104 ticked or failed across variants 01-06, with per-variant quantize reports.
score: {feel: 1, risk: 1, cost: 2}
source: docs/art/soldier-roster-v1.md:1093
decided: ""
---

## Why now
The widest unrun gate of the three: ten items across six variants, including
per-variant rules (zero Pale on 01/02/03/04/06; exactly 16 contiguous bounded Pale
pixels below head height on 05) and a global one that reads like a house rule worth
enforcing — *every eye in the set is a Dark notch, no pale eyes, anywhere, ever*.

It also gates the ASCII anchor blocks: exactly 48 lines of exactly 48 characters, only
`.#Sb@`. That is a mechanical check a script settles in seconds, and if it fails, every
downstream rotation inherits the fault — the `/sprite` pipeline notes that v3 reference
rotation is a faithful *rotator*, so anchor quality is the ceiling for the whole sheet.

## Done when
- All ten items checked per variant where the item is per-variant.
- Anchor blocks validated mechanically (48×48, charset `.#Sb@` only).
- `quantize --stage anchor` reporting 100% on-palette with strict alpha, per variant.
- The `dominant` vs `canon.value_dominance` match checked for 01, 03, 04, 05, and the
  02/06 exception handled as the doc specifies.
- Failures written up per variant. No regeneration.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Run the acceptance checklist at docs/art/soldier-roster-v1.md:1093-1104 across all six
soldier variants. This is a verification pass — you are NOT regenerating art.

Read docs/art/soldier-roster-v1.md in full (it is long — the checklist is at the end),
plus docs/art/aesthetic-direction.md and docs/data/art/palette.json.

Mechanical checks first, with a throwaway script in your scratchpad if it helps:
- each ASCII anchor block is exactly 48 lines of exactly 48 chars, charset .#Sb@ only
- Scripts/art/pixelpipe.py quantize --stage anchor per variant: 100% on-palette,
  alpha strictly 0 or 255
- reported `dominant` vs canon.value_dominance for 01, 03, 04, 05 (02 and 06 have a
  documented exception — follow what the doc says)
- zero Pale on 01, 02, 03, 04, 06; variant 05 gets exactly 16 contiguous Pale pixels,
  bounded on all four sides, below head height
- every eye in the set is a Dark notch — no pale eyes anywhere

Do NOT call any mcp__pixellab__* generation tool. This task holds no pixellab-credits
lock and generating costs real money.

Tick each item with its measurement or mark it FAILED with what you measured. Write ONLY
docs/art/soldier-roster-v1.md. Findings for failures; no fixes.
```
