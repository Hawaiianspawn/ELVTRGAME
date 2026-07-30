# variety.py — hero-build roll-and-rank report (task-080)

`Scripts/sim/variety.py` rolls a random LEGAL roster out of
`docs/data/hero-builds.json` (task-079's chassis x weapon-archetype x
projectile x modification x ability x origin-world space), evaluates
task-079's `synergy_rules` against that roster, runs the roster as the
RETINUE side of an existing `docs/data/scenarios/<name>.json`'s
wave-attrition fight (the enemy side is that scenario's, unchanged), and
prints a metrics block plus an ASCII top-10 table of hero-builds ranked by
damage dealt.

```powershell
py Scripts/sim/variety.py --scenario floor2-ranged-wave --seed 1
py Scripts/sim/variety.py --scenario floor1-swarm-wave --seed 2 --json
```

`--roster <k>` (default 20) is how many distinct builds to sample.
`--count-per-build <n>` (default 2) is how many soldiers each rolled build's
`WaveGroup` represents. `--seed <n>` (required) is the only source of
randomness — `random.Random(seed)`, no global RNG state, so the same seed
against unchanged data always reproduces byte-identical output (verified:
`--seed 1` run twice diffed clean; `--seed 2` differs from `--seed 1`).

## Read this before trusting a number out of the table

**This is a WAVE-ATTRITION result.** `docs/sim/LIMITATIONS.md` §1 already
states that model does not reproduce `GATE1-FUN-PROTOTYPE.md`'s measured
~110-of-120 wave-1 survival at this harness's committed defaults — it
predicts a full retinue wipe there instead, and (per this task's own runs
above) it predicts a full wipe for BOTH sampled floor scenarios here too, at
these arbitrary roster/headcount defaults. **The top-10 table is a RELATIVE
comparison between builds pushed through one shared, imperfect model — never
an absolute claim about how a build performs in the actual game.** A build
that ranks #1 here beat the other 19 rolled builds under this model's rules;
it is not a claim that build would perform well in a played run.

**What this tool's model does not represent at all** —
`docs/sim/LIMITATIONS.md` §4, inherited unchanged: stance play (Follow /
Charge / Hold / Rally), leash mechanics, positioning, knockback, telegraph/
windup timing, chokepoints, supply/degrade, items, or multi-wave carryover.
Nothing rolled here is fought with a stance, a leash radius, or a played
input — every fight is `GATE1-FUN-PROTOTYPE.md` §3's zero-input baseline.
Do not read a build's rank as a verdict on any of these mechanics; the model
never simulated them.

## The weapon-field -> fighter-field mapping

Every build resolves through two new `data_loader.py` functions —
`resolve_hero_build()` (intermediate components) then
`finalize_hero_build_fighter()` (the flat shape `combat_model.py` expects,
matching `retinue_fighter()`/`enemy_fighter()` exactly) — per
`hero-builds.schema.md`'s own `build_stat_block` derivation:

| Fighter field | Derivation | Notes |
|---|---|---|
| `max_hp` | `chassis.base_stats.max_hp x modification.max_hp_mult` | |
| `armor` | `chassis.base_stats.armor` | carried through the fighter dict; see "Armor is a dead field" below |
| `dps` | `damage_per_shot x rate_of_fire x accuracy`, then x any synergy `dps` multiplier | `damage_per_shot`/`rate_of_fire` carry their own `modification` multipliers first |
| `swing_interval` | `1 / rate_of_fire` | |
| `engage_range` | `weapon.range x modification.range_mult` | |
| `min_engage_range` | `weapon.min_range` | no modification touches this, per schema |
| `targets_per_hit` | `weapon.targets_per_shot + modification.targets_per_shot_add`, or `max(that, round(weapon.aoe_radius/40))` if the rolled projectile's `resolve_type == "area"` and `weapon.aoe_radius > 0` | the schema's own area-conversion formula, applied verbatim |
| `role` | `"ranged" if engage_range > 150 else "melee"` | **deviates from `retinue_fighter()`/`enemy_fighter()`'s rule** — see below |
| `accuracy` | `clamp(weapon.accuracy + projectile.accuracy_modifier, 0, 1)`, then x any synergy `accuracy` multiplier | folded into `dps` before the fighter dict is built, not a fighter field itself (`combat_model.py`'s fighter shape has no `accuracy` slot) |
| `move_speed_scale` | `chassis.base_stats.move_speed_scale x modification.move_speed_scale_mult` | computed, carried in `resolve_hero_build`'s intermediate dict, **not consumed anywhere** — neither combat model has a movement-speed primitive |

### Fields that don't map, stated plainly rather than silently dropped

- **`accuracy`** IS consumed — folded multiplicatively into `dps` per the
  schema's own formula, exactly as `hero-build-variety.md` §6 specifies. It
  does not survive as its own field on the fighter dict because
  `combat_model.py`'s fighter shape has no accuracy primitive of its own
  (accuracy is baked into steady-state dps everywhere else in this harness
  too — see `enemy_fighter()`/`retinue_fighter()`, neither of which has an
  accuracy column either).
- **`aoe_radius`** IS consumed, only when the rolled projectile's
  `resolve_type == "area"` — folded into `targets_per_hit` via the schema's
  `aoe_radius / 40` circle-packing conversion. It has no separate life in the
  fighter dict.
- **`armor_penetration_flat`** (the `piercing_rounds` modification) **IS now
  applied** — fixed 2026-07-29; the rest of this bullet is the superseded
  account of when it was not, kept because it explains the shape of the fix.
  `combat_model.effective_blow()` takes an optional `armor_penetration`
  argument (default 0.0, so every pre-existing caller is bit-identical), and
  `steady_state_dps()` reads `armor_penetration_flat` off the **attacker's own
  fighter dict** rather than taking it as a parameter — which is why no call
  site needed changing. `finalize_hero_build_fighter()` puts the key on the
  dict; fighters built by `enemy_fighter()`/`retinue_fighter()`/`hero_fighter()`
  don't carry it and behave exactly as before. Measured effect on a Line
  Breaker / Cleave Melee Sweep build (base dps 30.00, unchanged): +0.0% vs an
  unarmored victim, +19.0% vs Armor 6, +26.7% vs Armor 12, +57.1% vs Armor 20.
  Penetration reduces armor floored at 0, so it is never a bonus against an
  unarmored target — `piercing_rounds` is now an anti-armor specialisation
  rather than a strict upgrade. The formula is the one `hero-builds.json`
  itself specified: `EffectiveBlow = max(AttackerBlow - max(Victim.Armor -
  ArmorPenetrationFlat, 0), ArmorChipFloor)`. Until 2026-07-29 this bullet
  read "carried through but NOT applied anywhere", and a build that rolled
  `piercing_rounds` fought exactly as if it hadn't; that is no longer the case.
- **`move_speed_scale`** is computed and carried in the intermediate
  component dict for completeness (it's part of the schema's
  `build_stat_block`) but is never read by either combat model — no
  movement-speed primitive exists in this harness at all.

### `role` deliberately does not reuse the existing convention

`retinue_fighter()`/`enemy_fighter()` classify `role` as `"ranged" if
engage_range > 150 AND min_engage_range > 0 else "melee"`. That rule happens
to work for the two existing hand-written types only because their data
correlates `min_engage_range` with `range` (Archers 750/150,
brood_soldier_ranged 700/150; every melee type is 95/0). Several of
task-079's REACHABLE weapon archetypes (the ones actually rollable via a
`legal_weapons` list — `beam_continuous` 600uu/0, `turret_autocannon`
500uu/0, `rapid_skirmish_blaster` 350uu/0, `chain_bounce_shot` 500uu/50)
break that correlation: applying the old rule verbatim would misclassify
all four as "melee" despite being plainly stand-off weapons, silently
collapsing them into the frontage-capped path they were never meant to sit
in. Every reachable archetype's `range` cleanly separates on 150uu alone
(melee: 90-95uu; ranged: 350uu+), so `finalize_hero_build_fighter()` uses
`engage_range > 150` on its own. This is a judgment call, made because the
old rule would have silently broken the intent of several new weapon rows,
not a retune of any `hero-builds.json` number.

### Armor IS applied in the wave-attrition model (fixed 2026-07-29 — this section previously said the opposite)

**Superseded.** This section used to state that every `steady_state_dps(...)`
call inside `simulate_wave_attrition()` passed `victim_armor=0.0`
unconditionally, on both the incoming and outgoing paths, and that a
hero-build chassis's `armor` field therefore had no effect. That was an
accurate description of the harness at the time and it is no longer true.

Armor now applies on all four call sites. Each side's victim armor is the
alive-count-weighted mean over exactly the pool that absorbs that side's
damage in the apply step — `combat_model.mixed_victim_armor()`, which reuses
the well-mixed-target assumption the tick loop already makes when it
distributes damage proportional to alive-count share. So:

- A chassis's own `armor` (`combat_engineer`=2, `beastcaller`=1) now reduces
  incoming damage.
- The ENEMY's `Armor` (`entity-tiers.json`: `brood_soldier_melee`=6,
  `brood_elite`=12, `brood_titan`=20, `brood_boss`=14) is now subtracted from
  a hero-build's outgoing damage. Previously the harness gave enemy armor away
  for free in every wave fight, which systematically **overstated** the
  retinue's damage output.

