# Vanguard retinue tuning — growth rate, attrition, replenishment, per-floor cap

**What this is:** the Vanguard-specific retinue tuning pass called for by
`docs/RTS-VERTICAL-SLICE.md` §4 ("Vanguard retinue tuning: growth rate,
attrition, per-floor cap") and `SYSTEMS.md` §6 ("Per-class attrition/
replenishment rates remain the key balance dial"). It answers, for the
Vanguard only: how many units the class's own "Rescue & Rally" growth verb
(`CLASSES.md` §1) adds per floor, how many combat losses each floor's Arena
should be expected to cost, and what governs the retinue's size when neither
of those is enough — closing with a floor-by-floor ledger.

**Extends:** `SYSTEMS.md` §6 (retinue tuning, currently "attrition is handled
by the gate-1 combat model... per-class rates remain the key dial" — this doc
is that dial for one class) and §7 (the Supply/Embers economy, unchanged,
which this doc layers on top of rather than re-specs). Reads `CLASSES.md` §1
(Vanguard identity, growth verb, Hold/Shield Wall), `GDD.md` §7 (upkeep is
the governor, degrade-not-die) and §4 (stances, leash), `docs/design/
entity-tiers.md` + `docs/data/entity-tiers.json` (enemy stat blocks), `docs/
design/encounter-budget.md` + `docs/data/encounter-budget.json` (per-floor
population, composition, pulse timing — the source of every combat-side
number below), `docs/design/run-structure.md` (floor persistence: nothing
resets the retinue between floors except losses taken and Embers spent), and
`docs/GATE1-FUN-PROTOTYPE.md` (the one measured baseline this doc calibrates
against).

**Does not change:** the Supply/Embers economy, its numbers, or its rules
(`SYSTEMS.md` §7, `docs/data/growth-sites.json`, `docs/data/economy.json`) —
those stay exactly as decided. This doc adds a class-specific layer *on top*
of that economy (`docs/design/run-structure.md`'s own handoff to this task:
"if task-005 specs per-class attrition/replenishment rates... that
supersedes the plain-persistence assumption"), and does not re-derive or
re-tune the generic Recruit/Promote/Provision triangle. Nor does it touch
any locked population, composition, or pulse number in `encounter-budget.json`.

---

## 1. Sourcing — read this before the numbers

Per the brief: every number in the §6 ledger traces to a named cell in
`docs/data/encounter-budget.json` or `docs/data/entity-tiers.json`. That
covers the entire **combat-loss** side of this doc (§3) exactly, because
that's what those two files are *for* — locked population, composition, and
enemy stat blocks. It does not cover two things a retinue ledger cannot
honestly omit, and I'm stating the exception plainly rather than quietly
stretching the two-file rule to pretend it does:

- **Starting headcount (40).** No cell in either whitelisted file states how
  many units the Vanguard starts a run with. `docs/data/economy.json`
  `slice_targets.start_units` does (40, at Militia tier) — already-decided
  data, not re-derived here, cited because the ledger has to start somewhere
  real. Cross-checked, not coincidentally: `encounter-budget.json`
  `peak_concurrency_check[0].RetinuePeak` is also **40** for floor 1 — the
  two independently-authored files agree on the floor-1 number, which is
  the closest thing to a consistency check available.
- **The growth-site Recruit lane (§5's second scenario only).** The
  strictly-sourced ledger in §6 is **Rescue & Rally only** — the new
  mechanic this doc specs, 100% traceable to `encounter-budget.json`. §5
  additionally shows what the existing, unmodified Ember economy
  (`docs/data/growth-sites.json`, cited plainly, not re-tuned) adds on top,
  because presenting the Rescue-only number as if it were the whole picture
  would misrepresent how the game is actually played. That second scenario
  is clearly boxed off from the §6 ledger and never substituted for it.

Every other number below — population, composition, enemy DPS/Armor, pulse
timing, the Risk Room bonus pocket, the GATE1 calibration point — is a named
cell in one of the two files, cited inline.

---

## 2. Growth verb recap (read-only, `CLASSES.md` §1)

> Primary source: **rescue sites** (cells, slave pens, besieged survivors)
> seeded by the generator. Freeing them = recruiting them.
> Secondary: survivors of hard fights **promote** (Veteran system).
> Tension: rescues are noisy/defended — growth always costs a fight.

This doc gives that fiction a concrete mechanic (§4) sized against a room
type the generator already builds (`encounter-budget.json`'s Risk Room),
rather than inventing a new site or touching the growth-site economy that
already implements the generic Recruit lane every class shares. Veteran
promotion (quality, not headcount) is `upgrades.json`'s existing tier ladder
— unchanged, mentioned in §7 only for how it interacts with enemy Armor.

---

## 3. Attrition — expected combat losses per floor

### 3.1 Method

The only measured retinue-vs-swarm data point in the repo is `docs/
GATE1-FUN-PROTOTYPE.md` §3b's shipped-defaults baseline: 4 zero-input runs,
120 Militia-tier retinue vs. a single 250-Fodder wave, **109–111 survive**
(9–11 losses, mean **10**). `encounter-budget.json`'s own
`rank_arrival_context[0]` (`Name: "gate1_calibration_wave1"`) cites this
exact scenario (`PopulationN: 250, RetinueN: 120`), so the anchor point
itself is a named cell in the whitelisted file, not an import from outside
it.

Scale that one point by each floor's **danger index** — the population-
weighted mean enemy DPS, computed entirely from `entity-tiers.json`'s `DPS`
column and `encounter-budget.json`'s `pulse_schedule[]` composition (summed
across a floor's pulses):

```
DangerIndex(floor) = Σ_tier( PulseCount_tier(floor) × DPS_tier ) / Population(floor)

ExpectedLosses(floor) = Losses_anchor × [ Population(floor) × DangerIndex(floor) ]
                                       / [ Population_anchor × DangerIndex_anchor ]

Losses_anchor = 10          (GATE1 measured, mean of 9-11)
Population_anchor = 250     (rank_arrival_context.gate1_calibration_wave1.PopulationN)
DangerIndex_anchor = 35     (entity-tiers.json brood_fodder.DPS — the anchor wave was pure Fodder)
```

**Stated assumption, not hidden in the constant:** this scales absolute
losses by total floor danger (population × per-capita DPS), not by
defending headcount. With exactly one measured calibration point, fitting a
second free parameter for "how much does a bigger defending army reduce its
own absolute losses" would be curve-fitting noise, not data — so the model
doesn't attempt it, and losses-per-floor come out **independent of
headcount** in this approximation (see `docs/data/retinue-vanguard.json`
`expected_losses[]` for the full per-floor arithmetic). That is very likely
wrong in the optimistic direction at low headcount (a thin line should take
*more* losses per capita than a full one, not the same absolute count) —
flagged here and again in §8, not smoothed over.

### 3.2 Danger index and expected losses, per floor

| Floor | Fodder | Soldier-melee | Soldier-ranged | Population | DangerIndex | Sources |
|---|---|---|---|---|---|---|
| 1 | 212 | 38 | 0 | 250 | **36.06** | `pulse_schedule[floor1_pulse1]`; DPS 35/42/26 from `entity-tiers.json` tiers |
| 2 | 148+99=247 | 68+45=113 | 54+36=90 | 450 | **34.96** | `pulse_schedule[floor2_pulse1, floor2_pulse2]` |
| 3 | 94+110+110=314 | 52+61+61=174 | 64+74+74=212 | 700 | **34.01** | `pulse_schedule[floor3_pulse1..3]` |

**Finding, not an assumption going in:** the danger index barely moves
(36.06 → 34.96 → 34.01) across the whole slice — Soldier-melee's higher DPS
(42 vs Fodder's 35) is almost exactly offset by Soldier-ranged's lower DPS
(26) in the locked composition mix. The escalation across floors 1→3 is
driven almost entirely by **population**, not by the enemies getting
individually more dangerous — the same shape `entity-tiers.md` §4 found for
melee's surround cap (more bodies, not tougher bodies, is where the
pressure comes from — Design Law 3, applied here to the friendly side of
the fight instead of the enemy tier ladder).

| Floor | ExpectedLosses (arithmetic) |
|---|---|
| 1 | `10 × (250×36.06)/(250×35) = 10 × 1.030 =` **10.3 → 10** |
| 2 | `10 × (450×34.96)/(250×35) = 10 × 1.798 =` **18.0 → 18** |
| 3 | `10 × (700×34.01)/(250×35) = 10 × 2.721 =` **27.2 → 27** |

Full precision in `docs/data/retinue-vanguard.json` `expected_losses[]`.

---

## 4. Replenishment — Rescue & Rally

### 4.1 The mechanic (new, this doc)

The Risk Room (`encounter-budget.json` `room_types[]`, `risk_room_budget[]`)
is already a pure-Fodder, optional, additive-population room reached only
before the Arena's clock starts — a guarded pocket the player chooses to
clear for a loot roll and Embers. That's exactly the shape `CLASSES.md` §1
describes for a rescue site ("cells, slave pens... noisy/defended — growth
always costs a fight"), so rather than invent a second room type, **Rescue
& Rally reframes clearing the Vanguard's Risk Room as freeing captives held
behind that guard detail**, in addition to its existing loot/Ember reward
(unchanged) — no new room, no competing with the Risk Room's existing
design intent, no edit to `encounter-budget.json`.

```
RescuedFreed(floor) = round( RiskRoomBonusFodder(floor) × RescueConversionRate )

RescueConversionRate = 0.25   (proposed Vanguard.RescueConversionRate — PROTOTYPE DIAL, this
                                doc's own judgment call, same status as PulseLullSeconds/
                                EliteLeadSeconds elsewhere in docs/data/)
```

25% is a "one freed conscript per four guards cleared" ratio — legible
(roughly a quarter of what you kill walks out with you), and small enough
that Rescue & Rally reads as a genuine supplement to the growth-site economy
rather than a way to bypass it (§5 shows why that headroom matters: even at
this modest rate it's still not enough on its own, §6).

Rescued units enter at **Freed** tier (`upgrades.json`, unchanged — same
tier every growth-site Recruit action produces), draw the same uniform 1
upkeep/unit as any other retinue member (`economy.json`, unchanged — see §7
for why that matters), and are present for that floor's own Arena fight
(the Risk Room is cleared *before* the Arena starts, per `encounter-budget.
md` §3's timing rule), so they're exposed to that floor's losses like
anyone else already accounted for in §3's population figure.

### 4.2 Per floor

| Floor | RiskRoomBonusFodder (`risk_room_budget[]`) | × 0.25 | RescuedFreed |
|---|---|---|---|
| 1 | 20 | | **5** |
| 2 | 35 | | **9** (8.75, rounded) |
| 3 | 55 | | **14** (13.75, rounded) |

Optional exactly like the Risk Room itself is optional — a player who skips
the Risk Room gets none of this, same tradeoff `encounter-budget.md` §3
already describes ("time spent here is time not spent preparing/
positioning for the harder fight ahead").

---

## 5. What this doesn't include: the growth-site economy (context, not re-specced)

`SYSTEMS.md` §7's Recruit action (+10 Freed / 12 Embers) is unchanged and
sits on top of everything above — it's the primary lever the game already
has, this doc adds a secondary one. Cited here for scale, from
`docs/data/growth-sites.json` (plainly outside this doc's two-file
boundary, not re-tuned):

- **growth-A** (after floor 1): ~33 Embers on arrival → up to 2 Recruit
  actions (24 of 33 Embers) → **+20 Freed**, if the player spends the whole
  turn on breadth (a real cost — that's the triangle, `SYSTEMS.md` §7).
- **growth-B** (after floor 2): ~52 Embers → up to 4 Recruit actions (48 of
  52) → **+40 Freed**, same caveat.

§6 is Rescue-only, exactly as scoped. §6b shows the combined picture for
context, clearly boxed off.

---

## 6. Three-floor ledger

**Rescue & Rally only** — every figure below traces to `encounter-budget.
json` or `entity-tiers.json` per §1, except the starting headcount (§1's
stated exception). This isolates what this doc's own mechanic contributes;
§6b adds the existing (unmodified) growth-site economy for comparison.

| Floor | Entering headcount | + Rescue & Rally | Headcount into Arena | − Expected losses (§3) | Exiting headcount |
|---|---|---|---|---|---|
| 1 | 40 *(`economy.json` start_units, cross-checked against `encounter-budget.json peak_concurrency_check[0].RetinuePeak`=40)* | +5 | 45 | −10 | **35** |
| 2 | 35 | +9 | 44 | −18 | **26** |
| 3 | 26 | +14 | 40 | −27 | **13** |

Floor 3's exit (13) is who walks into the isolated boss room (`encounter-
budget.json` `room_types[floor3_boss_room]`: no growth site, no Risk Room,
no concurrent swarm — nothing between the Arena and the boss to add or
patch headcount, per `run-structure.md` §4).

**Verdict, read straight off the last column: starve.** Rescue & Rally
alone loses headcount every floor (35 → 26 → 13) — it does not keep pace
with §3's combat losses on its own. That is by design in one sense (§4.1:
it's sized as a *supplement*, not a replacement for the Recruit lane) and a
genuine warning in another: **this is the same headcount-vs-population gap
`scaling-curve.md` §3 and `run-structure.md` §3 already flagged**, arrived
at here by an independent method (a danger-index Fermi model, not the
Lanchester approach those docs used) and landing on the same qualitative
finding — worth treating as corroboration, not a new problem.

### 6b. For comparison — with the existing growth-site Recruit lane added (context only, sourced outside §1's two files)

| Floor | Entering | +Rescue | Into Arena | −Losses | Exit | + growth-site Recruit (§5, `growth-sites.json`, unmodified) | Next floor entering |
|---|---|---|---|---|---|---|---|
| 1 | 40 | +5 | 45 | −10 | 35 | +20 (growth-A, breadth-max) | 55 |
| 2 | 55 | +9 | 64 | −18 | 46 | +40 (growth-B, breadth-max) | 86 |
| 3 | 86 | +14 | 100 | −27 | **73** | — (no site before the boss) | — |

73 vs. 13 at the boss gate is the whole story: **the generic Ember economy
is still the dominant lever for Vanguard headcount, not Rescue & Rally.**
This doc's mechanic meaningfully softens the Rescue-only curve (§6) but
was never sized to replace growth-site spend, and shouldn't be read as
though it could.

---

## 7. Per-floor cap — soft, tied to Supply (Design Law 2: soft caps only)

This doc does not add a hard per-floor headcount ceiling — Design Law 2
rules that out, and `SYSTEMS.md` §7 already has the governor: Supply
capacity vs. upkeep demand, degrade (not die) below a 0.4× DPS floor when
demand outruns capacity (`economy.json`, unchanged).

**The Vanguard-specific point worth naming, because Rescue & Rally makes it
sharper than it is for a class with no free-headcount lane:** rescued units
draw the same uniform 1 upkeep/unit as any Ember-recruited unit
(`economy.json upkeep_per_unit`), but they cost **zero Embers** to obtain.
That's exactly the shape that makes Recruit the "upkeep-hungry" lane in
`SYSTEMS.md` §7's own framing — Rescue & Rally is a *free* version of that
same lane, which means a player who clears every Risk Room aggressively
without also spending Embers on Provision pushes Supply demand up for
nothing in return, degrading **the whole army, including Veterans**, not
just the newly-rescued Freed (`economy.json`'s uniform-upkeep design,
unchanged, bites everyone equally). The practical per-floor cap on how much
Rescue & Rally headcount is *worth taking* is therefore wherever Supply
capacity runs out relative to demand — a soft, play-driven ceiling that
falls straight out of an already-decided mechanic, not a new number this
doc invents. Concretely: a player who Rescues 5+9+14=28 units across the
slice without ever spending Embers on Provision (`SYSTEMS.md` §7's
sustain lane) is adding 28 to Supply demand for free — worth surfacing at
the growth-site panel (a UI/copy note, not a numbers one) so "free
headcount" doesn't quietly read as "free power."

---

## 8. The bimodal caveat — why §3's numbers are likely optimistic, not just imprecise

`GATE1-FUN-PROTOTYPE.md` §3b's own tuning sweep is the load-bearing warning
for this whole doc: the shipped combat model is **measured, not assumed, to
behave bimodally** near its own knife-edge — `RetinueTargetsPerHit` 3 loses
badly (~220 Fodder survive) while 4 wins outright (9–21 retinue alive),
one integer step apart. §3's danger-index model is a **smooth linear
interpolation**; the system it's approximating is not smooth at all near
collapse. Floor 1 here enters the Arena at 45 retinue against 250 population
(§6) — a population:retinue ratio of **5.6:1**, nearly **3× worse** than
the GATE1 calibration point's own 2.1:1 (120:250), which is itself the
ratio at which the shipped model already sits on a knife-edge. §3's "10
losses" for floor 1 should be read as an **order-of-magnitude floor, not a
ceiling** — the real risk at this ratio, per the only measured evidence
this repo has for how the model actually behaves under pressure, is a
collapse outcome (most or all of the 45 lost), not a graceful 22% skim.
This is the single most important number in this doc to get right before
anyone tunes content against §6's ledger, and it can't be resolved without
an in-engine measurement at Vanguard-realistic headcounts (§10).

---

## 9. Narrative requests

No new entities. One framing request:

- **Rescue & Rally reframes an existing room, so the fiction needs to say
  so.** The Risk Room is currently pure mechanic (bonus Fodder pocket,
  guaranteed loot). For the Vanguard specifically, clearing it should read
  as *freeing a guarded holding pen* — CLASSES.md's own language ("every
  cell door you break open, every conscript you free, your line grows
  longer"). What a player must feel: the Risk Room's reward isn't just loot
  when played as the Vanguard, it's *people* — worth a distinct
  approach-the-room beat (silhouettes behind bars, chained figures) once
  biome/faction canon exists to hang it on, per `FLAME-FOUNDATION.md`.
  Mechanically nothing changes if this lands as generic "you found
  survivors" flavor instead — it's a feel request, not a numbers one.

---

## 10. Simulation notes

**What was simulated:** a scratch Python script (session scratchpad, not
committed) that computed §3's danger index per floor from `entity-tiers.
json` DPS values and `encounter-budget.json` `pulse_schedule[]` composition,
the calibrated expected-losses arithmetic against the GATE1 anchor, §4's
rescue conversion, and both ledger scenarios (§6, §6b) sequentially across
three floors. Same house method as `entity-tiers.md` §7 / `encounter-
budget.md` §8: closed-form arithmetic over already-decided/measured/shipped
constants, no engine run, no `Scripts/sim/` harness invocation.

**Why the harness wasn't used, explicitly, per the brief's instruction:**
`docs/sim/LIMITATIONS.md` §1 states plainly that `simulate_wave_attrition`
cannot reproduce GATE1's own measured ~110-of-120 wave-1 survival at its
committed defaults (predicts a full wipe), and that arrival-timing has
already been tested (task-068) and found not to close that gap — leaving
`MaxAttackersPerUnit`'s pooled-vs-per-entity transfer as the untested
remaining candidate. Running this doc's floors through that harness would
therefore reproduce a known-bad, already-flagged result, not test anything
new about Vanguard-specific replenishment. This doc's danger-index model is
a different, narrower tool — it never claims to predict a survivor count
from first principles, only to scale the *one trustworthy measured point
that exists* (GATE1's zero-input baseline) by locked population/composition
data. §8 states exactly where that tool is known to be weakest (the
bimodal knife-edge) rather than presenting it as validated.

**Assumptions, stated plainly (none measured in-engine beyond the GATE1
anchor itself):**
- §3's losses-independent-of-headcount property (§3.1) is a direct
  consequence of having exactly one calibration point — not a claim that
  headcount doesn't matter, a stated limitation of what one data point can
  support.
- `RescueConversionRate` (0.25, §4.1) is a first-pass judgment call, the
  same status as `PulseLullSeconds`/`EliteLeadSeconds` in `encounter-
  budget.json` — to be judged by feel once Rescue & Rally exists on screen,
  not derived from anything measured.
- §6b's "breadth-max" growth-site scenario assumes every Ember at both
  growth sites goes to Recruit and none to Promote/Provision/items/hero
  nodes — an illustrative upper bound on headcount, not a claim about how
  the triangle should actually be played (`SYSTEMS.md` §7's own "if any
  single allocation always wins, the triangle has failed" falsification
  test applies here too — breadth-max is deliberately the least-balanced
  scenario, shown to bound the range, not recommended).
- §8's bimodal caveat is itself unmeasured at Vanguard-realistic headcounts
  (35–45, not GATE1's 120) — flagged as the doc's largest open dependency
  (§11), not resolved here.

---

## 11. Handoffs

**To whoever next runs an in-engine measurement of the shipped combat
model.** §8's bimodal caveat is this doc's single biggest open dependency:
a zero-input (or scripted-input) measurement at a Vanguard-realistic
headcount (35–45 retinue vs. 250 population, floor 1's actual entering
condition per §6) would tell us whether §3's Fermi losses are a reasonable
floor or a serious underestimate. Until then, treat §6's ledger as
directionally right (grow/hold/starve ordering) but not numerically
trustworthy at the knife-edge `GATE1-FUN-PROTOTYPE.md` §3b already measured
the model to have.

**To whoever revises `SYSTEMS.md` §2 or §7 next** (already an open item
from `scaling-curve.md` §6 and `run-structure.md` §7). §6's verdict is a
second, independently-derived confirmation of the same headcount-vs-
population gap — worth citing alongside the existing finding, not instead
of it.

**To whoever implements the Risk Room / growth-site UI.** §7's closing note
(Rescue & Rally is free-Ember headcount that still costs Supply) is a
copy/UI ask, not a numbers one — surfacing it prevents "free" from reading
as "free power."

**To the narrative-director**, per §9, once biome/faction canon exists.

**To whoever owns `SYSTEMS.md` next.** §3/§4/§7 above are candidates for a
new §6 sub-entry ("Vanguard: Rescue & Rally") once this spec is reviewed —
this doc doesn't edit `SYSTEMS.md` itself (out of scope for this task).

---

## 12. Canon proposals

None. Rescue & Rally is additive to `CLASSES.md` §1's already-stated growth
verb ("rescue sites... freeing them = recruiting them") — it gives that
sentence a concrete room-and-number binding rather than contradicting or
extending the class identity itself. Nothing here proposes a new stance,
ability, or identity change.
