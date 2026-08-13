# Preflight — what has to be true before the pivot gets built

**Opened:** 2026-08-13 · **Status:** proposal, nothing here is an owner call yet
**Reads:** `docs/OPEN-DECISIONS.md` (the register this feeds), `docs/design/castle-layout.md`,
`docs/design/intro-and-zones.md`, `docs/sim/LIMITATIONS.md`, `docs/perf/one-camera-bench.md`,
`docs/backlog/TEMPLATE.md` (the scoring rubric and lock rules this plans against)

---

> ## What this is
>
> The pivot has a design (`castle-layout.md`), an opening (`intro-and-zones.md`) and a
> decision register (`OPEN-DECISIONS.md`) with Tier 0 closed. This is the layer between
> those and the first line of code: **which of the twenty open decisions actually block
> building, which ones don't, which ones aren't decisions at all — and one finding that
> should probably reorder the whole plan.**
>
> §4 is a proposed task fan with the lock analysis already done, cut so `owns:` globs are
> disjoint per `AGENT-TEAMS.md` §3. It is meant to be handed to `/host`.

---

## 1. The finding: the premise sits on the untrustworthy half of the harness

**This is not a new discovery about the harness.** `docs/backlog/TEMPLATE.md` already
states the evidence bar plainly, and has since before the pivot:

> **Point-target** questions — army or hero DPS vs one Elite/Titan/Boss, TTK, breakpoints —
> go to `sim-director`: the point-target model is validated, reproducing `entity-tiers.md`
> §7's own table exactly. **Wave-attrition** questions — swarm-vs-swarm survivor or casualty
> counts — are the opposite case: `docs/sim/LIMITATIONS.md` §1 states the harness does not
> currently reproduce the one measured baseline it's checked against (GATE1's 110-of-120).
> **A wave-attrition run is a scaffold, never a standalone evidence bar.**

**What changed is not the harness. It is what the harness is being asked to carry.**

Before the pivot, the wave-attrition gap was a balance-tooling debt: annoying, scoped,
documented, and nothing load-bearing depended on it. After the pivot, `castle-layout.md` §1
makes **"the simulation's default output is a held line"** the premise the entire design
rests on — the thing that makes stalemate a floor rather than a failure, that answers
`FLAME-FOUNDATION.md` §4.1's standing-still risk, and that justifies bosses existing at all.

**That claim is a wave-attrition claim.** It is swarm-versus-swarm, and it is exactly the
question `LIMITATIONS.md` §1 says the harness cannot currently answer — the model predicts a
full retinue wipe where the engine measured ~110 of 120 surviving.

So the pivot silently promoted a known scaffold-only model to carrying the central premise.
**Nobody did anything wrong; the status of the debt changed underneath it.** That is worth
naming loudly, because the failure mode is quiet: a sweep gets run, it produces numbers, the
numbers get quoted, and the caveat falls off somewhere between the run and the decision.

### The useful half of this

**The point-target model — the validated one — is exactly the model beat A7 needs.** Seven
soldiers versus one marked boss is a point-target problem. `entity-tiers.md` §4's 35–55
surround cap, TTK, whether marks meaningfully change time-to-kill, whether seven inside the
cap really are "not meaningfully worse than seventy" (`castle-layout.md` §6.3) — **all of
that is answerable in the harness today, at the validated evidence bar.**

That is the opposite of the intuitive build order, and it should probably set it (Q28).

### The named, untested candidate

`LIMITATIONS.md` §1 has already eliminated one explanation and named the survivor:

- **Candidate (1), arrival/spawn pacing — TESTED (task-068), does not close the gap.** Real
  timing data from shipped CVars; the fight is delayed, not changed.
- **Candidate (2), `MaxAttackersPerUnit` not transferring** from a per-victim instantaneous
  bound in the real sim to an aggregate rate multiplier across an exposed perimeter in the
  pooled model. **Untested, and now the stronger explanation by elimination.**

This matters for scoping: closing the gap is **not open-ended research.** There is one named
hypothesis and it needs one in-engine measurement to confirm or kill. If the measurement
kills it, that is a cheap early exit and the honest answer becomes "we do not know why," at
which point Q27 should be re-taken rather than ground at.

---

## 2. Triage — what actually blocks code

Twenty decisions are open. Most of them do not block building anything.

### 2.1 Blocks first code — from the register

