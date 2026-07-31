Decision record for the `economy.json` / `growth-sites.json` supply reconciliation (task-101).
Extends `SYSTEMS.md` §7 and `economy.json`'s own `supply`/`site.actions` blocks; does not replace either.

## The collision, and the fix

`economy.json`'s `supply.start_capacity` was 60. `run-slice-three-wave.json` — the
only committed multi-wave run this task's tools can chain — starts every scenario
at 120 retinue (`gate1-calibration-wave1/2/3.json`, GATE1's refill-to-cap
convention). Demand (120 × `upkeep_per_unit`=1) was therefore already 2x capacity
before wave 1's first tick, so the retinue fought the whole run at the
`min_multiplier` floor's neighborhood from t=0 — not a played outcome, a data
mismatch between two independently-authored numbers (task-096's finding).

**Fix: `start_capacity` 60 → 120**, matching the retinue count the committed run
chain actually starts at, so demand==capacity and the opening multiplier is 1.0
instead of 0.50. Verified directly (`compute_degrade` in the re-run below): every
branch of `run-slice-three-wave.json` now opens undegraded.

This is a reconciliation, not an intentional-degrade call — `economy.json`'s own
`slice_targets.design_intent` already states the design goal ("played well [the
economy layer] turns the narrow loss into a win"), which a fixed, unconditional
2x-oversubscription at t=0 directly contradicts regardless of play. Silence was
the actual bug; there is no case for defending the collision as intended.

**A different, already-committed "40" also exists in this file**
(`slice_targets.start_units`) and is not the same number as the 120 above —
it's the *real* floor-1 headcount the growth-site economy is meant to produce
in actual play (cross-checked twice already: `floor1-swarm-wave.json`,
`docs/data/retinue-vanguard.json`). `gate1-calibration-wave1.json`'s own
description names its 120 a "gate-1-only prototype convention, not what any
floor of the actual game hands the player" — it exists to reproduce
`GATE1-FUN-PROTOTYPE.md`'s own zero-input engine measurement, not to model the
real economy. `start_capacity=120` reconciles against the 120 because that's
the number actually driving the only re-runnable multi-wave chain this task
has to report against; it does not disturb the 40-unit convention, since 40 is
comfortably under 120 either way (`floor1-swarm-wave.json`'s selftest — 40
units under a 60-or-120 capacity, multiplier guaranteed 1.0 — still passes
unchanged). Flagging this rather than silently picking one: a future task
that needs `start_capacity` sized against the *real* 40-unit floor economy
specifically (rather than the 120-unit validation fixture) should re-open this
number, not assume 120 was derived from the 40-unit convention.

## `provision`'s re-cost — fixes the greedy-drop bug, not a claim it now binds

`growth-sites.json`'s afford-check (`Scripts/sim/decisions.py`'s
`resolve_spend`, read-only to this task) drops the single costliest action,
repeatedly, until the target set fits the bank. At the old costs
(recruit 12 / promote 15 / provision 10), `promote` was the costliest of the
three — so when `triangle` (37 Embers) didn't fit a ~33-35 Ember bank, the
check dropped `promote`: the one lane task-097 measured as the standout
lever (`promote_only` beat every other single-lane branch by the full
measured spread). `triangle` ended up spending MORE Embers than
`promote_only` for a WORSE result.

**Fix: `provision` 10 → 16 Embers**, making it the costliest of the three
instead of `promote`. `promote` and `recruit` are untouched. Re-run
(`py Scripts/sim/decisions.py`, SECONDARY/TERTIARY — see full numbers below):
at growth-A, `triangle` now drops `provision` and keeps `promote`+`recruit`
(27 Embers, fits); at growth-B, with a bigger bank, all three now fit.
Result: `triangle`'s `total_killed` goes from 797.4 (worse than `promote_only`
at 859.0, spending more) to **932.5 — now the best of all five branches**,
strictly beating every single-lane branch including `promote_only`. This is
the specific bug the task named, fixed by reordering price rather than
touching the locked drop-priciest-first rule.

This re-cost is justified independent of the ordering fix, too: `provision`
is a pure support lane (raises a ceiling, does nothing by itself), and
task-097 already measured it as the one lane with *zero* standalone
contribution (`provision_only` byte-identical to `hoard`). Pricing the
speculative, support-only lane above the two lanes with measured direct
combat value is the more defensible ordering on its own terms, not just a
mechanism-driven patch.

**This does NOT mean `provision` binds on its own — it still doesn't, and the
re-run confirms why.** Pulled straight from the per-wave JSON
(`secondary_hypothetical_MA1`, real `economy.json` capacity): every one of
the 5 branches' `demand` sits at or under `capacity` at every wave, including
`provision_only` (`cap` rises to 145/170 across its two purchases, but
`demand` never gets anywhere close even at 120). The mechanism: this
harness's casualty curve crashes headcount to ~32-40% of the 120 start
within a single wave, for every branch — after that, no realistic amount of
`recruit`ing gets demand back near even the *unraised* 120 capacity, so
`provision`'s ceiling is never approached again. The one moment demand does
sit exactly at capacity (wave 0, t=0) has no growth-site stop before it in
`run-slice-three-wave.json`'s own structure (`Stops` only fire *after* a wave
completes) — so no spend, `provision` included, can ever act on it.

