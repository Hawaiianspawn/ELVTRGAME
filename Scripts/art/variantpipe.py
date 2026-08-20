# variantpipe.py -- the local half of the ELVTR variant-family pipeline.
#
# Companion to pixelpipe.py, same split (see that file's header): deterministic,
# offline work only. PixelLab's generators are MCP tools, not REST endpoints, so
# every generation call is made by Claude via mcp__pixellab__*. This script NEVER
# calls PixelLab -- `plan` only emits the tool+kwargs JSON for the agent to run,
# the same pattern pixelpipe.py's cmd_prompt uses at its line ~529.
#
# Codifies .claude/skills/variants/SKILL.md's hand-driven loop (steps 4-8) into
# four subcommands over one JSON spec per family. All measurement is
# silhouette_report.py's -- this file adds judging (§Findings 3 of
# docs/backlog/task-081-codify-variant-family-pipeline.md) and provenance on top,
# it does not reimplement measure()/variants()/bands().
#
# Usage:
#   py Scripts/art/variantpipe.py plan   <family>
#   py Scripts/art/variantpipe.py fetch  <family> <variant> --url URL [--url URL ...]
#   py Scripts/art/variantpipe.py judge  <family> [--json] [--cull]
#   py Scripts/art/variantpipe.py refine <family> [--dry-run]
#   py Scripts/art/variantpipe.py report <family> [--out sheet.html]
#
# Layout it owns:
#   docs/data/art/families/<family>/family.json     the spec: base, axis, constant, variants
#   docs/data/art/families/<family>/manifest.json   provenance + verdicts, written by judge/fetch
#   RawArt/Renders/<family>/raw/<variant>/[rotations/]*.png   downloads (never deleted -- retention rule)
#   RawArt/Renders/<family>/rejected/<variant>/               culled variants, MOVED not deleted

import argparse
import io
import json
import shutil
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

sys.path.insert(0, str(Path(__file__).resolve().parent))
import silhouette_report as sr  # noqa: E402  (path insert must come first)

REPO = Path(__file__).resolve().parents[2]
FAMILIES = REPO / "docs" / "data" / "art" / "families"
RENDERS = REPO / "RawArt" / "Renders"
UA = "Mozilla/5.0 (compatible; ELVTR-variantpipe/1.0)"  # CDN 403s urllib's default UA


class Fail(Exception):
    """A user-facing failure. Printed without a traceback."""


def now():
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def load_json(path):
    if not path.exists():
        raise Fail("missing file: %s" % path)
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


