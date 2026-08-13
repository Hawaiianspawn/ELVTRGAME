"""Generate the mechanical half of a composite atlas request.

A composite request (docs/data/art/requests/{team,enemy,swarm}-units.json) carries two
big blocks that nobody should ever type:

    composite.sources[].frames   16 entries per variant: 8 directions x walk0/walk1
    output.frame_map             one entry per cell: 16 per variant

team-units.json is 24 variants, so that is 768 hand-authored lines for 24 real facts.
Both blocks are 100% derivable from the ordered source list, verified against all three
shipped atlases 2026-07-31 (688 frame entries + 688 frame_map entries, zero mismatches):

    frames["<direction>.walk<w>"]  = "<rotations-dir>/<direction>.png"   (walk0 == walk1)
    frame_map[col + (variant*2 + w) * 8] = "<prefix>:<direction>.walk<w>"

So the source list is the only truth here and everything else is output. This script
regenerates the derived blocks in place. It deliberately writes the expansion back into
the request rather than teaching pixelpipe a second input form: `pack`, the schema, the
manifest and every existing consumer stay untouched, and the expanded file remains the
diffable, reviewable artifact that provenance depends on.

    py Scripts/art/atlas.py check --all
    py Scripts/art/atlas.py check team-units
    py Scripts/art/atlas.py sync  team-units
    py Scripts/art/atlas.py add   team-units --prefix archer-foo \
        --rotations RawArt/Renders/archer-foo/raw/state00/rotations --at 24
    py Scripts/art/atlas.py remove team-units --prefix archer-foo

`check` is the one to run in CI or before a pack: it re-derives both blocks and diffs
them against what is on disk, confirms every referenced PNG exists, and cross-checks the
grid and the SwarmSheet constants in SwarmFragments.h. Those C++ constants are the
"change one, change all four" trap called out in the request's own frame_map_note --
this reports the drift but never edits C++.

After add/remove you must still: bump the SwarmSheet constants this prints, re-pack
(`pixelpipe.py pack <id>`), re-import, and widen the Niagara emitter's Sub UV rows.
"""

import argparse
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REQUESTS = os.path.join(REPO, "docs", "data", "art", "requests")
FRAGMENTS = os.path.join(REPO, "ELVTR", "Source", "ELVTR", "Mass", "SwarmFragments.h")

# Column order is the atlas contract, shared by every sheet: SwarmSheet::Columns == 8 and
# SwarmFacing::ColumnFor resolves a view-relative facing into exactly this order. Changing
# it silently rotates every unit in the game.
DIRECTIONS = ["south", "south-east", "east", "north-east",
              "north", "north-west", "west", "south-west"]
WALK_FRAMES = 2   # row = variant*2 + frame; walk1 is a copy until a real walk lands

# Which SwarmSheet namespace each atlas feeds. swarm-units is the retired combined sheet
# (superseded by team+enemy 2026-07-25) and backs no live constant.
CPP_NAMESPACE = {"team-units": "Team", "enemy-units": "Enemy"}


class Fail(Exception):
    pass


def request_path(rid):
    p = os.path.join(REQUESTS, "%s.json" % rid)
    if not os.path.isfile(p):
        raise Fail("no such request: %s" % p)
    return p


def load(rid):
    with open(request_path(rid), encoding="utf-8") as fh:
        return json.load(fh)


def save(rid, req):
    # 2-space indent + trailing newline matches every request already in the tree, so a
    # sync produces no incidental diff noise.
    with open(request_path(rid), "w", encoding="utf-8", newline="\n") as fh:
        json.dump(req, fh, indent=2, ensure_ascii=False)
        fh.write("\n")


def sources_of(req, rid):
    comp = req.get("composite")
    if not comp or not comp.get("sources"):
        raise Fail("%s is not a composite request (no composite.sources)" % rid)
    return comp["sources"]


def rotations_dir(source):
    """The one directory every frame of a source points into.

    Inferred rather than stored: the existing requests have no field for it, and adding
    one would mean migrating three files and the schema to record something the paths
    already say unambiguously.
    """
    dirs = {os.path.dirname(v).replace("\\", "/") for v in source["frames"].values()}
    if len(dirs) != 1:
        raise Fail("source '%s' spans %d directories (%s) -- not expressible as a "
                   "generated block; fix the request by hand"
                   % (source["prefix"], len(dirs), ", ".join(sorted(dirs))))
    return dirs.pop()


