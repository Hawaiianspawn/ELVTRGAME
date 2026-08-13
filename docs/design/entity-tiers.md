# Entity tiers — fodder / soldier / elite / titan / boss

**What this is:** stat blocks for the enemy-side entity tiers left open by
`SYSTEMS.md` §1 ("full stat blocks for the elite/titan/boss tiers are still
open") and required by `docs/RTS-VERTICAL-SLICE.md` §4 ("entity tier stat
blocks for slice roster") and §5's bill-of-materials enemy roster (~5 types:
melee fodder, melee soldier, ranged, 1 anti-swarm elite, 1 boss). It also
introduces **Armor** as a first-class stat, because a live consumer
(`docs/design/feeding-distraction.md`, task-061) is currently proxying its
feed-duration formula off `MaxHP` with an explicit note that this doc owns the
real number.

**Extends:** `SYSTEMS.md` §1 (entity tiers — currently "still open" for
elite/titan/boss) and §6 (retinue tuning, for the Armor-vs-MaxHP interaction).
Reads `GDD.md` §7 (power scaling), §10 (Mass Entity constraints),
`docs/GATE1-FUN-PROTOTYPE.md` (the shipped combat model this extends without
changing), and `ELVTR/Source/ELVTR/Mass/SwarmCombat*.{h,cpp}` for what the sim
can actually express.

**Ownership boundary — what this doc does NOT redefine.** The friendly
retinue's own tier ladder (Freed/Militia/Veteran/Bannerman) is already
`SYSTEMS.md` §1-decided and numbered in `docs/data/upgrades.json`. This doc
only *reads* those numbers (as the reference attacker, §2.3) and does not
touch that file or that decision. Everything specced below is new: the enemy
roster's Fodder/Soldier/Elite/Titan/Boss tiers, and the Armor mechanic that
applies to any of them.

---

## 1. Taxonomy recap, and where the line falls

GDD §10 / SYSTEMS.md §1: `fodder → soldier → elite → titan → boss`.
Fodder/Soldier are **Mass Entity** — shared archetype, no per-unit uniqueness,
Design Law 5. Elite/Titan/Boss are **promoted Actors** — individually
meaningful, can carry a real fight (telegraphs, AoE, phases).

| Tier | Actor type | Slice roster? | Role |
|---|---|---|---|
| Fodder | Mass Entity | yes (shipped) | melee swarm — Brood, unchanged |
| Soldier (melee) | Mass Entity | yes (new) | tougher melee line, still swarm-cheap |
| Soldier (ranged) | Mass Entity | yes (new) | glass ranged threat, forces closing distance |
| Elite | Promoted Actor | yes (new) | anti-swarm — the roster's stated "1 anti-swarm elite" |
| Titan | Promoted Actor | **no** — see below | screen-filling wall, later-floor content |
| Boss | Promoted Actor | yes (new, baseline only) | the slice's 1 boss — full fight design is a separate deliverable |

**Titan is specced here for taxonomy completeness, not slice content.**
`RTS-VERTICAL-SLICE.md` §5's bill of materials lists exactly 5 enemy types for
the 3-floor slice and no titan. I'm giving it a stat block anyway because
SYSTEMS.md §1's taxonomy names it explicitly as "still open," and leaving a
named tier with zero numbers is worse than a clearly-flagged placeholder for
later floors. Nothing here schedules it into the slice.

**Technical precedent that de-risks the Actor promotion.** The codebase
already bridges a non-Mass Actor into the Mass grid combat model — the hero.
`SwarmCombatProcessors.cpp`'s `HeroMeleeRangeSq` / `HeroDamage` /
`FindOwnGridEntry` path is exactly "an Actor reads and writes against the same
grid Mass entities use, without becoming a Mass entity itself." Elite/Titan/Boss
should reuse this pattern rather than invent a second bridge — the "still
open" item in SYSTEMS.md §1 is the stat blocks and Armor mechanic below, not a
new architecture.

---

## 2. Armor — a first-class stat, not a proxy for MaxHP

### 2.1 Why not just raise MaxHP

