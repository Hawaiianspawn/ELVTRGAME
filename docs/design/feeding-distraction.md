# Feeding / distraction mechanic

Spec for task-053: a killing blow makes the killer go null on the corpse for a
duration keyed to what it killed. Extends `SYSTEMS.md` §1 (entity tiers) and §6
(retinue tuning), and reads `GDD.md` §4 (hero relevance), §7 (upkeep/soft caps),
§10 (Mass Entity constraints). Spec only — `task-054` builds this in Mass.

**Reader's map, in the order the "Done when" checklist asks for it:** §1 what
"null" means · §2 who "the killer" is (load-bearing, everything else depends on
it) · §3 the feed-duration curve · §4 the three-feeders-per-corpse rule · §5 what
happens when the body is gone · §6 whether feeding pays · §7 the symmetry/tone
problem · §8 the density check · §9 CVar dials · §10 Mass cost breakdown ·
§11 handoffs · §12 narrative requests · §13 canon proposals · §14 simulation notes.

---

## 0. The one-paragraph version

A unit that lands a killing blow has a chance to go null and spend a duration
(set by the dead unit's `MaxHP`, 1.5–8s) rooted at the kill, unable to fight or
move but still a normal, killable target. Up to 3 units can be doing this off
one corpse at once — but only if that many attackers happened to land the fatal
blow simultaneously; nobody ever *joins* a corpse after the fact. There is no
new persistent corpse entity — feeding state lives entirely on the (up to 3)
feeding units, and the corpse itself is a render-only artifact that fades when
they're done. **The arithmetic in §8 says naive 100%-chance symmetric feeding
would stall the retinue's own line far worse than it distracts the brood** — the
small army has the high kill rate, so it's the side that pays. `RetinueFeedChance
≈ 0.35` / `BroodFeedChance ≈ 0.85` are the load-bearing numbers that make this
survive wave 3.

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

---

## 5. What happens when the body is gone

**There is no persistent corpse entity, and there are no late joiners.**

The killer set is resolved once, at the death frame (§2), and is final — a unit
that wasn't part of that frame's simultaneous strikers never gets to feed on
that kill, no matter how soon it arrives. This is a direct consequence of §4:
since feeding never accelerates or extends based on who's present, there's no
mechanical reason for a corpse to wait around advertising open slots, and
modeling "open slots on a lingering body" would be exactly the kind of
per-corpse shared mutable state §4 already rejected for cost reasons.

Concretely: each of the (1-3) killer-set members gets a private countdown
fragment (`FSwarmFeedingFragment` or similar — task-054's naming) the instant
the kill resolves. **The corpse itself is a render-only artifact** — a sprite
left at the death location for however long the *longest* of its feeders is
still counting down, then it fades. There is no simulated "Corpse" Mass entity,
no slot-tracking data structure, and no despawn-timer edge case to design
around, because nothing is waiting on the corpse except the feeders that are
already committed. This is a stronger (cheaper) answer than the task brief's
framing assumed — it explicitly asks "how long does the body persist," and the
honest answer is that persistence isn't a simulation concern here at all,
only a rendering one. Flagged for the render-bridge hook in §11.

If a feeder is killed while feeding (it's still a valid target, §1), it dies
through the ordinary, unmodified `USwarmDeathProcessor` path — and because
"the killer" (§2) is a frame-local derivation with no memory of *why* the
victim wasn't fighting back, a unit that kills a distracted feeder can itself
enter the killer set for **that** kill and go null in turn. This chains for
free, with no special-casing, and it's thematically correct: getting caught
feeding is exactly when you're most exposed.

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

**Proposed narrative follow-up** — see §11's Narrative requests. "The Rite" is
named here provisionally; it should get the narrative-director's actual pass
before it ships in UI/VO, since it's the more load-bearing of the two names
(the one doing tone-repair work).

---

## 8. The density check — this is the one that can kill the feature

**Verdict: naive 100%-chance symmetric feeding does not survive wave 3 — but
not for the reason the task brief expected. The brood side is fine even
unthrottled. The retinue's own line is the one at risk of stalling itself, and
the fix is a feed-chance below 100%, tuned harder on the retinue side than the
brood side.**

### Method

Not a full engine run — a Little's-Law model built from the measured wave-3
outcome in `docs/GATE1-FUN-PROTOTYPE.md` ("shipped now," 2026-07-25) and the
live combat CVars, worked in a scratch script (§13). Concurrent feeders at
steady state = `kill_rate × feed_duration × feeders_per_kill`.

**Measured wave-3 inputs:** retinue refills to the 120 cap between waves, wave 3
spawns 700 fresh brood (`SYSTEMS.md:45`), and the shipped-defaults result is a
**full retinue wipe**, with **131-145 of the 700 brood surviving** — so the
retinue killed **~555-569 brood** (mid **562**) before dying, and the brood
killed all **120** retinue.

**Missing input, flagged honestly:** the doc gives ~95s for a *whole* 3-wave
zero-input run, not a per-wave duration, and wave 3 (the biggest wave, and
where the loss happens) isn't broken out. Rather than invent a false-precision
number, the arithmetic below is run across **five assumed wave-3 durations
(20s–80s)** to check whether the conclusion survives the uncertainty.

### Result — retinue side (feeding on brood corpses, "The Rite," 3.00s each)

| assumed wave-3 duration | brood kills/s | retinue feeding, killer-only (1x) | % of the 120-unit retinue | worst case, always-3-crowd (3x) |
|---|---|---|---|---|
| 20s | 28.1/s | 84.3 units | **70.3%** | 210.8% *(impossible — more than the whole army)* |
| 30s | 18.7/s | 56.2 units | **46.8%** | 140.5% |
| 45s | 12.5/s | 37.5 units | **31.2%** | 93.7% |
| 60s | 9.4/s | 28.1 units | **23.4%** | 70.3% |
| 80s | 7.0/s | 21.1 units | **17.6%** | 52.7% |

Even at the **most forgiving** assumed duration (80s) and the **cheapest**
crowding case (killer-only, no crowd bonus at all), a naive 100%-chance version
of this mechanic pulls **17.6%-70.3% of the player's own army offline at any
instant** during the fight they're already losing by default (§3 baseline: lost
by 4-13 brood; shipped-now: lost by 131-145). At the low end of the duration
range the always-3x case is **numerically impossible** (>100%) — a real
combat-scale signal that the mechanic can lock the whole line at once if
crowding and short fights coincide.

### Result — brood side (feeding on retinue corpses, "The Feed," 6.50s each)

| assumed wave-3 duration | retinue kills/s | brood feeding, killer-only (1x) | % of the 700-brood pool | worst case, 3x |
|---|---|---|---|---|
| 20s | 6.0/s | 39.0 units | 5.6% | 16.7% |
| 30s | 4.0/s | 26.0 units | 3.7% | 11.1% |
| 45s | 2.7/s | 17.3 units | 2.5% | 7.4% |
| 60s | 2.0/s | 13.0 units | 1.9% | 5.6% |
| 80s | 1.5/s | 9.8 units | 1.4% | 4.2% |

**Even at worst case, brood feeding never exceeds ~17% of the pool.** 700 is
just large enough that pulling a few dozen brood offline for 6.5s at a time
doesn't dent the fight. This side needs a safety valve for parity and for the
late-wave edge case (a thin, mostly-dead brood wave against a still-large
retinue — the ratio that made this side safe at wave-3 *peak* inverts as the
wave drains), but it isn't the thing that breaks the feature.

### Why the retinue side is the one at risk — and why it isn't just a wave-3 artifact

If `RetinueDPS`/cleave/formation shape make each soldier's kill rate roughly
independent of army size (each soldier engages its own local cluster via
K-nearest targeting, not the whole horde at once — which the existing combat
model is already built to do), then **retinue duty-cycle = per-soldier kill
rate × feed duration is scale-invariant**: it doesn't matter whether there are
120 retinue or 20, the *fraction* of the line pulled offline by feeding is
roughly the same, because both the numerator (kills) and the denominator (army
size) scale together. At the dur=45s working assumption, per-soldier kill rate
comes out to **~0.104 kills/s/soldier**, giving a **~31.2% duty cycle at ANY
scale**, not something specific to 700-brood density. That means this isn't a
"wave 3 is special" problem — it's baked into the DPS/HP/duration relationship
and would show up in wave 1 too, just with fewer total feeders in absolute
terms.

### The bound

Two complementary valves, both cheap (§10):

1. **`Swarm.Feeding.RetinueFeedChance` (default 0.35) / `Swarm.Feeding.BroodFeedChance`
   (default 0.85).** Per-killer-set-member independent coin flip, rolled once
   at kill resolution, on whether that unit actually enters feeding at all
   (vs. just continuing to fight normally). This directly scales the duty
   cycle: `0.35 × 31.2% ≈ 10.9%` on the retinue side — enough headroom that
   even the worst-case 3x-crowd scenario (`0.35 × 93.7% ≈ 32.8%`) stays
   survivable. Brood stays high (0.85) on purpose: feeding should still read
   often and visibly, since watching brood devour a soldier is the horror beat
   the tone section (§7) is built around, and the 700-unit pool can absorb it.
2. **`Swarm.Feeding.Cooldown` (default 4.0s), per-unit.** A unit that just
   finished a feed can't be selected into another killer set for this long.
   Secondary valve against the same unit chain-feeding back-to-back at short
   feed durations (the brood corpse's 3.00s duration is short enough that a
   high-kill-rate soldier could otherwise re-enter feeding almost immediately).

**This is the load-bearing tuning finding of the spec**: the per-corpse cap of
3 alone (§4) does not bound the *aggregate* fraction of either army that can be
feeding at once — it only bounds how many pile onto one kill. The feed-chance
dials are what actually keep the fight readable at wave-3 density, and they
need to be asymmetric even though the underlying rule is symmetric, because the
two sides' kill rates relative to their own army sizes are not remotely
symmetric.

---

## 9. Tuning dials (all CVars, `Swarm.Feeding.*`)

| CVar | Default | Rationale |
|---|---|---|
| `Swarm.Feeding.Enabled` | `1` (bool) | Master on/off, so task-054 can reproduce the §3-style zero-input A/B baseline with and without feeding — the house measurement pattern (`GATE1-FUN-PROTOTYPE.md` §3). |
| `Swarm.Feeding.MaxPerCorpse` | `3` | The owner's explicit per-corpse cap (§2, §4). |
| `Swarm.Feeding.ChompRate` | `0.05` (s/HP) | Feed-duration slope against the corpse's `MaxHP` (§3). |
| `Swarm.Feeding.MinDuration` | `1.5` (s) | Floor so even a fodder kill produces a legible pause, not a flicker. |
| `Swarm.Feeding.MaxDuration` | `8.0` (s) | Ceiling so a future high-`MaxHP` titan/elite doesn't freeze a unit absurdly long (§3). |
| `Swarm.Feeding.RetinueFeedChance` | `0.35` | Brings the scale-invariant ~31% retinue duty-cycle (§8) down to a survivable ~11%. The load-bearing anti-stall dial. |
| `Swarm.Feeding.BroodFeedChance` | `0.85` | Brood pool is large enough (§8) that this stays high on purpose — feeding should read often as the horror beat (§7). |
| `Swarm.Feeding.Cooldown` | `4.0` (s) | Per-unit re-entry cooldown after a feed completes; stops a single high-kill-rate unit chain-feeding (§8). |
| `Swarm.Feeding.HealFraction` | `0.20` | Self-heal fraction of the feeder's own `MaxHP`, paid only on uninterrupted completion (§6). |
| `Swarm.Feeding.HeroExempt` | `1` (bool) | The hero never feeds — see §11. Exposed as a CVar per house convention (every dial gets one), but this should stay `1`; it isn't a balance knob, it's a design-law guardrail (GDD §4 hero relevance). |

---

## 10. Mass Entity cost breakdown — what's cheap, what isn't

Per design law 5 and `SwarmCombat.h:10-14`'s no-cross-entity-writes constraint:

**Cheap — no new query, no shared mutable state, self-contained per entity:**
- Killer-set resolution reusing the existing per-victim neighbor walk (§2).
- Full-duration-each countdown, one private fragment per feeder (§4) — this is
  *why* §4 picked full-duration-each over a shared pool: a shared "corpse HP
  remaining" counter that 1-3 feeders all read/write would be exactly the
  cross-entity contention the combat model was built to avoid.
- No persistent corpse entity, no despawn-timer bookkeeping (§5) — genuinely
  cheaper than the task brief's framing anticipated.
- "Stays vulnerable/targetable while feeding" (§1) requires **zero** new code
  in `USwarmCombatProcessor`'s victim-side logic — a feeding unit is already a
  valid target via the existing team+distance check; it just never registers a
  `bStriking` grid entry while its feeding fragment is active, so it can't
  land blows. No new incoming-damage path needed.
- `FeedChance` roll, `Cooldown` check, `HealFraction` payout — all self-reads on
  the entity's own fragment, all independent, all trivially parallel-safe.

**Moderately expensive — a real but bounded, one-time cost, not a new pattern:**
- `FGridEntry` (`SwarmSubsystem.h:130`) needs an `FMassEntityHandle` field it
  doesn't have today, and the dying victim needs to capture up to
  `MaxAttackersPerUnit` (4) claimant handles during the frame it dies, so the
  killer set (§2) can be turned into actual entity handles to attach feeding
  fragments to. This is a struct-widening plus a small fixed-size array
  (bounded by the existing `MaxAttackersPerUnit` clamp), computed during a walk
  the processor already performs — not a new spatial query, not a new
  cross-entity write, but it does touch a hot-path struct and is worth task-054
  budgeting explicitly rather than discovering mid-implementation.

**Explicitly avoided, and why:**
- Shared per-corpse damage pool (would need cross-entity read/write between up
  to 3 feeders every frame — rejected in §4).
- Persistent Corpse Mass entity with its own slot-tracking (rejected in §5 —
  the render-only artifact does the same job for less).
- Post-death opportunistic joining / a claim radius for late arrivals (rejected
  in §5 — would need a new spatial query specifically to find corpses with open
  slots, on top of everything else already run per frame).

---

## 11. Handoffs

**To task-054 (Mass build):** read §2 first — everything downstream depends on
the killer-set definition and the `FGridEntry` handle addition in §10. Build
order that avoids rework: (1) widen `FGridEntry` + capture claimant handles on
death, (2) add the per-feeder fragment (countdown, team, heal-on-complete flag),
(3) wire the two `FeedChance` rolls + `Cooldown` at kill resolution, (4) skip
locomotion/striking while the fragment is active, (5) render hook (below).
Reproduce the §3-style zero-input measurement with `Swarm.Feeding.Enabled 0/1`
to confirm the §8 numbers hold in the real sim, not just the scratch model.

**Render hook, not scoped here:** a feeding unit needs a distinct visible pose
(eating/sealing/kneeling — locked animation, not idle) so the "distracted, free
kill" read (§1) actually lands at horde scale, plus whatever cosmetic corpse
sprite persists for the render-only duration in §5. This is a
`performance-director` / render-bridge ask once task-054 has the fragment data
to key off; flagging it here so it doesn't get lost between specs.

**To task-002 (entity tier stat blocks / armor):** see §3's proxy-replacement
note — swap `MaxHP` for an effective-toughness figure at the single
`FeedDuration()` call site once armor lands. Nothing else in this spec changes.

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

---

## 13. Canon proposals

Not editing `SYSTEMS.md` per the task's ownership boundary — this is the entry
for whoever folds it in (`task-038` or a later pass):

| Date | Decision | Rationale | Spec / data |
|---|---|---|---|
| 2026-07-27 | **Feeding/distraction**: killer set (up to 3, resolved at the death frame, §2) goes null for `clamp(MaxHP × 0.05, 1.5, 8.0)`s, full duration each (§3/§4), stays vulnerable/targetable/immobile/non-damaging (§1), no persistent corpse entity (§5), small self-heal on completion (§6), symmetric mechanic / per-side fiction ("The Rite" vs "The Feed", §7) | Owner-specified tactical valve; MaxHP-proxied pending `task-002` armor; feed-chance dials (`RetinueFeedChance 0.35`, `BroodFeedChance 0.85`) are load-bearing against a scale-invariant ~31% retinue self-stall found in §8 | `docs/design/feeding-distraction.md` · `docs/data/feeding.json` |

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
