# Squad Group System — Typed Units, Ranged Combat, and the Unit Cam Views

**What this is:** the squad as a command entity **with a type** — spearmen and archers,
Total War–style, per the owner's own framing — how a unit is recruited, forms up, takes
casualties and is reinforced; the minimum viable ranged-combat model archers need; and
a reopened framing target that answers "a whole army view **and** a unit view, more
dynamic to selection" against a wider "map mode."
**Extends:** `GDD.md` §4 (stances), §7 (upkeep/degrade), §10 (Mass Entity constraints);
`CLASSES.md` §1 (Vanguard retinue); `SYSTEMS.md` §1 (entity tiers), §6 (retinue tuning).
Feeds a rewrite of `task-046` (sim implementation) and a follow-up to `task-045` (Army
View / Map Mode rendering).
**Data:** `docs/data/squads.json` + `.schema.md` (squad-allocation dials, revised),
`docs/data/unit-types.json` + `.schema.md` (new — per-type stat/formation/growth dials).

**Supersedes the 2026-07-27 revision of this file in full.** That revision (still
readable in git history) answered "a whole-army view and a unit view" for an
**undifferentiated** retinue — every squad the same kind of soldier, absorbing growth
evenly. The owner has since named the actual model they want, in their own words:

> "I want to bring up the reason why I asked for retinue to be turned into simpler
> units, much like how Total War uses and controls units. One group of small spearmen
> and other a group of archers."

That is not a variant of the old spec — it's a different premise. §3.1 of the prior
revision (`SquadTargetSize = ceil(AliveRetinue / MaxSquads)`, all 8 squads absorbing
growth evenly) is wrong under a typed model: a spearman unit does not gain archers
when you recruit. This document replaces that premise everywhere it appears.

---

## 0. Why now, in one paragraph

`USwarmSubsystem` already has the right shape for a **squad**: `MaxSquads` (8),
`SquadStanding[]`, a per-squad centroid and stance once §1.3's sticky-membership fix
lands. What it has never had is a **type** on that squad. Today's retinue is one
undifferentiated archetype — the only "archetype" in the sim is the Mass technical
composition, not a game concept — and there is no ranged combat at all: `Swarm.
RetinueDPS` fights at contact range, full stop. The owner's Total War framing collapses
two things this repo already has separate vocabulary for into one: **a squad is a
unit, and a unit has a type.** Once that's true, "spearmen and archers" is a data
difference (stats, formation, engage range), not a new command system.

**One thing worth flagging before the detail: splitting one pool into two makes the
existing 8-squad-handle ceiling bind *sooner*, not later** — §4.2 simulates this and
finds the model breaks at a lower total retinue (~730) than the old single-type
model (~640-750), and for a worse reason (Spearmen alone can exhaust the whole
budget, independent of how small the Archer pool is). Flagged here so it isn't missed
on a skim; the full finding is in §4.2.

---

## 1. Units are typed — the squad *is* the unit

### 1.0 Terminology: squad = unit, from here on

The prior revision used "squad" for a cosmetic/command bucket of otherwise-identical
soldiers. Total War's word for the same thing — a command group with its own type,
formation and orders — is **unit**. This document uses **unit** throughout for exactly
what the prior spec called a squad; `SquadId`, `MaxSquads`, `squads.json` and the
existing muster-card addressing (§3) all keep their names in code and data, because
renaming live CVars and a shipped data file is churn with no design payoff — but they
now name a **typed** thing, and the design vocabulary is "unit" going forward.

### 1.1 v1 roster: exactly two types — Spearmen and Archers

**Not a menu — this is the recommendation.** The owner named these two by example;
holding v1 to exactly them (not "melee/ranged/support" or a longer ladder) is a direct
application of Design Law 5 (Mass Entity constraints: fodder/soldier units get
data-cheap, shared-per-type behavior; only elites/titans/bosses get individual
special-casing) and Design Law 2 (soft caps, not scope creep dressed as caps) — two
types is the smallest set that proves the typed-unit model end to end (recruit, form,
fight, command, render) without pre-building a roster of types that don't have a
fiction yet. A third type is additive later (the type dispatch in every system below
is already N-way, not hardcoded to 2 — see §1.7, §3, §2's K and §4.1's clamp), not a
redesign.

**Why these two, not e.g. a shield-and-pike split:** they are the two combat *shapes*
the sim currently has anything to say about — melee (contact range, already built) and
ranged (nothing built, §2 specs the minimum). Everything else is flavor on top of one
of those two shapes. Spearmen are mechanically **today's retinue, renamed and
formalized as a type** — same stats, same formation defaults, same stance reflavors
already in `CLASSES.md` §1 (Advance the Line / Shield Wall / To the Banner). Archers
are new.

**Relationship to `CLASSES.md`'s Pathfinder ("the few," ranged, named pack):** flagged
explicitly so nobody reads these as redundant. Vanguard archers are **anonymous,
mass-produced auxiliaries in ranks** — the same "your army is what you save" liberated-
militia fiction as spearmen, just armed differently — high count, disciplined,
faceless. Pathfinder's pack is **low count, elite, individually named and mourned**. A
mass archer line and a six-hound hunting pack are opposite corners of the count-vs-
quality space `CLASSES.md`'s roster overview already defines; they read as different
games on purpose. Worth one sentence in `CLASSES.md` so a future reader doesn't merge
them (§9 Canon proposals).

### 1.2 What a unit owns (per unit slot, ≤`MaxSquads` records — extends the prior revision's §1.1 table)

