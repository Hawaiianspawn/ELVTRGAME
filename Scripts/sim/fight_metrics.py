"""
fight_metrics.py -- reads Saved/SwarmTelemetry/fights.csv (task-107) and
reports, per group of runs, the median and spread of the exchange rate
("the one number balance lives or dies on" -- SwarmTelemetry.h), retinue
survivors, enemy survivors, duration, and time-to-first-blood, plus the
outcome mix (BroodCleared / RetinueWiped / HeroDown / Stalemate counts, not
a mean -- a wave that wipes 3-of-10 is a different problem from 10-of-10).

Grouping is two-level, both read straight off the CSV, nothing recomputed
or guessed:
  1. the 8 SwarmCombatTuning constants the recorder embeds in every row --
     runs from different balance passes never pool.
  2. (startRetinue, startBrood) within a tuning group -- the CSV has no
     stable "wave id", but a mop-up fight against 3 stragglers and a full
     250-brood wave are not the same run just because the constants match,
     and pooling them would make the median/outcome-mix numbers meaningless
     exactly the way the task's own scope notes warn against.
See docs/sim/FIGHT-METRICS.md for the full column reference and known gaps.

This file only reads fights.csv. It never modifies
ELVTR/Source/ELVTR/Mass/SwarmTelemetry.* -- a missing column is reported,
not added (task-107 hard rule).

Usage:
    py Scripts/sim/fight_metrics.py                 # default fights.csv
    py Scripts/sim/fight_metrics.py --file PATH.csv  # any other capture
    py Scripts/sim/fight_metrics.py --selftest       # the one runnable check
"""

from __future__ import annotations

import argparse
import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CSV = REPO_ROOT / "ELVTR" / "Saved" / "SwarmTelemetry" / "fights.csv"

# The tuning constants AppendSummaryRow() writes into every row (SwarmTelemetry.cpp).
TUNING_KEYS = [
    "retinueHP", "retinueDPS", "broodHP", "broodDPS",
    "heroHP", "heroDPS", "meleeRange", "maxAttackersPerUnit",
]
OUTCOME_ORDER = ["BroodCleared", "RetinueWiped", "HeroDown", "Stalemate", "InProgress"]

# FSwarmFightRecord::TimeToFirstBlood defaults to -1.0 and is only ever
# overwritten on a kill (SwarmTelemetry.h) -- "no kill this fight", not a
# real time. Must be excluded from the time-to-first-blood spread, not
# averaged in as if it were a very fast first blood.
NO_BLOOD = -1.0


