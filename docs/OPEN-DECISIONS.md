# Open decisions — the register

**Opened:** 2026-08-13 · **Status:** 23 open, 9 closed
**Last closed:** Q6 (the relief is another bearer), Q32 (light fragments retire) — 2026-08-13
**Sources consolidated here:** `docs/design/castle-layout.md` §10 (Q1–Q7),
`docs/design/intro-and-zones.md` §E (Q8–Q12), Q13–Q22 raised 2026-08-13, and
**Q24–Q28 raised in `docs/PREFLIGHT.md` §3** — the pre-implementation pass.

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

| ID | Decision | Tier | Blocked by | Blocks | Status |
|---|---|---|---|---|---|
| ~~Q13~~ | What is the player now that the army is seven? | 0 | — | Q2, Q14, Q15, Q16, Q21, Q23 | **CLOSED — C** |
| ~~Q1~~ | What a siege is, and what persists between them | 0 | — | Q3, Q6, Q10, Q19 | **CLOSED — A** |
| ~~Q7~~ | What the leash / light premise means now | 0 | — | Q11, Q15 | **CLOSED — A** |
| ~~Q29~~ | Fate of the four classes under the pivot | 1 | — | Q2, Q21, Q23 | **CLOSED — A** |
| ~~Q30~~ | Fate of gold and the shops under the siege | 2 | — | GDD Q26, Q34 | **CLOSED — B** |
| ~~Q31~~ | Who Supply/upkeep governs now | 3 | — | — | **CLOSED — A** |
| ~~Q32~~ | Fate of light fragments | 3 | — | — | **CLOSED — A** |
| **Q23** | The squad-channelled ability kit — what is in it? | **1** | — | Q2, Q14 | open *(opened by Q13)* |
| **Q14** | Is 7 a cap or a floor? | **1** | *unblocked* | Q2, Q21 | open |
| **Q15** | Can your seven be downed and revived? | **1** | *unblocked* | — | open |
| **Q16** | What ends beat A7? | **1** | *unblocked* | Q18 | open |
| Q17 | Can you die during the scripted loss? | 1 | — | — | open |
| Q9 | Does the intro's boss return? | 1 | — | Q10, Q18 | open |
| Q5 | The `Breaking` → `Fallen` window | 1 | — | Q8 | open — *measure* |
| Q18 | How the withdrawal count is shown | 1 | Q9, Q16 | — | open |
| Q12 | What a Warm zone actually costs | 2 | — | Q8 | open — *measure* |
| Q8 | Zone counts and sizes per layer | 2 | Q5, Q12 | — | open |
| Q2 | Composition of the seven | 2 | Q14, Q23 | Q21 | open |
| Q4 | Enemy: real planner or authored strategy set | 2 | — | — | open |
| Q10 | Does the withdrawal count persist? | 2 | Q9 | — | open |
| Q19 | Where the game saves | 2 | *unblocked* | — | open |
| ~~Q3~~ | Fate of the kills → army-level ratchet | 3 | — | — | **CLOSED — C** |
| ~~Q6~~ | Can the war be won, or only survived? | 3 | — | — | **CLOSED — C** |
| Q11 | Zoom range vs. the flame pool and dither | 3 | *unblocked* | — | open |
| Q20 | Fate of `squad-group-system.md` | 3 | — | — | open |
| Q21 | Art for seven named persistent soldiers | 3 | Q14, Q2 | — | open |
| Q22 | Do Elite and Titan accrete marks? | 3 | — | — | open |
| **Q24** | **Is the front ledger authoritative or derived?** | **0** | — | Q8, all streaming | open |
| **Q25** | Are the seven Mass entities or promoted Actors? | **1** | — | Q21 | open |
| **Q26** | How are orders issued? | **1** | Q23 | — | open |
| **Q27** | How does the stalemate premise get validated? | **1** | — | Q28 | open |
| **Q28** | What is the first slice? | **1** | Q25, Q26, Q27 | everything | open |

