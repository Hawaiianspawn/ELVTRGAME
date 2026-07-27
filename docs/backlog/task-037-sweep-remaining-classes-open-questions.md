---
id: 037
title: Sweep the remaining CLASSES C2-C11 open questions against current canon
status: proposed
agent: narrative-director
owns: ["docs/narrative/open-questions-audit.md"]
resources: []
depends-on: [18]
evidence: Each remaining open question marked still-open, answered-by-canon, or dissolved-by-the-reset, with the citation.
score: {gate: 1, risk: 1, cost: 2}
source: docs/GDD-TODO.md:116
decided: ""
---

## Why now
**Recommend parking for now, but not because it lacks value — because its output would be
untrustworthy today.**

`docs/GDD-TODO.md:116` bundles three unrelated question sets: `CLASSES.md` C2–C11,
`WORLD.md` W2–W7, and the GDD Open Questions log. The GDD half is already handled — this
backlog files its five unresolved rows individually (tasks 015, 016, 032, 033, 034). The
WORLD half is questions against a superseded document. What is genuinely left is C2–C11.

The reason to wait: this is an audit, and an audit run before the canon pointers are fixed
audits the wrong canon. Task-018 repoints the director agents off superseded WORLD.md. Run
this after that, and it produces a clean answer; run it now, and a narrative-director spawn
will read WORLD.md as truth while auditing whether WORLD.md is truth.

## Done when
- Each `CLASSES.md` C2–C11 question marked: still open, answered by current canon (with
  the citation), or dissolved by the 2026-07-22 reset.
- W2–W7 assessed as a group — most likely dissolved wholesale, but say so explicitly
  rather than leaving them ambiguous.
- Genuinely-still-open items handed back as candidate backlog tasks, not resolved here.

## Recommended verdict
`py Scripts/backlog.py park 37 -r "Audit of canon that runs before task-018 fixes the canon
pointers would audit the wrong canon. Unpark once 018 lands."`
