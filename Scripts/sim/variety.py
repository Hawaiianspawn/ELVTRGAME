"""
variety.py — task-080: roll a random LEGAL roster out of docs/data/hero-builds.json
(task-079's chassis x weapon x projectile x modification x ability x
origin-world space), evaluate task-079's synergy rules against it, run the
roster as the RETINUE side of an existing docs/data/scenarios/<name>.json's
wave-attrition fight (the enemy side is taken straight from that scenario,
unchanged), and report a metrics block plus an ASCII top-10 performers table
ranked by per-build damage output.

Usage:
    py Scripts/sim/variety.py --scenario <name> --seed <n> [--roster <k>]
        [--count-per-build <n>] [--json]

task-086 adds a second mode that reuses this same table-printing shape for a
different, much smaller question — see run_subtypes()/print_subtype_report()
below and docs/sim/SUBTYPE-VARIETY.md:

    py Scripts/sim/variety.py --mode subtypes --scenario <name> [--seed <n>] [--json]

READ docs/sim/VARIETY.md before trusting a number out of this — short
version: this is a WAVE-ATTRITION result, and docs/sim/LIMITATIONS.md §1
already states that model does not reproduce GATE1's measured survival at
this harness's committed defaults. The top-10 table ranks builds RELATIVE TO
EACH OTHER inside one shared, imperfect model — never an absolute claim
about how a build performs in the real game.

task-090 fixed weapon_id/projectile_id being rolled independently (see
sample_roster()'s docstring below) — a roll now picks the weapon first, then
the projectile from that weapon's own physically-compatible subset of the
chassis's legal_projectiles, per hero-builds.json's
weapon_projectile_coupling_note.

No stat blocks are hardcoded here — every fighter comes from data_loader.py.
No global random — random.Random(seed) only. No third-party dependencies.
"""

from __future__ import annotations

import argparse
import json
import operator
import random
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import data_loader as dl
import combat_model as cm
import scenario_runner as sr  # read-only reuse of to_json_safe(); never edited by this task

_OPS = {">=": operator.ge, ">": operator.gt, "<=": operator.le, "<": operator.lt, "==": operator.eq}


# ---------------------------------------------------------------------------
# 1. Sampling — LEGAL builds only (chassis first, then each other axis drawn
#    from THAT chassis's own legal_* list — a uniform draw over the raw
#    235,200-build product would roll off-theme combinations task-079's
#    chassis-coherence table exists to rule out).
#
# task-090: weapon and projectile are no longer drawn independently. A weapon
# is picked from the chassis's legal_weapons first, THEN the projectile is
# drawn only from that weapon's own physically-compatible subset of the
# chassis's legal_projectiles — chassis.legal_projectiles ∩ {p :
# projectiles[p].travel_type in weapon.legal_travel_types}, per
# hero-builds.json's weapon_projectile_coupling_note. An empty intersection
# means hero-builds.json's data is wrong (a chassis legalized a weapon with
# no compatible projectile in its own legal_projectiles list) — that's a data
# bug to surface loudly, not a case to silently paper over by falling back to
# the chassis's full projectile list.
# ---------------------------------------------------------------------------

def sample_roster(hero_builds: dict, roster_size: int, rng: random.Random) -> list[dict]:
    chassis_ids = list(hero_builds["chassis"].keys())
    weapons = hero_builds["weapon_archetypes"]
    projectiles = hero_builds["projectiles"]
    roster = []
    for idx in range(roster_size):
        chassis_id = rng.choice(chassis_ids)
        chassis = hero_builds["chassis"][chassis_id]
        weapon_id = rng.choice(chassis["legal_weapons"])
        legal_travel_types = weapons[weapon_id]["legal_travel_types"]
        compatible_projectiles = [
            pid for pid in chassis["legal_projectiles"]
            if projectiles[pid]["travel_type"] in legal_travel_types
        ]
        if not compatible_projectiles:
            raise ValueError(
                f"variety.py: chassis '{chassis_id}' legalizes weapon '{weapon_id}' (legal_travel_types="
                f"{legal_travel_types}) but none of its legal_projectiles {chassis['legal_projectiles']} "
                "have a matching travel_type -- hero-builds.json data is wrong, see "
                "weapon_projectile_coupling_note and hero-builds.schema.md 'weapon_archetypes.<id>.legal_travel_types'."
            )
        roster.append({
            "idx": idx,
            "chassis_id": chassis_id,
            "weapon_id": weapon_id,
            "projectile_id": rng.choice(compatible_projectiles),
            "modification_id": rng.choice(chassis["legal_modifications"] + [None]),
            "ability_id": rng.choice(chassis["legal_abilities"] + [None]),
            "origin_world_id": rng.choice(chassis["legal_origin_worlds"]),
        })
    return roster


