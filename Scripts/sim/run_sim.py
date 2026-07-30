"""
run_sim.py -- task-096: chains several existing wave_attrition scenarios into
one continuous RUN, carrying retinue survivors, ember income, and
supply/degrade across waves. Growth-site stops (docs/data/growth-sites.json)
grant embers between waves; this task does NOT spend them (task-097 does).

Usage:
    py Scripts/sim/run_sim.py <run-name>          -- human table
    py Scripts/sim/run_sim.py <run-name> --json   -- full result as JSON
    py Scripts/sim/run_sim.py --selftest          -- the two required checks

READ docs/sim/RUN-SIM.md and docs/sim/LIMITATIONS.md sec.1 before trusting a
number out of this. Every wave here is still fought by the SAME unvalidated
wave-attrition model (docs/sim/LIMITATIONS.md sec.1: does not reproduce
GATE1's own measured wave-1 survival at committed defaults, predicts a full
wipe instead). Chaining waves does not make any single wave's number any
more trustworthy -- it only lets ONE relative comparison (this run vs another
run of the same imperfect model) span multiple waves. A wipe faithfully
produced here is the correct result to report, not a bug to route around.

A run file (docs/data/scenarios/run-<name>.json) is read through
data_loader.load_scenario() like any other scenario file -- it is not a
"scenario" scenario_runner.py can run (no Kind combat_model.py understands;
see its own "Kind": "run_chain" field, read only so `scenario_runner.py
--all` fails with a clear message instead of a bare KeyError -- data_loader's
list_scenarios()/scenario_runner.py are both locked this task, so this is
the only mitigation available without touching either).

docs/data/economy.json and docs/data/growth-sites.json have NO
data_loader.py accessor, and data_loader.py is on this task's do-not-touch
list (task-076's lock forbids adding one there either). _load_economy() /
_load_growth_sites() below mirror data_loader._load_json's own plain-stdlib
read pattern instead of silently working around the "data_loader.py is the
ONLY module that touches docs/data/*.json" invariant its module docstring
states -- flagged here, in docs/sim/RUN-SIM.md, and in the task-096 handback
as a real gap this task could not close within its lock, not something
quietly patched over.

No stat blocks hardcoded -- every fighter still comes from data_loader.py.
No RNG anywhere in this driver (unlike variety.py/differentiation.py) --
the whole chain is a deterministic function of its input JSON.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import combat_model as cm
import scenario_runner as sr  # read-only reuse: sr.run() + sr.to_json_safe(), same as differentiation.py

REPO_ROOT = Path(__file__).resolve().parents[2]
DATA_DIR = REPO_ROOT / "docs" / "data"


def _load_economy() -> dict:
    with (DATA_DIR / "economy.json").open("r", encoding="utf-8") as f:
        return json.load(f)


def _load_growth_sites() -> dict:
    with (DATA_DIR / "growth-sites.json").open("r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Survivor carryover -- the pooled-scalar rule (see module docstring / RUN-SIM.md)
# ---------------------------------------------------------------------------

def _tier_rank(tier: str) -> int:
    """upgrades.json's tier_ladder.tiers list order (freed/militia/veteran/
    bannerman) IS the weakest-to-strongest ordering -- read through
    data_loader's public dict (insertion order preserved), not re-derived."""
    return list(dl.load_retinue_tiers().keys()).index(tier)


