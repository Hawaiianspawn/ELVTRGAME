# Blood particles (task-060) — measured cost and verdict at wave-3 density

**Measured 2026-07-28/29**, `flame-spotlight` branch, Development Editor (UnrealEditor.exe),
PIE (`PlayMode_InViewPort`), single client, `L_Spike1`, `ASpike1GameMode`'s own auto-fight
wave run (not `-SwarmBench` — see §4 for why). `EditorPerformanceSettings.
bThrottleCPUWhenNotForeground` disabled for the session and restored after
(`docs/AGENT-TEAMS.md` §8a — an unfocused agent-driven PIE session is throttled to ~3fps
otherwise, which would make every number below fiction).

## 1. The headline

**Blood has a real, measured cost — small in absolute terms, not free.** At wave-3 density
(820 live entities — 700 brood + 120 retinue, real combat, comparable intensity both ways —
peak mobbed 14-15 in both samples), clean frame time (outlier hitches excluded, §2/§3).
**Measured at the original tuning** (`ParticlesPerHit 6` / `MaxBurstsPerFrame 24` /
`SpeedScale 2.2`) — §5 found that tuning washes out on screen and halved the volume dials
before shipping, so this delta is a conservative (worst-case) upper bound on what actually
ships, not the final numbers:

| | FrameTime avg | GameThreadTime avg | RHI/DrawCalls avg | PrimitivesDrawn avg |
|---|---|---|---|---|
| Blood **ON** (original tuning, 6/24/2.2) | 9.121ms (110fps) | 8.635ms | 254.2 | 23,617 |
| Blood **OFF** | 8.388ms (119fps) | 7.281ms | 153.0 | 18,795 |
| **Delta** | **+0.733ms (+8.7%)** | **+1.354ms (+18.6%)** | **+101.2 (+66%)** | **+4,822 (+25.6%)** |

Both land comfortably inside the 16.6ms/60fps budget — blood-on is at 55% of budget, 45%
headroom to spare. **But this is not the "rendering is free" result this project usually
gets** (`one-camera-bench.md` §1: the swarm's own Niagara sprite path sits within noise of a
sim-only baseline at every count tested). The reason is architectural, not a particle-count
problem: the swarm is **one persistent, instanced Niagara emitter** that the render bridge
pushes positions into every frame; `UBloodSubsystem` instead calls
`UNiagaraFunctionLibrary::SpawnSystemAtLocation` **per burst** (up to
`Blood.MaxBurstsPerFrame` = 24 new components, each with its own draw call, allocated and
torn down fire-and-forget every time it fires). The +101 draw calls and +25.6% primitives
line up with exactly that — many small components, not one bigger one. It costs real
game-thread time (component spawn/destroy, not just draw submission) which is why
`GameThreadTime` moved more (+18.6%) than raw `FrameTime` (+8.7%).

## 2. Blood ON at wave-3 density (820 live entities)

`CsvProfile` (stock CSV Profiler, self-stopping `FRAMES=` variant), cross-referenced against
`Swarm.SpacingLogInterval 2` log reports and the game's own `SwarmFight` combat-summary
lines to confirm the exact live count and window. **Original tuning throughout** — this
predates the §5 wash fix — (`Blood.ParticlesPerHit 6`, `Blood.MaxBurstsPerFrame 24`,
`Blood.Lifetime 0.3`, `Blood.Size 6`, `Blood.SpeedScale 2.2`, `Blood.HeightOffset 15`).

Wave 3 spawned at t=49.56s from restart (700 brood, total tracked 820). Window sampled
t=53-80s (27s / 2,926 frames) — a few seconds after spawn (past the batch-spawn settle) through
well into the fight (peak mobbed 15, hero took 315 damage over the 30s exchange, meaning
**many** hits landed and **many** blood bursts fired in this window):

| | FrameTime avg | FrameTime max | GameThreadTime avg | GameThreadTime max |
|---|---|---|---|---|
| Raw (2,926 frames) | 9.229ms | 218.5ms | 8.707ms | 218.7ms |
| **Clean** (excl. 2 one-frame hitches >50ms) | **9.121ms (110fps)** | — | **8.635ms** | — |

`RHI/DrawCalls` avg 254.2, `RHI/PrimitivesDrawn` avg 23,617.

