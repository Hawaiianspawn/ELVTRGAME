---
name: cavemanhost
description: Same as /host — features in, lock-checked tasks out, independent ones dispatched in parallel after one approval — but every owner-facing summary is compressed to data blocks and fragments, and every task is scoped to the laziest rung that works. Use when the user runs /cavemanhost, or asks for host with terse output, compressed findings, less prose, or lazier task scoping.
---

# cavemanhost — host, compressed

**This is not a second host.** Read `.claude/skills/host/SKILL.md` and follow it in full —
every section, every gate, every script call, the interruption budget, the Never list.
Nothing about the *process* changes.

This file overrides two things only:

1. **How output reads** (caveman) — the owner-facing text.
2. **How tasks get scoped** (ponytail) — what teammates are told to build.

If this file and `host/SKILL.md` disagree on process, **host wins**. If they disagree on
prose, this file wins.

---

## 1 · The division of labour

| | Governs | Never touches |
|---|---|---|
| **caveman** | how findings and decisions are *said* | what gets built, what gets asked |
| **ponytail** | what teammates are told to *build* | how the owner is talked to |

Confusing these is the failure mode. Caveman does not make a task smaller. Ponytail does
not make a sentence shorter. They compose; they do not overlap.

## 2 · Caveman — the output register

Drop articles, filler, hedging, pleasantries. Fragments fine. **Every number, path, id,
delta and finding survives** — only connective prose dies. Never compress a technical term,
a CVar name, a file:line, or an error string.

**The rule that matters: if a data block already says it, do not say it again in a
sentence underneath.**

Before:

    task-118 ✓ landed — the line went in, rebuilt, re-ran. It went idle without
    reporting again, so I read the boards out of the log myself.

    Archers went from a flat 0 across all three waves to 3 → 33 → 303, unit1 from 0
    to 412, and the reconciliation stayed exact. That's the defaulted argument
    confirmed as the whole gap — the credit machinery was already correct.

After:

    task-118 ✓  (read from log; teammate went idle silent)
      wave  unit0  unit1  unit7  hero   sum   vs KilledBrood
      1     244    3      3      0      250   delta 0
      2     573    83     33     11     700   delta 0
      3     576    412    303    109    1400  delta 0
    archers 0 → 303. defaulted arg was whole gap. machinery fine.
    frame time ≤ baseline 1k..40k. Breather 7s.

Same facts. No paragraph explaining the table.

### Where the register applies

| host § | Shape |
|---|---|
| §4 present | keep the block. Kill `why now` prose → one fragment. `confidence` stays a full sentence — it is the line that tells the owner when *not* to click |
| §6 board | already terse. Unchanged |
| §7 receipt | one line. Already the rule; hold it |
| §7 evidence | data block per task. Numbers, paths, artifact URLs. No narration |
| `AskUserQuestion` | see §3 below |

### Where caveman switches OFF

Non-negotiable, straight from the caveman Auto-Clarity rule:

- **Destructive or irreversible actions** — force push, asset overwrite, editor restart,
  anything spending pixellab credits. Write those in full prose.
- **Security notes.**
- **Any sequence where dropped conjunctions make the order ambiguous.**
- **Commit messages, task files, spawn prompts.** These are code-adjacent artifacts read by
  people and teammates who do not have this context. Full prose, per `CLAUDE.md`.

A spawn prompt written in caveman is a bug — the teammate is not in this conversation and
needs every article you dropped.

## 3 · The gates stay buttons, and get shorter