def load_rows(path: Path) -> list[dict]:
    if not path.exists():
        raise FileNotFoundError(f"{path} does not exist")
    with path.open(newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise ValueError(f"{path} has no data rows")
    return rows


def tuning_key(row: dict) -> tuple:
    return tuple(row[k] for k in TUNING_KEYS)


def encounter_key(row: dict) -> tuple:
    return (row["startRetinue"], row["startBrood"])


def spread(values: list[float]) -> str:
    """Median + range. n=1 says so instead of printing a median that
    pretends to mean something (task-107 scope notes)."""
    if len(values) == 1:
        return f"{values[0]:.3f} (n=1)"
    return (f"{statistics.median(values):.3f} "
            f"[range {min(values):.3f}-{max(values):.3f}, n={len(values)}]")


def format_outcomes(outcomes: dict[str, int]) -> str:
    total = sum(outcomes.values())
    parts = [f"{name}={outcomes[name]}" for name in OUTCOME_ORDER if outcomes.get(name)]
    return f"{'/'.join(parts)} (of {total})"


def summarize(rows: list[dict]) -> dict:
    exchange = [float(r["exchangeRate"]) for r in rows]
    ret_surv = [float(r["endRetinue"]) for r in rows]
    enemy_surv = [float(r["endBrood"]) for r in rows]
    duration = [float(r["duration"]) for r in rows]
    tofb_all = [float(r["timeToFirstBlood"]) for r in rows]
    tofb = [t for t in tofb_all if t != NO_BLOOD]

    outcomes: dict[str, int] = defaultdict(int)
    for r in rows:
        outcomes[r["outcome"]] += 1

    return {
        "n": len(rows),
        "exchange_rate": spread(exchange),
        "retinue_survivors": spread(ret_surv),
        "enemy_survivors": spread(enemy_surv),
        "duration": spread(duration),
        "time_to_first_blood": spread(tofb) if tofb else "no fight in this group drew blood",
        "no_blood_fights": len(tofb_all) - len(tofb),
        "outcomes": dict(outcomes),
    }


def group_rows(rows: list[dict]) -> dict[tuple, dict[tuple, list[dict]]]:
    groups: dict[tuple, dict[tuple, list[dict]]] = defaultdict(lambda: defaultdict(list))
    for r in rows:
        groups[tuning_key(r)][encounter_key(r)].append(r)
    return groups


def print_report(rows: list[dict], out=sys.stdout) -> None:
    groups = group_rows(rows)
    print(f"{len(rows)} fight rows, {len(groups)} tuning group(s)\n", file=out)
    for tk, encounters in groups.items():
        print("=" * 78, file=out)
        print("tuning: " + ", ".join(f"{name}={val}" for name, val in zip(TUNING_KEYS, tk)), file=out)
        for ek, erows in sorted(encounters.items(), key=lambda kv: -len(kv[1])):
            s = summarize(erows)
            no_blood = f"  [{s['no_blood_fights']} fight(s) drew no blood]" if s["no_blood_fights"] else ""
            print(f"\n  encounter startRetinue={ek[0]} startBrood={ek[1]} (n={s['n']})", file=out)
            print(f"    exchange rate (brood/retinue): {s['exchange_rate']}", file=out)
            print(f"    retinue survivors:             {s['retinue_survivors']}", file=out)
            print(f"    enemy survivors:                {s['enemy_survivors']}", file=out)
            print(f"    duration (s):                   {s['duration']}", file=out)
            print(f"    time to first blood (s):        {s['time_to_first_blood']}{no_blood}", file=out)
            print(f"    outcome mix:                     {format_outcomes(s['outcomes'])}", file=out)
        print(file=out)


def _selftest() -> None:
    """The one runnable check: grouping separates tuning passes, n=1 groups
    say so, and the -1.0 sentinel never leaks into the first-blood spread."""

    def row(outcome, exch, endR, endB, dur, tofb, retHP, ek=("120", "250")):
        return {
            "outcome": outcome, "exchangeRate": str(exch),
            "endRetinue": str(endR), "endBrood": str(endB),
            "duration": str(dur), "timeToFirstBlood": str(tofb),
            "startRetinue": ek[0], "startBrood": ek[1],
            "retinueHP": retHP, "retinueDPS": "30", "broodHP": "60", "broodDPS": "14",
            "heroHP": "500", "heroDPS": "55", "meleeRange": "95", "maxAttackersPerUnit": "4",
        }

    rows = [
        row("BroodCleared", 16.0, 105, 0, 9.3, 0.67, retHP="130"),
        row("RetinueWiped", 1.0, 0, 3, 1.0, NO_BLOOD, retHP="130"),
        row("BroodCleared", 20.0, 108, 0, 8.9, 0.5, retHP="150"),   # different tuning pass
        row("Stalemate", 3.9, 4, 449, 21.7, 0.33, retHP="130", ek=("120", "450")),  # different encounter
    ]

    groups = group_rows(rows)
    assert len(groups) == 2, f"expected 2 tuning groups, got {len(groups)}"

    same_tuning = [g for g in groups if g[0] == "130"][0]
    encounters = groups[same_tuning]
    assert len(encounters) == 2, f"expected 2 encounter shapes within one tuning group, got {len(encounters)}"

    small_wave = encounters[("120", "250")]
    s = summarize(small_wave)
    assert s["n"] == 2
    assert "n=2" in s["exchange_rate"] and "range" in s["exchange_rate"]
    assert s["no_blood_fights"] == 1, "the RetinueWiped(-1) row must be excluded from time-to-first-blood"
    assert "n=1" in s["time_to_first_blood"], "the one real blood-time in this group is n=1, not a median"
    assert s["outcomes"] == {"BroodCleared": 1, "RetinueWiped": 1}

    solo = summarize([row("BroodCleared", 5.0, 1, 0, 1.0, 0.1, retHP="130")])
    assert "(n=1)" in solo["exchange_rate"] and "range" not in solo["exchange_rate"]

    print("fight_metrics selftest: OK")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Summarize Saved/SwarmTelemetry/fights.csv per tuning x encounter-shape group.")
    parser.add_argument("--file", type=Path, default=DEFAULT_CSV,
                         help="fights.csv path (default: ELVTR/Saved/SwarmTelemetry/fights.csv)")
    parser.add_argument("--selftest", action="store_true", help="run the self-check and exit")
    args = parser.parse_args()

    if args.selftest:
        _selftest()
        return 0

    try:
        rows = load_rows(args.file)
    except (FileNotFoundError, ValueError) as e:
        print(f"fight_metrics: {e}", file=sys.stderr)
        return 1

    print_report(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
