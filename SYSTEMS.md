# Gameplay Systems — Decision Record

**Version:** 0.2 (economy + upgrade layer designed) · Companion docs: `GDD.md`, `CLASSES.md`, `WORLD.md`
**Owner:** gameplay-director agent (this is the one canon file it edits directly)
**Last updated:** 2026-07-24

This file is the source of truth for gameplay-system decisions: scaling curves, loot
rules, entity tiers, encounter budgets, pacing. It records **decisions and rationale**;
tuned numbers live in `docs/data/`. Nothing here overrides `GDD.md` — conflicts are
canon proposals back to the user.

**Data files (tuned numbers, all PROTOTYPE DIALS pending play-tuning):**
- `docs/data/economy.json` — Supply model, Ember income, degrade formula, scaling curve
- `docs/data/upgrades.json` — unit-tier ladder, run-items, hero ability nodes
- `docs/data/growth-sites.json` — the growth site, its action costs, slice placement

---

## 1. Entity tiers

Working taxonomy (GDD §10): fodder → soldier → elite → titan → boss. Fodder/soldier
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

Constraints locked by GDD §7: exponential-feeling layered multipliers, soft caps only,
co-op scales by density not HP.

**DECIDED 2026-07-24 — the slice's first-act curve.** One curve across 3 waves:
**250 → 450 → 700 brood** (×1.8, ×1.56) — the gate-1 values, now canon as "the first
act of the exponential fantasy" (`docs/data/economy.json`). The player's *economic
throughput* (bodies + tier + supply headroom) is what must keep pace with brood
density — that is where the exponential feel comes from in the slice, before items and
hero ults compound it.

**The soft cap is Supply, not a number cap** — see §7.

## 3. Loot

GDD §8 direction (run-scoped, VS-style stacking, drops feed both hero and retinue).
The real loot/evolution system stays **deferred by design**.

**DECIDED 2026-07-24 — Loot v0 (slice only).** A catalog of **6 stacking run-items**
(`docs/data/upgrades.json`), explicitly the sanctioned "Loot v0" from
`RTS-VERTICAL-SLICE.md` §4 — *not* the real loot system. Each item feeds **hero, retinue,
or the economy** (the GDD §8 twist that loot serves the army): e.g. *Whetstone* (+DPS to
the whole retinue, stacks), *Iron Rations* (+Supply capacity), *Oath-Ledger* (Veterans
that survive yield Embers — an economy-engine item). Items are **offered 3, take 1** at a
growth site (§4), so acquiring one costs you a triangle turn.

## 4. Encounter budgets & procgen rules

GDD §9 constraints: every floor has ≥1 arena, ≥1 decision event, ≥1 optional risk room;
arenas sized for horde fights.

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
reliably catches everything passing through — not yet a stated procgen constraint.
Until it is, Hold reads as a wall only in authored pinch points, by accident of geometry
rather than by design guarantee.

## 5. Pacing director

*Not yet designed.* Intent: L4D-style intensity manager (spike → breather → spike)
reading party state and world flags. For the slice, pacing is **hand-authored** (the
fixed 3-wave rhythm), and the breather is now the growth-site decision beat — no director
AI yet (RTS-VERTICAL-SLICE §6 explicitly fakes this).

## 6. Retinue tuning

**DECIDED 2026-07-24 — the economy is the central retinue dial (see §7).** Attrition is
handled by the gate-1 combat model (continuous melee attrition, `MaxAttackersPerUnit`
cap); *replenishment* is now the **Recruit** action at growth sites rather than a free
refill. Per-class attrition/replenishment rates remain the key balance dial; identity-level
changes still belong to `CLASSES.md` (canon proposals only).

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
whether procgen (§4 above) can be trusted to place Vanguard-relevant chokepoints narrow
enough for Hold to matter, or whether that needs to become an explicit arena-generation
constraint. Untested — flag before any content leans on Shield Wall being reliable.
Relickeeper's Bulwark (`CLASSES.md` §2, "sentinels interlock into literal wall segments")
makes the identical claim and inherits the identical fork; not resolved here since the
Relickeeper isn't built in the sim yet, but it will hit the same wall (so to speak) the
day it is.

## 7. Retinue economy — Supply + Embers (**DESIGNED 2026-07-24**)

The run's economy, implementing GDD §7 (upkeep) and giving the "meaningful decisions"
pillar (§3) a continuous, playable form. Two resources:

### Supply — the size governor (implements GDD §7)
Supply is **capacity (headroom), not a draining stock**. Upkeep **demand** = the sum of
per-unit upkeep (uniform **1/unit** in the slice). When demand > capacity, units
**degrade — dimmed, weaker (DPS × `capacity/demand`, floored at 0.4), can't hold
formation — but never die.** Fresh recruits enter degraded until capacity recovers.
- **Why capacity/ratio and not a draining clock:** it makes "degrade not die" fall out
  for free (the degrade multiplier *is* the headroom ratio), needs no per-tick
  bookkeeping, and makes the triangle crisp — Recruit raises demand, Provision raises
  capacity, Promote is upkeep-neutral.