# ---------------------------------------------------------------------------
# 2. Synergy rules — evaluated against the ROLLED roster (one entry per
#    build, per hero-builds.schema.md: "by unit count, not soldier
#    headcount"). Dispatches on each rule's own `scope` field rather than
#    inferring scope from condition.type, matching the schema's documented
#    scope enum directly.
# ---------------------------------------------------------------------------

def evaluate_synergy_rules(hero_builds: dict, roster: list[dict]) -> tuple[list[dict], dict[int, dict]]:
    """
    Returns (fired_rules, per_build_mult):
      fired_rules   -- list of {id, description, scope, effect, affected_count}
                       for every rule whose condition evaluated true.
      per_build_mult -- {roster_idx: {"dps": m, "rate_of_fire": m, "accuracy": m}},
                        multiplicative, defaults 1.0, one entry per roster index.
    """
    n = len(roster)
    mult = {i: {"dps": 1.0, "rate_of_fire": 1.0, "accuracy": 1.0} for i in range(n)}
    fired: list[dict] = []
    if n == 0:
        return fired, mult

    origin_counts = Counter(b["origin_world_id"] for b in roster)
    majority_world, majority_count = origin_counts.most_common(1)[0]
    majority_share = majority_count / n

    for rule in hero_builds["synergy_rules"]["rules"]:
        cond = rule["condition"]
        ctype = cond["type"]

        if ctype == "origin_world_share":
            fires = _OPS[cond["op"]](majority_share, cond["threshold"])
        elif ctype == "origin_world_pair_present":
            w1, w2 = cond["worlds"]
            fires = w1 in origin_counts and w2 in origin_counts
        elif ctype == "ability_id_count":
            count = sum(1 for b in roster if b["ability_id"] == cond["ability_id"])
            fires = _OPS[cond["op"]](count, cond["threshold"])
        elif ctype == "distinct_origin_world_count":
            fires = _OPS[cond["op"]](len(origin_counts), cond["threshold"])
        else:
            raise ValueError(
                f"variety.py: unknown synergy_rules condition.type '{ctype}' in rule '{rule['id']}' — "
                "hero-builds.json has drifted ahead of this evaluator; see hero-builds.schema.md "
                "'synergy_rules.rules[]' Condition types table."
            )
        if not fires:
            continue

        scope = rule["scope"]
        if scope == "whole_roster":
            scope_indices = list(range(n))
        elif scope == "units_matching_condition_origin":
            scope_indices = [i for i, b in enumerate(roster) if b["origin_world_id"] == majority_world]
        elif scope == "units_matching_origin_in_pair":
            worlds = cond.get("worlds", [])
            scope_indices = [i for i, b in enumerate(roster) if b["origin_world_id"] in worlds]
        else:
            raise ValueError(
                f"variety.py: unknown synergy_rules scope '{scope}' in rule '{rule['id']}' — "
                "hero-builds.json has drifted ahead of this evaluator; see hero-builds.schema.md."
            )

        stat, value = rule["effect"]["stat"], rule["effect"]["value"]
        for i in scope_indices:
            mult[i][stat] *= value
        fired.append({
            "id": rule["id"], "description": rule["description"], "scope": scope,
            "effect": rule["effect"], "affected_count": len(scope_indices),
        })

    return fired, mult


