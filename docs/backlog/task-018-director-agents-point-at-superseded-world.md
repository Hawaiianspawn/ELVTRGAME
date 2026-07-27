---
id: 018
title: Repoint the director agents off superseded WORLD.md onto current narrative canon
status: proposed
agent: claude
owns: [".claude/agents/**"]
resources: []
depends-on: []
evidence: Every .claude/agents/*.md canon list checked against what is actually current, and a grep for WORLD.md across the agent definitions returning only intentional references.
score: {gate: 2, risk: 1, cost: 1}
source: .claude/agents/gameplay-director.md:17
decided: ""
---

## Why now
`.claude/agents/gameplay-director.md:17` lists `WORLD.md` under "Read-only source of
truth (never edit)". `WORLD.md` was **superseded** by the total narrative reset of
2026-07-22; current canon is `docs/narrative/FLAME-FOUNDATION.md`.

This is the worst kind of stale doc, because it is stale *in an agent's system prompt*.
Every gameplay-director spawn loads dead canon and is instructed to treat it as
authoritative. It has been doing so for four days. The other four director definitions
need the same check — the reset landed after they were all written.

Cheap, fast, and it corrupts everything downstream while it stands. Note that every
spawn prompt written into this backlog already carries a "do NOT read WORLD.md" line as
a stopgap; this task removes the need for the stopgap.

## Done when
- All five director definitions checked, not just the gameplay one.
- `WORLD.md` references replaced with `docs/narrative/FLAME-FOUNDATION.md` where the
  intent was "current narrative canon", or removed where the reference is simply dead.
- Every other file each definition claims to read or own verified to still exist and
  still be canon — including that `SYSTEMS.md`, `docs/data/`, and `docs/design/` exist
  (they do now; `GDD-TODO.md:91` claims the latter two do not, which is itself stale).
- A note in each touched definition recording the reset date, so the next reader knows
  when the canon list was last verified.

## Spawn prompt
```
You are correcting stale canon references inside agent definitions in the Emberkeep repo
(C:\Projects\ELVTRGAME).

.claude/agents/gameplay-director.md line 17 lists WORLD.md as read-only source of truth.
WORLD.md was SUPERSEDED by the total narrative reset on 2026-07-22; current narrative
canon is docs/narrative/FLAME-FOUNDATION.md. Any agent spawned from that definition loads
dead canon and is told to trust it.

Check ALL FIVE definitions in .claude/agents/ — gameplay, narrative, performance,
pixel-art, ui — since the reset postdates all of them.

For each: verify every file it claims to read or own still exists and is still canon.
Replace WORLD.md references with docs/narrative/FLAME-FOUNDATION.md where the intent was
"current narrative canon"; remove them where the reference is simply dead. Confirm that
SYSTEMS.md, docs/data/, and docs/design/ exist (they do — note that docs/GDD-TODO.md:91
claims docs/data/ and docs/design/ do not exist, which is itself stale, but do not edit
GDD-TODO.md here; task-001 owns that file).

Add a short line to each definition you touch recording that its canon list was verified
on 2026-07-26 and that the narrative reset was 2026-07-22.

Write ONLY files under .claude/agents/. Do not edit WORLD.md, GDD.md, docs/GDD-TODO.md,
or any skill. Do not change any agent's tools: or model: frontmatter — canon references
only.
```
