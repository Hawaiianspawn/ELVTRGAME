"""Draw Vanguard character states as ASCII pixel maps in the demichrome-4 legend.

WHY THIS EXISTS
---------------
`pixelpipe.py authored` renders a fenced ASCII block from an art spec straight to a
sprite: on-palette by construction, zero PixelLab generations. The catch is authoring the
block. A 56x60 grid is 3,360 characters that must hold exact column alignment, and
`find_pixel_blocks` silently discards a block containing ANY character outside the legend
-- one stray space loses the whole sprite. Hand-typing that is not reliable.

So the state axis is expressed as parameters instead, and the grid is drawn. Everything
the art spec's metaprompt lists as HELD CONSTANT comes from one shared body function, and
each state differs by exactly the four crown variables the metaprompt allows:

    apex_rows    how far the crown apex rises above the brim
    apex_w       width of the crown apex (taper)
    brim_w       width of the brim
    visor        open (Bone face patch) or barred (Dark bar)

Guaranteed by construction, not by review:
  * every row is exactly `W` characters, drawn from the legend only
  * the outline is unbroken -- it is derived by dilating the filled mask, so it cannot leak
  * values are Dark/Steel/Bone only; Pale is never spent (palette.json shape_carriers
    reserves it for the hero banner and the other three classes' marks)
  * alpha is binary because every cell is either a legend value or transparent

Run:  py Scripts/art/authored_states.py --emit    # print blocks
      py Scripts/art/authored_states.py --write docs/art/<spec>.md
"""
import argparse
import sys

W, H = 56, 60

DARK, STEEL, BONE, EMPTY = "#", "S", "b", "."

# ---------------------------------------------------------------- the state axis
# Four independent crown variables, one per state, per the spec's metaprompt.
# 01 is the unmarked baseline every other state is measured against.
STATES = [
    {"id": "01", "name": "plain dome, open face (baseline)",
     "apex_rows": 4, "apex_w": 9,  "brim_w": 15, "visor": "open",  "crest": False},
    {"id": "02", "name": "face barred by a visor",
     "apex_rows": 4, "apex_w": 9,  "brim_w": 15, "visor": "barred", "crest": False},
    {"id": "03", "name": "tall narrow peak",
     "apex_rows": 9, "apex_w": 5,  "brim_w": 15, "visor": "open",  "crest": False},
    {"id": "04", "name": "wide bulb and broader brim",
     "apex_rows": 4, "apex_w": 13, "brim_w": 19, "visor": "open",  "crest": False},
    {"id": "05", "name": "baseline dome plus a thin crest (the marginal one)",
     "apex_rows": 4, "apex_w": 9,  "brim_w": 15, "visor": "open",  "crest": True},
]


class Canvas:
    def __init__(self):
        self.g = [[EMPTY] * W for _ in range(H)]

    def rect(self, cx, w, y0, y1, ch):
        """Filled rect of width w centred on cx, rows y0..y1 inclusive."""
        x0 = cx - w // 2
        for y in range(y0, y1 + 1):
            for x in range(x0, x0 + w):
                if 0 <= y < H and 0 <= x < W:
                    self.g[y][x] = ch

    def taper(self, cx, w_top, w_bot, y0, y1, ch):
        """Rows y0..y1 widening linearly from w_top to w_bot. Draws the crown."""
        span = max(1, y1 - y0)
        for y in range(y0, y1 + 1):
            t = (y - y0) / span
            w = round(w_top + (w_bot - w_top) * t)
            w = max(1, w)
            self.rect(cx, w, y, y, ch)

    def outline(self):
        """Dilate the filled mask by one pixel of Dark. Cannot leave a gap."""
        filled = [[c != EMPTY for c in row] for row in self.g]
        edge = []
        for y in range(H):
            for x in range(W):
                if filled[y][x]:
                    continue
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < H and 0 <= nx < W and filled[ny][nx]:
                        edge.append((y, x))
                        break
        for y, x in edge:
            self.g[y][x] = DARK

    def rows(self):
        return ["".join(r) for r in self.g]


