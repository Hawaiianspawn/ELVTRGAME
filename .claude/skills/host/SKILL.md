---
name: host
description: Take one or more features the owner wants built, turn each into a lock-checked task, work out which of them are independent, and dispatch the independent ones to teammates in parallel once a reviewer agent passes the batch — no owner approval prompt. Use when the user runs /host, hands over a feature idea or a list of them to be planned and built, asks what can be worked on at the same time, asks to turn something into a task and get it started, asks to dispatch an already-approved backlog task, or asks what is currently running.
---

# host — features in, parallel teammates out

`/backlog` finds work already latent in the repo. `/host` is the front door for work the
owner *brings*: **write the features down as tasks, work out which are independent, run
the independent ones at once.**

    /host "flame flicker scales with army size; also the retinue cap needs tuning"
      → split the ask into features, one task each, and research each (§1)
      → clarify only what would change the work — as many rounds as it takes (§2)
      → draft a task per feature, validated and lock-checked (§3)
      → ask `waves` what can run at once, if there is more than one task (§3a)
      → present the plan, then put it to the quality gate (§4, §5)
      → gate passes → approve, dispatch wave 1, all at once, and post the board (§6)
      → one-line receipt as each lands; full evidence + a close button, once (§7)
      → commit and push what landed (§7a)
      → the run ends when the batch closes. Say so and stop (§8)

## Invocations

| You type | Do this |
|---|---|
| `/host <feature>` | The full flow above |
| `/host <feature A>; also <feature B>` | Same flow, split at §1, scheduled at §3a |
| `/host` *(nothing)* | **Do not open an empty prompt.** Run `py Scripts/paca.py list --status in-progress,needs-review` and `waves --approved-only` first. If anything is running, post the board (§6) — that is almost always what was meant. If nothing is running, name the top 3 approved-or-proposed tasks by score and ask which, as one `AskUserQuestion` |
| `/host status` | The board, from the script — see §`/host status` |
| `/host dispatch NNN` | The task is already approved: skip §1–§5 entirely, go straight to §6 |
| `/host models` | Ask, write `.claude/host-models.json`, stop |

## The interruption budget

This is the constraint the rest of the skill serves. **The close costs the owner at most
one question and one permission prompt, and never more. Approval costs zero — it is the
quality gate's job (§5). Clarify (§2) sits outside that cap — ask as many rounds as the
plan genuinely needs:**

| | What | When |
|---|---|---|
| Clarify | Uncapped — the good case is still skipping it entirely | §2 |
| Question 1 | **The close.** Did the evidence clear the bar | §7 |
| Prompt 1 | `done <ids>` — the signature that it landed | §7 |

**Every decision is a button, never typing.** If the owner has to choose — the close gate
(§7), a clarification (§2), which follow-up path a refusal takes,
what to do about a goal that is already filed — it arrives as an `AskUserQuestion` with
the real options as buttons. The owner should never have to compose a verdict in prose or
type an id list; the permission prompt behind a gate is the signature, not the decision.
Prose answers are always accepted, never required.

**A follow-up spends the same slot, not a new one.** When the owner clicks `Send NNN back`
or `Close some`, the next question is that decision continuing — ask it as buttons and it
does not breach the budget. Two questions to settle one close is fine; a second
*independent* question is not.

Anything else that wants an owner decision is a bug in the plan, not a question to ask.
Do not ask about models, do not ask the owner to confirm a feature split, do not ask
them to pick a score. Take the default, say which default you took in one line, and let
them correct you.

**Independence is computed, not judged.** Two tasks may run at once when they write
disjoint paths and share no resource — `py Scripts/paca.py waves` decides that from
the `owns:` and `resources:` you declared. Never eyeball it; a wave you reasoned your way
to is a wave `approve` may refuse.

**The backlog is Paca** — self-hosted at <http://localhost:8090>, project **Kindled**,
reachable as the `paca` MCP server and through `/paca`. Tasks keep their `task-NNN`
handle in the `Legacy ID` field, so every id in this skill and in the repo's prose still
resolves. `docs/backlog/` is a frozen archive of the 81 tasks that closed before the
move; the hook denies edits to it.