| Q | Why it blocks | Can a provisional answer unblock it? |
|---|---|---|
| **Q23** ability kit | Q13 = C means the player has **no** attack. There is literally nothing player-facing to implement without a kit. | **Yes** — a provisional kit is fine and expected. What must not happen is the provisional one becoming the decision by default. |
| **Q15** down / revive | Changes the squad state machine. Cheap now, expensive to retrofit once combat and animation assume one answer. | No. Pick one. |
| **Q16** what ends A7 | The encounter needs a terminating condition or it cannot be scripted at all. | No, and it is a small call. |
| **Q14** cap or floor | Only insofar as it decides whether a roster data structure exists. Fixed-seven needs no roster. | **Yes** — build fixed-seven, add roster later, *if* nothing hard-codes the count. |

### 2.2 Blocks first code — not in the register until now

These are §3's new decisions. **Two of them block harder than anything above**, because they
are architecture and they are expensive to reverse.

| Q | Why it blocks |
|---|---|
| **Q24** ledger authority | Determines whether "marks earned identically in all three bands" is *possible*. Get it wrong and beat A9's consequence is a lie the player eventually catches. |
| **Q25** squad as Mass or Actors | Decides whether the seven go down the crowd path at all — separate combat, animation and rendering code if not. |
| **Q26** control scheme | Distinct from Q23. Q23 is *what* the verbs are; this is *how they are issued*. Most likely of anything here to be decided by accident. |

### 2.3 Looks like a blocker, is actually an output

**These cannot be decided in advance and should not be.** Deciding them now is guessing with
extra steps, and it forecloses the evidence the first build exists to produce.

- **Q5 — the `Breaking` → `Fallen` window.** *The* pacing number for the game, and it is
  measured from traversal times once zones exist. **This is what a first prototype is for**,
  not an input to it.
- **Q8 — zone counts and sizes.** Follows from Q5 and Q12. Author provisionally; tune.
- **Q9 / Q17 / Q18 / Q10 / Q19** — all want playtest evidence or a second siege to exist.

### 2.4 Not decisions at all — measurements

- **Q12 — what a Warm zone costs, and whether drip-spawn scales linearly.** The whole zone
  model rests on two figures inferred by dividing measured batch numbers. `-SwarmBench`
  exists. **This is the cheapest high-value thing on the list.**
- **The `MaxAttackersPerUnit` transfer question** (§1). Also an in-engine measurement.

> **Both need the `unreal-editor` resource, so they serialise** (`AGENT-TEAMS.md` §5 — only
> one thing drives the editor). Plan for that rather than discovering it at dispatch.

---

## 3. The five decisions that were not in the register

Proposed as **Q24–Q28**, in the register's format. None of these is an owner call yet.

---

### Q24 — Is the front ledger authoritative, or derived?

**Tier 0-equivalent. Blocks: Q8, the whole streaming model, and the truth of §6.2's claim.**

`castle-layout.md` §5.1 defines a four-number ledger per front. §7 defines Live / Warm /
Cold bands. **Neither says which one is the source of truth**, and the answer decides whether
the simulation is one thing or two.

- [ ] **A · Derived.** The ledger is a read-only view computed from what entities are doing.
      Maximally truthful — and **impossible for Cold**, which has no entities to derive from.
      Fails on its own terms.
- [ ] **B · Authoritative always.** The ledger is the sim; entities are a rendering of it.
      Cheap, uniform, trivially consistent across bands — and **it kills the game**, because
      what the player does in a Live fight then cannot move the front.
- [ ] **C · Hybrid, by band.** **Live:** entities are authoritative and write back to the
      ledger. **Warm:** the ledger is authoritative and drives entities to match. **Cold:**
      ledger only. Conversion happens at promotion and demotion.

> **Recommendation: C**, because A and B are both disqualified by their own consequences
> rather than by preference. But C's cost must be stated up front, because it is where this
> will actually break:
>
> **The two handoffs are the hard part, and they are where the marks-parity rule lives.**
> Promotion (Cold → Warm) has to *synthesise* entities consistent with a ledger state that
> never had entities. Demotion (Live → Warm) has to *reconcile* a specific entity population
> back into four numbers. Neither is lossless, and `castle-layout.md` §7's rule — marks must
> be earned identically in all three bands — is a constraint on those two functions
> specifically, not on the bands.
>
> **Falsification test, and it is easy to run:** leave a Live fight at roughly even, come
> back. If the ledger resolved it somewhere the Live sim plainly would not have, the
> simulation is two different things and the player will find that out before you do.

---

### Q25 — Are the seven Mass entities or promoted Actors?

**Blocks: squad combat, animation, rendering, and Q21.**

