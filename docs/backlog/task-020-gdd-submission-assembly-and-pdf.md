---
id: 020
title: Assemble the four-section GDD submission document and export to PDF
status: proposed
agent: claude
owns: ["docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md"]
resources: []
depends-on: [1]
evidence: A single PDF containing all four required sections, or a recorded finding that the assignment was already submitted and this task is obsolete.
score: {feel: 1, risk: 1, cost: 2}
source: docs/GDD-TODO.md:77
decided: ""
---

## Why now
Honestly: possibly not now at all, and that is the point of filing it rather than
assuming. `GDD-TODO.md` Part A was scoped to Assignment #1, due **21 July 2026, 11:59 PM
ET**. That deadline passed five days ago. `docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md` exists
and `docs/Emberkeep-GDD-v0.5.pdf` exists, which suggests the work was done — but the
tracker's assembly, PDF-export, and proofread boxes at lines 77–79 are still unticked, so
the record does not say either way.

Depends on task-001 because that task establishes what in Part A is actually still open.
If task-001 concludes the assignment shipped, the right verdict here is **reject as
obsolete**, and that is a fine outcome — it costs one command and removes a phantom
deliverable from the board.

## Done when
Either:
- **Obsolete:** evidence the assignment was submitted (the existing PDF matches the
  requirements, or the owner confirms). Recorded, and this task is rejected with that
  reason.
- **Real:** the four required sections — Executive Summary, Game Mechanics, AI
  Architecture, Technical Strategy — assembled into one document and exported to PDF, with
  the anti-slop gate respected: every agent role tied to a concrete thing the player sees
  or does, no generic multi-agent framing.

## Spawn prompt
```
You are working on Emberkeep (C:\Projects\ELVTRGAME).

FIRST, determine whether this task is obsolete. docs/GDD-TODO.md Part A was scoped to
Assignment #1, due 21 July 2026 — five days ago. docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md and
docs/Emberkeep-GDD-v0.5.pdf both exist, suggesting it shipped, but the assembly/PDF/proofread
boxes at docs/GDD-TODO.md:77-79 are unticked. Task-001 has re-verified Part A — read its
output first.

If the assignment was submitted: say so, cite the evidence, change nothing, and report
that this task should be rejected as obsolete. That is a good outcome.

If it genuinely was not: assemble the four required sections into
docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md and export a PDF.
  - Executive Summary — from GDD.md §1
  - Game Mechanics — player-facing: what the player SEES and DOES (GDD §3, §4, §6)
  - AI Architecture — each .claude/agents/* dev agent, one plain-English role each, tied
    to a player-facing outcome
  - Technical Strategy — agent-roles table, token budget, API constraints, semester scope

Anti-slop gate from the brief: generic or placeholder framing, or multi-agent concepts not
grounded in specific player-facing mechanics, scores ZERO on two criteria regardless of
theoretical correctness. Every agent role must tie to a concrete thing the player sees.

The game is named Emberkeep (GDD.md:433, decided 2026-07-21). Do NOT read WORLD.md —
superseded by docs/narrative/FLAME-FOUNDATION.md.

Write ONLY docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md and its PDF export. Do not edit GDD.md,
CLASSES.md, SYSTEMS.md, or docs/GDD-TODO.md.
```
