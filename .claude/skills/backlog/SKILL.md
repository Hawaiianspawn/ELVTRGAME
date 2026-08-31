---
name: backlog
description: Kindled's work queue — sweep the repo for undone work, rank it by a stated score, present the top 7 for the owner to audit, and record their verdicts. Use when the user runs /backlog, asks what to work on next, asks to add or re-rank a task, approves or rejects backlog items, or wants the backlog swept for newly-landed or newly-stale work.
---

# backlog — propose, rank, audit

This skill **proposes and ranks only**. It never dispatches, never approves its own work,
and never edits canon. The owner decides what happens; this skill makes deciding cheap.

**Sibling front door.** `/backlog` finds work already latent in the repo. `/host` takes a
goal the owner brings, clarifies it, drafts one task, and — after approval — spawns the
teammate that builds it. Same task store, same scoring, same locks; `/host` is the only one
of the two that dispatches. The `host` agent definition backs both.

**`/backlog` presents the score; `/host` does not.** Ranking is what this skill is for —
a queue of 40 needs an order. A feature the owner just brought is not competing with
anything, so `/host` files the score and shows the plan instead.

**Every owner decision is a button, never typing.** If the owner has to choose — which
tasks to approve, whether a refusal is a reject or a park, what a stale tracker entry
should become — it arrives as an `AskUserQuestion` with the real options as buttons,
including the follow-up a refusal opens. A follow-up spends the same slot as the question
it came from; it is that decision continuing, not a new interruption. Prose answers are
always accepted, never required, and "what would you like to do about it?" is a question
you should have already turned into options.

Judgment lives here. Everything mechanical lives in `Scripts/paca.py` — id allocation,
validation, score math, INDEX regeneration, lock conflicts, status transitions, LOG
appends. Same split as `/sprite` ↔ `Scripts/art/pixelpipe.py`. Never hand-compute a score
or hand-set a status the script owns.

**The backlog is Paca** — self-hosted at <http://localhost:8090>, project **Kindled**,
reachable as the `paca` MCP server and through `/paca`. Tasks keep their `task-NNN`
handle in the `Legacy ID` field. `docs/backlog/` is a frozen archive of the 81 tasks
that closed before the move; the hook denies edits to it. Reading — listing, searching,
showing, commenting — is the MCP server or the web board; `paca.py` exists only for the
locks, the scheduling and the owner's verdict.

## Commands

| Invocation | Do this |
|---|---|
| `/backlog` | `py Scripts/paca.py list --status proposed`, take the top 7, present the audit queue, and ask for the verdicts as a question |
| `/backlog sweep` | Full ingest pass — see below |
| `/backlog approve 3,5` | `py Scripts/paca.py approve 3,5` |
| `/backlog reject 4 <reason>` | `py Scripts/paca.py reject 4 --reason "<reason>"` |
| `/backlog park 9 <reason>` | `py Scripts/paca.py park 9 --reason "<reason>"` |
| `/backlog add <description>` | Draft one spec, `py Scripts/paca.py new <spec.json>`, then `validate` |
| `/backlog show 12` | The `paca` MCP server's task read, or the board at <http://localhost:8090> |
| `/backlog epic <slug>` | `py Scripts/paca.py list --epic <slug>` — a threaded project's fan and what moves next |

Approve / reject / park / done will raise a permission prompt. That prompt **is** the
approval gate — never try to route around it, and never suggest allowlisting these
commands.

## Presenting the audit queue

