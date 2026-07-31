---
id: 092
title: Purge Emberkeep from docs, agent definitions and skills
status: done
agent: claude
model: sonnet
owns: ["docs/art/**", "docs/assets/**", "docs/data/art/**", "docs/design/**", "docs/narrative/**", "docs/perf/**", "docs/ui/**", "docs/SPRITE-SHEET-HANDOFF.md", ".claude/agents/**", ".claude/skills/**", "Scripts/art/silhouette_report.py"]
resources: []
depends-on: []
epic: emberkeep-purge
evidence: A grep for emberkeep returns zero hits across docs/ (excluding docs/backlog), .claude/agents/ and .claude/skills/, and the regenerated family report.html files carry the new name
score: {feel: 1, risk: 1, cost: 1}
source: user
teammate: emberkeep-docs
decided: "2026-07-31 done"
---

## Why now
The game is named **Kindled** (`GDD.md:481`, decided 2026-07-27). Emberkeep is the
discarded working title and it still appears across ~30 prose and tooling files. The
class that actually causes harm is `.claude/agents/*.md` and `.claude/skills/*.md`:
five director definitions and three skills carry the dead name, so a teammate that
loads one of those and follows it will reintroduce "Emberkeep" into fresh work. That
has already happened often enough to be a recorded habit note.

This is the safe third of the purge — pure prose, no reflected types, no CVars, no
editor. It can land without touching anything the engine reads.

## Done when
- `grep -ril emberkeep` returns **zero** hits under `docs/` (excluding `docs/backlog/`,
  see below), `.claude/agents/`, and `.claude/skills/`.
- The five director definitions (`gameplay-director`, `host`, `narrative-director`,
  `pixel-art-director`, `ui-director`) and three skills (`art-coverage`, `backlog`,
  `cvars`) name Kindled.
- `Scripts/art/silhouette_report.py` emits the new name, and the seven
  `docs/data/art/families/*/report.html` files are **regenerated** from it rather than
  hand-edited — they are generated artifacts.
- **`docs/backlog/` is explicitly OUT OF SCOPE.** Those 75 task files are the historical
  record of decisions; `TEMPLATE.md` keeps even rejected tasks deliberately. Do not
  rewrite them, and do not touch `Scripts/backlog.py`.
- Where a doc's occurrence is a *historical statement* ("originally named Emberkeep",
  a dated decision record), keep the history and make the tense explicit rather than
  pretending the name never existed. Renaming a decision record's own subject is how
  the reason for a decision gets lost.

## Spawn prompt

```
You are executing task-092.

The game is named KINDLED. "Emberkeep" is the discarded working title, decided
2026-07-27 (GDD.md:481). It still appears in roughly 30 prose and tooling files and
your job is to remove it from the safe ones.

WHY THIS MATTERS MORE THAN A TYPO SWEEP: five director agent definitions
(.claude/agents/gameplay-director.md, host.md, narrative-director.md,
pixel-art-director.md, ui-director.md) and three skills
(.claude/skills/art-coverage/SKILL.md, backlog/SKILL.md, cvars/SKILL.md) carry the dead
name. Any teammate that loads one of those and follows it reintroduces "Emberkeep" into
new work. Fixing those eight files is the highest-value part of this task.

YOU OWN, and may write only these:
  docs/art/**, docs/assets/**, docs/data/art/**, docs/design/**, docs/narrative/**,
  docs/perf/**, docs/ui/**, docs/*.md (top-level only),
  .claude/agents/**, .claude/skills/**, Scripts/art/silhouette_report.py

HARD EXCLUSIONS — do not touch these even though they contain the word:
  - docs/backlog/** — 75 task files. These are the HISTORICAL RECORD of decisions.
    TEMPLATE.md keeps even rejected tasks on purpose. Rewriting them edits history.
  - Scripts/backlog.py — same reason.
  - docs/GDD-TODO.md — task-001 owns it and is still open.
  - docs/RENDERING-LIGHTING.md and docs/UNIT-CAM-HANDOFF.md — task-093 owns these two,
    because their Emberkeep occurrences are CVar NAMES that only become correct once
    the CVars are actually renamed. Leave them.
  - ELVTR/** — anything at all. Source, Config, Content, Intermediate, Saved, Binaries.
    Two sibling tasks (task-093, task-094) own the C++ and CVar halves and they are
    genuinely dangerous; staying out of ELVTR/ is what keeps this task safe.
  - ELVTR/Intermediate/**, ELVTR/Saved/**, ELVTR/Binaries/**, any __pycache__ — these
    are GENERATED build artifacts. They are not stale, they are rebuilt. Editing them
    accomplishes nothing.

TWO THINGS THAT NEED JUDGMENT, not find-replace:

1. GENERATED HTML. The seven docs/data/art/families/*/report.html files are produced by
   Scripts/art/silhouette_report.py. Fix the GENERATOR, then REGENERATE the reports. Do
   not hand-edit the HTML — a hand-edit gets wiped the next time the script runs and
   leaves you with a false green grep.

2. HISTORICAL STATEMENTS. Some occurrences are a doc recording that the game USED to be
   called Emberkeep, or a dated decision record whose subject is the rename itself.
   Do NOT erase those — keep the history and make the tense explicit ("originally named
   Emberkeep", "the working title Emberkeep was dropped 2026-07-27"). Renaming a
   decision record's own subject is how the reason for the decision gets lost. Use your
   judgment per occurrence; if you are unsure, keep the history and flag it in the
   handback.

VERIFY BEFORE HANDING BACK:
    grep -ril emberkeep docs/ .claude/ Scripts/art/ | grep -v docs/backlog

should return nothing except any deliberate historical mentions you kept — list those
explicitly in the handback so they can be checked, with a one-line reason each.

Do NOT rename any C++ symbol, any console variable, or any file under ELVTR/. Do not
run a build or open the editor. This task is prose only.

HAND BACK: the file count changed, the list of deliberate historical mentions you kept
with reasons, confirmation the family report.html files were regenerated rather than
hand-edited, and the final grep output.
```
