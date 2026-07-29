# Swarm frame budget — scoreboard

> **SUPERSEDED IN PART, 2026-07-28 — read [one-camera-bench.md](one-camera-bench.md) first.**
>
> A standalone `-game` sweep across all renderers landed that day and changes the headline
> conclusions below:
>
> - **The gate is PASSED, not failing.** Niagara sprites hold **2.31ms (433fps)** at the
>   1,000-unit gate — a 7.2x margin. The "over budget before anything else in the frame runs"
>   framing below is true *of the debug-box renderer only*, which is no longer the default.
> - **Rendering is free.** Niagara's frame time sits on a sim-only baseline within noise at
>   every count from 500 to 20,000. **100% of the frame cost is the Mass sim on the game
>   thread**, ~0.75ms per 1,000 entities.
> - The debug-box table below is still accurate and still reproducible (136.8ms measured
>   standalone vs 135.5ms in-editor at 10,000) — it is simply no longer measuring the
>   shipping path.
> - `Swarm.SimLOD.Stride 4` (new, same day) raises the 60fps ceiling from ~21,000 to ~34,000.
>
> The per-pass and multiplayer gaps listed at the bottom of this file remain open and correct.
> The multiplayer one is now moot — GDD §10 went single-player 2026-07-27.


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

## Niagara sprite path — measured 2026-07-28 (standalone `-game`, `-SwarmBench` config sweep)

The baseline this file said did not exist. Retinue 100, brood swept. Full detail and the
Unit Cam / LOD comparisons: [one-camera-bench.md](one-camera-bench.md).

| brood | sim only | Niagara | debug box (flat) | Niagara vs box |
|---|---|---|---|---|
| 500 | 1.67 | 1.75 | 3.40 | 1.9x |
| 1,000 | 2.25 | **2.31** | 9.44 | **4.1x** |
| 2,000 | 3.07 | 3.06 | 21.19 | 6.9x |
| 5,000 | 5.00 | 5.02 | 59.93 | 11.9x |
| 10,000 | 8.53 | 8.32 | 136.84 | 16.4x |
| 20,000 | 15.24 | 15.90 | 355.07 | 22.3x |

**Reading it:** Niagara costs 0.036 µs per unit drawn, on the GPU, which never exceeds 4.2ms
across the whole sweep. The debug renderer's cost is CPU draw submission and scales with count.
`Swarm.DebugRender` now defaults to 0.

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
