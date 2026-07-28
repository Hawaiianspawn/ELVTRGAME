---
id: 051
title: Build the art asset matrix and a coverage audit that finds missing, unwired and off-ramp art
status: done
agent: claude
owns: ["docs/data/art/asset-matrix.json", "docs/data/art/asset-matrix.schema.md", ".claude/skills/art-coverage/**", "Scripts/art/coverage.py"]
resources: []
depends-on: []
evidence: Running the audit prints a report that correctly identifies the known real state of the project's art — including at least one asset that is generated but not drawn, and one that is required but absent — verified line by line against the repo rather than asserted.
score: {gate: 2, risk: 1, cost: 2}
source: user
teammate: art-coverage
decided: "2026-07-27 done"
---

## Why now
Owner, 2026-07-27: *"we should make a workflow or agent to tackle what art assets a
character needs"*. This session is the argument for it. Three art problems were found by
accident, any one of which a coverage check would have surfaced immediately:

- **Six finished, palette-validated soldier variants** (`T_Soldier_01`–`06`) sat in
  `Content/Sprites/` while the Unit Cam drew a single generic row from `T_Swarm_2bit`. Nobody
  knew. It was found by grepping during an unrelated task.
- **Archers have no art at all**, discovered only when the typed-unit design named them.
- **The archer proxy has no animation frames**, discovered while writing the task to import it.

The pipeline below this layer is good and must not be rebuilt: `sprite-request.schema.json`
plus 19 filed requests describe individual sprites, `/sprite` generates and imports them,
`pixel-art-director` writes the specs, `docs/briefs/` holds pending briefs. **What is missing
is the layer above** — nothing answers *"what is the complete asset set this character needs,
and which parts of it actually exist and are being drawn?"*

Owner chose: a data matrix plus a skill that reports gaps, audit-first.

## Done when
- **`docs/data/art/asset-matrix.json`** declares, per character kind (hero / retinue unit /
  brood / portrait / whatever the project actually has), the complete asset set required:
  frames, cell size, sheet layout, palette, and any per-kind extras. **Derived from the real
  format and the existing specs, not invented.**
- **A documented schema** (`asset-matrix.schema.md`) covering every field, its units and its
  valid values, matching the house style of the other `docs/data/` schema files.
