"""
runstore.py — declarative experiments + a versioned, auditable run-artifact
store for Scripts/sim/ (task-075).

WHY THIS EXISTS

Every number this harness computes currently goes to stdout and dies with the
terminal. There is no way to declare an experiment once and re-run it, no
artifact to compare a new result against an old one, and no record of WHICH
data files a persisted number was computed from. That last part is the point:
docs/sim/README.md opens by explaining this whole harness exists to replace
throwaway scratch scripts, and a result pasted into a doc with no fingerprint
of its inputs is a throwaway scratch script with extra steps.

    py Scripts/sim/runstore.py capture <experiment-name> [--json] [--publish]
    py Scripts/sim/runstore.py list [--experiment <name>] [--limit N]
    py Scripts/sim/runstore.py show <run-id> [--json]

An EXPERIMENT is docs/data/experiments/<name>.json: one scenario, N sweep
axes, a stated question, source refs. See docs/data/experiments/experiments.schema.md
for the format and docs/sim/RUNSTORE.md for the envelope this writes.

SERIAL ON PURPOSE — DO NOT "OPTIMISE" THIS
`capture` runs the full cross product serially, in one process. No pool, no
threads, no --workers flag. That is task-077's job, and task-077's crossover
measurement needs this serial path as its clean reference implementation. If
you are here to make it faster, you are in the wrong file.

THE GUARD (carried over verbatim from sweep.py's own; read
docs/sim/LIMITATIONS.md §1 first)
Neither this file nor report.py ranks, sorts, argmins, or selects a "best" or
"closest" cell against any target or measured value, for any axis family,
anywhere. Cells are emitted in the literal itertools.product order of the
experiment's Axes list and nothing is computed FROM the resulting set of rows
except that unranked list. LIMITATIONS.md §1 states plainly that some
untested combination of the family-3 constants could technically be found
that makes validate.py check 3 pass, and that finding one would be WORSE than
the current honest failure. Persisting results makes "which stored cell is
closest to 110" a tempting one-liner, so the capability does not exist here,
for any family, on purpose. Family-3 DIAGNOSTIC status is detected from the
axis target file (sweep.touches_family_3), never from a caller-supplied flag.

Plain stdlib only (json/hashlib/subprocess/pathlib), same as the rest of the
harness — no install step.
"""

from __future__ import annotations

import argparse
import contextlib
import hashlib
import itertools
import json
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import scenario_runner as sr
import sweep

REPO_ROOT = dl.REPO_ROOT
EXPERIMENTS_DIR = REPO_ROOT / "docs" / "data" / "experiments"
RUNS_DIR = REPO_ROOT / "docs" / "sim" / "runs"
PUBLISHED_DIR = RUNS_DIR / "published"
INDEX_PATH = RUNS_DIR / "index.json"

ENVELOPE_VERSION = 1

# Envelope keys this version knows how to interpret. A reader that meets a
# HIGHER envelope_version reports what it can't interpret instead of crashing
# (task-077 bumps the version) — see report.py's unknown-key notice.
KNOWN_ENVELOPE_KEYS = frozenset({
    "envelope_version", "run_id", "experiment", "scenario", "question",
    "created_utc", "harness", "inputs_fingerprint", "invocation",
    "diagnostic_family_3", "diagnostic_reason", "cells",
    "wall_clock_seconds", "cell_count",
})

DIAGNOSTIC_REASON = (
    "At least one axis targets combat-model-constants.json's "
    "wave_attrition_model block (family 3) — the harness's own "
    "Fermi/measured-midpoint dials, not gameplay-owned balance data. This "
    "artifact is sensitivity analysis, not a recommendation. See "
    "docs/sim/LIMITATIONS.md §1."
)


# ---------------------------------------------------------------------------
# Small shared helpers
# ---------------------------------------------------------------------------

def repo_rel(path: Path) -> str:
    """Repo-relative, forward slashes, so a fingerprint written on Windows is
    readable (and re-hashable) anywhere."""
    return Path(path).resolve().relative_to(REPO_ROOT).as_posix()


def hash_file(path: Path) -> str:
    """First 12 hex of sha256 over the file's bytes. Truncated because this is
    a change-detector, not a security boundary — 48 bits is ample to notice a
    data file was edited between a run and a report."""
    h = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()[:12]


def git_info() -> dict:
    def git(*args) -> str:
        try:
            proc = subprocess.run(["git", *args], cwd=str(REPO_ROOT),
                                  capture_output=True, text=True, timeout=30)
        except (OSError, subprocess.SubprocessError):
            return ""
        return proc.stdout.strip() if proc.returncode == 0 else ""

    return {"git_commit": git("rev-parse", "--short", "HEAD") or "unknown",
            "dirty": bool(git("status", "--porcelain"))}


