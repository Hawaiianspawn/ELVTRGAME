---
name: backlog
description: Emberkeep's work queue — sweep the repo for undone work, rank it by a stated score, present the top 7 for the owner to audit, and record their verdicts. Use when the user runs /backlog, asks what to work on next, asks to add or re-rank a task, approves or rejects backlog items, or wants the backlog swept for newly-landed or newly-stale work.
---

# backlog — propose, rank, audit

This skill **proposes and ranks only**. It never dispatches, never approves its own work,
and never edits canon. The owner decides what happens; this skill makes deciding cheap.

**Sibling front door.** `/backlog` finds work already latent in the repo. `/host` takes a
goal the owner brings, clarifies it, drafts one task, and — after approval — spawns the
teammate that builds it. Same task store, same scoring, same locks; `/host` is the only one
of the two that dispatches. The `host` agent definition backs both.

Judgment lives here. Everything mechanical lives in `Scripts/backlog.py` — id allocation,
validation, score math, INDEX regeneration, lock conflicts, status transitions, LOG
appends. Same split as `/sprite` ↔ `Scripts/art/pixelpipe.py`. Never hand-compute a score
or hand-write `INDEX.md`.

## Commands

| Invocation | Do this |
|---|---|
| `/backlog` | `py Scripts/backlog.py list --status proposed --top 7`, present the audit queue, and ask for the verdicts as a question |
| `/backlog sweep` | Full ingest pass — see below |
| `/backlog approve 3,5` | `py Scripts/backlog.py approve 3,5` |
| `/backlog reject 4 <reason>` | `py Scripts/backlog.py reject 4 -r "<reason>"` |
| `/backlog park 9 <reason>` | `py Scripts/backlog.py park 9 -r "<reason>"` |
| `/backlog add <description>` | Write one new task file, then `validate` + `reindex` |
| `/backlog show 12` | `py Scripts/backlog.py show 12` |
| `/backlog epic <slug>` | `py Scripts/backlog.py epic <slug>` — a threaded project's fan and what moves next |

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
`py Scripts/backlog.py approve 3,5,6` — one command, one prompt, one batched `LOG.md`
write. Never approve them one at a time: four prompts for one decision trains the owner to
wave the gate through, which is the one thing the gate cannot survive.

Rejects and parks need a *reason*, so they do not fit a click. Leave them to the free-text
answer the question already offers, or ask after. If the owner answers in prose instead —
"approve 1,3,4; park 2" — take it; the question exists to make deciding cheap, not to
insist on a format.

Then stop and wait. Do not start work, do not spawn anyone, do not assume approval from
enthusiasm. **`AskUserQuestion` is not the gate** — the permission prompt that
`Scripts/backlog_guard.py` raises on `approve` is what records the verdict. The click is the
decision, that prompt is the signature, and both happen.

If the owner disagrees with a score, edit the task's `score:` inputs and re-run `reindex`.
Score edits are not privileged — argue freely.

## Sweeping

`py Scripts/backlog.py sweep-report --json` returns the raw surface: unchecked boxes across
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

Copy `docs/backlog/TEMPLATE.md` below its `═══` line. Get the id from
`py Scripts/backlog.py next-id`. Then:

- **`agent:`** — one of the five directors, or `claude`. Match the definition's actual
  scope: `pixel-art-director` writes specs and never image files; `ui-director` has no
  shell and cannot drive the editor; only `claude` should hold source or content edits.
- **`owns:`** — every path glob the task will write. Under-declaring causes silent
  overwrites between concurrent teammates; over-declaring blocks work needlessly.
- **`resources:`** — `unreal-editor` for anything that PIEs or builds, `mcp-9000` for
  unreal-mcp, `pixellab-credits` for anything that generates. Credits are real money.
- **`evidence:`** — the artifact that proves it is done. A task that cannot name one is not
  ready to be proposed. Big changes hand over as a runnable build or on-screen evidence,
  never a diff plus "it works".
- **`score:`** — score honestly. A low score is information, not a failure; if nothing
  breaks while a task stays undone, say so in *Why now*.
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
one; `py Scripts/backlog.py epic <slug>` for where a running fan stands.

**Recommending a park is a real outcome.** An agent cannot set `parked`, so file it as
`proposed` with the recommended `backlog.py park` command in the body and let the owner run
it. Tasks 033–037 are the pattern.

## Re-ranking

Re-rank when a task closes, not on a timer. `unblocks` is computed from open dependents, so
closing a task changes the scores of everything downstream automatically — just run
`reindex`. A backlog that re-ranks on a schedule becomes a nag; one that re-ranks on
landings stays useful.

## Never

- Set `approved`, `done`, `rejected`, or `parked` by editing a file. The hook denies it and
  denying it is correct.
- Edit `GDD.md`, `SYSTEMS.md`, or `CLASSES.md` from this skill. Propose; the owner decides.
- Hand-edit `INDEX.md` or `LOG.md`.
- Dispatch approved work **from this skill**. Here, approval is not a start signal — the
  owner spawns, or runs `/host` to dispatch it. (In `/host` the owner's approval *is* the go
  signal, because they were shown the plan first. That is the difference between the two.)
