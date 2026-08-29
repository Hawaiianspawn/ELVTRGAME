"""Pull finished PixelLab 8-direction objects into the castle-props raw layout.

    py Scripts/art/obj_fetch.py

Each OBJECTS entry lands at RawArt/Renders/castle-props/raw/<slug>/rotations/<dir>.png, which
is what godot/data/roster.json "castle-props/<slug>" resolves to in godot_pack.py. Uses the
same authenticated client as forge.py (PIXELLAB_API_KEY or the MCP config key). Objects still
rendering are reported and skipped.
"""
import sys
from pathlib import Path
from urllib.request import Request, urlopen

sys.path.insert(0, str(Path(__file__).resolve().parent))
import forge  # noqa: E402
from atlas import DIRECTIONS  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
RAW = REPO / "RawArt" / "Renders" / "castle-props" / "raw"

# object id -> slug (roster name = slug)
OBJECTS = {
    "a4b9a216-09d5-4769-b7e0-537bb5887f38": "brazier",
    "d24d1cb8-0c19-4cef-aa2a-998fa57ab61c": "column_stump",
    "fafd6138-0ebb-4e9f-9e8b-50abcecfcb64": "statue_knight",
    "4f7a1069-1a39-4394-b3b9-6e58cd79ec57": "banner_pole",
    "e0f7101f-ddad-4e40-b9fa-e7f94380ea66": "rubble",
    "7b893d2f-47b4-4b81-9dec-3378c1cea11c": "barricade",
    "ad3e910c-5ed5-4258-b934-fab83247684c": "cage_skeleton",
    "7c17616f-6469-49bb-9a3c-71b267d495e8": "altar",
    "743db6ba-cb74-4e66-9bc8-0cd2e2a6a02d": "barrel",
    "fdd396c0-0836-43f1-8c97-18a8539c107f": "weapon_rack",
}


# character id -> (family, slug): 8-direction characters (boss, new enemy states) land at
# RawArt/Renders/<family>/raw/<slug>/rotations like every other unit.
CHARACTERS = {
    "52c196e6-91e2-4e12-bfdb-70f9ef612124": ("boss", "necromancer"),
    "a6cc31f8-bb35-451c-8af8-668b64926b5b": ("melee-undead", "ghoul"),
    "a6f83e96-f157-4211-a11a-6e2d4b7e4b60": ("melee-undead", "wraith"),
    "0780b162-10f6-4063-b1c8-9f7f6d4c69c9": ("melee-undead", "bone_knight"),
    "9acbce74-7e95-4b8c-8c12-c2ddc19bcb42": ("melee-undead", "plague_priest"),
}


def main() -> None:
    pending = 0
    jobs = [("objects", oid, RAW / slug / "rotations", slug) for oid, slug in OBJECTS.items()]
    jobs += [("characters", cid, REPO / "RawArt" / "Renders" / fam / "raw" / slug / "rotations", slug)
             for cid, (fam, slug) in CHARACTERS.items()]
    for kind, oid, d, slug in jobs:
        if all((d / f"{k}.png").exists() for k in DIRECTIONS):
            print(f"done     {slug}")
            continue
        rec = forge.pl("GET", f"/{kind}/{oid}", timeout=30)
        rot = rec.get("rotation_urls") or {}
        if (rec.get("status") or "").lower() != "completed" or not all(rot.get(k) for k in DIRECTIONS):
            print(f"pending  {slug} ({rec.get('status')})")
            pending += 1
            continue
        d.mkdir(parents=True, exist_ok=True)
        for k in DIRECTIONS:
            with urlopen(Request(rot[k], headers={"User-Agent": "Mozilla/5.0"}), timeout=60) as r:
                (d / f"{k}.png").write_bytes(r.read())
        print(f"fetched  {slug}")
    print(f"{pending} pending")


if __name__ == "__main__":
    main()
