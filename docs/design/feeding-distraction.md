# Feeding / distraction mechanic

Spec for task-053, amended by task-061: a killing blow makes the killer go null
on the corpse for a duration keyed to what it killed; corpses now persist to
the end of the round and any unit that walks up to one can claim an open slot,
not just the killers. Extends `SYSTEMS.md` §1 (entity tiers) and §6 (retinue
tuning), and reads `GDD.md` §4 (hero relevance), §7 (upkeep/soft caps), §10
(Mass Entity constraints). Spec only — `task-054` builds this in Mass.

**2026-07-28 amendment (task-061), read this before §5/§8:** the owner
overrode task-053's §5 no-persistent-corpse decision. Corpses are now
persistent, walk-up-claimable entities. §5 and §8 below are rewritten for
that; the superseded reasoning is kept inline as the record of why the call
changed. See §13 for the dated decision log entry.

**Reader's map, in the order the "Done when" checklist asks for it:** §1 what
"null" means · §2 who "the killer" is (load-bearing, everything else depends on
it — killers still get a guaranteed first claim) · §3 the feed-duration curve ·
§4 the three-feeders-per-corpse rule, now a lifetime cap with a
death-reopens-the-slot exception · §5 **[rewritten]** what a corpse is, how
long it persists, and how walk-up claiming works · §6 whether feeding pays ·
§7 the symmetry/tone problem · §8 **[rewritten]** the density check, re-run for
a field that accumulates bodies for a whole round · §9 CVar dials · §10 Mass
cost breakdown · §11 handoffs · §12 narrative requests · §13 canon proposals ·
§14 simulation notes.

---

## 0. The one-paragraph version

A unit that lands a killing blow has a chance to go null and spend a duration
(set by the dead unit's `MaxHP`, 1.5–8s) rooted at the kill, unable to fight or
move but still a normal, killable target. Up to 3 units can feed off one corpse
**over its whole lifetime** (§4) — the killer set gets a guaranteed first claim
on however many slots it fills, and **the corpse then persists on the field
until the round ends** (owner override, task-061), so any later unit of the
*opposing* team that walks within `Swarm.Feeding.ClaimRadius` of it can claim
whatever slots are still open, gated by a low per-second hazard rate rather
than a one-time roll. A corpse is a small, non-Mass record in `USwarmSubsystem`
(§5) — it never steers, fights, or collides, and a population cap plus a
decaying feed-duration on stale bodies keep both its cost and its pull on the
armies bounded (§5, §8). **The arithmetic in §8 says the walk-up version, left
unthrottled, would tax the retinue's line with a near-permanent floor on top of
the original kill-triggered load** — the retinue is already losing the
zero-input baseline fight, so a standing tax lands harder on it than on the
700-strong brood pool. Two new dials (`RetinueClaimRate ≈ 0.041/s`,
`BroodClaimRate ≈ 0.027/s`) plus a stale-body duration discount bring the
worst case back into roughly the same risk band task-053 originally accepted —
worse at the short/high-crowd edge, comparable or better in longer fights. See
§8 for the honest verdict on whether that's good enough.

---

## 1. What "null" means

Four independent switches. All four decisions below are driven by one framing:
**feeding must read as a vulnerability, not a reward.** The owner asked for
*distraction*, and a distraction that's also a shield is a different, better
mechanic than the one that was asked for.

| Switch | Decision | Why |
|---|---|---|
| Stops dealing damage | **Yes** | The whole point — this is the "hole in the line." |
| Stops moving | **Yes** | Rooted at the kill location; it reads as "occupied," which is what a distraction should look like. |
| Stops being targetable | **No — stays targetable** | See below. |
| Stops taking damage | **No — stays vulnerable** | See below. |

**Feeding units stay targetable and damageable, on purpose.** A feeder that's
also invulnerable is a body-block the player can farm on command (park three
soldiers on a kill, get three free seconds of literal invincibility). Keeping
them vulnerable makes feeding a real risk each time it happens — "my soldier
got caught mid-kill and died for it" is a legible, sympathetic story beat, and
it's the honest read of "distracted": not hiding, just not fighting back.
It also means the opposing side gets a genuine counterplay (a feeding unit is a
guaranteed, free kill for whoever reaches it) without any new AI logic — see
§10, this falls out of the existing team/distance check for free.

**What the player is meant to read off it:** a unit standing still, animation
locked into an eating/stripping/kneeling pose (render hook — see §11), taking
hits without responding. It should look exactly like what it is: a soldier who
stopped paying attention to the fight for a few seconds, and can die for it.

---

## 2. Who is "the killer" — no attribution exists today, this defines it

`SwarmCombat.h:10-14` is explicit: combat has no damage events and no kill
attribution. But the frame a victim's `HP` crosses `≤0`, `USwarmCombatProcessor`
has *already computed*, for that exact victim, how many enemies struck it that
frame (the local `Strikers` count in the per-victim loop, bounded by
`MaxAttackersPerUnit`) — that data just isn't kept past the loop iteration
today.

**Definition: the killer set of a victim is every attacker whose blow landed on
it during the frame its HP crossed `≤0`**, capped at `Swarm.Feeding.MaxPerCorpse`
(3). If more than 3 landed a blow that frame (possible up to
`MaxAttackersPerUnit`, 4), the excess attacker(s) get nothing extra and simply
keep fighting — a small, deterministic, already-bounded overflow, not a new rule.

This is deliberately **not** "whoever struck the fatal blow" in a single-attacker
sense — with cleave (`RetinueTargetsPerHit`, `BroodTargetsPerHit`) and a
surrounded victim, it's common for 2-4 attackers to land a blow on the same
victim in the same frame, and that IS the "surrounded" scenario `SwarmCombat.h`
§56-59 already treats as the defining lethality event. Multi-attacker killer
sets aren't an edge case here, they're the mechanic doing exactly what it was
asked to do: a target that dies surrounded produces a bigger distraction than
one that dies to a single clean hit. See §8 for why this matters more on one
side than the other.

**Why this is cheap:** it reuses the neighbor walk `USwarmCombatProcessor`
already performs to compute `BlowsClaimed` — no new spatial query. The one real
cost: `FGridEntry` (`SwarmSubsystem.h:130`) doesn't currently carry an entity
handle for the attacker, only its location/team/reach, so the victim's loop
can test "did this land a blow on me" but can't yet say *which entity* landed
it. Task-054 needs to widen `FGridEntry` with an `FMassEntityHandle` and have
the victim record up to `MaxAttackersPerUnit` claimant handles when it dies.
That's a struct-widening + a small fixed-size array, not a new cross-entity
write pattern — flagged as the one moderately-expensive item in §10.

**Task-061 addition: killers get a guaranteed first claim.** Everything above
is unchanged from task-053. What changes with persistent corpses (§5) is what
happens to the *rest* of the corpse's slots after the death frame: the killer
set is no longer the only way to fill a slot, but it is always resolved first
and always wins the slot if it wants it — a killer's claim never has to
compete with a later walk-up claimant for the same slot (§5 explains why this
falls out of processing order for free). Killer-set members still roll a
per-kill chance to actually enter feeding (now named
`Swarm.Feeding.RetinueKillerClaimChance` / `BroodKillerClaimChance`, §9 — same
mechanic as task-053's `RetinueFeedChance`/`BroodFeedChance`, renamed because
there are now two distinct claim channels and the old names stopped saying
which one they meant). Whatever slots the killer set doesn't fill (most kills
are a single clean blow, not a 3-attacker pile-up — §8) are what's left open
for §5's walk-up channel.

---

## 3. Feed-duration curve, keyed to `MaxHP`

```
FeedDuration(MaxHP) = clamp(MaxHP × ChompRate, MinDuration, MaxDuration)
```

`ChompRate` (s/HP), `MinDuration`, `MaxDuration` are all CVars (§9). Linear and
clamped, not a curve with breakpoints — there's only one input signal
(`MaxHP`) worth respecting until armor exists, and a clamp is enough to keep a
future titan from freezing a unit for an absurd span.