**This changed committed numbers**, which is why `docs/sim/baseline.json` was
deliberately refreshed (`drift_check.py --refresh --yes`) at the same time.
Four of the seven baselined sweeps moved, all in the same direction — enemies
survive more now: `floor1-swarm-wave` enemy survivors 138.62 -> 142.59 at the
shipped defaults, `floor2-ranged-wave` 348.08 -> 353.80. The three that did
NOT move are the correct ones to be unmoved: `D-gate1-frontage-model-sensitivity`
(that scenario is `brood_fodder`-only, Armor 0) and the two point-target
entries `E`/`F` (the point-target model already applied armor, and still does,
identically). `validate.py`'s closed-form checks 1 and 2 are unchanged to the
digit for the same reason.

The remaining approximation is named in `mixed_victim_armor()`'s own docstring
and in `docs/sim/LIMITATIONS.md`: averaging a mixed pool's armor is not
identical to applying armor per victim subgroup, because `effective_blow()`
floors each blow at `chip_floor`. It is close where armor values are similar
and diverges where one subgroup is far tougher than the rest.

## Abilities: what's inert in this tool, and the one that isn't

`hero-builds.json abilities.*.representable` states each ability's own
limit; this tool implements exactly what that field says is possible, for
the **wave-attrition model only** (this tool has no point-target mode):

