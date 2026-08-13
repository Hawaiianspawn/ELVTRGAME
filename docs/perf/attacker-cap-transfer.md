# The attacker-cap transfer — what Kindled.Boss.Report actually counts

task-145. `PREFLIGHT.md` §1/§4 P2, made load-bearing by `docs/sim/SEVEN-VS-BOSS.md` §2: the
seven-vs-seventy claim in `castle-layout.md` §6.3 flips entirely on whether the real in-engine
boss surround cap is ~7 or in the documented 35-55 band. This doc's job was to measure it.

## Headline

**Inconclusive, and the reason it's inconclusive is itself the finding.** `Kindled.Boss.Report`
does not measure what §2's flip-point question needs. It reports the peak number of soldiers
whose blow *landed* on the boss in a single frame — a quantity that reads near-zero (1-3,
measured below) whether 7 soldiers are in reach or 45 are, because blows are spread across each
soldier's own ~0.9s swing cadence. Five real samples pulled from an in-engine session run on
this branch about an hour before this report all read 1-3 against a cap of 45, which is
consistent with slice-a7.md §10 row 10's own claim that the cap "never binds in practice" — but
that same evidence is equally consistent with the true concurrent-bodies-in-reach number being 7
or being 45. **The instrument cannot tell the two apart, so it cannot adjudicate SEVEN-VS-BOSS.md
§2.** What would adjudicate it, and why I didn't run it myself, is in §5.

## 1. What was measured, and how

**Build/branch:** `build-space-differentiates`, current commit at time of writing
(`cb06098`, "Add walls and the 1000-body war test scenario"). No source changes made for this
task.

**Method:** I do not have MCP console-toolset access in this task invocation (no
`KindledConsoleToolset`/`KindledSwarmToolset` tools were available to me — file and shell tools
only), and per my own operating constraints I do not launch or drive the Unreal Editor directly.
A live editor process for this exact project was already running when I started
(`UnrealEditor.exe`, PID 41592, window title `ELVTR - Unreal Editor`, started ~70 minutes prior)
— almost certainly the session that built and validated task-144's ability kit. Rather than start
a second instance (resource-lock risk per `PREFLIGHT.md` §1's own note that `unreal-editor`
serialises, plus the missing-modules/Live-Coding hang risk this project's memory already
documents, which a headless launch has no way to dismiss), I mined that session's own log,
`ELVTR/Saved/Logs/ELVTR.log`, for real `Kindled.Boss.Report` output it already produced.

That log shows an MCP-driven session (`LogModelContextProtocol: Dispatching toolset tool:
'ELVTREditor.KindledConsoleToolset.Exec'`) spawning the slice boss five times
(`Kindled.Boss.Spawn quilled,ram,sated` / `quilled,sated`) against the default anchored garrison
(116-132 retinue standing, `Follow`/anchored-Hold stance — nothing issued a `Charge` or
per-unit `Hold` override in this log) while exercising the four ability-kit verbs, and calling
`Kindled.Boss.Report` five times by hand. This is genuine, current, in-engine data on this
branch — not fabricated, not from a stale log — but it was not run *for* this task, so it only
covers one engagement shape (a normal front-line fight) and none of the three shapes §2 below
was asked to compare.

## 2. What `Kindled.Boss.Report` counts — read from source, not assumed

`ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp:1056`:

```cpp
if (bRetinue && bBossAlive && Strike[i].bStrikeFrame && BossStrikers < BossSurroundCap)
{
    ...
    if (OwnEntry && OwnEntry->BlowsClaimed < OwnEntry->TargetsPerHit)
    {
        ++OwnEntry->BlowsClaimed;
        ++BossStrikers;
        BossDamage += FMath::Max(OwnEntry->BlowDamage - MyArmor, BossChipFloor);
    }
}
...
Swarm->SetBossAttackers(BossStrikers);   // line 1188
```

`SwarmSubsystem.h:324-335`:

```cpp
void SetBossAttackers(int32 Count)
{
    BossAttackers = Count;
    BossAttackersPeak = FMath::Max(BossAttackersPeak, Count);
}
int32 ConsumeBossAttackersPeak()   // called by Kindled.Boss.Report
{
    const int32 Peak = BossAttackersPeak;
    BossAttackersPeak = 0;
    return Peak;
}
```

