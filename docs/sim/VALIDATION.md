# Validation suite — actual output (committed for the record)

**Revision note (this section first, two rounds now):**

1. The first version of this file reported a 3x3 sensitivity sweep as
   evidence that a frontage-capped wave-attrition model cannot reproduce
   GATE1's measured survival number "across its entire documented parameter
   range." **That sweep was invalid — it was an algebraic tautology, not a
   test** — retracted below in full rather than quietly corrected.
2. The regression guard written to prevent that exact bug from recurring
   **did not catch it.** A second review reintroduced the bug and re-ran the
   guard as originally written (`TargetsPerHit` 1 vs 8): it passed anyway.
   The guard is fixed further down this file, and its own failure is
   recorded here in full for the same reason as (1): **a test that cannot
   fail is worth less than no test, because it is actively reassuring.**
   This file is now the place that lesson is on record twice, once for a
   sweep and once for the guard meant to replace it.

Both rounds are preserved below rather than edited away, because a future
reader needs to know a negative result — and a passing guard — were each
reported once that could never have shown anything else.

Run: `py Scripts/sim/validate.py`, 2026-07-29 (updated after the guard fix below).

```
=== task-063 validation suite ===

Militia vs Fodder TTK = 2.0000s (expected 2.0s) -> PASS
Hero(55dps) vs Elite TTK = 21.6000s (expected 21.6s) -> PASS
GATE1 calibration: retinue survivors = 0.00 of 120 (measured range 109-111) -> FAIL
Cleave sensitivity guard (K=MaxAttackersPerUnit=4 vs K=2xMaxAttackersPerUnit=8): 25.8 vs 19.7 enemy survivors, delta=6.1 (threshold 2.0) -> PASS
[smoke, not the regression guard] TargetsPerHit=1 -> 188.7, TargetsPerHit=8 -> 19.7 enemy survivors, delta=169.0 -> PASS
[bonus] Army(N=120, 80/20) vs Elite TTK = 1.848s (entity-tiers.md §7 table: 1.85s) -> PASS

REQUIRED closed-form checks (1, 2): PASS
Cleave-sensitivity regression guard (4): PASS
GATE1 wave-attrition reproduction (3): FAIL
```

## Checks 1 & 2 — closed-form TTK — PASS, exactly

Unchanged from the original run. Both reproduce `entity-tiers.md` §7's
stated numbers to 4 decimal places (exact rational arithmetic on committed
stat blocks). `combat_model.ttk_1v1` is trusted.

## Bonus check — point-target army TTK — PASS, exactly

Unchanged. Reproduces `entity-tiers.md` §7's own N=120-vs-Elite row (1.85s)
to 3 decimal places. `floor3-boss-point-target.json` runs on this model —
**trusted**, and untouched by everything below (the bug was wave-attrition-only).

## The bug that invalidated the original check-3 sweep

**Found in review, not by this harness's own author.** The wave-attrition
model bounded outgoing cleave damage (`TargetsPerHit`) by reusing
`engaging_enemy_melee` — the SAME quantity already computed to bound
*incoming* damage from `MaxAttackersPerUnit`. Substitute the two
definitions:

```
raw_slots            = exposed_retinue * TargetsPerHit
engaging_enemy_melee = min(enemy_melee_alive, exposed_retinue * MaxAttackersPerUnit)
```

Whenever the frontage cap actually bound (`engaging_enemy_melee = exposed_retinue *
MaxAttackersPerUnit`, i.e. population large relative to the frontage — true
for most of a wave-1-scale fight), `exposed_retinue` cancels out of
`contact_scale = engaging_enemy_melee / raw_slots`, leaving the CONSTANT
`MaxAttackersPerUnit / TargetsPerHit` (= 4/8 = 0.5 at shipped defaults),
independent of `EngagedSpacingUU`, `FormationSpacingUU`, exposed count, or
anything else. **`TargetsPerHit` — the retinue's entire designed cleave
advantage over single-target Fodder — had been deleted from the model's
outcome by construction.** The 3x3 sweep across `EngagedSpacingUU` x
`MaxAttackersPerUnit` in the original version of this file "found" that
every cell wiped the retinue; it could not have found anything else, because
none of the swept parameters could move the ratio that actually determined
the winner (`RetinueMaxHP/EnemyBlow` vs `EnemyMaxHP/RetinueBlow` — a fixed
15600/31.5 vs 15000/27 comparison, entirely independent of the frontage
model). **That sweep is retracted as evidence of anything about arrival
timing or any other design question.**