> **Tier 0 is closed.** The three that gated everything landed 2026-08-13. Seven decisions
> came unblocked with them (Q14, Q15, Q16, Q19, Q3, Q6, Q11), and **Q13 opened one new one
> (Q23)** — an ability kit is now required content that did not exist before the call.
>
> **Next up: Q23, Q14, Q15, Q16** — the four that decide whether the intro can be built.
>
> **Then `docs/PREFLIGHT.md` added Q24–Q28** — five decisions that were not in this register
> and that block building harder than most of what was. **Q24 (ledger authority) is
> Tier-0-equivalent**: it decides whether "marks earned identically in all three bands"
> (`castle-layout.md` §7) is even possible, and getting it wrong makes beat A9's consequence
> a lie. Full options, consequences and recommendations are in `PREFLIGHT.md` §3; they are
> kept there rather than duplicated here so the two files cannot drift.

**Tier 0** — nothing downstream resolves cleanly until these do.
**Tier 1** — blocks building the intro.
**Tier 2** — blocks tuning and measurement.
**Tier 3** — scope and cleanup; real decisions, but nothing waits on them.

---

# TIER 0 — CLOSED 2026-08-13

## Q13 — What is the player, now that the army is seven? · **CLOSED**

**Blocks:** Q2, Q14, Q15, Q16, Q21, Q23 · **Blocked by:** — · **Verdict: C** · **Date: 2026-08-13**

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
- [x] **C · A fighter whose entire output IS the seven.** You are embodied and in danger,
      but you have no independent attack worth using. Your abilities act *through* the
      squad — focus fire, reposition, shield, raise.
      *Cost:* an ability kit that does not exist yet, and it is the least conventional of
      the three.

> **TAKEN — C, 2026-08-13.** It closes the hero-relevance tension from both ends at once —
> you cannot be irrelevant because everything routes through you, and the seven cannot be
> decoration because they are your only output. It keeps the flame physical, and it turns
> mark-counterplay into a squad-composition read rather than a dodge-roll read, which is
> what the marks system was designed to reward.

### What C commits the project to

1. **An ability kit is now required content.** It does not exist and nothing else can
   substitute for it — this is the whole of the player's output. **Opened as Q23.**
2. **`HeroDamage` stops being the player's damage.** The hero Actor and its grid-bridge
   (`HeroMeleeRangeSq`, `FindOwnGridEntry`, `SwarmCombatProcessors.cpp`) stay — the hero is
   still a body in the grid that can be hit — but the damage path becomes a *channel*
   rather than a contribution. **The bridge pattern survives; the number retires.**
3. **A3's "seven turn and look at you" reads correctly and needs no change.** They are a
   squad forming around the person they will act for.
4. **The seven cannot be interchangeable.** If your output is the squad, each soldier is a
   different verb — which loads Q2 and Q23 heavily and makes Q14 more interesting.
5. **Boss marks become the primary read.** A boss's silhouette tells you which of your
   seven to spend, which is exactly the job `castle-layout.md` §6.3 gave them.
6. **The HUD's job changes.** It stops describing an army and starts describing seven
   individuals and what each can currently do. Nothing has specced that.

> **Not decided by this, and not to be inferred:** whether the player has *any* direct
> attack for self-defence, cooldown/resource structure, and whether abilities target
> individual soldiers or the squad as a unit. All of that is Q23.

---

## Q1 — What a siege is, and what persists between them · **CLOSED**

**Blocks:** Q3, Q6, Q10, Q19 · **Blocked by:** — · **Verdict: A** · **Date: 2026-08-13**

D2 (one-way collapse) and D3 (persistent keep) pull against each other. The castle only
falls inward, but the keep persists across sieges. Something has to reset.

- [x] **A · The siege is the session.** It ends when the Crown falls or a relief condition
      is met. Between sieges the castle is repaired, and **how deep the enemy got last time
      determines what state it starts in** — the keep ratchet is what you spend on repair.