# ---------------------------------------------------------------------------
# 3. Resolve each rolled build into a fighter + WaveGroup, with synergy
#    multipliers applied BEFORE the final dps/swing_interval fold (see
#    data_loader.resolve_hero_build's own docstring for why rate_of_fire/
#    accuracy must be adjusted pre-fold, while a "dps" effect is applied
#    post-fold as a final multiplicative wrapper — those are the only three
#    stats any of task-079's four rules ever target).
# ---------------------------------------------------------------------------

def build_retinue_groups(hero_builds: dict, roster: list[dict], mult: dict, count_per_build: int) -> list[cm.WaveGroup]:
    groups = []
    for b in roster:
        name = f"build{b['idx']:02d}_{b['chassis_id']}"
        components = dl.resolve_hero_build(
            b["chassis_id"], b["weapon_id"], b["projectile_id"], b["modification_id"], b["ability_id"],
        )
        m = mult[b["idx"]]
        components["rate_of_fire"] *= m["rate_of_fire"]
        components["accuracy"] = max(0.0, min(1.0, components["accuracy"] * m["accuracy"]))

        chassis_name = hero_builds["chassis"][b["chassis_id"]]["display_name"]
        weapon_name = hero_builds["weapon_archetypes"][b["weapon_id"]]["display_name"]
        display_name = f"{chassis_name} ({weapon_name})"

        fighter = dl.finalize_hero_build_fighter(name, display_name, components)
        fighter["dps"] *= m["dps"]
        b["fighter"] = fighter
        groups.append(cm.WaveGroup(name=name, fighter=fighter, count=float(count_per_build)))
    return groups


def build_enemy_groups(scenario: dict) -> list[cm.WaveGroup]:
    """Same construction scenario_runner.run_wave_attrition uses for
    Enemy.Composition — the enemy side is untouched by this variety layer."""
    return [
        cm.WaveGroup(
            name=row.get("Name", row["EntityTier"]), fighter=dl.enemy_fighter(row["EntityTier"]),
            count=float(row["Count"]), arrival_seconds=float(row.get("ArrivalSeconds", 0.0)),
        )
        for row in scenario["Enemy"]["Composition"]
    ]


# ---------------------------------------------------------------------------
# task-086: retinue sub-type identity mode. NOT a roll over a huge
# combinatorial space like the hero-builds mode above — docs/data/scenarios/
# retinue-subtypes.json is a small, FIXED set of 7 candidates, so this fields
# all of them at once (every candidate present, one WaveGroup each) rather
# than randomly sampling a subset. The scenario's 'spearmen' Composition
# row(s) are replaced by that same total headcount split across the 7
# candidates; every other row (e.g. Archers) is carried through UNCHANGED via
# the normal retinue_fighter() path, and the Enemy side is untouched — a
# like-for-like swap of the melee identity axis only, per task-086's scope
# fence. `--seed` is OPTIONAL here (unlike hero-builds mode): omitted, the
# split is EQUAL across all 7 (the primary, deterministic comparison); a seed
# draws a random per-candidate headcount split instead (random.Random(seed)
# only), a robustness check on whether the ranking holds regardless of how
# the fixed total headcount happens to be portioned across candidates.
# ---------------------------------------------------------------------------

def random_split(total: float, n: int, rng: random.Random) -> list[float]:
    """n positive weights drawn from rng, normalized to sum exactly to total."""
    weights = [rng.random() for _ in range(n)]
    wsum = sum(weights)
    return [total * w / wsum for w in weights]


