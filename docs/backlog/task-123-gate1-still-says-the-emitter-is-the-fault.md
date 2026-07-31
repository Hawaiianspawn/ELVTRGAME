---
id: 123
title: Retire the same stale emitter claim from GATE1, which sits 30 lines under its own FIXED heading
status: proposed
agent: performance-director
model: ""
owns:
  - "docs/GATE1-FUN-PROTOTYPE.md"
resources: []
depends-on: []
epic: ""
evidence: The zero-draw / "the fault" grep over docs/GATE1-FUN-PROTOTYPE.md returns nothing outside a clearly-marked historical section — the same bar task-019 was held to on docs/perf/niagara-sprite-refactor.md.
score: {feel: 1, risk: 1, cost: 1}
source: task-019 handback, 2026-07-31
teammate: ""
decided: ""
---

## Why now

task-019 retired the "NS_Swarm emitter graph is the fault" claim from
`docs/perf/niagara-sprite-refactor.md`. It was scoped to two files. The rot is in three.

`docs/GATE1-FUN-PROTOTYPE.md` carries the same claim, and carries it in the worst
possible arrangement — **about thirty lines below its own correction**:

- **:135** `### Sprite path status 2026-07-26 — **FIXED. The emitter graph was not the fault.**`
  followed by the correct cause (`SimTarget: GPUComputeSim` → `CPUSim`).
- **:167** the status table still ends `| **NS_Swarm emitter graph** | — | **the fault** |`
- **:168-170** `"No Niagara errors are logged; the emitter simply produces no visible
  particles. This is the §3a problem above, still live"`

A reader who lands on the table believes the emitter is broken. A reader who lands on the
heading believes it is fixed. Both are reading the same document, and the second one is
right. The task-019 teammate confirmed the finding independently and correctly refused to
touch it — `GATE1-FUN-PROTOTYPE.md` was not in its `owns:`.

This is the fourth time a stale render claim has cost a teammate real time
(`niagara-sprite-root-cause-gpu-sim`; several backlog task files carry a hand-written
"do not repeat this" warning because the docs could not be trusted). The fix is cheap
and the cost of leaving it is paid repeatedly.

## Done when

- The grep bar above passes on `docs/GATE1-FUN-PROTOTYPE.md`.
- The table row and the "still live" prose are struck or marked as superseded, pointing
  at the `FIXED` heading already in the same file — the originals kept as the record,
  same convention task-019 followed and `GDD-TODO.md:104` established.
- `ELVTR/SETUP-EDITOR.md` is checked and either confirmed clean or fixed the same way.
  It is named alongside the other two in the standing note about this claim, but a grep
  on 2026-07-31 found no surviving hits — verify rather than assume, and say which.

## Spawn prompt

```
You are executing task-123. You are the performance-director for Kindled
(C:\Projects\ELVTRGAME).

Established fact, do not re-derive it: NS_Swarm drew nothing because the Swarm emitter's
SimTarget was GPUComputeSim. Switching it to CPUSim fixed it. The emitter module graph
was NEVER at fault. docs/GATE1-FUN-PROTOTYPE.md:135 already states this correctly.

The problem is that the same file contradicts itself ~30 lines later:
  :167  the status table row  | **NS_Swarm emitter graph** | — | **the fault** |
  :168  "No Niagara errors are logged; the emitter simply produces no visible particles.
         This is the §3a problem above, still live"

Fix both so a reader skimming the table cannot come away believing the emitter is broken.

Convention this repo uses, and task-019 just followed on the sibling document: KEEP the
superseded text as the historical record, marked. Strike the row (~~like this~~) and
append a pointer to the FIXED heading above; mark the prose as superseded rather than
deleting it. Do not silently rewrite history — see docs/GDD-TODO.md:104.

Also check ELVTR/SETUP-EDITOR.md for the same claim. A grep on 2026-07-31 found no
surviving hits there, so it is probably already clean — confirm it and say so, or fix it
the same way if you find one. Note that file is currently MODIFIED in the shared working
tree by someone else; if it needs an edit, make the smallest possible one and do not
revert or stage anything else in it.

YOU OWN ONLY: docs/GATE1-FUN-PROTOTYPE.md  (plus a minimal edit to ELVTR/SETUP-EDITOR.md
if and only if you find a real hit there).

DO NOT TOUCH: docs/perf/niagara-sprite-refactor.md (task-019 just finished it),
docs/design/CAMERA-SCALE.md or CAMERA-SCALE-HANDOFF.md (already correct), any source,
any asset. Do not launch the editor — this is a documentation correction, and another
teammate is driving the editor right now.

Do not re-litigate any measured number in the file. The benches stand; only the
attribution of the render bug is wrong.

HAND BACK: the grep output proving the bar, the before/after of both passages, and your
verdict on SETUP-EDITOR.md.
```
