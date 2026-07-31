# test_variantpipe.py -- the one runnable check for variantpipe.py + its
# silhouette_report.py extensions. Not a framework: assert-based, run directly.
#
#   py Scripts/art/test_variantpipe.py
#
# Pins the archer-scifi table from docs/backlog/task-081's Findings 1 (measured
# 2026-07-29) and asserts the family judge()s as fully kept -- task-081's own
# warning is that an aspect-spread-only gate would reject this exact family
# (1.5x spread, below the skill's "2.5x+ is good" line) because it actually
# separates on asymmetry (0.30-0.92), not aspect. If this assertion ever fails,
# either the renders on disk changed or judge_family's scoring regressed to
# judging aspect alone -- both are worth stopping for.
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import silhouette_report as sr
import variantpipe as vp

ROOT = Path(__file__).resolve().parents[2] / "RawArt" / "Renders" / "archer-scifi" / "raw"

# (name, content w, content h, aspect, solidity, asymmetry, holes) -- task-081 Findings 1.
EXPECTED = [
    ("state01_marksman", 41, 46, 0.89, 0.52, 0.81, 0),
    ("state02_voltaic", 30, 46, 0.65, 0.71, 0.30, 0),
    ("state03_harpooner", 45, 46, 0.98, 0.54, 0.46, 0),
    ("state04_volley", 43, 48, 0.90, 0.65, 0.49, 1),
    ("state05_kite", 46, 46, 1.00, 0.50, 0.92, 0),
    ("state06_sharpshooter", 43, 46, 0.93, 0.52, 0.67, 0),
]


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def main():
    rows = dict(sr.variants(ROOT))
    check(len(rows) == 6, "expected 6 archer-scifi variants, found %d" % len(rows))

    for name, w, h, aspect, solidity, asym, holes in EXPECTED:
        check(name in rows, "missing variant '%s'" % name)
        m = sr.measure(rows[name])
        check(m["content"] == (w, h),
              "%s: content %s != expected %dx%d" % (name, m["content"], w, h))
        check(abs(m["aspect"] - aspect) < 0.005,
              "%s: aspect %.3f != expected %.2f" % (name, m["aspect"], aspect))
        check(abs(m["solidity"] - solidity) < 0.005,
              "%s: solidity %.3f != expected %.2f" % (name, m["solidity"], solidity))
        check(abs(m["asymmetry"] - asym) < 0.005,
              "%s: asymmetry %.3f != expected %.2f" % (name, m["asymmetry"], asym))
        check(m["holes"] == holes,
              "%s: holes %d != expected %d" % (name, m["holes"], holes))

    asp = [m["aspect"] for _, m in [(n, sr.measure(p)) for n, p in rows.items()]]
    spread = max(asp) / min(asp)
    check(1.4 < spread < 1.6, "aspect spread %.2fx drifted off the pinned 1.5x" % spread)

    # The real regression this guards against: a judge that scores aspect spread
    # alone would reject this family outright (task-081's whole point 1). It must
    # come back fully kept.
    _, verdicts, _ = vp.judge_family("archer-scifi")
    check(len(verdicts) == 6, "expected 6 judged variants, got %d" % len(verdicts))
    rejected = [n for n, v in verdicts.items() if v["verdict"] == "reject"]
    check(not rejected, "archer-scifi must not auto-reject anything, got: %s" % rejected)

    # `plan` emits kwargs an agent pastes straight into the MCP tool, so a wrong key
    # is silent until a real run fails on it -- which is exactly what happened on
    # task-082 (it emitted `state_description` and a `size` the tool has no parameter
    # for, and all six calls had to be hand-translated). Pin the real signature.
    STATE_SIG = {"character_id", "edit_description", "seed",
                 "use_color_palette_from_reference"}
    emitted = vp.plan_kwargs({"character_id": "abc", "canvas_size": 88},
                             {"slug": "v", "edit_description": "arms tucked"})
    check(set(emitted) <= STATE_SIG,
          "plan emits kwargs create_character_state rejects: %s"
          % sorted(set(emitted) - STATE_SIG))
    check("edit_description" in emitted, "plan must send edit_description")

    print("OK -- 6/6 archer-scifi variants match task-081 Findings 1, "
          "aspect spread %.2fx (1.5x), 0 rejected; plan kwargs match "
          "create_character_state's signature" % spread)
    return 0


if __name__ == "__main__":
    sys.exit(main())