def save_json(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(obj, fh, indent=2)
        fh.write("\n")


def spec_path(family):
    return FAMILIES / family / "family.json"


def manifest_path(family):
    return FAMILIES / family / "manifest.json"


def load_spec(family):
    p = spec_path(family)
    if not p.exists():
        raise Fail("no spec at %s -- write docs/data/art/families/%s/family.json "
                   "(see docs/data/art/family.schema.json)" % (p, family))
    return load_json(p)


def load_manifest(family):
    p = manifest_path(family)
    if p.exists():
        return load_json(p)
    return {"family": family, "variants": {}}


def raw_root(family):
    return RENDERS / family / "raw"


# ---------------------------------------------------------------- plan

def plan_kwargs(base, v):
    """The kwargs for ONE create_character_state call, in the tool's real signature.

    That signature is exactly character_id, edit_description, seed and
    use_color_palette_from_reference -- there is no `size`. A state inherits its
    canvas from the source character, so the thing that guarantees the canvas is
    confirming the base with get_character, not a kwarg. This emitted `size` and
    `state_description` until task-082's run hit it and hand-translated all six
    calls; test_variantpipe.py pins the signature so it cannot drift back.
    """
    kwargs = {
        "edit_description": v["edit_description"],
        "use_color_palette_from_reference": True,
    }
    if base.get("character_id"):
        kwargs["character_id"] = base["character_id"]
    if v.get("seed") is not None:
        kwargs["seed"] = v["seed"]
    return kwargs


def cmd_plan(args):
    spec = load_spec(args.family)
    base = spec.get("base") or {}
    root = raw_root(args.family)
    on_disk = {name for name, _ in sr.variant_dirs(root)} if root.exists() else set()

    todo = [v for v in spec["variants"] if v["slug"] not in on_disk]
    if not todo:
        print("all %d variant(s) in '%s' already have renders on disk under %s -- "
              "nothing to plan" % (len(spec["variants"]), args.family, root))
        return 0

    for v in todo:
        if not v.get("edit_description"):
            print("SKIP  '%s' has no edit_description to send -- fill it in "
                  "family.json first" % v["slug"], file=sys.stderr)
            continue
        print(json.dumps({"tool": "mcp__pixellab__create_character_state",
                          "variant": v["slug"], "kwargs": plan_kwargs(base, v)},
                         indent=2))
    return 0


# ---------------------------------------------------------------- fetch

def cmd_fetch(args):
    dest = RENDERS / args.family / "raw" / args.variant / "rotations"
    dest.mkdir(parents=True, exist_ok=True)

    written = []
    for url in args.url:
        # Mirrors pixelpipe.py's cmd_fetch: the result CDN 403s without a UA, and
        # a zip (the only way PixelLab hands back a full rotation set) is unpacked
        # keeping its own subdirectories.
        http_req = Request(url, headers={"User-Agent": UA})
        try:
            with urlopen(http_req, timeout=60) as resp:
                blob = resp.read()
        except HTTPError as e:
            raise Fail("download failed (%s %s) for %s -- result URLs expire, "
                       "re-run get_character for fresh ones" % (e.code, e.reason, url))
        except URLError as e:
            raise Fail("download failed (%s) for %s" % (e.reason, url))

        if blob[:4] == b"PK\x03\x04":
            with zipfile.ZipFile(io.BytesIO(blob)) as zf:
                for name in zf.namelist():
                    if not name.lower().endswith(".png"):
                        continue
                    out = dest / Path(name).name
                    out.write_bytes(zf.read(name))
                    written.append(out.name)
        else:
            name = Path(url.split("?")[0]).name or ("download-%d.png" % len(written))
            if not name.lower().endswith(".png"):
                name += ".png"
            (dest / name).write_bytes(blob)
            written.append(name)

    man = load_manifest(args.family)
    v = man.setdefault("variants", {}).setdefault(args.variant, {})
    v["urls"] = args.url
    v["fetched"] = now()
    v["files"] = sorted(set(v.get("files", []) + written))
    save_json(manifest_path(args.family), man)

    print("fetched %d file(s) into %s" % (len(written), dest))
    for w in sorted(set(written)):
        print("  %s" % w)
    return 0


# ---------------------------------------------------------------- judge

# Guesses, not measured -- see the report handed back with this task for why these
# two specific numbers. Nothing else in judge_family is a threshold; everything
# else is either an exact match (opaque count, canvas size) or a containment test.
ASPECT_TARGET_TOLERANCE = 0.35   # how far a numeric silhouette_target may miss by
                                  # before judge flags it as "undershot"
LOW_LUMA = 0.20                   # SKILL.md's own threshold, informational only

# The style-register rule, and the one threshold here that IS measured. A sprite
# carrying hundreds of distinct colours is a smooth render, not pixel art -- the
# "clean" failure. Swept all 123 variants on disk 2026-08-15: every derived state
# lands 10-129 distinct colours, and the four raw PixelLab base characters
# (p0_base, t0_base, type0_base, Merle_Pathfinder) land 871-881. Nothing sits
# between 129 and 871, so the split is real and 256 sits in the empty middle --
# also the natural line, being where an indexed palette stops.
#
# NOT a palette check. demichrome-4 stopped being the enforced default on
# 2026-07-28 (docs/art/aesthetic-direction.md AMENDMENT) and full colour has no
# written standard yet, so 'off the ramp' is not a rule any longer. Colour COUNT
# still is: the chibi gameplay register the amendment explicitly preserves cannot
# survive a continuous-tone render at 48px.
MAX_DISTINCT_COLOURS = 256

# The chibi half of the style rule is deliberately absent. A top-third/middle-third
# row-width ratio was built and cut the same day: measured 0.36-1.53 across the
# same 123 variants with no bimodal split, and its top end is t5_overhead (1.53)
# and p5_invtriangle (1.39) -- raised-arm POSES, not large heads. It measured pose
# and would have rejected two canon-approved topology states. There is also no
# non-chibi sprite on disk to calibrate a threshold against. Generating one
# deliberately non-chibi state as a negative control is what unblocks it; that
# costs credits, so it is an owner call, not an assumption.


# silhouette_report.check_rotations' own WATCH line: a variant drifting more than
# this in aspect across its rotations is an asymmetric/directional form (SKILL.md's
# "long low ridge", 0.59-2.60 across rotations), not a form that should ever be
# used as the *reference* a narrower sibling gets judged "redundant" against --
# its huge natural range would otherwise numerically swallow every tighter,
# genuinely distinct variant in the family. Measured on brood-ooze: without this
# guard, state05_ridge's 2.01 drift band contained six unrelated siblings
# (state01_sump, state02_bell, state04_wedge, state06_crown...) that plainly read
# as different shapes on the contact sheet.
DIRECTIONAL_DRIFT = 0.45


def _mentions(obj, needle_path, slug):
    """True if any string value in a parsed JSON doc names this variant --
    either its raw path (a "source" field) or its bare slug in a "state" field.
    """
    if isinstance(obj, dict):
        if obj.get("state") == slug:
            return True
        return any(_mentions(v, needle_path, slug) for v in obj.values())
    if isinstance(obj, list):
        return any(_mentions(v, needle_path, slug) for v in obj)
    if isinstance(obj, str):
        return needle_path in obj.replace("\\", "/")
    return False


def _external_reference(family, slug):
    """The docs/data/art/*.json path that names this variant, if any -- not the
    family's own spec/manifest, which name every slug as a matter of course.
    One grep, not a consumer registry: a variant nothing else names is free to
    cull; a variant something else's document of record points at is not.
    """
    own = {spec_path(family), manifest_path(family)}
    needle = "%s/raw/%s" % (family, slug)
    for p in (REPO / "docs" / "data" / "art").rglob("*.json"):
        if p in own:
            continue
        try:
            obj = json.loads(p.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
        if _mentions(obj, needle, slug):
            return str(p.relative_to(REPO)).replace("\\", "/")
    return None


def _band_inside(a, b, eps=0.02):
    """True if variant band `a` sits fully inside sibling band `b`, every axis."""
    if a.get("missing") or b.get("missing"):
        return False
    if b["aspect"][1] - b["aspect"][0] > DIRECTIONAL_DRIFT:
        return False
    for k in ("aspect", "solidity", "asymmetry"):
        if not (b[k][0] - eps <= a[k][0] and a[k][1] <= b[k][1] + eps):
            return False
    return set(a["holes"]) <= set(b["holes"])


def judge_family(family):
    """(rows, verdicts, summary) -- rows are [(name, south_measure_dict)], for
    sheet(); verdicts is {name: {"verdict": ..., "reasons": [...]}}. Pure function
    of what's on disk plus the family's spec -- cmd_judge and cmd_report both call
    this instead of one reading the other's output, so `report` never goes stale
    against a manifest nobody re-ran judge on.
    """
    root = raw_root(family)
    if not root.exists():
        raise Fail("no renders on disk for '%s': %s does not exist" % (family, root))

    spec = None
    try:
        spec = load_spec(family)
    except Fail:
        pass
    spec_by_slug = {v["slug"]: v for v in (spec or {}).get("variants", [])}
    canvas = (spec or {}).get("base", {}).get("canvas_size")

    per_direction = sr.measure_all_directions(root)
    bands = sr.bands(per_direction)
    rows = [(name, per["south"]) for name, per in per_direction.items() if "south" in per]
    if not rows:
        raise Fail("no south.png found for any variant under %s" % root)

    # Rule (c): identical opaque count at south is an automatic fail (the check
    # silhouette_report's print_table already makes). Every family here names its
    # deliberately-tracked states with a digit (state00, t0, p0, v0, type0); a
    # PixelLab auto-name (Black_ooze_with_teet, Baby_face, ...) never has one. When
    # a tied group has exactly one tracked name, it is the keeper -- measured on
    # brood-ooze: {Black_ooze_with_teet, Black_ooze_with_teet_copy, state00_base}
    # are pixel-identical, and state00_base is the one the numbered batch actually
    # builds on. An alphabetical tie-break would have discarded it and kept an
    # arbitrary auto-name instead. Falls back to alphabetical when no name in the
    # group is tracked, or more than one is.
    by_opaque = {}
    for name, m in rows:
        by_opaque.setdefault(m["opaque"], []).append(name)
    dup_reject = {}
    for group in by_opaque.values():
        if len(group) <= 1:
            continue
        tracked = [n for n in group if any(c.isdigit() for c in n)]
        pool = tracked if len(tracked) == 1 else group
        keeper = sorted(pool)[0]
        for n in sorted(group):
            if n != keeper:
                dup_reject[n] = keeper

    verdicts = {}
    names = [n for n, _ in rows]
    for name, m in rows:
        reasons = []
        verdict = "keep"
        b = bands.get(name, {})

        if b.get("missing"):
            verdict = "reject"
            reasons.append("missing %d of %d rotations" % (b["missing"], len(sr.DIRECTIONS)))
        elif canvas and tuple(m["canvas"]) != (canvas, canvas):
            verdict = "reject"
            reasons.append("canvas %dx%d does not match family canvas %dpx"
                           % (m["canvas"][0], m["canvas"][1], canvas))
        elif name in dup_reject:
            verdict = "reject"
            reasons.append("identical opaque count (%d px) to '%s' -- same outline, "
                           "differs on the interior only" % (m["opaque"], dup_reject[name]))
        else:
            for other in names:
                if other == name or bands.get(other, {}).get("missing"):
                    continue
                if other in dup_reject:
                    continue  # don't cite a variant that is itself being rejected
                if _band_inside(b, bands[other]) and not _band_inside(bands[other], b):
                    verdict = "reject"
                    reasons.append("band sits fully inside '%s' on every axis "
                                   "(aspect/solidity/asymmetry/holes) across all "
                                   "8 rotations" % other)
                    break
                if (_band_inside(b, bands[other]) and _band_inside(bands[other], b)
                        and other < name):
                    verdict = "reject"
                    reasons.append("band is identical to '%s' on every axis across "
                                   "all 8 rotations" % other)
                    break

        if verdict != "reject":
            target = spec_by_slug.get(name, {}).get("silhouette_target")
            if isinstance(target, (int, float)) and not b.get("missing"):
                # Check the target against the whole 8-direction band, not the
                # south value alone -- a directional form (SKILL.md's "long low
                # ridge") is correctly foreshortened from the south and only
                # hits its target from east/west. Judging south-only here would
                # flag exactly the variant task-081 warns must not be flagged.
                lo, hi = b["aspect"][0] - ASPECT_TARGET_TOLERANCE, b["aspect"][1] + ASPECT_TARGET_TOLERANCE
                if not (lo <= target <= hi):
                    verdict = "flag"
                    reasons.append("aspect target %.2f falls outside the measured "
                                   "%.2f-%.2f band across all 8 rotations"
                                   % (target, b["aspect"][0], b["aspect"][1]))
            # Applies to every variant, generated or legacy, and the legacy half is
            # the point. A state generated by create_character_state inherits its
            # base's palette and cannot exceed it -- all 48 generated variants on
            # disk peak at 47 distinct colours, so gating this on source made the
            # rule inert. The "clean" failure arrives on FRESH characters, and the
            # six that trip this are exactly those: raw create_character output at
            # 871-881 colours, each one a family's own base. All six carry a null
            # edit_description, so refine escalates them and never culls -- a base
            # is the owner's call, not something the loop regenerates underneath
            # the family built on it.
            if m["distinct"] > MAX_DISTINCT_COLOURS:
                verdict = "flag"
                reasons.append("%d distinct colours, over the %d ceiling -- reads as a "
                               "smooth render rather than pixel art, which is the "
                               "'clean' failure the chibi gameplay register cannot "
                               "carry at 48px"
                               % (m["distinct"], MAX_DISTINCT_COLOURS))
            if m["luma_mean"] < LOW_LUMA:
                reasons.append("luma %.3f -- not automatically wrong (SKILL.md), but the "
                               "outline is doing all the work" % m["luma_mean"])

        # A variant something under docs/data/art/ names as a source is never
        # cullable, whatever the measurement says -- state00_base's band is
        # genuinely inside pale_blue_skin_big_e's, but brood-variants.json (task-059,
        # live) consumes it at index 0 / weight 14, so the mechanical reject would
        # have culled a shipping asset out from under its own document of record.
        # Downgrades reject/flag to keep rather than just skipping the cull, so the
        # verdict itself stays honest (mechanically redundant, externally consumed)
        # and every other reader of judge's output sees the real reason too.
        if verdict in ("reject", "flag"):
            ref = _external_reference(family, name)
            if ref:
                reasons = ["externally referenced by %s -- keep despite: %s"
                          % (ref, "; ".join(reasons))]
                verdict = "keep"

        verdicts[name] = {"verdict": verdict, "reasons": reasons, "band": b}

    return rows, verdicts, spec


def cmd_judge(args):
    rows, verdicts, spec = judge_family(args.family)
    sr.print_table(rows)

    print("\nverdicts:")
    for name, _ in rows:
        v = verdicts[name]
        print("  %-26s %-6s %s" % (name, v["verdict"], "; ".join(v["reasons"]) or "-"))

    counts = {}
    for v in verdicts.values():
        counts[v["verdict"]] = counts.get(v["verdict"], 0) + 1
    print("\n%d variant(s): %s" % (len(rows), ", ".join(
        "%d %s" % (n, k) for k, n in sorted(counts.items()))))

    man = load_manifest(args.family)
    man["family"] = args.family
    man["judged"] = now()
    for name, m in rows:
        v = man.setdefault("variants", {}).setdefault(name, {})
        v["measured"] = sr.to_jsonable(m)
        v["band"] = verdicts[name]["band"]
        v["verdict"] = verdicts[name]["verdict"]
        v["reasons"] = verdicts[name]["reasons"]
        if v["verdict"] != "reject":
            # Stale from an earlier run's --cull (e.g. state00_base: culled, then
            # restored once the external-reference rule existed). A variant that
            # is back on disk and no longer verdict=reject must not still claim
            # to have been moved.
            v.pop("culled_to", None)
            v.pop("culled_at", None)

    cull_log = []
    if args.cull:
        for name, _ in rows:
            if verdicts[name]["verdict"] != "reject":
                continue
            dest = _cull(args.family, name)
            man["variants"][name]["culled_to"] = str(dest.relative_to(REPO)).replace("\\", "/")
            man["variants"][name]["culled_at"] = now()
            cull_log.append((name, dest))
        if cull_log:
            print("\nculled:")
            for name, dest in cull_log:
                print("  %s -> %s" % (name, dest.relative_to(REPO)))
        else:
            print("\nculled: nothing -- no variant judged 'reject' this run")

    save_json(manifest_path(args.family), man)

    if args.json:
        print(json.dumps({
            "family": args.family,
            "variants": {n: {"measured": sr.to_jsonable(m), **verdicts[n]}
                        for n, m in rows},
        }, indent=2))
    return 0


def _cull(family, slug):
    """Move a rejected variant's files to rejected/<slug>/. Never deletes -- the
    standing PixelLab retention rule.

    Moves the variant's OWN folder (root/<slug>) whole, preserving whatever is
    inside it (a rotations/ subfolder, direct PNGs, whatever) -- not
    variant_dirs()'s resolved leaf directory, which for the nested layout is
    root/<slug>/rotations and would strand an empty root/<slug> behind while
    flattening the rotations/ nesting on the far side. Handles the one layout
    quirk where the variant IS the family's raw/ root itself (archer-proxy's
    flat "base" state, no dedicated subfolder of its own) by moving only its
    loose files, not siblings' folders.

    A slug can be culled more than once -- refine culls on every attempt, so the
    second attempt at the same variant finds rejected/<slug>/ already there. The
    numbered suffix is what keeps that from becoming rejected/<slug>/<slug>:
    shutil.move onto an existing directory moves INTO it, so attempt 2 nested
    silently and attempt 3 raised "Destination path already exists". Each cull
    gets its own folder, and every earlier attempt stays readable beside it --
    which is the retention rule doing what it is for.
    """
    root = raw_root(family)
    if slug not in dict(sr.variant_dirs(root)):
        raise Fail("'%s' is not a variant of '%s'" % (slug, family))
    rejected = RENDERS / family / "rejected"
    dest = rejected / slug
    n = 2
    while dest.exists():
        dest = rejected / ("%s-a%d" % (slug, n))
        n += 1
    src = root / slug
    if src.is_dir():
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dest))
    else:
        dest.mkdir(parents=True, exist_ok=True)
        for p in list(root.iterdir()):
            if p.is_file():
                shutil.move(str(p), str(dest / p.name))
    return dest


