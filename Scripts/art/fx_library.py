"""Procedural slash / attack fx clips in the fx-slash style: 3-tone strokes, bright leading edge,
sparkle dissolve. Zero credits. Writes RawArt/Renders/fx-slash/raw/<name>/frames/frame_N.png,
which godot_pack.py rolls into the atlas as fx_<name> and the forge game page lists as a slash pick.

    py Scripts/art/fx_library.py            # write every clip in LIBRARY
    py Scripts/art/fx_library.py spin burst  # just these
    py Scripts/art/fx_library.py --sheet     # also write a contact sheet to RawArt/Renders/fx-slash/library.png
"""
import math
import os
import random
import sys

from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(REPO, "RawArt", "Renders", "fx-slash", "raw")
S = 64
GOLD = ((214, 120, 60), (255, 214, 120), (255, 250, 225))    # dark, mid, light — sampled from chop/sweep
BLUE = ((60, 120, 214), (120, 214, 255), (225, 250, 255))
RED = ((160, 40, 50), (240, 90, 80), (255, 220, 200))
GREEN = ((40, 130, 70), (110, 230, 120), (225, 255, 225))


def _put(px, x, y, c):
    if 0 <= x < S and 0 <= y < S:
        px[x, y] = c + (255,)


def arc(frames, cx, cy, r, a0, a1, thick=4, tail=0.55, pal=GOLD, phase=0.0):
    """A crescent whose head sweeps a0 -> a1 (radians) across `frames`, trailing `tail` of the span.
    Drawn by scanning pixels so thin curves stay crisp. Last quarter of the clip dissolves to sparkles."""
    n = len(frames)
    live = max(2, int(n * 0.72))
    for f, im in enumerate(frames):
        px = im.load()
        t = min(1.0, (f + 1) / live) if f < live else 1.0
        head = a0 + (a1 - a0) * t
        lo = head - (a1 - a0) * tail * (1.0 if f < live else 1.0 - (f - live + 1) / (n - live + 1))
        fade = f >= live
        for y in range(S):
            for x in range(S):
                dx, dy = x + 0.5 - cx, y + 0.5 - cy
                d = math.hypot(dx, dy)
                if abs(d - r) > thick / 2.0:
                    continue
                a = math.atan2(dy, dx)
                # unwrap into the sweep's frame of reference
                while a < min(a0, a1) - 1e-6:
                    a += math.tau
                while a > max(a0, a1) + 1e-6:
                    a -= math.tau
                if not (min(lo, head) <= a <= max(lo, head)):
                    continue
                if fade and random.random() < 0.7:
                    continue
                near_head = abs(head - a) < abs(a1 - a0) * 0.12
                c = pal[2] if (d < r - thick / 2.0 + 1.0 or near_head) else (pal[1] if d < r + thick / 2.0 - 1.0 else pal[0])
                _put(px, x, y, c)
        if f >= live - 1:
            sparkles(im, cx + r * math.cos(head), cy + r * math.sin(head), 4 + (f - live + 1) * 3, pal)


def sparkles(im, x, y, n, pal=GOLD, spread=10):
    px = im.load()
    for _ in range(n):
        sx, sy = int(x + random.uniform(-spread, spread)), int(y + random.uniform(-spread, spread))
        _put(px, sx, sy, pal[2])
        if random.random() < 0.4:
            for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                _put(px, sx + ox, sy + oy, pal[1])


