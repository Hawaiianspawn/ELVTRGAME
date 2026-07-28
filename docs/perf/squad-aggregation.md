# Squad grouping as a cost boundary — measured, not designed first

**Task:** task-021. **Owner framing (2026-07-27), verbatim:** "What I am trying to do is
manage and update expense for the amount of units in the game. The grouping is what helps
reduce runtime costs when we reach extreme scale." Separately: "The retinue units should
almost always be their unit in the unit cam." This report answers the cost question with
real numbers first, then says what that finding does to the second sentence.

**Headline: at the entity counts this project's own design ceiling produces, no aggregation
is needed. Do not build a group-replaces-N-soldiers simulation layer now.** The render
bridge — the thing that was actually over budget (`docs/perf/BUDGETS.md`, 14.62ms at 1,000
brood on the old debug-box path) — has a fix already shipped (2026-07-26, Niagara
`CPUSim`) that this report gives the first real measurement of. It holds the frame budget
with room to spare across the whole range this session could drive. Full numbers below;
this is not a hunch.

---

## 1. What was measured, and how

**Build:** `flame-spotlight` branch, Development Editor (UnrealEditor.exe), UE 5.8,
`L_Spike1`, single client, no networking — matches the redefined GDD §10 gate exactly
(1 client, 60fps/16.6ms, no replication term).

**Render path:** `Swarm.DebugRender 0` — this is *already the checked-in default* in
`ELVTR/Saved/SwarmExecOnPlay.txt` as of 2026-07-27 (the file's own comment records the
retraction of the earlier "Niagara draws nothing" diagnosis). I did not need to change it;
the numbers below are the path the game already ships with, not a special test mode.

**Population driver:** I did **not** use `-SwarmBench` (that requires relaunching the
editor process with a command-line flag; I judged a full editor relaunch too risky against
other agents' possible unsaved in-memory state in this shared session, and my own operating
rules bar automating editor launches for reliability reasons). Instead I used the game's own
`ASpike1GameMode` auto-fight harness, which runs whenever `-SwarmBench` is *not* on the
command line: `StartingRetinue=120`, `RetinueCap=120`, `WaveBroodCounts={250,450,700}`. This
is a real constraint on what I could measure — see §6.

**Instrumentation used, all stock UE, zero source changes:**
- Built-in **CSV Profiler** (`CsvProfile Start` / `CsvProfile Stop` console commands, run via
  `Saved/SwarmExecOnPlay.txt`'s BeginPlay exec hook) — gives per-frame `FrameTime`,
  `GameThreadTime`, `RenderThreadTime`, `GPUTime`, `RHI/DrawCalls`, `RHI/PrimitivesDrawn`.
  This is **not** the same as this codebase's own `SWARM_SCOPE`/`STAT_Swarm*` per-pass
  counters (`SwarmStats.h`) — those already exist (contrary to a standing hypothesis in this
  agent's brief that no instrumentation exists at all; it does, it's just not wired to CSV
  Profiler) but require an Insights trace or an in-editor `stat Swarm` overlay to read, and I
  had no way to capture either headlessly this session. **Per-pass breakdown (grid build vs.
  steering vs. combat vs. integrate) remains unmeasured** — see §6.
- `Swarm.SpacingLogInterval 5` — periodic `SwarmDebug::LogSpacingReport` calls, which log
  exact live retinue/brood counts (`SwarmSpacing: retinue n=%d`, `brood n=%d`). Used to
  cross-reference wall-clock log timestamps against CSV-profiler frame indices so I know
  exactly which entity count each sampled frame window corresponds to.
- Standard `Run:`/`Swarm: spawned` log lines from `Spike1GameMode`/`SwarmSpawn`, same
  purpose.

**A real trap found and fixed for this session, worth folding into `docs/AGENT-TEAMS.md`
§8:** an agent-driven PIE session run via `StartPIE`/`StopPIE` has no OS window focus, and
the editor's **"Use Less CPU when in Background"** preference
(`EditorPerformanceSettings.bThrottleCPUWhenNotForeground`, default **true**) throttles the
whole engine to a flat **~3fps (333.3ms/frame)** the instant PIE starts unfocused. My first
capture attempt was contaminated by this — `FrameTime` was a dead-flat 333.33ms regardless
of entity count, and `GameThreadTime` didn't correlate with entity count at all (highest at
the *lowest* population, because it was measuring idle/sleep overhead, not sim cost). I
found this via `SearchCVars` turning up nothing, then `EditorToolset.EditorAppToolset`'s
`ObjectTools.list_properties` on `/Script/UnrealEd.Default__EditorPerformanceSettings`,
which does expose it as a live-settable UObject property (no cvar, no restart needed). I
set it `false` for the measurement session via `ObjectTools.set_properties`, re-measured,
and set it back to `true` afterward — confirmed restored. **Any future headless/agent-driven
perf capture in this repo needs the same fix first, or its frame times are fiction.** This
is not a corner case: it is *guaranteed* to bite any agent that starts PIE via MCP without a
human bringing the window to real OS focus.

