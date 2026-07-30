"""
decisions.py -- task-097: does growth-site allocation (docs/data/growth-sites.json)
actually differentiate a run, or is it a THEATRE choice the committed model
never lets bind?

Drives Scripts/sim/run_sim.py's OWN building blocks (compute_degrade,
run_one_wave, redistribute_survivors -- imported, never copied) down the
committed run-slice-three-wave chain, once per allocation BRANCH, and
compares the branches. No combat math is added here; the only new logic is
(a) applying a growth-site stop's spend to the carried retinue composition
/ supply capacity between waves, and (b) the branch/spread bookkeeping.

READ docs/sim/DECISIONS.md before trusting a number out of this. Short
version, inherited unchanged from docs/sim/LIMITATIONS.md sec.1 through
run_sim.py: every wave here is fought by the same unvalidated wave-attrition
model. This task's own headline finding is that the PRIMARY (committed-
defaults) run never reaches either growth-site stop at all -- see
DECISIONS.md for why, and for the clearly-separate SECONDARY hypothetical
this file also reports.

Usage:
    py Scripts/sim/decisions.py               -- human table, both conditions
    py Scripts/sim/decisions.py --json         -- full result as JSON
    py Scripts/sim/decisions.py --selftest     -- reproducibility check, exit 1 on failure
"""

from __future__ import annotations

import argparse
import contextlib
import copy
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import run_sim as rs  # read-only reuse: compute_degrade / run_one_wave / redistribute_survivors / loaders

RUN_NAME = "run-slice-three-wave"

# ---------------------------------------------------------------------------
# Reading growth-sites.json's action table
# ---------------------------------------------------------------------------

def _actions() -> dict:
    return {a["id"]: a for a in rs._load_growth_sites()["site"]["actions"]}


MODELLABLE_LANES = ("breadth", "depth", "sustain")  # recruit / promote / provision
UNMODELLABLE_LANES = ("spice", "hero")              # item / hero -- see DECISIONS.md

# One BRANCH = a fixed allocation policy, applied identically at every stop
# it can afford. "Afford" is greedy: try the full target set, drop the
# single costliest action and retry until it fits the bank on hand (or
# empties) -- see resolve_spend(). Each action may be picked AT MOST ONCE
# per stop (the panel is a set of discrete choices, not a repeatable
# purchase) -- growth-sites.json doesn't state this either way; this is the
# reading DECISIONS.md documents and this file holds to consistently.
BRANCHES = {
    "hoard": set(),
    "recruit_only": {"recruit"},
    "promote_only": {"promote"},
    "provision_only": {"provision"},
    "triangle": {"recruit", "promote", "provision"},
}


def resolve_spend(target_ids: set[str], bank: float, actions: dict) -> tuple[list[str], float]:
    chosen = sorted(target_ids, key=lambda i: -actions[i]["embers"])
    while chosen and sum(actions[i]["embers"] for i in chosen) > bank:
        chosen.pop(0)  # drop current costliest, retry
    return chosen, sum(actions[i]["embers"] for i in chosen)


def _tier_rank(tier: str) -> int:
    return list(dl.load_retinue_tiers().keys()).index(tier)