def redistribute_survivors(target_rows: list[dict], survivors_total: float) -> list[dict]:
    """
    task-096 carryover rule: WEAKEST-FIRST casualties.

    Wave N's actual per-group survivor breakdown is discarded -- only the
    POOLED scalar (`result['retinue_survivors']`, the same field
    scenario_runner.py already reports) crosses into wave N+1, per the task's
    own "survivors come back as a pooled count, not identified units"
    framing. That scalar is then redistributed across wave N+1's OWN
    authored Composition shape (which UnitType/Tier rows exist, and their
    relative Counts): each row keeps its full authored Count as long as the
    running total allows, working STRONGEST tier first, so the WEAKEST row
    absorbs the shortfall first (dropping toward, and past, 0 before any
    stronger row loses a single unit). This is the default the task asked
    for ("weakest-first unless you can justify better") -- not measured
    against anything, just the more defensible of the two obvious choices:
    the alternative (cut every row by the same proportion) would imply a
    Veteran dies exactly as often as a Freed, which nothing in
    upgrades.json's tier ladder supports either (Veteran is the tier
    explicitly described there as holding formation under fear -- routing
    less, not equally).

    Fractional survivor counts are NOT rounded -- carried through as float.
    combat_model.WaveGroup.count/alive_count are already float (the model
    treats "count" as a continuous HP-pool proxy, not an integer headcount),
    so keeping this fractional is consistent with an existing convention,
    not a new one, and avoids an arbitrary rounding direction compounding
    across a multi-wave chain.

    If survivors_total exceeds the target shape's own authored sum (can't
    happen from plain attrition against these committed scenarios, but the
    function has to define SOMETHING), the surplus is added to the
    STRONGEST row -- symmetrical with the weakest row absorbing shortfall.
    """
    order = sorted(range(len(target_rows)), key=lambda i: _tier_rank(target_rows[i]["Tier"]))
    counts = [0.0] * len(target_rows)
    remaining = float(survivors_total)
    for i in reversed(order):  # strongest first
        want = float(target_rows[i]["Count"])
        take = min(want, remaining)
        counts[i] = take
        remaining -= take
    if remaining > 0:
        counts[order[-1]] += remaining  # overflow -> strongest row
    return [{**row, "Count": counts[i]} for i, row in enumerate(target_rows)]


# ---------------------------------------------------------------------------
# Supply / degrade -- economy.json's own formula, never hardcoded
# ---------------------------------------------------------------------------

def compute_degrade(retinue_rows: list[dict], economy: dict) -> dict:
    supply = economy["supply"]
    capacity = float(supply["start_capacity"])
    upkeep_per_unit = float(supply["upkeep_per_unit"])
    min_multiplier = float(supply["degrade"]["min_multiplier"])
    alive_units = sum(float(r["Count"]) for r in retinue_rows)
    demand = alive_units * upkeep_per_unit
    multiplier = max(min_multiplier, min(1.0, capacity / demand)) if demand > capacity else 1.0
    return {
        "alive_units": alive_units, "demand": demand,
        "capacity": capacity, "dps_multiplier": multiplier,
    }


# ---------------------------------------------------------------------------
# One wave -- mirrors scenario_runner.run_wave_attrition()'s own construction
# (that function can't be reused directly: it always reads the retinue
# composition off scenario["Retinue"]["Composition"], but this driver needs
# to substitute the CARRIED-FORWARD composition and apply a degrade DPS
# multiplier -- same "duplicate the ~15 lines because the existing function
# doesn't expose the right hook" precedent differentiation.py already set
# for run_point_target()).
# ---------------------------------------------------------------------------

def _build_retinue_groups(rows: list[dict], dps_multiplier: float) -> list[cm.WaveGroup]:
    groups = []
    for row in rows:
        fighter = dict(dl.retinue_fighter(row["UnitType"], row["Tier"]))
        fighter["dps"] = fighter["dps"] * dps_multiplier
        groups.append(cm.WaveGroup(
            name=f"{row['UnitType']}_{row['Tier']}", fighter=fighter, count=float(row["Count"]),
            arrival_seconds=float(row.get("ArrivalSeconds", 0.0)),
        ))
    return groups


def _build_enemy_groups(composition: list[dict]) -> list[cm.WaveGroup]:
    return [
        cm.WaveGroup(
            name=row.get("Name", row["EntityTier"]), fighter=dl.enemy_fighter(row["EntityTier"]),
            count=float(row["Count"]), arrival_seconds=float(row.get("ArrivalSeconds", 0.0)),
        )
        for row in composition
    ]


