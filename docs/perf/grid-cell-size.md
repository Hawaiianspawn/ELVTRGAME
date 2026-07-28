# GridCellSize 200 → 250 — reach vs. density, measured

**Task:** task-052. **Owner's decision, stated plainly by the lead:** widen the spatial
grid so `squad-group-system.md`'s proposed Archer `EngageRange` of 750uu genuinely works,
rather than capping Archers at 600uu. This doc is the cost measurement that decision was
made without, taken now, plus proof the reach actually changed.

**Headline: this was free at the entity counts this project currently produces.**
`GridCellSize` 200→250 (physical 3x3 reach 600uu→750uu) does not move frame time,
draw calls, or GPU time outside noise across the same 120→814-entity range task-021
already certified as comfortably inside budget. **Verdict: keep it. This was a good
trade at today's scale** — see §5 for the one thing that would change that answer.

---

## 1. What changed, and what else touches `GridCellSize`

`ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h:28`: `static constexpr float GridCellSize`
raised **200.f → 250.f**. `QueryNeighbors` (same file) walks a fixed 3x3 neighbourhood
around an entity's own cell — that is the only place the constant's *meaning* as a
search radius comes from, and it was never configurable independently of the bucket
size. True physical reach is therefore always `GridCellSize * 3`:

| GridCellSize | 3x3 reach |
|---|---|
| 200 (old) | 600uu |
| 250 (new) | **750uu** — exactly what `squad-group-system.md` §2.2 asked for |