def build_subtype_retinue_groups(scenario: dict, split_counts: list[float] | None) -> tuple[list[cm.WaveGroup], list[str]]:
    subtypes = dl.load_retinue_subtypes()["candidates"]
    candidate_ids = list(subtypes.keys())
    spearmen_total = sum(row["Count"] for row in scenario["Retinue"]["Composition"] if row["UnitType"] == "spearmen")
    other_rows = [row for row in scenario["Retinue"]["Composition"] if row["UnitType"] != "spearmen"]

    counts = split_counts if split_counts is not None else [spearmen_total / len(candidate_ids)] * len(candidate_ids)

    groups = [
        cm.WaveGroup(name=cid, fighter=dl.retinue_subtype_fighter(cid), count=c)
        for cid, c in zip(candidate_ids, counts)
    ]
    groups += [
        cm.WaveGroup(name=f"{row['UnitType']}_{row['Tier']}", fighter=dl.retinue_fighter(row["UnitType"], row["Tier"]),
                     count=float(row["Count"]))
        for row in other_rows
    ]
    return groups, candidate_ids


def run_subtypes(scenario_name: str, seed: int | None) -> dict:
    scenario = dl.load_scenario(scenario_name)
    if scenario["Kind"] != "wave_attrition":
        raise ValueError(
            f"variety.py --mode subtypes only runs Kind='wave_attrition' scenarios "
            f"(got Kind='{scenario['Kind']}' for '{scenario_name}')."
        )

    candidate_ids = list(dl.load_retinue_subtypes()["candidates"].keys())
    spearmen_total = sum(row["Count"] for row in scenario["Retinue"]["Composition"] if row["UnitType"] == "spearmen")
    split_counts = None
    if seed is not None:
        rng = random.Random(seed)
        split_counts = random_split(float(spearmen_total), len(candidate_ids), rng)

    retinue_groups, candidate_ids = build_subtype_retinue_groups(scenario, split_counts)
    enemy_groups = build_enemy_groups(scenario)

    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    result = cm.simulate_wave_attrition(
        retinue_groups=retinue_groups,
        enemy_groups=enemy_groups,
        chip_floor=dl.armor_chip_floor(),
        max_attackers_per_unit=float(consts["MaxAttackersPerUnit"]),
        formation_spacing=float(consts["FormationSpacingUU"]),
        engaged_spacing=float(consts["EngagedSpacingUU"]),
        melee_contact_facing_fraction=float(consts["MeleeContactFacingFraction"]),
        dt=dl.swing_interval_shared(),
        max_time=float(scenario.get("TimeLimitSeconds", 300)),
    )

    enemy_total_count = sum(g.count for g in enemy_groups)
    enemy_avg_max_hp = (
        sum(g.count * g.fighter["max_hp"] for g in enemy_groups) / enemy_total_count
        if enemy_total_count > 0 else 0.0
    )
    dmg_by_group = result.get("group_damage_dealt", {})
    total_dmg_dealt_by_retinue = result.get("total_damage_dealt", {}).get("enemy", 0.0)

    candidate_groups = {wg.name: wg for wg in retinue_groups if wg.name in candidate_ids}
    # share_of_candidate_damage's denominator is the sum of the 7 candidates'
    # OWN damage only, not total_dmg_dealt_by_retinue — floor1-swarm-wave also
    # carries an unchanged Archers row through retinue_groups, which deals
    # damage of its own and is correctly excluded from `ranked` (only the
    # melee identity axis is under test), so it must also be excluded from
    # this share's denominator or the 7 shares would never sum to 1.0.
    total_dmg_dealt_by_candidates = sum(dmg_by_group.get(cid, 0.0) for cid in candidate_ids)
    ranked = []
    for cid in candidate_ids:
        wg = candidate_groups[cid]
        fighter = wg.fighter
        dmg = dmg_by_group.get(cid, 0.0)
        est_kills = dmg / enemy_avg_max_hp if enemy_avg_max_hp > 0 else 0.0
        # combat_model.melee_reach_per_exposed_unit(): a candidate's raw
        # targets_per_hit is NOT its effective cleave — see
        # docs/data/scenarios/retinue-subtypes.json's own
        # engage_range_targets_per_hit_interaction_note.
        reach = cm.melee_reach_per_exposed_unit(
            fighter["engage_range"], float(consts["EngagedSpacingUU"]), float(consts["MeleeContactFacingFraction"]),
        )
        ranked.append({
            "candidate": cid,
            "max_hp": fighter["max_hp"], "dps": fighter["dps"], "engage_range": fighter["engage_range"],
            "targets_per_hit": fighter["targets_per_hit"],
            "effective_cleave": round(min(fighter["targets_per_hit"], reach), 2),
            "count_start": round(wg.count, 2), "count_survivors": round(wg.alive_count, 2),
            "survival_rate": round(wg.alive_count / wg.count, 4) if wg.count > 0 else 0.0,
            "damage_dealt": round(dmg, 1),
            "damage_per_unit_committed": round(dmg / wg.count, 2) if wg.count > 0 else 0.0,
            "share_of_candidate_damage": (
                round(dmg / total_dmg_dealt_by_candidates, 4) if total_dmg_dealt_by_candidates > 0 else 0.0
            ),
            "estimated_kills": round(est_kills, 2),
        })
    # rank by per-capita output (damage per unit committed), NOT raw damage_dealt
    # — split_counts can be unequal across candidates (random_split), so raw
    # damage_dealt alone would just reward whichever candidate got more bodies.
    ranked.sort(key=lambda r: r["damage_per_unit_committed"], reverse=True)

    if total_dmg_dealt_by_candidates > 0:
        share_sum = sum(r["share_of_candidate_damage"] for r in ranked)
        assert abs(share_sum - 1.0) < 0.01, (
            f"variety.py --mode subtypes: share_of_candidate_damage should sum to ~1.0, got {share_sum:.4f}"
        )

    return {
        "scenario": scenario_name,
        "scenario_display_name": scenario.get("DisplayName", scenario_name),
        "seed": seed,
        "split": "equal" if seed is None else f"random_split(seed={seed})",
        "spearmen_headcount_replaced": spearmen_total,
        "enemy_avg_max_hp": round(enemy_avg_max_hp, 2),
        "total_damage_dealt_by_retinue": round(total_dmg_dealt_by_retinue, 2),
        "total_damage_dealt_by_candidates": round(total_dmg_dealt_by_candidates, 2),
        "sim_result": result,
        "ranked_candidates": ranked,
    }


