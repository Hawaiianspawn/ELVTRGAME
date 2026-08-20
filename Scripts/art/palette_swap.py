"""Recolour one family's rotations onto another family's palette, by luminance.

    py Scripts/art/palette_swap.py <src-family> <palette-family> <out-family> [--warm]

Zero-credit local pass: every opaque source colour maps to the palette colour whose
luminance is nearest, so shading survives and hue moves. Output lands under
RawArt/Renders/<out-family>/raw/<slug>/rotations like any generated variant, so
selects/forge pick it up unchanged. Never touches the source renders.
"""
import sys, glob
from pathlib import Path
from PIL import Image

RENDERS = Path(__file__).resolve().parents[2] / "RawArt" / "Renders"


def luma(c):
    return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]


def palette(family, cool_only):
    cols = set()
    for f in glob.glob(str(RENDERS / family / "raw" / "*" / "rotations" / "*.png")):
        cols.update(p[:3] for p in Image.open(f).convert("RGBA").getdata() if p[3] > 0)
    if cool_only:  # steel/bone ramp: drop warm leather browns so armour does not go tan
        cols = {c for c in cols if c[2] >= c[0]}
    return sorted(cols, key=luma)


def swap(src, pal_family, out, cool_only=True):
    pal = palette(pal_family, cool_only)
    lut = {}
    n = 0
    for f in glob.glob(str(RENDERS / src / "raw" / "*" / "rotations" / "*.png")):
        f = Path(f)
        im = Image.open(f).convert("RGBA")
        px = im.load()
        for y in range(im.height):
            for x in range(im.width):
                r, g, b, a = px[x, y]
                if a == 0:
                    continue
                k = (r, g, b)
                if k not in lut:
                    L = luma(k)
                    lut[k] = min(pal, key=lambda c: abs(luma(c) - L))
                px[x, y] = lut[k] + (a,)
        dest = RENDERS / out / "raw" / f.parents[1].name / "rotations"
        dest.mkdir(parents=True, exist_ok=True)
        im.save(dest / f.name)
        n += 1
    return n


if __name__ == "__main__":
    a = [x for x in sys.argv[1:] if not x.startswith("--")]
    print(swap(a[0], a[1], a[2], cool_only="--warm" not in sys.argv), "frames")
