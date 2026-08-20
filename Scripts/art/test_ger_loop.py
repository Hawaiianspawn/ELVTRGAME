# test_ger_loop.py -- drives the whole generate/evaluate/refine loop end to end on
# synthetic sprites, in a temp tree, with no PixelLab call and no credits spent.
#
#   py Scripts/art/test_ger_loop.py
#
# test_variantpipe.py pins the MEASUREMENTS against real renders on disk. This
# file pins the LOOP: that judge's verdicts drive refine, that refine amends the
# recorded prompt and moves the old render aside instead of deleting it, that a
# variant which keeps failing burns exactly MAX_ATTEMPTS and then escalates
# instead of regenerating forever, and that the failure kinds no reword can fix
# escalate on sight without spending an attempt.
#
# The circuit breaker is the reason this exists. Everything else here was
# observable on the real families; the breaker only fires on the fourth pass of a
# variant that never improves, which no real run has reached.

import json
import shutil
import sys
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import silhouette_report as sr  # noqa: E402
import variantpipe as vp  # noqa: E402

CANVAS = 64


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def sprite(path, w, h, colours=3):
    """A centred solid block carrying exactly min(colours, w*h) distinct RGBs.

    Alpha is binary and the block is solid, so aspect is exactly w/h and every
    other measure is predictable -- the point is to control the numbers judge
    sees, not to look like a knight. Colours are spread per PIXEL, not per row:
    the first draft varied by row, which capped a 30-row sprite at 30 distinct
    values and could never reach the 256 ceiling the colour rule tests.
    """
    a = np.zeros((CANVAS, CANVAS, 4), dtype=np.uint8)
    y0, x0 = (CANVAS - h) // 2, (CANVAS - w) // 2
    for i in range(h):
        for j in range(w):
            k = (i * w + j) % max(1, colours)
            a[y0 + i, x0 + j] = (30 + k % 200, 40 + (k // 7) % 200,
                                 50 + (k // 13) % 200, 255)
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(a, "RGBA").save(path)


def rotations(root, variant, w, h, colours=3):
    for d in sr.DIRECTIONS:
        sprite(root / variant / "rotations" / ("%s.png" % d), w, h, colours)


def write_spec(fam_dir, variants):
    fam_dir.mkdir(parents=True, exist_ok=True)
    (fam_dir / "family.json").write_text(json.dumps({
        "family": fam_dir.name,
        "axis": {"name": "aspect", "description": "synthetic"},
        "constant": "synthetic",
        "base": {"character_id": "test-character", "canvas_size": CANVAS},
        "variants": variants,
    }, indent=2), encoding="utf-8")


class Args:
    def __init__(self, family, dry_run=False):
        self.family = family
        self.dry_run = dry_run


def capture(fn, *a):
    """Run a cmd_* and give back (exit code, stdout, stderr)."""
    import io
    from contextlib import redirect_stderr, redirect_stdout
    out, err = io.StringIO(), io.StringIO()
    with redirect_stdout(out), redirect_stderr(err):
        rc = fn(*a)
    return rc, out.getvalue(), err.getvalue()


def main():
    tmp = Path(tempfile.mkdtemp(prefix="ger-loop-"))
    # REPO comes along because manifest paths are recorded relative to it. In
    # production RENDERS is always inside the repo, so variantpipe is right to
    # assume it -- the temp tree is the only place that assumption breaks, and
    # that is the harness's problem to solve, not a guard to add to the product.
    orig = (vp.FAMILIES, vp.RENDERS, vp.REPO)
    vp.FAMILIES, vp.RENDERS, vp.REPO = tmp / "families", tmp / "renders", tmp
    try:
        run(tmp)
    finally:
        vp.FAMILIES, vp.RENDERS, vp.REPO = orig
        shutil.rmtree(tmp, ignore_errors=True)
    return 0


def run(tmp):
    FAM = "synth"
    fam_dir = vp.FAMILIES / FAM
    raw = vp.RENDERS / FAM / "raw"

    # wide:  40x20, aspect 2.00, no target       -> keep
    # short: 20x36, aspect 0.56, target 2.00     -> flag, and reachable by reword
    # gaudy: 30x30 with 900 colours              -> flag on the colour ceiling
    #
    # The three opaque areas (800 / 720 / 900 px) are deliberately unequal. The
    # first draft used 40x20 and 20x36 -- both 800px -- and judge rejected them as
    # the same outline, which is that rule working, not a bug. Keep them apart.
    write_spec(fam_dir, [
        {"slug": "wide", "edit_description": "a wide block", "source": "generated"},
        {"slug": "short", "edit_description": "a tall block", "silhouette_target": 2.00,
         "source": "generated"},
        {"slug": "gaudy", "edit_description": "a noisy block", "source": "generated"},
    ])
    rotations(raw, "wide", 40, 20)
    rotations(raw, "short", 20, 36)
    rotations(raw, "gaudy", 30, 30, colours=900)

    # ---- 1. the evaluator separates the three ------------------------------
    _, verdicts, _ = vp.judge_family(FAM)
    check(verdicts["wide"]["verdict"] == "keep",
          "a variant meeting every rule must keep, got %s" % verdicts["wide"])
    check(verdicts["short"]["verdict"] == "flag",
          "aspect 0.50 against a 2.00 target must flag, got %s" % verdicts["short"]["verdict"])
    check(any("aspect target" in r for r in verdicts["short"]["reasons"]),
          "the flag must name the aspect target, got %s" % verdicts["short"]["reasons"])
    check(verdicts["gaudy"]["verdict"] == "flag",
          "a 900-colour sprite must trip the colour ceiling, got %s" % verdicts["gaudy"]["verdict"])
    check(any("distinct colours" in r for r in verdicts["gaudy"]["reasons"]),
          "the colour flag must name the count, got %s" % verdicts["gaudy"]["reasons"])

    # ---- 2. refine amends the prompt and MOVES the render ------------------
    before = (fam_dir / "family.json").read_text(encoding="utf-8")
    rc, out, err = capture(vp.cmd_refine, Args(FAM))
    check(rc == 0, "a refinable batch must exit 0, got %d" % rc)

    spec = json.loads((fam_dir / "family.json").read_text(encoding="utf-8"))
    by = {v["slug"]: v for v in spec["variants"]}
    check("WIDER" in by["short"]["edit_description"],
          "short's prompt must carry the widen directive, got %r" % by["short"]["edit_description"])
    check(by["wide"]["edit_description"] == "a wide block",
          "a kept variant's prompt must not be touched")
    check(not (raw / "short").exists(), "the failing render must leave raw/")
    culled = vp.RENDERS / FAM / "rejected" / "short" / "rotations"
    check(len(list(culled.glob("*.png"))) == len(sr.DIRECTIONS),
          "all 8 rotations must survive the cull -- retention rule, moved not deleted")
    check((raw / "wide").exists(), "a kept variant must not be culled")

    man = json.loads((fam_dir / "manifest.json").read_text(encoding="utf-8"))
    check(man["variants"]["short"]["refine"]["attempts"] == 1,
          "first refine must record attempt 1")
    check(man["variants"]["short"]["refine"]["original"] == "a tall block",
          "the untouched original prompt must be preserved for later attempts")

    # ---- 3. the prompt never stacks across attempts ------------------------
    # Regenerate at the SAME failing shape twice more: the loop must keep trying
    # and keep rebuilding the directive from the original, not from its own
    # previous output.
    for attempt in (2, 3):
        rotations(raw, "short", 20, 36)
        rc, out, err = capture(vp.cmd_refine, Args(FAM))
        check(rc == 0, "attempt %d must still run, got rc=%d / %s" % (attempt, rc, err))
        spec = json.loads((fam_dir / "family.json").read_text(encoding="utf-8"))
        desc = {v["slug"]: v for v in spec["variants"]}["short"]["edit_description"]
        check(desc.count("WIDER") == 1,
              "attempt %d stacked directives -- %d copies in one prompt"
              % (attempt, desc.count("WIDER")))
        man = json.loads((fam_dir / "manifest.json").read_text(encoding="utf-8"))
        check(man["variants"]["short"]["refine"]["attempts"] == attempt,
              "attempt counter must read %d" % attempt)

    # ---- 4. THE CIRCUIT BREAKER -------------------------------------------
    rotations(raw, "short", 20, 36)
    rc, out, err = capture(vp.cmd_refine, Args(FAM))
    check(rc == 2, "an exhausted variant must exit non-zero, got %d" % rc)
    check("ESCALATED" in err, "the breaker must escalate on stderr, got %r" % err)
    check("3 attempts spent" in err,
          "the escalation must say how many attempts were spent, got %r" % err)
    check((raw / "short").exists(),
          "an escalated variant must be LEFT ALONE -- culling it after the loop gave "
          "up destroys the render the owner has to look at")
    man = json.loads((fam_dir / "manifest.json").read_text(encoding="utf-8"))
    check(man["variants"]["short"]["refine"]["attempts"] == 3,
          "the breaker must not spend a fourth attempt")

    # ---- 5. unpromptable failures escalate without spending an attempt -----
    write_spec(fam_dir, [{"slug": "wrongsize", "edit_description": "a block",
                          "source": "generated"}])
    (fam_dir / "manifest.json").unlink()
    shutil.rmtree(raw)
    for d in sr.DIRECTIONS:
        a = np.zeros((32, 32, 4), dtype=np.uint8)   # canvas 32, spec says 64
        a[8:24, 8:24] = (90, 90, 90, 255)
        p = raw / "wrongsize" / "rotations" / ("%s.png" % d)
        p.parent.mkdir(parents=True, exist_ok=True)
        Image.fromarray(a, "RGBA").save(p)

    _, verdicts, _ = vp.judge_family(FAM)
    check(verdicts["wrongsize"]["verdict"] == "reject",
          "a canvas mismatch must reject, got %s" % verdicts["wrongsize"]["verdict"])
    rc, out, err = capture(vp.cmd_refine, Args(FAM))
    check(rc == 2, "an unpromptable failure must exit non-zero, got %d" % rc)
    check("not a prompt problem" in err,
          "the escalation must say a reword cannot fix it, got %r" % err)
    man = json.loads((fam_dir / "manifest.json").read_text(encoding="utf-8"))
    check(man["variants"]["wrongsize"]["refine"]["attempts"] == 0,
          "a canvas mismatch must not burn an attempt on a reword that cannot work")
    check((raw / "wrongsize").exists(), "an escalated variant must not be culled")

    # ---- 6. --dry-run writes nothing --------------------------------------
    write_spec(fam_dir, [{"slug": "short", "edit_description": "a tall block",
                          "silhouette_target": 2.00, "source": "generated"}])
    (fam_dir / "manifest.json").unlink()
    shutil.rmtree(raw)
    rotations(raw, "short", 20, 36)
    snapshot = (fam_dir / "family.json").read_text(encoding="utf-8")
    rc, out, err = capture(vp.cmd_refine, Args(FAM, dry_run=True))
    check("DRY RUN" in out, "a dry run must say so")
    check((fam_dir / "family.json").read_text(encoding="utf-8") == snapshot,
          "--dry-run must not touch family.json")
    check(not (fam_dir / "manifest.json").exists(),
          "--dry-run must not write a manifest")
    check((raw / "short").exists(), "--dry-run must not cull")

    print("OK -- loop verified end to end on synthetic sprites, no credits spent:\n"
          "     evaluator separates keep/flag on both rules (aspect target, %d-colour ceiling)\n"
          "     refiner amends the prompt, culls by MOVING all 8 rotations, leaves keeps alone\n"
          "     %d attempts rebuild the directive from the original, never stacking\n"
          "     circuit breaker fires on attempt %d+1, exits 2, and leaves the render in place\n"
          "     unpromptable failures escalate on sight without spending an attempt\n"
          "     --dry-run writes nothing"
          % (vp.MAX_DISTINCT_COLOURS, vp.MAX_ATTEMPTS, vp.MAX_ATTEMPTS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
