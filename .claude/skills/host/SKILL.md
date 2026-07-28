---
name: host
description: Turn a goal you state into one approved, dispatched piece of work — clarify what's ambiguous, draft a scored and lock-checked task in docs/backlog/, present the plan for your verdict, then spawn the teammate that builds it. Use when the user runs /host, hands over a goal or feature idea to be planned and built, asks to turn something into a task and get it started, or asks to dispatch an already-approved backlog task to a teammate.
---

# host — goal in, teammate out

`/backlog` finds work already latent in the repo. `/host` is the front door for work the
owner *brings*: a goal in, a running teammate out, with one approval in the middle.

    /host "make the flame flicker scale with army size"
      → clarify what would change the work
      → draft task-NNN, validated and lock-checked
        (or, if the goal is genuinely several — a fan of siblings sharing an epic, §3a)
      → present the plan here, and stop
      → owner approves
      → dispatch: approve → dispatch → spawn the named teammate
        (a fan: one batched approve, then one dispatch + spawn per sibling)

Judgment lives here. Everything mechanical lives in `Scripts/backlog.py` — id allocation,
score math, lock conflicts, status transitions, `INDEX.md`, `LOG.md`. Never hand-compute a
score, hand-write `INDEX.md`, or hand-edit a status the script owns.

**This skill runs in the lead session and nowhere else.** A subagent cannot ask the owner a
question and cannot spawn a teammate (`docs/AGENT-TEAMS.md` §5, no nested teams). The
`host` agent definition is the delegatable half — research and drafting — and it knows it
cannot do steps 2 and 6.

## 1 · Intake

Read the goal, then spend a few tool calls earning the right to file it:

- `py Scripts/backlog.py sweep-report --json` → check `existing_sources` and the filed
  titles. **If the goal is already a task, say so and stop drafting.** Offer the real
  choices: re-score it, dispatch it if it is approved, or supersede it deliberately.
- Grep current canon for the thing being asked for. `GDD.md`, `SYSTEMS.md`, `CLASSES.md`,
  `docs/narrative/FLAME-FOUNDATION.md`. Goals get built that already exist.
- Note the traps in `.claude/skills/backlog/SKILL.md` §Sweeping — they apply to goals too.
  `WORLD.md` is superseded by the 2026-07-22 reset; `docs/perf/niagara-sprite-refactor.md`
  §2/§8.1 still carry the retracted GPU-sim claim.

Delegate this leg to the `host` agent when the goal reaches into territory you have not
read this session. Do it yourself when you already have the files in context — a cold
subagent re-deriving what you just read is the expensive path.

## 2 · Clarify

**One round of `AskUserQuestion`. Two at the absolute most.** The budget is the point: a
host that interrogates gets routed around.

Ask only what would change the work — where different answers produce materially different
tasks. In practice that is:

| Ask about | Because it changes |
|---|---|
| Scope boundary | `owns:` globs, and whether this is one task or a fan (§3a) |
| Which director executes | `agent:`, and therefore what tools the work can use |
| The evidence bar | `evidence:` — a runnable build vs. a written spec vs. a diff |
| Editor / credits | `resources:`, which is a hard mutex against other live work |

Do not ask what the repo can answer. Do not ask the owner to pick a `score:` — score it
yourself and let them argue in one sentence. **If the goal is already unambiguous, skip
this step entirely and say you skipped it.** That is the good case, not a missed
opportunity.

## 3 · Draft

`py Scripts/backlog.py next-id`, then write `docs/backlog/task-NNN-<slug>.md` from
`docs/backlog/TEMPLATE.md`, at `status: proposed`. Field by field the rules are in
`.claude/skills/backlog/SKILL.md` §Writing a task; the ones that bite here:

- **`agent:`** must match what the definition can actually do. `pixel-art-director` writes
  specs, never image files. `ui-director`, `pixel-art-director` and `narrative-director`
  have **no shell** — they cannot build, PIE, or run a script. Source and content edits go
  to `claude`.