`BossStrikers` only increments for a soldier whose `Strike[i].bStrikeFrame` is true **this
frame** — i.e. the edge-triggered instant its own swing cadence lands a blow, not every frame it
stands in range. `SetBossAttackers` is called once per combat pass (every game-thread frame);
`BossAttackersPeak` is the running max of that per-frame count since the last
`Kindled.Boss.Report` call. So the number printed is **peak same-frame STRUCK count**, gated
twice over — once by `BossSurroundCap` itself (a soldier can't be counted past 45 in one frame
regardless of how many are truly in reach) and once by the fact that "in reach" and "struck this
frame" are different sets whose ratio is roughly `frame-time / swing-interval`.

**It is not "bodies in reach." It is a strict lower bound on bodies-in-reach, and a heavily
downward-biased one.** `entity-tiers.md` §4 itself already separates these two concepts —
`SurroundCapEstimate`/`BossSurroundCap` is "a sustained-combat concurrency limit," distinct from
the generic `Swarm.MaxAttackersPerUnit` (default 4) which gates simultaneous claims on an
*ordinary* Mass-entity victim (`SwarmCombatProcessors.cpp:934`, the ordinary
retinue-vs-brood/hero exchange loop) and is never consulted on the boss path at all — the boss
uses its own dedicated `Kindled.Boss.SurroundCap` (45) instead. **These are two separate,
already-implemented mechanisms, not one cap that "does or doesn't transfer."** The transfer
question `PREFLIGHT.md` §1 names (candidate (2), `MaxAttackersPerUnit` in the pooled
wave-attrition model) is about the *first* mechanism and the wave-attrition sim gap task-068
already investigated; it is a different question from SEVEN-VS-BOSS.md §2's boss-surround-cap
flip point, which is about the *second*. Worth untangling explicitly since the task brief treats
them as one question.

## 3. The five real measurements

From `ELVTR/Saved/Logs/ELVTR.log`, all against `brood_boss` (6000 HP, Armor 14) with
`Kindled.Boss.SurroundCap` at its shipped default (45), garrison standing 116-132, anchored/
`Follow` (no `Charge` order issued in this log — this is the "front arrival" shape, by default,
not a scripted one):

| Time (log) | Marks | Boss HP | Blow | **peak attackers** | cap |
|---|---|---|---|---|---|
| 22:48:36 | QUILLED+RAM+SATED | 5872/6000 | 77 | **2** | 45 |
| 22:49:11 | QUILLED+RAM+SATED | 4675/6000 | 77 | **3** | 45 |
| 22:50:54 | QUILLED+RAM+SATED | 5562/6000 | 77 | **2** | 45 |
| 22:51:02 | QUILLED+RAM+SATED | 4950/6000 | 77 | **1** | 45 |
| 22:53:14 | QUILLED+SATED | 5970/6000 | 220 | **2** | 45 |

Every sample sits 1-3 against a cap of 45 — the cap is nowhere close to binding, matching
slice-a7.md §10 row 10's own note. **This is real evidence that the cap doesn't visibly bind in
a normal front-line fight** (a genuine, if narrow, result). It is not evidence about how many
soldiers are actually in reach, for the reason in §4.

## 4. Why "peak struck" reads low regardless of true concurrency — counted-operations check

Each soldier's blow is edge-triggered once per its own swing interval (`Swarm.SwingInterval`
0.9s baseline, `Swarm.ArcherSwingInterval` 1.5s, ±0.2 jitter fraction per unit
`Swarm.SwingIntervalJitter`), desynced by `FSwarmJitterFragment`'s per-unit phase. For `N`
soldiers genuinely within reach of the boss, the expected number whose edge lands in any single
game-thread frame of length `dt` is approximately:

```
E[same-frame strikers] ≈ N * dt / SwingInterval
```

At 30fps (`dt ≈ 0.033s`) and the 0.9s baseline: `E ≈ N * 0.037`. For `N = 7` that's ~0.26/frame;
for `N = 45` that's ~1.67/frame. Both are small, both produce an observed peak-over-a-window in
the single digits (the Poisson-ish tail of a low-rate process, sampled over the ~10-35s windows
between the five reports above), and **the two are not distinguishable at this sample size** —
1-3 is a plausible peak for either. This is a counted-operations argument, not a new
measurement; it explains why §3's real data can't answer §2's question rather than papering over
the gap with a guess.

## 5. What would actually adjudicate it, and why I didn't run it

Two concrete options, neither requiring code:

1. **A `Kindled.Boss.SurroundCap` sweep, black-box.** Set the cap to something small (3, 5, 10)
   and watch whether `Kindled.Boss.Report`'s peak tracks the cap 1:1 (evidence the natural
   struck-rate exceeds even a low cap — though per §4 this still doesn't cleanly separate from
   "in reach") or plateaus below it. Weaker than a direct in-reach count but usable without
   touching source.