def stab(frames, x0, y0, x1, y1, width=3, pal=GOLD, lines=True):
    """A thrust: a needle grows x0,y0 -> x1,y1 over the first half, hangs, then breaks into sparkles."""
    n = len(frames)
    ux, uy = x1 - x0, y1 - y0
    L = math.hypot(ux, uy)
    ux, uy = ux / L, uy / L
    for f, im in enumerate(frames):
        px = im.load()
        t = min(1.0, (f + 1) / (n * 0.45))
        fade = f >= n - 2
        for y in range(S):
            for x in range(S):
                dx, dy = x + 0.5 - x0, y + 0.5 - y0
                along = dx * ux + dy * uy
                side = abs(dx * uy - dy * ux)
                if along < 0 or along > L * t:
                    continue
                w = width * (1.0 - along / L) + 0.6            # tapers to the tip
                if side > w / 2.0:
                    if lines and f >= 1 and not fade and 2.2 < side < 3.2 and along < L * t * 0.6:
                        _put(px, x, y, pal[0])
                    continue
                if fade and random.random() < 0.75:
                    continue
                c = pal[2] if side < w / 2.0 - 1.0 or along > L * t - 4 else pal[1]
                _put(px, x, y, c)
        if f >= n - 2:
            sparkles(im, x0 + ux * L * t, y0 + uy * L * t, 6, pal, 6)


def burst(frames, cx, cy, rays=8, rmax=26, pal=GOLD):
    """Impact star: rays shoot out, hollow ring follows, sparkle out."""
    n = len(frames)
    for f, im in enumerate(frames):
        px = im.load()
        t = (f + 1) / n
        r_in, r_out = 3 + rmax * max(0.0, t - 0.35) * 1.6, rmax * min(1.0, t * 1.6)
        for i in range(rays):
            a = i * math.tau / rays + (0.2 if i % 2 else 0.0)
            for d in range(int(r_in), int(r_out)):
                c = pal[2] if d > r_out - 4 else (pal[1] if i % 2 == 0 else pal[0])
                _put(px, int(cx + math.cos(a) * d), int(cy + math.sin(a) * d), c)
        if f == 0:
            for y in range(S):
                for x in range(S):
                    if math.hypot(x - cx, y - cy) < 4:
                        _put(px, x, y, pal[2])
        if f >= n - 3:
            sparkles(im, cx, cy, 10, pal, rmax)


def new(n):
    return [Image.new("RGBA", (S, S)) for _ in range(n)]


def compose(*clips):
    """Overlay clips frame by frame; shorter ones sit at the front."""
    n = max(len(c) for c in clips)
    out = new(n)
    for c in clips:
        for i, im in enumerate(c):
            out[i].alpha_composite(im)
    return out


def delayed(clip, by):
    return new(by) + clip


# ---- the library: name -> builder returning a frame list ------------------------------------
def _arc_clip(n, *a, **k):
    fr = new(n)
    arc(fr, *a, **k)
    return fr


def _stab_clip(n, *a, **k):
    fr = new(n)
    stab(fr, *a, **k)
    return fr


def _burst_clip(n, *a, **k):
    fr = new(n)
    burst(fr, *a, **k)
    return fr


