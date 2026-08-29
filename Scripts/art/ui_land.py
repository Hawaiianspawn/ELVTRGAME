"""Copy the kept PixelLab UI pieces from RawArt/Renders/ui into godot/assets/ui.

    py Scripts/art/ui_land.py

Keep/reject lives here as the KEEP table; re-run after re-cropping. Fonts and
icons copy as-is; kit crops are renamed to what the game code loads.
"""
import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
RAW = REPO / "godot" / "RawArt" / "Renders" / "ui"
OUT = REPO / "godot" / "assets" / "ui"

KEEP = {
    # fonts
    "fonts/font_head16.ttf": "fonts/head16.ttf",
    "fonts/font_body16.ttf": "fonts/body16.ttf",
    "fonts/font_title32_b.ttf": "fonts/title32.ttf",
    # panels / kit pieces (crops are row-major from ui_crop.py)
    "crops/army_idle_b_00.png": "panel_idle.png",       # 359x185 stone card, iron corner bosses
    "crops/army_sel_b_00.png": "panel_sel.png",         # grey stone card, gold filigree corners (sel_a had a baked checkerboard)
    "crops/kit_pause_00.png": "frame_gargoyle.png",     # 233x155 gargoyle lintel panel
    "crops/kit_c_00.png": "frame_lintel.png",           # 481x113 griffin lintel + rosettes, hollow
    "crops/kit_medallion_13.png": "medallion.png",      # 98x98 laurel ring
    "crops/kit_hero_card_00.png": "hero_card.png",      # 169x212 tall card
    "crops/kit_pause_02.png": "button.png",             # 151x45
    "crops/kit_pause_05.png": "chip.png",               # 57x57 square slot
    "crops/kit_pause_12.png": "socket.png",             # 45x45 small square slot
    "crops/kit_pause_15.png": "bar.png",                # 281x29 thin bar housing
    "crops/kit_pause_09.png": "plate.png",              # 193x37 nameplate
    "crops/ui_toast_banner_00.png": "banner.png",       # 642x281 hanging banner, 9-slice
    # icons
    "icons/icon_bolt.png": "icons/bolt.png",
    "icons/icon_mend.png": "icons/heal.png",
    "icons/icon_wall.png": "icons/wall.png",
    "icons/icon_ember_ring.png": "icons/ember_ring.png",
    "icons/icon_iron_hymn.png": "icons/iron_hymn.png",
    "icons/icon_echo_bell.png": "icons/echo_bell.png",
    "icons/icon_green_tooth.png": "icons/green_tooth.png",
    "icons/icon_witch_lamp.png": "icons/witch_lamp.png",
    "icons/cursor_a.png": "cursor.png",
    "icons/crosshair_a.png": "crosshair.png",
}


def patch_banner(path: Path) -> None:
    """The generator rendered the piece label as literal text mid-cloth; cover it with cloth from the left."""
    from PIL import Image

    im = Image.open(path).convert("RGBA")
    patch = im.crop((130, 120, 250, 160))
    im.paste(patch, (262, 120))
    im.save(path)


HEROES = ["hero_knight", "hero_turret", "hero_sackhauler", "hero_dwarf", "hero_cover", "hero_samurai", "hero_ranger"]


def main() -> None:
    for h in HEROES:   # 64px busts from create_portrait_character(character_to_portrait)
        KEEP[f"../portraits/out/{h}.png"] = f"portraits/{h}.png"
    for src, dst in KEEP.items():
        d = OUT / dst
        d.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(RAW / src, d)
        if dst == "banner.png":
            patch_banner(d)
        print(dst)


if __name__ == "__main__":
    main()
