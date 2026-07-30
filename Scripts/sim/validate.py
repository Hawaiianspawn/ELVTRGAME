"""
validate.py — the gate. Nothing downstream should be trusted until this
passes (or, for the GATE1 check, until its failure has been read and
reported honestly — see docs/sim/VALIDATION.md).

Required checks (task-063's "Done when", plus one regression guard added
after review):
  1. Militia vs Fodder TTK == 2.00s          (entity-tiers.md §7)
  2. Hero(55dps) vs Elite TTK == 21.60s      (entity-tiers.md §7)
  3. GATE1's measured wave-1 survival: 109-111 of 120 (GATE1-FUN-PROTOTYPE.md §3b)
  4. Cleave-sensitivity regression guard — TargetsPerHit must demonstrably
     keep mattering ABOVE MaxAttackersPerUnit, not just somewhere in its
     range. ADDED after a review caught the wave-attrition model reusing the
     incoming-attacker bound (MaxAttackersPerUnit) to also cap outgoing
     cleave capacity, which made TargetsPerHit's effect on the outcome
     cancel to a constant (max_attackers_per_unit / targets_per_hit)
     whenever TargetsPerHit >= MaxAttackersPerUnit — a structural bug, not a
     balance finding. Fixed by deriving cleave capacity from its own
     geometry (combat_model.melee_reach_per_exposed_unit).
     REVISED AGAIN after review caught that the FIRST version of this guard
     (comparing TargetsPerHit=1 vs 8) also didn't catch the bug — 1 is below
     MaxAttackersPerUnit=4, so that comparison straddled the saturation
     point and passed under both the bugged and fixed model. The real guard
     compares K=MaxAttackersPerUnit against K=2xMaxAttackersPerUnit, both
     derived from the constants file. See check_cleave_sensitivity()'s
     docstring and docs/sim/VALIDATION.md for the full account of both
     rounds.

Plus one bonus, non-gating consistency check: the point-target army-TTK model
reproduces entity-tiers.md §7's own N=120-vs-Elite table row (1.85s), because
that's free to check and strengthens confidence in combat_model.py's
point-target path before it's used on the floor3 Boss scenario.

Run: py Scripts/sim/validate.py
Exit code 0 iff checks 1, 2, and 4 pass. Check 3 is reported but does NOT gate
the exit code — a documented, honestly-reported failure on check 3 is a
valid, useful outcome per task-063's own instructions, not a build failure.
Check 4 DOES gate the exit code: unlike check 3 (an open design question),
a model where TargetsPerHit provably does nothing is not a "the data says
no" result, it's a bug, and should block trusting anything downstream same
as checks 1-2.

task-076 adds three more GATING checks (5, 6, 7) protecting the new variance
layer (Scripts/sim/combat_model.py's jitter_*, scenario_runner.py's
compute_trial/run_trials):
  5. IDENTITY — variance OFF reproduces a set of literal expected numbers
     captured from the committed scenarios before the variance layer existed
     (docs/sim/VALIDATION.md's task-076 section has the exact before/after
     transcript this protects).
  6. REPRODUCIBILITY — run_trials(name, 8, root_seed=1234) called twice
     returns identical per-trial results.
  7. ORDER-INDEPENDENCE — the same 8 trials computed serially, in shuffled
     order, and via a 4-worker ProcessPoolExecutor all agree. Proves
     seed_for()'s derived (not streamed) seeding actually delivers the
     order-independence task-076 section 2 requires.
Exit code is now 0 iff checks 1, 2, 4, 5, 6, and 7 all pass. Check 3 remains
the sole non-gating check, unchanged.
"""

from __future__ import annotations

import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import combat_model as cm


def check_militia_vs_fodder() -> tuple[bool, str]:
    militia = dl.retinue_fighter("spearmen", "militia")
    fodder = dl.enemy_fighter("brood_fodder")
    ttk = cm.ttk_1v1(militia, fodder, dl.armor_chip_floor())
    expected = 2.00
    ok = abs(ttk - expected) < 0.005
    return ok, f"Militia vs Fodder TTK = {ttk:.4f}s (expected {expected}s) -> {'PASS' if ok else 'FAIL'}"


