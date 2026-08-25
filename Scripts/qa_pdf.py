"""Assemble docs/qa/README.md + the run reports + screenshots into one print-first HTML and
print it to PDF with headless Chrome (same recipe as the Assignment #3 report).

    py Scripts/qa_pdf.py            -> docs/Kindled-AdversarialQA-Assignment-09.{html,pdf}
"""
import base64, json, re, subprocess, sys
from pathlib import Path

import markdown

ROOT = Path(__file__).resolve().parents[1]
QA = ROOT / "docs" / "qa"
OUT = ROOT / "docs" / "Kindled-AdversarialQA-Assignment-09"
CHROME = Path(r"C:\Program Files\Google\Chrome\Application\chrome.exe")

SHOTS = [  # (file, caption)
    ("adversary_7_f1.png", "Bug 1 — outer file of the rank block drawn over the wall bricks, both sides (seed 7, t=1.5 s)."),
    ("adversary_23_f4.png", "Bug 1 after a swap — halberdiers at |wx| 222 vs HALL_HALF 200 (seed 23)."),
    ("adversary_11_f5.png", "Bug 2 — the frame Engine.time_scale had been pinned at 0.05 for a full second (seed 11)."),
    ("juggle_seed7_earlier_run.png", "Bug 5 — both teams tumbling above the ceiling line 15 s after launch, hero untouched (seed 7, earlier run)."),
    ("adversary_7.png", "Final frame of seed 7: 4 hordes deep, wave 1 still ticking."),
    ("after/adversary_23.png", "After the fixes — seed 23 final frame: the outer file now sits inside the bricks."),
    ("after/adversary_7.png", "After the fixes — seed 7 final frame: a juggled crowd held inside the frame by Unit._air_cap()."),
]

RUBRIC = [
    ("Findings", "4.0", "Six real bugs, each named to a file:line and mechanic (Army.slots stagger, Battle._hitstop re-arm, slash tween / hit() await on freed units, juggle re-pop, enemy past the lens). Section “What it found”."),
    ("Agent Logic", "3.0", "15 hostile behaviors cycled on a seeded RNG through the real input map plus direct calls; “broken” = 14 explicit invariants the game's own code claims. Section “How the agent tries to break things”."),
    ("Structured Report", "2.0", "adversary_&lt;seed&gt;.json / .csv per run with location {scene, wave, system, unit_type, wx, wd}, error_type, game_context {behavior, magic, army, fps…}, count, screenshot; engine stderr folded in. Excerpt in the appendix."),
    ("ReadMe", "1.0", "docs/qa/README.md — what it found, and “Was I surprised?”. Reproduced in full below."),
]


def b64(p: Path) -> str:
    return "data:image/png;base64," + base64.b64encode(p.read_bytes()).decode()


def main() -> None:
    body = markdown.markdown((QA / "README.md").read_text(encoding="utf-8"), extensions=["tables", "fenced_code"])
    body = body.replace("<h1>", "<h1 class='doc'>", 1)
    rubric = "".join(f"<tr><td>{c}</td><td class='pts'>/ {p}</td><td>{e}</td></tr>" for c, p, e in RUBRIC)
    shots = "".join(f"<figure><img src='{b64(QA / f)}'><figcaption>{cap}</figcaption></figure>" for f, cap in SHOTS)
    rep = json.loads((QA / "adversary_7.json").read_text(encoding="utf-8-sig"))
    excerpt = {"run": rep["run"], "findings": rep["findings"][:2] + ["… %d more" % max(0, len(rep["findings"]) - 2)],
               "engine_errors": rep["engine_errors"]}
    csv_head = "\n".join((QA / "adversary_7.csv").read_text(encoding="utf-8").splitlines()[:4])
    def run_rows(folder: Path, tag: str) -> str:
        return "".join(
            f"<tr><td>{tag}</td><td>{p.stem}</td><td>{len(d['findings'])}</td><td>{sum(e['count'] for e in d.get('engine_errors', []))}</td>"
            f"<td>{', '.join(sorted({f['error_type'] for f in d['findings']})) or '—'}</td></tr>"
            for p in sorted(folder.glob("adversary_*.json")) for d in [json.loads(p.read_text(encoding="utf-8-sig"))])
    runs = run_rows(QA, "before fixes") + run_rows(QA / "after", "after fixes")
    html = f"""<!doctype html><html><head><meta charset="utf-8">
<title>Kindled — Adversarial QA Agent (Assignment #09)</title>
<style>
@page {{ size: A4; margin: 16mm 14mm; }}
body {{ font: 10.5pt/1.45 Segoe UI, Helvetica, Arial, sans-serif; color: #1b1b1b; max-width: 180mm; margin: 0 auto; }}
h1.doc {{ font-size: 20pt; margin: 0 0 2mm; }} h2 {{ font-size: 13pt; margin: 8mm 0 2mm; border-bottom: 1px solid #999; }}
.sub {{ color: #555; margin-bottom: 6mm; }}
table {{ border-collapse: collapse; width: 100%; margin: 3mm 0; font-size: 9.5pt; }}
th, td {{ border: 1px solid #bbb; padding: 1.2mm 2mm; vertical-align: top; text-align: left; }} th {{ background: #eee; }}
td.pts {{ white-space: nowrap; font-weight: 600; }}
code, pre {{ font: 8.5pt/1.35 Consolas, monospace; }} pre {{ background: #f4f4f4; padding: 2mm; white-space: pre-wrap; word-break: break-all; }}
figure {{ margin: 4mm 0; page-break-inside: avoid; }} figure img {{ width: 100%; image-rendering: pixelated; border: 1px solid #999; }}
figcaption {{ font-size: 9pt; color: #444; margin-top: 1mm; }}
ol li, ul li {{ margin-bottom: 1.5mm; }} .break {{ page-break-before: always; }}
</style></head><body>
<h1 class="doc">Kindled — Adversarial QA Agent</h1>
<div class="sub">Multi-Agent AI for Game Development · Assignment #09 · Aaron Low · 24 Aug 2026<br>
Game: Kindled (Godot 4.7, <code>godot/</code>) · Agent: <code>godot/scripts/Adversary.gd</code> · Runner: <code>Scripts/godot-run.ps1 -Adversary</code> · Reports: <code>docs/qa/</code></div>
<h2>Rubric map</h2>
<table><tr><th>Criterion</th><th>Pts</th><th>Where the evidence is</th></tr>{rubric}</table>
<h2>Committed runs</h2>
<table><tr><th></th><th>Run</th><th>Findings</th><th>Engine errors</th><th>Error types</th></tr>{runs}</table>
<div class="break"></div>
{body}
<div class="break"></div>
<h2>Screenshots</h2>
<p>Each finding stores the frame it first broke on (<code>screenshot</code> field). Pixel art shown at native scale.</p>
{shots}
<div class="break"></div>
<h2>Appendix — report excerpt (<code>docs/qa/adversary_7.json</code>)</h2>
<pre>{json.dumps(excerpt, indent=1)}</pre>
<h2>Appendix — CSV head (<code>docs/qa/adversary_7.csv</code>)</h2>
<pre>{csv_head}</pre>
</body></html>"""
    OUT.with_suffix(".html").write_text(html, encoding="utf-8")
    cmd = [str(CHROME), "--headless=new", "--disable-gpu", "--no-pdf-header-footer",
           f"--print-to-pdf={OUT.with_suffix('.pdf')}", OUT.with_suffix(".html").as_uri()]
    subprocess.run(cmd, check=True, capture_output=True)
    print(OUT.with_suffix(".pdf"), OUT.with_suffix(".pdf").stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main()
