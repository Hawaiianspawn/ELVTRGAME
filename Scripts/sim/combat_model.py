"""
combat_model.py — the actual math. Two independent models, because the game
has two structurally different fight shapes:

1. POINT-TARGET model (`ttk_1v1`, `army_ttk_vs_point_target`): a single
   large-collision entity (Elite/Titan/Boss, or a lone Militia-vs-Fodder sanity
   check) fought by melee attackers that geometrically CAN'T all reach it at
   once (surround-capped) plus ranged attackers that can. This is the same
   closed-form method docs/design/entity-tiers.md §7 already validated
   in-repo — this module reproduces it exactly (see validate.py's bonus
   check), it does not invent a new one.

2. WAVE-ATTRITION model (`simulate_wave_attrition`): two amorphous pooled
   forces (retinue vs a swarm population). This is the model that matters and
   the one the discarded scaling-curve.md §7 sim got wrong IN KIND — it
   applied 100% of the enemy population's DPS simultaneously with no
   concurrency limit. The fix here is a FRONTAGE cap: only a geometry-bounded
   slice of each side's melee bodies can be in physical contact at once. See
   `exposed_frontage()`'s docstring for the derivation, and
   docs/sim/MODEL.md for the full writeup including what this does NOT fix
   (arrival timing — see docs/sim/LIMITATIONS.md).

Every number this module needs that ISN'T a per-entity stat block comes from
docs/data/scenarios/combat-model-constants.json, loaded by data_loader.py —
nothing is hardcoded here except the discrete-swing formula itself, which is
the shipped mechanic GATE1-FUN-PROTOTYPE.md §3b documents in prose (blow =
DPS x SwingInterval; EffectiveBlow = max(blow - Armor, ArmorChipFloor)).
"""

from __future__ import annotations

import math
import random
from dataclasses import dataclass, field


# ---------------------------------------------------------------------------
# Shared primitives
# ---------------------------------------------------------------------------

def blow(dps: float, swing_interval: float) -> float:
    """Damage delivered in one swing. GATE1-FUN-PROTOTYPE.md §3b: 'a blow
    removes DPS x SwingInterval at once.'"""
    return dps * swing_interval


def effective_blow(
    attacker_blow: float,
    victim_armor: float,
    chip_floor: float,
    armor_penetration: float = 0.0,
) -> float:
    """
    entity-tiers.md §2.2: EffectiveBlow = max(AttackerBlow - Armor, ArmorChipFloor).

    `armor_penetration` extends that with the term
    docs/data/hero-builds.json's `piercing_rounds` modification declares:

        EffectiveBlow = max(AttackerBlow - max(Armor - ArmorPenetrationFlat, 0),
                            ArmorChipFloor)

    Penetration reduces the victim's armor, floored at 0 — it never becomes a
    damage bonus against an unarmored victim, so a build that rolls
    piercing_rounds is never WORSE off and never gains anything for free. The
    default 0.0 leaves every pre-existing caller bit-identical.
    """
    return max(attacker_blow - max(victim_armor - armor_penetration, 0.0), chip_floor)


def steady_state_dps(attacker: dict, victim_armor: float, chip_floor: float) -> float:
    """
    Effective DPS one attacker deals into one victim, armor applied.

    Reads `armor_penetration_flat` off the attacker's own fighter dict rather
    than taking it as a parameter, so every call site gets penetration support
    without a signature change. Fighter dicts that don't carry the key (every
    one built by `enemy_fighter()`/`retinue_fighter()`/`hero_fighter()`) behave
    exactly as before.
    """
    a_blow = blow(attacker["dps"], attacker["swing_interval"])
    e_blow = effective_blow(
        a_blow, victim_armor, chip_floor,
        attacker.get("armor_penetration_flat", 0.0),
    )
    return e_blow / attacker["swing_interval"]


def ttk_1v1(attacker: dict, victim: dict, chip_floor: float) -> float:
    """Seconds for one attacker to solo-kill one victim."""
    dps = steady_state_dps(attacker, victim.get("armor", 0.0), chip_floor)
    return victim["max_hp"] / dps


# ---------------------------------------------------------------------------
# Point-target army TTK — same method as entity-tiers.md §7
# ---------------------------------------------------------------------------

