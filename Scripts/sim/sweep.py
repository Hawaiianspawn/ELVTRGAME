"""
sweep.py — committed cross-product parameter sweep over scenario_runner.py.

task-069. Before this, the 27-cell sweep behind docs/sim/VALIDATION.md's most
consequential finding (the retinue actually WINS in 15/27 cells once the
cleave-coupling bug was fixed) was produced by an uncommitted, thrown-away
script — the exact pattern docs/sim/README.md opens by saying this harness
exists to replace. This file is that sweep, made re-runnable, and generalized
to any of three axis families.

USAGE

    py Scripts/sim/sweep.py <scenario-name> --axis "<file>:<path>=<v1>,<v2>,..." [--axis ...] [--json]

Each --axis names one dial and a comma-separated list of values to try; the
tool runs the FULL CROSS PRODUCT across all --axis flags (N axes with
k1..kN values each => k1*k2*...*kN runs of the named scenario). Overrides
are applied to an IN-MEMORY COPY of the relevant docs/data/*.json (or the
scenario file itself) for the duration of one cell's run only — nothing is
ever written back to disk. See `_load_json_with_overrides` for the mechanism.

`<file>` selects which JSON structure the override path walks:

    entity-tiers   -> docs/data/entity-tiers.json          (family 1)
    unit-types     -> docs/data/unit-types.json             (family 1)
    upgrades       -> docs/data/upgrades.json                (family 1)
    constants      -> docs/data/scenarios/combat-model-constants.json
                       (family 3 -- DIAGNOSTIC ONLY, see THE GUARD below)
    scenario       -> the scenario file being run itself     (family 2)

Deliberately NOT offered: `economy` / `encounter-budget`. data_loader.py
never reads either at run time (economy.json's slice_targets and
encounter-budget.json's rank_arrival_timing[] are sources a HUMAN copies
numbers from when hand-authoring a scenario's Composition/ArrivalSeconds —
see scenarios.schema.md's own "never hand-typed... traces to
rank_arrival_timing[]" convention). Offering them as sweep axes would silently
do nothing to any run's output, which is worse than not offering them at all.
To sweep composition or arrival pacing, target `scenario:` directly (family 2).

`<path>` is a small path language, not a general JSONPath library (see
`resolve_and_set` below for the ~30-line implementation): dot-separated keys,
where any segment may carry a `[field=value]` filter to find one dict inside
a list by a field's value instead of an integer index (so paths survive a
data file's rows being reordered or having new rows inserted). Examples:

    entity-tiers:tiers[Name=brood_fodder].DPS=30,35,40,45
    unit-types:types.spearmen.combat.targets_per_hit=4,8,12
    upgrades:tier_ladder.tiers[id=militia].hp=110,130,150
    constants:wave_attrition_model.MaxAttackersPerUnit=1,2,4,8   (DIAGNOSTIC)
    scenario:Retinue.Composition[UnitType=spearmen].Count=32,40,48,56,64
    scenario:Enemy.Composition[Name=brood_fodder_rank0].ArrivalSeconds=3,5.85,8

THE THREE AXIS FAMILIES — what each is and is NOT good for

  1. GAME-BALANCE DATA (`entity-tiers` / `unit-types` / `upgrades`) — the
     gameplay director's committed numbers. Good for: finding where the
     scaling curve breaks or flattens (e.g. sweep an enemy tier's DPS/Armor
     and watch the survivor count's slope). This is READ-ONLY input in every
     other sense — a finding here is a report to the gameplay director
     (sim-director.md's own handoff rule), never a reason to edit the real
     data file from this tool.

  2. ENCOUNTER COMPOSITION (`scenario`) — wave sizes, tier mixes, per-row
     ArrivalSeconds, TimeLimitSeconds. Good for: "how many retinue does it
     take to stop the wipe at this population," "does staggering arrival
     change anything" (task-068 already answered that one for the GATE1
     fixture specifically — see docs/sim/VALIDATION.md). New ArrivalSeconds
     VALUES swept here that aren't already in encounter-budget.json's
     rank_arrival_timing[] are exploratory, not newly-cited data — say so in
     any handoff that uses them.

  3. HARNESS MODEL CONSTANTS (`constants`) — EngagedSpacingUU,
     MaxAttackersPerUnit, MeleeContactFacingFraction. DIAGNOSTIC ONLY. These
     three are the harness's own Fermi/measured-midpoint dials, not gameplay
     data — see combat-model-constants.json's own per-field notes for what's
     measured vs. estimated. Good for: sensitivity analysis (how much does
     the frontage model's OWN uncertainty band move the outcome). NOT good
     for, and structurally incapable of: producing a "recommended" or "best"
     value to adopt. See THE GUARD.

THE GUARD (this is a hard requirement, not a style choice — read
docs/sim/LIMITATIONS.md §1 first)

This tool NEVER ranks, sorts, or selects a "best" cell by proximity to any
target/measured value, for ANY axis family, anywhere in this file. That is a
deliberate, permanent omission: every cell's full result is printed/emitted
in sweep order (the literal itertools.product order of the --axis lists as
given on the command line) and NOTHING is computed FROM the resulting set of
rows except that unranked list itself — no argmin against GATE1's 109-111,
no "closest cell," no min/max/sort call over the results anywhere in this
module. `docs/sim/LIMITATIONS.md` §1 states plainly that some untested or
off-default combination of the family-3 constants could technically be found
that makes `validate.py` check 3 pass, and that finding one would be WORSE
than the current honest failure: it would trade a documented, cited-default
failure for a passing check with no citation behind the value that produced
it. Making "which cell is closest to 110" a flag away is exactly the fitting
shortcut this guard exists to prevent an agent in a hurry from reaching for,
so the capability does not exist in this file at all, for any family.

Enforcement detail: whenever ANY --axis in a run targets `constants`
(combat-model-constants.json's wave_attrition_model block), the tool prints
a mandatory DIAGNOSTIC banner before the table and tags every row's --json
output with `"diagnostic_family_3": true` plus the reason. This is detected
automatically from the override's target file, not from a separate
"--diagnostic" flag the caller could forget or the tool could be told to
skip — so it can't be silenced by mislabeling the sweep on the command line.
"""