## The fix — decoupling cleave reach from the incoming-attacker cap

"How many enemies can hit me" (an adjacency/elbow-room limit at point-blank
range, `MaxAttackersPerUnit`) and "how many enemies can I reach with a
weapon" (`TargetsPerHit` within `EngageRange`, a longer and geometrically
distinct reach — GATE1's own geometric Kth-nearest-within-range targeting)
are different physical quantities. `combat_model.melee_reach_per_exposed_unit`
now derives the second one independently, off `EngageRange` and a new,
separately-flagged Fermi input (`MeleeContactFacingFraction`, default 0.5 —
see `combat-model-constants.json`), with **no reference to
`MaxAttackersPerUnit` anywhere in its derivation.** Full mechanism in
`docs/sim/MODEL.md`.

## The regression guard, round 1 — also didn't catch the bug, and why

**First attempt (`validate.py` check 4, original version):** forced
`TargetsPerHit` to 1 and to 8 (shipped) on an otherwise identical
GATE1-calibration setup and asserted the two runs differ by more than 5
enemy survivors. Result: 188.7 (K=1) vs 19.7 (K=8) — delta 169, PASS.
**This guard is worthless, and a second review proved it by reintroducing
the original bug byte-for-byte and re-running the suite unmodified: it
still passed.**