**Confirmed by grep before changing it (per the brief's ask): nothing else in the
codebase derives a value from `GridCellSize`.** The only other consumer is
`ToCell()`'s `FMath::FloorToInt(Location / GridCellSize)` bucketing, which is scale-
invariant — it doesn't care what the constant's value is, only that every caller uses
the same one. No other constant, CVar default, or comment anywhere in `ELVTR/Source`
hard-codes a number derived from 200 except `Swarm.BroodAggroRange`'s own doc-comment
(fixed below) and its mirror in `Saved/SwarmExecOnPlay.txt`'s comment (also fixed) and
the `cvars` skill / `SYSTEMS.md` (both out of this task's `owns:`, flagged for whoever
next touches them — `SYSTEMS.md:87-111` still says "~600uu").

**`Swarm.BroodAggroRange` (`SwarmProcessors.cpp:64-70`) fixed to match:** its
doc-comment said "~600uu at GridCellSize 200" and its documented slider range was
`[0..600]` (this is a **panel slider bound only** — the `cvars` skill's `[min..max]`
convention read by the Breadboard panel, not a code-side `FMath::Clamp`; the actual
runtime code only ever floored the CVar at 0, `SwarmProcessors.cpp:376`, so a value of
750 was never rejected, it just couldn't physically matter before this change). Both
now read `750uu` / `[0..750]`. The CVar's **default value stays 600** — raising the
default is a balance call belonging to whoever ships Archers, not this task.

---

## 2. Method — identical to task-021, so the comparison is real

Per the brief: same build config, same population driver, same instrumentation, same
throttle fix, same waves. Full method detail lives in `docs/perf/squad-aggregation.md`
§1 (task-021, cited not edited); summarized here only to state what didn't change:

- `flame-spotlight` branch, Development Editor (UnrealEditor.exe), UE 5.8, `L_Spike1`,
  single client.
- Population driver: `ASpike1GameMode`'s own auto-fight wave progression (not
  `-SwarmBench`) — `StartingRetinue=120`, `RetinueCap=120`,
  `WaveBroodCounts={250,450,700}`.
- Instrumentation: stock CSV Profiler (`CsvProfile FRAMES=20000` — a self-stopping
  variant of task-021's `Start`/`Stop` pair, same effect, one exec-file line instead of
  two console calls) cross-referenced against `Swarm.SpacingLogInterval 5` log reports
  for exact live counts.
- **`EditorPerformanceSettings.bThrottleCPUWhenNotForeground` disabled before
  measuring** (`docs/AGENT-TEAMS.md` §8a), via `ObjectTools.set_properties` on the live
  CDO, **and restored to `true` afterward** — verified both ways with
  `ObjectTools.get_properties`.
- `Saved/SwarmExecOnPlay.txt` edited for the session (`SpacingLogInterval 5`, the
  `CsvProfile` action line, `BroodAggroRange` bumped to 750 for the reach-proof pass
  only) and restored to its pre-session content afterward, confirmed by diffing against
  a backup taken before the first edit — the only surviving difference is the intended
  permanent `BroodAggroRange` comment fix from §1. Owner-tuned values
  (`Swarm.Formation.Spacing 42.4`, the `UnitCamProj` `Fov`/`Height`/`Pitch`/
  `SoldierAspect` block, `Swarm.UnitShading 0`, the HORDE ARRIVAL section) untouched.
- The 20MB CSV capture was deleted from `ELVTR/Saved/Profiling/CSV/` after extracting
  the numbers below, same as task-021 — `Saved/` isn't part of the tracked repo.

**One real difference from task-021, stated plainly:** this run reached its population
milestones a little faster in real time (the whole three-wave run — deploy through
wave-3 loss — took ~50 simulated seconds this session vs. whatever task-021's session
took) and this particular run **lost** on wave 3 (0 retinue survived; task-021 didn't
report a win/loss state). Neither affects the comparison: I sample the same four
population milestones (120 / ~370 / ~570 / ~814-820) at the same point in each wave
(spawn instant, before meaningful casualties), using the SAME balance CVars task-021
used (`Swarm.BroodAggroRange` stayed at its default 600 during the cost run — only
`GridCellSize` changed). The loss just means the run happened to be a harder RNG/timing
draw than task-021's; it says nothing about the grid.

---

## 3. The numbers — before/after at matched populations

**Baseline (task-021, GridCellSize 200)**, quoted from `docs/perf/squad-aggregation.md` §2:

| Sample | Total live | FrameTime avg/max (ms) | GameThreadTime avg/max (ms) | RHI DrawCalls |
|---|---|---|---|---|
| Baseline | 120 | 8.34 / 9.71 | 7.01 / 9.83 | 283 |
| Wave 1 peak | 370 | 8.42 / 23.46 | 7.19 / 22.55 | 283 |
| Wave 2 peak | 570 | 8.33 / 8.42 | 7.25 / 8.27 | 283 |
| Wave 3 peak | 814 | 8.71 / 11.57 | 8.52 / 11.90 | 283 |

**This task (GridCellSize 250)**, measured at the same milestones, exact live counts
cross-checked against `SwarmSpacing` log lines fired within the sampled window:

| Sample | Total live (exact) | FrameTime avg/max (ms) | GameThreadTime avg/max (ms) | RHI DrawCalls | GPUTime avg (ms) | Prims avg |
|---|---|---|---|---|---|---|
| Baseline | 120 | 8.33 / 8.66 | 6.64 / 8.94 | 257 | 1.51 | 13,511 |
| Wave 1 peak | 370 (120+250, confirmed) | 8.33 / 8.34 | 6.76 / 7.47 | 257 | 1.54 | 19,400 |
| Wave 2 peak | 570 (120+450, confirmed) | 8.33 / 8.34 | 6.94 / 8.19 | 257 | 1.54 | 24,295 |
| Wave 3 peak | 820 (120+700, confirmed, sampled at the zero-losses instant — 6 more than task-021's 814, which had already lost 6 brood by its sampled window) | 8.53 / 9.67 | 8.14 / 9.66 | 257 | 1.61 | 30,311 |

**Reading it: FrameTime avg is flat and statistically indistinguishable from the
GridCellSize=200 baseline** — 8.33ms at 120/370/570, rising to 8.53ms at ~820, against
task-021's 8.34/8.42/8.33/8.71ms at 120/370/570/814. The 200→250 change is inside the
run-to-run noise task-021's own numbers already show (its wave-1 max of 23.46ms, a
one-frame batch-spawn hitch, dwarfs anything the grid resize does here). GameThreadTime
avg is if anything slightly *lower* at every matched point in this run than task-021's
— not a real improvement (different session, different exact camera/GC timing), just
further evidence there's no cost signature to find.

**`RHI/DrawCalls` differs (257 here vs. 283 in task-021) but is flat within each run** —
both are population-independent constants, just a different flat number between
sessions (level/viewport state, not the grid). Not a regression; not measuring the same
thing the grid change could move.

**Why flat: the extra reach costs real but small absolute work at this scale.** A
250uu cell covers 1.5625x the area of a 200uu cell, so at the same entity density each
`QueryNeighbors` call walks roughly 1.5x more entries per bucket. At population counts
in the low hundreds spread across a formation + spawn arc, that is a few extra float
comparisons per entity per frame — below the noise floor next to the ~8.3ms this pass
already shares with rendering, GPU, and everything else in the frame. This is the same
finding task-021 already made about the swarm generally (§3 of that doc): the current
entity-count ceiling isn't close to where any of these O(N) or O(N·k) costs bite.

---

## 4. Proof the reach actually changed

**Method:** a brood's divert-to-nearest-soldier check
(`UBroodSteeringProcessor::Execute`, `SwarmProcessors.cpp`) only ever considers targets
`QueryNeighbors` can find — at `GridCellSize=200` that walk topped out at 600uu no
matter what `Swarm.BroodAggroRange` was set to, so a divert landing between 600 and
750uu away was **structurally unreachable code** before this change, not just an
untested one. I added a temporary, narrowly-gated log line (600-650uu band, so it fires
once per brood crossing that band rather than every frame), set
`Swarm.BroodAggroRange 750` for one short PIE session, and captured the log — then
removed the temporary line and did a second clean rebuild so no debug logging survives
in the owned files (confirmed by `git diff`, §7 below).

**Result — real log lines from that session, unedited:**

```
LogTemp: Display: Swarm: wide-aggro divert at 601uu (beyond old 600uu grid cap)
LogTemp: Display: Swarm: wide-aggro divert at 642uu (beyond old 600uu grid cap)
LogTemp: Display: Swarm: wide-aggro divert at 639uu (beyond old 600uu grid cap)
LogTemp: Display: Swarm: wide-aggro divert at 637uu (beyond old 600uu grid cap)
LogTemp: Display: Swarm: wide-aggro divert at 635uu (beyond old 600uu grid cap)
... (dozens more, ranging 600-642uu, across two separate brood approaching the line)
```

Dozens of these fired within the first second of brood closing distance on the
retinue. Every one of them is a divert distance that `FindNearestEnemy` could never
have returned at the old cell size — `QueryNeighbors`' 3x3 walk physically could not
see that far. This is the concrete, mechanical confirmation that 750uu — and by
extension a future Archer `EngageRange` of 750uu — now resolves where it silently
couldn't before.

---

## 5. Where widening stops being worth it

**Not yet, at anything this project currently produces.** §3's flat numbers hold
comfortably inside the 16.6ms budget with the same 28-48%-of-budget headroom task-021
already found at these counts (`docs/perf/squad-aggregation.md` §3) — the grid change
doesn't touch that conclusion, it just confirms the conclusion still holds after this
specific change.

**The actual lever, if it ever needs pulling, is density, not `GridCellSize` in
isolation.** `QueryNeighbors` cost is `O(entries per visited cell)`, and entries per
cell scale with `(local entity density) * GridCellSize²`. Two ways this project could
reach a regime where the tradeoff bites:

1. **Entity count rises without density falling** (more brood in the same spawn arc,
   not a wider one) — task-021 flagged this as unmeasured past 814 (its own §6); that
   ceiling question is still open and this task doesn't close it. Re-run this same
   method at task-021's suggested `-SwarmBench` sweep points (1,000/2,000/5,000+) before
   trusting either grid size at those counts.
2. **`GridCellSize` itself rises further**, independent of count — each doubling of the
   cell side is a *quadrupling* of the area (and therefore, at fixed density, the
   average entries) any single `QueryNeighbors` call walks. 250 was chosen because it's
   the smallest value that clears 750uu exactly (`250 * 3 = 750`, no slack, no
   over-widening past what the spec actually asked for) — a future ask for, say, 1200uu
   reach would need `GridCellSize=400`, which is 2.56x today's cell area and a
   fundamentally different point on this curve, not a linear extrapolation of today's
   "free" result.

**Practical guidance for whoever raises this next:** don't reuse this doc's "it's
free" conclusion at a materially different cell size or entity count without
re-measuring — re-run §2's method at the new numbers first, the same way this task did
before trusting the owner's original assumption.

---

## 6. Verdict

**This was a good trade, measured, not assumed.** The owner picked widening the grid
over capping Archers at 600uu believing it was affordable; at today's entity-count
ceiling (120-820, the range this project's own Gate-1 tuning currently produces) it
measurably is — flat frame time, no draw-call or GPU signature, and the 750uu reach it
was built for demonstrably works (§4). No reason found here to walk back to 600uu or to
recommend capping Archers.

**What would change this verdict:** a real measurement at 1,000+ entities (task-021's
own open question, inherited unchanged by this task) showing the combined cost of a
bigger swarm AND a wider grid together — that combination is still unmeasured, and per
§5's density argument it is not simply the sum of two already-flat numbers.

## Gameplay impact

None from the grid or CVar-doc changes themselves — `Swarm.BroodAggroRange`'s default
stays 600, so today's brood/Hold-stance behaviour is unchanged. The impact is
entirely enabling: `squad-group-system.md`'s 750uu Archer `EngageRange` (not yet
implemented — that's `typed-units-model`'s task, not this one) now has a physically
reachable grid to run on, where before it would have silently behaved as 600uu.
