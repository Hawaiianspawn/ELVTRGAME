"""Measure a family of sprite variants by silhouette, and build a contact sheet to judge them.

WHY THIS EXISTS
---------------
Variant families fail in a way that eyeballing does not catch. PixelLab's own knight group
looked varied but four of its six states measured an *identical* 1,093-pixel outline --
they differed only on the interior, which is the part a small dark sprite carries least.
"They look different" is not evidence; a measured outline is.

So this reports the numbers that decide whether a family actually reads apart:

  aspect      width / height. The primary axis. Below 1.0 is taller than wide.
  solidity    fraction of the bounding box the mass fills. Low means a gappy or spiky
              outline -- a different KIND of shape, not just a different ratio.
  asymmetry   mirror difference, 0.00 = perfectly symmetrical. The only cheap way to tell
              a form with a front and a back from yet another blob.
  luma        mean and p95 of opaque pixels. On a dark panel a low mean means only the rim
              highlights reach the player, so the outline is doing all the work.
  off-ramp    pixels not on demichrome-4 (measured into each variant's dict, currently
              not surfaced in the table or contact sheet). demichrome-4 is opt-in, not
              the default, since docs/art/aesthetic-direction.md's 2026-07-28 AMENDMENT
              -- a full-colour family measuring high off-ramp here is expected, not a
              defect. Meaningful only for a family that has actually opted into the ramp.

Usage:
  py Scripts/art/silhouette_report.py <dir> [--out sheet.html] [--title T] [--scale N]
  py Scripts/art/silhouette_report.py <dir> --all-directions

<dir> is scanned for */rotations/<direction>.png, one subdirectory per variant.
--all-directions measures every rotation and flags variants whose silhouette changes shape
as they turn, which a south-only pass cannot see.
"""
import argparse
import base64
import io
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
DIRECTIONS = ["south", "south-east", "east", "north-east",
              "north", "north-west", "west", "south-west"]


def ramp():
    p = REPO / "docs/data/art/palette.json"
    vals = json.loads(p.read_text(encoding="utf-8"))["palettes"]["demichrome-4"]["values"]
    return {tuple(v["rgb"]): v["key"] for v in vals}


RAMP = ramp()


def count_holes(sub, min_area=6):
    """Enclosed background regions — the sprite's actual topology (its genus).

    Solidity says an outline is gappy; this says whether the gap is a real hole. A hole
    that survives downsampling is the most separable feature a sprite can have (the ranged
    Kite's detached drone, the brood Slug), because nothing else in a rank has open sky
    inside it. Regions under `min_area` are ignored — a 2-3 px gap closes up at panel scale
    and is not a hole a player will ever see.

    Flood-fills the background inward from the border; whatever it cannot reach is enclosed.
    """
    h, w = sub.shape
    bg = ~sub
    seen = np.zeros_like(bg)
    stack = [(y, x) for y in range(h) for x in (0, w - 1) if bg[y, x]]
    stack += [(y, x) for x in range(w) for y in (0, h - 1) if bg[y, x]]
    for y, x in stack:
        seen[y, x] = True
    while stack:
        y, x = stack.pop()
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            ny, nx = y + dy, x + dx
            if 0 <= ny < h and 0 <= nx < w and bg[ny, nx] and not seen[ny, nx]:
                seen[ny, nx] = True
                stack.append((ny, nx))

    enclosed = bg & ~seen
    holes, areas = 0, []
    todo = np.argwhere(enclosed)
    visited = np.zeros_like(enclosed)
    for sy, sx in todo:
        if visited[sy, sx]:
            continue
        area, st = 0, [(int(sy), int(sx))]
        visited[sy, sx] = True
        while st:
            y, x = st.pop()
            area += 1
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                ny, nx = y + dy, x + dx
                if (0 <= ny < h and 0 <= nx < w and enclosed[ny, nx]
                        and not visited[ny, nx]):
                    visited[ny, nx] = True
                    st.append((ny, nx))
        if area >= min_area:
            holes += 1
            areas.append(area)
    return holes, (max(areas) if areas else 0)


