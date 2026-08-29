"""Pull finished animate_image jobs into the godot_pack.py clip layout.

    py Scripts/art/anim_fetch.py            # fetch every job in JOBS not yet on disk
    py Scripts/art/anim_fetch.py --status   # just report done / pending per job

Frames land at RawArt/Renders/<family>/raw/<slug>/<clip>_<dir>/frame_N.png (the dir
godot_pack.py scans for clip rows). animate_image returns frame_count + 1 images
with index 0 = the input pose; loops (walk) drop it, one-shots (death, attack, hurt)
keep it as the wind-up frame. A job that is still rendering 404s and is skipped.
"""
import json
import sys
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import godot_pack as gp   # rotations_dir: selects.json may point a unit at RawArt/Aaron Selects, not raw/

REPO = Path(__file__).resolve().parents[2]
ROSTER = json.load(open(REPO / "godot" / "data" / "roster.json"))
SELECTS = json.load(open(gp.SELECTS, encoding="utf-8"))
API = "https://api.pixellab.ai/mcp/images/%s/download?index=%d"
LOOPS = {"walk"}

# (unit, clip, facing) -> (job id, generated frame count)
JOBS = {
    ("ooze", "death", "south"): ("68161511-b182-4594-80a5-c01435f13b21", 6),
    ("ooze", "attack", "south"): ("6e3f0cdb-4175-40bf-a6ff-01fff3ab7f56", 8),
    ("ooze", "walk", "south"): ("ae46076a-5e36-4625-8d83-5cd472469a9f", 8),
    ("mace_undead", "death", "south"): ("601736f0-c17a-496b-ac78-6822dea81179", 6),
    ("mace_undead", "attack", "south"): ("3750b734-45b0-46f5-b529-4ef9117b998b", 8),
    ("mace_undead", "walk", "south"): ("9c94a095-9535-4515-a026-762ac65ca34c", 8),
    ("staff_undead", "death", "south"): ("e202b1e8-06d0-4d6c-ba7a-38ea3a668901", 6),
    ("staff_undead", "attack", "south"): ("fa410af7-d856-429f-a517-8a8900f7b735", 8),
    ("staff_undead", "walk", "south"): ("e4565ed2-343c-4523-9d4e-7cacf6356fff", 8),
    ("bow_undead", "death", "south"): ("7638230d-d1a9-45fc-bd3d-1c1f5193ed8b", 6),
    ("bow_undead", "attack", "south"): ("49c7c34d-97b9-4029-a9dc-58885d19ae06", 8),
    ("bow_undead", "walk", "south"): ("6a0cddb7-1167-4cc0-a16e-0329094ce212", 8),
    ("undead2", "death", "south"): ("05489ccd-8c4f-4ebe-a474-6b404e065926", 6),
    ("undead2", "attack", "south"): ("b43f7f7b-f9cd-4ce5-bf1c-d339fac2d767", 8),
    ("undead2", "walk", "south"): ("c28722bd-1f30-4cf8-81bd-38e2e31170c4", 8),
    ("armored", "death", "south"): ("6e47e83e-c0c9-4a6f-9b00-cfde28285f83", 6),
    ("armored", "attack", "south"): ("f3a0a65e-0744-43de-8d3a-5e8f7a83c2e2", 8),
    ("armored", "walk", "south"): ("d7627941-8394-4f57-ab1e-22648075a1c0", 8),
    ("undead", "death", "south"): ("52bbd201-1382-4479-86e6-269144bb9287", 6),
    ("undead", "attack", "south"): ("204873d3-cbc7-4078-8630-3e4704de61b5", 8),
    ("undead", "walk", "south"): ("20fdb0c7-74ca-44a2-a8c4-a8e1de99dc33", 8),
    ("horse_undead", "death", "south"): ("8c22a73a-571e-4834-b7eb-2d50c0030373", 6),
    ("horse_undead", "attack", "south"): ("035b79af-ccff-49be-a3bd-e1661ef80ffc", 8),
    ("horse_undead", "walk", "south"): ("4245edfa-f78e-4cf4-ad35-775aed31f681", 8),
    ("veteran", "death", "north"): ("441890d6-03e7-43ca-8185-031010cc7ddb", 6),
    ("veteran", "walk", "north"): ("8740abbf-f6fe-462d-ac67-45cc4d6d6463", 8),
    ("halberdier", "death", "north"): ("0e130d8f-dd2f-498f-b602-38d760973e4c", 6),
    ("halberdier", "walk", "north"): ("2542ad1c-cc03-4d0a-bff6-df2ab65c8d99", 8),
    ("hammer", "death", "north"): ("08e24091-3953-467e-bbc3-d3f8c3998bf3", 6),
    ("hammer", "walk", "north"): ("aa2e1416-2a9d-4e98-bc78-1e2d39aa3d99", 8),
    ("vet_ranged", "death", "north"): ("e61e4c20-c7e9-4d4a-8df0-6c73f1187dc9", 6),
    ("vet_ranged", "walk", "north"): ("c29ec376-2b91-470e-a1ae-d72389492cc9", 8),
    ("hero_samurai", "attack", "north"): ("1636cc47-8599-4930-909a-2ab281cc225d", 8),
    ("hero_samurai", "hurt", "north"): ("6f30a966-1f7d-403b-8d03-cd292e9a60a0", 4),
    ("hero_samurai", "walk", "north"): ("bae68164-8af9-4789-bbfc-ad3cf8758e73", 8),
    ("hero_turret", "attack", "north"): ("be252811-c191-4539-85db-7b80b07f0321", 8),
    ("hero_turret", "hurt", "north"): ("517dcc25-d42e-4dfa-a575-6eaa55d28e81", 4),
    ("hero_turret", "walk", "north"): ("00ebbf8d-314e-4790-90ee-b14c7ac863c7", 8),
    ("hero_sackhauler", "attack", "north"): ("8d055523-0363-4638-8796-e403c06b04d3", 8),
    ("hero_sackhauler", "hurt", "north"): ("7de1a64b-7981-4752-9e7f-028ad9aa593a", 4),
    ("hero_sackhauler", "walk", "north"): ("a4b22e2c-2c17-4104-b7f7-6395a7137602", 8),
    ("hero_dwarf", "attack", "north"): ("e9d4f0b5-9483-4f78-988d-2d560546bbc6", 8),
    ("hero_dwarf", "hurt", "north"): ("d0a8d278-445a-489b-8134-6d0351a4d46a", 4),
    ("hero_dwarf", "walk", "north"): ("53a90a4c-2acf-46dc-a5d5-54aeae243b3b", 8),
    ("hero_cover", "attack", "north"): ("149f2299-290a-434f-8776-e87fb127dc0e", 8),
    ("hero_cover", "hurt", "north"): ("533a3b59-28ce-4ad2-add7-a2a897674dfc", 4),
    ("hero_cover", "walk", "north"): ("c7a6bf1a-c0e0-40fc-818b-644da00f678a", 8),
    ("hero_knight", "attack", "north"): ("dd19e2a7-f9b2-424f-94cf-bf7345965e6e", 8),
    ("hero_knight", "hurt", "north"): ("65033599-03bc-45d5-af33-fcb3b3d86b4c", 4),
    ("hero_knight", "walk", "north"): ("ff646bb7-93f6-4dba-8815-b4242f98a415", 8),
    ("hero_ranger", "attack", "north"): ("1fbd6236-60d2-43d9-b860-1b079b563ce8", 8),
    ("hero_ranger", "hurt", "north"): ("4e86e477-8e3c-4f8b-948b-695fdc762d07", 4),
    ("hero_ranger", "walk", "north"): ("fd55f5b6-af3f-4dac-97ea-2877b70edfac", 8),
    ("ghoul", "death", "south"): ("20a9845e-bb80-4bfc-96cf-96a4898a7572", 6),
    ("ghoul", "attack", "south"): ("3cdbc514-3a86-4017-83e1-3e0414a7e45e", 8),
    ("ghoul", "walk", "south"): ("8b42c2ed-7362-48ac-932c-450b9295b5e9", 8),
    ("bone_knight", "death", "south"): ("e5828733-0fe1-4297-ab20-deb07705c959", 6),
    ("bone_knight", "attack", "south"): ("c96394d3-6023-4434-8888-08b6793da429", 8),
    ("bone_knight", "walk", "south"): ("0ea31dd7-b657-4446-979a-f20bb4fca174", 8),
    ("plague_priest", "death", "south"): ("a0b086a6-e1fd-48aa-a0fe-86b196aea7cf", 6),
    ("plague_priest", "attack", "south"): ("8020f95f-f08c-4b89-80cd-1378d9d50e05", 8),
    ("plague_priest", "walk", "south"): ("747d161b-7fc1-4c9d-95c3-be72440c2447", 8),
    ("wraith", "death", "south"): ("15fb8ddf-842b-41ab-b14d-99f382d92bb9", 6),
    ("wraith", "attack", "south"): ("ca072d8d-4241-426a-a75c-db49b463b293", 8),
    ("wraith", "walk", "south"): ("a2fcedcc-f158-4bf9-abbb-c8af391f4fca", 8),
    ("necromancer", "death", "south"): ("dd578101-ed5e-4a5e-9ef7-cdc4263709e0", 6),
    ("necromancer", "attack", "south"): ("43656436-7d70-4e59-81c9-25ec40c447cb", 8),
    ("necromancer", "walk", "south"): ("ec21d987-26fc-4443-9547-4fd3f24b5285", 8),
    ("hero_paladin", "attack", "north"): ("413208b0-327f-4115-835a-cdb01780b7e7", 8),
    ("hero_paladin", "hurt", "north"): ("5cc64dcd-a115-408d-b298-b7c9c9648bee", 4),
    ("hero_paladin", "walk", "north"): ("efbc15ba-183f-44a7-9269-76e886821444", 8),
    ("hero_witchhunter", "attack", "north"): ("120f41bc-8d02-4419-ae30-4c458ad9ab03", 8),
    ("hero_witchhunter", "hurt", "north"): ("a2d5b8c4-cb7c-44e3-aad6-83f4bf704519", 4),
    ("hero_witchhunter", "walk", "north"): ("c818ad5d-f0e1-4311-baea-2e50510cb9e1", 8),
    ("hero_berserker", "attack", "north"): ("6875c318-e9ed-48da-81c5-d4c3e5305cac", 8),
    ("hero_berserker", "hurt", "north"): ("a609c9ea-63b3-47da-a81b-4966c056eed8", 4),
    ("hero_berserker", "walk", "north"): ("1a38f02b-8f9d-44f7-b1e7-fc0cb79434e6", 8),
}
# necromancer hurt1-7: superseded by animate_character template clips (taking-punch/leg-sweep/
# cross-punch/crouching/surprise-uppercut/hurricane-kick/getting-up), not animate_image jobs, so
# they don't fit this job-id/index table - frames were pulled straight from get_character() URLs.
# take1 (animate_image v3, flame-variation only, rejected) kept at raw/necromancer/hurtN_south_take1.