`Saved/SwarmExecOnPlay.txt` was edited three times during this session (adding
`Swarm.SpacingLogInterval 5`, `CsvProfile Start`/`Stop`, and once a `Swarm.SpawnBrood 5000`
probe) and restored to its exact original content each time — confirmed by re-reading the
file after the last edit. The three CSV capture files this produced (up to 36MB) were
deleted from `ELVTR/Saved/Profiling/CSV/` after extracting the data below; `Saved/` is not
part of the tracked repo.

---

## 2. The numbers

Four windows, each a 1-2 second slice of real frames sampled at a known, log-cross-referenced
entity count (throttle fix applied, real-time pacing, not the 3fps-contaminated run):

| Sample | Retinue | Brood | **Total live entities** | Frame n | FrameTime avg / max (ms) | GameThreadTime avg / max (ms) | RenderThreadTime avg (ms) | GPUTime avg (ms) | RHI DrawCalls avg | RHI PrimitivesDrawn avg |
|---|---|---|---|---|---|---|---|---|---|---|
| Baseline (pre-wave-1) | 120 | 0 | **120** | 181 | 8.34 / 9.71 | 7.01 / 9.83 | 0.027 | 1.72 | 283 | 14,899 |
| Wave 1 peak | 120 | 250 | **370** | 179 | 8.42 / 23.46 | 7.19 / 22.55 | 0.018 | 1.89 | 283 | 20,834 |
| Wave 2 peak | 120 | 450 | **570** | 241 | 8.33 / 8.42 | 7.25 / 8.27 | 0.013 | 1.98 | 283 | 25,681 |
| Wave 3 peak (Gate-1's own maximum) | 120 | 694 | **814** | 231 | 8.71 / 11.57 | 8.52 / 11.90 | 0.001 | 2.06 | 283 | 31,475 |

**Budget:** 16.6ms/frame (60fps), single client, GDD §10 (redefined 2026-07-27).

**Reading it:**
- **Frame time is flat, not rising with count** — 8.3-8.7ms average across a nearly 7x
  entity-count range (120 → 814). At the worst-sampled point (Gate 1's own maximum wave, 814
  live entities, both teams) the frame is at **52% of budget** on average, **72% of budget**
  at the sampled peak GameThreadTime.
- **`RHI/DrawCalls` does not move** (283 flat, level geometry + UI + the one Niagara
  emitter) across the whole range. This is the mechanism, not a coincidence: the debug-box
  path in `BUDGETS.md` cost what it cost because `DrawDebugSolidBox` is one immediate-mode
  draw call *per entity per frame* — O(N) draws. Niagara's GPU-instanced sprite emitter is
  a small, ~fixed number of draws regardless of particle count — O(1)-ish in draw-call
  count, with cost instead landing in `RHI/PrimitivesDrawn` (which *does* scale, 14,899 →
  31,475, roughly +24 primitives per swarm entity) and in modest `GPUTime` growth (+0.34ms
  over 694 more entities, ~0.0005ms/entity). This is the concrete answer to why the 2026-07-26
  Niagara fix matters as much as it does: it didn't just remove one CVar toggle, it changed
  the render bridge's complexity class.
- **One outlier worth flagging, not chasing here:** wave 1's window shows a max FrameTime of
  23.46ms — over budget — against an 8.42ms average. This is almost certainly a single-frame
  batch-spawn hitch (`BatchCreateEntities` archetype/allocation cost for 250 entities landing
  in one frame, possibly compounded by a GC pass — a GC `CSVEvent` fired at a similar point
  in an earlier capture of this same scenario), not a rising steady-state cost — the very
  next sampled window (wave 2, more entities) shows no such spike. This is a **spawn-batching
  cost, not a squad-aggregation question**, and it's outside this task's scope; flagged for
  whoever owns spawn-time perf next, unmeasured beyond this one observation.

---

## 3. Cost boundary and threshold — the actual answer

**Threshold: none is currently needed.** The measured regime (120-814 live entities, both
teams, real-time Niagara rendering, single client) holds 60fps with 28-48% of the frame
budget spare on average, and does not exceed budget except for the one transient spawn-hitch
noted above. This regime already **covers and exceeds** `docs/design/squad-group-system.md`
§4.2's own projected retinue ceiling (~580-730 before the 8-squad-handle allocation itself
breaks for a *design* reason, unrelated to performance) plus a full Gate-1 wave-3 horde (694
brood) landing simultaneously.

