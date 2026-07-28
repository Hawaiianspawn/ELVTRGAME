# coverage.py -- read-only art asset coverage audit.
#
# Answers "what art assets does a character need, and which parts actually exist and
# are being drawn?" against docs/data/art/asset-matrix.json. This is a layer ABOVE
# Scripts/art/pixelpipe.py, not a replacement for it: it never generates, imports,
# edits art, or calls any mcp__pixellab__* tool. It only reads files that already
# exist and reports gaps.
#
# Five categories, one per matrix entry:
#   missing      required by the matrix, absent from disk and/or Content
#   unwired      exists on disk/Content but nothing in ELVTR/Source loads it
#   off-ramp     the packed sheet has pixels off the locked 4-value Demichrome ramp
#   unrecorded   no entry for it in docs/data/art/provenance.json
#   incomplete   present but short of the matrix's required frames/frame count
#
# Usage:
#     py Scripts/art/coverage.py                 human-readable report, all entries
#     py Scripts/art/coverage.py --id T_Soldier_01   one entry only
#     py Scripts/art/coverage.py --json           machine-readable report on stdout
#     py Scripts/art/coverage.py --category unwired   only entries with that finding
#
# What would fool this tool -- read before trusting a clean report:
#   - The 'unwired' check is a literal grep for the texture name (and, for the
#     composite entry, the composite request's own frame_map keys) across
#     ELVTR/Source/**/*.cpp/*.h. It CANNOT see: Blueprint graphs (binary .uasset),
#     Niagara systems/materials (binary/editor-only), DataAssets, or any texture
#     loaded via a path built at runtime (string concatenation, a DataTable row, a
#     soft-object-ptr resolved from data) rather than a literal string. A texture
#     referenced only from a Blueprint or a Niagara renderer will be reported
#     unwired even if it is, in fact, drawn in-game.
#   - The 'off-ramp' check inspects the RawArt/Sheets/<texture>.png source file, NOT
#     the imported .uasset -- there is no read-only way to inspect an imported
#     texture's raw pixels from outside the editor. If Content and RawArt/Sheets
#     have drifted (someone re-imported a different PNG by hand), this check is
#     blind to that drift.
#   - The palette check is exact-hex membership on opaque pixels (plus alpha
#     strictly 0/255), not pixelpipe.py's fuller quantize pass -- it does NOT run
#     the caged-light (pale_uncaged) audit, dither-block-size check, or
#     value_dominance/pale_usage cross-check. A sheet can pass this tool's
#     off-ramp check and still fail pixelpipe.py's quantize findings.
#   - 'unrecorded' checks ONE of two places depending on the entry, matching the two
#     provenance conventions actually in use in this repo: an entry with a
#     source_request is checked against that request's own
#     RawArt/Renders/<id>/r<rev>/manifest.json (pixelpipe.py's convention); an entry
#     with no request (the archer proxy) is checked against
#     docs/data/art/provenance.json, which docs/data/art/provenance.json's own
#     top-of-file note says exists specifically for assets that skipped the request
#     pipeline. If that file is ever deleted, every no-request entry reports
#     unrecorded -- a real finding, not a bug.
#   - 'missing' and 'incomplete' are computed from the matrix's declared
#     requirements. An entry whose requirement was itself invented rather than
#     read from a filed request (see asset-matrix.schema.md's derived_from field,
#     and T_Soldier_Archer's open_questions) will report gaps against a
#     requirement nobody has actually signed off on. Read open_questions before
#     treating a finding as an action item.

import argparse
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DATA = REPO / "docs" / "data" / "art"
MATRIX_FILE = DATA / "asset-matrix.json"
PALETTE_FILE = DATA / "palette.json"
PROVENANCE_FILE = DATA / "provenance.json"
SHEETS = REPO / "RawArt" / "Sheets"
CONTENT = REPO / "ELVTR" / "Content"
SOURCE = REPO / "ELVTR" / "Source"


def load_json(path):
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


# ---------------------------------------------------------------- palette