def measure(path):
    im = Image.open(path).convert("RGBA")
    a = np.array(im)
    alpha = a[..., 3]
    m = alpha > 0
    if not m.any():
        raise SystemExit("%s is fully transparent" % path)

    ys, xs = np.where(m)
    y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
    sub = m[y0:y1 + 1, x0:x1 + 1]
    h, w = sub.shape

    px = a[m][:, :3]
    lum = (0.2126 * px[:, 0] + 0.7152 * px[:, 1] + 0.0722 * px[:, 2]) / 255.0

    f = sub.astype(float)
    asym = float(np.abs(f - f[:, ::-1]).sum() / max(1.0, f.sum()))
    holes, biggest_hole = count_holes(sub)

    off = sum(1 for rgb in map(tuple, px) if rgb not in RAMP)

    return {
        "path": path,
        "canvas": (im.width, im.height),
        "content": (int(w), int(h)),
        "aspect": w / float(h),
        "solidity": float(sub.sum()) / float(w * h),
        "asymmetry": asym,
        "holes": holes,
        "hole_px": biggest_hole,
        "luma_mean": float(lum.mean()),
        "luma_p95": float(np.percentile(lum, 95)),
        "opaque": int(m.sum()),
        "distinct": int(len({tuple(v) for v in px})),
        "off_ramp": off,
        "binary_alpha": bool(set(np.unique(alpha)) <= {0, 255}),
        "crop": im.crop((int(x0), int(y0), int(x1) + 1, int(y1) + 1)),
    }


def variant_dirs(root):
    """(name, dir) pairs where `dir` directly contains `<direction>.png` files.

    Two layouts occur on disk. Nested -- "<variant>/rotations/<dir>.png" -- is the
    common case. Flat -- "<variant>/<dir>.png" -- is archer-proxy's and
    brood-ooze-colour's, deliberately (see docs/data/art/provenance.json: "flat,
    matching T_Soldier_Archer's own flat raw/ layout rather than knight's
    raw/rotations/ subfolder"). A family's root can also BE a flat variant with no
    subdirectory at all, either because it is the one state that predates the
    per-variant-folder convention (archer-proxy's base) or because the whole family
    is a single character (brood-ooze-colour). That variant is named "base".
    """
    root = Path(root)
    out = []
    if (root / "south.png").exists():
        out.append(("base", root))
    for d in sorted(root.iterdir()):
        if not d.is_dir():
            continue
        rot = d / "rotations"
        if (rot / "south.png").exists():
            out.append((d.name, rot))
        elif (d / "south.png").exists():
            out.append((d.name, d))
    return out


def variants(root, direction="south"):
    out = [(name, d / ("%s.png" % direction)) for name, d in variant_dirs(root)]
    out = [(n, p) for n, p in out if p.exists()]
    if not out:
        raise SystemExit("no <variant>/[rotations/]%s.png found under %s"
                         % (direction, root))
    return out


def to_jsonable(m):
    """measure()'s dict, minus the two fields json.dumps chokes on."""
    return {k: v for k, v in m.items() if k not in ("path", "crop")}


# ------------------------------------------------------------------ reporting

def print_table(rows):
    hdr = ("variant", "content", "aspect", "solid", "asym", "holes", "luma", "colours")
    print("%-26s %-9s %-7s %-6s %-6s %-9s %-6s %s" % hdr)
    for name, m in rows:
        holes = "%d (%dpx)" % (m["holes"], m["hole_px"]) if m["holes"] else "-"
        print("%-26s %-9s %-7.2f %-6.2f %-6.2f %-9s %-6.3f %d%s"
              % (name[:26], "%dx%d" % m["content"], m["aspect"], m["solidity"],
                 m["asymmetry"], holes, m["luma_mean"], m["distinct"],
                 "" if m["binary_alpha"] else "  SOFT-ALPHA"))

    asp = [m["aspect"] for _, m in rows]
    print("\naspect spread %.2f-%.2f (%.1fx) across %d variants"
          % (min(asp), max(asp), max(asp) / max(1e-6, min(asp)), len(rows)))
    widths = [m["content"][0] for _, m in rows]
    print("width spread  %d-%d px (%.1fx)"
          % (min(widths), max(widths), max(widths) / max(1, min(widths))))

    # the failure PixelLab's knight group actually had
    seen = {}
    for name, m in rows:
        seen.setdefault(m["opaque"], []).append(name)
    dupes = {k: v for k, v in seen.items() if len(v) > 1}
    if dupes:
        print("\nIDENTICAL OUTLINES -- these differ on the interior only:")
        for n, names in dupes.items():
            print("  %d opaque px: %s" % (n, ", ".join(names)))


