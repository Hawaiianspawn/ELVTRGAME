"""Kindled combat SFX: silent placeholders + trim/normalise a picked Sonniss source clip.

    py Scripts/art/sfx.py placeholders                       # write missing cue files as silence
    py Scripts/art/sfx.py pick <src.wav> <out-name> [--len 1.2]

`placeholders` reads godot/data/units.json (per-unit "sfx", top-level "hero_sfx") and
godot/data/spells.json (per-spell "sfx") for every filename the game references, and writes a
0.1s silent mono 44.1kHz WAV for any that don't exist yet under godot/assets/sfx/ — so every
Sound.gd hook resolves even before the real Sonniss library is picked from.

`pick` takes one WAV file (16- or 24-bit PCM, mono or stereo, any sample rate) and writes a
game-ready cue: strips leading silence below -40 dBFS, cuts to --len seconds (hard cap 1.5s),
resamples to 44.1kHz by linear interpolation, peak-normalises to -1 dBFS, and writes 16-bit mono.
Pure stdlib (wave + array) — no ffmpeg dependency, though `where ffmpeg` is fine if present;
this script never requires it.
"""
import argparse
import array
import json
import os
import sys
import wave

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DATA = os.path.join(REPO, "godot", "data")
OUT_DIR = os.path.join(REPO, "godot", "assets", "sfx")

SR_OUT = 44100
SILENCE_DBFS = -40.0
NORM_DBFS = -1.0
HARD_CAP_SECS = 1.5


def _db_to_amp(db: float) -> float:
    return 10.0 ** (db / 20.0)


def cue_names() -> set[str]:
    names: set[str] = set()
    units = json.load(open(os.path.join(DATA, "units.json"), encoding="utf-8"))
    for k, v in units.items():
        if not isinstance(v, dict):
            continue
        if k == "hero_sfx":
            names.update(v.values())
        else:
            names.update(v.get("sfx", {}).values())
    spells = json.load(open(os.path.join(DATA, "spells.json"), encoding="utf-8"))
    for s in spells.get("spells", []):
        if "sfx" in s:
            names.add(s["sfx"])
    names.discard("")
    return names


def write_silence(path: str, seconds: float, sr: int = SR_OUT) -> None:
    n = int(seconds * sr)
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sr)
        wf.writeframes(b"\x00\x00" * n)


def placeholders() -> None:
    os.makedirs(OUT_DIR, exist_ok=True)
    names = cue_names()
    made = 0
    for name in sorted(names):
        path = os.path.join(OUT_DIR, name)
        if os.path.exists(path):
            continue
        write_silence(path, 0.1)
        made += 1
    print(f"placeholders: {made} written, {len(names) - made} already present, {len(names)} cues total")


def _read_mono_floats(path: str) -> tuple[list[float], int]:
    with wave.open(path, "rb") as wf:
        nch = wf.getnchannels()
        sw = wf.getsampwidth()
        sr = wf.getframerate()
        n = wf.getnframes()
        raw = wf.readframes(n)
    if sw == 2:
        ints = array.array("h")
        ints.frombytes(raw)
        floats = [s / 32768.0 for s in ints]
    elif sw == 3:
        floats = []
        for i in range(0, len(raw) - 2, 3):
            v = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if v & 0x800000:
                v -= 0x1000000
            floats.append(v / 8388608.0)
    else:
        raise SystemExit(f"unsupported sample width {sw * 8}-bit in {path} (need 16- or 24-bit PCM)")
    if nch > 1:
        floats = [sum(floats[i:i + nch]) / nch for i in range(0, len(floats) - len(floats) % nch, nch)]
    return floats, sr


def pick(src: str, out_name: str, out_len: float = 1.2) -> tuple[str, float]:
    mono, sr = _read_mono_floats(src)

    thresh = _db_to_amp(SILENCE_DBFS)
    start = next((i for i, s in enumerate(mono) if abs(s) >= thresh), 0)
    mono = mono[start:]

    max_len = min(out_len, HARD_CAP_SECS)
    mono = mono[:int(max_len * sr)]

    if sr != SR_OUT and mono:
        n_out = max(1, round(len(mono) * SR_OUT / sr))
        resampled = []
        last = len(mono) - 1
        for i in range(n_out):
            pos = i * sr / SR_OUT
            lo = min(int(pos), last)
            hi = min(lo + 1, last)
            frac = pos - lo
            resampled.append(mono[lo] * (1.0 - frac) + mono[hi] * frac)
        mono = resampled

    peak = max((abs(s) for s in mono), default=0.0)
    if peak > 0.0:
        scale = _db_to_amp(NORM_DBFS) / peak
        mono = [s * scale for s in mono]

    ints_out = array.array("h", (max(-32768, min(32767, round(s * 32767))) for s in mono))
    os.makedirs(OUT_DIR, exist_ok=True)
    out_path = os.path.join(OUT_DIR, out_name)
    with wave.open(out_path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SR_OUT)
        wf.writeframes(ints_out.tobytes())
    dur = len(ints_out) / SR_OUT
    print(f"wrote {out_path} ({dur:.2f}s, from {src})")
    return out_path, dur


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("placeholders")
    p_pick = sub.add_parser("pick")
    p_pick.add_argument("src")
    p_pick.add_argument("out_name")
    p_pick.add_argument("--len", type=float, default=1.2, dest="out_len")
    args = ap.parse_args()
    if args.cmd == "placeholders":
        placeholders()
    elif args.cmd == "pick":
        pick(args.src, args.out_name, args.out_len)


if __name__ == "__main__":
    main()
