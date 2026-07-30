---
name: host
description: Take one or more features the owner wants built, turn each into a lock-checked task, work out which of them are independent, and dispatch the independent ones to teammates in parallel after a single approval. Use when the user runs /host, hands over a feature idea or a list of them to be planned and built, asks what can be worked on at the same time, asks to turn something into a task and get it started, asks to dispatch an already-approved backlog task, or asks what is currently running.
---

# host — features in, parallel teammates out

`/backlog` finds work already latent in the repo. `/host` is the front door for work the
owner *brings*: **write the features down as tasks, work out which are independent, run
the independent ones at once.**

    /host "flame flicker scales with army size; also the retinue cap needs tuning"
      → split the ask into features, one task each, and research each (§1)
      → clarify only what would change the work — at most one question (§2)
      → draft a task per feature, validated and lock-checked (§3)
      → ask `waves` what can run at once, if there is more than one task (§3a)
      → present the plan and ask for the one verdict (§4, §5) — then stop
      → owner approves → dispatch wave 1, all at once, and post the board (§6)
      → one-line receipt as each lands; full evidence once, at the close (§7)
      → close the batch into one branch and one PR for review (§7a)
      → the run ends when the batch closes. Say so and stop (§8)

## Invocations

| You type | Do this |
|---|---|
| `/host <feature>` | The full flow above |
| `/host <feature A>; also <feature B>` | Same flow, split at §1, scheduled at §3a |
| `/host` *(nothing)* | **Do not open an empty prompt.** Run `py Scripts/backlog.py list --status in-progress,needs-review` and `waves --approved-only` first. If anything is running, post the board (§6) — that is almost always what was meant. If nothing is running, name the top 3 approved-or-proposed tasks by score and ask which, as one `AskUserQuestion` |
| `/host status` | The board, from the script — see §`/host status` |
| `/host dispatch NNN` | The task is already approved: skip §1–§5 entirely, go straight to §6 |
| `/host models` | Ask, write `.claude/host-models.json`, stop |

## The interruption budget

This is the constraint the rest of the skill serves. **One `/host` run costs the owner at
most two questions and two permission prompts, and never more:**

| | What | When |
|---|---|---|
| Question 1 | Clarify — *optional*, and the good case is skipping it | §2 |
| Question 2 | **The verdict.** Build this, or not | §5 |
| Prompt 1 | `approve <ids>` — the signature on that verdict | §5 |
| Prompt 2 | `done <ids>` — the signature that it landed | §7 |

Anything else that wants an owner decision is a bug in the plan, not a question to ask.
Do not ask about models, do not ask the owner to confirm a feature split, do not ask
them to pick a score. Take the default, say which default you took in one line, and let
them correct you.

**Independence is computed, not judged.** Two tasks may run at once when they write
disjoint paths and share no resource — `py Scripts/backlog.py waves` decides that from
the `owns:` and `resources:` you declared. Never eyeball it; a wave you reasoned your way
to is a wave `approve` may refuse.

Judgment lives here. Everything mechanical lives in `Scripts/backlog.py` — id allocation,
score math, lock conflicts, wave packing, status transitions, `INDEX.md`, `LOG.md`. Never
hand-compute a score, hand-write `INDEX.md`, hand-edit a status the script owns, or
hand-plan a wave.

**This skill runs in the lead session and nowhere else.** A subagent cannot ask the owner
a question and cannot spawn a teammate (`docs/AGENT-TEAMS.md` §5, no nested teams). The
`host` agent definition is the delegatable half — research and drafting — and it knows it
cannot do §2 or §6.

## 1 · Intake — split the ask, then earn the right to file it

The owner may hand over one feature or six. **First job: name the features.** One feature
is one thing that could be approved, built, and shown evidence for on its own. Write the
split back in one line as a *statement*, not a question, and keep going:

    reading that as three features — flicker-scales-with-army, retinue cap tuning,
    and the HUD readout. Correct me if that is one thing or four.

Two ways the split goes wrong, both expensive:

- **Splitting what is really one write.** Two "features" that both edit `SYSTEMS.md` are
  one task, or one task plus a join (§3a). Splitting them buys nothing and costs a collision.
