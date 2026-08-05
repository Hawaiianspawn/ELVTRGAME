# Backlog archive — the live backlog moved to Paca

**The live backlog is Paca**, self-hosted at <http://localhost:8090>, project
**Kindled**. Nothing in this directory is live. It is the record of the 81 tasks
that closed before the move on 2026-08-01, kept because a closed task file is the
historical account of why something was built the way it was.

## Where things went

| Was | Now |
|---|---|
| `docs/backlog/task-NNN-*.md` (open work) | Paca tasks, keyed by the `Legacy ID` custom field — `task-108` is still `task-108` |
| `docs/backlog/task-NNN-*.md` (done) | still here, frozen |
| `INDEX.md` — generated board | the Paca board, and `/paca` |
| `LOG.md` — verdict history | still here for the pre-move record; new verdicts go to Paca's activity log |
| `TEMPLATE.md` | still here — the scoring rubric and evidence bar are unchanged canon |
| `Scripts/backlog.py` | `Scripts/paca.py` for validate / waves / dispatch / verdicts; everything else is the `paca` MCP server |
| `Scripts/backlog_guard.py` | `Scripts/paca_guard.py` |

## What did not move, and why

**The 81 `done` tasks stayed.** Importing them would have put 81 closed items on a
board whose whole purpose is to make the open decisions visible.

**Dependencies on those 81 were discharged, not dropped.** A task that read
`depends-on: [053]` where `053` is `done` carries no dependency in Paca. The full
historical list is in this archive.

## The two rules Paca does not enforce

`Scripts/paca.py` holds them, because they are this project's and not Scrum's:

- **Locks.** Two simultaneously-active tasks may not write overlapping `owns:` path
  globs (`docs/AGENT-TEAMS.md` §3) or hold the same resource (§5). `validate` and
  `waves` are the single definition of independence.
- **The owner's verdict.** `approve` / `reject` / `park` / `done` go through
  `paca.py`, behind the `paca_guard.py` prompt. That prompt is the signature.

Editing the frozen records in this directory is denied by the hook. That is
deliberate. This file is the exception, because it is the signpost.
