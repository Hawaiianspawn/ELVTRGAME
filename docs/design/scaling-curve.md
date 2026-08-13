# Scaling curve — the vertical slice's three floors

**What this is:** the one scaling curve `SYSTEMS.md` §2 and
`docs/RTS-VERTICAL-SLICE.md` §4 call "still open" — floor-by-floor enemy
composition and the retinue growth curve that has to keep pace with it, for
the 3-floor single-player slice. **Single-player only** (GDD §12 Q20,
2026-07-27) — there is no party-size axis, and `RTS-VERTICAL-SLICE.md`'s own
"party-size scaling" language (§4, referring to growth-site count) is stale
for the same reason; this doc treats it as retired.

**Extends:** `SYSTEMS.md` §2 (locks the enemy-population numbers, 250/450/700,
as canon and does not change them), §7 (the Supply/Embers economy, whose
tuned numbers this doc simulates against rather than re-derives), and
`docs/design/entity-tiers.md` / `docs/data/entity-tiers.json` (the Fodder/
Soldier/Elite/Boss stat blocks this doc schedules onto the 3 floors — Titan is
correctly excluded, per that doc's own scope note).

**Does not change:** any tier stat block in `entity-tiers.json`, the locked
population numbers in `SYSTEMS.md` §2 / `economy.json`, or the growth-site
action costs in `growth-sites.json`. Everything here is a *schedule* over
already-decided numbers, plus one un-decided finding this doc surfaces and
does not resolve (§3).

---

## 1. The floor roster curve

`SYSTEMS.md` §2 already locks total Mass-Entity population per floor at
**250 → 450 → 700** (×1.8, ×1.56) — the gate-1 wave values, unchanged here.
What was still open is *composition*: entity-tiers.json added Soldier-melee,
Soldier-ranged, Elite, and Boss, and none of them had a floor placement.

**DECIDED — composition shifts each floor from pure Fodder toward armed,
organized, individually dangerous.** Reads as narrative escalation
("what you fight has had time to arm it," entity-tiers.md §6) as much as a
difficulty curve.

| Floor | Population (locked, `SYSTEMS.md` §2) | Fodder | Soldier (melee) | Soldier (ranged) |
|---|---|---|---|---|
| 1 | 250 | 85% (212) | 15% (38) | 0% |
| 2 | 450 (×1.8) | 55% (248) | 25% (112) | 20% (90) |
| 3 | 700 (×1.56) | 45% (315) | 25% (175) | 30% (210) |

Floor 1 is a Fodder floor with a first taste of Soldier-melee ("a tougher,
harder-hitting Brood" — entity-tiers.md's own framing for that tier's
identity). Floor 2 introduces the ranged threat and pulls Fodder's share down
by 30 points in one step — the single biggest composition swing in the curve,
matched to the population jump also being the curve's steepest (×1.8). Floor
3 keeps shifting the mix toward Soldier without changing the taxonomy again;
by floor 3 more than half the population is armed.

### Elite / Boss — instanced, not part of the population count

Elite/Boss are `PromotedActor`s (entity-tiers.md §1), not Mass Entities — they
don't compete with the 250/450/700 budget and are scheduled as small,
explicit instance counts:

| Floor | Elite instances | Boss instances | Encounter mode |
|---|---|---|---|
| 1 | 0 | 0 | — |
| 2 | 1 | 0 | **embedded** — spawns inside the floor's live Fodder/Soldier population, not a separate room |
| 3 | 2 | 1 (finale) | Elites **embedded**; Boss **isolated** — its own arena, per `RTS-VERTICAL-SLICE.md` §5's "boss room" prefab |

**DECIDED — Floor 3 reuses the Elite type at 2 instances rather than
introducing a new stat block.** This is Design Law 3 ("more enemies, not
spongier ones") applied to the promoted-Actor tier, not just the swarm: the
roster stays at its stated 5 types (`RTS-VERTICAL-SLICE.md` §5) and the
escalation is entirely "the thing you learned to respect on floor 2 now
happens twice." Titan would be the next rung on this ladder and is correctly
out of slice scope.

**DECIDED — Elites are embedded in the live swarm, never fought as a clean
1-on-1.** entity-tiers.md's own Elite row says its `TargetsPerHit 5` AoE
"punishes the retinue for clustering on it" — that only means something if
the retinue is *also* under pressure from the swarm around it, not standing
in an empty room. This is a real, load-bearing decision, not a flavor note —
see §4, where an isolated-fight assumption produces a badly misleading TTK
number for exactly this tier.

---

## 2. The retinue growth curve

`economy.json`'s `slice_targets` already commits to **start 40 units at
Militia**, **2 growth sites** (after floor 1, after floor 2 — matching this
doc's 3-floor structure exactly: one growth site per floor transition), and
`growth-sites.json` prices out the triangle (Recruit 12E/+10 Freed, Promote
15E/up to 20 units +1 tier, Provision 10E/+25 Supply) against an estimated
arrival pool of ~33 Embers (site A) and ~52 Embers (site B). Nothing here
changes those numbers — this section is what they *produce*, run forward as
two bounding play patterns, both spending inside the stated per-site budgets.
Recruit's 80/20 Spearmen/Archer split (`unit-types.json`) is carried through
the whole curve (an unresolved assumption on archer tier-scaling is flagged
in §7).

### Balanced (recruit + promote + provision, spread across both sites)

| Stage | Units | Archers | Supply cap / demand | Degrade | Army eff. DPS | ×Militia baseline |
|---|---|---|---|---|---|---|
| Floor 1 start | 40 | 8 (20%) | 60 / 40 | none | 1104 | ×36.8 |
| Floor 2 start | 50 | 10 (20%) | 85 / 50 | none | 1288 | ×42.9 |
| Floor 3 start | 60 | 12 (20%) | 110 / 60 | none | 1702 | ×56.7 |

Site A: Recruit + Provision (22 of ~33E). Site B: Promote 20 (10 Freed→Militia,
10 Militia→Veteran) + Recruit + Provision (37 of ~52E) — by floor 3 this path
carries **10 Veterans**, never touches the degrade floor, and ends with
**60** total bodies.

### Recruit-max (spend every Ember on bodies; bounds the other end)

| Stage | Units | Archers | Supply cap / demand | Degrade | Army eff. DPS | ×Militia baseline |
|---|---|---|---|---|---|---|
| Floor 1 start | 40 | 8 (20%) | 60 / 40 | none | 1104 | ×36.8 |
| Floor 2 start | 60 | 12 (20%) | 60 / 60 | none | 1472 | ×49.1 |
| Floor 3 start | 90 | 18 (20%) | 85 / 90 | **0.944×** | 1912 | ×63.7 |

Site A: Recruit ×2 (24 of ~33E — the budget only just covers two, with 9E
banked). Site B: Recruit ×3 + Provision (46 of ~52E). This path never
promotes a single unit, reaches supply exactly at the line by floor 2
(capacity == demand, borderline), and **crosses into degrade by floor 3**
(capacity 85, demand 90 → 0.944× DPS) — the "over-recruit past supply → a
degraded, dimmed army" case from `SYSTEMS.md` §7's own falsification test,
falling out of the stated Ember income and Recruit cost without any extra
assumption.

**Reading the multiplier column against Design Law 1:** ×36.8 → ×56.7–63.7 is
real growth but not "orders of magnitude" — and it isn't supposed to be yet.
`SYSTEMS.md` §2 already frames 250/450/700 as *"the first act of the
exponential fantasy,"* not the whole curve; items (Whetstone alone adds a flat
+3 DPS/unit per stack — two stacks on a 60-unit army is +360 army DPS, ~+21%
on top of the table above) and hero nodes aren't modeled here and are exactly
where the rest of the exponential feel is meant to come from. Flagged so this
table isn't mistaken for the full promise — it's the retinue-count-and-tier
layer only.

---

## 3. Where the curve actually collapses — a headcount reality check

**This is the headline finding, and it is not resolved here.** An initial
attempt to simulate floor-by-floor swarm clear time from stat blocks alone
(§7, Simulation notes) produced an unusable result — 100% retinue wipe, every
floor, both scenarios — that contradicts the *measured* gate-1 baseline
outright. That divergence turned out to be diagnostic, not a dead end: the
reason a closed-form stat model can't reproduce `GATE1-FUN-PROTOTYPE.md`'s
measured numbers is that survival there depends on **encounter pacing**
(spawn rate and arrival timing over a wave), which no stat-block table
captures and which `RTS-VERTICAL-SLICE.md` §4 lists as a still-unbuilt
deliverable (`[ ] Encounter budget table per floor`). So instead of trusting
an invented physics model, this section uses `GATE1-FUN-PROTOTYPE.md`'s own
**measured** shipped-defaults data as a calibration point — the same
"cross-check against shipped numbers before trusting new ones" method
`entity-tiers.md` §7 already uses.

**The calibration.** Gate-1's shipped defaults, zero player input, retinue
**flat-refilled to 120 before every wave**:

| Population faced | Retinue count | Survivors | Survival rate |
|---|---|---|---|
| 250 | 120 | ~110 | 92% |
| 450 | 120 | ~20 | 17% |
| 700 | 120 | 0 (wiped mid-fight) | 0% |

Survival collapses hard between a population/army ratio of **2.08** (250/120)
and **3.75** (450/120) — consistent with `GATE1-FUN-PROTOTYPE.md` §3b's own
"bimodal" finding: this combat model holds a line easily or loses it
completely, with a narrow knee in between, not a smooth gradient.

**The problem: the growth-site economy never gets near 120.** §2's two
scenarios top out at **60** (balanced) and **90** (recruit-max) by floor 3 —
and floor 1 starts at **40**, a third of the calibration army, before a single
Ember has been spent. Running the same population/army ratio against §2's
actual counts:

| Floor | Population | Balanced ratio | Recruit-max ratio | Gate-1 calibration ratio at this population |
|---|---|---|---|---|
| 1 | 250 | **6.25** (N=40) | **6.25** (N=40) | 2.08 (N=120) |
| 2 | 450 | **9.00** (N=50) | **7.50** (N=60) | 3.75 (N=120) |
| 3 | 700 | **11.67** (N=60) | **7.78** (N=90) | 5.83 (N=120) |

Every floor of the growth-site-fed curve sits at a worse population/army
ratio than the ratio at which gate-1's *own measured baseline was already
being wiped*. Floor 1 alone (ratio 6.25) is already past that wipe point.
Read plainly: **as currently tuned, the growth-site economy (`economy.json`,
`growth-sites.json`) does not produce enough bodies to survive the
already-locked population curve (`SYSTEMS.md` §2), starting on floor 1** —
this isn't a late-slice problem, it's a floor-1 problem, because the economy
replaces gate-1's flat refill-to-120 (`SYSTEMS.md` §4) with a much smaller
starting and growth budget and nothing in `SYSTEMS.md` §2's locked numbers
was re-examined when that replacement was decided.

**What this is not.** This ratio model can't credit stance play, positioning,
or Hold chokepoints — gate-1's calibration point is itself the *zero-input*
baseline specifically because those are the variables a real run adds
(`GATE1-FUN-PROTOTYPE.md` §3). Skilled play narrows the gap. But the gap here
is large (floor 1 needs to close roughly a 3x ratio deficit, floor 3 close to
2x even in the *better* scenario), and player skill is exactly what gate-1's
own zero-input framing exists to factor *out* of a calibration comparison —
I don't think skill alone accounts for a gap this size, and this doc doesn't
have the tooling to prove otherwise. Flagging plainly rather than asserting
it either way.

**Not resolved here — three live options, in order of how little they cost
to try first:**
1. **Cut floor 1/2 population** so early floors sit inside a ratio the
   economy's early headcount can actually hold (e.g., scale floor 1 to
   something nearer 100–130 rather than 250) — cheapest change, but it means
   revisiting `SYSTEMS.md` §2's locked numbers, which this doc isn't
   authorized to do.
2. **Raise Recruit yield or Ember income** so the economy reaches something
   closer to the 120-count calibration point by floor 1 — touches
   `economy.json`/`growth-sites.json`, not mine either.
3. **Lean harder into quality (Promote) over quantity (Recruit)** and accept
   that the ratio model itself is the wrong lens for a promoted, capped-cleave
   army (`RetinueTargetsPerHit 8` already lets a smaller force punch above its
   headcount in a way flat ratio comparison doesn't capture) — cheapest in
   effort, but unproven; needs the encounter-pacing tool from §4's own gap to
   actually test rather than assert.

This is the item to bring back to whoever next revises `SYSTEMS.md` §2 or §7 —
it is a real conflict between two already-decided systems, not a modeling
artifact, and the acceptance test SYSTEMS.md §7 already states ("if a single
allocation always wins the triangle has failed") can't even be evaluated
until this gap is closed, because both scenarios currently point at "loses,"
just by different amounts.

---

## 4. Elite / Boss — the melee-cap tension, confronted

task-002 (`entity-tiers.md` §4) found melee DPS against a single point-target
is **geometrically capped** (`SurroundCapEstimate`) and flat from N=50 to
N=250, so all army-size scaling against Elite/Titan/Boss comes from the
uncapped Archer contribution — a direct tension with Design Law 3 ("scale by
more enemies, not spongier ones"), since past the cap, more enemies-of-your-own
literally do nothing.

**Verdict for this curve, specifically: the melee-cap ceiling is not the
acute problem at slice-scale army sizes.** task-002's own sweep tested
N=50/120/250; §2's actual floor-by-floor armies never exceed 90. At that
range the cap doesn't starve melee so much as make *extra* headcount past it
free — visible directly in the committed harness (`Scripts/sim/
scenario_runner.py`, run against `docs/data/scenarios/floor2-elite-point-
target*.json`, `floor3-elite-point-target.json`, `floor3-boss-point-
target*.json`):

| Scenario | Target | Army N | TTK | Melee dps (engaged/total spearmen) | Archer dps (count) | Hero dps |
|---|---|---|---|---|---|---|
| Balanced | Floor 2 Elite | 50 | **2.13s** | 333.3 (20 of 40) | 46.7 (10) | 41.7 |
| Recruit-max | Floor 2 Elite | 60 | **2.09s** | 333.3 (20 of 48) | 56.0 (12) | 41.7 |
| Balanced | Floor 3 Elite (×2, sequential) | 60 | **2.09s each** (4.18s total) | 333.3 (20 of 48) | 56.0 (12) | 41.7 |
| Recruit-max | Floor 3 Elite (×2, sequential), undegraded | 90 | 1.96s each (3.92s total) | 333.3 (20 of 72) | 84.0 (18) | 41.7 |
| Recruit-max | Floor 3 Elite (×2, sequential), 0.944× degrade applied | 90 | **2.07s each** (4.13s total) | 314.7 (20 of 72, degraded) | 79.3 (18, degraded) | 41.7 |
| Balanced | Floor 3 Boss | 60 | **8.23s** | 650.0 (45 of 48) | 40.0 (12) | 39.4 |
| Recruit-max | Floor 3 Boss, undegraded | 90 | 8.01s | 650.0 (45 of 72) | 60.0 (18) | 39.4 |
| Recruit-max | Floor 3 Boss, 0.944× degrade applied | 90 | **8.46s** | 613.6 (45 of 72, degraded) | 56.6 (18, degraded) | 39.4 |

The four "undegraded" rows are the harness's literal output — it has no
first-class degrade input (`docs/sim/LIMITATIONS.md` §4) — and the three
"degrade applied" rows are this doc hand-multiplying `scaling-curve.json`'s
own 0.944× recruit-max-floor-3 factor onto the retinue-only DPS (Spearmen +
Archers; Hero excluded, since degrade is a Supply/retinue mechanic,
`economy.json` §7, not something that touches the hero) — exactly the
adjustment `floor3-boss-point-target-recruitmax.json`'s own Notes field
already calls for.

**Corrected verdict — this reverses the previous draft's reading, not just
its numbers.** The table above replaces one that used TTK figures from a
discarded, uncommitted scratch script (§7) whose implied per-unit Spearman
DPS against the Elite (293.3 dps / 20 engaged = 14.665) does not match the
per-unit DPS `entity-tiers.md` §2.2 actually derives for that matchup
(Militia Blow 27 − Elite Armor 12 = 15.0 Blow → 15.0 / 0.9s = **16.667**
DPS/unit — the same number the committed harness uses and the same number
`entity-tiers.md` §7's own Elite row and `validate.py`'s bonus check both
reproduce). Re-run through the harness with the correct per-unit numbers, the
Elite result **reverses**: recruit-max beats balanced at Floor 2 (2.09s vs
2.13s) and beats it again at Floor 3, both undegraded (1.96s vs 2.09s) and —
narrowly, a ~1% margin — even after hand-applying the 0.944× degrade penalty
(2.07s vs 2.09s). Recruit-max's extra, uncapped Archer headcount
(`SurroundCapEstimate` doesn't bind ranged attackers, entity-tiers.md §4)
outweighs its extra melee headcount being wasted above the cap, at both
floors' Elite fights.

**The Boss matchup is the one place the triangle-opposition reading
survives, and only once degrade is counted honestly.** Because the harness
doesn't model degrade natively, its literal recruit-max Boss output (8.01s)
is an *optimistic* bound, exactly as `floor3-boss-point-target-
recruitmax.json`'s own Notes field states. Hand-applying the 0.944×
multiplier gives 8.46s — slower than balanced's 8.23s — so recruit-max does
lose the Boss fight once its own over-recruit penalty is counted. That's a
real but narrow (~3%) confirmation of the triangle's intent, and it rests on
a hand-adjustment made outside the harness, not a harness-native result.

**So: the triangle does not validate cleanly across every point-target
fight, as the previous draft of this section claimed.** Recruit-max beats
balanced in both Elite matchups (comfortably at Floor 2 and undegraded Floor
3; by a hair once Floor 3's own degrade penalty is applied) and only loses
the Boss matchup — and even that loss depends on a degrade adjustment the
harness itself doesn't compute. The *mechanism* behind Design Law 3
(surround-cap flattens melee's contribution, Archers scale past it) is
confirmed exactly as `entity-tiers.md` §4 describes — that part was never in
question. What doesn't hold up is the specific prior claim that this caps
out to "balanced wins every point-target fight": it wins one of three. The
triangle's real opposition, on this evidence, lives in §3's headcount-ratio
gap and in the Supply-degrade mechanic doing its intended job on the Boss row
— not in melee-cap alone favoring balanced across the board.

**Two things this table gets wrong if read as a final number, both flagged
rather than fixed here (out of this doc's scope):**
1. **Elite TTK (1.96s–2.13s across all four rows) is a clean 1-on-1 number,
   and §1 already decided Elites are never fought clean.** An Elite embedded
   in a live Fodder/Soldier population won't have the retinue's full melee/
   archer commitment available at t=0 the way this table assumes — some of
   the army is tied up on the swarm around it. The real in-context TTK is
   longer than this table shows, plausibly by a lot; this table is a **lower
   bound**, not a prediction. Whoever builds the encounter-budget/procgen
   scope (§4 of `SYSTEMS.md`) needs a concurrent-spawn model to get the real
   number, and should treat "the Elite currently dies in ~2 swing cycles when
   fought clean" as the reason a clean 1-on-1 undersells the intended "stop
   recruiting, start promoting" gate (entity-tiers.md §6) — it isn't a call
   to raise Elite HP, since raising HP is the exact anti-pattern Design Law 3
   forbids for a mass-scale-affordability reason that doesn't even apply to a
   single promoted Actor; the fix is encounter design, not a stat.
2. **Boss TTK (8.01s–8.46s, once degrade is applied honestly) is
   raw-HP-only** — `entity-tiers.md` §3 explicitly scopes phase/mechanic
   design out of this stat block. A boss meant to read as "positioning +
   stances, not a DPS check" (`RTS-VERTICAL-SLICE.md` §5) dying to flat
   attrition in ~8 seconds, before any phase would even trigger, is a direct
   handoff to the Boss & elite design deliverable: it needs phase gates or
   damage-immune windows that don't reduce to more HP, or the "not a DPS
   check" identity doesn't survive contact with the numbers in front of it.
   (The previous draft's 13.69s recruit-max figure was wrong — the corrected,
   honestly-degraded number is 8.46s, a much narrower balanced/recruit-max
   gap than previously stated, though the underlying "too fast for phases"
   finding is unchanged.)

**So: task-002's finding is real and matters for the *game's* longer arc**
(a run that grows past ~120–150 units, which co-op's eventual return or a
longer single-player campaign will reach) — but for *this specific 3-floor
curve*, where army size tops out at 90, the acute risk isn't melee-cap
starvation, it's §3's headcount-vs-population gap in the swarm floors and the
too-fast Elite/Boss kills these tables expose. I'm not recommending a
recruit-weighting change or a multi-hitpoint boss for the slice on this
evidence — flagging task-002's `growth_source_weight` concern forward for
whichever content actually reaches N>120, which this curve doesn't.

---

## 5. Armor and floor multipliers — a deliberate non-interaction

**DECIDED: floor-to-floor scaling multiplies *count* and unlocks *tier*. It
never re-scales `Armor`, `MaxHP`, or `DPS` on an existing entity-tiers.json
row.** Two reasons, both load-bearing:

- **Design Law 2 (soft caps only) reads as a caution against numeric
  multiplier stacking, not just against hard caps.** entity-tiers.md's Armor
  mechanic already does exactly the asymmetric, promote-rewarding work a
  "per-floor Armor multiplier" would try to do — cutting cheap attackers far
  harder than invested ones — and it does it once, by being the *right stat*,
  not by being multiplied. Stacking a floor multiplier on top would blur that
  asymmetry (whose whole value is being a fixed, legible number a player can
  learn) into a moving target, and would functionally be a per-floor
  hard-to-read damage-reduction ratchet — the thing Law 2 rules out.
- **Introducing a new tier already delivers the "gate" effect Law 1 wants,
  without touching a number `entity-tiers.json` owns.** Floor 2's Elite
  arriving with Armor 12 *is* the difficulty spike; multiplying that Armor
  again per floor would be re-solving a problem the tier system already
  solves, and would make `entity-tiers.json`'s stat blocks a second source of
  truth alongside whatever multiplier table lived here. One source of truth
  for enemy stats stays in `entity-tiers.json`; this doc only ever touches
  *how many* and *which tiers*.

The corollary: floor 3's harder feel comes entirely from **more bodies +
armed composition + a second Elite + the Boss**, never from a Floor-3 Fodder
secretly hitting harder than a Floor-1 Fodder. That is Design Law 3 stated as
a literal implementation rule, not just a vibe.

---

## 6. Handoffs

**To whoever revises `SYSTEMS.md` §2 or §7 next.** §3's headcount gap is the
single most important finding in this doc — the locked population curve and
the decided economy numbers don't fit together as currently tuned, and
resolving it needs a decision only that revision can make (not this doc,
which was told not to edit either file).

**To whoever builds the "Encounter budget table per floor"
(`RTS-VERTICAL-SLICE.md` §4, still `[ ]`).** Two direct dependencies: (a) §3's
ratio model is a stand-in for real spawn-pacing data and should be replaced
once that table exists; (b) §4's Elite TTK numbers are a clean-fight lower
bound that needs a concurrent-spawn model to become trustworthy.

**To the "Boss & elite design" deliverable.** §4's raw-HP boss TTK
(8.01s–8.46s, once the recruit-max row's own degrade penalty is applied
honestly) is too fast for "positioning + stances, not a DPS check" to read as
intended — needs phase gates or damage-immune windows, not more HP. (This
range is corrected from an earlier draft's 7.8–13.7s, which used per-unit DPS
numbers that didn't trace to `entity-tiers.md` §2.2 — see §4 and §7. The
"too fast" finding itself is unchanged; only the magnitude and the
balanced-vs-recruit-max gap were wrong.)

**To whoever tunes `unit-types.json`'s `growth_source_weight` next, if a
longer/co-op run ever pushes army size past ~120.** task-002's melee-cap
finding is real at that scale; it just doesn't bind inside this 3-floor
curve's own army sizes (§4).

---

## 7. Simulation notes

**What was simulated:** originally, a scratch Python script (session
scratchpad, never committed, reproducible from `entity-tiers.json`,
`upgrades.json`, `unit-types.json`, `economy.json`, `growth-sites.json`)
computing (1) a sanity check against `entity-tiers.md` §7's own numbers, (2)
the two retinue-growth scenarios in §2 from the stated growth-site action
costs, (3) a discarded from-scratch swarm-vs-swarm attrition model, (4) a §4
Elite/Boss TTK table intended to use the same closed-form method
`entity-tiers.md` §7 already validated.

**Correction (2026-07-29, task-072): item (4) above did not actually use
that method, and §4's table has been replaced.** The scratch script's
implied per-unit Spearman DPS against the Elite (293.3 / 20 = 14.665) does
not match `entity-tiers.md` §2.2's own worked derivation for the same
matchup (16.667 DPS/unit), and doesn't trace to any committed source. §4 now
uses the committed harness (`Scripts/sim/scenario_runner.py`, run against
`docs/data/scenarios/floor2-elite-point-target*.json`, `floor3-elite-
point-target.json`, `floor3-boss-point-target*.json`) instead — it reproduces
`entity-tiers.md` §7's table exactly (`validate.py`'s bonus check, N=120
Elite, 1.848s vs the doc's stated 1.85s) and is the harness `docs/sim/
LIMITATIONS.md` §3 already documents as trustworthy for exactly this kind of
clean point-target comparison. The correction reverses §4's headline
conclusion (recruit-max wins the Elite matchup at both floors instead of
losing it) — see §4 for the full account. Items (1)–(3) below are unaffected
by this correction; they were not the source of the discrepancy.

**Sanity check (passed):** Militia vs Fodder TTK = 2.00s exactly, Hero(55dps)
vs Elite TTK = 21.60s — both reproduce `entity-tiers.md` §7's own stated
numbers before anything new here was trusted.

**The discarded swarm-vs-swarm model, and why.** A pooled two-sided Lanchester
attrition sim (each side's total DPS applied proportionally against the
other's total HP pool, armor-mitigated per matchup) predicted a **100%
retinue wipe on floor 1 in both scenarios** — 40 units against 250 population
lost with the enemy pool barely dented. That directly contradicts
`GATE1-FUN-PROTOTYPE.md`'s own *measured* result at the same population (110
of 120 survive). The model isn't a Fermi approximation of the real thing —
it's wrong in kind, because it applies the enemy's *entire* population's DPS
simultaneously with no concurrency or arrival-timing limit, and the real
combat model's survivability depends heavily on **spawn pacing over a wave**
(a variable this doc has no data for — `RTS-VERTICAL-SLICE.md` §4 lists the
per-floor encounter-budget table, which would carry that data, as still
unbuilt). Discarded rather than reported as a number; replaced with the §3
ratio-calibration approach, which uses only measured gate-1 data and states
its own limits plainly.

**The §4 Elite/Boss TTK numbers (now harness-run, not scratch-computed) are
trustworthy where the swarm sim wasn't**, because they don't need
spawn-pacing data — Elite/Boss are already-present, already-engaged point
targets with a known committed army at t=0, the same assumption
`entity-tiers.md` §7's own N=50/120/250 sweep already made, that `docs/sim/
LIMITATIONS.md` §3 confirms this class of comparison is trustworthy for, and
that this doc explicitly flags as a lower bound once the Elite is embedded in
a live swarm (§4, point 1).

**Assumptions stated plainly (none measured in-engine):**
- **Archer tier-scaling.** `unit-types.json` states Archer combat stats
  (70 HP / 18 DPS) once, not per-tier, while Spearmen inherit the Freed/
  Militia/Veteran ladder directly. This doc assumes Archer HP/DPS scale by
  the same ratio as the Spearmen tier they're notionally paired with (e.g., a
  Veteran Archer = 18 DPS × 45/30). `docs/design/squad-group-system.md`
  (task-046) owns the real type×tier product and hasn't resolved it yet —
  flagged, not decided here.
- **Growth-site spend patterns** (§2's two scenarios) are illustrative
  bounds, not a claim about optimal play — chosen to spend inside the stated
  ~33/~52 Ember arrival estimates (`growth-sites.json`) while representing a
  genuinely different lane emphasis (triangle-balanced vs. Recruit-only).
- **§3's ratio model** assumes survival rate is primarily a function of
  population/army ratio, calibrated at exactly 3 points (gate-1's 3 waves at
  fixed N=120). A 3-point calibration is thin; it's used here only to compare
  *relative* standing (is the new curve inside or outside the measured
  collapse band), not to predict an exact survival percentage for the new
  scenarios.
- **§4's Elite/Boss sim** carries forward every assumption `entity-tiers.md`
  §7 already stated as unmeasured (`SurroundCapEstimate` as a Fermi estimate,
  not a crowd-sim measurement).