# ---------------------------------------------------------------- refine

# The loop gets three shots at a variant before it stops being the loop's problem.
# Not measured -- there is no run on disk where a fourth reword rescued a variant
# three rewords could not, and burning PixelLab credits to find out is real money
# (SKILL.md's `pixellab-credits` is a hard mutex for that reason).
MAX_ATTEMPTS = 3

# Failure kinds no reword can move: a wrong canvas and a short rotation set are
# both generation-job problems, not description problems. Escalate on first
# sight rather than spending an attempt proving it.
UNPROMPTABLE = ("canvas ", "missing ")


# Appended to every directive that asks for a shape change. Measured need, not
# caution: three attempts at v5_widecross were told to "extend the carried gear
# further from the body" and the generator read that as ADD GEAR -- attempt 1 grew
# a second sword and dropped the shield, attempt 2 carried two swords, a shield and
# a horn on its back. None of it was catchable here. A second sword and a missing
# shield cancelled to +25 opaque px, well inside normal variation, and a wider
# silhouette is what the aspect rule REWARDS, so the evaluator was pushing toward
# the defect. Silhouette measurement cannot see kit drift; the prompt is the only
# place this can be held.
KIT_LOCK = (" Do not add, remove or duplicate any equipment: the character carries "
            "exactly the same weapons, shield and kit as the source, in the same "
            "quantity. Change only the pose and how the existing items are held.")


