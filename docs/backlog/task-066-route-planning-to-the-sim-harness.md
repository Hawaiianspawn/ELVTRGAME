---
id: 066
title: Route gameplay-validation work to the sim harness instead of PIE by default
status: done
agent: claude
model: sonnet
owns: [".claude/skills/host/SKILL.md", ".claude/skills/backlog/SKILL.md", "docs/backlog/TEMPLATE.md"]
resources: []
depends-on: []
epic: ""
evidence: A diff across the three planning docs adding sim-director to the agent roster and a stated rule for choosing sim vs. PIE, plus a worked example showing one already-filed task whose evidence bar should have been a harness run — and the honest counter-example where PIE is still correct.
score: {feel: 1, risk: 1, cost: 1}
source: user
teammate: sim-routing
decided: "2026-07-29 done"
---

## Why now

`task-063` landed a committed simulation harness — `Scripts/sim/`, `docs/sim/`,
four scenarios under `docs/data/scenarios/`, and a `sim-director` agent — and
**nothing in the planning flow knows it exists.** Grepping `sim-director`,
`Scripts/sim` and "simulation harness" across `.claude/skills/host/SKILL.md`,
`.claude/skills/backlog/SKILL.md` and `docs/backlog/TEMPLATE.md` returns zero
hits in all three. `sim-director` appears in exactly three files repo-wide: its
own definition, `INDEX.md`, and `task-063`'s file.

The consequence is live, not hypothetical. `TEMPLATE.md:82`'s `agent:` comment
still reads `gameplay|narrative|performance|pixel-art|ui-director, or claude` —
a roster that predates the sim director — so a task needing a numerical
gameplay check gets routed to a director who reasons it out in prose, or to
`claude` holding the `unreal-editor` lock. `task-004` was dispatched on
2026-07-29 with an evidence bar reading *"plus a simulated floor walkthrough"*
and a spawn prompt asking for a prose `## Simulation notes` section, with no
mention of the harness that had landed the day before. That was corrected
mid-flight by `SendMessage`, which is not a process.

The `unreal-editor` lock is the practical cost. It is a global mutex that
serialises every task holding it, so each gameplay question answered by PIE
that could have been answered by a scenario run narrows every wave it appears in.

## Done when

- `TEMPLATE.md`'s `agent:` line names `sim-director` with a one-line scope note,
  and the `resources:` comment says plainly that a harness run holds **no** lock
  while a PIE run holds `unreal-editor`.
- `/host` §1 and `/backlog`'s task-writing section carry a stated rule for
  choosing between the two. The rule must be specific enough to apply without
  judgment on the common cases, and it must be **honest about the harness's
  limits** rather than routing everything to sim:
  - `docs/sim/LIMITATIONS.md` §3 — the **point-target** model (army vs. a single
    Elite/Titan/Boss) is validated and reproduces `entity-tiers.md` §7 exactly.
    Trustworthy.
  - `docs/sim/LIMITATIONS.md` §1 — the **wave-attrition** model does NOT
    reproduce `GATE1-FUN-PROTOTYPE.md`'s measured ~110-of-120 wave-1 survival.
    A survivor count out of it is not a prediction and must not be filed as an
    evidence bar as though it were.
  - `docs/sim/LIMITATIONS.md` §4 — stances, leash, supply/degrade, items,
    knockback, positioning and multi-wave carryover are **not modelled at all**.
    Anything turning on those is a PIE question and the rule must say so.
- The dependency direction is written down, because it is counter-intuitive and
  was nearly got backwards: `docs/sim/LIMITATIONS.md` §2 names the per-floor
  encounter budget table (`task-004`) as the deliverable that would supply the
  arrival/spawn-pacing data the wave model is missing. Design tasks feed the
  harness as much as the harness checks them.
- A worked example each way: one filed task whose evidence bar should have been
  a harness run, and one that is correctly PIE (`task-008`'s five feel questions
  are the obvious candidate — "does the hero feel like a commander or a camera
  with a sword" is not a number).

## Spawn prompt

```
You are executing task-066 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

THE GAP: task-063 landed a committed simulation harness (Scripts/sim/, docs/sim/,
docs/data/scenarios/, and a sim-director agent). The planning workflow does not know it
exists. Verify this yourself first — grep for "sim-director", "Scripts/sim" and
"simulation harness" across .claude/skills/host/SKILL.md, .claude/skills/backlog/SKILL.md
and docs/backlog/TEMPLATE.md. At time of filing all three returned zero hits.

Consequence: TEMPLATE.md:82's agent: comment lists a director roster that predates the sim
director, so tasks needing a numerical gameplay check get routed to prose reasoning or to
a teammate holding the unreal-editor lock — a global mutex that serialises waves.

READ BEFORE WRITING, all four, in full:
  docs/sim/README.md         -- what the harness is and how it runs
  docs/sim/LIMITATIONS.md    -- THE IMPORTANT ONE. §1 §2 §3 §4 all bear on the rule you write
  .claude/agents/sim-director.md  -- scope, and what it explicitly refuses to do
  docs/backlog/task-063-game-simulation-harness-and-agent.md

YOU OWN EXACTLY:
  .claude/skills/host/SKILL.md
  .claude/skills/backlog/SKILL.md
  docs/backlog/TEMPLATE.md

DO NOT WRITE ANYTHING ELSE. In particular do NOT edit Scripts/sim/**, docs/sim/**,
docs/data/scenarios/**, or .claude/agents/sim-director.md — those are sim-director's and it
is not running. If you conclude the agent definition needs a change, write that up as a
finding and hand it back.

WHAT TO WRITE: a rule for choosing sim vs. PIE when filing a task, in TEMPLATE.md and in
both skills' task-writing sections, plus sim-director added to TEMPLATE.md's agent: roster.

THE RULE MUST NOT BE "prefer sim". That would be worse than the status quo, because it would
route wave-attrition questions to a model that LIMITATIONS.md §1 states plainly does not
reproduce the one measured baseline it can be checked against. Be specific:
  - point-target questions (army vs. one Elite/Titan/Boss): sim is validated, use it
  - wave-attrition survivor counts: scaffold only, never file one as an evidence bar
    without §1's caveat attached
  - stances, leash, supply/degrade, items, knockback, positioning, multi-wave carryover:
    not modelled at all (§4), these are PIE questions
  - anything about feel, readability, or what something looks like: PIE or the owner,
    never sim
Also record the dependency direction from §2: design deliverables (notably the per-floor
encounter budget table, task-004) supply data the harness is missing. It is not one-way.

MATCH THE HOUSE STYLE. Both SKILL.md files are written in a specific voice — dense tables,
imperative headers, "Never" sections, prose that states a reason rather than a rule alone.
Read enough of each to write in it rather than bolting on a section that reads foreign.
Keep the addition tight; these are working documents, and length is a real cost.

INCLUDE TWO WORKED EXAMPLES from tasks already on the board: one whose evidence bar should
have been a harness run, and one that is correctly PIE. task-008 (Play Gate 1 — "does the
hero feel like a commander, or a camera with a sword") is the obvious PIE case. Pick the
other yourself by reading docs/backlog/INDEX.md.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.
DO NOT edit any task-NNN file, including this one.

HAND BACK: the diff you made, the rule you landed on in three or four sentences, and
anything you found that suggests the rule is wrong or incomplete. If you conclude the
routing gap is smaller or larger than this task assumes, say so plainly.
```