- **Lumping what is really several.** One task owning four unrelated directories cannot be
  parallelised and gives the owner one verdict where they wanted four.

The test is the one the scheduler uses: **would these write the same files?**

Then, per feature, spend a few tool calls before drafting:

- `py Scripts/backlog.py sweep-report --json` → check `existing_sources` and filed titles.
  **If the goal is already a task, say so and stop drafting.** Offer the real choices:
  re-score it, dispatch it if approved, or supersede it deliberately.
- Grep current canon: `GDD.md`, `SYSTEMS.md`, `CLASSES.md`, `docs/narrative/FLAME-FOUNDATION.md`.
  Goals get built that already exist.
- Note the traps in `.claude/skills/backlog/SKILL.md` §Sweeping — they apply to goals too.
  `WORLD.md` is superseded by the 2026-07-22 reset; `docs/perf/niagara-sprite-refactor.md`
  §2/§8.1 still carry the retracted GPU-sim claim.

**Do this inline.** The lead already has the conversation, and that is most of the context
a good task file needs. Delegate to the `host` agent only when the goal reaches into
territory this session has not read at all — and when you do, pass it what the conversation
knows that the repo does not, because it cannot ask.

**Researching several features in parallel is fine — allocating their ids in parallel is
not.** `next-id` reads the directory, so two agents drafting at once will both be told `063`
and one will silently overwrite the other. If you fan the research out, **run `next-id` in
the lead, once, and tell each agent the exact id and filename to write.**

## 2 · Clarify — once, or not at all

**At most one `AskUserQuestion`, covering every feature.** Not one round per feature. If
four features each need clarifying, the ask was too big to plan in one pass — say that
instead of opening a second round.

Ask only where different answers produce materially different tasks:

| Ask about | Because it changes |
|---|---|
| Scope boundary | `owns:` globs — and therefore what runs beside what |
| Which director executes | `agent:`, and therefore what tools the work can use |
| The evidence bar | `evidence:` — a runnable build vs. a written spec vs. a diff |
| Editor / credits | `resources:`, a hard mutex that serialises a wave |

Do not ask what the repo can answer. Do not ask the owner to pick a score. **If everything
is already unambiguous, skip this step entirely and say you skipped it** — that is the
good case, and it means the owner sees exactly one question all run.

## 3 · Draft one task per feature

`py Scripts/backlog.py next-id`, then write `docs/backlog/task-NNN-<slug>.md` from
`docs/backlog/TEMPLATE.md`, at `status: proposed`. Field rules are in
`.claude/skills/backlog/SKILL.md` §Writing a task; the ones that bite here:

