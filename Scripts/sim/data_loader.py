"""
data_loader.py — the ONLY place this harness touches docs/data/*.json.

Everything downstream (combat_model.py, scenario_runner.py, validate.py) asks
this module for stat blocks by name; nothing else in Scripts/sim/ is allowed
to hardcode a MaxHP, DPS, Armor, or tier number that already lives in a
committed data file. If a new stat is needed, it goes in a data file (or
docs/data/scenarios/combat-model-constants.json for the handful of dials that
no other file owns — see that file's own $schema_note), not in Python.

Plain stdlib only (json + pathlib). No new dependencies.
"""

from __future__ import annotations

import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DATA_DIR = REPO_ROOT / "docs" / "data"
SCENARIOS_DIR = DATA_DIR / "scenarios"


def _load_json(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(
            f"data_loader: expected data file missing: {path}\n"
            "This harness reads its inputs from docs/data/*.json — it does not "
            "fall back to hardcoded numbers."
        )
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Enemy tiers — docs/data/entity-tiers.json
# ---------------------------------------------------------------------------

def load_entity_tiers() -> dict:
    """Returns {tier_name: row_dict, ..., '_design_constants': {...}}."""
    raw = _load_json(DATA_DIR / "entity-tiers.json")
    tiers = {row["Name"]: row for row in raw["tiers"]}
    tiers["_design_constants"] = raw["design_constants"]
    return tiers


def enemy_fighter(tier_name: str) -> dict:
    """Normalize an entity-tiers.json row into the flat fighter shape combat_model.py expects."""
    tiers = load_entity_tiers()
    if tier_name not in tiers:
        raise KeyError(f"Unknown entity tier '{tier_name}'. Known: {sorted(k for k in tiers if not k.startswith('_'))}")
    row = tiers[tier_name]
    role = "ranged" if row["EngageRange"] > 150 and row["MinEngageRange"] > 0 else "melee"
    return {
        "name": row["Name"],
        "display_name": row["DisplayName"],
        "max_hp": float(row["MaxHP"]),
        "armor": float(row["Armor"]),
        "dps": float(row["DPS"]),
        "swing_interval": float(row["SwingInterval"]),
        "engage_range": float(row["EngageRange"]),
        "min_engage_range": float(row["MinEngageRange"]),
        "targets_per_hit": int(row["TargetsPerHit"]),
        "role": role,
        "surround_cap_estimate": row.get("SurroundCapEstimate"),
        "source": "docs/data/entity-tiers.json",
    }


def armor_chip_floor() -> float:
    return float(load_entity_tiers()["_design_constants"]["armor_chip_floor"])


def swing_interval_shared() -> float:
    return float(load_entity_tiers()["_design_constants"]["swing_interval_shared"])


# ---------------------------------------------------------------------------
# Retinue tier ladder — docs/data/upgrades.json (Freed/Militia/Veteran/Bannerman)
# ---------------------------------------------------------------------------

def load_retinue_tiers() -> dict:
    raw = _load_json(DATA_DIR / "upgrades.json")
    return {t["id"]: t for t in raw["tier_ladder"]["tiers"]}


# ---------------------------------------------------------------------------
# Typed retinue units — docs/data/unit-types.json (spearmen / archers)
# ---------------------------------------------------------------------------

def load_unit_types() -> dict:
    raw = _load_json(DATA_DIR / "unit-types.json")
    return raw["types"], raw.get("allocation", {}), raw.get("ranged_combat_model", {})


def retinue_fighter(unit_type: str, tier: str) -> dict:
    """
    Combine unit-types.json (role/engage-range/targets-per-hit/formation shape)
    with upgrades.json's tier ladder (HP/DPS by tier) to get one fighter block.

    Spearmen inherit the tier ladder directly (unit-types.json's own note:
    "Mechanically today's retinue... Stats are the shipped Gate 1 defaults").

    Archers do NOT have a per-tier HP/DPS row anywhere in docs/data — unit-types.json
    states Archer combat stats once, flat, not per-tier. This harness reproduces
    the exact assumption docs/design/entity-tiers.md §7 already stated plainly and
    left unresolved: an Archer at a given tier scales HP/DPS by the same ratio as
    the Spearmen tier it's nominally paired with, relative to Militia (the anchor
    tier archers.json's flat numbers were tuned against). This is NOT a new
    invention — it is the same stated, flagged assumption, applied consistently.
    """
    types, _, _ = load_unit_types()
    if unit_type not in types:
        raise KeyError(f"Unknown unit type '{unit_type}'. Known: {sorted(types)}")
    row = types[unit_type]
    combat = row["combat"]
    tiers = load_retinue_tiers()
    if tier not in tiers:
        raise KeyError(f"Unknown retinue tier '{tier}'. Known: {sorted(tiers)}")
    tier_row = tiers[tier]

    if unit_type == "spearmen":
        max_hp = float(tier_row["hp"])
        dps = float(tier_row["dps"])
        scaling_note = "direct tier-ladder read (upgrades.json)"
    elif unit_type == "archers":
        anchor = tiers["militia"]
        hp_ratio = tier_row["hp"] / anchor["hp"]
        dps_ratio = tier_row["dps"] / anchor["dps"]
        max_hp = float(combat["max_hp"]) * hp_ratio
        dps = float(combat["dps"]) * dps_ratio
        scaling_note = (
            "ASSUMED — Archer flat stats (unit-types.json) scaled by the Spearmen "
            f"tier ratio vs Militia (entity-tiers.md §7 stated assumption, unresolved). "
            f"hp_ratio={hp_ratio:.4f} dps_ratio={dps_ratio:.4f}"
        )
    else:
        max_hp = float(combat["max_hp"])
        dps = float(combat["dps"])
        scaling_note = "no tier ladder for this type; used unit-types.json flat stats as-is"

    role = "ranged" if combat["engage_range"] > 150 and combat["min_engage_range"] > 0 else "melee"
    return {
        "name": f"{unit_type}_{tier}",
        "display_name": f"{row['display_name']} ({tier})",
        "unit_type": unit_type,
        "tier": tier,
        "max_hp": max_hp,
        "armor": 0.0,  # no Armor column exists for friendly tiers in upgrades.json
        "dps": dps,
        "swing_interval": swing_interval_shared(),
        "engage_range": float(combat["engage_range"]),
        "min_engage_range": float(combat["min_engage_range"]),
        "targets_per_hit": int(combat["targets_per_hit"]),
        "role": role,
        "growth_source_weight": float(row.get("growth_source_weight", 0.0)),
        "source": "docs/data/unit-types.json + docs/data/upgrades.json",
        "scaling_note": scaling_note,
    }


# ---------------------------------------------------------------------------
# Retinue sub-type CANDIDATES (task-086) — docs/data/scenarios/retinue-subtypes.json.
# These are sim-director EXPERIMENT INPUTS derived from task-082's kept knight
# silhouettes, never gameplay-director canon. Deliberately NOT folded into
# retinue_fighter()/unit-types.json's tier-ladder path (data_loader.py:140's
# "no tier ladder for this type" fallback needs the type to live in
# unit-types.json, which is off limits to this task) — a small parallel
# reader instead, same flat-fighter shape, sourced from the candidates file.
# ---------------------------------------------------------------------------

def load_retinue_subtypes() -> dict:
    return _load_json(SCENARIOS_DIR / "retinue-subtypes.json")


def retinue_subtype_fighter(candidate_id: str) -> dict:
    """One docs/data/scenarios/retinue-subtypes.json candidate -> the same
    flat fighter shape retinue_fighter()/enemy_fighter() produce. Role is
    hardcoded 'melee' (every candidate's engage_range is well under the
    150uu melee/ranged threshold retinue_fighter()/finalize_hero_build_fighter()
    both already use, so this isn't a special case, just a known constant).
    Armor 0.0, matching retinue_fighter()'s own note: no Armor column exists
    for friendly units anywhere in docs/data."""
    candidates = load_retinue_subtypes()["candidates"]
    if candidate_id not in candidates:
        raise KeyError(f"Unknown retinue subtype candidate '{candidate_id}'. Known: {sorted(candidates)}")
    combat = candidates[candidate_id]["combat"]
    return {
        "name": candidate_id,
        "display_name": candidates[candidate_id].get("display_name", candidate_id),
        "max_hp": float(combat["max_hp"]),
        "armor": 0.0,
        "dps": float(combat["dps"]),
        "swing_interval": swing_interval_shared(),
        "engage_range": float(combat["engage_range"]),
        "min_engage_range": float(combat["min_engage_range"]),
        "targets_per_hit": int(combat["targets_per_hit"]),
        "role": "melee",
        "source": "docs/data/scenarios/retinue-subtypes.json (task-086 CANDIDATE, not unit-types.json canon)",
    }


# ---------------------------------------------------------------------------
# Combat-model constants this harness owns (docs/data/scenarios/**) — the
# handful of dials (Hero stats, frontage-model spacing) that no gameplay-owned
# data file has a row for. See that file's own $schema_note for why.
# ---------------------------------------------------------------------------

def load_combat_model_constants() -> dict:
    return _load_json(SCENARIOS_DIR / "combat-model-constants.json")


def hero_fighter() -> dict:
    c = load_combat_model_constants()["hero"]
    return {
        "name": "hero",
        "display_name": "Hero (Vanguard)",
        "max_hp": float(c["MaxHP"]),
        "armor": 0.0,
        "dps": float(c["DPS"]),
        "swing_interval": float(c["SwingInterval"]),
        "engage_range": 150.0,
        "min_engage_range": 0.0,
        "targets_per_hit": 1,
        "role": "melee",
        "source": "docs/data/scenarios/combat-model-constants.json (cites GATE1-FUN-PROTOTYPE.md §3)",
    }


# ---------------------------------------------------------------------------
# Hero-build variety layer — docs/data/hero-builds.json (task-079). The only
# consumer targeted today is Scripts/sim/variety.py (task-080), which still
# must not open this file itself — same exclusivity invariant as every other
# docs/data/*.json this module owns.
# ---------------------------------------------------------------------------

def load_hero_builds() -> dict:
    return _load_json(DATA_DIR / "hero-builds.json")


def resolve_hero_build(
    chassis_id: str,
    weapon_id: str,
    projectile_id: str,
    modification_id: str | None = None,
    ability_id: str | None = None,
) -> dict:
    """
    Resolve one hero-builds.json axis-pick tuple (chassis/weapon/projectile,
    plus an optional modification/ability) into an INTERMEDIATE component
    dict, per hero-builds.schema.md's `build_stat_block` derivation —
    stopping one step short of the final dps/swing_interval fold so a caller
    (Scripts/sim/variety.py) can apply a synergy_rules multiplier to
    rate_of_fire/accuracy FIRST: the schema states rate_of_fire is read
    "pre-swing-interval-inversion" and accuracy "pre-dps-computation", i.e.
    both must be adjusted before finalize_hero_build_fighter() folds them
    into one steady-state dps number. `ability_id` isn't a stat input at all
    (none of the six abilities write a build_stat_block field — see
    hero-builds.json abilities.*.representable_note) — it's threaded through
    only so a caller can still see which build picked it, for synergy-rule
    evaluation and reporting. origin_world is likewise not a stat input
    (schema: "NOT constrained by weapon/projectile choice") and isn't
    accepted here at all; callers track it themselves alongside the roll.

    `piercing_rounds`' `armor_penetration_flat` is carried through in the
    returned dict but not consumed by anything downstream — see
    docs/sim/VARIETY.md for why (Scripts/sim/combat_model.py's EffectiveBlow
    has no term for it, and this task's combat_model.py change is scoped to
    an additive damage-attribution key only, not a math change).
    """
    hb = load_hero_builds()
    chassis = hb["chassis"][chassis_id]
    weapon = hb["weapon_archetypes"][weapon_id]
    projectile = hb["projectiles"][projectile_id]
    mod = hb["modifications"][modification_id] if modification_id else None
    effect = mod["effect"] if mod else {}

    base = chassis["base_stats"]
    # hero-builds.schema.md 'resolve_type -> targets_per_hit derivation':
    targets_per_shot_effective = weapon["targets_per_shot"] + effect.get("targets_per_shot_add", 0)
    if projectile["resolve_type"] == "area" and weapon["aoe_radius"] > 0:
        targets_per_hit = max(targets_per_shot_effective, round(weapon["aoe_radius"] / 40))
    else:
        targets_per_hit = targets_per_shot_effective

    return {
        "chassis_id": chassis_id,
        "weapon_id": weapon_id,
        "projectile_id": projectile_id,
        "modification_id": modification_id,
        "ability_id": ability_id,
        "max_hp": float(base["max_hp"]) * effect.get("max_hp_mult", 1.0),
        "armor": float(base["armor"]),
        "rate_of_fire": float(weapon["rate_of_fire"]) * effect.get("rate_of_fire_mult", 1.0),
        "damage_per_shot": float(weapon["damage_per_shot"]) * effect.get("damage_per_shot_mult", 1.0),
        "accuracy": max(0.0, min(1.0, float(weapon["accuracy"]) + float(projectile["accuracy_modifier"]))),
        "engage_range": float(weapon["range"]) * effect.get("range_mult", 1.0),
        "min_engage_range": float(weapon["min_range"]),
        "targets_per_hit": int(targets_per_hit),
        "move_speed_scale": float(base["move_speed_scale"]) * effect.get("move_speed_scale_mult", 1.0),
        "armor_penetration_flat": float(effect.get("armor_penetration_flat", 0.0)),
        "source": "docs/data/hero-builds.json",
    }


def finalize_hero_build_fighter(name: str, display_name: str, components: dict) -> dict:
    """
    Folds a (possibly synergy-modified) component dict from resolve_hero_build
    into the flat fighter shape combat_model.py expects — the same shape
    retinue_fighter()/enemy_fighter() already produce, so a rolled hero-build
    drops into the existing sim harness with no translation layer.

    `role` deliberately does NOT reuse retinue_fighter()/enemy_fighter()'s own
    rule (`engage_range > 150 AND min_engage_range > 0`) — that rule happens
    to work for the two existing hand-written types (Spearmen/Archers) only
    because their data correlates min_engage_range with range. Several
    reachable hero-builds weapon archetypes break that correlation (e.g.
    beam_continuous 600uu range / 0 min-range, turret_autocannon 500uu / 0,
    rapid_skirmish_blaster 350uu / 0) — under the old rule those would
    misclassify as "melee" despite being clearly stand-off weapons. Every
    reachable archetype's `range` cleanly separates on 150uu alone (melee:
    90-95uu, ranged: 350uu+), so `engage_range > 150` on its own is the
    correct discriminator for this axis space. See docs/sim/VARIETY.md.
    """
    rate_of_fire = components["rate_of_fire"]
    dps = components["damage_per_shot"] * rate_of_fire * components["accuracy"]
    role = "ranged" if components["engage_range"] > 150 else "melee"
    return {
        "name": name,
        "display_name": display_name,
        "max_hp": components["max_hp"],
        "armor": components["armor"],
        "dps": dps,
        "swing_interval": (1.0 / rate_of_fire) if rate_of_fire > 0 else float("inf"),
        "engage_range": components["engage_range"],
        "min_engage_range": components["min_engage_range"],
        "targets_per_hit": int(components["targets_per_hit"]),
        "role": role,
        # piercing_rounds' penetration term. combat_model.steady_state_dps()
        # reads this off the attacker dict, so it now actually applies — see
        # combat_model.effective_blow()'s docstring for the formula.
        "armor_penetration_flat": float(components.get("armor_penetration_flat", 0.0)),
        "source": "docs/data/hero-builds.json",
    }


# ---------------------------------------------------------------------------
# Scenario files — docs/data/scenarios/*.json (excluding this constants file)
# ---------------------------------------------------------------------------

def load_scenario(name: str) -> dict:
    path = SCENARIOS_DIR / f"{name}.json"
    if not path.exists():
        raise FileNotFoundError(f"No scenario file at {path}")
    return _load_json(path)


def list_scenarios() -> list[str]:
    return sorted(
        p.stem for p in SCENARIOS_DIR.glob("*.json")
        if p.stem != "combat-model-constants"
    )
