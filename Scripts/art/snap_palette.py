"""Snap sprites onto another sprite's colours.

Match a look to a unit that already ships -- the reference is a rotations dir, not a named
canon palette, because "the knight palette" is only ever what v7_barestance is made of.

    py Scripts/art/snap_palette.py REF_ROT_DIR OUT_FAMILY SRC_ROT_DIR [SRC_ROT_DIR ...]

Writes RawArt/Renders/<OUT_FAMILY>/raw/<slug>/rotations/*.png, leaving every source frame
untouched (retention rule: generated raws are never overwritten).
"""
import sys
from collections import Counter
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parents[2]
RENDERS = REPO / "RawArt" / "Renders"
DIRS = ("south", "south-east", "east", "north-east",
        "north", "north-west", "west", "south-west")


def palette_of(ref_dir, cover=0.995):
    """Exact opaque colours of the reference, most-used first, down to `cover` of the body."""
    c = Counter()
    for p in sorted(Path(ref_dir).glob("*.png")):
        im = Image.open(p).convert("RGBA")
        c.update(px[:3] for px in im.getdata() if px[3] >= 128)
    total = sum(c.values()) or 1
    out, run = [], 0
    for rgb, n in c.most_common():
        out.append(rgb)
        run += n
        if run / total >= cover:
            break
    return out


def snap_image(im, pal):
    """Nearest palette colour per opaque pixel, alpha kept as-is."""
    # ponytail: exact-colour memo + linear scan over a <100-entry palette. Fast enough for
    # 88x88 sheets; reach for a k-d tree only if this ever runs on real textures.
    memo, out = {}, []
    for r, g, b, a in im.convert("RGBA").getdata():
        if a < 128:
            out.append((0, 0, 0, 0))
            continue
        hit = memo.get((r, g, b))
        if hit is None:
            hit = min(pal, key=lambda c: (c[0] - r) ** 2 + (c[1] - g) ** 2 + (c[2] - b) ** 2)
            memo[(r, g, b)] = hit
        out.append(hit + (a,))
    res = Image.new("RGBA", im.size)
    res.putdata(out)
    return res


def snap_dir(src, out_dir, pal):
    out_dir.mkdir(parents=True, exist_ok=True)
    n = 0
    for d in DIRS:
        f = Path(src) / ("%s.png" % d)
        if not f.exists():
            continue
        snap_image(Image.open(f), pal).save(out_dir / f.name)
        n += 1
    return n


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    cover = 0.995
    if argv[0] == "--cover":                 # drop the rare colours (stray leather, trim)
        cover, argv = float(argv[1]), argv[2:]
    ref, family, srcs = argv[0], argv[1], argv[2:]
    pal = palette_of(ref, cover)
    print("reference palette: %d colours from %s" % (len(pal), ref))
    for s in srcs:
        s = Path(s)
        slug = s.parent.name if s.name == "rotations" else s.name
        out = RENDERS / family / "raw" / slug / "rotations"
        print("%s -> %s (%d frames)" % (slug, out.relative_to(REPO), snap_dir(s, out, pal)))


def demo():
    pal = [(0, 0, 0), (200, 200, 200)]
    im = Image.new("RGBA", (2, 1))
    im.putdata([(10, 10, 10, 255), (180, 190, 200, 40)])
    got = list(snap_image(im, pal).getdata())
    assert got[0] == (0, 0, 0, 255), got          # snapped to the near colour
    assert got[1] == (0, 0, 0, 0), got            # transparent stays transparent
    print("ok")


if __name__ == "__main__":
    (demo if sys.argv[1:2] == ["--demo"] else lambda: main(sys.argv[1:]))()