The reason: the bug's constant-ratio collapse only fires when `TargetsPerHit
>= MaxAttackersPerUnit` (4, shipped). Below that ratio, `min(1, M/K)`
hasn't saturated yet and the bugged model still responds to `TargetsPerHit`
completely normally — so a check comparing K=1 (below M) against K=8 (above
M) straddles the saturation point, and the *entire* delta it measures comes
from the K=1-to-K=4 climb, which is IDENTICAL under the bugged and fixed
model. Confirmed empirically by sweeping K with M=4 fixed:

| K | 1 | 2 | 4 | 6 | 8 | 16 |
|---|---:|---:|---:|---:|---:|---:|
| **fixed** model, enemy survivors | 188.7 | 121.8 | 25.8 | 20.2 | 19.7 | 19.7 |
| **bugged** model, enemy survivors | 188.7 | 121.8 | 25.8 | 25.8 | 25.8 | 25.8 |

Identical up to K=4 (=`MaxAttackersPerUnit`), then the bugged model FREEZES
— every K above `MaxAttackersPerUnit` produces byte-identical output, which
is exactly what the cancellation predicts. The fixed model keeps improving
(cleave still has marginal value past K=4, bounded by local weapon-reach
geometry) and only truly flattens out much higher, against the living
population bound.

## The regression guard, round 2 — fixed: compare K=M against K=2M

The actual discriminator sits entirely ABOVE the saturation point.
`check_cleave_sensitivity()` now compares `K = MaxAttackersPerUnit` against
`K = 2 x MaxAttackersPerUnit` (both derived from `combat-model-constants.json`,
not hardcoded, so the guard keeps working if that CVar default is ever
retuned) and requires a delta greater than 2 enemy survivors. Verified both
ways, same reintroduce-the-bug-and-rerun method the reviewer used:

- **Bug present:** K=4 -> 25.8, K=8 -> 25.8, delta=0.0 -> **FAIL** (correctly
  catches it — exit code 1).
- **Bug fixed (current state):** K=4 -> 25.8, K=8 -> 19.7, delta=6.1 -> **PASS**.

The original K=1-vs-8 comparison is kept as `smoke_check_cleave_wired()` — a
fine sanity check that cleave is wired into the model at all — but it is
explicitly labeled non-gating and NOT the regression guard, both in its
output line and its docstring, so nobody mistakes it for coverage it doesn't
have.

## Check 3, re-run honestly — GATE1 wave-1 reproduction — still FAIL, but now a real result

Same setup as before (120 Militia vs 250 Fodder, GATE1's exact zero-input
configuration). At the harness's committed defaults (`MaxAttackersPerUnit`
4, `EngagedSpacingUU` 45, `MeleeContactFacingFraction` 0.5): **retinue wiped
by t=4.5s, ~20 of 250 Fodder survive.** Still a total miss against the
measured 109-111 survivors — but this time the model is actually capable of
representing the opposite outcome (see below), so the miss means something.

### Full 27-cell sweep — `EngagedSpacingUU` x `MaxAttackersPerUnit` x `MeleeContactFacingFraction`

All three of the model's free-but-cited dials, full cross product, no
cherry-picking:

| ES | MA | FF | Retinue survivors | Enemy survivors | Time | Result |
|---:|---:|---:|---:|---:|---:|---|
| 25 | 1 | 0.25 | 29.80 | 0.00 | 300.6s | **enemy_wiped** |
| 25 | 1 | 0.5  | 29.80 | 0.00 | 300.6s | **enemy_wiped** |
| 25 | 1 | 1.0  | 29.80 | 0.00 | 300.6s | **enemy_wiped** |
| 25 | 2 | 0.25 | 0.00  | 0.63  | 9.0s   | retinue_wiped |
| 25 | 2 | 0.5  | 0.00  | 0.63  | 9.0s   | retinue_wiped |
| 25 | 2 | 1.0  | 0.00  | 0.63  | 9.0s   | retinue_wiped |
| 25 | 4 (shipped) | 0.25 | 0.00 | 19.28 | 4.5s | retinue_wiped |
| 25 | 4 | 0.5  | 0.00  | 19.28 | 4.5s   | retinue_wiped |
| 25 | 4 | 1.0  | 0.00  | 19.28 | 4.5s   | retinue_wiped |
| 45 (harness default) | 1 | 0.25 | 47.87 | 0.00 | 300.6s | **enemy_wiped** |
| 45 | 1 | 0.5  | 47.87 | 0.00 | 300.6s | **enemy_wiped** |
| 45 | 1 | 1.0  | 47.87 | 0.00 | 300.6s | **enemy_wiped** |
| 45 | 2 | 0.25 | 13.22 | 0.00 | 300.6s | **enemy_wiped** |
| 45 | 2 | 0.5  | 13.22 | 0.00 | 300.6s | **enemy_wiped** |
| 45 | 2 | 1.0  | 13.22 | 0.00 | 300.6s | **enemy_wiped** |
| 45 | 4 (shipped) | 0.25 | 0.00 | 27.76 | 4.5s | retinue_wiped |
| 45 | 4 (shipped) | 0.5 (shipped) | 0.00 | 19.73 | 4.5s | retinue_wiped |
| 45 | 4 | 1.0  | 0.00  | 19.28 | 4.5s   | retinue_wiped |
| 51 | 1 | 0.25 | 47.61 | 0.00 | 300.6s | **enemy_wiped** |
| 51 | 1 | 0.5  | 53.26 | 0.00 | 300.6s | **enemy_wiped** |
| 51 | 1 | 1.0  | 53.26 | 0.00 | 300.6s | **enemy_wiped** |
| 51 | 2 | 0.25 | 8.56  | 0.00 | 300.6s | **enemy_wiped** |
| 51 | 2 | 0.5  | 20.33 | 0.00 | 300.6s | **enemy_wiped** |
| 51 | 2 | 1.0  | 20.33 | 0.00 | 300.6s | **enemy_wiped** |
| 51 | 4 (shipped) | 0.25 | 0.00 | 89.51 | 3.6s | retinue_wiped |
| 51 | 4 | 0.5  | 0.00  | 20.42 | 4.5s   | retinue_wiped |
| 51 | 4 | 1.0  | 0.00  | 19.28 | 4.5s   | retinue_wiped |

**15 of 27 cells now have the retinue winning** — a real, non-tautological
range of outcomes this time, driven almost entirely by `MaxAttackersPerUnit`
(`MA`): at `MA=1` the retinue wins in all 9 (ES, FF) combinations; at `MA=2`
it's mixed (5 of 9 win); at the shipped default `MA=4` it loses in all 9.
`TargetsPerHit`'s role is now real too (see the guard above) but doesn't
show up as a swept axis here since it isn't a "free" combat-model-constants.json
dial — it's a per-entity stat already committed in `unit-types.json` /
`entity-tiers.json`, and this sweep only varies the three dials this harness
owns.

**At the harness's actual committed defaults (MA=4, ES=45, FF=0.5 — the
shipped `MaxAttackersPerUnit` CVar default and the two Fermi estimates this
harness picked as its stated midpoints), the retinue is wiped.** This is a
real result now, not an artifact: it says reproducing GATE1's measured
survival with this model requires assuming a STRICTER incoming-attacker cap
than the game's own shipped `Swarm.MaxAttackersPerUnit=4` default — either a
sign the pooled/geometric approximation doesn't transfer that CVar cleanly
(see `docs/sim/LIMITATIONS.md`), or a sign that something else in the model
(most likely `exposed_frontage`'s own perimeter estimate) is too generous.

**Even in the model's best-case cell (MA=1, FF=0.5 or 1.0), survivors top
out at 53.26 of 120 — under half the measured 109-111.** So the honest
conclusion is narrower than the original (retracted) claim: this is not "the
model always fails no matter what," it's "the model, at its cited/shipped
default parameters, fails; and even hand-picking the most favorable
untested parameter combination in its swept range, it still falls well short
of the measured number." See `docs/sim/LIMITATIONS.md` for what's still
unaccounted for.

### What this means for the shipped scenarios

`floor1-swarm-wave.json` (40 vs 250) and `floor2-ranged-wave.json` (50 vs
450) both still resolve as retinue wipes at the harness's default
parameters when run (`py Scripts/sim/scenario_runner.py --all`) — read their
survivor counts as illustrating the (now-correct) mechanism, not as
validated predictions, same caveat as before, for the same underlying
reason (check 3 still fails at defaults). `floor3-boss-point-target.json`
is unaffected by any of this (point-target model, untouched) and remains
trustworthy.

No constant in `combat-model-constants.json` was changed to make this table
look better in either direction — `EngagedSpacingUU` (45), `MaxAttackersPerUnit`
(4), and `MeleeContactFacingFraction` (0.5) are the harness's defaults
because they're the documented/cited/midpoint values, not because of
anything in this table.

## task-068 — arrival gating tested against candidate (1), and the result

`docs/data/encounter-budget.json` (task-004) shipped real, cited per-rank
arrival timing derived from `Swarm.BroodSpawnRadiusMin`/`BroodFormation.*`/
`BroodSpeed` CVar defaults — the data `LIMITATIONS.md` §1 candidate 1 named as
missing. `combat_model.simulate_wave_attrition` now has an `arrival_seconds`
gate per `WaveGroup` (default 0.0, opt-in, see `docs/sim/MODEL.md` §3 for the
exact mechanism). `gate1-calibration-wave1.json`'s single 250-count
`brood_fodder` row is now 5 per-rank rows carrying `ArrivalSeconds`
5.85/6.29/6.73/7.17/7.60 (`rank_arrival_timing[]`'s
`gate1_calibration_wave1_rank0..4`, nominal, no jitter) — same 250 total,
only the timing changed.

Run: `py Scripts/sim/validate.py`, 2026-07-29 (after wiring in arrival gating):

```
=== task-063 validation suite ===

