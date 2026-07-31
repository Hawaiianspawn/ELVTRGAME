"""
scenario_runner.py — loads one docs/data/scenarios/<name>.json and runs it
through the matching model in combat_model.py.

Usage:
    py Scripts/sim/scenario_runner.py <scenario-name>
    py Scripts/sim/scenario_runner.py --all
    py Scripts/sim/scenario_runner.py --list
    py Scripts/sim/scenario_runner.py <scenario-name> --trials 200 --seed 1234

No stat blocks are hardcoded here — every fighter comes from data_loader.py.

SEEDED VARIANCE (task-076, docs/sim/MODEL.md §4). `--trials`/`--seed` and the
`run_trials()` API turn one configuration into a distribution. Everything
about it is opt-in: with no `--seed` and no `--trials`, this file's output is
bit-identical to the pre-task-076 harness, because `run()` only builds an RNG
when handed a seed and `combat_model`'s variance helpers return exactly 1.0
without one. A spread out of this layer is NOT a confidence interval on the
real game and cannot be used to argue validation check 3 passes — read
docs/sim/LIMITATIONS.md §6 before quoting one.
"""

from __future__ import annotations

import argparse
import contextlib
import dataclasses
import hashlib
import json
import random
import statistics
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import combat_model as cm


# ---------------------------------------------------------------------------
# Variance layer plumbing (task-076)
# ---------------------------------------------------------------------------

# Which sources can physically do anything in which model. `arrival_jitter`
# is deliberately absent from point_target: that model is a closed-form
# snapshot with no time axis, so there is no arrival to perturb. Listing it
# as "enabled" on a point_target run would be a claim the model can't back.
VARIANCE_APPLICABILITY = {
    "wave_attrition": ("arrival_jitter", "damage_roll"),
    "point_target": ("damage_roll",),
}

# Sources whose MAGNITUDE traces to a shipped CVar or committed data file.
# Anything enabled and NOT in here makes a run diagnostic — see
# combat-model-constants.json's variance_model block for each source's
# citation (or its plain admission of not having one).
CITED_SOURCES = frozenset({"arrival_jitter"})

SUMMARY_FIELDS = {
    "wave_attrition": ("retinue_survivors", "enemy_survivors", "elapsed_seconds"),
    "point_target": ("ttk_seconds",),
}


def variance_config(kind: str) -> dict:
    """
    {source_name: magnitude} for every source that is BOTH enabled in
    combat-model-constants.json's variance_model block AND applicable to
    this model kind. Disabled sources are absent, not zero-valued, so
    combat_model.variance_roll() short-circuits on them.
    """
    block = dl.load_combat_model_constants().get("variance_model", {})
    enabled = {}
    for source in VARIANCE_APPLICABILITY.get(kind, ()):
        cfg = block.get(source)
        if not isinstance(cfg, dict) or not cfg.get("Enabled"):
            continue
        magnitude = float(cfg.get("Magnitude", 0.0))
        if magnitude > 0.0:
            enabled[source] = magnitude
    return enabled


def derive_seed(root_seed, scenario_name: str, overrides: dict | None, trial_index: int) -> int:
    """
    Every trial's RNG seed is DERIVED, never drawn off a shared stream
    advanced across cells or trials. A shared stream makes each result
    depend on how many results were computed before it, which breaks the
    moment the work is split across a process pool and makes every persisted
    artifact unreproducible. Hashing the coordinates instead means trial 7 of
    a run is the same trial 7 whether it was computed first, last, alone, or
    in one of four worker processes.

        seed = int(sha256(canonical_json([root_seed, scenario_name,
                                          sorted(overrides.items()),
                                          trial_index]))[:8 bytes], big-endian)

    Documented identically in docs/sim/MODEL.md §4 so it is reproducible from
    the doc alone. `sort_keys` + compact separators + sorted overrides make
    the JSON canonical; `default=str` is a safety net for an override value
    json can't natively encode, so a seed is always derivable rather than
    raising deep inside a pool worker.
    """
    canonical = json.dumps(
        [root_seed, scenario_name, sorted((overrides or {}).items()), trial_index],
        sort_keys=True, separators=(",", ":"), default=str,
    )
    return int.from_bytes(hashlib.sha256(canonical.encode("utf-8")).digest()[:8], "big")


@contextlib.contextmanager
def _overrides_applied(scenario_name: str, overrides: dict | None):
    """
    Apply `{"<file>:<path>": value}` overrides for the duration of one run,
    reusing sweep.py's existing in-memory `_load_json` patch and its
    `<file>:<path>` language rather than reimplementing either (see that
    module's docstring). Nothing is ever written to disk. Imported lazily
    because sweep.py imports THIS module at its own import time.
    """
    if not overrides:
        yield
        return
    import sweep
    cell = []
    for key, value in sorted(overrides.items()):
        axis = sweep.parse_axis(f"{key}={value}")
        cell.append((axis, axis.values[0]))
    original = dl._load_json
    dl._load_json = sweep._make_patched_load_json(f"{scenario_name}.json", cell)
    try:
        yield
    finally:
        dl._load_json = original


