"""Pull a named PixelLab character animation onto disk next to its rotations.

    py Scripts/art/fetch_anim.py <family>/<slug> <character_id> [animation_name=attack_north]

Frames land at RawArt/Renders/<family>/raw/<slug>/<animation_name>/frame_N.png (one dir
per direction when the clip has several: <animation_name>/<direction>/frame_N.png), which
is where godot_pack.py looks for an attack row. Exits 1 while the clip is still rendering.
"""
import sys
from pathlib import Path
from urllib.request import Request, urlopen
sys.path.insert(0, str(Path(__file__).parent))
import forge, variantpipe as vp


def fetch(ref, cid, name="attack_north"):
    fam, slug = ref.split("/", 1)
    d = forge.pl("GET", "/characters/%s" % cid, timeout=30)
    clips = [a for a in d.get("animations") or [] if a.get("display_name") == name]
    if not clips:
        print("no finished clip named %r on %s yet" % (name, cid)); return 1
    n = 0
    for dd in clips[0]["directions"]:
        dest = vp.RENDERS / fam / "raw" / slug / name
        if len(clips[0]["directions"]) > 1:
            dest = dest / dd["direction"]
        dest.mkdir(parents=True, exist_ok=True)
        for i, url in enumerate(dd["frames"]):
            with urlopen(Request(url, headers={"User-Agent": vp.UA}), timeout=60) as r:
                (dest / ("frame_%d.png" % i)).write_bytes(r.read())
            n += 1
        print(dest, len(dd["frames"]), "frames")
    return 0 if n else 1


if __name__ == "__main__":
    sys.exit(fetch(*sys.argv[1:]))