def corrective_clause(reasons, band=None, target=None):
    """The directive appended to a failing variant's edit_description, or None
    when nothing in `reasons` is something a reworded prompt can move.

    Matches on substrings of judge_family's own reason strings. They are written
    twenty lines up in this same file, so the coupling is local and visible --
    the alternative, tagging every reason with a code, edits the one function
    test_variantpipe.py pins.
    """
    for r in reasons:
        if "identical opaque count" in r:
            return ("The previous attempt changed the interior only -- its outline was "
                    "pixel-identical to a sibling. Change the OUTLINE this time: alter "
                    "the pose and limb spread so the filled area itself differs. Do not "
                    "settle for different shading." + KIT_LOCK)
        if "band sits fully inside" in r or "band is identical" in r:
            return ("The previous attempt read as a subset of a sibling from every one "
                    "of the 8 rotations. Push the silhouette much further along this "
                    "family's axis -- clearly narrower or wider, clearly taller or "
                    "squatter -- so the two separate at thumbnail size." + KIT_LOCK)
        if "distinct colours" in r:
            return ("The previous attempt came back as a smooth, continuous-tone render "
                    "-- hundreds of distinct colours, soft gradients, anti-aliased "
                    "edges. Redo it as flat pixel art: a small number of solid colour "
                    "blocks, hard unblended edges, no gradients and no soft shading. "
                    "Match the source character's existing colours exactly.")
        if "aspect target" in r:
            if band and isinstance(target, (int, float)):
                if target < band["aspect"][0]:
                    return ("The previous attempt came out too wide: measured aspect "
                            "%.2f-%.2f across 8 rotations against a %.2f target. Make "
                            "the form NARROWER -- tuck the arms in, hold the existing "
                            "gear closer to the body, tighten the stance."
                            % (band["aspect"][0], band["aspect"][1], target) + KIT_LOCK)
                return ("The previous attempt came out too narrow: measured aspect "
                        "%.2f-%.2f across 8 rotations against a %.2f target. Make the "
                        "form WIDER -- spread the arms and stance further apart, holding "
                        "the existing gear out at arm's length."
                        % (band["aspect"][0], band["aspect"][1], target) + KIT_LOCK)
            return ("The previous attempt missed its stated silhouette target. Push the "
                    "form further toward that target shape." + KIT_LOCK)
    return None


