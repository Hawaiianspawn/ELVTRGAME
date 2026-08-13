# Open decisions — the register

**Opened:** 2026-08-13 · **Status:** 22 open, 0 closed
**Sources consolidated here:** `docs/design/castle-layout.md` §10 (Q1–Q7),
`docs/design/intro-and-zones.md` §E (Q8–Q12), and Q13–Q22, raised 2026-08-13 and
not previously written down anywhere.

---

## How to use this file

Each decision below is a **tickable choice**. To close one:

1. Tick the option. Fill in `Verdict` and `Date`.
2. **Update the source doc in the same commit.** A decision closed here and not
   written into the doc that depends on it is worse than an open one, because the
   next reader trusts the doc.
3. Update the status line at the top and the table below.

> **Why the ceremony.** `docs/GDD-TODO.md` was this project's last tracker and it went
> stale enough to need task-001 to correct it. The failure mode was closing things in
> conversation and never writing them down. IDs here are **stable and never renumbered** —
> Q1–Q12 keep the numbers the source docs already cite.

**The standing rule this register enforces:** an open question is closed by an owner
call, not by a later doc quietly assuming an answer.

---

## Board

| ID | Decision | Tier | Blocked by | Blocks |
|---|---|---|---|---|
| **Q13** | What is the player now that the army is seven? | **0** | — | Q2, Q14, Q15, Q16, Q21 |
| **Q1** | What a siege is, and what persists between them | **0** | — | Q3, Q6, Q10, Q19 |
| **Q7** | What the leash / light premise means now | **0** | — | Q11, Q15 |
| Q14 | Is 7 a cap or a floor? | 1 | Q13 | Q2, Q21 |
| Q15 | Can your seven be downed and revived? | 1 | Q13, Q7 | — |
| Q16 | What ends beat A7? | 1 | Q13 | Q18 |
| Q17 | Can you die during the scripted loss? | 1 | — | — |
| Q9 | Does the intro's boss return? | 1 | — | Q10, Q18 |
| Q5 | The `Breaking` → `Fallen` window | 1 | — | Q8 |
| Q18 | How the withdrawal count is shown | 1 | Q9, Q16 | — |
| Q12 | What a Warm zone actually costs | 2 | — | Q8 |
| Q8 | Zone counts and sizes per layer | 2 | Q5, Q12 | — |
| Q2 | Composition of the seven | 2 | Q13, Q14 | Q21 |
| Q4 | Enemy: real planner or authored strategy set | 2 | — | — |
| Q10 | Does the withdrawal count persist? | 2 | Q1, Q9 | — |
| Q19 | Where the game saves | 2 | Q1 | — |
| Q3 | Fate of the kills → army-level ratchet | 3 | Q1 | — |
| Q6 | Can the war be won, or only survived? | 3 | Q1 | — |
| Q11 | Zoom range vs. the flame pool and dither | 3 | Q7 | — |
| Q20 | Fate of `squad-group-system.md` | 3 | — | — |
| Q21 | Art for seven named persistent soldiers | 3 | Q13, Q14, Q2 | — |
| Q22 | Do Elite and Titan accrete marks? | 3 | — | — |

**Tier 0** — nothing downstream resolves cleanly until these do.
**Tier 1** — blocks building the intro.
**Tier 2** — blocks tuning and measurement.
**Tier 3** — scope and cleanup; real decisions, but nothing waits on them.

---

# TIER 0

## Q13 — What is the player, now that the army is seven?

**Blocks:** Q2, Q14, Q15, Q16, Q21 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

`CAMERA-SCALE.md` names "GDD §4's hero-relevance tension," and the hero is real in code —
`HeroMeleeRangeSq`, `HeroDamage`, `FindOwnGridEntry` bridge a non-Mass Actor into the grid.
**The pivot inverts that tension.** The old worry was a hero lost behind 120 units. With
seven, the risk runs the other way: the hero may be most of the damage, and the seven
become an entourage.

This decides the boss fight, the control scheme, what the HUD is for, and whether A3's
"they turn and look at you" is a squad joining a fighter or a bodyguard forming around a
commander.

- [ ] **A · A fighter among the seven.** You are the eighth body. You deal real damage; the
      squad multiplies you. Boss fights are action fights.
      *Cost:* with a 35–55 surround cap and seven allies, your own DPS may swamp
      positioning, and the squad drifts toward decoration — the original tension, unfixed.
