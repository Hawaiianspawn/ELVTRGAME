# check_brood_variants.py
#
# The swarm split into TWO independent atlases in task-085 (team, enemy), and each one's
# layout has to be right in FOUR places at once, three of which are text files, checked here:
#
#     1. ELVTR/Source/ELVTR/Mass/SwarmFragments.h  -- SwarmSheet::Team/Enemy::Variants
#     2. docs/data/art/requests/team-units.json /   -- output.grid + frame_map
#        enemy-units.json
#     3. docs/data/art/team-variants.json /         -- one weight per variant, and the
#        brood-variants.json                           compiled CVar default that mirrors it
#
# The FOURTH is each side's Niagara Sprite Renderer "Sub UV" field on NS_Swarm, which lives in
# a .uasset and cannot be read from here. This script prints the value each one must hold;
# verify by reading it back from the asset (see docs/perf/niagara-sprite-path.md).
#
# A drift in any of these is SILENT at runtime and looks like every unit on that side wearing
# the wrong frame, which is why this exists.
#
#     python Scripts/art/check_brood_variants.py

import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[2]
FRAGMENTS = REPO / "ELVTR/Source/ELVTR/Mass/SwarmFragments.h"
PROCESSORS = REPO / "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp"

DIRS = ["south", "south-east", "east", "north-east",
        "north", "north-west", "west", "south-west"]


def constant(text, name, within=None, literal=True):
    """First `constexpr int32 <name> = ...` in text, optionally restricted to the slice of
    text after a marker (used to disambiguate SwarmSheet::Team vs ::Enemy, which both
    declare a same-named `Variants` constant in their own nested namespace).

    literal=False returns the right-hand side verbatim instead of an int, for the
    constants that are defined in terms of another rather than as a number."""
    if within:
        start = text.index(within)
        text = text[start:]
    pattern = r"constexpr int32 %s = (\d+)" % name if literal \
        else r"constexpr int32 %s = ([A-Za-z_][A-Za-z0-9_]*)" % name
    m = re.search(pattern, text)
    assert m, "%s not found -- did the constant get renamed?" % name
    return int(m.group(1)) if literal else m.group(1)


