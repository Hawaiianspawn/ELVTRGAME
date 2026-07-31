---
id: 071
title: Close the loop — a committed balance baseline and a scheduled sweep that reports drift
status: done
agent: sim-director
model: sonnet
owns: ["Scripts/sim/**", "docs/sim/**"]
resources: []
depends-on: [69, 70]
epic: ""
evidence: A committed baseline of expected sweep results, a drift check that exits non-zero on a real regression, a demonstration of it catching a deliberately-introduced data change and then passing again once reverted, and a written recommendation on cadence with its token cost stated.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: baseline-drift
decided: "2026-07-29 done"
---

## Why now

The owner asked for the sweep to run **both** on demand and on a recurring schedule. The
on-demand half is `task-069`. This is the recurring half, and it needs something `069` does
not: a definition of what counts as a regression.

A scheduled run that prints numbers nobody reads is worse than no scheduled run — it burns
tokens on a cadence and trains everyone to ignore its output. The thing that makes it worth
running is a **committed baseline** plus a drift threshold, so a run is silent when nothing
moved and loud when a committed data file changed the curve.

This is the real payoff of the whole epic. `docs/data/*.json` is edited constantly by the
gameplay director — `entity-tiers.json`, `unit-types.json`, `economy.json`, `upgrades.json`,
`encounter-budget.json` are all live design surfaces. Today, an edit that quietly breaks the
scaling curve is caught only if someone thinks to re-run a scenario and happens to remember
what the number used to be. That is exactly the failure `docs/design/scaling-curve.md` §7
records: a wrong-in-kind model caught only because a measured number happened to exist to
check it against.

## Done when

- A committed baseline of expected sweep results lives in the repo, with each entry
  traceable to the command that produced it.
- A drift check compares a fresh sweep against that baseline and **exits non-zero on a real
  regression**, so it can gate rather than merely report.
- The threshold is stated and justified. A check that fires on floating-point noise gets
  muted within a week; one that only fires on a catastrophic break never fires at all. Say
  what was chosen and why.
- **Demonstrated end to end**: deliberately perturb a value in a committed data file, show
  the check catching it with a useful message, revert, show it passing. A drift check that
  has never actually caught anything is untested code.
- Baseline refresh is a deliberate, documented act — not something a failing run does to
  itself. A baseline that auto-updates on failure is not a baseline.
- A written recommendation on cadence with **the token cost stated plainly**. A scheduled
  agent run is billed every time it fires; the owner should see that number before choosing
  a frequency. Recommend, do not assume.
- The `LIMITATIONS.md` §1 boundary holds: a wave-attrition number may be baselined for
  *drift* (has this changed?) but must never be presented as *correct* (is this right?).
  Those are different claims and the output must not blur them.

## Spawn prompt

```
You are executing task-071 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.
You are the sim-director. This is the last task of the sim-tuning-loop epic, and it depends
on task-069 (sweep runner) and task-070 (scenario library), both of which are CLOSED before
you start. Read what they landed before writing anything — do not rebuild what exists.

THE JOB: the owner wants the sweep to run on a recurring schedule as well as on demand.
task-069 built the on-demand half. This is the recurring half, and the hard part is not
scheduling — it is defining WHAT COUNTS AS A REGRESSION.

A scheduled run that prints numbers nobody reads is worse than nothing: it burns tokens on a
cadence and teaches everyone to ignore it. What makes it worth running is a committed
baseline plus a stated drift threshold — silent when nothing moved, loud when a committed
data file changed the curve.

READ FIRST:
  Scripts/sim/** and docs/sim/**   -- everything task-069 landed, especially the sweep
                                      command and its --json output shape
  docs/data/scenarios/**           -- what task-070 landed
  docs/sim/LIMITATIONS.md          -- §1 and §3. This bounds what you may baseline.
  docs/design/scaling-curve.md §7  -- the cautionary tale this whole harness exists for

YOU OWN:
  Scripts/sim/**
  docs/sim/**

DO NOT WRITE ANYTHING ELSE. docs/data/**, ELVTR/**, and .claude/** are all out of bounds.
You may TEMPORARILY perturb a docs/data/*.json value to demonstrate the check catching it —
you MUST revert it in the same session and confirm the revert in your handback. Leaving a
perturbed balance file behind would silently corrupt design canon.

BUILD:
1. A committed baseline of expected sweep results, each entry traceable to the command that
   produced it.
2. A drift check that compares a fresh sweep to the baseline and EXITS NON-ZERO on a real
   regression, so it can gate rather than only report.
3. A stated, justified threshold. Too tight and it fires on floating-point noise and gets
   muted within a week; too loose and it never fires. Say what you chose and why.
4. Documentation of how a baseline gets REFRESHED — as a deliberate, explicit act. If a
   failing run can update its own baseline, it is not a baseline. Make that impossible or
   at minimum loud.

DEMONSTRATE IT END TO END. Perturb a real value in a committed data file, run the check,
show it failing with a message that identifies WHAT moved and BY HOW MUCH, revert, run
again, show it passing. Commit that transcript. A drift check that has never caught anything
is untested code, and I will send it back if the demonstration is missing.

THE BOUNDARY THAT MATTERS: LIMITATIONS.md §1 says the wave-attrition model does not
reproduce its one measured baseline. You may baseline a wave-attrition number for DRIFT
("has this changed?") but never present it as CORRECT ("is this right?"). Those are different
claims. Your output must not blur them, and the docs must say which one the baseline makes.

CADENCE: recommend one, and STATE THE TOKEN COST PLAINLY — a scheduled agent run is billed
every time it fires, and the owner should see that number before picking a frequency. Do not
create any scheduled job, cron entry, or routine yourself; the owner decides whether and at
what frequency. Write the recommendation and stop.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: the baseline format, the threshold and its justification, the full
perturb/catch/revert transcript, explicit confirmation that the perturbed data file was
reverted, and your cadence recommendation with its cost. State plainly anything you could
not do.
```