- [ ] **B · Successive castles.** Each siege is a different fortress as the front retreats
      across a region. The keep ratchet carries as doctrine and troops rather than as stone.
- [ ] **C · One unbroken siege, no session boundary.** The war is continuous; the keep
      ratchet is spent live.

> **TAKEN — A, 2026-08-13.** Promotes `castle-layout.md` §4.1 from recommendation to
> decision. One-way collapse holds *within* a siege; the keep ratchet sits on the
> between-siege clock where D3 implies it belongs.

### What A commits the project to

1. **A siege has a win condition and it is not killing anything.** "A relief condition" is
   a placeholder — **what actually ends a siege in your favour is undecided**, and it is
   the live half of Q6. Do not infer "survive N waves."
2. **Repair is a spend, so it needs a currency.** The keep ratchet now has a sink but no
   source. This makes **Q3 (the kills economy) load-bearing rather than housekeeping** —
   there is shipped attribution code and now somewhere obvious for it to point.
3. **Layer state is save state.** Which gates stand at siege start is persistent data, so
   **Q19 has a natural answer** — save at the siege boundary, because that is the only
   place the persistent set changes wholesale.
4. **A wipe is cheap and a bad siege is expensive.** Losing costs the siege; losing *early*
   costs the next one too, because the castle opens damaged. That is the ratchet having
   teeth without punishing a loss directly.

---

## Q7 — What the leash / light premise means now · **CLOSED**

**Blocks:** Q11, Q15 · **Blocked by:** — · **Verdict: A** · **Date: 2026-08-13**

**The largest unexamined consequence of the pivot, and it is shipped code.** `LeashRadius`,
the break latch and `LeashWarnBit` are built, and `FLAME-FOUNDATION.md` rests on *outside
the light, the dark takes you*. With hundreds of allies holding fronts across five layers
while you are two rings away, that cannot be literally true as written.

- [x] **A · The castle has its own light.** Braziers, the Lantern Court, gate-fires. Your
      flame is portable light in a place that has fixed light. The leash applies only to
      your seven and to ground the castle has lost.
- [ ] **B · The light means something other than survival.** The garrison survives fine;
      what your flame does is different — it rallies, restores, or lets units be commanded
      at all. The leash becomes a command radius rather than a life-support radius.
- [ ] **C · The premise changes.** The dark stops being lethal and becomes atmosphere. The
      leash retires as a survival mechanic.

> **TAKEN — A, 2026-08-13.** The existing fiction survives intact and the shipped leash code
> keeps a correct job. `FLAME-FOUNDATION.md` needs no retraction.

### What A commits the project to

1. **The leash keeps working, unchanged, at a new scale.** `LeashRadius`, the break latch
   and `LeashWarnBit` now govern seven units instead of 120. **Nothing retires** — but
   `LeashRadius` was tuned for a congregation and is almost certainly wrong for a squad.
   That is a tuning pass, not a redesign.
2. **A fallen layer goes dark, and that is a mechanic.** The castle's fixed light is *the
   castle's* — when a layer falls, its braziers go with it. So the ground behind the enemy
   is lethal dark, which explains why the collapse is one-way without any extra rule, and
   gives the player a visible reason they cannot walk back out.
3. **Your flame is now the only light on ground the castle has lost** — which is exactly
   the condition of beat A7. **The Great Gate fight is the first and only time in the intro
   the leash matters**, because it is the only ground the castle no longer holds.
4. **Q15 is narrowed, not answered.** The castle's light is what stands the garrison back
   up (beat A2). Whether it reaches *your* seven is still open — but option C there
   (permanent squad death) now has a fiction that supports it: **you fight where the
   castle's light does not reach.**
5. **Q11 is narrowed.** The flame pool is no longer the only light source in frame, so its
   proportion of the screen matters less than it did when it was everything.

---

# TIER 1 — blocks building the intro

## Q29 — Fate of the four classes under the pivot · **CLOSED**

