#!/usr/bin/env python3
"""
Build the reviewer-facing OUTPUT report for the encounter crew.

Runs the crew for real (floor 3 first, to capture a genuine multi-round
negotiation; then floor 1, which restores the committed artifacts), embeds every
emitted file verbatim, and prints the whole thing to PDF through headless Chrome.

    py crew/build_output_pdf.py

Writes:
    crew/out/a03-output.html
    docs/Kindled-EncounterCrew-Output.pdf
"""

from __future__ import annotations

import html
import os
import re
import subprocess
import sys
from datetime import date

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
OUT = os.path.join(HERE, "out")
CREW = os.path.join(HERE, "kindled_crew.py")
PDF = os.path.join(REPO, "docs", "Kindled-EncounterCrew-Output.pdf")

CHROME_CANDIDATES = [
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
]


def run_crew(*args: str) -> str:
    """Run the crew and return its console output."""
    proc = subprocess.run([sys.executable, CREW, *args], cwd=REPO,
                          capture_output=True, text=True, encoding="utf-8",
                          errors="replace")
    return (proc.stdout or "") + (proc.stderr or "")


def read(path: str) -> str:
    with open(path, encoding="utf-8") as fh:
        return fh.read()


def write(path: str, text: str) -> None:
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)


def pre(text: str, cls: str = "") -> str:
    return f'<pre class="{cls}">{html.escape(text.strip())}</pre>'


def _inline(text: str) -> str:
    text = html.escape(text)
    text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
    return re.sub(r"`(.+?)`", r"<code>\1</code>", text)


def md(text: str) -> str:
    """Render the crew's own report markdown.

    Deliberately covers only what `_write_report` emits — headings, bold, inline
    code, bullets, pipe tables and fenced blocks. A pasted-as-monospace report is
    a wall of pipe characters; the whole point of this PDF is that a reviewer can
    read the output without running anything.

    ponytail: subset renderer, swap for a real markdown lib if the report grows
    nested lists, links or images.
    """
    out, rows, fence = [], [], None

    def flush_table() -> None:
        if not rows:
            return
        head, *body = rows
        out.append("<table><thead><tr>"
                   + "".join(f"<th>{_inline(c)}</th>" for c in head)
                   + "</tr></thead><tbody>"
                   + "".join("<tr>" + "".join(f"<td>{_inline(c)}</td>" for c in r)
                             + "</tr>" for r in body)
                   + "</tbody></table>")
        rows.clear()

    for line in text.splitlines():
        if line.startswith("```"):
            if fence is None:
                fence = []
            else:
                out.append(pre("\n".join(fence)))
                fence = None
            continue
        if fence is not None:
            fence.append(line)
            continue

        if line.startswith("|"):
            cells = [c.strip() for c in line.strip("|").split("|")]
            if not all(set(c) <= set("-: ") for c in cells):   # skip the rule row
                rows.append(cells)
            continue
        flush_table()

        if not line.strip():
            continue
        if line.startswith("## "):
            out.append(f"<h4>{_inline(line[3:])}</h4>")
        elif line.startswith("# "):
            out.append(f"<h3>{_inline(line[2:])}</h3>")
        elif line.startswith("- "):
            item = f"<li>{_inline(line[2:])}</li>"
            if out and out[-1].startswith("<ul>"):
                out[-1] = out[-1][:-len("</ul>")] + item + "</ul>"
            else:
                out.append(f"<ul>{item}</ul>")
        else:
            out.append(f"<p>{_inline(line)}</p>")
    flush_table()
    return "\n".join(out)


def style() -> str:
    """Reuse the report stylesheet so both PDFs look like one document."""
    src = read(os.path.join(OUT, "a03.html"))
    return re.search(r"<style>.*?</style>", src, re.S).group(0)