- [ ] **A · Promoted Actors**, on the same grid-bridge the hero already uses
      (`HeroMeleeRangeSq`, `FindOwnGridEntry`, `SwarmCombatProcessors.cpp`).
- [ ] **B · Mass entities** with a per-unit uniqueness side-channel.
- [ ] **C · Hybrid** — Actors that register into the Mass grid for combat queries only.

> **Recommendation: A, and it is very close to forced.** The seven need names, persistent
> identity, adaptation rungs, distinct verbs (Q13 = C), and individual down-state. That is
> per-unit uniqueness, which **Design Law 5 forbids for Mass** (`entity-tiers.md` §1). Eight
> Actors is nothing — `entity-tiers.md` already promotes Elite, Titan and Boss on exactly
> this pattern and calls the hero bridge the precedent that de-risks it.
>
> **The cost, stated so it is chosen rather than discovered:** the squad does not go down the
> crowd path. Separate animation, separate rendering, separate combat code from the hundreds
> around them — and **they still have to read as being *of* the same world**, which is
> precisely what makes Q21 (variant-family with pinned seeds) the right art answer rather
> than bespoke.

---

### Q26 — How are orders issued?

**Blocks: any player-facing prototype. Must be answered *with* Q23, not after it.**

- [ ] **A · Direct target.** Click a thing; the appropriate soldier acts on it.
- [ ] **B · Select then order.** RTS-conventional; explicit, slower, reads as strategy.
- [ ] **C · Contextual single button.** One input, meaning resolved by what is under the
      cursor and who is available.
- [ ] **D · Radial / verb wheel.** Explicit verb selection, then a target.

> **No recommendation — this is a feel question and it needs hands on it, not an argument.**
> What is worth stating: **Q23 and Q26 are one decision wearing two hats.** A kit that lives
> in the soldiers (Q23 = B) wants A or C; a fixed player kit (Q23 = A) wants D. **Answering
> them in separate sessions is how a control scheme gets decided by accident.**

---

### Q27 — How does the stalemate premise get validated?

**The decision §1 exists to force.**

- [ ] **A · Close the wave-attrition gap first.** Measure the `MaxAttackersPerUnit` transfer
      in-engine, fix the pooled model against it, then validate the premise in the harness —
      headless, repeatable, sweepable, and guarded forever after by `drift_check.py`.
- [ ] **B · Validate in PIE only.** Fast, matches the existing bar for feel questions
      (`TEMPLATE.md`: stances, leash, positioning "stay PIE"). Not repeatable, not sweepable.
- [ ] **C · Accept it untested.** Build, and let the first playable be the test.

> **Recommendation: A — but the argument is specific, not a preference for rigour.**
>
> The premise is not "does this fight feel good." It is **"does the sim's default output sit
> at Holding across a parameter range."** That is a sweep question by construction, and
> `sweep.py` and `drift_check.py` already exist to answer sweep questions. **B cannot answer
> it** — one PIE session tells you about one parameter set, and the claim is about the space.
>
> **The honest counter, which should be weighed:** this project has already tried to close
> this gap once and failed (task-068). A could eat real time. **That is why the measurement
> comes before the fix** in §4 — if the measurement shows `MaxAttackersPerUnit` transfers
> cleanly, candidate (2) dies, the honest answer becomes "we still do not know why," and
> **Q27 should be re-taken rather than ground at.** Build the early exit in deliberately.
>
> **If C is taken, take it out loud.** An untested premise is a legitimate risk to carry —
> what is not legitimate is carrying it without anyone having said so.

---

### Q28 — What is the first slice?

**Not in the register, and arguably the decision that orders everything else.** The project
has done this deliberately before (`GATE1-FUN-PROTOTYPE.md`, `RTS-VERTICAL-SLICE.md`); there
is no equivalent for the pivot.

- [ ] **A · The A7 boss fight.** Seven versus a marked boss. No zones, no ledger, no camera
      work, no streaming.
- [ ] **B · One seam and a slide.** Two zones, pre-warm, the transition. Tests D6 and the
      streaming model.
- [ ] **C · The ledger and the stalemate, headless.** Tests the premise — **blocked on Q27**.

> **Recommendation: A.** It is the only candidate that is simultaneously (i) the core loop,
> (ii) the test of the largest design decision the project has taken — Q13 = C, whether the
> player's output routing through seven soldiers is any good — and (iii) **inside the
> validated half of the harness** (§1), so its balance questions can be answered at the real
> evidence bar rather than at the scaffold bar.
>
> It also needs the least: no zone graph, no ledger authority, no streaming, no camera.
> **B second**, because the streaming model's risk is technical rather than design and
> technical risk keeps better.

