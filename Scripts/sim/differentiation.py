"""
differentiation.py — task-091: does the (task-090-corrected) hero-build space
actually differentiate builds, or does one weapon (Cleave Melee Sweep,
targets_per_shot=8) dominate every roll regardless of scenario?

THIN DRIVER, per task-091's own instruction: reruns variety.py's existing
roll+wave-attrition path (via variety.run(), completely unmodified) across
many seeds for the WAVE side of the comparison. For the POINT-TARGET side,
variety.py cannot help directly — its run() only accepts Kind='wave_attrition'
scenarios (see its own guard) and task-090 owns that file, so it is not
touched here. Instead this rolls the SAME roster independently:
variety.sample_roster() is the only RNG consumer in variety.run()'s pipeline,
so a given seed produces an IDENTICAL roster regardless of which scenario
Kind it's then run against — the wave and point-target rows for one seed are
always the same 20 rolled builds, just fought two different ways.

Usage:
    py Scripts/sim/differentiation.py [--seeds N] [--roster K] [--count-per-build N]
        [--wave-scenario NAME] [--point-scenarios NAME [NAME ...]] [--json]

READ docs/sim/LIMITATIONS.md before trusting a wave-attrition number out of
this — section 1: that model does not reproduce GATE1's measured survival at
committed defaults. Point-target numbers reproduce entity-tiers.md §7's own
table (docs/sim/VALIDATION.md) and are trustworthy for what they cover
(docs/sim/LIMITATIONS.md §3). All comparisons here are RELATIVE (build vs
build inside one shared scenario), never an absolute survivor/TTK claim.

No stat blocks hardcoded — every fighter comes from data_loader.py via the
exact same resolve_hero_build/finalize_hero_build_fighter path variety.py
uses. No global random — random.Random(seed) only. No third-party deps.
"""

from __future__ import annotations

import argparse
import json
import random
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import combat_model as cm
import variety
import scenario_runner as sr  # read-only reuse of to_json_safe()

# A build counts as "competitive with rank 1" if its damage/dps is >= this
# fraction of rank 1's own. 0.5 ("within 2x") is the loosest band that still
# means something -- not tuned to flatter either verdict. STRICT_BAND (0.8,
# "within 25%") is reported alongside it so the reader isn't trusting one
# arbitrary cutoff -- see docs/sim/DIFFERENTIATION.md for both.
COMPETITIVE_BAND = 0.5
STRICT_BAND = 0.8


# ---------------------------------------------------------------------------
# Point-target side: mirrors variety.run()'s roll + build-resolution steps,
# targeting combat_model.army_ttk_vs_point_target instead of
# simulate_wave_attrition. Duplicates variety.build_retinue_groups()'s ~15
# lines of fighter-resolution because that function builds cm.WaveGroup, not
# cm.ArmyGroup -- everything upstream of it (sample_roster,
# evaluate_synergy_rules) is imported and reused, not reimplemented.
# ---------------------------------------------------------------------------