def measure_all_directions(root):
    """{variant: {direction: measure() dict}} for every variant under root.

    Shared by check_rotations (the printed report) and variantpipe.py's judge
    (which needs the same per-direction numbers to compute a band per axis rather
    than reprint them).
    """
    out = {}
    for name, d in variant_dirs(root):
        got = {}
        for dirn in DIRECTIONS:
            p = d / ("%s.png" % dirn)
            if p.exists():
                got[dirn] = measure(p)
        out[name] = got
    return out


def bands(per_direction):
    """(variant -> axis -> [lo, hi]) plus the set of hole-counts seen, from
    measure_all_directions()'s output. A variant with fewer than 8 directions gets
    "missing" instead of a band -- the judge in variantpipe.py treats that as an
    unambiguous reject rather than scoring a partial band.
    """
    out = {}
    for name, per in per_direction.items():
        if len(per) < len(DIRECTIONS):
            out[name] = {"missing": len(DIRECTIONS) - len(per)}
            continue
        ms = list(per.values())
        out[name] = {
            "aspect": [min(m["aspect"] for m in ms), max(m["aspect"] for m in ms)],
            "solidity": [min(m["solidity"] for m in ms), max(m["solidity"] for m in ms)],
            "asymmetry": [min(m["asymmetry"] for m in ms), max(m["asymmetry"] for m in ms)],
            "holes": sorted({m["holes"] for m in ms}),
        }
    return out


def check_rotations(root):
    """A south-only pass cannot see a variant that loses its shape as it turns."""
    print("rotation coherence -- aspect and solidity across all 8 directions\n")
    per_direction = measure_all_directions(root)
    worst = []
    for name, per in per_direction.items():
        if not per:
            continue
        asp = [m["aspect"] for m in per.values()]
        sol = [m["solidity"] for m in per.values()]
        drift = max(asp) - min(asp)
        print("%-26s aspect %.2f-%.2f (drift %.2f)  solidity %.2f-%.2f"
              % (name[:26], min(asp), max(asp), drift, min(sol), max(sol)))
        worst.append((drift, name))
    worst.sort(reverse=True)
    if worst and worst[0][0] > 0.45:
        print("\nWATCH: %s drifts %.2f in aspect across its rotations. An asymmetric or "
              "spiky form can read clearly facing south and go ambiguous from behind -- "
              "look at its north frames before packing it."
              % (worst[0][1], worst[0][0]))
    return per_direction


# ------------------------------------------------------------------ contact sheet

def b64(im):
    buf = io.BytesIO()
    im.save(buf, "PNG")
    return base64.b64encode(buf.getvalue()).decode()


def flat(im, hexcol="#a0a08b"):
    a = np.array(im)
    out = np.zeros_like(a)
    rgb = tuple(int(hexcol[i:i + 2], 16) for i in (1, 3, 5))
    k = a[..., 3] > 0
    out[k, 0], out[k, 1], out[k, 2] = rgb
    out[k, 3] = 255
    return Image.fromarray(out, "RGBA")