| Ability | Representable here? | What variety.py does |
|---|---|---|
| `rally_cry` | inert (its own periodic-aura effect is explicitly "not usable at all in the pooled wave-attrition model, which has no per-unit position") | its OWN effect is never applied to any fighter; it still counts toward the `choirs_ward` synergy rule's `ability_id_count` condition, which is a separate, roster-level mechanism |
| `camouflage_strike` | inert ("no") | not applied |
| `shield_regen` | inert ("no") | not applied |
| `last_stand` | inert ("no") | not applied |
| `chain_reactor` | inert here ("partial", representable ONLY in the point-target model, which this tool doesn't run) | not applied |
| `guardian_angel` | **IMPLEMENTED** — its own `representable_note` states it's representable "ONLY as a flat +1 adjustment to the wave-attrition model's final survivor count per instance present in the roster" | after the fight, `variety.py` counts how many rolled builds picked `guardian_angel` and reports `retinue_survivors + that count` as a clearly separate, labeled `retinue_survivors_with_guardian_angel` field/line — a post-hoc headcount correction applied in `variety.py`, NOT inside `combat_model.py` (which stays untouched apart from the additive damage-attribution key) |

Every one of task-079's 6 abilities is accounted for above — none are
silently dropped, per the brief's own instruction.

## Modifications: all six are representable, one caveat already covered

`focus_optic`, `overcharge_core`, `reinforced_plating`, `lightweight_frame`,
`wide_choke` are fully applied via the `build_stat_block` formula (range/
rate-of-fire/damage/HP/move-speed/targets-per-shot multipliers and
additions). `piercing_rounds`' `armor_penetration_flat` is carried but unused
— see "Fields that don't map" above.

## Synergy rules: all four applied, the reading taken on ambiguity

All four rules in `hero-builds.json synergy_rules.rules[]` are evaluated
every run (`variety.py.evaluate_synergy_rules()`), including the anti-synergy
(`babel_discord`), per the brief's explicit instruction that a report which
only applies buffs is wrong.

**The one reading call made:** the schema documents `condition.type` and
`effect.scope` as two separate fields, and (in this data) they happen to
correlate 1:1 per rule (`origin_world_share` <-> `units_matching_
condition_origin`, `origin_world_pair_present` <-> `units_matching_origin_
in_pair`, `ability_id_count`/`distinct_origin_world_count` <-> `whole_
roster`). `evaluate_synergy_rules()` dispatches scope-application off the
rule's own `scope` STRING, not off which condition type fired — a
deliberate choice to keep the evaluator matching "the concrete shape
task-080's evaluator should target" (schema's own words) even if a future
rule ever paired a condition type with a different scope than today's four
rows do. Nothing else about the four rules was ambiguous: `origin_world_
share` uses the roster's single most-common world (ties are impossible to
observe as ambiguous here since `Counter.most_common(1)` just picks one
deterministically; not exercised in any run so far); `ability_id_count`/
`distinct_origin_world_count` are literal counts against a threshold;
`origin_world_pair_present` is a simple presence check on both named worlds.
Every condition counts ROLLED BUILDS (one per roster entry), never
`count_per_build` army headcount, per the schema's own "by unit count, not
soldier headcount" language.

