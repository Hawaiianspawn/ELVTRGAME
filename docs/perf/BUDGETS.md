# Swarm frame budget — scoreboard

Owned by the performance-director. Updated whenever a new measurement lands. States what
we're holding today against what the design needs, not what we hope to hold eventually.

**Target:** 60fps (16.6ms/frame) at the GDD §10 entity-count gate (4 players x late-run
retinue + enemy hordes, "the entity budget above" — not yet pinned to a single number in
this doc; see GDD.md §10 for the gate language). Everything below is measured on the
`flame-spotlight` branch, single client, no networking, via the in-engine `-SwarmBench`
harness (`SwarmRenderActor.cpp:448-527`). Platform/build config not yet recorded by whoever
ran it — flag if that matters before trusting these across machines.

## Render bridge — measured 2026-07-26 (team-lead, in-editor, `-SwarmBench`)

Retinue held at 100 throughout; brood swept. `Swarm.DebugRender 1` (the only renderer that
currently draws anything — see [niagara-sprite-refactor.md](niagara-sprite-refactor.md)).
A = `Swarm.UnitShading 1` (default, two-box directional shading). B = `Swarm.UnitShading 0`
(flat single box).

| brood | A draw ms | B draw ms | A/B ratio | A frame ms | B frame ms |
|---|---|---|---|---|---|
| 500 | 4.85 | 3.09 | 1.57x | 4.85 | 3.10 |
| 1000 | 14.62 | 7.59 | 1.93x | 14.62 | 7.59 |
| 2000 | 40.65 | 18.42 | 2.21x | 40.66 | 18.43 |
| 5000 | 132.33 | 57.15 | 2.32x | 132.33 | 57.15 |
| 10000 | 350.04 | 135.47 | 2.58x | 350.02 | 135.47 |

At 10000 brood: GPU 23.11ms (A) / 13.71ms (B). Game thread (non-render) 26.33ms (A) /
22.18ms (B).

**Reading it:** draw ms *is* frame ms at every count in both configurations — the
game/GPU threads are not the constraint, ever, in this data. This is CPU-side draw
submission on immediate-mode `DrawDebugSolidBox` calls, not a shading or hardware cost.
1000 brood (well inside GDD's stated Spike-1 floor of "1,000+ units at 60fps") already
costs 14.6ms on draw alone — over budget before anything else in the frame runs. **The
debug-box renderer cannot hold the Spike 1 bar, let alone the 4-player gate**, regardless
of `UnitShading`. Full writeup and what follows from it: [niagara-sprite-refactor.md](niagara-sprite-refactor.md).

**Not yet measured / owned by other tasks:**
- Per-pass `STAT_Swarm*` breakdown (grid build / steering / combat / integrate) — the
  `-SwarmBench` harness gives thread-level ms only, not per-processor cost. My
  counted-operations estimate for the combat neighbour-walk (finding #2, uncapped
  per-neighbour `GetSafeNormal` + K-insertion-sort) is *not* isolated by this data — the
  game thread stays under 27ms everywhere here, so whatever that pass costs, it isn't
  currently the constraint. Still worth a real capture once the render bridge stops
  dominating and can no longer hide it.
- Multiplayer/replication cost — GDD §10's actual gate condition (4-client aggregate
  replication) is untested; everything above is single-client.
- Cost at the Niagara/Mass sprite path once it exists — no baseline yet, by definition.

## Combat pass (`SwarmCombatProcessors.cpp`) — unmeasured

Two static-analysis findings from the flame-spotlight diff review (2026-07-26), neither
turned into real numbers yet: an uncapped per-melee-neighbour `GetSafeNormal` +
insertion-sort in the K-nearest cleave targeting, and per-entity `Atan2` in the facing
resolve inside Integrate. Both are linear-in-contact-density, not entity count, and the
render-bridge numbers above suggest they are not the current bottleneck. Revisit once the
render bridge is fixed and no longer the dominant cost.