Militia vs Fodder TTK = 2.0000s (expected 2.0s) -> PASS
Hero(55dps) vs Elite TTK = 21.6000s (expected 21.6s) -> PASS
GATE1 calibration: retinue survivors = 0.00 of 120 (measured range 109-111) -> FAIL
Cleave sensitivity guard (K=MaxAttackersPerUnit=4 vs K=2xMaxAttackersPerUnit=8): 25.8 vs 19.7 enemy survivors, delta=6.1 (threshold 2.0) -> PASS
[smoke, not the regression guard] TargetsPerHit=1 -> 188.7, TargetsPerHit=8 -> 19.7 enemy survivors, delta=169.0 -> PASS
[bonus] Army(N=120, 80/20) vs Elite TTK = 1.848s (entity-tiers.md §7 table: 1.85s) -> PASS

REQUIRED closed-form checks (1, 2): PASS
Cleave-sensitivity regression guard (4): PASS
GATE1 wave-attrition reproduction (3): FAIL
```

Check 3 still fails, with the identical `0.00` survivor count. **The number
in the printed line didn't move — what changed is invisible at the top level
and has to be read from the full tick log:**

```
py Scripts/sim/scenario_runner.py gate1-calibration-wave1

  Retinue: 120 start -> 0.0 survivors
  Enemy:   250 start -> 19.2 survivors
  Result: retinue_wiped  (elapsed 11.7s)
       t   retinue     enemy   exposed   dmg->ret  dmg->enemy
     0.0     120.0     250.0     74.21        0.0         0.0
     5.4     120.0     250.0     74.21        0.0         0.0
    10.8       0.0     19.24       1.0      126.0       189.0
