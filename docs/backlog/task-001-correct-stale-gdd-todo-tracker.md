---
id: 001
title: Correct the stale blockers in GDD-TODO.md and point it at the backlog
status: in-progress
agent: claude
owns: ["docs/GDD-TODO.md"]
resources: []
depends-on: []
evidence: GDD-TODO.md Part A shows no false blockers; a diff showing each corrected line against the canon line that settles it.
score: {feel: 1, risk: 1, cost: 1}
source: docs/GDD-TODO.md:46
decided: "2026-07-31 in-progress"
model: sonnet
teammate: gdd-todo-retry
---

## Why now
`GDD-TODO.md` is the doc anyone opens to ask "what is left", and it is lying. Line 46
still flags 🔴 **Name the game** as a blocker on Game Specificity; `GDD.md:433` records
Q10 as decided on 2026-07-21 — the game is **Emberkeep**. Line 48 flags the win/loss
condition as a second 🔴 blocker; that needs the same check against current canon.
Part A was scoped to an assignment due 21 July, which has passed, so the whole section
is being read five days out of date.

Nothing breaks in the build. What breaks is trust in the tracker, and every plan made
from it starts from a false premise.

## Done when
- Every Part A item is checked against `GDD.md`, `CLASSES.md`, and
  `docs/narrative/FLAME-FOUNDATION.md`, and either ticked with a pointer to the canon
  line that settles it, or left open with a note saying it was re-verified today.
- The two 🔴 blockers are resolved or re-justified explicitly.
- A header points at `docs/backlog/INDEX.md` as the live queue, with GDD-TODO kept as
  the assignment-scoped record it actually is.
- No design decisions are made in this task. It corrects the record, nothing else.

## Spawn prompt
```
You are correcting a stale tracker in the Kindled repo (C:\Projects\ELVTRGAME).

docs/GDD-TODO.md was written 2026-07-21 for an assignment due that night. It has not
been updated since, and it now contradicts canon. Confirmed example: line 46 lists
"Name the game" as a red blocker, but GDD.md line 481 records Q10 as re-decided on
2026-07-27 — the game is named KINDLED. "Emberkeep" is the DISCARDED working title;
GDD-TODO.md's Part A blocker table and its parked-candidates list are both stale on
this point. Never re-introduce Emberkeep as the name.

Go through every unchecked item in Part A and Part B. For each one, check it against
GDD.md (especially the §12 Open Questions table at line 411), CLASSES.md, SYSTEMS.md,
and docs/narrative/FLAME-FOUNDATION.md. Note that WORLD.md is SUPERSEDED by the
2026-07-22 narrative reset — do not treat it as canon.

Then edit docs/GDD-TODO.md only:
- tick items that canon already settles, each with a pointer to the file:line proving it
- leave genuinely open items unchecked, annotated "re-verified 2026-07-26"
- add a header noting that docs/backlog/INDEX.md is now the live work queue, and that
  GDD-TODO.md remains the assignment-scoped record

Do not edit GDD.md, SYSTEMS.md, CLASSES.md, or any other file. Do not make design
decisions — if an item is genuinely unresolved, it stays unresolved. Hand back a list
of every correction with the canon line that justified it.
```
