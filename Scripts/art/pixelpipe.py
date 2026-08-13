# pixelpipe.py -- the local half of the ELVTR sprite pipeline.
#
# Everything here is deterministic, offline work: validate a request, compose a
# prompt, fetch already-generated assets, enforce whichever palette ramp a request
# opts into (demichrome-4 remains available; nothing is assumed by default -- see
# docs/art/aesthetic-direction.md's 2026-07-28 AMENDMENT and request_palette() below),
# pack a SubUV sheet, report QC. It NEVER calls the PixelLab API -- PixelLab's own docs
# say these are MCP tools, not REST endpoints, so all generation calls are made
# by Claude via mcp__pixellab__*. This script only touches files and the public,
# auth-free download URLs those tools hand back.
#
# Usage:
#     py Scripts/art/pixelpipe.py validate  <id>
#     py Scripts/art/pixelpipe.py prompt    <id> [--stage anchor|rotation]
#     py Scripts/art/pixelpipe.py fetch     <id> --stage anchor --url URL [--url URL ...]
#     py Scripts/art/pixelpipe.py quantize  <id> --stage anchor|rotation|anim
#     py Scripts/art/pixelpipe.py quantize  --in DIR --out DIR [--moving]      (standalone)
#     py Scripts/art/pixelpipe.py pack      <id>
#     py Scripts/art/pixelpipe.py report    <id>
#     py Scripts/art/pixelpipe.py manifest  <id> --stage anchor --set character_id=UUID
#
# Layout it owns:
#     docs/data/art/requests/<id>.json      the request (hand-authored, committed)
#     RawArt/Renders/<id>/r<rev>/raw/       downloads, NEVER modified (retention rule)
#     RawArt/Renders/<id>/r<rev>/quantized/ the enforced 4-value output
#     RawArt/Renders/<id>/r<rev>/manifest.json   provenance: UUIDs, urls, gens spent
#     RawArt/Renders/<id>/r<rev>/report.json     QC results
#     RawArt/Sheets/<texture>.png           the packed SubUV sheet

import argparse
import io
import json
import os
import re
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
DATA = REPO / "docs" / "data" / "art"
REQUESTS = DATA / "requests"
PALETTE_FILE = DATA / "palette.json"
SCHEMA_FILE = DATA / "sprite-request.schema.json"
RENDERS = REPO / "RawArt" / "Renders"
SHEETS = REPO / "RawArt" / "Sheets"

DIRECTIONS = ["south", "south-west", "west", "north-west",
              "north", "north-east", "east", "south-east"]

# Longest first when matching filenames: "north-east" must be tested before "north",
# or every compound direction collapses onto its cardinal prefix.
DIRECTIONS_BY_LENGTH = sorted(DIRECTIONS, key=len, reverse=True)

HOUSE_AVOID = ["saturated colour", "colour gradients", "anti-aliasing",
               "soft shading", "drop shadows", "blur"]

# prompt.style_mode == "depth": a render-treatment experiment. Asking for
# "detailed shading" while the prompt says "flat unlit ... no soft shading" measures
# nothing but the contradiction, so the depth set lifts the three negatives that
# fight a shading param. "saturated colour" and "blur" stay -- the palette is locked
# either way and blur destroys hard pixel edges at any depth. Never the default.
HOUSE_AVOID_DEPTH = ["saturated colour", "drop shadows", "blur"]

# The result CDN rejects urllib's default User-Agent with a 403.
UA = "Mozilla/5.0 (compatible; ELVTR-pixelpipe/1.0)"


# ---------------------------------------------------------------- infrastructure

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


def load_request(request_id):
    return load_json(REQUESTS / ("%s.json" % request_id))


def rev_dir(req):
    return RENDERS / req["id"] / ("r%d" % req["revision"])


def manifest_path(req):
    return rev_dir(req) / "manifest.json"


def is_composite(req):
    """True for a request that GENERATES nothing and only packs other requests' frames.

    A composite has no prompt/pixellab/anchor/canon of its own, so every code path
    that reaches for those has to check this first.
    """
    return bool(req.get("composite"))


def require_subject_request(req, verb):
    """Reject a composite from the stages that only make sense for a real subject.

    A composite has no prompt, pixellab, anchor, canon or budget, so these would each
    die on a KeyError several lines in. Failing here says why instead.
    """
    if is_composite(req):
        raise Fail("'%s' does not apply to composite request '%s' -- it generates "
                   "nothing. Composites only support validate, pack and report; run "
                   "'%s' against one of its sources instead: %s"
                   % (verb, req["id"], verb,
                      ", ".join(s.get("request") or ("(placeholder '%s')" % s["prefix"])
                                for s in req["composite"]["sources"])))


def load_manifest(req):
    path = manifest_path(req)
    if path.exists():
        return load_json(path)
    man = {
        "id": req["id"],
        "revision": req["revision"],
        "created": now(),
        "stages": {},
        "generations_spent": 0,
    }
    # A composite spends nothing and has no prompt to snapshot; what identifies it is
    # the set of sources it drew from.
    if is_composite(req):
        man["request_snapshot"] = {"composite": req["composite"]}
    else:
        man["request_snapshot"] = {"prompt": req["prompt"], "pixellab": req["pixellab"]}
    return man


# ---------------------------------------------------------------- palette

class Palette:
    def __init__(self, key):
        # No default key. Silently falling back to demichrome-4 is exactly the
        # "enforce a retired rule by default" bug task-062 exists to remove -- see
        # request_palette() below, which is how every request-driven call site
        # resolves a key. A caller that truly wants demichrome-4 says so.
        raw = load_json(PALETTE_FILE)
        if key not in raw["palettes"]:
            raise Fail("palette '%s' is not defined in %s" % (key, PALETTE_FILE))
        p = raw["palettes"][key]
        self.key = key
        self.values = p["values"]
        self.rgb = np.array([v["rgb"] for v in self.values], dtype=np.float32)
        self.luma = np.array([v["luma"] for v in self.values], dtype=np.float32)
        self.keys = [v["key"] for v in self.values]
        self.hexes = [v["hex"].lower() for v in self.values]
        self.alpha_threshold = p["mask"]["threshold"]
        self.light_shift = p["light_shift"]["map"]
        self.dim_shift = p.get("dim_shift", {}).get("map", {})
        self.legend = p.get("ascii_legend", {}).get("map", {})
        self.dither = p["dither"]
        w = raw["luma_model"]["weights"]
        self.weights = np.array([w["r"], w["g"], w["b"]], dtype=np.float32)
        self.retired = {e["hex"].lower(): e for e in raw["retired_hexes"]["entries"]}

    def luma_of(self, rgb):
        """rgb: (...,3) float 0-255 -> luma 0-1, Rec.709 in gamma space."""
        return (rgb * self.weights).sum(axis=-1) / 255.0


def load_retired_hexes():
    """The retired-hex list, independent of which palette (if any) a request names.

    retired_hexes lives at the top of palette.json, not inside any one palette entry --
    it is voided-forever history, not a property of demichrome-4 specifically. Callers
    that need to check for retired hexes (cmd_validate) but may not have a resolved
    Palette (canon.palette missing) use this instead of pal.retired.
    """
    raw = load_json(PALETTE_FILE)
    return {e["hex"].lower(): e for e in raw["retired_hexes"]["entries"]}