```

The retinue takes **zero** damage before the front rank arrives (~5.85s,
confirmed by `dmg->ret = 0.0` through t=5.4s) — arrival gating is genuinely
wired in and doing something real. Elapsed time to resolution nearly tripled
(4.5s ungated -> 11.7s gated). **But the final tally is the same fight,
delayed:** retinue still fully wiped, enemy survivors 19.2 (gated) vs. 19.73
(ungated, `combat-model-constants.json` defaults) — within noise of each
other, not a meaningfully different outcome.

**Checked this isn't an artifact of the nominal timing** — reran with the
full `BroodSpeedJitter` ±6% bracket (`ArrivalSecondsFast`/`ArrivalSecondsSlow`
substituted for `ArrivalSecondsNominal` on all 5 ranks, ad hoc script, not
committed — the point was to check robustness, not to produce a fourth
scenario file):

| Arrival timing | Retinue survivors | Enemy survivors | Elapsed | Result |
|---|---:|---:|---:|---|
| Fast (-6%) | 0.00 | 20.19 | 11.7s | retinue_wiped |
| Nominal | 0.00 | 19.24 | 11.7s | retinue_wiped |
| Slow (+6%) | 0.00 | 22.50 | 11.7s | retinue_wiped |
| (ungated baseline) | 0.00 | 19.73 | 4.5s | retinue_wiped |

Retinue survivors are `0.00` in all three jittered runs and enemy survivors
sit in a tight 20-23 band around the ungated baseline's 19.73. **Robust, not
a coincidence of the nominal number.**

**The mechanistic reason, checked directly against the harness's own
numbers, not asserted:** at N=250 enemy vs. a 120-retinue formation,
`exposed_retinue * MaxAttackersPerUnit` = 74.21 x 4 = ~296.8 already exceeds
the entire enemy population. `engaging_enemy_melee = min(enemy_melee_alive,
exposed_retinue * MaxAttackersPerUnit)` is therefore bounded by
`enemy_melee_alive` itself, not by the frontage cap, for essentially any
nonzero rank count. Once any meaningful fraction of the enemy has arrived,
the incoming-damage rate is "full arrived-population DPS" — the same regime
as the t=0 case, just entered later. Nothing in `simulate_wave_attrition`
depends on elapsed time or accumulated fatigue, only on current alive-counts
— so a pure arrival delay cannot, by construction, change what the fight
converges to, only when it starts converging.

**Verdict on `LIMITATIONS.md`'s two candidates, at the confidence this
result actually supports:** candidate (1) (arrival/spawn-pacing timing) is
now tested with real, cited, shipped-derived data, not estimated or assumed,
and demonstrably does not close the check-3 gap — robust across the full
measured jitter bracket. This is evidence against candidate (1) as the
(or even a significant part of the) explanation, not proof it contributes
nothing at all to the real game's measured survival (a real per-entity sim
has geometry, fatigue-independent mechanics, and player input this pooled
model still doesn't capture — see `docs/sim/LIMITATIONS.md` §4). Candidate
(2) (`MaxAttackersPerUnit`'s pooled-vs-per-entity transfer) is untouched by
this change and, by elimination within what this harness can test, is now
the stronger of the two open explanations — still not confirmed, still
needs an in-engine measurement to actually check, per `LIMITATIONS.md`'s own
standing caveat.

**Cleave-sensitivity guard (check 4) and the closed-form checks (1, 2, bonus)
are unaffected** — `arrival_seconds` defaults to `0.0` everywhere except the
one scenario this task edited, so every other check ran on byte-identical
inputs to before.

## task-069 — the 27-cell sweep above, reproduced from committed source

The 27-cell table above was originally produced by an uncommitted script —
this file was, at the time, the only record of both the sweep and its
result. `Scripts/sim/sweep.py` now exists (`docs/sim/SWEEPS.md`) and can
reproduce it as a single command:

```powershell
py Scripts/sim/sweep.py gate1-calibration-wave1 `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank0].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank1].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank2].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank3].ArrivalSeconds=0" `
  --axis "scenario:Enemy.Composition[Name=brood_fodder_rank4].ArrivalSeconds=0" `
  --axis "constants:wave_attrition_model.EngagedSpacingUU=25,45,51" `
  --axis "constants:wave_attrition_model.MaxAttackersPerUnit=1,2,4" `
  --axis "constants:wave_attrition_model.MeleeContactFacingFraction=0.25,0.5,1.0"
```

