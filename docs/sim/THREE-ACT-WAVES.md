# Three-act waves (early/mid/late) — task-103, run against task-102's curve

**What this is:** three `wave_attrition` scenario fixtures built directly from
`docs/design/wave-scaling-three-act.md` / `docs/data/wave-scaling.json`
(task-102), run through the committed harness, plus this report. It does not
propose or adjust a single number in task-102's curve — it runs the curve as
written and reports what came out, per the same convention
`scaling-curve.md` §7 and `entity-tiers.md` §7 already use.

Files written:
- `docs/data/scenarios/three-act-early.json`
- `docs/data/scenarios/three-act-mid.json`
- `docs/data/scenarios/three-act-late.json`

**Read `docs/sim/LIMITATIONS.md` §1 before reading a single number below as a
prediction.** The short version, restated inline at every survivor count in
this report because a skim-reader must not come away thinking these are
forecasts: **the wave-attrition model does not reproduce its own single
measured baseline** (GATE1's 109-111-of-120 wave-1 survival) at the harness's
committed defaults — it predicts a full retinue wipe there instead, and even
its single best untested parameter cell only reaches 53/120 (under half).
Every survivor count below inherits that exact gap.

---

## 1. Validation state before running anything new

`py Scripts/sim/validate.py` — same result as already on record in
`docs/sim/VALIDATION.md`, no new failures introduced by adding these three
scenarios:

```
Militia vs Fodder TTK = 2.0000s (expected 2.0s) -> PASS
Hero(55dps) vs Elite TTK = 21.6000s (expected 21.6s) -> PASS
GATE1 calibration: retinue survivors = 0.00 of 120 (measured range 109-111) -> FAIL (pre-existing, on record)
Cleave sensitivity guard (K=4 vs K=8): 25.8 vs 19.7 enemy survivors, delta=6.1 (threshold 2.0) -> PASS
[smoke] TargetsPerHit=1 -> 188.7, TargetsPerHit=8 -> 19.7, delta=169.0 -> PASS
[bonus] Army(N=120, 80/20) vs Elite TTK = 1.848s (entity-tiers.md §7: 1.85s) -> PASS

REQUIRED closed-form checks (1, 2): PASS
Cleave-sensitivity regression guard (4): PASS
GATE1 wave-attrition reproduction (3): FAIL
```

Check 3's FAIL is the pre-existing, already-diagnosed gap (`VALIDATION.md`'s
task-068 section) — not something this task introduced or needs to explain
further.

`py Scripts/sim/drift_check.py` — **CLEAN**, all 7 baselined sweeps
unperturbed (A through G, 0 cells drifted). None of the 7 baseline sweeps
reference the three new scenario files, so this is exactly the expected
result, not a coincidence.

`py Scripts/sim/scenario_runner.py --all` **crashes with a bare `KeyError:
'Kind'`**, but not on anything I touched — it dies on
`docs/data/scenarios/retinue-subtypes.json`, which per the task brief
"currently has uncommitted changes from other work" and, confirmed by
reading it, isn't a scenario file at all (no `Kind` field — it's a
sub-type stat-profile candidate table from a different task, `--all`
iterates every `.json` in the directory and has no way to skip it). This is
a pre-existing gap unrelated to task-103: alphabetically `retinue-subtypes`
sorts before `three-act-*`, so `--all` never even reaches my three new
files. I ran each of the three individually instead
(`scenario_runner.py three-act-early` / `-mid` / `-late`) — all three run
cleanly, no errors, shown below. Not fixing `retinue-subtypes.json` or
`scenario_runner.py`/`data_loader.py`'s `--all` dispatch — both outside this
task's file ownership.

---

## 2. Results — all three waves

```
py Scripts/sim/scenario_runner.py three-act-early
  Retinue: 60 start -> 0.0 survivors
  Enemy:   120 start -> 3.8 survivors
  Result: retinue_wiped  (elapsed 5.4s)
       t   retinue     enemy   exposed   dmg->ret  dmg->enemy
     0.0     30.92      66.0     52.48     3780.0      3240.0

py Scripts/sim/scenario_runner.py three-act-mid
  Retinue: 120 start -> 0.0 survivors
  Enemy:   400 start -> 221.2 survivors
  Result: retinue_wiped  (elapsed 1.8s)
       t   retinue     enemy   exposed   dmg->ret  dmg->enemy
     0.0     29.79    287.75     66.38    10758.4      8512.8

py Scripts/sim/scenario_runner.py three-act-late
  Retinue: 600 start -> 0.0 survivors
  Enemy:   20000 start -> 19641.2 survivors
  Result: retinue_wiped  (elapsed 0.9s)
       t   retinue     enemy   exposed   dmg->ret  dmg->enemy
     0.0       0.0   19641.2    148.43   160437.6     28260.8
```