def check_side(label, frag, variants, blocks, request_path, weights_path):
    """One side of the split (team or enemy).

    `blocks` is the side's SUB-TABLES, in atlas order: (name, count, cvar, weights_key).
    The enemy side has exactly one; the team side has had two since task-126 -- spearmen
    and archers -- because the render int32's variant field is four bits and the team
    atlas outgrew a flat 0-15 index. Each block owns its own weights CVar and its own
    list in the weights JSON, but they share ONE grid, so the row check below runs over
    the concatenation and the atlas index keeps counting across the boundary.
    """
    req = json.loads(request_path.read_text(encoding="utf-8"))
    grid_cols, grid_rows = req["output"]["grid"]
    frame_map = req["output"]["frame_map"]
    weights_doc = json.loads(weights_path.read_text(encoding="utf-8"))

    assert grid_cols == 8, "%s: grid columns %d != SwarmSheet::Columns 8" % (label, grid_cols)
    expected_rows = variants * 2
    assert grid_rows == expected_rows, \
        "%s: grid rows %d != Variants*2 = %d" % (label, grid_rows, expected_rows)
    assert len(frame_map) == grid_cols * grid_rows, \
        "%s: frame_map has %d cells, grid holds %d" % (label, len(frame_map), grid_cols * grid_rows)

    prefixes = [s["prefix"] for s in req["composite"]["sources"]]
    proc = PROCESSORS.read_text(encoding="utf-8")
    total = 0
    for name, count, cvar_name, weights_key in blocks:
        weights = weights_doc[weights_key]
        assert len(weights) == count, \
            "%s/%s: weights file lists %d variants, code says %d" \
            % (label, name, len(weights), count)
        for offset, v in enumerate(weights):
            idx = v["index"]
            assert idx == total + offset, \
                "%s/%s: entry %d claims atlas index %d, expected %d" \
                % (label, name, offset, idx, total + offset)
            assert v["id"] == prefixes[idx], \
                "%s: variant %d is '%s' in the weights file but '%s' in the request" \
                % (label, idx, v["id"], prefixes[idx])
            for frame in (0, 1):
                for col, direction in enumerate(DIRS):
                    cell = str(col + (idx * 2 + frame) * grid_cols)
                    want = "%s:%s.walk%d" % (v["id"], direction, frame)
                    assert frame_map.get(cell) == want, \
                        "%s: cell %s is %r, expected %r" % (label, cell, frame_map.get(cell), want)

        m = re.search(r'TEXT\("%s"\), TEXT\("([^"]*)"\)' % re.escape(cvar_name), proc)
        assert m, "%s: %s default not found in SwarmProcessors.cpp" % (label, cvar_name)
        compiled = [int(x) for x in m.group(1).split(",")]
        documented = [v["weight"] for v in weights]
        assert compiled == documented, \
            "%s/%s: CVar default %s != weights file %s" % (label, name, compiled, documented)

        # Four bits, 0-15 -- the WITHIN-BLOCK index is what the render int32 carries, so
        # this is a per-block ceiling, not a per-side one. That is the whole point of the
        # row offset SwarmRenderActor.cpp applies (task-126).
        assert count <= 16, \
            "%s/%s: %d variants will not fit 4 bits" % (label, name, count)
        print("OK  %s/%s: %d variants at atlas index %d-%d, %s = %s"
              % (label, name, count, total, total + count - 1, cvar_name, documented))
        total += count

    assert total == variants, \
        "%s: sub-tables total %d, SwarmSheet says %d" % (label, total, variants)
    print("OK  %s: %d variants, %dx%d atlas (%d cells)"
          % (label, variants, grid_cols, grid_rows, len(frame_map)))
    print("REMINDER  NS_Swarm's %s emitter Sub UV must read %d x %d"
          % (label, grid_cols, grid_rows))


def check():
    frag = FRAGMENTS.read_text(encoding="utf-8")
    cols = constant(frag, "Columns")
    assert cols == 8, "SwarmSheet::Columns moved off 8 -- update DIRS/this script too"
    # Team::Variants is a SUM of its two sub-table constants, not a literal, so read the
    # parts and add them here -- the regex only ever matched literals, which is why this
    # script has been dead since task-126 split the team table (caught by task-128).
    team_spear = constant(frag, "SpearVariants", within="namespace Team")
    team_archer = constant(frag, "ArcherVariants", within="namespace Team")
    team_base = constant(frag, "ArcherVariantBase", within="namespace Team", literal=False)
    assert team_base == "SpearVariants", \
        "Team::ArcherVariantBase is %r, not SpearVariants -- the blocks no longer abut" % team_base
    team_variants = team_spear + team_archer
    enemy_variants = constant(frag, "Variants", within="namespace Enemy")
    m = re.search(r"constexpr int32 VariantShift = (\d+)", frag)
    shift = int(m.group(1)) if m else None
    assert shift == 21, "VariantShift moved to %s -- update the docs" % shift

    check_side("team", frag, team_variants,
               [("spearmen", team_spear, "Swarm.TeamVariantWeights", "variants"),
                ("archers", team_archer, "Swarm.ArcherVariantWeights", "archer_variants")],
               REPO / "docs/data/art/requests/team-units.json",
               REPO / "docs/data/art/team-variants.json")
    check_side("enemy", frag, enemy_variants,
               [("brood", enemy_variants, "Swarm.BroodVariantWeights", "variants")],
               REPO / "docs/data/art/requests/enemy-units.json",
               REPO / "docs/data/art/brood-variants.json")
    return 0


if __name__ == "__main__":
    sys.exit(check())
