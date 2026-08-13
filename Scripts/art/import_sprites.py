# import_sprites.py
#
# Imports a packed sprite sheet from RawArt/Sheets/ into the UE Content Browser with
# ELVTR's canonical pixel-art settings, so nobody has to remember the four dropdowns
# in ELVTR/SETUP-EDITOR.md ever again.
#
# Settings applied (from ELVTR/SETUP-EDITOR.md §1, restated in every art doc):
#     Filter          = Nearest          (pixel art -- no bilinear smear)
#     MipGenSettings  = NoMipmaps        (mips destroy a 48px cell at distance)
#     Compression     = UserInterface2D  (RGBA, no DXT blocking artifacts)
#     sRGB            = on               (off for mask-only textures, per
#                                         docs/ui/UI-PROTOTYPE-PLAN.md)
#
# Run from the Unreal editor console (~ then type):
#     py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" hero-vanguard
# or with no argument to import every request that has a packed sheet:
#     py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py"
# or Tools > Execute Python Script... and pick this file (imports everything).
#
# Reads the destination and texture name from each request's manifest
# (RawArt/Renders/<id>/r<rev>/manifest.json, "sheet" block), which pixelpipe.py pack
# writes. That keeps this script free of hardcoded paths -- the request is the contract.
#
# The MCP *Python sandbox* still cannot `import unreal`. That no longer means this is
# unreachable from MCP: as of 2026-07-31 the project's own toolset can run any console
# command, and the console's `py` reaches the editor's real interpreter --
#     KindledConsoleToolset.Exec('py "C:/Projects/ELVTRGAME/Scripts/art/import_sprites.py" <id>')
# Verified 2026-07-31. Output goes to the log, not the tool result, so read it back with
# LogsToolset.GetLogEntries(Category:"", Pattern:"..."). See .claude/skills/atlas/SKILL.md.

import json
import os
import sys

import unreal

# This file is Scripts/art/import_sprites.py, so the repo root is THREE levels up.
# It used to be two, which resolved REPO to <repo>/Scripts and made every lookup miss:
# latest_manifest() found no manifest.json anywhere and the script warned "has no packed
# sheet yet -- run pack first" for every request, while exiting 0. The advice was wrong
# and the failure was silent, which is the worst combination. pixelpipe.py has always
# used parents[2] for the same reason -- keep the two in step.
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REQUESTS = os.path.join(REPO, "docs", "data", "art", "requests")
RENDERS = os.path.join(REPO, "RawArt", "Renders")
TAG = "SPRITE import"


def log(msg):
    unreal.log("%s: %s" % (TAG, msg))


def warn(msg):
    unreal.log_warning("%s: %s" % (TAG, msg))


def latest_manifest(request_id):
    """Newest revision that actually has a packed sheet."""
    root = os.path.join(RENDERS, request_id)
    if not os.path.isdir(root):
        return None
    revs = sorted((d for d in os.listdir(root) if d.startswith("r") and d[1:].isdigit()),
                  key=lambda d: int(d[1:]), reverse=True)
    for rev in revs:
        path = os.path.join(root, rev, "manifest.json")
        if not os.path.isfile(path):
            continue
        with open(path, "r") as fh:
            man = json.load(fh)
        if "sheet" in man:
            return man
    return None


def import_sheet(man):
    sheet = man["sheet"]
    src = os.path.join(REPO, sheet["path"].replace("/", os.sep))
    if not os.path.isfile(src):
        warn("sheet missing on disk, skipped: %s" % src)
        return None

    pkg = sheet["content_path"]
    name = sheet["texture"]
    obj_path = "%s/%s.%s" % (pkg, name, name)

    task = unreal.AssetImportTask()
    task.filename = src
    task.destination_path = pkg
    task.destination_name = name
    task.automated = True          # no import dialog
    task.replace_existing = True   # re-importing a sheet is the normal case
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    tex = unreal.EditorAssetLibrary.load_asset(obj_path)
    if tex is None:
        warn("import produced no asset at %s" % obj_path)
        return None

    # The import task cannot carry these, so set them after and re-save.
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings",
                            unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("compression_settings",
                            unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("srgb", bool(sheet.get("srgb", True)))
    unreal.EditorAssetLibrary.save_asset(obj_path)

    # Read back rather than trusting the writes -- a silent no-op here would look
    # identical to success and only show up as a blurry sprite much later.
    got = {
        "filter": str(tex.get_editor_property("filter")),
        "mips": str(tex.get_editor_property("mip_gen_settings")),
        "compression": str(tex.get_editor_property("compression_settings")),
        "srgb": tex.get_editor_property("srgb"),
        "size": "%dx%d" % (tex.blueprint_get_size_x(), tex.blueprint_get_size_y()),
    }
    ok = ("NEAREST" in got["filter"]
          and "NO_MIPMAPS" in got["mips"]
          and got["size"] == "%dx%d" % (sheet["size"][0], sheet["size"][1]))
    log("%s -> %s  %s  filter=%s mips=%s comp=%s srgb=%s  cells %dx%d @ %dpx  [%s]"
        % (os.path.basename(src), obj_path, got["size"], got["filter"], got["mips"],
           got["compression"], got["srgb"],
           sheet["grid"][0], sheet["grid"][1], sheet["cell"],
           "OK" if ok else "VERIFY FAILED"))
    if not ok:
        warn("read-back did not match the requested settings for %s" % obj_path)
    return obj_path


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if args:
        ids = args
    else:
        if not os.path.isdir(REQUESTS):
            warn("no requests directory at %s" % REQUESTS)
            return
        ids = sorted(f[:-5] for f in os.listdir(REQUESTS) if f.endswith(".json"))

    log("importing %d request(s): %s" % (len(ids), ", ".join(ids)))
    done, skipped = [], []
    for rid in ids:
        man = latest_manifest(rid)
        if man is None:
            skipped.append(rid)
            warn("%s has no packed sheet yet -- run 'pixelpipe.py pack %s' first"
                 % (rid, rid))
            continue
        path = import_sheet(man)
        (done if path else skipped).append(rid)

    log("done. imported %d, skipped %d." % (len(done), len(skipped)))


main()