def to_json_safe(obj):
    """
    Recursively converts a result dict/list into something json.dumps can
    handle. The only non-JSON-native thing scenario results carry is
    wave-attrition's `log`: a list of `combat_model.WaveTickRecord`
    dataclass instances (accessed elsewhere as row.t / row.retinue_alive,
    not as dict keys) — task-069's note that this needs REAL serialization,
    not `json.dumps(result)` blowing up on a dataclass with a TypeError.
    `dataclasses.asdict` does the actual conversion; this just walks
    containers to find where dataclasses are hiding.
    """
    if dataclasses.is_dataclass(obj) and not isinstance(obj, type):
        return to_json_safe(dataclasses.asdict(obj))
    if isinstance(obj, dict):
        return {k: to_json_safe(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [to_json_safe(v) for v in obj]
    return obj


def _retinue_groups(composition: list[dict]) -> list:
    return [
        (row, dl.retinue_fighter(row["UnitType"], row["Tier"]))
        for row in composition
    ]


def run_wave_attrition(scenario: dict, rng=None, variance: dict | None = None) -> dict:
    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    chip_floor = dl.armor_chip_floor()

    retinue_groups = [
        cm.WaveGroup(
            name=f"{row['UnitType']}_{row['Tier']}", fighter=fighter, count=float(row["Count"]),
            arrival_seconds=float(row.get("ArrivalSeconds", 0.0)),
        )
        for row, fighter in _retinue_groups(scenario["Retinue"]["Composition"])
    ]
    if scenario.get("HeroPresent"):
        retinue_groups.append(cm.WaveGroup(name="hero", fighter=dl.hero_fighter(), count=1.0))

    enemy_groups = [
        cm.WaveGroup(
            name=row.get("Name", row["EntityTier"]), fighter=dl.enemy_fighter(row["EntityTier"]),
            count=float(row["Count"]), arrival_seconds=float(row.get("ArrivalSeconds", 0.0)),
        )
        for row in scenario["Enemy"]["Composition"]
    ]

    result = cm.simulate_wave_attrition(
        retinue_groups=retinue_groups,
        enemy_groups=enemy_groups,
        chip_floor=chip_floor,
        max_attackers_per_unit=float(consts["MaxAttackersPerUnit"]),
        formation_spacing=float(consts["FormationSpacingUU"]),
        engaged_spacing=float(consts["EngagedSpacingUU"]),
        melee_contact_facing_fraction=float(consts["MeleeContactFacingFraction"]),
        dt=dl.swing_interval_shared(),
        max_time=float(scenario.get("TimeLimitSeconds", 300)),
        rng=rng,
        variance=variance,
    )
    result["scenario"] = scenario["Name"]
    result["kind"] = "wave_attrition"
    return result


def run_point_target(scenario: dict, rng=None, variance: dict | None = None) -> dict:
    chip_floor = dl.armor_chip_floor()
    target = dl.enemy_fighter(scenario["Target"]["EntityTier"])

    groups = [
        cm.ArmyGroup(name=f"{row['UnitType']}_{row['Tier']}", fighter=fighter, count=int(row["Count"]))
        for row, fighter in _retinue_groups(scenario["Retinue"]["Composition"])
    ]
    hero = dl.hero_fighter() if scenario.get("HeroPresent") else None

    ttk, breakdown = cm.army_ttk_vs_point_target(
        target, groups, chip_floor, hero=hero, rng=rng, variance=variance)
    return {
        "scenario": scenario["Name"],
        "kind": "point_target",
        "target": target["display_name"],
        "target_max_hp": target["max_hp"],
        "surround_cap_estimate": target.get("surround_cap_estimate"),
        "ttk_seconds": round(ttk, 2),
        "breakdown": breakdown,
    }


def run(name: str, seed: int | None = None) -> dict:
    """
    One point estimate. `seed=None` (the default, and every pre-task-076
    call site) means NO rng is constructed and NO variance config is read —
    output is bit-identical to before the variance layer existed. Pass a
    seed and the enabled+applicable variance sources fire, and the result
    carries `seed` / `variance_sources` keys recording which.
    """
    scenario = dl.load_scenario(name)
    kind = scenario["Kind"]
    rng = random.Random(seed) if seed is not None else None
    variance = variance_config(kind) if rng is not None else None

    if kind == "wave_attrition":
        result = run_wave_attrition(scenario, rng=rng, variance=variance)
    elif kind == "point_target":
        result = run_point_target(scenario, rng=rng, variance=variance)
    else:
        raise ValueError(f"Unknown scenario Kind '{kind}' in {name}.json")

    if rng is not None:
        result["seed"] = seed
        result["variance_sources"] = sorted(variance)
    return result


def run_trial(name: str, trial_index: int, root_seed=None, overrides: dict | None = None) -> dict:
    """
    ONE trial of a trials run. Module-level (not a closure) on purpose: a
    ProcessPoolExecutor on Windows/spawn has to pickle whatever it calls, and
    this is the unit task-077's batch runner will hand to a pool.

    Fully determined by (root_seed, name, overrides, trial_index) — see
    derive_seed(). Computing trials in any order, in any number of
    processes, gives the same per-trial results; validate.py's
    ORDER-INDEPENDENCE check enforces exactly that, pool included.
    """
    seed = derive_seed(root_seed, name, overrides, trial_index)
    with _overrides_applied(name, overrides):
        result = run(name, seed=seed)
    result["trial_index"] = trial_index
    return result


def summarize_trials(results: list[dict], kind: str) -> dict:
    """
    Distribution summary over the numeric result fields the point-estimate
    dicts already carry (SUMMARY_FIELDS) — nothing derived, nothing new
    invented to summarize.

    PERCENTILE METHOD, stated because p5/p95 on small n is method-sensitive
    and a reader comparing two runs needs to know which one produced the
    number: `statistics.quantiles(data, n=20, method="inclusive")`, taking
    cut point 0 as p5 and cut point 18 as p95. "inclusive" treats the sample
    as the whole population and linearly interpolates between order
    statistics (the same convention as numpy's default `linear`), so p5/p95
    never fall outside the observed min/max. With n < 2 there is nothing to
    interpolate: p5/p95 collapse to the single value and stdev is 0.0.
    """
    summary = {}
    for field in SUMMARY_FIELDS.get(kind, ()):
        values = sorted(float(r[field]) for r in results if field in r)
        if not values:
            continue
        n = len(values)
        if n >= 2:
            cuts = statistics.quantiles(values, n=20, method="inclusive")
            p5, p95, sd = cuts[0], cuts[18], statistics.stdev(values)
        else:
            p5 = p95 = values[0]
            sd = 0.0
        summary[field] = {
            "n": n,
            "mean": round(statistics.fmean(values), 4),
            "median": round(statistics.median(values), 4),
            "p5": round(p5, 4),
            "p95": round(p95, 4),
            "min": round(values[0], 4),
            "max": round(values[-1], 4),
            "stdev": round(sd, 4),
        }
    return summary


def run_trials(name: str, trials: int, root_seed=None, overrides: dict | None = None) -> dict:
    """
    Run one configuration `trials` times under the seeded variance layer and
    return the per-trial results plus a distribution summary.

    Calling this IS the opt-in: unlike run(), every trial gets a derived seed
    even when `root_seed` is None (None is just another hash input, so the
    run stays reproducible — it is an unlabelled root seed, not an absent
    one). Which sources actually fire is still decided by
    combat-model-constants.json's variance_model block plus what the model
    kind can express.

    THE SPREAD THIS RETURNS IS NOT A CONFIDENCE INTERVAL on the real game. It
    reflects only the sources modelled here, at magnitudes that are cited or
    (for `damage_roll`) admittedly invented, on top of a wave-attrition model
    that does not reproduce its one measured baseline. docs/sim/LIMITATIONS.md
    §6 states plainly what it may and may not be used to argue.
    """
    # Under the same overrides the trials themselves run with — an override
    # can target the variance_model block, and reporting the UNoverridden
    # source list next to overridden results would be a quiet lie.
    with _overrides_applied(name, overrides):
        kind = dl.load_scenario(name)["Kind"]
        variance = variance_config(kind)
    results = [run_trial(name, i, root_seed, overrides) for i in range(trials)]
    return {
        "scenario": name,
        "kind": kind,
        "trials": trials,
        "root_seed": root_seed,
        "overrides": overrides or {},
        "variance_sources_enabled": sorted(variance),
        "diagnostic_invented_variance": any(s not in CITED_SOURCES for s in variance),
        "results": results,
        "summary": summarize_trials(results, kind),
    }


def print_result(scenario: dict, result: dict) -> None:
    print(f"\n=== {scenario.get('DisplayName', result['scenario'])} ({result['scenario']}) ===")
    if result["kind"] == "wave_attrition":
        print(f"  Retinue: {result['retinue_start']:.0f} start -> {result['retinue_survivors']:.1f} survivors")
        print(f"  Enemy:   {result['enemy_start']:.0f} start -> {result['enemy_survivors']:.1f} survivors")
        print(f"  Result: {result['result']}  (elapsed {result['elapsed_seconds']}s)")
        print(f"  {'t':>6} {'retinue':>9} {'enemy':>9} {'exposed':>9} {'dmg->ret':>10} {'dmg->enemy':>11}")
        for row in result["log"]:
            print(f"  {row.t:>6} {row.retinue_alive:>9} {row.enemy_alive:>9} {row.exposed_retinue:>9} "
                  f"{row.dmg_to_retinue:>10} {row.dmg_to_enemy:>11}")
    else:
        print(f"  Target: {result['target']} (MaxHP {result['target_max_hp']:.0f}, "
              f"SurroundCapEstimate {result['surround_cap_estimate']})")
        print(f"  TTK: {result['ttk_seconds']}s")
        for gname, g in result["breakdown"].items():
            print(f"    {gname:>16}: {g['role']:>7} count={g['count']:>4} engaged={g['engaged']:>5} "
                  f"per-unit dps={g['per_unit_dps']:>7} group dps={g['group_dps']:>8}")


DIAGNOSTIC_VARIANCE_BANNER = (
    "\n"
    "============= DIAGNOSTIC: INVENTED VARIANCE ENABLED =============\n"
    "At least one enabled variance source has NO citation — its magnitude\n"
    "is a made-up number, not a shipped CVar or a committed measurement.\n"
    "See combat-model-constants.json's variance_model block for which, and\n"
    "docs/sim/LIMITATIONS.md §6 for what a spread produced this way may and\n"
    "may not be used to argue. It is not a confidence interval, and it\n"
    "cannot make validation check 3 pass.\n"
    "=================================================================\n"
)


def print_trials(payload: dict) -> None:
    print(f"\n=== {payload['scenario']} — {payload['trials']} trials "
          f"(root_seed={payload['root_seed']}) ===")
    sources = payload["variance_sources_enabled"]
    print(f"  variance sources enabled: {', '.join(sources) if sources else '(none)'}")
    if payload["diagnostic_invented_variance"]:
        print(DIAGNOSTIC_VARIANCE_BANNER)
    if not sources:
        print("  NOTE: no applicable variance source is enabled — every trial is the same "
              "point estimate, and the spread below is identically zero by construction.")
    print(f"  {'field':>18} {'n':>4} {'mean':>10} {'median':>10} {'p5':>10} {'p95':>10} "
          f"{'min':>10} {'max':>10} {'stdev':>9}")
    for field, s in payload["summary"].items():
        print(f"  {field:>18} {s['n']:>4} {s['mean']:>10} {s['median']:>10} {s['p5']:>10} "
              f"{s['p95']:>10} {s['min']:>10} {s['max']:>10} {s['stdev']:>9}")
    if payload["kind"] == "wave_attrition":
        outcomes = {}
        for r in payload["results"]:
            outcomes[r["result"]] = outcomes.get(r["result"], 0) + 1
        print("  outcomes: " + ", ".join(f"{k}={v}" for k, v in sorted(outcomes.items())))


def main() -> None:
    parser = argparse.ArgumentParser(description="Run docs/data/scenarios/*.json wave/point-target scenarios.")
    parser.add_argument("name", nargs="?", help="scenario file name (without .json)")
    parser.add_argument("--all", action="store_true", help="run every scenario in docs/data/scenarios/")
    parser.add_argument("--list", action="store_true", help="list available scenario names")
    parser.add_argument("--json", action="store_true",
                         help="emit the full, unrounded-past-what-combat_model.py-already-rounds result "
                              "as JSON instead of the human table (single object, or a JSON array under --all)")
    parser.add_argument("--trials", type=int, default=1,
                         help="run the scenario N times under the seeded variance layer and print a "
                              "distribution summary instead of one point estimate (task-076). "
                              "Absent or 1 with no --seed = unchanged single-run behaviour.")
    parser.add_argument("--seed", type=int, default=None,
                         help="root seed for --trials. Per-trial seeds are DERIVED from it (see "
                              "derive_seed), never streamed, so trial order and process count don't "
                              "affect results. Omitted = no variance at all on a plain run.")
    args = parser.parse_args()

    if args.list:
        for n in dl.list_scenarios():
            print(n)
        return

    names = dl.list_scenarios() if args.all else ([args.name] if args.name else [])
    if not names:
        parser.print_help()
        return

    if args.trials > 1 or args.seed is not None:
        payloads = [run_trials(n, max(1, args.trials), root_seed=args.seed) for n in names]
        if args.json:
            print(json.dumps(to_json_safe(payloads if args.all else payloads[0]), indent=2))
        else:
            for payload in payloads:
                print_trials(payload)
        return

    if args.json:
        results = [to_json_safe(run(n)) for n in names]
        print(json.dumps(results if args.all else results[0], indent=2))
        return

    for n in names:
        scenario = dl.load_scenario(n)
        result = run(n)
        print_result(scenario, result)


if __name__ == "__main__":
    main()