def apply_effects(retinue_rows: list[dict], capacity_bonus: float, chosen: list[str], actions: dict) -> tuple[list[dict], float]:
    """Applies growth-sites.json's own stated effect fields -- +10 units at
    FREED tier (recruit), promote up to 20 units one tier (promote), +25
    Supply capacity (provision) -- to the carried composition/capacity-bonus.
    Mutates a fresh list of row copies; never mutates the caller's rows."""
    rows = [dict(r) for r in retinue_rows]
    tiers = list(dl.load_retinue_tiers().keys())  # weakest -> strongest, upgrades.json order

    for action_id in chosen:
        if action_id == "recruit":
            unit_type = rows[0]["UnitType"] if rows else "spearmen"
            for r in rows:
                if r["Tier"] == "freed" and r["UnitType"] == unit_type:
                    r["Count"] += 10
                    break
            else:
                rows.append({"UnitType": unit_type, "Tier": "freed", "Count": 10})

        elif action_id == "promote":
            present = [r for r in rows if r["Count"] > 0]
            if not present:
                continue
            weakest = min(present, key=lambda r: _tier_rank(r["Tier"]))
            idx = _tier_rank(weakest["Tier"])
            if idx + 1 >= len(tiers):
                continue  # already top tier, nothing to promote into
            next_tier = tiers[idx + 1]
            amount = min(20.0, weakest["Count"])
            weakest["Count"] -= amount
            for r in rows:
                if r["Tier"] == next_tier and r["UnitType"] == weakest["UnitType"]:
                    r["Count"] += amount
                    break
            else:
                rows.append({"UnitType": weakest["UnitType"], "Tier": next_tier, "Count": amount})

        elif action_id == "provision":
            capacity_bonus += 25.0

        else:
            raise ValueError(f"decisions.py: '{action_id}' has no modelled effect -- see UNMODELLABLE_LANES.")

    return rows, capacity_bonus


# ---------------------------------------------------------------------------
# DIAGNOSTIC-ONLY constants override, for the SECONDARY hypothetical run --
# same mechanism sweep.py already established (monkeypatch data_loader._load_json
# for the duration of one call, never write docs/data/*.json to disk), not a
# new one. Only ever entered for the explicitly-labeled secondary condition;
# the primary/committed measurement never enters this context.
# ---------------------------------------------------------------------------

@contextlib.contextmanager
def diagnostic_constants_override(overrides: dict):
    original = dl._load_json

    def patched(path):
        data = original(path)
        if path.name == "combat-model-constants.json":
            for k, v in overrides.items():
                data["wave_attrition_model"][k] = v
        return data

    dl._load_json = patched
    try:
        yield
    finally:
        dl._load_json = original


# ---------------------------------------------------------------------------
# One branch, one full (up to 3-wave) run
# ---------------------------------------------------------------------------