| Owned by the unit | Prior revision | This revision |
|---|---|---|
| Standing count | `SquadStanding[8]` | unchanged |
| **Type** (Spearmen / Archers) | didn't exist | **new — assigned once at recruit time, permanent** (§1.4) |
| Stance (Follow/Charge/Hold/Rally) | promoted to `SquadStance[8]` (unbuilt) | unchanged proposal, now **interpreted per (type, stance)** — §1.8 |
| Stance anchor | `SquadStanceAnchor[8]` (unbuilt) | unchanged |
| Centroid | `SquadCentroidSum[8]`/count (unbuilt) | unchanged |
| Formation shape / dense slot packing | one retinue-wide dense repack | **one dense repack PER TYPE**, not per unit — see §8; a type's units still share one dense index space that subdivides into unit-sized chunks exactly as the prior revision's per-squad repack argument described, just scoped to the type's own pool instead of the whole retinue |

None of this is per-soldier data — same cost class as the prior revision's table, see
§8.

### 1.3 The sticky-`SquadId` finding — carried forward, now load-bearing for Type too

**Unchanged from the prior revision, still true, still verified against the code by
the lead:** `SquadIdForSlot(Slot) = Slot / SquadTargetSize` derives a unit ID from the
dense, formation-repack slot index, and that index silently renumbers on *any*
casualty anywhere in the retinue (`NeedsFormationRepack` fires on any death). A soldier
in unit 4 can become unit 3 by pure arithmetic the moment someone in a *different* unit
dies. That was already broken once a unit carries an independent stance; it is now
**also** broken for type, because a soldier reassigned from unit 4 to unit 3 by a
repack must not silently become the *other type's* soldier.

**Fix, unchanged in mechanism, now carrying one more field:** `SquadId` — and now
**Type** — are assigned **once**, at recruit time, and persist for that soldier's
lifetime regardless of what repacking happens elsewhere. A spearman does not become an
archer through a repack, a promotion, or a reinforcement wave; type is fixed at
recruitment (§1.4) the same way `SquadId` is. Dense repacking still exists for
*formation shape*, but now happens **per type, among that type's own units only** —
see §4.

### 1.4 Recruitment

Growth stays `CLASSES.md`'s existing Vanguard verb, **Rescue & Rally** — this spec does
not add a second growth mechanic. What's new is that a rescue/growth site now yields a
**type**.

**v1 recommendation: generator-tagged, not a player choice.** Each growth site the
procgen layer places is tagged with which type it yields (`docs/data/unit-types.json`'s
`growth_source_weight`, proposed **Spearmen 0.8 / Archers 0.2** — see §7's simulation
for why that ratio, not 50/50, keeps the model legible longer). This matches the
existing GDD §9 scope guardrail ("flags, not simulation" — a site's yield is a
generator-time tag, not a live economy) and doesn't require a new decision-event UI. A
**player-choice-at-rescue** version ("arm them with spears or bows") is a real,
better-feeling alternative and fits GDD §6's decision system cleanly, but it's a UI
surface this spec doesn't have a green light to design — flagged as a Canon proposal
(§9), not built here.

Newly recruited soldiers are assigned to the **least-full existing unit of their type**
(fill-lowest-first, same rule as the prior revision's membership policy, now scoped per
type) up to that type's current target size (§4); a new unit of that type is only
created once the type's derived unit count (§4) grows.

### 1.5 Casualties, degradation, and wipe

Casualties work exactly as today per-soldier — the combat pass kills individual
entities, `SquadStanding` (now per typed unit) drops. **Type never changes through
combat.** Upkeep-driven degradation (GDD §7, unbuilt) applies per soldier regardless of
type; nothing here proposes a type-specific upkeep rule.

**A unit hitting zero doesn't leave a ghost slot.** Because unit *count per type* is
**derived** each formation repack from that type's live pool (§4.1's
`ceil(pool / ceiling)` formula), a wiped unit's contribution to that formula simply
drops out at the next repack — the type's unit count recomputes smaller on its own,
with no separate "is this slot empty" bookkeeping. This is a direct benefit of keeping
allocation formula-driven instead of slot-owned, and it's the same mechanism that made
the prior revision's growth formula elegant, now reused for shrinkage.

**Veteran promotion (`CLASSES.md` §1) crosses with type, not instead of it.** A
Freed→Militia→Veteran→Bannerman promotion ladder applies within either type — "Veteran
Archer" and "Veteran Spearman" both make sense, quality and type are orthogonal axes.
`CLASSES.md` doesn't currently say this explicitly; flagged in §9.

### 1.6 Reinforcement

Reinforcement (wave-breather refill, `Spike1GameMode`'s existing refill-to-cap
behavior) refills **within** each type's existing units first (§1.4's fill-lowest-first
rule), and only grows a type's *unit count* once its derived count (§4) rises past what
today's units already cover. Growth is absorbed **within a type**, not spread evenly
across all 8 handles — the concrete way this revision replaces the old "all 8 squads
absorb growth evenly" rule (§4 has the full mechanism and its own breaking point).

### 1.7 Per-type formation defaults