`dps`/`rate_of_fire`/`accuracy` are the only three `effect.stat` values any
rule uses today. `rate_of_fire` and `accuracy` multipliers are applied to
`resolve_hero_build()`'s intermediate components BEFORE
`finalize_hero_build_fighter()` folds them into one `dps` number (schema:
rate_of_fire is read "pre-swing-interval-inversion", accuracy "pre-dps-
computation" — both must move first). A `dps` effect (`kindred_ranks`,
`choirs_ward`) is applied as a final multiplicative wrapper AFTER that fold,
since neither rule's schema note asks for it to happen earlier.

## The metrics block — a declared default, not a settled spec

The owner's ask ("basic metrics below") had no attached list. This task
declares the following as the default set and states here, plainly, that
it's open to change: seed, scenario, builds sampled (roster size), count
per build, active/fired synergies, result, elapsed seconds, retinue
survivors/start (plus the guardian_angel-adjusted figure when nonzero),
enemy survivors/start, total damage dealt by each side
(`combat_model.py`'s new `total_damage_dealt` key — a running sum of the
same `dmg_to_retinue`/`dmg_to_enemy` scalars the tick loop already computed
and previously discarded, nothing new computed), and the `enemy_avg_max_hp`
figure the estimated-kills column is derived from.

## "Estimated kills" — explicitly an estimate, never a body count

The wave-attrition model pools HP per subgroup; it has no notion of
individual bodies dying at any tick. `estimated_kills` for a build =
`group_damage_dealt[build] / enemy_avg_max_hp`, where `enemy_avg_max_hp` is
the named scenario's Enemy.Composition count-weighted average `max_hp`
across all its rows. This is a DAMAGE-TO-EQUIVALENT-BODIES conversion, not
a count of anything the model actually tracked killing — the table header
and this doc both say so; do not read it as "this build killed N enemies."

## `count_per_build` / `--roster` defaults — arbitrary, stated as such

Default `--roster 20` x `--count-per-build 2` = 40 total rolled retinue
headcount, chosen to land roughly on `floor1-swarm-wave.json`'s own real
retinue size (40) rather than run a dozen-vs-hundreds mismatch that would
make every single roll a trivial wipe or a trivial win regardless of which
builds got rolled. This is a documented, arbitrary convenience default, not
a citation to any design number — override either flag for a different
total army size.

## The `combat_model.py` change — what it is, and isn't

