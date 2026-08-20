"""Contact sheet for an animation probe: play the frames, don't just list them.

    py Scripts/art/anim_probe_page.py RawArt/Renders/brood-anim-probe out.html

Reads <root>/raw/<look>/<action>/frame_N.png plus <root>/jobs.json and lays the
result out as a matrix: one row per look, one column per action. Identical action
prompt down each column, so the only variable within a column is the body shape.

The thing being measured is NOT "is this a nice animation". It is whether bite and
hurt end up different enough to be worth two atlas rows -- an attack pose and a hit
pose that move the silhouette the same way are one row wearing two names. See
`separation`. Frames go in as data URIs so the page is self-contained (the Artifact
CSP blocks every external host), and a scale toggle drives every sprite at once
because the only size that settles anything here is the small one: brood art has
already failed a legibility gate at gameplay scale
(docs/data/art/provenance.json qc, 2026-07-29).
"""
import base64
import json
import os
import sys

from PIL import Image

VARIANTS = os.path.join("docs", "data", "art", "brood-variants.json")
ACTIONS = ["bite", "hurt", "walk"]

# A hit pose has to be tellable from an attack pose at a 14px cell, where the only
# thing left of the sprite is its outline. Two poses that both grow the body by a
# few pixels are the same pose. So: opposite directions is a clean separation,
# same direction with a big magnitude gap is a usable one, and anything else is not
# worth a second row.
MAGNITUDE_GAP = 8


def frames(d):
    return [os.path.join(d, f)
            for f in sorted(os.listdir(d),
                            key=lambda n: int("".join(c for c in n if c.isdigit()) or 0))
            if f.endswith(".png")]


def uri(p):
    return "data:image/png;base64," + base64.b64encode(open(p, "rb").read()).decode()


def heights(paths):
    out = []
    for p in paths:
        bb = Image.open(p).convert("RGBA").split()[3].getbbox()
        out.append(0 if bb is None else bb[3] - bb[1])
    return out


def peak(hs):
    """(frame index, signed height change) for the frame furthest from rest."""
    rest = hs[0]
    i = max(range(len(hs)), key=lambda k: abs(hs[k] - rest))
    return i, hs[i] - rest


def separation(cells):
    """Can bite and hurt be two atlas rows on this body?

    'direction' -- one grows, one shrinks; readable at any size.
    'magnitude' -- both move the same way, but far enough apart to tell.
    'none'      -- the two poses do the same thing. One row, not two.
    """
    if "bite" not in cells or "hurt" not in cells:
        return "none", 0
    b = cells["bite"]["delta"]
    h = cells["hurt"]["delta"]
    gap = b - h
    if (b > 0) != (h > 0):
        return "direction", gap
    if abs(gap) >= MAGNITUDE_GAP:
        return "magnitude", gap
    return "none", gap


def load_looks(root):
    meta = {}
    if os.path.exists(VARIANTS):
        for v in json.load(open(VARIANTS))["variants"]:
            meta[v["state"]] = v
    rawdir = os.path.join(root, "raw")
    looks = []
    for name in sorted(os.listdir(rawdir)):
        d = os.path.join(rawdir, name)
        if not os.path.isdir(d):
            continue
        cells, rest = {}, 0
        for action in ACTIONS:
            ad = os.path.join(d, action)
            if not os.path.isdir(ad):
                continue
            fs = frames(ad)
            if len(fs) < 2:
                continue
            hs = heights(fs)
            pk, delta = peak(hs)
            rest = hs[0]
            cells[action] = {"uris": [uri(f) for f in fs], "hs": hs,
                             "peak": pk, "delta": delta, "drift": hs[-1] - hs[0]}
        if not cells:
            continue
        m = meta.get(name, {})
        sep, gap = separation(cells)
        looks.append({"name": name, "cells": cells, "rest": rest, "sep": sep, "gap": gap,
                      "weight": m.get("weight", 0), "look": m.get("look", ""),
                      "id": m.get("id", name)})
    # Sorted by resting height, tallest first -- that is the axis the result falls on,
    # so the split shows up as a line in the table instead of a claim in prose.
    looks.sort(key=lambda l: -l["rest"])
    return looks


SEP_LABEL = {"direction": "two rows", "magnitude": "two rows", "none": "one row"}


def render_cell(look, action):
    c = look["cells"].get(action)
    if not c:
        return '<div class="cell empty">&mdash;</div>'
    sign = "+" if c["delta"] > 0 else ""
    note = ("returns %+dpx" % c["drift"]) if action == "walk" else ("frame %d" % c["peak"])
    return """<div class="cell">
  <div class="well"><img class="play" data-look="%s" data-action="%s" alt="%s, %s"></div>
  <p class="read"><b>%s%dpx</b><i>%s</i></p>
</div>""" % (look["name"], action, look["name"], action, sign, c["delta"], note)


