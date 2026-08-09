# check_adaptation_cvars.py
#
# Adaptation's engine side is a HAND TRANSCRIPTION of two JSON files, because nothing in the
# runtime loads docs/data (the sim owns those files) and the tables therefore ship as CVar
# defaults compiled into C++:
#
#     1. Kindled.Adaptation.Ladders  (SwarmCommands.cpp)         <- docs/data/unit-types.json
#                                                                   adaptation.ladders[]
#     2. Swarm.TierHP / Swarm.TierDPS (SwarmCombatProcessors.cpp) <- docs/data/upgrades.json
#                                                                   tier_ladder.tiers[]
#
# A drift in either is SILENT: the game keeps running, units keep adapting, and the only
# symptom is that a rung fights or looks like the wrong row of a spec nobody re-reads.
# Scripts/sim/drift_check.py guards those JSON numbers against being moved; this guards the
# C++ against disagreeing with them, which is the half that lives outside Python's reach.
#
# Also checks the two rules the spec states and the runtime cannot enforce for itself:
# rung order IS tier order (adaptation.md sec.3), and every rung index lands in the atlas
# block its ladder's unit_type resolves to (SwarmSheet::Team, spearmen 0-10 / archers 11-23) —
# a mismatch would make the render bridge draw eleven rows early.
#
#     py Scripts/check_adaptation_cvars.py

import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = REPO / "ELVTR/Source/ELVTR/Mass/SwarmCommands.cpp"
COMBAT = REPO / "ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp"
FRAGMENTS = REPO / "ELVTR/Source/ELVTR/Mass/SwarmFragments.h"
UNIT_TYPES = REPO / "docs/data/unit-types.json"
UPGRADES = REPO / "docs/data/upgrades.json"


def cvar_default(source, name):
    """The default string literal of TAutoConsoleVariable<FString> CVarX(TEXT("name"), TEXT("...")).

    The help text that follows is a run of concatenated TEXT() literals, so only the FIRST
    literal after the name is the default — which is exactly what this returns."""
    text = source.read_text(encoding="utf-8")
    match = re.search(r'TEXT\("%s"\)\s*,\s*TEXT\("([^"]*)"\)' % re.escape(name), text)
    assert match, "%s: no CVar default found for %s" % (source.name, name)
    return match.group(1)


def constant(name):
    """A SwarmSheet::Team constexpr, resolved through one level of aliasing (ArcherVariantBase
    is defined as SpearVariants, and Variants as a sum, so both are read as expressions)."""
    text = FRAGMENTS.read_text(encoding="utf-8")
    text = text[text.index("namespace Team"):]
    match = re.search(r"constexpr int32 %s = ([^;]+);" % name, text)
    assert match, "SwarmFragments.h: no SwarmSheet::Team::%s" % name
    return match.group(1).split("//")[0].strip()


def floats(csv):
    return [float(part) for part in csv.split(",")]


def check():
    failures = []

    # --- the atlas blocks the C++ itself declares -----------------------------------
    spear_variants = int(constant("SpearVariants"))
    # ArcherVariantBase is written as `SpearVariants`, Variants as `SpearVariants + ArcherVariants`.
    archer_variants = int(constant("ArcherVariants"))
    archer_base = spear_variants
    total_variants = spear_variants + archer_variants
    assert constant("ArcherVariantBase") == "SpearVariants", \
        "SwarmFragments.h: ArcherVariantBase is no longer SpearVariants — this check's block " \
        "arithmetic, and Kindled.Adapt's own conversion, both assume it is."

    # --- tier spine: upgrades.json tier_ladder -> Swarm.TierHP / Swarm.TierDPS ------
    upgrades = json.loads(UPGRADES.read_text(encoding="utf-8"))
    tiers = upgrades["tier_ladder"]["tiers"]
    want_hp = [float(t["hp"]) for t in tiers]
    want_dps = [float(t["dps"]) for t in tiers]
    got_hp = floats(cvar_default(COMBAT, "Swarm.TierHP"))
    got_dps = floats(cvar_default(COMBAT, "Swarm.TierDPS"))
    if got_hp != want_hp:
        failures.append("Swarm.TierHP is %s, upgrades.json tier_ladder is %s" % (got_hp, want_hp))
    if got_dps != want_dps:
        failures.append("Swarm.TierDPS is %s, upgrades.json tier_ladder is %s" % (got_dps, want_dps))

    tier_ids = [t["id"] for t in tiers]

    # --- ladders: unit-types.json adaptation.ladders[] -> Kindled.Adaptation.Ladders -
    unit_types = json.loads(UNIT_TYPES.read_text(encoding="utf-8"))
    want_ladders = {}
    for ladder in unit_types["adaptation"]["ladders"]:
        want_ladders[ladder["id"]] = (ladder["unit_type"],
                                      [r["variant_index"] for r in ladder["rungs"]],
                                      [r["tier"] for r in ladder["rungs"]])

    got_ladders = {}
    for entry in cvar_default(COMMANDS, "Kindled.Adaptation.Ladders").split(";"):
        if not entry.strip():
            continue
        ladder_id, csv = entry.split(":", 1)
        got_ladders[ladder_id.strip()] = [int(p) for p in csv.split(",")]

    missing = sorted(set(want_ladders) - set(got_ladders))
    extra = sorted(set(got_ladders) - set(want_ladders))
    if missing:
        failures.append("Kindled.Adaptation.Ladders is missing %s (in unit-types.json)" % missing)
    if extra:
        failures.append("Kindled.Adaptation.Ladders has %s, which unit-types.json does not" % extra)

    for ladder_id in sorted(set(want_ladders) & set(got_ladders)):
        unit_type, want_rungs, want_tiers = want_ladders[ladder_id]
        got_rungs = got_ladders[ladder_id]
        if got_rungs != want_rungs:
            failures.append("ladder '%s': CVar has %s, unit-types.json has %s"
                            % (ladder_id, got_rungs, want_rungs))

        # Rung order IS tier order (adaptation.md sec.3) — the runtime uses the rung index
        # directly as the Swarm.TierHP row, so a ladder that skips or reorders tiers would
        # silently stat a rung off a row it was never assigned.
        if want_tiers != tier_ids[:len(want_tiers)]:
            failures.append("ladder '%s': rung tiers are %s, but a rung index IS a "
                            "Swarm.TierHP row index, so they must be tier_ladder order %s"
                            % (ladder_id, want_tiers, tier_ids[:len(want_tiers)]))

        # Atlas block vs unit_type — Kindled.Adapt refuses these at runtime, but refusing
        # at runtime means discovering it in PIE.
        low, high = (archer_base, total_variants) if unit_type == "archers" else (0, spear_variants)
        for rung, index in enumerate(got_rungs):
            if not low <= index < high:
                failures.append("ladder '%s' rung %d: atlas index %d is outside the %s block "
                                "(%d-%d)" % (ladder_id, rung, index, unit_type, low, high - 1))

        if len(got_rungs) > len(want_hp):
            failures.append("ladder '%s' has %d rungs but only %d tier rows exist"
                            % (ladder_id, len(got_rungs), len(want_hp)))

    for line in failures:
        print("FAIL: %s" % line)
    if failures:
        return 1
    print("OK: %d ladder(s) and %d tier row(s) agree across C++ and docs/data."
          % (len(want_ladders), len(want_hp)))
    return 0


if __name__ == "__main__":
    sys.exit(check())
