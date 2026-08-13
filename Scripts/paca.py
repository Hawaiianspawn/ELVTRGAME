#!/usr/bin/env python3
"""paca.py — the scheduling and approval gate over Paca.

Paca (http://localhost:8090, project Kindled) is the backlog. It holds the tasks,
the board, the statuses and the history; `docs/backlog/` is a frozen archive of the
work that closed before the move.

What Paca does not have is this project's two hard rules, so they live here:

  * **Locks.** Two simultaneously-active tasks may not write overlapping path globs
    (AGENT-TEAMS §3) or hold the same resource (§5). `validate` and `waves` are the
    single definition of independence — never eyeball it.
  * **The owner's verdict.** `approve` / `reject` / `park` / `done` are the owner's
    to give. They are privileged verbs behind the PreToolUse hook in
    .claude/settings.json, and that prompt is the signature.

Everything else — listing, showing, searching, epics, the board — is the Paca MCP
server or the web UI. Nothing is reimplemented here that Paca already does.

Config: PACA_API_URL / PACA_API_KEY, or Scripts/.paca.json (gitignored).
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CONFIG = Path(__file__).resolve().parent / ".paca.json"

# Paca status name -> the backlog vocabulary the skills and prose speak.
STATUS_FROM_PACA = {
    "Proposed": "proposed", "Approved": "approved", "In Progress": "in-progress",
    "Needs Review": "needs-review", "Done": "done", "Rejected": "rejected",
    "Parked": "parked",
}
STATUS_TO_PACA = {v: k for k, v in STATUS_FROM_PACA.items()}

ACTIVE = {"approved", "in-progress", "needs-review"}
CLOSED = {"done", "rejected", "parked"}
RESOURCES = {"unreal-editor", "pixellab-credits", "mcp-9000"}
PRIVILEGED = {"approve": "approved", "reject": "rejected",
              "park": "parked", "done": "done"}
DEFAULT_WIDTH = 4


# ---------------------------------------------------------------- transport


def config() -> tuple:
    url = os.environ.get("PACA_API_URL")
    key = os.environ.get("PACA_API_KEY")
    if not (url and key) and CONFIG.is_file():
        c = json.loads(CONFIG.read_text(encoding="utf-8"))
        url = url or c.get("api_url")
        key = key or c.get("api_key")
        project = c.get("project_id")
    else:
        project = os.environ.get("PACA_PROJECT_ID")
    if not (url and key and project):
        raise SystemExit(
            "paca.py: no credentials. Set PACA_API_URL / PACA_API_KEY / "
            f"PACA_PROJECT_ID, or write {CONFIG}.")
    return url.rstrip("/"), key, project


API_URL, API_KEY, PROJECT = "", "", ""


def api(method: str, path: str, body=None):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(
        API_URL + path, data=data, method=method,
        headers={"Content-Type": "application/json", "X-API-Key": API_KEY})
    try:
        with urllib.request.urlopen(req) as r:
            return json.loads(r.read()).get("data")
    except urllib.error.HTTPError as e:
        raise SystemExit(f"paca.py: {method} {path} -> {e.code}\n{e.read().decode()[:400]}")
    except urllib.error.URLError as e:
        raise SystemExit(f"paca.py: cannot reach {API_URL} — {e.reason}\n"
                         f"Is the stack up?  docker compose --env-file .env ps")


# ---------------------------------------------------------------- model


class Task:
    """One Paca task, wearing the field names the scheduler and the skills use."""

    def __init__(self, raw: dict, status_names: dict):
        self.raw = raw
        self.uuid = raw["id"]
        self.cf = raw.get("custom_fields") or {}
        self.status = STATUS_FROM_PACA.get(
            status_names.get(raw.get("status_id"), ""), "proposed")

    @property
    def id(self) -> int:
        m = re.search(r"(\d+)", str(self.cf.get("legacy_id") or ""))
        return int(m.group(1)) if m else 0

    @property
    def title(self) -> str:
        return self.raw.get("title") or ""

    @property
    def agent(self) -> str:
        return str(self.cf.get("agent") or "claude")

    @property
    def epic(self) -> str:
        tags = self.raw.get("tags") or []
        return tags[0] if tags else ""

    @property
    def owns(self) -> list:
        return [x for x in str(self.cf.get("owns") or "").splitlines() if x.strip()]

    @property
    def resources(self) -> list:
        v = self.cf.get("resources") or []
        return list(v) if isinstance(v, list) else [x.strip() for x in str(v).split(",") if x.strip()]

    @property
    def deps(self) -> list:
        return [int(m) for m in re.findall(r"\d+", str(self.cf.get("depends_on") or ""))]

    @property
    def teammate(self) -> str:
        return str(self.cf.get("teammate") or "")

    def num(self, key: str, default: int = 1) -> int:
        try:
            return int(self.cf.get(key, default))
        except (TypeError, ValueError):
            return default

    def total(self, unblocks: int) -> float:
        cost = max(1, self.num("cost", 1))
        return round((self.num("feel") * self.num("risk") * self.num("perf")
                      * unblocks) / cost, 2)


def load_all() -> list:
    names = {s["id"]: s["name"]
             for s in api("GET", f"/api/v1/projects/{PROJECT}/task-statuses")["items"]}
    types = {t["id"]: t["name"]
             for t in api("GET", f"/api/v1/projects/{PROJECT}/task-types")["items"]}
    out, cursor = [], ""
    while True:
        page = api("GET", f"/api/v1/projects/{PROJECT}/tasks?page_size=200{cursor}")
        for raw in page["items"]:
            if types.get(raw.get("task_type_id")) == "Epic":
                continue  # epics are containers, not schedulable work
            out.append(Task(raw, names))
        if not page.get("next_cursor"):
            break
        cursor = f"&cursor={page['next_cursor']}"
    return sorted(out, key=lambda t: t.id)


def status_ids() -> dict:
    names = api("GET", f"/api/v1/projects/{PROJECT}/task-statuses")["items"]
    return {s["name"]: s["id"] for s in names}


def unblock_counts(tasks: list) -> dict:
    """1 + the number of not-yet-closed tasks that name this one in depends_on."""
    counts = {t.id: 1 for t in tasks}
    for t in tasks:
        if t.status in CLOSED:
            continue
        for d in t.deps:
            if d in counts:
                counts[d] += 1
    return counts


# ---------------------------------------------------------------- independence


def glob_prefix(pattern: str) -> str:
    """Reduce a glob to the literal path prefix before its first wildcard."""
    p = pattern.replace("\\", "/").lstrip("./")
    for i, ch in enumerate(p):
        if ch in "*?[":
            return p[:i]
    return p


def overlaps(a: str, b: str) -> bool:
    pa, pb = glob_prefix(a), glob_prefix(b)
    return pa.startswith(pb) or pb.startswith(pa)


def pair_conflict(a: Task, b: Task) -> str:
    """Why these two may not run at the same time, or "" if they may.

    The single definition of independence in this project. `validate`, `waves` and
    `approve`'s dry run all ask here, so there is one answer rather than three that
    drift.
    """
    for ga in a.owns:
        for gb in b.owns:
            if overlaps(ga, gb):
                return f"overlapping paths ({ga!r} vs {gb!r})"
    shared = sorted(set(a.resources) & set(b.resources))
    if shared:
        return f"both hold the {shared[0]!r} lock"
    return ""


def find_conflicts(tasks: list) -> list:
    live = [t for t in tasks if t.status in ACTIVE]
    problems = []
    for i, a in enumerate(live):
        for b in live[i + 1:]:
            why = pair_conflict(a, b)
            if why:
                problems.append(
                    f"task-{a.id:03d} and task-{b.id:03d} are both active and {why}")
    return problems


def find_cycles(tasks: list) -> list:
    by_id = {t.id: t for t in tasks}
    problems, seen = [], set()
    for start in sorted(by_id):
        stack = [(start, [start])]
        while stack:
            node, path = stack.pop()
            for d in (by_id[node].deps if node in by_id else []):
                if d not in by_id:
                    continue
                if d == start:
                    ring = tuple(sorted(path))
                    if ring not in seen:
                        seen.add(ring)
                        chain = " -> ".join(f"task-{i:03d}" for i in path + [start])
                        problems.append(f"depends-on cycle: {chain}")
                elif d not in path and d > start:
                    stack.append((d, path + [d]))
    return problems


def plan_waves(tasks: list, candidates: list, max_width: int = DEFAULT_WIDTH) -> dict:
    """Partition candidates into waves that can run at the same time.

    Wave 1 is dispatchable now; wave 2 becomes dispatchable when wave 1 closes.
    Tasks already in flight seed wave 1 as occupants — not dispatched again, but
    their locks are real and a plan that ignored them is a plan `approve` refuses.
    """
    max_width = max(1, int(max_width))
    counts = unblock_counts(tasks)
    closed_ids = {t.id for t in tasks if t.status in CLOSED}
    in_flight = [t for t in tasks if t.status in {"in-progress", "needs-review"}]
    cand_ids = {t.id for t in candidates}
    flight_ids = {t.id for t in in_flight}
    by_id = {t.id: t for t in tasks}
    remaining = sorted(candidates, key=lambda t: (-t.total(counts.get(t.id, 1)), t.id))

    waves, reasons, landed = [], {}, set()
    occupants = [list(in_flight)] if in_flight else []

    while remaining:
        idx = len(waves)
        held = occupants[idx] if idx < len(occupants) else []
        wave, deferred = [], []
        for t in remaining:
            why = ""
            unmet = [d for d in t.deps if d not in closed_ids and d not in landed]
            if unmet:
                d = unmet[0]
                if d in cand_ids:
                    where = "in this batch"
                elif d in flight_ids:
                    where = f"in flight with {by_id[d].teammate or '?'}"
                elif d in by_id:
                    where = f"{by_id[d].status}, outside this batch"
                else:
                    where = "unknown task"
                why = f"waits on task-{d:03d} ({where})"
            if not why:
                for other in held + wave:
                    clash = pair_conflict(t, other)
                    if clash:
                        tag = "in flight" if other in in_flight else "this wave"
                        why = f"{clash} with task-{other.id:03d} ({tag})"
                        break
            if not why and len(wave) >= max_width:
                why = f"wave is full at {max_width} teammates"
            (deferred if why else wave).append(t)
            if why:
                reasons[t.id] = why
        if not wave:
            break
        waves.append(wave)
        landed |= {t.id for t in wave}
        remaining = deferred

    return {"waves": waves, "in_flight": in_flight, "stranded": remaining,
            "reasons": reasons, "width": max_width}


# ---------------------------------------------------------------- commands


def cmd_validate(args) -> int:
    tasks = load_all()
    by_id = {t.id: t for t in tasks}
    problems, warnings = [], []

    for t in tasks:
        if not t.id:
            problems.append(f"{t.uuid}: no legacy_id — the scheduler cannot name it")
        if t.status not in STATUS_TO_PACA:
            problems.append(f"task-{t.id:03d}: unknown status {t.status!r}")
        for r in t.resources:
            if r not in RESOURCES:
                warnings.append(f"task-{t.id:03d}: unknown resource {r!r}")
        for d in t.deps:
            if d not in by_id:
                problems.append(f"task-{t.id:03d}: depends on task-{d:03d}, which does not exist")
        if t.status in ACTIVE and not t.owns:
            warnings.append(f"task-{t.id:03d}: active with no owns: — it locks nothing")

    problems += find_conflicts(tasks)
    problems += find_cycles(tasks)

    for w in warnings:
        print(f"warn  {w}")
    for p in problems:
        print(f"FAIL  {p}")
    print(f"\n{len(tasks)} tasks · {len(problems)} problems · {len(warnings)} warnings")
    return 1 if problems else 0


def cmd_waves(args) -> int:
    tasks = load_all()
    counts = unblock_counts(tasks)
    if args.ids:
        want = {int(x) for x in re.findall(r"\d+", args.ids)}
        candidates = [t for t in tasks if t.id in want]
        missing = want - {t.id for t in candidates}
        if missing:
            raise SystemExit(f"no such task: {sorted(missing)}")
    elif args.approved_only:
        candidates = [t for t in tasks if t.status == "approved"]
    else:
        candidates = [t for t in tasks if t.status in {"proposed", "approved"}]

    plan = plan_waves(tasks, candidates, args.max_width)

    if plan["in_flight"]:
        print("in flight — wave 1 is already partly occupied")
        for t in plan["in_flight"]:
            print(f"  task-{t.id:03d}  {t.teammate or '?':<16} {t.title[:56]}")
        print()

    for i, wave in enumerate(plan["waves"], 1):
        when = "dispatchable now" if i == 1 else f"after wave {i - 1} closes"
        print(f"wave {i} — {len(wave)} teammate(s), {when}")
        for t in wave:
            score = t.total(counts.get(t.id, 1))
            print(f"  task-{t.id:03d}  {score:>5}  {t.agent:<21} {t.title[:48]}")
            if t.owns:
                print(f"            owns {', '.join(t.owns)}")
            if t.resources:
                print(f"            needs {', '.join(t.resources)}")
        print()

    deferred = [t for w in plan["waves"][1:] for t in w] + plan["stranded"]
    if deferred:
        print("why these missed wave 1")
        for t in deferred:
            print(f"  task-{t.id:03d}  {plan['reasons'].get(t.id, '—')}")
    if plan["stranded"]:
        print("\nstranded — blocked by something outside this batch:")
        for t in plan["stranded"]:
            print(f"  task-{t.id:03d}  {plan['reasons'].get(t.id, '—')}")
    return 0


def cmd_next_id(args) -> int:
    print(f"{max([t.id for t in load_all()] or [0]) + 1:03d}")
    return 0


def resolve(tasks: list, ids: str) -> list:
    want = [int(x) for x in re.findall(r"\d+", ids)]
    by_id = {t.id: t for t in tasks}
    missing = [i for i in want if i not in by_id]
    if missing:
        raise SystemExit(f"no such task: {missing}")
    return [by_id[i] for i in want]


def patch_fields(t: Task, fields: dict) -> None:
    api("PATCH", f"/api/v1/projects/{PROJECT}/tasks/{t.uuid}",
        {"custom_fields": {**t.cf, **fields}})


def transition(args, verb: str) -> int:
    """The owner's verdict. Reached only through the hook prompt in settings.json."""
    new = PRIVILEGED[verb]
    tasks = load_all()
    targets = resolve(tasks, args.ids)
    sids = status_ids()

    if new == "approved":
        # Dry-run the lock table before granting permission, the way approve always has.
        would_be_active = [t for t in tasks
                           if t.status in ACTIVE or t in targets]
        clashes = []
        for i, a in enumerate(would_be_active):
            for b in would_be_active[i + 1:]:
                why = pair_conflict(a, b)
                if why:
                    clashes.append(f"task-{a.id:03d} and task-{b.id:03d} {why}")
        if clashes:
            print("refused — approving these would put conflicting tasks in flight:")
            for c in clashes:
                print(f"  {c}")
            print("\nNarrow an owns: glob, or add a depends-on and re-run.")
            return 1

    for t in targets:
        api("PATCH", f"/api/v1/projects/{PROJECT}/tasks/{t.uuid}",
            {"status_id": sids[STATUS_TO_PACA[new]]})
        patch_fields(t, {"decided": args.reason or verb})
        print(f"task-{t.id:03d}  {t.status} -> {new}   {t.title[:52]}")
    print(f"\n{len(targets)} task(s) {new}. History is in Paca's activity log.")
    return 0