- **`owns:`** every path the teammate will write, and nothing more. Under-declaring causes
  silent overwrites; over-declaring blocks other work for no reason.
- **`resources:`** `unreal-editor` for anything that builds or PIEs, `mcp-9000` for
  unreal-mcp, `pixellab-credits` for anything that generates. Credits are real money.
- **Spawn prompt** — self-contained and paste-ready. The teammate loads `CLAUDE.md` and the
  repo but **not this conversation**, so everything you just clarified with the owner has
  to be written into the prompt itself, along with the canon warnings and an explicit list
  of what it must not touch.

Then `py Scripts/backlog.py validate` and `reindex`. If validate reports a lock conflict,
fix it before presenting — either narrow `owns:`, or add a `depends-on` and say in the plan
that this cannot start until the other task closes.

## 3a · Threading a project into a fan

Most goals are one task. A goal that is genuinely several — enough work for several
teammates at once — becomes a **fan**: sibling tasks sharing an `epic:` slug, drafted in one
pass and approved in one batch.

**Cut on file ownership, not subject matter.** Two threads exist because they write
different files. If two halves of the goal would write the same file, that is not two
threads — it is one thread, or it is two threads plus a join.

**The join task.** Where the siblings have to converge — `GDD.md`, `SYSTEMS.md`, a shared
DataTable — that write is its own task, owning the shared file, with `depends-on` naming
every sibling. It is the only way several teammates land in one file without overwriting
each other, and `dispatch` refuses it until the siblings close. `task-038` is the precedent:
five design specs each wanted `SYSTEMS.md`, so the shared write was split out.

**Resources cap the width.** `unreal-editor`, `mcp-9000` and `pixellab-credits` are global
mutexes, so two siblings that both need the editor cannot be approved together — the fan
serialises and `validate` warns. In practice this means specs fan wide and build work goes
in the join. Size fans at 3–4 threads, matching the teammate sizing in AGENT-TEAMS §3; the
lead carries every dispatch and handback, and that context is the real ceiling.

`py Scripts/backlog.py validate` rejects siblings claiming the same path before the owner
ever sees the plan, and `epic <slug>` is the roll-up for driving it afterwards.

## 4 · Present, and stop

Show one block. Everything the owner needs to disagree in a sentence, nothing else:

```
task-044 · Flame flicker scales with army size          docs/backlog/task-044-….md
  score 3.0   gate 3 × risk 2 × unblocks 1 ÷ cost 2
  agent       claude
  owns        ELVTR/Source/ELVTR/Rendering/**, docs/perf/flame-flicker.md
  resources   unreal-editor
  evidence    <the artifact that proves it done>
  won't touch GDD.md, SYSTEMS.md, NS_Swarm.uasset
  why now     <two sentences>
  confidence  high — <the one thing that would drop it>
  open        <anything you assumed past — or omit the line>
```

**`confidence` is required and it is about the plan, not the outcome.** High means the
scope, the `owns:` set and the evidence bar all survived contact with the repo. Medium or
low means name what is soft — an unverified canon claim, an `owns:` glob you guessed at, a
teammate that may not have the tools. The owner is about to approve with one click, so the
line that tells them when *not* to is the one carrying its weight.

For a fan, show one such block per sibling under an `epic <slug>` heading, then one
line naming the batch: which ids approve together, which is the join, and what the
owner gets back at the end. The fan is approved or refused as a whole — a plan the
owner can only half-approve was cut wrong.

Then **stop**. Do not start work. Do not spawn. Do not read approval into enthusiasm.
"Looks good" about the *shape* of a plan is not "go".

If the honest recommendation is that this should not be built — already done, better
waited on, needs a decision first — say that instead of presenting a plan. Recommending
work not happen is part of the job.

## 5 · The verdict