---

## 4. Proposed task fan — lock analysis done

**Epic: `pivot-preflight`.** Cut on file ownership per `TEMPLATE.md`, so `owns:` globs are
disjoint and siblings can be batch-approved. Scores follow the rubric honestly — **most of
these are `feel: 1` on purpose**, because they are docs and tooling, and `unblocks` does the
lifting exactly as the template says it should.

> **Two constraints the fan respects.**
> **(1) `unreal-editor` is a resource lock** — the two measurement tasks cannot be active at
> the same time, no matter how disjoint their files are.
> **(2) Use file-level globs, not `docs/design/**`.** Three of these write new files into
> `docs/design/`; a directory glob would collide the whole fan and it could not be
> batch-approved at all.

| # | Task | agent | owns | resources | depends-on | score |
|---|---|---|---|---|---|---|
| P1 | Measure Warm-zone cost and drip-spawn rate (Q12) | performance-director | `docs/perf/zone-fidelity-bands.md` | `unreal-editor` | — | feel 1 · risk 3 · cost 2 |
| P2 | Measure the `MaxAttackersPerUnit` transfer in-engine | performance-director | `docs/perf/attacker-cap-transfer.md` | `unreal-editor` | — | feel 1 · risk 3 · cost 2 |
| P3 | Close (or formally fail to close) the wave-attrition gap | sim-director | `Scripts/sim/**`, `docs/sim/**` | — | P2 | feel 1 · risk 3 · cost 3 |
| P4 | Decide and spec ledger authority (Q24) | gameplay-director | `docs/design/war-ledger.md` | — | — | feel 1 · risk 3 · cost 2 |
| P5 | Spec the seven as promoted Actors (Q25) | gameplay-director | `docs/design/squad-actors.md` | — | — | feel 1 · risk 2 · cost 2 |
| P6 | Draft ability-kit and control options for owner call (Q23 + Q26) | gameplay-director | `docs/design/ability-kit.md` | — | — | feel 1 · risk 2 · cost 2 |
| P7 | **Join** — fold verdicts into the register and the specs | claude | `docs/OPEN-DECISIONS.md`, `docs/design/castle-layout.md`, `docs/design/intro-and-zones.md` | — | P3, P4, P5, P6 | feel 1 · risk 1 · cost 1 |

**Parallelism this actually buys:** P4, P5, P6 run concurrently from the start — three
teammates, disjoint files, no resource contention. P1 and P2 run one at a time behind the
editor lock, in either order. P3 waits on P2. P7 is the join and `dispatch` will refuse it
until its dependencies close, which is the behaviour we want.

**Not in this fan, deliberately:** the A7 slice itself (Q28 = A). It depends on P5 and P6
landing, and cutting it before those close would mean guessing at the two things they exist
to decide.

### On P6, specifically

**P6 drafts options; it does not decide.** Q23 and Q26 are owner calls and the register's
standing rule is that an open question is closed by an owner call, never by a later doc
quietly assuming an answer. A teammate that returns a *decided* ability kit has overstepped,
and the spawn prompt should say so in those words.

### On P3, specifically

**P3 must be allowed to fail and say so.** The task is "close the gap **or** establish that
candidate (2) is not the explanation" — and the second outcome is a real result worth the
session, exactly as `LIMITATIONS.md` treats candidate (1)'s elimination. `validate.py`'s
own convention already models this: check 3 reports honestly and deliberately does not flip
the exit code, because "a documented, honestly-reported failure there is a valid, useful
result, not a build failure to be suppressed."

---

## 5. What this document is not

- **Not an owner decision.** Q24–Q28 are proposals in the register's format so they can be
  taken or rejected the same way Tier 0 was.
- **Not a schedule.** No estimates beyond the rubric's `cost` axis.
- **Not a re-litigation of Tier 0.** Q13, Q1 and Q7 are closed and this plans against them.
- **Not new measurement.** Every number referenced traces to `one-camera-bench.md` (2026-07-28)
  or `LIMITATIONS.md`. Nothing here measured anything.

---

## 6. Decision log

*(Append as Q24–Q28 close. Mirror into `docs/OPEN-DECISIONS.md`.)*

- **2026-08-13** — Preflight opened. Q24–Q28 proposed. §1's finding recorded: the pivot
  promoted the wave-attrition gap from balance debt to premise risk, without anyone changing
  the harness or the design. Nothing decided.