def run_point_target(hero_builds: dict, scenario_name: str, seed: int, roster_size: int, count_per_build: int) -> dict:
    scenario = dl.load_scenario(scenario_name)
    if scenario["Kind"] != "point_target":
        raise ValueError(f"differentiation.py: '{scenario_name}' is not a point_target scenario.")

    rng = random.Random(seed)
    roster = variety.sample_roster(hero_builds, roster_size, rng)
    fired_rules, mult = variety.evaluate_synergy_rules(hero_builds, roster)

    chip_floor = dl.armor_chip_floor()
    target = dl.enemy_fighter(scenario["Target"]["EntityTier"])
    hero = dl.hero_fighter() if scenario.get("HeroPresent") else None

    groups = []
    for b in roster:
        name = f"build{b['idx']:02d}_{b['chassis_id']}"
        components = dl.resolve_hero_build(
            b["chassis_id"], b["weapon_id"], b["projectile_id"], b["modification_id"], b["ability_id"],
        )
        m = mult[b["idx"]]
        components["rate_of_fire"] *= m["rate_of_fire"]
        components["accuracy"] = max(0.0, min(1.0, components["accuracy"] * m["accuracy"]))
        chassis_name = hero_builds["chassis"][b["chassis_id"]]["display_name"]
        weapon_name = hero_builds["weapon_archetypes"][b["weapon_id"]]["display_name"]
        fighter = dl.finalize_hero_build_fighter(name, f"{chassis_name} ({weapon_name})", components)
        fighter["dps"] *= m["dps"]
        groups.append(cm.ArmyGroup(name=name, fighter=fighter, count=int(count_per_build)))

    ttk, breakdown = cm.army_ttk_vs_point_target(target, groups, chip_floor, hero=hero)

    # denominator excludes "hero" -- hero isn't a rolled build, matching
    # variety.py's own convention of ranking only the rolled roster.
    total_group_dps = sum(breakdown[g.name]["group_dps"] for g in groups)
    ranked = []
    for b, g in zip(roster, groups):
        bd = breakdown[g.name]
        ranked.append({
            "idx": b["idx"], "name": g.name, "chassis_id": b["chassis_id"], "weapon_id": b["weapon_id"],
            "group_dps": bd["group_dps"], "engaged": bd["engaged"], "count": bd["count"],
            "share_of_roster_dps": round(bd["group_dps"] / total_group_dps, 4) if total_group_dps > 0 else 0.0,
        })
    ranked.sort(key=lambda r: r["group_dps"], reverse=True)

    return {
        "scenario": scenario_name, "seed": seed, "ttk_seconds": round(ttk, 2),
        "target": target["display_name"], "surround_cap_estimate": target.get("surround_cap_estimate"),
        "total_group_dps": round(total_group_dps, 2),
        "ranked_builds": ranked,
    }


# ---------------------------------------------------------------------------
# Shared concentration summary
# ---------------------------------------------------------------------------

def summarize(ranked: list[dict], damage_key: str, share_key: str) -> dict:
    rank1 = ranked[0]
    rank1_val = rank1[damage_key]
    competitive = sum(1 for r in ranked if rank1_val > 0 and r[damage_key] >= COMPETITIVE_BAND * rank1_val)
    strict = sum(1 for r in ranked if rank1_val > 0 and r[damage_key] >= STRICT_BAND * rank1_val)
    return {
        "rank1_weapon_id": rank1["weapon_id"],
        "rank1_chassis_id": rank1["chassis_id"],
        "rank1_share": rank1[share_key],
        "competitive_count_0.5x": competitive,
        "competitive_count_0.8x": strict,
        "roster_size": len(ranked),
    }