def expand_frames(rot_dir):
    return {"%s.walk%d" % (d, w): "%s/%s.png" % (rot_dir, d)
            for d in DIRECTIONS for w in range(WALK_FRAMES)}


def expand_frame_map(prefixes, cols):
    fm = {}
    for variant, prefix in enumerate(prefixes):
        for w in range(WALK_FRAMES):
            for col, d in enumerate(DIRECTIONS):
                cell = col + (variant * WALK_FRAMES + w) * cols
                fm[str(cell)] = "%s:%s.walk%d" % (prefix, d, w)
    return fm


def derive(req, rid):
    """Both derived blocks, plus the grid, for the request's current source list."""
    sources = sources_of(req, rid)
    cols = req["output"]["grid"][0]
    if cols != len(DIRECTIONS):
        raise Fail("%s has grid width %d but the atlas contract is %d direction columns"
                   % (rid, cols, len(DIRECTIONS)))
    frames = {s["prefix"]: expand_frames(rotations_dir(s)) for s in sources}
    prefixes = [s["prefix"] for s in sources]
    if len(set(prefixes)) != len(prefixes):
        dupes = sorted({p for p in prefixes if prefixes.count(p) > 1})
        raise Fail("duplicate prefixes in %s: %s" % (rid, ", ".join(dupes)))
    return frames, expand_frame_map(prefixes, cols), [cols, len(sources) * WALK_FRAMES]


# ------------------------------------------------------------------ C++ cross-check

def cpp_variants(namespace):
    """`Variants` as SwarmFragments.h declares it, or None if it can't be read.

    Best-effort on purpose: a parse miss must not block a pack. Returns (total, detail).
    """
    try:
        with open(FRAGMENTS, encoding="utf-8") as fh:
            text = fh.read()
    except OSError:
        return None, "could not read %s" % FRAGMENTS

    start = text.find("namespace %s" % namespace)
    if start < 0:
        return None, "no `namespace %s` in SwarmFragments.h" % namespace
    end = text.find("\n\t}", start)
    block = text[start:end if end > 0 else len(text)]
    consts = {m.group(1): int(m.group(2))
              for m in re.finditer(r"constexpr int32 (\w+)\s*=\s*(\d+)", block)}

    # Team splits its one grid into two sub-tables; its `Variants` is a sum expression the
    # regex above does not see, so rebuild it from the parts.
    if "SpearVariants" in consts and "ArcherVariants" in consts:
        total = consts["SpearVariants"] + consts["ArcherVariants"]
        return total, ("SpearVariants %d + ArcherVariants %d = %d"
                       % (consts["SpearVariants"], consts["ArcherVariants"], total))
    if "Variants" in consts:
        return consts["Variants"], "Variants %d" % consts["Variants"]
    return None, "no Variants constant found in namespace %s" % namespace


# ------------------------------------------------------------------ commands

