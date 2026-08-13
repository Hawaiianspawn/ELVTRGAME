# Loot v0 — battle drops + the growth-site item shelf

**What this is:** the design for `RTS-VERTICAL-SLICE.md` §4's still-unchecked
line, `Loot v0: unit orbs + healing + ~4-6 stacking items (not the real loot
system)`, extended (owner request, 2026-07-28) to add a third real-time
category: **buffs**. Closes "unit orbs," "healing," and now "buffs" with three
new real-time battle-drop types; the "4-6 stacking items" fourth component
is **already decided** (`SYSTEMS.md` §3, 2026-07-24, `docs/data/upgrades.json`
`items.catalog`) and is referenced here, not redesigned.

**Extends:** `SYSTEMS.md` §3 (Loot), which this doc completes rather than
reopens — see §1 below for the exact gap it closes. Reads `GDD.md` §8 (loot
direction, deferred-by-design placeholder) and §12 Q7 (loot deferred to this
doc's scope), `docs/design/entity-tiers.md` (the Fodder/Soldier/Elite/Boss
tiers this doc's drop tables are keyed to), `docs/design/scaling-curve.md`
(the locked floor populations this doc's Monte Carlo runs against — task-003,
closed, numbers final), `SYSTEMS.md` §7 (Supply/Embers economy — this doc's
Unit Orb is additive to it, not a second economy), and
`docs/design/feeding-distraction.md` (the corpse/claim-radius pattern this
doc's battle drops deliberately reuse rather than inventing a second one).

**Does not touch:** `docs/data/upgrades.json`, `docs/data/economy.json`,
`docs/data/growth-sites.json`, `SYSTEMS.md`, `GDD.md`, `ELVTR/Source`,
`ELVTR/Content`. Everything tuned here is new and lives in
`docs/data/loot-v0.json` / `loot-v0.schema.md`.

---

## 1. What SYSTEMS.md §3 already decided, and what this doc adds

`SYSTEMS.md` §3 declares "Loot v0 (slice only)" **DECIDED** and points at
`upgrades.json`'s 6-item stacking catalog. Read narrowly, that closes the
checklist line — but `RTS-VERTICAL-SLICE.md` §4 still shows the box
unchecked, and for a real reason: the stacking-item catalog is a
**growth-site shelf item** — offered 3, take 1, spent from the same Ember
pool as Recruit/Promote/Provision (`growth-sites.json`). It is not "unit
orbs," and it is not "healing." Those two (plus buffs, added below) are a
different delivery grain entirely: `GDD.md` §3's own moment-to-moment loop is
*"Move → position your retinue → fight → collect drops → push deeper"* —
**"collect drops" is a combat-second beat**, not a breather-menu beat.
Nothing in the repo built that beat yet. This doc does, and the result is
that Loot v0 is genuinely **two delivery grains, not one**:

| Grain | Cadence | Currency | Choice | Owner |
|---|---|---|---|---|
| Battle drops — units, healing, buffs (**new, this doc**) | real-time, mid-fight | none — automatic pickup | none — no menu | `loot-v0.json` |
| Growth-site items (**existing**) | breather | Embers | curated, offer-3-take-1 | `upgrades.json` |

Both matter and neither replaces the other. §7 below explains why they don't
compete for the same design budget — including the one place a battle drop
(Rally Ember, §6) does something shape-similar to an existing growth-site
item (Whetstone) and why that's fine.

---

## 2. Design principles this doc holds itself to

- **Loot feeds both hero and retinue** (`GDD.md` §8's stated twist). Unit Orb
  feeds the retinue directly (a body); Kindling Ember and Rally Ember feed
  both hero and retinue (HP, and a DPS buff respectively) in the same
  pickup. Nothing here is hero-only.
- **Run-scoped, no persistent gear** (Design Law 7). All three drop types are
  consumed instantly on pickup — there is no inventory, no carry-over between
  floors, no state that survives a run. Rally Ember's buff is temporary
  *within* that same run-scoped frame (§6) — it isn't gear, it's a clock.
- **No rarity tiers, no evolution trees.** Per the task brief's explicit stop
  condition. Each drop type has exactly one, unconditional effect — Rally
  Ember stacks in magnitude (up to a flat cap) and refreshes in duration, but
  there is no roll table, no upgrade path, and no second effect it can
  become.
- **Mass Entity constraints apply to drops too** (Design Law 5), not just to
  units. A drop is a small, data-cheap record — never a Mass entity, never
  something a unit actively seeks. §3 below is built entirely around reusing
  a pattern this repo already reviewed and shipped for exactly this shape of
  problem (`feeding-distraction.md`'s persistent-corpse claim pass), rather
  than inventing a second one. Rally Ember's buff is a single global
  multiplier and a single global expiry clock — not a per-unit fragment —
  for the same reason.
- **Additive to the Supply/Ember economy, never a second economy.** A
  Unit-Orb-recruited Freed unit draws upkeep exactly like any other Freed
  unit (`economy.json`'s uniform 1/unit demand). There is no orb-exempt unit,
  and none of the three drop types can be banked, saved, or converted into
  Embers — they are a pure, small, real-time bonus layered on top of the
  growth-site triangle, not a parallel currency competing with it (see §7).

---

## 3. The shared mechanism: reuse the corpse claim pattern, don't invent one

`feeding-distraction.md` already solved "a small number of world objects that
units incidentally interact with, at horde scale, without a new steering
system or per-unit special-casing" — and it solved it twice (killer-claim,
then walk-up-claim) with real cost accounting both times. All three drop
types in this doc are built on the same shape, deliberately:

- **A drop is a small, non-Mass record** (`Location`, `DropType`,
  `SpawnTick`), living in a small array on `USwarmSubsystem` alongside the
  existing `Grid` and (once built) the corpse arrays — not a Mass entity, no
  archetype, no chunk iteration, exactly the reasoning `feeding-distraction.md`
  §5.1 already made for corpses.
- **"Units notice, they don't seek."** No unit or the hero ever breaks off
  its current target, formation slot, or stance behaviour to walk toward a
  drop. A drop is collected only as an incidental consequence of existing
  combat/formation movement carrying a friendly entity within
  `PickupRadius` — the identical rule `feeding-distraction.md` §5.2 states
  for corpse claiming, reused verbatim rather than re-derived.
- **Resolution is drop-centric, not unit-centric**, for the same cost reason:
  iterating a bounded number of drops (`MaxActiveDrops = 60`) and querying
  the existing spatial grid within `PickupRadius` of each is far cheaper than
  every unit asking "is there a drop near me" every tick — the same
  "far fewer corpses than units" formulation `feeding-distraction.md` §5.2
  already validated.
- **`PickupRadius = 150uu`, reused wholesale from
  `Swarm.Feeding.ClaimRadius`.** Same justification applies unchanged: close
  enough to read as "standing next to it" (`MeleeRange` is 95uu), inside
  `GridCellSize` (250uu) so the pickup pass reuses the same 3×3-cell
  neighbour scan the combat pass already performs.
- **A despawn timer + population cap bound the cost**, mirroring
  `MaxCorpsesPerTeam`'s "graceful skip" rule: at the volumes this doc's
  simulation produces (§8), neither is expected to bind — they exist as
  safety valves, not as tuned limits.

Unlike feeding, collection here is **deterministic on contact**, not a
per-second hazard rate — pickups are meant to be collected, not survived, so
there is no reason to gate them behind a probability the way a *vulnerability*
mechanic needs to be gated. The reuse is the resolution pattern and the cost
shape, not the hazard-rate mechanic itself.

---

## 4. Unit Orb (working name) — feeds the retinue directly

**Effect:** on pickup by any friendly entity, instantly adds **+1 Freed-tier
unit** to the retinue, via the same spawn entry point growth-site Recruit
already uses (`SwarmSpawn.h`, per `GATE1-FUN-PROTOTYPE.md` §2's existing
"shared spawn entry points" note). The new unit enters at Freed tier — the
same rule every recruited unit follows (`SYSTEMS.md` §1) — and it is **not
upkeep-exempt**: it adds +1 to Supply demand like any other Freed unit. There
is exactly one definition of "what a unit costs" in this system, and Unit Orb
does not create a second one.

**Drop source:** Soldier-melee and Soldier-ranged kills only, at **0.6%**
per kill — deliberately **never Fodder**. Two reasons, both load-bearing:

1. **Volume.** A floor has 200–300+ Fodder kills; even a very low per-kill
   chance would flood the pickup pass and the screen. Soldier kills are an
   order of magnitude rarer, which keeps drop volume — and the pickup-pass
   cost (§3) — proportional to `MaxActiveDrops` headroom by construction.
2. **Readability and identity.** `entity-tiers.md` already draws the
   Fodder/Soldier line as "a Brood that's visibly equipped... a step toward
   organized, not just numerous" (§6 Narrative requests). Making the reward
   tier match that line — you have to beat something that fights back to earn
   a body — keeps the drop legible as a *reward for the harder fight*, not
   background noise (Design Law 6 applied to a drop, not just a threat).

**Guaranteed drops:** Elite kills grant **+1** Unit Orb; Boss grants **+2**.
These are fixed counts on a named-instance death, not a per-kill roll —
Elite/Boss are single `PromotedActor` instances (`scaling-curve.json`
`elite_boss_schedule`), not part of the Mass-Entity population count the
probabilistic roll applies to. This gives the fights `entity-tiers.md` calls
"the one that makes you stop recruiting and start promoting" a small,
earned body reward for succeeding at them anyway — a nice-to-have, not a
contradiction of that tier's own design intent (the reward is a few extra
bodies for winning the fight, not a discount on fighting it).

**What this deliberately is not:** a fix for `scaling-curve.md` §3's
flagged headcount gap (the growth-site economy's army sizes sitting at a
worse population/army ratio than gate-1's own measured collapse point). §8's
simulation shows the cumulative Unit Orb yield across all 3 floors (~8 units)
is nowhere near the scale of that gap (that doc's own framing: floor 1 alone
needs roughly 3× more bodies than the economy currently produces). Unit Orb
is a felt, real-time bonus layered on a working economy — it is not sized to
patch a structural shortfall in that economy, and this doc doesn't claim
otherwise.

---

## 5. Kindling Ember (working name) — feeds hero and retinue HP

**Effect:** on pickup, applies a single AoE heal burst centred on the drop's
location (radius **250uu**, one `GridCellSize`): the hero, if inside the
burst, is healed **25 HP** (5% of the hero's 500 MaxHP —
`GATE1-FUN-PROTOTYPE.md` §3 tuning history); every retinue unit inside the
burst is healed **15 HP** (a little over half a Militia blow, 27 —
`entity-tiers.json`'s `RefBlow`). A flat, uniform formula, applied once at
pickup — no per-unit special-casing, no new per-tick system layered onto the
combat model (Design Law 5).

**Preventative, not restorative-from-death.** Kindling Ember cannot revive a
downed unit — only reduce how much a wave costs the line while it's still
standing. This keeps Vanguard Veteran permadeath (`CLASSES.md` §5's
class-aware sacrifice pricing, "Vanguard Veterans permadeath") meaningful:
Kindling Ember can help a unit survive the fight that would have killed it,
but it never undoes a death that already happened.

**Drop source:** any kill can drop one — **1.5%** per Fodder kill, **3%**
per Soldier kill (melee or ranged), plus a guaranteed **+1** on an Elite kill
and **+2** on a Boss kill. Unlike Unit Orb, Fodder *does* contribute here:
healing carries no economy risk (it never touches Supply demand or Embers),
so there's no reason to withhold it from the tier that dies by the hundreds
— and a frequent, low-stakes drop is exactly the Vampire-Survivors-style
cadence the task brief asks for ("frequent drops," GDD §8).

---

## 6. Rally Ember (working name) — feeds hero and retinue power, temporarily

**Effect:** on pickup, applies one stack of a **temporary DPS buff** to the
hero and the whole retinue simultaneously — **+10% DPS per stack, up to 3
stacks (+30% cap), lasting 15 seconds.** A pickup while a buff is already
active refreshes the single global duration clock; a pickup at the stack cap
still refreshes duration without adding a fourth stack (a diminishing-returns
clamp on this one mechanic, not a claim about the power curve's own cap —
Supply/upkeep remains the real governor, `SYSTEMS.md` §7). Implemented as one
global multiplier read at damage-application time, the same shape the Supply
degrade multiplier already uses — not a per-unit fragment, no per-stack
bookkeeping (Design Law 5).

**Why this is the third category, not a rename of the other two.** Unit Orb
feeds the retinue's *count*; Kindling Ember feeds *survivability*. Neither
gives the moment-to-moment spike Design Law 1 asks for ("breakpoints and
run-defining pickups... create spike moments, not just smooth growth").
Rally Ember is that spike, delivered at combat speed rather than at a
growth-site breather.

**Drop source:** Soldier-melee and Soldier-ranged kills only, at **1%** per
kill, plus a guaranteed **+1** on an Elite kill and **+1** on a Boss kill —
never Fodder, for the identical volume/readability reasons Unit Orb gives
(§4). The Elite/Boss guarantee is deliberate: those are exactly the fights
where a temporary DPS spike matters most, so the drop that helps most with a
tough point-target fight (`entity-tiers.md` §4's melee-cap finding — a DPS
buff is one of the few things that helps *every* attacker against a
surround-capped target, archers and capped melee alike) is guaranteed
right where the player needs it, not left to chance.

**Relationship to `whetstone` (the existing growth-site item).** `upgrades.json`
already has a permanent, Ember-bought +3 DPS/stack item with the identical
"stacks, applies to all Liberated" shape. Rally Ember is deliberately the
temporary, free, automatic counterpart — see §7 for why the two coexist
rather than one making the other redundant. Naming leans on the Vanguard's
own "Rescue & Rally" growth verb and "To the Banner" rally stance
(`CLASSES.md` §1) — a battlefield-fervor effect, not a light/flame effect,
which is reserved narrative territory for the Lampbearer's "the light
sustains" identity (`CLASSES.md` §4, see §10 below).

---

## 7. Why battle drops don't compete with the growth-site triangle

`SYSTEMS.md` §7's acceptance test is explicit: *"if a single allocation
always wins, the triangle has failed."* Battle drops are designed to sit
outside that test entirely, on purpose:

- **They spend no Embers and offer no choice.** A player cannot allocate
  toward "more orbs," "more healing," or "more buff uptime" the way they
  allocate toward Recruit/Promote/Provision — there is no lever here to make
  dominant or degenerate. The triangle's tension is unaffected by this doc.
- **Unit Orb still draws upkeep** (§4), so it cannot make Recruit's upkeep
  cost look worse by comparison, and it cannot be stacked to bypass the
  Supply governor — it's subject to the exact same degrade math as every
  other Freed unit (checked directly in §8's Supply-interaction result).
- **The one real interaction — and it's a deliberately honest one:** extra
  Freed units from orbs *do* add to Supply demand, so a recruit-max run (the
  scenario `scaling-curve.md` §2 shows already flirting with the degrade
  line by floor 3) picks up a slightly worse degrade multiplier than it
  would without orbs (§8: 0.944 → 0.867 at floor 3). That's not a bug —
  it's the same governor (`SYSTEMS.md` §7, "degrade, don't cap") doing its
  job on a body count the player didn't directly choose, which is exactly
  the "soft cost, not hard cap" posture Design Law 2 asks for applied
  consistently, not an exemption carved out for combat drops.
- **Rally Ember and Whetstone don't double-count, and don't compete for the
  same decision either.** Whetstone's flat +3 DPS/stack applies to the
  army's base DPS; Rally Ember's +10%/stack is a multiplier evaluated on top
  of whatever that base already is (including any Whetstone stacks) — one
  flat term, one multiplicative term, applied in a fixed order, never both
  reads of the same number. And because Rally Ember costs no Embers and
  offers no choice, it can never be "the reason not to buy Whetstone" — a
  player who never sees a single Rally Ember drop still faces the exact same
  growth-site Item-lane decision `SYSTEMS.md` §7 designed. The two systems
  are additive in effect and independent in decision-space, which is why
  shipping both isn't the same mistake as offering the same choice twice.

---

## 8. Simulation notes

**What was simulated:** a 20,000-run binomial/normal-approximation Monte
Carlo (scratch Python, session scratchpad, not committed — reproducible from
`loot-v0.json`'s `drop_sources[]` chances and `scaling-curve.json`'s
`floor_roster[]` per-floor tier counts), computing, per floor: (1) Unit Orbs
spawned and collected, (2) Kindling Embers spawned and collected, (3) Rally
Embers spawned and collected, (4) the cumulative Unit Orb total across all 3
floors, (5) that total's size relative to a single growth-site Recruit action
(+10 Freed for 12 Embers), (6) the Supply/degrade impact of adding that total
on top of `scaling-curve.md` §2's two bounding scenarios (balanced,
recruit-max), (7) a Fermi-level healing magnitude sanity check against the
hero's MaxHP.

**Method note on the "collected" step:** spawned counts are drawn directly
from `drop_sources[]`'s stated per-kill chances (a straightforward binomial
over each floor's kill counts, normal-approximated where `n×p` is large
enough for that to be a good fit — soldier/fodder counts here, hundreds per
floor). Collected counts then apply the `assumed_pickup_rate` (0.92, stated
as unmeasured in `loot-v0.json`) as a second binomial thinning step. Both
steps are Fermi arithmetic over already-decided repo numbers, the same house
method `entity-tiers.md` §7 and `scaling-curve.md` §7 already use for a
non-engine estimate — not a claim to have measured anything in-engine.

**Headline results:**

| Floor | Unit Orbs spawned (mean, p10–p90) | Unit Orbs collected (mean) | Kindling Embers collected (mean) | Rally Embers collected (mean) |
|---|---|---|---|---|
| 1 | 0.23 (0–1) | 0.21 | 4.00 | 0.34 |
| 2 | 2.21 (1–4) | 2.03 | 9.92 | 2.78 |
| 3 | 6.30 (4–8) | 5.80 | 18.65 | 6.31 |

- **Cumulative Unit Orbs collected across the whole run: 8.04 (mean).**
  Floor 1 produces zero orbs in **79.4%** of simulated runs — appropriate for
  a floor with only 38 Soldier kills to roll against; this reads as "a rare
  early treat," not a reliable crutch, which matches the tier's own stated
  identity (§4).
- **As a fraction of one Recruit action (+10 Freed):** floor 1 ≈ 2%, floor 2
  ≈ 20%, floor 3 ≈ 58%. Floor 3's share is the largest because 4 of its
  expected ~5.8 collected orbs come from the guaranteed Elite×2/Boss×1
  drops, not the base per-kill roll — the probabilistic component alone
  (~2.3 at floor 3) stays modest even as population triples. This was a
  design target, not an accident: an earlier pass at 2%/kill produced a
  floor-3 total exceeding one full Recruit action (117%), which would have
  meant the free, automatic channel out-yielding the deliberate,
  Ember-spent one — retuned down to the 0.6% shown here specifically to
  avoid that (§7).
- **Supply-interaction check (recruit-max, floor 3, the scenario already
  closest to the degrade line in `scaling-curve.md` §2):** base demand 90
  against capacity 85 (degrade multiplier 0.944) becomes demand ~98 against
  the same capacity (multiplier **0.867**) once the expected orb total is
  added. Balanced floor 3 (demand 60, capacity 110) absorbs the same ~8 extra
  units with no degrade triggered (demand 68 < capacity 110). Both results
  are stated in §7 as the intended, honest behaviour of the governor, not a
  problem to fix.
- **Healing magnitude, Fermi sanity only (not a prediction):** if every
  Kindling Ember collected on a floor went to the hero alone, that floor's
  total would represent roughly 20% / 50% / 93% of the hero's MaxHP across
  floors 1/2/3. This is a **bound**, not a realistic outcome — in practice
  the burst is shared with nearby retinue, drops arrive spread across the
  whole fight rather than banked, and healing offsets ongoing damage rather
  than accumulating toward a full top-off. It's reported only to confirm the
  magnitude is in a sane band (not a trickle, not a full-heal-on-demand
  button), the same "does the order of magnitude make sense" check
  `entity-tiers.md` §7 and `feeding-distraction.md` §14 both use their Fermi
  models for.
- **Rally Ember uptime, contextually.** Floor 1 collects zero Rally Embers in
  **68.6%** of runs (again, appropriately rare for a 38-Soldier floor).
  Floors 2 and 3 collect enough (2.78, 6.31 mean) that, spread across a
  15s-per-stack refreshing duration, the buff should be up for a meaningful
  share of each floor's higher-stakes fights, concentrated exactly where the
  guaranteed Elite/Boss drops land it (§6) — not simulated as a continuous
  uptime percentage (that needs a real per-encounter timeline, which this
  Fermi pass doesn't have), but the collected-count order of magnitude
  supports the intended read of "a spike tool that shows up when it matters,"
  not "always on" or "never seen."

**What this doc does NOT claim, stated plainly:**
- It does not close, or materially narrow, `scaling-curve.md` §3's flagged
  population/army ratio gap (§4 above states this directly). ~8 extra bodies
  over a whole run is a rounding error against a gap that doc characterizes
  as needing roughly 3× more bodies on floor 1 alone.
- The 0.92 pickup rate is an assumption, not a measurement — flagged in
  `loot-v0.json`'s `design_constants.assumed_pickup_rate_note` for whoever
  next has a running sim to check it against.
- The Supply-interaction numbers are computed against `scaling-curve.md`
  §2's already-published scenario tables, not re-simulated from scratch —
  if those tables are re-tuned, this doc's §7/§8 interaction numbers should
  be recomputed, not assumed to still hold.
- Rally Ember's uptime is a Fermi count, not a simulated timeline (see the
  bullet above) — a real per-encounter measurement, once a sim exists, could
  show the buff landing at less useful moments than "guaranteed on
  Elite/Boss" implies if a pickup's 15s window regularly expires between
  encounters rather than during them.

---

## 9. Handoffs

**To whoever builds this in Mass/`USwarmSubsystem`.** Reuse
`feeding-distraction.md`'s corpse-record pattern (§5.1 of that spec) for the
drop record shape, and its claim-pass structure (§5.2) for pickup resolution
— both are directly reusable, not just analogous. `loot-v0.json`'s
`design_constants.resolution_pattern_note` restates the pointer. Unit Orb's
spawn call should go through the same entry point growth-site Recruit uses
(`SwarmSpawn.h`), not a new path. Rally Ember's buff should be implemented as
a single global DPS multiplier read at the same point the Supply degrade
multiplier is already applied — not a second, independent multiplier chain.

**To whoever next touches `RTS-VERTICAL-SLICE.md` §4.** This doc closes the
"unit orbs + healing" two-thirds of that checklist line, plus the owner's
follow-up ask for a buffs category; the "4-6 stacking items" fourth
component was already closed by `SYSTEMS.md` §3. The line as a whole should
be checkable once this doc lands — flagged rather than edited directly,
since this doc's write scope doesn't include that file.

**To whoever revises `scaling-curve.md` §3's headcount-gap finding.** §4/§8
above are the honest read of what Unit Orb can and can't contribute to that
gap — it's real, positive, and far too small to be the fix. If that gap gets
closed by a different lever (population cut, economy retune, or the
quality-over-quantity reading `scaling-curve.md` §3 itself proposes), this
doc's numbers don't need to change; they were never sized against that gap
in the first place.

**To the sim-harness effort (`Scripts/sim/`, in progress elsewhere in this
session).** This doc's Monte Carlo is a scratch, throwaway script per house
convention, but its inputs are fully stated (§8) — `drop_sources[]` chances,
`scaling-curve.json` floor counts, the 0.92 pickup-rate assumption — so it
should be directly reproducible in that harness once available, without
needing anything from this session. Rally Ember's uptime-vs-encounter-timing
question (§8) is a good first candidate for that harness once it exists,
since it needs exactly the per-encounter timeline this doc's Fermi model
doesn't have.

---

## 10. Narrative requests

Per the handoff convention — all three drop types in `loot-v0.json` are
`WorkingNameOnly: true`, since current canon (`FLAME-FOUNDATION.md`) names no
factions or biomes yet.

- **Unit Orb ("Freed Orb")** — mechanically: a battle drop from Soldier-tier
  kills and up that instantly adds a Freed unit to the retinue on pickup.
  What a player must feel: this should read as *a captive breaking free in
  the middle of the fight*, not as loot dropping from a corpse — the
  Vanguard's whole retinue identity is "your army is what you save"
  (`CLASSES.md` §1), and a Soldier-tier kill freeing someone who was being
  held near it is the version of this drop that's on-theme rather than a
  generic gem pickup. Whether that's literally "a chained conscript nearby
  who joins you" (ties to a specific kill) or something more abstract is a
  narrative call, not a mechanical one — flagged here, not resolved.
- **Kindling Ember** — mechanically: a common battle drop from any kill that
  heals the hero and nearby retinue in a burst on pickup. What a player must
  feel: ties directly to Design Law 8 ("light is a resource") — this should
  read as *scavenged flame*, embers that scattered from the fight and can be
  gathered back into the bearer's own fire, not a medical pickup or a potion.
  The name is chosen to lean into that already; narrative should confirm or
  replace it with the real vocabulary once one exists.
- **Rally Ember** — mechanically: a rarer battle drop from Soldier-tier kills
  and up (guaranteed on Elite/Boss) that grants the hero and whole retinue a
  temporary, stacking DPS buff. What a player must feel: a battlefield
  fervor spike — *the line hears the fight is turning and pushes harder* —
  deliberately **not** a light/flame-flaring effect, since "the light
  sustains" is the Lampbearer's reserved design axis (`CLASSES.md` §4) and
  this is Vanguard-slice content; lean on the Vanguard's own martial
  vocabulary (rally cries, banner-fervor, "second wind") instead. Should read
  as urgent and earned, especially in its guaranteed Elite/Boss appearances —
  the moment the fight visibly gets harder is also the moment the line gets
  a boost, and that pairing should feel intentional on screen, not
  coincidental.
- **Gameplay readability constraint for all three, to carry into any art
  brief:** each needs a small, legible pickup silhouette that reads at horde
  scale without competing visually with corpses (`feeding-distraction.md`'s
  sealed/eaten bodies) or the flame/leash rendering already on screen —
  ideally one bright, reserved-palette-value glint per type (mirrors the
  Lampbearer's stated "reserved palette value" convention for marks/glow in
  `CLASSES.md`, even though this is Vanguard-slice content), distinct enough
  between the three that a player scanning a chaotic fight can tell "unit"
  from "heal" from "buff" from "corpse" from "enemy" at a glance (Design Law
  6). Rally Ember additionally needs a clear on-hero/on-retinue "buffed"
  read (a distinct tint or particle while active) so its 15s window is
  legible while it's running, not just at the moment of pickup.