Show the top 7 and nothing else. For each: rank, score with its four inputs visible, title,
agent, cost, the evidence-on-done, and **your one-word recommendation with confidence** —
`approve · high`, `park · medium`, `needs a decision first`. The inputs are the point — the
owner should be able to disagree in one sentence ("risk is 1, the swarm sim already proved
that") instead of re-reading the task; the recommendation is what lets them agree without
writing anything at all.

**Then collect the verdict with `AskUserQuestion`, in the same turn.** The owner clicks;
they should not have to type ids. Seven items do not fit one question, so split them,
`multiSelect: true` on both:

- Q1 `Ranks 1–4` — "Approve which of these?" One option per task, label = rank + short
  title, description = your recommendation and the reason for it.
- Q2 `Ranks 5–7` — same shape.

Two questions, one dialog, one round trip. Then run the whole answer as a single
`py Scripts/paca.py approve 3,5,6` — one command, one prompt, one batched
write. Never approve them one at a time: four prompts for one decision trains the owner to
wave the gate through, which is the one thing the gate cannot survive.

**Rejects and parks are buttons too.** A reason does not make a decision unclickable — you
read the task, so you know the two or three reasons it would be refused. Anything left
unticked in Q1/Q2 gets one follow-up `AskUserQuestion`, header `Refused`, one option per
untouched task: `park — <the likely reason>`, `reject — <the likely reason>`, `leave
proposed`. Take the ticked reason as the `-r` string. `Other` is always there for the case
you misread. If the owner answers in prose instead — "approve 1,3,4; park 2, it needs the
palette call first" — take it; the question exists to make deciding cheap, not to insist on
a format.

Then stop and wait. Do not start work, do not spawn anyone, do not assume approval from
enthusiasm. **`AskUserQuestion` is not the gate** — the permission prompt that
`Scripts/paca_guard.py` raises on `approve` is what records the verdict. The click is the
decision, that prompt is the signature, and both happen.

If the owner disagrees with a score, edit the task's score fields in Paca.
Score edits are not privileged — argue freely.

## Sweeping

`py Scripts/paca.py sweep-report --json` returns the raw surface: unchecked boxes across
all docs, non-✅ rows in the GDD §12 table, pending briefs, TODO/FIXME markers under
`ELVTR/Source`, and every already-filed task's `source`. The script finds candidates; you
decide what is real work.

**Verify before filing. This is the rule that makes a sweep worth running** — without it
you produce a second stale list beside the first one. Every candidate gets checked against
current canon before it becomes a task. Known traps, all of which have already bitten:

- **A box may already be done.** `docs/GDD-TODO.md:46` flagged "name the game" as a
  blocker for five days after `GDD.md:433` recorded the answer (Emberkeep). Those file as
  *tracker corrections*, not as work.
- **A box may not be a task at all.** 30 of the 86 unchecked boxes in this repo are art
  *acceptance criteria* inside specs, and 3 more are a pick-one verdict in
  `SPIKE1-RESULTS.md`. Collapse a spec's checklist into one verification task; do not file
  eleven.
- **The same work appears in two docs.** `SYSTEMS.md`'s fill list and
  `docs/RTS-VERTICAL-SLICE.md` §4 are mirrors. One task, both sources cited.
- **WORLD.md is superseded** by the 2026-07-22 narrative reset; current canon is
  `docs/narrative/FLAME-FOUNDATION.md`. Work sourced from WORLD.md needs a canon-migration
  question answered before it is real work.
- **`ELVTR/Source` was clean as of 2026-07-26** — zero TODO/FIXME. Re-run it, but do not
  be surprised by a nil result.

Dedupe against `existing_sources` in the report before writing anything.

## Writing a task

Draft the spec as JSON and file it with `py Scripts/paca.py new <spec.json>`, which
allocates the id and lands it at `proposed`. The field rules are unchanged —
`docs/backlog/TEMPLATE.md` still holds the scoring rubric and the evidence bar. Then:

- **`agent:`** — one of the six directors, or `claude`. Match the definition's actual
  scope: `pixel-art-director` writes specs and never image files; `ui-director` has no
  shell and cannot drive the editor; `sim-director` runs `Scripts/sim/` and never touches
  `SYSTEMS.md` or `docs/design/`; only `claude` should hold source or content edits.
- **`owns:`** — every path glob the task will write. Under-declaring causes silent
  overwrites between concurrent teammates; over-declaring blocks work needlessly.
- **`resources:`** — `unreal-editor` for anything that PIEs or builds, `mcp-9000` for
  unreal-mcp, `pixellab-credits` for anything that generates. Credits are real money.
- **`evidence:`** — the artifact that proves it is done. A task that cannot name one is not
  ready to be proposed. Big changes hand over as a runnable build or on-screen evidence,
  never a diff plus "it works".
- **A numeric gameplay claim routes by question shape, not by default.** Point-target
  (army/hero vs one Elite/Titan/Boss — TTK, breakpoints) is validated: `sim-director`
  reproduces `entity-tiers.md` §7's own table exactly (`task-003`'s TTK tabulation is the
  case that proved it out). Wave-attrition (swarm-vs-swarm survivor counts) is a scaffold
  only — `docs/sim/LIMITATIONS.md` §1 says plainly the harness doesn't yet reproduce the
  one measured baseline it's checked against — never file that as a standalone evidence
  bar. Stances, leash, supply/degrade, items, positioning, and feel or readability
  (`task-008` is the pattern) stay PIE; `LIMITATIONS.md` §4 lists what the harness doesn't
  model at all. Full rule: `TEMPLATE.md`'s "Evidence bar: sim vs PIE".
