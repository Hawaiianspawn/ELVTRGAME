---
id: 117
title: Repoint the prose that cites console variables and headers the rename deleted
status: proposed
agent: claude
model: ""
owns: ["docs/art/aesthetic-direction.md", "docs/data/squads.json", "docs/data/art/palette.json", "docs/data/art/provenance.json", "docs/data/art/requests/swarm-units.json", "docs/design/**", "docs/perf/**", "docs/SPRITE-SHEET-HANDOFF.md", ".claude/skills/art-coverage/SKILL.md", "Scripts/backlog.py"]
resources: []
depends-on: [93]
epic: emberkeep-purge
evidence: A grep for Emberkeep across the listed paths returns zero hits, and every CVar name and header path that was rewritten is confirmed to exist under its new name in ELVTR/Source or ELVTR/Config
score: {feel: 1, risk: 1, cost: 1}
source: task-092 and task-093 handbacks
---

## Why now
This is the seam between the two halves of the purge, and neither half was wrong.

`task-092` deliberately left every prose mention that named a **real** console variable or
header — `Emberkeep.Quantize`, `Emberkeep.PaletteSteps`, `EmberkeepHud.cpp` — on the
grounds that renaming a doc's reference to a thing that still exists under the old name
would make the doc lie. Correct at the time: `docs/ASSIGNMENT-02-FINAL-GDD.md` says so in
its own text.

Then `task-093` renamed those exact things. The references are now stale in the other
direction: the docs point at CVars and files that no longer exist.

Confirmed stale after both landed: `.claude/skills/art-coverage/SKILL.md:39`,
`docs/art/aesthetic-direction.md:9,18`, `docs/data/squads.json:63,90`, plus the
`docs/design/` and `docs/perf/` files task-092 listed in its handback. Separately,
`Scripts/backlog.py:2`'s docstring calls this "the Emberkeep host" — pure prose that was
excluded only because the file was fenced off to protect the task history.

Blocked on **task-093** closing, because until the rebuild proves the new CVar names
actually bind, rewriting docs to cite them is writing a second unverified claim.

## Done when
- Every `Emberkeep.*` CVar citation in the owned paths names the `Kindled.*` variable that
  now exists, verified against `ELVTR/Config/SwarmExecOnPlay.canonical.txt` rather than
  assumed from the prefix.
- Every `Emberkeep*.h` / `Emberkeep*.cpp` path citation names the renamed file, verified
  to exist on disk.
- `Scripts/backlog.py`'s docstring says Kindled. **Nothing else in that file changes** —
  it was fenced off to protect the task history and that reason still holds.
- `docs/data/squads.json` is valid JSON after the edit, and any consumer script still
  parses it.
- Historical statements stay historical: this task rewrites *citations*, never a decision
  record's subject.

## Spawn prompt

```
You are executing task-117. Small, mechanical, and easy to get subtly wrong.

BACKGROUND: task-092 purged "Emberkeep" from prose but deliberately left every mention
that cited a REAL console variable or header file, because renaming those would have made
the docs cite things that did not exist. task-093 then renamed those CVars and headers.
The citations are now stale the other way. You are repointing them.

DO NOT blanket find-replace. For each occurrence:
  1. Work out what it cites — a CVar, a file path, or prose.
  2. VERIFY the new name exists: CVars against ELVTR/Config/SwarmExecOnPlay.canonical.txt
     and `grep -rn 'TEXT("Kindled\.' ELVTR/Source/`; file paths with an actual ls.
  3. Only then rewrite it.
A citation rewritten to a name that does not exist is worse than the stale one, because
it reads as verified.

KNOWN OCCURRENCES (confirmed after both halves landed — re-grep, do not trust this list
to be complete):
  .claude/skills/art-coverage/SKILL.md:39      Emberkeep.Quantize=0
  docs/art/aesthetic-direction.md:9,18         Emberkeep.Quantize, Emberkeep.PaletteSteps
  docs/data/squads.json:63,90                  Emberkeep.UnitCamProj.FollowSpeed,
                                               CastFocusSpeed, LookLerp
  docs/design/**, docs/perf/**, docs/SPRITE-SHEET-HANDOFF.md — see task-092's handback
  Scripts/backlog.py:2                         docstring: "the Emberkeep host"

SPECIAL CASES:
  - Scripts/backlog.py — change the DOCSTRING ON LINE 2 AND NOTHING ELSE. The file was
    fenced off from the purge to protect the backlog's task history; that reason still
    holds for every other line.
  - docs/data/squads.json — it is JSON. Validate it parses after you edit it.
  - A HISTORICAL STATEMENT is not a citation. If a doc says "originally named Emberkeep"
    or records the rename decision, LEAVE IT. You are fixing references to code, not
    rewriting history.

DO NOT TOUCH: docs/backlog/** (task history), docs/ui/** (three UI tasks own those files
and may be running beside you), GDD.md, docs/ASSIGNMENT-*.md, docs/assets/*.html,
docs/narrative/FLAME-FOUNDATION.md, ELVTR/** (task-093's, already done).

VERIFY: grep for Emberkeep across your owned paths returns zero, and every new name you
wrote was checked against the source or config, not inferred.

Stage only the paths you own — never `git add -A`, the tree is shared. Commit with the
caveman-commit skill per CLAUDE.md and push.

HAND BACK: the per-file list of what you rewrote, which names you verified against which
source, and anything you left because it was history rather than a citation.
```