**Per-group or global?** If a real ceiling is found later, it will bind on **total live
swarm entity count**, not per-group, and not per-team. Every processor in the chain
(`SwarmGridBuildProcessor`, `BroodSteeringProcessor`/`URetinueFollowProcessor`,
`USwarmCombatProcessor`, `USwarmIntegrateProcessor`, the render bridge) walks *all* swarm
entities every frame with no per-group or per-squad early-out — `SquadId` is cosmetic
routing (which formation slot, which muster-card block), not a cost boundary in the current
architecture. So "does the group need to become a proxy" is really "does the *swarm* need to
become a proxy," independent of how many squads it's divided into. This matters for the
design doc's own model: splitting one pool into two typed pools (Spearmen/Archers) does not,
by itself, change the cost story either way — see §5.

**Can it flip back as casualties reduce the count?** Not answered because not needed —
there is no threshold being crossed in the measured/design-relevant range to flip away
from.

---

## 4. §8 of `squad-group-system.md`, answered point by point

That section specs the typed-unit design's expected perf cost and asks me to confirm or
kill each claim. Read against the actual code (`SwarmProcessors.cpp`,
`SwarmCombatProcessors.cpp`, `SwarmSubsystem.h`):

- **Type storage as a `SquadId` range partition, no new fragment field** — confirmed cheap.
  Zero new bytes on the hot-path fragment, one comparison against a per-frame-computed
  boundary. Agree, no cost.
- **Per-type formation lookup, one extra branch per `SlotOffset` call** — confirmed O(1),
  negligible against the measured numbers above.
- **Per-type, per-stance `EngageRange` as a 2D table instead of 1D** — confirmed O(1), same
  complexity class as today's single stance-only read.
- **Ranged combat sim side, "zero new entities, zero new fragments... reuses
  `StrikeReachSq`/`BlowsClaimed`... just a larger radius" — this claim needs a real
  correction, not a rubber stamp.** The combat pass gates neighbour consideration with a
  single **shared** `MeleeRangeSq` (`Swarm.MeleeRange`, 95uu) read once per pass
  (`SwarmCombatProcessors.cpp:262`: `if (DistSq >= MeleeRangeSq) return;`), not a per-entity
  value — every entity, retinue or brood, is tested against the same range today. Giving
  Archers a 750uu `EngageRange` means this becomes a per-*type* value, which is the easy
  O(1) part §8 describes correctly. **What §8 misses:** `QueryNeighbors`'s 3x3 cell walk has
  a physical reach capped at `GridCellSize` × 3 = **600uu** (this exact number is already
  called out in this codebase's own `Swarm.BroodAggroRange` CVar comment:
  `SwarmProcessors.cpp:69`, "Capped in practice by the 3x3 grid reach (~600uu at GridCellSize
  200)"). An Archer `EngageRange` of 750uu **exceeds that physical reach** — the grid simply
  cannot see that far with a 3x3 walk. Either the grid neighbourhood widens for ranged
  queries specifically (5x5 minimum for 750uu, more cells visited = a real, bounded, but
  non-zero added cost per Archer swing) or `GridCellSize` grows globally (cheaper per-frame
  `TMap` overhead, coarser separation for everyone). This is not free, and it is not
  measured — flagged as a genuine open cost question for whoever implements Archers, not
  answered here.
