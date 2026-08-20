"""Pull every finished animate_image job in a probe's jobs.json onto disk.

    py Scripts/art/fetch_probe_frames.py RawArt/Renders/brood-anim-probe

Frames land at <root>/raw/<look>/<action>/frame_N.png. Already-downloaded frames
are skipped, so this is safe to re-run while jobs are still finishing -- it
reports what is still missing and exits non-zero until the set is complete.

Per the PixelLab retention rule everything downloaded stays; nothing here culls.
"""
import json
import os
import sys
import urllib.error
import urllib.request

FRAMES = 9  # frame_count 8 plus the untouched input frame at index 0; jobs.json "frames" overrides per job
URL = "https://api.pixellab.ai/mcp/images/%s/download?index=%d"


def fetch(job, dest, frames=FRAMES):
    got = 0
    for i in range(frames):
        out = os.path.join(dest, "frame_%d.png" % i)
        if os.path.exists(out) and os.path.getsize(out) > 0:
            got += 1
            continue
        try:
            with urllib.request.urlopen(URL % (job, i), timeout=30) as r:
                data = r.read()
        except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError):
            continue
        if not data.startswith(b"\x89PNG"):
            continue
        os.makedirs(dest, exist_ok=True)
        open(out, "wb").write(data)
        got += 1
    return got


def main(root):
    spec = json.load(open(os.path.join(root, "jobs.json")))
    missing = []
    for look, actions in sorted(spec["jobs"].items()):
        for action, job in sorted(actions.items()):
            dest = os.path.join(root, "raw", look, action)
            want = spec.get("frames", {}).get(look, FRAMES)
            got = fetch(job, dest, want)
            if got < want:
                missing.append("%s/%s %d/%d" % (look, action, got, want))
    if missing:
        print("still pending (%d):" % len(missing))
        for m in missing:
            print("  " + m)
        return 1
    print("complete: %d jobs, %d frames" % (
        sum(len(a) for a in spec["jobs"].values()),
        sum(len(a) for a in spec["jobs"].values()) * FRAMES))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