def render_row(look, divider):
    rule = ('<div class="divider"><span>below this line a body has no height left to lose</span></div>'
            if divider else "")
    return """%s
<article class="%s">
  <div class="who">
    <h2>%s</h2>
    <p class="shape">%s</p>
    <p class="stats"><span>rest</span><b>%dpx</b><span>seen</span><b>%d%%</b></p>
    <p class="sep %s">%s<i>bite/hurt gap %+dpx</i></p>
  </div>
  %s
</article>""" % (rule, look["sep"], look["id"], look["look"] or look["name"],
                 look["rest"], look["weight"], look["sep"], SEP_LABEL[look["sep"]],
                 look["gap"], "".join(render_cell(look, a) for a in ACTIONS))


def main(root, out):
    looks = load_looks(root)
    spec = json.load(open(os.path.join(root, "jobs.json")))

    works = [l for l in looks if l["sep"] != "none"]
    fails = [l for l in looks if l["sep"] == "none"]
    cover = sum(l["weight"] for l in works)
    tallest_fail = max((l["rest"] for l in fails), default=0)
    shortest_work = min((l["rest"] for l in works), default=0)

    rows, seen_fail = [], False
    for l in looks:
        divider = (l["sep"] == "none" and not seen_fail)
        if divider:
            seen_fail = True
        rows.append(render_row(l, divider))

    data = "{%s}" % ",".join(
        '"%s":{%s}' % (l["name"], ",".join(
            '"%s":[%s]' % (a, ",".join('"%s"' % u for u in c["uris"]))
            for a, c in l["cells"].items())) for l in looks)
    peaks = "{%s}" % ",".join(
        '"%s":{%s}' % (l["name"], ",".join('"%s":%d' % (a, c["peak"])
                                           for a, c in l["cells"].items())) for l in looks)

    open(out, "w", encoding="utf-8").write(TEMPLATE % {
        "rows": "".join(rows), "data": data, "peaks": peaks,
        "works": len(works), "total": len(looks), "cover": cover,
        "heaviest": fails[0]["id"] if fails else "&mdash;",
        "heaviest_w": max((l["weight"] for l in fails), default=0),
        "cut_lo": tallest_fail, "cut_hi": shortest_work,
        "jobs": sum(len(v) for v in spec["jobs"].values()),
        "bite": spec["actions"]["bite"], "hurt": spec["actions"]["hurt"],
        "walk": spec["actions"]["walk"],
    })
    print("wrote %s (%d looks, %d separate, %d%% of the horde)"
          % (out, len(looks), len(works), cover))