- [ ] **B · A commander who rarely fights.** Your body exists — it carries the flame, it
      anchors the camera — but your damage is negligible. You win by positioning the seven.
      *Cost:* collapses toward an RTS with seven units, and the "you carry the only fire"
      fantasy stops being physical.
- [ ] **C · A fighter whose entire output IS the seven.** You are embodied and in danger,
      but you have no independent attack worth using. Your abilities act *through* the
      squad — focus fire, reposition, shield, raise.
      *Cost:* an ability kit that does not exist yet, and it is the least conventional of
      the three.

> **Recommendation: C.** It closes the hero-relevance tension from both ends at once — you
> cannot be irrelevant because everything routes through you, and the seven cannot be
> decoration because they are your only output. It keeps the flame physical, and it turns
> mark-counterplay into a squad-composition read rather than a dodge-roll read, which is
> what the marks system was designed to reward.
>
> **A is the lowest-risk answer** and the one most players will expect. If the project
> wants a conventional spine here, take A knowingly rather than by drift.

---

## Q1 — What a siege is, and what persists between them

**Blocks:** Q3, Q6, Q10, Q19 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

D2 (one-way collapse) and D3 (persistent keep) pull against each other. The castle only
falls inward, but the keep persists across sieges. Something has to reset.

- [ ] **A · The siege is the session.** It ends when the Crown falls or a relief condition
      is met. Between sieges the castle is repaired, and **how deep the enemy got last time
      determines what state it starts in** — the keep ratchet is what you spend on repair.
- [ ] **B · Successive castles.** Each siege is a different fortress as the front retreats
      across a region. The keep ratchet carries as doctrine and troops rather than as stone.
- [ ] **C · One unbroken siege, no session boundary.** The war is continuous; the keep
      ratchet is spent live.

> **Recommendation: A**, restated from `castle-layout.md` §4.1 where it was written as a
> recommendation and explicitly not as a decision. It preserves one-way collapse *within* a
> siege and puts the keep ratchet on the between-siege clock, where D3 implies it belongs.
> **B and C were not ruled out.**

---

## Q7 — What the leash / light premise means now

**Blocks:** Q11, Q15 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

**The largest unexamined consequence of the pivot, and it is shipped code.** `LeashRadius`,
the break latch and `LeashWarnBit` are built, and `FLAME-FOUNDATION.md` rests on *outside
the light, the dark takes you*. With hundreds of allies holding fronts across five layers
while you are two rings away, that cannot be literally true as written.

- [ ] **A · The castle has its own light.** Braziers, the Lantern Court, gate-fires. Your
      flame is portable light in a place that has fixed light. The leash applies only to
      your seven and to ground the castle has lost.
- [ ] **B · The light means something other than survival.** The garrison survives fine;
      what your flame does is different — it rallies, restores, or lets units be commanded
      at all. The leash becomes a command radius rather than a life-support radius.
- [ ] **C · The premise changes.** The dark stops being lethal and becomes atmosphere. The
      leash retires as a survival mechanic.

> **No recommendation — this is a narrative call as much as a mechanical one.** What is
> worth saying: **A costs the least** (it explains the existing fiction without changing it,
> and it gives the lost layers a visible property — they go dark), and **B is the most
> interesting**, because it makes your arrival at a Breaking front mechanically meaningful
> rather than just numerically helpful.

---

# TIER 1 — blocks building the intro

## Q14 — Is 7 a cap or a floor?

**Blocks:** Q2, Q21 · **Blocked by:** Q13 · **Verdict:** ________ · **Date:** ________

D3 made the squad a ratchet but did not say on which axis. Size or depth.

- [ ] **A · Hard cap, seven forever.** The ratchet is depth only — the adaptation ladder in
      `docs/design/adaptation.md`, which already exists and is currently unclaimed by the
      pivot. Cheapest, and makes each soldier maximally precious.
- [ ] **B · Seven is a start; it grows.** Reintroduces a size ratchet at small scale and
      puts adaptation and headcount in competition for the same reward slot.
- [ ] **C · Seven in the field, larger roster behind.** You pick seven per sortie from a
      growing company. Depth *and* breadth, with a fixed field size.