def print_subtype_report(data: dict) -> None:
    result = data["sim_result"]
    print(f"\n=== Retinue sub-type identity roll (task-086): {data['scenario_display_name']} ({data['scenario']}) ===")
    print(f"  split={data['split']}  spearmen headcount replaced={data['spearmen_headcount_replaced']:.0f} "
          f"(spread across {len(data['ranked_candidates'])} candidates from docs/data/scenarios/retinue-subtypes.json)")
    print()
    print("  --- WAVE-ATTRITION result. docs/sim/LIMITATIONS.md section 1: this model does NOT reproduce")
    print("  --- GATE1's measured survival at committed defaults. Ranks candidates RELATIVE TO EACH OTHER")
    print("  --- inside one shared, imperfect model — never an absolute claim about the real game. ---")
    print(f"  Result: {result['result']}  (elapsed {result['elapsed_seconds']}s)")
    print(f"  Retinue: {result['retinue_start']:.0f} start -> {result['retinue_survivors']:.1f} survivors  "
          f"(includes archers/other rows carried through unchanged)")
    print(f"  Enemy ({data['scenario']}'s composition, unchanged): {result['enemy_start']:.0f} start -> "
          f"{result['enemy_survivors']:.1f} survivors")
    print()
    header = (f"  {'#':>2} {'candidate':<26} {'hp':>6} {'dps':>6} {'engage':>7} {'tgt/hit':>7} "
              f"{'eff.cleave':>10} {'count':>7} {'surv%':>6} {'dmg/unit':>9} {'shr-of-7':>8} {'est kills':>9}")
    print(header)
    for rank, row in enumerate(data["ranked_candidates"], start=1):
        print(f"  {rank:>2} {row['candidate']:<26} {row['max_hp']:>6.1f} {row['dps']:>6.1f} "
              f"{row['engage_range']:>7.1f} {row['targets_per_hit']:>7} {row['effective_cleave']:>10.2f} "
              f"{row['count_start']:>7.2f} {row['survival_rate']*100:>5.1f}% {row['damage_per_unit_committed']:>9.2f} "
              f"{row['share_of_candidate_damage']*100:>7.1f}% {row['estimated_kills']:>9.2f}")
    print("  shr-of-7 = this candidate's share of the 7 candidates' own combined damage output "
          "(excludes any unchanged Archers row).")
    print()