def cmd_refine(args):
    rows, verdicts, spec = judge_family(args.family)
    if spec is None:
        raise Fail("no family.json for '%s' -- refine amends the recorded "
                   "edit_description, so it needs the spec, not just renders on disk"
                   % args.family)

    spec_by_slug = {v["slug"]: v for v in spec.get("variants", [])}
    man = load_manifest(args.family)
    base = spec.get("base") or {}
    emitted, escalated = [], []

    for name, _ in rows:
        v = verdicts[name]
        if v["verdict"] == "keep":
            continue

        entry = man.setdefault("variants", {}).setdefault(name, {})
        ref = entry.setdefault("refine", {"attempts": 0, "history": []})
        sv = spec_by_slug.get(name)

        blocked = None
        if sv is None:
            blocked = "not in family.json -- no recorded prompt to amend"
        elif not sv.get("edit_description"):
            blocked = "edit_description is null (legacy render) -- nothing to reword"
        elif any(r.startswith(UNPROMPTABLE) for r in v["reasons"]):
            blocked = "not a prompt problem: %s" % v["reasons"][0]
        elif ref["attempts"] >= MAX_ATTEMPTS:
            blocked = ("%d attempts spent and still %s -- the loop cannot self-correct "
                       "this one" % (ref["attempts"], v["verdict"]))

        clause = None
        if blocked is None:
            clause = corrective_clause(v["reasons"], v.get("band"),
                                       sv.get("silhouette_target"))
            if clause is None:
                blocked = "no corrective directive for: %s" % "; ".join(v["reasons"])

        if blocked:
            escalated.append((name, v["verdict"], blocked))
            continue

        # Rebuild from the ORIGINAL every attempt. Appending to the already-amended
        # text stacks three contradictory directives by attempt 3 and the last run
        # of the loop sends the worst prompt of the three.
        original = ref.get("original") or sv["edit_description"]
        ref["original"] = original
        amended = "%s %s" % (original.rstrip(), clause)
        attempt_no = ref["attempts"] + 1

        if not args.dry_run:
            ref["history"].append({"attempt": attempt_no, "at": now(),
                                   "verdict": v["verdict"], "reasons": v["reasons"],
                                   "sent": amended})
            ref["attempts"] = attempt_no
            sv["edit_description"] = amended
            # The old render has to leave raw/ or `plan` skips the slug as already
            # generated. _cull moves, never deletes (retention rule), and
            # judge_family's external-reference override already downgraded any
            # consumed asset to keep, so nothing shipping can reach this line.
            dest = _cull(args.family, name)
            entry["culled_to"] = str(dest.relative_to(REPO)).replace("\\", "/")
            entry["culled_at"] = now()

        emitted.append((name, attempt_no, v["verdict"], amended))

    if not args.dry_run and (emitted or escalated):
        save_json(manifest_path(args.family), man)
        if emitted:
            save_json(spec_path(args.family), spec)

    if escalated:
        print("ESCALATED -- owner call, the loop is done with these:", file=sys.stderr)
        for name, verdict, why in escalated:
            print("  %-26s %-6s %s" % (name, verdict, why), file=sys.stderr)
        print("", file=sys.stderr)

    if not emitted:
        print("nothing to refine in '%s' -- %d variant(s) judged, %d escalated"
              % (args.family, len(rows), len(escalated)))
        return 2 if escalated else 0

    print("refined %d variant(s)%s. Run these, then `fetch` and `judge` again:"
          % (len(emitted), " (DRY RUN -- nothing written)" if args.dry_run else ""))
    for name, attempts, verdict, amended in emitted:
        # Name the cull in the dry run too -- it is the one destructive thing a
        # real run does, and a `flag` reaching this list means a variant judge
        # decided to KEEP is about to be moved aside and regenerated.
        print("\n# %s -- was %s, attempt %d of %d, old render moves to "
              "RawArt/Renders/%s/rejected/%s/"
              % (name, verdict, attempts, MAX_ATTEMPTS, args.family, name))
        print(json.dumps({"tool": "mcp__pixellab__create_character_state",
                          "variant": name,
                          "kwargs": plan_kwargs(base, {"slug": name,
                                                       "edit_description": amended,
                                                       "seed": spec_by_slug[name].get("seed")})},
                         indent=2))
    return 2 if escalated else 0