> **Recommendation depends on Q13.** If Q13 = C (your output is the squad), then **C here is
> strong** — loadout selection becomes the counterplay to a boss's marks, which is exactly
> the read the marks system wants. If Q13 = A, take **A** and keep it simple.
> **B is the weakest** of the three under any Q13 answer.

---

## Q15 — Can your seven be downed and revived?

**Blocks:** — · **Blocked by:** Q13, Q7 · **Verdict:** ________ · **Date:** ________

**There is a live inconsistency here.** The intro's first ten seconds teach *down is a
state, not death* — that is beat A2's entire payload, and it is the existing
degrade-don't-die rule. If your own squad does not obey it, the opening teaches a rule that
does not apply to you.

Answering this also forces the thing D8 sidestepped: **do you ever get a healer at all?**
The old "light fragments convert into healer units" route belongs to the retired design.

- [ ] **A · Same rule as the garrison.** Downed, not dead; raised by a healer or by you.
      Fully consistent with A2. Requires a healer answer.
- [ ] **B · Downed, self-recovering after the engagement.** No healer needed. Cheapest way
      to stay consistent with A2.
- [ ] **C · Squad deaths are permanent.** Maximum weight on D3's named soldiers.
      **Only viable if the intro explicitly marks your seven as exempt** — otherwise A2 is
      teaching a lie the player will catch.

> **Recommendation: B as the floor, A as the target if healers exist.** C is the most
> dramatic and it can work, but it is not free — it costs an added line in the opening that
> says *the light does not reach you the way it reaches them*, and that line has to be
> earned by Q7's answer.

---

## Q16 — What ends beat A7?

**Blocks:** Q18 · **Blocked by:** Q13 · **Verdict:** ________ · **Date:** ________

The withdrawal beat needs a terminating condition. It currently has none.

- [ ] **A · The column is clear.** Last non-combatant through the Cart Gate. Player-driven,
      unambiguous success read.
- [ ] **B · The boss reaches the gate.** Threat-driven. Creates a race with a visible clock
      that is not a UI clock.
- [ ] **C · Both — whichever comes first.** Column clear = full score; boss arrives = the
      beat ends early with whoever made it.

> **Recommendation: C.** It makes the score real in both directions and gives the beat a
> failure mode that is not death — which matters, because A7 is a beat you are meant to lose
> and dying is not supposed to be how you lose it.

---

## Q17 — Can you die during the scripted loss, and what does it cost?

**Blocks:** — · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

Hero death is instant-loss globally (`GDD.md` §3). In a beat authored to be lost, that is
undefined.

- [ ] **A · You cannot die.** Damage capped or scripted. Safest tutorial, weakest stakes,
      and it teaches a rule that stops being true five minutes later.
- [ ] **B · You can go down and are dragged out — and the column stops moving while you are
      down.** Your failure is paid in the same currency the beat is already scored in.
- [ ] **C · Full hero-death rule; the intro restarts.** Consistent with the rest of the
      game, at the cost of replaying a five-minute scripted sequence.

> **Recommendation: B.** It keeps real stakes without making an authored-loss beat
> restartable, and it converts your mistake into fewer people saved rather than into a
> reload — which is the same lesson A7 is already teaching.

---

## Q9 — Does the intro's boss return?

**Blocks:** Q10, Q18 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

Carried from `intro-and-zones.md` §A9. Beat A9 sets this up whether or not it is taken.

- [ ] **A · Yes — it becomes the game's recurring antagonist**, met again at a gate that
      matters, carrying the marks it earned on you personally in the first five minutes.
- [ ] **B · No — it is one of many.** A9's shot still works as a statement about how bosses
      grow generally, but **A9 must then be re-cut**: a specific visible consequence that
      never returns is a promise the game breaks.

> **Recommendation: A.** It is the single strongest thing available in the opening — the
> recurring antagonist is exactly as strong as you let it become, before you knew that was
> what was happening. But it commits the project to a named antagonist with authored
> reappearances, which is real content, and that is an owner call and not a writer's.

---

## Q5 — The `Breaking` → `Fallen` window

**Blocks:** Q8 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

**The pacing number for the entire game.** How long the player has to answer an alarm.
Every layer's size, every zone's pre-warm placement (`intro-and-zones.md` §B4), and whether
the alarm is answerable at all resolve against it. There is no basis for it yet.