**Every one of the 27 rows above reproduces exactly** (spot-checked all 27
cells against this table's numbers — byte-identical, e.g. cell
ES=51/MA=4/FF=0.25 -> 89.51 enemy survivors, 3.6s, matching this table's
corresponding row exactly). The five `ArrivalSeconds=0` axes are required and
are not decorative: `gate1-calibration-wave1.json`'s own data changed under
task-068 (single 250-count row -> 5 timed per-rank rows, 5.85-7.60s), so
running the sweep against the *current* fixture unmodified gives different
numbers (e.g. 19.24 vs this table's 19.73 enemy survivors at the shipped
ES=45/MA=4/FF=0.5 cell) — a real, expected divergence from the fixture's own
data changing, not a harness regression. Zeroing all five reconstructs the
exact pre-task-068 configuration this table was originally run against. Full
account and both demonstration sweeps (game-balance data, encounter
composition): `docs/sim/SWEEPS.md`.

## task-076 — the variance layer, its safety-property proof, and its validate.py checks

Adds `Scripts/sim/scenario_runner.py`'s `run_trials()`/`compute_trial()` and
`Scripts/sim/combat_model.py`'s `jitter_arrival_seconds`/`jitter_fighter_dps`
— see `docs/sim/MODEL.md` §4 for the design and `docs/sim/LIMITATIONS.md` §6
for what a resulting spread may and may not be used to argue. This section
records the actual before/after safety-property proof and the full
`validate.py` output with the three new checks, per that task's own evidence
bar.

### The before/after identity proof

