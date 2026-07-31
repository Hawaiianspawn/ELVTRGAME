"""Drive PIE on L_Spike1 and grab the STYLED game frame, one capture per config.

    py docs/perf/evidence/black-frame/capture.py <name> [Cvar Value ...]

Writes ELVTR/Saved/SwarmExecOnPlay.txt = the owner's file + the overrides given on
the command line, runs one PIE session, screenshots the editor, crops the PIE
viewport out of it, and saves <name>.png next to this script. Restores the owner's
exec file before it exits, always.

WHY NOT Swarm.DebugShotAfter -- measured 2026-07-31, task-122:
  Its SceneCapture2D shot does NOT contain the demichrome post-process. Captures
  taken with Swarm.DebugPlainView 1 and 0 came back BYTE-IDENTICAL (md5
  d09802fa8239f2124a7511c7d179ced3 both), mean luminance 2.05/255, max 12/255, with
  no flame pool anywhere -- while the same frame on screen is a bright dithered floor
  peaking at 255. The constructor comment in SwarmRenderActor.cpp claiming
  SCS_FinalColorLDR "is what pulls the demichrome post-process into the capture" is
  wrong; the CVar's own help text ("NOT VALID FOR JUDGING ART") is right.
  Any lighting measurement taken through that path measured an unlit scene.

So this uses the editor screenshot instead: it is literally the frame the owner
photographed, post-process and all. Cost is resolution (the viewport is whatever
size the editor window is) and imprecise timing -- CAPTURE_AT is best-effort, not
frame-exact, so unit positions differ slightly between runs.
"""
import base64, json, subprocess, sys, time
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
PROJ = HERE.parents[3] / "ELVTR"
EXEC = PROJ / "Saved" / "SwarmExecOnPlay.txt"
TOOLSET = "EditorToolset.EditorAppToolset"
MCP_PS1 = HERE.parents[3] / "Scripts" / "ue-mcp.ps1"
BACKUP = Path(r"C:\Users\HAWAII~1\AppData\Local\Temp\claude"
              r"\C--Projects-ELVTRGAME-ELVTR\f51d1379-66cd-4a59-880e-aeff967ad4f4"
              r"\scratchpad\SwarmExecOnPlay.BACKUP.txt")

# t=0 retinue deploy, t=1.0 wave 1 spawns (Spike1GameMode DeploySeconds / WaveBroodCounts
# {250,450,700}). Every run is pinned to SLOMO so the shot lands at a repeatable point in
# the sim: CaptureEditorImage fires whenever the MCP round-trip lands, roughly +1.5s of
# wall clock after the warmup, and at full speed that jitter moves the horde hundreds of
# uu between runs. At 0.2x it moves them tens. 13s of warmup puts the shot near sim t=2.9,
# wave 1 marching in with the front rank around the pool edge.
SLOMO = 0.2
CAPTURE_AT = 13.0

# Flicker is pinned off in EVERY run, baseline included. Swarm.FlameFlicker 0.06 swings the
# pool +/-6% frame to frame, which is noise on top of every dial being measured. Same value
# on both sides of the comparison, so it cannot flatter one of them.
PINNED = [("Swarm.FlameFlicker", 0), ("slomo", SLOMO)]


def tool(name, args=None):
    """One MCP call, through the repo's existing PowerShell client.

    Not a hand-rolled HTTP client: the plugin's streamable-http endpoint answers a
    tools/call with 200 and an EMPTY body to a plain urllib request, so a Python
    client silently gets nothing back while the tool does run. Scripts/ue-mcp.ps1
    already talks to it correctly, so this borrows it rather than debugging it.
    """
    ps = [f". '{MCP_PS1}'"]
    call = f"Invoke-McpTool -Toolset '{TOOLSET}' -ToolName '{name}'"
    if args:
        call += " -Arguments (@'\n" + json.dumps(args) + "\n'@ | ConvertFrom-Json)"
    # Straight through a file: a base64 PNG is far past what stdout survives intact.
    ps.append(f"[IO.File]::WriteAllText('{HERE / '_mcp.json'}', ({call}))")
    p = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", "; ".join(ps)],
                       capture_output=True, text=True, timeout=600)
    if p.returncode != 0:
        raise RuntimeError(f"{name}: {p.stderr.strip()}")
    return (HERE / "_mcp.json").read_text(encoding="utf-8")


def viewport(img):
    """Crop the PIE viewport out of a whole-editor screenshot.

    Found by texture, not by hard-coded offsets, so it survives the owner moving a
    panel: editor chrome is FLAT fill, and every part of the game frame carries the
    Bayer dither -- including the black band up top, which still has a nonzero
    spread. So the viewport is the largest contiguous block of rows and columns
    whose spread is above the flat-fill floor.
    """
    a = np.asarray(img.convert("RGB")).astype(float)
    lum = 0.2126 * a[..., 0] + 0.7152 * a[..., 1] + 0.0722 * a[..., 2]

    def longest(flags):
        best = cur = (0, 0)
        for i, f in enumerate(flags):
            cur = (cur[0], i + 1) if f and cur[1] == i else ((i, i + 1) if f else cur)
            if cur[1] - cur[0] > best[1] - best[0]:
                best = cur
        return best

    r0, r1 = longest(lum.std(axis=1) > 2)
    c0, c1 = longest(lum[r0:r1].std(axis=0) > 2)
    if r1 - r0 < 200 or c1 - c0 < 400:
        raise RuntimeError(f"viewport crop looks wrong: rows {r0}:{r1} cols {c0}:{c1}")
    return img.crop((c0, r0, c1, r1))


def run(name, overrides):
    # The owner's exec file is the thing every run has to hand back untouched, so the
    # pristine copy lives outside the repo and outside Saved/ -- one backup for the whole
    # session, taken before the first override is ever written.
    if not BACKUP.exists():
        BACKUP.parent.mkdir(parents=True, exist_ok=True)
        BACKUP.write_text(EXEC.read_text(encoding="utf-8"), encoding="utf-8")
    base = BACKUP.read_text(encoding="utf-8")

    lines = [base, "\n; ---- task-122 measurement overrides ----\n"]
    lines += [f"{k} {v}\n" for k, v in PINNED + list(overrides)]
    EXEC.write_text("".join(lines), encoding="utf-8")

    try:
        if "true" in tool("IsPIERunning").lower():
            tool("StopPIE")
            time.sleep(2)
        tool("StartPIE", {"options": {"bSimulate": False,
                                      "playMode": "PlayMode_InViewPort",
                                      "warmupSeconds": CAPTURE_AT}})
        raw = json.loads(tool("CaptureEditorImage"))["returnValue"]["data"]
        tool("StopPIE")
    finally:
        EXEC.write_text(base, encoding="utf-8")

    shot = HERE / f"_{name}_editor.png"
    shot.write_bytes(base64.b64decode(raw))
    out = HERE / f"{name}.png"
    viewport(Image.open(shot)).save(out)
    shot.unlink()
    print(f"{name}: {out.name} {Image.open(out).size}  "
          f"[{', '.join(f'{k}={v}' for k, v in overrides) or 'shipped'}]")
    return out


if __name__ == "__main__":
    a = sys.argv[1:]
    if not a:
        raise SystemExit(__doc__)
    run(a[0], list(zip(a[1::2], a[2::2])))
