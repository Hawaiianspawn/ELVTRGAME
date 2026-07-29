---
id: 013
title: Write the Lampbearer rework art spec (brief-006)
status: proposed
agent: pixel-art-director
owns: ["docs/art/noll.md", "docs/briefs/brief-006-lampbearer-rework.md"]
resources: []
depends-on: []
evidence: "docs/art/noll.md rewritten against the new fiction, and brief-006 flipped to status done with its spec field pointing at it."
score: {feel: 1, risk: 1, cost: 2}
source: docs/briefs/brief-006-lampbearer-rework.md
decided: ""
---

## Why now
Same failure mode as task-012 and the same age: `brief-006` is pending while
`docs/art/noll.md` still specs a 28-year-old male foundling. The current Lampbearer is
a woman in her mid-twenties carrying her dead mentor's lamp, with the mentor's soul-flame
alive and aware inside it.

The lamp's visual grammar carries over unchanged — flame, halo, wisp points, the shared
one-flame rule, never guttered even on hero-down. What changed is *who holds it and why*,
which is exactly the kind of change a stale spec hides: the object still looks right, so
nobody notices the character underneath is wrong.

Same filename caveat as task-012: `noll.md` is a leftover from the reversed named-hero
decision. Do not rename it.

## Done when
- `docs/art/noll.md` rewritten: female, mid-twenties, carrying an inherited lamp rather
  than her own.
- The mentor's flame specified as an *ongoing aware presence* inside the lamp, not a
  silent artifact — with a readability answer for how that reads at gameplay zoom.
- The lamp's existing visual grammar preserved verbatim where the brief says it carries
  over; changes limited to what actually changed.
- `brief-006` frontmatter: `status: done`, `spec: ../art/noll.md`.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

Process docs/briefs/brief-006-lampbearer-rework.md, which is status: pending.

Read it in full, plus the stale spec it supersedes (docs/art/noll.md — written against
the OLD fiction: a 28-year-old male foundling), docs/art/aesthetic-direction.md
(Direction A LOCKED: strict 4-value Demichrome), docs/art/portrait-register.md,
docs/data/art/palette.json, docs/art/flame-bearer-status.md, and
docs/narrative/FLAME-FOUNDATION.md.
Do NOT read WORLD.md — it is superseded.

Rewrite docs/art/noll.md. She is female, mid-twenties, and the lamp is her dead mentor's,
with the mentor's soul-flame captured inside as an ongoing aware presence. The lamp's
visual grammar (flame + halo + wisp points, one-flame rule, never guttered including
hero-down) carries over UNCHANGED — do not redesign it. Change only what the brief says
changed, and give the aware-presence-inside-the-lamp a readability answer at gameplay
zoom.

Then set brief-006's frontmatter to status: done with spec: ../art/noll.md.

Write ONLY docs/art/noll.md and docs/briefs/brief-006-lampbearer-rework.md. Written specs
only — no image files, no mcp__pixellab__* calls. Do not rename noll.md.
```