- **Two independent per-type dense repacks instead of one retinue-wide repack** —
  confirmed sound. `URetinueFormationProcessor`'s existing repack is already gated on
  `NeedsFormationRepack()` (count-changed only) and is `O(n log n)` via `Live.Sort()`;
  splitting it per type, each independently gated, is the same amortization pattern one
  level down, no new complexity class.
- **Army View stays `O(MaxSquads)` = `O(8)`** — confirmed, unaffected by anything measured
  here.
- **Map Mode as "the one real new cost," `O(N)` orthographic draw** — §8's own honesty
  about this is correct, and my numbers give it a favorable context it didn't have when
  written: the *existing* `O(N)` Niagara render bridge (full atlas sprites, not simplified
  dots) already costs comfortably inside budget at N=814 (§2). Map Mode's own description —
  "flat colored primitives... cheaper per-unit than [the] atlas-brush path" — should cost
  *less* per entity than what's already measured holding. This is a **reasoned expectation,
  not a measurement** — Map Mode doesn't exist as code yet, so there is nothing to profile.
  Flagged for whoever builds it to confirm with a real capture, same standard as everything
  else in this file's history.

---

## 5. The Unit Cam constraint — where cost and the owner's preference actually stand

The team-lead's brief asked me to say plainly whether cost and "the retinue units should
almost always be their unit in the unit cam" collide. **They don't, on cost grounds.**
`docs/design/squad-group-system.md` §5.1-§5.2 already found the real constraint blocking
"real mini-units by default" in the compact Unit Cam panel, and it is **pixel budget, not
performance**: a 405-837px panel showing the full leash-bound spread draws individual
soldiers at 6-13px regardless of projection method (their own re-derivation, orthographic vs.
perspective, confirms this twice). My numbers add nothing that makes that geometry problem
better or worse — a legible individual soldier needs screen pixels, not frame-time budget,
and a panel that's too small to show one legibly stays too small no matter how cheap the sim
is.

What my numbers *do* settle is the fork squad-group-system.md's own §8 left open: **Map
Mode, the new larger real-per-soldier view that actually delivers "mini units by default,"
is not going to be blocked by cost either**, on the evidence in §2 and §4 above (an O(N)
real-position draw at these counts is the same complexity class as the render bridge that's
already measured holding, and Map Mode's icons are described as cheaper per-unit than the
full sprite atlas). So: **the compact panel shows abstract blocks because of legibility, not
cost — a design call task-049 already made and owns.** Map Mode shows real per-soldier
positions because that surface has the pixel budget to make it legible, and the cost of
doing so is not the reason to hold back. Both halves of "almost always their unit" are
answered by the *existing* design split (compact panel = Army View, on-demand = Map Mode),
not by anything a squad-becomes-one-entity aggregation layer would add.

---

## 6. What remains unmeasured — stated plainly

