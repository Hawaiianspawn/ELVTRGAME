---
id: 022
title: Design Spike 2 — replication for 2-4 players with aggregate swarms
status: proposed
agent: performance-director
owns: ["docs/perf/replication-spike2.md"]
resources: []
depends-on: [7]
evidence: A replication design plus the measured bandwidth projection at 4 clients and slice density, with the go/no-go criterion stated before the test runs.
score: {gate: 2, risk: 3, cost: 3}
source: docs/RTS-VERTICAL-SLICE.md:98
decided: ""
---

## Why now
`docs/RTS-VERTICAL-SLICE.md:98` scopes it: heroes, elites, and the boss replicate fully;
swarms replicate as aggregates. `GDD.md` §12 Q19 raised **Spike 2 to full 4-client load as
the gate** — this is the second of the two named technical risks, and the first (Spike 1)
is task-007.

Four-player co-op is a locked pillar (Q4, decided 2026-07-09), so this is not an optional
feature whose failure downgrades gracefully. If replication cannot carry the entity counts
at 4 clients, the design changes, and it is much cheaper to learn that now.

Depends on task-007: the entity counts Spike 1 lands on are the input to every bandwidth
number here.

## Done when
- What replicates fully vs. as an aggregate, stated per entity tier.
- Bandwidth projection at 4 clients and slice density, with assumptions.
- The go/no-go criterion written down **before** the test, not after — a threshold chosen
  after seeing the result is not a gate.
- Host-authority questions handled: GDD Q8 settled party votes on world-scarring
  decisions, which implies who owns what state.
- An explicit statement of what fails if the numbers come back bad.

## Spawn prompt
```
You are the performance-director for Emberkeep (C:\Projects\ELVTRGAME).

Design Spike 2 — replication for 2-4 players, per docs/RTS-VERTICAL-SLICE.md:98 and
GDD.md §12 Q19, which raised Spike 2 to full 4-client load as the gate.

Read docs/SPIKE1-RESULTS.md first (task-007 output; this depends on it — its entity counts
are the input to every bandwidth number you produce). Then GDD.md §10 and §12 (Q4: 4-player
co-op is LOCKED; Q8: party vote on world-scarring decisions, which constrains state
ownership), docs/RTS-VERTICAL-SLICE.md, docs/perf/BUDGETS.md, and the Mass sources.

Design: heroes, elites, boss replicate fully; swarms replicate as aggregates. State per
tier what crosses the wire. Project bandwidth at 4 clients at slice density with your
assumptions written out.

Write the go/no-go criterion BEFORE describing the test. A threshold picked after seeing
results is not a gate.

Write ONLY docs/perf/replication-spike2.md. Do NOT edit ELVTR/Source or ELVTR/Content.
This is a design and projection task; you hold no editor lock.

State plainly what happens to the design if the numbers come back bad. 4-player co-op is a
locked pillar, so "it doesn't work" has consequences that need naming now.
```