def request_palette(req):
    """Resolve the palette a request's canon block names. No silent default.

    sprite-request.schema.json marks canon.palette required, so a schema-valid
    request always names one -- this only trips for a malformed/unvalidated request,
    and it should say so rather than quietly quantizing to demichrome-4, which is
    exactly the "enforce a retired rule by default" bug task-062 exists to remove
    (docs/art/aesthetic-direction.md AMENDMENT 2026-07-28). Anything that actually
    wants demichrome-4 still gets it -- it just has to say so, same as it always did
    in every request file that exists today.
    """
    canon = req.get("canon") or {}
    key = canon.get("palette")
    if not key:
        raise Fail("request '%s' has no canon.palette set -- palette enforcement is "
                   "opt-in, not a silent default (see docs/art/aesthetic-direction.md "
                   "AMENDMENT 2026-07-28). Name a palette key explicitly in the request, "
                   "e.g. \"palette\": \"demichrome-4\", to opt into that ramp." % req.get("id", "?"))
    return Palette(key)


# ---------------------------------------------------------------- validate

def hexes_in(text):
    return set(m.lower() for m in re.findall(r"#[0-9a-fA-F]{6}", text))


def cmd_validate(args):
    req = load_request(args.id)
    errors, warnings = [], []

    # -- structural: JSON Schema
    try:
        import jsonschema
        schema = load_json(SCHEMA_FILE)
        v = jsonschema.Draft202012Validator(schema)
        for e in sorted(v.iter_errors(req), key=lambda e: list(e.path)):
            loc = "/".join(str(p) for p in e.path) or "(root)"
            errors.append("schema: %s: %s" % (loc, e.message))
    except ImportError:
        warnings.append("jsonschema not installed -- structural validation skipped "
                        "(canon checks below still ran). pip install jsonschema")

    if req.get("id") != args.id:
        errors.append("id field '%s' does not match filename '%s.json'"
                      % (req.get("id"), args.id))

    # No silent demichrome-4 fallback -- see request_palette()'s docstring for why.
    # Schema requires canon.palette, so absence here means either a malformed request
    # (already caught above as a schema error) or an unrecognised key (a Fail from
    # Palette() itself, folded into errors rather than crashing validate outright so
    # the rest of the report still prints).
    canon_palette_key = req.get("canon", {}).get("palette")
    pal = None
    if not canon_palette_key:
        warnings.append("canon.palette is not set -- the composed-prompt-length and "
                        "on-palette-hex checks below are skipped. Schema validation "
                        "above should already flag this as a required field.")
    else:
        try:
            pal = Palette(canon_palette_key)
        except Fail as e:
            errors.append(str(e))

    # Retired hexes are voided game-wide, independent of which palette (if any) this
    # request names -- check them regardless of whether pal resolved.
    retired = load_retired_hexes()

    # -- structural: the COMPOSED prompt must fit PixelLab's own cap.
    # prompt.description is capped at 600 by the schema, but the composer then appends
    # must_include, reads_as, the ramp clause and every must_avoid entry, and the API
    # rejects the result over 2000 chars. Measured 2026-07-25: soldier-01/06 passed
    # validate at 582-char descriptions and were refused at 2413/2458 composed. Without
    # this check the only way to find out is a failed call.
    if pal is not None and not req.get("composite"):
        try:
            for stage in ("anchor", "rotation"):
                n = len(compose_prompt(req, pal, stage))
                if n > 2000:
                    errors.append("composed %s prompt is %d chars; PixelLab's cap is "
                                  "2000. Trim prompt.must_avoid/must_include -- the "
                                  "description is only %d of it"
                                  % (stage, n, len(req["prompt"]["description"])))
                elif n > 1900:
                    warnings.append("composed %s prompt is %d chars, close to "
                                    "PixelLab's 2000 cap" % (stage, n))
        except (KeyError, TypeError):
            pass  # schema errors above already describe a malformed prompt block

    # -- canon: no retired hexes anywhere in the request or its linked spec
    blob = json.dumps(req)
    for h in hexes_in(blob):
        if h in retired:
            e = retired[h]
            errors.append("retired hex %s (%s) appears in the request -- voided by %s"
                          % (h, e["was"], e["retired_by"]))
        elif pal is not None and h not in pal.hexes:
            warnings.append("hex %s in the request is neither a palette value nor a "
                            "known retired hex -- the composer adds palette hexes for "
                            "you, so this is probably a mistake" % h)

    # -- canon: the prose spec must exist, and must not itself cite a retired hex
    spec_rel = req.get("subject", {}).get("spec")
    if spec_rel:
        spec_path = (REQUESTS / spec_rel).resolve()
        if not spec_path.exists():
            errors.append("subject.spec does not resolve to a file: %s" % spec_path)
        else:
            text = spec_path.read_text(encoding="utf-8", errors="replace")
            stale = [h for h in hexes_in(text) if h in retired]
            if stale:
                errors.append(
                    "linked spec %s cites retired hex(es) %s -- respec it before "
                    "generating, or the prompt will carry dead canon"
                    % (spec_path.name, ", ".join(sorted(stale))))

    brief_rel = req.get("subject", {}).get("brief")
    if brief_rel:
        if not (REQUESTS / brief_rel).resolve().exists():
            warnings.append("subject.brief does not resolve: %s" % brief_rel)

    # -- canon: output geometry (the schema cannot express these conditionals)
    out = req.get("output", {})
    kind = req.get("subject", {}).get("kind")
    cell = out.get("cell")
    if kind != "ui" and cell != 48:
        errors.append("output.cell is %s -- locked to 48 for kind '%s' "
                      "(owner decision 2026-07-25; only kind 'ui' may differ)"
                      % (cell, kind))
    grid = out.get("grid") or []
    # COLUMNS must be a power of two. ROWS need not be, and the old rule that said
    # otherwise was folklore: the Niagara Sprite Renderer's Sub UV is a pair of floats and
    # the decode is a ratio, so 20 rows work exactly as well as 16. Measured 2026-07-29 on
    # the 8x20 swarm atlas (task-059) -- see docs/perf/niagara-sprite-path.md. Rounding up
    # to the next power of two would have cost 1.6MB of transparent texels to satisfy a
    # constraint the engine does not have.
    for i, n in enumerate(grid):
        if n < 1:
            errors.append("output.grid[%d] = %s must be at least 1" % (i, n))
        elif i == 0 and (n & (n - 1)) != 0:
            errors.append("output.grid[0] = %s (columns) is not a power of two" % n)
    fm = out.get("frame_map") or {}
    if grid and len(grid) == 2:
        cells = grid[0] * grid[1]
        bad = [k for k in fm if int(k) >= cells]
        if bad:
            errors.append("frame_map cells %s are outside a %dx%d grid (%d cells)"
                          % (sorted(bad), grid[0], grid[1], cells))
        if not fm:
            warnings.append("frame_map is empty -- pack will produce a blank sheet")

    # -- composite: sources must be resolvable, and frame_map must be namespaced
    if is_composite(req):
        prefixes = []
        for i, src in enumerate(req["composite"].get("sources", [])):
            pfx = src.get("prefix")
            prefixes.append(pfx)
            has_req, has_frames = bool(src.get("request")), bool(src.get("frames"))
            if has_req == has_frames:
                errors.append("composite.sources[%d] ('%s') needs exactly one of "
                              "'request' or 'frames', got %s"
                              % (i, pfx, "both" if has_req else "neither"))
            if has_req:
                sub = REQUESTS / ("%s.json" % src["request"])
                if not sub.exists():
                    errors.append("composite.sources[%d] references request '%s' which "
                                  "does not exist: %s" % (i, src["request"], sub))
            for key, rel in (src.get("frames") or {}).items():
                if not (REPO / rel).exists():
                    errors.append("composite.sources[%d] frame '%s' does not resolve: %s"
                                  % (i, key, rel))
            if has_frames:
                warnings.append("composite.sources[%d] ('%s') is UNMANAGED placeholder "
                                "art -- it did not come through this pipeline, and the "
                                "manifest and report will say so. %s"
                                % (i, pfx, src.get("note", "")))
        if len(set(prefixes)) != len(prefixes):
            errors.append("composite.sources have duplicate prefixes: %s" % prefixes)
        for cell_idx, key in (out.get("frame_map") or {}).items():
            if ":" not in key:
                errors.append("frame_map['%s'] = '%s' is not namespaced -- a composite "
                              "needs '<prefix>:<direction>.<state>'" % (cell_idx, key))
            elif key.split(":", 1)[0] not in prefixes:
                errors.append("frame_map['%s'] = '%s' uses prefix '%s', which is not a "
                              "declared source (%s)"
                              % (cell_idx, key, key.split(":", 1)[0], prefixes))

    # -- canon: pale_usage vs the subject's shape carrier
    canon = req.get("canon", {})
    if canon.get("pale_usage") == "none" and kind == "hero":
        warnings.append("canon.pale_usage is 'none' for a hero -- heroes normally own a "
                        "shape carrier for the bright value (see palette.json "
                        "shape_carriers). Confirm this is deliberate.")

    # -- anchor coherence
    anchor = req.get("anchor", {})
    if anchor.get("strategy") == "shared-anchor" and not anchor.get("shared_anchor_id"):
        errors.append("anchor.strategy is 'shared-anchor' but shared_anchor_id is unset")
    if anchor.get("strategy") == "text-only":
        warnings.append("anchor.strategy is 'text-only' -- PixelLab enforces no palette "
                        "and offers no seed, so this route has the weakest style control. "
                        "Use it to explore, not to ship.")

    # -- animations
    for a in req.get("animations", []):
        mode = a.get("mode", "template")
        if mode == "template" and not a.get("template_animation_id"):
            errors.append("animation '%s' is mode=template but has no "
                          "template_animation_id" % a.get("name"))
        if mode in ("v3", "pro") and not a.get("action_description"):
            errors.append("animation '%s' is mode=%s but has no action_description"
                          % (a.get("name"), mode))
        if mode == "v3" and a.get("frame_count", 8) % 2:
            errors.append("animation '%s' frame_count must be even" % a.get("name"))

    # -- budget sanity against the declared plan. A composite generates nothing, so
    # costing it against an absent budget would be a guaranteed false failure.
    if is_composite(req):
        est, budget = 0, 0
    else:
        est = estimate_cost(req)
        budget = req.get("budget", {}).get("max_generations", 0)
        if est > budget:
            errors.append("estimated cost %d generations exceeds budget.max_generations %d"
                          % (est, budget))

    for w in warnings:
        print("WARN  %s" % w)
    for e in errors:
        print("FAIL  %s" % e)
    if errors:
        print("\n%d error(s), %d warning(s) -- %s is NOT ready to generate."
              % (len(errors), len(warnings), args.id))
        return 1
    print("OK    %s validates. Estimated cost %d generations (budget %d)."
          % (args.id, est, budget))
    return 0


