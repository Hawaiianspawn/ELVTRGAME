"""
scenario_runner.py — loads one docs/data/scenarios/<name>.json and runs it
through the matching model in combat_model.py.

Usage:
    py Scripts/sim/scenario_runner.py <scenario-name>
    py Scripts/sim/scenario_runner.py --all
    py Scripts/sim/scenario_runner.py --list
    py Scripts/sim/scenario_runner.py <scenario-name> --trials 200 --seed 1

No stat blocks are hardcoded here — every fighter comes from data_loader.py.

VARIANCE LAYER (task-076). `run(name, seed=None)` and the CLI's default
(no --trials/--seed) are bit-identical to this module's pre-task-076
behavior — variance is only ever applied when a caller explicitly supplies a
seed, AND only for whichever sources are individually enabled in
docs/data/scenarios/combat-model-constants.json's `variance_model` block
(both default 'enabled': false, see that file's own per-source notes). See
docs/sim/MODEL.md's variance-layer section for the full design (seed
derivation, percentile method, what each source is and isn't) and
docs/sim/LIMITATIONS.md for what a distribution produced here may NOT be
used to argue.
"""

from __future__ import annotations

import argparse
import copy
import dataclasses
import hashlib
import json
import random
import re
import statistics
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import combat_model as cm


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


# ---------------------------------------------------------------------------
# Scenario-only overrides for run_trials()/compute_trial() (task-076). Same
# tiny dot/[field=value] path language sweep.py's `scenario:` axis family
# uses, duplicated in miniature here rather than imported — this task owns
# scenario_runner.py, not sweep.py, and the two are meant to stay
# independent (sweep.py is explicitly off-limits to this task). Unlike
# sweep.py's CLI parsing, values here are NOT string-coerced: a caller passes
# real Python values already (this is a programmatic dict, not a CLI flag).
# ---------------------------------------------------------------------------

_OVERRIDE_SEG_RE = re.compile(r'^([A-Za-z0-9_]+)(\[([A-Za-z0-9_]+)=([^\]]+)\])?$')


def _walk_override_segment(node, seg: str):
    m = _OVERRIDE_SEG_RE.match(seg)
    if not m:
        raise ValueError(f"Bad override path segment {seg!r} — expected 'key' or 'key[field=value]'")
    key, _, filter_key, filter_raw = m.groups()
    node = node[key]
    if filter_key is not None:
        for item in node:
            if str(item.get(filter_key)) == filter_raw:
                return item
        raise KeyError(f"No element with {filter_key}={filter_raw!r} under {key!r}")
    return node


def _apply_scenario_overrides(scenario: dict, overrides: dict | None) -> dict:
    """Deep-copies `scenario` and applies each {path: value} override to the
    copy — the original scenario dict (and whatever data_loader cache/read it
    came from) is never mutated. No-op (returns the same object) if
    `overrides` is falsy, so the common no-overrides case does no copying."""
    if not overrides:
        return scenario
    patched = copy.deepcopy(scenario)
    for path, value in overrides.items():
        segs = path.split(".")
        node = patched
        for seg in segs[:-1]:
            node = _walk_override_segment(node, seg)
        m = _OVERRIDE_SEG_RE.match(segs[-1])
        if not m or m.group(3) is not None:
            raise ValueError(f"Terminal override path segment must be a plain field name, got {segs[-1]!r}")
        node[m.group(1)] = value
    return patched


def _canonical_json_bytes(obj) -> bytes:
    """Stable serialization for seed_for()'s hash input — sorted keys, no
    incidental whitespace, so the same logical payload always hashes to the
    same bytes regardless of dict insertion order."""
    return json.dumps(obj, sort_keys=True, separators=(",", ":")).encode("utf-8")


def seed_for(root_seed, scenario_name: str, overrides: dict | None, trial_index: int) -> int:
    """
    Derives one trial's seed from (root_seed, scenario, overrides, trial_index)
    — NEVER from a shared/advancing RNG stream or the `random` module's global
    state. This is what makes trials [0, 1, 2, ...] computable in any order,
    in any number of processes, and still agree: each trial's seed depends
    only on its own identity, not on what ran before it. See
    docs/sim/MODEL.md's variance-layer section for why derived beats streamed
    here (a process pool has no shared stream to advance) and
    validate.py's order-independence check for the proof.

        seed_for(root_seed, name, overrides, i)
          = int.from_bytes(sha256(canonical_json(
                [root_seed, name, sorted(overrides.items()), i]
            )).digest()[:8], "big")

    `overrides` may be None (treated as {}); its items are sorted so
    insertion order never changes the hash.
    """
    payload = [root_seed, scenario_name, sorted((overrides or {}).items()), trial_index]
    digest = hashlib.sha256(_canonical_json_bytes(payload)).digest()
    return int.from_bytes(digest[:8], "big")