TEMPLATE = """<title>Nasher Motion Matrix</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Fraunces:opsz,wght@9..144,400;9..144,600&family=IBM+Plex+Mono:wght@400;500;600&family=Public+Sans:wght@400;600&display=swap">
<style>
/* Ground and ramp are the project's own named values (docs/data/art/palette.json):
   demichrome-4 #211e20 / #555568 / #a0a08b / #e9efec, with rust-gold-4's #bb7f57
   as the single accent. Nothing invented. */
:root{
  --bg:#e9efec; --panel:#dfe6e2; --fg:#211e20; --dim:#5d6360; --rule:#c6d0cb;
  --accent:#8a5533; --well:#17161a;
  --ok:#4b7f52; --bad:#a34a3c;
  --serif:"Fraunces",Georgia,serif;
  --sans:"Public Sans",-apple-system,"Segoe UI",sans-serif;
  --mono:"IBM Plex Mono",ui-monospace,Consolas,monospace;
  --sprite:88px;
}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){
  --bg:#141316; --panel:#1c1b1f; --fg:#e9efec; --dim:#9a9c99; --rule:#33323a;
  --accent:#d6a05e; --well:#0c0b0e; --ok:#7fb388; --bad:#d97f6f;
}}
:root[data-theme="dark"]{
  --bg:#141316; --panel:#1c1b1f; --fg:#e9efec; --dim:#9a9c99; --rule:#33323a;
  --accent:#d6a05e; --well:#0c0b0e; --ok:#7fb388; --bad:#d97f6f;
}

*{box-sizing:border-box;}
body{background:var(--bg);color:var(--fg);font-family:var(--sans);font-size:16px;line-height:1.6;margin:0;padding:clamp(2rem,5vw,4rem) 1.25rem 5rem;}
main{max-width:72rem;margin:0 auto;}

.eyebrow{font-family:var(--mono);font-size:.72rem;letter-spacing:.14em;text-transform:uppercase;color:var(--accent);margin:0 0 .9rem;}
h1{font-family:var(--serif);font-weight:600;font-size:clamp(2rem,5vw,3.2rem);line-height:1.04;letter-spacing:-.02em;margin:0 0 1rem;text-wrap:balance;max-width:20em;}
.lede{font-size:1.05rem;color:var(--dim);max-width:42em;margin:0 0 2rem;}
.lede b{color:var(--fg);font-weight:600;}

.summary{display:grid;grid-template-columns:repeat(auto-fit,minmax(14rem,1fr));gap:1rem;margin:0 0 1.75rem;}
.sum{background:var(--panel);border-radius:4px;padding:1.1rem 1.2rem;}
.sum b{display:block;font-family:var(--serif);font-size:2.1rem;font-weight:600;line-height:1;letter-spacing:-.02em;margin-bottom:.35rem;font-variant-numeric:tabular-nums;}
.sum p{font-size:.85rem;color:var(--dim);margin:0;}
.sum.warn b{color:var(--bad);}

.controls{display:flex;align-items:center;gap:.5rem;flex-wrap:wrap;font-family:var(--mono);font-size:.75rem;color:var(--dim);padding:.9rem 0;border-top:1px solid var(--rule);border-bottom:1px solid var(--rule);position:sticky;top:0;background:var(--bg);z-index:2;}
button{font:inherit;color:var(--dim);background:transparent;border:1px solid var(--rule);border-radius:3px;padding:.28rem .7rem;cursor:pointer;}
button[aria-pressed="true"]{color:var(--bg);background:var(--fg);border-color:var(--fg);}
button:focus-visible{outline:2px solid var(--accent);outline-offset:2px;}

.head{display:grid;grid-template-columns:minmax(0,14rem) repeat(3,minmax(0,1fr));gap:.9rem 1rem;padding:1.1rem 0 .3rem;font-family:var(--mono);font-size:.7rem;letter-spacing:.1em;text-transform:uppercase;color:var(--dim);}
article{display:grid;grid-template-columns:minmax(0,14rem) repeat(3,minmax(0,1fr));gap:.9rem 1rem;padding:1.15rem 0;border-top:1px solid var(--rule);align-items:start;}
article.none .well{opacity:.72;}

.divider{display:flex;align-items:center;gap:1rem;padding:1.6rem 0 .4rem;}
.divider::before,.divider::after{content:"";flex:1;height:1px;background:var(--bad);opacity:.5;}
.divider span{font-family:var(--mono);font-size:.68rem;letter-spacing:.08em;text-transform:uppercase;color:var(--bad);}

.who h2{font-family:var(--mono);font-weight:600;font-size:.9rem;margin:0 0 .3rem;}
.shape{font-size:.82rem;color:var(--dim);margin:0 0 .55rem;}
.stats{font-family:var(--mono);font-size:.72rem;color:var(--dim);margin:0 0 .5rem;display:flex;gap:.35rem;align-items:baseline;flex-wrap:wrap;font-variant-numeric:tabular-nums;}
.stats b{color:var(--fg);font-size:.85rem;margin-right:.55rem;}
.sep{font-family:var(--mono);font-size:.68rem;letter-spacing:.06em;text-transform:uppercase;font-weight:600;margin:0;display:flex;gap:.5rem;align-items:baseline;flex-wrap:wrap;}
.sep i{font-style:normal;letter-spacing:0;text-transform:none;font-weight:400;color:var(--dim);}
.sep.direction,.sep.magnitude{color:var(--ok);}
.sep.none{color:var(--bad);}

.cell{display:flex;flex-direction:column;gap:.5rem;}
.cell.empty{color:var(--dim);font-family:var(--mono);}
.well{background:var(--well);border-radius:3px;min-height:calc(var(--sprite) + 1.4rem);display:flex;align-items:center;justify-content:center;padding:.7rem;}
img{image-rendering:pixelated;display:block;width:var(--sprite);height:var(--sprite);}
.read{font-family:var(--mono);font-size:.72rem;color:var(--dim);margin:0;display:flex;gap:.6rem;align-items:baseline;font-variant-numeric:tabular-nums;}
.read b{color:var(--fg);font-weight:600;}
.read i{font-style:normal;}

footer{border-top:1px solid var(--rule);margin-top:2.5rem;padding-top:1.75rem;color:var(--dim);font-size:.93rem;}
footer h3{font-family:var(--serif);font-weight:600;font-size:1.2rem;color:var(--fg);margin:0 0 .6rem;}
footer p{max-width:46em;}
footer code{font-family:var(--mono);font-size:.85em;color:var(--fg);}
footer dl{font-family:var(--mono);font-size:.78rem;margin:1.2rem 0 0;}
footer dt{color:var(--fg);text-transform:capitalize;}
footer dd{margin:0 0 .55rem;}

@media (max-width:760px){
  .head{display:none;}
  article{grid-template-columns:1fr 1fr;}
  .who{grid-column:1 / -1;}
}
</style>

<main>
  <p class="eyebrow">Brood animation probe &middot; nine looks &middot; south rotation</p>
  <h1>Only the tall nashers can tell a bite from a wound</h1>
  <p class="lede">Bite, hurt and walk generated on all nine brood bodies. <b>Identical prompt down
  each column</b>, same seed, so the only thing that changes within a column is the shape. The test
  is not whether an animation looks good &mdash; it is whether <b>bite and hurt end up different
  enough to deserve two atlas rows</b>. Rows are ordered by resting height, because that is the
  axis the answer fell on.</p>

  <div class="summary">
    <div class="sum"><b>%(works)s / %(total)s</b><p>shapes where the hit pose and the attack pose
      move the silhouette differently</p></div>
    <div class="sum warn"><b>%(cover)s%%</b><p>of the horde is covered by those shapes &mdash; the
      other half cannot use a hurt row at all</p></div>
    <div class="sum warn"><b>%(heaviest)s</b><p>fails, and it is %(heaviest_w)s%% of every brood
      you meet &mdash; the single most-seen body is the worst case</p></div>
    <div class="sum"><b>%(cut_lo)s&ndash;%(cut_hi)spx</b><p>the split, in resting height. Every
      body above it works; every body below it fails</p></div>
  </div>

  <div class="controls">
    <span>Sprite size</span>
    <button data-size="88" aria-pressed="true">88px</button>
    <button data-size="56" aria-pressed="false">56px &mdash; atlas cell</button>
    <button data-size="14" aria-pressed="false">14px &mdash; gameplay</button>
    <button id="freeze" aria-pressed="false">Freeze on peak</button>
  </div>

  <div class="head"><span>Look</span><span>Bite</span><span>Hurt</span><span>Walk</span></div>
%(rows)s

  <footer>
    <h3>Why height decides it</h3>
    <p>A hurt pose works by taking the body down &mdash; the model squashes what is there to
    squash. A tall body has room to lose, so hurt shortens it by 14&ndash;19px while bite only
    moves it 5px, and the two poses read apart. A low wide mass has nothing to give, so the model
    <b>adds</b> height instead, and hurt comes out doing the same thing as bite: both grow the
    silhouette by three to seven pixels. At a 14px cell that is one pose wearing two names.</p>
    <p>This is the owner's read, sharpened: it is not bipeds versus blobs. <code>brood0-base</code>
    is the limbed one and it works, but so do the bell, the wedge and the narrow stalk, none of
    which have legs. What they share is vertical extent. The four that fail are the four flattest
    bodies in the roster.</p>
    <p>The numbers beside each sprite are the signed height change at the frame furthest from
    rest, measured off the opaque bounding box &mdash; not judged by eye. Walk reports how far the
    last frame still sits from rest, because a cycle has to come back and a rise does not.</p>
    <dl>
      <dt>Bite</dt><dd>%(bite)s</dd>
      <dt>Hurt</dt><dd>%(hurt)s</dd>
      <dt>Walk</dt><dd>%(walk)s</dd>
    </dl>
    <p><code>%(jobs)s generations</code>, south only. Frames retained at
    <code>RawArt/Renders/brood-anim-probe/</code>, nothing culled.</p>
  </footer>
</main>

<script>
const FRAMES=%(data)s, PEAKS=%(peaks)s;
const imgs=[...document.querySelectorAll("img.play")];
const reduce=window.matchMedia("(prefers-reduced-motion: reduce)").matches;
let frozen=reduce, tick=0;

function paint(){
  for(const el of imgs){
    const f=FRAMES[el.dataset.look][el.dataset.action];
    el.src=frozen ? f[PEAKS[el.dataset.look][el.dataset.action]] : f[tick%%f.length];
  }
}
paint();
setInterval(()=>{ if(!frozen){ tick++; paint(); } },110);

for(const b of document.querySelectorAll("button[data-size]")){
  b.addEventListener("click",()=>{
    document.documentElement.style.setProperty("--sprite",b.dataset.size+"px");
    for(const o of document.querySelectorAll("button[data-size]")) o.setAttribute("aria-pressed",String(o===b));
  });
}
const fz=document.getElementById("freeze");
fz.setAttribute("aria-pressed",String(frozen));
fz.addEventListener("click",()=>{ frozen=!frozen; fz.setAttribute("aria-pressed",String(frozen)); paint(); });
</script>"""

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
