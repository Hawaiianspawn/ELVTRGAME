"""Kindled combat SFX: silent placeholders + trim/normalise a picked Sonniss source clip.

    py Scripts/art/sfx.py placeholders                       # write missing cue files as silence
    py Scripts/art/sfx.py pick <src.wav> <out-name> [--len 1.2]

`placeholders` reads godot/data/units.json (per-unit "sfx", top-level "hero_sfx") and
godot/data/spells.json (per-spell "sfx") for every filename the game references, and writes a
0.1s silent mono 44.1kHz WAV for any that don't exist yet under godot/assets/sfx/ — so every
Sound.gd hook resolves even before the real Sonniss library is picked from.

`pick` takes one WAV file (16/24-bit PCM or 32-bit float, mono or stereo, any sample rate) and writes a
game-ready cue: strips leading silence below -40 dBFS, cuts to --len seconds (hard cap 1.5s),
resamples to 44.1kHz by linear interpolation, peak-normalises to -1 dBFS, fades the last 40 ms, and
writes 16-bit mono. `--pitch -4` shifts four semitones down tape-style (slower and longer) before the cut; `--layer <wav>`
mixes a second clip under it from its onset. An out-name with a directory writes there instead of godot/assets/sfx.
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
FADE_SECS = 0.04


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


def _read_float_wav(path: str) -> tuple[int, int, int, bytes]:
    """RIFF walk for 32-bit float WAVs (format tag 3 / extensible), which stdlib wave rejects."""
    import struct
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise SystemExit(f"not a RIFF/WAVE file: {path}")
    pos, fmt, pcm = 12, None, b""
    while pos + 8 <= len(data):
        cid, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = struct.unpack("<HHIIHH", body[:16])
        elif cid == b"data":
            pcm = body
        pos += 8 + size + (size & 1)
    if fmt is None or not pcm:
        raise SystemExit(f"no fmt/data chunk in {path}")
    tag, nch, sr, _, _, bits = fmt
    if bits != 32:
        raise SystemExit(f"unsupported {bits}-bit format tag {tag} in {path}")
    return nch, sr, 4, pcm


def _read_mono_floats(path: str) -> tuple[list[float], int]:
    try:
        with wave.open(path, "rb") as wf:
            nch = wf.getnchannels()
            sw = wf.getsampwidth()
            sr = wf.getframerate()
            n = wf.getnframes()
            raw = wf.readframes(n)
    except wave.Error:
        nch, sr, sw, raw = _read_float_wav(path)
    if sw == 4:
        fl = array.array("f")
        fl.frombytes(raw[:len(raw) - len(raw) % 4])
        floats = list(fl)
    elif sw == 2:
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
        raise SystemExit(f"unsupported sample width {sw * 8}-bit in {path} (need 16/24-bit PCM or 32-bit float)")
    if nch > 1:
        floats = [sum(floats[i:i + nch]) / nch for i in range(0, len(floats) - len(floats) % nch, nch)]
    return floats, sr


def _lowpass(x: list[float], sr: int, fc: float, passes: int = 2) -> list[float]:
    """Butterworth 2nd-order biquad, run `passes` times (2 = 24 dB/oct)."""
    import math
    w0 = 2.0 * math.pi * fc / sr
    cw, sw = math.cos(w0), math.sin(w0)
    alpha = sw / (2.0 * 0.7071)
    b0 = (1.0 - cw) / 2.0
    b1 = 1.0 - cw
    b2 = b0
    a0 = 1.0 + alpha
    a1 = -2.0 * cw
    a2 = 1.0 - alpha
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    for _ in range(passes):
        y = []
        x1 = x2 = y1 = y2 = 0.0
        for s in x:
            v = b0 * s + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
            x2, x1, y2, y1 = x1, s, y1, v
            y.append(v)
        x = y
    return x


def retro(mono: list[float], sr: int, rate: int, bits: int) -> list[float]:
    """Old-sampler treatment: anti-alias lowpass, sample-hold down to `rate`, quantise to `bits`."""
    mono = _lowpass(mono, sr, rate * 0.42)
    hold = max(1, round(sr / rate))
    levels = float(1 << (bits - 1))
    out = []
    for i in range(0, len(mono), hold):
        q = round(mono[i] * levels) / levels
        out.extend([q] * min(hold, len(mono) - i))
    return out


def _resample(mono: list[float], step: float) -> list[float]:
    """Linear-interpolation resample: output sample i reads input position i*step."""
    n_out = max(1, round(len(mono) / step))
    last = len(mono) - 1
    out = []
    for i in range(n_out):
        pos = i * step
        lo = min(int(pos), last)
        hi = min(lo + 1, last)
        frac = pos - lo
        out.append(mono[lo] * (1.0 - frac) + mono[hi] * frac)
    return out


def _detune(mono: list[float], cents: float) -> list[float]:
    """Synth-ensemble thickener: the clip mixed with copies of itself resampled ±cents."""
    up = _resample(mono, 2.0 ** (cents / 1200.0))
    dn = _resample(mono, 2.0 ** (-cents / 1200.0))
    out = []
    for i, s in enumerate(mono):
        v = s * 0.6
        if i < len(up):
            v += up[i] * 0.35
        if i < len(dn):
            v += dn[i] * 0.35
        out.append(v)
    return out


def _echo(mono: list[float], sr: int, delay_s: float, fb: float) -> list[float]:
    """Feedback comb echo; the buffer grows three repeats past the dry end (capped later)."""
    d = max(1, int(delay_s * sr))
    out = mono + [0.0] * d * 3
    for i in range(d, len(out)):
        out[i] += out[i - d] * fb
    return out


def pick(src: str, out_name: str, out_len: float = 1.2, rate: int = 0, bits: int = 16, pitch: float = 0.0,
         layer: str = "", layer_db: float = -6.0, echo: str = "", detune: float = 0.0,
         start: float = 0.0) -> tuple[str, float]:
    mono, sr = _read_mono_floats(src)
    if start:
        mono = mono[int(start * sr):]   # slice a take out of a longer recording (source time)
    if pitch:
        # tape-style shift: pitch down = stretch (longer), pitch up = squeeze
        mono = _resample(mono, 2.0 ** (pitch / 12.0))

    # onset: first sample within 20 dB of the clip's peak, minus 10 ms pre-roll — the old -40 dBFS
    # gate kept up to half a second of room tone in front of the transient
    peak_in = max((abs(s) for s in mono), default=0.0)
    thresh = max(_db_to_amp(SILENCE_DBFS), peak_in * _db_to_amp(-20.0))
    start = next((i for i, s in enumerate(mono) if abs(s) >= thresh), 0)
    mono = mono[max(0, start - int(0.010 * sr)):]

    max_len = min(out_len, HARD_CAP_SECS)
    mono = mono[:int(max_len * sr)]

    if sr != SR_OUT and mono:
        mono = _resample(mono, sr / SR_OUT)

    if layer:
        # second clip mixed under the first from its onset, peak-matched then dropped by layer_db
        lay, lsr = _read_mono_floats(layer)
        lpeak = max((abs(s) for s in lay), default=0.0)
        lthresh = max(_db_to_amp(SILENCE_DBFS), lpeak * _db_to_amp(-20.0))
        lstart = next((i for i, s in enumerate(lay) if abs(s) >= lthresh), 0)
        lay = lay[max(0, lstart - int(0.010 * lsr)):]
        if lsr != SR_OUT:
            lay = _resample(lay, lsr / SR_OUT)
        peak = max((abs(s) for s in mono), default=0.0)
        lpeak = max((abs(s) for s in lay), default=0.0)
        g = (peak / lpeak) * _db_to_amp(layer_db) if lpeak > 0.0 and peak > 0.0 else 0.0
        for i in range(min(len(mono), len(lay))):
            mono[i] += lay[i] * g

    if detune:
        mono = _detune(mono, detune)
    if echo:
        ms, fb = (echo.split(",") + ["0.45"])[:2]
        mono = _echo(mono, SR_OUT, float(ms) / 1000.0, float(fb))
        mono = mono[:int(HARD_CAP_SECS * SR_OUT)]   # --len sets the dry cut; the tail rings to the cap

    peak = max((abs(s) for s in mono), default=0.0)
    if peak > 0.0:
        scale = _db_to_amp(NORM_DBFS) / peak
        mono = [s * scale for s in mono]

    # fade-out so a clip still ringing at the --len cap doesn't end in a click
    n_fade = min(len(mono), int(FADE_SECS * SR_OUT))
    for i in range(n_fade):
        mono[len(mono) - n_fade + i] *= 1.0 - (i + 1) / n_fade

    if rate:
        mono = retro(mono, SR_OUT, rate, bits)

    ints_out = array.array("h", (max(-32768, min(32767, round(s * 32767))) for s in mono))
    out_path = out_name if os.path.dirname(out_name) else os.path.join(OUT_DIR, out_name)
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with wave.open(out_path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SR_OUT)
        wf.writeframes(ints_out.tobytes())
    dur = len(ints_out) / SR_OUT
    print(f"wrote {out_path} ({dur:.2f}s, from {src})")
    return out_path, dur


def loop(src: str, out_name: str, start: float = 0.0, out_len: float = 24.0, xfade: float = 1.0) -> None:
    """Seam-free ambient loop: slice, resample, crossfade the post-end tail into the head."""
    mono, sr = _read_mono_floats(src)
    mono = mono[int(start * sr):int((start + out_len + xfade) * sr)]
    if sr != SR_OUT:
        mono = _resample(mono, sr / SR_OUT)
    n = min(int(out_len * SR_OUT), len(mono))
    nx = int(xfade * SR_OUT)
    body = mono[:n]
    tail = mono[n:n + nx]
    for i in range(min(nx, len(tail), len(body))):
        t = (i + 1) / nx
        body[i] = body[i] * t + tail[i] * (1.0 - t)   # head continues where the loop end left off
    peak = max((abs(s) for s in body), default=0.0)
    if peak > 0.0:
        body = [s * _db_to_amp(NORM_DBFS) / peak for s in body]
    ints_out = array.array("h", (max(-32768, min(32767, round(s * 32767))) for s in body))
    out_path = out_name if os.path.dirname(out_name) else os.path.join(OUT_DIR, out_name)
    with wave.open(out_path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SR_OUT)
        wf.writeframes(ints_out.tobytes())
    print(f"wrote {out_path} ({len(ints_out) / SR_OUT:.2f}s loop, from {src})")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("placeholders")
    p_pick = sub.add_parser("pick")
    p_pick.add_argument("src")
    p_pick.add_argument("out_name")
    p_pick.add_argument("--len", type=float, default=1.2, dest="out_len")
    p_pick.add_argument("--retro", action="store_true", help="old-sampler treatment: 11025 Hz sample-hold, 8-bit")
    p_pick.add_argument("--rate", type=int, default=0, help="sample-hold rate in Hz (implies retro)")
    p_pick.add_argument("--bits", type=int, default=8, help="quantise depth when retro (default 8)")
    p_pick.add_argument("--layer", default="", help="second source WAV mixed under the first from its onset")
    p_pick.add_argument("--layer-db", type=float, default=-6.0, dest="layer_db", help="layer level relative to the main clip (default -6)")
    p_pick.add_argument("--pitch", type=float, default=0.0, help="shift in semitones, negative = down (tape-style, changes length)")
    p_pick.add_argument("--echo", default="", help="feedback echo 'delay_ms[,feedback]'; tail rings past --len up to the 1.5s cap")
    p_pick.add_argument("--detune", type=float, default=0.0, help="synth-ensemble detune in cents: mixes ±cents copies under the clip")
    p_pick.add_argument("--start", type=float, default=0.0, help="seconds into the source to slice from (cut one take out of many)")
    p_loop = sub.add_parser("loop", help="ambient loop: no 1.5s cap, tail crossfaded into the head for seam-free looping")
    p_loop.add_argument("src")
    p_loop.add_argument("out_name")
    p_loop.add_argument("--start", type=float, default=0.0)
    p_loop.add_argument("--len", type=float, default=24.0, dest="out_len")
    p_loop.add_argument("--xfade", type=float, default=1.0)
    args = ap.parse_args()
    if args.cmd == "placeholders":
        placeholders()
    elif args.cmd == "loop":
        loop(args.src, args.out_name, args.start, args.out_len, args.xfade)
    elif args.cmd == "pick":
        rate = args.rate or (11025 if args.retro else 0)
        pick(args.src, args.out_name, args.out_len, rate, args.bits if rate else 16, args.pitch, args.layer, args.layer_db,
             args.echo, args.detune, args.start)


if __name__ == "__main__":
    main()