**Blocks:** Q2, Q21, Q23 · **Blocked by:** — · **Verdict: A** · **Date: 2026-08-13**

`CLASSES.md`'s four classes are defined by the retinue each commands. With the
commanded force at seven, the class system had no stated home — and the four kits,
tier ladders and identities are among the best-developed content in the project.
The Pathfinder's pack ("low count, elite, named, individually mourned, cap ≈6–12,
each member takes upgrades like a mini-hero") was already the pivot's shape,
written 2026-07-11.

- [x] **A · The four survive as squad archetypes — mixed squad, no class select.**
      The seven are drawn across all four identities — line soldier (Liberated),
      guardian (Awakened), hunter (Pack), light (Guided) — and **evolve mid-fight
      up the tiered adaptation tree** (`docs/design/adaptation.md`). The player is
      one identity: the flame-bearer (Q13 = C).
- [ ] **B · Class select survives; the seven are one class's units.** Four distinct
      squads at 4× roster/kit cost; a healer-only squad as your entire output is a
      hard problem.
- [ ] **C · Class = the player's channel kit; squad mixed regardless.** Rebuilds the
      soldier-interchangeability failure Q13 = C was chosen to avoid.

> **TAKEN — A, 2026-08-13.** Owner call, given as "keep the 4 classes but they can
> evolve over the fight into our tiered tree system," then confirmed against A/B/C.

### What A commits the project to

1. **The adaptation tree is claimed by the pivot.** Rungs climb *during* a siege
   ("over the fight") and persist per D3. **The climb mechanism is deliberately
   unspecified** — kills, XP, or something else is not decided; do not infer. It is
   adjacent to Q3 (the kill economy needs a sink; this may be one).
2. **`CLASSES.md` becomes a sourcebook, not a menu.** The four hero kits are the
   source material for Q23 (Mark Quarry = focus fire, Shield Wall / Ward Circle =
   shield, Kindle = raise, Banner Slam / The Muster = rally). Vanguard's
   Freed → Militia → Veteran → Bannerman ladder is the model archetype tree.
3. **Q2 is narrowed, not closed.** The archetype palette is fixed at four; the
   split across seven bodies and the selection mechanism (fixed / loadout /
   survivors) stay open with Q14.
4. **Q21 is narrowed.** Each archetype already has a silhouette language — lines /
   blocks / darts / glow — so the seven inherit an art read for free.
5. **Class select is deferred, not dead** — same marker style as `[MP — deferred]`.
6. `adaptation.md`'s **captain rung (fields its own retinue ≤ 8) still conflicts
   with D1** and needs its re-read regardless (already flagged, `castle-layout.md`
   §11).
7. **A second topology now exists in code and does not match either.** `StressWar`
   (`docs/design/stress-war.md`) fields a **company lead**: one named body on its *own*
   handle, commanding a company of several troop handles — the inverse of
   `adaptation.md` §6's "a captain plus its retinue is one handle". Neither is canon yet;
   the re-read above has to pick one word for one shape. Logged 2026-08-19, testbed only.

---

## Q23 — The squad-channelled ability kit: what is in it?

**Blocks:** Q2, Q14 · **Blocked by:** — · **Verdict:** ________ · **Date:** ________
**Opened 2026-08-13 by Q13's answer.** This is now the player's entire output, so it is the
largest single piece of undesigned content in the project.

Q13 = C says your abilities act *through* the seven. Nothing says what they are. The shape
of the answer matters more than the list:

- [ ] **A · A fixed kit on the player.** Four or five verbs — focus fire, reposition,
      shield, raise, rally — that apply to whichever soldiers are in range. The seven are
      the *targets* of your kit.
      *Cost:* the seven risk becoming interchangeable again, which is the failure Q13 = C
      was chosen to avoid.
