"""
scenario_runner.py — loads one docs/data/scenarios/<name>.json and runs it
through the matching model in combat_model.py.

Usage:
    py Scripts/sim/scenario_runner.py <scenario-name>
    py Scripts/sim/scenario_runner.py --all
    py Scripts/sim/scenario_runner.py --list

No stat blocks are hardcoded here — every fighter comes from data_loader.py.
"""

from __future__ import annotations

import argparse
import dataclasses
import json
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


def _retinue_groups(composition: list[dict]) -> list:
    return [
        (row, dl.retinue_fighter(row["UnitType"], row["Tier"]))
        for row in composition
    ]


def run_wave_attrition(scenario: dict) -> dict:
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
    )
    result["scenario"] = scenario["Name"]
    result["kind"] = "wave_attrition"
    return result


def run_point_target(scenario: dict) -> dict:
    chip_floor = dl.armor_chip_floor()
    target = dl.enemy_fighter(scenario["Target"]["EntityTier"])

    groups = [
        cm.ArmyGroup(name=f"{row['UnitType']}_{row['Tier']}", fighter=fighter, count=int(row["Count"]))
        for row, fighter in _retinue_groups(scenario["Retinue"]["Composition"])
    ]
    hero = dl.hero_fighter() if scenario.get("HeroPresent") else None

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


def run(name: str) -> dict:
    scenario = dl.load_scenario(name)
    kind = scenario["Kind"]
    if kind == "wave_attrition":
        return run_wave_attrition(scenario)
    if kind == "point_target":
        return run_point_target(scenario)
    raise ValueError(f"Unknown scenario Kind '{kind}' in {name}.json")


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


def main() -> None:
    parser = argparse.ArgumentParser(description="Run docs/data/scenarios/*.json wave/point-target scenarios.")
    parser.add_argument("name", nargs="?", help="scenario file name (without .json)")
    parser.add_argument("--all", action="store_true", help="run every scenario in docs/data/scenarios/")
    parser.add_argument("--list", action="store_true", help="list available scenario names")
    parser.add_argument("--json", action="store_true",
                         help="emit the full, unrounded-past-what-combat_model.py-already-rounds result "
                              "as JSON instead of the human table (single object, or a JSON array under --all)")
    args = parser.parse_args()

    if args.list:
        for n in dl.list_scenarios():
            print(n)
        return

    names = dl.list_scenarios() if args.all else ([args.name] if args.name else [])
    if not names:
        parser.print_help()
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
