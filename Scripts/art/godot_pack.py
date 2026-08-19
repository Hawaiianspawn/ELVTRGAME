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
    os.makedirs(OUT, exist_ok=True)
    manifest = {}
    for name, ref in roster.items():
        d = rotations_dir(ref, selects)
        frames = [Image.open(os.path.join(d, "%s.png" % f)).convert("RGBA") for f in DIRECTIONS]
        cell = max(max(im.size) for im in frames)
        strip = Image.new("RGBA", (cell * 8, cell))
        for i, im in enumerate(frames):
            strip.paste(im, (i * cell + (cell - im.width) // 2, cell - im.height))
        strip.save(os.path.join(OUT, name + ".png"))
        manifest[name] = {"cell": cell, "source": ref}
        print("%-24s %3dpx  %s" % (name, cell, ref))
    json.dump(manifest, open(os.path.join(OUT, "manifest.json"), "w"), indent=2)


if __name__ == "__main__":
    main()