class Palette:
    """Exact-hex membership check against docs/data/art/palette.json.

    Deliberately simpler than pixelpipe.py's Palette/quantize_array: this tool
    reports whether a shipped sheet IS on-ramp, not how to snap it onto the ramp.
    """

    def __init__(self, key="demichrome-4"):
        pal = load_json(PALETTE_FILE)["palettes"][key]
        self.key = key
        self.alpha_threshold = pal["mask"]["threshold"]
        self.rgb_by_key = {v["key"]: tuple(v["rgb"]) for v in pal["values"]}
        self.rgb_set = set(self.rgb_by_key.values())

    def offramp_stats(self, rgba):
        """rgba: numpy array (H, W, 4) uint8. Returns dict of counts."""
        import numpy as np

        rgb = rgba[..., :3].astype(np.int32)
        alpha = rgba[..., 3].astype(np.int32)
        opaque = alpha >= self.alpha_threshold
        partial_alpha = int(((alpha > 0) & (alpha < 255)).sum())

        on_ramp = np.zeros(rgb.shape[:2], dtype=bool)
        for r, g, b in self.rgb_set:
            on_ramp |= (rgb[..., 0] == r) & (rgb[..., 1] == g) & (rgb[..., 2] == b)

        opaque_count = int(opaque.sum())
        off_ramp_count = int((opaque & ~on_ramp).sum())
        return {
            "opaque_px": opaque_count,
            "off_ramp_px": off_ramp_count,
            "off_ramp_pct": round(100.0 * off_ramp_count / opaque_count, 2) if opaque_count else 0.0,
            "partial_alpha_px": partial_alpha,
        }


# ---------------------------------------------------------------- checks

def check_disk_and_content(entry):
    sheet_path = REPO / entry["disk_sheet"]
    disk_present = sheet_path.exists()

    # .uasset presence under the expected content_path -- the closest read-only proxy
    # for "is this in Content" available outside the editor. content_path is a /Game
    # package path; ELVTR/Content mirrors it 1:1 minus the /Game prefix.
    rel = entry["content_path"].removeprefix("/Game/")
    uasset_path = CONTENT / rel / (entry["texture"] + ".uasset")
    content_present = uasset_path.exists()

    return {
        "disk_present": disk_present,
        "disk_path": str(sheet_path.relative_to(REPO)),
        "content_present": content_present,
        "content_path": str(uasset_path.relative_to(REPO)),
    }


def check_wiring(entry):
    check = entry["wiring"]["check"]
    if check["method"] != "grep_source":
        return {"method": check["method"], "wired": None, "note": "not a grep_source check"}

    hits = []
    for pattern in check["patterns"]:
        for path in SOURCE.rglob("*"):
            if path.suffix.lower() not in (".cpp", ".h"):
                continue
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except OSError:
                continue
            if pattern in text:
                hits.append(str(path.relative_to(REPO)))
    return {"method": "grep_source", "wired": len(hits) > 0, "hits": sorted(set(hits))}


def check_offramp(entry, pal):
    if not (REPO / entry["disk_sheet"]).exists():
        return {"checked": False, "reason": "no source PNG on disk to inspect"}
    try:
        import numpy as np
        from PIL import Image
    except ImportError:
        return {"checked": False, "reason": "PIL/numpy not available in this environment"}

    img = Image.open(REPO / entry["disk_sheet"]).convert("RGBA")
    rgba = np.array(img)
    stats = pal.offramp_stats(rgba)
    stats["checked"] = True
    stats["off_ramp"] = stats["off_ramp_px"] > 0 or stats["partial_alpha_px"] > 0
    return stats