`AskUserQuestion` still carries every decision (host's rule, unchanged). What changes is
the `description` field: **lead with the verdict-relevant fact, not a sentence introducing
it.** Two lines max.

Before:

    Both cleared their bars. 115 answered every 'done when' item incl. first-launch
    and CommonUI; 116 came back with Lost and the menu collision after one send-back,
    and its data contract is precise enough to file an engine task from verbatim.

After:

    115: all 'done when' items + CommonUI rec. 116: Lost + menu collision, one
    send-back. Contract filable verbatim. Artifacts up.

Option *labels* were already short. Leave them.

## 4 · Ponytail — task scoping

Applies when drafting a task (host §3) and writing its spawn prompt.

**Run the ladder on the feature before filing it.** Stop at the first rung that holds:

1. Does this need to exist? Speculative → say so in one line, do not file.
2. Already in the repo? Reuse beats rebuild. host §1 already makes you check
   `sweep-report` and canon — this extends it to *code*, not just filed tasks.
3. Stdlib / native engine feature covers it?
4. Already-installed dependency? No new ones.
5. One line? One line.
6. Only then: minimum that works.

**Two rungs work → take the higher one and file that.**

### What this changes in a task file

- **`owns:` gets narrower, never wider.** A narrow declaration that turns out short is
  fixable mid-flight (widen it, revalidate — this is normal). A wide one falsely
  serialises the whole batch.
- **Spawn prompts name the lazy path and forbid the elaborate one.** Not "build kill
  attribution" — "add one uint8 to the transient struct; do NOT build proportional-damage
  credit, the spec rejected it." Naming the rejected design is what stops a teammate
  rediscovering and building it.
- **Every non-trivial task leaves one runnable check.** Smallest thing that fails if the
  logic breaks. Not a suite. host's `evidence:` field already carries this — ponytail just
  says keep it to *one*.
- **Deliberate corners get a `ponytail:` comment** naming the ceiling and the upgrade path.
  Those get harvested by `/ponytail-debt` later.

### Where ponytail switches OFF

Never scope away: input validation at trust boundaries, error handling that prevents data
loss, security, accessibility, or **anything the owner explicitly asked for**. If the owner
wants the full version, that is the version — do not re-argue it in the plan block.

Also never lazy about *understanding*. The ladder shortens the solution, never the reading.
host §1's research step is not optional and does not get compressed.

## 5 · The ledger — what was decided, what is actually finished

**Every turn that changes state ends with this block.** Not prose, not scattered through
the reply. One block, last thing.

Two questions it must answer without the owner asking: *what did we decide*, and *what is
genuinely finished*.

### "Finished" is four states, not one

This is the ambiguity to kill. A task can be any of these and the words get used
interchangeably:

| State | Means | Owner can rely on it? |
|---|---|---|
| **landed** | teammate handed back, evidence read | no — not judged yet |
| **closed** | `done <id>` ran, script status is `done` | no — exists only in the working tree |
| **committed** | in a local commit | no — not shared |
| **pushed** | on the remote | **yes. this is finished** |

**Only `pushed` is finished.** Say `done` for the script status and `finished` only for
pushed — never mix them. `task-085` is the standing example: status `done`, evidence
approved, and its render refactor lived uncommitted in the working tree for days. That is
not finished and calling it done hid that.

### The block

```
FINISHED (pushed)
  eb7c8f6  task-118  squad kill credit + Breather 7s
  f6271c0  task-076  variance layer
  4ca0476  task-001  GDD-TODO corrected
  115f0ac  task-115 116  ui-showcase epic

RUNNING
  (none)

DECIDED THIS RUN
  owner  re-dispatch 001 + 076, 076 on Opus     both teammates had died silent
  owner  close 115+116, build 118 on Opus
  owner  merge BreatherSeconds into 118          one file, one rebuild
  lead   widened 118 owns: +SwarmProcessors.cpp  verified only AddToGrid call site
  lead   sent 116 back once                      Lost phase + menu collision unhandled
  116    board dismissed by menu summon          rejected blocking and z-order

OPEN / NOT MINE
  task-085 status=done but UNCOMMITTED           render split lives in working tree only
  ~1.5ms regression @1k brood vs one-camera-bench predates 118
  black frame: DitherWorldAnchor 0.378 vs canonical 1
```

Rules:

- **`FINISHED` carries the commit sha.** No sha, not finished. A closed-but-uncommitted
  task goes in `OPEN`, never `FINISHED`, and says so.
- **`DECIDED` names who decided.** `owner` / `lead` / a task id. The owner needs to see
  which calls were theirs and which I made on their behalf — those are the ones to
  overturn.
- **One fragment of reason per decision**, right-aligned. Not a sentence.
- **`OPEN / NOT MINE`** is the standing list of things found and deliberately not acted on.
  It is how a finding survives the run instead of scrolling away. Drop an item only when it
  is filed as a task or the owner waves it off.
- **Omit an empty section** except `RUNNING` — `RUNNING (none)` is load-bearing, it is how
  the owner knows nothing is silently in flight.

## 6 · The one thing that does not compress

**The evidence bar.** `show-a-build-for-review` still holds: big changes hand over as a
runnable build or on-screen evidence, never a diff plus "it works". Caveman compresses how
the evidence is *presented*, never how much of it there is.

A terse receipt over unverified work is worse than a verbose one — it reads as confidence.
When a teammate hands back without a report, say so in the receipt and state that you
verified it yourself:

    task-076 ✓  (teammate idle, no report — lead ran the bar)