from __future__ import annotations

import argparse
import dataclasses
import itertools
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import scenario_runner as sr

# ---------------------------------------------------------------------------
# The tiny path language: dot-separated keys, optional [field=value] filter
# per segment to select one dict out of a list.
# ---------------------------------------------------------------------------

_SEG_RE = re.compile(r'^([A-Za-z0-9_]+)(\[([A-Za-z0-9_]+)=([^\]]+)\])?$')


def _coerce_scalar(raw: str):
    """int, then float, then bool, then leave as string -- used for both
    filter values ([id=militia] vs [Count=60]) and override values."""
    if raw in ("true", "false"):
        return raw == "true"
    for conv in (int, float):
        try:
            return conv(raw)
        except ValueError:
            pass
    return raw


def _walk_segment(node, seg: str):
    m = _SEG_RE.match(seg)
    if not m:
        raise ValueError(f"Bad path segment {seg!r} — expected 'key' or 'key[field=value]'")
    key, _, filter_key, filter_raw = m.groups()
    node = node[key]
    if filter_key is not None:
        target = _coerce_scalar(filter_raw)
        for item in node:
            if item.get(filter_key) == target:
                return item
        raise KeyError(f"No element with {filter_key}={filter_raw!r} under {key!r}")
    return node


def resolve_and_set(root: dict, path: str, value) -> object:
    """Walks `path` (dot-separated, [field=value] filters allowed on any
    segment except the last) into `root` and sets the final segment's key to
    `value`. Returns the PRIOR value (useful for --json provenance, unused
    for anything that could rank cells). The last segment must be a plain
    key, never a filter — you set a scalar FIELD, not a list row."""
    segs = path.split(".")
    node = root
    for seg in segs[:-1]:
        node = _walk_segment(node, seg)
    last = segs[-1]
    m = _SEG_RE.match(last)
    if not m or m.group(3) is not None:
        raise ValueError(f"Terminal path segment must be a plain field name, got {last!r}")
    key = m.group(1)
    old = node.get(key)
    node[key] = value
    return old


