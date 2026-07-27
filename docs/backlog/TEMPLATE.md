# Backlog Task Template

Copy everything below the `═══` line into `task-<id>-<slug>.md`
(e.g. `task-012-fill-systems-entity-tiers.md`). IDs are zero-padded three digits —
get the next one with `py Scripts/backlog.py next-id`. One task per file: one thing
that can be approved, done, and shown evidence for on its own.

Written by: `host` (or the user), through `/backlog` for work swept out of the repo
and `/host` for a goal the owner brings. Audited by: the user. Executed by the
`agent:` named in the frontmatter.

**Statuses.** `proposed → approved → in-progress → needs-review → done`, with
`rejected` and `parked` as exits. An agent may set `proposed`, `in-progress`, and
`needs-review`. The other four are the owner's verdict — `Scripts/backlog_guard.py`
denies them as file edits, and they go through `py Scripts/backlog.py approve|reject|park|done`,
which prompts the owner and appends to `LOG.md`.

**Dispatch.** `py Scripts/backlog.py dispatch <id> --teammate <name>` is `start` with
the spawned teammate recorded. It refuses unless the task is already `approved` with
every dependency closed, which is what keeps "spawn a teammate on unapproved work"
out of reach. One teammate per task. The `teammate:` stamp exists because in-process
teammates do not survive `/resume` (AGENT-TEAMS §5) — a resumed session reads the
name, knows it is a ghost, and re-spawns instead of messaging it.

**Rejections keep their files.** A rejected or parked task stays here with its
reason, the same way `GDD-TODO.md` kept the cut Base Camp Loot Manager entry as the
historical record of the idea.

**Scoring.** `total = (gate × risk × unblocks) ÷ cost`. `unblocks` is computed — it
is 1 plus the number of open tasks naming this one in `depends-on`, so do not write
it. Score the other three honestly:

| | 1 | 2 | 3 |
|---|---|---|---|
| `gate` | no gate impact | unblocks a gate item | blocks Gate 1 / Gate 2 directly |
| `risk` | known work | some unknown | retires a real technical unknown |
| `cost` | under an hour | one session | multi-session (use 4 for spike-sized) |

**`owns` and `resources` are locks.** Two simultaneously-active tasks may not claim
overlapping path globs (AGENT-TEAMS §3 — teammates editing the same file overwrite
each other) or the same resource (§5 — only one thing drives the Unreal editor).
`py Scripts/backlog.py validate` enforces both, and `approve` refuses a transition
that would create a collision. `pixellab-credits` is a lock because it is real money.

═══════════════════════════════════════════════════════════════════════════

---
id: 001
title: <imperative phrase — "Fill SYSTEMS.md §1 entity tier stat blocks">
status: proposed          # proposed|approved|in-progress|needs-review|done|rejected|parked
agent: gameplay-director  # gameplay|narrative|performance|pixel-art|ui-director, or claude
owns: []                  # path globs this task will write, e.g. ["docs/design/**"]
resources: []             # unreal-editor | pixellab-credits | mcp-9000
depends-on: []            # task ids that must close first, e.g. [4, 12]
evidence: <the artifact that proves this is done>
score: {gate: 1, risk: 1, cost: 2}
source: <file:line this was ingested from, or "user">
teammate: ""              # written by backlog.py dispatch — the agent holding this
decided: ""               # written by backlog.py on the owner's verdict
---

## Why now
Two or three sentences. What this unblocks, or what breaks while it stays undone.
If the honest answer is "nothing breaks", say so — that is what a low score means.

## Done when
The finish line, stated so someone else could judge it. Not "improve X" but
"`SYSTEMS.md` §1 has a stat block per tier and `docs/data/entity-tiers.json`
imports as a DataTable". The `evidence:` field is the artifact; this is the bar.

## Spawn prompt
Self-contained — teammates load `CLAUDE.md` and the repo but **not** the lead
session's conversation history, so anything only a live session knows must be
written out here. Paste-ready:

```
<the full prompt, naming the agent type, the files it owns, what it must not
touch, and what to hand back>
```
