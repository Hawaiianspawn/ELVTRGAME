"""Split a PixelLab UI kit sheet into its pieces.

    py Scripts/art/ui_crop.py <sheet.png> <out_dir> [min_px=12]

Pieces sit on a loose grid separated by fully transparent rows/columns, so a
row-band then column-band projection split finds them without scipy. Writes
<out_dir>/<stem>_NN.png (row-major) and prints "NN x y w h" per piece.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def bands(mask_1d: np.ndarray, min_len: int) -> list[tuple[int, int]]:
    out, start = [], None
    for i, v in enumerate(mask_1d):
        if v and start is None:
            start = i
        elif not v and start is not None:
            if i - start >= min_len:
                out.append((start, i))
            start = None
    if start is not None and len(mask_1d) - start >= min_len:
        out.append((start, len(mask_1d)))
    return out


def main() -> None:
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    min_px = int(sys.argv[3]) if len(sys.argv) > 3 else 12
    out.mkdir(parents=True, exist_ok=True)
    im = Image.open(src).convert("RGBA")
    a = np.array(im)[:, :, 3] > 0
    k = 0
    for y0, y1 in bands(a.any(axis=1), min_px):
        for x0, x1 in bands(a[y0:y1].any(axis=0), min_px):
            piece = im.crop((x0, y0, x1, y1))
            bb = piece.getbbox()
            piece = piece.crop(bb)
            piece.save(out / f"{src.stem}_{k:02d}.png")
            print(f"{k:02d} {x0 + bb[0]} {y0 + bb[1]} {piece.width} {piece.height}")
            k += 1


if __name__ == "__main__":
    main()
