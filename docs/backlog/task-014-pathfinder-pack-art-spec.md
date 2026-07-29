---
id: 014
title: "Write the Pathfinder's Pack retinue art spec (brief-007)"
status: proposed
agent: pixel-art-director
owns: ["docs/art/pathfinder-pack.md", "docs/briefs/brief-007-pathfinder-pack-rework.md"]
resources: []
depends-on: [12]
evidence: A new docs/art/pathfinder-pack.md covering hawk, hounds, and scouts, and brief-007 flipped to status done pointing at it.
score: {feel: 1, risk: 1, cost: 2}
source: docs/briefs/brief-007-pathfinder-pack-rework.md
decided: ""
---

## Why now
Unlike tasks 012 and 013, this is **new scope, not a correction** — the brief says so
explicitly. No sprite work exists for individual Pack members; brief-004 specced only
the hero. The brief was filed to stop the gap being lost, and it has been sitting
pending ever since, which is precisely the losing it was meant to prevent.

Depends on task-012 because the Pack is found-family to a hero whose own visual identity
is currently stale — speccing strays who match a superseded Pathfinder wastes the pass.

Note the naming asymmetry worth preserving: companions keep proper names (**Relay** the
signal-bird, hounds **Latch** and **Ash**); only the four player classes are nameless.

## Done when
- `docs/art/pathfinder-pack.md` covers the hawk, the two hounds, and the human scouts.
- Every member reads as a **stray** — survivors who found each other — not as inherited
  royal-hunt stock. Relay is an ex-Legion signal-bird; the hounds are feral mongrel
  descendants of Legion war-dogs.
- Readability answer for the Pack inside a horde: these are friendly units among hundreds,
  and the militia's rules (task-010) do not automatically cover animals.
- `brief-007` frontmatter: `status: done`, `spec: ../art/pathfinder-pack.md`.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Process docs/briefs/brief-007-pathfinder-pack-rework.md, which is status: pending. This
is NEW scope — no sprite work exists for individual Pack members yet.

Read it in full, plus docs/art/merle.md (the Pathfinder hero spec — reworked in task-012,
which this depends on), CLASSES.md §3 for the Pack's gameplay role,
docs/art/aesthetic-direction.md (Direction A LOCKED: 4-value Demichrome),
docs/art/retinue-militia.md as the precedent for how a retinue is specced,
docs/data/art/palette.json, and docs/narrative/FLAME-FOUNDATION.md.
Do NOT read WORLD.md — superseded.

Write docs/art/pathfinder-pack.md covering: the hawk Relay (a former Legion signal-bird,
her old service designation kept as her name — not a hunting hawk), the hounds Latch and
Ash (feral mongrel descendants of Legion war-dogs, not kennel breed stock), and the human
scouts (fellow war-orphans). Everything must read as strays who found each other, never
as inherited stock.

Give an explicit readability answer for animals inside a horde — the militia rules in
docs/art/retinue-militia.md were written for humanoids and do not carry over unexamined.

Then set brief-007 to status: done with spec: ../art/pathfinder-pack.md.

Write ONLY those two files. Written specs only — no image files, no mcp__pixellab__* calls.
```
