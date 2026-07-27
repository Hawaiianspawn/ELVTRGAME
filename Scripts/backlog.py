#!/usr/bin/env python3
"""backlog.py — the deterministic half of the Emberkeep host.

The `/backlog` and `/host` skills do the judgment (what is work, which director,
why now). This script does everything mechanical, so two runs never disagree:

    py Scripts/backlog.py validate        # schema, dup ids, dangling deps, lock conflicts
    py Scripts/backlog.py next-id         # next free 3-digit id
    py Scripts/backlog.py reindex         # regenerate docs/backlog/INDEX.md
    py Scripts/backlog.py list --top 7    # the audit queue, ranked
    py Scripts/backlog.py show 3          # one task, resolved score
    py Scripts/backlog.py sweep-report    # raw ingest surface as JSON

Status transitions:

    py Scripts/backlog.py start 3            # -> in-progress   (agents may run this)
    py Scripts/backlog.py review 3           # -> needs-review  (agents may run this)
    py Scripts/backlog.py approve 3,5        # -> approved      | PRIVILEGED
    py Scripts/backlog.py done 3             # -> done          | PRIVILEGED
    py Scripts/backlog.py reject 4 -r "..."  # -> rejected      | PRIVILEGED
    py Scripts/backlog.py park 4 -r "..."    # -> parked        | PRIVILEGED

Dispatch — `start` with the teammate recorded, for the /host flow:

    py Scripts/backlog.py dispatch 3 --teammate flame-flicker

It refuses unless the task is already `approved` with every dependency closed, so
"spawn a teammate on unapproved work" is not reachable by accident. It is not itself
privileged: the owner's decision already happened at `approve`.

PRIVILEGED transitions are the owner's call. They are unreachable through file
editing (Scripts/backlog_guard.py denies it) and this command is deliberately
kept out of .claude/settings.local.json so it always raises a permission prompt.
Every transition appends to docs/backlog/LOG.md.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import date
from pathlib import Path

import yaml

REPO = Path(__file__).resolve().parent.parent
BACKLOG = REPO / "docs" / "backlog"
INDEX = BACKLOG / "INDEX.md"
LOG = BACKLOG / "LOG.md"

STATUSES = [
    "proposed",
    "approved",
    "in-progress",
    "needs-review",
    "done",
    "rejected",
    "parked",
]
PRIVILEGED = {"approved", "done", "rejected", "parked"}
# Statuses that hold a lock on their `owns:` globs and `resources:`.
ACTIVE = {"approved", "in-progress", "needs-review"}
# Statuses that are finished and no longer compete for anything.
CLOSED = {"done", "rejected", "parked"}

AGENTS = {
    "gameplay-director",
    "narrative-director",
    "performance-director",
    "pixel-art-director",
    "ui-director",
    "claude",
}
RESOURCES = {"unreal-editor", "pixellab-credits", "mcp-9000"}

REQUIRED = ["id", "title", "status", "agent", "owns", "resources",
            "depends-on", "evidence", "score", "source"]
SCORE_KEYS = ["gate", "risk", "cost"]

FM = re.compile(r"\A---\r?\n(.*?)\r?\n---\r?\n", re.DOTALL)
# Mirrors the Agent tool's own `name` constraint, so a name recorded here is always
# one the lead can actually spawn and address with SendMessage.
TEAMMATE_NAME = re.compile(r"\A[A-Za-z0-9][A-Za-z0-9_-]{0,63}\Z")


# ---------------------------------------------------------------- model


class Task:
    def __init__(self, path: Path, meta: dict, body: str, raw: str):
        self.path = path
        self.meta = meta
        self.body = body
        self.raw = raw

    @property
    def id(self) -> int:
        # Read the id straight from the raw frontmatter, never from the YAML load.
        # PyYAML is YAML 1.1, where a leading-zero integer is OCTAL: `id: 010` would
        # silently become 8 and collide with task-008.
        m = re.search(r"(?m)^id:\s*[\"']?(\d+)", self.raw)
        return int(m.group(1)) if m else 0

    @property
    def status(self) -> str:
        return str(self.meta.get("status", "")).strip()

    @property
    def title(self) -> str:
        return str(self.meta.get("title", "")).strip()

    @property
    def agent(self) -> str:
        return str(self.meta.get("agent", "")).strip()

    def listy(self, key: str) -> list:
        v = self.meta.get(key) or []
        if isinstance(v, str):
            v = [x.strip() for x in v.split(",") if x.strip()]
        return list(v)

    @property
    def owns(self) -> list:
        return [str(x) for x in self.listy("owns")]

    @property
    def resources(self) -> list:
        return [str(x) for x in self.listy("resources")]

    @property
    def deps(self) -> list:
        out = []
        for x in self.listy("depends-on"):
            try:
                out.append(int(x))
            except (TypeError, ValueError):
                pass
        return out

    def score_input(self, key: str, default: int = 1) -> int:
        s = self.meta.get("score") or {}
        if not isinstance(s, dict):
            return default
        try:
            return int(s.get(key, default))
        except (TypeError, ValueError):
            return default

    def total(self, unblocks: int) -> float:
        cost = max(1, self.score_input("cost", 1))
        gate = self.score_input("gate", 1)
        risk = self.score_input("risk", 1)
        return round((gate * risk * unblocks) / cost, 2)


def load_all() -> list:
    tasks = []
    if not BACKLOG.is_dir():
        return tasks
    for p in sorted(BACKLOG.glob("task-*.md")):
        raw = p.read_text(encoding="utf-8")
        m = FM.match(raw)
        if not m:
            raise SystemExit(f"{p.name}: no YAML frontmatter block")
        try:
            meta = yaml.safe_load(m.group(1)) or {}
        except yaml.YAMLError as e:
            raise SystemExit(f"{p.name}: unparseable frontmatter — {e}")
        if not isinstance(meta, dict):
            raise SystemExit(f"{p.name}: frontmatter is not a mapping")
        tasks.append(Task(p, meta, raw[m.end():], raw))
    return tasks


def unblock_counts(tasks: list) -> dict:
    """1 + the number of not-yet-closed tasks that name this one in depends-on."""
    counts = {t.id: 1 for t in tasks}
    for t in tasks:
        if t.status in CLOSED:
            continue
        for d in t.deps:
            if d in counts:
                counts[d] += 1
    return counts


# ---------------------------------------------------------------- locks


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


def find_conflicts(tasks: list) -> list:
    """File-ownership and resource collisions between simultaneously active tasks.

    Mechanises AGENT-TEAMS.md §3 (disjoint files per teammate) and §5 (one editor).
    """
    live = [t for t in tasks if t.status in ACTIVE]
    problems = []
    for i, a in enumerate(live):
        for b in live[i + 1:]:
            for ga in a.owns:
                for gb in b.owns:
                    if overlaps(ga, gb):
                        problems.append(
                            f"task-{a.id:03d} and task-{b.id:03d} are both active and both "
                            f"claim overlapping paths ({ga!r} vs {gb!r})"
                        )
            for r in set(a.resources) & set(b.resources):
                problems.append(
                    f"task-{a.id:03d} and task-{b.id:03d} are both active and both "
                    f"hold the {r!r} lock"
                )
    return problems


# ---------------------------------------------------------------- validate


def cmd_validate(args) -> int:
    tasks = load_all()
    errors, warnings = [], []
    seen = {}

    for t in tasks:
        n = t.path.name
        for key in REQUIRED:
            if key not in t.meta:
                errors.append(f"{n}: missing required key {key!r}")
        if t.status and t.status not in STATUSES:
            errors.append(f"{n}: status {t.status!r} not one of {'|'.join(STATUSES)}")
        if t.agent and t.agent not in AGENTS:
            errors.append(f"{n}: agent {t.agent!r} is not a known agent type")
        for r in t.resources:
            if r not in RESOURCES:
                errors.append(f"{n}: unknown resource {r!r}")
        for key in SCORE_KEYS:
            v = t.meta.get("score")
            if not isinstance(v, dict) or key not in v:
                errors.append(f"{n}: score.{key} missing")
        if not str(t.meta.get("evidence", "")).strip():
            errors.append(f"{n}: evidence is empty — a task that cannot name its "
                          f"evidence is not ready to be approved")
        expect = f"task-{t.id:03d}-"
        if not n.startswith(expect):
            errors.append(f"{n}: filename does not match id {t.id} (expected {expect}*)")
        if t.id in seen:
            errors.append(f"{n}: duplicate id {t.id} (also {seen[t.id]})")
        else:
            seen[t.id] = n

    ids = set(seen)
    for t in tasks:
        for d in t.deps:
            if d not in ids:
                errors.append(f"{t.path.name}: depends-on {d} does not exist")
        if t.status in ACTIVE:
            for d in t.deps:
                dep = next((x for x in tasks if x.id == d), None)
                if dep and dep.status not in CLOSED:
                    warnings.append(
                        f"task-{t.id:03d} is {t.status} but depends on task-{d:03d} "
                        f"which is still {dep.status}"
                    )

    errors.extend(find_conflicts(tasks))

    for w in warnings:
        print(f"warn:  {w}")
    for e in errors:
        print(f"ERROR: {e}")
    print(f"\n{len(tasks)} task(s), {len(errors)} error(s), {len(warnings)} warning(s)")
    return 1 if errors else 0


# ---------------------------------------------------------------- read cmds


def cmd_next_id(args) -> int:
    tasks = load_all()
    print(f"{(max((t.id for t in tasks), default=0) + 1):03d}")
    return 0


def ranked(tasks: list) -> list:
    counts = unblock_counts(tasks)
    rows = [(t.total(counts[t.id]), counts[t.id], t) for t in tasks]
    rows.sort(key=lambda r: (-r[0], r[2].id))
    return rows


def cmd_list(args) -> int:
    tasks = load_all()
    if args.status:
        want = set(args.status.split(","))
        tasks = [t for t in tasks if t.status in want]
    rows = ranked(tasks)
    if args.top:
        rows = rows[: args.top]
    if not rows:
        print("(no matching tasks)")
        return 0
    print(f"{'id':>4}  {'score':>5}  {'g':>1} {'r':>1} {'u':>1} {'c':>1}  "
          f"{'status':<12} {'agent':<20} title")
    print("-" * 100)
    for total, unb, t in rows:
        print(f"{t.id:>4}  {total:>5}  "
              f"{t.score_input('gate')} {t.score_input('risk')} {unb} "
              f"{t.score_input('cost')}  "
              f"{t.status:<12} {t.agent:<20} {t.title}")
    return 0


def cmd_show(args) -> int:
    tasks = load_all()
    counts = unblock_counts(tasks)
    for raw in args.ids.split(","):
        tid = int(raw.strip())
        t = next((x for x in tasks if x.id == tid), None)
        if not t:
            print(f"task-{tid:03d}: not found")
            continue
        print(f"\n=== task-{t.id:03d} · {t.title} ===")
        print(f"status    {t.status}")
        print(f"agent     {t.agent}")
        print(f"score     {t.total(counts[t.id])}  "
              f"(gate {t.score_input('gate')} × risk {t.score_input('risk')} "
              f"× unblocks {counts[t.id]} ÷ cost {t.score_input('cost')})")
        print(f"owns      {', '.join(t.owns) or '(none)'}")
        print(f"resources {', '.join(t.resources) or '(none)'}")
        print(f"depends   {', '.join(f'task-{d:03d}' for d in t.deps) or '(none)'}")
        mate = str(t.meta.get("teammate", "") or "").strip()
        if mate:
            print(f"teammate  {mate}  (in-process — gone after /resume; re-spawn, "
                  f"do not message)" if t.status == "in-progress" else f"teammate  {mate}")
        print(f"evidence  {t.meta.get('evidence')}")
        print(f"source    {t.meta.get('source')}")
        print(t.body.rstrip())
    return 0


# ---------------------------------------------------------------- reindex


def cmd_reindex(args) -> int:
    tasks = load_all()
    counts = unblock_counts(tasks)
    rows = ranked(tasks)

    out = []
    out.append("# Backlog index")
    out.append("")
    out.append("<!-- GENERATED by Scripts/backlog.py reindex — DO NOT HAND-EDIT. -->")
    out.append("<!-- Edit the task-*.md files, then re-run reindex. -->")
    out.append("")
    out.append("Score is `(gate × risk × unblocks) ÷ cost`. The inputs are printed beside")
    out.append("the total so a disagreement costs one sentence, not a re-read.")
    out.append("")
    tally = {s: sum(1 for t in tasks if t.status == s) for s in STATUSES}
    out.append("| " + " | ".join(STATUSES) + " |")
    out.append("|" + "---|" * len(STATUSES))
    out.append("| " + " | ".join(str(tally[s]) for s in STATUSES) + " |")
    out.append("")

    queue = [r for r in rows if r[2].status == "proposed"][:7]
    out.append("## Audit queue — top 7 awaiting your verdict")
    out.append("")
    if not queue:
        out.append("*Nothing proposed. Run `/backlog sweep` to look for new work.*")
    else:
        out.append("| # | score | g×r×u÷c | task | agent | cost | evidence on done |")
        out.append("|---|---|---|---|---|---|---|")
        for i, (total, unb, t) in enumerate(queue, 1):
            out.append(
                f"| {i} | **{total}** | "
                f"{t.score_input('gate')}×{t.score_input('risk')}×{unb}÷{t.score_input('cost')} | "
                f"[{t.title}]({t.path.name}) `#{t.id:03d}` | {t.agent} | "
                f"{t.score_input('cost')} | {t.meta.get('evidence')} |"
            )
        out.append("")
        out.append("Approve with `py Scripts/backlog.py approve <ids>` "
                   "(or `/backlog approve <ids>`).")
    out.append("")

    for status in STATUSES:
        group = [r for r in rows if r[2].status == status]
        if not group:
            continue
        out.append(f"## {status} ({len(group)})")
        out.append("")
        out.append("| id | score | task | agent | owns | source |")
        out.append("|---|---|---|---|---|---|")
        for total, unb, t in group:
            owns = ", ".join(f"`{g}`" for g in t.owns) or "—"
            out.append(
                f"| `{t.id:03d}` | {total} | [{t.title}]({t.path.name}) | "
                f"{t.agent} | {owns} | `{t.meta.get('source', '')}` |"
            )
        out.append("")

    conflicts = find_conflicts(tasks)
    if conflicts:
        out.append("## ⚠ Lock conflicts")
        out.append("")
        for c in conflicts:
            out.append(f"- {c}")
        out.append("")

    out.append("---")
    out.append("")
    out.append(f"*Regenerated {date.today().isoformat()} · "
               f"{len(tasks)} task(s) · decisions in [LOG.md](LOG.md).*")

    INDEX.parent.mkdir(parents=True, exist_ok=True)
    INDEX.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {INDEX.relative_to(REPO)} ({len(tasks)} tasks, {len(queue)} in audit queue)")
    return 0


# ---------------------------------------------------------------- transitions


def set_field(head: str, key: str, value: str, required: bool = False) -> str:
    """Rewrite one frontmatter key in place, or append it before the closing fence.

    The replacement goes through a lambda: teammate names and task titles can carry
    backslashes, which re.sub would otherwise read as group references.
    """
    pat = re.compile(rf"(?m)^{re.escape(key)}:[^\r\n]*$")
    head2, n = pat.subn(lambda _m: f"{key}: {value}", head, count=1)
    if n:
        return head2
    if required:
        raise SystemExit(f"no {key}: line to rewrite")
    head = head.rstrip("\n")
    return head[: head.rfind("---")] + f"{key}: {value}\n---\n"


def write_fields(t: Task, fields: list) -> None:
    """Surgical frontmatter rewrite so the diff shows only what changed."""
    m = FM.match(t.raw)
    head, rest = t.raw[: m.end()], t.raw[m.end():]
    for key, value, required in fields:
        try:
            head = set_field(head, key, value, required)
        except SystemExit as e:
            raise SystemExit(f"{t.path.name}: {e}")
    t.path.write_text(head + rest, encoding="utf-8")


def set_status(t: Task, new: str) -> None:
    stamp = f"{date.today().isoformat()} {new}"
    write_fields(t, [("status", new, True), ("decided", f'"{stamp}"', False)])


def log(entries: list) -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    if not LOG.exists():
        LOG.write_text(
            "# Backlog decision log\n\n"
            "Append-only. Written by `Scripts/backlog.py`; never hand-edited.\n"
            "Rejected and parked tasks keep their files — this records why.\n\n",
            encoding="utf-8",
        )
    with LOG.open("a", encoding="utf-8") as fh:
        for e in entries:
            fh.write(e + "\n")


def transition(args, new: str) -> int:
    tasks = load_all()
    by_id = {t.id: t for t in tasks}
    reason = (getattr(args, "reason", "") or "").strip()
    if new in {"rejected", "parked"} and not reason:
        print(f"error: {new} requires --reason (rejections keep their reason forever)")
        return 1

    targets = []
    for raw in args.ids.split(","):
        raw = raw.strip()
        if not raw:
            continue
        tid = int(raw)
        if tid not in by_id:
            print(f"error: task-{tid:03d} does not exist")
            return 1
        targets.append(by_id[tid])

    if new == "approved":
        # Dry-run the approval: would these tasks collide with anything already live?
        original = {t.id: t.meta.get("status") for t in targets}
        for t in targets:
            t.meta["status"] = "approved"
        conflicts = find_conflicts(tasks)
        for t in targets:
            t.meta["status"] = original[t.id]
        if conflicts:
            print("error: approving these would create lock conflicts:")
            for c in conflicts:
                print(f"  - {c}")
            print("Finish or park the conflicting task first.")
            return 1

    today = date.today().isoformat()
    entries = []
    for t in targets:
        old = t.status
        set_status(t, new)
        tail = f" — {reason}" if reason else ""
        entries.append(f"- {today} · `task-{t.id:03d}` · {old} → **{new}** · "
                       f"{t.title}{tail}")
        print(f"task-{t.id:03d}  {old} → {new}  {t.title}")
    log(entries)
    cmd_reindex(args)
    return 0


def cmd_dispatch(args) -> int:
    """approved -> in-progress, with the teammate that is picking it up recorded.

    The last gate before real tokens get spent. It deliberately refuses more than
    `start` does: unapproved work and open dependencies both stop here, so the /host
    flow cannot spawn a teammate on something the owner has not seen.
    """
    name = (args.teammate or "").strip()
    if not name:
        print("error: dispatch requires --teammate <name> — the spawned teammate's "
              "name, so a resumed session can tell a live teammate from a ghost")
        return 1
    if not TEAMMATE_NAME.match(name):
        print(f"error: teammate name {name!r} is not a valid agent name "
              f"(letters, digits, - and _; must start alphanumeric; max 64)")
        return 1

    tasks = load_all()
    by_id = {t.id: t for t in tasks}
    targets = []
    for raw in args.ids.split(","):
        raw = raw.strip()
        if not raw:
            continue
        tid = int(raw)
        if tid not in by_id:
            print(f"error: task-{tid:03d} does not exist")
            return 1
        targets.append(by_id[tid])

    if len(targets) > 1:
        print("error: dispatch takes one task — a teammate name identifies one "
              "spawned agent, and two tasks sharing it makes the record a lie")
        return 1

    for t in targets:
        if t.status != "approved":
            print(f"error: task-{t.id:03d} is {t.status!r}, not 'approved'. Dispatch is "
                  f"only for work the owner has approved — run "
                  f"`py Scripts/backlog.py approve {t.id}` first.")
            return 1
        for d in t.deps:
            dep = by_id.get(d)
            if dep and dep.status not in CLOSED:
                print(f"error: task-{t.id:03d} depends on task-{d:03d}, which is still "
                      f"{dep.status!r}. Close it first or drop the dependency.")
                return 1

    today = date.today().isoformat()
    entries = []
    for t in targets:
        old = t.status
        stamp = f"{today} in-progress"
        write_fields(t, [
            ("status", "in-progress", True),
            ("teammate", name, False),
            ("decided", f'"{stamp}"', False),
        ])
        entries.append(f"- {today} · `task-{t.id:03d}` · {old} → **in-progress** · "
                       f"{t.title} — dispatched to teammate `{name}`")
        print(f"task-{t.id:03d}  {old} → in-progress  (teammate: {name})  {t.title}")
    log(entries)
    cmd_reindex(args)
    return 0


# ---------------------------------------------------------------- sweep


def sweep_report() -> dict:
    """Raw ingest surface. The skill decides what is real work; this only finds it."""
    rep = {"unchecked_boxes": [], "gdd_open_questions": [], "pending_briefs": [],
           "source_markers": [], "existing_sources": []}

    doc_files = sorted(REPO.joinpath("docs").rglob("*.md"))
    doc_files += sorted(REPO.glob("*.md"))
    for p in doc_files:
        if BACKLOG in p.parents:
            continue
        try:
            lines = p.read_text(encoding="utf-8").splitlines()
        except (UnicodeDecodeError, OSError):
            continue
        for i, line in enumerate(lines, 1):
            if re.match(r"^\s*[-*]\s*\[ \]\s+", line):
                rep["unchecked_boxes"].append({
                    "source": f"{p.relative_to(REPO).as_posix()}:{i}",
                    "text": line.strip()[5:].strip(),
                })

    gdd = REPO / "GDD.md"
    if gdd.exists():
        for i, line in enumerate(gdd.read_text(encoding="utf-8").splitlines(), 1):
            if not line.startswith("|"):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) >= 4 and cells[0].isdigit() and "✅" not in cells[3]:
                rep["gdd_open_questions"].append({
                    "source": f"GDD.md:{i}", "q": cells[0],
                    "question": cells[1], "lean": cells[2], "status": cells[3],
                })

    for p in sorted(REPO.joinpath("docs", "briefs").glob("brief-*.md")):
        text = p.read_text(encoding="utf-8")
        m = re.search(r"(?m)^status:\s*(\S+)", text)
        if m and m.group(1) in {"pending", "in-progress", "blocked"}:
            title = re.search(r"(?m)^title:\s*(.+)$", text)
            rep["pending_briefs"].append({
                "source": p.relative_to(REPO).as_posix(),
                "status": m.group(1),
                "title": title.group(1).strip() if title else p.stem,
            })

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
                        "text": line.strip(),
                    })

    for t in load_all():
        rep["existing_sources"].append({
            "id": t.id, "status": t.status, "source": str(t.meta.get("source", "")),
            "title": t.title,
        })
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


# ---------------------------------------------------------------- cli


def main() -> int:
    # Windows consoles default to cp1252, which cannot encode the arrows and em dashes
    # that live in task titles. Never let a print crash a status transition.
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, OSError):
        pass

    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("validate").set_defaults(fn=cmd_validate)
    sub.add_parser("next-id").set_defaults(fn=cmd_next_id)
    sub.add_parser("reindex").set_defaults(fn=cmd_reindex)

    p = sub.add_parser("list")
    p.add_argument("--status", help="comma-separated statuses to include")
    p.add_argument("--top", type=int, help="only the N highest-scoring")
    p.set_defaults(fn=cmd_list)

    p = sub.add_parser("show")
    p.add_argument("ids")
    p.set_defaults(fn=cmd_show)

    p = sub.add_parser("sweep-report")
    p.add_argument("--json", action="store_true")
    p.set_defaults(fn=cmd_sweep)

    p = sub.add_parser("dispatch")
    p.add_argument("ids", help="the one task id being handed to a teammate")
    p.add_argument("--teammate", required=True, help="name of the spawned teammate")
    p.set_defaults(fn=cmd_dispatch)

    for name, target in [("start", "in-progress"), ("review", "needs-review"),
                         ("approve", "approved"), ("done", "done"),
                         ("reject", "rejected"), ("park", "parked")]:
        p = sub.add_parser(name)
        p.add_argument("ids", help="comma-separated task ids")
        p.add_argument("-r", "--reason", default="")
        p.set_defaults(fn=(lambda a, _t=target: transition(a, _t)))

    args = ap.parse_args()
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
