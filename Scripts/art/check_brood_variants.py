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


def constant(text, name, within=None):
    """First `constexpr int32 <name> = N` in text, optionally restricted to the slice of
    text between two markers (used to disambiguate SwarmSheet::Team vs ::Enemy, which both
    declare a same-named `Variants` constant in their own nested namespace)."""
    if within:
        start = text.index(within)
        text = text[start:]
    m = re.search(r"constexpr int32 %s = (\d+)" % name, text)
    assert m, "%s not found -- did the constant get renamed?" % name
    return int(m.group(1))


def check_side(label, frag, variants, cvar_name, request_path, weights_path, retinue_offset):
    """One side of the split (team or enemy). retinue_offset is 0 for enemy (every row pair
    is a plain variant) -- kept as a parameter rather than hardcoded because the two sides
    share this exact shape now that the retinue moved into the team table as variant 0."""
    req = json.loads(request_path.read_text(encoding="utf-8"))
    grid_cols, grid_rows = req["output"]["grid"]
    frame_map = req["output"]["frame_map"]
    weights = json.loads(weights_path.read_text(encoding="utf-8"))["variants"]

    assert grid_cols == 8, "%s: grid columns %d != SwarmSheet::Columns 8" % (label, grid_cols)
    expected_rows = variants * 2
    assert grid_rows == expected_rows, \
        "%s: grid rows %d != Variants*2 = %d" % (label, grid_rows, expected_rows)
    assert len(frame_map) == grid_cols * grid_rows, \
        "%s: frame_map has %d cells, grid holds %d" % (label, len(frame_map), grid_cols * grid_rows)

    prefixes = [s["prefix"] for s in req["composite"]["sources"]]
    assert len(weights) == variants, \
        "%s: weights file lists %d variants, code says %d" % (label, len(weights), variants)
    for v in weights:
        idx = v["index"]
        assert v["id"] == prefixes[idx], \
            "%s: variant %d is '%s' in the weights file but '%s' in the request" \
            % (label, idx, v["id"], prefixes[idx])
        for frame in (0, 1):
            for col, direction in enumerate(DIRS):
                cell = str(col + (idx * 2 + frame) * grid_cols)
                want = "%s:%s.walk%d" % (v["id"], direction, frame)
                assert frame_map.get(cell) == want, \
                    "%s: cell %s is %r, expected %r" % (label, cell, frame_map.get(cell), want)

    proc = PROCESSORS.read_text(encoding="utf-8")
    m = re.search(r'TEXT\("%s"\), TEXT\("([^"]*)"\)' % re.escape(cvar_name), proc)
    assert m, "%s: %s default not found in SwarmProcessors.cpp" % (label, cvar_name)
    compiled = [int(x) for x in m.group(1).split(",")]
    documented = [v["weight"] for v in weights]
    assert compiled == documented, \
        "%s: CVar default %s != weights file %s" % (label, compiled, documented)

    assert variants <= 16, "%s: %d variants will not fit 4 bits" % (label, variants)

    print("OK  %s: %d variants, %dx%d atlas (%d cells), weights %s"
          % (label, variants, grid_cols, grid_rows, len(frame_map), documented))
    print("REMINDER  NS_Swarm's %s emitter Sub UV must read %d x %d"
          % (label, grid_cols, grid_rows))


def check():
    frag = FRAGMENTS.read_text(encoding="utf-8")
    cols = constant(frag, "Columns")
    assert cols == 8, "SwarmSheet::Columns moved off 8 -- update DIRS/this script too"
    team_variants = constant(frag, "Variants", within="namespace Team")
    enemy_variants = constant(frag, "Variants", within="namespace Enemy")
    m = re.search(r"constexpr int32 VariantShift = (\d+)", frag)
    shift = int(m.group(1)) if m else None
    assert shift == 21, "VariantShift moved to %s -- update the docs" % shift

    check_side("team", frag, team_variants, "Swarm.TeamVariantWeights",
               REPO / "docs/data/art/requests/team-units.json",
               REPO / "docs/data/art/team-variants.json", retinue_offset=0)
    check_side("enemy", frag, enemy_variants, "Swarm.BroodVariantWeights",
               REPO / "docs/data/art/requests/enemy-units.json",
               REPO / "docs/data/art/brood-variants.json", retinue_offset=0)
    return 0


if __name__ == "__main__":
    sys.exit(check())