One new mechanism only: `simulate_wave_attrition()` already computed each
retinue group's outgoing damage per tick (`group_output`, and the ranged
equivalent) and threw it away. It's now accumulated, per-`WaveGroup.name`,
across the whole fight, into two NEW return keys:

- `group_damage_dealt: {wave_group_name: total_damage_over_the_fight}` —
  melee entries have the same `contact_scale` applied that the pooled
  `dmg_to_enemy_melee` total already used; ranged entries are uncapped, same
  as the pooled total.
- `total_damage_dealt: {"retinue": ..., "enemy": ...}` — the same per-tick
  `dmg_to_retinue`/`dmg_to_enemy` scalars, summed instead of discarded.

No existing return key was renamed, reordered, or changed in value, and the
attribution keys themselves changed no math. That claim was originally proved
by `drift_check.py` passing against an UNCHANGED `docs/sim/baseline.json`.

**The armor fix of 2026-07-29 is a separate, deliberate math change** and it
did move committed numbers — see "Armor IS applied" above. `baseline.json` was
refreshed explicitly (`--refresh --yes`) at that point, so the current clean
`drift_check` run is measured against the post-armor-fix baseline, not the
original one.

## Which axes actually move the ranking at these defaults (and which don't)

Not every axis pick changes a build's rank, and knowing which ones don't is
the point of this section — a tie means "indistinguishable to the model",
never "equally good in the game".

**Measured, before and after the 2026-07-29 armor/penetration fix.** Method:
`--roster 20 --count-per-build 2`, seeds 1-10 on both `floor2-ranged-wave` and
`floor1-swarm-wave` (20 rolls), comparing every same-chassis/weapon/projectile/
origin_world pair in each roster that differs in modification and/or ability.
36 such pairs exist across the 20 rolls, both times.

| | pairs tied | share |
|---|---|---|
| before the fix (`per_unit_dps`) | 30 of 36 | 83% |
| after the fix (`damage_dealt`) | 17 of 36 | 47% |

The modification axis went from mostly invisible to mostly visible, because
`piercing_rounds` is now applied and because enemy armor now varies what a
build's damage is worth. Concretely, on `floor2-ranged-wave --seed 1`, ranks
3/4/5 (Beastcaller / Cleave Melee Sweep / Shrapnel Spread, all dps 25.38) used
to read an identical 319.8 damage across `piercing_rounds`,
`lightweight_frame` and `none`. They now read **319.8 / 298.9 / 298.9** — the
piercing build pulls ~7% ahead against that scenario's 112 `brood_soldier_melee`
at Armor 6.

**What still ties, and why.** The residual 47% is dominated by the ability
axis, not modifications:

- **Abilities** — 5 of 6 are inert in this model (see the ability table
  above). Burst, regen, stealth and positioning effects have nothing to attach
  to in a pooled attrition model. This is a model-primitive gap, not a data
  problem, and it is the largest remaining source of ties.
- **`lightweight_frame`** — its `max_hp_mult` 0.9 IS read, but a 10% HP swing
  usually doesn't change the outcome before the roster is wiped, so it
  frequently still ties in practice. Its `move_speed_scale_mult` is never read:
  no movement primitive exists (`LIMITATIONS.md` §4).

**Axes that DO move the ranking:** `chassis`, `weapon_archetype` (the dominant
driver — every top-3 build across the evidence runs is a `cleave_melee_sweep`,
`turret_autocannon` or `siege_artillery` pick), `origin_world` (via the synergy
rules), and **all six modifications** now have at least one live path —
`focus_optic`/`overcharge_core` through dps, `wide_choke` through cleave,
`reinforced_plating`/`lightweight_frame` through HP, and `piercing_rounds`
through armor penetration. `guardian_angel` is the one ability that moves
anything, via the post-hoc survivor adjustment rather than the ranking.

**The decision-relevant read for design:** weapons and chassis carry the
variety; modifications now contribute real differentiation; abilities are
currently invisible to any comparison run through this harness. If abilities
are meant to be a load-bearing part of hero variety, the model needs
primitives it does not have — that is a scoping decision about the harness,
not a flaw in `hero-builds.json`.

## Evidence