def run_all(seeds: list[int], roster_size: int, count_per_build: int,
            wave_scenario: str, point_scenarios: list[str]) -> dict:
    hero_builds = dl.load_hero_builds()
    wave_rows = []
    point_rows = {name: [] for name in point_scenarios}

    for seed in seeds:
        wave_data = variety.run(wave_scenario, seed, roster_size, count_per_build)
        wave_ranked = wave_data["ranked_builds"]
        wave_rows.append({"seed": seed, "ranked": wave_ranked,
                           **summarize(wave_ranked, "damage_dealt", "share_of_retinue_damage")})

        for point_scenario in point_scenarios:
            point_data = run_point_target(hero_builds, point_scenario, seed, roster_size, count_per_build)
            point_ranked = point_data["ranked_builds"]
            point_rows[point_scenario].append({"seed": seed, "ranked": point_ranked,
                                                **summarize(point_ranked, "group_dps", "share_of_roster_dps")})

    return {
        "wave_scenario": wave_scenario, "point_scenarios": point_scenarios,
        "seeds": seeds, "roster_size": roster_size, "count_per_build": count_per_build,
        "wave_rows": wave_rows, "point_rows": point_rows,
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def _band_stats(rows: list[dict]) -> dict:
    n = len(rows)
    return {
        "n_seeds": n,
        "avg_rank1_share": round(sum(r["rank1_share"] for r in rows) / n, 4),
        "min_rank1_share": round(min(r["rank1_share"] for r in rows), 4),
        "max_rank1_share": round(max(r["rank1_share"] for r in rows), 4),
        "avg_competitive_0.5x": round(sum(r["competitive_count_0.5x"] for r in rows) / n, 2),
        "avg_competitive_0.8x": round(sum(r["competitive_count_0.8x"] for r in rows) / n, 2),
        "rank1_weapon_frequency": dict(Counter(r["rank1_weapon_id"] for r in rows).most_common()),
    }


def print_report(data: dict) -> None:
    print(f"\n=== Build-space differentiation (task-091): {len(data['seeds'])} seeds, "
          f"roster={data['roster_size']}, count_per_build={data['count_per_build']} ===")
    print(f"  COMPETITIVE_BAND=0.5x / STRICT_BAND=0.8x of rank 1's own damage/dps")

    print(f"\n  --- WAVE ({data['wave_scenario']}) -- LIMITATIONS.md sec.1: relative ranking only, NOT an "
          f"absolute survivor claim ---")
    wave_stats = _band_stats(data["wave_rows"])
    print(f"  avg rank1 share: {wave_stats['avg_rank1_share']*100:.1f}%  "
          f"(range {wave_stats['min_rank1_share']*100:.1f}%-{wave_stats['max_rank1_share']*100:.1f}%)")
    print(f"  avg builds competitive within 0.5x: {wave_stats['avg_competitive_0.5x']:.2f} / {data['roster_size']}, "
          f"within 0.8x: {wave_stats['avg_competitive_0.8x']:.2f} / {data['roster_size']}")
    print(f"  rank-1 weapon frequency: {wave_stats['rank1_weapon_frequency']}")

    for point_scenario, rows in data["point_rows"].items():
        print(f"\n  --- POINT-TARGET ({point_scenario}) -- VALIDATED model, trustworthy within "
              f"LIMITATIONS.md sec.3's assumptions ---")
        point_stats = _band_stats(rows)
        print(f"  avg rank1 share: {point_stats['avg_rank1_share']*100:.1f}%  "
              f"(range {point_stats['min_rank1_share']*100:.1f}%-{point_stats['max_rank1_share']*100:.1f}%)")
        print(f"  avg builds competitive within 0.5x: {point_stats['avg_competitive_0.5x']:.2f} / {data['roster_size']}, "
              f"within 0.8x: {point_stats['avg_competitive_0.8x']:.2f} / {data['roster_size']}")
        print(f"  rank-1 weapon frequency: {point_stats['rank1_weapon_frequency']}")
    print()


# ---------------------------------------------------------------------------
# One runnable check: the two independently-rolled sides for a single seed
# must be the SAME roster (this script's entire load-bearing claim -- that
# wave and point-target rows are directly comparable per seed, not just
# per-aggregate). Run standalone: py Scripts/sim/differentiation.py --selftest
# ---------------------------------------------------------------------------

def selftest() -> None:
    hero_builds = dl.load_hero_builds()
    seed, roster_size = 7, 20
    roster_a = variety.sample_roster(hero_builds, roster_size, random.Random(seed))
    roster_b = variety.sample_roster(hero_builds, roster_size, random.Random(seed))
    keys = ("chassis_id", "weapon_id", "projectile_id", "modification_id", "ability_id", "origin_world_id")
    for a, b in zip(roster_a, roster_b):
        assert all(a[k] == b[k] for k in keys), f"selftest FAILED: same seed rolled different rosters ({a} vs {b})"
    print("selftest OK: same seed -> identical roster, independent of which scenario Kind consumes it.")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Measure hero-build differentiation across many seeds, wave vs point-target (task-091)."
    )
    parser.add_argument("--selftest", action="store_true", help="run the roster-determinism self-check and exit")
    parser.add_argument("--seeds", type=int, default=25, help="number of seeds, 1..N (default 25)")
    parser.add_argument("--roster", type=int, default=20, help="builds per roll (default 20, matches variety.py)")
    parser.add_argument("--count-per-build", type=int, default=2, help="soldiers per build (default 2)")
    parser.add_argument("--wave-scenario", default="floor1-swarm-wave")
    parser.add_argument("--point-scenarios", nargs="+",
                         default=["floor2-elite-point-target", "floor3-boss-point-target"])
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    if args.selftest:
        selftest()
        return

    seeds = list(range(1, args.seeds + 1))
    data = run_all(seeds, args.roster, args.count_per_build, args.wave_scenario, args.point_scenarios)

    if args.json:
        print(json.dumps(sr.to_json_safe(data), indent=2))
        return
    print_report(data)


if __name__ == "__main__":
    main()
