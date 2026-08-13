# Gameplay Systems — Decision Record

**Version:** 0.3 (Embers retired; the persistent army is the meta-progression) · Companion
docs: `GDD.md`, `CLASSES.md`, `docs/narrative/FLAME-FOUNDATION.md`
**Owner:** gameplay-director agent (this is the one canon file it edits directly)
**Last updated:** 2026-07-31

> **`WORLD.md` is SUPERSEDED** (total narrative reset 2026-07-22) and is no longer a
> companion doc of this file. Live narrative canon is `docs/narrative/FLAME-FOUNDATION.md`.

This file is the source of truth for gameplay-system decisions: scaling curves, loot
rules, entity tiers, encounter budgets, pacing. It records **decisions and rationale**;
tuned numbers live in `docs/data/`. Nothing here overrides `GDD.md` — conflicts are
canon proposals back to the user.

**Data files (tuned numbers, all PROTOTYPE DIALS pending play-tuning):**
> **The three files below are not yet marked on disk.** The 2026-07-31 decision retires
> parts of each, but the JSON carries no retirement header yet — so the files still read as
> live canon. Treat every retirement here as **PENDING** and check
> `docs/design/DIRECTION-2026-07-31.md` before trusting anything in them.

- `docs/data/economy.json` — Supply model, degrade formula, scaling curve. Its `embers`
  block is **retired by the 2026-07-31 decision (§7) — file not yet marked**; Supply and the
  degrade formula are untouched.
- `docs/data/upgrades.json` — unit-tier ladder, run-items, hero ability nodes. The ladder
  and the 6-item Loot v0 catalog survive; every Ember price and the growth-site purchase
  route are **retired by the 2026-07-31 decision (§7–8) — file not yet marked**.
- `docs/data/growth-sites.json` — **retired in full by the 2026-07-31 decision (§7) — file
  not yet marked**. The growth-site triangle no longer exists. Kept in the tree as history,
  not as a live dial.

---

## 1. Entity tiers

Working taxonomy (`docs/design/entity-tiers.md`): fodder → soldier → elite → titan → boss. Fodder/soldier
are Mass Entity; elite/titan/boss are promoted Actors. *Full stat blocks for the
elite/titan/boss tiers are still open.*

**DECIDED 2026-07-24 — the Vanguard retinue ladder is real (slice).** The Liberated
tier ladder from `CLASSES.md` §1 is now the concrete "soldier" tier for the vertical
slice, with numbers in `docs/data/upgrades.json`:

- **Freed** (90/20) — improvised weapons, routs under pressure. *What a fresh recruit
  enters as.*
- **Militia** (130/30) — the **balance anchor**; this is the gate-1 prototype unit.
- **Veteran** (190/45) — holds formation under fear, persists across waves.
- **Bannerman** (rare, aura) — **slice stretch**, reward-only, not a purchasable tier.