```
py Scripts/sim/validate.py
```
Unchanged from before this task: checks 1, 2, 4 PASS; check 3 (GATE1
wave-attrition reproduction) FAILs exactly as `docs/sim/LIMITATIONS.md` §1
already documents (retinue survivors 0.00 of 120 vs the measured 109-111
band) — same failure, same numbers, this task changed nothing about it.

```
py Scripts/sim/drift_check.py
```
`RESULT: CLEAN. No drift beyond tolerance in any baselined sweep.` — all 7
baselined entries (A-G) report `OK` against the unmodified
`docs/sim/baseline.json`, confirming the `combat_model.py` change is purely
additive.

```
py Scripts/sim/variety.py --scenario floor2-ranged-wave --seed 1
py Scripts/sim/variety.py --scenario floor2-ranged-wave --seed 1   # re-run
```
Byte-identical `--json` output on both runs (diffed clean). Top of the
`--seed 1` table: `Line Breaker (Cleave Melee Sweep)` / Chain Bolt /
Reinforced Plating / Guardian Angel / Verdant Wilds, dps 28.20, damage dealt
440.1, share 20.2%, WIPED. Fired: `twin_world_resonance`, `babel_discord`.
(Pre-armor-fix this row read 463.9 damage / 19.7% share — the drop is enemy
armor now being subtracted.)

```
py Scripts/sim/variety.py --scenario floor1-swarm-wave --seed 2
```
Different roll, different table (confirms seed-dependence): top build
`Line Breaker (Cleave Melee Sweep)` / Melee Swing / Reinforced Plating /
Last Stand / Verdant Wilds, dps 29.61, damage dealt 1284.6, share 33.7%,
est. kills 17.94, WIPED. `choirs_ward` fired this run (2+ Rally Cry builds
rolled) in addition to `twin_world_resonance` and `babel_discord` — all
three visibly different from the `--seed 1` run's fired-rule set, confirming
synergy evaluation responds to the actual roll rather than being fixed.

```
py Scripts/sim/variety.py --scenario floor2-ranged-wave --seed 42
```
A third seed, different scenario roster composition, different top build
(`Beastcaller (Cleave Melee Sweep)` / Shrapnel Spread / none / Last Stand /
Verdant Wilds, dps 26.65, damage dealt 369.0, share 33.5%) — spread across
three independent rolls is visible: `--seed 1`'s and `--seed 2`'s top builds
differ from each other and from `--seed 42`'s.

```
py Scripts/sim/variety.py --scenario floor1-swarm-wave --seed 7
```
Added after a review caught `share_of_retinue_damage` dividing by the wrong
`total_damage_dealt` key — `variety.py` originally read `["retinue"]`
(damage dealt TO the retinue, incoming) instead of `["enemy"]` (damage dealt
BY the retinue, outgoing, the actual share denominator wanted — the
metrics-block print above it already used the correct `retinue -> enemy`
convention). This let a share exceed 100% on this exact seed/scenario:
rank 1's `damage_dealt` was 4620.4 against a wrong denominator
(`total_damage_dealt["retinue"]` 3241.92) = 142.5%. **Fixed**
(`variety.py` now reads `["enemy"]`; correct denominator
`total_damage_dealt["enemy"]` 9941.6 gives 46.5%) and a runnable check now
runs on every invocation (`assert abs(share_sum - 1.0) < 0.01` across the
whole roster) so a repeat of this exact class of bug fails loudly instead of
silently. Top build after the fix: `Beastcaller (Cleave Melee Sweep)` /
Melee Swing / Lightweight Frame / none / Iron Reach, dps 29.61, damage dealt
4620.4, **share 46.5%**, est. kills 62.71, WIPED. The assertion held (no
`AssertionError`) on all four evidence runs in this section.

Every scenario run above ends `result: retinue_wiped` at the documented
default `--roster 20 --count-per-build 2` (40 total retinue vs. the named
scenario's real 250-450 enemy population) — consistent with, not contrary
to, `docs/sim/LIMITATIONS.md` §1's finding that this model does not
reproduce measured survival at these scales; it is not a new failure this
task introduced.