At shipped defaults (`ChompRate = 0.05`, `MinDuration = 1.5`, `MaxDuration = 8.0`)
and the live combat CVars (`SwarmCombatProcessors.cpp`: `Swarm.BroodMaxHP = 60`,
`Swarm.RetinueMaxHP = 130`):

| Corpse | `MaxHP` | Feed duration | Who feeds on it |
|---|---|---|---|
| Brood | 60 | **3.00s** | retinue killer (§7: "The Rite") |
| Retinue/Spearmen | 130 | **6.50s** | brood killer (§7: "The Feed") |
| *(future) a 2000-HP titan* | 2000 | **8.00s (clamped)** | either — the ceiling doing its job |

**Where the `MaxHP` proxy will read wrong once `task-002` lands armor:** `MaxHP`
alone conflates "hard to kill because tough" with "hard to kill because heavily
plated." A high-HP-low-armor brute and a low-HP-heavily-armored elite would
currently produce the same feed duration if their `MaxHP` matched, but only the
second one should read as "a big armored death." **Clean handoff for task-002:**
swap the formula's input from `MaxHP` to an effective-toughness figure once
armor exists (e.g. `EffectiveHP = MaxHP / (1 - DamageReduction)`, or
`MaxHP + Armor × ArmorToDurationWeight`) — this is a **single substitution at
the `FeedDuration()` call site**, wherever task-054 implements it. Nothing else
in this spec needs to change: the clamp, the per-corpse cap, the killer-set
definition, and the density bounds in §8 are all agnostic to what produces the
input number.

---

## 4. How three feeders share one corpse — full duration each, not divided

**Decision: each feeder in the killer set serves the full `FeedDuration`
independently. Crowding does not speed anything up.**

This is the single most important tuning call in the spec, and it's forced by
the owner's own stated fantasy: *a big armored death punches a hole in the
enemy line.* A shared-pool model (3 feeders split the duration three ways,
finishing in a third of the time) makes crowding **efficient** — the smart play
becomes throwing 3 units at every kill to clear it fast, which produces a
*smaller* hole, not a bigger one, and turns feeding into free-to-ignore chip
time. Full-duration-each makes crowding a **trap**: if 3 attackers happen to
land the fatal blow together (the "surrounded" scenario §2 describes), all 3
are held out of the fight for the *entire* duration, not a third of it. That's
the actual hole — three fewer bodies fighting for the next several seconds,
scaled by exactly how decisively that kill happened.

It also reads correctly on both sides of the same kill: a brood horde that
swarms a soldier 3-deep and gets the kill *should* pay for that pile-up with
three brood briefly out of the fight, not be rewarded with a faster kill. Full
duration each is the only formulation where "gang up for the kill" costs the
gang something.

**Cost note:** this is also the cheap formulation for Mass — no shared,
mutable "corpse remaining" counter that every feeder would need to read/write
against each frame (which would be exactly the cross-entity contention
`SwarmCombat.h:10-14` rules out). Each feeder just carries its own countdown.
Zero coordination between the up-to-3 feeders is required after the killer set
is resolved at the death frame.

**Task-061 addition: the cap of 3 is now a lifetime cap, not just a
per-instant cap.** Task-053 never had to answer this — a corpse that fades the
moment its last feeder finishes has no "after" for slots to matter in. A
corpse that sits on the field for up to a whole round does, and §2's "no late
joiners" is exactly the rule the owner overrode, so this has to be settled:

- **A slot, once claimed, does not reopen when its feeder finishes normally.**
  Fictionally, a body that's been fully sealed ("The Rite") or fully consumed
  its share ("The Feed") by 3 separate feeders over its lifetime has nothing
  left — reopening the slot would mean the same 3 pounds of corpse feed a 4th,
  5th, 6th unit indefinitely, which turns one kill into an unbounded feeding
  trough over a long round. `OpenSlots` starts at 3 and only ever decrements
  on a successful claim (killer or walk-up) — this is the entire rule, and it
  needs no new bookkeeping beyond what §6 already tracks (whether a feed ended
  in completion or interruption).
- **A slot *does* reopen if its feeder is killed mid-feed** (§5's "still a
  valid target" rule, carried over from task-053 §5, still true). An
  interrupted feed didn't finish consuming its share, so `OpenSlots += 1` when
  a feeding unit dies before its countdown reaches zero. This is the one case
  where a corpse can end up hosting more than 3 *total* feeders across its
  life — never more than 3 *concurrently* or 3 *completed*, but a corpse that
  keeps getting its feeders killed can keep re-offering the same slot. That's
  the correct read: a body that's contested enough that feeders keep dying on
  it is a body that's still legitimately fought-over, not one that's been
  fully claimed.
- **Practical effect:** a corpse's slot economy is exactly 3 *successful*
  feeds over its lifetime, full stop — this is what makes §5's population
  model tractable, since "how much feeding capacity does the field currently
  hold" only depends on `open corpses × open slots`, not on any per-corpse
  history beyond that one counter.

---

## 5. What a corpse is, how long it persists, and how walk-up claiming works

**Superseded 2026-07-28 (task-061) — kept as the record of why the call
changed.** Task-053's original answer to this section was a firm no:

> *There is no persistent corpse entity, and there are no late joiners. The
> killer set is resolved once, at the death frame (§2), and is final...
> modeling "open slots on a lingering body" would be exactly the kind of
> per-corpse shared mutable state §4 already rejected for cost reasons... The
> corpse itself is a render-only artifact — a sprite left at the death
> location for however long the longest of its feeders is still counting
> down, then it fades. There is no simulated "Corpse" Mass entity, no
> slot-tracking data structure, and no despawn-timer edge case to design
> around...*

That reasoning was sound on its own terms and the cost concern it raised was
real — walk-up eating does need a new per-frame(ish) spatial pass, and §8
shows it does add real load. The owner read that reasoning and overrode it
anyway: *"downed units persistent state until the round is over. Those can
get eaten by the enemy like our gameplay plan."* Given a direct choice between
killer-only persistence and full walk-up persistence, with the cost of the
walk-up version shown, the owner chose walk-up. That is the current design.
The rest of this section replaces the quoted text above.

### 5.1 What a corpse is at the sim level

**Not a Mass entity.** A corpse never steers, fights, or collides — giving it
a full Mass archetype (locomotion fragment, combat fragment, its own chunk
iteration) would mean carrying entity-management overhead for something with
no behavior at all, which is a worse fit for design law 5 than fodder gets,
not a better one. Instead, a corpse is a small plain-old-data record living in
`USwarmSubsystem` alongside the existing `Grid` (`SwarmSubsystem.h:582`) — two
fixed-capacity arrays, one per team (§5.3 explains the split), indexed by a
generational handle (index + a small version counter, the same "slot map"
pattern any array that reuses freed slots needs — see §10 for why this
specific detail is a correctness requirement, not a nicety).

Minimum fields, per corpse:

| Field | Type | Why it's needed |
|---|---|---|
| `Location` | `FVector` | Render position; anchor for the claim-radius query (§5.2). |
| `Team` | 1 bit | Which team *died* here — fixes which side may claim (§5.3) and which fiction/pose applies (§7, unchanged: opposing team only). |
| `MaxHP` | `float` | The dead unit's `MaxHP`, captured once at corpse creation — feeds `FeedDuration()` (§3) for every claim on this corpse, killer or walk-up. |
| `SpawnTick` | `float`/frame counter | Corpse age — drives the population cull order (§5.4) and the stale-duration discount (§5.5). |
| `OpenSlots` | `int8`, 0–3 | §4's lifetime-cap counter. Decrements on any successful claim, increments back only if a feeder dies mid-feed. |

That's it — five small fields, no per-corpse array of feeder handles needed
(each feeder already knows which corpse it's on, via its own fragment; the
corpse doesn't need to know which units are currently eating it, only how
many open slots remain).

### 5.2 Walk-up claiming: units notice, they don't seek

The task brief drew the load-bearing distinction correctly: *"whether a unit
actively seeks out corpses or only notices one it happens to be standing
near — those are very different mechanics and only the second is cheap."*
**This spec picks the second, and rules out the first outright.** Feeding is
not a steering objective. No unit ever breaks off its current target, its
formation slot, or its stance behavior to walk toward a corpse — that would
mean touching `SwarmProcessors.cpp`'s steering code, which task-054 doesn't
own (task-046/052 do), and it would turn every corpse into a magnet competing
with combat and formation-following for the same movement budget. A unit only
becomes a candidate for a corpse it is *already* near, as an incidental
consequence of its normal combat/formation movement having carried it there —
exactly the "gang up on a kill, corpse happens to be where the fight already
is" scenario §2 describes for killers, just extended to any unit that later
passes through the same ground.

**Claim radius: `Swarm.Feeding.ClaimRadius = 150uu`.** Chosen relative to two
existing constants so it reads as "standing next to it," not "detects it from
across the field": `Swarm.MeleeRange` is 95uu (so 150uu is close enough that a
unit fighting one step away from a corpse still qualifies, but a unit two
fights over does not), and it's comfortably inside `GridCellSize` (250uu,
`SwarmSubsystem.h:40`) so the claim query can reuse the same "own cell + 8
neighbors" scan the combat pass already performs — no new grid resolution, no
new query shape, just a new *walk* over that existing structure (§5.6).