D = math.radians
LIBRARY = {
    # crescents
    "slash_diag":   lambda: _arc_clip(8, 32, 32, 19, D(-60), D(120)),                        # top-right to bottom-left
    "slash_diag_r": lambda: _arc_clip(8, 32, 32, 19, D(240), D(60)),                         # mirror
    "uppercut":     lambda: _arc_clip(8, 32, 34, 20, D(80), D(-80), thick=3),               # rises bottom to top
    "cleave_heavy": lambda: _arc_clip(9, 32, 40, 24, D(200), D(340), thick=5, tail=0.7),    # wide thick overhead
    "spin":         lambda: _arc_clip(10, 32, 32, 19, D(-90), D(270), tail=0.45),            # full turn
    "spin_blue":    lambda: _arc_clip(10, 32, 32, 19, D(-90), D(270), tail=0.45, pal=BLUE),
    "double_sweep": lambda: compose(_arc_clip(8, 32, 28, 19, D(200), D(340)),
                                    delayed(_arc_clip(8, 32, 40, 19, D(340), D(200)), 2)),
    "slash_x":      lambda: compose(_arc_clip(8, 32, 32, 19, D(-60), D(120), thick=3),
                                    delayed(_arc_clip(8, 32, 32, 19, D(240), D(60), thick=3), 3)),
    "flurry":       lambda: compose(_arc_clip(6, 24, 26, 14, D(-60), D(120), thick=3),
                                    delayed(_arc_clip(6, 40, 30, 14, D(240), D(60), thick=3), 2),
                                    delayed(_arc_clip(6, 32, 40, 14, D(-60), D(120), thick=3), 4)),
    "sweep_red":    lambda: _arc_clip(8, 32, 32, 22, D(200), D(340), pal=RED),
    "sweep_green":  lambda: _arc_clip(8, 32, 32, 22, D(200), D(340), pal=GREEN),
    # thrusts
    "stab_up":      lambda: _stab_clip(6, 32, 58, 32, 8, width=4),
    "stab_diag":    lambda: _stab_clip(6, 10, 54, 54, 10, width=4),
    "stab_triple":  lambda: compose(_stab_clip(6, 32, 60, 32, 6, width=3),
                                    delayed(_stab_clip(6, 22, 60, 12, 12, width=3), 1),
                                    delayed(_stab_clip(6, 42, 60, 52, 12, width=3), 1)),
    "pierce":       lambda: _stab_clip(6, 2, 32, 62, 32, width=3),
    "stab_blue":    lambda: _stab_clip(6, 32, 58, 32, 8, width=4, pal=BLUE),
    # impacts
    "burst":        lambda: _burst_clip(7, 32, 32),
    "burst_big":    lambda: _burst_clip(8, 32, 32, rays=12, rmax=30),
    "burst_blue":   lambda: _burst_clip(7, 32, 32, pal=BLUE),
    "burst_red":    lambda: _burst_clip(7, 32, 32, rays=6, pal=RED),
    "crash":        lambda: compose(_arc_clip(8, 32, 40, 24, D(200), D(340), thick=5, tail=0.7),
                                    delayed(_burst_clip(6, 32, 46, rays=6, rmax=18), 4)),
}


# ---- other styles: not glowing strokes ------------------------------------------------------
BONE = (236, 230, 214)
INK = (12, 12, 16)
ASH = ((60, 58, 62), (120, 116, 118), (190, 186, 182))     # dust dither tones
BLOOD = ((70, 10, 14), (140, 18, 24), (200, 40, 40))


def _fill(im, pred, c):
    px = im.load()
    for y in range(S):
        for x in range(S):
            if pred(x + 0.5, y + 0.5):
                _put(px, x, y, c)


def _outline(im, c=INK):
    """1px outline around every opaque pixel (comic/ink look)."""
    px = im.load()
    hits = [(x, y) for y in range(S) for x in range(S) if px[x, y][3]]
    for x, y in hits:
        for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if 0 <= x + ox < S and 0 <= y + oy < S and not px[x + ox, y + oy][3]:
                px[x + ox, y + oy] = c + (255,)


def _unwrap(a, a0, a1):
    while a < min(a0, a1) - 1e-6:
        a += math.tau
    while a > max(a0, a1) + 1e-6:
        a -= math.tau
    return a


def ink_arc(frames, cx, cy, r, a0, a1, thick=4, tail=0.5, fill=BONE, outline=None):
    """Flat one-colour crescent, hard edge, no glow, no sparkles. Fades by thinning, not dissolving."""
    n = len(frames)
    span = a1 - a0
    for f, im in enumerate(frames):
        t = (f + 1) / n
        head = a0 + span * min(1.0, t * 1.35)
        lo = head - span * tail * (1.0 - max(0.0, t - 0.6) / 0.4)
        th = thick * (1.0 - max(0.0, t - 0.5) / 0.5 * 0.8)

        def pred(x, y):
            dx, dy = x - cx, y - cy
            if abs(math.hypot(dx, dy) - r) > th / 2.0:
                return False
            a = _unwrap(math.atan2(dy, dx), a0, a1)
            return min(lo, head) <= a <= max(lo, head)
        _fill(im, pred, fill)
        if outline:
            _outline(im, outline)


