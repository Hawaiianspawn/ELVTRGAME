"""Strip the back-banner (pole + yellow pennant) from the veteran's rotations and attack clip.

    py Scripts/art/debanner.py <src dir> <out dir>

Colour-keys the pennant/gold pole, drops the thin pole fragments it leaves behind, then fills
any hole punched through the helmet from the same row's neighbours. Raw PixelLab frames are
never touched: write to a Selects copy and repoint selects.json at it.
"""
import sys, os, glob
from PIL import Image, ImageFilter

N8 = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy]
N4 = [(1, 0), (-1, 0), (0, 1), (0, -1)]
YLIM = 42   # banner lives in the top half; the sword hilt below is the same gold


def comps(pts):
    pts = set(pts); seen = set(); res = []
    for s in pts:
        if s in seen:
            continue
        st = [s]; seen.add(s); c = []
        while st:
            p = st.pop(); c.append(p)
            for dx, dy in N8:
                q = (p[0] + dx, p[1] + dy)
                if q in pts and q not in seen:
                    seen.add(q); st.append(q)
        res.append(c)
    return res


TAN = False   # attack clip only: the pole over the helmet is skin-toned, and no face shows facing north


def gold(c):
    r, g, b, a = c
    if brown(c):
        return True
    if TAN and a and r > 160 and g > 110 and b > 80 and r - g > 40 and g - b > 25:
        return True
    return a and r > 110 and g > 85 and b < 80 and r - b > 60 and r >= g > b


def brown(c):
    return c[3] and c[:3] in ((53, 42, 31), (102, 67, 53))


def dark(c):
    return c[3] and sum(c[:3]) < 80


def clean(im):
    W, H = im.size
    px = im.load()

    def nb(p, n=N8):
        return [(p[0] + dx, p[1] + dy) for dx, dy in n if 0 <= p[0] + dx < W and 0 <= p[1] + dy < H]

    alpha0 = {(x, y) for x in range(W) for y in range(H) if px[x, y][3]}
    ys = {(x, y) for x in range(W) for y in range(YLIM) if gold(px[x, y])}
    kill = set(ys)
    for p in ys:
        for q in nb(p):
            if not dark(px[q]) or q in kill:
                continue
            edge = any(r not in alpha0 for r in nb(q, N4))
            body = any(px[r][3] and not dark(px[r]) and not gold(px[r]) for r in nb(q))
            if (edge and not body) or not edge:
                kill.add(q)
    for p in kill:
        px[p] = (0, 0, 0, 0)
    for _ in range(4):
        a = im.split()[3].point(lambda v: 255 if v else 0)
        core = a.filter(ImageFilter.MinFilter(3)).filter(ImageFilter.MaxFilter(3)).load()
        frag = [(x, y) for x in range(W) for y in range(YLIM) if px[x, y][3] and not core[x, y]]
        allc = comps([(x, y) for x in range(W) for y in range(H) if px[x, y][3]])
        big = max(len(c) for c in allc)
        killed = 0
        for c in allc:
            if len(c) < big and len(c) < 80 and min(y for _, y in c) < YLIM:
                for p in c:
                    px[p] = (0, 0, 0, 0); kill.add(p); killed += 1
        for c in comps(frag):
            if any(brown(px[p]) for p in c) or any(q in kill for p in c for q in nb(p)):
                for p in c:
                    px[p] = (0, 0, 0, 0); kill.add(p); killed += 1
        if not killed:
            break
    # Spear tip: a wider stub left where the pole met the helmet. Only stubs touching the cut go.
    a = im.split()[3].point(lambda v: 255 if v else 0)
    core5 = a.filter(ImageFilter.MinFilter(5)).filter(ImageFilter.MaxFilter(5)).load()
    frag = [(x, y) for x in range(W) for y in range(YLIM) if px[x, y][3] and not core5[x, y]]
    for c in comps(frag):
        if len(c) < 40 and any(q in kill for p in c for q in nb(p)):
            for p in c:
                px[p] = (0, 0, 0, 0); kill.add(p)
    # Fill holes the pole punched through the body: a killed pixel enclosed on all four sides.
    for y in range(YLIM + 2):
        for x in range(W):
            if px[x, y][3] or (x, y) not in kill:
                continue
            def walk(dx, dy):
                q = (x + dx, y + dy)
                while 0 <= q[0] < W and 0 <= q[1] < H and not px[q][3]:
                    if q not in kill:
                        return None
                    q = (q[0] + dx, q[1] + dy)
                return q if 0 <= q[0] < W and 0 <= q[1] < H else None
            sides = [walk(*d) for d in N4]
            if all(sides):
                l, r = px[sides[1]], px[sides[0]]
                src = l if dark(r) else r if dark(l) else None
                px[x, y] = src or tuple((a + b) // 2 for a, b in zip(l, r))
    return im


def main(src, out):
    for f in sorted(glob.glob(os.path.join(src, "rotations", "*.png")) + glob.glob(os.path.join(src, "attack_*", "*.png"))):
        rel = os.path.relpath(f, src)
        global TAN
        TAN = rel.startswith("attack")
        os.makedirs(os.path.dirname(os.path.join(out, rel)), exist_ok=True)
        clean(Image.open(f).convert("RGBA")).save(os.path.join(out, rel))
        print(rel)


if __name__ == "__main__":
    main(*sys.argv[1:3])