The two >50ms outliers are one-frame hitches, not a blood cost: similar single-frame spikes
recur throughout every run this session (blood on **and** off, at every wave), matching this
project's own documented "one-frame batch-spawn/GC hitch" pattern
(`docs/perf/squad-aggregation.md` — wave-1's 23.46ms max on an otherwise-8.3ms frame). The
blood-OFF run (§3) shows the same pattern at larger, more frequent magnitude (~1.9s spikes
every ~35s) — but that run also ran ~8x longer in wall-clock/entity-churn terms (multiple
in-session wave restarts before finally reaching wave 3, §4), which is the more likely driver
of GC pressure than blood itself. Excluded from both "clean" figures for that reason; not
investigated further here.

## 3. Blood OFF at wave-3 density (820 live entities) — the comparison sample

Same instrumentation. `Swarm.BroodAggroRange` was temporarily bumped 600→900 for this
session only (restored to 600 after — same precedent as `grid-cell-size.md`'s temporary 750
bump) to help the wave-clear gate actually resolve (see §4 for why that gate is fragile).
Wave 3 spawned at t=377.8s from this run's BeginPlay (the 6th attempt this session to reach
wave 3 cleanly — see §4). `SwarmFight 6` ran the actual combat: 50.8s, retinue 120→47,
brood 700→7 (killed 693), **peak mobbed 14** — directly comparable combat intensity to the
blood-ON sample's peak mobbed 15. Window sampled t=383-437s (54s / 5,977 frames), covering
the settle + the full fight:

| | FrameTime avg | GameThreadTime avg |
|---|---|---|
| Raw (5,977 frames) | 9.035ms | 7.929ms |
| **Clean** (excl. 2 hitches >50ms — the ~1.9s GC-pattern spikes, §2) | **8.388ms (119fps)** | **7.281ms** |

`RHI/DrawCalls` avg 153.0, `RHI/PrimitivesDrawn` avg 18,795 — both meaningfully lower than
the blood-ON sample. See §1 for the delta table and the architectural read (many small
fire-and-forget Niagara components, not amortized rendering cost).

## 4. Why the wave-gated run, not `-SwarmBench` — and why blood-OFF took 6 attempts

`-SwarmBench` needs a relaunch with a command-line flag, which the project's own prior
perf work (`squad-aggregation.md` §1) already declined for exactly the same reason I did:
too risky against other agents' in-memory editor state in a shared session. `BenchExec`'s
own density-setup trick (`Swarm.Clear` + `Swarm.SpawnRetinue` + `Swarm.SpawnBrood` from the
`Saved/SwarmExecOnPlay.txt` ACTIONS block) was tried as a faster alternative to waiting
through the wave sequence — **confirmed not to work here**: `ASpike1GameMode::BeginPlay`'s
`RestartRun()` runs after the render actor's exec-file pass and wipes/overrides any manual
spawn, empirically verified (a direct-spawn attempt landed as an in-progress wave-1/2 state,
not the requested 820). So the real `ASpike1GameMode` wave sequence is the only route to a
genuine wave-3 population, same as `docs/perf/squad-aggregation.md` and
`docs/perf/grid-cell-size.md` used.

**Getting the blood-OFF sample took 6 full wave-gated attempts and ~40 minutes**, worth
recording plainly since it very nearly meant reporting no comparison at all. `ASpike1GameMode`'s
wave-clear gate is `PhaseTimer >= WaveClearGraceSeconds && Swarm->GetAliveBrood() == 0`
(`Spike1GameMode.cpp:140`) — it needs **every last brood dead**, with no timeout. 5 of the 6
attempts stalemated at wave 2: a handful of brood (1-6, out of 450) stopped taking any further
casualties for tens of seconds at a time — `SwarmFight N: Stalemate ... brood 6->6 (killed 0)`
repeating — evidently having wandered somewhere the retinue's detection/pursuit doesn't
reliably reach. With brood count never hitting zero, `wave cleared` never fired and wave 3
never spawned; one run needed over 5 minutes past wave-2 spawn before the last stragglers
finally died and wave 3 opened. `Swarm.BroodAggroRange` (temporarily 600→900, §3) visibly
helped runs grind stragglers down faster but did not eliminate the failure mode outright.
**This is a real, reproducible gap in the current wave-clear logic** (not a blood issue, not
touched by this task — `Spike1GameMode.cpp` is out of scope here) worth a follow-up task in
its own right: either a wave-clear timeout, a stronger stray-brood pursuit behaviour, or both.
The `CsvProfile FRAMES=` budget had to be sized very generously (55,000 frames, ~8 minutes)
to have any chance of the capture window still being open whenever wave 3 finally arrived.

## 5. It washed out — seen on screen, then tuned down