- **An audit** — `Scripts/art/coverage.py` plus a `/art-coverage` skill that drives it —
  reporting per character: **missing** (required, absent), **unwired** (exists on disk but
  nothing draws it), **off-ramp** (present but not on the locked 4-value Demichrome ramp),
  **unrecorded** (in `Content/` with no provenance entry), and **incomplete** (present but
  short of the matrix's required frames — the archer-proxy case).
- The audit is **read-only**. It reports; it never generates, imports, edits art, or spends
  PixelLab credits.
- Verified against ground truth: the report's claims are checked against the repo line by
  line, not asserted.

## Spawn prompt
```
You are executing task-051 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight
branch.

GOAL, from the owner: "we should make a workflow or agent to tackle what art assets a
character needs". They chose the shape: a DATA MATRIX plus a SKILL THAT REPORTS GAPS, and
they want the AUDIT half first — finding what is missing, unwired or broken today, ahead of
forward-planning new characters.

WHY THIS EXISTS. Three art problems were found by accident this session, each of which a
coverage check would have caught instantly:
- T_Soldier_01 through T_Soldier_06 — six finished, palette-validated variants — sat in
  ELVTR/Content/Sprites/Units/ while the Unit Cam drew a single generic row out of
  T_Swarm_2bit. Found by grepping during an unrelated task.
- Archers have no art at all. Found only when the typed-unit design named them.
- The archer proxy has no animation frames. Found while writing the task to import it.

DO NOT REBUILD WHAT EXISTS. Read all of this first; you are adding a layer ABOVE it:
- docs/data/art/sprite-request.schema.json and the 19 requests in docs/data/art/requests/ —
  these describe ONE sprite each, in detail. Your matrix describes what a character NEEDS;
  a request describes what was ASKED FOR. They are different layers. Reuse its vocabulary.
- .claude/skills/sprite/SKILL.md — the generation pipeline (request -> generate -> quantize ->
  pack -> import) and Scripts/art/pixelpipe.py. Your audit calls NOTHING in it. Read it to
  learn the real format constraints and the provenance manifest convention.
- docs/data/art/palette.json and docs/art/aesthetic-direction.md — the locked 4-value
  Demichrome ramp.
- docs/art/soldier-roster-v1.md, vanguard.md, retinue-militia.md, flame-bearer.md — existing
  specs. Their acceptance checklists tell you what "correct" means per asset.
- .claude/agents/pixel-art-director.md — it owns SPECS. You are not replacing it.

DERIVE THE MATRIX, DO NOT INVENT IT. The required asset set must come from what the code and
the specs actually demand:
- ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp and the SwarmSheet decode in
  ELVTR/Source/ELVTR/Mass/SwarmFragments.h define the real sheet geometry the game reads.
- Measured facts to check rather than trust: RawArt/Sheets/T_Swarm_2bit.png is 384x192 (8x4
  grid of 48px cells); T_Soldier_01..06 are 192x96; T_Hero_Vanguard is 192x192;
  T_Unit_Retinue is 192x48. docs/SPRITE-SHEET-HANDOFF.md describes an OLDER 4x2 272x136
  layout at 68px cells — that is superseded; confirm against the files, not the doc.
- WHERE A GENUINE DESIGN QUESTION COMES UP — for instance whether a retinue unit requires 8
  directional variants or only action frames, since PixelLab characters are 8-direction while
  the atlas is action frames — DO NOT DECIDE IT. Record it in the matrix as an explicit open
  field with a comment, and list it in your handback for the owner. Inventing a requirement
  would make the audit report fake gaps, which is worse than reporting none.

WHAT THE AUDIT MUST DETECT, per character:
  missing      required by the matrix, absent from disk
  unwired      exists on disk but nothing in code draws it   <- the six-variants failure
  off-ramp     present but not on the locked 4-value ramp
  unrecorded   in ELVTR/Content/ with no docs/data/art/provenance.json entry
  incomplete   present but short of the required frames      <- the archer-proxy failure
"unwired" is the hardest and the most valuable — it is the one that actually bit this
project. A grep for the asset name across ELVTR/Source is a legitimate implementation; say
plainly in the skill doc how it decides, and what would fool it (an asset loaded by a
computed path, for instance).

READ-ONLY, ALWAYS. The audit reports. It must never generate, import, edit art, or call any
mcp__pixellab__* tool. PixelLab generation costs real money and is not this task's business.

EVIDENCE — this is the bar. RUN THE AUDIT and hand back its real output, then VERIFY ITS
CLAIMS LINE BY LINE against the repo. A report that looks plausible but is wrong is worse
than no tool, because the next person will trust it. Specifically confirm it correctly finds
at least one genuinely unwired asset and one genuinely missing one, and say how you checked.
NOTE: task-050 is running concurrently and is wiring T_Soldier_01..06 and T_Hero_Vanguard
into the Unit Cam right now. So the six-variants case may be FIXED by the time you run — do
not assume it still reports as unwired. Verify against the repo as it is when you run, and if
the known-bad cases have been fixed, find real current examples rather than reporting stale
ones. If you cannot find a real instance of a category, say the category is untested rather
than fabricating one.

YOU OWN: docs/data/art/asset-matrix.json, docs/data/art/asset-matrix.schema.md,
.claude/skills/art-coverage/** , Scripts/art/coverage.py.

DO NOT TOUCH: ELVTR/Source/**, ELVTR/Content/**, RawArt/**, docs/data/art/provenance.json,
docs/data/art/requests/**, docs/data/art/sprite-request.schema.json, .claude/skills/sprite/**,
docs/art/**, docs/design/**, or any docs/backlog/ file. task-050 is concurrently writing
ELVTR/Content/Sprites/**, RawArt/** and provenance.json — do not write anything it owns.

HAND BACK: the matrix's structure and what you derived each requirement from, any design
question you deliberately left open rather than inventing, the audit's real output, how you
verified its claims against the repo, and what would fool it.
```
