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

  5. IDENTITY — with variance off, four committed scenarios still produce
     the exact numbers they produced before the task-076 variance layer
     existed. The regression wall around the validated point-target model
     and around docs/sim/baseline.json's assumption that this harness has no
     randomness on its default path.
  6. REPRODUCIBILITY — run_trials(..., root_seed=1234) twice gives identical
     per-trial results.
  7. ORDER-INDEPENDENCE — the same trials computed shuffled, and computed in
     a 4-worker ProcessPoolExecutor, give the same per-trial results as the
     serial in-order computation. This is what makes derived seeding worth
     having over a shared RNG stream, so it is checked, not asserted.

Checks 5, 6 and 7 all GATE the exit code.

Plus one bonus, non-gating consistency check: the point-target army-TTK model
reproduces entity-tiers.md §7's own N=120-vs-Elite table row (1.85s), because
that's free to check and strengthens confidence in combat_model.py's
point-target path before it's used on the floor3 Boss scenario.

Run: py Scripts/sim/validate.py
Exit code 0 iff checks 1, 2, 4, 5, 6 and 7 pass. Check 3 is reported but does NOT gate
the exit code — a documented, honestly-reported failure on check 3 is a
valid, useful outcome per task-063's own instructions, not a build failure.
Check 4 DOES gate the exit code: unlike check 3 (an open design question),
a model where TargetsPerHit provably does nothing is not a "the data says
no" result, it's a bug, and should block trusting anything downstream same
as checks 1-2.
"""

from __future__ import annotations

import sys
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


# ---------------------------------------------------------------------------
# task-076 variance-layer checks (5, 6, 7). All three gate the exit code.
# ---------------------------------------------------------------------------

# Captured 2026-07-31 by running each scenario with --json BEFORE the variance
# layer was written, and pasted here as literals rather than recomputed. That
# is the whole point: if a future change to the variance layer leaks into the
# default (unseeded) path, these numbers move and this check fails, without
# anyone having to remember to re-capture a baseline first. Deliberately
# covers both models and both trust levels — the two point_target rows are the
# only VALIDATED model in this harness (docs/sim/LIMITATIONS.md §3), so they
# are the ones a silent regression would do the most damage to.
IDENTITY_EXPECTED = {
    "gate1-calibration-wave1": dict(retinue_survivors=0.0, enemy_survivors=19.24,
                                     elapsed_seconds=11.7, result="retinue_wiped"),
    "floor1-swarm-wave": dict(retinue_survivors=0.0, enemy_survivors=142.59,
                               elapsed_seconds=1.8, result="retinue_wiped"),
    "floor2-ranged-wave": dict(retinue_survivors=0.0, enemy_survivors=353.8,
                                elapsed_seconds=0.9, result="retinue_wiped"),
    "floor2-elite-point-target": dict(ttk_seconds=2.13),
    "floor3-boss-point-target": dict(ttk_seconds=8.23),
}


def check_variance_off_identity() -> tuple[bool, str]:
    """
    CHECK 5. With no seed, every committed scenario reproduces its
    pre-variance-layer numbers exactly. docs/sim/DRIFT-CHECK.md's baseline
    rests on "combat_model.py has no randomness anywhere"; that claim is now
    conditional ("...unless a seed is passed"), and this is what keeps the
    condition true.
    """
    import scenario_runner as sr
    bad = []
    for name, expected in sorted(IDENTITY_EXPECTED.items()):
        result = sr.run(name)
        for field, want in expected.items():
            got = result[field]
            if isinstance(want, str):
                if got != want:
                    bad.append(f"{name}.{field}: {got!r} != {want!r}")
            elif abs(float(got) - float(want)) > 1e-9:
                bad.append(f"{name}.{field}: {got} != {want}")
        if "seed" in result or "variance_sources" in result:
            bad.append(f"{name}: unseeded run leaked variance keys into its result dict")
    ok = not bad
    return ok, (
        f"Variance-OFF identity ({len(IDENTITY_EXPECTED)} scenarios, "
        f"{sum(len(v) for v in IDENTITY_EXPECTED.values())} fields): "
        + ("PASS" if ok else "FAIL — " + "; ".join(bad))
    )


def check_trials_reproducible() -> tuple[bool, str]:
    """CHECK 6. Same call, twice, identical per-trial results."""
    import scenario_runner as sr
    a = sr.run_trials("gate1-calibration-wave1", 8, root_seed=1234)
    b = sr.run_trials("gate1-calibration-wave1", 8, root_seed=1234)
    ok = _trial_fingerprints(a["results"]) == _trial_fingerprints(b["results"])
    return ok, (
        f"Trials reproducibility (run_trials x2, 8 trials, root_seed=1234, "
        f"sources={a['variance_sources_enabled']}): -> {'PASS' if ok else 'FAIL'}"
    )


def _trial_fingerprints(results: list[dict]) -> list[tuple]:
    """The comparable part of a trial result. `log` holds dataclasses and is
    derived from these anyway; seed is included because two runs agreeing on
    outcome while disagreeing on seed would mean the derivation drifted."""
    return [
        (r["trial_index"], r["seed"],
         r.get("retinue_survivors"), r.get("enemy_survivors"),
         r.get("elapsed_seconds"), r.get("result"), r.get("ttk_seconds"))
        for r in sorted(results, key=lambda r: r["trial_index"])
    ]


def check_trials_order_independent() -> tuple[bool, str]:
    """
    CHECK 7. Trials 0..7 computed (a) serially in order, (b) serially in a
    shuffled order, (c) across a 4-worker ProcessPoolExecutor, must all
    agree. A shared/streamed RNG would fail (b) and (c) instantly; derived
    seeding is the reason they pass, and this is the check that makes that
    claim testable instead of just documented.
    """
    import random as _random
    from concurrent.futures import ProcessPoolExecutor
    import scenario_runner as sr

    name, n, root = "gate1-calibration-wave1", 8, 1234
    serial = [sr.run_trial(name, i, root) for i in range(n)]

    order = list(range(n))
    _random.Random(99).shuffle(order)
    shuffled = [sr.run_trial(name, i, root) for i in order]

    with ProcessPoolExecutor(max_workers=4) as pool:
        pooled = list(pool.map(_pool_trial, [(name, i, root) for i in range(n)]))

    base = _trial_fingerprints(serial)
    ok_shuffled = _trial_fingerprints(shuffled) == base
    ok_pooled = _trial_fingerprints(pooled) == base
    ok = ok_shuffled and ok_pooled
    return ok, (
        f"Trials order-independence (8 trials: in-order vs shuffled {order} vs "
        f"ProcessPoolExecutor x4): shuffled={'MATCH' if ok_shuffled else 'MISMATCH'}, "
        f"pooled={'MATCH' if ok_pooled else 'MISMATCH'} -> {'PASS' if ok else 'FAIL'}"
    )


def _pool_trial(args):
    """Module-level so ProcessPoolExecutor can pickle it under Windows/spawn."""
    import scenario_runner as sr
    name, index, root = args
    return sr.run_trial(name, index, root)


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

    ok5, msg5 = check_variance_off_identity()
    print(msg5)

    ok6, msg6 = check_trials_reproducible()
    print(msg6)

    ok7, msg7 = check_trials_order_independent()
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
              "survivor-count prediction, until this closes.")

    if ok5 and ok6 and ok7:
        print("Variance-layer checks (5, 6, 7): PASS — variance is off by default and bit-identical, "
              "and seeded trials are reproducible and order/process independent.")
    else:
        print("Variance-layer checks (5, 6, 7): FAIL — see the lines above. A failing check 5 means the "
              "variance layer has leaked into the DEFAULT numeric path and docs/sim/baseline.json can no "
              "longer be trusted; a failing 6 or 7 means persisted trial artifacts are not reproducible.")

    return 0 if (ok1 and ok2 and ok4 and ok5 and ok6 and ok7) else 1


if __name__ == "__main__":
    sys.exit(main())
