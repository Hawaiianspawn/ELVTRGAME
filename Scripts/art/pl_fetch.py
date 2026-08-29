"""Fetch a finished PixelLab MCP result into RawArt/Renders.

    py Scripts/art/pl_fetch.py ui <ui_asset_id> <out.png>
    py Scripts/art/pl_fetch.py image <job_id> <out.png> [index]
    py Scripts/art/pl_fetch.py font <job_id> <out_stem>      -> out_stem.png + out_stem.ttf

No auth: the MCP download endpoints are public by id.
"""
import sys
import urllib.request

API = "https://api.pixellab.ai/mcp"


def get(url: str, out: str) -> None:
    with urllib.request.urlopen(url, timeout=120) as r, open(out, "wb") as f:
        f.write(r.read())
    print(out)


def main() -> None:
    kind, ident, out = sys.argv[1:4]
    if kind == "ui":
        get(f"{API}/ui-assets/{ident}/download", out)
    elif kind == "image":
        idx = sys.argv[4] if len(sys.argv) > 4 else "0"
        get(f"{API}/images/{ident}/download?index={idx}", out)
    elif kind == "font":
        get(f"{API}/font/{ident}/download", out + ".png")
        get(f"{API}/font/{ident}/download_ttf", out + ".ttf")
    else:
        raise SystemExit("kind must be ui | image | font")


if __name__ == "__main__":
    main()
