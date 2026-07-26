"""SUPERSEDED 2026-07-25 — do not use. Kept only as a record of how the first sheet
was assembled before the pipeline could express a multi-subject texture.

The swarm sheet is now owned by the /sprite pipeline as a **composite request**:

    docs/data/art/requests/swarm-units.json
    py Scripts/art/pixelpipe.py pack swarm-units      -> RawArt/Sheets/T_Swarm_2bit.png

Use that. It does everything below plus provenance in a manifest, a per-source
anti-wobble bbox, the 48px cell lock, and a report that flags which cells are still
placeholder art. This script writes to a *different* path (RawArt/T_Swarm_2bit.png),
so having both live is how you end up importing the stale one.

---

Composite the swarm sprite sheet (T_Swarm_2bit) from individual pose frames.

Takes 8 PNGs, snaps every pixel to the locked 4-value Demichrome palette, and lays
them out as the 4x2 grid the renderers expect:

    col:    0        1        2       3
          walk0    walk1   ATTACK   HIT      <- row 0 = brood
          walk0    walk1   ATTACK   HIT      <- row 1 = retinue

That grid is mirrored in three other places and none of them can read this file:
`SwarmSheet` in Source/ELVTR/Mass/SwarmFragments.h, the Niagara Sprite Renderer's
"Sub UV" field, and ELVTR/SETUP-EDITOR.md. Change one, change all four.

Usage:
    py Scripts/build_swarm_sheet.py \
        --brood   b_walk0.png b_walk1.png b_attack.png b_hit.png \
        --retinue r_walk0.png r_walk1.png r_attack.png r_hit.png \
        --out RawArt/T_Swarm_2bit.png

Requires Pillow (present: `py -c "import PIL"`).
"""

import argparse
import os
import sys

from PIL import Image

# docs/art/aesthetic-direction.md, locked 2026-07-12. No sprite may introduce a fifth
# value without an explicit owner exception, so this list is the whole gamut.
DARK = (0x21, 0x1E, 0x20)   # outline, void, recess
STEEL = (0x55, 0x55, 0x68)  # armor plate, cloth, hard material
BONE = (0xA0, 0xA0, 0x8B)   # skin, bandage, worn/soft material
PALE = (0xE9, 0xEF, 0xEC)   # RESERVED: eyes, marks, lamps, glyphs only

# Rec.601 luma of each palette entry. Quantising in luminance rather than RGB distance
# is deliberate: the generator returns arbitrary hues, and the palette has no hue channel
# left to preserve (strict global palette), so value is the only thing worth matching.
LUMA = [
    (0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2], c)
    for c in (DARK, STEEL, BONE, PALE)
]

ALPHA_CUTOFF = 128  # M_Swarm is a Masked material — alpha is binary, not a gradient.


def quantise(img, allow_pale):
    """Snap to the 4-value palette. Alpha becomes strictly 0 or 255.

    allow_pale=False collapses the brightest bucket into Bone. That is not a stylistic
    nicety: docs/art/npc-silhouette-brief.md reserves Pale for eyes/marks/lamps and
    states a militia unit at rest carries ZERO Pale pixels. A naive luminance quantise
    would spend Pale on ordinary armour highlights and break the one channel that tells
    a friendly unit apart from a Quiet creature's eye-dots.
    """
    img = img.convert("RGBA")
    px = img.load()
    w, h = img.size

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < ALPHA_CUTOFF:
                px[x, y] = (0, 0, 0, 0)
                continue
            luma = 0.299 * r + 0.587 * g + 0.114 * b
            best = min(LUMA, key=lambda entry: abs(entry[0] - luma))[1]
            if not allow_pale and best == PALE:
                best = BONE
            px[x, y] = (best[0], best[1], best[2], 255)
    return img


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--brood", nargs=4, required=True,
                    metavar=("WALK0", "WALK1", "ATTACK", "HIT"))
    ap.add_argument("--retinue", nargs=4, required=True,
                    metavar=("WALK0", "WALK1", "ATTACK", "HIT"))
    ap.add_argument("--out", required=True)
    ap.add_argument("--cell", type=int, default=0,
                    help="Cell size in px. 0 = use the first frame's size (frames must "
                         "all match). PixelLab returns a canvas ~40%% larger than the "
                         "nominal character so limbs have room during a swing — keep "
                         "that canvas rather than cropping, or the attack pose clips.")
    args = ap.parse_args()

    rows = [args.brood, args.retinue]  # row 0 = brood, row 1 = retinue (TeamBit)

    for path in args.brood + args.retinue:
        if not os.path.isfile(path):
            sys.exit("missing frame: %s" % path)

    first = Image.open(rows[0][0])
    cell = args.cell or first.size[0]
    if first.size[0] != first.size[1]:
        sys.exit("frames must be square, got %dx%d" % first.size)

    sheet = Image.new("RGBA", (cell * 4, cell * 2), (0, 0, 0, 0))

    for row_index, frames in enumerate(rows):
        # Brood keep Pale so their eye-dots survive — that is the entire tell for the
        # "amorphous dark with something looking out of it" read. Retinue lose it.
        allow_pale = (row_index == 0)
        for col_index, path in enumerate(frames):
            frame = Image.open(path)
            if frame.size != (cell, cell):
                frame = frame.resize((cell, cell), Image.NEAREST)  # never smooth pixel art
            sheet.paste(quantise(frame, allow_pale), (col_index * cell, row_index * cell))

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    sheet.save(args.out)
    print("wrote %s  (%dx%d, %dpx cells, 4x2 grid)"
          % (args.out, sheet.size[0], sheet.size[1], cell))


if __name__ == "__main__":
    main()