# ---------------------------------------------------------------------------
# 4. Run
# ---------------------------------------------------------------------------

def run(scenario_name: str, seed: int, roster_size: int, count_per_build: int) -> dict:
    scenario = dl.load_scenario(scenario_name)
    if scenario["Kind"] != "wave_attrition":
        raise ValueError(
            f"variety.py only runs Kind='wave_attrition' scenarios for the enemy side "
            f"(got Kind='{scenario['Kind']}' for '{scenario_name}')."
        )

    hero_builds = dl.load_hero_builds()
    rng = random.Random(seed)
    roster = sample_roster(hero_builds, roster_size, rng)
    fired_rules, mult = evaluate_synergy_rules(hero_builds, roster)

    retinue_groups = build_retinue_groups(hero_builds, roster, mult, count_per_build)
    enemy_groups = build_enemy_groups(scenario)

    consts = dl.load_combat_model_constants()["wave_attrition_model"]
    result = cm.simulate_wave_attrition(
        retinue_groups=retinue_groups,
        enemy_groups=enemy_groups,
        chip_floor=dl.armor_chip_floor(),
        max_attackers_per_unit=float(consts["MaxAttackersPerUnit"]),
        formation_spacing=float(consts["FormationSpacingUU"]),
        engaged_spacing=float(consts["EngagedSpacingUU"]),
        melee_contact_facing_fraction=float(consts["MeleeContactFacingFraction"]),
        dt=dl.swing_interval_shared(),
        max_time=float(scenario.get("TimeLimitSeconds", 300)),
    )

    # abilities.guardian_angel.representable_note (hero-builds.json):
    # "representable ONLY as a flat +1 adjustment to the wave-attrition
    # model's final survivor count per instance present in the roster" — a
    # post-hoc headcount correction, not a stat-block field, applied here
    # (not in combat_model.py) exactly as the note prescribes.
    guardian_angel_count = sum(1 for b in roster if b["ability_id"] == "guardian_angel")

    enemy_total_count = sum(g.count for g in enemy_groups)
    enemy_avg_max_hp = (
        sum(g.count * g.fighter["max_hp"] for g in enemy_groups) / enemy_total_count
        if enemy_total_count > 0 else 0.0
    )

    dmg_by_group = result.get("group_damage_dealt", {})
    # "retinue" in total_damage_dealt means damage DEALT TO the retinue (dmg_to_retinue,
    # incoming) -- the denominator for "this build's share of what the retinue put out"
    # is "enemy" (dmg_to_enemy, outgoing), matching the retinue -> enemy print above.
    total_dmg_dealt_by_retinue = result.get("total_damage_dealt", {}).get("enemy", 0.0)
    ranked = []
    for b, wg in zip(roster, retinue_groups):
        name = wg.name
        dmg = dmg_by_group.get(name, 0.0)
        est_kills = dmg / enemy_avg_max_hp if enemy_avg_max_hp > 0 else 0.0
        ranked.append({
            "idx": b["idx"], "name": name,
            "chassis_id": b["chassis_id"], "weapon_id": b["weapon_id"], "projectile_id": b["projectile_id"],
            "modification_id": b["modification_id"], "ability_id": b["ability_id"],
            "origin_world_id": b["origin_world_id"],
            "count_start": count_per_build, "count_survivors": round(wg.alive_count, 2),
            "survived": wg.alive_count > 0.5,
            "per_unit_dps": round(b["fighter"]["dps"], 2),
            "damage_dealt": round(dmg, 1),
            "share_of_retinue_damage": (
                round(dmg / total_dmg_dealt_by_retinue, 4) if total_dmg_dealt_by_retinue > 0 else 0.0
            ),
            "estimated_kills": round(est_kills, 2),
        })
    ranked.sort(key=lambda r: r["damage_dealt"], reverse=True)

    # one runnable check: shares should sum to ~1.0 across the whole roster
    # (every build's damage_dealt is a slice of the same total_dmg_dealt_by_retinue
    # denominator) -- catches exactly the wrong-key class of bug this replaced.
    if total_dmg_dealt_by_retinue > 0:
        share_sum = sum(r["share_of_retinue_damage"] for r in ranked)
        assert abs(share_sum - 1.0) < 0.01, (
            f"variety.py: share_of_retinue_damage should sum to ~1.0 across the roster, got {share_sum:.4f} "
            f"(total_dmg_dealt_by_retinue={total_dmg_dealt_by_retinue}, "
            f"sum(group_damage_dealt)={sum(dmg_by_group.values()):.2f}) -- check total_damage_dealt['enemy'] "
            "vs group_damage_dealt for a drift between the two combat_model.py accumulators."
        )

    return {
        "scenario": scenario_name,
        "scenario_display_name": scenario.get("DisplayName", scenario_name),
        "seed": seed,
        "roster_size": roster_size,
        "count_per_build": count_per_build,
        "fired_synergy_rules": fired_rules,
        "guardian_angel_instances": guardian_angel_count,
        "retinue_survivors_with_guardian_angel": round(result["retinue_survivors"] + guardian_angel_count, 2),
        "enemy_avg_max_hp": round(enemy_avg_max_hp, 2),
        "sim_result": result,
        "ranked_builds": ranked,
    }


