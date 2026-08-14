# The attacker-cap transfer — what Kindled.Boss.Report actually counts

task-145. `PREFLIGHT.md` §1/§4 P2, made load-bearing by `docs/sim/SEVEN-VS-BOSS.md` §2: the
seven-vs-seventy claim in `castle-layout.md` §6.3 flips entirely on whether the real in-engine
boss surround cap is ~7 or in the documented 35-55 band.

## Headline

**The real number isn't a number, it's a curve, and it crosses the flip point mid-fight.**
Converting two controlled standalone captures' HP-loss rate into effective concurrent attackers
(`N_eff`, derivation in §4) gives:

| Phase | N_eff | vs SEVEN-VS-BOSS.md §2's cap=7 flip point |
|---|---|---|
| Front contact (boss parked against the line) | **0.7 - 1.0** | far below — seven ≈ seventy regime |
| Mid-push (boss advancing through a thinned line) | **~2.9** | below — still seven ≈ seventy regime |
| Late-front, static but thinning (bonus sample, §4) | **~7.0** | at the flip point |
| Surrounded endgame (boss pinned near the bearer) | **~16.2** | above — seventy-wins regime |

None of these sit anywhere near the documented 35-55 `SurroundCapEstimate` band. Most of a fight's
duration (the front-contact and mid-push phases measured here span 57s + 39s = 96s) sits *below*
the cap=7 flip point; only the closing ~18s, once the boss is pinned and the survivors compress
onto it, clears 7 and starts to look like the 35-55 band's "mass wins" story. **A single static
`SurroundCapEstimate` is the wrong shape of number for what the engine actually does** — real
concurrency is small and rises sharply as a fight closes, not a fixed ceiling. §7 unpacks what
that means for §2's table.

## 1. What was measured

**Build/branch:** `build-space-differentiates`, same commit as before (`cb06098`). No source
changes.