- [ ] **A · Fixed window**, the same at every gate. Predictable; the player learns one number.
- [ ] **B · Scales with the distance from the player to the front.** Always answerable,
      which risks making the alarm feel staged.
- [ ] **C · Scales with the front's own state** — how badly it is losing, independent of
      where you are. Honest, and it means some alarms are genuinely unanswerable.

> **This one should be measured, not chosen.** The sim harness (`docs/sim/`) can bracket it:
> run the traverse from each layer to each gate, and pick the window from what is actually
> reachable. **Recommendation: measure first, then take C**, with the floor set by the
> longest traverse the level actually contains.

---

## Q18 — How the withdrawal count is shown

**Blocks:** — · **Blocked by:** Q9, Q16 · **Verdict:** ________ · **Date:** ________

- [ ] **A · Purely diegetic.** You watch them go through the gate. No number, ever.
- [ ] **B · A quiet running count**, resolving at A8.
- [ ] **C · No count at the time — A9's boss size *is* the readout.**

> **Recommendation: C, with A.** The consequence being legible *on the enemy's body* rather
> than in a results screen is the whole point of the beat. **Depends on Q9** — if the boss
> does not return, C loses most of its force and B becomes necessary.

---

# TIER 2 — blocks tuning and measurement

## Q12 — What a Warm zone actually costs

**Blocks:** Q8 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

Two figures in `intro-and-zones.md` §B are **inferred from measured batch numbers, not
measured**: that a Warm zone at `SimLOD.Stride 4` costs about a quarter of Live, and that
drip-spawn scales linearly off the 23.46 ms / 250-entity measurement (which sets the
**~3.3 s pre-warm lead** every level must respect).

- [ ] **Measured.** Record the numbers here and correct §B4 and §B6 in the same commit.

> Not a design choice — a task. `-SwarmBench` can settle both. Until it does, **no level
> should be authored against the 3.3 s figure.**

---

## Q8 — Zone counts and sizes per layer

**Blocks:** — · **Blocked by:** Q5, Q12 · **Verdict:** ________ · **Date:** ________

How many zones each of L1–L5 is cut into, and how big each is. The binding constraint is
the pre-warm lead (Q12): **seams cannot be closer together than the promotion time**, or the
zone graph thrashes.

- [ ] Authored per layer. Record the table here.

> Needs the level, not a doc. L1 is the widest ring and wants several; L5 is one.

---

## Q2 — Composition of the seven

**Blocks:** Q21 · **Blocked by:** Q13, Q14 · **Verdict:** ________ · **Date:** ________

- [ ] **A · Fixed roles.** Authored archetypes, same every time.
- [ ] **B · Player-chosen loadout** from a roster.
- [ ] **C · Whoever survived the last siege.** D3's named-persistence read to its conclusion.

> Largely determined by Q13 and Q14 once those land. **If Q14 = C, this is C by construction.**

---

## Q4 — Enemy: real planner or authored strategy set

**Blocks:** — · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

- [ ] **A · A real planner** — goal decomposition over the front ledger.
- [ ] **B · A small authored strategy set** with weighted selection.

> **Recommendation: B for the vertical slice.** Dramatically cheaper and probably
> indistinguishable at slice scale. A is a real project; take it deliberately or not at all.

---

## Q10 — Does the withdrawal count persist?

**Blocks:** — · **Blocked by:** Q1, Q9 · **Verdict:** ________ · **Date:** ________

- [ ] **A · A mark on the boss and nothing else.** It exists only as `Column-fed`.
- [ ] **B · A stat that feeds the keep ratchet** (D3) — people saved are garrison later.
- [ ] **C · Both.**

> **B is the one that makes the opening matter mechanically rather than only dramatically**,
> and it is the first thing the game asks the player to care about. Blocked by Q1 because
> "persists" has no meaning until the session boundary does.

---

## Q19 — Where the game saves

**Blocks:** — · **Blocked by:** Q1 · **Verdict:** ________ · **Date:** ________

A siege is now a session rather than a 20-minute run, and nothing has said where it saves.
Candidates: at gate transitions; at layer falls; only between sieges; continuous.

- [ ] Decide after Q1.

> **No recommendation until Q1.** Note that gate transitions are already hard seams with a
> natural pause, so they are the cheapest place to put it.