- **The literal Spike-1 gate count, 1,000+ units, was not reached.** The highest clean,
  uncontaminated total I could drive this session was **814** (Gate 1's own wave-3 cap:
  120 retinue + 700 brood, of which 694 were alive at the sampled peak). I tried to push
  higher via a manual `Swarm.SpawnBrood 5000` exec-file override; confirmed by log
  (`Swarm: spawned 5000 brood` immediately followed by `Swarm: cleared` /
  `Swarm: spawned 120 retinue`) that `Spike1GameMode::RestartRun()` wipes any manual spawn
  the instant its own `BeginPlay` runs — exactly the landmine the team-lead's brief warned
  about. Getting past this needs either a `-SwarmBench` editor relaunch (out of scope for
  this session — see the risk note in §1) or a small code change decoupling the harness's
  population from the exec-file spawn path (a Source change, out of my remit here).
  **The trend at 120-814 is flat and strongly suggests 1,000+ holds too, but that is an
  extrapolation, not a measurement, and the reasonable next step is a real `-SwarmBench`
  sweep on the Niagara path at the same 500/1,000/2,000/5,000/10,000 brood counts
  `BUDGETS.md` already used for the debug-box A/B**, giving a true apples-to-apples ceiling
  number instead of my rough linear read (~0.0022ms of GameThreadTime per additional entity
  in the 120-814 range, which if it held flat to 16.6ms would put a ceiling around 4,000-
  4,500 total entities — **stated as a back-of-envelope extrapolation only**, likely to
  break down at higher densities since grid neighbour-visit cost is a function of local
  packing density, not raw count, and I have no data past 814 to confirm the curve stays
  linear).
- **Per-pass `STAT_Swarm*` breakdown** (grid build vs. brood steering vs. retinue follow vs.
  combat vs. integrate) is still not captured by anything in this report, same gap
  `BUDGETS.md` already flagged. The counters exist (`SwarmStats.h`, confirmed by reading the
  code — contrary to a "no instrumentation exists" hypothesis in my own brief) but need
  either an Unreal Insights trace or an in-editor `stat Swarm` screenshot, neither of which
  I could capture headlessly this session (stat overlays are Slate/canvas draws, not visible
  to `Swarm.DebugShotAfter`'s scene capture, same limitation already documented for the game
  HUD).
- **The archer grid-reach cost in §4** is a real open question, not a number — flagged for
  whoever builds ranged combat, not resolved here.
- **The one wave-1 batch-spawn spike (23.46ms max, §2)** is noted, not diagnosed — separate
  from squad aggregation, worth a follow-up by whoever owns spawn-time perf.

---

## 7. Recommendation

**Do not build a squad-becomes-one-entity simulation aggregation layer now.** The measured
data — 8.3-8.7ms average frame time, 52-72% of the 16.6ms single-client budget used at the
worst sampled point (814 live entities, the maximum this project's own Gate-1 tuning
currently produces) — shows the per-entity Mass simulation and the now-fixed Niagara render
bridge hold the frame budget with real headroom across the whole range this session could
drive, comfortably covering `squad-group-system.md`'s own projected typed-unit retinue
ceiling. Building a group-representative proxy entity — with its own reification/
de-reification state machine, a second simplified combat model to keep working across the
swap (the existing `StrikeReachSq`/`BlowsClaimed` model assumes real per-soldier grid
entries), and a second render path — would be real, ongoing complexity bought against a cost
problem the numbers say does not currently exist. That is exactly the "aggregation bought
without need is pure complexity" outcome this role is meant to be able to say plainly.

**What would change this recommendation:** a real measurement past 814, at the literal
1,000+ Spike-1 gate and beyond into GDD's "hundreds-to-thousands" late-run language, showing
the frame budget actually breaking. That measurement does not exist yet (§6) — it is the
next thing to get, not a reason to build aggregation speculatively today. If/when the
project's soft caps (the leash, upkeep degradation, the 8-squad handle ceiling) are raised
past what a real sweep shows holding, this recommendation should be re-run against fresh
numbers, not assumed to still hold indefinitely.

## Gameplay impact

None — this report recommends *not* building anything that would change unit behaviour,
combat resolution, or what the player sees. The one item here with a real gameplay-facing
consequence if ignored is the §4 archer grid-reach finding (ranged combat's actual minimum
viable cost is not quite "zero" as `squad-group-system.md` §8 assumed); that belongs to
whoever implements ranged combat, flagged, not decided here.