**Method:** the team lead ran the two captures I couldn't run myself (no MCP console-toolset
access in this task invocation, and a live editor session was already open on this project —
see the previous revision of this doc for why I didn't drive it myself). Two standalone `-game`
launches, unmarked boss, no abilities cast, `Kindled.Seven.LogInterval 3` (which also fires
`Kindled.Boss.Report`'s line every 3s, `Spike1GameMode.cpp:492`), brood waves running throughout
(unavoidable — `WaveBroodCounts` is hard-coded, not a dial):

```
capmeasure-A-front.log:
UnrealEditor.exe ... ELVTR.uproject L_Spike1 -game -windowed -ResX=1280 -ResY=720 ^
  -ExecCmds="Kindled.Boss.AutoWave 1,Kindled.Boss.AutoMarks none,Kindled.Seven.LogInterval 3" ^
  ABSLOG=...\capmeasure-A-front.log

capmeasure-B-charge.log:
UnrealEditor.exe ... ELVTR.uproject L_Spike1 -game -windowed -ResX=1280 -ResY=720 ^
  -ExecCmds="Kindled.Boss.AutoWave 1,Kindled.Boss.AutoMarks none,Kindled.Seven.LogInterval 3,Swarm.UnitStance 7 Charge" ^
  ABSLOG=...\capmeasure-B-charge.log
```

Garrison: 128 bodies, mixed Spearmen/Archers, no adaptation rungs assigned (only the Seven get
`SetSquadRung` in this slice — `SwarmProcessors.cpp:620-624`: "nothing about today's shipped
balance moves until a rung is assigned"), so combat stats are the shipped Gate 1 defaults
(§4 blend). Boss: 6000 HP, Armor 14, unmarked.

## 2. The `Swarm.UnitStance 7 Charge` correction to the brief

**Not a refusal — an overwrite, and the timing is the whole story.** `capmeasure-B-charge.log`
line 1913 shows the command actually executing:

```
LogTemp: Warning: Swarm.UnitStance: unit 7 is THE GARRISON — the war, not your squad. ...
LogTemp: Display: Swarm: unit 7 stance = CHARGE (type SPEARMEN)
```

`SetUnitStance` ran and set the garrison's stance to `Charge`, exactly as the command's own doc
comment says it will (`SwarmCommands.cpp:625-627`, "the command still goes through, deliberately
... it warns so it can never happen by accident"). It just didn't matter: `-ExecCmds` runs at
engine boot, **before** the level's `Deploying`-phase setup, and that setup unconditionally
re-anchors the garrison every run start —

```cpp
// Spike1GameMode.cpp:401-402
const FVector Line = SwarmSpawn::TideBearingPoint(GetWorld(), CVarWarStandoff.GetValueOnGameThread());
Swarm->SetUnitStance(USwarmSubsystem::GarrisonUnit, ESwarmStance::Hold, Line);
```

— which stomps the boot-time `Charge` back to `Hold` a few frames later, before any brood exist to
charge at. (One more small trap: `LogTheSeven`'s garrison line always prints the literal string
`"unordered"` — `Spike1GameMode.cpp:287-288` — it's a hardcoded label, not a live read of
`GetUnitStance(GarrisonUnit)`, so it can't be used as evidence either way.) **The fix for a real
Charge capture is to issue the command live, after `Run: restarted` fires, not via `-ExecCmds`.**
Not done here — flagged for whoever runs that capture next.

What run B measured instead — the boss killing its immediate contact pocket and then walking
forward through the line as that pocket died and the *next* nearest retinue centroid pulled it
on (`ResolveTarget`'s ordinary unmarked fallback, `SpikeBossActor.cpp:185-187`) — turned out to be
a more informative shape than a scripted charge would have been: it's the same "mid-push" and
"surrounded endgame" progression a real siege produces on its own.

## 3. What `Kindled.Boss.Report` counts — unchanged from the previous revision

`SwarmCombatProcessors.cpp:1056` gates `BossStrikers` on `Strike[i].bStrikeFrame` — a soldier only
counts the frame its own ~0.9s-cadence blow lands, not every frame it stands in range.
`SetBossAttackers`/`ConsumeBossAttackersPeak` (`SwarmSubsystem.h:324-335`) then reports the peak
single-frame value since the last line. **Both captures confirm the §4 (previous revision)
cadence math directly: peak-strikers-per-frame sits pinned at 1 (occasionally 0 or 2) for the
entire ~5.5 minutes of both runs, regardless of phase** — it never once distinguishes the
front-contact phase (real concurrency ~0.7-1) from the surrounded endgame sixteen lines later
(real concurrency ~16). This is exactly why §4's `N_eff` had to be derived from the HP slope
instead of read off the instrument directly.

Also unchanged and still worth keeping visible: `Swarm.MaxAttackersPerUnit` (default 4, the
generic per-victim clamp for ordinary brood/retinue exchange, `SwarmCombatProcessors.cpp:934`)
and `Kindled.Boss.SurroundCap` (default 45, the boss's own dedicated concurrency cap,
`SwarmCombatProcessors.cpp:1056`) are two separate, already-implemented mechanisms. The boss path
never consults `MaxAttackersPerUnit`. `PREFLIGHT.md`'s "transfer" framing names the first; §2's
flip point is about the second — don't conflate them.

## 4. Deriving N_eff from the HP slope

Neither capture's peak-strikers instrument resolves concurrency (§3), so `N_eff` — effective
concurrent attackers — comes from how fast the boss actually lost HP, divided by how much damage
one attacking body deals per second, post-Armor:

```
N_eff = slope(HP/s) / blended per-body DPS-after-Armor(HP/s)
```

**Blend assumption, stated explicitly (not the 19/blow guess from the brief):** the garrison
carries no adaptation rung, so its combat stats are the shipped defaults —
`docs/data/unit-types.json` Spearmen 30 DPS / `Swarm.SwingInterval` 0.9s cadence → blow
`30 × 0.9 = 27`; Archers `Swarm.ArchersDPS` 18 / `Swarm.ArcherSwingInterval` **1.5s** (not 0.9 —
the brief's formula assumed a uniform 0.9s cadence, which archers don't run on) → blow
`18 × 1.5 = 27`. Both blows land at 27 pre-Armor; against the boss's Armor 14
(`Kindled.Boss.ArmorChipFloor` 3), both reduce to the *same* post-Armor blow of
`max(27-14, 3) = 13` — coincidence of these particular numbers, not a modelling choice. Per-body
DPS still differs by cadence: melee `13/0.9 = 14.44 HP/s`, archer `13/1.5 = 8.67 HP/s`. Blended at
`docs/data/unit-types.json`'s shipped `growth_source_weight` (0.8 Spearmen / 0.2 Archers — the
same split `entity-tiers.md` §7 uses for its own army-composition sim):

```
blended per-body DPS = 0.8 × 14.44 + 0.2 × 8.67 = 13.29 HP/s
```

**Deviation from the brief's literal formula, stated so it isn't silently different:** the brief
said divide by `(blow / 0.9s)` uniformly. I used each role's real cadence instead (0.9 melee,
1.5 archer) before blending, because forcing archers through a 0.9s divisor overstates their
per-body contribution by 67% — 14.44 vs their real 8.67 HP/s. The two post-Armor blow values
happening to match (13 = 13) made this easy to get wrong by not noticing the cadence still
differs.

| Phase | Window | ΔHP | slope (HP/s) | **N_eff** |
|---|---|---|---|---|
| A — front contact, steady state, boss parked (2009,6) | 57.03s | 555 | 9.73 | **0.73** |
| B — front contact 2, boss parked (1664,-56) before advancing | 30.02s | 391 | 13.03 | **0.98** |
| B — mid-push, boss walking (1535,-57)→(475,44) | 39.01s | 1522 | 39.02 | **2.94** |
| A — bonus: late front, static (2479,-166), garrison thinning | 45.02s | 4163 | 92.47 | **6.96** |
| B — surrounded endgame, boss pinned (475,44) to death | 18.01s | 3885 | 215.76 | **16.24** |

The two independent front-contact samples (A: 0.73, B: 1.0) agree closely — good cross-check that
the steady-state number is real and not a fluke of one run. The bonus row (A, boss never moved,
same location the whole time) shows `N_eff` climbing sharply anyway, from garrison attrition
alone thinning the field down to a denser knot around the boss — concurrency is not just a
function of boss position, it rises as the fight progresses even standing still.

## 5. Instrument-vs-derivation gap (§5 from the previous revision, resolved)

The previous revision proposed a `Kindled.Boss.SurroundCap` sweep and a new bodies-in-reach
counter as the two ways to get a real number without the peak-strikers instrument's blind spot.
The HP-slope derivation in §4 supplied a working substitute for the sweep — it didn't need the
cap touched at all. **The bodies-in-reach counter is still not built** (still a source change,
still out of scope here) and would be the more precise version of §4's estimate — `N_eff` is a
damage-rate proxy for concurrency, not a body count, and assumes every attacking body in a phase
deals its *full* blended rate the whole window, which smooths over the phase's own internal
variance (the mid-push window, for instance, covers several discrete stop-and-resume segments).

## 6. The unit case (`MaxAttackersPerUnit` = 4 direct analogue)

Unchanged: still not measured, still no existing log surface for "how many bodies simultaneously
get a soldier-sized target in reach" (confirmed by a full read of `SwarmDebug.cpp` and
`SwarmTelemetry.cpp` in the previous revision). Neither capture measured this — both were boss
fights.

## 7. Verdict against SEVEN-VS-BOSS.md §2

§2's table needs `SurroundCapEstimate` to know whether seven-vs-seventy flips (cap ≈ 7) or stays
a 5.0x-7.9x loss for the seven (cap in the documented 35-55). The measured answer is **it's both,
at different points in the same fight**:

- **For most of a fight's duration** (front contact + mid-push, 96 of the ~114 measured seconds
  in run B), `N_eff` sits at 0.7-2.9 — **below** the cap=7 flip point, in the "seven ≈ seventy"
  regime §6.3 claims. At these numbers, a fixed roster of seven specialists genuinely isn't
  giving up much to a fixed roster of seventy of the same tier, because *neither* squad can get
  more than a handful of bodies onto the boss regardless of its own size.
- **Only in the closing stretch**, once the boss is pinned and the survivors compress onto it
  (the last 18 of ~114 seconds here), does `N_eff` climb to ~16 — past the flip point and into
  the direction the documented 35-55 band argues, where more bodies means a faster kill.
- **The documented 35-55 band itself is not supported anywhere in this data.** The highest
  measured `N_eff`, in the single most concentrated moment of two fights, is 16.2 — under half
  the bottom of that range. Nothing measured here reaches it.

**What this means for §2's table, plainly:** quoting a single number into it — 7, or 16, or
anything else — would misrepresent a quantity that visibly isn't constant. §2's own "clean-fight
caveat" already names the gap this falls into: the point-target model computes one static TTK
number per cap value, and the real fight this task measured has three-to-five different regimes
in under two minutes. **Closing this properly needs either a time-weighted or phase-aware
extension to the point-target model, not a single corrected constant** — that's a modelling
change to `Scripts/sim/`, not a number for this doc to hand over. What this task *can* say with
confidence: pick any single cap value to represent "the real cap" and it will be wrong for most
of the fight, because most of the fight is nowhere near it.

## What remains unmeasured

- A direct bodies-in-reach count (§5) — `N_eff` is a damage-rate proxy, not a body count.
- The unit-case `MaxAttackersPerUnit` analogue (§6) — no data, no existing readout.
- A genuine Charge-ordered mob capture — `Swarm.UnitStance 7 Charge` needs to be issued live,
  post-run-start, not via `-ExecCmds` (§2).
- Marked-boss variants (Quilled/Ram/Sated) — both captures ran unmarked, per the brief.
- Whether `N_eff`'s rise late in a fight is really "boss pinned near the bearer" or just
  "garrison thinned enough that a higher fraction of what's left clusters" — the bonus row in §4
  (boss never moved, `N_eff` still climbed to ~7) suggests the latter matters at least as much as
  position, but the two captures here don't isolate them.