def _retinue_groups(composition: list[dict]) -> list:
    return [
        (row, dl.retinue_fighter(row["UnitType"], row["Tier"]))
        for row in composition
    ]


def _run_wave_attrition_trial(scenario: dict, rng: random.Random | None, variance_cfg: dict) -> dict:
    """
    The actual wave-attrition scenario setup, parameterized by an OPTIONAL
    rng. `rng=None` (every call site before task-076, and `run(name)` /
    `run_wave_attrition(scenario)` today) takes the exact same code path as
    before this task — the `if arrival_on` / `if dps_on` branches below are
    both unconditionally False whenever rng is None, regardless of what
    `variance_cfg` says, so passing `variance_cfg={}` or a fully-enabled
    config makes no difference to an unseeded call. That's what makes the
    task-076 safety property ("no seed -> bit-identical output") true by
    construction rather than by care at each call site.
    """
    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    chip_floor = dl.armor_chip_floor()

    arrival_cfg = variance_cfg.get("arrival_jitter", {}) if isinstance(variance_cfg, dict) else {}
    dps_cfg = variance_cfg.get("damage_roll_jitter", {}) if isinstance(variance_cfg, dict) else {}
    arrival_on = rng is not None and arrival_cfg.get("enabled", False)
    dps_on = rng is not None and dps_cfg.get("enabled", False)
    arrival_magnitude = float(arrival_cfg.get("magnitude", 0.0))
    dps_magnitude = float(dps_cfg.get("magnitude", 0.0))

    def maybe_jitter_arrival(seconds: float) -> float:
        return cm.jitter_arrival_seconds(seconds, rng, arrival_magnitude) if arrival_on else seconds

    def maybe_jitter_dps(fighter: dict) -> dict:
        return cm.jitter_fighter_dps(fighter, rng, dps_magnitude) if dps_on else fighter

    retinue_groups = [
        cm.WaveGroup(
            name=f"{row['UnitType']}_{row['Tier']}", fighter=maybe_jitter_dps(fighter), count=float(row["Count"]),
            arrival_seconds=maybe_jitter_arrival(float(row.get("ArrivalSeconds", 0.0))),
        )
        for row, fighter in _retinue_groups(scenario["Retinue"]["Composition"])
    ]
    if scenario.get("HeroPresent"):
        retinue_groups.append(cm.WaveGroup(name="hero", fighter=maybe_jitter_dps(dl.hero_fighter()), count=1.0))

    enemy_groups = [
        cm.WaveGroup(
            name=row.get("Name", row["EntityTier"]), fighter=maybe_jitter_dps(dl.enemy_fighter(row["EntityTier"])),
            count=float(row["Count"]), arrival_seconds=maybe_jitter_arrival(float(row.get("ArrivalSeconds", 0.0))),
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
    )
    result["scenario"] = scenario["Name"]
    result["kind"] = "wave_attrition"
    return result


def run_wave_attrition(scenario: dict) -> dict:
    """Unseeded entry point — bit-identical to this module's pre-task-076
    behavior (see _run_wave_attrition_trial's docstring for why)."""
    return _run_wave_attrition_trial(scenario, rng=None, variance_cfg={})


def _run_point_target_trial(scenario: dict, rng: random.Random | None, variance_cfg: dict) -> dict:
    """Same rng=None-is-a-no-op contract as _run_wave_attrition_trial. Only
    damage_roll_jitter applies here — arrival_jitter's `applies_to` in
    combat-model-constants.json omits point_target (that model has no
    arrival/time axis, see combat_model.army_ttk_vs_point_target's closed-form
    snapshot method), so it's simply never consulted in this function."""
    chip_floor = dl.armor_chip_floor()
    target = dl.enemy_fighter(scenario["Target"]["EntityTier"])

    dps_cfg = variance_cfg.get("damage_roll_jitter", {}) if isinstance(variance_cfg, dict) else {}
    dps_on = rng is not None and dps_cfg.get("enabled", False)
    dps_magnitude = float(dps_cfg.get("magnitude", 0.0))

    def maybe_jitter_dps(fighter: dict) -> dict:
        return cm.jitter_fighter_dps(fighter, rng, dps_magnitude) if dps_on else fighter

    groups = [
        cm.ArmyGroup(name=f"{row['UnitType']}_{row['Tier']}", fighter=maybe_jitter_dps(fighter), count=int(row["Count"]))
        for row, fighter in _retinue_groups(scenario["Retinue"]["Composition"])
    ]
    hero = maybe_jitter_dps(dl.hero_fighter()) if scenario.get("HeroPresent") else None

    ttk, breakdown = cm.army_ttk_vs_point_target(target, groups, chip_floor, hero=hero)
    return {
        "scenario": scenario["Name"],
        "kind": "point_target",
        "target": target["display_name"],
        "target_max_hp": target["max_hp"],
        "surround_cap_estimate": target.get("surround_cap_estimate"),
        "ttk_seconds": round(ttk, 2),
        "breakdown": breakdown,
    }


def run_point_target(scenario: dict) -> dict:
    """Unseeded entry point — bit-identical to this module's pre-task-076
    behavior."""
    return _run_point_target_trial(scenario, rng=None, variance_cfg={})


def run(name: str, seed: int | None = None) -> dict:
    """
    `seed=None` (every call site before task-076, and the default for every
    call site after it) is bit-identical to this module's pre-task-076
    behavior: no rng is constructed, so `_run_*_trial`'s `arrival_on`/
    `dps_on` are unconditionally False regardless of what
    `combat-model-constants.json`'s `variance_model` block says.

    `seed` given constructs ONE `random.Random(seed)` directly (not via
    `seed_for`'s trial-index derivation — that's for `run_trials`/
    `compute_trial`'s multi-trial case; a single explicit seed here needs no
    further derivation) and applies whichever variance sources are enabled
    in the committed constants file. This is what the CLI's `--seed S`
    (without `--trials`, or with `--trials 1`) calls.
    """
    scenario = dl.load_scenario(name)
    kind = scenario["Kind"]
    rng = random.Random(seed) if seed is not None else None
    variance_cfg = dl.load_combat_model_constants().get("variance_model", {}) if rng is not None else {}
    if kind == "wave_attrition":
        return _run_wave_attrition_trial(scenario, rng, variance_cfg)
    if kind == "point_target":
        return _run_point_target_trial(scenario, rng, variance_cfg)
    raise ValueError(f"Unknown scenario Kind '{kind}' in {name}.json")


def compute_trial(name: str, trial_index: int, root_seed=None, overrides: dict | None = None) -> dict:
    """
    Computes exactly ONE trial, fully self-contained — re-reads the scenario
    and constants data itself rather than sharing any state with a caller —
    so it is safe to call from a separate process. Determinism depends ONLY
    on (root_seed, name, overrides, trial_index) via `seed_for`, never on
    call order or which process computed it. This is the function
    validate.py's order-independence check calls directly, in shuffled order
    and inside a `ProcessPoolExecutor`, to prove that property holds.
    """
    scenario = dl.load_scenario(name)
    scenario = _apply_scenario_overrides(scenario, overrides)
    kind = scenario["Kind"]
    variance_cfg = dl.load_combat_model_constants().get("variance_model", {})
    seed = seed_for(root_seed, name, overrides, trial_index)
    rng = random.Random(seed)
    if kind == "wave_attrition":
        return _run_wave_attrition_trial(scenario, rng, variance_cfg)
    if kind == "point_target":
        return _run_point_target_trial(scenario, rng, variance_cfg)
    raise ValueError(f"Unknown scenario Kind '{kind}' in {name}.json")


# Numeric result fields run_trials() computes a distribution summary over,
# per scenario kind — the fields _run_wave_attrition_trial/_run_point_target_trial
# already produce, nothing new.
_SUMMARY_FIELDS = {
    "wave_attrition": ("retinue_survivors", "enemy_survivors", "elapsed_seconds"),
    "point_target": ("ttk_seconds",),
}


def _percentiles_5_95(sorted_values: list[float]) -> tuple[float, float]:
    """
    p5/p95 via `statistics.quantiles(data, n=100, method="inclusive")`.
    'inclusive' is stdlib's name for the conventional linear-interpolation
    percentile definition (numpy's default, Excel's PERCENTILE.INC) — chosen
    explicitly, not left as the module default ('exclusive'), because
    percentile choice is method-sensitive at the small trial counts (e.g.
    n=8) this harness's reproducibility/order-independence checks use, and a
    reader comparing two runs needs to know which convention produced the
    numbers. See docs/sim/MODEL.md's variance-layer section.
    `quantiles(..., n=100)` returns 99 cut points for percentiles 1..99;
    index 4 is p5, index 94 is p95.
    """
    if len(sorted_values) < 2:
        v = sorted_values[0]
        return v, v
    cuts = statistics.quantiles(sorted_values, n=100, method="inclusive")
    return cuts[4], cuts[94]


def _summarize(results: list[dict], kind: str) -> dict:
    summary = {}
    for field_name in _SUMMARY_FIELDS[kind]:
        values = sorted(r[field_name] for r in results)
        n = len(values)
        p5, p95 = _percentiles_5_95(values)
        summary[field_name] = {
            "n": n,
            "mean": statistics.fmean(values),
            "median": statistics.median(values),
            "p5": p5,
            "p95": p95,
            "min": values[0],
            "max": values[-1],
            "stdev": statistics.stdev(values) if n > 1 else 0.0,
        }
    return summary


def run_trials(name: str, trials: int, root_seed=None, overrides: dict | None = None) -> dict:
    """
    Runs `trials` independent, deterministically-seeded trials of scenario
    `name` and returns their results plus a stdlib-computed distribution
    summary. See `compute_trial`/`seed_for` for the per-trial seed
    derivation (order-independent by construction) and
    docs/sim/LIMITATIONS.md's variance-layer section for what the resulting
    spread may and may not be used to argue.
    """
    scenario = dl.load_scenario(name)
    scenario = _apply_scenario_overrides(scenario, overrides)
    kind = scenario["Kind"]
    variance_cfg = dl.load_combat_model_constants().get("variance_model", {})

    results = [compute_trial(name, i, root_seed, overrides) for i in range(trials)]

    applicable_enabled = [
        src for src, cfg in variance_cfg.items()
        if isinstance(cfg, dict) and cfg.get("enabled") and kind in cfg.get("applies_to", [])
    ]
    diagnostic = any(variance_cfg[src].get("status") == "invented" for src in applicable_enabled)

    return {
        "scenario": name,
        "kind": kind,
        "trials": trials,
        "root_seed": root_seed,
        "variance_sources_enabled": applicable_enabled,
        "diagnostic_invented_variance": diagnostic,
        "results": results,
        "summary": _summarize(results, kind),
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


def print_trials_summary(scenario: dict, trial_result: dict) -> None:
    print(f"\n=== {scenario.get('DisplayName', trial_result['scenario'])} ({trial_result['scenario']}) — "
          f"{trial_result['trials']} trials, root_seed={trial_result['root_seed']} ===")
    print(f"  kind: {trial_result['kind']}")
    if trial_result["variance_sources_enabled"]:
        print(f"  variance sources enabled: {', '.join(trial_result['variance_sources_enabled'])}")
    else:
        print("  variance sources enabled: (none — every trial is identical; see combat-model-constants.json's "
              "variance_model block to turn one on)")
    if trial_result["diagnostic_invented_variance"]:
        print("  " + "=" * 68)
        print("  DIAGNOSTIC: an enabled variance source is HARNESS-INVENTED (no shipped")
        print("  CVar or committed data backs its magnitude). Treat this spread as")
        print("  illustrative only, not a measured confidence band.")
        print("  " + "=" * 68)
    for field_name, stats in trial_result["summary"].items():
        print(f"  {field_name:>18}: n={stats['n']:<4} mean={stats['mean']:>9.3f} median={stats['median']:>9.3f} "
              f"p5={stats['p5']:>9.3f} p95={stats['p95']:>9.3f} min={stats['min']:>9.3f} max={stats['max']:>9.3f} "
              f"stdev={stats['stdev']:>8.3f}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Run docs/data/scenarios/*.json wave/point-target scenarios.")
    parser.add_argument("name", nargs="?", help="scenario file name (without .json)")
    parser.add_argument("--all", action="store_true", help="run every scenario in docs/data/scenarios/")
    parser.add_argument("--list", action="store_true", help="list available scenario names")
    parser.add_argument("--json", action="store_true",
                         help="emit the full, unrounded-past-what-combat_model.py-already-rounds result "
                              "as JSON instead of the human table (single object, or a JSON array under --all)")
    parser.add_argument("--trials", type=int, default=1,
                         help="run N seeded trials of a single named scenario and print a distribution summary "
                              "(task-076). N absent or 1 with no --seed leaves output unchanged from before "
                              "this flag existed.")
    parser.add_argument("--seed", type=int, default=None,
                         help="root seed for --trials>1 (trial seeds are derived from it, see "
                              "scenario_runner.seed_for), or the single seed for a bare seeded run "
                              "(--trials absent/1). Omitted entirely -> no seed constructed, output unchanged.")
    args = parser.parse_args()

    if args.list:
        for n in dl.list_scenarios():
            print(n)
        return

    names = dl.list_scenarios() if args.all else ([args.name] if args.name else [])
    if not names:
        parser.print_help()
        return

    if args.trials > 1:
        if args.all or len(names) != 1:
            parser.error("--trials requires a single scenario name, not --all")
        name = names[0]
        trial_result = run_trials(name, args.trials, root_seed=args.seed)
        if args.json:
            print(json.dumps(to_json_safe(trial_result), indent=2))
        else:
            print_trials_summary(dl.load_scenario(name), trial_result)
        return

    if args.json:
        results = [to_json_safe(run(n, seed=args.seed)) for n in names]
        print(json.dumps(results if args.all else results[0], indent=2))
        return

    for n in names:
        scenario = dl.load_scenario(n)
        result = run(n, seed=args.seed)
        print_result(scenario, result)


if __name__ == "__main__":
    main()