def cmd_list(args) -> int:
    tasks = load_all()
    counts = unblock_counts(tasks)
    if args.status:
        want = {s.strip() for s in args.status.split(",")}
        tasks = [t for t in tasks if t.status in want]
    if args.epic:
        tasks = [t for t in tasks if t.epic == args.epic]
    tasks.sort(key=lambda t: (-t.total(counts.get(t.id, 1)), t.id))
    for t in tasks:
        print(f"task-{t.id:03d}  {t.total(counts.get(t.id, 1)):>5}  "
              f"{t.status:<12} {t.agent:<21} {t.title[:52]}")
        if t.teammate:
            print(f"            held by {t.teammate}")
    print(f"\n{len(tasks)} task(s)")
    return 0


def cmd_review(args) -> int:
    """Agent-settable: work is done and waiting on the owner's eye. Not a verdict."""
    tasks = load_all()
    sid = status_ids()["Needs Review"]
    for t in resolve(tasks, args.ids):
        api("PATCH", f"/api/v1/projects/{PROJECT}/tasks/{t.uuid}", {"status_id": sid})
        print(f"task-{t.id:03d}  {t.status} -> needs-review")
    return 0


def cmd_new(args) -> int:
    """File a task from the JSON /host drafts. Always lands at `proposed`.

    Shape (the frontmatter fields, as JSON):
      {"title": ..., "agent": ..., "owns": [...], "resources": [...],
       "depends-on": [8, 25], "epic": "...", "evidence": "...",
       "score": {"feel": 2, "risk": 1, "perf": 1, "cost": 2},
       "source": "...", "body": "## Why now\\n..."}
    """
    spec = json.loads(Path(args.json).read_text(encoding="utf-8"))
    tasks = load_all()
    new_id = max([t.id for t in tasks] or [0]) + 1
    score = spec.get("score") or {}

    for r in spec.get("resources") or []:
        if r not in RESOURCES:
            raise SystemExit(f"unknown resource {r!r} — one of {sorted(RESOURCES)}")

    types = {t["name"]: t["id"]
             for t in api("GET", f"/api/v1/projects/{PROJECT}/task-types")["items"]}
    body = {
        "title": spec["title"],
        "description": [{"id": "1", "type": "paragraph",
                         "props": {"textColor": "default", "backgroundColor": "default",
                                   "textAlignment": "left"},
                         "content": [{"type": "text", "text": spec.get("body", ""),
                                      "styles": {}}], "children": []}],
        "task_type_id": types["Task"],
        "status_id": status_ids()["Proposed"],
        "tags": [x for x in [spec.get("epic", ""), spec.get("agent", "")] if x],
        "custom_fields": {
            "legacy_id": f"task-{new_id:03d}",
            "agent": spec.get("agent", "claude"),
            "owns": "\n".join(spec.get("owns") or []),
            "resources": spec.get("resources") or [],
            "feel": int(score.get("feel", 1)), "risk": int(score.get("risk", 1)),
            "perf": int(score.get("perf", 1)), "cost": int(score.get("cost", 1)),
            "evidence": spec.get("evidence", ""),
            "depends_on": ", ".join(f"task-{int(d):03d}"
                                    for d in (spec.get("depends-on") or [])),
            "source": spec.get("source", ""), "model": "", "teammate": "", "decided": "",
        },
    }
    created = api("POST", f"/api/v1/projects/{PROJECT}/tasks", body)
    print(f"task-{new_id:03d}  proposed   {spec['title']}")
    print(f"  {API_URL}  ·  {created['id']}")
    print("\nRun `py Scripts/paca.py validate` before presenting it.")
    return 0


