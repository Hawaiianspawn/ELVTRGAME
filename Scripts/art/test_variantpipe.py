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

    # refine's clause picker reads judge_family's own reason strings. Both live in
    # variantpipe.py, so a reworded reason silently turns every refine into an
    # escalation -- pin one live reason per promptable failure kind, copied from
    # judge_family, not invented.
    dup = vp.corrective_clause(["identical opaque count (1044 px) to 'state00_base' "
                                "-- same outline, differs on the interior only"])
    check(dup and "OUTLINE" in dup, "duplicate-outline reason must produce an outline directive")

    sub = vp.corrective_clause(["band sits fully inside 'state05_ridge' on every axis "
                                "(aspect/solidity/asymmetry/holes) across all 8 rotations"])
    check(sub and "8 rotations" in sub, "band-containment reason must produce a push directive")

    band = {"aspect": [1.20, 1.60], "solidity": [0.5, 0.6], "asymmetry": [0.3, 0.4], "holes": []}
    narrow = vp.corrective_clause(["aspect target 0.60 falls outside the measured "
                                   "1.20-1.60 band across all 8 rotations"], band, 0.60)
    check(narrow and "NARROWER" in narrow,
          "a target below the measured band must ask for a narrower form, got: %s" % narrow)
    wide = vp.corrective_clause(["aspect target 2.40 falls outside the measured "
                                 "1.20-1.60 band across all 8 rotations"], band, 2.40)
    check(wide and "WIDER" in wide,
          "a target above the measured band must ask for a wider form, got: %s" % wide)

    clean = vp.corrective_clause(["871 distinct colours, over the 256 ceiling -- reads as "
                                  "a smooth render rather than pixel art, which is the "
                                  "'clean' failure the chibi gameplay register cannot "
                                  "carry at 48px"])
    check(clean and "flat pixel art" in clean,
          "the colour-count reason must ask for flat pixel art, got: %s" % clean)

    # MAX_DISTINCT_COLOURS sits in a gap measured across all 123 variants on disk
    # (derived states 10-129, raw base characters 871-881). If a future sweep puts
    # anything between those, the threshold stopped being measured and became a
    # guess -- re-measure before moving it.
    check(vp.MAX_DISTINCT_COLOURS == 256, "MAX_DISTINCT_COLOURS drifted off the measured gap")
    base = sr.measure(ROOT.parent.parent / "knight-primitive" / "raw" / "p0_base" / "rotations" / "south.png")
    check(base["distinct"] > vp.MAX_DISTINCT_COLOURS,
          "p0_base measured %d distinct -- the raw-base end of the gap moved"
          % base["distinct"])
    for _, m in [(n, sr.measure(p)) for n, p in rows.items()]:
        check(m["distinct"] < vp.MAX_DISTINCT_COLOURS,
              "an archer-scifi state measured %d distinct, above the ceiling -- a "
              "treated variant must sit well under it" % m["distinct"])

    # Every directive that asks for a SHAPE change must also lock the kit. Three
    # real attempts at v5_widecross proved why: told to "extend the carried gear
    # further from the body", the generator grew a second sword, dropped the
    # shield, then added a horn. Nothing here can measure that -- a swapped sword
    # for shield moved opaque px by +25 -- so the prompt is the only guard.
    for c in (dup, sub, narrow, wide):
        check("Do not add, remove or duplicate any equipment" in c,
              "a shape directive shipped without the kit lock: %s" % c)
    # The colour directive is about rendering, not pose, and must NOT carry it.
    check("duplicate any equipment" not in clean,
          "the colour directive should not be talking about equipment: %s" % clean)

    # The luma note is informational (judge never rejects on it) and nothing else
    # in the file promises a directive for it -- it must NOT consume an attempt.
    check(vp.corrective_clause(["luma 0.180 -- not automatically wrong (SKILL.md), but "
                                "the outline is doing all the work"]) is None,
          "an informational reason must not produce a corrective directive")

    # The circuit breaker's two triggers, as constants rather than behaviour: a
    # real exercise would have to spend PixelLab credits.
    check(vp.MAX_ATTEMPTS == 3, "MAX_ATTEMPTS drifted off 3")
    for reason in ("canvas 64x64 does not match family canvas 88px",
                   "missing 3 of 8 rotations"):
        check(reason.startswith(vp.UNPROMPTABLE),
              "'%s' must be caught as unpromptable and escalate on sight" % reason)

    print("OK -- 6/6 archer-scifi variants match task-081 Findings 1, "
          "aspect spread %.2fx (1.5x), 0 rejected; plan kwargs match "
          "create_character_state's signature; refine directs 5 failure kinds, "
          "ignores 1, and escalates 2 unpromptable ones; the %d-colour ceiling "
          "still sits in the measured gap" % (spread, vp.MAX_DISTINCT_COLOURS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