| Wave | Retinue start | Retinue survivors | Enemy start | Enemy survivors | Result | Elapsed |
|---|---:|---:|---:|---:|---|---:|
| 1 Early | 60 | **0.0** | 120 | **3.8** | retinue_wiped | 5.4s |
| 2 Mid | 120 | **0.0** | 400 | **221.2** | retinue_wiped | 1.8s |
| 3 Late | 600 | **0.0** | 20,000 | **19,641.2** | retinue_wiped | 0.9s |

**Every one of these six survivor counts is directional, not predictive** —
`docs/sim/LIMITATIONS.md` §1's gap applies identically to all three rows.
None of these waves is a case where the harness's known failure mode happens
not to apply.

None of the three scenarios includes the wave's Elite/Boss instances
(`wave-scaling.json`'s `elite_boss_schedule`) — `scenarios.schema.md` states
Elite/Titan/Boss are `point_target`-only entities, not representable in a
`wave_attrition` `Composition` row, and `LIMITATIONS.md` §3 notes the
point-target model itself assumes a clean 1-on-1 fight with nothing else
competing for attention, so there's no existing model that captures "Elite
embedded in a live swarm" at all. Wave 2's result above is the swarm-only
fight, minus its 1 Elite; wave 3's is the swarm-only fight, minus its 3 Elite
+ 1 Boss. Both are a lower bound on each wave's actual difficulty, not the
full encounter.

---

## 3. Which waves the harness can say something trustworthy about — none of them, in the predictive sense

**Trustworthy for:** illustrating the mechanism (frontage/exposed-count
attrition, the same qualitative shape GATE1-calibration-wave1/2/3 already
show at 120-vs-250/450/700) and for *relative* comparison within the
harness's own model (e.g. wave 2 introduces ranged combat and the model
does register a change in exposed-count and damage split when
`brood_soldier_ranged` enters the enemy composition, per `LIMITATIONS.md`
§2's "relative comparisons within itself" clause).

**Not trustworthy for:** any of the six survivor numbers above, taken as a
forecast of what an actual playtest would show. All three waves share the
identical, already-documented root cause (`VALIDATION.md`'s task-068
section): `exposed_frontage` grows only as `sqrt(alive_melee_count)`, so
once enemy population is large enough that
`exposed_retinue * MaxAttackersPerUnit` no longer bounds
`engaging_enemy_melee`, the incoming-damage rate is set by the enemy's full
arrived population, not by any frontage limit — and that threshold is
already crossed at wave 1's own modest 60-vs-120 (ratio 2.00, below even
GATE1's win-side 2.08 calibration point but still a wipe here at these
smaller absolute counts, because `MaxAttackersPerUnit`'s shipped default of
4 combined with `exposed_frontage`'s formula produces a similar dynamic at
this scale too — see the wave-1 tick line above: `exposed=52.48` out of 60
retinue, essentially the whole force already engaged at t=0). None of the
three waves sits in a regime this model has been shown to get right.

If forced to rank them by how much of a leap each one is *beyond* the
model's own already-tested envelope (not by how much I trust the number):
wave 1 (60 vs 120, ratio 2.0) is the closest to GATE1's tested band
(2.08-5.83); wave 2 (120 vs 400, ratio 3.33) still sits inside that band;
**wave 3 (600 vs 20,000, ratio 33.3) is nearly 6x past the highest ratio
(5.83) this model's 27-cell sweep or any GATE1 calibration point has ever
touched** — not a new failure mode, just the same one taken to an extreme
nobody has checked before.

---

## 4. The population where the pooled model stops meaning anything

**It does not break down computationally.** `three-act-late` (20,600 total
entities) ran in 0.079s wall-clock, resolved in 0.9s simulated time, produced
finite, sane-looking numbers (no NaN, no overflow, no hang) — the harness
handles 20,000-scale populations fine as arithmetic. If the question is
"does it hang or produce garbage at this scale," the answer is no.

**But "doesn't crash" is a much lower bar than "means something," and this
run clears the first without coming close to the second.** There is no
population or ratio at which this pooled wave-attrition model has been
*validated* — its one calibration point (GATE1 N=250, ratio 2.08) already
fails at defaults, and its best untested cell across the full 27-cell sweep
still only reaches 53/120 survivors. So there's no "breakdown point" to
report as a number past a known-good region, because there is no known-good
region for this model to begin with. What I can report specifically:

- **Wave 3's result is the same known mechanism, just far more extreme, not
  a new pathology triggered by scale.** `exposed_retinue` at N=600 is 148.43
  (sqrt-scaling, per `combat_model.exposed_frontage`'s docstring: perimeter
  grows with `sqrt(N)`, not `N`). At `MaxAttackersPerUnit=4` that bounds
  incoming attackers at ~594 — trivial against a 20,000-strong enemy
  population, so incoming damage is set by the enemy's full population from
  t=0 (`dmg->ret = 160,437.6` in a single first tick, versus a 600-strong
  retinue's pooled HP). The retinue is wiped in under one tick's reporting
  interval; the enemy, at nearly 300x the retinue's ranged+melee
  throughput, loses under 2% of its population (359 of 20,000) in that same
  window.
- **This ratio (33.3) is ~5.7x past the highest ratio (5.83, GATE1's
  120-vs-700 wave-3) this model's calibration or its 27-cell sweep has ever
  exercised.** Nothing in the harness's validated (or even tested) range
  says whether 33.3 "should" look like this, worse, or better — this is
  genuinely uncharted territory for the model, not an interpolation within
  a tested band.
- **Practical read:** the harness is safe to *run* at 20,000+ population
  (no engineering concern), but the number it hands back at that scale
  carries the least confidence of anything in this report — strictly more
  speculative than wave 1 or wave 2's already-untrustworthy numbers, because
  it's also the furthest extrapolation past anything this model has been
  checked against.

---

## 5. Contradictions between task-102's numbers and committed data files — both self-reported candidates verified, both real

**(a) Wave 1's 100% Fodder composition diverges from `scaling-curve.md`
§1's own floor-1 row.** Verified directly:
`docs/design/scaling-curve.md` line 40 — `| 1 | 250 | 85% (212) | 15% (38) |
0% |` (85% Fodder / 15% Soldier-melee at floor 1). `wave-scaling.json`'s
`wave1_early` enemy composition is 100% Fodder (120 of 120), 0%
Soldier-melee. **Confirmed real, and already self-disclosed** in
`wave-scaling-three-act.md` §2 as a deliberate choice (so wave 2's
Soldier-melee introduction reads as a genuinely new unit rather than a
density bump of something wave 1 already had) — not an oversight, but a
literal divergence from the cited table nonetheless, worth restating
plainly here since this report is the first place both numbers have been
run against each other rather than just cross-referenced in prose.

**(b) Wave 3's 600-count retinue exceeds every economy-derived growth
scenario this repo has simulated.** Verified directly against
`docs/data/scaling-curve.json`'s `growth_scenarios` block: `balanced_floor3`
tops out at `TotalUnits: 60`, `recruit_max_floor3` (the more aggressive
path) tops out at `TotalUnits: 90` — both floor-3 endpoints, both far short
of 600. `docs/data/economy.json`'s `supply.start_capacity` is 60. **Confirmed
real** — 600 is roughly 6.7x the most aggressive simulated growth
endpoint (90) this repo has on record, and 10x `start_capacity`. Also
self-disclosed in the design doc (§1: "a proposal past anything
`scaling-curve.json`'s own simulated growth scenarios reach"), not new
information, but independently confirmed against the actual file rather
than taken on the design doc's word.

**One additional contradiction found, not self-reported by task-102:**
wave 1's retinue (60) exactly equals `economy.json`'s `supply.start_capacity`
(60), which the design doc states resolves the upkeep-degrade collision at
wave 1 specifically. That's correct as far as it goes — but it means **wave
1 is the only one of the three waves where that collision is resolved**:
wave 2's retinue (120) is 2x `start_capacity`, and wave 3's (600) is 10x —
both already flagged by the design doc itself in its own §1 and §7
handoff to task-101, so this isn't a new finding, just confirming the
doc's own forward-flag is accurate against the actual `economy.json` value
rather than restating it secondhand.

No other numeric mismatch was found between `wave-scaling.json` and
`entity-tiers.json`/`unit-types.json`/`upgrades.json` — the retinue
composition rows (`spearmen`/`archers`, `militia` tier) and enemy tier keys
(`brood_fodder`/`brood_soldier_melee`/`brood_soldier_ranged`) all resolve
cleanly against those files' actual keys (confirmed by the scenarios
running without a `data_loader.py` lookup error).

---

## 6. Follow-ups noted, not built here

- **Chained run with carryover.** `run-slice-three-wave.json` already
  chains the GATE1 fixtures with survivor carryover, degrade, and embers via
  `run_sim.py`. If task-102's per-wave retinue counts (60/120/600) are meant
  to be reached by carryover + growth-site purchase rather than independent
  flat refills each wave (§1's "Refill-to-rising-cap" vs. "Growth-site
  purchase" — the design doc explicitly does not choose between them), a
  `three-act-*` companion chain is the natural next step. Not built here —
  out of this task's four-file scope, and would need task-102 (or a
  follow-up) to pick one of its two named mechanisms first.
- **Point-target companion for the Elite/Boss instances.** None of the
  three wave_attrition fixtures represents the embedded Elites or the wave-3
  Boss at all (§2 above). A `point_target` scenario for each could report a
  TTK the way `floor3-boss-point-target.json` does, but that TTK would
  still inherit `LIMITATIONS.md` §3's "fought clean, nothing else competing
  for attention" assumption — exactly wrong for an Elite embedded in a live
  swarm fight, per `scaling-curve.md` §1's own decision that they always
  are. Flagging rather than building a misleading number.