**This is the finding that matters most in this report, and it overturned what §1-4 alone
would have implied.** The frame-time numbers above (measured with the *original* tuning,
`ParticlesPerHit 6` / `MaxBurstsPerFrame 24` / `SpeedScale 2.2`) say blood is cheap and
bounded. A screenshot taken partway through this same session, at only **wave-1 density**
(370 total entities — barely half of wave 2, a fraction of wave 3), said something different:

| Original tuning (6 / 24 / 2.2) — wave-1 density, mid-fight | |
|---|---|
| `Saved/Screenshots/SwarmDebugShot_20260729_002338.png` | A near-continuous red band spanning most of the visible fighting line — individual bursts have merged into one smear, not readable as discrete hits. |

This is exactly the failure mode the task brief warned about and asked to be tuned away if
found: *"if it washes out, tune it to what works and say what you changed and why."* At
double the entity count of wave 3 the wash would only be worse, so this was not a
borderline call.

**Fix applied, verified by re-shooting the same scenario twice:**

| Step | `ParticlesPerHit` | `MaxBurstsPerFrame` | `SpeedScale` | Result |
|---|---|---|---|---|
| Original | 6 | 24 | 2.2 | Continuous smear (above) |
| Pass 1 | 3 | 12 | 2.2 (unchanged) | `SwarmDebugShot_20260729_002559.png` — better, but bursts still bled into each other across most of the line |
| **Pass 2 (landed)** | **3** | **12** | **1.1** | `SwarmDebugShot_20260729_002743.png` — distinct clusters per hit point with visible black gaps between them. Reads as **flecks tracking the line**, not a wash |

