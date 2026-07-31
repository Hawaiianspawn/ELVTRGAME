"""
report.py — read run artifacts written by runstore.py: tabulate one, delta two
or more, and ALWAYS state a staleness verdict (task-075).

    py Scripts/sim/report.py <run-id-or-path> [<run-id-or-path> ...] [--json]

STALENESS IS THE POINT. A persisted number is only worth persisting if a
reader can tell whether it still describes the current data files. Every
artifact records a sha256 fingerprint of every docs/data file the run actually
read (runstore.py's `inputs_fingerprint`); this tool re-hashes those files as
they are on disk NOW and names any that changed. A survivor count computed
against an entity-tiers.json that has since been edited is reported as STALE,
never quietly presented as current, and a stale artifact makes this tool exit
non-zero so it is usable as a check.

`harness.dirty` and a `git_commit` that no longer matches HEAD are surfaced as
WARNings, not staleness: this repo's working tree carries uncommitted binary
assets more often than not, so treating dirty as a failure would make the
exit code meaningless. Changed INPUT DATA is the thing that invalidates a
number, and that is what flips the exit code.

THE GUARD (read docs/sim/LIMITATIONS.md §1 and sweep.py's own GUARD section
before touching the comparison code below)
This tool NEVER ranks, sorts, argmins, or selects a "best" or "closest" cell
against any target or measured value, for any axis family, anywhere. It shows
A-vs-B deltas as given, in the artifact's canonical cell order, and computes
nothing from the set of rows except that unranked list. Persisted artifacts
make "which stored cell is closest to GATE1's 110" a tempting one-liner —
LIMITATIONS.md §1 states that finding such a cell would be WORSE than the
current honest failure, because it trades a documented, cited-default failure
for a passing check with no citation behind the value that produced it. The
capability does not exist in this file, on purpose. Family-3 DIAGNOSTIC
status is read off the artifact (detected at capture time from the axis target
file), and its banner is printed here too.

FORWARD COMPATIBILITY: task-077 bumps `envelope_version`. A higher version is
reported — naming the keys this reader cannot interpret — and read
best-effort, never crashed on.

Plain stdlib only. No install step.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import runstore as rs


# ---------------------------------------------------------------------------
# Staleness
# ---------------------------------------------------------------------------

def staleness(artifact: dict) -> dict:
    changed, missing, unchanged = [], [], []
    for path, digest in (artifact.get("inputs_fingerprint") or {}).items():
        p = rs.REPO_ROOT / path
        if not p.exists():
            missing.append(path)
        elif rs.hash_file(p) != digest:
            changed.append({"path": path, "recorded": digest, "on_disk": rs.hash_file(p)})
        else:
            unchanged.append(path)
    head = rs.git_info()
    recorded_commit = (artifact.get("harness") or {}).get("git_commit")
    return {
        "changed": changed,
        "missing": missing,
        "unchanged_count": len(unchanged),
        "stale": bool(changed or missing),
        "recorded_commit": recorded_commit,
        "head_commit": head["git_commit"],
        "commit_mismatch": recorded_commit != head["git_commit"],
        "dirty_at_capture": bool((artifact.get("harness") or {}).get("dirty")),
    }


def print_staleness(artifact: dict, verdict: dict) -> None:
    print(f"\n--- staleness: {artifact.get('run_id')} ---")
    if verdict["stale"]:
        print("  STALE — this artifact's numbers were computed from data that has since changed.")
        for c in verdict["changed"]:
            print(f"    CHANGED  {c['path']}  (recorded {c['recorded']} -> on disk {c['on_disk']})")
        for m in verdict["missing"]:
            print(f"    MISSING  {m}  (input file no longer exists)")
    else:
        print(f"  CURRENT — all {verdict['unchanged_count']} recorded input files hash unchanged.")
    if verdict["commit_mismatch"]:
        print(f"  WARN: captured at commit {verdict['recorded_commit']}, HEAD is now "
              f"{verdict['head_commit']} (harness code may have changed; not staleness on its own).")
    if verdict["dirty_at_capture"]:
        print("  WARN: the working tree was dirty at capture time — the recorded commit does not "
              "fully describe the code that produced these numbers.")


# ---------------------------------------------------------------------------
# Envelope-version tolerance
# ---------------------------------------------------------------------------

def print_version_notice(artifact: dict) -> None:
    version = artifact.get("envelope_version")
    if not isinstance(version, int) or version <= rs.ENVELOPE_VERSION:
        return
    unknown = sorted(set(artifact) - rs.KNOWN_ENVELOPE_KEYS)
    print(f"\n  NOTE: envelope_version {version} is newer than this reader "
          f"(v{rs.ENVELOPE_VERSION}). Reading best-effort.")
    if unknown:
        print(f"        Top-level keys this reader cannot interpret: {', '.join(unknown)}")
    cell_keys = set()
    for cell in artifact.get("cells", []):
        cell_keys |= set(cell)
    unknown_cell = sorted(cell_keys - {"overrides", "trial", "seed", "result"})
    if unknown_cell:
        print(f"        Per-cell keys this reader cannot interpret: {', '.join(unknown_cell)}")


# ---------------------------------------------------------------------------
# Cell matching + tabulation
# ---------------------------------------------------------------------------

def cell_key(cell: dict) -> str:
    """Match key for A-vs-B: the overrides dict, canonicalised, plus `trial`.
    Sorting the override KEY NAMES makes the string canonical; nothing here
    orders anything by a result VALUE (see THE GUARD)."""
    overrides = json.dumps(cell.get("overrides", {}), sort_keys=True, separators=(",", ":"), default=str)
    return f"{overrides}|trial={cell.get('trial', 0)}"


def index_cells(artifact: dict) -> dict:
    return {cell_key(c): c for c in artifact.get("cells", [])}


def ordered_keys(indexed: list[dict]) -> list[str]:
    """Canonical cell order: the first artifact's order, then any key only
    later artifacts have, in THEIR order. Never reordered by result value."""
    order, seen = [], set()
    for cells in indexed:
        for key in cells:
            if key not in seen:
                seen.add(key)
                order.append(key)
    return order


def field_order(cells: list[dict]) -> list[str]:
    names, seen = [], set()
    for cell in cells:
        for name in rs.scalar_result_fields(cell.get("result", {})):
            if name not in seen:
                seen.add(name)
                names.append(name)
    return names


def tabulate_one(artifact: dict) -> None:
    print(f"\n=== {artifact.get('run_id')} — {artifact.get('cell_count')} cells ===")
    print(f"  question: {artifact.get('question')}")
    for i, cell in enumerate(artifact.get("cells", [])):
        result = cell.get("result", {})
        print(f"\n--- cell {i}: {rs.overrides_label(cell.get('overrides', {}))} ---")
        for name, value in rs.scalar_result_fields(result).items():
            print(f"  {name:>22}  {value}")


def delta_cells(artifacts: list[dict]) -> None:
    indexed = [index_cells(a) for a in artifacts]
    labels = [a.get("run_id", "?") for a in artifacts]
    base_label = labels[0]

    for key in ordered_keys(indexed):
        present = [cells.get(key) for cells in indexed]
        shown = next(c for c in present if c is not None)
        print(f"\n--- cell: {rs.overrides_label(shown.get('overrides', {}))} "
              f"(trial {shown.get('trial', 0)}) ---")

        absent = [labels[i] for i, c in enumerate(present) if c is None]
        if absent:
            print(f"  PRESENT IN SOME RUNS ONLY — absent from: {', '.join(absent)}")

        base = present[0]
        if base is None:
            print(f"  no baseline in {base_label}; showing raw values only, no delta computed.")

        names = field_order([c for c in present if c is not None])
        header = f"  {'field':>22}  {'base':>14}"
        for _ in labels[1:]:
            header += f"  {'value':>14}  {'delta':>12}"
        print(header + ("" if len(labels) == 1 else f"   (base = {base_label})"))

        for name in names:
            base_val = rs.scalar_result_fields(base.get("result", {})).get(name) if base else None
            row = f"  {name:>22}  {_fmt(base_val):>14}"
            for cells in indexed[1:]:
                other = cells.get(key)
                other_val = rs.scalar_result_fields(other.get("result", {})).get(name) if other else None
                row += f"  {_fmt(other_val):>14}  {_fmt_delta(base_val, other_val):>12}"
            print(row)


def _fmt(value) -> str:
    if value is None:
        return "-"
    if rs.is_number(value):
        return f"{float(value):.4g}"
    return str(value)


def _fmt_delta(a, b) -> str:
    if a is None or b is None:
        return "n/a"
    if rs.is_number(a) and rs.is_number(b):
        d = float(b) - float(a)
        return f"{d:+.4g}"
    return "same" if a == b else "CHANGED"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Tabulate one run artifact, delta two or more, and report staleness. "
                    "Never ranks or selects a 'best' cell — see this file's THE GUARD section.")
    parser.add_argument("refs", nargs="+", help="run-ids, published experiment names, or paths")
    parser.add_argument("--json", action="store_true", help="emit the staleness verdicts as JSON")
    args = parser.parse_args()

    artifacts = [rs.load_artifact(ref) for ref in args.refs]
    verdicts = [staleness(a) for a in artifacts]

    if args.json:
        print(json.dumps({
            "artifacts": [
                {"run_id": a.get("run_id"), "experiment": a.get("experiment"),
                 "envelope_version": a.get("envelope_version"),
                 "diagnostic_family_3": a.get("diagnostic_family_3"),
                 "staleness": v}
                for a, v in zip(artifacts, verdicts)
            ],
        }, indent=2))
        return 1 if any(v["stale"] for v in verdicts) else 0

    for artifact in artifacts:
        print_version_notice(artifact)

    if any(a.get("diagnostic_family_3") for a in artifacts):
        print(rs.sweep.DIAGNOSTIC_BANNER)
        for artifact in artifacts:
            if artifact.get("diagnostic_family_3"):
                print(f"  [{artifact.get('run_id')}] {artifact.get('diagnostic_reason')}")

    if len(artifacts) == 1:
        tabulate_one(artifacts[0])
    else:
        print("\n=== delta ===")
        for i, a in enumerate(artifacts):
            print(f"  [{i}] {a.get('run_id')}  experiment={a.get('experiment')}  "
                  f"scenario={a.get('scenario')}  created={a.get('created_utc')}")
        experiments = {a.get("experiment") for a in artifacts}
        if len(experiments) > 1:
            print(f"  NOTE: these artifacts come from different experiments ({', '.join(sorted(experiments))}) "
                  "— cells match only where their overrides are identical.")
        delta_cells(artifacts)

    for artifact, verdict in zip(artifacts, verdicts):
        print_staleness(artifact, verdict)

    if any(v["stale"] for v in verdicts):
        print("\nRESULT: STALE — at least one artifact was computed from data that has since "
              "changed on disk. Re-capture before citing these numbers.")
        return 1
    print("\nRESULT: CURRENT — every artifact's recorded inputs still hash unchanged.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