- **`agent:`** must match what the definition can actually do. `pixel-art-director` writes
  specs, never image files. `ui-director`, `pixel-art-director` and `narrative-director`
  have **no shell** — they cannot build, PIE, or run a script. Source and content edits go
  to `claude`. `sim-director` does have a shell and runs `Scripts/sim/` — a point-target
  claim (army/hero vs one Elite/Titan/Boss) routes there, not to a PIE evidence bar; a
  wave-attrition survivor count from the same agent is a scaffold, never a standalone
  evidence bar, per `docs/sim/LIMITATIONS.md` §1 (`TEMPLATE.md`'s "Evidence bar: sim vs
  PIE" has the full rule).
- **`owns:`** every path the teammate will write, and nothing more. **This field decides
  the schedule.** Under-declaring causes silent overwrites; over-declaring falsely
  serialises work that could have run in parallel. `docs/**` on a task that writes one file
  blocks every other doc task in the batch.
- **`resources:`** `unreal-editor` for anything that builds or PIEs, `mcp-9000` for
  unreal-mcp, `pixellab-credits` for anything that generates. Credits are real money.
- **`score:`** file it, do not present it (§4). `feel` is the primary axis — how much this
  changes the moment-to-moment feel or gameplay — and the rubric is in `TEMPLATE.md`. An
  owner-brought feature is not automatically `feel: 3`; tooling and doc work that never
  shows up in play is `feel: 1` even when the owner asked for it out loud.
- **`model:`** leave it `""`. `dispatch --model` stamps it in §6, so the frontmatter records
  what was actually spawned rather than what was intended.
- **Spawn prompt** — self-contained and paste-ready. The teammate loads `CLAUDE.md` and the
  repo but **not this conversation**, so everything clarified with the owner has to be
  written into the prompt, along with the canon warnings and an explicit list of what it
  must not touch.

Then `py Scripts/backlog.py validate` and `reindex`. If validate reports a lock conflict,
fix it before presenting — narrow `owns:`, or add a `depends-on` and say so in the plan.

Give the batch a shared `epic:` slug when the features came in as one ask. It is what
`epic <slug>` rolls up afterwards, and it costs nothing.

## 3a · Work out what runs in parallel

**One task means nothing to schedule. Skip this section, say "one task, nothing to
schedule", and go to §4.** Running `waves` on a single id is ceremony.

For two or more, **ask the scheduler, do not reason about it**:

    py Scripts/backlog.py waves 63,64,65,66

It packs the batch into waves — wave 1 is everything dispatchable now, wave 2 is what
becomes dispatchable when wave 1 closes — and for every task that missed wave 1 it names
the specific thing holding it back. That per-task reason is the output; a wave plan
without it is just a list.

| Reason | What it means | What to do |
|---|---|---|
| `overlapping paths` | two tasks write the same place | re-cut the features (§1), or add a **join** |
| `both hold the 'unreal-editor' lock` | a real mutex; one thing drives the editor | nothing. It serialises. Say so |
| `waits on task-NNN` | a declared `depends-on` | nothing. That is the dependency working |
| `wave is full` | the width cap, not a conflict | raise it with `--max-width` if the lead can carry it |

**The join task.** Where features must converge on one file — `GDD.md`, `SYSTEMS.md`, a
shared DataTable — that write becomes its own task owning the shared file, with
`depends-on` naming every feature that feeds it. It is the only way several teammates land
in one file without overwriting each other; `dispatch` refuses it until they close, and
`waves` places it automatically. `task-038` is the precedent.

**Width.** `--max-width` defaults to 6. The ceiling is not the lock table — it is that the
lead carries every dispatch and every handback in *this* context. §7a moved the evidence
write-up out to a PR, which is what bought 4 → 6; dispatch and checking each task against
its evidence bar are still the lead's, which is why it is not 8. A wave of pure doc/spec
tasks carries `--max-width 8` fine. Prefer letting wave 2 exist over maxing the width.

**Resources cap width harder than files do.** `unreal-editor`, `mcp-9000` and
`pixellab-credits` are global mutexes, so specs fan wide and anything that builds, PIEs or
generates serialises. Five design specs is a genuine five-wide wave; five things that all
need the editor is a queue, and saying that plainly is more useful than dressing it as a fan.

## 4 · Present, and stop

Show one block per task. Everything needed to disagree in a sentence, nothing else — **no
score arithmetic.** The owner brought this feature; it is not being ranked against a queue.

```
task-044 · Flame flicker scales with army size          docs/backlog/task-044-….md
  you get     <the artifact that lands, in one line>
  agent       claude · sonnet builds
  touches     ELVTR/Source/ELVTR/Rendering/**, docs/perf/flame-flicker.md
  won't touch GDD.md, SYSTEMS.md, NS_Swarm.uasset
  needs       unreal-editor  (serialises with 065)
  why now     <two sentences>
  confidence  high — <the one thing that would drop it>
  open        <anything you assumed past — or omit the line>
```

**`confidence` is required and it is about the plan, not the outcome.** High means the
scope, the `owns:` set and the evidence bar all survived contact with the repo. Medium or
low means name what is soft — an unverified canon claim, an `owns:` glob you guessed, a
teammate that may not have the tools. The owner approves with one click, so the line that
tells them when *not* to is the one carrying its weight.

The `sonnet builds` half of the `agent` line is the last cheap moment to say "this one is
too fiddly for Sonnet". If you think the default build model is wrong for this task, say so
there and carry it into §5 as an option.

**For several features, group the blocks by wave** — the schedule is the headline:

```
epic flame-and-retinue · 4 tasks · 2 waves · sonnet builds

wave 1 — 3 teammates, in parallel, starting on approval
  task-063 · …    [block as above]
  task-064 · …
  task-065 · …

wave 2 — after wave 1 closes
  task-066 · Fold both results into SYSTEMS.md          ← join
       waits on 063, 064 (it owns SYSTEMS.md, they feed it)

serialised because   task-065 holds unreal-editor, so 067 could not join wave 1
you get back         <what lands when the epic closes>
```

Name the serialisation in the owner's terms. **Never present a wave the scheduler did not
produce** — if your prose and `waves` disagree, `waves` is right and the prose is a bug.

The batch is approved or refused whole. A plan the owner can only half-approve was cut
wrong — go back to §1 and re-split rather than offering per-task verdicts.

Then ask for the verdict (§5) and **stop**. Do not start work. Do not spawn. Do not read
approval into enthusiasm — "looks good" about the *shape* of a plan is not "go".

If the honest recommendation is that this should not be built — already done, better
waited on, needs a decision first — say that instead of presenting a plan. Recommending
work not happen is part of the job.

## 5 · The verdict — the one gate

**Ask with `AskUserQuestion`, in the same turn as the plan.** The owner should never have
to type a verdict. One question, header `Verdict`, `multiSelect: false`:

| Option | What it means | Then |
|---|---|---|
| `Approve all N & run wave 1` | go | §6, once per wave-1 task |
| `Approve, hold dispatch` | approved, not spawning yet | run `approve`, then stop |
| `Revise first` | the plan is wrong somewhere | ask what, redraft, re-present |
| `Don't build this` | `reject` or `park`, with a reason | ask which and why |

**The question is about the batch, not the tasks.** One click approves every task in the
plan and starts every task in wave 1 — put the number in the option label so the owner
knows how many teammates one click launches. Do not offer one option per task.

Name the build model in the recommended option's `description` — *dispatches to 3 Sonnet
teammates* — so approving is also approving the model. If §4 flagged a task wanting a
different model, that is a fifth option (`Approve & dispatch on Opus`), not something the
owner has to type.

Lead each option's `description` with your confidence and the reason for it — `High
confidence: locks are clean, evidence is a runnable build.` **Only put approve first when
you actually recommend it.** If the honest read is `Revise first`, that carries the
`(Recommended)` tag — a question whose first option is always "yes" is a rubber stamp with
extra steps.

The click is the owner's decision. **The record is still the hook.** `py
Scripts/backlog.py approve NNN` raises a permission prompt from `Scripts/backlog_guard.py`,
and **that prompt writes the verdict to `docs/backlog/LOG.md`**. `AskUserQuestion` sits in
front of that gate and never replaces it — never route around the prompt, never suggest
allowlisting it. Two clicks for an approval is the correct cost: the first is the decision,
the second is the signature.

If the owner answers in prose — "approve 1,3, park 2" — take it. The question exists to
make deciding cheap, not to insist on a format.

**A batch approves as one command** — `approve 63,64,65,66`, every task in the plan
including later waves. One prompt, one dry-run against the lock table, one batched `LOG.md`
write. Approving one at a time trains the owner to wave the gate through, which is the one
thing the gate cannot survive.

Approving a wave-2 task is safe and is the point — `approved` is permission, `dispatch` is
the schedule, and `dispatch` refuses anything whose dependencies are still open.

If the owner changes a score input instead, edit `score:` and re-run `reindex`. Score edits
are not privileged — argue freely.

## 6 · Dispatch, then post the board

Approval is the go signal here (unlike `/backlog`, where the owner spawns). Per task:

1. `py Scripts/backlog.py dispatch NNN --teammate <name> --model <build>` — records the
   teammate and model, moves to `in-progress`, logs it. It **refuses** unless the task is
   `approved` with every dependency closed. Run it *before* spawning: if it refuses,
   nothing has been spent.
2. Spawn with `Agent`: `subagent_type` = the task's `agent:`, `name` = the same `<name>`
   you recorded, `model` = the same `<build>`, `run_in_background` = true, `prompt` = the
   task's spawn prompt **verbatim**, prefixed `You are executing task-NNN. ` and suffixed
   with the handback instruction. `name` is what makes it addressable by `SendMessage`.

**The `--model` flag and the `model` argument are the same value, always.** The frontmatter
is the record a resumed session re-spawns from (§Never); a teammate spawned at a model the
file does not name makes that record a lie. If you dispatch without `--model`, spawn
without `model` too.

Teammate names come from the task slug — `flame-flicker`, `palette-lut` — must match
`[A-Za-z0-9][A-Za-z0-9_-]{0,63}`, and **one teammate per task**; `dispatch` refuses reuse.

For a fan, run both steps once per sibling — `dispatch` takes a single id by design. The
join refuses until its siblings close; that refusal is the schedule, not an error.

**Then post the board — once, as the last thing in the turn.** This is what stops the run
going dark:

```
running · epic flame-and-retinue · 3 of 4 dispatched

  task-063  flame-flicker    sonnet   → MPC_Flame drives off army size, PIE capture
  task-064  retinue-cap      sonnet   → retinue table in docs/data/, sim walkthrough
  task-065  hud-readout      sonnet   → UMG readout wired, screenshot

  task-066  join             waiting on 063, 064

say `/host status` any time · I'll post a one-liner as each lands
```

## 7 · Handback — receipt now, evidence at the close

You own the closing transitions, not the teammate — three of the five directors have no
shell and cannot run the script at all.

**As each teammate lands, post exactly one line and nothing more:**

    task-063 ✓ landed — MPC_Flame scales off army size, PIE capture attached · 2 of 3 in

or, when it missed:

    task-064 ✗ missed the bar — wrote the table but no simulation · sent it back

Read what it produced, check it against `evidence:`, run `py Scripts/backlog.py review NNN`,
and if it missed, `SendMessage` the teammate rather than filing a second task. **Do not
present the full evidence yet, and do not paste the teammate's report.** The receipt is
the whole interruption.

**When the batch closes, present the evidence once** — every task's artifact together, in
one block, with `done <ids>` as a single batched command and therefore a single prompt.
Big changes hand over as a runnable build or on-screen evidence, never a diff plus "it
works" (`show-a-build-for-review`). The owner launches the editor for review.

### 7a · Close the batch into one PR

**One branch per batch, one PR at the close — never one per task.** Teammates share the
working tree (`concurrent-sessions-share-the-tree`), so there is one branch to be on and
per-teammate branches are not available. `owns:` is what keeps their writes disjoint; the
branch is the batch's, not the teammate's.

The PR is a **review surface for work that already landed**, not the approval gate.
`approve` (§5) and `done` (§7) keep raising the `backlog_guard.py` prompt, and that prompt
is still what writes the verdict to `LOG.md`. Never route around it, never allowlist it.

After the batch reads `n/n closed`, stage **only the `owns:` paths of the tasks in the
batch** and open the PR:

```
git add <the owns: globs, named explicitly>      # never -A, never .
git commit
git push -u origin <branch>
gh pr create --title "<epic slug> · <n> tasks" --body-file <scratchpad>/pr-body.md
```

**`git add -A` is the landmine, and it is not hypothetical.** Another session shares this
tree — 218 modified paths were sitting in it when this section was written, none of them
this batch's. Stage the declared `owns:` paths by name. If something outside them needs
committing, that is a finding to report, not a path to sweep in.

The PR body is the §7 evidence block you were going to write anyway — one section per
task, what landed, and the evidence. Two things it must do that a diff cannot:

- **`*.uasset` and `*.png` are LFS**, so the PR diff renders them "binary file not shown".
  Link the blob URL for any capture or sprite instead of relying on the diff.
- **Name what was NOT verified.** A PR approval reads as "this is good"; if a task's
  evidence bar was met by a sim rather than a PIE run, or a capture showed geometry but
  cannot certify palette (`docs/AGENT-TEAMS.md` §8), say so in the body.

Post the PR URL in the same turn as the evidence block. The owner reviews it wherever they
like; nothing waits on it, and the batch is already closed either way.

A sibling that is `done` releases its `owns:` lock, which is what lets the join dispatch —
so close siblings as they land even though you present them together. `epic <slug>` carries
the running state in the meantime.

**A miss is evidence about the model, not just the task.** When a Sonnet teammate hands
back work that is wrong in kind — misread the architecture, solved the adjacent problem —
say so and offer re-running that task on Opus. Two Sonnet attempts at work that needed Opus
costs more than one Opus attempt. When a Sonnet teammate lands it clean, that is worth a
sentence too: it is what keeps the default honest.

## 8 · The end of a run

**A `/host` run has an end, and reaching it is a success.** When the batch reads `n/n
closed`, say so, name what landed, and **stop**. Do not sweep for the next thing, do not
draft follow-on tasks unprompted, do not roll straight into another epic — the lead's
context is now full of four teammates' build detail, which is the worst possible starting
condition for planning the next piece of work.

    epic flame-and-retinue · 4/4 closed. Flicker scales, retinue capped, HUD reads it,
    SYSTEMS.md folded. Fresh session for whatever is next — this one is carrying four
    build reports.

Follow-ups that surfaced during the run get filed as `proposed` tasks and mentioned in one
line. Filing is cheap; planning them here is not. See the `new-work-new-session` habit.

## `/host status`

Any turn the owner asks what is running — or says `/host status` — answer from the script,
not from memory:

    py Scripts/backlog.py list --status in-progress,needs-review
    py Scripts/backlog.py waves --approved-only        # what could start now
    py Scripts/backlog.py epic <slug>                  # if the batch is an epic

Print the §6 board shape from that output. It costs one turn and it is the answer to "what
is going on" — never reconstruct it from what you remember spawning, because `/resume` does
not restore in-process teammates and a `teammate:` stamp may name a ghost.

## `/host models`

The build model defaults to **sonnet** and is written to `.claude/host-models.json` on
first run without asking:

```json
{ "build": "sonnet" }
```

Read it, print one line — *sonnet builds — say `/host models` to change* — and move on.
**Only ask when the owner says `/host models`, passes `--models`, or names a model in the
goal.** Then one `AskUserQuestion`, header `Models`, two options that matter:

| Option | When |
|---|---|
| `Sonnet builds` | The default. The spec is written; the teammate is executing it. |
| `Opus builds` | The *build* is the hard part: Mass/Niagara internals, a perf hunt, anything where a wrong turn is expensive to unwind. |

Write the choice back to the file. It is sticky, not permanent — any turn the owner says
"use opus for this one", take it for that run without rewriting the file.

Planning always happens in the lead session, at whatever model the owner is on. There is
no "think model": a skill cannot change the session's model, and routing the plan through
a cold subagent to fake one costs a re-derivation of context the lead already has and a
clarify round to repair the assumptions it made. Use `/model` if you want to plan at Opus.

## Never

- Ask the owner more than two questions in one run, or ask about anything other than a
  clarification (§2) and the verdict (§5). Take the default and say which default you took.
- Spawn a teammate on a task that is not `approved`. `dispatch` enforces it; do not work
  around it by calling `Agent` first.
- Set `approved`, `done`, `rejected`, or `parked` by editing a file. The hook denies it and
  denying it is correct.
- Present a plan and start building in the same turn.
- Paste a teammate's full report into the lead. Receipt now, evidence at the close (§7).
- `git add -A` or `git add .` when closing a batch (§7a). Another session shares this tree;
  stage the batch's declared `owns:` paths by name and nothing else.
- Open one PR per task, or treat a PR approval as the gate that replaces `approve`/`done`.
  The `backlog_guard.py` prompt is the recorded verdict; the PR is a review surface.
- Give two live teammates overlapping `owns:` globs or the same resource. `validate` catches
  it; heed it rather than loosening the declaration.
- Widen a sibling's `owns:` to make a fan "work". An overlap means the cut was wrong: move
  the shared write into a join task. Loosening the declaration does not remove the
  collision, it just stops the script telling you about it.
- Message a teammate from a resumed session. `/resume` does not restore in-process
  teammates (`docs/AGENT-TEAMS.md` §5) — a `teammate:` stamp on an `in-progress` task may
  name a ghost. Re-spawn fresh with the same prompt **and the same `model:` the frontmatter
  names**.
- Spawn at a model the task's `model:` does not name, or quietly upgrade a build to Opus
  because the work looked hard mid-flight. Re-dispatching at a different model is the
  owner's decision, at handback (§7), with the failed attempt in front of them.
- Roll into the next epic after a batch closes (§8).
