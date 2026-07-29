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

**`model` is which model builds it, not which model planned it.** `/host` picks a
model profile up front — a thinking model for intake and drafting, an implementation
model for the teammate that executes — and stamps the implementation half here at
`dispatch --model`. It is optional: an empty value means the teammate inherits the
lead session's model, which is how every task filed before the profile existed reads.
Nothing about scoring, locking or ranking depends on it; only the spawn reads it, and
a resumed session reads it to re-spawn the same way.

**Rejections keep their files.** A rejected or parked task stays here with its
reason, the same way `GDD-TODO.md` kept the cut Base Camp Loot Manager entry as the
historical record of the idea.

**Evidence bar: sim vs PIE.** A numeric gameplay-balance claim doesn't default to a PIE
session. **Point-target** questions — army or hero DPS vs one Elite/Titan/Boss, TTK,
breakpoints — go to `sim-director`: `Scripts/sim/`'s point-target model is validated,
reproducing `entity-tiers.md` §7's own table exactly (`docs/sim/README.md`; `task-003`'s
TTK tabulation is the case that later proved it out). **Wave-attrition** questions —
swarm-vs-swarm survivor or casualty counts — are the opposite case: `docs/sim/LIMITATIONS.md`
§1 states the harness does not currently reproduce the one measured baseline it's checked
against (GATE1's 110-of-120). A wave-attrition run is a scaffold, never a standalone
evidence bar, without that caveat attached. Stances, leash, supply/degrade, items,
knockback, positioning, and multi-wave carryover aren't modelled at all (§4) — those stay
PIE, same as anything about feel or readability (`task-008` is the pattern). The
dependency runs both ways: `task-004`'s encounter-budget table is data the wave model is
*waiting on* (§2), not something the harness supplies to it.

**Scoring.** `total = (feel × risk × unblocks) ÷ cost`. `unblocks` is computed — it
is 1 plus the number of open tasks naming this one in `depends-on`, so do not write
it. Score the other three honestly:

| | 1 | 2 | 3 |
|---|---|---|---|
| `feel` | invisible in play — tooling, docs, refactor, a tracker correction | changes how something **reads or plays** — a tuning pass, a visual layer, a new readout | changes the **core feel or gameplay** — a mechanic, the combat loop, what the player does |
| `risk` | known work | some unknown | retires a real technical unknown |
| `cost` | under an hour | one session | multi-session (use 4 for spike-sized) |

**`feel` is the primary axis** (owner, 2026-07-28). It replaced `gate`, which ranked
by what blocked a milestone gate — that reliably floated plumbing above things the
player would actually notice. Rank by the most significant change in feel or gameplay
instead. Gate-blocking is not gone, it just moved: a task that unblocks a gate but
never shows up in play is `feel: 1` with a *Why now* that says what it unblocks, and
`unblocks` does the lifting. **An owner-brought feature is not automatically `feel: 3`**
— tooling the owner asked for out loud is still tooling.

Task files written before 2026-07-28 may still carry `score: {gate: …}`. `backlog.py`
reads it as `feel` so nothing silently drops to 1, and `validate` warns until it is
re-scored on the rubric above.

**`owns` and `resources` are locks.** Two simultaneously-active tasks may not claim
overlapping path globs (AGENT-TEAMS §3 — teammates editing the same file overwrite
each other) or the same resource (§5 — only one thing drives the Unreal editor).
`py Scripts/backlog.py validate` enforces both, and `approve` refuses a transition
that would create a collision. `pixellab-credits` is a lock because it is real money.

**`epic` threads one project across several teammates.** Optional, kebab-case, and
omitted for standalone work. Sibling tasks sharing an `epic:` are a *fan*: cut so
their `owns:` are disjoint, approved in one batch (`approve 44,45,46` — one prompt,
one collision dry-run), then dispatched one teammate each. Where siblings must
converge on a shared file, that write is a **join task** owning it with `depends-on`
naming every sibling; `dispatch` refuses the join until they close. Cut on file
ownership, not subject matter — two threads exist because they write different
files. `py Scripts/backlog.py epic <slug>` shows where the fan is and what may move
next; `validate` rejects siblings that claim the same path and warns when two of
them hold the same resource, because that fan cannot be batch-approved at all.

═══════════════════════════════════════════════════════════════════════════

---
id: 001
title: <imperative phrase — "Fill SYSTEMS.md §1 entity tier stat blocks">
status: proposed          # proposed|approved|in-progress|needs-review|done|rejected|parked
agent: gameplay-director  # <gameplay|narrative|performance|pixel-art|sim|ui>-director, or claude
model: ""                 # optional — opus|sonnet|haiku|fable; "" inherits the lead's
owns: []                  # path globs this task will write, e.g. ["docs/design/**"]
resources: []             # unreal-editor | pixellab-credits | mcp-9000
depends-on: []            # task ids that must close first, e.g. [4, 12]
epic: ""                  # optional kebab-case slug grouping a fan of sibling tasks
evidence: <the artifact that proves this is done>
score: {feel: 1, risk: 1, cost: 2}
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