@dataclass
class ArmyGroup:
    name: str
    fighter: dict
    count: int


def army_ttk_vs_point_target(
    target: dict,
    groups: list[ArmyGroup],
    chip_floor: float,
    hero: dict | None = None,
) -> tuple[float, dict]:
    """
    Melee groups are capped at target['surround_cap_estimate'] simultaneous
    attackers (None/absent = uncapped, e.g. for a 1v1 sanity check). Ranged
    groups are never capped — entity-tiers.md §4's own finding: 'Ranged
    attackers are not subject to this cap... every archer in the army can
    contribute simultaneously.'
    """
    total_dps = 0.0
    breakdown = {}
    cap = target.get("surround_cap_estimate")

    for g in groups:
        engaged = g.count if g.fighter["role"] == "ranged" or cap is None else min(g.count, cap)
        per_unit_dps = steady_state_dps(g.fighter, target["armor"], chip_floor)
        group_dps = engaged * per_unit_dps
        breakdown[g.name] = {
            "role": g.fighter["role"],
            "count": g.count,
            "engaged": engaged,
            "per_unit_dps": round(per_unit_dps, 3),
            "group_dps": round(group_dps, 3),
        }
        total_dps += group_dps

    if hero is not None:
        hero_dps = steady_state_dps(hero, target["armor"], chip_floor)
        breakdown["hero"] = {"role": "melee", "count": 1, "engaged": 1,
                              "per_unit_dps": round(hero_dps, 3), "group_dps": round(hero_dps, 3)}
        total_dps += hero_dps

    if total_dps <= 0:
        return math.inf, breakdown
    return target["max_hp"] / total_dps, breakdown


# ---------------------------------------------------------------------------
# Wave-attrition model — frontage-capped, two-sided, pooled HP
# ---------------------------------------------------------------------------

def exposed_frontage(alive_melee_count: float, formation_spacing: float, engaged_spacing: float) -> float:
    """
    Fermi estimate of how many of a distributed force's MELEE bodies can be in
    simultaneous physical contact at the perimeter of its own formation — the
    generalization of entity-tiers.json's SurroundCapEstimate (built for a
    single point target) to a distributed blob/ring formation. NOT measured;
    same epistemic status as every SurroundCapEstimate in this repo.

    Derivation (docs/sim/MODEL.md has the full writeup):
      - Footprint area of a compact formation of N units at spacing S
        (square-grid packing approx): Area ~= N * S^2
      - Disk radius of that footprint: r = S * sqrt(N / pi)
      - Disk perimeter: P = 2 * pi * r = 2 * S * sqrt(pi * N)
      - Exposed slots on that perimeter, packed at the tighter
        engaged/compressed spacing: P / engaged_spacing

    formation_spacing should be the AT-REST spacing of the force whose
    perimeter this is (GATE1-FUN-PROTOTYPE.md §3a measured 86uu for the
    retinue's ring formation). engaged_spacing is how tightly attacking
    bodies can pack against that perimeter (§3a measured 25-51uu compressed,
    median ~43-51 — see combat-model-constants.json for the exact value used
    and its sensitivity range).
    """
    if alive_melee_count <= 0:
        return 0.0
    radius = formation_spacing * math.sqrt(alive_melee_count / math.pi)
    perimeter = 2.0 * math.pi * radius
    return max(1.0, min(alive_melee_count, perimeter / engaged_spacing))


def mixed_victim_armor(victim_groups: list["WaveGroup"]) -> float:
    """
    Alive-count-weighted mean Armor across the groups that will actually absorb
    this tick's damage.

    The wave model pools HP and distributes each side's damage across that
    side's subgroups proportional to current alive-count share (the well-mixed
    target assumption stated in simulate_wave_attrition()'s step 6). A single
    attacking group therefore has no one identifiable victim whose Armor to
    subtract — it is hitting a mixture. Weighting Armor by the same alive-count
    share the damage itself is split by is the assumption the rest of the model
    already makes, applied consistently rather than a new one.

    This is an APPROXIMATION and it is not conservative in both directions:
    because effective_blow() floors at chip_floor per blow, averaging the armor
    of a mixed pool is not identical to running each subgroup separately and
    summing. It is close where armor values are similar and diverges where one
    subgroup is far tougher than the rest (e.g. a Titan at Armor 20 mixed with
    Fodder at Armor 0). Splitting damage per victim subgroup before applying
    armor would remove the approximation; that is a larger restructuring of the
    tick loop than correcting the hardcoded zero, and is noted in
    docs/sim/LIMITATIONS.md rather than done here.
    """
    total = sum(g.alive_count for g in victim_groups)
    if total <= 0:
        return 0.0
    return sum(g.fighter.get("armor", 0.0) * g.alive_count for g in victim_groups) / total