Everything the shipped combat model expresses today is HP and DPS
(`FSwarmHealthFragment`, `SwarmCombatTuning::*MaxHP/*DPS`). An elite that
should "shrug off the swarm but still respect real threats" could in
principle be built as a bigger HP pool — but that produces a unit that is
*uniformly* harder to kill by everyone, including the hero and Veteran-tier
soldiers, which is the wrong shape: Design Law 4 needs the hero and quality
troops to stay relevant against exactly the targets meant to punish cheap
swarm tactics, not get diluted by the same wall that stops the swarm.

### 2.2 The mechanism: flat, per-blow damage reduction

```
EffectiveBlow(Attacker -> Victim) = max(AttackerBlow - Victim.Armor, ArmorChipFloor)
```

`ArmorChipFloor` (proposed `Swarm.ArmorChipFloor`, default **3**) guarantees a
lone attacker always makes *some* progress — GDD §7's "soft costs over hard
caps" applied to a per-hit mitigation stat, not just to army-size governance.

**This is a flat subtraction, not a percentage, and that's the load-bearing
choice.** A flat value removes a *larger fraction* of a small blow than of a
large one:

| Attacker | Blow (DPS x 0.9s) | vs Elite (Armor 12) | vs Titan (Armor 20) |
|---|---|---|---|
| Freed | 18.0 | 6.0 (**-67%**) | 3.0 floor (**-83%**) |
| Militia | 27.0 | 15.0 (**-44%**) | 7.0 (**-74%**) |
| Archer | 16.2 | 4.2 (**-74%**) | 3.0 floor (**-81%**) |
| Veteran | 40.5 | 28.5 (**-30%**) | 20.5 (**-49%**) |
| Hero | 49.5 | 37.5 (**-24%**) | 29.5 (**-40%**) |

Armor doesn't just make an elite "tanky" — it specifically **blunts the
cheap/numerous end of the roster (Freed, base Archers) far harder than it
blunts the invested end (Veteran, hero)**. That is the actual mechanical
definition of "anti-swarm elite" this system gives you: it isn't a bigger HP
bar, it's a stat that makes the *Promote* lane (SYSTEMS.md §7's triangle) the
correct answer instead of *Recruit*. Percentage-based mitigation could not do
this — it would blunt every attacker's blow by the same fraction and would
read as pure HP inflation with extra math.

**Where it lives in the combat model.** `FGridEntry::BlowDamage`
(`SwarmSubsystem.h`) already carries per-striker damage so a victim can be hit
by different attacker types for different amounts (the task-046 addition for
Spearmen-vs-Archers). Armor mitigation is the natural next step on the same
seam: the *victim* applies its own `Armor` when it consumes a claimed blow —
`Victim.HP -= max(Entry.BlowDamage - Victim.Armor, ArmorChipFloor)`. This
needs one new float on `FSwarmHealthFragment` (Fodder/Soldier) or its
promoted-Actor equivalent (Elite/Titan/Boss), read only by the entity that
owns it. No new cross-entity read, no new query shape, no violation of the
victim-pull rule `SwarmCombat.h` documents — it's a self-read at the exact
point damage is already being applied.

### 2.3 EffectiveHP — the single number the feeding-distraction handoff needs

`docs/design/feeding-distraction.md` §3 keys its feed-duration curve off
`MaxHP` and explicitly flags that it's a placeholder pending this doc's real
armor stat, offering `EffectiveHP = MaxHP + Armor x ArmorToDurationWeight` as
one candidate shape. I'm specifying the concrete formula instead of the
additive placeholder, because the additive shape has no natural units (what
is one point of Armor "worth" in HP without reference to some attacker's blow
size?) — a multiplicative form anchored to a real attacker closes that gap
cleanly:

```
EffectiveHP = MaxHP x RefBlow / max(RefBlow - Armor, ArmorChipFloor)
```

`RefBlow = 27` (Militia's blow — SYSTEMS.md §1's stated "balance anchor," the
one reference point already privileged elsewhere in canon). At `Armor = 0`
this reduces to `EffectiveHP = MaxHP` exactly — the formula is a pure
extension, not a redefinition, and every already-shipped Fodder/friendly-tier
number is unaffected.