# ---------------------------------------------------------------------------
# --axis parsing: "<file>:<path>=<v1>,<v2>,..."
# ---------------------------------------------------------------------------

FILE_MAP = {
    "entity-tiers": "entity-tiers.json",
    "unit-types": "unit-types.json",
    "upgrades": "upgrades.json",
    "constants": "combat-model-constants.json",
    # "scenario" is intentionally absent here -- its filename depends on
    # which scenario is being run, resolved in run_sweep() below, not fixed
    # up front like the other four.
}

FAMILY_OF = {
    "entity-tiers": "1-balance-data",
    "unit-types": "1-balance-data",
    "upgrades": "1-balance-data",
    "constants": "3-harness-model-constants (DIAGNOSTIC ONLY)",
    "scenario": "2-encounter-composition",
}


@dataclasses.dataclass
class Axis:
    file: str        # key into FILE_MAP, or "scenario"
    path: str
    values: list


def parse_axis(spec: str) -> Axis:
    if ":" not in spec or "=" not in spec:
        raise ValueError(f"--axis must look like 'file:path=v1,v2,...', got {spec!r}")
    file_part, rest = spec.split(":", 1)
    path, values_part = rest.rsplit("=", 1)
    file_part = file_part.strip()
    if file_part != "scenario" and file_part not in FILE_MAP:
        raise ValueError(f"Unknown axis file {file_part!r}. Known: scenario, {', '.join(FILE_MAP)}")
    values = [_coerce_scalar(v.strip()) for v in values_part.split(",")]
    return Axis(file=file_part, path=path.strip(), values=values)


# ---------------------------------------------------------------------------
# In-memory override plumbing — never writes docs/data/*.json back to disk.
# ---------------------------------------------------------------------------

def _make_patched_load_json(scenario_filename: str, cell_overrides: list[tuple[Axis, object]]):
    """
    Returns a drop-in replacement for data_loader._load_json that applies
    this cell's overrides to a freshly-loaded copy of whichever file is
    being read, then discards the copy. data_loader._load_json is called
    fresh (json.load from disk) on every invocation with no caching, so
    monkeypatching this one function for the duration of a single cell's
    `sr.run(name)` call is sufficient and self-contained — nothing persists
    between cells, and the actual files on disk are never touched.
    """
    original = dl._load_json

    def patched(path: Path):
        data = original(path)  # real disk read, unmodified file, every time
        for axis, value in cell_overrides:
            target_filename = scenario_filename if axis.file == "scenario" else FILE_MAP[axis.file]
            if path.name == target_filename:
                resolve_and_set(data, axis.path, value)
        return data

    return patched


def run_cell(scenario_name: str, cell_overrides: list[tuple[Axis, object]]) -> dict:
    original_load_json = dl._load_json
    dl._load_json = _make_patched_load_json(f"{scenario_name}.json", cell_overrides)
    try:
        return sr.run(scenario_name)
    finally:
        dl._load_json = original_load_json  # always restore, even on exception


# ---------------------------------------------------------------------------
# Sweep driver
# ---------------------------------------------------------------------------