def estimate_cost(req):
    """Reference costs from docs/PIXELLAB-MCP.md and the MCP tool schemas."""
    px = req.get("pixellab", {})
    size = px.get("size", 48)
    mode = px.get("mode", "v3")
    per_char = {"standard": 1, "v3": 2 if size <= 48 else 4, "pro": 30}.get(mode, 2)
    total = per_char                                    # the anchor
    if req.get("anchor", {}).get("strategy") != "text-only":
        total += per_char                               # the rotation pass
    for a in req.get("animations", []):
        dirs = len(a.get("directions") or DIRECTIONS)
        m = a.get("mode", "template")
        total += dirs * {"template": 1, "v3": 1, "pro": 30}.get(m, 1)
    p = req.get("portrait", {})
    if p.get("enabled"):
        total += 25 if p.get("result_size", 128) >= 128 else 20
    return total


# ---------------------------------------------------------------- prompt

def compose_prompt(req, pal, stage="anchor"):
    """Deterministic prompt composition.

    create_character accepts no seed, so identical text is the only reproducibility
    we get. Never hand-type a description at call time -- change the request and
    bump its revision instead.
    """
    p = req["prompt"]
    canon = req["canon"]
    parts = [p["description"].strip().rstrip(".")]

    if p.get("must_include"):
        parts.append("must show " + ", ".join(p["must_include"]))

    parts.append("silhouette reads as " + canon["reads_as"].strip().rstrip("."))

    ramp = ", ".join("%s %s" % (v["hex"], v["key"]) for v in pal.values)
    parts.append("strict 4 colour palette only: " + ramp)

    style = p.get("style_mode", "flat")
    if style == "depth":
        parts.append("unlit pixel art, hard pixel edges")
        house = HOUSE_AVOID_DEPTH
    else:
        parts.append("flat unlit pixel art, hard pixel edges, dither for any intermediate tone")
        house = HOUSE_AVOID

    avoid = list(p.get("must_avoid", [])) + house
    seen, uniq = set(), []
    for a in avoid:
        if a.lower() not in seen:
            seen.add(a.lower())
            uniq.append(a)
    parts.append("no " + ", no ".join(uniq))

    return ", ".join(parts) + "."


def cmd_prompt(args):
    req = load_request(args.id)
    require_subject_request(req, "prompt")
    pal = request_palette(req)
    px = dict(req["pixellab"])
    description = compose_prompt(req, pal, args.stage)

    kwargs = {"description": description, "name": req["subject"]["title"]}
    for k in ("mode", "size", "view", "outline", "detail", "body_type",
              "text_guidance_scale", "shading", "n_directions", "template"):
        if k in px and px[k] is not None:
            kwargs[k] = px[k]
    if "proportions" in px:
        kwargs["proportions"] = json.dumps(px["proportions"])

    # v3 ignores several params; drop them so the call reflects what actually applies.
    if kwargs.get("mode") == "v3":
        for k in ("shading", "proportions", "text_guidance_scale", "n_directions"):
            kwargs.pop(k, None)
    if kwargs.get("mode") == "pro":
        for k in ("shading", "proportions", "text_guidance_scale",
                  "n_directions", "outline", "detail"):
            kwargs.pop(k, None)

    if args.stage == "rotation":
        anchor_png = quantized_dir(req, "anchor") / ("%s.png"
                                                     % req["anchor"].get("source_direction", "south"))
        kwargs["mode"] = "v3"
        kwargs["reference_image_base64"] = "<base64 of %s>" % anchor_png
        if not req["anchor"].get("approved"):
            print("WARN  anchor.approved is false -- the stage-D human gate has not "
                  "passed. Everything downstream inherits this anchor.\n", file=sys.stderr)

    print(json.dumps({"tool": "mcp__pixellab__create_character", "kwargs": kwargs},
                     indent=2))
    return 0


# ---------------------------------------------------------------- authored anchor

def find_pixel_blocks(text, legend):
    """Every fenced code block that is a pure pixel map in the spec's legend.

    An art spec's silhouette guide is already a complete, on-palette, correctly
    proportioned drawing of the sprite. Rendering it beats asking a model to
    reinvent it: zero generations, and it cannot come back off-palette or with the
    class's defining prop missing.
    """
    chars = set(legend)
    blocks = []
    for body in re.findall(r"```[a-zA-Z0-9]*\n(.*?)```", text, re.DOTALL):
        lines = [ln.rstrip("\r") for ln in body.split("\n")]
        while lines and not lines[0].strip():
            lines.pop(0)
        while lines and not lines[-1].strip():
            lines.pop()
        if not lines:
            continue
        if any(set(ln) - chars for ln in lines):
            continue
        w = max(len(ln) for ln in lines)
        lines = [ln.ljust(w, ".") for ln in lines]
        blocks.append(lines)
    return blocks