**Where the MaxHP proxy actually reads wrong — concretely, not just in
principle.** `feeding-distraction.md` §3's clamp (`MaxDuration = 8.0s` at
`ChompRate = 0.05`) means *any* entity above `MaxHP ~= 160` already saturates
the feed-duration ceiling on `MaxHP` alone — Elite/Titan/Boss all clear that
bar by a wide margin regardless of Armor, so for those three tiers the proxy
and the real formula land on the **same clamped 8.0s**, and the divergence
this doc was asked to characterize is, for them, invisible in the output
(worth stating plainly rather than overselling the fix). The tier where it
actually bites is **Soldier (melee)**, which sits *below* the clamp:

| | MaxHP | Armor | `FeedDuration` from MaxHP alone | `FeedDuration` from EffectiveHP |
|---|---|---|---|---|
| Soldier, melee | 150 | 6 | **7.50s** | EffHP 192.9 -> clamped to **8.00s** |
| (hypothetical) same-HP, no-armor "brute" | 150 | 0 | 7.50s | 7.50s (identity, no divergence) |
| Soldier, ranged | 85 | 0 | 4.25s | 4.25s (identity — armor-less, proxy is exact) |
| Fodder | 60 | 0 | 3.00s | 3.00s (identity) |

Armor is exactly what pushes Soldier-melee's feeder from an unclamped 7.5s
read to the clamped 8.0s ceiling — a real, frame-visible timing difference
(0.5s), not an abstract percentage, and it's the one row in the current
roster where the MaxHP proxy would have quietly shipped the wrong number.
Every zero-armor row (Fodder, Soldier-ranged, and any future entity with
`Armor = 0`) is *exactly* reproduced by the proxy — the formula degrades to
identity gracefully, so nothing about the existing feeding spec's already-shipped
math for Brood/retinue corpses needs to change.