def run_sweep(scenario_name: str, axes: list[Axis]) -> list[dict]:
    """
    Full cross product, sweep order == itertools.product order of the
    --axis value-lists exactly as given on the command line. Returns the
    list of per-cell result rows UNRANKED — see module docstring's GUARD
    section for why this function never sorts or filters that list before
    returning it.
    """
    rows = []
    value_lists = [axis.values for axis in axes]
    for combo in itertools.product(*value_lists):
        cell_overrides = list(zip(axes, combo))
        result = run_cell(scenario_name, cell_overrides)
        row = {
            "overrides": {f"{a.file}:{a.path}": v for a, v in cell_overrides},
            "result": result,
        }
        rows.append(row)
    return rows


def touches_family_3(axes: list[Axis]) -> bool:
    return any(a.file == "constants" for a in axes)


DIAGNOSTIC_BANNER = (
    "\n"
    "=================== DIAGNOSTIC ONLY (family 3) ===================\n"
    "This sweep touches combat-model-constants.json's wave_attrition_model\n"
    "dials (EngagedSpacingUU / MaxAttackersPerUnit / MeleeContactFacingFraction).\n"
    "These are the harness's OWN Fermi/measured-midpoint estimates, not\n"
    "gameplay-owned balance data. This output is sensitivity analysis, not a\n"
    "recommendation: no cell below is marked 'best', none is ranked by\n"
    "closeness to any measured target, and this tool has no feature to do so\n"
    "for this or any axis family, on purpose. See docs/sim/LIMITATIONS.md §1\n"
    "before citing any number from this section in a design conversation.\n"
    "====================================================================\n"
)


def print_cell(i: int, row: dict) -> None:
    result = row["result"]
    override_str = ", ".join(f"{k}={v}" for k, v in row["overrides"].items())
    print(f"\n--- cell {i}: {override_str} ---")
    if result["kind"] == "wave_attrition":
        print(f"  Retinue: {result['retinue_start']:.0f} -> {result['retinue_survivors']:.2f} survivors")
        print(f"  Enemy:   {result['enemy_start']:.0f} -> {result['enemy_survivors']:.2f} survivors")
        print(f"  Result: {result['result']}  (elapsed {result['elapsed_seconds']}s)")
    else:
        print(f"  Target: {result['target']} (MaxHP {result['target_max_hp']:.0f})")
        print(f"  TTK: {result['ttk_seconds']}s")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Committed cross-product parameter sweep over Scripts/sim/scenario_runner.py. "
                     "See this file's module docstring for the axis-path language and the anti-fitting guard.",
    )
    parser.add_argument("name", help="scenario file name (without .json) to sweep")
    parser.add_argument("--axis", action="append", default=[], required=True,
                         help="'file:path=v1,v2,...' — repeatable, cross-product across all given axes. "
                              "file is one of: scenario, entity-tiers, unit-types, upgrades, constants.")
    parser.add_argument("--json", action="store_true",
                         help="emit the full unrounded-past-combat_model.py cell list as JSON")
    args = parser.parse_args()

    axes = [parse_axis(spec) for spec in args.axis]
    diagnostic = touches_family_3(axes)

    rows = run_sweep(args.name, axes)

    if args.json:
        payload = {
            "scenario": args.name,
            "axes": [{"file": a.file, "path": a.path, "values": a.values, "family": FAMILY_OF[a.file]} for a in axes],
            "diagnostic_family_3": diagnostic,
            "diagnostic_note": DIAGNOSTIC_BANNER.strip() if diagnostic else None,
            "cells": [
                {
                    "overrides": row["overrides"],
                    "result": sr.to_json_safe(row["result"]),
                }
                for row in rows
            ],
        }
        print(json.dumps(payload, indent=2))
        return

    print(f"=== sweep: {args.name} ===")
    for a in axes:
        print(f"  axis [{FAMILY_OF[a.file]}] {a.file}:{a.path} = {a.values}")
    if diagnostic:
        print(DIAGNOSTIC_BANNER)
    for i, row in enumerate(rows):
        print_cell(i, row)
    print(f"\n{len(rows)} cells run. No 'best' cell is computed or printed — see this file's module "
          f"docstring, section THE GUARD.")


if __name__ == "__main__":
    main()
