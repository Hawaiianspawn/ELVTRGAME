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