# ---------------------------------------------------------------------------
# 5. Reporting — plain ASCII only (this repo has already shipped a mojibake
#    em-dash from console output on this machine; every string below sticks
#    to "->" and plain hyphens).
# ---------------------------------------------------------------------------

def _axis_display(hero_builds: dict, table: str, key: str | None) -> str:
    if key is None:
        return "none"
    return hero_builds[table][key]["display_name"]


def print_report(data: dict, hero_builds: dict) -> None:
    result = data["sim_result"]
    print(f"\n=== Hero-build variety roll: {data['scenario_display_name']} ({data['scenario']}) ===")
    print(f"  seed={data['seed']}  builds sampled={data['roster_size']}  count per build={data['count_per_build']}")
    print()
    print("  --- metrics ---")
    print(f"  Result: {result['result']}  (elapsed {result['elapsed_seconds']}s)")
    print(f"  Retinue (rolled hero-builds): {result['retinue_start']:.0f} start -> "
          f"{result['retinue_survivors']:.1f} survivors")
    if data["guardian_angel_instances"] > 0:
        print(f"    + guardian_angel post-hoc adjustment ({data['guardian_angel_instances']} instance(s) in "
              f"roster, +1 survivor each, hero-builds.json abilities.guardian_angel.representable_note): "
              f"{data['retinue_survivors_with_guardian_angel']:.2f} adjusted survivors")
    print(f"  Enemy ({data['scenario']}'s composition, unchanged): {result['enemy_start']:.0f} start -> "
          f"{result['enemy_survivors']:.1f} survivors")
    print(f"  Total damage dealt this fight: retinue -> enemy = {result['total_damage_dealt']['enemy']:.1f}, "
          f"enemy -> retinue = {result['total_damage_dealt']['retinue']:.1f}")
    print(f"  enemy_avg_max_hp used for the estimated-kills column: {data['enemy_avg_max_hp']:.1f} "
          f"(count-weighted average across {data['scenario']}'s Enemy.Composition)")

    print()
    if data["fired_synergy_rules"]:
        print("  Synergies fired:")
        for r in data["fired_synergy_rules"]:
            print(f"    - {r['id']}: {r['effect']['stat']} x{r['effect']['value']} on {r['affected_count']} "
                  f"build(s) [{r['scope']}]  -- {r['description']}")
    else:
        print("  Synergies fired: none")

    print()
    print("  --- top 10 by damage dealt (WAVE-ATTRITION, RELATIVE ranking only -- see docs/sim/LIMITATIONS.md ---")
    print("  ---  section 1: this model does not reproduce GATE1's measured survival at these defaults) ---")
    header = (f"  {'#':>2} {'chassis':<16} {'weapon':<22} {'projectile':<16} {'mod':<18} {'ability':<18} "
              f"{'origin':<14} {'dps':>7} {'dmg dealt':>10} {'share':>7} {'est kills':>9} {'status':>7}")
    print(header)
    for rank, row in enumerate(data["ranked_builds"][:10], start=1):
        chassis = _axis_display(hero_builds, "chassis", row["chassis_id"])
        weapon = _axis_display(hero_builds, "weapon_archetypes", row["weapon_id"])
        projectile = _axis_display(hero_builds, "projectiles", row["projectile_id"])
        mod = _axis_display(hero_builds, "modifications", row["modification_id"])
        ability = _axis_display(hero_builds, "abilities", row["ability_id"])
        origin = _axis_display(hero_builds, "origin_worlds", row["origin_world_id"])
        status = "OK" if row["survived"] else "WIPED"
        print(f"  {rank:>2} {chassis:<16} {weapon:<22} {projectile:<16} {mod:<18} {ability:<18} "
              f"{origin:<14} {row['per_unit_dps']:>7.2f} {row['damage_dealt']:>10.1f} "
              f"{row['share_of_retinue_damage']*100:>6.1f}% {row['estimated_kills']:>9.2f} {status:>7}")
    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Roll a random legal hero-build roster (docs/data/hero-builds.json), apply task-079's "
                    "synergy rules, run it as the retinue side of a named wave_attrition scenario's enemy "
                    "composition, and report a metrics block plus an ASCII top-10 table ranked by damage dealt. "
                    "--mode subtypes (task-086) instead compares docs/data/scenarios/retinue-subtypes.json's "
                    "candidate melee identities against each other."
    )
    parser.add_argument("--mode", choices=["hero-builds", "subtypes"], default="hero-builds",
                        help="hero-builds (default, task-080): roll a random legal hero-build roster. "
                             "subtypes (task-086): field all retinue-subtypes.json candidates at once.")
    parser.add_argument("--scenario", required=True, help="wave_attrition scenario name (without .json) "
                                                            "supplying the ENEMY side")
    parser.add_argument("--seed", type=int, default=None,
                        help="random.Random seed. REQUIRED for --mode hero-builds (the roster roll). OPTIONAL for "
                             "--mode subtypes: omitted = equal headcount split across all candidates (the primary, "
                             "deterministic comparison); given = a random per-candidate split, a robustness check.")
    parser.add_argument("--roster", type=int, default=20,
                        help="[hero-builds mode only] number of distinct hero-builds to roll (default 20)")
    parser.add_argument("--count-per-build", type=int, default=2,
                        help="[hero-builds mode only] soldiers per rolled build's WaveGroup (default 2 -- with the "
                             "default --roster 20 this totals 40, matching floor1-swarm-wave's real retinue size; "
                             "an arbitrary but documented default, see docs/sim/VARIETY.md)")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON instead of the human report")
    args = parser.parse_args()

    if args.mode == "subtypes":
        data = run_subtypes(args.scenario, args.seed)
        if args.json:
            print(json.dumps(sr.to_json_safe(data), indent=2))
            return
        print_subtype_report(data)
        return

    if args.seed is None:
        parser.error("--seed is required for --mode hero-builds")

    hero_builds = dl.load_hero_builds()
    data = run(args.scenario, args.seed, args.roster, args.count_per_build)

    if args.json:
        print(json.dumps(sr.to_json_safe(data), indent=2))
        return

    print_report(data, hero_builds)


if __name__ == "__main__":
    main()