def melee_reach_per_exposed_unit(engage_range: float, engaged_spacing: float, facing_fraction: float) -> float:
    """
    Fermi estimate of how many DISTINCT enemy bodies simultaneously sit
    within ONE exposed unit's own melee EngageRange — the bound that governs
    cleave (TargetsPerHit) capacity.

    ADDED after review caught a structural bug: an earlier version of this
    model bounded cleave-target availability by reusing
    `engaging_enemy_melee` (the MaxAttackersPerUnit-capped INCOMING-attacker
    count). Substituting the two definitions showed the frontage term
    (`exposed_retinue`) cancels identically whenever the frontage cap binds,
    collapsing the ratio to a CONSTANT `max_attackers_per_unit /
    targets_per_hit` regardless of spacing, formation size, or any swept
    parameter — silently deleting TargetsPerHit's effect (the retinue's
    entire designed cleave advantage) from the model. See docs/sim/MODEL.md
    'Two different physical limits, not one' and docs/sim/VALIDATION.md for
    the full account of that bug and why the original sweep it produced was
    an algebraic tautology, not a finding.

    "How many can hit me" (MaxAttackersPerUnit, an adjacency/elbow-room
    limit at point-blank contact) and "how many I can reach with a weapon"
    (TargetsPerHit within EngageRange, a longer, geometrically distinct
    reach — GATE1-FUN-PROTOTYPE.md §3b: geometric Kth-nearest-within-range
    targeting) are different physical quantities and must not share a
    derivation. This function computes the second one independently:

      - Circle of radius EngageRange around one exposed unit: area = pi * R^2
      - facing_fraction of that circle is enemy-occupied territory, not the
        unit's own formation's interior (a perimeter unit's neighbours-of-
        the-same-side fill roughly the other half) — a NEW, separately
        flagged Fermi estimate, sensitivity-tested in VALIDATION.md same as
        every other one in this file.
      - Enemy bodies that fit in that area at the compressed engaged_spacing:
        (pi * R^2 * facing_fraction) / engaged_spacing^2

    Fermi estimate, NOT measured — same status as exposed_frontage() and
    entity-tiers.json's SurroundCapEstimate.
    """
    if engaged_spacing <= 0:
        return 0.0
    area = math.pi * engage_range * engage_range * facing_fraction
    footprint = engaged_spacing * engaged_spacing
    return area / footprint


@dataclass
class WaveGroup:
    name: str
    fighter: dict
    count: float
    arrival_seconds: float = 0.0
    hp_pool: float = field(init=False)

    def __post_init__(self):
        self.hp_pool = self.fighter["max_hp"] * self.count

    @property
    def alive_count(self) -> float:
        if self.fighter["max_hp"] <= 0:
            return 0.0
        return max(0.0, self.hp_pool / self.fighter["max_hp"])

    def has_arrived(self, t: float) -> bool:
        """
        task-068 (docs/sim/LIMITATIONS.md §1 candidate 1). A group with
        arrival_seconds > t is not yet in contact range — it exists (its
        hp_pool is intact, it still counts toward the population totals that
        decide when the fight actually ends) but contributes to NOTHING in
        the per-tick combat math: not exposed_frontage, not the
        incoming/outgoing damage bounds, not the damage-application split,
        and it cannot be damaged. Default 0.0 (already arrived) preserves
        every existing scenario's behavior exactly — this is opt-in per row.
        """
        return t >= self.arrival_seconds


