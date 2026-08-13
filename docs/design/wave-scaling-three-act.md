# Wave scaling — the three-act curve (early / mid / late)

**What this is:** a replacement wave shape for the 3-wave slice, reframing it as
early/mid/late game rather than a flat difficulty ramp — task-102, from the owner's
direct ask. **Extends** `docs/design/scaling-curve.md` §1 (its 85/15 → 55/25/20 →
45/25/30 composition table and Elite/Boss instance schedule, by reference — not
rewritten) and `docs/design/encounter-budget.md` (whose pulse/Lull pacing mechanism
this doc assumes as the eventual arrival schedule for whatever population numbers
land here, without re-deciding it). **Proposes superseding** `SYSTEMS.md` line 45
(§6 below) — that is the user's call, not this doc's.

**Does not change:** any `entity-tiers.json` stat block, any `unit-types.json`
combat/formation dial, `docs/data/economy.json` or `growth-sites.json` (task-101's
scope), or `docs/data/scenarios/**` (task-103's scope). This doc sets the *target
numbers* — starting retinue, enemy population, composition, ranged share, per wave —
not the pacing (pulse schedule), the economy mechanics that reach the retinue
numbers, or the runnable scenario fixtures.

---

## 0. The shape, up front

| Wave | Label | Retinue | Retinue mix | Enemy population | Enemy mix | Elites | Boss | Ratio | Total Mass entities | Headroom vs. 34k ceiling |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | **Early** | **60** | 100% Spearmen | **120** | 100% Fodder | 0 | 0 | 2.00 | 180 | 99.5% |
| 2 | **Mid** | **120** | 80% Spearmen / 20% Archers | **400** | 55% Fodder / 25% Soldier-melee / 20% Soldier-ranged | 1 | 0 | 3.33 | 520 | 98.5% |
| 3 | **Late** | **600** | 80% Spearmen / 20% Archers | **20,000** | 45% Fodder / 25% Soldier-melee / 30% Soldier-ranged | 3 | 1 | 33.3 | 20,600 | 39.4% |

Full machine-readable version: `docs/data/wave-scaling.json`, schema in
`docs/data/wave-scaling.schema.md`.

---

## 1. Retinue — count, composition, and how it gets there

**Early (60) is explicitly smaller than today's shipped 120** — the owner's stated
requirement. It is not an arbitrary cut: at 60, the wave-1 population/army ratio
(2.00) sits just under GATE1's own measured 250/120 = 2.08 "comfortable win"
calibration point (`GATE1-FUN-PROTOTYPE.md` §3b, `docs/sim/LIMITATIONS.md` §1 — read
directionally, not as a survivor prediction). Keeping the *old* 250 population
against a 60-count retinue would instead push the ratio to 4.17 — past the 450/120 =
3.75 point where GATE1's own curve is already a near-total wipe. A smaller early
retinue only reads as "early game, low stakes" if the population shrinks with it;
this doc cuts wave 1's population for exactly that reason, not as an independent
scope change (see §6 for what this means for the `SYSTEMS.md` lock).

**60 also happens to equal `economy.json`'s `supply.start_capacity` exactly**
(1 upkeep/unit, uniform, §7 of `SYSTEMS.md`). At 60 retinue, upkeep demand (60)
equals capacity (60) — `demand > capacity` never triggers, so wave 1 opens with
**no degrade multiplier**, unlike the current shipped combination where a 120-count
wave assumption fights the whole run already degraded against a 60-capacity economy.
**Flagging as instructed: this materially changes task-101's premise for wave 1 at
least** — the collision `scaling-curve.md` §3 and this task's own brief called out
(120 retinue vs. 60 capacity) doesn't exist at this doc's wave-1 number. It still
exists from wave 2 onward (120 and 600 retinue both exceed 60 capacity by a wide
margin) unless task-101 revises `start_capacity` or the Provision economy scales
with the wave — not this doc's call, surfaced for task-101 to read.

**Mid (120) returns to today's shipped flat number**, but its *composition*
changes — see §3. **Late (600)** is a proposal past anything `scaling-curve.json`'s
own simulated growth scenarios reach (balanced/recruit-max top out at 60–90 by
floor 3) — flagged in `wave-scaling.json`'s `wave_definitions.RetinueMechanism`
field rather than hidden. This doc does not invent a new economy to justify 600; it
states the *target* the eventual mechanism must reach.

**Mechanism — not resolved to one system, both are named as valid:**
- **Refill-to-rising-cap**: the minimal extension of what `Spike1GameMode.h` already
  does. `WaveBroodCounts` is already a `TArray<int32>`; `RetinueCap` (currently one
  flat `int32`) would become the same shape — `{60, 120, 600}` — and the existing
  refill-at-breather logic needs no other change. This is the cheapest path to the
  numbers in this doc and requires no economy redesign.
- **Growth-site purchase** (`SYSTEMS.md` §6/§7, task-101's scope): the intended
  fuller replacement, where the player earns toward these same target counts through
  Recruit/Promote rather than a flat refill. This doc does not choose between the
  two — it states the count each wave should reach; which system produces it is an
  implementation decision outside this doc's file boundary.
- **Carryover** (exact survivor count feeding the next wave) is not proposed here.
  `docs/sim/LIMITATIONS.md` §4 states plainly that no committed model chains wave
  survivors today; refill-to-cap is the shipped behavior and this doc keeps that
  shape, just with a cap that rises per wave instead of holding at 120.

**Tier**: Militia throughout (the "balance anchor," `SYSTEMS.md` §1) — this doc does
not model Promote-driven tier growth across waves; that is `economy.json`'s
triangle, not a wave-scaling decision. Stated as an assumption, not a finding.

**Archer split (80/20 at mid and late)** is `unit-types.json`'s
`growth_source_weight` (spearmen 0.8 / archers 0.2), cited directly, not invented.

---

## 2. Enemy population and composition, by tier

Extending `scaling-curve.md` §1's existing per-floor composition table (which this
doc's mid/late percentages intentionally echo — 55/25/20 and 45/25/30 are the exact
mid-floor and late-floor splits already decided there) onto this doc's different
population scale and wave framing:

| Wave | Fodder | Soldier-melee | Soldier-ranged | Total |
|---|---|---|---|---|
| 1 Early | 120 (100%) | 0 | 0 | 120 |
| 2 Mid | 220 (55%) | 100 (25%) | 80 (20%) | 400 |
| 3 Late | 9,000 (45%) | 5,000 (25%) | 6,000 (30%) | 20,000 |

**Deliberate divergence from `scaling-curve.md`'s own floor-1 row (85% Fodder / 15%
Soldier-melee)**: this doc's wave 1 is **100% Fodder**, introducing nothing but the
baseline tier. That is a direct consequence of the owner's "mid introduces unique
units that wave 1 does not have" requirement (§3) — if wave 1 already had
Soldier-melee, wave 2's "new unit" story would be ranged-only, not the full
melee+ranged+Elite reveal this doc gives it. Stated as an intentional choice, not an
oversight of the existing table.