Before any code changed, `py Scripts/sim/scenario_runner.py <name> --json`
was captured for all 10 runnable committed scenarios (`retinue-subtypes.json`
is picked up by `data_loader.list_scenarios()` but has no `Kind` field and
was never runnable — `py Scripts/sim/scenario_runner.py --all --json`
errors on it with the same `KeyError: 'Kind'` both before and after this
task's changes, confirmed byte-for-byte identical traceback content, just
different line numbers because the file grew — a pre-existing, unrelated
condition, not touched or caused by this task). After landing the variance
layer, all 10 were re-run and diffed against the "before" capture:

```
floor1-swarm-wave: IDENTICAL
floor2-elite-point-target: IDENTICAL
floor2-elite-point-target-recruitmax: IDENTICAL
floor2-ranged-wave: IDENTICAL
floor3-boss-point-target: IDENTICAL
floor3-boss-point-target-recruitmax: IDENTICAL
floor3-elite-point-target: IDENTICAL
gate1-calibration-wave1: IDENTICAL
gate1-calibration-wave2: IDENTICAL
gate1-calibration-wave3: IDENTICAL
```

**Byte-identical, all 10.** `py Scripts/sim/drift_check.py` was also re-run
against the UNCHANGED `docs/sim/baseline.json` (not refreshed — refreshing
is explicitly not this task's call, see `docs/sim/DRIFT-CHECK.md`) and
diffed against its own pre-task capture: also byte-identical, `RESULT:
CLEAN`, exit 0. This proof was repeated a second time after a formatting
accident (a Python `json.dump` revert pass round-tripped the constants file
through `ensure_ascii=True`, corrupting its unicode punctuation into
`\uXXXX` escapes) was caught and fixed by hand — the identity/drift proof
above is the FINAL state, confirmed clean after that fix, not the
pre-fix state.

### `py Scripts/sim/validate.py` — full output, checks 5/6/7 added

```
=== task-063 validation suite ===

Militia vs Fodder TTK = 2.0000s (expected 2.0s) -> PASS
Hero(55dps) vs Elite TTK = 21.6000s (expected 21.6s) -> PASS
GATE1 calibration: retinue survivors = 0.00 of 120 (measured range 109-111) -> FAIL (see docs/sim/VALIDATION.md for the read)
Cleave sensitivity guard (K=MaxAttackersPerUnit=4 vs K=2xMaxAttackersPerUnit=8): 25.8 vs 19.7 enemy survivors, delta=6.1 (threshold 2.0) -> PASS
[smoke, not the regression guard] TargetsPerHit=1 -> 188.7, TargetsPerHit=8 -> 19.7 enemy survivors, delta=169.0 -> PASS
[bonus] Army(N=120, 80/20) vs Elite TTK = 1.848s (entity-tiers.md §7 table: 1.85s) -> PASS
Variance-off identity (10 scenarios, 20 fields) -> PASS
Variance reproducibility (same root_seed, two independent run_trials calls, 8 trials each) -> PASS
Variance order-independence (serial vs reversed vs 4-worker process pool, 8 trials): reversed match=True, pool match=True -> PASS

REQUIRED closed-form checks (1, 2): PASS
Cleave-sensitivity regression guard (4): PASS — TargetsPerHit still has real effect.
GATE1 wave-attrition reproduction (3): FAIL — per task-063's own instructions, this is reported honestly, not fudged. See docs/sim/VALIDATION.md and docs/sim/LIMITATIONS.md for the numbers and the read on why. Wave-attrition scenario OUTPUT (floor1/floor2) should be read as illustrative of the MECHANISM (frontage concurrency limits), not as a trusted survivor-count prediction, until this closes. A variance layer (task-076) does not change this verdict — see docs/sim/LIMITATIONS.md's variance-layer section: a spread can never be used to declare this check passing 'within variance.'
Variance-layer regression checks (5, 6, 7): PASS — variance-off output is identity-locked, and seeded trials are reproducible and order-independent.
```

Exit code 0. Checks 5 (identity), 6 (reproducibility), and 7
(order-independence — serial vs reversed-order vs a 4-worker
`ProcessPoolExecutor`, all three agree) all pass and all gate the exit code,
same as check 4. Check 3 is unaffected and remains non-gating, unchanged.

### Two distributions at `--trials 200`, and what they do NOT show

Both committed variance sources default `"enabled": false`
(`combat-model-constants.json`'s `variance_model` block). The runs below
required a temporary, manual edit of that committed file to `true` for the
duration of one demonstration, following the same "flip -> demonstrate ->
revert -> confirm clean" method `docs/sim/DRIFT-CHECK.md`'s own
demonstration already uses — reverted immediately after, confirmed by the
identity/drift proof above (the FINAL one, post-revert). No `--trials`/
`--seed` flag can turn a source on; it is a committed-file edit, by design
(`docs/sim/MODEL.md` §4).

**wave_attrition** — `py Scripts/sim/scenario_runner.py
gate1-calibration-wave1 --trials 200 --seed 1` (both sources temporarily
enabled):

```
=== GATE1 calibration - wave 1 (validation fixture, not a design scenario) (gate1-calibration-wave1) - 200 trials, root_seed=1 ===
  kind: wave_attrition
  variance sources enabled: arrival_jitter, damage_roll_jitter
  ====================================================================
  DIAGNOSTIC: an enabled variance source is HARNESS-INVENTED (no shipped
  CVar or committed data backs its magnitude). Treat this spread as
  illustrative only, not a measured confidence band.
  ====================================================================
   retinue_survivors: n=200  mean=    0.024 median=    0.000 p5=    0.000 p95=    0.000 min=    0.000 max=    2.340 stdev=   0.221
     enemy_survivors: n=200  mean=   22.062 median=   21.100 p5=    2.075 p95=   43.788 min=    0.000 max=   53.070 stdev=  13.892
     elapsed_seconds: n=200  mean=   16.146 median=   11.700 p5=   10.800 p95=   13.500 min=    9.900 max=  300.600 stdev=  35.203
```

**Reading it, precisely and no further:** `retinue_survivors` mean 0.024,
median 0.0, p95 0.0, max observed 2.34 of 120 across 200 trials — the
retinue is wiped in the overwhelming majority of trials, with a thin tail
that never approaches GATE1's measured 109-111. **This is the check-3
failure shown to be robust under this layer's own cited-plus-invented
perturbations, not a different result.** Per task-076's explicit
instruction and `docs/sim/LIMITATIONS.md` §6: this is reported as that
observation, with both sources' citation status stated plainly above (one
cited, one flagged diagnostic) — not reframed as check 3 passing, not used
to argue anything about the real game's actual variance, and no magnitude
in `variance_model` was chosen or would be chosen to move this number.

**point_target** — `py Scripts/sim/scenario_runner.py
floor2-elite-point-target --trials 200 --seed 1` (`damage_roll_jitter`
applicable and enabled; `arrival_jitter` doesn't apply to this model kind):

```
=== Floor 2 - Elite point-target fight (balanced-path retinue) (floor2-elite-point-target) - 200 trials, root_seed=1 ===
  kind: point_target
  variance sources enabled: damage_roll_jitter
  ====================================================================
  DIAGNOSTIC: an enabled variance source is HARNESS-INVENTED (no shipped
  CVar or committed data backs its magnitude). Treat this spread as
  illustrative only, not a measured confidence band.
  ====================================================================
         ttk_seconds: n=200  mean=    2.167 median=    2.155 p5=    1.890 p95=    2.480 min=    1.840 max=    2.610 stdev=   0.192
```

This one is entirely diagnostic — its only enabled source is the invented
`damage_roll_jitter` — and is shown only to demonstrate the point-target
model kind also produces a distribution, not as any kind of finding. The
underlying point estimate (2.13s, this file's own bonus check and
`docs/sim/LIMITATIONS.md` §3) remains the trustworthy number; this spread
is not.