Rationale: recruits entering at **Freed** (weak) rather than Militia makes the
*promotion* economy meaningful and dovetails with GDD §7 ("newly recruited units enter
degraded") — a fresh body is both weak-by-tier and degraded-if-unfed until you invest.

## 2. Scaling & difficulty

Constraints locked by GDD §7: exponential-feeling layered multipliers, soft caps only.
*(A third constraint, "co-op scales by density not HP", is **[MP — deferred 2026-07-27]**
and no longer binds anything — GDD §7 carries no co-op term.)*

**DECIDED 2026-07-24 — the slice's first-act curve.** One curve across 3 waves:
**250 → 450 → 700 brood** (×1.8, ×1.56) — the gate-1 values, now canon as "the first
act of the exponential fantasy" (`docs/data/economy.json`). The player's *economic
throughput* (bodies + tier + supply headroom) is what must keep pace with brood
density — that is where the exponential feel comes from in the slice, before items and
hero ults compound it.

**The soft cap is Supply, not a number cap** — see §7.

**OPEN CONFLICT (flagged 2026-07-31, C13) — two live wave curves, neither retired.** The
250/450/700 curve in §2 above is a **dated decision record and stays as history whatever
wins**.
It now sits alongside `docs/data/wave-scaling.json` (task-102, dated 2026-07-30), which
proposes **120 → 400 → 20,000 brood with retinue 60 → 120 → 600** and whose `$schema_note`
claims to supersede §2's curve outright. **Nothing has adjudicated that claim.**
`encounter-budget.json`, `scaling-curve.json` and `retinue-vanguard.json` all derive from
the OLD curve, so adopting the new one re-derives three files, while keeping the old one
voids a day of task-102's work. **Deliberately NOT picked here — see O2 in the open
questions.** Whoever picks it owns the three derived files in the same change.

## 3. Loot

GDD §8 direction (run-scoped, VS-style stacking, drops feed both hero and retinue).
The real loot/evolution system stays **deferred by design**.

**DECIDED 2026-07-24 — Loot v0 (slice only).** A catalog of **6 stacking run-items**
(`docs/data/upgrades.json`), explicitly the sanctioned "Loot v0" from
`RTS-VERTICAL-SLICE.md` §4 — *not* the real loot system. Each item feeds **hero, retinue,
or the economy** (the GDD §8 twist that loot serves the army): e.g. *Whetstone* (+DPS to
the whole retinue, stacks), *Iron Rations* (+Supply capacity).

**AMENDED 2026-07-31 — the catalog survives, its storefront moves.** With the growth-site
triangle retired (§4/§7), *"offered 3, take 1 at a growth site, so acquiring one costs you
a triangle turn"* is **RETIRED**. The same 6 items are now **bought with gold at shops**
(§7) and the **stash of them persists between runs**. This catalog is the shops' starting
stock — the full itemisation system (task-034) stays deferred by design; do not build it
to fill the shelves. One casualty: *Oath-Ledger* paid out in **Embers**, a currency that no
longer exists (§7), so its payout needs re-denominating in gold before it ships —
**not re-priced here.**

## 4. Encounter budgets

GDD §9 constraint, live: arenas are **hand-authored** and sized for horde fights.

**RETIRED 2026-07-31 (D8) — the per-stage baseline and the generator behind it.** *"Every
floor has ≥1 arena, ≥1 decision event, ≥1 optional risk room"* is retired with procedural
generation (`GDD.md` §9). Kept here as the dated record of what was constrained, not as a
live rule. A run is a sequence of authored arena **stages** — "floor" is a retired noun,
renamed to *stage* throughout `GDD.md` on the same date.

**RETIRED 2026-07-31 (D4) — the growth site is DELETED.** Kills auto-level the army (§7),
so there is no Ember pool to allocate and no camp panel to allocate it in. The 2026-07-24
decision below is kept **as the dated record of what was tried** — it is no longer live.
What survives is the *pillar*, not the panel: meaningful decisions move to the shops (§7).
The tuning passes that made this allocation non-inert (**task-097**, **task-101**) are
**superseded, not deleted from history** — correct work against a model that stopped
shipping.

**DECIDED 2026-07-24 — the growth site is where continuous decisions live.** Between
encounters (the slice's breathers), the player reaches a **growth site**: a camp panel
where they allocate a scarce Ember pool across the breadth/depth/sustain triangle plus
items/hero nodes (`docs/data/growth-sites.json`). This **replaces the gate-1 flat
refill-to-120**, which was a non-decision.

Key relationship: the growth site is the **continuous grain of the same axis** as the
GDD §6 authored fork events. "Free the pens vs. raid the armory" is breadth-vs-depth as a
one-shot story beat; the growth site is the player-driven version of that choice every
breather. **The economy and the "meaningful decisions" pillar are one system, at two
grains.** The slice's 2 authored decision events sit *on top of* the growth sites, not
instead of them.

*Full encounter/density budget tables per wave remain to be tuned in play.*

**Open dependency (flagged 2026-07-26, see §6).** Hold's value as a chokepoint tool
depends on arenas/corridors being narrow enough that `Swarm.BroodAggroRange` (~600uu)
reliably catches everything passing through — not yet a stated arena-**authoring**
constraint (it was a procgen constraint until generation retired, `GDD.md` §9, 2026-07-31).
Until it is, Hold reads as a wall only in authored pinch points, by accident of geometry
rather than by design guarantee.

## 5. Pacing director

*Not yet designed.* Intent: L4D-style intensity manager (spike → breather → spike)
reading **run state (army level, stage index, retinue health)**. *It read "party state and
world flags" until 2026-07-31; both inputs are dead — world flags were discarded with the
narrative reset (2026-07-22, `GDD.md` §6a) and the party is **[MP — deferred 2026-07-27]**.*
For the slice, pacing is **hand-authored** (the
fixed 3-wave rhythm) — no director AI yet (RTS-VERTICAL-SLICE §6 explicitly fakes this).
The breather was the growth-site decision beat until that retired 2026-07-31 (§4); the
breather is now a **shop stop** (§7).

## 6. Retinue tuning

**DECIDED 2026-07-24 — the economy is the central retinue dial (see §7).** Attrition is
handled by the gate-1 combat model (continuous melee attrition, `MaxAttackersPerUnit`
cap); *replenishment* is bought, not free-refilled. The **Recruit action at growth sites**
is **RETIRED 2026-07-31** with the triangle (§4) — recruits are now a **merchant good
bought with gold** (§7). Per-class attrition/replenishment rates remain the key balance
dial; identity-level changes still belong to `CLASSES.md` (canon proposals only).

**DECIDED 2026-07-26 — Hold is a positioning tool, not a barricade.** `CLASSES.md` §1
(Vanguard, Hold → Shield Wall) promised a line that "blocks enemy pathing entirely." The
sim doesn't do that, and at horde scale it shouldn't: brood pick a target with a
stance-agnostic aggro radius (`Swarm.BroodAggroRange`, ~600uu, itself capped by the
neighbour-query cell size — see the `cvars` skill / `docs/GATE1-FUN-PROTOTYPE.md`) and
otherwise beeline for the attractor. Hold does not raise that radius. What Hold actually
does: it pins the retinue's formation anchor to the point it was issued at instead of
tracking the hero, so the line stays exactly where you put it while the hero moves on to
fight elsewhere (within the leash — `GDD.md` §4; the leash rule itself is unchanged). That
is real and useful — a chosen point on the map that keeps fighting from itself — but it is
not an obstacle. **A held line only reads as a wall where the level geometry already makes
it one**: a doorway or corridor mouth narrow enough that anything passing through comes
within aggro range regardless. Planted in open ground, the tide finds the gaps around it.

This closes GDD §4's "Leash vs. Hold-wall (OPEN)" item in the direction of the sim: the
canon doc moves, not the sim (canon proposal filed 2026-07-26, `CLASSES.md` §1). A literal
blocking wall would need per-unit pathing obstruction — the kind of individual special
case GDD §10's Mass Entity constraints rule out at horde scale — and cuts against the
"soft costs over hard numeric caps" rule in GDD §7. **Open follow-up, not yet answered:**
whether an authored arena (§4 above) can be trusted to contain Vanguard-relevant
chokepoints narrow enough for Hold to matter, or whether that needs to become an explicit
arena-**authoring** constraint. *(A procgen question until generation retired 2026-07-31;
`GDD.md` §4 carries the same fork in the same words.)* Untested — flag before any content leans on Shield Wall being reliable.
Relickeeper's Bulwark (`CLASSES.md` §2, "sentinels interlock into literal wall segments")
makes the identical claim and inherits the identical fork; not resolved here since the
Relickeeper isn't built in the sim yet, but it will hit the same wall (so to speak) the
day it is.

**DECIDED 2026-07-31 — the player commands by unit TYPE, not by group.** Orders and
stances (Hold included) are addressed to *all archers*, *all spearmen*, *all healers* —
never to a numbered squad. **This SUPERSEDES the MaxSquads = 8 group model**
(`docs/design/squad-group-system.md:376-390`, `SwarmSubsystem.h:46-54`), which measurably
breaks at ~730 retinue by folding fresh recruits into unit 0. Command-handle count now
scales with the number of unit **types**, so it cannot fold however large the army gets —
the only version that survives an army that persists and only grows (§7). Hold itself is
unchanged in *what it does*; only its addressee changes, so a held archer line and a
mobile spear line are now separately orderable.

## 7. Economy — Supply + three currencies (**DESIGNED 2026-07-24 · RESHAPED 2026-07-31**)

The economy implements GDD §7 (upkeep) and carries the "meaningful decisions" pillar (§3).
**Embers are DELETED (2026-07-31, D5).** Three currencies remain, one per timescale:

| Currency | Earned by | Buys | Lifetime |
|---|---|---|---|
| **Kills** | killing anything | **army level** — automatic, no allocation panel, no player spend | **PERSISTS** between runs; monotonic, never falls |
| **Gold** | drops | items and merchant goods, at shops (below) | spent in-run; the **item stash persists** |
| **Fragments** | gathered through a mission | enough of them in one place convert into a **healer unit** | in-run |

Boss fights drop **special fragments that act as MODIFIERS** rather than converting into
bodies. **How fragments are gathered is OPEN** — kill drops with auto-vacuum, placed
caches, both, or rescue-only. The owner did not answer; see **O1**. Do not invent it.

Supply is unchanged and still governs size:

### Supply — the size governor (implements GDD §7)
Supply is **capacity (headroom), not a draining stock**. Upkeep **demand** = the sum of
per-unit upkeep (uniform **1/unit** in the slice). When demand > capacity, units
**degrade — dimmed, weaker (DPS × `capacity/demand`, floored at 0.4), can't hold
formation — but never die.** Fresh recruits enter degraded until capacity recovers.
- **Why capacity/ratio and not a draining clock:** it makes "degrade not die" fall out
  for free (the degrade multiplier *is* the headroom ratio), needs no per-tick
  bookkeeping, and keeps the levers crisp — recruiting raises demand, buying capacity
  raises it back, promoting is upkeep-neutral.
- **Why uniform upkeep:** it makes **Promote** the upkeep-efficient path (more power, no
  new mouths) and **Recruit** the upkeep-hungry path (cheap bodies that strain supply).
  Per-tier upkeep is a parked v2 knob.

**DECIDED 2026-07-31 (C7) — Supply capacity is the MERCHANT'S HEADLINE GOOD, bought with
gold.** The size governor above is kept in full; only its purchase route changes, from
Provision-for-Embers at a growth site to a shop purchase. That makes **the shops the thing
that unlocks scale** — the correct incremental shape — and it costs **zero re-tuning**: the
degrade formula (DPS × clamp(capacity/demand, 0.4, 1.0)), the uniform 1/unit demand and the
120 starting capacity are all untouched.

### Embers — **RETIRED 2026-07-31 (D5)**
The record, kept: Embers were the spend currency, earned from **brood kills** (0.1 each) +
a **grant on reaching a growth site** (10), tuned so a player arrived at each site with
~30–40 — enough for **2–3 actions, never all** (`docs/data/economy.json` `embers`).
Scarcity was the decision. **Deleted.** Kills now pay the army directly and automatically
(below), gold pays the shops, and no currency is hand-allocated anywhere. The `embers`
block in `economy.json` is retired data, not a live dial.

### The triangle — breadth / depth / sustain — **RETIRED 2026-07-31 (D4)**
The growth-site allocation panel is **deleted with `docs/data/growth-sites.json`**. The
dated record of what it was:

| Lane | Action | Effect | Cost / tension |
|---|---|---|---|
| **Breadth** | Recruit | +10 Freed | +10 upkeep demand → toward the degrade line |
| **Depth** | Promote | up to 20 units +1 tier | upkeep-neutral, but headcount stays thin |
| **Sustain** | Provision | +25 Supply capacity | safe, but a turn spent *not* getting stronger |
| *(spice)* | Item | 1 of 3 offered | 20 Embers — costs you most of a triangle turn |
| *(hero)* | Hero node | 1 Vanguard upgrade | competes with the army for the same Embers |

The rationale (lanes that genuinely oppose each other; hero nodes competing for the same
currency as the army, expressing the GDD §4 hero-vs-army tension as a spend choice) was
sound and is superseded rather than disowned — as are **task-097** and **task-101**, the
tuning passes that made the allocation non-inert. Not every lane died: **recruiting and
Supply capacity are now merchant goods bought with gold**; ~~**promotion's home is open**~~
— **CLOSED 2026-08-08: promotion's home is Adaptation**, see below;
**hero nodes' route is open** (§8).

### Kills → army level — the automatic ratchet (**DECIDED 2026-07-31, D4**)
Kills auto-level the army. **Full auto: no allocation panel, no player spend on army
power, ever.** Army level = f(lifetime kills), monotonic — it never falls, not even on a
wipe. The kill economy **already ships in C++** (per-squad and per-hero attribution,
`SwarmSubsystem.h:159-186`), so this is a payout rule on plumbing that exists, not a new
system.

### The shops — gold, deliberately NOT competing with army growth
Shops are **separate venues on their own currency** (gold from drops). Because army power
is never for sale, a shop visit can never be the wrong choice *against* levelling — that
separation is the whole point. Venues: the **merchant** (headline good: Supply capacity —
C7 above; plus recruits), a **secret shop**, and further types later. **The item stash
persists between runs.** This overturns Loot v0's no-second-economy rule, owner-accepted.
Stock it from **Loot v0's existing 6 stacking run-items (§3) plus the Supply/recruit
goods** — do **not** build the full task-034 itemisation system to fill the shelves.
Gold's drop rate and sources are **OPEN** (**O4**).

### Design intent / falsification test
Gate-1's zero-input baseline **loses wave 3 by 4–13 brood**. The economy layer must be
the margin: **played well it turns that narrow loss into a win; over-recruited past supply
(a degraded, dimmed army) it becomes a blowout loss.** The old form of the test — *"if any
single allocation always wins, the triangle has failed"* — **retires with the triangle
(2026-07-31)**. Its replacement: **if the shops are never worth a stop, or are always the
only right stop, the split between gold and army level has failed** and prices need
re-tuning. This is the acceptance test when the layer is wired into the prototype.

### Meta-progression — the persistent army (**DECIDED 2026-07-31, D3**)
This slot was left **deliberately unwritten** until 2026-07-31. It is now closed:
**the persistent army *is* the meta-progression. There is no separate meta-currency.**

- **Army level = f(lifetime kills)** — monotonic, automatic, never falls.
- **The army carries between runs and only ever grows.** A wipe costs **stage progress
  only**: army, gold, items and stash all survive.
- **No meta-currency, no meta-shop, no allocation screen.** Anything that would need one is
  the wrong shape — the ratchet *is* the kill counter.

**SUPERSEDED 2026-07-31 — "upgrades are army-shaping, not stat-creep" (recorded
2026-07-24):** *"Every upgrade (tier promotion, items, hero nodes) changes how the army
behaves and reads — formation discipline, auras, a wider Shield Rush — not just a damage
number, and all of it is run-scoped (reset each run). This keeps the hard GDD §3 line: 'the
reward for a run isn't +5% damage.' The reward is a bigger, better, or better-fed army."*
The **army-shaping half stands** and is still the design bar. The **run-scoped half does
not**: army level, gold and the item stash persist. The GDD §3 line it leaned on ("player
power fully resets each run") is superseded by the same owner decision — flagged here,
moved in `GDD.md`, never silently rewritten.

### Adaptation — promotion's home, closed (**DECIDED 2026-08-08**)

Spec: `docs/design/adaptation.md`. Data: `docs/data/unit-types.json` `adaptation`.

The triangle's retirement left promotion homeless (above). **Adaptation is where it lands.**
The split that keeps it from colliding with D3/D4: **army *size* stays automatic on kills;
Adaptation is army *shape*.**

- **Every character template has an evolution ladder.** A rung is the triple
  `(unit_type, tier, variant_index)`. `tier` keys **§1's existing four-tier ladder**
  (`upgrades.json` `tier_ladder`) — no second stat ladder is invented. Rank is array order.
- **The player picks from a branch, and Adaptations are also shop stock.** Both. The stock
  rule needs no number: the shop offers rungs on branches the player's pick did not grant.
  **Price is OPEN (O8), blocked on O4.**
- **The top rung is a captain** fielding its own retinue of ≤ 8. `bannerman` is reused as
  that rung — its aura trait and its `rare` flag already fit, and its existing *"item/event
  reward only, not a purchasable tier"* line makes the captain the one rung the shop cannot
  sell. Captains are earned; everything below them is buyable.
- **One command handle per branch, not per rung** (a branch is a type under §6's D14; a rung
  is a look-and-stat move inside a type). A captain plus its retinue is one handle.
- **Friendly side only for v1.** The rung triple is unchanged for enemies — swap `tier` to
  `entity-tiers.json` — so the enemy pass needs no schema change.

**The C++ cannot express this yet, and that is filed, not hidden.** `EUnitType` is one bit,
all eight command handles are consumed at the shipped retinue cap, D14 itself is unimplemented,
and a unit's look is recomputed from spawn phase every frame with nowhere to store an assigned
variant. `adaptation.md` §6 carries the list with line numbers. The spec is authored so it
adds **zero atlas repack debt** in the meantime.

**Amends 2026-07-31's "no player spend on army power, ever"** (the kill-ratchet subsection
above, and `GDD.md` Q22): that stands for army **size** and is amended for army **shape** — an
Adaptation is buyable. The original sentence is not deleted.

## 8. Hero progression (slice)

**DECIDED 2026-07-24 — the node catalog.** The Vanguard's three abilities (Banner Slam,
Shield Rush, The Muster) each get a one-of-two upgrade node (`docs/data/upgrades.json`).
The catalog is unchanged and still ships.

**RETIRED 2026-07-31 — how they were bought.** *"…bought with Embers at growth sites.
Run-scoped, and in direct currency competition with retinue growth — hero power is a
choice against army power, never a free parallel track."* All three props are gone: Embers
are deleted (D5), growth sites are deleted (D4), and army power is no longer bought at all
(it levels off kills), so there is no shared currency left to compete for. **OPEN, not
answered here:** where hero nodes are earned instead — gold at a shop, or a hero track off
the same kill ratchet. The owner has not been asked; do not invent it. The GDD §4
hero-vs-army *relevance* tension is unaffected — it simply stops being expressed as a
spend choice.

---

## Open questions (raised 2026-07-31 — do not answer by inference)

| # | Question | Why it is open |
|---|---|---|
| **O1** | How **Light fragments** are gathered — kill drops with auto-vacuum, placed caches, both, or rescue-only | The owner specified the conversion (enough fragments in one place → a healer unit) and the boss-fragment modifiers, but never the gather method. §7 |
| **O2** | **Which wave curve wins** — §2's 250/450/700 or `wave-scaling.json`'s 120/400/20,000 | Both live, both dated; picking one re-derives `encounter-budget.json`, `scaling-curve.json` and `retinue-vanguard.json`. Deliberately not picked here (C13). §2 |
| **O4** | **Gold's drop rate and sources** | The shops are decided (§7); what fills the purse is not |
| — | Where **hero ability nodes** are earned now that Embers and growth sites are gone | §8. The catalog stands; the purchase route does not. Owner not yet asked |
| **O6** | **Rung count per Adaptation ladder** | Default is §1's existing four tiers. A fifth rung means inventing an HP/DPS row — the inference this project bans. Owner not asked. §7 |
| **O7** | **How many captains a run supports**, and whether captain retinue draws Supply upkeep | The ≤ 8 retinue cap is a legibility call (half `TypeLegibilityCeiling`); neither the count nor the upkeep was decided. `upkeep_per_retinue_body` is an explicit `null`. §7 |
| **O8** | **Adaptation shop price** | Blocked on **O4** — nothing can be priced before gold's rate and sources exist. §7 |

*(O3 — end-of-run headcount — is deliberately deferred to task-108's measurement, not a
design question. O5 — whether the leash survives an army in the thousands commanded by
type — is logged against `GDD.md` §4, not here.)*

## Decision log

| Date | Decision | Rationale | Spec / data |
|---|---|---|---|
| 2026-07-24 | Economy = **Supply (size governor) + Embers (spend currency)** — **Embers SUPERSEDED 2026-07-31 (D5); Supply survives unchanged** | Implements GDD §7 upkeep as capacity/headroom; two dials give clean breadth-vs-sustain tension | §7 · `economy.json` |
| 2026-07-24 | Supply is **capacity ratio**, degrade = `capacity/demand` | "Degrade not die" falls out for free; no per-tick bookkeeping; crisp triangle | §7 · `economy.json` |
| 2026-07-24 | **Uniform upkeep 1/unit** (slice) | Makes Promote upkeep-efficient and Recruit upkeep-hungry — the triangle's core | §7 · `economy.json` |
| 2026-07-24 | Meaningful decisions = **growth-site triangle** (recruit/promote/provision + item/hero) — **SUPERSEDED 2026-07-31 (D4): triangle deleted, decisions move to the shops** | Continuous grain of the same axis as §6 fork events; unifies economy + decisions pillar | §4/§7 · `growth-sites.json` (RETIRED) |
| 2026-07-24 | Recruits enter at **Freed** (weak tier) | Makes the promotion economy matter; dovetails with GDD §7 "fresh units enter degraded" | §1 · `upgrades.json` |
| 2026-07-24 | **Loot v0 = 6 stacking items**, offer-3-take-1 — **AMENDED 2026-07-31: same 6 items, bought with gold at shops, stash persists; offer-3-take-1 retired with the growth site** | Sanctioned slice loot (RTS-VERTICAL-SLICE §4); feeds army not just hero (GDD §8 twist); not the real loot system | §3/§7 · `upgrades.json` |
| 2026-07-24 | Hero nodes **spend the same Embers** as the retinue — **SUPERSEDED 2026-07-31 (D5): no shared currency exists; how nodes are earned is OPEN** | Expresses GDD §4 hero-vs-army relevance as a spend choice | §8 · `upgrades.json` |
| 2026-07-24 | Slice curve locked at **250/450/700 brood** — **STANDS AS THE DATED RECORD; in OPEN CONFLICT from 2026-07-31 (C13) with `wave-scaling.json`'s 120/400/20,000. Not adjudicated.** | Gate-1 values adopted as the first act of the exponential fantasy | §2 · `economy.json` · O2 |
| 2026-07-26 | **Hold/Shield Wall is porous by design** — a positioning tool, not a barricade; doc moves to match the sim, not the reverse | Matches GDD §7 "soft costs over hard caps" and §10 Mass Entity constraints; a literal blocking wall needs per-unit pathing obstruction, which horde scale can't afford | §6 · `CLASSES.md` §1 (canon proposal pending) · closes GDD §4 "Leash vs. Hold-wall (OPEN)" |
| 2026-07-31 | **Embers DELETED** (D5). Three currencies remain, one per timescale: kills → army level (auto, **persistent**), gold → items (shops, **stash persists**), fragments → healer units (in-run) | One currency per timescale, none hand-allocated; drops the meta-currency the incremental shape does not need | §7 · `economy.json` `embers` RETIRED |
| 2026-07-31 | **Growth-site triangle DELETED** (D4) — kills auto-level the army, no allocation panel, no player spend on army power | The kill economy already ships in C++ (`SwarmSubsystem.h:159-186`); an automatic ratchet needs no panel. task-097 / task-101 tuning **superseded, not deleted from history** | §4/§7 · `growth-sites.json` RETIRED |
| 2026-07-31 | **The persistent army IS the meta-progression** (D3) — army level = f(lifetime kills), monotonic, never falls; no separate meta-currency | Closes the slot §7 left deliberately unwritten; supersedes the run-scoped-power line (GDD §3) | §7 |
| 2026-07-31 | **Command by unit TYPE, not by group** (D14) — supersedes the MaxSquads = 8 group model | Handle count scales with type count so it never folds; the group model measurably breaks at ~730 retinue by folding recruits into unit 0 | §6 · `squad-group-system.md:376-390` · `SwarmSubsystem.h:46-54` |
| 2026-07-31 | **Supply capacity = the merchant's headline good, bought with gold** (C7) | Keeps the size governor and the degrade formula untouched (zero re-tuning) while making the shops the thing that unlocks scale — the correct incremental shape | §7 · `economy.json` |
| 2026-07-31 | **OPEN CONFLICT logged, not resolved: two live wave curves** (C13) | 250/450/700 (§2) vs. `wave-scaling.json`'s 120/400/20,000; three derived files hang off the old curve. Deliberately NOT picked | §2 · O2 |
| 2026-07-31 | `WORLD.md` dropped as a companion doc (C5) | Superseded by the 2026-07-22 narrative reset; live narrative canon is `docs/narrative/FLAME-FOUNDATION.md` | header |
| 2026-08-08 | **Adaptation** — every template gets an evolution ladder; player picks a branch and the shop stocks the rest; top rung is a captain with a ≤ 8 retinue; one handle per branch | Fills the promotion slot D4 vacated without touching the kill ratchet: army **size** stays automatic, Adaptation is army **shape**. Reuses §1's four-tier ladder as the stat spine and existing atlas rows as the looks, so it adds zero repack debt | §7 · `docs/design/adaptation.md` · `unit-types.json` `adaptation` |