**Read this finding through `docs/sim/LIMITATIONS.md` §1, not as a verdict on
the mechanic.** That file already states the wave-attrition model does not
reproduce `GATE1-FUN-PROTOTYPE.md`'s own measured wave-1 result (109-111 of
120 retinue survive, ~92%) — it predicts a full wipe instead. A harness that
crashes headcount to a third of capacity within one wave, when the real
engine measurement holds it near full strength, is not good evidence that a
capacity-ceiling mechanic is pointless in the actual game — it's evidence
this particular pooled casualty curve is steeper than the real one, which
LIMITATIONS.md already said not to trust for absolute survivor counts. On
that basis: **`provision` is kept, unscoped, re-costed only** — cutting a
mechanic because an already-flagged-unreliable harness can't make it bind
would be tuning to the model's known bug, exactly what `LIMITATIONS.md`
already warns against doing to `combat-model-constants.json`. If a future,
validated version of this harness (post the §1 gap closing) still shows
`provision` never binding, that's the point to revisit the mechanic itself.

## Re-run results (task's required evidence)

`py Scripts/sim/run_sim.py run-slice-three-wave`:

```
   w scenario                   ret_start  ret_surv enemy_start enemy_surv         result  demand   cap  mult  embers+ embers_tot
   0 gate1-calibration-wave1        120.0       0.0       250.0       19.2  retinue_wiped   120.0 120.0  1.00    23.08      23.08
  RUN ENDED EARLY: retinue wiped on wave 0 -- remaining waves not run.
```

`mult` is now `1.00` (was `0.50`) — the supply collision's effect on this run
is gone. The run still wipes wave 1: with `demand==capacity` there is no
degrade left to blame, so this remaining wipe is entirely the separate,
already-tracked `LIMITATIONS.md` §1 gap (the pooled frontage model
under-predicting survival relative to the real engine), not a supply-economy
problem — out of this task's scope to close. One relative improvement worth
noting: enemy survivors dropped from 132.2 (pre-fix, degraded) to 19.2
(post-fix, undegraded) — much closer to the real engine's own ~19-23 enemy
survivor range for this fixture, even though `LIMITATIONS.md` §1 still means
the retinue side of this same tick log should not be trusted as a survivor
prediction.

`py Scripts/sim/decisions.py`:

**PRIMARY (committed, MA=4) — verdict unchanged: THEATRE.** All 5 branches
still byte-identical (`spread(total_killed)=0.00`) — the run still wipes wave
1 before growth-A, for the LIMITATIONS §1 reason above, not a supply reason.
Reconciling supply capacity does not and cannot move this verdict; it was
never the cause of PRIMARY's wipe once `demand==capacity` removed the degrade
component.

**SECONDARY (MA=1, hypothetical combat override, REAL economy.json capacity)
— now reaches both stops and wins waves 1-2 outright, every branch.** This is
the direct fix to task-097's sharpened finding: previously, `MA=1` alone won
the fight but the (old) real 60-capacity degrade sank it anyway. Re-run at
the reconciled capacity: `waves_survived=2` for all 5 branches — an
independently-winning combat cell is no longer sunk by the supply collision.
**SECONDARY and TERTIARY are now numerically identical** (`decisions.py`'s
TERTIARY diagnostic override sets capacity to 120 — the same value
`economy.json` now carries for real — so the override is inert; TERTIARY's
`--capacity-override=120` mechanism is unedited and still labeled
hypothetical, it just no longer differs from the real economy).

**growth-site verdict, SECONDARY/TERTIARY: REAL DECISION at both stops, and
`triangle` is now the best branch, not a trap.**

```
  branch           target                       waves_surv final_ret   killed  embers
  hoard            (none)                                2       0.0    709.5   90.94
  recruit_only     recruit                               2       0.0    778.8   73.88
  promote_only     promote                               2       0.0    859.0   75.90
  provision_only   provision                             2       0.0    709.5   58.95
  triangle         promote,provision,recruit             2       0.0    932.5   43.24
  spread(total_killed) = 223.00  (709.5 - 932.5)
    [triangle wave 0 growth-A target=promote,provision,recruit chosen=promote,recruit          cost=27.0]
    [triangle wave 1 growth-B target=promote,provision,recruit chosen=provision,promote,recruit cost=43.0]
```

## The quality-over-quantity signal, preserved

`promote` remains the standout single lane (`promote_only` 859.0 total
killed vs. `hoard`'s 709.5 — the full task-097 spread, unchanged since
neither `promote`'s cost nor effect moved) and now compounds correctly
inside `triangle` instead of being greedily dropped from it. The
headcount-held-equal isolation task-097 ran (`promote_only` vs `hoard`
entering wave 1 at the identical 38.21, exiting at 8.48 vs 0.66 — ~12x) is
untouched by either change in this task and still holds, since neither
`promote`'s Embers cost nor `upgrades.json`'s tier effects were edited.

## Verification

`py Scripts/sim/run_sim.py --selftest`, `py Scripts/sim/decisions.py --selftest`,
`py Scripts/sim/validate.py`, and `py Scripts/sim/drift_check.py` were all
re-run after these edits (read-only use, nothing under `Scripts/sim/` or
`docs/data/scenarios/` touched). All selftests pass; `validate.py`'s existing
check-3 FAIL is the pre-existing, already-documented `LIMITATIONS.md` §1 gap,
unaffected by this task; `drift_check.py` reports CLEAN (no drift in any
baselined sweep — this task's data edits don't touch any sweep's own
constants).

## What this task did not do

Did not touch `Scripts/sim/`, `GDD.md`, `SYSTEMS.md`, `CLASSES.md`, any
`.uasset`, or any `docs/data/scenarios/*.json` file (including
`run-slice-three-wave.json` and the `gate1-calibration-wave*` fixtures — the
120-unit convention this task reconciles against is read, not edited). Did
not change `upgrades.json`'s tier ladder, `promote`'s or `recruit`'s Embers
cost, or `provision`'s +25 capacity effect size — only `provision`'s price.
Did not resolve `LIMITATIONS.md` §1 (the frontage-model gap) or the PRIMARY
THEATRE verdict it causes — that remains open, tracked there, and is not a
supply-economy problem this task's file ownership can fix.