Pass 1 alone (just cutting particle *count*) was not enough — the picture was still one
connected shape. What actually separated the bursts was cutting `SpeedScale` roughly in
half: 2.2 was spraying particles far enough from the hit point to overlap the *neighbouring*
hit's cloud, so the volume cut in pass 1 was fighting a spatial-overlap problem it couldn't
solve alone. `SpeedScale` had been bumped from 1→2.2 specifically for "punch" (1 "read
flat/floaty") before this wash was ever seen on screen — 1.1 is a compromise that keeps most
of that punch without spraying far enough to bridge gaps.

**One more honest data point, not fully resolved:** a third screenshot
(`SwarmDebugShot_20260729_003018.png`), taken at wave-2 density (570 entities) with the
*tuned* values, caught a moment of concentrated melee (many retinue fighting in one small
cluster rather than spread along a line) and shows two adjacent bursts merged into a single
denser red mass in that corner. **The tuned values fix the "spread along a long line" case
well but a tight mob of simultaneous hits can still locally merge into a small blob.** This
is a real, visible residual risk at higher density (wave 3 mobs harder — peak mobbed 14-15
vs this shot's more modest cluster) that a wave-3-density screenshot would confirm or deny —
see §7 for why that screenshot could not be taken this session.

**New values shipped** (`ELVTR/Config/SwarmExecOnPlay.canonical.txt`): `ParticlesPerHit 3`,
`MaxBurstsPerFrame 12`, `SpeedScale 1.1`. §1-4's frame-time numbers were measured at the
**original** (6/24/2.2) tuning, before this fix — halving the burst volume can only make
that cost picture better, not worse, so the measured delta is a conservative (worst-case)
number relative to what actually ships.

## 6. Particle budget shipped, and why

| CVar | Value | Why |
|---|---|---|
| `Blood.Enable` | 1 | Master on/off; 0 skips the scan entirely (zero cost off) |
| `Blood.ParticlesPerHit` | **3** (down from 6, §5) | Small burst per hit — "very simple" per the owner's own framing; cut in half after the wash was seen on screen |
| `Blood.MaxBurstsPerFrame` | **12** (down from 24, §5) | **The cost-bounding dial.** `SwarmAnim::HitFlashBit` stays set for the whole `Swarm.HitFlashTime` window (0.1s), not one frame, so many units can be mid-flash at once at horde density — without this cap, cost scales with hit rate, which is unbounded at 700 brood. Also halved as part of the wash fix |
| `Blood.Lifetime` | 0.3s | Sub-second by design — owner: "we dont have to have it live for long" |
| `Blood.Size` | 6uu | Pixel-scale, matched to the sprites — not an fx-scale splat. Not changed by the wash fix, though it's a candidate if the residual mob-cluster merging (§5) needs a second pass |
| `Blood.SpeedScale` | **1.1** (down from 2.2, §5) | Multiplier over NS_Blood's authored spray range. Owner had bumped 1→2.2 for punch before the wash was seen on screen; 1.1 is the compromise that separates adjacent bursts while keeping most of that punch |
| `Blood.HeightOffset` | 15uu | Lift above the (ground-corrected) hit position, toward roughly where a blow lands |

Tuned values live in `ELVTR/Config/SwarmExecOnPlay.canonical.txt`, the source the `/cvars`
skill regenerates `Saved/SwarmExecOnPlay.txt` from.

## 7. Wave-3-density screenshot: not captured, and why

**Neither state (on or off) has a wave-3-density (820-entity) screenshot.** Two separate,
unrelated obstacles stacked:

1. **Reaching wave 3 at all is slow and unreliable** (§4) — up to 6 attempts and 8 minutes
   per sample. The three screenshots in §5 are all wave-1/wave-2 density (370/570 entities)
   specifically because those are reachable in 10-40 seconds, which is what let three
   iterations of the tuning fix get shot and checked inside this session's time budget. A
   wave-3 shot would have cost that same iteration loop 8 minutes per try.
2. **`Swarm.DebugShotAfter`'s capture came back fully black on every attempt made deep into
   a run** — not the documented "blown out to flat white" exposure-convergence issue
   (`docs/AGENT-TEAMS.md` §8), a different failure with no content at all, ground plane
   included. Tried twice, both around t=+380-400s of real elapsed `PlayTime` (deep into a
   long, multi-restart session reaching wave 3 the slow way). The three *working* shots in
   §5 were all taken early (`PlayTime` 10-38s). This correlation — elapsed time, not
   necessarily entity count or mob density — is a better-supported theory than this report's
   first guess (camera clipped inside crowd geometry): the wave-2 shot in §5 caught a
   tight mob (two merged bursts) at t=38s and still rendered fine, which a pure
   density/clipping theory would not predict. **Not confirmed, not chased further** —
   `DebugCaptureComponent` lives in `SwarmRenderActor.cpp`, out of this task's file boundary
   (task-059's territory).

Recording both rather than picking one to report: the honest state is a capture pipeline
that works reliably early in a PIE session and unreliably (so far, always black) late into
one, for a reason not yet isolated.

## 8. What shipped, and what didn't

**Shipped, verified:**
- `UBloodSubsystem` (`ELVTR/Source/ELVTR/Rendering/BloodSubsystem.{h,cpp}`) — reads the
  swarm's published render arrays only, no sim change, decoupled from `SwarmRenderActor`/
  `SwarmFragments.h`/`Content/Spike1/**` per the file boundary.
- `NS_Blood` / `M_Blood` (`ELVTR/Content/Gore/`) — confirmed via
  `NiagaraToolset_System.GetEmitterData`: `SimTarget: CPUSim`, `InterpolatedSpawnMode:
  NoInterpolation` (both engine traps from the brief, verified correct, no fix needed).
  `M_Blood`: `MSM_Unlit`, `BLEND_Translucent`, two-sided — matches the sprites' unlit look.
  Exempted from the demichrome flame lift the same way units are (`Swarm.UnitStencil`,
  read live) — chosen because leaving it un-exempted would grade blood differently from the
  unit it came out of the instant it stood inside the flame's white core.
- Every dial is a CVar with house-style prose (§6), tuned values in
  `ELVTR/Config/SwarmExecOnPlay.canonical.txt`.
- **Blood ON and OFF both measured and clean at real wave-3 density (§1-3)** — the hard
  requirement this task exists to retire. Delta: +0.7ms FrameTime / +1.4ms GameThreadTime
  at the *original*, pre-fix tuning — a conservative (worst-case) number relative to what
  ships. Both comfortably inside budget either way.
- **The wash was found on screen (not inferred) and tuned down (§5)** — this is the
  "honest finding worth more than a clean number" the task exists for. Landed on
  `ParticlesPerHit 3` / `MaxBurstsPerFrame 12` / `SpeedScale 1.1`, verified by re-shooting.

**Not shipped / not done, stated plainly:**
- No wave-3-density (820-entity) screenshot, either blood state, either tuning (§7) —
  capture pipeline gap, pre-existing, not blood-specific, not chased further since it's
  outside this task's file boundary (`SwarmRenderActor.cpp`).
- One residual visual risk, seen but not resolved: a tight mob of simultaneous hits can
  still locally merge two bursts into one denser blob even at the new tuning (§5, third
  screenshot) — wave 3's heavier mobbing (peak mobbed 14-15) makes this worth re-checking
  with an actual wave-3 shot once §7's capture gap is fixed.
- The wave-clear stalemate (§4) is a real gap in `Spike1GameMode`'s wave logic worth its own
  follow-up task — flagged, not fixed (out of this task's owned files).