**Resolution is corpse-centric, not unit-centric, and that's what keeps it
cheap.** Rather than every one of ~820 units asking "is there an unclaimed
corpse near me" every tick, the claim pass runs the other direction: for each
corpse with `OpenSlots > 0` (at most `2 × Swarm.Feeding.MaxCorpsesPerTeam`,
§5.4 — a number in the low hundreds, not the high hundreds), query the
existing spatial grid within `ClaimRadius` of the corpse's `Location` for
eligible units of the *opposing* team (alive, not already feeding, off
cooldown). This is the "far fewer corpses than units" formulation task-054's
brief already anticipated — a bounded, dedicated pass over corpses only, using
the grid that's already built every frame for combat, not a new per-unit
query. §5.6/§10 have the cost accounting.

**The claim itself is a hazard rate, not a one-shot roll.** A kill-triggered
claim (§2) happens once, at a single frame, so a single Bernoulli roll made
sense. A walk-up candidate can remain in range of an open corpse for many
seconds — rolling once per tick at the old `FeedChance` magnitude would mean
sustained exposure converges to "will definitely claim eventually," which
defeats the point of a probability. Walk-up claims are gated by a **per-second
rate** (`Swarm.Feeding.RetinueClaimRate` / `BroodClaimRate`, §9) converted to a
per-tick probability as `1 − (1 − Rate)^dt`, evaluated only while a unit is
in-range, un-fed, and off cooldown. §8 derives the values and shows the
resulting steady-state duty cycle in closed form.

### 5.3 Who may claim — opposing team only, still no mutual eating

**A corpse can only ever be claimed by the team that did *not* die there.**
This isn't a new rule so much as making explicit what §2/§7 already implied:
"the killer" is definitionally the winning side of that exchange, and §7's
fork (retinue "seals" a brood corpse, brood "devours" a retinue corpse) is
keyed to the corpse's *fixed* team, never the claimant's. Extending that same
rule to walk-up claims means a corpse's fiction and animation are settled
once, at creation, and never need to branch per-claimant — which directly
answers the task brief's question of "whether a body can be eaten by both
sides over its lifetime": **no.** A brood corpse is retinue-claimable only,
for its entire life; a retinue corpse is brood-claimable only. This is also
what keeps §5.1's `Team` field a single bit instead of a per-slot record, and
it rules out the narratively wrong case of a retinue soldier "sealing" a
fellow soldier's own body, which §7 already rejected once as "a different,
sadder trigger" when task-053 was choosing "The Rite" over "kneeling over a
fallen comrade."

### 5.4 Population bound — expected peak and the cull rule

Corpses accumulate for as long as they persist, and kills happen fast at
wave-3 density (§8's measured range: 7–28 brood kills/s, 1.5–6 retinue
kills/s). Left completely unculled, cumulative corpses over a single wave-3
run would run into the hundreds on the brood-corpse side alone (§8's model:
~560 brood corpses created across a 20–80s wave-3 fight) — clearly too many
bodies to carry indefinitely, and "the round" isn't even unambiguous: nothing
in current canon defines a "round" boundary distinct from "the whole 3-wave
run to win/loss" (`GATE1-FUN-PROTOTYPE.md` §1's deploy → wave 1 → wave 2 →
wave 3 → win structure is the only structure that exists). **This spec reads
"round" as that whole run** (deploy-to-win/loss), the closest existing analog,
and flags the reading as an assumption task-054 should confirm rather than
silently reinterpret. Under that reading, an unculled corpse population could
in principle exceed a *thousand* bodies by the time a 3-wave run ends — the
hard cap below is what actually matters, independent of which "round"
definition turns out to be correct.

**`Swarm.Feeding.MaxCorpsesPerTeam = 100`** (200 corpses total: up to 100
brood-team corpses claimable by retinue, up to 100 retinue-team corpses
claimable by brood — split per team, not shared, so one side's higher kill
rate can't starve the other side's claim supply). At measured wave-3 rates
this cap **binds almost immediately** — §8's model shows it's hit within
3.6–14.3s of brood-corpse creation and 16.7–66.7s of retinue-corpse creation,
i.e., for most of a wave-3 fight the corpse count is pinned at the cap, not
still growing. That means 100/team, not the uncapped cumulative total, is the
honest "peak corpse count" answer: the population reaches and holds at 200
total for the majority of wave 3, well below the 700+120 unit count already
on screen (a ~24% addition to the existing per-frame grid-neighbor query
volume — see §10 for why that's the real cost line item, not corpse count on
its own).