def run_one_wave(retinue_rows: list[dict], scenario: dict, dps_multiplier: float) -> dict:
    """Runs one wave_attrition scenario with the retinue composition and
    degrade multiplier this driver supplies, not the scenario file's own
    (unmodified) Retinue block. dps_multiplier == 1.0 with retinue_rows ==
    scenario["Retinue"]["Composition"] reproduces scenario_runner.run()
    byte-identically -- exactly what --selftest checks.

    The degrade multiplier is applied to the RETINUE only, not the Hero --
    economy.json's supply section describes upkeep/degrade purely in terms
    of recruited retinue units ("recruiting raises demand"); the Hero is
    never mentioned as an upkeep-consuming unit anywhere in that file, so
    this driver does not invent that it should be.
    """
    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    chip_floor = dl.armor_chip_floor()

    retinue_groups = _build_retinue_groups(retinue_rows, dps_multiplier)
    if scenario.get("HeroPresent"):
        retinue_groups.append(cm.WaveGroup(name="hero", fighter=dl.hero_fighter(), count=1.0))

    enemy_groups = _build_enemy_groups(scenario["Enemy"]["Composition"])

    result = cm.simulate_wave_attrition(
        retinue_groups=retinue_groups, enemy_groups=enemy_groups, chip_floor=chip_floor,
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


# ---------------------------------------------------------------------------
# The chain
# ---------------------------------------------------------------------------

def run_chain(run_name: str) -> dict:
    run_data = dl.load_scenario(run_name)
    economy = _load_economy()

    per_brood_killed = float(economy["embers"]["income"]["per_brood_killed"])
    growth_site_grant = float(economy["embers"]["income"]["growth_site_grant"])
    stops_after = {s["AfterWaveIndex"]: s["GrowthSiteId"] for s in run_data.get("Stops", [])}

    retinue_rows = [dict(r) for r in run_data["StartingComposition"]]
    embers_banked = 0.0
    waves_out = []
    stopped_early = False

    wave_names = run_data["Waves"]
    for idx, scenario_name in enumerate(wave_names):
        scenario = dl.load_scenario(scenario_name)
        if scenario["Kind"] != "wave_attrition":
            raise ValueError(f"run_sim.py: '{scenario_name}' is not a wave_attrition scenario -- cannot chain it.")

        degrade = compute_degrade(retinue_rows, economy)
        result = run_one_wave(retinue_rows, scenario, degrade["dps_multiplier"])
        embers_from_kills = (result["enemy_start"] - result["enemy_survivors"]) * per_brood_killed
        embers_banked += embers_from_kills

        wave_row = {
            "wave_index": idx,
            "scenario": scenario_name,
            "retinue_start": sum(r["Count"] for r in retinue_rows),
            "retinue_survivors": result["retinue_survivors"],
            "enemy_start": result["enemy_start"],
            "enemy_survivors": result["enemy_survivors"],
            "result": result["result"],
            "elapsed_seconds": result["elapsed_seconds"],
            "degrade": degrade,
            "embers_from_kills": round(embers_from_kills, 3),
        }

        if result["result"] == "retinue_wiped":
            # A wiped retinue never reaches the next breather -- no growth-site
            # grant, no carryover, the chain ends here. Report this wave, then stop.
            wave_row["embers_banked_running_total"] = round(embers_banked, 3)
            waves_out.append(wave_row)
            stopped_early = True
            break

        stop_id = stops_after.get(idx)
        if stop_id is not None:
            embers_banked += growth_site_grant
            wave_row["growth_site_stop"] = {"id": stop_id, "embers_grant": growth_site_grant}
        wave_row["embers_banked_running_total"] = round(embers_banked, 3)
        waves_out.append(wave_row)

        if idx + 1 < len(wave_names):
            next_scenario = dl.load_scenario(wave_names[idx + 1])
            target_rows = next_scenario["Retinue"]["Composition"]
            retinue_rows = redistribute_survivors(target_rows, result["retinue_survivors"])

    return {
        "run": run_name,
        "waves": waves_out,
        "embers_final": round(embers_banked, 3),
        "stopped_early": stopped_early,
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_report(data: dict) -> None:
    print(f"\n=== Run chain: {data['run']} ===")
    print(f"  {'w':>2} {'scenario':<26} {'ret_start':>9} {'ret_surv':>9} {'enemy_start':>11} "
          f"{'enemy_surv':>10} {'result':>14} {'demand':>7} {'cap':>5} {'mult':>5} "
          f"{'embers+':>8} {'embers_tot':>10}")
    for w in data["waves"]:
        print(f"  {w['wave_index']:>2} {w['scenario']:<26} {w['retinue_start']:>9.1f} "
              f"{w['retinue_survivors']:>9.1f} {w['enemy_start']:>11.1f} {w['enemy_survivors']:>10.1f} "
              f"{w['result']:>14} {w['degrade']['demand']:>7.1f} {w['degrade']['capacity']:>5.1f} "
              f"{w['degrade']['dps_multiplier']:>5.2f} {w['embers_from_kills']:>8.2f} "
              f"{w['embers_banked_running_total']:>10.2f}")
        if "growth_site_stop" in w:
            gs = w["growth_site_stop"]
            print(f"       [growth-site stop: {gs['id']}, +{gs['embers_grant']:.0f} embers]")
    if data["stopped_early"]:
        print(f"\n  RUN ENDED EARLY: retinue wiped on wave {data['waves'][-1]['wave_index']} "
              f"-- remaining waves not run.")
    print(f"\n  Final embers banked: {data['embers_final']:.2f}")
    print("\n  TRUST CAVEAT (docs/sim/LIMITATIONS.md sec.1, inherited unchanged): the wave-attrition")
    print("  model does not reproduce GATE1's own measured ~110-of-120 wave-1 survival at the harness's")
    print("  committed defaults, and predicts a full wipe instead. Every number above is a RELATIVE")
    print("  comparison between runs of this one imperfect model -- never an absolute claim about a")
    print("  played run. See docs/sim/RUN-SIM.md.")


# ---------------------------------------------------------------------------
# --selftest
# ---------------------------------------------------------------------------

def _selftest_reproduces_scenario_runner() -> None:
    """(a) A single-wave run with demand under capacity must reproduce
    scenario_runner.py's numbers for that scenario byte-identically --
    proof this driver has not silently changed the combat math.
    floor1-swarm-wave.json's 40 retinue units sit well under economy.json's
    start_capacity (60), so its degrade multiplier is guaranteed 1.0."""
    scenario_name = "floor1-swarm-wave"
    scenario = dl.load_scenario(scenario_name)
    retinue_rows = scenario["Retinue"]["Composition"]
    economy = _load_economy()
    degrade = compute_degrade(retinue_rows, economy)
    assert degrade["dps_multiplier"] == 1.0, (
        f"selftest fixture assumption broke: {scenario_name}'s {degrade['alive_units']:.0f} units should sit "
        f"under start_capacity ({degrade['capacity']:.0f}) -- demand={degrade['demand']:.0f}. Pick a different "
        f"fixture scenario for this check if economy.json's start_capacity has changed."
    )

    result = run_one_wave(retinue_rows, scenario, degrade["dps_multiplier"])
    reference = sr.run(scenario_name)
    for key in ("retinue_survivors", "enemy_survivors", "retinue_start", "enemy_start", "result", "elapsed_seconds"):
        assert result[key] == reference[key], f"selftest FAILED: '{key}' mismatch: {result[key]} != {reference[key]}"
    assert sr.to_json_safe(result["log"]) == sr.to_json_safe(reference["log"]), "selftest FAILED: tick log mismatch"
    print(f"selftest OK (a): run_one_wave('{scenario_name}', multiplier=1.0) is byte-identical to "
          f"scenario_runner.run('{scenario_name}').")


def _selftest_deterministic_repeat() -> None:
    """(b) Same inputs, run twice, must be identical. There is no RNG anywhere
    in this driver (unlike variety.py/differentiation.py) -- the whole chain
    is a pure function of its input JSON, so "same seed run twice" reduces to
    "call run_chain() twice with the same run name and diff the output."
    Guards against a shared-mutable-state bug (e.g. a fighter dict mutated
    in place and reused across calls) that a single run would never surface."""
    run_name = "run-slice-three-wave"
    a = run_chain(run_name)
    b = run_chain(run_name)
    assert sr.to_json_safe(a) == sr.to_json_safe(b), (
        f"selftest FAILED: run_chain('{run_name}') produced different output across two calls with "
        "identical inputs -- look for shared mutable state (e.g. a fighter dict edited in place)."
    )
    print(f"selftest OK (b): run_chain('{run_name}') is identical across two independent calls.")


def selftest() -> None:
    _selftest_reproduces_scenario_runner()
    _selftest_deterministic_repeat()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Chain wave_attrition scenarios into one multi-wave run with carryover/degrade/embers (task-096)."
    )
    parser.add_argument("name", nargs="?", help="run file name under docs/data/scenarios/ (without .json)")
    parser.add_argument("--json", action="store_true", help="emit the full result as JSON")
    parser.add_argument("--selftest", action="store_true", help="run the two required self-checks and exit")
    args = parser.parse_args()

    if args.selftest:
        selftest()
        return

    if not args.name:
        parser.print_help()
        return

    data = run_chain(args.name)
    if args.json:
        print(json.dumps(sr.to_json_safe(data), indent=2))
        return
    print_report(data)


if __name__ == "__main__":
    main()
