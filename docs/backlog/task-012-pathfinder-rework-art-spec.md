---
id: 012
title: Write the Pathfinder rework art spec (brief-005)
status: proposed
agent: pixel-art-director
owns: ["docs/art/merle.md", "docs/briefs/brief-005-pathfinder-rework.md"]
resources: []
depends-on: []
evidence: "docs/art/merle.md rewritten against the new fiction, and brief-005 flipped to status done with its spec field pointing at it."
score: {feel: 1, risk: 1, cost: 2}
source: docs/briefs/brief-005-pathfinder-rework.md
decided: ""
---

## Why now
`brief-005` has sat `status: pending` while the spec it supersedes — `docs/art/merle.md`,
delivered under brief-004 and marked `done` — is stale on **every point of physical
description**. The Pathfinder went from a 22-year-old Gatecamp trapper with a claw scar
to a teenager, orphan of the Fall, with a healed burn from scalp to jaw. Anyone reading
`merle.md` today is reading a description of a character that no longer exists.

The scar is not decoration: it is the deliberate mechanism for the character's gender
being unreadable at a glance. That has to survive into the spec as a *specific described
wound*, not softened into "androgynous features", or the design intent is lost.

Filename note: `merle.md` is a leftover from the reversed named-hero decision (classes
are now role-only). Renaming is the owner's call — do not rename it as part of this task.

## Done when
- `docs/art/merle.md` rewritten against brief-005's fiction: age uncertainty, feral
  upbringing, the scalp-to-jaw scar with subtly wrong bone beneath, no proper name.
- Silhouette and value assignments that hold at gameplay zoom, per the locked
  4-value Demichrome register.
- The gender-unreadability mechanism preserved as the described wound.
- `brief-005` frontmatter: `status: done`, `spec: ../art/merle.md`.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Process docs/briefs/brief-005-pathfinder-rework.md, which is status: pending.

Read it in full, plus the stale spec it supersedes (docs/art/merle.md, written against
the OLD fiction under brief-004), docs/art/aesthetic-direction.md (Direction A LOCKED:
strict 4-value Demichrome), docs/art/portrait-register.md, docs/data/art/palette.json,
and docs/narrative/FLAME-FOUNDATION.md for current canon.
Do NOT read WORLD.md — it is superseded.

Rewrite docs/art/merle.md against the new fiction. The critical design intent: the
scalp-to-jaw burn scar is the deliberate mechanism by which the character's gender is
unreadable at a glance. Spec it as that specific wound obscuring the facial landmarks a
glance sorts by — do not soften it into "androgynous features".

Then set brief-005's frontmatter to status: done with spec: ../art/merle.md.

Write ONLY docs/art/merle.md and docs/briefs/brief-005-pathfinder-rework.md. You produce
written specs only — never image files, and do not call any mcp__pixellab__* tool. Do not
rename merle.md; the filename is a leftover from a reversed naming decision and renaming
is the owner's call.

If the brief implies a canon change, end with `## Canon proposals`.
```