def check_hero_vs_elite() -> tuple[bool, str]:
    hero = dl.hero_fighter()
    elite = dl.enemy_fighter("brood_elite")
    ttk = cm.ttk_1v1(hero, elite, dl.armor_chip_floor())
    expected = 21.60
    ok = abs(ttk - expected) < 0.01
    return ok, f"Hero(55dps) vs Elite TTK = {ttk:.4f}s (expected {expected}s) -> {'PASS' if ok else 'FAIL'}"


def check_gate1_calibration() -> tuple[bool, str, dict]:
    import scenario_runner as sr
    result = sr.run("gate1-calibration-wave1")
    survivors = result["retinue_survivors"]
    lo, hi = 109, 111
    ok = lo <= survivors <= hi
    msg = (
        f"GATE1 calibration: retinue survivors = {survivors:.2f} of "
        f"{result['retinue_start']:.0f} (measured range {lo}-{hi}) -> "
        f"{'PASS' if ok else 'FAIL (see docs/sim/VALIDATION.md for the read)'}"
    )
    return ok, msg, result


def _run_gate1_with_targets_per_hit(k: float) -> dict:
    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    chip_floor = dl.armor_chip_floor()
    dt = dl.swing_interval_shared()
    fodder = dl.enemy_fighter("brood_fodder")
    militia = dict(dl.retinue_fighter("spearmen", "militia"))
    militia["targets_per_hit"] = k
    retinue = [cm.WaveGroup("spearmen_militia", militia, 120.0)]
    enemy = [cm.WaveGroup("brood_fodder", fodder, 250.0)]
    return cm.simulate_wave_attrition(
        retinue, enemy, chip_floor,
        float(consts["MaxAttackersPerUnit"]), float(consts["FormationSpacingUU"]),
        float(consts["EngagedSpacingUU"]), float(consts["MeleeContactFacingFraction"]),
        dt, 300.0,
    )


def check_cleave_sensitivity() -> tuple[bool, str]:
    """
    THE REAL REGRESSION GUARD for the coupling bug a review of this harness
    caught (see docs/sim/VALIDATION.md's account, including a second round —
    the first version of THIS check also failed to catch the bug it was
    written for, and that failure is recorded there too, not quietly fixed).

    The bug made contact_scale == MaxAttackersPerUnit / TargetsPerHit,
    constant, WHENEVER TargetsPerHit >= MaxAttackersPerUnit (the frontage-cap
    regime). Below that ratio (TargetsPerHit < MaxAttackersPerUnit) the
    bugged model still responded to TargetsPerHit normally, because the
    min(1, ...) clamp hadn't saturated yet — so a check that only compares a
    low K against a high K straddles the saturation point and passes on
    BOTH the bugged and the fixed model, catching nothing. (The original
    version of this function compared K=1 vs K=8 against a shipped
    MaxAttackersPerUnit=4 and did exactly that.)

    The actual discriminator is entirely ABOVE the saturation point: compare
    K = MaxAttackersPerUnit against K = 2 x MaxAttackersPerUnit. A bugged
    model gives IDENTICAL output for any K >= MaxAttackersPerUnit (it can
    only ever express damage up to the M/K==1 ceiling). A correctly
    decoupled model keeps improving past that point, bounded only by local
    weapon-reach geometry and the living enemy population, and only truly
    flattens out much higher. Both K values are derived from the constants
    file, not hardcoded, so this keeps working if MaxAttackersPerUnit is
    ever retuned.
    """
    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    m = float(consts["MaxAttackersPerUnit"])
    k_at_cap = m
    k_above_cap = 2.0 * m

    at_cap = _run_gate1_with_targets_per_hit(k_at_cap)
    above_cap = _run_gate1_with_targets_per_hit(k_above_cap)
    delta = abs(above_cap["enemy_survivors"] - at_cap["enemy_survivors"])
    threshold = 2.0
    ok = delta > threshold
    msg = (
        f"Cleave sensitivity guard (K=MaxAttackersPerUnit={k_at_cap:.0f} vs K=2xMaxAttackersPerUnit={k_above_cap:.0f}): "
        f"{at_cap['enemy_survivors']:.1f} vs {above_cap['enemy_survivors']:.1f} enemy survivors, "
        f"delta={delta:.1f} (threshold {threshold}) -> "
        f"{'PASS' if ok else 'FAIL — cleave above MaxAttackersPerUnit has no marginal effect, the exact bug this guard exists to catch'}"
    )
    return ok, msg