- [ ] **B · The kit lives in the soldiers; you spend them.** Each of the seven *is* a verb —
      one breaches, one screens, one raises the fallen — and your role is choosing who acts
      and when. The player's own kit is small or empty.
      *Cost:* closest to an RTS with seven units; needs each soldier to be genuinely
      distinct or the choice is hollow.
- [ ] **C · Both — a small player kit that modifies what the soldiers do.** Your verbs are
      amplifiers and redirects on their verbs.
      *Cost:* most design surface, most likely to be legible only to the designer.

> **Q29 = A names the source material (2026-08-13):** the four class hero kits in
> `CLASSES.md` — Mark Quarry, Shield Wall / Ward Circle, Kindle, Banner Slam / The
> Muster — already cover Q13 = C's sketch list (focus fire, shield, raise, rally).
> The kit question is now *which of these verbs, on what structure*, not invention
> from nothing.
>
> **No recommendation yet — this needs a prototype, not an argument.** What is worth saying:
> **B is the option most consistent with the rest of the pivot.** The whole design already
> says that reading a boss's marks and picking the answer is the tactical layer
> (`castle-layout.md` §6.3); if the answer is always the same four player verbs, that read
> has nothing to resolve into. B makes the marks matter and makes Q14 = C attractive.
>
> **Do not let this be decided by the first thing that gets implemented.**

---

## Q14 — Is 7 a cap or a floor?

**Blocks:** Q2, Q21 · **Blocked by:** ~~Q13~~ *(closed — C)*, Q23 · **Verdict:** ________ · **Date:** ________

D3 made the squad a ratchet but did not say on which axis. Size or depth.

- [ ] **A · Hard cap, seven forever.** The ratchet is depth only — the adaptation ladder in
      `docs/design/adaptation.md`, which already exists and is currently unclaimed by the
      pivot. Cheapest, and makes each soldier maximally precious.
- [ ] **B · Seven is a start; it grows.** Reintroduces a size ratchet at small scale and
      puts adaptation and headcount in competition for the same reward slot.
- [ ] **C · Seven in the field, larger roster behind.** You pick seven per sortie from a
      growing company. Depth *and* breadth, with a fixed field size.

> **Narrowed by Q29 = A (2026-08-13):** whatever the answer, the depth axis is
> settled — the seven evolve mid-fight up the tiered adaptation tree. This question
> is now only about *headcount*: cap, growth, or bench.
>
> **Recommendation: C — sharpened by Q13 = C, 2026-08-13.** Now that the player's entire
> output routes through the squad, **who you bring is the decision**, and a fixed seven
> means you bring the same answer to every boss. Loadout selection becomes the counterplay
> to a boss's marks — which is precisely the read `castle-layout.md` §6.3 asks for and the
> only thing that stops the marks system resolving into one dominant response.
>
> **B remains the weakest** under any answer. **A stays viable** and is much the cheapest —
> take it if the project wants scope control, knowing it costs the loadout read.
>
> **Genuinely contingent on Q23.** If Q23 = A (a fixed player kit), the roster in C has
> little to express and A here becomes the honest answer instead.

---

## Q15 — Can your seven be downed and revived?

**Blocks:** — · **Blocked by:** ~~Q13, Q7~~ *(both closed)* · **Verdict:** ________ · **Date:** ________

> **Narrowed by Q7 = A, 2026-08-13.** The castle's own light is what stands the garrison
> back up in beat A2 — so option C below is no longer inconsistent with the opening. It now
> has a fiction that carries it: **you fight where the castle's light does not reach.**
> That does not decide it, but it removes the objection that used to rule C out.

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

**Blocks:** Q21 · **Blocked by:** ~~Q13~~ *(closed — C)*, Q14, Q23 · **Verdict:** ________ · **Date:** ________

- [ ] **A · Fixed roles.** Authored archetypes, same every time.
- [ ] **B · Player-chosen loadout** from a roster.
- [ ] **C · Whoever survived the last siege.** D3's named-persistence read to its conclusion.

