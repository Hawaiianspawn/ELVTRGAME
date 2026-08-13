"""The roster: every unit the game has or wants, what stage it is at, and what it is for.

WHY THIS EXISTS
---------------
The project knows a great deal about each unit and none of it in one place. Whether a look
has art is a directory listing. Whether it measured well is a family manifest. Whether it
ships is a source entry in a composite request. Whether it is actually VISIBLE in game is a
constant in SwarmFragments.h and a field in a .uasset. Whether it was ever meant to exist
at all is prose in a design doc. Nothing joins them, so "what units do we have, and what
still needs doing" has never been answerable without reading five kinds of file.

This joins them. It is a model, not a page -- Scripts/art/forge.py renders it.

WHAT IS STORED VS DERIVED
-------------------------
Almost nothing is stored. Stage is COMPUTED from what is on disk and in the manifests every
time it is asked for, so it cannot go stale and there is no second source of truth to
reconcile. `roster.json` holds only the things no file in the repo can answer:

    group          which bucket a unit belongs in -- nothing in the tree records that a
                   look is a "hero" rather than "team", and no rule can infer it (the
                   atlas split cannot tell a knight from an archer)
    expectation     what the unit is FOR, in the owner's words
    notes           the running conversation about it
    concept units   units that are wanted but have no art yet, which by definition cannot
                    be discovered from disk

The owner's approve/deny verdict is deliberately NOT stored here: it already lives in
`docs/data/art/families/<f>/manifest.json` under `owner`, written by forge, and duplicating
it would create exactly the drift this file exists to prevent.

THE STAGE LADDER
----------------
    concept    named, has an expectation, no art
    generated  eight rotations on disk
    measured   silhouette numbers and a judge verdict recorded
    approved   the owner's gate -- the only stage a human sets
    animated   real walk, attack and death frames (see ANIM_STATES)
    packed     an atlas row, a packed sheet, and an imported texture
    live       the C++ constant and the sheet agree, so it is actually on screen

Stage is the HIGHEST rung satisfied, not the furthest along an unbroken chain. A look that
shipped before any of this tooling existed reads `packed` even though nobody ever clicked
approve -- that is the truth about it, and `gate_passed` reports the missing click
separately rather than pretending the unit is less finished than it is.

`live` is a rung of its own because `ship` cannot reach it: adding an atlas row needs a
`SwarmSheet::` constant bumped and the Niagara emitter's Sub UV widened, and a unit sitting
between `packed` and `live` is invisible in game while looking finished everywhere else.
"""

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import silhouette_report as sr   # noqa: E402
import variantpipe as vp         # noqa: E402

REPO = vp.REPO
REQUESTS = REPO / "docs" / "data" / "art" / "requests"
FRAGMENTS = REPO / "ELVTR" / "Source" / "ELVTR" / "Mass" / "SwarmFragments.h"
ROSTER = REPO / "docs" / "data" / "art" / "roster.json"
Fail = vp.Fail

STAGES = ["concept", "generated", "measured", "approved", "animated", "packed", "live"]

# What "animated" means, decided 2026-08-08. Walk is free -- both sheets already carry a
# walk0/walk1 row axis whose two frames are duplicates of each other, waiting to be filled.
# Attack and death are not: attack is faked today by a lunge (the render position leads the
# true position mid-swing) and death has no bit and no cell at all, so both need a sheet
# layout change. HIT is deliberately absent: SwarmFragments.h records it as lost on the
# sprite path, and listing it here would put a permanent red mark against every unit for a
# decision nobody has made.
ANIM_STATES = ["walk", "attack", "death"]

# The render int32's variant field is four bits, so a block stops at 16 looks. It is a real
# ceiling worth showing on the page -- SwarmFragments.h's own note explains that blocks
# carry a WITHIN-BLOCK index and the pack loop adds an offset, which is why the team atlas
# can hold 24 looks total while neither of its two blocks may exceed 16.
BLOCK_CAP = 16

DEFAULT_GROUPS = ["heroes", "team", "enemies", "tests", "unsorted"]


# ---------------------------------------------------------------- roster.json