def wedge(frames, cx, cy, r, a0, a1, fill=BONE, outline=INK):
    """Solid fan sector (the blade's swept area) that snaps in then shrinks from the trailing edge."""
    n = len(frames)
    span = a1 - a0
    for f, im in enumerate(frames):
        t = (f + 1) / n
        head = a0 + span * min(1.0, t * 2.0)
        lo = a0 + span * max(0.0, (t - 0.4) / 0.6)

        def pred(x, y):
            dx, dy = x - cx, y - cy
            if math.hypot(dx, dy) > r * (1.0 - max(0.0, t - 0.7)):
                return False
            a = _unwrap(math.atan2(dy, dx), a0, a1)
            return min(lo, head) <= a <= max(lo, head)
        _fill(im, pred, fill)
        if outline:
            _outline(im, outline)


def speedlines(frames, x0, y0, x1, y1, count=8, spread=14, fill=BONE):
    """Anime speed lines: parallel 1px streaks along a direction, staggered, that shorten and vanish."""
    n = len(frames)
    ux, uy = x1 - x0, y1 - y0
    L = math.hypot(ux, uy)
    ux, uy = ux / L, uy / L
    nx, ny = -uy, ux
    rng = random.Random(3)
    lanes = [(rng.uniform(-spread, spread), rng.uniform(0.0, 0.25), rng.uniform(0.8, 1.0)) for _ in range(count)]
    for f, im in enumerate(frames):
        px = im.load()
        t = (f + 1) / n
        for off, start, ln in lanes:
            a = max(0.0, t * 1.5 - start - 0.55) * L
            b = min(L, (t * 1.5 - start) * L * ln)
            d = a
            while d < b:
                _put(px, int(x0 + ux * d + nx * off), int(y0 + uy * d + ny * off), fill)
                d += 1.0


def dust(frames, cx, cy, rmax=20, pal=ASH, up=0.6):
    """Impact dust: a dithered puff that billows out and drifts up, then thins to nothing."""
    n = len(frames)
    rng = random.Random(11)
    for f, im in enumerate(frames):
        px = im.load()
        t = (f + 1) / n
        r = rmax * (0.3 + 0.7 * min(1.0, t * 1.4))
        y_c = cy - rmax * up * t
        density = 1.0 - max(0.0, t - 0.45) / 0.55
        for y in range(S):
            for x in range(S):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - y_c) / r
                if d > 1.0:
                    continue
                k = ((x & 1) + (y & 1) * 2)            # ordered dither: solid core, lace at the rim
                want = (1.0 - d) * density * 4.0
                if want > k + rng.uniform(-0.3, 0.3):
                    _put(px, x, y, pal[0] if d < 0.35 else (pal[1] if d < 0.7 else pal[2]))


def shockwave(frames, cx, cy, rmax=28, fill=BONE, thick=2):
    """A single expanding ring, 1-2px, flat. Reads as force rather than a blade."""
    n = len(frames)
    for f, im in enumerate(frames):
        t = (f + 1) / n
        r = 3 + (rmax - 3) * t
        th = thick if t < 0.6 else 1
        _fill(im, lambda x, y: abs(math.hypot(x - cx, y - cy) - r) <= th / 2.0
              and (t < 0.75 or (int(x) + int(y)) & 1), fill)


def chips(frames, cx, cy, count=7, fill=BONE, outline=INK, speed=5.0):
    """Debris: square chips thrown out with gravity, flat with outline."""
    n = len(frames)
    rng = random.Random(5)
    parts = [(rng.uniform(-math.pi, 0), rng.uniform(0.5, 1.0) * speed, rng.choice((2, 2, 3, 3, 4))) for _ in range(count)]
    for f, im in enumerate(frames):
        px = im.load()
        for a, v, sz in parts:
            x = cx + math.cos(a) * v * f
            y = cy + math.sin(a) * v * f + 0.9 * f * f
            if f >= n - 2 and sz < 3:
                continue
            for oy in range(sz):
                for ox in range(sz):
                    _put(px, int(x) + ox, int(y) + oy, fill)
        if outline:
            _outline(im, outline)