Judgment lives here. Everything mechanical lives in `Scripts/paca.py` — id allocation,
score math, lock conflicts, wave packing, and the privileged status transitions. Never
hand-compute a score, hand-set a status the script owns, or hand-plan a wave. Reading —
listing, searching, showing a task, comments — is the `paca` MCP server; `paca.py` exists
only for the parts Paca has no opinion about.

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

- Search Paca for the goal before drafting it — the `paca` MCP server's task search, or
  `py Scripts/paca.py list` and read the titles.
  **If the goal is already a task, say so and stop drafting.** Put the real choices up as
  an `AskUserQuestion`, header `Already filed` — re-score it, dispatch it if approved,
  supersede it deliberately. That question replaces §2; it is not an extra one.
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
not.** `paca.py new` allocates from the highest id in Paca, so two agents filing at once
will both be told `134` and one id will be reused. If you fan the research out, **have the
agents hand back drafts and file them from the lead, one at a time.**

## 2 · Clarify — ask what the plan needs

**No cap on rounds.** One `AskUserQuestion` per feature that is genuinely ambiguous, and
another round on the same feature if the first answer opens a new question. Batch
independent ambiguities into one question when they fit together, but do not force
unrelated features into a single question just to keep the count down — a forced merge
produces answers that do not map cleanly back to the feature they were about.

Ask only where different answers produce materially different tasks:

| Ask about | Because it changes |
|---|---|
| Scope boundary | `owns:` globs — and therefore what runs beside what |
| Which director executes | `agent:`, and therefore what tools the work can use |
| The evidence bar | `evidence:` — a runnable build vs. a written spec vs. a diff |
| Editor / credits | `resources:`, a hard mutex that serialises a wave |

Do not ask what the repo can answer. Do not ask the owner to pick a score. **If everything
is already unambiguous, skip this step entirely and say you skipped it** — that is still
the good case; asking more only when the plan actually needs it.

## 3 · Draft one task per feature

Write the draft to a JSON spec and file it with `py Scripts/paca.py new <spec.json>`,
which allocates the id and lands it at `proposed`. The spec is the old frontmatter, as
JSON — put the `## Why now` / `## Done when` prose in `body`:

```json
{"title": "…", "agent": "claude", "owns": ["docs/perf/**"],
 "resources": ["unreal-editor"], "depends-on": [8], "epic": "flame-and-retinue",
 "evidence": "…", "score": {"feel": 2, "risk": 1, "perf": 1, "cost": 2},
 "source": "user, 2026-08-01", "body": "## Why now\n…\n\n## Done when\n…"}
```

The spawn prompt goes in `body` too, under a `## Spawn prompt` heading — §6 reads it
back from the task. Field rules are in `docs/backlog/TEMPLATE.md` and
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
- **`model`** is not yours to set. `dispatch --model` stamps it in §6, so the task records
  what was actually spawned rather than what was intended.
- **Spawn prompt** — self-contained and paste-ready. The teammate loads `CLAUDE.md` and the
  repo but **not this conversation**, so everything clarified with the owner has to be
  written into the prompt, along with the canon warnings and an explicit list of what it
  must not touch.

Then `py Scripts/paca.py validate`. If it reports a lock conflict, fix it before
presenting — narrow `owns:`, or add a `depends-on` and say so in the plan.

Give the batch a shared `epic:` slug when the features came in as one ask. It is what
`epic <slug>` rolls up afterwards, and it costs nothing.

## 3a · Work out what runs in parallel

**One task means nothing to schedule. Skip this section, say "one task, nothing to
schedule", and go to §4.** Running `waves` on a single id is ceremony.

For two or more, **ask the scheduler, do not reason about it**:

    py Scripts/paca.py waves 63,64,65,66

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

**Width.** `--max-width` defaults to 4. The ceiling is not the lock table — it is that the
lead carries every dispatch and every handback in *this* context. Prefer letting wave 2
exist over running eight teammates at once.

**Resources cap width harder than files do.** `unreal-editor`, `mcp-9000` and
`pixellab-credits` are global mutexes, so specs fan wide and anything that builds, PIEs or
generates serialises. Five design specs is a genuine five-wide wave; five things that all
need the editor is a queue, and saying that plainly is more useful than dressing it as a fan.