def main() -> int:
    # Floor 3 first: it is the one that forces the auditor to reject and the
    # architect to redesign. Floor 1 runs second so the committed artifacts in
    # crew/out/ are left as the floor-1 pass, exactly as the repo tracks them.
    f3_console = run_crew("--floor", "3")
    f3_report = read(os.path.join(OUT, "run-report.md"))
    f3_json = read(os.path.join(OUT, "encounters.json"))
    # Floor 3 is the run worth showing — the one where the auditor rejects — so
    # keep it under its own name instead of letting the floor-1 restore erase it.
    write(os.path.join(OUT, "run-report.floor3.md"), f3_report)
    write(os.path.join(OUT, "encounters.floor3.json"), f3_json)

    run_crew("--floor", "1")
    f1_report = read(os.path.join(OUT, "run-report.md"))
    f1_json = read(os.path.join(OUT, "encounters.json"))
    f1_schema = read(os.path.join(OUT, "encounters.schema.md"))

    drop_console = run_crew("--drop", "roster-architect")

    doc = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Kindled — Encounter Crew Output</title>
{style()}
<style>
  /* an embedded file, rendered rather than pasted as a wall of pipe characters */
  .file{{border:.8pt solid var(--rule);border-left:2.5pt solid var(--ember);
        padding:9pt 12pt 10pt;margin:7pt 0 11pt;background:#fdfdfd}}
  .file h3{{font-size:10pt;margin:0 0 5pt;padding-bottom:3pt;
           border-bottom:.8pt solid var(--rulelt)}}
  .file h4{{font-size:8.6pt;font-weight:750;letter-spacing:.03em;color:var(--soft);
           margin:10pt 0 3pt}}
  .file table{{margin:4pt 0 7pt}}
  .file pre{{margin:4pt 0 7pt}}
</style>
</head>
<body>

<div class="cover">
  <p class="eyebrow">Multi-Agent AI for Game Development &nbsp;·&nbsp; Assignment #03 &nbsp;·&nbsp; Crew Output</p>
  <h1>Encounter Crew — Output</h1>
  <p class="sub">Every file the crew emits, verbatim, plus the negotiation that produced it</p>
  <p class="meta">
    <strong>Aaron Low</strong> &nbsp;·&nbsp; {date.today().strftime('%-d %B %Y') if os.name != 'nt' else date.today().strftime('%#d %B %Y')} &nbsp;·&nbsp; Python 3.8+, standard library only<br>
    <strong>Code:</strong> <code>crew/kindled_crew.py</code> &nbsp;·&nbsp;
    <strong>ReadMe:</strong> <code>crew/README.md</code> &nbsp;·&nbsp;
    <strong>Output:</strong> <code>crew/out/</code><br>
    <strong>Reproduce this document:</strong> <code>py crew/build_output_pdf.py</code>
  </p>
</div>

<div class="callout">
  <span class="lbl">What this document is</span>
  <p>The companion report described the crew's design. <strong>This one is the output
  itself.</strong> Nothing below is written by hand — every block is a file the crew wrote
  or a console session it printed, pasted verbatim from a live run made while building this
  PDF. The commands that produced each block are named above it, so any of it can be
  reproduced in one line.</p>
</div>

<h2><span class="n">1</span>The three artifacts</h2>

<table>
  <thead><tr><th style="width:31%">File</th><th style="width:11%">Size</th><th>What it is</th></tr></thead>
  <tbody>
    <tr><td><code>crew/out/encounters.json</code></td><td>{len(f1_json)} B</td><td>Flat, typed rows that import directly as an Unreal <strong>DataTable</strong> — the game data the encounter ships as</td></tr>
    <tr><td><code>crew/out/encounters.schema.md</code></td><td>{len(f1_schema)} B</td><td>The column contract for that JSON, plus both audit verdicts at generation time</td></tr>
    <tr><td><code>crew/out/run-report.md</code></td><td>{len(f1_report)} B</td><td>Every agent's declared role, the encounter table, the budget verdict, and the full negotiation</td></tr>
  </tbody>
</table>

<p>Sections 2 and 3 are <code>run-report.md</code> from two different runs — floor 1, which passes
first try, and floor 3, where the budget auditor rejects the plan twice before the crew converges.
Sections 4 and 5 are the emitted data and its schema. Section 6 proves no agent is decorative.</p>

<h2 class="pb"><span class="n">2</span>run-report.md — floor 1</h2>

<p><code>py crew/kindled_crew.py</code> &nbsp;→&nbsp; <code>crew/out/run-report.md</code>. The shipped
Gate-1 wave structure fits the frame budget on the first pass, so the negotiation closes in one round.</p>

<div class="file">{md(f1_report)}</div>

<h2 class="pb"><span class="n">3</span>run-report.md — floor 3, the real negotiation</h2>

<p><code>py crew/kindled_crew.py --floor 3</code>. This is the run worth reading. At the escalated
density the floor peaks <strong>2.3× over the frame budget</strong>; the budget auditor rejects it,
names how many bodies are actually affordable, and the architect redesigns. Two rejections, three
rounds, converging on a floor with zero headroom.</p>

<div class="file">{md(f3_report)}</div>

<div class="callout">
  <span class="lbl">The design finding</span>
  <p>That convergence is a real result for Kindled, not a demonstration: <strong>the
  escalating-density promise in the vertical slice cannot be paid for by the current
  renderer.</strong> The crew reached it by arithmetic over the project's own measured cost curve —
  from the opposite direction to the project's independent renderer conclusion, and agreeing with it.</p>
</div>

<h3>The same run as the console saw it</h3>

{pre(f3_console)}

<h2 class="pb"><span class="n">4</span>encounters.json — the emitted game data</h2>

<p>Floor 1, verbatim. Unique <code>Name</code> row key, scalar columns only, no nesting — exactly what
Unreal's DataTable importer requires. <code>_meta</code> carries provenance, so any row traces back to
the measurement that justified it.</p>

{pre(f1_json)}

<h2 class="pb"><span class="n">5</span>encounters.schema.md — the column contract</h2>

<div class="file">{md(f1_schema)}</div>

<h3>Floor 3's rows, for comparison</h3>

<p>Same schema, same generator, different verdict block — the numbers moved because the negotiation
moved them, not because anything was re-authored by hand.</p>

{pre(f3_json)}

<h2 class="pb"><span class="n">6</span>Proof that no agent is removable</h2>

<p><code>py crew/kindled_crew.py --drop roster-architect</code>. Every agent declares the blackboard
keys it may read and write, and the blackboard <em>enforces</em> those declarations. Drop a producer
and the run stops at the first consumer that needs it — a different point and a different missing
artifact for each agent.</p>

{pre(drop_console)}

<footer class="doc">
  Kindled — Encounter Design Crew · output report · generated {date.today().isoformat()} by
  <code>crew/build_output_pdf.py</code> from a live run. Companion design report:
  <code>docs/Kindled-AgentCrew-Report.pdf</code>.
</footer>

</body>
</html>
"""

    html_path = os.path.join(OUT, "a03-output.html")
    with open(html_path, "w", encoding="utf-8") as fh:
        fh.write(doc)
    print(f"wrote {html_path}")

    chrome = next((c for c in CHROME_CANDIDATES if os.path.exists(c)), None)
    if not chrome:
        print("!! no Chrome/Edge found — open the HTML and print to PDF manually")
        return 1

    subprocess.run([chrome, "--headless=new", "--disable-gpu",
                    "--no-pdf-header-footer", f"--print-to-pdf={PDF}",
                    f"file:///{html_path.replace(os.sep, '/')}"],
                   check=True, capture_output=True)
    print(f"wrote {PDF}  ({os.path.getsize(PDF)} B)")
    return 0


def _selfcheck() -> None:
    """py crew/build_output_pdf.py --check — the renderer is the only real logic here."""
    got = md("# T\n\n**b** and `c`\n\n| A | B |\n|---|---|\n| 1 | 2 |\n\n- x\n- y\n\n```\nraw <tag>\n```\n")
    assert "<h3>T</h3>" in got, got
    assert "<strong>b</strong>" in got and "<code>c</code>" in got, got
    assert "<th>A</th><th>B</th>" in got and "<td>1</td><td>2</td>" in got, got
    assert got.count("<ul>") == 1 and got.count("<li>") == 2, got   # bullets merge
    assert "raw &lt;tag&gt;" in got, got                            # fences escape
    assert "---" not in got, got                                    # rule row dropped
    print("selfcheck OK")


if __name__ == "__main__":
    if "--check" in sys.argv:
        _selfcheck()
    else:
        sys.exit(main())
