"""Pack owner-approved 8-direction rotations into strips for the Godot web build.

    py Scripts/art/godot_pack.py

Reads godot/data/roster.json ({"<out-name>": "<family>/<variant>"}), resolves each through
RawArt/Aaron Selects/selects.json (or a raw RawArt/Renders path when the value already is
one), and writes godot/assets/sprites/<out-name>.png: 8 cells wide, one direction each,
in the shared atlas column order from atlas.py. Cell size = tallest source (88 or 92).
"""
import json, os, sys
from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from atlas import DIRECTIONS  # column contract shared with the UE atlases

SELECTS = os.path.join(REPO, "RawArt", "Aaron Selects", "selects.json")
ROSTER = os.path.join(REPO, "godot", "data", "roster.json")
UNITS = os.path.join(REPO, "godot", "data", "units.json")
OUT = os.path.join(REPO, "godot", "assets", "sprites")


def rotations_dir(ref, selects):
    e = selects.get(ref, {})
    if e.get("verdict") == "deny":
        print("WARN %s is denied in selects.json" % ref)
    for p in (e.get("source"), ref, "RawArt/Renders/%s/raw/%s/rotations" % tuple(ref.split("/", 1))):
        if p and os.path.isdir(os.path.join(REPO, p)):
            return os.path.join(REPO, p)
    raise SystemExit("unknown source: %s" % ref)


def main():
    selects = json.load(open(SELECTS, encoding="utf-8"))
    roster = json.load(open(ROSTER, encoding="utf-8"))
    # units.json "attack_frames": keep only the first N frames of a unit's attack clip (trailing
    # rest-pose duplicates are cut at pack time so the raw PixelLab frames stay untouched).
    cut = {u["sprite"]: int(u["attack_frames"]) for u in json.load(open(UNITS, encoding="utf-8")).values()
           if isinstance(u, dict) and u.get("attack_frames")}
    os.makedirs(OUT, exist_ok=True)
    manifest = {}
    strips = []
    for name, ref in roster.items():
        d = rotations_dir(ref, selects)
        frames = [Image.open(os.path.join(d, "%s.png" % f)).convert("RGBA") for f in DIRECTIONS]
        cell = max(max(im.size) for im in frames)
        # Feet on the cell floor: sources carry transparent padding under the feet, and Godot puts the
        # cell bottom on the unit origin, so any padding becomes a float that grows with sprite scale.
        floor_y = max(im.getbbox()[3] for im in frames if im.getbbox())
        strip = Image.new("RGBA", (cell * 8, cell))
        for i, im in enumerate(frames):
            strip.paste(im, (i * cell + (cell - im.width) // 2, cell - floor_y))
        strip.save(os.path.join(OUT, name + ".png"))
        # Alpha centroid height above the feet, averaged over the 8 rotations: the airborne
        # spin pivot. Box-resize to 1px wide = mean alpha per row, so weapons and headroom
        # weigh in exactly as much pixel mass as they have.
        coms = []
        for im in frames:
            rows = list(im.split()[3].resize((1, im.height), Image.BOX).getdata())
            m = sum(rows)
            if m:
                coms.append(floor_y - sum(yy * v for yy, v in enumerate(rows)) / m)
        manifest[name] = {"cell": cell, "source": ref,
                          "com": round(sum(coms) / len(coms), 1) if coms else cell * 0.42}
        strips.append((name, strip))
        print("%-24s %3dpx  %s" % (name, cell, ref))
        # Optional clip rows: <rotations>/../<clip>_north/frame_*.png (PixelLab v3 frames, same
        # canvas as the rotations) packed as extra strips right under the unit's rotations.
        for clip in ("attack", "slam"):
            adir = os.path.join(os.path.dirname(d), clip + "_north")
            if not os.path.isdir(adir):
                continue
            names = sorted((f for f in os.listdir(adir) if f.startswith("frame_") and f.endswith(".png")),
                           key=lambda f: int(f[6:-4]))
            if clip == "attack" and name in cut:
                names = names[:cut[name]]
            frames = [Image.open(os.path.join(adir, f)).convert("RGBA") for f in names]
            strip = Image.new("RGBA", (cell * len(frames), cell))
            for i, im in enumerate(frames):
                strip.paste(im, (i * cell + (cell - im.width) // 2, cell - floor_y))
            manifest[name][clip] = {"frames": len(frames)}
            strips.append((name + "/" + clip, strip))
            print("%-24s %s_north x%d" % ("", clip, len(frames)))
    # Effect clips: RawArt/Renders/fx-slash/raw/<name>/frames/frame_N.png -> "fx_<name>" row, no rotations.
    fxroot = os.path.join(REPO, "RawArt", "Renders", "fx-slash", "raw")
    for fx in sorted(os.listdir(fxroot)) if os.path.isdir(fxroot) else []:
        fdir = os.path.join(fxroot, fx, "frames")
        if not os.path.isdir(fdir):
            continue
        names = sorted((f for f in os.listdir(fdir) if f.startswith("frame_") and f.endswith(".png")),
                       key=lambda f: int(f[6:-4]))
        frames = [Image.open(os.path.join(fdir, f)).convert("RGBA") for f in names]
        cell = max(max(im.size) for im in frames)
        strip = Image.new("RGBA", (cell * len(frames), cell))
        for i, im in enumerate(frames):
            strip.paste(im, (i * cell, 0))
        manifest["fx_" + fx] = {"cell": cell, "frames": len(frames), "source": "fx-slash/" + fx}
        strips.append(("fx_" + fx, strip))
        print("%-24s %3dpx  fx x%d" % ("fx_" + fx, cell, len(frames)))
    # One atlas for the whole roster: every sprite shares a texture so the 2D batcher keeps them in one draw call.
    width = max(im.width for _, im in strips)
    height = sum(im.height for _, im in strips)
    atlas = Image.new("RGBA", (width, height))
    y = 0
    for name, im in strips:
        atlas.paste(im, (0, y))
        if "/" in name:
            base, clip = name.split("/")
            manifest[base][clip]["y"] = y
        else:
            manifest[name]["y"] = y
        y += im.height
    atlas.save(os.path.join(OUT, "atlas.png"))
    print("atlas %dx%d" % (width, height))
    json.dump(manifest, open(os.path.join(OUT, "manifest.json"), "w"), indent=2)


if __name__ == "__main__":
    main()