## 4 · Present, then gate

Show one block per task. Everything needed to disagree in a sentence, nothing else — **no
score arithmetic.** The owner brought this feature; it is not being ranked against a queue.

```
task-044 · Flame flicker scales with army size                 localhost:8090 · KIND
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
teammate that may not have the tools. The reviewer (§5) reads this line first, and a soft
spot named here is a soft spot it can check instead of rediscover.

The `sonnet builds` half of the `agent` line is the last cheap moment to say "this one is
too fiddly for Sonnet". If you think the default build model is wrong for this task, say
so there — the owner can override in prose before dispatch.

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

The batch stands or falls whole. A plan that only half-passes the gate was cut wrong —
go back to §1 and re-split rather than dispatching the passing half around the failure.

Then run the quality gate (§5). Do not spawn anything until it passes — the presentation
is the owner's window to interject, and a prose interjection overrides the gate both ways.

If the honest recommendation is that this should not be built — already done, better
waited on, needs a decision first — say that instead of presenting a plan, and still ask
with `AskUserQuestion`: the recommendation first, the plan-anyway option second.
Recommending work not happen is part of the job; making the owner argue back in prose is not.

## 5 · The quality gate — an agent approves, not a prompt

**The owner is not asked and no permission prompt fires.** Approval is earned from a
fresh reviewer agent that judges the drafts cold, then recorded with `py Scripts/paca.py
approve <ids>` — the hook exempts `approve`; `reject`, `park` and `done` still prompt.

Spawn **one reviewer for the batch** — `Agent`, `subagent_type: claude`, fresh context.
It gets the task ids, the wave plan, and this bar; it does **not** get this conversation,
because a reviewer that inherits the drafter's reasoning inherits its blind spots:

- `validate` passes and the wave plan came from `waves`, not prose
- `owns:` covers every path the spawn prompt tells the teammate to write — and no more
- the evidence bar is checkable by reading the artifact, not by trusting the teammate's word
- the spawn prompt is self-contained: canon warnings, clarified answers, an explicit
  must-not-touch list
- the feature is not already built — grep canon (`GDD.md`, `SYSTEMS.md`, `CLASSES.md`)
  before passing this one
- `agent:` can actually do the work — tools and shell per §3's table

The reviewer returns **pass or fail per task, with the failing reason**. All pass →
`approve <ids>` as one batched command — every task in the plan including later waves —
and go to §6. A fail → fix the draft, re-run the reviewer on that task only. A fail that
needs the owner's *intent* to resolve — not a mechanical fix — is a §2 clarification, the
one case that still asks.

**The reviewer is never the lead.** The lead wrote the drafts; approving its own work is
the thing the old prompt existed to prevent. The reviewer's verdict in the turn plus
Paca's activity log is the audit trail the prompt used to be — quote the reviewer's
per-task reasons in the turn that approves, so the record of *why* survives.

Approving a wave-2 task is safe and is the point — `approved` is permission, `dispatch` is
the schedule, and `dispatch` refuses anything whose dependencies are still open.

An owner interjection in prose — "don't build 64", "hold dispatch" — overrides the gate
in both directions, before or after it runs. If the owner changes a score input, edit the
task's score fields in Paca. Score edits are not privileged — argue freely.

## 6 · Dispatch, then post the board

Approval is the go signal here (unlike `/backlog`, where the owner spawns). Per task:

1. `py Scripts/paca.py dispatch NNN --teammate <name> --model <build>` — records the
   teammate and model and moves the task to `in-progress`. It **refuses** unless the task is
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

Read what it produced, check it against `evidence:`, run `py Scripts/paca.py review NNN`,
and if it missed, `SendMessage` the teammate rather than filing a second task. **Do not
present the full evidence yet, and do not paste the teammate's report.** The receipt is
the whole interruption.

**When the batch closes, present the evidence once** — every task's artifact together, in
one block, with `done <ids>` as a single batched command and therefore a single prompt.
Big changes hand over as a runnable build or on-screen evidence, never a diff plus "it
works" (`show-a-build-for-review`). The owner launches the editor for review.

**Then ask for the close with `AskUserQuestion`, in the same turn as the evidence** —
header `Close`, `multiSelect: false`. The owner clicks; they never type an id list:

| Option | What it means | Then |
|---|---|---|
| `Close all N` | every task cleared its bar | `done <ids>`, one batched prompt |
| `Send NNN back` | one task missed | `SendMessage` that teammate, leave it `needs-review` |
| `Close some` | mixed | one follow-up `AskUserQuestion`, `multiSelect: true`, one option per task — the owner ticks what clears. `done` those, hold the rest |
| `Re-run NNN on Opus` | it missed *in kind*, not in detail | re-dispatch at Opus (§7 model note) |

Lead each `description` with what the evidence actually shows, and only
put `Close all` first when you actually believe it cleared. A close question whose first
option is always "yes" is the rubber stamp the bar exists to prevent. If the owner answers
in prose instead — "close 63 and 65, 64 needs another pass" — take it.

Offer `Re-run NNN on Opus` as a fifth option only when a miss is wrong-in-kind, never as
standing furniture.

A sibling that is `done` releases its `owns:` lock, which is what lets the join dispatch —
so close siblings as they land even though you present them together. `list --epic <slug>` carries
the running state in the meantime.

**A miss is evidence about the model, not just the task.** When a Sonnet teammate hands
back work that is wrong in kind — misread the architecture, solved the adjacent problem —
say so and offer re-running that task on Opus. Two Sonnet attempts at work that needed Opus
costs more than one Opus attempt. When a Sonnet teammate lands it clean, that is worth a
sentence too: it is what keeps the default honest.

## 7a · Commit and push what landed

**Closed work that is not pushed did not land.** After `done <ids>` clears, commit and push
in the same turn — no extra question, this is part of the close, not a decision:

    git add <the closed tasks' owns: paths>
    <commit with the `caveman-commit` skill, per CLAUDE.md>
    git push

Rules that bite:

- **Stage the `owns:` paths, never `git add -A`.** The tree is shared with other sessions
  (`concurrent-sessions-share-the-tree`) and carries unrelated modified assets. Sweeping
  them in commits someone else's work under your message.
- **One commit per batch**, not per task, unless the tasks are genuinely unrelated.
- If the push is refused — protected branch, no upstream, conflict — say so in one line and
  hand the owner the fix. Do not force, do not rebase the shared branch unasked.

**This applies to the skill itself.** Any change to `.claude/skills/host/SKILL.md` — a rule
added, a gate reworded — gets committed and pushed in the turn it is made, on its own
commit. A process improvement living only in one session's working tree is not a process
improvement.

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

    py Scripts/paca.py list --status in-progress,needs-review
    py Scripts/paca.py waves --approved-only           # what could start now
    py Scripts/paca.py list --epic <slug>              # if the batch is an epic

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

- Ask about anything other than a clarification (§2) and the close (§7) — plus the
  follow-up a partial close opens, which spends the same slot as the gate it follows.
  The close stays capped at one question; clarify is uncapped but still only for what
  changes the work. Take the default and say which default you took.
- Put a decision in prose. Every owner choice in this skill is an `AskUserQuestion` with
  the real options as buttons — the gates, the clarify, and every follow-up off them.
  Prose answers are accepted, never required, and "what would you like me to change?" is
  a question you should have already turned into options.
- Spawn a teammate on a task that is not `approved`. `dispatch` enforces it; do not work
  around it by calling `Agent` first.
- Set `approved`, `done`, `rejected`, or `parked` straight through the Paca API, the MCP
  tools, or the web UI. Those four go through `paca.py` — `approve` behind the reviewer
  gate (§5), the rest behind the hook prompt that is the owner's signature. Routing
  around either is the one thing the gate cannot survive.
- Run `approve` before the reviewer agent has passed the batch, or spawn the reviewer
  with the lead's conversation context.
- Paste a teammate's full report into the lead. Receipt now, evidence at the close (§7).
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
- Leave closed work uncommitted, `git add -A` in a shared tree, or edit this skill without
  committing and pushing that edit (§7a).
- Roll into the next epic after a batch closes (§8).
