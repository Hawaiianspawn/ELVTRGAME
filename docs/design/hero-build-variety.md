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

**Legal count, after each chassis's own coherence constraint AND the
weapon/projectile travel-type coupling (task-090, §2a below): 774** (was 729
before task-098 widened archer's `legal_modifications`, §2b) —
verified by script, not hand arithmetic (§9). Each `chassis` row in
`hero-builds.json` declares its own `legal_weapons` / `legal_projectiles` /
`legal_modifications` / `legal_abilities` / `legal_origin_worlds` — a subset
of each axis, not the whole thing — but **a chassis's `legal_weapons` and
`legal_projectiles` are NOT a free cross product**: a given legal weapon only
pairs with the subset of the chassis's legal projectiles that share its
travel type (§2a). Summed per chassis, per legal weapon, then across the
chassis (`hero-builds.json`'s `reachable_build_count.per_chassis`):

| Chassis | Legal weapons | Weapon-projectile pairs (not weapons x projectiles) | Legal mods (+none) | Legal abilities (+none) | Legal origins | Legal builds |
|---|---|---|---|---|---|---|
| Archer | 3 | 5 (2+2+1, not 3x3=9) | **3** (+1) | 2 (+1) | 3 | **180** |
| Gunner | 2 | 4 (2+2) | 2 (+1) | 2 (+1) | 3 | 108 |
| Line Breaker | 2 | 2 (1+1, not 2x2=4) | 2 (+1) | 2 (+1) | 3 | **54** |
| Skirmisher | 2 | 2 (1+1, not 2x2=4) | 2 (+1) | 2 (+1) | 3 | **54** |
| Arcane Caster | 2 | 4 (2+2) | 2 (+1) | 2 (+1) | 3 | 108 |
| Combat Engineer | 2 | 4 (2+2) | 2 (+1) | 2 (+1) | 3 | 108 |
| Beastcaller | 2 | 2 (1+1, not 2x2=4) | 2 (+1) | 2 (+1) | 3 | **54** |
| Siege Artillerist | 2 | 4 (2+2) | 2 (+1) | 2 (+1) | 3 | 108 |
| **Total** | | | | | | **774** |

Archer's mod count went 2->3 in task-098 (§2b): it gained `piercing_rounds`,
raising `(legal_modifications+1)` from 3 to 4 and its row from 135 to 180 —
the only chassis task-098 touched.

**Correction record (task-090):** the original 999 (still visible in prior
commits of `hero-builds.json`) multiplied each chassis's `legal_weapons x
legal_projectiles` as a free cross product — it assumed
every legal weapon could fire every one of that chassis's legal projectiles.
That's false: `Scripts/sim/variety.py`, before task-090, rolled `weapon_id`
and `projectile_id` independently and produced builds like "Beastcaller /
Cleave Melee Sweep / Shrapnel Spread" (a melee sweep firing a thrown
shrapnel cone) and "Archer / Continuous Beam / Fletched Arrow" (a laser
firing an arrow). §2a below is the fix; **729 is the corrected count, not an
estimate** — verified by the same scratch-script method as the original 999
(§9), and it still comes out to 3.1% of the raw 235,200, comfortably in the
"hundreds" the brief asked for.

Archer is the one irregular row (3 legal weapons/projectiles instead of 2)
— deliberately, since it's the owner's own named example and needed room to
host both the calibration weapon (`precision_longbow`, reproducing
`unit-types.json`'s Archers exactly, §7) and the two flavor-divergent weapons
(`arcing_aoe_lobber` "RPG", `beam_continuous` "laser cannon") in the same
chassis, side by side.

### 2a. Weapon x projectile is a constrained pairing, not a free product (task-090)

Chassis coherence (§2's "somewhat themed consistently") constrains WHICH
weapons and WHICH projectiles a chassis can roll. It never constrained
whether a given weapon could physically fire a given one of those
projectiles — `Scripts/sim/variety.py` drew `weapon_id` and `projectile_id`
independently, so any chassis with, say, one melee weapon and one hitscan
weapon in its `legal_weapons`, and one melee projectile and one physical
projectile in its `legal_projectiles`, could roll all four crossings —
including the two nonsensical ones.

**The fix:** each `weapon_archetypes.<id>` row now carries its own
`legal_travel_types` (reusing `projectiles.<id>.travel_type`'s existing
enum — no second taxonomy). A rolled build's legal projectile set is
`chassis.legal_projectiles ∩ {p : projectiles[p].travel_type in
weapon.legal_travel_types}`. Most weapons take exactly one travel type (a
Cleave Melee Sweep only takes `melee_instant`, a Continuous Beam only takes
`hitscan`, an Arcing AoE Lobber only takes `ballistic_arc` — the shape the
task brief named directly); `chain_bounce_shot` takes two (`hitscan` AND
`physical_straight`) because its own note already says "ricochet slug" /
"barbed boomerang" as much as "chain lightning wand" — a bounced physical
shot is as legitimate a flavor for it as a chained energy one, and Beastcaller
needed that width to keep `chain_bounce_shot` reachable at all (Beastcaller's
own `legal_projectiles` is `[melee_swing, shrapnel_spread]`, and
`shrapnel_spread`'s `travel_type` is `physical_straight`).

**`Scripts/sim/variety.py`'s `sample_roster()` raises if this intersection is
ever empty** for a legal chassis/weapon pair, rather than silently falling
back to the chassis's full `legal_projectiles` list — an empty intersection
means `hero-builds.json` itself is wrong (a chassis legalized a weapon with
no compatible projectile among its own legal projectiles), and that has to
surface as a crash, not a quietly-wrong roll. Every current chassis/weapon
pair was checked by hand before this shipped (the table in §2 above) and has
a non-empty intersection — no chassis's `legal_projectiles` list needed to
change to make that true.

Both `calibration_builds` (Spearmen, Archers) and both `worked_example_builds`
(RPG Archer, Laser Archer) remain exactly legal under this constraint — see
§7 and §6 respectively; nothing about their axis picks or `build_stat_block`s
changed.

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

## 2b. task-098 — opening the armor gate

`docs/sim/DIFFERENTIATION.md` (task-091) found the real driver of
`cleave_melee_sweep`'s point-target rank-1 rate wasn't chassis-legality
breadth, it was an **armor gate**: against `brood_elite`'s Armor 12 /
`brood_boss`'s Armor 14, 6 of the 9 reachable weapon archetypes sit at or
effectively at `combat-model-constants.json`'s `chip_floor` (3.0) regardless
of how often they're drawn, and only `siege_artillery`, `arcing_aoe_lobber`,
and `cleave_melee_sweep` clear it meaningfully. This task's brief was to open
that gate from the BUILD side — `entity-tiers.json`'s armor values and the
chip floor itself are locked (task-076/091's own citations depend on them
staying put) — using the two levers the data actually owns: weapon
damage/swing values, and which chassis can reach `piercing_rounds`
(`armor_penetration_flat` 4, `EffectiveBlow = max(AttackerBlow -
max(Armor - ArmorPenetrationFlat, 0), ArmorChipFloor)`).

**Two changes, both in `hero-builds.json`:**

1. **`chassis.archer.legal_modifications` gained `piercing_rounds`** (was
   `[focus_optic, overcharge_core]`, now adds a third option). `precision_longbow`
   (Archer's calibration weapon, blow 16.2) was the ONE weapon in
   DIFFERENTIATION.md's floored group whose blow was already close enough to
   the armor values that a flat +4 penetration alone crosses it from "barely
   clears Elite / floors Boss" to clearing both with real margin — it just had
   no chassis with penetration access (`piercing_rounds` only reached
   `line_breaker`/`beastcaller`, i.e. `cleave_melee_sweep`'s own two chassis —
   DIFFERENTIATION.md's own "concentration is part of the cause" line).
   Rather than retuning a CALIBRATION weapon row (`precision_longbow` is cited
   against `unit-types.json`'s Archers and `entity-tiers.md` §2.2's own stated
   Archer blow value — changing its numbers would break that citation), this
   only widens WHICH chassis can reach an existing modification.
   `calibration_builds.archers` keeps `modification: null` and is completely
   unaffected — this only adds a build a player could ADDITIONALLY roll, not a
   change to the calibration point itself.
2. **`weapon_archetypes.chain_bounce_shot.damage_per_shot` raised 14.0 -> 19.0**
   (blow 11.9 -> 16.15). `chain_bounce_shot` was the second-closest floored
   weapon to clearing (blow 11.9 vs. `precision_longbow`'s 16.2), and unlike
   `precision_longbow` it's a PROTOTYPE DIAL, not a calibration row, so its
   own number was the correct lever rather than borrowing someone else's
   chassis access. It's ALSO already legal on `beastcaller` alongside
   `piercing_rounds` (`beastcaller.legal_weapons = [cleave_melee_sweep,
   chain_bounce_shot]`, `legal_modifications = [lightweight_frame,
   piercing_rounds]`) — no chassis-access edit needed, only the damage number.
   +35.7% raw dps (17.85 -> 24.22) sits above the pre-change Monte-Carlo mean
   (18.85, §9) but below the roster ceiling (`cleave_melee_sweep`'s 30) — a
   real jump, not "+X% everything," and it moves exactly one weapon, not the
   whole table.

**What this deliberately does NOT touch, and why:** `dual_strike_melee`
(blow 9.0), `turret_autocannon` (7.2), `beam_continuous` (6.0), and
`rapid_skirmish_blaster` (4.2) stay floored. Closing any of them the same way
would have needed either a much larger penetration value (breaking
`cleave_melee_sweep`'s own already-cited "+26.7% at Armor 12," `docs/sim/
VARIETY.md`) or a damage_per_shot increase large enough (roughly +75-115%,
checked by hand against each weapon's own blow) to push its raw dps past the
current roster ceiling — trading "opens the armor gate" for "creates a new
dominant weapon," the exact failure mode this task's brief named as
disqualifying. These four keep their own identity instead:
`rapid_skirmish_blaster`/`turret_autocannon` are the roster's sustained
suppression/pressure tools (highest ROF, most simultaneous targets per
archetype-tier), `beam_continuous` is the steady-tick single-target
alternative to `precision_longbow`, and `dual_strike_melee` is the
mobility-flanking melee option next to `cleave_melee_sweep`'s wall-cleave —
none of that is what a lone armored elite/boss punishes, and that's an
honest, stated asymmetry (a swarm-pressure or mobility tool losing a duel
with a single heavily-armored target), not neglect.

**Verified numbers** (`chip_floor` 3.0, Elite Armor 12, Boss Armor 14,
`piercing_rounds` pen 4 — recomputed by scratch script off the live data,
same method §9 uses, not hand arithmetic):

| Weapon | Blow | Elite (no pen) | Boss (no pen) | Elite w/ piercing | Boss w/ piercing | Piercing-legal chassis |
|---|---|---|---|---|---|---|
| `siege_artillery` | 154.00 | 142.00 | 140.00 | n/a | n/a | none |
| `arcing_aoe_lobber` | 44.00 | 32.00 | 30.00 | 36.00 | 34.00 | **archer** (new) |
| `cleave_melee_sweep` | 27.00 | 15.00 | 13.00 | 19.00 | 17.00 | line_breaker, beastcaller |
| `precision_longbow` | 16.20 | 4.20 | 3.00 (floors) | **8.20** | **6.20** | **archer** (new) |
| `chain_bounce_shot` | **16.15** (was 11.90) | 4.15 | 3.00 (floors) | **8.15** | **6.15** | beastcaller |
| `dual_strike_melee` | 9.00 | 3.00 (floors) | 3.00 (floors) | 3.00 (floors) | 3.00 (floors) | line_breaker — floors regardless |
| `turret_autocannon` | 7.20 | 3.00 (floors) | 3.00 (floors) | n/a | n/a | none |
| `beam_continuous` | 6.00 | 3.00 (floors) | 3.00 (floors) | 3.00 (floors) | 3.00 (floors) | **archer** (new) — floors regardless |
| `rapid_skirmish_blaster` | 4.25 | 3.00 (floors) | 3.00 (floors) | n/a | n/a | none |

**Result: 5 of 9 reachable weapon archetypes now have a real (non-floor,
armor-clearing-with-real-margin) path against both Elite and Boss armor**,
up from 3 — `siege_artillery`, `arcing_aoe_lobber`, `cleave_melee_sweep`
(unchanged) plus `precision_longbow` and `chain_bounce_shot` (new, both
conditional on rolling `piercing_rounds`). Neither new entrant threatens
`cleave_melee_sweep`'s lead: their own steady-state dps once armor is applied
(`precision_longbow` 9.11 Elite / 6.89 Boss, `chain_bounce_shot` 12.22 Elite /
9.22 Boss) stays well under `cleave_melee_sweep`'s (21.11 Elite / 18.89 Boss)
and `siege_artillery`'s (21.30 Elite / 21.00 Boss) — they join the
competitive middle of the pack, not the top. `docs/sim/DIFFERENTIATION.md`'s
task-098 section has the measured before/after harness run.

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
| Chain / Bounce Shot | 1.5 | 500 | 50 | 19.0 (task-098, was 14.0 — §2b) | 3 | 0 | 0.85 | discrete multi-target chain, no splash |
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
   lists**, not hand arithmetic — confirmed **235,200 raw / 729 legal**
   (originally 999, corrected task-090, §2a — the difference is entirely
   removed weapon/projectile crossings that no physically-consistent build
   could ever use), matching `hero-builds.json`'s `reachable_build_count`
   block exactly at the time. **Re-confirmed at 235,200 raw / 774 legal
   post-task-098** (§2b's archer `piercing_rounds` legality is the entire
   delta, +45 on archer's own row, nothing else moved), again matching
   `hero-builds.json`'s current `reachable_build_count` block exactly.
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
   **Re-run post-task-098** (same method, archer's new `piercing_rounds`
   legality and `chain_bounce_shot`'s raised `damage_per_shot` folded in):
   **12.67 to 30.0**, mean **20.06**, median **18.0** — floor and ceiling
   unchanged (`cleave_melee_sweep` still both the tightest-melee-range and
   the highest-dps row; `piercing_rounds` only reduces a VICTIM's armor, it
   never raises the wielder's own `dps`, so it can't move this band at all),
   only the mean shifted up by `chain_bounce_shot`'s single raised row —
   consistent with §2b's "moves exactly one weapon, not the whole table."

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
6. **A second incoherence class exists, found but NOT fixed by task-090
   (out of scope — task-090's brief was weapon x projectile only):** three
   axis rows are legal nowhere. `weapon_archetypes.shotgun_spread` does not
   appear in any `chassis.legal_weapons` list, and `projectiles.crossbow_bolt`
   / `projectiles.homing_seeker` do not appear in any `chassis.legal_projectiles`
   list — all three are fully-specified rows (Shotgun Spread even has its own
   `legal_travel_types: ["physical_straight"]` now) that no chassis can ever
   roll. That's not a weapon/projectile PAIRING bug (nothing pairs them
   incoherently, because nothing pairs them at all) — it's dead data: either
   these three rows should gain a `legal_weapons`/`legal_projectiles` home on
   at least one chassis, or they should be understood as reserved-for-later
   and labeled as such. Left for a follow-up task; not touched here.

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