# ---------------------------------------------------------------- report

def cmd_report(args):
    rows, verdicts, spec = judge_family(args.family)
    rows = sorted(rows, key=lambda kv: kv[0])
    for name, m in rows:
        m["verdict"] = verdicts[name]["verdict"]
        m["reasons"] = verdicts[name]["reasons"]

    axis = (spec or {}).get("axis", {}).get("description", "")
    out = args.out or str(FAMILIES / args.family / "report.html")
    Path(out).write_text(
        sr.sheet(rows, args.title or ("%s -- variant family" % args.family),
                args.scale, axis),
        encoding="utf-8")
    print("wrote %s" % out)
    return 0


# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("plan", help="emit mcp__pixellab__* tool+kwargs for ungenerated variants")
    p.add_argument("family")
    p.set_defaults(func=cmd_plan)

    p = sub.add_parser("fetch", help="download a variant's rotations")
    p.add_argument("family")
    p.add_argument("variant")
    p.add_argument("--url", action="append", required=True)
    p.set_defaults(func=cmd_fetch)

    p = sub.add_parser("judge", help="measure every variant and apply the reject/flag/keep rules")
    p.add_argument("family")
    p.add_argument("--json", action="store_true")
    p.add_argument("--cull", action="store_true",
                   help="move rejected variants to rejected/ (default: report only)")
    p.set_defaults(func=cmd_judge)

    p = sub.add_parser("refine", help="reword every failing variant's prompt and re-emit it; "
                                      "escalate what the loop cannot fix")
    p.add_argument("family")
    p.add_argument("--dry-run", action="store_true",
                   help="print the amended prompts without writing family.json or culling")
    p.set_defaults(func=cmd_refine)

    p = sub.add_parser("report", help="publish the contact sheet, verdicts included")
    p.add_argument("family")
    p.add_argument("--out")
    p.add_argument("--title")
    p.add_argument("--scale", type=int, default=3)
    p.set_defaults(func=cmd_report)

    args = ap.parse_args()
    try:
        return args.func(args)
    except Fail as e:
        print("FAIL  %s" % e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
