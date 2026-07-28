---
id: 053
title: Spec the feeding-distraction mechanic — killers go null on the corpse, three per body, armor sets the chomp
status: needs-review
agent: gameplay-director
owns: ["docs/design/feeding-distraction.md", "docs/data/feeding.json", "docs/data/feeding.schema.md"]
resources: []
depends-on: []
epic: feeding-distraction
evidence: docs/design/feeding-distraction.md with the feed-duration curve, the per-corpse slot rules, and a simulated wave-3 walkthrough showing what fraction of each side is feeding at peak — plus docs/data/feeding.json importing cleanly as a UE DataTable.
score: {gate: 1, risk: 2, cost: 2}
source: user
teammate: feeding-distraction-spec
decided: "2026-07-27 needs-review"
---

## Why now

The owner wants a new tactical valve: a unit that lands a killing blow stops fighting and
devours the body, going effectively null for a duration set by how armored the corpse was.
Up to three attackers can crowd one body. Both sides do it.

That is a real mechanic and it changes the shape of every fight — a big armored death
becomes a hole in the enemy line you can drive through. But it lands on a combat model that
has none of the three things it needs. There is no armor stat anywhere in the project. There
are no corpses — `USwarmDeathProcessor` (`SwarmCombatProcessors.cpp:455`) destroys the entity
the frame HP crosses zero. And combat is continuous attrition with no kill attribution at all
(`SwarmCombat.h:10-14`): nobody "defeats" anybody, HP just bleeds at `DPS × EnemyCount × dt`.

So the design has to be settled before anything is built, or the build task will invent
answers to four questions that belong to the design. This task is that settlement.

## Done when

`docs/design/feeding-distraction.md` answers all of the following, each with a number and a
reason, not a gesture:

- **What "null" means, exactly.** Does a feeding unit stop dealing damage, stop taking
  damage, stop moving, stop being targetable, all four? Each answer is a different mechanic.
  A feeder that still soaks damage is a body-block; one that is untargetable is a unit
  removed from the fight. Pick, and say what the player is supposed to read off it.

- **The feed-duration curve, keyed to the corpse's MaxHP.** Armor does not exist yet and the
  owner has agreed to proxy off `MaxHP` until it does (`task-002` owns the real stat). Give
  the actual function — linear, floor, ceiling — with the resulting durations for a brood
  corpse and a retinue corpse at current tuning (`SwarmCombatTuning::BroodMaxHP()` /
  `RetinueMaxHP()`, both CVar-backed in `SwarmCombatProcessors.cpp`). State plainly where the
  `MaxHP` proxy will read wrong once armor lands, so `task-002` inherits a clean handoff.

- **How three feeders share one corpse.** The cap is per-corpse, not global. Settle whether
  three feeders finish the body in a third of the time (a race that resolves fast) or each
  serves its own full duration (three units held for the same span). These produce opposite
  tactical behavior — the first makes crowding efficient, the second makes it a trap.

- **What happens when the body is gone.** Does the corpse vanish on consumption, does it
  linger, and do late arrivals that found the slots full ever get to feed?

- **Whether feeding pays.** The owner specified distraction, not reward. But an involuntary
  null with zero upside is pure punishment for whoever kills — which reads as a bug unless
  it is clearly instinctive and out of the unit's control. Decide whether feeding heals,
  buffs, or gives nothing, and make the answer consistent with it being involuntary.

- **The symmetry problem, honestly.** The owner chose both sides feed. Mechanically symmetric
  is fine; **narratively it is not.** Design law 9 (`gameplay-director.md:45`) is *"what you
  fight was taken, not born hostile"* and the tone decision is *"we are the good guys"*
  (`GDD.md:172`, `CLASSES.md:5`). Your retinue eating corpses breaks that. The likely answer
  is that the mechanic is symmetric while the *fiction* differs per side — the brood devour,
  the retinue do something else that costs the same time (stripping a body for salvage,
  burning it so it cannot rise, kneeling over a fallen comrade). Name what the retinue side
  actually is. Do not write cannibal retinue and do not quietly drop the symmetry the owner
  asked for — resolve it.