def draw(st):
    """One state. Body geometry is identical across states by construction --
    only the crown block reads anything out of `st`."""
    c = Canvas()
    cx = W // 2

    # ---- constants (the metaprompt's held-constant list) ------------------
    BRIM_Y = 18          # the brim row -- the crown's baseline, fixed for all states
    FACE_Y0, FACE_Y1 = 19, 22
    SHOULDER_Y0, SHOULDER_Y1 = 24, 26
    TORSO_Y0, TORSO_Y1 = 27, 38
    BELT_Y = 34
    SKIRT_Y0, SKIRT_Y1 = 39, 43
    LEG_Y0, LEG_Y1 = 44, 50
    BOOT_Y = 51
    SHOULDER_W, TORSO_W, SKIRT_W = 19, 15, 17

    # ---- crown: the ONLY thing the state axis touches --------------------
    apex_y = BRIM_Y - st["apex_rows"]
    c.taper(cx, st["apex_w"], st["brim_w"] - 2, apex_y, BRIM_Y - 1, STEEL)
    c.rect(cx, st["brim_w"], BRIM_Y, BRIM_Y, STEEL)
    if st["crest"]:
        # a decoration on top of an existing shape, not a new shape -- state 05's
        # whole point, and why the spec calls it marginal
        c.rect(cx, 1, apex_y - 3, apex_y - 1, STEEL)

    # ---- face band -------------------------------------------------------
    if st["visor"] == "barred":
        c.rect(cx, st["brim_w"] - 4, FACE_Y0, FACE_Y1, STEEL)
    else:
        c.rect(cx, st["brim_w"] - 4, FACE_Y0, FACE_Y1, BONE)

    # ---- body ------------------------------------------------------------
    c.rect(cx, SHOULDER_W, SHOULDER_Y0, SHOULDER_Y1, STEEL)
    c.rect(cx, TORSO_W, TORSO_Y0, TORSO_Y1, STEEL)
    c.rect(cx, SKIRT_W, SKIRT_Y0, SKIRT_Y1, STEEL)
    c.rect(cx, TORSO_W - 2, LEG_Y0, LEG_Y1, STEEL)
    c.rect(cx, TORSO_W - 1, BOOT_Y, BOOT_Y, STEEL)

    # ---- shield: Vanguard's registered rectangle, left of the body -------
    c.rect(cx - 12, 7, TORSO_Y0 + 1, TORSO_Y1 - 1, STEEL)

    # ---- spear: dead vertical, right of the body ------------------------
    c.rect(cx + 12, 2, 8, LEG_Y1, STEEL)
    # a 3px bridge at chest height, so the spear reads as GRIPPED rather than as a
    # pole standing beside the soldier
    c.rect(cx + 9, 5, TORSO_Y0 + 4, TORSO_Y0 + 5, STEEL)

    c.outline()

    # ---- interior marks, inside the fill so the outline stays intact ----
    if st["visor"] == "barred":
        # 2 rows only -- leaving Steel above and below keeps the crown attached to the
        # shoulders. A full-height bar detaches the head into a floating hat.
        c.rect(cx, st["brim_w"] - 4, FACE_Y0 + 1, FACE_Y0 + 2, DARK)
    else:
        for dx in (-3, 2):                       # two Dark eye recesses, never Pale
            c.rect(cx + dx, 2, FACE_Y0 + 1, FACE_Y0 + 2, DARK)
    c.rect(cx, TORSO_W, BELT_Y, BELT_Y, BONE)    # the single Bone belt band
    c.rect(cx - 12, 5, TORSO_Y0 + 6, TORSO_Y0 + 7, BONE)   # shield's single Bone band
    # the leg split: 2px of Dark, so two Steel greaves read as legs. A wide split
    # turns the whole lower body into a black void.
    c.rect(cx, 2, LEG_Y0, LEG_Y1, DARK)
    c.rect(cx + 12, 2, 8, 9, BONE)               # flared spear tip

    return c.rows()


def check(rows, st):
    """Fail loudly rather than let pixelpipe silently discard a block."""
    legend = set(".#Sb@")
    errs = []
    if len(rows) != H:
        errs.append("height %d != %d" % (len(rows), H))
    for i, r in enumerate(rows):
        if len(r) != W:
            errs.append("row %d width %d != %d" % (i, len(r), W))
        bad = set(r) - legend
        if bad:
            errs.append("row %d has non-legend %r" % (i, sorted(bad)))
    if "@" in "".join(rows):
        errs.append("spends Pale -- reserved by shape_carriers")
    if errs:
        raise SystemExit("state %s INVALID:\n  %s" % (st["id"], "\n  ".join(errs)))
    body = "".join(rows)
    return {"opaque": sum(body.count(ch) for ch in "#Sb"),
            "dark": body.count("#"), "steel": body.count("S"), "bone": body.count("b")}


def blocks():
    out = []
    for st in STATES:
        rows = draw(st)
        stats = check(rows, st)
        out.append((st, rows, stats))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true", help="print blocks to stdout")
    ap.add_argument("--stats", action="store_true", help="print value counts only")
    args = ap.parse_args()

    for st, rows, stats in blocks():
        n = stats["opaque"]
        print("### Vanguard State %s -- %s" % (st["id"], st["name"]), file=sys.stderr)
        print("    apex_rows=%(apex_rows)d apex_w=%(apex_w)d brim_w=%(brim_w)d "
              "visor=%(visor)s" % st, file=sys.stderr)
        print("    opaque %d  dark %d (%.0f%%)  steel %d (%.0f%%)  bone %d (%.0f%%)"
              % (n, stats["dark"], 100 * stats["dark"] / n,
                 stats["steel"], 100 * stats["steel"] / n,
                 stats["bone"], 100 * stats["bone"] / n), file=sys.stderr)
        if args.emit:
            print("```")
            print("\n".join(rows))
            print("```")
            print()


if __name__ == "__main__":
    main()
