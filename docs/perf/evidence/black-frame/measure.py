"""Luminance of the styled game frame, broken out by region.

    py docs/perf/evidence/black-frame/measure.py [name ...]      # table
    py docs/perf/evidence/black-frame/measure.py --masks <name>  # write <name>_masks.png

Regions, and how each one is found without a second reference capture:

  HUD          the combat HUD panel is a flat, bright UMG block. Excluded from every
               other region rather than turned off -- Kindled.Cam.HudBias frames the
               hero inside the strip the HUD leaves, so hiding it would move the
               camera and make before/after captures incomparable.
  upper        everything above the ground horizon. The horizon is found as the row
               where the mean luminance steps up, not assumed.
  mid / near   the ground, split in half by screen row. "near" is the bottom half,
               nearest the camera, where the flame pool is brightest.
  retinue      MEASURED, not guessed: the dithered floor is exactly neutral -- max(RGB)
               minus min(RGB) is 0.00 over every floor pixel in every capture -- so a
               pixel carrying any chroma at all is sprite art. Soldier armour, cloth and
               skin all carry chroma; the floor and the flame pool never do.
  brood        the brood have no chroma either (near-black art plus a NEUTRAL additive
               lift), so chroma cannot see them. They are found as holes in the dither
               instead: every floor pixel has a bright Bayer neighbour within 5px, and a
               brood body does not. Both masks are written out by --masks so the split
               can be checked by eye rather than trusted.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

HERE = Path(__file__).resolve().parent
BRIGHT = 60.0      # a floor pixel always has a Bayer neighbour at least this bright...
NEAR = 5           # ...within this radius


def _f(mask_or_lum, size, op):
    """MaxFilter / MinFilter over a 2D float or bool array, via PIL (no scipy here)."""
    im = Image.fromarray(np.asarray(mask_or_lum, float).clip(0, 255).astype(np.uint8), "L")
    return np.asarray(im.filter(op(size))).astype(float)


def dilate(m, size):
    return _f(m.astype(float) * 255, size, ImageFilter.MaxFilter) > 127


def erode(m, size):
    return _f(m.astype(float) * 255, size, ImageFilter.MinFilter) > 127


def opening(m, size):
    return dilate(erode(m, size), size)


def peak_filter(lum, size):
    return _f(lum, size, ImageFilter.MaxFilter)


def load(name):
    a = np.asarray(Image.open(HERE / f"{name}.png").convert("RGB")).astype(float)
    lum = 0.2126 * a[..., 0] + 0.7152 * a[..., 1] + 0.0722 * a[..., 2]
    return a, lum


def masks(a, lum):
    h, w = lum.shape
    chroma = a.max(2) - a.min(2)

    # HUD: a solid bright block, so it survives an opening that erodes sprite-sized detail.
    # Taken as the BOUNDING BOX of what survives -- the panel's own dark interior cells are
    # as flat as a brood body and would otherwise be counted as one.
    core = opening(lum > 110, 9)
    hud = np.zeros((h, w), bool)
    if core.any():
        r, c = np.where(core)
        hud[r.min():r.max() + 1, c.min():c.max() + 1] = True
    retinue = (chroma > 8) & ~hud
    horizon = ground_row(lum, hud)

    # A brood body is FLAT: the light model hands it one neutral value across the whole
    # body. Lit GROUND is never flat -- the Bayer dither keeps a local range everywhere
    # the pool reaches -- so flatness separates body from floor at every brightness, which
    # a luminance threshold cannot do once the floor gets as dark as the art.
    rng = _f(lum, 5, ImageFilter.MaxFilter) - _f(lum, 5, ImageFilter.MinFilter)
    ground = np.zeros((h, w), bool)
    ground[horizon:] = True          # above the horizon there is no ground and no dither:
    brood = (dilate(opening((rng < 3) & (chroma <= 8) & ~hud & ground, 3), 3)
             & (chroma <= 8) & ~hud & ground)

    upper = ~ground & ~hud & ~retinue
    mid = np.zeros((h, w), bool)
    mid[horizon:horizon + (h - horizon) // 2] = True
    near = np.zeros((h, w), bool)
    near[horizon + (h - horizon) // 2:] = True
    floor = ~hud & ~retinue & ~brood
    return {"upper frame (above horizon)": upper,
            "mid floor": mid & floor,
            "near floor": near & floor,
            "retinue body": retinue,
            "brood body": brood}, horizon, hud


def ground_row(lum, hud):
    """First row where the ground starts: the biggest step up in row-mean luminance.

    Not assumed: the shipped camera is Kindled.Cam.Pitch -8.2 at Fov 45.6, so a large
    slice of the frame is above the horizon and carries no ground at all.
    """
    rows = np.where(hud.any(axis=1), np.nan, lum.mean(axis=1))
    prof = np.nan_to_num(rows, nan=np.nanmean(rows))
    step = prof[1:] - prof[:-1]
    return int(np.argmax(step[: len(step) // 2])) + 1


def stats(lum, m):
    v = lum[m]
    if v.size == 0:
        return None
    return dict(px=v.size, mean=v.mean(), p99=np.percentile(v, 99),
                zero=(v == 0).mean() * 100, sat=(v >= 254).mean() * 100)


def table(names):
    print(f"{'capture':<22}{'region':<28}{'px':>9}{'mean':>8}{'p99':>8}"
          f"{'%at 0':>8}{'%at 255':>9}")
    for n in names:
        a, lum = load(n)
        reg, horizon, _ = masks(a, lum)
        first = True
        for label, m in reg.items():
            s = stats(lum, m)
            if not s:
                continue
            print(f"{(n if first else ''):<22}{label:<28}{s['px']:>9}{s['mean']:>8.1f}"
                  f"{s['p99']:>8.1f}{s['zero']:>8.1f}{s['sat']:>9.2f}")
            first = False
        print(f"{'':<22}{'(horizon at row)':<28}{horizon:>9}"
              f"{horizon / lum.shape[0] * 100:>8.1f}%")


def write_masks(name):
    a, lum = load(name)
    reg, horizon, hud = masks(a, lum)
    out = np.dstack([lum] * 3)
    out[reg["retinue body"]] = (0, 200, 0)
    out[reg["brood body"]] = (220, 0, 0)
    out[hud] = (0, 0, 160)
    out[horizon, :] = (255, 255, 0)
    Image.fromarray(out.astype(np.uint8)).save(HERE / f"{name}_masks.png")
    print(f"{name}_masks.png  green=retinue red=brood blue=HUD yellow=horizon(row {horizon})")


if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "--masks":
        for n in args[1:]:
            write_masks(n)
    else:
        table(args or sorted(p.stem for p in HERE.glob("*.png")
                             if not p.stem.startswith("_") and "_masks" not in p.stem))
