---
id: 039
title: Retire the dead Gatecamp hexes from six art specs, CLASSES.md, and the art-director definition
status: done
agent: pixel-art-director
owns: ["docs/art/hallam.md", "docs/art/edda.md", "docs/art/merle.md", "docs/art/noll.md", "docs/art/warden-captain-bree.md", "docs/art/brees-stairwell.md", "docs/art/aesthetic-direction.md", ".claude/agents/pixel-art-director.md"]
resources: []
depends-on: []
evidence: A grep for every hex in palette.json's retired_hexes list returns hits only inside palette.json, palette-exceptions.md, and clearly-marked history sections.
score: {feel: 3, risk: 2, cost: 2}
source: docs/art/aesthetic-direction.md:556
decided: "2026-07-28 done"
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

CORRECTION 2026-07-28 (lead session) — READ THIS, IT REVERSES PART OF THE ABOVE.
This task was dispatched 2026-07-26. Two days later the owner reversed the colour gate.
`docs/art/aesthetic-direction.md` now opens with an **AMENDMENT 2026-07-28: the strict
4-value global palette is SUPERSEDED. Colour is back.** `Emberkeep.Quantize` is 0; the
game ships full colour, full range. The 4-value table is explicitly kept as HISTORY:
*"do not cite it as current, and do not delete it."*

What that changes about your job:

- **STILL DO** the key-citation work. Replace retired hex LITERALS with palette KEYS.
  This is now MORE valuable, not less — it is exactly the decoupling that stops a palette
  change from meaning a six-file edit, which just happened for the second time.
- **DO NOT** write "hue is gone", "there is one bright value", or anything that treats the
  4-value ramp as the current standard.
- **DO NOT** force class identity onto the shape carriers as a *replacement* for hue. Hue
  is available again. The shape carriers remain a legitimate identity channel — keep any
  existing shape-carrier language — but do not strip colour identity out to satisfy a
  constraint the owner has lifted.
- **DO NOT invent** the new colour standard. There isn't one written down yet. Where a spec
  needs a hue decision you cannot make from existing canon, FLAG it in a
  `## Open — needs a colour decision` section at the end of your handback. Flagging is the
  deliverable there, not resolving.

Also correct the false all-clear at aesthetic-direction.md:556-563 — but KEEP the history
sections. This repo keeps the record rather than deleting it (see docs/GDD-TODO.md:104).

Do NOT edit CLASSES.md. End with `## Canon proposals` giving the owner the exact
replacement text for CLASSES.md:78, which currently reads "Roll-Gold #f0b84a".

Note tasks 012 and 013 rework merle.md and noll.md for FICTION. If either has already
landed, re-hex on top of their output rather than reverting it.

STALE REFERENCES IN THIS PROMPT — verified by the lead 2026-07-28:
- The line numbers "aesthetic-direction.md:556-563" are WRONG now. The 2026-07-28
  amendment added ~35 lines at the top and pushed everything down. Find the false
  all-clear by CONTENT ("Update 2026-07-12: all six redrawn"), not by line number.
- `docs/data/art/palette.json` is dated 2026-07-25 and still encodes the superseded
  4-value lock. Its own note says the markdown wins and that a disagreement means the
  JSON is the bug — so it currently IS the bug. It is NOT in your owns list. Do not edit
  it. Flag it in your handback.
- `docs/art/hero-palettes.md` also contains all four retired hexes and is NOT in your
  owns list. Do not edit it. Flag it.
- Those six specs are ALSO flagged stale on a second axis — aesthetic-direction.md has an
  unactioned note that they need a chibi-proportion revision pass. That is OUT OF SCOPE
  here. Do not attempt it; just don't write anything that contradicts it.

A prior teammate on this task did not survive its session and appears to have completed
none of the work — all four retired hexes were still present in all six specs when the
lead re-checked on 2026-07-28. Start from the current file contents, not from an
assumption that some of it is already done.
```