def load_roster():
    if ROSTER.exists():
        return vp.load_json(ROSTER)
    return {"groups": list(DEFAULT_GROUPS), "units": {}}


def save_roster(r):
    vp.save_json(ROSTER, r)


def entry(r, key):
    return (r.get("units") or {}).get(key) or {}


# ---------------------------------------------------------------- the tree, read once

# Which composite atlases are actually wired to the sim. Mirrors atlas.py's CPP_NAMESPACE,
# and swarm-units' absence is the point: SwarmFragments.h records it as the retired
# combined sheet, superseded by the team/enemy split on 2026-07-25, backing no live
# constant. It still exists and still lists sources.
LIVE_ATLASES = ("team-units", "enemy-units")


def composite_index():
    """{absolute rotations dir -> (request_id, prefix, index, sheet_exists)}.

    Keyed on the DIRECTORY a source's frames point into rather than on its prefix, because
    the prefix is a naming convention ('archer-' + slug) and the directory is a fact. A
    look packed under a prefix that no longer matches its slug still resolves.

    One directory can be packed into SEVERAL atlases, so this cannot be a plain
    last-writer-wins map. The nine brood-ooze folders are sources of both `enemy-units`
    and the retired `swarm-units`; resolving them to the retired sheet made every one of
    them read as team, packed-but-not-live, because swarm-units backs no C++ constant to
    compare against. A live atlas therefore always beats a retired one, and ties fall to
    the alphabetically first so the answer is stable.
    """
    found = {}
    for p in sorted(REQUESTS.glob("*.json")):
        try:
            req = json.loads(p.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
        sources = (req.get("composite") or {}).get("sources")
        if not sources:
            continue
        sheet = REPO / "RawArt" / "Sheets" / ("%s.png" % req["output"]["texture"])
        for i, s in enumerate(sources):
            dirs = {str(Path(v).parent) for v in (s.get("frames") or {}).values()}
            if len(dirs) != 1:
                continue
            key = (REPO / dirs.pop()).resolve()
            found.setdefault(key, []).append((p.stem, s["prefix"], i, sheet.exists()))
    return {k: min(v, key=lambda e: (e[0] not in LIVE_ATLASES, e[0]))
            for k, v in found.items()}


def cpp_blocks():
    """{block: (declared, cap)} from SwarmFragments.h, best effort.

    Read rather than assumed, and never written: atlas.py established that these constants
    are reported and fixed by hand, because changing one is a recompile.
    """
    try:
        text = FRAGMENTS.read_text(encoding="utf-8")
    except OSError:
        return {}
    got = {}
    for ns, keys in (("Enemy", [("Variants", "brood")]),
                     ("Team", [("SpearVariants", "spearmen"),
                               ("ArcherVariants", "archers")])):
        start = text.find("namespace %s" % ns)
        if start < 0:
            continue
        block = text[start:text.find("\n\t}", start) or len(text)]
        for const, label in keys:
            m = re.search(r"constexpr int32 %s\s*=\s*(\d+)" % const, block)
            if m:
                got[label] = (int(m.group(1)), BLOCK_CAP)
    return got


def atlas_counts():
    """{request_id: (sources_in_request, declared_in_cpp)} -- the drift `live` turns on."""
    ns = {"team-units": ("SpearVariants", "ArcherVariants"), "enemy-units": ("Variants",)}
    blocks = cpp_blocks()
    label = {"SpearVariants": "spearmen", "ArcherVariants": "archers", "Variants": "brood"}
    out = {}
    for rid, consts in ns.items():
        p = REQUESTS / ("%s.json" % rid)
        if not p.exists():
            continue
        req = json.loads(p.read_text(encoding="utf-8"))
        n = len((req.get("composite") or {}).get("sources") or [])
        declared = sum(blocks.get(label[c], (0, 0))[0] for c in consts)
        out[rid] = (n, declared)
    return out


def anim_of(dirs):
    """{state: bool} for a unit, given the directories its animation frames could be in.

    Nothing on disk uses a per-state folder for a family variant yet -- no source character
    has a generated walk -- so this reads false across the board today and is expected to.
    Two subject requests do have frames (hero-vanguard's walk, unit-retinue's), which is
    what keeps this from being an untested code path.
    """
    got = {s: False for s in ANIM_STATES}
    for d in dirs:
        if not d or not d.is_dir():
            continue
        for sub in d.iterdir():
            if not sub.is_dir():
                continue
            name = sub.name.lower()
            for state in ANIM_STATES:
                if state in name and any(sub.glob("*.png")):
                    got[state] = True
    return got


# ---------------------------------------------------------------- discovery

def discover_variants(ctx):
    """One unit per variant folder under every family that has renders."""
    units = []
    fam_root = vp.FAMILIES
    if not fam_root.exists():
        return units
    for fdir in sorted(p for p in fam_root.iterdir() if p.is_dir()):
        family = fdir.name
        raw = vp.raw_root(family)
        if not raw.exists():
            continue
        man = vp.load_manifest(family)
        for slug, rot in sr.variant_dirs(raw):
            rec = (man.get("variants") or {}).get(slug) or {}
            packed = ctx["composite"].get(rot.resolve())
            units.append({
                "key": "%s/%s" % (family, slug),
                "kind": "variant",
                "title": slug,
                "family": family, "slug": slug,
                "rotations": rot,
                "measured": bool(rec.get("measured")),
                "judge": rec.get("verdict"),
                "owner": rec.get("owner") or {},
                "refine_notes": rec.get("refine_notes") or [],
                "packed": packed,
                "anim": anim_of([rot.parent / "anim"]),
            })
    return units


def discover_orphans(ctx, claimed):
    """Units that SHIP but that family discovery cannot see.

    Not every folder under RawArt/Renders is a registered family, and a shipped look is not
    allowed to be invisible here whatever its layout. The retinue base look is the case
    that proved it: it sits at `unit-retinue-colour/raw/rotations/` -- flat, one level
    shallower than the `raw/<slug>/rotations/` convention -- inside a folder with no
    family.json, so it was the one source of team-units with no row on the roster.

    Scope is deliberately narrow: only directories a live atlas actually sources from.
    Sweeping every folder under RawArt/Renders would drag in rejected culls, style probes
    and scratch folders, which are candidates rather than units.
    """
    units = []
    for rot, packed in sorted(ctx["composite"].items()):
        if rot in claimed or not (rot / "south.png").exists():
            continue
        rel = rot.relative_to(vp.RENDERS) if str(rot).startswith(str(vp.RENDERS)) else rot
        units.append({
            "key": "shipped/%s" % str(rel).replace("\\", "/"),
            "kind": "variant",
            "title": packed[1],
            "family": None, "slug": None,
            "rotations": rot,
            "measured": False, "judge": None, "owner": {}, "refine_notes": [],
            "packed": packed,
            "anim": anim_of([rot.parent / "anim"]),
            "unregistered": True,
        })
    return units


def discover_requests(ctx):
    """One unit per non-composite request -- heroes, monsters, the soldier roster."""
    units = []
    for p in sorted(REQUESTS.glob("*.json")):
        try:
            req = json.loads(p.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
        if req.get("composite"):
            continue
        rid = p.stem
        rev = vp.RENDERS / rid / ("r%d" % req.get("revision", 1))
        rot = None
        for cand in (rev / "raw" / "rotation", rev / "quantized" / "rotation"):
            if cand.is_dir() and (cand / "south.png").exists():
                rot = cand
                break
        man = vp.load_json(rev / "manifest.json") if (rev / "manifest.json").exists() else {}
        sheet = man.get("sheet") or {}
        sheet_ok = bool(sheet) and (REPO / sheet.get("path", "x")).exists()
        content = req["output"]["content_path"].replace("/Game/", "ELVTR/Content/")
        uasset = REPO / content / ("%s.uasset" % req["output"]["texture"])
        units.append({
            "key": "request/%s" % rid,
            "kind": "request",
            "title": req["subject"]["title"],
            "request": rid, "subject_kind": req["subject"]["kind"],
            "rotations": rot,
            "measured": bool(man.get("qc")),
            "judge": None,
            # anchor.approved IS this path's owner gate -- the stage-D human sign-off the
            # /sprite skill describes. Reuse it rather than inventing a second one.
            "owner": {"verdict": "approve"} if req.get("anchor", {}).get("approved") else {},
            "refine_notes": [],
            "packed": (rid, req["output"]["texture"], None, sheet_ok) if sheet_ok else None,
            "live_asset": uasset.exists(),
            "anim": anim_of([rev / "raw" / "anim"]),
        })
    return units


# ---------------------------------------------------------------- stage

def stage_of(u, ctx):
    """(stage, gate_passed). Highest rung satisfied -- see the module docstring."""
    reached = ["concept"]
    if u.get("rotations") is not None:
        reached.append("generated")
    if u.get("measured"):
        reached.append("measured")
    gate = (u.get("owner") or {}).get("verdict") == "approve"
    if gate:
        reached.append("approved")
    if all(u["anim"].get(s) for s in ANIM_STATES):
        reached.append("animated")
    packed = u.get("packed")
    if packed and packed[3]:
        reached.append("packed")
        if u["kind"] == "request":
            if u.get("live_asset"):
                reached.append("live")
        else:
            n, declared = ctx["atlas"].get(packed[0], (0, -1))
            if n == declared:
                reached.append("live")
    return max(reached, key=STAGES.index), gate


def is_rejected(u):
    return (u.get("owner") or {}).get("verdict") == "deny"


def build(roster=None):
    """Every unit, staged and grouped. The one call a page needs."""
    r = roster or load_roster()
    ctx = {"composite": composite_index(), "atlas": atlas_counts()}
    units = discover_variants(ctx)
    claimed = {u["rotations"] for u in units if u.get("rotations")}
    units += discover_orphans(ctx, claimed) + discover_requests(ctx)

    for u in units:
        u["stage"], u["gate_passed"] = stage_of(u, ctx)
        u["rejected"] = is_rejected(u)
        e = entry(r, u["key"])
        u["group"] = e.get("group") or "unsorted"
        u["expectation"] = e.get("expectation") or ""
        u["notes"] = e.get("notes") or []

    # concept units exist only in roster.json -- nothing on disk can reveal them
    have = {u["key"] for u in units}
    for key, e in (r.get("units") or {}).items():
        if key in have or not key.startswith("concept/"):
            continue
        units.append({
            "key": key, "kind": "concept", "title": e.get("title") or key.split("/", 1)[1],
            "rotations": None, "measured": False, "judge": None, "owner": {},
            "refine_notes": [], "packed": None,
            "anim": {s: False for s in ANIM_STATES},
            "stage": "concept", "gate_passed": False, "rejected": False,
            "group": e.get("group") or "unsorted",
            "expectation": e.get("expectation") or "", "notes": e.get("notes") or [],
        })

    units.sort(key=lambda u: (u["group"], -STAGES.index(u["stage"]), u["title"].lower()))
    return units, r, ctx


# ---------------------------------------------------------------- edits

def set_field(key, field, value, title=None):
    if field not in ("group", "expectation", "title"):
        raise Fail("cannot set %r" % field)
    r = load_roster()
    u = r.setdefault("units", {}).setdefault(key, {})
    u[field] = value
    if title and not u.get("title"):
        u["title"] = title
    if field == "group" and value not in r.setdefault("groups", list(DEFAULT_GROUPS)):
        r["groups"].append(value)
    save_roster(r)
    return u


def add_note(key, text):
    text = (text or "").strip()
    if not text:
        raise Fail("empty note")
    r = load_roster()
    u = r.setdefault("units", {}).setdefault(key, {})
    u.setdefault("notes", []).append({"at": vp.now(), "text": text})
    save_roster(r)
    return u["notes"]


def add_concept(slug, title, group, expectation):
    slug = re.sub(r"[^a-z0-9]+", "-", (slug or "").lower()).strip("-")
    if not slug:
        raise Fail("a concept unit needs a slug")
    key = "concept/%s" % slug
    r = load_roster()
    if key in (r.get("units") or {}):
        raise Fail("'%s' is already on the roster" % slug)
    r.setdefault("units", {})[key] = {
        "title": title or slug, "group": group or "unsorted",
        "expectation": expectation or "", "notes": [],
    }
    if (group or "unsorted") not in r.setdefault("groups", list(DEFAULT_GROUPS)):
        r["groups"].append(group)
    save_roster(r)
    return key


# ---------------------------------------------------------------- report / seed

def summary(units, ctx):
    by_group, by_stage = {}, {s: 0 for s in STAGES}
    for u in units:
        by_group.setdefault(u["group"], []).append(u)
        by_stage[u["stage"]] += 1
    caps = {k: v for k, v in cpp_blocks().items()}
    return {"groups": by_group, "stages": by_stage, "capacity": caps,
            "atlas": ctx["atlas"], "total": len(units)}


def seed():
    """First-run grouping, so the page does not open with everything in one pile.

    A one-time guess written into roster.json, NOT a live derivation -- the file is the
    source of truth from the moment it exists and anything can be re-filed from the page.
    Where a look ships is the only signal the tree actually carries, so that is what this
    reads; it cannot tell a knight from an archer and does not try.
    """
    units, r, _ = build()
    for u in units:
        if entry(r, u["key"]).get("group"):
            continue
        p = u.get("packed")
        if u["kind"] == "request" and u.get("subject_kind") == "hero":
            g = "heroes"
        elif "test" in u["key"] or "style-" in u["key"]:
            g = "tests"
        elif p and p[0] == "enemy-units":
            g = "enemies"
        elif p and p[0] in ("team-units", "swarm-units"):
            g = "team"
        elif u["kind"] == "request" and u.get("subject_kind") == "monster":
            g = "enemies"
        elif u["kind"] == "request" and u.get("subject_kind") == "retinue":
            g = "team"
        else:
            g = "unsorted"
        rec = r.setdefault("units", {}).setdefault(u["key"], {})
        rec["group"] = g
        rec.setdefault("title", u["title"])
    save_roster(r)
    return r


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--seed", action="store_true", help="write first-run group assignments")
    ap.add_argument("--json", action="store_true")
    # set_field() has existed since this file was written; the CLI just never reached it,
    # so the expectation/note layer sat at 2 of 137 units. Positional, not a subparser,
    # to keep the bare `py roster.py` roster print as the default behaviour.
    ap.add_argument("cmd", nargs="?", choices=["set", "note"],
                    help="set <key> <group|expectation|title> <value>, or note <key> <text>")
    ap.add_argument("rest", nargs="*", help="arguments for cmd")
    args = ap.parse_args()

    if args.cmd == "set":
        if len(args.rest) != 3:
            raise Fail("set takes <key> <field> <value>")
        key, field, value = args.rest
        set_field(key, field, value)
        print("%s.%s set" % (key, field))
        return 0

    if args.cmd == "note":
        if len(args.rest) < 2:
            raise Fail("note takes <key> <text>")
        key = args.rest[0]
        add_note(key, " ".join(args.rest[1:]))
        print("%s note added" % key)
        return 0

    if args.seed:
        seed()
        print("seeded %s" % ROSTER.relative_to(REPO))

    units, r, ctx = build()
    s = summary(units, ctx)
    if args.json:
        print(json.dumps({"units": [{k: v for k, v in u.items() if k != "rotations"}
                                    for u in units], "summary": s}, indent=2, default=str))
        return 0

    for g, us in sorted(s["groups"].items()):
        print("\n%s (%d)" % (g.upper(), len(us)))
        for u in us:
            anim = "".join(x[0].upper() if u["anim"][x] else "-" for x in ANIM_STATES)
            print("  %-34s %-10s anim %s %s%s"
                  % (u["title"][:34], u["stage"], anim,
                     "" if u["gate_passed"] else "(no gate)",
                     "  REJECTED" if u["rejected"] else ""))
    print("\nstages: %s" % ", ".join("%d %s" % (n, k) for k, n in s["stages"].items() if n))
    for block, (used, cap) in sorted(s["capacity"].items()):
        print("capacity %-10s %d/%d looks" % (block, used, cap))
    for rid, (n, declared) in sorted(s["atlas"].items()):
        print("%-12s request %d sources, C++ declares %d%s"
              % (rid, n, declared, "" if n == declared else "  <-- DRIFT, not live"))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Fail as e:
        print("FAIL  %s" % e, file=sys.stderr)
        sys.exit(1)