def sweep_report() -> dict:
    """Raw ingest surface. The skill decides what is real work; this only finds it.

    Storage-independent by nature — it reads the repo, not the backlog — so it came
    across from backlog.py unchanged except for where `existing_sources` is read from.
    """
    rep = {"unchecked_boxes": [], "gdd_open_questions": [], "pending_briefs": [],
           "source_markers": [], "existing_sources": []}
    archive = REPO / "docs" / "backlog"

    doc_files = sorted(REPO.joinpath("docs").rglob("*.md")) + sorted(REPO.glob("*.md"))
    for p in doc_files:
        if archive in p.parents:
            continue
        try:
            lines = p.read_text(encoding="utf-8").splitlines()
        except (UnicodeDecodeError, OSError):
            continue
        for i, line in enumerate(lines, 1):
            if re.match(r"^\s*[-*]\s*\[ \]\s+", line):
                rep["unchecked_boxes"].append({
                    "source": f"{p.relative_to(REPO).as_posix()}:{i}",
                    "text": line.strip()[5:].strip()})

    gdd = REPO / "GDD.md"
    if gdd.exists():
        for i, line in enumerate(gdd.read_text(encoding="utf-8").splitlines(), 1):
            if not line.startswith("|"):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) >= 4 and cells[0].isdigit() and "✅" not in cells[3]:
                rep["gdd_open_questions"].append({
                    "source": f"GDD.md:{i}", "q": cells[0], "question": cells[1],
                    "lean": cells[2], "status": cells[3]})

    for p in sorted(REPO.joinpath("docs", "briefs").glob("brief-*.md")):
        text = p.read_text(encoding="utf-8")
        m = re.search(r"(?m)^status:\s*(\S+)", text)
        if m and m.group(1) in {"pending", "in-progress", "blocked"}:
            title = re.search(r"(?m)^title:\s*(.+)$", text)
            rep["pending_briefs"].append({
                "source": p.relative_to(REPO).as_posix(), "status": m.group(1),
                "title": title.group(1).strip() if title else p.stem})

    src = REPO / "ELVTR" / "Source"
    if src.is_dir():
        for p in sorted(src.rglob("*")):
            if p.suffix.lower() not in {".cpp", ".h", ".cs"}:
                continue
            try:
                lines = p.read_text(encoding="utf-8", errors="replace").splitlines()
            except OSError:
                continue
            for i, line in enumerate(lines, 1):
                if re.search(r"\b(TODO|FIXME|HACK|XXX)\b", line):
                    rep["source_markers"].append({
                        "source": f"{p.relative_to(REPO).as_posix()}:{i}",
                        "text": line.strip()})

    for t in load_all():
        rep["existing_sources"].append({
            "id": t.id, "status": t.status,
            "source": str(t.cf.get("source", "")), "title": t.title})
    return rep