@dataclass
class WaveTickRecord:
    t: float
    retinue_alive: float
    enemy_alive: float
    exposed_retinue: float
    dmg_to_retinue: float
    dmg_to_enemy: float


def simulate_wave_attrition(
    retinue_groups: list[WaveGroup],
    enemy_groups: list[WaveGroup],
    chip_floor: float,
    max_attackers_per_unit: float,
    formation_spacing: float,
    engaged_spacing: float,
    melee_contact_facing_fraction: float,
    dt: float,
    max_time: float,
    log_every_seconds: float = 5.0,
) -> dict:
    """
    Frontage-capped two-sided attrition, ticking at the shared discrete swing
    interval (dt). Each tick:

      1. Compute each side's live MELEE alive-count (ranged groups excluded —
         they never contend for frontage).
      2. exposed_retinue = frontage estimate off the retinue's own melee
         alive-count (its own ring/blob formation is what bounds how many of
         its bodies are reachable at once).
      3. engaging_enemy_melee = min(enemy melee alive, exposed_retinue *
         max_attackers_per_unit) — GATE1 §3b: 'incoming damage is still at
         most MaxAttackersPerUnit x enemy DPS' per exposed victim. This is
         the INCOMING bound only.
      4. Melee damage OUT uses a SEPARATE, independently-derived bound
         (`melee_reach_per_exposed_unit`, keyed off EngageRange, not
         MaxAttackersPerUnit) — see that function's docstring for why: an
         earlier version of this model reused `engaging_enemy_melee` here,
         which made the retinue's cleave advantage (TargetsPerHit)
         algebraically cancel out of the fight outcome entirely whenever the
         frontage cap bound (contact_scale collapsed to the constant
         `max_attackers_per_unit / targets_per_hit`, independent of
         exposed_retinue, spacing, or anything else swept). Fixed after
         review; see docs/sim/VALIDATION.md.
      5. Ranged groups on both sides ALWAYS fully engage (entity-tiers.md §4:
         ranged is not surround-capped) — added on top, uncapped.
      6. Damage is distributed across each side's own subgroups proportional
         to current alive-count share (well-mixed-target assumption — this
         was NOT the flaw in the discarded model; the flaw was skipping step
         2-3 entirely and applying 100% of both populations' DPS every tick).

    Stops when max_time is reached or either side's pooled HP hits 0.

    ARRIVAL GATING (task-068, docs/sim/LIMITATIONS.md §1 candidate 1). Any
    WaveGroup may carry arrival_seconds > 0 (docs/data/encounter-budget.json
    rank_arrival_timing[]). A not-yet-arrived group still EXISTS — its
    hp_pool is untouched and it counts toward total_alive() for the
    stop-condition and the final survivor report, because it has not been
    fought yet, not because it has been defeated — but it is excluded from
    every per-tick combat computation below: it is not part of
    exposed_retinue's frontage input, not part of the enemy_melee_alive that
    bounds engaging_enemy_melee (incoming) or contact_scale (outgoing), not a
    target retinue cleave can reach, and it receives none of dmg_to_enemy
    this tick. This is a straightforward reading of what "not in contact
    range yet" means physically — no new free parameter, arrival_seconds
    just partitions each group's timeline into "off the field" / "on it."
    """
    t = 0.0
    log: list[WaveTickRecord] = []
    next_log_t = 0.0

    # task-080 (Scripts/sim/variety.py): per-retinue-group damage attribution,
    # ADDITIVE only — accumulates numbers the tick loop below already
    # computes and previously discarded (group_output, and the ranged
    # equivalent). Does not change dmg_to_retinue / dmg_to_enemy_melee /
    # dmg_to_enemy_ranged / dmg_to_enemy or any existing return key.
    group_damage_dealt: dict[str, float] = {}
    total_dmg_to_retinue = 0.0
    total_dmg_to_enemy = 0.0

    def total_alive(groups, role_filter=None):
        return sum(g.alive_count for g in groups if role_filter is None or g.fighter["role"] == role_filter)

    def arrived(groups, current_t, role_filter=None):
        return [g for g in groups
                if (role_filter is None or g.fighter["role"] == role_filter) and g.has_arrived(current_t)]

    while t < max_time:
        # total_alive() is NOT arrival-filtered — a group that hasn't arrived
        # yet still exists (full HP), so it must still count as "alive" for
        # the stop condition and the final survivor totals. arrived() below
        # is the gate on everything that actually happens in combat this tick.
        retinue_alive_total = total_alive(retinue_groups)
        enemy_alive_total = total_alive(enemy_groups)

        if retinue_alive_total <= 0 or enemy_alive_total <= 0:
            break

        retinue_melee_active = arrived(retinue_groups, t, "melee")
        retinue_ranged_active = arrived(retinue_groups, t, "ranged")
        enemy_melee_active = arrived(enemy_groups, t, "melee")
        enemy_ranged_active = arrived(enemy_groups, t, "ranged")

        retinue_melee_alive = sum(g.alive_count for g in retinue_melee_active)
        enemy_melee_alive = sum(g.alive_count for g in enemy_melee_active)

        exposed_retinue = exposed_frontage(retinue_melee_alive, formation_spacing, engaged_spacing)

        # Each side's mixed Armor, weighted over exactly the pool that receives
        # that side's damage in the apply step below (melee + ranged arrived
        # groups, both sides). Previously both were hardcoded 0.0 with the
        # comment "retinue has no Armor column" — true of upgrades.json, but it
        # silently discarded the ENEMY's Armor too (entity-tiers.json:
        # brood_soldier_melee 6, brood_elite 12, brood_titan 20, brood_boss 14),
        # and docs/data/hero-builds.json now gives the retinue side nonzero
        # Armor as well (combat_engineer 2, beastcaller 1).
        retinue_victim_armor = mixed_victim_armor(retinue_melee_active + retinue_ranged_active)
        enemy_victim_armor = mixed_victim_armor(enemy_melee_active + enemy_ranged_active)

        # --- damage INTO retinue (melee contact, frontage-capped) ---
        engaging_enemy_melee = min(enemy_melee_alive, exposed_retinue * max_attackers_per_unit)
        dmg_to_retinue_melee = 0.0
        if enemy_melee_alive > 0:
            for g in enemy_melee_active:
                share = g.alive_count / enemy_melee_alive
                bodies = engaging_enemy_melee * share
                per_unit_dps = steady_state_dps(g.fighter, retinue_victim_armor, chip_floor)
                dmg_to_retinue_melee += bodies * per_unit_dps * dt

        # --- damage INTO retinue (ranged, uncapped) ---
        dmg_to_retinue_ranged = 0.0
        for g in enemy_ranged_active:
            per_unit_dps = steady_state_dps(g.fighter, retinue_victim_armor, chip_floor)
            dmg_to_retinue_ranged += g.alive_count * per_unit_dps * dt

        dmg_to_retinue = dmg_to_retinue_melee + dmg_to_retinue_ranged

        # --- damage INTO enemy (melee, exposed retinue's own cleave capacity) ---
        # A cleave-K attacker delivers up to K blows, but only if K DISTINCT
        # enemies are within ITS OWN weapon reach — a per-unit local-density
        # question, answered by melee_reach_per_exposed_unit(), NOT by
        # dividing the incoming-attacker bound (engaging_enemy_melee) across
        # exposed units. Those are different physical limits: "how many can
        # hit me" (adjacency, MaxAttackersPerUnit) vs "how many can I reach"
        # (weapon range, EngageRange) — conflating them previously made
        # TargetsPerHit cancel out of the fight outcome (see the function's
        # docstring and docs/sim/VALIDATION.md's account of that bug).
        dmg_to_enemy_melee = 0.0
        if retinue_melee_alive > 0 and enemy_melee_alive > 0:
            raw_output = 0.0     # potential damage if every reachable cleave slot connects
            raw_slots = 0.0      # potential target-slots claimed, from LOCAL reach, not incoming-attacker count
            raw_output_by_group: dict[str, float] = {}  # task-080 attribution, pre-contact_scale
            for g in retinue_melee_active:
                share = g.alive_count / retinue_melee_alive
                exposed_share = exposed_retinue * share
                reach = melee_reach_per_exposed_unit(
                    g.fighter["engage_range"], engaged_spacing, melee_contact_facing_fraction
                )
                cleave = min(g.fighter["targets_per_hit"], reach)
                per_unit_dps = steady_state_dps(g.fighter, enemy_victim_armor, chip_floor)
                group_slots = exposed_share * cleave
                group_output = group_slots * per_unit_dps * dt
                raw_slots += group_slots
                raw_output += group_output
                raw_output_by_group[g.name] = raw_output_by_group.get(g.name, 0.0) + group_output
            # can't hit more enemies than have arrived, full stop — this is
            # the ONLY population-level cap left on the outgoing side,
            # independent of MaxAttackersPerUnit (and now also arrival-gated:
            # a not-yet-arrived enemy is not a reachable target either)
            contact_scale = 1.0 if raw_slots <= 0 else min(1.0, enemy_melee_alive / raw_slots)
            dmg_to_enemy_melee = raw_output * contact_scale
            # task-080 attribution: same contact_scale applied per group as
            # was applied to the pooled total above — additive record-keeping
            # only, doesn't feed back into dmg_to_enemy_melee's own math.
            for name, raw in raw_output_by_group.items():
                group_damage_dealt[name] = group_damage_dealt.get(name, 0.0) + raw * contact_scale

        # --- damage INTO enemy (ranged, uncapped) ---
        dmg_to_enemy_ranged = 0.0
        for g in retinue_ranged_active:
            per_unit_dps = steady_state_dps(g.fighter, enemy_victim_armor, chip_floor)
            output = g.alive_count * per_unit_dps * dt
            dmg_to_enemy_ranged += output
            # task-080 attribution: uncapped, so no contact_scale to apply.
            group_damage_dealt[g.name] = group_damage_dealt.get(g.name, 0.0) + output

        dmg_to_enemy = dmg_to_enemy_melee + dmg_to_enemy_ranged
        total_dmg_to_retinue += dmg_to_retinue
        total_dmg_to_enemy += dmg_to_enemy

        # --- apply, distributed proportional to current ARRIVED alive share
        # only — a not-yet-arrived group is off the field and takes none of
        # this tick's damage, same logic as it dealing none of it above.
        retinue_active_alive_total = sum(g.alive_count for g in retinue_melee_active) + \
            sum(g.alive_count for g in retinue_ranged_active)
        enemy_active_alive_total = enemy_melee_alive + sum(g.alive_count for g in enemy_ranged_active)

        if retinue_active_alive_total > 0:
            for g in retinue_melee_active + retinue_ranged_active:
                share = g.alive_count / retinue_active_alive_total
                g.hp_pool = max(0.0, g.hp_pool - dmg_to_retinue * share)
        if enemy_active_alive_total > 0:
            for g in enemy_melee_active + enemy_ranged_active:
                share = g.alive_count / enemy_active_alive_total
                g.hp_pool = max(0.0, g.hp_pool - dmg_to_enemy * share)

        if t >= next_log_t:
            log.append(WaveTickRecord(
                t=round(t, 1),
                retinue_alive=round(total_alive(retinue_groups), 2),
                enemy_alive=round(total_alive(enemy_groups), 2),
                exposed_retinue=round(exposed_retinue, 2),
                dmg_to_retinue=round(dmg_to_retinue, 1),
                dmg_to_enemy=round(dmg_to_enemy, 1),
            ))
            next_log_t += log_every_seconds

        t += dt

    final_retinue = total_alive(retinue_groups)
    final_enemy = total_alive(enemy_groups)
    return {
        "elapsed_seconds": round(t, 2),
        "retinue_survivors": round(final_retinue, 2),
        "enemy_survivors": round(final_enemy, 2),
        "retinue_start": sum(g.count for g in retinue_groups),
        "enemy_start": sum(g.count for g in enemy_groups),
        "result": (
            "retinue_wiped" if final_retinue <= 0.5 else
            "enemy_wiped" if final_enemy <= 0.5 else
            "timed_out"
        ),
        "log": log,
        # task-080 (Scripts/sim/variety.py) additions, NEW keys only — nothing
        # above this line changed. group_damage_dealt: total damage dealt
        # OVER THE WHOLE FIGHT, keyed by retinue WaveGroup.name (one hero
        # build = one WaveGroup, so this is per-build attribution "for
        # free" — melee entries already have contact_scale applied, ranged
        # entries are uncapped, matching how dmg_to_enemy itself is composed).
        # total_damage_dealt: the same tick-by-tick dmg_to_retinue/dmg_to_enemy
        # scalars this function already computed, summed rather than discarded.
        "group_damage_dealt": {k: round(v, 2) for k, v in group_damage_dealt.items()},
        "total_damage_dealt": {
            "retinue": round(total_dmg_to_retinue, 2),
            "enemy": round(total_dmg_to_enemy, 2),
        },
    }