def smoke_check_cleave_wired() -> tuple[bool, str]:
    """
    NOT the regression guard — a much weaker smoke test that cleave is wired
    into the model AT ALL (K=1 vs K=8, shipped RetinueTargetsPerHit). Kept
    because it's a fine sanity check, but it does NOT catch the coupling bug
    check_cleave_sensitivity() targets: K=1 sits below MaxAttackersPerUnit=4,
    so this comparison straddles the saturation point and passes under both
    the bugged and the fixed model. See check_cleave_sensitivity()'s
    docstring and docs/sim/VALIDATION.md for the full account. Non-gating.
    """
    low = _run_gate1_with_targets_per_hit(1)
    high = _run_gate1_with_targets_per_hit(8)
    delta = abs(high["enemy_survivors"] - low["enemy_survivors"])
    ok = delta > 5.0
    return ok, (
        f"[smoke, not the regression guard] TargetsPerHit=1 -> {low['enemy_survivors']:.1f}, "
        f"TargetsPerHit=8 -> {high['enemy_survivors']:.1f} enemy survivors, delta={delta:.1f} -> "
        f"{'PASS' if ok else 'FAIL'}"
    )


# ---------------------------------------------------------------------------
# task-076 — variance layer regression checks (5, 6, 7). All three GATE the
# exit code, same as check 4 — a variance-layer regression is a bug, not an
# open design question like check 3.
# ---------------------------------------------------------------------------

# Literal expected numbers captured from `py Scripts/sim/scenario_runner.py
# <name> --json` run BEFORE the variance layer existed (task-076's required
# "before" baseline — full transcript in docs/sim/VALIDATION.md's task-076
# section). `retinue-subtypes.json` is excluded: data_loader.list_scenarios()
# picks it up but it has no 'Kind' field and was never a runnable scenario —
# a pre-existing, unrelated condition (confirmed: `py Scripts/sim/
# scenario_runner.py --all --json` errors on it identically before and after
# this task's changes, same KeyError, same cause).
_IDENTITY_EXPECTED = {
    "floor1-swarm-wave": {"retinue_survivors": 0.0, "enemy_survivors": 142.59, "elapsed_seconds": 1.8},
    "floor2-ranged-wave": {"retinue_survivors": 0.0, "enemy_survivors": 353.8, "elapsed_seconds": 0.9},
    "gate1-calibration-wave1": {"retinue_survivors": 0.0, "enemy_survivors": 19.24, "elapsed_seconds": 11.7},
    "gate1-calibration-wave2": {"retinue_survivors": 0.0, "enemy_survivors": 124.96, "elapsed_seconds": 3.6},
    "gate1-calibration-wave3": {"retinue_survivors": 0.0, "enemy_survivors": 307.06, "elapsed_seconds": 3.6},
    "floor2-elite-point-target": {"ttk_seconds": 2.13},
    "floor2-elite-point-target-recruitmax": {"ttk_seconds": 2.09},
    "floor3-boss-point-target": {"ttk_seconds": 8.23},
    "floor3-boss-point-target-recruitmax": {"ttk_seconds": 8.01},
    "floor3-elite-point-target": {"ttk_seconds": 2.09},
}