SHEET_CSS = """
:root{--ground:#141418;--surface:#1d1d24;--line:#31313c;--sunk:#0e0e12;
  --bone:#a0a08b;--pale:#e9efec;--steel:#7f8494;--blood:#8f4038;
  --ink:var(--pale);--dim:var(--steel);
  --serif:"Iowan Old Style","Palatino Linotype",Palatino,Georgia,serif;
  --mono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;}
@media (prefers-color-scheme:light){:root{--ground:#f3f2ed;--surface:#fff;--line:#dad8ce;
  --sunk:#e8e6dd;--ink:#20202a;--dim:#5d6270;--bone:#7b7b66;}}
:root[data-theme="light"]{--ground:#f3f2ed;--surface:#fff;--line:#dad8ce;--sunk:#e8e6dd;
  --ink:#20202a;--dim:#5d6270;--bone:#7b7b66;}
:root[data-theme="dark"]{--ground:#141418;--surface:#1d1d24;--line:#31313c;--sunk:#0e0e12;
  --ink:#e9efec;--dim:#7f8494;--bone:#a0a08b;}
body{background:var(--ground);color:var(--ink);font-family:var(--serif);margin:0;
  padding:clamp(1.25rem,3.5vw,3rem);line-height:1.55;}
.wrap{max-width:108rem;margin:0 auto;display:flex;flex-direction:column;gap:2rem;}
header{max-width:62ch;display:flex;flex-direction:column;gap:.6rem;}
.eyebrow{font-family:var(--mono);font-size:.7rem;letter-spacing:.16em;text-transform:uppercase;
  color:var(--bone);margin:0;}
h1{font-size:clamp(1.8rem,3.6vw,2.5rem);margin:0;font-weight:600;text-wrap:balance;
  line-height:1.15;}
header p{margin:0;color:var(--dim);}
.tiles{display:grid;gap:.75rem;grid-template-columns:repeat(auto-fit,minmax(11rem,1fr));}
.t{background:var(--surface);border:1px solid var(--line);padding:.8rem .9rem;display:flex;
  flex-direction:column;gap:.15rem;}
.t b{font-family:var(--mono);font-size:1.4rem;font-weight:600;line-height:1;
  font-variant-numeric:tabular-nums;color:var(--bone);}
.t span{font-size:.78rem;color:var(--dim);}
/* One family = ONE horizontal row, read left to right, because that is how a player
   compares silhouettes. Wrapping into short stacked rows made the page a vertical
   scroll and put siblings above/below each other instead of side by side.
   The row scrolls inside ITSELF (overflow-x on .strip, never on body) so a 13-variant
   family stays a row without the page scrolling sideways. */
.strip{display:grid;gap:.75rem;grid-auto-flow:column;
  grid-auto-columns:minmax(11rem,1fr);overflow-x:auto;padding-bottom:.5rem;
  scrollbar-width:thin;overscroll-behavior-x:contain;}
.s{margin:0;background:var(--surface);border:1px solid var(--line);display:flex;
  flex-direction:column;}
.pair{display:flex;align-items:flex-end;justify-content:center;gap:.45rem;padding:.7rem .35rem;
  min-height:9rem;background:repeating-conic-gradient(from 0deg,rgba(127,132,148,.06) 0% 25%,
  transparent 0% 50%) 0 0/14px 14px;}
.s img{height:auto;max-width:100%;image-rendering:pixelated;}
.s img.sil{opacity:.92;}
figcaption{border-top:1px solid var(--line);padding:.5rem .65rem;display:flex;
  flex-direction:column;gap:.1rem;}
figcaption b{font-size:.95rem;font-weight:600;}
.mx{margin:.25rem 0 0;font-family:var(--mono);font-size:.63rem;color:var(--dim);
  font-variant-numeric:tabular-nums;line-height:1.5;}
em{font-style:normal;font-family:var(--mono);font-size:.63rem;text-transform:uppercase;
  letter-spacing:.04em;}
em.bad{color:var(--blood);}
.legend{display:flex;flex-wrap:wrap;gap:.9rem;font-family:var(--mono);font-size:.68rem;
  color:var(--dim);align-items:center;}
.legend i{width:11px;height:11px;display:block;border:1px solid var(--line);
  background:#a0a08b;}
.scroll{overflow-x:auto;}
"""