# Loose effect clips: name -> (job id, generated frame count, keep index 0?). Land at
# RawArt/Renders/fx-slash/raw/<name>/frames/frame_N.png, which godot_pack.py packs as fx_<name>.
FX = {
    "bolt": ("7db9d1ec-ea12-4cba-81b7-4c89b315c446", 4, False),
    "bolt_hit": ("6ffe1e78-150f-4bee-b82a-26c27b356630", 6, True),
    "wall_fire": ("abb38e25-8d1f-493f-8388-198bf369380e", 4, False),
    "ring": ("92517467-006b-4d74-a6a0-7726ca992467", 6, True),
    "smash": ("1f5ca7ac-cc8c-402e-8f6a-01ff41904c92", 6, True),
    "smoke_green": ("400b2600-1361-40f2-a3f2-0c18afae7ad2", 4, False),
    "mend": ("6500e32c-34ce-46cb-93f2-25b180349ce9", 6, True),
    "orb": ("0bfe229b-e6cc-4c6f-8ed4-64e46f0dd295", 4, False),
    "relic": ("b05eddad-3dfd-46f2-ae57-0c044ec1b5ef", 6, True),
    # snappy hit sparks (32 px, 4 frames): melee impact alternates star / x
    "hit_star": ("0033daa5-595e-4842-ab83-d08d5e78690f", 4, True),
    "hit_x": ("b9e59957-54b3-4749-a6f6-fc1f85f761eb", 4, True),
    "hit_ring": ("19dfd740-ce48-42c3-a287-c9dc47d9dc9f", 4, True),
}


