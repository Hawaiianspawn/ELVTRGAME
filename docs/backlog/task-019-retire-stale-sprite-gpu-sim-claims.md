---
id: 019
title: Retire the stale "emitter draws zero particles" claims across the perf and camera docs
status: proposed
agent: performance-director
owns: ["docs/perf/niagara-sprite-refactor.md", "docs/design/CAMERA-SCALE.md"]
resources: []
depends-on: []
evidence: A grep for the zero-draw / unfixed-emitter claim across docs returning nothing outside a clearly-marked historical record section.
score: {feel: 1, risk: 1, cost: 1}
source: docs/perf/niagara-sprite-refactor.md:94
decided: ""
---

## Why now
The root cause is known and fixed — `NS_Swarm` drew nothing because the emitter was
`GPUComputeSim`; switching to `CPUSim` fixed it, and the emitter graph was never at
fault. The docs have not caught up:

- `docs/perf/niagara-sprite-refactor.md:94` still says *"has actually opened the graph and
  confirmed this is the specific broken node"* is an unmet gap, and builds a cost estimate
  on that inference. Line 275 budgets **up to a day** for a human to diagnose the module
  graph — work that no longer exists.
- `docs/design/CAMERA-SCALE-HANDOFF.md:143` already flags that `CAMERA-SCALE.md` §5's
  "emitter draws zero particles" is superseded, and that the refactor doc's §2 and §8.1
  are stale despite corrections at the top.

Someone is going to read a day of phantom work into a plan. The handoff doc has already
done the diagnosis of *which* passages are wrong — this is applying it.

## Done when
- `niagara-sprite-refactor.md` §2 and §8.1 corrected in the body, not just at the top.
  A correction only readers of the header see is not a correction.
- The "up to a day for a human to diagnose the graph" budget at line ~275 removed or
  restated against the real fix.
- `CAMERA-SCALE.md` §5 corrected per the handoff doc's finding.
- Superseded passages kept as clearly-marked historical record where they explain how the
  wrong conclusion was reached — the repo's convention is to keep the record, not delete it.
- Nothing else in either doc re-litigated; the measurements stay as measured.

## Spawn prompt
```
You are the performance-director for Emberkeep (C:\Projects\ELVTRGAME).

Retire a stale root-cause claim that survives in two documents.

Established fact: NS_Swarm drew nothing because the emitter was GPUComputeSim. Switching
it to CPUSim fixed it. The emitter module graph was NEVER at fault, and no human needs to
diagnose it.

Stale passages to fix:
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 — corrected at the top of the file but
  still wrong in the body. Line ~94 treats "nobody has opened the graph" as an open gap
  and builds a cost estimate on it. Line ~275 budgets "up to a day" for a human to
  diagnose the module graph. That work does not exist.
- docs/design/CAMERA-SCALE.md §5 — "emitter draws zero particles".
  docs/design/CAMERA-SCALE-HANDOFF.md:143 already identifies this as superseded; read that
  handoff first, it has done the diagnosis for you.

Fix the BODIES, not just the headers. Keep superseded passages as clearly-marked
historical record where they explain how the wrong conclusion was reached — this repo
keeps the record rather than deleting it (see docs/GDD-TODO.md:104 for the convention).

Write ONLY docs/perf/niagara-sprite-refactor.md and docs/design/CAMERA-SCALE.md. Do not
edit CAMERA-SCALE-HANDOFF.md, source, or content. Do not re-litigate the measurements —
they stay as measured. Do not launch the editor; this is a documentation correction.
```