def crack(frames, x0, y0, x1, y1, fill=BONE, outline=INK, jag=5, forks=2):
    """Jagged bolt from x0,y0 to x1,y1 with short forks: strikes in one frame, flickers, gone."""
    n = len(frames)
    rng = random.Random(9)
    segs = 7
    nx, ny = -(y1 - y0), (x1 - x0)
    L = math.hypot(nx, ny)
    nx, ny = nx / L, ny / L
    pts = [(x0, y0)]
    for i in range(1, segs):
        t = i / segs
        o = rng.uniform(-jag, jag)
        pts.append((x0 + (x1 - x0) * t + nx * o, y0 + (y1 - y0) * t + ny * o))
    pts.append((x1, y1))
    fk = []
    for _ in range(forks):
        i = rng.randrange(1, segs - 1)
        fk.append((pts[i], (pts[i][0] + rng.uniform(-10, 10), pts[i][1] + rng.uniform(-10, 10))))

    def line(im, a, b):
        px = im.load()
        k_n = max(1, int(math.hypot(b[0] - a[0], b[1] - a[1])))
        for k in range(k_n + 1):
            _put(px, int(a[0] + (b[0] - a[0]) * k / k_n), int(a[1] + (b[1] - a[1]) * k / k_n), fill)
    for f, im in enumerate(frames):
        if f == n - 1 or (f == n - 2 and f % 2):
            continue                      # flicker out
        upto = len(pts) if f > 0 else max(2, int(len(pts) * 0.6))
        for a, b in zip(pts[:upto - 1], pts[1:upto]):
            line(im, a, b)
        if 1 <= f < n - 2:
            for a, b in fk:
                line(im, a, b)
        if outline and f < n - 2:
            _outline(im, outline)


def _clip(n, fn, *a, **k):
    fr = new(n)
    fn(fr, *a, **k)
    return fr


STYLES = {
    # flat ink crescents: hard edge, one colour, thins out instead of sparkling
    "ink_slash":     lambda: _clip(6, ink_arc, 32, 32, 19, D(-60), D(120), thick=4),
    "ink_sweep":     lambda: _clip(7, ink_arc, 32, 30, 22, D(200), D(340), thick=5, tail=0.6),
    "ink_dark":      lambda: _clip(6, ink_arc, 32, 32, 19, D(240), D(60), thick=4, fill=INK),
    # comic: bone fill with ink outline
    "comic_slash":   lambda: _clip(7, ink_arc, 32, 32, 19, D(-60), D(120), thick=5, outline=INK),
    "comic_wedge":   lambda: _clip(7, wedge, 32, 36, 24, D(200), D(340)),
    "comic_wedge_r": lambda: _clip(7, wedge, 32, 36, 24, D(340), D(200)),
    # anime speed lines
    "lines_fwd":     lambda: _clip(6, speedlines, 6, 32, 58, 32),
    "lines_up":      lambda: _clip(6, speedlines, 32, 60, 32, 6, count=6, spread=10),
    "lines_diag":    lambda: _clip(6, speedlines, 8, 56, 56, 8, count=6),
    # impact without a blade
    "dust":          lambda: _clip(8, dust, 32, 44),
    "dust_small":    lambda: _clip(6, dust, 32, 44, rmax=12),
    "shockwave":     lambda: _clip(7, shockwave, 32, 32),
    "shock_ink":     lambda: _clip(7, shockwave, 32, 32, fill=INK, thick=3),
    "chips":         lambda: _clip(8, chips, 32, 40),
    "chips_blood":   lambda: _clip(8, chips, 32, 40, fill=BLOOD[1], outline=BLOOD[0], count=9, speed=4.0),
    "crack":         lambda: _clip(6, crack, 32, 4, 32, 60),
    "crack_diag":    lambda: _clip(6, crack, 6, 8, 58, 56, forks=3),
    # combos in the new styles
    "ink_crash":     lambda: compose(_clip(7, ink_arc, 32, 30, 22, D(200), D(340), thick=5, tail=0.6),
                                     delayed(_clip(6, dust, 32, 46, rmax=14), 3)),
    "comic_hit":     lambda: compose(_clip(7, wedge, 32, 36, 24, D(200), D(340)),
                                     delayed(_clip(7, chips, 32, 44, count=6), 2)),
    "lines_stab":    lambda: compose(_clip(6, speedlines, 6, 32, 58, 32, count=4, spread=6),
                                     delayed(_clip(6, shockwave, 56, 32, rmax=14), 2)),
}
LIBRARY.update(STYLES)