def check_variance_identity() -> tuple[bool, str]:
    """
    Check 5. With variance OFF (`sr.run(name)`, no seed — the exact call
    every pre-task-076 caller already made), every committed scenario above
    must still reproduce the literal numbers captured before the variance
    layer existed. This is the regression wall protecting every already-
    validated/trusted scenario (including the point-target model check 3
    can't touch) from the variance layer's own existence — task-076's
    section 1 safety property, made a permanent gating check rather than a
    one-time before/after diff.
    """
    import scenario_runner as sr
    mismatches = []
    for name, expected in _IDENTITY_EXPECTED.items():
        result = sr.run(name)
        for field_name, expected_value in expected.items():
            actual = result[field_name]
            if abs(actual - expected_value) > 0.005:
                mismatches.append(f"{name}.{field_name}: expected {expected_value}, got {actual}")
    ok = not mismatches
    msg = (
        f"Variance-off identity ({len(_IDENTITY_EXPECTED)} scenarios, {sum(len(v) for v in _IDENTITY_EXPECTED.values())} fields) -> "
        f"{'PASS' if ok else 'FAIL — ' + '; '.join(mismatches)}"
    )
    return ok, msg


def check_variance_reproducibility() -> tuple[bool, str]:
    """
    Check 6. `run_trials(name, 8, root_seed=1234)` called twice must return
    identical per-trial results — proves the seed derivation alone (not
    incidental process/module state, not the `random` module's global
    stream) determines every trial's outcome.
    """
    import scenario_runner as sr
    first = sr.run_trials("gate1-calibration-wave1", 8, root_seed=1234)
    second = sr.run_trials("gate1-calibration-wave1", 8, root_seed=1234)
    ok = first["results"] == second["results"]
    return ok, f"Variance reproducibility (same root_seed, two independent run_trials calls, 8 trials each) -> {'PASS' if ok else 'FAIL'}"


def _order_independence_trial_sets() -> tuple[list, list, list]:
    """
    Computes the same 8 trials three ways and returns all three lists for
    comparison: serial (in-order), reversed-order (a different, still
    deterministic execution order), and via a 4-worker ProcessPoolExecutor.
    `sr.compute_trial` is the target — a module-level, picklable function
    that re-reads its own scenario/constants data per call, exactly so it's
    safe to hand to a worker process with no shared state (see its
    docstring). This machine is Windows/spawn, so the pool is only ever
    constructed from inside check_order_independence(), which is only ever
    called from main(), which only runs under this file's own
    `if __name__ == "__main__":` guard at the bottom — the same guard
    task-076's brief asked for, already satisfied by this script's existing
    structure rather than a second, redundant one.
    """
    import scenario_runner as sr
    name, root_seed, n = "gate1-calibration-wave1", 4321, 8

    serial = [sr.compute_trial(name, i, root_seed) for i in range(n)]

    reversed_order = list(range(n))[::-1]
    reversed_map = {i: sr.compute_trial(name, i, root_seed) for i in reversed_order}
    reversed_list = [reversed_map[i] for i in range(n)]

    with ProcessPoolExecutor(max_workers=4) as ex:
        futures = {i: ex.submit(sr.compute_trial, name, i, root_seed, None) for i in range(n)}
        pooled_list = [futures[i].result() for i in range(n)]

    return serial, reversed_list, pooled_list


def check_order_independence() -> tuple[bool, str]:
    """Check 7. The same 8 trials, computed serially, in reversed order, and
    via a 4-worker process pool, must produce identical per-trial results —
    proves seed_for()'s derived (not streamed) seeding actually delivers
    execution-order independence, both within one process and across a
    process pool (the shape task-075's planned batch runner uses)."""
    serial, reversed_list, pooled_list = _order_independence_trial_sets()
    ok = (reversed_list == serial) and (pooled_list == serial)
    return ok, (
        f"Variance order-independence (serial vs reversed vs 4-worker process pool, 8 trials): "
        f"reversed match={reversed_list == serial}, pool match={pooled_list == serial} -> {'PASS' if ok else 'FAIL'}"
    )