- **The density check that decides if this is viable at all.** Both sides feeding at wave-3
  numbers (700 brood, per `SYSTEMS.md:45`) could take a large fraction of the field offline
  mid-fight. Simulate it: at the slice's kill rate, what percentage of each side is feeding
  at peak, and does the fight still read? If the honest answer is that it stalls combat, say
  so and propose the bound — a global ceiling on top of the per-corpse cap, a feed chance
  under 100%, or brood-only feeding as a fallback.

- **`docs/data/feeding.json`** carries every tuning value, with a schema doc beside it,
  importable as a UE DataTable — matching the pattern in `docs/data/unit-types.json`.

## Spawn prompt

```
You are executing task-053 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

GOAL, from the owner, in their words: "a monster eating feature that basically if they defeat
a monster they become null and eat the body. Up to three can be distracted, or armor can play
with how long one can be distracted (have to chomp through the damage)."

The owner has already settled four forks — treat these as given, not open:
1. BOTH SIDES FEED. Symmetric: anything that lands a killing blow goes null on the corpse.
2. THE CAP OF THREE IS PER CORPSE. One body can occupy up to 3 attackers at once; a 4th
   finds no slot and keeps fighting.
3. FEED DURATION PROXIES OFF THE DEAD UNIT'S MaxHP, because armor does not exist yet.
4. THIS TASK IS SPEC ONLY. A separate task (054) builds it in Mass and will read your spec
   as its input. Write for that reader.

WHAT ALREADY EXISTS — read all of this before designing anything, because the mechanic
depends on three things the project does not currently have:

- ELVTR/Source/ELVTR/Mass/SwarmCombat.h lines 10-14. Combat is CONTINUOUS ATTRITION, not
  discrete swings: each unit counts enemies in its melee radius and bleeds HP at
  DPS * EnemyCount * dt. There are no damage events and NO KILL ATTRIBUTION — nobody
  "defeats" anybody. A swing cadence was layered on later (SwingInterval / SwingStrikeAt,
  and strikers hit their K nearest per RetinueTargetsPerHit / BroodTargetsPerHit), so a
  killing blow is DERIVABLE, but it is not tracked today. Your spec must say what counts as
  "the killer" in this model.
- ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp:455. USwarmDeathProcessor destroys the
  entity the frame HP <= 0. THERE ARE NO CORPSES. A body that persists long enough to be
  eaten is a new state, and how long it persists is a number you own.
- THERE IS NO ARMOR STAT. Not in SwarmCombat.h (HP / DPS / melee range only), not in
  SYSTEMS.md, not in CLASSES.md. task-002 (Spec the entity tier stat blocks) is where armor
  properly belongs and it is still `proposed`. You are proxying off MaxHP deliberately, as a
  bridge. Write the handoff note that lets task-002 replace the proxy cleanly.
- Current tuning lives as CVar-backed getters in SwarmCombatProcessors.cpp:
  SwarmCombatTuning::RetinueMaxHP() / RetinueDPS() / BroodMaxHP() / BroodDPS(). Read the
  actual defaults out of that file — do not guess them. SwarmCombat.h:56-59 records the
  intent behind them: one soldier kills a brood in ~2s and survives ~2.3s against four, so
  BEING SURROUNDED is what kills you, not aggregate numbers. Feeding changes how many
  attackers are on a unit at a given moment, which means it lands directly on that
  relationship. Say what it does to it.
- SYSTEMS.md:45 — the slice curve is locked at 250 -> 450 -> 700 brood across three waves.
  Wave 3 is your density test case.

WHAT TO PRODUCE:

docs/design/feeding-distraction.md, answering every bullet in this task file's "Done when"
section. The ones most likely to be hand-waved, so do not hand-wave them:

A. WHAT "NULL" MEANS. Stops dealing damage / stops taking damage / stops moving / stops
   being targetable — four independent switches. Pick each one and justify it. A feeder that
   still soaks damage is a body-block the player can exploit; an untargetable one is a unit
   deleted from the fight for N seconds. These play completely differently.

B. THE THREE-FEEDERS-PER-CORPSE RULE. Do three feeders finish the body in a third of the
   time, or does each serve a full duration? First makes crowding efficient, second makes it
   a trap. This is the single most important tuning decision in the feature — the owner's
   stated fantasy is that a big armored death punches a HOLE in the enemy line, so pick the
   one that produces that and explain how.

C. THE DENSITY CHECK. This is the one that can kill the feature, so do it early and for
   real. At wave 3 (700 brood) with both sides feeding, work out from the current DPS/HP
   numbers roughly how many kills per second are happening, multiply by feed duration and by
   up-to-3 feeders, and state what fraction of each side is null at peak. If that number is
   large enough to stall the fight, SAY SO and propose the bound (global ceiling on top of
   the per-corpse cap, sub-100% feed chance, or brood-only as a fallback). A spec that
   asserts this will be fine without arithmetic is not done.

D. THE TONE PROBLEM. The owner chose symmetric feeding. Design law 9 in your own agent
   definition is "what you fight was taken, not born hostile", and GDD.md:172 / CLASSES.md:5
   lock the tone as "we are the good guys". Retinue soldiers eating corpses violates that
   outright. Resolve it rather than picking a side: the mechanic can stay symmetric while
   the FICTION differs per side — the brood devour; the retinue do something that costs the
   same seconds and means something else (stripping a body for salvage, burning it so it
   cannot be taken, kneeling over a fallen comrade). Name it concretely and make the
   duration-scales-with-armor logic still make sense for whatever you name. If you genuinely
   cannot make symmetric work with the tone, say that plainly and recommend brood-only — but
   the owner asked for symmetric, so that is a recommendation you have to earn.

E. docs/data/feeding.json with every tuning value, plus docs/data/feeding.schema.md beside
   it. Follow the existing pattern in docs/data/unit-types.json and its schema doc. It must
   import cleanly as a UE DataTable.

CONSTRAINTS:
- MASS ENTITY CONSTRAINTS ARE DESIGN LAW (design law 5, and GDD section 10). No per-unit
  uniqueness, no special-casing at horde scale. Note especially that "claim one of three
  slots on a corpse" is a CROSS-ENTITY WRITE, and the whole combat model is deliberately
  chunk-local and parallel-safe with no random-access writes between entities
  (SwarmCombat.h:10-14). Do not design something that requires per-frame arbitration between
  arbitrary entities. If your rule needs it, find a formulation that does not — a
  deterministic slot derived from spatial order, a claim resolved in a dedicated pass, or a
  probabilistic bound. Flag which of your rules are the expensive ones so task-054 knows
  where the cost is.
- Every tuning value should be expressible as a CVar so it can be tuned live — that is the
  house pattern (see the Swarm.* dials in SwarmCommands.cpp and SwarmCombatProcessors.cpp).
  Name the dials you want and give each a one-line prose rationale.
- You have NO SHELL restriction here — you may use Bash/PowerShell for arithmetic and to read
  files. You may NOT build or run the editor; this task claims no `unreal-editor` resource.

YOU OWN, and may write only: docs/design/feeding-distraction.md, docs/data/feeding.json,
docs/data/feeding.schema.md.

DO NOT TOUCH: SYSTEMS.md, GDD.md, CLASSES.md (task-038 owns the SYSTEMS.md write and the
others are canon), any ELVTR/Source/** file (task-054 builds this), docs/data/entity-tiers*
(task-002), docs/data/unit-types* or docs/design/squad-group-system.md (task-049), or any
docs/backlog/ file. If your spec implies a SYSTEMS.md decision record, write the proposed
entry INTO your own doc for later folding — do not edit SYSTEMS.md.

CANON WARNINGS:
- WORLD.md IS SUPERSEDED by the 2026-07-22 narrative reset. Current canon is
  docs/narrative/FLAME-FOUNDATION.md — you bear the only flame in a pitch-dark world, your
  army needs your light, bearers are treated as gods. Do not cite WORLD.md.
- docs/perf/niagara-sprite-refactor.md sections 2 and 8.1 carry a RETRACTED claim that the
  swarm emitter draws zero particles. Irrelevant to you, but do not repeat it.

HAND BACK: the feed-duration curve with real numbers for a brood and a retinue corpse, your
answer on the three-feeder sharing rule and why it produces the "hole in the line" fantasy,
the wave-3 density arithmetic and your verdict on whether symmetric feeding survives it, how
you resolved the tone problem, and a clear list of which rules are cheap and which are
expensive for the Mass implementation to follow.
```