---

# TIER 3 — scope and cleanup

## Q3 — Fate of the kills → army-level ratchet

**Blocks:** — · **Blocked by:** Q1 · **Verdict:** ________ · **Date:** ________

It is **shipped, working C++** — per-squad and per-hero attribution in `Mass/SwarmSubsystem.h`.

- [ ] **A · Feeds the squad ratchet.**  - [ ] **B · Feeds the keep ratchet.**
- [ ] **C · Feeds both.**  - [ ] **D · Retires.**

> Retiring working code and repointing it cost differently. The attribution path is cheap to
> keep aimed somewhere.

---

## Q6 — Can the war be won, or only survived?

**Blocks:** — · **Blocked by:** Q1 · **Verdict:** ________ · **Date:** ________

D2 says the *castle* only falls. It does not say the *war* is unwinnable. **Tone-defining.**

- [ ] **A · Winnable** — there is an end and you can reach it.
- [ ] **B · Survivable only** — the question is how long and at what cost.
- [ ] **C · Winnable but not by you** — you buy time for something else to decide it.

---

## Q11 — Zoom range vs. the flame pool and dither

**Blocks:** — · **Blocked by:** Q7 · **Verdict:** ________ · **Date:** ________

Inherited live from `CAMERA-SCALE.md` §4 Q5, which D7 narrowed but did not answer.
`Swarm.FlameRadius` is 900 uu against an `OrthoWidth` of 2400, and `DitherWorldAnchor 1`
ties dither density to world space — so **any** player zoom changes both.

- [ ] **A · No player zoom.** One framing, full stop. Deletes the problem.
- [ ] **B · Clamped zoom**, with flame radius and dither scale driven off `OrthoWidth`.
- [ ] **C · Free zoom**, and both effects are re-tuned to be zoom-invariant.

> **A is free and B is cheap.** C is a real tuning job for an affordance nobody has asked for.

---

## Q20 — Fate of `docs/design/squad-group-system.md`

**Blocks:** — · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

48 KB of squad abstraction built for 120 commanded units. With seven, most of it has no
owner.

- [ ] **A · Retire wholesale.**
- [ ] **B · Retarget to the garrison** — it becomes how the castle's *autonomous* forces
      organise, which the war sim needs regardless.
- [ ] **C · Keep for the seven at reduced scale.**

> **Recommendation: B.** The war sim needs unit organisation for hundreds of autonomous
> allies, which is precisely what that document specced. It did not die; it changed owner.

---

## Q21 — Art for seven named persistent soldiers

**Blocks:** — · **Blocked by:** Q13, Q14, Q2 · **Verdict:** ________ · **Date:** ________

The art pipeline (variant families, atlases, `roster.json`, `provenance.json`) was built to
make **mass** legible. Seven named individuals are a different problem, and the roster does
not cover it.

- [ ] **A · Bespoke per soldier.**
- [ ] **B · Variant-family with pinned seeds** — each of the seven is a locked roll from the
      existing pipeline, so they are consistent, individual, and nearly free.
- [ ] **C · Bespoke silhouettes + family-generated kit variation** as they climb the
      adaptation ladder.

> **Recommendation: B as the floor, C as the target.** B costs almost nothing given the
> pipeline already exists, and it means the seven are visibly *of* the same world as the
> hundreds around them — which is the right read for soldiers who were standing in that
> infirmary yard five minutes ago.

---

## Q22 — Do Elite and Titan accrete marks?

**Blocks:** — · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

Marks (`castle-layout.md` §6.1) currently attach to Boss only.

- [ ] **A · Boss only.** Smallest surface.
- [ ] **B · Elite and above.**
- [ ] **C · Everything above Fodder.** — **architecturally not available.** Fodder and
      Soldier are Mass Entities with no per-unit uniqueness by Design Law 5
      (`docs/design/entity-tiers.md` §1); marking them means promoting them to Actors.

> **Recommendation: B.** Titan is currently a specced tier with a stat block and no content
> home — `entity-tiers.md` says so outright. Marks give it one, at no new architecture:
> Elite/Titan/Boss are already promoted Actors sharing the hero's grid-bridge pattern.

---

## Decision log

*(Append here as decisions close. One line each: ID, verdict, date, and the doc updated.)*

- —