> **Q13 = C raises the stakes here.** If the player's whole output is the squad, the seven
> cannot be interchangeable — each has to be a distinct verb or the choice of who to spend
> is hollow. **If Q14 = C, this is C by construction.** Still gated on Q23.
>
> **Narrowed by Q29 = A (2026-08-13):** the palette is fixed — the seven are drawn
> mixed across the four class archetypes (line / guardian / hunter / light). What
> remains open here is the *split* across seven bodies and which of A/B/C picks it.

---

## Q4 — Enemy: real planner or authored strategy set

**Blocks:** — · **Blocked by:** — · **Verdict:** ________ · **Date:** ________

- [ ] **A · A real planner** — goal decomposition over the front ledger.
- [ ] **B · A small authored strategy set** with weighted selection.

> **Recommendation: B for the vertical slice.** Dramatically cheaper and probably
> indistinguishable at slice scale. A is a real project; take it deliberately or not at all.

---

## Q10 — Does the withdrawal count persist?

**Blocks:** — · **Blocked by:** ~~Q1~~ *(closed — A)*, Q9 · **Verdict:** ________ · **Date:** ________

- [ ] **A · A mark on the boss and nothing else.** It exists only as `Column-fed`.
- [ ] **B · A stat that feeds the keep ratchet** (D3) — people saved are garrison later.
- [ ] **C · Both.**

> **B is the one that makes the opening matter mechanically rather than only dramatically**,
> and it is the first thing the game asks the player to care about. **Q1 = A gives "persists"
> a meaning:** the siege boundary. People saved in the intro would be garrison at the start
> of siege 2.

---

## Q19 — Where the game saves

**Blocks:** — · **Blocked by:** ~~Q1~~ *(closed — A)* · **Verdict:** ________ · **Date:** ________

- [ ] **A · At the siege boundary only.** Q1 = A makes the persistent set change wholesale
      exactly once per siege, so this is the only point where "the save" is unambiguous.
      A quit mid-siege loses the siege.
- [ ] **B · Siege boundary, plus a resume point at each gate transition.** Seams are already
      hard pauses with a natural stop. Costs a mid-siege snapshot of live zone state.
- [ ] **C · Continuous.**

> **Recommendation: B.** A is the honest reading of Q1 = A, but a siege is a long session to
> ask someone to hold in one sitting, and the seams are free pauses that already exist.
> **The cost is real and should be named:** B means serialising a Live zone mid-fight, which
> A does not.

---

## Q30 — Fate of gold and the shops under the siege · **CLOSED**

**Blocks:** `GDD.md` Q26 (gold rate/sources), Q34 (rung price) · **Blocked by:** — ·
**Verdict: B** · **Date: 2026-08-13**

The 2026-07-31 economy — gold drops → merchant + secret shop at stage beats → item
stash persists → Supply capacity as the merchant's headline good — lost its venue
(no beats) and its headline good (Q31 moves Supply to the war) in the pivot.

- [ ] **A · Gold retires.** Items from drops/war outcome; keep points the only spend.
- [x] **B · Gold survives; the merchant becomes the castle's quartermaster.** Venue
      lives in **held layers** (the Crown between sorties; venue detail open) and
      **the stock is war-coupled: lose the Works, lose the stores** — stock thins
      and prices climb as the castle shrinks, so the economy feels the war without
      a new system. The stash persists (D12 unchanged). Headline goods: items for
      the seven, and **off-branch adaptation rungs** (Q31-GDD's shop-stock rule,
      retargeted to the squad).
- [ ] **C · One merged currency.** Reopens the repair-vs-items competition the
      2026-07-31 direction deliberately ruled out.