def cmd_sweep(args) -> int:
    rep = sweep_report()
    if args.json:
        print(json.dumps(rep, indent=2))
        return 0
    print(f"unchecked boxes    {len(rep['unchecked_boxes'])}")
    print(f"open GDD questions {len(rep['gdd_open_questions'])}")
    print(f"pending briefs     {len(rep['pending_briefs'])}")
    print(f"source markers     {len(rep['source_markers'])}")
    print(f"already filed      {len(rep['existing_sources'])}")
    print("\nRe-run with --json for the full surface.")
    return 0


def cmd_dispatch(args) -> int:
    tasks = load_all()
    by_id = {t.id: t for t in tasks}
    t = by_id.get(int(args.id))
    if not t:
        raise SystemExit(f"no such task: {args.id}")
    if t.status != "approved":
        raise SystemExit(f"task-{t.id:03d} is {t.status}, not approved. "
                         f"Dispatch is only for approved work.")

    open_deps = [d for d in t.deps if d in by_id and by_id[d].status not in CLOSED]
    if open_deps:
        raise SystemExit("refused — depends on still-open " +
                         ", ".join(f"task-{d:03d}" for d in open_deps))

    for other in tasks:
        if other.uuid == t.uuid or other.status not in {"in-progress", "needs-review"}:
            continue
        why = pair_conflict(t, other)
        if why:
            raise SystemExit(f"refused — {why} with task-{other.id:03d}, in flight "
                             f"with {other.teammate or '?'}")

    if not re.match(r"\A[A-Za-z0-9][A-Za-z0-9_-]{0,63}\Z", args.teammate):
        raise SystemExit(f"teammate name {args.teammate!r} is not spawnable")
    taken = [o for o in tasks if o.teammate == args.teammate
             and o.status in {"in-progress", "needs-review"}]
    if taken:
        raise SystemExit(f"teammate {args.teammate!r} already holds task-{taken[0].id:03d}")

    api("PATCH", f"/api/v1/projects/{PROJECT}/tasks/{t.uuid}",
        {"status_id": status_ids()["In Progress"]})
    patch_fields(t, {"teammate": args.teammate, "model": args.model or ""})
    print(f"task-{t.id:03d} -> in-progress, held by {args.teammate}"
          f"{' at ' + args.model if args.model else ''}")
    print(f"  {t.title}")
    print("\nNow spawn the teammate with the task's spawn prompt, at this exact name.")
    return 0