- **`score:`** — `(feel × risk × unblocks) ÷ cost`, and **`feel` — how much this changes
  the moment-to-moment feel or gameplay — is the primary axis** (owner, 2026-07-28; it
  replaced `gate`). Rubric in `docs/backlog/TEMPLATE.md`. Tooling, docs and refactors are
  `feel: 1` however necessary; what they unblock goes in *Why now*. A low score is
  information, not a failure. Files predating the rename still say `gate:` — read as
  `feel`, warned by `validate`, and due a re-score.
- **Spawn prompt** — self-contained. Teammates load `CLAUDE.md` and the repo but **not**
  this session's history. Name what the task must not touch, and carry the canon warnings
  (superseded WORLD.md, stale `niagara-sprite-refactor.md` §2/§8.1) into the prompt itself.

- **`epic:`** — optional, kebab-case, only when one project was threaded into several
  sibling tasks meant to run at once. Omit it for standalone work; a fan of one is just a
  task, and `validate` warns about it.

**Two tasks must never own the same file.** If several tasks all need to write one file —
`SYSTEMS.md` is the recurring case — split the shared write into its own task that depends
on the rest. `task-038` exists for exactly that reason. When those tasks are siblings in an
`epic:`, that split-out task is the fan's **join**, and `validate` treats a sibling overlap
as an error rather than waiting for `approve` to refuse it. See `/host` §3a for how to cut
one; `py Scripts/paca.py list --epic <slug>` for where a running fan stands.

**Recommending a park is a real outcome.** An agent cannot set `parked`, so file it as
`proposed` with the recommended `paca.py park` command in the body and let the owner run
it. Tasks 033–037 are the pattern.

## Commit and push

**A queue that only exists in a working tree is not a queue.** Whenever this skill writes —
tasks filed from a sweep, a re-score, a verdict — all of that now lives in Paca, so a
sweep may leave the working tree clean. Commit whatever repo files the pass did touch —
commit and push in the same turn. No question, this is part of the write, not a decision:

    git add <the tracker files you corrected>
    <commit per CLAUDE.md commit rules>
    git push

Rules that bite:

- **Stage the backlog paths, never `git add -A`.** The tree is shared with other sessions
  (`concurrent-sessions-share-the-tree`) and carries unrelated modified assets.
- **One commit per pass** — a whole sweep is one commit, a batch of verdicts is one commit.
- Commit *after* the `approve`/`reject`/`park` command, so the repo and Paca go
  in together. A commit that records a task without the verdict that moved it is a lie.
- If the push is refused — protected branch, no upstream, conflict — say so in one line and
  hand the owner the fix. Do not force, do not rebase the shared branch unasked.

**This applies to the skill itself.** Any change to `.claude/skills/backlog/SKILL.md` gets
committed and pushed in the turn it is made, on its own commit. A process improvement
living only in one session's working tree is not a process improvement.

## Re-ranking

Re-rank when a task closes, not on a timer. `unblocks` is computed from open dependents, so
closing a task changes the scores of everything downstream automatically — just run
a sweep. A backlog that re-ranks on a schedule becomes a nag; one that re-ranks on
landings stays useful.

## Never

- Set `approved`, `done`, `rejected`, or `parked` by editing a file. The hook denies it and
  denying it is correct.
- Edit `GDD.md`, `SYSTEMS.md`, or `CLASSES.md` from this skill. Propose; the owner decides.
- Set `approved`, `done`, `rejected` or `parked` through the Paca API, the MCP tools or
  the web UI. Those four go through `paca.py`, behind the hook prompt — that prompt is
  the owner's signature.
- Edit anything under `docs/backlog/` except `INDEX.md`. It is a frozen archive.
- Put a decision in prose, or ask the owner to type a reason a button could have carried.
- Leave filed tasks or recorded verdicts uncommitted, `git add -A` in a shared tree, or
  edit this skill without committing and pushing that edit.
- Dispatch approved work **from this skill**. Here, approval is not a start signal — the
  owner spawns, or runs `/host` to dispatch it. (In `/host` the owner's approval *is* the go
  signal, because they were shown the plan first. That is the difference between the two.)