2. **A direct concurrent-bodies-in-reach count.** No existing log surface computes this — I
   searched `SwarmDebug.cpp` (only `Swarm.SpacingReport`, a nearest-neighbour distance
   histogram over the render buffers, not a per-target count) and `SwarmTelemetry.cpp` (only a
   static CSV dump of the `MaxAttackersPerUnit` *CVar value*, not a live measured count) —
   confirmed by reading both files in full. This is the one that actually answers §2's question,
   and it needs a counter alongside `bContact`/`InReach` at the boss-claim site
   (`SwarmCombatProcessors.cpp:1074`'s `BossDistSq <= MyReachSq/MyRangeSq` test) — one line, but
   it is a source change, which is outside this task's scope per its own hard constraint
   ("you should not need to touch C++... if you believe you need code, stop and say so").
   **Stopping and saying so:** this is that case.

Neither of these did I run, because I have no way to drive a live PIE/standalone session in this
task invocation, and starting a new `UnrealEditor.exe` process myself is outside what I'll do
unsupervised (see §1). Exact commands, ready to hand to whoever can run them:

```
Kindled.Boss.Clear
Kindled.Boss.Spawn quilled,ram,sated
Kindled.Seven.LogInterval 3
# shape 1 — front arrival: do nothing else, let the anchored garrison meet it
# shape 2 — Charge-ordered mob: Swarm.UnitStance 7 Charge   (warns; goes through anyway)
# shape 3 — surround from standstill: hold the garrison, let the boss walk in and stop
#           (already the default anchored behaviour — no extra command needed)
Kindled.Boss.Report        # call a few times per shape, 10-20s apart
```

Sweep test, same session:

```
Kindled.Boss.SurroundCap 5
Kindled.Boss.Report   # repeat
Kindled.Boss.SurroundCap 45   # restore before ending the session
```

## 6. The unit case (`MaxAttackersPerUnit` = 4 direct analogue)

Not measured, for the same reason. I confirmed there is no existing log surface for "how many
bodies simultaneously get a soldier-sized target in reach" — `SwarmDebug.cpp` and
`SwarmTelemetry.cpp` were read in full and contain no such counter; the only place
`MaxAttackersPerUnit` appears outside the CVar definition is the gate at
`SwarmCombatProcessors.cpp:934` itself and a static CSV/log dump of its configured value
(`SwarmTelemetry.cpp:280-344`), never a live measured count of how many claims actually land
against it. Per the task's own instruction, reporting this gap honestly rather than inventing
instrumentation.

## 7. Verdict against SEVEN-VS-BOSS.md §2

§2's table needs one number — the real `SurroundCapEstimate`/`BossSurroundCap` — to know whether
seven-vs-seventy flips (cap ≈ 7) or stays a 5.0x-7.9x loss for the seven (cap in 35-55). **This
task does not supply that number.** What it supplies is narrower and still worth having:

- `Kindled.Boss.Report`'s "peak attackers" is not that number and cannot become it without a
  source change (§5.2) — quoting it into §2's table, as originally asked, would misrepresent a
  struck-per-frame count as a concurrency count and should not be done.
- The five real samples (§3) rule out one specific failure mode — the cap is not visibly pinned
  at 45 in ordinary play — but say nothing about whether the true concurrency is nearer 7 or
  nearer 45, because (§4) the metric can't see that far.
- SEVEN-VS-BOSS.md §2's own finding stands as written: **the flip point (cap = 7) is still
  unconfirmed in-engine, and the documented 35-55 band is still an unmeasured Fermi estimate.**
  This task neither closes nor advances that gap; it establishes that the tool named to close it
  (`Kindled.Boss.Report`) cannot, on its own, and names what could (§5).

## What remains unmeasured

- Concurrent bodies-in-reach of the boss, at any engagement shape — the number §2 actually
  needs. Needs one new counter at the boss-claim site (source change, out of scope here).
- The three engagement shapes (front arrival, Charge-ordered mob, standstill surround) as a
  controlled comparison — only the front-arrival shape has any real data behind it (§3), and
  that data was incidental to a different task, not a controlled capture.
- The unit-case analogue of `MaxAttackersPerUnit = 4` (§6) — no data at all, no existing readout.
- The `Kindled.Boss.SurroundCap` sweep falsification test (§5.1) — proposed, not run.