No Titan tier is used at any wave, matching `entity-tiers.json`'s own scope note
("NOT built in the 3-floor vertical slice roster") — kept consistent rather than
introduced here without a stated reason.

---

## 3. What makes Mid "unique" — the wave's identity, not a stat bump

**Everything below is absent at wave 1 and present starting wave 2, on both sides,
simultaneously:**

| Side | New at wave 2 | Source |
|---|---|---|
| Retinue | **Archers** (ranged_line, 70 HP / 18 DPS / 750 engage range) | `unit-types.json` `types.archers` |
| Enemy | **Soldier-melee** (150 HP / 6 Armor / 42 DPS — "a tougher, harder-hitting Brood") | `entity-tiers.json` `brood_soldier_melee` |
| Enemy | **Soldier-ranged** (85 HP / 0 Armor / 26 DPS / 700 engage — "a glass ranged threat") | `entity-tiers.json` `brood_soldier_ranged` |
| Enemy (Actor) | **First Elite** (900 HP / 12 Armor / 65 DPS, `TargetsPerHit` 5, embedded in the live swarm per `scaling-curve.md` §1) | `entity-tiers.json` `brood_elite` |

This is the wave's identity by design, not incidental: wave 1 is a pure melee
scrum (fodder vs. spearmen, nothing else on the field); wave 2 is the moment ranged
combat, an armed enemy line, and the first individually-dangerous target all arrive
at once. **Ranged units are part of the test starting exactly here** — both the
retinue's own Archers and the enemy's Soldier-ranged are new simultaneously, so the
mid wave is the first place ranged-vs-ranged and ranged-vs-melee interactions can be
observed at all; wave 1 has none of that axis.

