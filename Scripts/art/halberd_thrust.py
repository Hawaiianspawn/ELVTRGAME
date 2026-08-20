"""Build the halberdier's thrust clip: body turned to north-east, pike in the right hand lowering
from upright toward a forward (up-right, away from camera) level and driving ahead, then recovering.

    py Scripts/art/halberd_thrust.py [out_dir] [--body north-west]

--body swaps the body layer for another rotation (hero pose: north-west, owner 2026-08-20);
the pike layer, grip and PLAN stay exactly as composed from north-east.

PixelLab v3 kept turning this into a sideways sweep (two takes), so the clip is composed
here from the north-east rotation: the pike layer is rotated about the grip and
foreshortened, the body never changes.
"""
import os, sys, math
from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RAW = os.path.join(REPO, "RawArt", "Renders", "melee-tropes", "raw", "halberdier")
SRC = os.path.join(RAW, "rotations", "north-east.png")
ARGS = sys.argv[1:]
BODY = ARGS.pop(ARGS.index("--body") + 1) if "--body" in ARGS else None
if BODY:
    ARGS.remove("--body")
OUT = ARGS[0] if ARGS else os.path.join(RAW, "attack_north" + ("_" + BODY.replace("-", "") if BODY else ""))
# per-rotation pike mask: (x, y) -> True where the pike (head + shaft, not the hand) lives
PIKE_AT = {
    "north-east": lambda x, y: (x >= 53 and y <= 41) or (54 <= x <= 57 and y >= 50),
    "north-west": lambda x, y: (x <= 32 and y <= 44) or (30 <= x <= 32 and y >= 52),
}
OUTLINE = (0, 9, 7, 255)
GRIP = (55.5, 46.0)     # right hand on the shaft; the pike pivots here
# (angle deg CCW toward the far-left, length factor, forward push (dx, dy), behind body) per frame
REST = (0, 1.08, (0, 0), False)        # length >1: shaft stretched 8% about the grip (owner: "a little taller")
EXTENDED = (0, 1.03, (-3, -12), False)     # owner sketch 2026-08-20: tip pokes up and a little left, in front
# Two poses only (owner 2026-08-20): rest and fully extended, no lowering tween. Same 8-frame length.
PLAN = [REST, REST, EXTENDED, EXTENDED, EXTENDED, EXTENDED, REST, REST]


def dark(c):
    return c[3] and sum(c[:3]) < 90


SHAFT_AT = {"north-east": lambda x, y: 54 <= x <= 57, "north-west": lambda x, y: 30 <= x <= 32}


def split(im, pike_at=PIKE_AT["north-east"], shaft_at=SHAFT_AT["north-east"]):
    """(body, pike). Pike = axe head + shaft beside the body; the hand keeps its grip rows."""
    W, H = im.size
    body, pike = im.copy(), Image.new("RGBA", im.size)
    bp, pp, sp = body.load(), pike.load(), im.load()
    for y in range(H):
        for x in range(W):
            if pike_at(x, y):
                pp[x, y] = sp[x, y]
                bp[x, y] = (0, 0, 0, 0)
            elif shaft_at(x, y):            # grip rows: shaft rides with the pike, hand stays on the body
                pp[x, y] = sp[x, y]
    for y in range(H):
        for x in range(W):
            c = bp[x, y]
            if c[3] and not dark(c) and any(
                    0 <= x + dx < W and 0 <= y + dy < H and not bp[x + dx, y + dy][3]
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                bp[x, y] = OUTLINE
    return body, pike


def posed(pike, deg, length, push):
    """Pike rotated `deg` CCW about GRIP, shaft foreshortened to `length`, then pushed."""
    t = math.radians(deg)
    cx, cy = GRIP[0] + push[0], GRIP[1] + push[1]
    # output -> input: undo push, rotate back by -deg, un-foreshorten along the shaft (y axis)
    cs, sn = math.cos(t), math.sin(t)
    # rotation by -deg in image coords (y down): [x', y'] = [cs*x - sn*y, sn*x + cs*y]
    a, b = cs, -sn
    d, e = sn, cs
    d, e = d / length, e / length
    c = GRIP[0] - (a * cx + b * cy)
    f = GRIP[1] - (d * cx + e * cy)
    return pike.transform(pike.size, Image.AFFINE, (a, b, c, d, e, f), Image.NEAREST)


def main():
    body, pike = split(Image.open(SRC).convert("RGBA"))
    if BODY:
        body, _ = split(Image.open(os.path.join(RAW, "rotations", BODY + ".png")).convert("RGBA"), PIKE_AT[BODY], SHAFT_AT[BODY])
    upper = pike.copy()                      # behind the body only the shaft above the grip shows
    upper.paste((0, 0, 0, 0), (0, int(GRIP[1]) + 1, upper.width, upper.height))
    os.makedirs(OUT, exist_ok=True)
    for i, (deg, length, push, behind) in enumerate(PLAN):
        src = upper if behind else pike
        layer = posed(src, deg, length, push) if deg or length != 1 or any(push) else src
        out = Image.new("RGBA", body.size)
        if behind:
            out.alpha_composite(layer); out.alpha_composite(body)
        else:
            out.alpha_composite(body); out.alpha_composite(layer)
        out.save(os.path.join(OUT, "frame_%d.png" % i))
    print(OUT, len(PLAN), "frames")


if __name__ == "__main__":
    main()
