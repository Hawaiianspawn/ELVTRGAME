---
id: 039
title: Retire the dead Gatecamp hexes from six art specs, CLASSES.md, and the art-director definition
status: in-progress
agent: pixel-art-director
owns: ["docs/art/hallam.md", "docs/art/edda.md", "docs/art/merle.md", "docs/art/noll.md", "docs/art/warden-captain-bree.md", "docs/art/brees-stairwell.md", "docs/art/aesthetic-direction.md", ".claude/agents/pixel-art-director.md"]
resources: []
depends-on: []
evidence: A grep for every hex in palette.json's retired_hexes list returns hits only inside palette.json, palette-exceptions.md, and clearly-marked history sections.
score: {gate: 3, risk: 2, cost: 2}
source: docs/art/aesthetic-direction.md:556
decided: "2026-07-26 in-progress"
---

## Why now
Six art specs are written against a **retired** palette, and `aesthetic-direction.md`
asserts they are fine. That false all-clear is what makes this urgent — a stale doc you
know about is a nuisance; a stale doc certified as current is a trap.

`aesthetic-direction.md:556-563` claims: *"Update 2026-07-12: all six redrawn, both axes,
in one pass — none of them are stale anymore… cite the current Gatecamp Bright
`#f0c260` shape-only palette throughout."* But `#f0c260` sits in `palette.json`'s
`retired_hexes` list, voided by the reset banner **at the top of that same document,
dated the same day**. The six were updated to Direction B's revised hexes and then
Direction A superseded all of it. Verified by grep, 2026-07-26:

| Retired hex | Was | Still cited in |
|---|---|---|
| `#f0c260` | Gatecamp Bright | all six specs |
| `#211210` | Vault Dark | all six specs |
| `#5e2d20` | Patched Steel | five specs |
| `#c76b2a` | Kitchen Tin | five specs |
| `#f0b84a` | Roll-Gold | **`CLASSES.md:78`** — canon |
| `#1a1c2c` | Vault Dark (older) | **`.claude/agents/pixel-art-director.md`** |

Two of those are the same class of bug as the superseded `WORLD.md` reference in
`gameplay-director.md` (task-018): a dead value inside canon and inside an agent's
system prompt, loaded fresh on every spawn.

This gates sprite production. Anything generated from these specs inherits a palette the
validator will reject.

## Amended 2026-07-26 — cite keys, not hexes
Owner call while reviewing palette strategy: the six specs should reference
`palette.json` **keys** — `dark` / `steel` / `bone` / `pale` — rather than literal hexes,
with the hex values living in `palette.json` alone.

Reason: `docs/RENDERING-LIGHTING.md` §2.1's Phase B turns the palette into LUT data
(task-041), and the whole point of that work is that swapping a ramp shouldn't mean
touching art. If these specs are re-hexed to literals today, the next palette decision
re-opens all six. Naming the role instead makes this the **last** time a palette change
requires an art-spec pass. It is barely more work than the literal substitution.

Where a hex must appear for an artist's benefit, write it as `bone (#a0a08b)` — key
first, hex as a parenthetical, so the key is what the spec is asserting.

## Done when
- All six specs reference the locked ramp **by `palette.json` key** — `dark` / `steel` /
  `bone` / `pale` — not by bare hex. Literal hexes appear only as parentheticals.
- Where a spec used the retired *bright* as a class identity, it moves to the surviving
  mechanism — the shape carriers in `palette.json`: rectangle-flip (Vanguard),
  dot-cluster (Relickeeper), thin-contour (Pathfinder), point+halo (Lampbearer). The
  bright is one hex now; only shape distinguishes it.
- `.claude/agents/pixel-art-director.md` stripped of `#1a1c2c`.
- `aesthetic-direction.md:556-563`'s false all-clear corrected — but the history sections
  stay, per the repo's keep-the-record convention.
- `CLASSES.md:78` is **canon and not yours to edit** — end with a `## Canon proposals`
  section giving the owner the exact replacement.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Six art specs are written against a RETIRED palette while docs/art/aesthetic-direction.md
lines 556-563 certifies them as current. They are not.

The locked ramp (docs/art/aesthetic-direction.md reset banner, 2026-07-12; enforced by
docs/data/art/palette.json) is exactly four hexes:
  #211e20 Dark / #555568 Steel / #a0a08b Bone / #e9efec Pale

Retired hexes still present, verified by grep on 2026-07-26:
  #f0c260 Gatecamp Bright   - all six specs
  #211210 Vault Dark        - all six specs
  #5e2d20 Patched Steel     - five specs
  #c76b2a Kitchen Tin       - five specs
  #1a1c2c Vault Dark (old)  - .claude/agents/pixel-art-director.md AND brees-stairwell.md
  #f0b84a Roll-Gold         - CLASSES.md:78  (CANON - do not edit, propose only)

Read first: docs/art/aesthetic-direction.md (the reset banner lines 8-46 is the
authority; note its own line 556 contradicts it), docs/data/art/palette.json
(retired_hexes and shape_carriers), docs/art/hero-palettes.md, and
docs/art/palette-exceptions.md.

Re-point these seven files at the locked ramp:
  docs/art/hallam.md, edda.md, merle.md, noll.md, warden-captain-bree.md,
  brees-stairwell.md, and .claude/agents/pixel-art-director.md

OWNER AMENDMENT 2026-07-26 — cite KEYS, not bare hexes. Write `bone` / `steel` / `dark` /
`pale` (the palette.json keys), with a literal hex only as a parenthetical where an
artist needs it: `bone (#a0a08b)`. Do NOT scatter bare hex literals through the prose.
Reason: docs/RENDERING-LIGHTING.md §2.1 Phase B (task-041) turns the palette into LUT
data, and the point of that work is that changing a ramp never again means editing art
specs. Re-hexing to literals today would re-open all six on the next palette decision.

Critical: where a spec used the retired BRIGHT as class identity, hue is gone — there is
one bright value now. Identity moves to the shape carriers in palette.json:
rectangle-flip = Vanguard, dot-cluster = Relickeeper, thin-contour = Pathfinder,
point+halo = Lampbearer. Do not invent a substitute hue.

Also correct the false all-clear at aesthetic-direction.md:556-563 — but KEEP the history
sections. This repo keeps the record rather than deleting it (see docs/GDD-TODO.md:104).

Do NOT edit CLASSES.md. End with `## Canon proposals` giving the owner the exact
replacement text for CLASSES.md:78, which currently reads "Roll-Gold #f0b84a".

Note tasks 012 and 013 rework merle.md and noll.md for FICTION. If either has already
landed, re-hex on top of their output rather than reverting it.
```