def render_pixel_block(lines, pal):
    h, w = len(lines), len(lines[0])
    out = np.zeros((h, w, 4), dtype=np.uint8)
    counts = {}
    for y, row in enumerate(lines):
        for x, ch in enumerate(row):
            key = pal.legend.get(ch)
            counts[ch] = counts.get(ch, 0) + 1
            if key is None:
                continue
            out[y, x, :3] = pal.rgb[pal.keys.index(key)].astype(np.uint8)
            out[y, x, 3] = 255
    return out, counts


def cmd_authored(args):
    req = load_request(args.id)
    require_subject_request(req, "authored")
    pal = request_palette(req)
    if not pal.legend:
        raise Fail("palette '%s' defines no ascii_legend" % pal.key)

    if args.from_file:
        src = Path(args.from_file)
        text = src.read_text(encoding="utf-8", errors="replace")
    else:
        src = (REQUESTS / req["subject"]["spec"]).resolve()
        text = src.read_text(encoding="utf-8", errors="replace")

    blocks = find_pixel_blocks(text, pal.legend)
    if not blocks:
        raise Fail("no pixel-map code block found in %s. Blocks must contain only "
                   "the legend characters %s"
                   % (src, "".join(sorted(pal.legend))))

    cell = req["output"]["cell"]
    exact = [b for b in blocks if len(b) == cell and len(b[0]) == cell]
    chosen = exact[args.index] if exact else blocks[args.index]
    if not exact:
        print("WARN  no block is exactly %dx%d; using a %dx%d block "
              "(index %d of %d found)"
              % (cell, cell, len(chosen[0]), len(chosen), args.index, len(blocks)))

    arr, counts = render_pixel_block(chosen, pal)
    dest = raw_dir(req, "anchor")
    dest.mkdir(parents=True, exist_ok=True)
    direction = req["anchor"].get("source_direction", "south")
    target = dest / ("%s.png" % direction)
    Image.fromarray(arr, "RGBA").save(target)

    opaque = int((arr[..., 3] > 0).sum())
    print("rendered %dx%d pixel map from %s -> %s"
          % (len(chosen[0]), len(chosen), src.name, target))
    print("  opaque pixels %d" % opaque)
    for ch in sorted(counts):
        key = pal.legend.get(ch)
        pct = 100.0 * counts[ch] / max(1, opaque) if key else 0.0
        print("    '%s' %-12s %5d %s" % (ch, key or "(transparent)", counts[ch],
                                         ("%5.1f%% of opaque" % pct) if key else ""))

    man = load_manifest(req)
    st = man.setdefault("stages", {}).setdefault("anchor", {})
    st.update({"source": "authored", "spec": str(src.relative_to(REPO)).replace("\\", "/"),
               "generations": 0, "updated": now()})
    save_json(manifest_path(req), man)
    print("\nauthored anchor costs 0 generations. Quantize it next to confirm it is "
          "on-palette, then it becomes the v3 reference image.")
    return 0


# ---------------------------------------------------------------- fetch

def raw_dir(req, stage):
    return rev_dir(req) / "raw" / stage


def quantized_dir(req, stage):
    return rev_dir(req) / "quantized" / stage


def cmd_fetch(args):
    req = load_request(args.id)
    require_subject_request(req, "fetch")
    dest = raw_dir(req, args.stage)
    if args.into:
        # Animation frames come back as 0.png..3.png per direction, so they must be
        # kept apart or every direction overwrites the last.
        parts = [p for p in Path(args.into).parts if p not in ("", ".", "..")]
        dest = dest.joinpath(*parts)
    dest.mkdir(parents=True, exist_ok=True)

    urls = list(args.url or [])
    man = load_manifest(req)
    if not urls:
        urls = man.get("stages", {}).get(args.stage, {}).get("urls", [])
    if not urls:
        raise Fail("no urls given and none recorded in the manifest for stage '%s'. "
                   "Pass --url, or record them first with the 'manifest' verb."
                   % args.stage)

    written = []
    for url in urls:
        # The CDN 403s on urllib's default User-Agent. The URLs need no auth --
        # the UUID in the path is the access key -- but they do need a UA.
        http_req = Request(url, headers={"User-Agent": UA})
        try:
            with urlopen(http_req, timeout=60) as resp:
                blob = resp.read()
        except HTTPError as e:
            raise Fail("download failed (%s %s) for %s\nResult URLs expire -- "
                       "re-run get_character to get fresh ones."
                       % (e.code, e.reason, url))
        except URLError as e:
            raise Fail("download failed (%s) for %s" % (e.reason, url))
        if blob[:4] == b"PK\x03\x04":
            with zipfile.ZipFile(io.BytesIO(blob)) as zf:
                for name in zf.namelist():
                    if not name.lower().endswith(".png"):
                        continue
                    # Keep the archive's own subdirectories. Flattening to basenames
                    # collides rotations/south.png with animations/walk/south_0.png.
                    parts = [p for p in Path(name).parts
                             if p not in ("", ".", "..") and not p.endswith(":")]
                    out = dest.joinpath(*parts)
                    out.parent.mkdir(parents=True, exist_ok=True)
                    out.write_bytes(zf.read(name))
                    written.append("/".join(parts))
        else:
            name = Path(url.split("?")[0]).name or ("download-%d.png" % len(written))
            if not name.lower().endswith(".png"):
                name += ".png"
            (dest / name).write_bytes(blob)
            written.append(name)

    stages = man.setdefault("stages", {}).setdefault(args.stage, {})
    stages["urls"] = urls
    stages["fetched"] = now()
    stages["files"] = sorted(set(stages.get("files", []) + written))
    save_json(manifest_path(req), man)

    print("fetched %d file(s) into %s" % (len(written), dest))
    for w in sorted(set(written)):
        print("  %s" % w)
    return 0


# ---------------------------------------------------------------- quantize