- **Why uniform upkeep:** it makes **Promote** the upkeep-efficient path (more power, no
  new mouths) and **Recruit** the upkeep-hungry path (cheap bodies that strain supply).
  Per-tier upkeep is a parked v2 knob.

### Embers — the spend currency
Earned from **brood kills** (0.1 each) + a **grant on reaching a growth site** (10).
Tuned so a player arrives at each site with ~30–40 Embers: enough for **2–3 actions,
never all** (`docs/data/economy.json`). Scarcity is the decision.

### The triangle — breadth / depth / sustain
At each growth site the player allocates Embers across:
| Lane | Action | Effect | Cost / tension |
|---|---|---|---|
| **Breadth** | Recruit | +10 Freed | +10 upkeep demand → toward the degrade line |
| **Depth** | Promote | up to 20 units +1 tier | upkeep-neutral, but headcount stays thin |
| **Sustain** | Provision | +25 Supply capacity | safe, but a turn spent *not* getting stronger |
| *(spice)* | Item | 1 of 3 offered | 20 Embers — costs you most of a triangle turn |
| *(hero)* | Hero node | 1 Vanguard upgrade | competes with the army for the same Embers |

The three triangle lanes genuinely oppose each other (breadth vs. sustain most directly),
so every allocation carries an opportunity cost **and** a downstream consequence — the
definition of a real economy. Hero nodes competing for the same currency is deliberate:
it's the GDD §4 hero-vs-army relevance tension expressed as a spend choice.

### Design intent / falsification test
Gate-1's zero-input baseline **loses wave 3 by 4–13 brood**. The economy layer must be
the margin: **played well it turns that narrow loss into a win; over-recruited past supply
(a degraded, dimmed army) it becomes a blowout loss.** **If any single allocation always
wins, the triangle has failed** and the costs need re-tuning. This is the acceptance test
when the layer is wired into the prototype.

### Upgrades are army-shaping, not stat-creep
Every upgrade (tier promotion, items, hero nodes) changes **how the army behaves and
reads** — formation discipline, auras, a wider Shield Rush — not just a damage number, and
all of it is **run-scoped** (reset each run). This keeps the hard GDD §3 line: "the reward
for a run isn't +5% damage." The reward is a bigger, better, or better-fed *army*.

## 8. Hero progression (slice)

**DECIDED 2026-07-24.** The Vanguard's three abilities (Banner Slam, Shield Rush, The
Muster) each get a one-of-two upgrade node, bought with Embers at growth sites
(`docs/data/upgrades.json`). Run-scoped, and in direct currency competition with retinue
growth — hero power is a *choice against* army power, never a free parallel track.

---

## Decision log

| Date | Decision | Rationale | Spec / data |
|---|---|---|---|
| 2026-07-24 | Economy = **Supply (size governor) + Embers (spend currency)** | Implements GDD §7 upkeep as capacity/headroom; two dials give clean breadth-vs-sustain tension | §7 · `economy.json` |
| 2026-07-24 | Supply is **capacity ratio**, degrade = `capacity/demand` | "Degrade not die" falls out for free; no per-tick bookkeeping; crisp triangle | §7 · `economy.json` |
| 2026-07-24 | **Uniform upkeep 1/unit** (slice) | Makes Promote upkeep-efficient and Recruit upkeep-hungry — the triangle's core | §7 · `economy.json` |
| 2026-07-24 | Meaningful decisions = **growth-site triangle** (recruit/promote/provision + item/hero) | Continuous grain of the same axis as §6 fork events; unifies economy + decisions pillar | §4/§7 · `growth-sites.json` |
| 2026-07-24 | Recruits enter at **Freed** (weak tier) | Makes the promotion economy matter; dovetails with GDD §7 "fresh units enter degraded" | §1 · `upgrades.json` |
| 2026-07-24 | **Loot v0 = 6 stacking items**, offer-3-take-1 | Sanctioned slice loot (RTS-VERTICAL-SLICE §4); feeds army not just hero (GDD §8 twist); not the real loot system | §3 · `upgrades.json` |
| 2026-07-24 | Hero nodes **spend the same Embers** as the retinue | Expresses GDD §4 hero-vs-army relevance as a spend choice | §8 · `upgrades.json` |
| 2026-07-24 | Slice curve locked at **250/450/700 brood** | Gate-1 values adopted as the first act of the exponential fantasy | §2 · `economy.json` |
| 2026-07-26 | **Hold/Shield Wall is porous by design** — a positioning tool, not a barricade; doc moves to match the sim, not the reverse | Matches GDD §7 "soft costs over hard caps" and §10 Mass Entity constraints; a literal blocking wall needs per-unit pathing obstruction, which horde scale can't afford | §6 · `CLASSES.md` §1 (canon proposal pending) · closes GDD §4 "Leash vs. Hold-wall (OPEN)" |
