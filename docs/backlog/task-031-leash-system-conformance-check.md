---
id: 031
title: Check the shipped leash system against RTS-VERTICAL-SLICE §2's spec
status: approved
agent: claude
owns: ["docs/RTS-VERTICAL-SLICE.md"]
resources: []
depends-on: [8]
evidence: A per-clause conformance table — radius, hysteresis, warning, break-to-Follow — each marked shipped, partial, or missing against the source.
score: {feel: 2, risk: 1, cost: 1}
source: docs/RTS-VERTICAL-SLICE.md:97
decided: "2026-07-31 approved"
---

## Why now
This is filed as a **verification**, not a build, and that distinction is the finding.
`docs/RTS-VERTICAL-SLICE.md:97` lists "Leash system per §2 (radius, hysteresis, warning,
break-to-Follow)" as an unchecked build item. But Gate 1 ships a leash: the HUD reports
how many units are currently leash-broken, and `Swarm.LeashRadius` is a live tunable.

So the box is probably wrong, in the same way `GDD-TODO.md`'s "name the game" box was
wrong. What is unknown is whether all four clauses shipped or only some — hysteresis and
the warning state are the ones that quietly go missing, and their absence is exactly what
would make Gate 1's "does the leash break read clearly?" question (task-008) come back
negative.

## Done when
- Each of the four clauses checked against the actual Mass steering source: radius,
  hysteresis, warning, break-to-Follow.
- Each marked shipped / partial / missing, with the file:line that proves it.
- The `docs/RTS-VERTICAL-SLICE.md:97` box ticked or annotated with what is genuinely left.
- Findings cross-referenced with task-008's playtest answer — if the leash break did not
  read clearly in play and the warning clause turns out to be missing, that is one finding,
  not two.

## Spawn prompt
```
You are verifying shipped behaviour against spec in Emberkeep (C:\Projects\ELVTRGAME).

docs/RTS-VERTICAL-SLICE.md:97 lists the leash system as an unchecked build item: "Leash
system per §2 (radius, hysteresis, warning, break-to-Follow)". But Gate 1 clearly ships a
leash — docs/GATE1-FUN-PROTOTYPE.md says the HUD reports how many units are leash-broken,
and Swarm.LeashRadius is a live CVar. The box is likely stale.

Read docs/RTS-VERTICAL-SLICE.md §2 for the four clauses, docs/GATE1-FUN-PROTOTYPE.md, and
then the actual Mass steering source under ELVTR/Source (SwarmProcessors.cpp has the
stance-aware retinue steering and leash; SwarmCombat.h has the tunables).

Also read task-008's playtest output if it exists — it asks whether the leash break reads
clearly. If the answer was negative AND the warning clause turns out to be missing, that
is ONE finding, not two.

Produce a per-clause conformance table: radius, hysteresis, warning, break-to-Follow —
each marked shipped / partial / missing, each with the file:line that proves it. Hysteresis
and the warning state are the two that quietly go missing; check them specifically rather
than assuming they came along with the radius.

Then tick the box at docs/RTS-VERTICAL-SLICE.md:97 or annotate it with exactly what is
left.

Write ONLY docs/RTS-VERTICAL-SLICE.md. Do NOT edit ELVTR/Source — if a clause is missing,
report it; implementing it is a separate task the owner approves.
```