> **TAKEN — B, 2026-08-13.** Keeps every 2026-07-31 economy decision that still has
> a leg (gold, stash, shop-stocked rungs) and the non-competition rule. `GDD.md`
> Q26 (gold's rate and sources) and Q34 (rung pricing) stay open — nothing here
> prices anything.

---

## Q31 — Who Supply/upkeep governs now · **CLOSED**

**Blocks:** — · **Blocked by:** — · **Verdict: A** · **Date: 2026-08-13**

Upkeep existed to govern retinue *size* (`GDD.md` §7). A fixed seven makes that
meaningless for the squad — and `castle-layout.md` §3.2 already points upkeep at
the war: the Works fall and the garrison starves.

- [x] **A · War-economy only.** Upkeep governs the garrison; degrade-don't-die
      unchanged in kind; L2's fall is the starvation lever. **The seven draw no
      upkeep, and there is no player-facing Supply purchase** — the merchant's old
      headline good is gone (see Q30 for its replacements).
- [ ] **B · The seven also draw upkeep.**
- [ ] **C · Defer.**

> **TAKEN — A, 2026-08-13.** The 2026-07-31 "Supply capacity is the merchant's
> headline good" amendment is superseded; the degrade formula itself
> (`economy.json`) survives untouched as garrison tuning.

---

# TIER 3 — scope and cleanup

## Q3 — Fate of the kills → army-level ratchet · **CLOSED**

**Blocks:** — · **Blocked by:** ~~Q1~~ *(closed — A)* · **Verdict: C** · **Date: 2026-08-13**

> **Promoted by Q1 = A.** Repair between sieges is a *spend*, so the keep ratchet now has a
> sink and no source. **Q29 = A added a second sink** — the seven's mid-fight rung climbs
> need a driver.

It is **shipped, working C++** — per-squad and per-hero attribution in `Mass/SwarmSubsystem.h`.

- [ ] **A · Feeds the squad ratchet.**  - [ ] **B · Feeds the keep ratchet.**
- [x] **C · Feeds both — split by attribution.**  - [ ] **D · Retires.**

> **TAKEN — C, 2026-08-13.** Kills credited to the seven and to the player's channelled
> abilities pay the **squad ratchet** (the Q29 rung climbs). The **keep ratchet** is paid
> by **war outcome — fronts and layers held, normalised — not raw garrison body count**,
> which is a huge number from a sim the player does not control and would be noise. The
> per-squad / per-hero attribution split maps 1:1 onto the squad side; the hero channel
> becomes "kills channelled through you," Q13 = C made mechanical. Written to
> `castle-layout.md` §8 and §10. **Rates, thresholds and the war-outcome formula are
> tuning, not decided.**

---

## Q6 — Can the war be won, or only survived? · **CLOSED**

**Blocks:** — · **Blocked by:** ~~Q1~~ *(closed — A)* · **Verdict: C** · **Date: 2026-08-13**

> **Q1 = A left a hole here that must not be filled by inference.** "Ends when the Crown
> falls **or a relief condition is met**" — that clause is a placeholder. **What actually
> ends a siege in the player's favour is undecided.** Do not default to "survive N waves."

D2 says the *castle* only falls. It does not say the *war* is unwinnable. **Tone-defining.**

- [ ] **A · Winnable** — there is an end and you can reach it.
- [ ] **B · Survivable only** — the question is how long and at what cost.
- [x] **C · Winnable but not by you** — you buy time for something else to decide it.

> **TAKEN — C, 2026-08-13, and the "something else" is named: the relief is another
> bearer.** You hold until a second flame answers yours. This gives
> `FLAME-FOUNDATION.md` §4.4 — *uniting flames as a run objective*, decided
> 2026-07-27 — its mechanical home: the war is won by the uniting, not by the
> castle. **How progress toward relief is earned, paced and shown is open — do not
> infer "survive N sieges."** Written to `castle-layout.md` §4.1. The narrative
> pass connecting this to FLAME-FOUNDATION is a follow-up, not done here.

---

## Q32 — Fate of light fragments · **CLOSED**

**Blocks:** — · **Blocked by:** — · **Verdict: A** · **Date: 2026-08-13**

The 2026-07-31 route — fragments gather in-run, a threshold converts them into a
healer unit, boss fragments drop as modifiers — belongs to the retired design
(Q15 says so for the healer half). Under the pivot, healing already has two homes:
the garrison's triage anchors rooted at the Lantern Court, and the Guided-light
archetype as one of the seven (Q29 = A).

- [x] **A · Fragments retire entirely.** Healing = light-archetype soldier +
      garrison. `GDD.md` Q24 (gather method) closes as superseded — the currency it
      priced no longer exists. One less currency in the game.
- [ ] **B · Boss-drop modifiers only.**
- [ ] **C · Defer.**

> **TAKEN — A, 2026-08-13.** Written to `GDD.md` §3 and Q24 (markers). Whether the
> light archetype's raise reaches the seven themselves is still Q15.

---

## Q11 — Zoom range vs. the flame pool and dither

**Blocks:** — · **Blocked by:** ~~Q7~~ *(closed — A)* · **Verdict:** ________ · **Date:** ________

> **Narrowed by Q7 = A.** The castle has fixed light of its own, so the flame pool is no
> longer the only light in frame and its screen proportion matters less than when it was
> everything. The dither anchoring is untouched by that and still bites.

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

- **2026-08-13 · Q13 = C** — the player's entire output routes through the seven.
  Written to `docs/design/castle-layout.md` §6.3 and §12. **Opened Q23** (the ability kit)
  and retired `HeroDamage` as a player-damage number, keeping the Actor grid-bridge.
- **2026-08-13 · Q1 = A** — a siege is the session; how deep the enemy got sets the state
  the next one opens in. Promotes `castle-layout.md` §4.1 from recommendation to decision.
  Leaves the relief condition explicitly unwritten (Q6).
- **2026-08-13 · Q7 = A** — the castle has its own light; your flame is portable light in a
  place that has fixed light. Written to `castle-layout.md` §10 and §12. The leash survives
  unchanged in kind at a new scale, and **a fallen layer going dark becomes the in-fiction
  reason the collapse is one-way.**
- **2026-08-13 · Q29 = A** — the four classes survive as squad archetypes: mixed seven,
  no class select, mid-fight evolution up the tiered adaptation tree. Written to
  `CLASSES.md` (banner) and `GDD.md` §5 (marker). Narrows Q2 (palette fixed, split open)
  and Q14 (depth axis settled), sources Q23 from the four hero kits. The climb mechanism
  is deliberately unspecified — do not infer.
- **2026-08-13 · Q3 = C** — kills feed both ratchets, split by attribution: squad-credited
  kills (incl. the hero channel) pay rung climbs; war outcome (fronts/layers held,
  normalised) pays keep repair. Written to `castle-layout.md` §8 and §10. Answers the
  climb-mechanism gap Q29 left open; rates and formulas are tuning.
- **2026-08-13 · Q30 = B** — gold and the stash survive; the merchant becomes the
  castle's quartermaster, and its stock is war-coupled: the shop is made of the layers
  you still hold. Stocks items for the seven + off-branch rungs. Written to `GDD.md` §8
  (marker). GDD Q26/Q34 stay open.
- **2026-08-13 · Q31 = A** — Supply/upkeep belongs to the war: it governs the garrison,
  the Works' fall starves it, the seven draw none, and no player-facing Supply purchase
  exists. Written to `GDD.md` §7 (marker); the degrade formula survives as garrison
  tuning.
- **2026-08-13 · Q6 = C** — the war is winnable but not by you: **the relief is another
  bearer** — hold until a second flame answers yours. Uniting flames
  (FLAME-FOUNDATION §4.4, 2026-07-27) gets its mechanical home. Written to
  `castle-layout.md` §4.1. Pacing/earning of relief progress is open — do not infer.
- **2026-08-13 · Q32 = A** — light fragments retire entirely; healing = the
  Guided-light archetype among the seven + the garrison's triage anchors. Closes
  `GDD.md` Q24 as superseded. Written to `GDD.md` §3 and Q24 (markers).