def main() -> int:
    global API_URL, API_KEY, PROJECT
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("validate", help="lock collisions, cycles, dangling deps"
                   ).set_defaults(fn=cmd_validate)
    sub.add_parser("next-id", help="the next legacy task id").set_defaults(fn=cmd_next_id)

    p = sub.add_parser("waves", help="what can run in parallel, and what waits")
    p.add_argument("ids", nargs="?", default="")
    p.add_argument("--approved-only", action="store_true")
    p.add_argument("--max-width", type=int, default=DEFAULT_WIDTH)
    p.set_defaults(fn=cmd_waves)

    p = sub.add_parser("list", help="tasks, ranked by score")
    p.add_argument("--status", default="")
    p.add_argument("--epic", default="")
    p.set_defaults(fn=cmd_list)

    p = sub.add_parser("review", help="mark work as waiting on the owner's eye")
    p.add_argument("ids")
    p.set_defaults(fn=cmd_review)

    p = sub.add_parser("sweep-report", help="undone work latent in the repo")
    p.add_argument("--json", action="store_true")
    p.set_defaults(fn=cmd_sweep)

    p = sub.add_parser("new", help="file a task from a drafted JSON spec")
    p.add_argument("json", help="path to the spec")
    p.set_defaults(fn=cmd_new)

    p = sub.add_parser("dispatch", help="hand an approved task to a teammate")
    p.add_argument("id")
    p.add_argument("--teammate", required=True)
    p.add_argument("--model", default="")
    p.set_defaults(fn=cmd_dispatch)

    for verb in PRIVILEGED:
        p = sub.add_parser(verb, help=f"OWNER VERDICT — mark {PRIVILEGED[verb]}")
        p.add_argument("ids")
        p.add_argument("--reason", default="")
        p.set_defaults(fn=lambda a, v=verb: transition(a, v))

    args = ap.parse_args()
    API_URL, API_KEY, PROJECT = config()
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