**Cull rule, evaluated only when a new corpse would exceed the per-team cap:**
cull the oldest corpse (`SpawnTick`) **among corpses with zero active
feeders.** A corpse currently hosting even one in-progress feed is never
culled — the feeder's fragment doesn't need the corpse struct to keep counting
down (§4's private-countdown design still holds), but yanking the corpse out
from under a unit mid-animation would be a visible pop, not just a bookkeeping
nicety, so it's excluded on cost *and* on polish grounds. If every corpse at
the cap somehow has an active feeder (only possible if `MaxCorpsesPerTeam` is
tuned far below the realistic concurrent-feeder count, which §8's numbers say
it isn't by roughly an order of magnitude), the new corpse is simply not
created — the kill still resolves normally through `USwarmDeathProcessor`,
the entity is destroyed as it is today, there's just no body left for future
feeders. A graceful skip, not a crash or a stall.

### 5.5 Stale bodies feed faster — the §8 guard rail

New for task-061, and load-bearing for §8's result: a walk-up claim (never a
killer claim — see below) started on a corpse older than
`Swarm.Feeding.StaleAge` (15s) uses a *discounted* feed duration,
`FeedDuration() × Swarm.Feeding.StaleDurationScale` (0.5). Fictionally: a body
that's been lying on the field being sealed or eaten for a while doesn't have
as much left to chomp through as a fresh one. Mechanically: this is the one
rule that actually shrinks with time rather than staying flat, which is
exactly what §8 needed — the walk-up channel's problem isn't that it's active,
it's that it never turns off, so the fix that matters most is one that makes
its *cost per claim* shrink the longer a fight runs, not just its arrival
rate.

**Only walk-up claims discount. Killer claims always get the full, undiscounted
duration**, because a killer claim only ever happens at a corpse's age-zero
death frame (§2) — the discount can never apply to it, so this isn't a special
case to implement, it falls out of "claim age" being definitionally 0 for
every killer claim.

### 5.6 What happens at round end

Round end (win or loss transition) clears all corpse records outright — this
is a run-scoped encounter (design law 7's loot framing already treats a run as
its own reset), and there is no next-wave or next-floor reason for a body to
survive past it. A feeder mid-meal when the round ends is treated as an
interruption, identically to dying mid-feed: no heal (§6's "uninterrupted
completion only" already covers this — round-end is just one more way a feed
can fail to complete, not a new rule), fragment cleared along with everything
else the end-of-run teardown already resets.

---

## 6. Does feeding pay

**Yes — a modest self-heal, paid only on completing the full duration, never
on interruption.**

`Swarm.Feeding.HealFraction` (default **0.20**) of the feeder's own `MaxHP` is
restored when its countdown reaches zero uninterrupted. A unit killed mid-feed
gets nothing — the heal is the payoff for surviving the vulnerability window
described in §1, not a reward for the act of feeding itself. Since feed
duration already scales with the corpse's `MaxHP` (§3), the heal scales with it
too for free: a bigger kill is a bigger risk *and* a bigger payoff if the
feeder survives it, which is the same asymmetric-stakes shape as everything
else in this spec.

This keeps the mechanic consistent with being involuntary: nobody is choosing
to feed for the heal (there's no player input here at all, §1's four switches
are the whole behavior), so the heal reads as "the payoff was incidental to an
instinct they couldn't control," not "the AI optimizes for free feeds." It also
gives the player something to feel good about when they watch their own line
survive a mob of feeding, which matters because §1 otherwise makes this a pure
downside for the killer's side.

---

## 7. The symmetry problem, resolved

**The mechanic stays symmetric — the owner asked for both sides to feed, and
nothing in the arithmetic (§8) forces dropping that. The fiction differs per
side, keyed off which team the killer belongs to, not which corpse exists.**

Because "the killer" (§2) is always the side that *won* that exchange, and it
always pauses over the enemy corpse it just made (never its own dead), the
fork is clean:

- **Brood killer, pausing over a fallen retinue soldier → "The Feed."**
  Literal. Brood are monsters; devouring what they killed is the horror beat
  the tone framing (design law 9, `docs/backlog/task-053...md`) explicitly
  wants the player to feel — *what you fight was taken, not born hostile* — a
  brood gorging on a soldier it just overwhelmed should be genuinely upsetting
  to watch, not glorified. No tone problem on this side; it needs one.

- **Retinue killer, pausing over a fallen brood → "The Rite."** Not eating.
  The retinue soldier plants a hand on the corpse and **seals it** — a beat
  that burns or marks the body so the dark can't reclaim it. This isn't a
  cosmetic reskin of the same animation: it's chosen because it's the one
  framing that's actually **on-canon**, not just tone-safe. `FLAME-FOUNDATION.md`
  §1 states the premise's central physical law as *"the dark is the condition
  of everything, and it takes what it touches."* A fallen brood is something
  the dark already took once; "sealing" it so that doesn't happen twice gives
  the retinue's pause a real stake tied directly to the world's own rules,
  instead of borrowing "salvage" (which reads as looting, and sits oddly next
  to "we are the good guys") or "kneeling over a fallen comrade" (which is a
  different, sadder trigger — mourning an ally — not what's happening here,
  since the killer is always standing over the *enemy* it just killed, never
  its own dead).

**Why full symmetry survives:** the mechanical rule (killer set, duration
curve, full-duration-each, heal-on-completion) is identical on both sides —
only the fiction and the two feed-chance CVars differ, which is the same
"mechanically symmetric, tuned per team" pattern the codebase already uses for
`RetinueTargetsPerHit` vs `BroodTargetsPerHit`. Nothing about "The Rite" needs a
different formula, a different duration curve, or a different implementation
path from "The Feed" — it's the same system wearing two different verbs.

**Task-061 addition:** a walk-up claimant (§5.2) performs the identical Rite/
Feed beat as a killer claimant on the same corpse — same pose, same duration
formula (modulo §5.5's stale discount), same heal-on-completion. §5.3 already
settles that a corpse's team (and therefore which of the two beats plays) is
fixed at creation and never depends on who ends up claiming it, so this needed
no new fork: a soldier who wanders up to seal a brood corpse ten seconds after
someone else killed it is doing the same duty as the one who killed it.

**Proposed narrative follow-up** — see §11's Narrative requests. "The Rite" is
named here provisionally; it should get the narrative-director's actual pass
before it ships in UI/VO, since it's the more load-bearing of the two names
(the one doing tone-repair work).

---

## 8. The density check — re-run for a persistent, walk-up-claimable field

**Verdict up front:** the owner's walk-up version is **not** a stalemate, but
it is a genuine regression relative to the kill-triggered mechanic task-053
tuned, and it regresses in a different place than the obvious one. It doesn't
mainly hurt the short, frantic, high-kill-rate stretches of wave 3 — those
were already the risky case and stay roughly where they were. **It hurts the
calm stretches**, because a persistent, walk-up-claimable corpse field creates
a standing feeding tax that never fully turns off, even when the kill rate
drops. That's new, and it's the reason two new dials alone aren't enough — §8
also needed the stale-duration discount from §5.5 to keep the tax from being
flat for the length of the whole fight. With that guard rail in, the retinue's
worst realistic case (~25% at wave-3's shortest/most-crowded assumed duration)
lands *below* the old model's own already-shipped worst case (which went
mathematically impossible — over 100% — under the same assumption), and its
typical case (9–15% across most of the swept range) is worse than task-053's
~11% target but in the same order of magnitude, not a different regime.
**Recommendation: ship with the retuned defaults below, but this is still a
Fermi model — task-054 should reproduce the `Swarm.Feeding.Enabled 0/1` A/B
from §11 before treating these numbers as settled.**

### Method

Same starting point as task-053's model (Little's Law on the measured
wave-3 outcome, `docs/GATE1-FUN-PROTOTYPE.md`, "shipped now" 2026-07-25),
extended with a second mechanism for the walk-up channel, since persistence
breaks the assumption that let the original model treat "one kill → up to 3
feeders, resolved once" as the whole story. Full derivation and the Monte
Carlo cross-check are in the scratch script referenced in §14; the closed
forms are reproduced here because they're what the CVar defaults come from.

**Channel 1 (killer claim) is unchanged in form** from task-053: concurrent
killer-feeders = `kill_rate × KillerClaimChance × FeedDuration`. Only the
CVar's name changed (§2), and its value did — see "The bound," below.

**Channel 2 (walk-up claim) is new.** Because eligibility now persists for as
long as a corpse's open slots do (not just one death frame), the right model
isn't Little's Law on discrete arrival events, it's a birth-death process: at
any instant, a fraction `f` of the living, eligible army is feeding; units
leave the feeding pool at rate `1/D` (finishing) and join it at rate `λ × C ×
(1 − f)` (an eligible non-feeder rolls its per-second claim rate, discounted
by `C`, the fraction of the army within `ClaimRadius` of *some* open-slot
corpse at any given moment — modeled as a parameter, not measured, with `C =
1.0` — a dense melee scrum where corpses spawn exactly where the fighting
already is — treated as the design-target (worst) case, and `C = 0.5` shown
for sensitivity). Solving for the steady state:

```
f = C·λ·D / (1 + C·λ·D)          (equilibrium duty cycle)
λ = f / (C·D·(1 − f))            (rate needed to hit a target f)
```

This converges fast relative to a wave-3 fight — the relaxation time constant
is `1 / (Cλ + 1/D)`, which comes out to **2.7s for the retinue's tuned values
and 5.4s for brood's**, so equilibrium is reached well inside even the
shortest (20s) assumed wave-3 duration in the sweep, and the closed form is a
fair stand-in for a full simulation across the whole range. A discrete-event
Monte Carlo (per-unit Bernoulli-per-tick, independent implementation of the
same rules) confirmed the closed form at the dur=45s midpoint: target `f =
0.110` → simulated `0.1155`; target `f = 0.150` → simulated `0.1634`. Both
land within Monte Carlo noise of the closed form (the MC running slightly
high is expected — it's tracking a finite population, not an infinite-server
approximation — and gives a small safety margin in the conservative direction
for the retuned defaults below).

**Measured wave-3 inputs, unchanged from task-053:** retinue refills to 120
between waves, wave 3 spawns 700 brood (`SYSTEMS.md:45`), shipped defaults
produce a full retinue wipe with 131–145/700 brood surviving — retinue killed
~555–569 brood (mid **562**) before dying; brood killed all 120 retinue. Same
missing-input caveat as before: wave-3 duration isn't broken out from the
~95s whole-run figure, so the same **20–80s sweep** stands in for it.

### Result — retinue side (feeding on brood corpses, "The Rite," 3.00s fresh)

`RetinueKillerClaimChance = 0.20` (down from task-053's `0.35` — see "The
bound"), `RetinueClaimRate = 0.0412/s` at `C = 1.0` (target walk-up
equilibrium `f = 0.11`, matching task-053's original target so the two
channels can be compared on the same footing):

| assumed wave-3 duration | brood kills/s | killer channel | walk-up channel (fresh-duration equilibrium) | **total, % of 120** |
|---|---|---|---|---|
| 20s | 28.1/s | 16.9 units | 13.2 units | **25.1%** |
| 30s | 18.7/s | 11.2 units | 13.2 units | **20.4%** |
| 45s | 12.5/s | 7.5 units | 13.2 units | **17.2%** |
| 60s | 9.4/s | 5.6 units | 13.2 units | **15.7%** |
| 80s | 7.0/s | 4.2 units | 13.2 units | **14.5%** |

The walk-up channel's contribution is flat across the sweep (13.2 units at
every duration) — that's the "never turns off" problem stated plainly as a
number: it's a standing 11% tax on the retinue's own line for the *entire*
fight, independent of how the fight is actually going, on top of whatever the
killer channel is doing. **§5.5's stale-duration discount is what keeps this
from being the final answer** — see "With the stale discount," below.