def sheet(rows, title, scale, subtitle):
    asp = [m["aspect"] for _, m in rows]
    widths = [m["content"][0] for _, m in rows]
    cards = []
    for name, m in rows:
        w = m["content"][0] * scale
        flags = []
        if not m["binary_alpha"]:
            flags.append('<em class="bad">soft alpha</em>')
        if m["luma_mean"] < 0.20:
            flags.append('<em class="bad">luma %.3f</em>' % m["luma_mean"])
        # variantpipe.py's judge stamps "verdict"/"reasons" onto the same dict
        # before calling sheet() -- not part of measure()'s own output.
        verdict = m.get("verdict")
        if verdict in ("reject", "flag"):
            flags.append('<em class="bad">%s</em>' % verdict.upper())
            flags.extend('<em class="bad">%s</em>' % r for r in m.get("reasons", []))
        elif verdict == "keep":
            flags.append('<em>keep</em>')
        cards.append(
            '        <figure class="s">\n'
            '          <div class="pair">\n'
            '            <img src="data:image/png;base64,%s" alt="%s" style="width:%dpx">\n'
            '            <img class="sil" src="data:image/png;base64,%s" alt="%s outline" '
            'style="width:%dpx">\n'
            '          </div>\n'
            '          <figcaption><b>%s</b>\n'
            '            <p class="mx">%d&times;%d &middot; aspect %.2f &middot; solidity %.2f'
            ' &middot; asym %.2f</p>\n'
            '            <p class="mx">luma %.3f / p95 %.3f &middot; %d colours</p>\n'
            '            %s\n'
            '          </figcaption>\n'
            '        </figure>'
            % (b64(m["crop"]), name, w, b64(flat(m["crop"])), name, w, name,
               m["content"][0], m["content"][1], m["aspect"], m["solidity"],
               m["asymmetry"], m["luma_mean"], m["luma_p95"], m["distinct"],
               ('<p class="mx">' + " ".join(flags) + "</p>") if flags else ""))

    return """<title>%s</title>
<style>%s</style>
<div class="wrap">
  <header>
    <p class="eyebrow">Kindled &middot; silhouette report</p>
    <h1>%s</h1>
    <p>%s Every variant appears as rendered and again as a flat outline, because the outline
    is what survives at panel scale. All figures share one zoom, so relative size is true.</p>
  </header>
  <div class="tiles">
    <div class="t"><b>%.2f&ndash;%.2f</b><span>aspect range, a %.1f&times; spread across %d
      variants.</span></div>
    <div class="t"><b>%d&ndash;%d px</b><span>width range, a %.1f&times; spread.</span></div>
    <div class="t"><b>%.2f</b><span>widest asymmetry &mdash; above ~0.4 means a form with a
      front and a back.</span></div>
    <div class="t"><b>%.2f</b><span>lowest solidity &mdash; the gappiest outline, a different
      kind of shape rather than a different ratio.</span></div>
  </div>
  <div class="legend"><b><i></i></b>
    <span>outline &mdash; solidity is the share of the bounding box filled; asymmetry is the
    mirror difference, 0.00 being perfectly symmetrical.</span>
  </div>
  <section><div class="strip">
%s
  </div></section>
</div>
""" % (title, SHEET_CSS, title, subtitle,
       min(asp), max(asp), max(asp) / max(1e-6, min(asp)), len(rows),
       min(widths), max(widths), max(widths) / max(1, min(widths)),
       max(m["asymmetry"] for _, m in rows),
       min(m["solidity"] for _, m in rows),
       "\n".join(cards))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("root", help="directory of <variant>/rotations/*.png")
    ap.add_argument("--out", help="write a self-contained HTML contact sheet here")
    ap.add_argument("--title", default="Silhouette report")
    ap.add_argument("--subtitle", default="")
    ap.add_argument("--scale", type=int, default=3, help="integer zoom on the sheet")
    ap.add_argument("--direction", default="south")
    ap.add_argument("--all-directions", action="store_true",
                    help="measure every rotation and flag shape drift")
    ap.add_argument("--json", action="store_true",
                    help="print machine-readable results instead of (or after) the table")
    args = ap.parse_args()

    if args.all_directions:
        per_direction = check_rotations(args.root)
        if args.json:
            out = {name: {d: to_jsonable(m) for d, m in per.items()}
                   for name, per in per_direction.items()}
            print(json.dumps({"directions": out, "bands": bands(per_direction)}, indent=2))
        return 0

    rows = [(n, measure(p)) for n, p in variants(args.root, args.direction)]
    if args.json:
        print(json.dumps([{"name": n, **to_jsonable(m)} for n, m in rows], indent=2))
    else:
        print_table(rows)

    if args.out:
        Path(args.out).write_text(
            sheet(rows, args.title, args.scale, args.subtitle), encoding="utf-8")
        print("\nwrote %s" % args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