**Handoff to task-061 / whoever wires `FeedDuration()`:** swap the call
site's `MaxHP` argument for `EffectiveHP` as defined above — a single
substitution, per `feeding-distraction.md` §3's own request. `RefBlow` and
`ArmorChipFloor` are both in `docs/data/entity-tiers.json`'s
`design_constants` block so the substitution doesn't need a second source of
truth. Nothing else in that spec (the clamp, the per-corpse cap, the killer-set
definition, §5.5's stale-duration discount) changes — they're all agnostic to
what produces the input number, exactly as that spec's own handoff note says.

---

## 3. Per-tier stat blocks

Full numeric detail lives in `docs/data/entity-tiers.json` /
`entity-tiers.schema.md`; this section is the *why* behind each row.

### Fodder — Brood (shipped, unchanged)
60 HP / 35 DPS / Armor 0 / `TargetsPerHit` 1 / 95uu melee. Documented here
only to complete the taxonomy row — no redesign. A lone Militia kills one in
2.0s, matching `GATE1-FUN-PROTOTYPE.md`'s own stated "a soldier kills a brood
in 2s" (§3), which is a useful cross-check that the model in this doc
reproduces the already-playtested shipped numbers exactly before it's trusted
for the new tiers.

### Soldier, melee (new)
150 HP / 42 DPS / **Armor 6** / `TargetsPerHit` 1, same 95uu melee range and
0.9s cadence as Fodder — it shares Fodder's grid path entirely, no new query.
The step up from Fodder is deliberately "tougher and hits harder," not
"cleaves" — that differentiation is reserved for Elite. A lone Militia needs
6.43s to kill one (vs 2.0s for a Brood); it kills a Militia in 3.10s (vs a
Brood's 3.71s) — a soldier is a *harder* fight on both axes, the identity a
"soldier" tier should have relative to fodder.

### Soldier, ranged (new)
85 HP / 26 DPS / **Armor 0** / 700uu engage, 150uu min-engage, `TargetsPerHit`
1. Zero armor is deliberate — glass, dies fast once reached, mirrors the
retinue's own Archers pattern (`unit-types.json`) rather than inventing a new
shape. Engage range (700) is intentionally *shorter* than the retinue
Archers' own 750uu, so a player who invests in ranged retinue already
out-ranges this threat — a stated asymmetry, not an oversight.

### Elite — anti-swarm (new, the slice's "1 anti-swarm elite")
900 HP / 65 DPS / **Armor 12** / `TargetsPerHit` 5 (AoE) / 160uu melee /
1.8s `SwingInterval` (long relative to the shared 0.9s, so its swing reads as
a distinct telegraphed beat, Design Law 6). Armor 12 is chosen specifically
in the "bites Freed/Archers hard, barely touches Veteran/hero" band worked
out in §2.2 — this is the tier where Armor is doing its intended job most
visibly. `TargetsPerHit` 5 makes it punish the retinue for clustering on it,
the mirror image of what its own Armor already does to the retinue's cheap
attackers: both halves of "anti-swarm" (it resists being swarmed, and it
punishes being swarmed by it) come from the same two numbers.

### Titan — screen wall (new, **not built in the slice**)
3200 HP / 140 DPS / **Armor 20** / `TargetsPerHit` 8 / 220uu melee / 2.5s
`SwingInterval` / 0.5x move speed. Armor 20 floor-clamps Freed and base-Archer
blows to the chip floor entirely and still halves Veteran/hero blows — the
"you need a properly promoted army plus the hero, not just numbers" gate that
Design Law 1's exponential curve needs somewhere in the tier ladder. Slow move
speed is the counterplay: kiteable, not a pure DPS check. Flagged again:
speccing this fills SYSTEMS.md §1's open taxonomy slot; it schedules nothing
for the 3-floor slice.

### Boss — slice baseline (new, **stat block only**)
6000 HP / 110 DPS baseline / **Armor 14** / `TargetsPerHit` 6 / 250uu melee /
2.0s baseline `SwingInterval`. Armor is deliberately *lower* than Titan's
despite roughly double the HP: `RTS-VERTICAL-SLICE.md` §5 states the boss is
"positioning + stances, not a DPS check," so its toughness should read as
HP/phases/adds rather than as the same kind of armor-wall Titan is built to
be — two big targets with genuinely different identities, not two reskins of
the same number. **Full fight design (phases, arena, mechanics) is out of
scope for this doc** — it belongs to the separate "Boss & elite design" scope
area and should be built on top of this baseline, not instead of it.

---

## 4. Finding: melee is surround-capped against a single big target; ranged isn't

Elite/Titan/Boss are each **one** entity with **one** `Location` for the
grid's melee-range query — every retinue unit within `EngageRange` of that
point can strike it. Only finitely many bodies can physically occupy that
disc at once (collision + the existing separation force, `SeparationRadius =
60uu`). I model that as a **`SurroundCapEstimate`** per tier — a Fermi
estimate from a circle-packing approximation against each tier's `EngageRange`
and the measured compressed-combat spacing (~45uu, `GATE1-FUN-PROTOTYPE.md`
§3a) — **not measured in-engine**, and stated as a range (Elite 15-27, Titan
28-45, Boss 35-55) rather than false precision. `MaxAttackersPerUnit`
(`Swarm.MaxAttackersPerUnit`, default 4) is a *different* thing and doesn't
substitute for this: it's a per-frame safety clamp against a pathological
same-tick pile-up, not a sustained-combat concurrency limit — don't confuse
the two when this gets implemented.

**Ranged attackers (ArchersEngageRange 750uu) are not subject to this cap.**
An archer only needs range and a valid Kth-nearest target, not physical
contact — every archer in the army can contribute to a single-target fight
simultaneously, bounded only by how many archers exist, not by a body-packing
limit. Section 7's simulation shows this concretely: melee's contribution to
an Elite/Titan/Boss kill is **flat** past the surround cap (identical whether
the army is 50 or 250), while the fight only gets faster with army size
because the **archer** contribution keeps growing.

**This interacts with an existing default that's worth flagging up.**
`unit-types.json`'s `growth_source_weight` is 0.8 Spearmen / 0.2 Archers — the
current recruit mix is melee-heavy, i.e. weighted *away* from the sub-type
that actually scales against single big targets. That's not a bug in this
doc's numbers (Elite/Titan/Boss aren't tuned around a specific mix), but it is
a real tension worth naming: as specced, an Elite/Titan/Boss encounter
structurally rewards the 20% of the army that is Archers far more than the
80% that is Spearmen, and nothing in the current recruit-weight decision
(`unit-types.json`) was made with that in mind. **Not resolved here** — flagged
for whoever tunes `growth_source_weight` next, or for a future multi-hitpoint
boss design that gives melee a way to matter past its own surround cap (e.g.
several simultaneously-attackable weak points instead of one `Location`), a
genre-standard pattern worth considering when the full Boss & elite design
scope area gets built.

---

## 5. Handoffs

**To SYSTEMS.md (whoever folds this in — this doc doesn't own that file for
this task).** §1's "full stat blocks for the elite/titan/boss tiers are still
open" is answered by §3 above; the Armor mechanic (§2) is a new decision that
belongs in §1 or a new §1a once folded in, with a pointer to this file the
same way §1 already points at `upgrades.json`.

**To task-061 / whoever implements feeding on real corpses.** See §2.3 —
swap `FeedDuration()`'s `MaxHP` argument for `EffectiveHP`
(`docs/data/entity-tiers.json` has `RefBlow`/`ArmorChipFloor`). Single
substitution, nothing else in that spec changes.

**To whoever builds Elite/Titan/Boss in Mass/Actors.** Reuse the hero-bridge
pattern already in `SwarmCombatProcessors.cpp` (§1) rather than inventing a
new Actor-vs-Mass-grid path. Armor is one new float per entity, read
victim-side at the point `BlowDamage` is already consumed (§2.2) — no new
cross-entity write. `SurroundCapEstimate` (§4) is a design assumption to
validate with a real measurement (a `Swarm.SpacingReport`-style tool aimed at
a single large-collision target) before it's trusted for real balance, the
same way every other Fermi estimate in this repo gets flagged.

**To whoever tunes `unit-types.json` `growth_source_weight` next, or scopes
the Boss & elite design deliverable.** See §4's recruit-mix tension.

---

## 6. Narrative requests

Per the handoff convention — all six rows in `entity-tiers.json` are
`WorkingNameOnly: true` (`brood_fodder`, `brood_soldier_melee`,
`brood_soldier_ranged`, `brood_elite`, `brood_titan`, `brood_boss`), since
current canon (`FLAME-FOUNDATION.md`) names no factions or biomes yet.

- **Soldier, melee/ranged** — mechanically: a Brood that's visibly *equipped*
  (weapon/armor read, not just a bigger silhouette) rather than the fodder's
  bare/feral look, split into a melee brute and a ranged skirmisher. What a
  player must feel: these are what the fodder becomes once whatever took it
  has had time to arm it — a step toward "what you fight was taken, not born
  hostile" reading as *organized*, not just *numerous*.
- **Elite (anti-swarm)** — mechanically: shrugs off cheap/numerous attacks,
  punishes the retinue for clustering on it with a telegraphed AoE swing
  (1.8s windup). What a player must feel: this is the one that makes you stop
  recruiting and start promoting — it should read as *disciplined*, the
  opposite of the fodder's chaos, so throwing bodies at it visibly doesn't
  work.
- **Titan** — mechanically: armor-floors cheap attacks almost entirely, slow,
  screen-filling, kiteable rather than fightable head-on. What a player must
  feel: a wall you route around or bring your whole army's *quality* to, not
  a boss fight — later-floor content, not in the first act.
- **Boss (slice)** — baseline stats only; the narrative brief for the actual
  fight (phases, arena, what it *is*) belongs with the separate Boss & elite
  design deliverable, not this stat-block pass.
- **Gameplay readability constraint for all four new/promoted tiers, to
  carry into any art brief:** Elite/Titan/Boss telegraphs (1.8-2.5s windups)
  must read as a *distinct beat* at horde scale — silhouette or color shift
  during windup, not just a longer version of the shared swing pose, since a
  player scanning a screen full of Fodder/Soldier needs to pick out "this one
  is about to hit hard" without zooming in (Design Law 6).

---

## 7. Simulation notes

**What was simulated:** a closed-form Fermi model in a scratch Python script
(session scratchpad, not committed, reproducible from the numbers in this doc
and `docs/data/entity-tiers.json`) — no engine run. Same house method as
`feeding-distraction.md` §14: arithmetic built entirely from measured/shipped
constants (`SwarmCombatProcessors.cpp` CVar defaults, `upgrades.json`'s tier
numbers, `unit-types.json`'s recruit-mix weight), not new measurement.

**What it computed, and the headline results:**
1. **Sanity check against the shipped model.** A lone Militia (Blow 27) kills
   a Fodder Brood (HP 60, Armor 0) in exactly **2.00s**, matching
   `GATE1-FUN-PROTOTYPE.md` §3's own stated "a soldier kills a brood in 2s" —
   confirms the TTK formula reproduces already-playtested numbers before
   trusting it for the new tiers.
2. **Hero-solo TTK per tier** (55 DPS, Blow 49.5, Armor applied): Fodder
   1.09s, Soldier-melee 3.10s, Soldier-ranged 1.55s, **Elite 21.6s, Titan
   97.6s, Boss 152.1s.** The wide gap from Soldier to Elite is intentional —
   it's a direct, numeric demonstration that the hero "cannot replace the
   line" (GDD §4) against anything Armor-bearing, without needing a separate
   qualitative argument.
3. **Retinue-army TTK vs Elite/Titan/Boss at N = 50 / 120 / 250**, 80/20
   Spearmen/Archer split (`unit-types.json`'s shipped weight), melee capped
   at the tier's `SurroundCapEstimate`, archers uncapped, hero included:

   | Tier | N=50 | N=120 | N=250 |
   |---|---|---|---|
   | Elite (cap 20) | 2.13s | 1.85s | 1.48s |
   | Titan (cap 35) | 9.46s | 8.31s | 6.78s |
   | Boss (cap 45) | 9.22s | 7.80s | 7.01s |

   The melee (Spearmen) DPS contribution is **flat** across all three columns
   for a given tier once N's Spearmen count exceeds the surround cap (true at
   every N shown for Elite; true from N=120 up for Titan/Boss) — all of the
   improvement from 50 -> 250 comes from the uncapped Archer contribution
   (§4). This is the finding behind §4's recruit-mix flag, not an assumption
   going in — it fell out of running the numbers.
4. **EffectiveHP vs MaxHP-only feed-duration proxy** (§2.3's table) — the
   divergence is real but narrow: it matters for exactly one current row
   (Soldier-melee, 7.5s proxy vs 8.0s real) because every entity above ~160
   `MaxHP` already saturates `feeding-distraction.md`'s own duration clamp
   regardless of Armor.

**Assumptions, stated plainly (none of these are measured):**
- `SurroundCapEstimate` per tier (§4) — a circle-packing Fermi estimate
  against `EngageRange` and the measured compressed-combat spacing (~45uu),
  not a real crowd-sim measurement. Given as a range for exactly that reason.
- 80/20 Spearmen/Archer composition assumed uniform across all three army
  sizes — the real recruit mix drifts over a run (squad-group-system.md's own
  simulation notes flag the same caveat for its own extrapolations) and isn't
  separately re-derived here.
- `RefBlow = Militia (27)` as the single reference attacker for `EffectiveHP`
  (§2.3) is a design choice (Militia is SYSTEMS.md's stated "balance
  anchor"), not something the sim could measure — a different reference
  attacker would shift every `EffectiveHP` number, though the *qualitative*
  finding (armor bites cheap attackers harder, in the direction that matters
  for feed-duration) is robust to that choice.
- Elite/Titan/Boss `SwingInterval` values (1.8s/2.5s/2.0s) are readability
  choices (Design Law 6), not derived from anything measured — first-pass
  telegraph timings, to be judged by feel once these exist on screen, the
  same status as every other timing constant in `docs/data/`.