def bonus_check_elite_n120() -> tuple[bool, str]:
    """entity-tiers.md §7's own table: Elite (cap 20), N=120, 80/20 split -> TTK 1.85s."""
    elite = dl.enemy_fighter("brood_elite")
    spearmen = dl.retinue_fighter("spearmen", "militia")
    archers = dl.retinue_fighter("archers", "militia")
    groups = [
        cm.ArmyGroup("spearmen_militia", spearmen, 96),
        cm.ArmyGroup("archers_militia", archers, 24),
    ]
    hero = dl.hero_fighter()
    ttk, _ = cm.army_ttk_vs_point_target(elite, groups, dl.armor_chip_floor(), hero=hero)
    expected = 1.85
    ok = abs(ttk - expected) < 0.02
    return ok, f"[bonus] Army(N=120, 80/20) vs Elite TTK = {ttk:.3f}s (entity-tiers.md §7 table: {expected}s) -> {'PASS' if ok else 'FAIL'}"


def main() -> int:
    print("=== task-063 validation suite ===\n")

    ok1, msg1 = check_militia_vs_fodder()
    print(msg1)

    ok2, msg2 = check_hero_vs_elite()
    print(msg2)

    ok3, msg3, gate1_result = check_gate1_calibration()
    print(msg3)

    ok4, msg4 = check_cleave_sensitivity()
    print(msg4)

    ok_smoke, msg_smoke = smoke_check_cleave_wired()
    print(msg_smoke)

    ok_bonus, msg_bonus = bonus_check_elite_n120()
    print(msg_bonus)

    ok5, msg5 = check_variance_identity()
    print(msg5)

    ok6, msg6 = check_variance_reproducibility()
    print(msg6)

    ok7, msg7 = check_order_independence()
    print(msg7)

    print()
    if ok1 and ok2:
        print("REQUIRED closed-form checks (1, 2): PASS")
    else:
        print("REQUIRED closed-form checks (1, 2): FAIL — do not trust anything downstream.")

    if ok4:
        print("Cleave-sensitivity regression guard (4): PASS — TargetsPerHit still has real effect.")
    else:
        print("Cleave-sensitivity regression guard (4): FAIL — the wave-attrition model has a bug that "
              "cancels out the retinue's cleave advantage. Do NOT trust any wave-attrition scenario output "
              "until this is fixed.")

    if ok3:
        print("GATE1 wave-attrition reproduction (3): PASS — wave-attrition scenarios are trustworthy "
              "for relative comparisons at this scale.")
    else:
        print("GATE1 wave-attrition reproduction (3): FAIL — per task-063's own instructions, this is "
              "reported honestly, not fudged. See docs/sim/VALIDATION.md and docs/sim/LIMITATIONS.md "
              "for the numbers and the read on why. Wave-attrition scenario OUTPUT (floor1/floor2) should "
              "be read as illustrative of the MECHANISM (frontage concurrency limits), not as a trusted "
              "survivor-count prediction, until this closes. A variance layer (task-076) does not change "
              "this verdict — see docs/sim/LIMITATIONS.md's variance-layer section: a spread can never be "
              "used to declare this check passing 'within variance.'")

    if ok5 and ok6 and ok7:
        print("Variance-layer regression checks (5, 6, 7): PASS — variance-off output is identity-locked, "
              "and seeded trials are reproducible and order-independent.")
    else:
        print("Variance-layer regression checks (5, 6, 7): FAIL — do not trust any --trials/--seed output "
              "until this is fixed (see individual check lines above for which one failed).")

    return 0 if (ok1 and ok2 and ok4 and ok5 and ok6 and ok7) else 1


if __name__ == "__main__":
    sys.exit(main())