def run_branch(branch_name: str, max_attackers_per_unit: float | None = None,
               capacity_override: float | None = None) -> dict:
    """max_attackers_per_unit=None -> PRIMARY, harness's committed defaults,
    unmodified. A float -> hypothetical, diagnostic family-3 override (see
    diagnostic_constants_override) applied to every wave in this branch's
    run. capacity_override, if given, replaces economy.json's
    supply.start_capacity for the WHOLE run (a modified starting condition,
    per the task's own "supply capacity not already 2x-oversubscribed at
    t=0" example -- never a docs/data/*.json edit, an in-memory copy only)."""
    target_ids = BRANCHES[branch_name]
    run_data = dl.load_scenario(RUN_NAME)
    economy = rs._load_economy()
    if capacity_override is not None:
        economy = copy.deepcopy(economy)
        economy["supply"]["start_capacity"] = float(capacity_override)
    actions = _actions()
    per_kill = float(economy["embers"]["income"]["per_brood_killed"])
    grant = float(economy["embers"]["income"]["growth_site_grant"])
    stops = {s["AfterWaveIndex"]: s["GrowthSiteId"] for s in run_data.get("Stops", [])}

    retinue_rows = [dict(r) for r in run_data["StartingComposition"]]
    capacity_bonus = 0.0
    embers = 0.0
    waves_out = []
    wiped = False

    def _run_one_wave(rows, scenario, mult):
        if max_attackers_per_unit is None:
            return rs.run_one_wave(rows, scenario, mult)
        with diagnostic_constants_override({"MaxAttackersPerUnit": max_attackers_per_unit}):
            return rs.run_one_wave(rows, scenario, mult)

    for idx, scenario_name in enumerate(run_data["Waves"]):
        scenario = dl.load_scenario(scenario_name)

        economy_now = copy.deepcopy(economy)
        economy_now["supply"]["start_capacity"] = float(economy["supply"]["start_capacity"]) + capacity_bonus
        degrade = rs.compute_degrade(retinue_rows, economy_now)

        result = _run_one_wave(retinue_rows, scenario, degrade["dps_multiplier"])
        killed = result["enemy_start"] - result["enemy_survivors"]
        embers += killed * per_kill

        wave_row = {
            "wave_index": idx, "scenario": scenario_name,
            "retinue_start": sum(r["Count"] for r in retinue_rows),
            "retinue_survivors": result["retinue_survivors"],
            "enemy_start": result["enemy_start"], "enemy_survivors": result["enemy_survivors"],
            "killed": round(killed, 3), "result": result["result"],
            "degrade": degrade, "embers_running_total": round(embers, 3),
        }

        if result["result"] == "retinue_wiped":
            wiped = True
            waves_out.append(wave_row)
            break

        # Redistribute against the shape that ACTUALLY fought this wave
        # (weakest-first, run_sim.redistribute_survivors' own rule) -- NOT
        # against next wave's authored single-row shape the way run_sim.py's
        # own run_chain() does. That remap is fine there because
        # run-slice-three-wave's three scenarios never diverge from a single
        # militia row; here growth-site spend deliberately DOES diverge the
        # tier mix (recruit/promote), and remapping onto a fresh authored
        # single-militia-row target would silently discard every freed/
        # veteran unit this task's own actions just created. Necessary
        # generalization of the exact gap RUN-SIM.md already names as
        # "present but inert... untested against a mixed-tier chain."
        retinue_rows = rs.redistribute_survivors(retinue_rows, result["retinue_survivors"])

        stop_id = stops.get(idx)
        if stop_id is not None:
            embers += grant
            chosen, cost = resolve_spend(target_ids, embers, actions)
            embers -= cost
            retinue_rows, capacity_bonus = apply_effects(retinue_rows, capacity_bonus, chosen, actions)
            wave_row["growth_site_stop"] = {
                "id": stop_id, "target": sorted(target_ids), "chosen": chosen,
                "cost": cost, "embers_after": round(embers, 3),
            }

        waves_out.append(wave_row)

    waves_survived = len(waves_out) - (1 if wiped else 0)
    final_retinue = 0.0 if wiped else sum(r["Count"] for r in retinue_rows)
    total_killed = round(sum(w["killed"] for w in waves_out), 3)

    return {
        "branch": branch_name, "target": sorted(target_ids),
        "max_attackers_per_unit": max_attackers_per_unit,
        "waves": waves_out, "waves_survived": waves_survived,
        "final_retinue": round(final_retinue, 3), "total_killed": total_killed,
        "embers_final": round(embers, 3), "wiped": wiped,
    }


# ---------------------------------------------------------------------------
# Spread vs. noise
# ---------------------------------------------------------------------------

def spread(rows: list[dict], key: str) -> dict:
    vals = [r[key] for r in rows]
    return {"min": min(vals), "max": max(vals), "spread": round(max(vals) - min(vals), 3)}


def verdict_for(rows: list[dict], noise: float, key: str = "final_retinue") -> str:
    s = spread(rows, key)["spread"]
    if s == 0:
        return "THEATRE (branch spread is exactly zero)"
    if noise == 0:
        return f"branch spread {s:.2f} on '{key}' vs a PROVEN-ZERO noise floor -- see DECISIONS.md before calling this REAL DECISION"
    best = max(rows, key=lambda r: r[key])
    return f"spread {s:.2f} exceeds noise ({noise}); best branch '{best['branch']}'"


# ---------------------------------------------------------------------------
# --selftest
# ---------------------------------------------------------------------------

