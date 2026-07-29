---
id: 016
title: Write GDD §12 Q6's resolution back into the table
status: proposed
agent: claude
owns: ["GDD.md"]
resources: []
depends-on: []
evidence: GDD.md §12 Q6 reads as decided, citing aesthetic-direction.md's 2026-07-12 reset, and no doc still describes the palette question as open.
score: {feel: 1, risk: 1, cost: 1}
source: GDD.md:430
decided: ""
---

## Why now
**Corrected 2026-07-26.** This task was originally filed as "settle a live contradiction
between two canon docs." That was wrong, and the correction matters more than the task.

Q6 is **not open**. `docs/art/aesthetic-direction.md` line 8 records the owner's verbatim
directive of 2026-07-12 — *"I want to use the 2bit Demichrome as the entire color
palette, no other variations unless I explicitly overwrite it"* — and lines 20 and 569
both state the consequence explicitly: **"GDD dependency #6 is now RESOLVED: strict
global palette."** Direction B is superseded, Direction C is dead, and
`docs/data/art/palette.json` has enforced the single ramp in the pipeline since.

`GDD.md:430` still shows `Per-faction | Open`. It is a stale row, not a second opinion —
the identical failure to `GDD-TODO.md:46`'s "name the game" blocker, which `GDD.md:433`
had already answered. Nothing is blocked by it. It just misleads anyone who reads the
Open Questions table as current, which is what it is for.

Scored low deliberately: this is a record fix, not a decision. The decision was made
fourteen days ago.

## Done when
- `GDD.md` §12 Q6 reads as decided: strict global 4-value Demichrome, dated 2026-07-12,
  citing `docs/art/aesthetic-direction.md`'s reset banner.
- The row notes that any faction-reserved value or palette swap now needs an explicit
  owner exception, logged in `docs/art/palette-exceptions.md`.
- No design decision is made or revisited. If the write-back surfaces another table row
  that canon has already answered, note it — do not fix it here.

## Spawn prompt
```
You are correcting a stale row in canon for Emberkeep (C:\Projects\ELVTRGAME).

GDD.md §12 Q6 (line ~430) shows the palette question as "Per-faction | Open". It is not
open. docs/art/aesthetic-direction.md line 8 records the owner's 2026-07-12 directive
locking Direction A — a strict GLOBAL 4-value 2-bit Demichrome palette for the entire
game — and lines 20 and 569 state that this RESOLVES GDD dependency #6.
docs/data/art/palette.json has enforced that single ramp in the pipeline since.

Read aesthetic-direction.md's reset banner (lines 8-46) and its "Depends on" section
(line ~569), plus palette.json, before touching anything.

Edit ONLY GDD.md §12's Q6 row: mark it decided 2026-07-26 with the 2026-07-12 decision
date, state the answer as strict global palette, and cite
docs/art/aesthetic-direction.md. Add that any faction-reserved value or palette swap now
requires an explicit owner exception logged in docs/art/palette-exceptions.md.

Do NOT make or revisit a design decision — you are transcribing one that already
happened. Do not edit aesthetic-direction.md, palette.json, CLASSES.md, or any art spec;
the dead-hex cleanup those need is task-039 and it owns those files.

While you are in the §12 table, check the other non-decided rows against current canon
and REPORT any that are similarly stale. Do not fix them in this task.
```