def fetch_fx() -> int:
    pending = 0
    for name, (job, n, keep0) in FX.items():
        if not job:
            continue
        d = REPO / "RawArt" / "Renders" / "fx-slash" / "raw" / name / "frames"
        first = 0 if keep0 else 1
        if all((d / f"frame_{i - first}.png").exists() for i in range(first, n + 1)):
            print(f"done     fx_{name}")
            continue
        d.mkdir(parents=True, exist_ok=True)
        if all(fetch(job, i, d / f"frame_{i - first}.png") for i in range(first, n + 1)):
            print(f"fetched  fx_{name} x{n + 1 - first}")
        else:
            for f in d.glob("frame_*.png"):
                f.unlink()
            pending += 1
            print(f"pending  fx_{name}")
    return pending


def dest_dir(unit: str, clip: str, facing: str) -> Path:
    return Path(gp.rotations_dir(ROSTER[unit], SELECTS)).parent / f"{clip}_{facing}"


def fetch(job: str, index: int, out: Path) -> bool:
    try:
        with urllib.request.urlopen(API % (job, index), timeout=60) as r:
            data = r.read()
    except urllib.error.HTTPError:
        return False
    if not data.startswith(b"\x89PNG"):
        return False
    out.write_bytes(data)
    return True


def main() -> None:
    status_only = "--status" in sys.argv
    pending = 0
    for (unit, clip, facing), (job, n) in JOBS.items():
        d = dest_dir(unit, clip, facing)
        first = 1 if clip in LOOPS else 0
        want = list(range(first, n + 1))
        have = all((d / f"frame_{i - first}.png").exists() for i in want)
        if have:
            print(f"done     {unit:16s} {clip}_{facing}")
            continue
        if status_only:
            print(f"pending  {unit:16s} {clip}_{facing}")
            pending += 1
            continue
        d.mkdir(parents=True, exist_ok=True)
        ok = True
        for i in want:
            if not fetch(job, i, d / f"frame_{i - first}.png"):
                ok = False
                break
        if not ok:
            for f in d.glob("frame_*.png"):
                f.unlink()
            pending += 1
            print(f"pending  {unit:16s} {clip}_{facing}")
        else:
            print(f"fetched  {unit:16s} {clip}_{facing} x{len(want)}")
    if not status_only:
        pending += fetch_fx()
    print(f"{pending} pending")


if __name__ == "__main__":
    main()