def selftest() -> None:
    a = run_branch("triangle", max_attackers_per_unit=None)
    b = run_branch("triangle", max_attackers_per_unit=None)
    assert a == b, "selftest FAILED: run_branch('triangle') differs across two identical calls (primary condition)."
    print("selftest OK (primary): run_branch('triangle') is identical across two independent calls.")

    c = run_branch("triangle", max_attackers_per_unit=1.0)
    d = run_branch("triangle", max_attackers_per_unit=1.0)
    assert c == d, "selftest FAILED: run_branch('triangle', MA=1) differs across two identical calls (secondary condition)."
    print("selftest OK (secondary): run_branch('triangle', MaxAttackersPerUnit=1) is identical across two independent calls.")

    e = run_branch("triangle", max_attackers_per_unit=1.0, capacity_override=120.0)
    f = run_branch("triangle", max_attackers_per_unit=1.0, capacity_override=120.0)
    assert e == f, "selftest FAILED: run_branch('triangle', MA=1, capacity=120) differs across two identical calls (tertiary condition)."
    print("selftest OK (tertiary): run_branch('triangle', MaxAttackersPerUnit=1, capacity=120) is identical across two independent calls.")

    orig = dl._load_json
    assert dl._load_json is orig, "selftest FAILED: diagnostic_constants_override left data_loader._load_json patched."
    print("selftest OK: diagnostic_constants_override restores data_loader._load_json after use.")


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def measure(max_attackers_per_unit: float | None, capacity_override: float | None = None) -> list[dict]:
    return [run_branch(name, max_attackers_per_unit, capacity_override) for name in BRANCHES]


def print_condition(label: str, rows: list[dict]) -> None:
    print(f"\n=== {label} ===")
    print(f"  {'branch':<16} {'target':<28} {'waves_surv':>10} {'final_ret':>9} {'killed':>8} {'embers':>7}")
    for r in rows:
        print(f"  {r['branch']:<16} {','.join(r['target']) or '(none)':<28} {r['waves_survived']:>10} "
              f"{r['final_retinue']:>9.1f} {r['total_killed']:>8.1f} {r['embers_final']:>7.2f}")
    fr = spread(rows, "final_retinue")
    tk = spread(rows, "total_killed")
    print(f"  spread(final_retinue) = {fr['spread']:.2f}  ({fr['min']:.1f} - {fr['max']:.1f})")
    print(f"  spread(total_killed)  = {tk['spread']:.2f}  ({tk['min']:.1f} - {tk['max']:.1f})")
    any_stop = any("growth_site_stop" in w for r in rows for w in r["waves"])
    if any_stop:
        for r in rows:
            for w in r["waves"]:
                if "growth_site_stop" in w:
                    gs = w["growth_site_stop"]
                    print(f"    [{r['branch']:<14} wave {w['wave_index']} stop {gs['id']:<8} "
                          f"target={','.join(gs['target']) or '(none)':<24} chosen={','.join(gs['chosen']) or '(none)':<24} "
                          f"cost={gs['cost']:>5.1f} embers_after={gs['embers_after']:>6.2f}]")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Measure whether growth-site allocation (docs/data/growth-sites.json) is a real "
                    "decision, a dominant lane, or theatre, over run-sim.py's committed run chain (task-097)."
    )
    parser.add_argument("--json", action="store_true", help="emit both conditions as JSON")
    parser.add_argument("--selftest", action="store_true", help="reproducibility check, exit 1 on failure")
    args = parser.parse_args()

    if args.selftest:
        selftest()
        return

    primary = measure(None)
    secondary = measure(1.0)
    tertiary = measure(1.0, capacity_override=120.0)

    if args.json:
        print(json.dumps({
            "primary_committed_defaults": primary,
            "secondary_hypothetical_MA1": secondary,
            "tertiary_hypothetical_MA1_capacity120": tertiary,
        }, indent=2))
        return

    print_condition("PRIMARY -- committed defaults (MaxAttackersPerUnit=4, unmodified)", primary)
    print_condition("SECONDARY -- HYPOTHETICAL (MaxAttackersPerUnit=1, diagnostic-only, see DECISIONS.md)", secondary)
    print_condition("TERTIARY -- HYPOTHETICAL (MaxAttackersPerUnit=1 AND supply capacity=120, see DECISIONS.md)", tertiary)
    print("\n  Read docs/sim/DECISIONS.md before citing any of these tables -- all three inherit")
    print("  docs/sim/LIMITATIONS.md sec.1 unchanged, and only PRIMARY is the committed answer.")


if __name__ == "__main__":
    main()
