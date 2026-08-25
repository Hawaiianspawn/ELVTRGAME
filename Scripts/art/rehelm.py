"""Restore the veteran's helmet in attack frames 0-3 of the Selects copy.

    py Scripts/art/rehelm.py

debanner.py stripped the banner pole off the attack clip and took the helmet's right side with
it on frames 0-3 (the pole crossed the helmet there). Frames 4-7 never had the pole over the
helmet, so frame 4's helmet is the donor: aligned on the helmet bbox top-left and pasted over the
target's helmet rows. Originals kept beside as frame_N.prehelm.png.
"""
import os
from PIL import Image

SEL = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                   "RawArt", "Aaron Selects", "Elf Branch", "v8_heavycloak", "attack_north")
DONOR, TARGETS = 4, (0, 1, 2, 3)
HELM_ROWS = range(0, 40)   # helmet dome + collar overlap; the cloak is told apart by colour


def helmet_mask(im):
    px = im.load()
    def metal(c):  # greys (steel, highlight, outline); the cloak is olive (r > g > b)
        r, g, b, a = c
        return a and abs(r - g) < 14 and abs(g - b) < 20 and r + g + b > 120   # not the outline
    pts = {(x, y) for x in range(im.width) for y in HELM_ROWS if metal(px[x, y])}
    # keep only the blob containing the topmost metal pixel: the dome, not the sword or cloak outline
    start = min(pts, key=lambda p: (p[1], p[0]))
    blob, st = {start}, [start]
    while st:
        x, y = st.pop()
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                q = (x + dx, y + dy)
                if q in pts and q not in blob:
                    blob.add(q); st.append(q)
    # grow by one pixel to take the dome's own dark outline with it
    out = set(blob)
    for x, y in blob:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                q = (x + dx, y + dy)
                if 0 <= q[0] < im.width and 0 <= q[1] < im.height and px[q][3] and sum(px[q][:3]) <= 120:
                    out.add(q)
    return out


def bbox(pts):
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    return min(xs), min(ys), max(xs), max(ys)


def main():
    donor = Image.open(os.path.join(SEL, "frame_%d.png" % DONOR)).convert("RGBA")
    dm = helmet_mask(donor)
    dx0, dy0, _, _ = bbox(dm)
    dp = donor.load()
    for i in TARGETS:
        path = os.path.join(SEL, "frame_%d.png" % i)
        im = Image.open(path).convert("RGBA")
        keep = path.replace(".png", ".prehelm.png")
        if not os.path.exists(keep):
            im.save(keep)
        tm = helmet_mask(im)
        tx0, ty0, _, _ = bbox(tm)
        off = (tx0 - dx0, ty0 - dy0)
        px = im.load()
        for p in tm:                         # whole helmet is replaced, not patched: the sheared
            px[p] = (0, 0, 0, 0)             # outline would otherwise show as a step inside the dome
        added = 0
        for (x, y) in dm:
            q = (x + off[0], y + off[1])
            if 0 <= q[0] < im.width and 0 <= q[1] < im.height:
                px[q] = dp[x, y]
                added += 1
        im.save(path)
        print("frame_%d: +%d px (offset %s)" % (i, added, off))


if __name__ == "__main__":
    main()