def cmd_check(args):
    ids = sorted(os.path.splitext(f)[0] for f in os.listdir(REQUESTS)
                 if f.endswith(".json")) if args.all else [args.id]
    problems = 0
    checked = 0

    for rid in ids:
        req = load(rid)
        if not req.get("composite", {}).get("sources"):
            if not args.all:
                raise Fail("%s is not a composite request" % rid)
            continue
        checked += 1
        frames, fm, grid = derive(req, rid)
        bad = []

        for source in sources_of(req, rid):
            have = source["frames"]
            want = frames[source["prefix"]]
            if have != want:
                only_have = sorted(set(have) - set(want))
                only_want = sorted(set(want) - set(have))
                changed = sorted(k for k in set(have) & set(want)
                                 if have[k].replace("\\", "/") != want[k])
                bad.append("  source '%s' frames drift: %d unexpected, %d missing, "
                           "%d wrong path" % (source["prefix"], len(only_have),
                                              len(only_want), len(changed)))

        if req["output"].get("frame_map") != fm:
            have = req["output"].get("frame_map") or {}
            diff = sorted(set(have.items()) ^ set(fm.items()))
            bad.append("  frame_map drift: %d cell(s) differ from the derived layout "
                       "(have %d entries, derived %d)" % (len(diff), len(have), len(fm)))

        if req["output"]["grid"] != grid:
            bad.append("  output.grid is %s but %d variants derive %s"
                       % (req["output"]["grid"], len(sources_of(req, rid)), grid))

        for source in sources_of(req, rid):
            for key, rel in sorted(source["frames"].items()):
                if not os.path.isfile(os.path.join(REPO, rel)):
                    bad.append("  missing PNG: %s (%s:%s)" % (rel, source["prefix"], key))
                    break   # one per source is enough to act on

        ns = CPP_NAMESPACE.get(rid)
        if ns:
            total, detail = cpp_variants(ns)
            n = len(sources_of(req, rid))
            if total is None:
                bad.append("  could not cross-check SwarmSheet::%s (%s)" % (ns, detail))
            elif total != n:
                bad.append("  SwarmSheet::%s says %s but the request has %d variants -- "
                           "the sheet and the sim disagree about what a row means"
                           % (ns, detail, n))

        if bad:
            problems += 1
            print("%s: DRIFT" % rid)
            for line in bad:
                print(line)
        else:
            print("%s: ok (%d variants, %d cells, grid %dx%d)"
                  % (rid, len(sources_of(req, rid)), len(fm), grid[0], grid[1]))

    if not checked:
        raise Fail("no composite requests found")
    if problems:
        print("\n%d atlas(es) drifted. `atlas.py sync <id>` rewrites the derived blocks; "
              "C++ constant drift must be fixed by hand." % problems)
    return 1 if problems else 0


def cmd_sync(args):
    rid = args.id
    req = load(rid)
    frames, fm, grid = derive(req, rid)
    before = (json.dumps([s["frames"] for s in sources_of(req, rid)], sort_keys=True),
              json.dumps(req["output"].get("frame_map"), sort_keys=True),
              req["output"]["grid"])

    for source in sources_of(req, rid):
        source["frames"] = frames[source["prefix"]]
    req["output"]["frame_map"] = fm
    req["output"]["grid"] = grid

    after = (json.dumps([s["frames"] for s in sources_of(req, rid)], sort_keys=True),
             json.dumps(req["output"]["frame_map"], sort_keys=True),
             req["output"]["grid"])

    if before == after:
        print("%s: already in sync (%d variants, %d cells)"
              % (rid, len(sources_of(req, rid)), len(fm)))
        return 0
    save(rid, req)
    print("%s: rewrote %d frame entries + %d frame_map cells, grid %dx%d"
          % (rid, sum(len(f) for f in frames.values()), len(fm), grid[0], grid[1]))
    return 0


def _post_change_notes(rid, req):
    n = len(sources_of(req, rid))
    print("\nStill to do by hand -- this script only owns the request:")
    ns = CPP_NAMESPACE.get(rid)
    if ns:
        total, detail = cpp_variants(ns)
        print("  1. SwarmFragments.h  SwarmSheet::%s -- now %s, needs to total %d"
              % (ns, detail if total is not None else "unreadable", n))
    print("  %d. py Scripts/art/pixelpipe.py pack %s" % (2 if ns else 1, rid))
    print("  %d. re-import the sheet (Scripts/art/import_sprites.py %s)"
          % (3 if ns else 2, rid))
    print("  %d. widen the Niagara emitter's Sub UV to %d x %d"
          % (4 if ns else 3, len(DIRECTIONS), n * WALK_FRAMES))