The owner's "go" runs `py Scripts/backlog.py approve NNN`. That command raises a permission
prompt from `Scripts/backlog_guard.py`, and **that prompt is the recorded verdict** — it is
what writes the decision to `docs/backlog/LOG.md`. Never try to route around it and never
suggest allowlisting it.

A fan approves as one command — `approve 44,45,46,47`. That is one prompt, one dry-run of
the whole set against the lock table, and one batched `LOG.md` write. Do not approve
siblings one at a time: four prompts for one decision trains the owner to wave the gate
through, which is the one thing the gate cannot survive.

If the owner changes a score input instead, edit the task's `score:` and re-run `reindex`.
Score edits are not privileged — argue freely.

## 6 · Dispatch

Approval is the go signal in this flow (unlike `/backlog`, where the owner spawns). Three
steps, in order:

1. `py Scripts/backlog.py dispatch NNN --teammate <name>` — records the teammate, moves the
   task to `in-progress`, logs it. It **refuses** unless the task is `approved` with every
   dependency closed, so an unapproved spawn is not reachable by accident. Run it *before*
   spawning: if it refuses, nothing has been spent.
2. Spawn with the `Agent` tool: `subagent_type` = the task's `agent:`, `name` = the same
   `<name>` you just recorded, `run_in_background` = true, `prompt` = the task file's spawn
   prompt **verbatim**, prefixed with `You are executing task-NNN. ` and suffixed with the
   handback instruction. `name` is what makes it addressable by `SendMessage`.
3. Tell the owner it is running, and how to reach it.

Pick the teammate name from the task slug — `flame-flicker`, `palette-lut`. It must match
`[A-Za-z0-9][A-Za-z0-9_-]{0,63}`; `dispatch` rejects anything else.

**One teammate per task.** The name is the record of which agent holds which work; two
tasks sharing a name makes it a lie, and `dispatch` refuses it.

For a fan, run those three steps once per sibling — `dispatch` takes a single id by design,
so there is no batch form and there should not be. The join task will refuse until its
siblings close; that refusal is the schedule, not an error. `py Scripts/backlog.py epic
<slug>` prints exactly which siblings are dispatchable now and which are still blocked, so
drive the fan from that rather than from memory of what you spawned.

## 7 · Handback

You own the closing transitions, not the teammate — three of the five directors have no
shell and cannot run the script at all.

When the teammate reports back: read what it actually produced, check it against the
task's `evidence:` bar, then `py Scripts/backlog.py review NNN`. Present the evidence to
the owner; `done` is their verdict and it is privileged. If the work missed the bar, say
so plainly and `SendMessage` the teammate rather than filing a second task.

Big changes hand over as a runnable build or on-screen evidence, never a diff plus "it
works" — see the `show-a-build-for-review` habit. The owner launches the editor for review.

Siblings in a fan hand back independently and are closed as they land — a sibling that is
`done` releases its `owns:` lock, which is what lets the join dispatch. Present the fan's
evidence once, at the join, rather than interrupting the owner four times; `epic <slug>`
carries the running state in the meantime. The project is finished when the epic reads
`n/n closed`, and that is the moment to say so.

## Never

- Spawn a teammate on a task that is not `approved`. `dispatch` enforces it; do not work
  around it by calling `Agent` first.
- Set `approved`, `done`, `rejected`, or `parked` by editing a file. The hook denies it and
  denying it is correct.
- Present a plan and start building in the same turn.
- Give two live teammates overlapping `owns:` globs or the same resource. `validate` catches
  it; heed it rather than loosening the declaration.
- Widen a sibling's `owns:` to make a fan "work". An overlap means the cut was wrong: move
  the shared write into a join task instead. Loosening the declaration does not remove the
  collision, it just stops the script from telling you about it.
- Message a teammate from a resumed session. `/resume` does not restore in-process
  teammates (`docs/AGENT-TEAMS.md` §5) — a `teammate:` stamp on an `in-progress` task may
  name a ghost. Re-spawn fresh with the same task prompt; do not `SendMessage` into the
  void.