def check_unrecorded(entry, provenance):
    """An entry is 'recorded' by either of two conventions, both real in this repo:

    1. It went through the normal /sprite request pipeline, which records its own
       provenance per-request at RawArt/Renders/<id>/r<rev>/manifest.json
       (Scripts/art/pixelpipe.py's convention) -- checked whenever the matrix entry
       has a source_request, using that request's own 'revision' field so this
       can't drift from which revision is actually current.
    2. It is one of the EXCEPTIONS docs/data/art/provenance.json exists to cover --
       art that did not go through the pipeline (its own top-of-file note says so
       explicitly) -- checked by texture name against provenance.json's entries.

    An entry with a source_request is checked against (1) only: provenance.json's
    own stated purpose is for assets that have no request, so an entry that HAS one
    should not be counted unrecorded merely for being absent from the exceptions
    file. An entry with no source_request (the archer) is checked against (2) only.
    """
    if entry.get("source_request") and (REPO / entry["source_request"]).exists():
        req = load_json(REPO / entry["source_request"])
        manifest_path = REPO / "RawArt" / "Renders" / req["id"] / ("r%d" % req["revision"]) / "manifest.json"
        recorded = manifest_path.exists()
        return {
            "checked": True,
            "recorded": recorded,
            "convention": "per-request manifest.json (pixelpipe.py)",
            "expected_path": str(manifest_path.relative_to(REPO)),
        }

    if provenance is None:
        return {"checked": True, "recorded": False,
                "convention": "docs/data/art/provenance.json (exceptions file)",
                "reason": "docs/data/art/provenance.json does not exist"}
    entries = provenance.get("entries", []) if isinstance(provenance, dict) else []
    recorded = any(e.get("texture") == entry["texture"] for e in entries)
    return {"checked": True, "recorded": recorded, "convention": "docs/data/art/provenance.json (exceptions file)"}


def check_incomplete(entry, disk_status):
    """Compares what the matrix requires against what the packed sheet's own grid
    can hold, and -- for entries with no request file -- what has actually been
    generated under RawArt/Renders/<provenance_id>/.
    """
    required_count = entry.get("required_frame_count")
    if required_count is None and entry.get("required_frames"):
        required_count = len(entry["required_frames"])
    if required_count is None:
        return {"checked": False, "reason": "matrix entry states no required frame count"}

    grid = entry["sheet"]["grid"]
    cell_capacity = grid[0] * grid[1]

    result = {
        "checked": True,
        "required_frame_count": required_count,
        "sheet_cell_capacity": cell_capacity,
    }

    if entry.get("source_request") and (REPO / entry["source_request"]).exists():
        req = load_json(REPO / entry["source_request"])
        actual_keys = list(req.get("output", {}).get("frame_map", {}).values())
        result["actual_frame_count"] = len(actual_keys)
        result["actual_frame_keys"] = actual_keys
        if entry.get("required_frames"):
            missing = [k for k in entry["required_frames"] if k not in actual_keys]
            result["missing_frame_keys"] = missing
            result["incomplete"] = len(missing) > 0
        else:
            result["incomplete"] = len(actual_keys) < required_count
    else:
        # No request file (e.g. the archer proxy) -- fall back to counting distinct
        # PNGs actually generated under RawArt/Renders/<provenance_id>/, which is
        # the best available read-only signal of what exists.
        renders_dir = REPO / "RawArt" / "Renders" / entry["provenance_id"]
        png_count = len(list(renders_dir.rglob("*.png"))) if renders_dir.exists() else 0
        result["renders_dir"] = str(renders_dir.relative_to(REPO)) if renders_dir.exists() else None
        result["actual_frame_count"] = png_count
        result["incomplete"] = png_count < required_count
        result["note"] = ("no source_request filed; frame count is a floor estimate from "
                           "distinct PNGs under RawArt/Renders/, not a validated frame_map")

    # A sheet whose own grid can't even HOLD the required frame count is incomplete
    # by construction, regardless of what got packed into it.
    if cell_capacity < required_count:
        result["incomplete"] = True
        result["capacity_shortfall"] = required_count - cell_capacity

    return result


# ---------------------------------------------------------------- audit