### Result — brood side (feeding on retinue corpses, "The Feed," 6.50s fresh)

`BroodKillerClaimChance = 0.85` (unchanged — still a tone read the 700-pool
absorbs easily), `BroodClaimRate = 0.0272/s` at `C = 1.0` (target walk-up
equilibrium `f = 0.15` — deliberately looser than the retinue's target,
matching task-053's original asymmetric-by-design posture):

| assumed wave-3 duration | retinue kills/s | killer channel | walk-up channel (fresh-duration equilibrium) | **total, % of 700** |
|---|---|---|---|---|
| 20s | 6.0/s | 33.2 units | 105.0 units | **19.7%** |
| 30s | 4.0/s | 22.1 units | 105.0 units | **18.2%** |
| 45s | 2.7/s | 14.9 units | 105.0 units | **17.1%** |
| 60s | 2.0/s | 11.1 units | 105.0 units | **16.6%** |
| 80s | 1.5/s | 8.3 units | 105.0 units | **16.2%** |

Brood's total (16–20%) is higher than task-053's original mitigated range
(roughly 2–14%), but the pool is large enough that this stays comfortably
inside "reads often as the horror beat" rather than crossing into "dents the
fight" — 700 units absorbing ~130 feeders at once is a fifth of the pool
distracted, not a majority, and per §7 this side's whole design intent was to
read *more* visibly, not less. **No further throttling proposed on this
side** — the higher number here is a feature of persistence working as
intended, not a risk.

### With the stale-duration discount (§5.5) applied

The retinue table above assumes every walk-up claim gets the fresh, full
3.00s duration. In practice, once a fight runs long enough for
`Swarm.Feeding.StaleAge` (15s) to matter, most of the *standing* corpse
population claimants encounter is old, not freshly spawned — the corpse cap
(§5.4) binds within 3.6–14.3s on the retinue side, meaning almost the entire
back half of any wave-3-length fight is drawing walk-up claims against a
field of corpses well past 15s old. Applying `StaleDurationScale = 0.5`
(halving `D` for those claims) to the walk-up channel's equilibrium:

| assumed wave-3 duration | walk-up channel (stale, D≈1.5s) | **total, % of 120** |
|---|---|---|
| 20s *(mostly pre-stale — shown unchanged)* | 13.2 units | 25.1% |
| 30s | 7.0 units | **15.2%** |
| 45s | 7.0 units | **12.1%** |
| 60s | 7.0 units | **10.5%** |
| 80s | 7.0 units | **9.3%** |

This inverts the shape of the problem back toward something task-053 would
recognize: **the danger is concentrated at the short/high-crowd end (dur=20s,
still 25.1%, since staleness barely has time to kick in), and the standing
tax shrinks toward ~9–10% the longer a fight runs** — long fights are exactly
where an un-discounted walk-up channel would otherwise have been worst
(flattest, most persistent tax), and now they're the *safest* case instead.
That's the shape a distraction mechanic should have: costly when the fight is
already chaotic and crowded, cheap once it settles into a grind.

### Worst case, killer channel at always-3x-crowd (dur=20s, the danger zone)

| Side | killer (3x) | + walk-up | total | % of army |
|---|---|---|---|---|
| Retinue | 50.6 units | 13.2 | 63.8 | **53.2% of 120** |
| Brood | 99.4 units | 105.0 | 204.4 | **29.2% of 700** |

Worth stating plainly: task-053's *own* worst case at this same assumed
duration was **mathematically impossible** (210.8% — more units feeding than
exist in the whole army), which was already flagged as "the mechanic can lock
the whole line at once if crowding and short fights coincide." This walk-up
version's worst case (53.2%) is *numerically contained* by comparison — bad,
genuinely more than half the army, but a real, survivable number rather than
a sign the model breaks down. That's the honest comparison point: the walk-up
version is worse than task-053's *typical* case, but not worse than
task-053's own *worst* case, which was already the acknowledged edge of what
the mechanic could tolerate.

### Why the retinue side is still the one at risk — and it's still scale-invariant

Task-053's finding stands: retinue duty cycle from the killer channel alone
doesn't depend on army size, only on per-soldier kill rate and duration, so
it isn't a wave-3-specific artifact. The walk-up channel adds a *second*
scale-invariant term on top (the equilibrium `f` in the birth-death model
also doesn't depend on population size, only on `λ`, `D`, `C`) — so the
combined total in the tables above would look the same shape at any wave
size, just with smaller absolute unit counts. The retinue is structurally the
side that pays for this mechanic, at any scale, for the same reason task-053
identified: it's the smaller army with the higher per-capita kill rate.

### The bound

Three complementary valves now, one new:

1. **`RetinueKillerClaimChance` (0.20, down from 0.35) / `BroodKillerClaimChance`
   (0.85, unchanged).** The retinue value moved down because it now shares
   the duty-cycle budget with the walk-up channel — task-053's 0.35 was tuned
   assuming this was the *only* channel; leaving it there while adding walk-up
   would have meant the retinue table above starting from a worse baseline
   than shown. Brood didn't need to move — its walk-up contribution, even
   added on top of the unchanged 0.85, stays well inside what the 700-pool
   can absorb.
2. **`RetinueClaimRate` (0.0412/s) / `BroodClaimRate` (0.0272/s), the new
   walk-up hazard rates (§5.2, §9).** Deliberately tiny per-tick numbers —
   at a 10Hz claim-pass tick (§5.2/§10), that's roughly a 0.4% chance per
   tick for an eligible retinue unit. That's the correct order of magnitude
   for "abundant standing opportunity, low intrinsic rate," not a tuning
   mistake: because eligibility now persists for the length of a fight
   instead of a single frame, the *rate* has to be proportionally smaller
   than a one-shot `FeedChance` to land at a comparable duty cycle.
3. **`StaleAge` (15s) / `StaleDurationScale` (0.5), §5.5 — new, and the one
   that actually shrinks with fight length rather than staying flat.** This
   is the guard rail the task brief asked for by name ("a decaying feed
   duration on stale bodies"), and it's the one doing the real work in "With
   the stale discount" above — without it, the retinue's total sits at a
   flat ~14.5–25% for the whole sweep; with it, the typical case drops to
   ~9–15% and only the short/crowded edge stays high.

`Swarm.Feeding.Cooldown` (4.0s, unchanged) still applies to both channels as
a secondary anti-chain-feed valve, though the Monte Carlo check shows its
effect on the equilibrium is second-order next to the three valves above.

**What did NOT happen, on purpose:** the retinue's killer-claim chance was
retuned once (0.35 → 0.20) to make room for the new channel, and that's the
only place a chance value moved. The rest of the fix is structural (a new
rate-based channel with its own low default, plus a decay rule), not a
second and third round of squeezing the same dial — which is what the task
brief explicitly warned against ("do not quietly retune it into the old
design"). If the retinue numbers above still read as too costly once
task-054 measures them for real, the next lever should be `StaleAge` (make
bodies go stale sooner) or `ClaimRadius` (shrink the catchment), not another
cut to `RetinueClaimRate` — that would just re-hide the walk-up channel's
cost rather than actually bound it.

---

## 9. Tuning dials (all CVars, `Swarm.Feeding.*`)

**Unchanged from task-053:**

| CVar | Default | Rationale |
|---|---|---|
| `Swarm.Feeding.Enabled` | `1` (bool) | Master on/off, so task-054 can reproduce the §3-style zero-input A/B baseline with and without feeding — the house measurement pattern (`GATE1-FUN-PROTOTYPE.md` §3). |
| `Swarm.Feeding.MaxPerCorpse` | `3` | The owner's explicit per-corpse cap — now a *lifetime* cap per corpse (§4), not just a per-instant one. |
| `Swarm.Feeding.ChompRate` | `0.05` (s/HP) | Feed-duration slope against the corpse's `MaxHP` (§3). |
| `Swarm.Feeding.MinDuration` | `1.5` (s) | Floor so even a fodder kill produces a legible pause, not a flicker. |
| `Swarm.Feeding.MaxDuration` | `8.0` (s) | Ceiling so a future high-`MaxHP` titan/elite doesn't freeze a unit absurdly long (§3). |
| `Swarm.Feeding.Cooldown` | `4.0` (s) | Per-unit re-entry cooldown after a feed completes; applies to both the killer and walk-up channels (§8). |
| `Swarm.Feeding.HealFraction` | `0.20` | Self-heal fraction of the feeder's own `MaxHP`, paid only on uninterrupted completion (§6). |
| `Swarm.Feeding.HeroExempt` | `1` (bool) | The hero never feeds — see §11. Exposed as a CVar per house convention (every dial gets one), but this should stay `1`; it isn't a balance knob, it's a design-law guardrail (GDD §4 hero relevance). |

**Renamed from task-053 (same mechanic, new name — see §2):**

| CVar | Default | Was (task-053) | Rationale |
|---|---|---|---|
| `Swarm.Feeding.RetinueKillerClaimChance` | `0.20` | `RetinueFeedChance` (`0.35`) | Retuned down from task-053's value to leave headroom for the new walk-up channel below — §8 "The bound." |
| `Swarm.Feeding.BroodKillerClaimChance` | `0.85` | `BroodFeedChance` (`0.85`) | Value unchanged — brood's walk-up addition stays inside what the 700-pool absorbs without moving this. |

**New for task-061 (persistent corpses, §5):**

| CVar | Default | Rationale |
|---|---|---|
| `Swarm.Feeding.ClaimRadius` | `150` (uu) | Walk-up eligibility distance (§5.2) — close enough to read as "standing next to it" (`MeleeRange` is 95uu), inside one `GridCellSize` (250uu) so the claim query reuses the existing 3×3-cell neighbor scan. |
| `Swarm.Feeding.RetinueClaimRate` | `0.0412` (per second, while eligible) | Walk-up hazard rate for retinue claimants on brood corpses (§8) — targets an ~11% equilibrium duty cycle at `C=1.0` coverage, matching task-053's original retinue target so the two channels are comparable. |
| `Swarm.Feeding.BroodClaimRate` | `0.0272` (per second, while eligible) | Walk-up hazard rate for brood claimants on retinue corpses (§8) — targets an ~15% equilibrium duty cycle, deliberately looser than the retinue rate per the same asymmetric-by-design posture as the killer-claim chances. |
| `Swarm.Feeding.ClaimTickHz` | `10` | The walk-up claim pass (§5.2, §10) doesn't need frame-perfect resolution against feed durations of 1.5–8s — running it at 10Hz instead of full tick rate cuts its cost roughly 6× at no measured loss of accuracy (per-tick probability is derived from the per-second rate via `1-(1-Rate)^dt` regardless of tick rate). |
| `Swarm.Feeding.MaxCorpsesPerTeam` | `100` | Per-team corpse population cap (§5.4) — binds within 3.6–14.3s of brood-corpse creation at wave-3 rates, so this is the practical peak corpse count for most of a fight, not the uncapped cumulative total. |
| `Swarm.Feeding.StaleAge` | `15` (s) | Age past which a walk-up claim (never a killer claim) uses the discounted duration below (§5.5). The load-bearing anti-flat-tax dial in §8's re-run. |
| `Swarm.Feeding.StaleDurationScale` | `0.5` | Multiplier on `FeedDuration()` for walk-up claims on a corpse older than `StaleAge`. Halves the walk-up channel's cost for the back half of any fight that runs longer than ~15s. |

---

## 10. Mass Entity cost breakdown — what's cheap, what isn't

Per design law 5 and `SwarmCombat.h:10-14`'s no-cross-entity-writes constraint:

**Cheap — no new query, no shared mutable state, self-contained per entity:**
- Killer-set resolution reusing the existing per-victim neighbor walk (§2).
- Full-duration-each countdown, one private fragment per feeder (§4) — this is
  *why* §4 picked full-duration-each over a shared pool: a shared "corpse HP
  remaining" counter that 1-3 feeders all read/write would be exactly the
  cross-entity contention the combat model was built to avoid.
- "Stays vulnerable/targetable while feeding" (§1) requires **zero** new code
  in `USwarmCombatProcessor`'s victim-side logic — a feeding unit is already a
  valid target via the existing team+distance check; it just never registers a
  `bStriking` grid entry while its feeding fragment is active, so it can't
  land blows. No new incoming-damage path needed.
- `KillerClaimChance` roll, `Cooldown` check, `HealFraction` payout — all
  self-reads on the entity's own fragment, all independent, all trivially
  parallel-safe. Unchanged from task-053.
- The corpse record itself (§5.1) — five small fields (`Location`, `Team`,
  `MaxHP`, `SpawnTick`, `OpenSlots`) in a subsystem-owned array, not a Mass
  entity. No archetype, no chunk iteration, no per-corpse Actor.

**Moderately expensive — a real but bounded cost, not a new pattern (task-053, unchanged):**
- `FGridEntry` (`SwarmSubsystem.h:130`) needs an `FMassEntityHandle` field it
  doesn't have today, and the dying victim needs to capture up to
  `MaxAttackersPerUnit` (4) claimant handles during the frame it dies, so the
  killer set (§2) can be turned into actual entity handles to attach feeding
  fragments to. This is a struct-widening plus a small fixed-size array
  (bounded by the existing `MaxAttackersPerUnit` clamp), computed during a walk
  the processor already performs — not a new spatial query, not a new
  cross-entity write, but it does touch a hot-path struct and is worth task-054
  budgeting explicitly rather than discovering mid-implementation.

**Task-061 additions — moderately expensive, bounded by corpse count, not army size:**
- **The walk-up claim pass (§5.2).** A new pass, run at `ClaimTickHz` (10Hz,
  not every frame — §9), that iterates the corpse arrays (bounded by
  `2 × MaxCorpsesPerTeam` = 200) and for each corpse with `OpenSlots > 0`,
  queries the existing spatial grid within `ClaimRadius` for eligible
  opposing-team units. This reuses the grid `USwarmCombatProcessor` already
  builds every frame — no new spatial structure — but it is a genuinely new
  *walk* over it: at wave-3 density with the cap bound (§5.4), this is
  roughly a 24% addition to the volume of grid-neighbor queries already run
  per frame for combat (200 corpse queries against ~820 unit queries),
  mitigated by running at 10Hz instead of full tick rate. Task-054 should
  measure this specifically, not assume it's free by analogy to the
  killer-channel's zero-new-query claim.
- **Corpse array indexing needs a generational handle, not a raw index.**
  Because the corpse arrays are capped (§5.4) and cull-and-replace entries
  when full, a feeder's fragment can't hold a bare array index to "its"
  corpse — if that slot gets culled and reused for an unrelated corpse before
  the feeder finishes, a raw index would silently misattribute the feeder's
  slot-release-on-death (§4) to the wrong corpse. Task-054 needs the standard
  fix (index + a small version counter per slot, bump the version on cull,
  reject a release/lookup whose version doesn't match) — a well-understood
  pattern, but a real correctness requirement, not an optional nicety, since
  getting it wrong fails silently rather than crashing.
- **Corpse cull-on-overflow (§5.4)** is a linear scan of one team's corpse
  array (≤100 entries) for the oldest zero-active-feeder entry, run only when
  a new corpse would exceed the cap — bounded by corpse population, not by
  kill rate or army size, and small enough (≤100) not to need its own budget
  line beyond "runs once per corpse-cap-overflow event."

**Explicitly avoided, and why:**
- Shared per-corpse damage pool (would need cross-entity read/write between up
  to 3 feeders every frame — rejected in §4, still rejected).
- Corpses as full Mass entities with their own archetype (§5.1) — no
  locomotion, no combat, no need for chunk-iteration machinery for something
  that never steers, fights, or collides.
- Active corpse-seeking by units (§5.2) — rejected outright, not just for
  cost: it would mean touching steering code task-054 doesn't own, and it
  would compete with combat/formation movement for the same locomotion
  budget. Only passive, incidental proximity counts.

---

## 11. Handoffs

**To task-054 (Mass build), updated for task-061.** Read §2 first for the
killer-set definition and the `FGridEntry` handle addition, then §5 for the
corpse record and walk-up claiming — everything downstream depends on both.
Revised build order:

1. Widen `FGridEntry` + capture claimant handles on death (§2, unchanged from
   task-053).
2. Add the corpse record and its two per-team arrays in `USwarmSubsystem`
   (§5.1) — five fields, generational handles (§10), not a Mass entity.
3. Add the per-feeder fragment (countdown, team, corpse handle,
   heal-on-complete flag, killer-vs-walk-up flag so §5.5's stale discount only
   ever applies to walk-up claims).
4. Wire the killer-claim path: `KillerClaimChance` roll + `Cooldown` at kill
   resolution (§2), spawn/attach the corpse record, decrement `OpenSlots` for
   however many killer-set members claim.
5. Wire the walk-up claim pass (§5.2, §10): a new pass at `ClaimTickHz`,
   corpse-centric, querying the existing grid within `ClaimRadius`, applying
   `RetinueClaimRate`/`BroodClaimRate` and §5.5's stale-duration discount.
6. Wire the cull-on-overflow rule (§5.4) at corpse-creation time.
7. Skip locomotion/striking while a feeding fragment is active (unchanged).
8. Round-end teardown: clear all corpse records, treat any active feed as an
   interrupted one (§5.6).
9. Render hook (below).

Reproduce the §3-style zero-input measurement with `Swarm.Feeding.Enabled 0/1`
to confirm the §8 numbers hold in the real sim, not just the scratch model —
this matters more here than it did for task-053, since §8's walk-up channel
is a birth-death equilibrium approximation, not a direct measured count.

**Render hook, not scoped here:** a feeding unit needs a distinct visible pose
(eating/sealing/kneeling — locked animation, not idle) so the "distracted, free
kill" read (§1) actually lands at horde scale, plus a corpse sprite that now
needs to persist and render for up to the whole round (§5.6), not just a fade
window — a materially longer on-screen commitment than task-053 scoped. This
is a `performance-director` / render-bridge ask once task-054 has the corpse
record and fragment data to key off; flagging it here so it doesn't get lost
between specs.

**To task-060 (blood particles):** task-060 is explicitly out-of-scope for a
distinct death burst because deaths were, until now, never positioned data —
`USwarmDeathProcessor` counts them but never reads `Transform`. Once
task-054 builds the corpse record above, that's no longer true: a corpse
*is* a death event with a `Location` (§5.1). Task-060 (or a follow-up) can key
a larger, distinct death burst off corpse creation once task-054 lands,
without either task touching the other's owned files.

**To task-002 (entity tier stat blocks / armor):** see §3's proxy-replacement
note — swap `MaxHP` for an effective-toughness figure at the single
`FeedDuration()` call site once armor lands. Nothing else in this spec changes,
including §5.5's stale-duration discount, which multiplies whatever
`FeedDuration()` returns regardless of what feeds its input.

---

## 12. Narrative requests

For the narrative-director, per the handoff convention:

- **"The Rite"** (working name, §7) — mechanically: a retinue unit that lands
  a killing blow on a brood pauses over the corpse for 1.5-8s (scaled to what
  it killed), unable to fight, to seal the body against the dark. Faction/biome:
  none named yet (canon is faction-less per `FLAME-FOUNDATION.md`). What a
  player must feel: this should NOT read as looting or as a battle pause — it's
  a duty performed under fire, the retinue protecting the world even mid-fight,
  and it should feel a little dangerous to watch (the soldier is exposed the
  whole time). It's the one piece of this spec doing real tone-repair work for
  "we are the good guys" against a mechanic that's otherwise symmetric with
  monster-devours-corpse — it deserves the real name and the real beat, not the
  placeholder used here.
- **"The Feed"** (brood side, §7) — mechanically identical duration/vulnerability
  shape, opposite valence: a brood devouring a fallen soldier it just
  overwhelmed. What a player must feel: grief and urgency, not spectacle —
  this is "what you fight was taken, not born hostile" made literal and should
  land as a reason to fight harder, not a horror-for-its-own-sake beat.
- **Gameplay readability constraint for both, to carry into any art brief:**
  must be legible as "this unit is out of the fight" at horde scale in a single
  glance — no animation nuance that only reads at 1:1 zoom, since both units in
  contact with a feeder and the player scanning the whole field need the same
  read. A locked, non-idle pose plus (ideally) a distinct silhouette shift is
  enough; it does not need per-unit uniqueness (design law 5).
- **New for task-061 — corpses are now a sustained battlefield-litter element,
  not a brief fade.** A body can sit on the field, visibly sealed or
  half-eaten, for most of a wave-3 fight (§5.4's cap-binding math: tens of
  seconds, not the few seconds task-053 scoped the render hook for). That's a
  bigger visual commitment than the original ask — worth a look from the
  narrative/art side at whether an inert, already-fed-on corpse needs its own
  static "spent" look distinct from a fresh one (§4's lifetime-cap means a
  corpse's 3 slots can fill and it goes fully inert while still lying there
  for the rest of the round), or whether that distinction doesn't matter at
  horde scale and a single decayed-body sprite covers the whole lifetime.

---

## 13. Canon proposals

Not editing `SYSTEMS.md` per the task's ownership boundary — this is the entry
for whoever folds it in (`task-038` or a later pass):

| Date | Decision | Rationale | Spec / data |
|---|---|---|---|
| 2026-07-27 | **Feeding/distraction**: killer set (up to 3, resolved at the death frame, §2) goes null for `clamp(MaxHP × 0.05, 1.5, 8.0)`s, full duration each (§3/§4), stays vulnerable/targetable/immobile/non-damaging (§1), no persistent corpse entity (§5, superseded 2026-07-28 below), small self-heal on completion (§6), symmetric mechanic / per-side fiction ("The Rite" vs "The Feed", §7) | Owner-specified tactical valve; MaxHP-proxied pending `task-002` armor; feed-chance dials (`RetinueFeedChance 0.35`, `BroodFeedChance 0.85`) were load-bearing against a scale-invariant ~31% retinue self-stall found in §8 | `docs/design/feeding-distraction.md` · `docs/data/feeding.json` |
| 2026-07-28 | **Persistent, walk-up-claimable corpses (task-061), overriding §5 above.** Corpses persist to round end (read as the whole 3-wave run, §5.4) as a small non-Mass record (§5.1: location, team, `MaxHP`, spawn age, open-slot count). Any unit of the *opposing* team within `ClaimRadius` (150uu) of an open slot may claim it via a per-second hazard rate rather than a one-shot roll (§5.2); killers still get first, guaranteed claim (§2). The per-corpse cap of 3 is now a lifetime cap that reopens only on a feeder's death mid-meal (§4). Population is bounded per-team (`MaxCorpsesPerTeam = 100`, §5.4) with an oldest-empty-first cull rule, and walk-up claims on bodies older than `StaleAge` (15s) use a discounted duration (`StaleDurationScale = 0.5`, §5.5) to keep the added load from being a flat tax for the whole fight. Killer-claim chances retuned (`RetinueKillerClaimChance` 0.35→0.20; `BroodKillerClaimChance` unchanged at 0.85) to share the duty-cycle budget with the new walk-up channel. | Direct owner override, given the original cost/no-late-joiners reasoning and a live choice between killer-only and full walk-up persistence: *"downed units persistent state until the round is over. Those can get eaten by the enemy like our gameplay plan."* §8's re-run found walk-up eating is a real regression (a near-flat standing tax that doesn't shrink with kill rate, unlike the original kill-triggered mechanic) but not a stalemate — with the stale-duration discount, the retinue's typical-case duty cycle lands at ~9–15% (vs. task-053's ~11% target) and its worst case (~25–53% at the short/high-crowd extreme) stays below the original mechanic's own worst case, which was mathematically impossible (>100%) under the same assumption. | `docs/design/feeding-distraction.md` §5, §8, §9 · `docs/data/feeding.json` |

---

## 14. Simulation notes

**What was simulated:** not an engine run (this task claims no
`unreal-editor` resource) — a Little's-Law Fermi model in a scratch Python
script, built entirely from measured numbers already in the repo: the shipped
combat CVar defaults (`SwarmCombatProcessors.cpp`) and the "shipped now"
zero-input wave-3 outcome (`docs/GATE1-FUN-PROTOTYPE.md`, 4 runs, 2026-07-25:
109-111/120 wave-1 survivors, full retinue wipe by wave 3, 131-145/700 brood
surviving). The script computed: (1) the feed-duration curve at current
`BroodMaxHP`/`RetinueMaxHP`, (2) concurrent-feeder counts via
`kill_rate × feed_duration × feeders_per_kill` across a swept range of assumed
wave-3 durations (20-80s, since the doc only gives a whole-run duration, ~95s
for 3 waves, not a per-wave breakout), for both killer-only and always-3-crowd
cases, on both sides, (3) a scale-invariance check on the retinue-side duty
cycle, (4) the `RetinueFeedChance` value needed to bring that duty cycle to a
~10% target.

**Headline result:** naive, unthrottled symmetric feeding pulls 17.6%-70.3% of
the retinue's own army offline at once at wave-3 peak (killer-only case; the
always-3x-crowd case exceeds 100% at short assumed durations, i.e. is not
survivable at all), while the brood side never exceeds ~17% even in the worst
case tested. The retinue-side number is structural (scale-invariant across
army size, not a wave-3-specific artifact) rather than a density fluke, which
is why the fix is a per-kill probability (`FeedChance`) rather than a
population-scaled cap.

**Assumptions, stated plainly:**
- Wave-3 combat duration is **not measured** in the repo; the sensitivity
  sweep (20-80s) is meant to substitute for that missing number, and the
  qualitative conclusion (retinue duty-cycle is the risk, brood is fine) holds
  across the entire swept range, which is why the recommendation doesn't wait
  on a real duration measurement.
- Kill rate is treated as roughly uniform over the fight's duration (a
  simplification — the real fight likely front-loads kills before the retinue
  starts dying in earnest, which the model doesn't capture, and a real measured
  curve would let task-054 tighten the `FeedChance` defaults rather than rely
  on this Fermi approximation).
- "Per-soldier kill rate is roughly independent of army size" (the
  scale-invariance argument in §8) assumes each soldier engages its own local
  cluster via the existing K-nearest targeting rather than the whole horde at
  once — consistent with how the combat model is built, but not independently
  verified against a smaller-wave measurement in this pass.
- The `FeedDuration` curve's `ChompRate`/`Min`/`MaxDuration` constants are
  first-pass PROTOTYPE DIALS (same status as every other tuned number in
  `docs/data/`), chosen to produce a legible-but-not-excessive duration at
  current `MaxHP` values (§3's table) — not separately playtested.

Script kept at the session scratchpad (not committed — throwaway per house
convention), reproducible from the numbers cited above if a follow-up pass
wants to extend the sweep.

### Task-061 addition: the walk-up channel model

**What was simulated:** a second scratch Python script, built on the same
measured inputs as above (same wave-3 kill-rate table, same 20-80s sweep),
adding: (1) a closed-form birth-death equilibrium for the walk-up channel
(`f = C·λ·D / (1 + C·λ·D)`, §8), (2) a discrete-event Monte Carlo (per-unit
Bernoulli-per-tick over a 200s window with a 100s warmup discard) as an
independent cross-check of that closed form at the dur=45s midpoint, (3) the
combined killer+walk-up totals across the full duration sweep for both sides,
both with and without the §5.5 stale-duration discount, (4) the always-3x
worst case recomputed with the walk-up channel added on top, (5) cumulative
corpse-population growth against the `MaxCorpsesPerTeam` cap, to find how
quickly the cap binds at each assumed duration.

**Headline result:** the closed-form equilibrium matched the Monte Carlo
within ~5% at both tested targets (retinue: 0.110 target vs. 0.1155
simulated; brood: 0.150 target vs. 0.1634 simulated), with the Monte Carlo
running consistently a little high — a small conservative margin in the
already-conservative direction. Without the stale-duration discount, the
walk-up channel adds a flat ~11%/~15% duty cycle to both sides for the entire
sweep, regardless of how the fight is actually going; with the discount
applied (justified by the corpse-population-cap math showing the field is
dominated by 15s+-old bodies for most of any wave-3-length fight), the
retinue's typical case drops to ~9-15% and only the short/high-crowd extreme
(dur=20s) stays near its undiscounted value, because staleness hasn't had
time to apply yet at that duration.

**Assumptions, stated plainly, in addition to the ones already listed above:**
- **Coverage `C = 1.0`** (the fraction of the living army within `ClaimRadius`
  of some open-slot corpse at any instant) is an assumption, not a measured
  quantity — chosen as the worst-case "dense melee scrum" reading because
  that's the shape combat already takes (units engage via K-nearest
  targeting, packed close by the leash and stance systems), and because §8's
  method throughout this spec has been to test the worst case first. `C = 0.5`
  is shown once for sensitivity (roughly halves the walk-up contribution) but
  isn't separately tabulated across the full sweep — a real measurement of
  actual corpse-to-unit proximity at wave-3 density, once task-054 has a
  running sim, would tighten this considerably.
- **"Round" is read as the whole 3-wave run** (deploy-to-win/loss), not a
  per-wave boundary, because no other "round" concept exists in current canon
  (§5.4). This reading only matters for the *uncapped* cumulative-corpse math
  used to argue the cap is necessary — the cap itself, and everything §8
  computes from it, doesn't depend on getting this reading right, since the
  cap binds well before a full run's cumulative kills would ever be reached.
- **The stale-duration discount's effect on §8 is approximated, not
  simulated exactly.** The "With the stale discount" table applies
  `StaleDurationScale` to the walk-up equilibrium as if *all* walk-up claims
  after the cap binds were against stale bodies — true directionally (the cap
  binds fast, so the field is mostly old bodies for most of the fight) but
  not frame-accurate; a real simulation would show a smoother transition from
  the fresh-duration number to the stale-duration number as the corpse
  population ages, rather than the two-regime approximation used here.
- **The birth-death model treats `λ`, `D`, and `C` as constant** within each
  swept duration, ignoring that army size (and therefore, weakly, `C`)
  declines over the course of a wave as units die — consistent with §8's
  scale-invariance finding (the duty-cycle *fraction* doesn't depend on
  population size), but the model doesn't independently verify that the
  approximation holds as gracefully in a shrinking-army regime as in the
  roughly-constant-population regime it was validated against via Monte Carlo.

Script kept at the session scratchpad (not committed — throwaway per house
convention, same as above), reproducible from the numbers cited in §8 if a
follow-up pass wants to extend the sweep or replace `C` with a measured value.
