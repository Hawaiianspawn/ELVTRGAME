---
id: 067
title: Retire the retracted parameter-sweep claim from the sim-director definition
status: done
agent: claude
model: sonnet
owns: [".claude/agents/sim-director.md"]
resources: []
depends-on: []
epic: ""
evidence: sim-director.md's §"one rule that matters" paragraph restated to match LIMITATIONS.md §1's corrected finding — the 27-cell sweep with 15 wins and the two undisentangled candidates — with the retracted "entire documented parameter range" wording gone.
score: {feel: 1, risk: 1, cost: 1}
source: user
teammate: simdir-claim-fix
decided: "2026-07-29 done"
---

## Why now

`.claude/agents/sim-director.md:51` states the wave-attrition model fails to reproduce
GATE1's measured survival *"across its entire documented parameter range (see
`VALIDATION.md`'s sensitivity sweep)"*.

`docs/sim/LIMITATIONS.md` §1 retracts that exact sentence. Verbatim: *"The original
version of this section reported the failure as robust 'across the harness's entire
documented parameter range' — that claim was wrong; the sweep behind it could not have
shown anything else, by construction."* The sweep was invalidated by a structural bug —
cleave capacity was derived from the same bound as the incoming-attacker cap, which
cancelled `TargetsPerHit`'s effect to a constant. With the bug fixed, a real 27-cell sweep
has the retinue **winning in 15 of 27 cells**, driven almost entirely by
`MaxAttackersPerUnit`. The model is capable of the correct qualitative outcome; it just
loses at committed defaults.

So the agent definition contradicts the document it instructs the agent to read, and the
contradiction is not cosmetic — it is the difference between "this model is structurally
broken" and "this model is right in kind and missing one input". Found by the `task-066`
teammate while reading both for the routing rule, and correctly left alone as outside its
file boundary.

The cost is compounding now that `task-066` has landed a routing rule that sends numeric
gameplay claims to this agent: a planner or teammate reading `sim-director.md` alone gets
the retracted version and will over-discount a wave-attrition result that `LIMITATIONS.md`
treats as a scaffold worth building on. `LIMITATIONS.md` §2 is explicit that the scaffold
plugs in an arrival-rate term without restructuring — that is a live path, and the stale
sentence reads as though it were a dead end.

## Done when

- The paragraph at `sim-director.md:46-63` states the corrected finding: bug found and
  fixed, 15-of-27 cells win, `MaxAttackersPerUnit` dominates, still loses at committed
  defaults, best untested cell reaches ~53 of the measured 109-111.
- Both open candidates survive the edit — arrival/spawn-pacing timing, and
  `MaxAttackersPerUnit`'s pooled-vs-per-entity transfer — along with the plain statement
  that the harness cannot currently distinguish between them.
- The prohibition the paragraph exists to carry is **not** weakened: do not tune
  `EngagedSpacingUU`, `MaxAttackersPerUnit` or `MeleeContactFacingFraction` to force check 3
  to pass. `LIMITATIONS.md` §1's closing point — that some off-default combination
  technically could be found that passes, and that finding one would trade a bad number for
  a worse one with no citation behind it — is the part most easily lost in a rewrite and is
  the whole reason the paragraph exists.
- `docs/sim/**` is unchanged. `LIMITATIONS.md` is already correct; this is the agent
  definition catching up to it.

## Spawn prompt

```
You are executing task-067 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

A stale, explicitly-retracted claim is sitting in the sim-director agent definition.

.claude/agents/sim-director.md:51 says the wave-attrition model fails to reproduce GATE1's
measured survival "across its entire documented parameter range (see VALIDATION.md's
sensitivity sweep)". docs/sim/LIMITATIONS.md §1 retracts that exact wording and explains
why: a structural bug (cleave capacity derived from the same bound as the incoming-attacker
cap, cancelling TargetsPerHit's effect to a constant) meant the original sweep could not
have shown anything else. The corrected 27-cell sweep has the retinue WINNING in 15 of 27
cells. The model still loses at committed defaults, and its best untested cell only reaches
~53 of the measured 109-111 survivors.

READ FIRST, in full and in this order:
  docs/sim/LIMITATIONS.md    -- §1 especially; it is the corrected source of truth
  docs/sim/VALIDATION.md     -- "The bug that invalidated the original check-3 sweep"
  .claude/agents/sim-director.md  -- the file you are fixing, all of it

YOU OWN EXACTLY ONE FILE:
  .claude/agents/sim-director.md

DO NOT edit docs/sim/**, Scripts/sim/**, docs/data/scenarios/**, the backlog skills, or any
task-NNN file. docs/sim/LIMITATIONS.md is ALREADY CORRECT — it is the reference, not the
target. If you find yourself wanting to change it, you have the direction backwards.

REWRITE the "## The one rule that matters more than any other" paragraph so it states the
corrected finding. Keep all of:
  - the bug was found and fixed; the original sweep was invalid by construction
  - 15 of 27 cells win; MaxAttackersPerUnit dominates the outcome (strict=1 always wins,
    shipped default=4 always loses)
  - it still loses at committed defaults, best untested cell ~53 vs measured 109-111
  - BOTH open candidates: arrival/spawn-pacing timing, and MaxAttackersPerUnit's
    pooled-vs-per-entity transfer — and that the harness cannot distinguish them
  - the point-target model remains validated and trustworthy within LIMITATIONS.md §3

DO NOT WEAKEN THE PROHIBITION. The paragraph exists to stop a future agent nudging a
constant until the output looks plausible. Every dial in combat-model-constants.json is
cited, not tuned. LIMITATIONS.md §1 is explicit that some off-default combination could
technically be found that passes check 3, and that finding one would be worse than failing —
a passing check with no citation behind the value that produced it. That point must survive
your edit, stated at least as strongly as it is now. If your rewrite makes the model sound
more trustworthy without making it more trustworthy, you have made the file worse.

Keep the file's voice — direct, plain statements, no hedging. Do not lengthen it materially;
this is a correction, not an expansion.

DO NOT run `py Scripts/backlog.py` — the lead session owns backlog transitions.

HAND BACK: the diff, and confirmation that the anti-tuning prohibition is still present and
at least as strong. If you find any OTHER stale claim in the file while you are in there,
report it rather than fixing it unless it falls inside the same paragraph.
```