def cmd_add(args):
    rid = args.id
    req = load(rid)
    sources = sources_of(req, rid)

    if any(s["prefix"] == args.prefix for s in sources):
        raise Fail("%s already has a source with prefix '%s'" % (rid, args.prefix))

    rot = args.rotations.replace("\\", "/").rstrip("/")
    missing = [d for d in DIRECTIONS
               if not os.path.isfile(os.path.join(REPO, rot, "%s.png" % d))]
    if missing and not args.allow_missing:
        raise Fail("%s is missing %d of the %d rotations (%s).\nGenerate them first, or "
                   "pass --allow-missing to wire up the rows anyway (pack will then fail "
                   "loudly instead of this script failing early)."
                   % (rot, len(missing), len(DIRECTIONS), ", ".join(missing)))

    at = len(sources) if args.at is None else args.at
    if not 0 <= at <= len(sources):
        raise Fail("--at %d out of range 0..%d" % (at, len(sources)))
    if at != len(sources):
        print("WARNING inserting at %d shifts every later variant's row AND its index. "
              "Anything keyed on the old index -- Swarm.TeamVariantWeights, task-095's "
              "knight stat rows, brood-variants.json weights -- is now pointing at the "
              "wrong look. Appending is the safe move." % at)

    sources.insert(at, {
        "prefix": args.prefix,
        "frames": expand_frames(rot),
        "quantize": args.quantize,
        "note": args.note or ("Added by atlas.py. Rotations: %s" % rot),
    })

    frames, fm, grid = derive(req, rid)
    for source in sources:
        source["frames"] = frames[source["prefix"]]
    req["output"]["frame_map"] = fm
    req["output"]["grid"] = grid
    save(rid, req)

    print("%s: added '%s' at index %d -- now %d variants, rows %d-%d, grid %dx%d"
          % (rid, args.prefix, at, len(sources),
             at * WALK_FRAMES, at * WALK_FRAMES + 1, grid[0], grid[1]))
    if missing:
        print("NOTE  %d rotation PNG(s) do not exist yet: %s" % (len(missing),
                                                                 ", ".join(missing)))
    _post_change_notes(rid, req)
    return 0


def cmd_remove(args):
    rid = args.id
    req = load(rid)
    sources = sources_of(req, rid)

    hit = [i for i, s in enumerate(sources) if s["prefix"] == args.prefix]
    if not hit:
        raise Fail("%s has no source with prefix '%s' (have: %s)"
                   % (rid, args.prefix, ", ".join(s["prefix"] for s in sources)))
    at = hit[0]
    if at != len(sources) - 1:
        print("WARNING removing index %d shifts every later variant's index down by one. "
              "Re-check Swarm.TeamVariantWeights, task-095's stat rows and "
              "brood-variants.json before packing." % at)

    sources.pop(at)
    frames, fm, grid = derive(req, rid)
    for source in sources:
        source["frames"] = frames[source["prefix"]]
    req["output"]["frame_map"] = fm
    req["output"]["grid"] = grid
    save(rid, req)

    print("%s: removed '%s' (was index %d) -- now %d variants, grid %dx%d"
          % (rid, args.prefix, at, len(sources), grid[0], grid[1]))
    _post_change_notes(rid, req)
    return 0


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("check", help="re-derive both blocks and diff against disk")
    p.add_argument("id", nargs="?")
    p.add_argument("--all", action="store_true", help="every composite request")
    p.set_defaults(fn=cmd_check)

    p = sub.add_parser("sync", help="rewrite the derived blocks in place")
    p.add_argument("id")
    p.set_defaults(fn=cmd_sync)

    p = sub.add_parser("add", help="append (or insert) a variant and re-derive")
    p.add_argument("id")
    p.add_argument("--prefix", required=True, help="frame_map key, e.g. archer-foo")
    p.add_argument("--rotations", required=True,
                   help="repo-relative dir holding <direction>.png for all 8 directions")
    p.add_argument("--at", type=int, default=None,
                   help="insert index; default appends. Inserting renumbers later "
                        "variants -- see the warning it prints.")
    p.add_argument("--note", default=None, help="provenance note for the source")
    p.add_argument("--quantize", action="store_true",
                   help="repair onto a palette ramp at pack time. Off by default: the "
                        "4-value lock was superseded 2026-07-28 and every composite "
                        "source in the tree sets quantize false.")
    p.add_argument("--allow-missing", action="store_true",
                   help="wire up rows whose PNGs are not generated yet")
    p.set_defaults(fn=cmd_add)

    p = sub.add_parser("remove", help="drop a variant and re-derive")
    p.add_argument("id")
    p.add_argument("--prefix", required=True)
    p.set_defaults(fn=cmd_remove)

    args = ap.parse_args()
    if args.cmd == "check" and not args.all and not args.id:
        ap.error("check needs an id, or --all")
    try:
        return args.fn(args)
    except Fail as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
