# Hero-side build variety — chassis x weapon x projectile x ability x origin-world

**What this is:** the combinatorial layer that turns the friendly roster from
two hand-written types (`docs/data/unit-types.json`'s Spearmen/Archers) into a
data-driven space that produces **hundreds** of distinct unit builds, while the
enemy roster stays at its stated 6-8 types (`docs/data/entity-tiers.json`,
read-only for this task). Simulation-space only, per the owner (2026-07-29):
nothing here needs to be UE-DataTable-imported or runtime-read yet — the
target is a JSON file a Python script (task-080) can load and rank builds
against each other.

**Extends:** `docs/data/unit-types.json` (generalizes its two rows into an
axis space instead of two hand-typed records — both rows stay reachable, see
§7), `docs/data/upgrades.json` §`tier_ladder` (a build still crosses the
Freed/Militia/Veteran/Bannerman promotion ladder orthogonally, the same way
`squad-group-system.md` §1.5 already states type and tier are independent
axes — nothing here changes that), and `docs/narrative/FLAME-FOUNDATION.md`
§4.4 (origin-worlds are the mechanical anchor for "uniting flames" — `WORLD.md`
is SUPERSEDED and not cited anywhere in this doc).

**Data:** `docs/data/hero-builds.json` + `.schema.md`. **Does not touch:**
`docs/data/entity-tiers.json`, `docs/data/unit-types.json`,
`docs/data/upgrades.json`, `docs/data/squads.json`, `SYSTEMS.md`, `GDD.md` —
read freely, write nowhere in that list.

---

## 1. The owner's ask, and what shape it demands

> "A new game design goal is to give specific rules and buffs based on their
> variety, so example we have a rpg archer, he would likely shoot an rpg, the
> next a laser cannon, stats rate of fire, range, etc. This is the randomizing
> and synergies that makes it fun."
>
> "We are focusing on making many individual character varieties for the
> heroes, but not the enemies. Enemies get like 6-8 types, the Hero units on
> our side should get 100's."
>
> "We should be able to get somewhat themed consistently to their character
> view. Example RPG archer would be shooting explosives that do damage in an
> area."

Three things fall out of this directly: (1) hundreds of builds means
**multiplying axes**, not a hand-typed list — the same "if you find yourself
typing a 300-row list, you've taken the wrong shape" instruction this task
was given; (2) "somewhat themed consistently" means the multiplication has to
be **constrained**, not a free product — an archer chassis rolling a
shield-wall ability is illegal, and that has to be enforceable as data, not
prose; (3) "rate of fire, range, etc." names the actual numeric shape a
weapon needs, and "their projectile can be different" names a second,
independent axis under it.

---

## 2. The axes, their cardinality, and the two counts that matter

| Axis | Members | File table |
|---|---|---|
| Chassis (visual/role identity) | 8 | `hero-builds.json` `chassis` |
| Weapon archetype (numeric fight-shape) | 10 | `weapon_archetypes` |
| Projectile (what travels, how it resolves) | 10 | `projectiles` |
| Modification (alters own numbers; incl. "none") | 6 + none = 7 | `modifications` |
| Ability (conditional/periodic; incl. "none") | 6 + none = 7 | `abilities` |
| Origin-world (synergy key) | 6 | `origin_worlds` |

**Raw product (no constraints): 8 x 10 x 10 x 7 x 7 x 6 = 235,200.** This is
the number that would let an Archer chassis legally roll `shield_wall` (which
doesn't even exist in this space, but the point stands) or fire a melee-only
projectile — every combination is "reachable" in the sense of existing in the
axis tables, and almost none of it is thematically sane.

**Legal count, after each chassis's own coherence constraint: 999** — verified
by script, not hand arithmetic (§8). Each `chassis` row in `hero-builds.json`
declares its own `legal_weapons` / `legal_projectiles` / `legal_modifications`
/ `legal_abilities` / `legal_origin_worlds` — a subset of each axis, not the
whole thing. Summed per chassis (`hero-builds.json`'s
`reachable_build_count.per_chassis`):

| Chassis | Legal weapons | Legal projectiles | Legal mods (+none) | Legal abilities (+none) | Legal origins | Legal builds |
|---|---|---|---|---|---|---|
| Archer | 3 | 3 | 2 (+1) | 2 (+1) | 3 | **243** |
| Gunner | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| Line Breaker | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| Skirmisher | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| Arcane Caster | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| Combat Engineer | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| Beastcaller | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| Siege Artillerist | 2 | 2 | 2 (+1) | 2 (+1) | 3 | 108 |
| **Total** | | | | | | **999** |

**999 is 0.42% of the raw 235,200** — the constraint table is doing real
work, and 999 lands solidly in the "hundreds" the brief asked for, not the
"dozens" its own failure mode names. Archer is the one irregular row (3 legal
weapons/projectiles instead of 2) — deliberately, since it's the owner's own
named example and needed room to host both the calibration weapon
(`precision_longbow`, reproducing `unit-types.json`'s Archers exactly, §7) and
the two flavor-divergent weapons (`arcing_aoe_lobber` "RPG", `beam_continuous`
"laser cannon") in the same chassis, side by side.

**What I loosened, honestly:** origin-world is NOT cross-constrained against
weapon/projectile choice (any of a chassis's 3 legal origin-worlds can pair
with any of its legal weapons). A fully cross-constrained table (this weapon
only from that world) would be more "thematically tight" but crushes the
count into the dozens per chassis — origin-world is deliberately the loosest
axis, both because the fiction (worlds merging, `FLAME-FOUNDATION.md` §4.4)
wants exactly that looseness, and because it's the axis a synergy rule reads
(§6) rather than the axis "theme" needs to police most tightly. Chassis
already carries the load-bearing coherence constraint the owner asked for
(no archer with a shield-wall ability); origin-world's job is to be a real
combinatorial multiplier that also means something at the roster level, not
a second layer of the same constraint.

---

## 3. Weapon archetypes — numbers first, flavor as a label list

Ten rows, each carrying `rate_of_fire` (shots/sec), `range`, `min_range`,
`damage_per_shot`, `targets_per_shot`, `aoe_radius`, `accuracy`, and a
`flavor_names` list spanning multiple origin-worlds. Full table:
`docs/data/hero-builds.json` `weapon_archetypes`; schema/units:
`hero-builds.schema.md`.

| Archetype | ROF (shots/s) | Range | Min range | Dmg/shot | Targets | AoE | Acc | Fight shape |
|---|---|---|---|---|---|---|---|---|
| Precision Longbow | 1.11 | 750 | 150 | 16.2 | 1 | 0 | 1.0 | slow, single-target, long reach — CALIBRATION (=Archers) |
| Cleave Melee Sweep | 1.11 | 95 | 0 | 27.0 | 8 | 0 | 1.0 | contact-range wide cleave — CALIBRATION (=Spearmen) |
| Arcing AoE Lobber | 0.4 | 650 | 120 | 55.0 | 1 | 180 | 0.8 | rare heavy splash hit, can't fire point-blank — "RPG archer" |
| Continuous Beam | 3.0 | 600 | 0 | 6.0 | 1 | 0 | 1.0 | rapid small ticks, same steady dps class as Longbow — "laser cannon" |
| Rapid Skirmish Blaster | 4.0 | 350 | 0 | 5.0 | 1 | 0 | 0.85 | highest ROF, lowest per-shot, close pressure |
| Chain / Bounce Shot | 1.5 | 500 | 50 | 14.0 | 3 | 0 | 0.85 | discrete multi-target chain, no splash |
| Dual Strike Melee | 1.8 | 90 | 0 | 9.0 | 2 | 0 | 1.0 | fast dual-hit melee |
| Shotgun Spread | 1.2 | 140 | 0 | 10.0 | 4 | 0 | 0.9 | close-range instantaneous cone |
| Siege Artillery | 0.15 | 1100 | 300 | 220.0 | 1 | 260 | 0.7 | biggest single hit + AoE, backline-only |
| Turret Autocannon | 2.5 | 500 | 0 | 8.0 | 2 | 0 | 0.9 | sustained mid-rate suppression |

**These differ in kind, not just in scale.** `rate_of_fire` spans 0.15-4.0
shots/sec (a 27x range), `range` spans 90-1100uu, `targets_per_shot` spans
1-8, and three rows carry a nonzero `aoe_radius` while seven don't — a
long-range slow single-target weapon (Precision Longbow) and a short-range
fast cleaving one (Cleave Melee Sweep) produce genuinely different fight
shapes at genuinely similar steady-state dps (18.0 vs 30.0, not orders of
magnitude apart) — the variety is in cadence/reach/target-count, exactly what
"differ in kind, not just scale" asks for, confirmed by the Monte-Carlo dps
spread in §8 (min 12.67 / max 30.0 / median 18.0 across 2000 random legal
builds — a tight overall power band despite wildly different `rate_of_fire`
and `range`).

---

## 4. Projectile — its own axis, sim-consumable vs. decorative

"Their projectile can be different" (owner) gets its own ten-row table:
`travel_type` (hitscan / ballistic_arc / physical_straight / melee_instant /
homing), `resolve_type` (point / pierce / area), `travel_speed_uu_s`, and
`accuracy_modifier`, plus its own `flavor_names`.

**Sim-consumable:** `resolve_type` (governs the `targets_per_hit` derivation,
§5) and `accuracy_modifier` (added into the weapon's `accuracy` before `dps`
is computed). **Presentation-only, no mapping onto anything the sim can
consume:** `travel_type` and `travel_speed_uu_s` — `docs/sim/LIMITATIONS.md`
§4 states plainly that neither combat model has a projectile-travel-time
primitive (no kiting, no dodge windows, no arc timing), and this doc isn't
inventing one. A rocket's 650uu/s arc speed is real data for a future render
bridge or a future travel-time model; it is decoration to the model that
exists today, and this doc says so rather than smuggling it in as a number
task-080 would silently misuse.

The one real mechanical payoff of a separate projectile axis: the SAME weapon
archetype can throw different things with different `resolve_type`s. `chain_
bounce_shot`'s `targets_per_shot: 3` pairs naturally with `chain_bolt`'s
`resolve_type: "pierce"` (the 3 targets ARE the chain), while `arcing_aoe_
lobber`'s single `targets_per_shot: 1` pairs with `rocket`'s
`resolve_type: "area"` (the `aoe_radius` field does the multi-target work
instead) — two structurally different ways to hit more than one thing,
distinguished by projectile, not by weapon.

---

## 5. Abilities and modifications — a first-class axis, split honestly

**A MODIFICATION alters the build's own numbers**, resolved once, at build
time:

| Modification | Effect |
|---|---|
| Focus Optic | range x1.4, rate_of_fire x0.8 |
| Overcharge Core | damage_per_shot x1.25, rate_of_fire x0.85 |
| Reinforced Plating | max_hp x1.25, move_speed_scale x0.9 |
| Lightweight Frame | move_speed_scale x1.15, max_hp x0.9 |
| Piercing Rounds | armor_penetration_flat +4 (reduces the VICTIM's Armor pre-chip-floor) |
| Wide Choke | targets_per_shot +1, range x0.9 |

Every one of these is a trade, not a pure upsize — the design intentionally
avoids a modification that's simply "+X% everything," so a rolled
modification is a real choice, not a strictly-better coin flip.

**An ABILITY is a conditional or periodic effect**, evaluated during the
fight, not folded into the build's steady-state numbers:

| Ability | Trigger | Representable in the flat DPS/HP model? |
|---|---|---|
| Rally Cry | periodic aura, 33% uptime | **Approximate** — only as a flat expected-value multiplier (1.04x) to nearby allies in the point-target model; no positional aura tracking in either model |
| Camouflage Strike | +200% on the first shot from stealth | **No** — a one-shot burst has no representation in a steady-state number |
| Shield Regen | 4%/s HP regen after 3s out of combat | **No** — neither model tracks in/out-of-combat state or time-varying HP |
| Last Stand | +50% dps below 25% HP | **No** — state-dependent on a per-soldier HP threshold neither model tracks (point-target only tracks the TARGET's HP; wave-attrition tracks pooled subgroup HP) |
| Chain Reactor | kill refreshes swing_interval | **Partial** — only in the point-target model, where one attacker's kill rate against the target is computable; not in wave-attrition, where kills aren't attributed per-attacker |
| Guardian Angel | prevents one death per fight, 400uu | **Partial** — only as a flat +1 survivor-count adjustment in the wave-attrition model's final output; not a stat-block field, not usable in the point-target model at all |

**Zero of the six abilities write a value into the shared `build_stat_block`.**
That's the honest answer the brief asked for rather than encoding a number
the sim would silently ignore — every row states its own limit in
`hero-builds.json`'s `representable_note`, and task-080 has to implement
whichever approximation it wants (or skip the ability entirely), not read a
pre-baked field.

---

## 6. The Build Stat Block — one comparable measurement per unique type

Every build resolves to exactly the flat shape `Scripts/sim/data_loader.py`'s
`retinue_fighter()`/`enemy_fighter()` already use — no translation layer
needed to drop a build into the existing harness:

`max_hp` (HP) · `armor` (flat) · `dps` (damage/sec) · `swing_interval` (sec) ·
`engage_range` (uu) · `min_engage_range` (uu) · `targets_per_hit` (count) ·
`move_speed_scale` (multiplier).

Derivation (full formulas + the `resolve_type` conversion:
`hero-builds.schema.md`):

```
effective_rate_of_fire   = weapon.rate_of_fire x (modification.rate_of_fire_mult or 1)
effective_accuracy       = clamp(weapon.accuracy + projectile.accuracy_modifier, 0, 1)
effective_damage_per_shot= weapon.damage_per_shot x (modification.damage_per_shot_mult or 1)

dps               = effective_damage_per_shot x effective_rate_of_fire x effective_accuracy
swing_interval    = 1 / effective_rate_of_fire
engage_range      = weapon.range x (modification.range_mult or 1)
min_engage_range  = weapon.min_range
targets_per_hit   = see resolve_type conversion (schema doc)
max_hp            = chassis.base_stats.max_hp x (modification.max_hp_mult or 1)
armor             = chassis.base_stats.armor
move_speed_scale  = chassis.base_stats.move_speed_scale x (modification.move_speed_scale_mult or 1)
```

### Worked example — same chassis, two builds, radically different shape

Both builds below are the **Archer** chassis (hp 70 / armor 0 / move 0.9,
identical), no modification, no ability — the only things that differ are
weapon, projectile, and origin-world, exactly the owner's own paired example:

| Field | RPG Archer (Ashworks) | Laser Archer (Deep Static) |
|---|---|---|
| Weapon | Arcing AoE Lobber | Continuous Beam |
| Projectile | Rocket (area) | Energy Beam (point) |
| `max_hp` | 70 | 70 |
| `armor` | 0 | 0 |
| `dps` | **16.5** | **18.0** |
| `swing_interval` | **2.5s** | **0.33s** |
| `engage_range` | 650 | 600 |
| `min_engage_range` | **120** | **0** |
| `targets_per_hit` | **4** (splash-converted) | **1** |
| `move_speed_scale` | 0.9 | 0.9 |

Nearly identical steady-state `dps` (16.5 vs 18.0) and comparable `hp`/`armor`
hide a completely different fight: the RPG Archer lands one huge, 4-target
splash hit every 2.5s and physically cannot fire at anything inside 120uu;
the Laser Archer lands small single-target ticks 7.5x more often and has no
minimum range at all. That's the "differ in kind, not just scale" test
passing on the owner's own two named examples.

---

## 7. Calibration — the current two types stay reachable, exactly

| Type | Chassis | Weapon | Projectile | `build_stat_block` | Matches |
|---|---|---|---|---|---|
| Spearmen | Line Breaker | Cleave Melee Sweep | Melee Swing | hp130 / armor0 / dps30.0 / swing0.9 / engage95 / min0 / targets8 / move1.0 | `unit-types.json` Spearmen — **exact** |
| Archers | Archer | Precision Longbow | Fletched Arrow | hp70 / armor0 / dps18.0 / swing0.9 / engage750 / min150 / targets1 / move0.9 | `unit-types.json` Archers — **exact** |

Both weapon-archetype rows behind these two builds were deliberately tuned to
reproduce not just `unit-types.json`'s numbers but the already-cited blow
values one layer down: Cleave Melee Sweep's `damage_per_shot: 27.0` IS
`entity-tiers.json`'s `reference_blow_value` (Militia's blow, the stated
system-wide "balance anchor"), and Precision Longbow's `damage_per_shot: 16.2`
IS the Archer blow value `entity-tiers.md` §2.2's own table already states
(`DPS 18 x SwingInterval 0.9`). This isn't a coincidence built to look good —
it's the same shared `swing_interval = 0.9`s cadence every existing
Fodder/Soldier/retinue row already uses (`entity-tiers.json`
`design_constants.swing_interval_shared`), applied to this axis space instead
of invented fresh.

---

## 8. Origin-world and the synergy rules

Origin-world is the synergy key (owner's cross-world merge premise,
`FLAME-FOUNDATION.md` §4.4 — six WorkingNameOnly rows, same convention
`entity-tiers.json`'s `brood_*` rows use, since no world is named in canon
yet). Four rules, machine-checkable (`hero-builds.json` `synergy_rules`,
condition/effect shape documented in `hero-builds.schema.md`):

1. **Kindred Ranks** (bonus) — roster's majority origin-world (>=50% share)
   gets +8% dps. Rewards concentration.
2. **Twin-World Resonance** (bonus) — Ashworks + Deep Static both present
   -> +10% rate_of_fire to units of either world. Rewards a specific pairing.
3. **Choir's Ward** (bonus) — 2+ Rally Cry units -> +5% dps, whole roster.
   Rewards investing in one ability axis value more than once.
4. **Babel Discord** (ANTI-SYNERGY / cost) — 5+ of the 6 origin-worlds present
   in one roster -> -6% accuracy, whole roster. Directly opposes rule 1: a
   roster chasing maximum world diversity (which the flavor axis actively
   invites — "not locked to just medieval") pays a real, whole-roster cost
   for it, so "maximize every buff" is not the answer — a concentrated or
   deliberately-paired roster (rules 1-3) is mechanically the stronger play,
   and a maximally diverse one is a real, statable alternative, not simply
   worse-in-every-way flavor-only.

---

## 9. Simulation notes

**What was simulated:** a scratch Python script (session scratchpad, not
committed — same house method `entity-tiers.md` §7 and `squad-group-system.md`
§7 both use for their own combinatorial/geometric checks), reproducing
`hero-builds.json`'s `chassis`/`weapon_archetypes`/`projectiles` tables in
code and computing:

1. **Raw product and legal count from the actual per-chassis constraint
   lists**, not hand arithmetic — confirmed **235,200 raw / 999 legal**,
   matching `hero-builds.json`'s `reachable_build_count` block exactly.
2. **Both calibration builds**, computed through the full `build_stat_block`
   derivation pipeline (§6) — confirmed both reproduce `unit-types.json`'s
   Spearmen and Archers rows exactly (hp/dps/swing_interval/engage_range/
   min_engage_range/targets_per_hit all match to the number, modulo float
   noise at the 1e-15 level from `1/0.9`).
3. **The RPG-Archer / Laser-Archer worked example** (§6), through the same
   pipeline — confirmed the table in §6.
4. **A 2000-sample Monte-Carlo draw of random LEGAL builds** (uniform over
   chassis, then uniform over that chassis's own legal weapon/projectile/
   modification lists) — `dps` ranged **12.67 to 30.0**, mean **18.85**,
   median **18.0**. This is the check behind §3's "differ in kind, not
   scale" claim: despite `rate_of_fire` spanning 0.15-4.0 and `range`
   spanning 90-1100uu across the roster, steady-state `dps` stays in a
   tight band — variety lives in cadence/reach/target-count, not in a
   power-level spread that would make half the roster strictly worse.

**Assumptions, stated plainly (none of these are measured):** every numeric
value in `chassis`, `weapon_archetypes` (except the two calibration rows),
`projectiles`, `modifications`, and `synergy_rules` is a PROTOTYPE DIAL —
first-pass, unplaytested, same epistemic status as `docs/data/economy.json`
and `docs/data/unit-types.json`'s own Archer row before it. The
`aoe_radius / 40` splash-to-cleave conversion (§4/`schema.md`) is a Fermi
circle-packing estimate, not a measurement — same epistemic status as
`entity-tiers.json`'s `SurroundCapEstimate`.

---

## 10. Honest provenance

**Cited, exact:**
- `unit-types.json` Spearmen/Archers combat rows — reproduced exactly by the
  two calibration builds (§7).
- `entity-tiers.json` `design_constants.swing_interval_shared` (0.9s) and
  `reference_blow_value` (27.0, Militia) — both calibration weapon archetypes
  are built directly off these.
- `entity-tiers.md` §2.2's Archer blow value (16.2) — Precision Longbow's
  `damage_per_shot`.
- `entity-tiers.md` §2.2's `EffectiveBlow` formula — cited as the target
  `piercing_rounds` extends (not yet implemented in `Scripts/sim/
  combat_model.py`, flagged in §11).
- `Scripts/sim/data_loader.py`'s `retinue_fighter()`/`enemy_fighter()` flat
  fighter shape — the `build_stat_block`'s field list is that shape exactly.
- `docs/narrative/FLAME-FOUNDATION.md` §4.4 — origin-world's role as the
  synergy key, and the "run objective, not co-op lobby" framing that makes
  "uniting flames" a single-player mechanic. `WORLD.md` is NOT cited anywhere
  in this doc (superseded).
- `docs/data/squads.json` `squad_manager.max_squads` (8) — cited only in
  §11's open question about command-UI strain, not used as an input to any
  computation here.

**PROTOTYPE DIALS (first pass, unmeasured) — everything else:** all 8
chassis's role/base-stat choices beyond the two calibration rows, all 10
weapon archetypes beyond the two calibration rows, all 10 projectiles' numeric
fields, all 6 modifications' effect sizes, all 6 abilities' trigger numbers,
all 6 origin-worlds' names/flavor (also explicitly WorkingNameOnly, not
canon), and all 4 synergy rules' thresholds and effect sizes.

---

## 11. Open questions this doc did not settle

1. **Recruit-time roll vs. loot-style upgrade.** Is a build fixed at recruit
   time (generator-tagged, the same pattern `squad-group-system.md` §1.4
   already uses for Spearmen/Archers' 80/20 split), or is a chassis's weapon/
   projectile/modification/ability slot something a run-scoped item can
   change later (GDD Law 7's "loot serves hero AND retinue")? Both are
   plausible reads of "randomizing" in the owner's own framing; this doc
   specs the axis space either answer would draw from, not which one is used.
2. **Command-UI strain at hundreds of chassis.** `squad-group-system.md` §4.2
   already found the `MaxSquads=8` handle ceiling binds sooner once a SECOND
   type (Archers) exists (~730 total retinue vs. ~640-750 single-type). This
   doc doesn't attempt to re-derive that finding for "hundreds of chassis" —
   flagging it as the natural next stress-test once a real roster exists,
   not solved here.
3. **`piercing_rounds`' `armor_penetration_flat` needs a one-line
   `EffectiveBlow` extension** `Scripts/sim/combat_model.py` doesn't have yet
   (`EffectiveBlow = max(AttackerBlow - max(Victim.Armor - ArmorPenetrationFlat,
   0), ArmorChipFloor)`) — real code work, out of this task's file-write
   scope, flagged for whoever implements task-080.
4. **Growth-site tagging for chassis/origin-world** (which growth site yields
   which chassis, the way `unit-types.json`'s `growth_source_weight` already
   tags Spearmen/Archers) is not designed here — belongs to the procgen
   encounter-rules scope area, a separate deliverable.
5. **`rally_cry`'s and `chain_reactor`'s approximations are genuinely
   author's-choice, not derived** — a different average-uptime assumption
   would move their numbers; flagged rather than presented as settled math.

---

## 12. Narrative requests (-> narrative-director)

- **Origin-worlds need names and one-paragraph identities**, same
  WorkingNameOnly-to-canon path `entity-tiers.json`'s `brood_*` rows are
  already waiting on. Six slots, mechanically load-bearing right now (the
  synergy rules read them): a medieval-martial default (currently
  `iron_reach`), an industrial/gunpowder world (`ashworks`), a high-tech/
  energy world (`deep_static`), an organic/beast world (`verdant_wilds`), a
  ritual/arcane world (`hollow_choir`), and a crystal/psionic world
  (`glass_tide`). What a player must feel: these are pieces of OTHER bearers'
  small congregations, folded into yours as flames unite — not a
  Bestiary-style location roster, a found-family roster (`FLAME-FOUNDATION.md`
  §1's "there are other bearers... united, flames do what no single fire
  can").
- **The Babel Discord anti-synergy needs a sympathetic frame, not a
  "penalty" one.** Mechanically it's a flat -6% accuracy when 5+ worlds mix
  in one roster; what a player must feel is closer to "these people don't
  yet have a shared drill, not that diversity is bad" — the tension is meant
  to read as a real, textured cost of the merge (GDD Law 9's "the congregation
  dies for you gladly, and that should never feel comfortable"), not a "don't
  mix your army" balance note.
- **Chassis need a visual identity pass distinct from the enemy roster's
  language** — `squad-group-system.md` §1.1 already flags that Vanguard
  archers (mass, anonymous) must read as different from Pathfinder's named
  pack; this doc's 8 chassis (Archer, Gunner, Line Breaker, Skirmisher,
  Arcane Caster, Combat Engineer, Beastcaller, Siege Artillerist) are all
  in the same "mass, anonymous, faceless" register as Spearmen/Archers
  already are — none of them should read as elite/named. Gameplay
  readability need to carry into any art brief: a chassis's silhouette must
  telegraph its weapon's `min_range`/`aoe_radius` shape at a glance (a
  Siege Artillerist's huge min-range/backline identity should look
  unmistakably different from a Line Breaker's point-blank one) so a player
  scanning a screen full of typed units can read "who does what" without
  opening a menu.

## Canon proposals (owner decides)

- **`CLASSES.md` §1** would need a note that the Vanguard's retinue can now
  express itself through this chassis/weapon/projectile/ability/origin-world
  space instead of just the two hand-typed Spearmen/Archers rows, once (and
  if) this layer ships past simulation-space — not proposed as an edit here,
  flagged as the natural follow-up canon touch.
- **A recruit-time chassis CHOICE vs. generator-tag** (open question 1,
  §11) is a real decision-event candidate for `GDD.md` §6, the same shape
  `squad-group-system.md` §9 already flagged for type choice at rescue —
  worth folding into the same future canon pass rather than deciding twice.