# ---------------------------------------------------------------------------
# Variance layer (task-076) — OPTIONAL. Nothing above this line changed, and
# neither simulate_wave_attrition() nor army_ttk_vs_point_target() takes an
# rng parameter or gets one added: variance is applied to their INPUTS by the
# caller (scenario_runner.py's compute_trial/run_trials), never woven into
# the validated deterministic core. Every function below is a pure sampling
# helper — takes an explicit `random.Random` instance, returns a perturbed
# COPY, never mutates its input, and is never called unless a variance
# source is both (a) enabled in docs/data/scenarios/combat-model-
# constants.json's `variance_model` block and (b) an rng was actually
# supplied (i.e. a caller asked for a seeded/trial run). See
# docs/sim/MODEL.md's variance-layer section for the derivation and citation
# status of each source's magnitude, and docs/sim/LIMITATIONS.md for what a
# spread produced by these may NOT be used to argue.
# ---------------------------------------------------------------------------

def jitter_arrival_seconds(arrival_seconds: float, rng: random.Random, magnitude: float) -> float:
    """
    CITED. `Swarm.BroodSpeedJitter` (docs/data/encounter-budget.json's
    `rank_arrival_source_cvars`) is a real shipped +/-6% per-brood speed
    jitter at spawn. That file's own `rank_arrival_formula` divides travel
    time by `BroodSpeed * (1 +/- jitter)`, i.e. ArrivalSeconds scales by
    `1 / (1 + jitter)` — reproduced here exactly rather than approximated as
    a linear +/- on arrival_seconds itself. Confirmed against that file's own
    committed numbers: `gate1_calibration_wave1_rank0`'s nominal 5.85s with
    jitter=+0.06 -> 5.85/1.06=5.518s (file's ArrivalSecondsFast: 5.52),
    jitter=-0.06 -> 5.85/0.94=6.223s (file's ArrivalSecondsSlow: 6.23) — both
    match that file's own precomputed Fast/Slow brackets to rounding.

    A group with arrival_seconds <= 0.0 (already-arrived — the default for
    every scenario that doesn't opt into per-rank arrival timing) is left
    untouched: there is no shipped meaning to jittering "arrives at t=0."
    """
    if arrival_seconds <= 0:
        return arrival_seconds
    jitter = rng.uniform(-magnitude, magnitude)
    return arrival_seconds / (1.0 + jitter)


def jitter_fighter_dps(fighter: dict, rng: random.Random, magnitude: float) -> dict:
    """
    HARNESS-INVENTED — no citation. No shipped CVar or committed data file
    describes swing-to-swing damage-roll variance for this game; this exists
    only so the variance layer has a second, uncited source alongside the
    cited `jitter_arrival_seconds`, flagged as diagnostic wherever it's
    enabled (see combat-model-constants.json's `variance_model.
    damage_roll_jitter` block and scenario_runner.py's `diagnostic_
    invented_variance`). Samples ONE multiplicative DPS factor per fighter
    GROUP for the whole trial (not per-swing, not per-body) — a coarse
    "some groups roll hot, some roll cold this fight" approximation, which is
    the finest grain this pooled/discrete-swing model has a seam for; there
    is no per-swing event loop to attach a per-hit roll to. Returns a
    shallow copy; never mutates the input fighter dict (the same dict object
    is reused across every trial by scenario_runner.py's data loading, so
    mutating it in place would leak jitter from one trial into the next).
    """
    jittered = dict(fighter)
    jittered["dps"] = fighter["dps"] * (1.0 + rng.uniform(-magnitude, magnitude))
    return jittered
