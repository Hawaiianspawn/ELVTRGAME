"""
drift_check.py — task-071, the recurring half of the sim-tuning-loop epic.

task-069 (sweep.py) made a sweep reproducible on demand. task-070 (scenario
library) gave it 10 scenarios to sweep. Neither answers "did today's sweep
say something different from yesterday's" — that is this file's one job.

WHAT THIS IS NOT: a ranking or best-cell tool. It inherits sweep.py's GUARD
(no argmin, no "closest to measured," no sort) because a drift check answers
a different question than a fit check — "did a committed data file move the
curve," never "which cell is closest to correct." See docs/sim/LIMITATIONS.md
§1 and sweep.py's own module docstring before changing anything here.

THE MODEL IS DETERMINISTIC. combat_model.py has no RNG anywhere (verified:
no `random`, no wall-clock input) — identical docs/data/*.json content
always produces byte-identical output. That means a nonzero diff between a
fresh run and the committed baseline is NEVER run-to-run noise; it is always
either (a) rounding-floor dust from combat_model.py's own round(x, 2) calls
(bounded at 0.01), or (b) a real change to a committed data file or to this
harness's own math. THE_TOLERANCE below is set at 2x that rounding floor for
exactly this reason — see its docstring for the full justification.

USAGE

    py Scripts/sim/drift_check.py                  # CHECK mode (default).
                                                     # Exit 0 = no drift found.
                                                     # Exit 1 = a regression
                                                     # was found, OR the
                                                     # baseline file itself is
                                                     # missing/stale/malformed.
    py Scripts/sim/drift_check.py --refresh         # PREVIEW a new baseline
                                                     # against the committed
                                                     # one. Prints the full
                                                     # diff. NEVER writes.
    py Scripts/sim/drift_check.py --refresh --yes   # WRITES docs/sim/baseline.json.
                                                     # Requires a human to pass
                                                     # --yes explicitly, on top
                                                     # of --refresh. There is no
                                                     # flag combination that
                                                     # lets a --check run (the
                                                     # one a schedule would
                                                     # invoke) write the file it
                                                     # is comparing against.

WHAT'S BASELINED — SWEEP_DEFINITIONS below, 7 committed sweep.py commands
covering all 4 axis families (entity-tiers, unit-types, upgrades, constants,
scenario composition) across 5 of the 10 committed scenarios (both
wave_attrition and point_target kinds; both illustrative-only and
trustworthy model paths — each entry's "trust" field says which). This is a
curated subset, not exhaustive coverage of every scenario x every axis —
see docs/sim/DRIFT-CHECK.md for why this set and not a larger or smaller
one.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import sweep as sw  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
BASELINE_PATH = REPO_ROOT / "docs" / "sim" / "baseline.json"

# ---------------------------------------------------------------------------
# THE TOLERANCE — one number, justified once, used everywhere below.
# ---------------------------------------------------------------------------

# combat_model.py rounds every metric this file tracks to 2 decimal places
# before it ever leaves the model (WaveTickRecord's final-result dict:
# retinue_survivors/enemy_survivors/elapsed_seconds all round(x, 2);
# scenario_runner.run_point_target: ttk_seconds round(ttk, 2)). Because the
# model is deterministic (see module docstring), the ONLY nonzero delta a
# rerun against UNCHANGED data can ever produce is exactly 0 — there is no
# rounding path that depends on anything but the input data itself. So this
# tolerance is not absorbing measurement noise (there isn't any); it exists
# so that a genuinely tiny, real change (e.g. a data file edit that only
# moves a downstream metric by 0.01 through several multiplications) doesn't
# get reported with the same alarm as a multi-point swing. 2x the harness's
# own round(x, 2) floor (0.01) is the smallest threshold that still
# guarantees "below this = could only ever be presentation rounding, never a
# real change too small to matter" — set any lower and it stops meaning
# anything different from 0.01; set it higher and it starts hiding changes
# on the same order as the effects this harness exists to detect (e.g.
# SWEEPS.md's HP-breakpoint demo moves survivors by single digits per 100-HP
# step — a threshold in the tens would swallow exactly that kind of finding).
ABS_TOLERANCE = 0.02

NUMERIC_METRICS = {"retinue_survivors", "enemy_survivors", "elapsed_seconds", "ttk_seconds"}

# The `result` field (wave_attrition only: "retinue_wiped" / "enemy_wiped" /
# "timed_out") is categorical, not numeric — ANY change is flagged
# regardless of ABS_TOLERANCE. A win/loss flip is the headline signal this
# whole harness exists to surface (docs/sim/SWEEPS.md: "the retinue actually
# WINS in 15 of 27 cells") and must never be filtered by a numeric threshold
# built for continuous metrics.

# ---------------------------------------------------------------------------
# SWEEP_DEFINITIONS — the committed set. Edit this list, then run
# `--refresh --yes` deliberately, to add/change what's baselined.
# ---------------------------------------------------------------------------

SWEEP_DEFINITIONS = [
    {
        "id": "A-floor1-militia-hp-breakpoint",
        "scenario": "floor1-swarm-wave",
        "axes": ["upgrades:tier_ladder.tiers[id=militia].hp=130,200,300,400,600,900"],
        "trust": (
            "wave_attrition — DRIFT-ONLY signal, not a correctness claim. "
            "docs/sim/LIMITATIONS.md §1: this model does not reproduce its one "
            "measured baseline (GATE1 109-111 of 120); at committed defaults it "
            "predicts a full retinue wipe. This entry exists to catch CHANGE in "
            "upgrades.json's Militia HP row, never to assert these survivor "
            "counts are right. Exact reproduction of docs/sim/SWEEPS.md "
            "Demonstration 1 — same command, same 6 cells."
        ),
    },
    {
        "id": "B-floor1-spearmen-count-breakpoint",
        "scenario": "floor1-swarm-wave",
        "axes": ["scenario:Retinue.Composition[UnitType=spearmen].Count=32,64,96,128,160,200,250"],
        "trust": (
            "wave_attrition — DRIFT-ONLY, same caveat as A. Catches drift in "
            "floor1-swarm-wave.json's own composition numbers and in the enemy "
            "population's stat block (brood_fodder / brood_soldier_melee), "
            "neither of which A's axis touches. Exact reproduction of "
            "docs/sim/SWEEPS.md Demonstration 2 — same command, same 7 cells."
        ),
    },
    {
        "id": "C-floor2-ranged-dps-band",
        "scenario": "floor2-ranged-wave",
        "axes": ["entity-tiers:tiers[Name=brood_soldier_ranged].DPS=20,26,32,40"],
        "trust": (
            "wave_attrition — DRIFT-ONLY, same caveat as A. Only entry covering "
            "the ranged-enemy path (uncapped-by-frontage damage on both sides). "
            "20-40 brackets the shipped default (26)."
        ),
    },
    {
        "id": "D-gate1-frontage-model-sensitivity",
        "scenario": "gate1-calibration-wave1",
        "axes": [
            "constants:wave_attrition_model.EngagedSpacingUU=25,45,51",
            "constants:wave_attrition_model.MaxAttackersPerUnit=1,2,4",
            "constants:wave_attrition_model.MeleeContactFacingFraction=0.25,0.5,1.0",
        ],
        "trust": (
            "wave_attrition — DIAGNOSTIC (family 3, harness constants), "
            "DRIFT-ONLY. Deliberately run against the CURRENT gate1-calibration-"
            "wave1.json fixture (real per-rank ArrivalSeconds, task-068), not "
            "the historical zeroed-arrival reconstruction docs/sim/SWEEPS.md "
            "uses to reproduce the old 27-cell table byte-for-byte — that "
            "reconstruction is a one-time apples-to-apples check against a "
            "retired table, not something worth re-baselining forever. This "
            "entry instead tracks the harness's REAL current sensitivity band "
            "across combat-model-constants.json's own documented ranges "
            "(EngagedSpacingUU_range, MaxAttackersPerUnit shipped-vs-strict, "
            "MeleeContactFacingFraction_range) — the most load-bearing 27 "
            "numbers this repo currently has an opinion about."
        ),
    },
    {
        "id": "E-floor2-elite-armor-band",
        "scenario": "floor2-elite-point-target",
        "axes": ["entity-tiers:tiers[Name=brood_elite].Armor=8,12,16,20"],
        "trust": (
            "point_target — TRUSTWORTHY model path (docs/sim/LIMITATIONS.md §3: "
            "reproduces entity-tiers.md §7's own table exactly), still a lower-"
            "bound / clean-fight assumption per that same section. 8-20 "
            "brackets the shipped default (12)."
        ),
    },
    {
        "id": "F-floor3-boss-militia-dps-band",
        "scenario": "floor3-boss-point-target",
        "axes": ["upgrades:tier_ladder.tiers[id=militia].dps=24,30,36,42"],
        "trust": (
            "point_target — TRUSTWORTHY model path, same caveat as E. Catches "
            "upgrades.json drift on the trustworthy path (A/B/G below only "
            "cover upgrades.json on the illustrative-only wave path). 24-42 "
            "brackets the shipped default (30)."
        ),
    },
    {
        "id": "G-floor1-spearmen-cleave-band",
        "scenario": "floor1-swarm-wave",
        "axes": ["unit-types:types.spearmen.combat.targets_per_hit=4,8,12"],
        "trust": (
            "wave_attrition — DRIFT-ONLY, same caveat as A. Only entry covering "
            "unit-types.json. 4-12 brackets the shipped default (8) — the same "
            "TargetsPerHit dial validate.py's own cleave-sensitivity regression "
            "guard exists to keep meaningful (docs/sim/VALIDATION.md)."
        ),
    },
]


# ---------------------------------------------------------------------------
# Running a definition and extracting the metrics that matter
# ---------------------------------------------------------------------------

def extract_metrics(result: dict) -> dict:
    """Pulls just the metrics this file tracks out of one scenario_runner
    result dict — deliberately NOT the full result (no `log`, no
    per-group `breakdown`): those are useful for a human reading a sweep,
    not stable enough (or meaningful enough) as drift-check targets."""
    if result["kind"] == "wave_attrition":
        return {
            "retinue_survivors": result["retinue_survivors"],
            "enemy_survivors": result["enemy_survivors"],
            "elapsed_seconds": result["elapsed_seconds"],
            "result": result["result"],
        }
    return {"ttk_seconds": result["ttk_seconds"]}


def run_definition(defn: dict) -> dict:
    """Runs one SWEEP_DEFINITIONS entry through sweep.py's own machinery
    (parse_axis + run_sweep — the exact code path `py Scripts/sim/sweep.py`
    uses from the command line) and returns a baseline-file-shaped entry."""
    axes = [sw.parse_axis(spec) for spec in defn["axes"]]
    rows = sw.run_sweep(defn["scenario"], axes)
    return {
        "id": defn["id"],
        "scenario": defn["scenario"],
        "command": "py Scripts/sim/sweep.py {} {}".format(
            defn["scenario"], " ".join(f'--axis "{a}"' for a in defn["axes"]),
        ),
        "axes": [{"file": a.file, "path": a.path, "values": a.values} for a in axes],
        "trust": defn["trust"],
        "diagnostic_family_3": sw.touches_family_3(axes),
        "cells": [
            {"overrides": row["overrides"], "metrics": extract_metrics(row["result"])}
            for row in rows
        ],
    }


def build_fresh_baseline() -> dict:
    return {
        "$schema_note": (
            "task-071. Committed expected results for the 7 sweep.py commands "
            "in Scripts/sim/drift_check.py's SWEEP_DEFINITIONS. Regenerated ONLY "
            "by `py Scripts/sim/drift_check.py --refresh --yes` — a deliberate, "
            "explicit act (see docs/sim/DRIFT-CHECK.md). Never hand-edited."
        ),
        "tolerance": {
            "abs": ABS_TOLERANCE,
            "note": (
                "2x the harness's own round(x, 2) output precision. The model "
                "is fully deterministic (no RNG anywhere in combat_model.py), so "
                "any nonzero delta against unchanged data is impossible — this "
                "tolerance separates real-but-tiny changes from rounding dust, "
                "not run-to-run noise. See drift_check.py's ABS_TOLERANCE "
                "docstring for the full justification."
            ),
            "categorical_result_field": (
                "wave_attrition cells' `result` field (retinue_wiped / "
                "enemy_wiped / timed_out) is flagged on ANY change, regardless "
                "of ABS_TOLERANCE — a qualitative win/loss flip is the headline "
                "signal this harness exists to catch."
            ),
        },
        "entries": [run_definition(d) for d in SWEEP_DEFINITIONS],
    }


# ---------------------------------------------------------------------------
# Comparison
# ---------------------------------------------------------------------------

def diff_cell(old_metrics: dict, new_metrics: dict) -> list[str]:
    """Returns a list of human-readable diff lines for one cell, empty if
    the cell is within tolerance on every tracked metric."""
    lines = []
    for key, new_val in new_metrics.items():
        old_val = old_metrics.get(key)
        if key == "result":
            if old_val != new_val:
                lines.append(f"    result: {old_val!r} -> {new_val!r}  [QUALITATIVE FLIP]")
            continue
        if key in NUMERIC_METRICS:
            if old_val is None:
                lines.append(f"    {key}: <missing in baseline> -> {new_val}")
                continue
            delta = abs(new_val - old_val)
            if delta > ABS_TOLERANCE:
                lines.append(f"    {key}: {old_val} -> {new_val}  (delta={delta:.4f}, tolerance={ABS_TOLERANCE})")
    return lines


def diff_entry(old_entry: dict, new_entry: dict) -> list[str]:
    """Returns a list of human-readable diff report lines for one
    SWEEP_DEFINITIONS entry. Matches cells by index (both baseline and fresh
    run use the SAME itertools.product order off the SAME axes list, per
    sweep.py's own GUARD section — no ranking, no re-sorting, so index
    alignment is exact as long as the definition's axes/values haven't
    changed) but sanity-checks the overrides dict at each index first and
    reports a structural mismatch loudly rather than silently comparing the
    wrong cells."""
    lines = []
    old_cells = old_entry.get("cells", [])
    new_cells = new_entry["cells"]
    if len(old_cells) != len(new_cells):
        lines.append(
            f"  [STRUCTURAL] cell count changed: baseline had {len(old_cells)}, fresh run has {len(new_cells)} "
            f"— SWEEP_DEFINITIONS' axis values changed since the baseline was written. Refresh required; "
            f"this is not a data-drift finding."
        )
        return lines
    for i, (old_cell, new_cell) in enumerate(zip(old_cells, new_cells)):
        if old_cell.get("overrides") != new_cell["overrides"]:
            lines.append(
                f"  [STRUCTURAL] cell {i} overrides changed: baseline {old_cell.get('overrides')} vs "
                f"fresh {new_cell['overrides']} — SWEEP_DEFINITIONS changed since the baseline was written. "
                f"Refresh required; this is not a data-drift finding."
            )
            continue
        cell_diff = diff_cell(old_cell.get("metrics", {}), new_cell["metrics"])
        if cell_diff:
            override_str = ", ".join(f"{k}={v}" for k, v in new_cell["overrides"].items())
            lines.append(f"  cell {i} ({override_str}):")
            lines.extend(cell_diff)
    return lines


def load_baseline() -> dict | None:
    if not BASELINE_PATH.exists():
        return None
    with BASELINE_PATH.open("r", encoding="utf-8") as f:
        return json.load(f)


def run_check() -> int:
    baseline = load_baseline()
    if baseline is None:
        print(f"NO BASELINE FOUND at {BASELINE_PATH}.")
        print("Run `py Scripts/sim/drift_check.py --refresh --yes` to create one deliberately.")
        return 1

    fresh_entries = {d["id"]: run_definition(d) for d in SWEEP_DEFINITIONS}
    old_entries = {e["id"]: e for e in baseline.get("entries", [])}

    any_regression = False
    any_structural = False
    print("=== drift_check: comparing fresh sweep results against docs/sim/baseline.json ===\n")

    all_ids = list(dict.fromkeys(list(old_entries) + list(fresh_entries)))
    for entry_id in all_ids:
        old_entry = old_entries.get(entry_id)
        new_entry = fresh_entries.get(entry_id)
        if old_entry is None:
            print(f"[{entry_id}] NEW entry in SWEEP_DEFINITIONS, absent from baseline -- refresh needed.")
            any_structural = True
            continue
        if new_entry is None:
            print(f"[{entry_id}] REMOVED from SWEEP_DEFINITIONS but still in baseline -- refresh needed.")
            any_structural = True
            continue

        entry_lines = diff_entry(old_entry, new_entry)
        if not entry_lines:
            print(f"[{entry_id}] OK — {len(new_entry['cells'])} cells, no drift beyond tolerance.")
            continue

        structural = any(l.strip().startswith("[STRUCTURAL]") for l in entry_lines)
        if structural:
            any_structural = True
            print(f"[{entry_id}] STRUCTURAL CHANGE, not a data-drift finding:")
        else:
            any_regression = True
            print(f"[{entry_id}] DRIFT DETECTED ({new_entry['scenario']}):")
            print(f"  {new_entry['trust']}")
        for l in entry_lines:
            print(l)
        print()

    print("=" * 70)
    if any_regression:
        print("RESULT: DRIFT DETECTED. One or more committed data files (or the "
              "harness's own math) produced a different sweep result than the "
              "committed baseline. See lines above for exactly what moved and "
              "by how much.")
    if any_structural:
        print("RESULT: STRUCTURAL CHANGE. SWEEP_DEFINITIONS in drift_check.py no "
              "longer matches docs/sim/baseline.json's entries -- run --refresh "
              "to inspect, then --refresh --yes once the change is intentional.")
    if not any_regression and not any_structural:
        print("RESULT: CLEAN. No drift beyond tolerance in any baselined sweep.")
    return 1 if (any_regression or any_structural) else 0


def run_refresh(write: bool) -> int:
    old_baseline = load_baseline()
    fresh = build_fresh_baseline()

    if old_baseline is not None:
        old_entries = {e["id"]: e for e in old_baseline.get("entries", [])}
        print("=== drift_check --refresh: preview of what would change ===\n")
        any_diff = False
        for entry in fresh["entries"]:
            old_entry = old_entries.get(entry["id"])
            if old_entry is None:
                print(f"[{entry['id']}] NEW entry (not in current baseline).")
                any_diff = True
                continue
            entry_lines = diff_entry(old_entry, entry)
            if entry_lines:
                any_diff = True
                print(f"[{entry['id']}]:")
                for l in entry_lines:
                    print(l)
                print()
        removed = set(old_entries) - {e["id"] for e in fresh["entries"]}
        for rid in removed:
            print(f"[{rid}] REMOVED (no longer in SWEEP_DEFINITIONS).")
            any_diff = True
        if not any_diff:
            print("No difference from the committed baseline.")
    else:
        print("=== drift_check --refresh: no committed baseline exists yet — this "
              "would CREATE docs/sim/baseline.json ===\n")

    if not write:
        print("\nPREVIEW ONLY. docs/sim/baseline.json was NOT written.")
        print("Re-run with `--refresh --yes` to write it -- a deliberate, explicit act.")
        return 0

    BASELINE_PATH.parent.mkdir(parents=True, exist_ok=True)
    with BASELINE_PATH.open("w", encoding="utf-8") as f:
        json.dump(fresh, f, indent=2)
        f.write("\n")
    print(f"\nWROTE {BASELINE_PATH}.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--refresh", action="store_true",
                         help="Preview (or, with --yes, write) a fresh baseline instead of checking.")
    parser.add_argument("--yes", action="store_true",
                         help="Required in addition to --refresh to actually write docs/sim/baseline.json. "
                              "Without it, --refresh only previews the diff.")
    args = parser.parse_args()

    if args.refresh:
        return run_refresh(write=args.yes)
    return run_check()


if __name__ == "__main__":
    sys.exit(main())