# ---- Judgement Cut (DMC3 Vergil): a sphere of space is sliced many times at once, then shatters ---
YAMATO = ((70, 110, 200), (150, 200, 255), (240, 250, 255))   # dark, mid, white


def _line(px, a, b, c, dash=0):
    L = max(1, int(math.hypot(b[0] - a[0], b[1] - a[1])))
    for k in range(L + 1):
        if dash and (k // dash) % 2:
            continue
        _put(px, int(a[0] + (b[0] - a[0]) * k / L), int(a[1] + (b[1] - a[1]) * k / L), c)


def judgement_cut(frames, cx=32, cy=32, r=22, cuts=7, pal=YAMATO, seed=4, shatter=0.25, sphere_on=True, spread=0.75):
    """0-1 sphere blinks in (dither fill, thin rim). 2..n-4 the cuts: bold white chords land one or two
    per frame and stay -- they are the point of the effect. Then a short, faint shatter: the rim
    breaks into a few arcs that drift out a little and the lines dim, all scaled by `shatter`."""
    n = len(frames)
    rng = random.Random(seed)
    chords = []
    order = list(range(cuts))
    rng.shuffle(order)                                # even angular spacing, random landing order
    for i in order:
        a = (i + 0.5) * math.pi / cuts + rng.uniform(-0.08, 0.08)
        o = rng.uniform(-r * spread, r * spread)
        nx, ny = math.cos(a), math.sin(a)
        half = math.sqrt(max(0.0, r * r - o * o))
        mx, my = cx + nx * o, cy + ny * o
        tx, ty = -ny, nx
        chords.append(((mx - tx * half, my - ty * half), (mx + tx * half, my + ty * half), (nx, ny)))
    cut_end = n - 3                                   # last three frames are the shatter
    arcs = [(rng.uniform(0, math.tau), rng.uniform(0.4, 1.0)) for _ in range(6)]

    def sphere(px, rr, fill):
        if not sphere_on:
            return
        for y in range(S):
            for x in range(S):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if abs(d - rr) < 1.0:
                    _put(px, x, y, pal[1])
                elif fill and d < rr and ((x ^ y) & 3) == 0:
                    _put(px, x, y, pal[0])

    def chord(px, a, b, nrm, c, bold):
        _line(px, a, b, c)
        if bold:                                      # 2px: offset a copy along the chord normal
            _line(px, (a[0] + nrm[0], a[1] + nrm[1]), (b[0] + nrm[0], b[1] + nrm[1]), c)

    for f, im in enumerate(frames):
        px = im.load()
        start = 2 if sphere_on else 0
        if f < start:
            sphere(px, r * (0.6 if f == 0 else 1.0), f == 1)
            continue
        if f < cut_end:
            sphere(px, r, True)
            k = f - start
            steps = cut_end - start
            shown = min(cuts, int(math.ceil(cuts * (k + 1) / steps)))
            prev = min(cuts, int(math.ceil(cuts * k / steps)))
            for i, (a, b, nrm) in enumerate(chords[:shown]):
                fresh = i >= prev
                chord(px, a, b, nrm, pal[2], bold=True)
                if fresh:                             # landing flash: a halo line either side
                    _line(px, (a[0] - nrm[0] * 2, a[1] - nrm[1] * 2), (b[0] - nrm[0] * 2, b[1] - nrm[1] * 2), pal[1])
            continue
        t = (f - cut_end + 1) / 3.0                   # shatter, faint
        gap = 6 * shatter * t
        for a, b, nrm in chords:
            chord(px, a, b, nrm, pal[2] if t < 0.5 else pal[1], bold=t < 0.5)
        if t < 1.0 and sphere_on:
            for ang, spd in arcs:
                ox, oy = math.cos(ang) * gap * spd, math.sin(ang) * gap * spd
                for sdx in range(0, 24):
                    a = ang - 0.3 + sdx * 0.026
                    _put(px, int(cx + ox + math.cos(a) * r), int(cy + oy + math.sin(a) * r), pal[1] if t < 0.5 else pal[0])
        if shatter > 0:
            sparkles(im, cx, cy, int(3 + 6 * shatter * t), pal, r)


def swipes(frames, cx=32, cy=32, r=22, cuts=10, per=2, pal=YAMATO, seed=4, spread=0.25, linger=1):
    """Quick swipes: 1px white cuts land through the centre a few per frame and are gone a frame or
    two later. Successive cuts alternate sides of the centre and lean (left-leaning, then right-
    leaning), lengths and timing jitter, so it never reads as a fan. No sphere, no glow, no shatter."""
    n = len(frames)
    rng = random.Random(seed)
    chords = []
    t = 0.0
    for i in range(cuts):
        side = 1 if i % 2 else -1                         # alternate which side of centre
        a = rng.uniform(0.15, 1.35) + (0.0 if (i // 2) % 2 else math.pi / 2)   # alternate lean
        o = side * rng.uniform(r * spread * 0.3, r * spread)
        nx, ny = math.cos(a), math.sin(a)
        half = math.sqrt(max(0.0, r * r - o * o))
        mx, my = cx + nx * o, cy + ny * o
        tx, ty = -ny, nx
        h0, h1 = half * rng.uniform(0.5, 1.0), half * rng.uniform(0.5, 1.0)   # uneven ends
        born = int(t)
        t += rng.uniform(0.25, 1.0) * 2.0 / per
        chords.append(((mx - tx * h0, my - ty * h0), (mx + tx * h1, my + ty * h1), born, linger + rng.choice((0, 0, 1))))
    for f, im in enumerate(frames):
        px = im.load()
        for a, b, born, life in chords:
            if born <= f <= born + life:
                _line(px, a, b, pal[2])


LIBRARY.update({
    "judgement_cut":     lambda: _clip(6, swipes),
    "judgement_cut_big": lambda: _clip(8, swipes, r=28, cuts=16, seed=9),
    "judgement_cut_gold": lambda: _clip(6, swipes, pal=GOLD, seed=6),
    "judgement_cut_lines": lambda: _clip(8, judgement_cut, sphere_on=False, spread=0.10),
    "judgement_cut_sphere": lambda: _clip(10, judgement_cut, spread=0.4),
})


def write(name, frames):
    d = os.path.join(OUT, name, "frames")
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        os.remove(os.path.join(d, f))
    for i, im in enumerate(frames):
        im.save(os.path.join(d, "frame_%d.png" % i))
    return len(frames)


def sheet(names, path, zoom=2):
    rows = [(n, sorted(os.listdir(os.path.join(OUT, n, "frames")), key=lambda f: int(f[6:-4]))) for n in names]
    cols = max(len(r[1]) for r in rows)
    im = Image.new("RGBA", (cols * S * zoom, len(rows) * S * zoom), (30, 30, 34, 255))
    for r, (n, fs) in enumerate(rows):
        for c, f in enumerate(fs):
            fr = Image.open(os.path.join(OUT, n, "frames", f)).convert("RGBA").resize((S * zoom, S * zoom), Image.NEAREST)
            im.alpha_composite(fr, (c * S * zoom, r * S * zoom))
    im.save(path)


if __name__ == "__main__":
    random.seed(7)
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    names = args or list(LIBRARY)
    for n in names:
        print("%-14s x%d" % (n, write(n, LIBRARY[n]())))
    if "--sheet" in sys.argv:
        allnames = sorted(d for d in os.listdir(OUT) if os.path.isdir(os.path.join(OUT, d, "frames")))
        p = os.path.join(OUT, "..", "library.png")
        sheet(allnames, p)
        print("sheet", os.path.normpath(p), "rows:", " ".join(allnames))