def quantize_array(rgba, pal, moving, normalize="auto", light_shift=False,
                   dim_shift=False):
    """The enforcement pass. Returns (out_rgba_uint8, stats dict).

    1. binarize alpha       -- the material is Unlit+Masked; partial alpha is the
                               most common silent breakage
    2. map to the ramp      -- nearest of the four values by luma, because the ramp
                               is near-neutral and luma separates it evenly
    3. de-stipple to 2x2    -- only where a 1px checkerboard is actually detected,
                               so eyes, marks and 1px outlines survive untouched
    """
    arr = rgba.astype(np.float32)
    rgb, alpha = arr[..., :3], arr[..., 3]
    opaque = alpha >= pal.alpha_threshold
    stats = {
        "pixels": int(rgba.shape[0] * rgba.shape[1]),
        "opaque_pixels": int(opaque.sum()),
        "alpha_partial_before": int(((alpha > 0) & (alpha < 255)).sum()),
    }
    if not opaque.any():
        stats["empty"] = True
        out = np.zeros_like(rgba)
        return out, stats

    # how far off-palette was the input, before we touch it?
    exact = np.zeros(rgb.shape[:2], dtype=bool)
    for c in pal.rgb:
        exact |= np.all(np.abs(rgb - c) < 0.5, axis=-1)
    on_pal_before = int((exact & opaque).sum())
    stats["off_palette_before"] = int(opaque.sum()) - on_pal_before
    stats["on_palette_before_pct"] = round(100.0 * on_pal_before / max(1, opaque.sum()), 2)

    L = pal.luma_of(rgb)
    lo, hi = np.percentile(L[opaque], [2, 98])
    span = float(hi - lo)
    stats["luma_span_before"] = round(span, 3)

    do_norm = (normalize == "always") or (
        normalize == "auto" and stats["on_palette_before_pct"] < 95.0 and span < 0.35)
    stats["normalized"] = bool(do_norm)
    if do_norm:
        # A muddy generation -- all midtones, no true black or white -- would
        # otherwise collapse into steel+bone and read flat. Stretch it first.
        L = np.clip((L - lo) / max(span, 1e-6), 0.0, 1.0)
        L = L * (pal.luma[-1] - pal.luma[0]) + pal.luma[0]

    idx = np.abs(L[..., None] - pal.luma[None, None, :]).argmin(axis=-1)

    if light_shift and dim_shift:
        raise Fail("--light-shift and --dim-shift are mutually exclusive")
    if light_shift or dim_shift:
        table = pal.light_shift if light_shift else pal.dim_shift
        if not table:
            raise Fail("palette '%s' defines no %s map"
                       % (pal.key, "light_shift" if light_shift else "dim_shift"))
        shift = np.array([pal.keys.index(table[k]) for k in pal.keys])
        idx = shift[idx]
        stats["value_shift"] = "light" if light_shift else "dim"

    # -- 2x2 dither enforcement
    stipple = detect_stipple(idx, opaque)
    stats["stipple_px_before"] = int(stipple.sum())
    stats["dither_enforced"] = bool(moving and stipple.any())
    if stats["dither_enforced"]:
        # Snapping one block can expose new stipple at its border, so iterate to a
        # fixed point. Converges in 2-3 passes; the cap is a guard, not a budget.
        passes = 0
        while stipple.any() and passes < 6:
            nxt = snap_blocks(idx, opaque, stipple)
            if np.array_equal(nxt, idx):
                break
            idx = nxt
            stipple = detect_stipple(idx, opaque)
            passes += 1
        stats["dither_passes"] = passes
    stats["stipple_px_after"] = int(detect_stipple(idx, opaque).sum())

    out = np.zeros_like(rgba)
    out[..., :3] = pal.rgb[idx].astype(np.uint8)
    out[..., 3] = np.where(opaque, 255, 0).astype(np.uint8)
    out[~opaque] = 0

    hist = {}
    for i, k in enumerate(pal.keys):
        hist[k] = int(((idx == i) & opaque).sum())
    stats["histogram"] = hist
    stats["values_used"] = sum(1 for v in hist.values() if v > 0)
    total = max(1, opaque.sum())
    stats["histogram_pct"] = {k: round(100.0 * v / total, 1) for k, v in hist.items()}
    stats["dominant"] = max(hist, key=hist.get)

    # -- caged-light audit
    # Owner ruling 2026-07-25 (soldier-roster-v1.md canon proposal 1, ratified): the
    # Lampbearer's point_halo is FREE light; every other unit's bright must be CAGED --
    # enclosed on all four sides, never detached from the body. A pale pixel that touches
    # transparency is free light by definition, so it counterfeits a hero carrier.
    # Palette/alpha/dither checks cannot see this, which is how a 6px uncaged bar shipped
    # through a clean quantize on soldier-05 east/west before this existed.
    if "pale" in pal.keys:
        pale_mask = (idx == pal.keys.index("pale")) & opaque
        if pale_mask.any():
            pad = np.pad(opaque, 1, constant_values=False)
            exposed = pale_mask & ~(pad[:-2, 1:-1] & pad[2:, 1:-1]
                                    & pad[1:-1, :-2] & pad[1:-1, 2:])
            stats["pale_uncaged"] = int(exposed.sum())
        else:
            stats["pale_uncaged"] = 0
    return out, stats