Reuses `SwarmFormation`'s existing vocabulary exactly, per the brief — `Shape /
Columns / Spacing / RankSpacing / Forward / Arc*` — **no parallel vocabulary invented.**
A spear block and an archer line are not the same shape and must not stand in the same
place: Total War doesn't put its bowmen in the front rank, and neither should this.

**Mechanism, following the precedent `task-047` already set:** `Swarm.BroodFormation.*`
already coexists with `Swarm.Formation.*` as a second, independently-tunable CVar set
sharing the same `FParams` field names, specifically because a formation is a **live
dial while the game runs**, not a spawn-time constant (`SwarmFormation.h`'s own stated
reason for being a header full of pure functions instead of baked data). This spec
proposes the identical pattern one more time: `Swarm.Formation.Spearmen.*` and
`Swarm.Formation.Archers.*`, two independent `FParams` read by
`SwarmFormation::ReadParamsForType(EUnitType)`, replacing the single global
`ReadParams()` retinue-wide call. `unit-types.json`'s `formation` block records the
**shipped CVar defaults**, exactly the role `squads.json` already plays for the
allocation dials — not a runtime-read table (consistent with `SwarmExecOnPlay.txt`
already being the actual default-override mechanism for combat tuning).

| Field | Spearmen (v1 default) | Archers (v1 default) | Why |
|---|---|---|---|
| `Shape` | Block | Block | Both read as ranks; Arc/Wedge are available later per-type without new plumbing. |
| `Columns` | 12 | 20 | Archers form a wide, shallow line (more frontage, few ranks) — the readable "line behind the wall" silhouette. |
| `Spacing` | 42.4 (today's tuned value, unchanged) | 55 | Slightly looser — a firing line doesn't need shoulder-to-shoulder density. |
| `RankSpacing` | 110 (unchanged) | 70 | Shallower depth; an archer line is 1-2 ranks deep at v1 counts, not stacked. |
| `Forward` | 250 | 40 | **The load-bearing number.** Spearmen push out ahead of the bearer (toward the enemy); archers sit close to the bearer, well short of the spear line — the spear block physically screens them. |

**These are placeholder prototype dials, unmeasured** — same epistemic status as every
other number in this file (§11). The one number worth defending on reasoning alone is
the `Forward` gap (250 vs 40): it's what makes "archers behind spearmen" true on the
ground rather than just true in a data file, and it's cheap to get wrong safely (it's a
live CVar, not a spawn-baked property — see the table above).

### 1.8 Per-type stance reflavor

`CLASSES.md` §1 already reflavors the four verbs for Vanguard-as-a-whole (Advance the
Line / Shield Wall / To the Banner). Under typed units those apply to **Spearmen**
unchanged — they *are* what `CLASSES.md` was describing. **Archers need their own
reflavor**, because "charge" cannot mean "run into melee" for a unit that must never
close to melee range (§2 explains why mechanically):

| Verb | Spearmen (unchanged, `CLASSES.md` §1) | Archers (new) |
|---|---|---|
| Follow *(default)* | escort at `FollowEngageRange` (250uu), engage what comes close | hold formation, engage anything inside their own `EngageRange` (§2) **without closing distance** — a ranged unit's "default" is already "stand and shoot" |
| Charge → | Advance the Line: ranks push forward as a wall, reduced damage while advancing | **Volley Advance**: hold ground, but a temporary rate/cleave bump (not movement) — the "aggressive" read without breaking the never-melee rule |
| Hold → | Shield Wall: anchors, becomes a positioning tool where geometry helps (unchanged) | **Loose from Cover**: anchors identically; barely distinguishable from Follow *behaviorally* since archers are already static shooters — the value is in **addressing** one unit to hold a firing position while others move |
| Rally → | To the Banner: collapses on the banner/hero | **Fall Back**: collapses behind the nearest Spearmen unit if one exists (falls back to the hero otherwise) — archers retreating behind the shield wall, thematically exact |

**Combat targeting engine, reused, not rebuilt:** every per-type-stance engage range
above plugs into the *same* `EngageRange`-per-(type, stance) lookup the combat pass
already needs for §2's ranged model — this is a 2D table read instead of today's 1D
one (stance only), still O(1) per unit, still zero cross-entity writes, still compliant
with Design Law 5.

---

## 2. Ranged combat — minimum viable model, and the recommendation

Archers need it; it does not exist; the brief asked for an honest size estimate and a
recommendation, not a menu.

### 2.1 What's missing today

`Swarm.RetinueDPS` fights entirely at contact range (`MeleeRange`, ~95uu) via a
grid-based, victim-pull, geometric-targeting model: a striker publishes its squared
distance to its Kth-nearest enemy (`Swarm.TargetsPerHit`), a victim inside that radius
claims the blow (`BlowsClaimed`, capped). There is no concept of "in range but not
adjacent," no projectile, no line of fire.

### 2.2 The minimum viable model — reuse the existing radius, don't build a new system

**The core finding this recommendation rests on: the existing combat mechanism already
generalizes to range almost for free.** `StrikeReachSq` is *just* "how far this
attacker's blow reaches" — nothing about the grid, the victim-pull rule, or
`BlowsClaimed`'s conservation-of-damage guarantee assumes that radius is small. Giving
Archers a much larger `EngageRange` (**750uu** proposed, `unit-types.json`'s per-type
combat block, comfortably inside `CombatScan`'s existing 1200uu so the Unit Cam's
auto-look already "sees" archer fights) and reading it through the exact same combat
pass **is** a minimum viable ranged model:

- **No physical projectile entities.** Consistent with Design Law 5 — a projectile-
  per-shot would be a new per-unit-uniqueness cost horde scale can't afford. A blow
  still lands on the same discrete swing cadence (`SwingInterval`) everyone else
  already uses; it just lands from further away.
- **"Volleys vs. individual projectiles" — answered: neither, mechanically.** The
  *visual* is a volley (a cheap arcing Niagara trail or ribbon triggered on
  `SwingBit`, purely cosmetic, no gameplay state, sized and owned by whoever builds the
  render bridge — flagged to performance-director in §8, not specced here). The
  *simulation* stays one blow, one victim, same as melee — there is no discrete
  "volley event" object.
- **Line of fire crossing the front rank — no true occlusion, one cheap mitigation.**
  True line-of-sight against a moving horde of hundreds is exactly the kind of
  per-unit special-casing Design Law 5 rules out — not attempted. The proposed
  mitigation is a **`MinEngageRange`** (150uu, just past `MeleeRange`'s 95uu): an
  archer will not target anything already tangled in melee range of *anyone*, so it
  can't visibly "shoot into" its own scrum, even though it still can't tell whether a
  *specific* ally is standing between it and a *specific* target at range. This is an
  approximation, stated plainly as one, not a physically accurate LOS system.
- **No enemy ranged combat in v1**, and therefore **no incoming-projectile telegraph
  requirement** (Design Law 6 would otherwise demand one) — brood stay melee-only
  (§11's brood-typing assumption, answered no for v1).

### 2.3 Size estimate and recommendation

**Recommendation: ship the minimum viable model above in v1. Do not stub archers.** A
stub that behaves identically to a spearman (no range) doesn't deliver what the owner
asked for — the entire point of naming "spearmen and archers" was two unit types that
*fight differently*, and a stub fails that test even though it satisfies "archers
exist" on paper.

**Why the size estimate is smaller than it looks at first:** most of what §1 already
specs — a second formation block, a second stance reflavor, per-type stats, per-type
recruitment — is **required regardless of whether archers use ranged or melee
combat**, because Total War-style typed units need their own formation and orders even
if they fought identically. The cost *specifically attributable to ranged combat*,
once §1's typed-unit plumbing exists anyway, is:
- one new tunable (`EngageRange`/`MinEngageRange` per type) read by the existing
  combat pass — no new pass, no new fragment;
- one cosmetic VFX cue (the volley arc) — render-only, zero sim cost, sized by
  performance-director, not gated on this spec;
- the `TargetsPerHit`-per-type split (Archers proposed at 1 — precise single-target
  volleys, not cleave — so mass archers don't trivialize hordes with free cleave they
  didn't earn through positioning the way Spearmen's `TargetsPerHit=8` does).

**What is explicitly NOT v1, and is the genuinely large future scope:** physically
simulated arrow travel time (kiting/dodge play), true line-of-sight occlusion by
allies or terrain, friendly fire, discrete grouped volley timing (units firing in
disciplined ranks rather than independent swing clocks), and any ranged enemy. That is
the "bigger than everything else combined" version the brief warned about — it's real,
it's just not what's needed to answer "spearmen and archers" honestly for v1.

---

## 3. Order flow and the micromanagement answer

**Unchanged in substance from the prior revision** — still exactly the four stance
verbs, still addressed to "all units" (default) or one named unit (muster card /
hotkey 1-8), still no individual-soldier selection, still no player-driven roster
reassignment. Reworded only to say **unit** instead of squad and to note that an order
to "all units" now fans out across **both types**, each interpreting the same verb per
its own §1.8 reflavor — a player issuing Charge to everyone gets Spearmen advancing the
line *and* Archers volley-advancing, simultaneously, from one button press. That
composability — one verb, N type-specific readings — is the actual payoff of keeping
the vocabulary at four verbs instead of growing a type-specific command set.

```
Player issues a stance verb (Follow/Charge/Hold/Rally)
  -> targeting an ADDRESS: "All units" (default) or "Unit N" (explicit)
  -> writes UnitStance[addr] + UnitStanceAnchor[addr]
       ("All" writes every slot, both types — unchanged from today's only behavior)
  -> soldier steering reads UnitStance[soldier.SquadId] / UnitStanceAnchor[soldier.SquadId]
       AND the soldier's own fixed Type (§1.3) to resolve which §1.8 row applies
```

---

## 4. Scaling behavior — typed unit allocation

### 4.1 The old formula breaks under two types; this is its replacement

The prior revision's `SquadTargetSize = ceil(AliveRetinue / MaxSquads)` assumed one
pool. Under two typed pools, `MaxSquads` (8, unchanged — still the RTS-control-group-
style, not-a-power-cap handle budget from Design Law 2) has to be **split between
types**, and a proportional-by-headcount split produces bad results immediately: at an
80/20 spearmen/archer split and retinue 800, a naive headcount-proportional rule hands
Archers a single ~700-body "unit" that blows through the 80-body legibility ceiling by
nearly 9x while Spearmen sit comfortably under it.

**Proposed formula, per type:**

```
WantedUnits(type) = ceil(Pool(type) / SquadSizeLegibilityCeiling)   [ceiling = 80, unchanged]
Units(Spearmen)    = max(1, WantedUnits(Spearmen)) if Pool(Spearmen) > 0 else 0
Units(Archers)     = clamp(max(1, WantedUnits(Archers)), 0, MaxSquads - Units(Spearmen)) if Pool(Archers) > 0 else 0
```

**Spearmen claim first, in that order, deliberately — not arbitrarily.** Spearmen are
the class's primary identity (`CLASSES.md`: "high count, disciplined," the many);
Archers are the smaller complement (§1.4's 80/20 growth-source split matches this).
Recomputed every formation repack, same cadence as today. If Archers' derived want
exceeds what's left of the 8-slot budget, the overflow folds into Archers' own existing
units (the same "overflow folds into the last unit" behavior the pre-formula
single-type model used as its *only* rule — here it's the *fallback* once the fair-share
formula is exhausted, not the default, so the single-type flaw the ceiling formula
fixed doesn't reappear for the common case).

### 4.2 Where this breaks — simulated, and it breaks sooner than the single-type model, for a new reason

**Simulated** (scratch Python, not committed): extended the prior revision's growth
trajectory (start retinue 40, +25%/site placeholder growth, 20 sites — same
unmeasured-but-consistent assumptions) to a two-pool split at 80/20.

| Retinue (approx) | Spearmen pool | Spearmen units | Archer pool | Archer units | State |
|---|---|---|---|---|---|
| 373 (site 10) | 298 | 4 (75/unit) | 75 | 1 (75/unit) | clean |
| 582 (site 12) | 466 | 6 (78/unit) | 116 | 2 (58/unit) | clean, both types under ceiling |
| **728 (site 13)** | **582** | **8 (73/unit)** | **146** | **1 (146/unit) — folded, over ceiling** | **breaks** |

**Headline result: the model holds cleanly to roughly the same retinue ≈600-750 the
single-type model found — but breaks for a different, worse reason.** In the old
model, one type's growth eventually outpaces 8 units evenly splitting it. In this
model, **Spearmen alone consume the entire 8-unit budget once their own pool passes
~580** (independent of Archers entirely — verified separately: `ceil(580/80) = 8`),
at which point Archers are force-folded into a single oversized unit regardless of how
small the Archer pool actually is. **This is a cross-type competition failure, not a
single-type growth failure, and it happens at a lower total retinue than the old
ceiling once a second type exists** (~730 vs. the old ~640-750, because Spearmen now
have to share the budget). Say this plainly to the owner: **the two-type split makes
the 8-handle ceiling bind *sooner*, not later** — the same soon-to-exist bigger-retinue
stage that was already flagged as the real test of the single-type ceiling is now the
real test of this one too, with a lower bar to clear.

### 4.3 What doesn't change

`MaxSquads` stays 8 (Assumption 10, §11) for the same reasons as before — the muster
HUD's 4-card-per-wing layout, and Design Law 2's "bound the handles, not the power."
Raising it, or reserving a *guaranteed minimum* per type instead of a fold-on-overflow
policy, is the natural next lever if §4.2's ~730 ceiling proves too low in the bigger
stage — flagged, not solved here.

---

## 5. The framing target — three tiers, not two

**Reopened again, owner ask 2026-07-27:** *"a whole army view option and a unit
option... more dynamic to selection... I need MINI RETINUE UNITS by default, not
abstract blocks."* This is a real reversal of the prior revision's §4.1 recommendation
(aggregate blocks), and it deserves the same treatment that recommendation got: check
it against the lens geometry, say plainly what's possible, and say what gives way.

### 5.1 The finding this section turns on — projection method doesn't rescue legibility, pixel budget does

**Simulated** (scratch Python, not committed): the prior revision's §6.2 finding
(individual billboards at leash-covering distance draw 6.1-12.6px, illegible) was
computed for a **perspective, 1/depth** virtual camera. The obvious question the
owner's ask raises is whether an **orthographic** (fixed world-to-pixel scale, no
depth falloff) top-down layout — closer to what "mini retinue units" usually means in
an RTS minimap — escapes that limit.

**It does not.** Re-derived at the same panel sizes (405px / 837px) and the same
worst-case leash-bound spread (4000uu diameter): orthographic drawn sprite height is
**6.08px / 12.55px** — matching the perspective numbers (6.1 / 12.6) almost exactly.
This isn't a coincidence: both are the same underlying ratio, `panel_px × unit_size /
world_diameter`; a camera's projection method changes *how* that ratio gets computed,
not what it evaluates to. **The binding constraint is pixel budget vs. world coverage,
full stop — not perspective vs. orthographic, and not (as the prior revision already
correctly found) headcount.** Switching projection method alone cannot deliver
legible mini-units at the compact panel's current size.

**What *does* change the answer: more pixels.** Solving `panel_px = target_height ×
4000 / 60` (60uu = the sprite's own world height, `2×UnitHalf×SoldierScale`) for a
few legibility targets:

| Target sprite height | Panel height needed (orthographic, full leash coverage) |
|---|---|
| 10px (barely a fleck) | 667px |
| 12px | 800px |
| 15px (the prior revision's own "usable" floor, §6.2) | 1000px |
| 20px (comfortably readable) | 1333px |

At a **1200px** panel — roughly double the compact panel's current large-headcount
size (837px) and nearly 3x its small-headcount size (405px) — a formation's own
in-rank spacing (42.4uu default) draws at ~13px between neighbors and a soldier
sprite draws at ~18px tall: workably legible, distinct little units, not a fleck-crowd
and not a rectangle.

### 5.2 The reconciliation — what gives way

Per the brief: say plainly, propose which gives way, don't quietly reverse and don't
quietly dig in.

**The compact embedded Unit Cam panel (the HUD band's default, sized 405-837px by
today's `SizeMin`/`SizeMax`) cannot show legible individual mini-units across the
whole leash-bound army, at any zoom, under any projection method.** That's not a taste
call this spec is making — it's the geometry, checked twice now (§5.1). **What gives
way is the compact panel's literal fidelity to "mini units."** It keeps the prior
revision's §4.1 aggregate-block representation, unchanged: ≤8 typed, tinted, labelled
blocks, still the cheapest and most legible thing that panel can show, still the
resting-state default when nothing is selected.

**What answers "mini retinue units by default" is a third view this spec adds: Map
Mode.** Not a stretch goal bolted onto Army View — the actual deliverable that
satisfies the owner's ask, because it's the only surface with the pixel budget to do
it honestly (§5.1's table). See §5.5.

**This is a real interpretive call, not an obvious reading of "by default"** — flagged
explicitly as Assumption 8, §11, because "by default" could instead mean "the compact
panel's resting state should just accept the legibility hit and show real mini-units
anyway." That alternative is technically buildable (draw real per-soldier orthographic
icons in the existing compact panel, accept 6-13px sprites) — it is not recommended,
because it reproduces exactly the "fleck, not a soldier" failure the prior revision
already measured and rejected once, just via a different code path.

### 5.3 Unit/Squad View — unchanged in substance

Same target as both prior drafts: frame the **selected** unit (§1's typed units, §5.6
selection), individual per-soldier billboards, same fraction/floor table:

| Panel | Fraction of the selected unit in frame | Body floor | Sacrifices first | Protects |
|---|---|---|---|---|
| **UnitCamProjector** (close-up) | ≥ **60%** | **6** | proximity to the bearer | unit cohesion |
| **ViewFeed** (wide/real capture) | ≥ **80%** | **6** | proximity to the bearer | unit cohesion |

No change to the mechanism (`FrameFraction`/`FrameFloor` pull-back, Design Law 6 over
Design Law 4 scoped to this one panel). The only new wrinkle: a selected unit now
draws its own type's sprite once that art exists (§10) — nothing in the framing math
changes because of type.

### 5.4 Army View — unchanged mechanism, now typed

Exactly the prior revision's §4.1: ≤8 aggregate blocks, positioned at real centroids
(today still the disclosed placeholder ring — `SquadCentroidSum` remains unbuilt, see
`task-045`'s own honesty note), sized by `standing / target-size-for-that-type`, tinted
by stance. **New:** each block also carries its **type** — a label prefix (`S·34` /
`A·12`) or a distinct silhouette/icon per type, since "which blocks are my spearmen and
which are my archers" is now a real question Army View has to answer, where it wasn't
before. This is a small rendering addition (one more field per block, still O(8)
draws), not a new representation — flagged to whoever builds it, not specced pixel-
exact here (art brief territory, §10).

### 5.5 Map Mode — new

**What it's for:** the wider, on-demand surface the brief asked for — more information
and more options than the tight, always-on Unit Cam band. It is where the owner's
"mini retinue units by default" is actually delivered, and where several already-known
gaps have a natural home:

- **Real per-unit positions**, both types, orthographic top-down, at a fixed
  world-to-pixel scale sized to cover the full leash-bound area (§5.1's ~1000-1300px
  panel) — no camera pull-back math needed at all (that complexity is exactly what
  perspective projection required and orthographic doesn't).
- **Typed rendering**: spearmen and archers read as visually distinct clusters by
  shape/silhouette once that art exists — squads no longer need an abstract block to
  communicate "this is a different kind of unit," because typed mini-sprites do that
  for free (a genuine synergy with §1, not a coincidence — typed units make Map Mode's
  case *stronger*, since an abstract block was already going to need extra iconography
  to show type that real sprites don't need).
- **Selection surface**: clicking a cluster selects that unit — the *same* action as
  a muster-card click or hotkey (§5.6), so Map Mode is a second input surface for the
  identical command, not a separate targeting system.
- **The leash radius — an opportunity flagged here, not delivered by this spec:**
  Map Mode is the natural place to finally answer `FLAME-FOUNDATION.md` §3a's
  still-open ask ("the leash becomes visible... rendered as the lit floor") and
  `GATE1-FUN-PROTOTYPE.md`'s known gap ("leash warning has no visual"). Drawing the
  2000uu leash ring under the real unit layout is close to free once Map Mode's
  world-to-pixel scale already exists — **flagged as a natural fit, not claimed as
  solved here**; it's a rendering task for whoever builds Map Mode, not a new
  mechanic this spec is proposing.
- **Enemy read**: brood positions in the same orthographic layout, at least as
  clustered blips (typed or not, per §11's brood-typing assumption — untyped for now).

**Deployment:** a distinct, larger overlay (think: a toggled full/near-full-screen
surface, not squeezed into the HUD band) — orthogonal to the compact panel's own
one-panel/two-panel open question (§5.7 carries that question forward unchanged; Map
Mode doesn't resolve it, it adds a third surface alongside whichever answer that
question gets).

**Cost, honestly:** this gives up the compact panel's O(8) "strictly cheaper than
today" guarantee — Map Mode draws real per-unit positions, so it's an O(N) pass, same
complexity class as today's Unit/Squad View walk (not a new asymptotic class), and
cheaper per-unit than that view's atlas-brush draw if Map Mode's icons are flat
colored dots rather than sprite frames (§8).

### 5.6 Selection responsiveness — unchanged, now "unit" not "squad," now includes Map Mode as an input surface

Everything in the prior revision's §4.3 carries forward unchanged in mechanism —
same-action select-for-command-and-camera, `VInterpTo` travel at speed 10 (not a hard
cut), latch not idle-decay, Army View as the resting default, explicit selection
beating the auto-derived "most engaged" fallback, drop-to-Army-View on a wiped
selection, cast-focus outranking everything. The only addition: **Map Mode's cluster
click is the same selection action** as the muster card / hotkey (§5.5) — one more
input surface feeding the same `SelectedSquad` state, not a parallel selection model.

### 5.7 One panel or two — carried forward unchanged, still open

`Emberkeep.UI.ViewCam` still defaults to 0; the two-panel-vs-one-panel-mode-switch
question from the prior revision's §4.4 is **unresolved by this revision and not
reopened** — Map Mode sits alongside whichever answer that question gets, per §5.5.

---

## 6. Yaw discipline — carried forward, unchanged

**Unchanged from the prior revision's §5, kept verbatim in substance because it is
still true and unaffected by typed units or Map Mode.** In default Hero-focus,
`Outward = FocusPos - HeroPos` is zero (focus == hero position), so `AutoLook 2`'s yaw
is driven entirely by the enemy-cluster centroid inside `CombatScan`, which can swing
through wide arcs with damping (`LookLerp`) but no clamp — the reported "swings
left/right." Fix: base heading = bearer→selected-unit-centroid (falls back to the
bearer's last movement heading when degenerate), clamped to **±30°**, `LookLerp`
**1.5**, easing back to **0° offset** when nothing is pulling. Scope: **Unit/Squad
View's perspective virtual camera only** — Army View and Map Mode are both fixed
top-down layouts with no perspective camera and no yaw concept, unaffected. `task-045`
already implemented this envelope (`UnitCamDirector.cpp`: `YawClampDeg` 30, `LookLerp`
1.5, `SelectSpeed` 10) against the prior revision's spec of it; nothing here changes
those values or their meaning, only what "selected" now selects (a typed unit instead
of an undifferentiated squad).

---

## 7. Simulation notes

1. **Squad-size trajectory, single-type (prior revision, unaffected):** unchanged,
   retained in history — see the prior revision's §6.1 for the single-pool numbers.
2. **Army View lens geometry, perspective (prior revision, unaffected):** unchanged,
   retained in history — see the prior revision's §6.2.
3. **Orthographic re-derivation (§5.1, new this revision):** confirms projection
   method does not change Army View's legibility ceiling — 6.08px/12.55px orthographic
   vs. 6.1px/12.6px perspective at the same panel sizes and worst-case spread. Panel-
   size-vs-legibility table computed from the same constants (`UnitHalf` 40uu,
   `SoldierScale` 0.75, leash-bound 4000uu diameter). **Not measured against a real
   play session** — same status as every geometry number in this file; the qualitative
   conclusion (pixel budget, not projection method) is robust, the exact px/uu
   crossings are estimates.
4. **Typed-unit allocation trajectory (§4.2, new this revision):** extended the prior
   revision's growth sim (start 40, +25%/site placeholder, 20 sites — same unmeasured
   placeholder growth rate, explicitly not real economy data) to an 80/20 Spearmen/
   Archer split. Headline: the model holds cleanly to retinue ≈580-730, breaking
   *sooner* than the single-type ~640-750 ceiling because Spearmen alone can consume
   the full 8-unit budget independent of Archers. **All growth-rate, split-ratio, and
   ceiling assumptions here are placeholders**, same status as `economy.json` and
   every other prototype dial in this repo — re-run once real growth-site data exists.
5. **Ranged combat model:** not separately simulated — §2's recommendation rests on
   reasoning about the existing, already-measured combat model (Gate 1's geometric-
   targeting fix, `GATE1-FUN-PROTOTYPE.md` §3b) generalizing to a larger radius, not
   on new balance numbers. The HP/DPS/`TargetsPerHit` values proposed for Archers in
   `unit-types.json` are **unmeasured guesses** and need the same zero-input-baseline
   treatment `GATE1-FUN-PROTOTYPE.md` §3 gave the original retinue tuning before they
   should be trusted.

---

## 8. Performance requests (→ performance-director)

**Type storage costs nothing new.** Type is encoded as a **range partition of
`SquadId`** (e.g. unit IDs 0..`Units(Spearmen)-1` are Spearmen, the rest Archers),
not a new fragment field — deriving type from which range a `SquadId` falls in is a
comparison against a small per-frame-computed boundary, not a new byte on
`FSwarmAnimFragment`. This follows the exact reasoning `SwarmRenderPack`'s own header
comment already states for avoiding class-layout changes on a hot path: a new
`uint8 Type` field would be a class-layout change forcing a full editor-closed
rebuild every time it's touched; a range partition is a value computed from data
that's already there.

**Per-type formation lookup:** one extra branch (which type's `FParams`) per unit per
`SlotOffset` call — O(1), same complexity class as today's single global lookup.

**Per-type, per-stance engage range:** the combat pass's `EngageRange` read moves from
a 1D table (stance) to a 2D one (type × stance) — still one indexed read per unit, no
new pass, no new cross-entity access pattern.

**Ranged combat, the sim side:** **zero new entities, zero new fragments.** Archers
reuse the existing `StrikeReachSq`/`BlowsClaimed` grid mechanism with a larger radius
value — this is a data change (`EngageRange` per type) inside an already-measured,
already-cheap pass, not a new system. The volley **visual** (an arc/trail on
`SwingBit`) is a render-bridge ask, not a sim cost — flagging it here so whoever owns
the Niagara bridge sizes it, not proposing sim work for it.

**Two independent dense repacks (one per type) instead of one retinue-wide repack:**
same argument the prior revision made for per-squad repacks, one level up — cheaper in
aggregate (two smaller `O(n log n)` sorts, each gated on that type's own pool having
moved) than one big retinue-wide sort, and lets a stable type skip work while the
other reforms.

**Army View:** unchanged from the prior revision — still `O(MaxSquads)` = `O(8)`
aggregate draws, still strictly cheaper than a per-soldier loop. Adding a type
label/icon per block is a fixed small addition to that same O(8) pass, not a new
walk.

**Map Mode is the one real new cost, and it should be sized honestly, not undersold:**
an `O(N)` orthographic draw pass, same complexity class as today's Unit/Squad View
walk (not a new asymptotic class this codebase hasn't already paid for), and cheaper
per-unit than that view's atlas-brush path if Map Mode's icons are flat colored
primitives rather than sprite frames — worth confirming with a profiling pass once
built, same as every other "should be cheap" claim in this file's history has needed
one. At the retinue sizes this spec's own ceiling analysis (§4.2, ~730) treats as the
near-term target, this is not expected to be a bottleneck, but it is the first
addition in this document's history that reintroduces an `O(N)` walk to a mode that
previously had an `O(8)` guarantee — flagged plainly rather than glossed over, per the
brief's own standard for this section.

---

## 9. Canon proposals (owner decides — no canon file edited here)

- **`CLASSES.md` §1 (Vanguard retinue)** needs a real subsection splitting "the
  Liberated" into the two v1 types (Spearmen, Archers), with the Veteran/Bannerman
  promotion ladder stated as crossing type rather than replacing it (§1.5). Also
  worth one explicit sentence distinguishing Vanguard archers (mass, anonymous,
  disciplined) from Pathfinder's pack (few, elite, named) — see §1.1 — so a future
  reader doesn't read them as the same idea twice.
- **`GDD.md` §4** — the prior revision already proposed a note that stances address a
  narrower target (squad, not whole retinue) without adding verbs. This revision adds:
  a stance's *meaning* is now also a function of **unit type** (§1.8), still without
  adding verbs — worth folding into the same proposed §4 note rather than a second one.
- **`GDD.md` §10 (Mass Entity constraints)** — worth a line confirming "unit type" is
  now a first-class composition axis (like team) that stays within the shared-
  archetype rule: every Spearman behaves like every other Spearman, every Archer like
  every other Archer, no per-unit special-casing. This spec's whole design leans on
  that rule holding; stating it in the canon doc makes it citable for whoever builds
  a third type later.
- **A recruit-time type CHOICE** (§1.4's "player-choice-at-rescue" alternative to the
  generator-tagged default) is a real, better-feeling decision-event candidate for
  GDD §6 — flagged as a future proposal, not designed here.
- **`SYSTEMS.md`** needs new entries once this lands: typed units, ranged combat's
  minimum-viable model, and the Army View / Map Mode split — not written here per the
  file-write restriction; this doc is the source, `SYSTEMS.md`'s job is the pointer +
  rationale summary.

---

## 10. Narrative requests (→ narrative-director)

- **Archers need fiction distinct from the Pathfinder's pack**, or the two ranged
  identities will read as redundant to a player even though `CLASSES.md` will state
  they aren't (§1.1, §9). What does it mean, in `FLAME-FOUNDATION.md`'s premise, that
  some of the liberated take up a bow instead of a spear — trained faster, held back
  from the wall by choice or by the bearer's assignment, a different kind of rescue
  site entirely? A concrete hook: since growth sites are generator-tagged by type
  (§1.4), a "this was an archery range / hunting store, not a cell block" flavor of
  rescue site is a cheap, legible way to justify the split without new mechanics.
- **Map Mode is the natural place to finally render the leash as the lit floor**
  (`FLAME-FOUNDATION.md` §3a's still-open promise) — flagging this as a real
  opportunity for whoever writes the art brief for Map Mode: this is the first UI
  surface in the repo with both the pixel budget and the reason to show "where the
  light reaches" as a first-class visual, not a debug ring. Gameplay readability need
  that should travel into that brief: the leash boundary must read as *ground*, at a
  minimum lit-vs-dark value split (Demichrome, no new hue) — matching the "leash
  becomes visible, stops being a rule" mandate — not as a HUD overlay line, so it
  keeps reading as world, not UI chrome.
- **Squad/unit identity across attrition** (prior revision's §9, unchanged, still
  live): once units are typed, "what does a Spearman company's banner mean vs. an
  Archer company's" is a slightly richer version of the same open question — worth
  folding into the same narrative pass rather than answering twice.

---

## 11. Assumptions the owner should confirm

1. **v1 unit types are exactly Spearmen and Archers**, not a longer roster (§1.1) —
   the recommendation, grounded in Design Law 5 and in proving the typed model
   end-to-end before widening it.
2. **"Squad" and "unit" are the same thing from here on** (§1.0) — a terminology
   merge, not a code rename; `SquadId`/`MaxSquads`/`squads.json` keep their names.
3. **Type is assigned once at recruit time and is permanent** (§1.3/§1.4) — a
   Spearman never becomes an Archer through promotion, reinforcement, or repacking.
4. **Recruit-time type is generator-tagged (80/20 Spearmen/Archers), not a player
   choice** (§1.4) — the simpler v1 answer; a decision-event version is flagged as a
   future canon proposal (§9), not built here.
5. **Unit-count-per-type allocation uses the `ceil(pool/80)`, Spearmen-claims-first
   formula** (§4.1) — a real policy call, not the only reasonable one; §4.2's
   simulation shows it breaks (folds Archers into one oversized unit) once the
   Spearmen pool alone passes ~580, sooner than the old single-type ceiling.
6. **Ranged combat ships in v1 using the minimum-viable reused-grid model** — no
   physical projectiles, no true line-of-sight occlusion, no friendly fire (§2). This
   is the spec's central recommendation and is not yet owner-confirmed.
7. **Brood stay untyped in v1** — no ranged brood, deliberate asymmetry with the now-
   typed retinue (§2.2, §5.5) — revisit when elites are designed.
8. **The compact Unit Cam panel keeps abstract per-type blocks (Army View, unchanged
   mechanism); "mini retinue units by default" is delivered by the new, larger Map
   Mode instead** (§5.2) — the single biggest interpretive call in this revision,
   grounded in re-derived geometry that projection method doesn't rescue legibility at
   the compact panel's size. If the owner meant the compact panel itself should show
   literal mini-units regardless of the legibility cost, that's the point to say so,
   the same way the prior revision flagged its own block-vs-billboard call.
9. **Per-type formation `Forward` values (Spearmen 250 / Archers 40) and all other
   `unit-types.json` numbers are untested placeholder dials** (§1.7, §7) — same status
   as every other prototype dial in this repo, explicitly not playtested.
10. **`MaxSquads` stays 8** (§4.3) through the near-term bigger-retinue stage; §4.2's
    lower breaking point (~730 vs. the old ~640-750) is flagged as the thing to
    re-check first once that stage exists.