def atomic_write_json(path: Path, payload) -> None:
    """Write-temp-then-os.replace. A killed process must never leave a
    half-written artifact or a truncated index.json for task-078's watcher to
    trip over; os.replace is atomic within a filesystem, and the temp file is
    created in the destination directory so it always is one."""
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=str(path.parent), prefix=path.name + ".", suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump(payload, f, indent=2)
            f.write("\n")
        os.replace(tmp, path)
    except BaseException:
        Path(tmp).unlink(missing_ok=True)
        raise


def load_json_file(path: Path):
    with Path(path).open("r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Experiment files
# ---------------------------------------------------------------------------

def experiment_path(name: str) -> Path:
    return EXPERIMENTS_DIR / f"{name}.json"


def list_experiments() -> list[str]:
    if not EXPERIMENTS_DIR.exists():
        return []
    return sorted(p.stem for p in EXPERIMENTS_DIR.glob("*.json"))


def load_experiment(name: str) -> dict:
    path = experiment_path(name)
    if not path.exists():
        raise FileNotFoundError(
            f"No experiment file at {path}. Known: {', '.join(list_experiments()) or '(none)'}")
    exp = load_json_file(path)
    if exp.get("Name") != name:
        raise ValueError(f"{path.name}: 'Name' is {exp.get('Name')!r} but the filename says {name!r} "
                         "— they must match (experiments.schema.md).")
    if not exp.get("Scenario"):
        raise ValueError(f"{path.name}: 'Scenario' is required.")
    if not isinstance(exp.get("Axes", []), list):
        raise ValueError(f"{path.name}: 'Axes' must be a list of 'file:path=v1,v2,...' strings.")
    return exp


# ---------------------------------------------------------------------------
# Input fingerprinting — observed reads, never a hardcoded file list
# ---------------------------------------------------------------------------

@contextlib.contextmanager
def _recording_reads(seen: dict):
    """
    Records every docs/data path data_loader actually reads for the duration
    of a run, hashing each once. Same in-process wrap-and-restore pattern as
    sweep._make_patched_load_json, always restoring in a `finally`.

    Deliberately NOT a hardcoded list of the files we think get read: that
    would silently rot the moment data_loader starts reading a new one, and a
    fingerprint that misses an input is worse than no fingerprint at all.
    This nests correctly under sweep.run_cell's own patch — run_cell captures
    dl._load_json at call time, so its override-applying wrapper calls THIS
    recorder as its underlying read.
    """
    original = dl._load_json

    def recording(path: Path):
        data = original(path)
        key = repo_rel(path)
        if key not in seen:
            seen[key] = hash_file(path)
        return data

    dl._load_json = recording
    try:
        yield
    finally:
        dl._load_json = original


# ---------------------------------------------------------------------------
# Cell summaries — one place, shared with report.py so `show` and `report`
# never drift into two different readings of the same artifact.
# ---------------------------------------------------------------------------

def scalar_result_fields(result: dict) -> dict:
    """The flat numeric/string fields of one cell's result. Nested structures
    (wave-attrition's `log`, point-target's `breakdown`) are left out of
    tables and deltas — they're carried in the artifact in full, but a
    per-tick log is not a comparison unit."""
    return {k: v for k, v in result.items()
            if isinstance(v, (int, float, str, bool)) or v is None}


def is_number(v) -> bool:
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def summarize_cell(result: dict) -> str:
    kind = result.get("kind")
    if kind == "wave_attrition":
        return (f"retinue {result['retinue_start']:.0f}->{result['retinue_survivors']:.2f}  "
                f"enemy {result['enemy_start']:.0f}->{result['enemy_survivors']:.2f}  "
                f"{result['result']} @ {result['elapsed_seconds']}s")
    if kind == "point_target":
        return f"TTK {result['ttk_seconds']}s vs {result['target']}"
    return f"(unrecognised kind {kind!r})"


def overrides_label(overrides: dict) -> str:
    if not overrides:
        return "(no overrides — scenario as committed)"
    return ", ".join(f"{k}={v}" for k, v in overrides.items())


# ---------------------------------------------------------------------------
# capture
# ---------------------------------------------------------------------------

def capture(name: str, argv: list[str], publish: bool = False) -> dict:
    exp = load_experiment(name)

    trials = int(exp.get("Trials", 1) or 1)
    if trials > 1:
        raise SystemExit(
            f"{name}.json: Trials={trials}. Trials > 1 requires the batch driver — task-077, "
            "not yet wired. The seeded variance layer itself exists (task-076, "
            "scenario_runner.run_trial); what is missing is the driver that runs N of them "
            "and folds them into this envelope. Set Trials to 1 or omit it.")

    axes = [sweep.parse_axis(spec) for spec in exp.get("Axes", [])]
    diagnostic = sweep.touches_family_3(axes)

    seen: dict[str, str] = {}
    exp_path = experiment_path(name)
    seen[repo_rel(exp_path)] = hash_file(exp_path)

    cells = []
    started = time.perf_counter()
    with _recording_reads(seen):
        # Canonical order == itertools.product over the Axes lists exactly as
        # given. Two runs against unchanged data produce byte-identical
        # `cells`. task-077 parallelises this loop and must sort its results
        # back into this same order before writing the envelope.
        for combo in itertools.product(*[a.values for a in axes]):
            cell_overrides = list(zip(axes, combo))
            result = sweep.run_cell(exp["Scenario"], cell_overrides)
            cells.append({
                "overrides": {f"{a.file}:{a.path}": v for a, v in cell_overrides},
                "trial": 0,   # reserved — task-077 wires Trials
                "seed": None,  # reserved — task-077 wires Seed
                "result": sr.to_json_safe(result),
            })
    wall = time.perf_counter() - started

    stamp = time.strftime("%Y%m%d-%H%M%S", time.gmtime())
    digest = hashlib.sha256(json.dumps({"argv": argv, "experiment": name, "axes": exp.get("Axes", [])},
                                       sort_keys=True).encode("utf-8")).hexdigest()[:6]

    artifact = {
        "envelope_version": ENVELOPE_VERSION,
        "run_id": f"{stamp}-{name}-{digest}",
        "experiment": name,
        "scenario": exp["Scenario"],
        "question": exp.get("Question", ""),
        "created_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "harness": git_info(),
        "inputs_fingerprint": dict(sorted(seen.items())),
        "invocation": {"argv": argv, "workers": 1, "serial": True, "trigger": "cli"},
        "diagnostic_family_3": diagnostic,
        "diagnostic_reason": DIAGNOSTIC_REASON if diagnostic else None,
        "cells": cells,
        "wall_clock_seconds": round(wall, 4),
        "cell_count": len(cells),
    }

    atomic_write_json(RUNS_DIR / f"{artifact['run_id']}.json", artifact)
    _append_index(artifact)
    if publish:
        atomic_write_json(PUBLISHED_DIR / f"{name}.json", artifact)
    return artifact


def _append_index(artifact: dict) -> None:
    index = []
    if INDEX_PATH.exists():
        try:
            index = load_json_file(INDEX_PATH).get("runs", [])
        except (json.JSONDecodeError, OSError):
            index = []  # a corrupt index is rebuilt from this run forward, never crashes a capture
    index.append({
        "run_id": artifact["run_id"],
        "experiment": artifact["experiment"],
        "created_utc": artifact["created_utc"],
        "cell_count": artifact["cell_count"],
        "wall_clock_seconds": artifact["wall_clock_seconds"],
        "git_commit": artifact["harness"]["git_commit"],
        "dirty": artifact["harness"]["dirty"],
    })
    atomic_write_json(INDEX_PATH, {"runs": index})


# ---------------------------------------------------------------------------
# read side
# ---------------------------------------------------------------------------

def read_index() -> list[dict]:
    if not INDEX_PATH.exists():
        return []
    try:
        return load_json_file(INDEX_PATH).get("runs", [])
    except (json.JSONDecodeError, OSError):
        return []


def resolve_artifact_path(ref: str) -> Path:
    """A run-id, a published experiment name, or a plain path — in that
    order."""
    for candidate in (RUNS_DIR / f"{ref}.json", PUBLISHED_DIR / f"{ref}.json", Path(ref)):
        if candidate.exists() and candidate.is_file():
            return candidate
    raise FileNotFoundError(
        f"No run artifact for {ref!r}. Looked in {RUNS_DIR}, {PUBLISHED_DIR}, and as a plain path. "
        "`py Scripts/sim/runstore.py list` shows what exists locally.")


def load_artifact(ref: str) -> dict:
    return load_json_file(resolve_artifact_path(ref))


def print_header(artifact: dict) -> None:
    h = artifact.get("harness", {})
    print(f"run_id            {artifact.get('run_id')}")
    print(f"envelope_version  {artifact.get('envelope_version')}")
    print(f"experiment        {artifact.get('experiment')}")
    print(f"scenario          {artifact.get('scenario')}")
    print(f"question          {artifact.get('question')}")
    print(f"created_utc       {artifact.get('created_utc')}")
    print(f"harness           {h.get('git_commit')} (dirty={h.get('dirty')})")
    print(f"cells             {artifact.get('cell_count')} in {artifact.get('wall_clock_seconds')}s "
          f"(serial={artifact.get('invocation', {}).get('serial')}, "
          f"workers={artifact.get('invocation', {}).get('workers')})")
    print("inputs_fingerprint")
    for path, digest in artifact.get("inputs_fingerprint", {}).items():
        print(f"  {digest}  {path}")


def print_diagnostic_banner(artifact: dict) -> None:
    if artifact.get("diagnostic_family_3"):
        print(sweep.DIAGNOSTIC_BANNER)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def cmd_capture(args, argv) -> int:
    artifact = capture(args.experiment, argv, publish=args.publish)
    if args.json:
        print(json.dumps(artifact, indent=2))
        return 0
    print_header(artifact)
    print_diagnostic_banner(artifact)
    for i, cell in enumerate(artifact["cells"]):
        print(f"\n--- cell {i}: {overrides_label(cell['overrides'])} ---")
        print(f"  {summarize_cell(cell['result'])}")
    print(f"\nwrote {repo_rel(RUNS_DIR / (artifact['run_id'] + '.json'))}")
    if args.publish:
        print(f"published {repo_rel(PUBLISHED_DIR / (artifact['experiment'] + '.json'))} "
              "(committed record — publishing is a decision, not a default)")
    print(f"{artifact['cell_count']} cells. No 'best' cell is computed or stored — see this file's "
          "module docstring, section THE GUARD.")
    return 0


def cmd_list(args, argv) -> int:
    runs = [r for r in read_index()
            if args.experiment is None or r.get("experiment") == args.experiment]
    if args.limit:
        runs = runs[-args.limit:]  # most recent N, in capture order (not ranked)
    if not runs:
        print("No runs recorded. `py Scripts/sim/runstore.py capture <experiment>` writes one.")
        return 0
    print(f"{'run_id':>46}  {'experiment':>34}  {'cells':>5}  {'secs':>7}  {'commit':>9}  dirty")
    for r in runs:
        print(f"{r.get('run_id',''):>46}  {r.get('experiment',''):>34}  {r.get('cell_count',0):>5}  "
              f"{r.get('wall_clock_seconds',0):>7}  {r.get('git_commit',''):>9}  {r.get('dirty')}")
    return 0


def cmd_show(args, argv) -> int:
    artifact = load_artifact(args.run_id)
    if args.json:
        print(json.dumps(artifact, indent=2))
        return 0
    print_header(artifact)
    print_diagnostic_banner(artifact)
    for i, cell in enumerate(artifact.get("cells", [])):
        print(f"\n--- cell {i}: {overrides_label(cell.get('overrides', {}))} ---")
        print(f"  {summarize_cell(cell.get('result', {}))}")
    print("\nFor a full field table, a delta against another artifact, or a staleness verdict: "
          "py Scripts/sim/report.py <run-id> [<run-id> ...]")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run a committed docs/data/experiments/*.json experiment and persist a "
                    "versioned run artifact. See docs/sim/RUNSTORE.md.")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_cap = sub.add_parser("capture", help="run one experiment serially and write one artifact")
    p_cap.add_argument("experiment", help=f"experiment name (no .json). Known: "
                                          f"{', '.join(list_experiments()) or '(none)'}")
    p_cap.add_argument("--json", action="store_true", help="emit the full envelope as JSON")
    p_cap.add_argument("--publish", action="store_true",
                       help="ALSO write docs/sim/runs/published/<experiment>.json, which is "
                            "committed. A deliberate act, never a default.")
    p_cap.set_defaults(func=cmd_capture)

    p_list = sub.add_parser("list", help="list captured runs from docs/sim/runs/index.json")
    p_list.add_argument("--experiment", default=None)
    p_list.add_argument("--limit", type=int, default=None, help="show only the most recent N")
    p_list.set_defaults(func=cmd_list)

    p_show = sub.add_parser("show", help="print one artifact")
    p_show.add_argument("run_id", help="a run-id, a published experiment name, or a path")
    p_show.add_argument("--json", action="store_true")
    p_show.set_defaults(func=cmd_show)

    args = parser.parse_args()
    return args.func(args, sys.argv[1:])


if __name__ == "__main__":
    sys.exit(main())