def detect_stipple(idx, opaque):
    """Pixels that are part of a 1px checkerboard field.

    Signature: NO orthogonal neighbour shares this pixel's value, but at least
    three diagonal neighbours do. True of a checker interior; false of a 1px
    diagonal line, a lone eye dot, or a thin outline -- which is the point, since
    those must survive untouched.

    The pixel must also have all four orthogonal neighbours opaque. Without that,
    silhouette-edge pixels register as isolated purely because transparency ate
    their neighbours -- e.g. a dark pixel beside a 1px pale eye at the edge of a
    head reads as a checker when it is nothing of the kind.
    """
    h, w = idx.shape
    pad_i = np.pad(idx, 1, mode="edge")
    pad_o = np.pad(opaque, 1, mode="constant", constant_values=False)
    same_ortho = np.zeros((h, w), dtype=np.int8)
    open_ortho = np.zeros((h, w), dtype=np.int8)
    same_diag = np.zeros((h, w), dtype=np.int8)
    for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
        n = pad_i[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
        no = pad_o[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
        same_ortho += ((n == idx) & no).astype(np.int8)
        open_ortho += no.astype(np.int8)
    for dy, dx in ((-1, -1), (-1, 1), (1, -1), (1, 1)):
        n = pad_i[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
        no = pad_o[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
        same_diag += ((n == idx) & no).astype(np.int8)
    return opaque & (open_ortho == 4) & (same_ortho == 0) & (same_diag >= 3)


def snap_blocks(idx, opaque, stipple):
    """2x2 majority vote, applied only to blocks touching the stipple mask.

    Blocks are aligned to the sprite's own alpha bounding box so the grid is
    stable across frames of an animation. A perfect 1px checker gives every block
    a 2-2 tie, broken by block parity -- which reproduces the field as a genuine
    2x2 checkerboard rather than flattening it. A stray speckle sits in a 3-1
    block and gets absorbed.
    """
    ys, xs = np.nonzero(opaque)
    oy, ox = int(ys.min()), int(xs.min())
    h, w = idx.shape
    out = idx.copy()
    for by in range(oy, h, 2):
        for bx in range(ox, w, 2):
            sl = (slice(by, min(by + 2, h)), slice(bx, min(bx + 2, w)))
            if not stipple[sl].any():
                continue
            block, om = idx[sl], opaque[sl]
            if not om.any():
                continue
            vals, counts = np.unique(block[om], return_counts=True)
            top = counts.max()
            winners = vals[counts == top]
            if len(winners) == 1:
                win = winners[0]
            else:
                win = winners[((by - oy) // 2 + (bx - ox) // 2) % len(winners)]
            out[sl] = np.where(om, win, block)
    return out


def cmd_quantize(args):
    if args.in_dir:
        src = Path(args.in_dir)
        dst = Path(args.out_dir) if args.out_dir else src.parent / (src.name + "-quantized")
        if not args.palette:
            raise Fail("standalone quantize (--in/--out) needs --palette -- there is no "
                       "silent default; name the ramp explicitly, e.g. --palette "
                       "demichrome-4 (see docs/art/aesthetic-direction.md AMENDMENT "
                       "2026-07-28)")
        pal = Palette(args.palette)
        moving = args.moving
        label = str(src)
        req = None
    else:
        if not args.id:
            raise Fail("give a request id, or use --in/--out for standalone mode")
        req = load_request(args.id)
        require_subject_request(req, "quantize")
        pal = request_palette(req)
        src = raw_dir(req, args.stage)
        dst = quantized_dir(req, args.stage)
        moving = req["canon"]["moving"] and req["subject"]["kind"] != "ui"
        label = "%s r%d / %s" % (req["id"], req["revision"], args.stage)

    if not src.exists():
        raise Fail("source directory does not exist: %s" % src)
    pngs = sorted(p for p in src.rglob("*.png"))
    if not pngs:
        raise Fail("no PNGs found under %s" % src)

    dst.mkdir(parents=True, exist_ok=True)
    results = {}
    print("quantize %s -- %d frame(s), palette %s, dither enforcement %s"
          % (label, len(pngs), pal.key, "ON" if moving else "off"))
    print("%-28s %7s %7s %6s %6s  %s" %
          ("frame", "on-pal%", "stipple", "vals", "alpha", "histogram (d/s/b/p %)"))

    for p in pngs:
        img = Image.open(p).convert("RGBA")
        arr = np.array(img)
        out, st = quantize_array(arr, pal, moving, normalize=args.normalize,
                                 light_shift=args.light_shift,
                                 dim_shift=args.dim_shift)
        rel = p.relative_to(src)
        target = dst / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        Image.fromarray(out, "RGBA").save(target)
        results[str(rel).replace("\\", "/")] = st

        if st.get("empty"):
            print("%-28s %7s %7s %6s %6s  (fully transparent)" % (rel, "-", "-", "-", "-"))
            continue
        hp = st["histogram_pct"]
        print("%-28s %7.1f %4d->%-3d %6d %6d  %4.0f/%4.0f/%4.0f/%4.0f%s"
              % (str(rel)[:28], st["on_palette_before_pct"],
                 st["stipple_px_before"], st["stipple_px_after"],
                 st["values_used"], st["alpha_partial_before"],
                 hp["dark"], hp["steel"], hp["bone"], hp["pale"],
                 "  NORMALIZED" if st["normalized"] else ""))

    if req is not None:
        man = load_manifest(req)
        man.setdefault("stages", {}).setdefault(args.stage, {})["quantized"] = now()
        man.setdefault("qc", {})[args.stage] = results
        save_json(manifest_path(req), man)

    flag_findings(results, req, pal)
    print("\nwrote %d frame(s) to %s" % (len(results), dst))
    return 0


def flag_findings(results, req, pal):
    """Everything that should stop a human before generations get spent downstream."""
    notes = []
    for name, st in results.items():
        if st.get("empty"):
            notes.append("%s is fully transparent" % name)
            continue
        if st["values_used"] < 3 and not st.get("value_shift"):
            notes.append("%s uses only %d of 4 values -- the generation was too "
                         "low-contrast; re-roll rather than shipping it flat"
                         % (name, st["values_used"]))
        if st["stipple_px_after"] > 0:
            notes.append("%s still has %d 1px-dither pixel(s) after enforcement"
                         % (name, st["stipple_px_after"]))
        if req is not None:
            want = req["canon"].get("value_dominance")
            if want and want != "mixed" and st["dominant"] != want:
                notes.append("%s is dominated by '%s' but canon.value_dominance says "
                             "'%s' -- check the disjointness audit"
                             % (name, st["dominant"], want))
            if req["canon"].get("pale_usage") == "none" and st["histogram"]["pale"] > 0:
                notes.append("%s spends %d pale pixel(s) but canon.pale_usage is 'none'"
                             % (name, st["histogram"]["pale"]))
            if st.get("pale_uncaged"):
                notes.append("%s has %d pale pixel(s) touching transparency -- that is "
                             "FREE light, which is the Lampbearer's point_halo carrier. "
                             "Every other unit's bright must be caged (enclosed on all "
                             "four sides, never detached)"
                             % (name, st["pale_uncaged"]))
    if notes:
        print("\nfindings:")
        for n in notes:
            print("  ! %s" % n)
    else:
        print("\nfindings: none -- all frames on-palette, binary alpha, no 1px dither.")


# ---------------------------------------------------------------- pack

def anim_dirs(anim_root):
    """The per-animation directories under a quantized anim stage.

    Two layouts occur in practice and only one was anticipated:

      fetch --url <per-frame url>   ->  anim/<name>/south/0.png
      fetch --url <download endpoint> -> anim/<CharName>/animations/<name>/south/frame_000.png

    The second is PixelLab's own zip, which is the ONLY way to get animation frames
    (get_character does not expose per-frame animation URLs), so it is the normal
    route rather than an edge case. It wraps everything in a character-name directory
    and also ships a rotations/ copy alongside the animations.

    Left unhandled, the wrapper makes every animation frame invisible to
    collect_frames -- pack then fails complaining the frame_map references frames
    "that were not produced", pointing at the frame_map instead of at the layout.
    Measured on unit-retinue 2026-07-25: 0 of 14 animation frames collected.
    """
    out = []
    for p in sorted(x for x in anim_root.iterdir() if x.is_dir()):
        if p.name.lower() == "rotations":
            continue  # the rotation stage owns these; counting them here double-books
        nested = p / "animations"
        if nested.is_dir():
            out.extend(sorted(x for x in nested.iterdir() if x.is_dir()))
        else:
            out.append(p)
    return out


def collect_frames(req):
    """frame key -> path, over every quantized stage.

    Keys are '<direction>.<state>'. Rotations give '<dir>.idle'; animation frames
    give '<dir>.<anim><n>' numbered from 1.
    """
    frames = {}
    qroot = rev_dir(req) / "quantized"
    if not qroot.exists():
        return frames

    for stage in ("rotation", "anchor"):
        d = qroot / stage
        if not d.exists():
            continue
        for p in sorted(d.glob("*.png")):
            name = p.stem.lower().replace("_", "-")
            for direction in DIRECTIONS_BY_LENGTH:
                if (name == direction
                        or name.endswith("-" + direction)
                        or name.startswith(direction + "-")):
                    frames.setdefault("%s.idle" % direction, p)
                    break

    anim_root = qroot / "anim"
    if anim_root.exists():
        for adir in anim_dirs(anim_root):
            anim = adir.name.lower()
            per_dir = {}
            # Two layouts are accepted: flat and prefixed (walk/south-0.png), or one
            # subdirectory per direction (walk/south/0.png), which is what fetch
            # --into produces since PixelLab names every frame 0.png.
            for p in sorted(adir.rglob("*.png")):
                rel = p.relative_to(adir)
                token = (rel.parts[0] if len(rel.parts) > 1 else rel.stem)
                token = token.lower().replace("_", "-")
                for direction in DIRECTIONS_BY_LENGTH:
                    if token == direction or token.startswith(direction + "-"):
                        per_dir.setdefault(direction, []).append(p)
                        break
            for direction, paths in per_dir.items():
                ordered = sorted(paths, key=lambda q: (len(q.stem), q.stem))
                for i, p in enumerate(ordered, start=1):
                    frames["%s.%s%d" % (direction, anim, i)] = p
    return frames


def collect_composite_frames(req, pal):
    """('<prefix>:<dir>.<state>' -> path, provenance-by-prefix) for a composite.

    Managed sources delegate to collect_frames on the source request, so a composite
    can never see a frame the pipeline did not quantize. Unmanaged sources are an
    explicit path map and are tagged 'placeholder' so the manifest and the report can
    say out loud that those cells are a stand-in.
    """
    frames, provenance = {}, {}
    for src in req["composite"]["sources"]:
        pfx = src["prefix"]
        if src.get("request"):
            sub = load_request(src["request"])
            sub_frames = collect_frames(sub)
            if not sub_frames:
                raise Fail("composite source '%s' (request '%s') has no quantized "
                           "frames yet -- run its quantize stage first"
                           % (pfx, src["request"]))
            for k, p in sub_frames.items():
                frames["%s:%s" % (pfx, k)] = p
            provenance[pfx] = {
                "kind": "request", "request": src["request"],
                "revision": sub["revision"],
            }
        else:
            for k, rel in (src.get("frames") or {}).items():
                p = REPO / rel
                if not p.exists():
                    raise Fail("composite source '%s' frame '%s' missing: %s"
                               % (pfx, k, p))
                frames["%s:%s" % (pfx, k)] = p
            provenance[pfx] = {
                "kind": "placeholder",
                "note": src.get("note", ""),
                "frames": dict(src.get("frames") or {}),
            }
        provenance[pfx]["quantize"] = bool(src.get("quantize", True))
    return frames, provenance


def _bbox_group_of(key, composite):
    """Which anti-wobble group a frame key belongs to.

    Per SOURCE, not per sheet. The global-bbox rule exists so a walk cycle does not
    wobble inside its cells, and that only needs to hold within one subject. Sharing
    one bbox across two different subjects would let whichever has the longest reach
    dictate everyone's centring — a big weapon swing on one team would shove the
    other team off-centre in every cell.
    """
    return key.split(":", 1)[0] if composite else "_"


def cmd_pack(args):
    req = load_request(args.id)
    out = req["output"]
    cell = out["cell"]
    cols, rows = out["grid"]
    fm = out.get("frame_map") or {}
    if not fm:
        raise Fail("output.frame_map is empty -- nothing to pack")

    composite = is_composite(req)
    provenance = {}
    if composite:
        # A composite has no canon block at all (schema if/then excludes it), so it has
        # no palette to resolve up front -- and doesn't need one unless a source is
        # actually unmanaged AND asks to be repaired (source.quantize, default true).
        # collect_composite_frames doesn't use the pal argument; pass None.
        frames, provenance = collect_composite_frames(req, None)
        pal = None
    else:
        frames = collect_frames(req)
        pal = request_palette(req)

    missing = [v for v in fm.values() if v not in frames]
    if missing:
        avail = ", ".join(sorted(frames)) or "(none)"
        raise Fail("frame_map references frames that were not produced: %s\n"
                   "available: %s" % (", ".join(sorted(set(missing))), avail))

    used = sorted(set(fm.values()))
    imgs = {k: Image.open(frames[k]).convert("RGBA") for k in used}

    # Unmanaged sources have not been through quantize, so enforce the ramp here with
    # the same pass rather than trusting the caller. Managed frames are already snapped,
    # making this a measured no-op that still reports if it wasn't.
    repaired = {}
    for k, im in imgs.items():
        pfx = _bbox_group_of(k, composite)
        if composite and not provenance.get(pfx, {}).get("quantize", True):
            continue
        if pal is None:
            # Only reachable for a composite source with quantize:true -- i.e. it wants
            # repair, but a composite has nowhere to declare which ramp to repair onto.
            # The old code silently defaulted this to demichrome-4; failing loudly here
            # is the fix, not a new restriction -- every composite source in the repo
            # today sets quantize:false explicitly (see swarm-units.json, both sources,
            # 2026-07-28) so this does not fire for anything that currently packs.
            raise Fail("composite source '%s' has quantize:true (or unset, default "
                       "true) but composite requests have no canon.palette to repair "
                       "onto. Set \"quantize\": false if '%s' is already on-ramp or is "
                       "deliberately unmanaged full-colour art, or pack it through a "
                       "subject request (which does declare canon.palette) instead."
                       % (pfx, pfx))
        moving = bool(req.get("canon", {}).get("moving", True))
        arr, stats = quantize_array(np.array(im), pal, moving)
        if stats.get("empty"):
            continue
        imgs[k] = Image.fromarray(arr, "RGBA")
        if stats["on_palette_before_pct"] < 100.0:
            repaired[k] = stats["on_palette_before_pct"]

    # Global alpha bbox per anti-wobble group (see _bbox_group_of).
    groups = {}
    for k, im in imgs.items():
        bb = im.getbbox()
        if bb is None:
            raise Fail("frame '%s' is fully transparent" % k)
        groups.setdefault(_bbox_group_of(k, composite), []).append(bb)

    boxes = {}
    for g, bbs in sorted(groups.items()):
        gx0 = min(b[0] for b in bbs)
        gy0 = min(b[1] for b in bbs)
        gx1 = max(b[2] for b in bbs)
        gy1 = max(b[3] for b in bbs)
        bw, bh = gx1 - gx0, gy1 - gy0
        print("sprite bbox for '%s' across %d frame(s): %dx%d px" % (g, len(bbs), bw, bh))
        if bw > cell or bh > cell:
            raise Fail("'%s' content is %dx%d, larger than the %dpx cell. Never scale "
                       "pixel art -- lower pixellab.size and regenerate, or raise "
                       "output.cell (which breaks the 48px lock)." % (g, bw, bh, cell))
        boxes[g] = (gx0, gy0, gx1, gy1, bw, bh)

    sheet = Image.new("RGBA", (cols * cell, rows * cell), (0, 0, 0, 0))
    for cell_idx, key in sorted(fm.items(), key=lambda kv: int(kv[0])):
        i = int(cell_idx)
        cx, cy = (i % cols) * cell, (i // cols) * cell
        gx0, gy0, gx1, gy1, bw, bh = boxes[_bbox_group_of(key, composite)]
        crop = imgs[key].crop((gx0, gy0, gx1, gy1))
        sheet.paste(crop, (cx + (cell - bw) // 2, cy + (cell - bh) // 2))

    SHEETS.mkdir(parents=True, exist_ok=True)
    target = SHEETS / ("%s.png" % out["texture"])
    sheet.save(target)

    man = load_manifest(req)
    man["sheet"] = {
        "path": str(target.relative_to(REPO)).replace("\\", "/"),
        "size": [cols * cell, rows * cell],
        "cell": cell, "grid": [cols, rows],
        "content_path": out["content_path"],
        "texture": out["texture"],
        "srgb": out.get("sheet_srgb", True),
        "sprite_bbox": {g: [b[4], b[5]] for g, b in boxes.items()},
        "packed": now(),
        "frame_map": fm,
    }
    if composite:
        man["sheet"]["composite_sources"] = provenance
        man["sheet"]["placeholder_cells"] = sorted(
            (int(i) for i, k in fm.items()
             if provenance.get(_bbox_group_of(k, True), {}).get("kind") == "placeholder"))
    if repaired:
        man["sheet"]["palette_repaired_at_pack"] = repaired
    save_json(manifest_path(req), man)

    print("packed %d cell(s) into %s (%dx%d, %dx%d grid of %dpx cells)"
          % (len(fm), target, cols * cell, rows * cell, cols, rows, cell))
    if repaired:
        print("NOTE  %d frame(s) were off-palette and were repaired at pack time: %s"
              % (len(repaired), ", ".join(sorted(repaired))))
    if composite:
        ph = man["sheet"]["placeholder_cells"]
        if ph:
            print("NOTE  cells %s are UNMANAGED placeholder art -- not generated by this "
                  "pipeline. report will flag this." % ph)
    return 0


# ---------------------------------------------------------------- report / manifest

def cmd_report(args):
    req = load_request(args.id)
    man = load_manifest(req)

    # A composite has no quantize stage of its own -- its QC lives in the source
    # requests' reports. What it can report is provenance: which cells are real
    # pipeline output and which are a stand-in. That distinction is the whole reason
    # the composite shape exists, so it is the headline, not a footnote.
    if is_composite(req):
        return report_composite(req, man)

    qc = man.get("qc", {})
    if not qc:
        raise Fail("no QC data yet -- run quantize first")

    pal = request_palette(req)
    total_frames = off = stipple = alpha = low = 0
    for stage, frames in qc.items():
        for st in frames.values():
            if st.get("empty"):
                continue
            total_frames += 1
            off += 0 if st["on_palette_before_pct"] >= 100 else 1
            stipple += 1 if st["stipple_px_after"] else 0
            alpha += 1 if st["alpha_partial_before"] else 0
            low += 1 if st["values_used"] < 3 else 0

    print("QC -- %s r%d" % (req["id"], req["revision"]))
    print("  frames quantized      %d" % total_frames)
    print("  needed palette repair %d" % off)
    print("  residual 1px dither   %d" % stipple)
    print("  had partial alpha     %d" % alpha)
    print("  under 3 values used   %d" % low)
    print("  generations spent     %d (budget %d)"
          % (man.get("generations_spent", 0), req["budget"]["max_generations"]))
    if "sheet" in man:
        s = man["sheet"]
        print("  sheet                 %s %dx%d" % (s["path"], s["size"][0], s["size"][1]))

    verdict = "PASS" if (stipple == 0 and low == 0) else "REVIEW"
    print("  verdict               %s" % verdict)

    save_json(rev_dir(req) / "report.json", {
        "generated": now(), "id": req["id"], "revision": req["revision"],
        "palette": pal.key, "frames": total_frames,
        "needed_palette_repair": off, "residual_1px_dither": stipple,
        "had_partial_alpha": alpha, "under_three_values": low,
        "verdict": verdict, "qc": qc,
    })
    print("\nwrote %s" % (rev_dir(req) / "report.json"))
    return 0


def report_composite(req, man):
    sheet = man.get("sheet")
    if not sheet:
        raise Fail("nothing packed yet -- run pack first")

    srcs = sheet.get("composite_sources", {})
    placeholder = sheet.get("placeholder_cells", [])
    total_cells = len(sheet.get("frame_map", {}))

    print("COMPOSITE -- %s r%d" % (req["id"], req["revision"]))
    print("  sheet                 %s %dx%d"
          % (sheet["path"], sheet["size"][0], sheet["size"][1]))
    print("  cells packed          %d" % total_cells)
    for pfx, info in sorted(srcs.items()):
        if info["kind"] == "request":
            print("  source '%s'%s managed: request %s r%d"
                  % (pfx, " " * max(1, 12 - len(pfx)), info["request"], info["revision"]))
        else:
            print("  source '%s'%s PLACEHOLDER (unmanaged)%s"
                  % (pfx, " " * max(1, 12 - len(pfx)),
                     " -- " + info["note"] if info.get("note") else ""))
    if sheet.get("palette_repaired_at_pack"):
        print("  off-palette repaired  %d frame(s) at pack time"
              % len(sheet["palette_repaired_at_pack"]))

    # A composite carrying placeholder cells is not a failure -- it is a legitimate
    # way to ship a sheet whose second subject has no canon yet. But it must never
    # read as finished, so the verdict says so rather than printing PASS.
    if placeholder:
        print("  placeholder cells     %s" % placeholder)
        verdict = "PLACEHOLDER"
    else:
        verdict = "PASS"
    print("  verdict               %s" % verdict)

    save_json(rev_dir(req) / "report.json", {
        "generated": now(), "id": req["id"], "revision": req["revision"],
        "composite": True, "cells": total_cells,
        "sources": srcs, "placeholder_cells": placeholder,
        "verdict": verdict,
    })
    print("\nwrote %s" % (rev_dir(req) / "report.json"))
    return 0


def cmd_manifest(args):
    req = load_request(args.id)
    require_subject_request(req, "manifest")
    man = load_manifest(req)
    stage = man.setdefault("stages", {}).setdefault(args.stage, {})
    for pair in args.set or []:
        if "=" not in pair:
            raise Fail("--set expects key=value, got '%s'" % pair)
        k, v = pair.split("=", 1)
        if k == "generations":
            # A stage's 'generations' is its RUNNING TOTAL, so it is replaced, not added
            # to -- re-recording a stage after queueing another animation is normal.
            # generations_spent is therefore DERIVED below rather than incremented here.
            # It used to be incremented by the same value that replaced the stage total,
            # so every re-set of a stage double-counted and the two fields drifted apart
            # (measured on unit-retinue: 9 reported against a true spend of 5, which
            # falsely tripped the budget guard).
            stage["generations"] = int(v)
        else:
            stage[k] = v
    for url in args.url or []:
        stage.setdefault("urls", [])
        if url not in stage["urls"]:
            stage["urls"].append(url)
    stage["updated"] = now()

    # Single source of truth: the sum of what each stage says it spent. Idempotent, so
    # re-recording a stage can never inflate the total.
    man["generations_spent"] = sum(int(s.get("generations", 0) or 0)
                                   for s in man["stages"].values())
    save_json(manifest_path(req), man)

    spent, budget = man["generations_spent"], req["budget"]["max_generations"]
    print("manifest updated: %s" % manifest_path(req))
    print("  stage '%s': %s" % (args.stage, json.dumps(stage)))
    print("  generations spent %d / %d" % (spent, budget))
    if spent > budget:
        print("  ! OVER BUDGET")
        return 1
    return 0


# ---------------------------------------------------------------- cli

def main():
    ap = argparse.ArgumentParser(
        prog="pixelpipe",
        description="Local half of the ELVTR PixelLab sprite pipeline. "
                    "Never calls the PixelLab API.")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("validate", help="schema + canon checks on a request")
    p.add_argument("id")
    p.set_defaults(fn=cmd_validate)

    p = sub.add_parser("prompt", help="compose the deterministic prompt + tool kwargs")
    p.add_argument("id")
    p.add_argument("--stage", choices=["anchor", "rotation"], default="anchor")
    p.set_defaults(fn=cmd_prompt)

    p = sub.add_parser("authored",
                       help="render the spec's ASCII pixel map as the anchor (0 gens)")
    p.add_argument("id")
    p.add_argument("--from", dest="from_file",
                   help="read the pixel map from this file instead of subject.spec")
    p.add_argument("--index", type=int, default=0,
                   help="which pixel-map block to use when the spec has several")
    p.set_defaults(fn=cmd_authored)

    p = sub.add_parser("fetch", help="download generated assets into raw/")
    p.add_argument("id")
    p.add_argument("--stage", required=True)
    p.add_argument("--url", action="append")
    p.add_argument("--into", help="subdirectory under the stage dir, e.g. walk/south -- "
                                  "needed for animation frames, which are all named 0.png")
    p.set_defaults(fn=cmd_fetch)

    p = sub.add_parser("quantize", help="enforce a named palette ramp (opt-in; "
                                        "request mode reads canon.palette, "
                                        "standalone mode requires --palette)")
    p.add_argument("id", nargs="?")
    p.add_argument("--stage", default="anchor")
    p.add_argument("--in", dest="in_dir", help="standalone: quantize any directory")
    p.add_argument("--out", dest="out_dir")
    p.add_argument("--moving", action="store_true",
                   help="standalone: enforce the 2x2 dither rule")
    p.add_argument("--palette", default=None,
                   help="required in standalone (--in/--out) mode -- e.g. "
                        "demichrome-4. No default: palette enforcement is opt-in, "
                        "not assumed (see docs/art/aesthetic-direction.md AMENDMENT "
                        "2026-07-28)")
    p.add_argument("--normalize", choices=["auto", "always", "never"], default="auto")
    p.add_argument("--light-shift", action="store_true",
                   help="emit the one-value-brighter lamp-radius variant")
    p.add_argument("--dim-shift", action="store_true",
                   help="emit the one-value-darker variant (outside the leash, "
                        "or a guttering flame)")
    p.set_defaults(fn=cmd_quantize)

    p = sub.add_parser("pack", help="assemble the SubUV sheet")
    p.add_argument("id")
    p.set_defaults(fn=cmd_pack)

    p = sub.add_parser("report", help="QC summary + report.json")
    p.add_argument("id")
    p.set_defaults(fn=cmd_report)

    p = sub.add_parser("manifest", help="record provenance from MCP results")
    p.add_argument("id")
    p.add_argument("--stage", required=True)
    p.add_argument("--set", action="append", metavar="KEY=VALUE")
    p.add_argument("--url", action="append")
    p.set_defaults(fn=cmd_manifest)

    args = ap.parse_args()
    try:
        return args.fn(args)
    except Fail as e:
        print("ERROR %s" % e, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