def audit_entry(entry, pal, provenance):
    findings = []

    disk_status = check_disk_and_content(entry)
    if entry["required"] and not disk_status["content_present"]:
        detail = ("absent from RawArt/Sheets and ELVTR/Content"
                  if not disk_status["disk_present"]
                  else "generated to RawArt/Sheets but NOT imported into ELVTR/Content")
        findings.append(("missing", detail))

    wiring_status = check_wiring(entry)
    if disk_status["content_present"] and wiring_status.get("wired") is False:
        findings.append(("unwired", wiring_status["method"] + ": no match for "
                          + ", ".join(entry["wiring"]["check"].get("patterns", []))
                          + " anywhere in ELVTR/Source"))

    offramp_status = check_offramp(entry, pal)
    if offramp_status.get("checked") and offramp_status.get("off_ramp"):
        findings.append(("off-ramp",
                          "%d off-ramp px (%.2f%% of opaque), %d partial-alpha px"
                          % (offramp_status["off_ramp_px"], offramp_status["off_ramp_pct"],
                             offramp_status["partial_alpha_px"])))

    unrecorded_status = check_unrecorded(entry, provenance)
    if unrecorded_status.get("checked") and not unrecorded_status.get("recorded"):
        default = "no manifest.json found (expected %s)" % unrecorded_status.get("expected_path")
        findings.append(("unrecorded", unrecorded_status.get("reason", default)
                          + " [convention: %s]" % unrecorded_status.get("convention")))

    incomplete_status = check_incomplete(entry, disk_status)
    if incomplete_status.get("checked") and incomplete_status.get("incomplete"):
        if "missing_frame_keys" in incomplete_status and incomplete_status["missing_frame_keys"]:
            detail = "missing frame keys: " + ", ".join(incomplete_status["missing_frame_keys"])
        elif "capacity_shortfall" in incomplete_status:
            detail = ("sheet grid %s can only hold %d cells, %d short of the required %d"
                       % (entry["sheet"]["grid"], incomplete_status["sheet_cell_capacity"],
                          incomplete_status["capacity_shortfall"], incomplete_status["required_frame_count"]))
        else:
            detail = ("%d frame(s) present, %d required"
                       % (incomplete_status.get("actual_frame_count", 0),
                          incomplete_status["required_frame_count"]))
        findings.append(("incomplete", detail))

    return {
        "id": entry["id"],
        "character": entry["character"],
        "required": entry["required"],
        "findings": findings,
        "detail": {
            "disk": disk_status,
            "wiring": wiring_status,
            "off_ramp": offramp_status,
            "provenance": unrecorded_status,
            "completeness": incomplete_status,
        },
        "open_questions": entry.get("open_questions", []),
    }


def run(matrix, only_id=None, only_category=None):
    pal = Palette()
    provenance = load_json(PROVENANCE_FILE) if PROVENANCE_FILE.exists() else None

    results = []
    for entry in matrix["characters"]:
        if only_id and entry["id"] != only_id:
            continue
        result = audit_entry(entry, pal, provenance)
        if only_category and not any(cat == only_category for cat, _ in result["findings"]):
            continue
        results.append(result)
    return results


# ---------------------------------------------------------------- reporting

CATEGORY_ORDER = ["missing", "unwired", "off-ramp", "unrecorded", "incomplete"]


def print_report(results, matrix):
    print("art coverage audit -- %s" % matrix.get("updated", "?"))
    print("%d character(s) audited\n" % len(results))

    for r in results:
        tag = "REQUIRED" if r["required"] else "optional"
        print("=== %-20s %-40s [%s] ===" % (r["id"], r["character"], tag))
        if not r["findings"]:
            print("  clean -- no findings")
        for cat, detail in r["findings"]:
            print("  ! %-11s %s" % (cat, detail))
        if r["open_questions"]:
            print("  open questions:")
            for q in r["open_questions"]:
                print("    - %s" % (q[:160] + ("..." if len(q) > 160 else "")))
        print()

    counts = {c: 0 for c in CATEGORY_ORDER}
    for r in results:
        for cat, _ in r["findings"]:
            counts[cat] += 1
    print("--- summary ---")
    for c in CATEGORY_ORDER:
        print("  %-11s %d" % (c, counts[c]))
    untested = [c for c in CATEGORY_ORDER if counts[c] == 0]
    if untested:
        print("  untested categories (no matrix entry currently exercises them): %s" % ", ".join(untested))


def main():
    # Windows consoles default to a codepage (cp1252) that can't encode every
    # character some matrix entries' free-text fields use (x02192 arrows, x000d7
    # multiplication signs in "4x2"-style dimensions). Force utf-8 so a report
    # doesn't crash on a stray character in someone's prose.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--id", help="audit only this matrix entry id (e.g. T_Soldier_01)")
    ap.add_argument("--category", choices=CATEGORY_ORDER, help="show only entries with this finding")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    args = ap.parse_args()

    matrix = load_json(MATRIX_FILE)
    results = run(matrix, only_id=args.id, only_category=args.category)

    if args.json:
        json.dump({"updated": matrix.get("updated"), "results": results}, sys.stdout, indent=2)
        print()
    else:
        print_report(results, matrix)

    return 0


if __name__ == "__main__":
    sys.exit(main())