---

## 4. Late — the population, argued against the measured ceiling

**The number: 20,000 enemy Mass entities**, alongside 600 retinue — **20,600 total**,
against the measured **34,000-entity, 60fps, `Swarm.SimLOD.Stride 4` ceiling**
(`one-camera-bench.md` §8, run 5 — measured out to 40,000, not extrapolated: 30,000
→ 14.73ms, 40,000 → 19.06ms/52fps).

**Headroom: 13,400 entities, 39.4%.** Arithmetic:
```
34,000 (measured ceiling) − 20,600 (retinue 600 + enemy 20,000) = 13,400
13,400 / 34,000 = 39.41%
```

**What the headroom is reserved for, and what already fits inside the 20,600
itself:**
- **Elites and the Boss are NOT part of the 20,600** — they are `PromotedActor`s
  (`entity-tiers.json`'s own `ActorType` column), a different code path from the
  Mass-Entity sim this ceiling measures. 3 Elite + 1 Boss is 4 individual Actors;
  negligible against a 20,000-scale Mass budget, but their **per-actor tick/render
  cost is genuinely unmeasured** — flagged in §7, not assumed free.
- **Blood particles are architecturally bounded, not population-scaling**
  (`docs/perf/blood-particles.md` §1): `Blood.MaxBurstsPerFrame` (12) hard-caps new
  Niagara components per frame regardless of how many entities are alive. The
  measured cost (+0.7ms FrameTime / +1.4ms GameThreadTime) was taken at 820 total
  entities under heavy mobbing — already saturating that same cap. At 20,600
  entities the cap is unchanged, so cost should not exceed what's already measured
  by much, though it has **not been re-measured at this scale** — stated as an
  argument from the cap mechanism, not a new data point.
- **The 39.4% headroom itself is reserved for**: the gap between this bench's
  *static, settled* measurement and a real fight's *continuously arriving* wave
  (the bench note: "a real fight with waves continuously walking in from the dark
  has a permanently larger far-population than this benchmark does" — §5 of
  `one-camera-bench.md`), and for whatever pulse/Lull arrival schedule
  (`encounter-budget.md`'s mechanism) eventually paces this population in — which
  would only ever *lower* peak concurrency below the all-at-once 20,600 this doc's
  figure assumes, never raise it, since `Spike1GameMode::BeginWave()` today spawns
  a wave's entire population in one call.

**One load-bearing caveat: this design needs `Swarm.SimLOD.Stride 4` to hold.**
Against the **no-LOD** ceiling (~21,000, same doc §5/§6), headroom collapses to
**400 entities (1.9%)** — this wave is barely inside that ceiling at all. Stride 4
is already the shipped default (`one-camera-bench.md` §7, item 2, "done"), so this
is a stated dependency, not a new requirement — but it is the reason 20,600 is the
number here and not something closer to 34,000 itself: leaving margin against the
weaker of the two measured ceilings, not just the stronger one.

**Why 20,000 enemy and not more.** The task brief's own worked example ("if the
answer is 20,000, leaving 40% headroom...") independently lands within a point of
this doc's arithmetic (39.4% vs. 40%) — this doc arrived at 20,000 from the
ratio-and-ceiling reasoning above before checking that example, and the agreement is
a useful sanity check, not the reasoning itself. A rounder, larger number (e.g.
25,000) would still clear the 34k ceiling but would eat further into the no-LOD
1.9% margin and leave less room for the un-modeled "continuously arriving" gap
above; 20,000 was chosen as the largest round figure that keeps both margins
comfortable.

---

## 5. Ranged share, both sides

| Wave | Retinue Archers | Enemy Soldier-ranged |
|---|---|---|
| 1 Early | 0% | 0% |
| 2 Mid | 20% (24 of 120) | 20% (80 of 400) |
| 3 Late | 20% (120 of 600) | 30% (6,000 of 20,000) |

Retinue archer share holds flat at 20% (`unit-types.json`'s stated growth weight,
not re-derived per wave) from mid onward; enemy ranged share **widens** from 20% to
30% at late. That widening gap is deliberate, not an oversight — see §7's melee-cap
handoff: at retinue N=600, melee DPS against a point-target (Elite/Boss) is
geometrically capped (`entity-tiers.json` §4's `SurroundCapEstimate`) while ranged
output is not, so an enemy that leans harder into its own ranged share at this scale
raises symmetric pressure against exactly the axis (Archers) that still scales for
the retinue past that cap.

**Scope boundary, stated per the brief:** all of the above uses `EUnitType::Archers`
and `brood_soldier_ranged` exactly as they exist today — instant-hit at engage range,
`TargetsPerHit` 1, no travel time. There is no projectile system in the engine
(`grep -r Projectile ELVTR/Source` returns nothing), and this doc does not design one.
See §8 for the open question this leaves.

---

## 6. Canon proposals

**Supersedes `SYSTEMS.md` line 45** — the decision record reading:

> "One curve across 3 waves: **250 → 450 → 700 brood** (×1.8, ×1.56) — the gate-1
> values, now canon as 'the first act of the exponential fantasy'"

**Proposed replacement**: **120 → 400 → 20,000 enemy Mass entities** (×3.33, ×50),
alongside a retinue that now varies per wave (60 → 120 → 600) rather than holding
flat at 120 — an axis `SYSTEMS.md` §2 does not currently address at all. The
×1.8/×1.56 "smooth doubling" framing is explicitly retired by this proposal: the
late-wave jump is a full engine-ceiling argument (§4), not an incremental step, and
reads as a genuine act break rather than a continuation of the same curve.

**This is the user's call, not this doc's** — filed as a proposal per the task
brief's instruction, `SYSTEMS.md` itself untouched.

---

## 7. Handoffs

**To whoever revises `SYSTEMS.md` §2**, per §6 above: the supersession is ready to
adopt or reject as written.

**To task-101 (economy/supply reconciliation)**: wave 1's retinue (60) resolves the
120-vs-60-capacity collision at that wave specifically (§1); it reopens at wave 2
(120 retinue vs. 60 capacity) and returns much larger at wave 3 (600 retinue, an
economy-scale gap `scaling-curve.md` §3 already flagged and this doc's late number
makes considerably wider). Not resolved here — task-101's scope.

**To task-103 (scenario fixtures)**: `wave-scaling.json`'s tables are ready to turn
into runnable `docs/data/scenarios/**` fixtures. `wave3_late` is a `wave_attrition`
shape at a scale no existing scenario in that directory approaches (20,600 vs. the
largest existing fixture's few hundred) — worth flagging to task-103 directly, since
`docs/sim/LIMITATIONS.md` §1 already states the model can't reproduce even the
120-count GATE1 baseline; running it at 20,600 will not produce a trustworthy
survivor count and should not be read as one.

**To whoever builds `encounter-budget.md`'s next revision**: this doc's populations
are the input its pulse/Lull mechanism should schedule; peak on-screen concurrency
under a real pulse schedule will be lower than the all-at-once 20,600 this doc's
headroom arithmetic assumes (§4).

**To whoever next tunes `unit-types.json`'s `growth_source_weight` or
`entity-tiers.json`'s `SurroundCapEstimate`** (`scaling-curve.md` §6's own forward
flag): wave 3's 600-count retinue is the "army size past ~120" scenario that doc
named and didn't have content to point at yet — this doc is that content. Untested,
not resolved here.

**To performance-director**: 3 Elite + 1 Boss `PromotedActor` tick/render cost at
concurrent horde-scale (wave 3) is unmeasured; blood-particle cost at true 20,600-entity
density is argued from the `MaxBurstsPerFrame` cap, not re-measured (§4).

---

## 8. Open

- **Real projectile travel-time / miss model.** Out of scope per the brief — the
  ranged share above uses the existing instant-hit `Archers`/`brood_soldier_ranged`
  types only. If "ranged units are part of the test" is meant to eventually exercise
  actual projectile travel time, that is a separate mechanic and a separate task.
- **Elite/Boss per-actor cost at wave-3 concurrency** — unmeasured (§4, §7).
- **Blood-particle cost at true 20,600-entity density** — argued from the cap
  mechanism, not measured at this scale (§4).
- **Whether procgen arena geometry (`encounter-budget.md` §3) can host a
  20,000-population Arena at all**, or whether this population necessarily implies
  a pulse schedule finer-grained than the existing 2-3-pulse pattern — this doc sets
  the target population, not the room/pacing design that would host it.
- **Melee `SurroundCapEstimate` behavior at retinue N=600** — untested past N=250
  (`entity-tiers.md` §4's own sweep ceiling); §5's widening ranged-share gap is
  argued from the mechanism, not a measured outcome at this specific count.

---

## 9. Simulation notes

**What was simulated:** closed-form arithmetic only (ratios, sums, percentages,
headroom against the two measured perf ceilings) — same house method
`scaling-curve.md` §7 and `encounter-budget.md` §8 both already use for this class
of doc. No engine run, no wave-attrition survivor-count sim was attempted.

**Why no survivor-count sim was attempted:** `docs/sim/LIMITATIONS.md` §1 states
plainly that the wave-attrition harness cannot reproduce its own single measured
baseline (GATE1's 109-of-120 wave-1 survival) at the harness's committed defaults,
and the task brief explicitly instructs against writing a spec whose justification
depends on a survivor number nobody has yet. Every difficulty argument in this doc
(§1, §4) is stated as a **ratio comparison against GATE1's measured calibration
points**, read directionally per that same document's own stated limit, not as a
prediction for the new wave counts.

**What was checked, and how:** wave populations/ratios/headroom percentages
(§0's table) were computed directly from this doc's own stated counts —
`EnemyPopulation / RetinueStart` for ratio, `RetinueStart + EnemyPopulation` for
total Mass entities, and `(34000 − total) / 34000` for headroom. Verified by
independent arithmetic before writing; reproducible from `wave-scaling.json`
directly with no hidden inputs.

**Assumptions stated plainly (none measured in-engine):**
- All retinue rows are Militia tier throughout (§1) — this doc does not model
  Promote-driven tier growth across waves.
- The 80/20 Spearmen/Archers split at mid and late is `unit-types.json`'s stated
  `growth_source_weight`, not independently derived or re-tuned for this curve.
- The mid/late enemy composition percentages (55/25/20, 45/25/30) are carried over
  from `scaling-curve.md` §1's already-decided floor table rather than freshly
  derived — an explicit reuse, not a new finding.
- `TotalMassEntities`/headroom figures are worst-case, all-at-once numbers (§4,
  and `wave-scaling.schema.md`'s own stated convention) — a real pulsed encounter
  would show a lower peak, never a higher one.
