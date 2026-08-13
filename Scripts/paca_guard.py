#!/usr/bin/env python3
"""paca_guard.py — PreToolUse hook protecting the backlog's approval gate.

Wired in .claude/settings.json. Reads the hook payload on stdin.

ASK, on shell commands:

  Any invocation of paca.py with a privileged verb — approve, reject, park, done.
  These are the owner's verdicts on a task, and the prompt this raises IS the
  decision point: an agent must never authorise its own work.

  It has to be enforced here rather than by omission from an allowlist:
  .claude/settings.local.json carries a blanket `Bash(py *)` allow rule, so
  "we never allowlisted it" would leave the gate wide open. A hook `ask` decision
  overrides an allow rule; a narrower allow rule would not.

DENY, on file writes:

  Edits to the frozen records in docs/backlog/ — the closed task files and LOG.md.
  That directory is the archive of work that closed before the move to Paca; the
  live backlog is Paca itself. Editing an archived task changes nothing and misleads
  whoever reads it next. INDEX.md is exempt: it is the signpost to Paca, not a
  record, and it has to stay maintainable.

Everything else passes silently: agents create tasks, move their own work to
in-progress / needs-review, and run validate / waves / dispatch without friction.

This is a speed bump plus an audit trail, not a cryptographic gate. Its job is to
make approval impossible to perform incidentally.
"""

from __future__ import annotations

import json
import re
import sys

# paca.py invoked with a verb only the owner may authorise. Deliberately loose
# about interpreter and path form — `py Scripts\paca.py`, `python3 C:/.../paca.py`
# and `& paca.py` must all be caught.
PRIVILEGED_CMD = re.compile(
    r"paca\.py[\"']?\s+(approve|reject|park|done)\b", re.IGNORECASE)


def decide(decision: str, reason: str) -> None:
    json.dump({
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": decision,
            "permissionDecisionReason": reason,
        }
    }, sys.stdout)
    sys.stdout.flush()
    sys.exit(0)


def main() -> int:
    try:
        payload = json.load(sys.stdin)
    except (json.JSONDecodeError, ValueError):
        return 0  # never block on a payload we cannot read

    tool = payload.get("tool_name", "")
    ti = payload.get("tool_input") or {}

    if tool in {"Bash", "PowerShell"}:
        m = PRIVILEGED_CMD.search(str(ti.get("command") or ""))
        if m:
            decide("ask",
                   f"This records the owner's '{m.group(1)}' verdict on a Kindled "
                   f"backlog task in Paca. Approve only if the owner asked for it — "
                   f"an agent must never authorise its own work.")
        return 0

    if tool not in {"Edit", "Write", "MultiEdit", "NotebookEdit"}:
        return 0

    path = str(ti.get("file_path") or ti.get("notebook_path") or "").replace("\\", "/")
    lower = path.lower()
    if "docs/backlog/" in lower and not lower.endswith("docs/backlog/index.md"):
        decide("deny",
               "docs/backlog/ is a frozen archive of work that closed before the move "
               "to Paca. The live backlog is Paca (http://localhost:8090, project "
               "Kindled) — use the paca MCP tools to read or write tasks, and "
               "`py Scripts/paca.py` for validate / waves / dispatch / verdicts.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
