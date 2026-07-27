---
id: 034
title: Full loot & itemization system
status: proposed
agent: gameplay-director
owns: ["docs/design/loot-full.md"]
resources: []
depends-on: [6]
evidence: A full itemization spec — rarity tiers, stacking, evolutions, hero/retinue split — but only after loot v0 has been played.
score: {gate: 1, risk: 1, cost: 4}
source: docs/GDD-TODO.md:112
decided: ""
---

## Why now
**It is not now — recommend parking.** This is the full system that task-006's loot v0
deliberately is not.

`GDD.md` §12 Q7 already deferred it, and the deferral has since been *upgraded* rather
than merely maintained: the Q7 row now reads *"deferral is now legitimate: retinue growth
is the run reward loop (§3 genre spine), not loot."* Q15 confirmed the same thing from the
other direction — the roguelike run is primary and retinue growth is the progression axis.

So the design has actively moved away from needing this, and building it now would create
a second progression system competing with the one the game is actually about. The right
sequence is: ship loot v0 (task-006), play it, and find out whether the game wants more.

## Done when
Nothing, until v0 has been played and the answer is genuinely "we want more". If so:
- Rarity tiers, weights, stacking and evolution rules.
- Hero-vs-retinue item split — the banner-for-all-soldiers vs. weapon-for-hero twist.
- Pity and anti-frustration mechanics.
- Run-scoped, no persistent gear (design law 7), and subordinate to retinue growth rather
  than competing with it.

## Recommended verdict
`py Scripts/backlog.py park 34 -r "GDD Q7 deferral upgraded to legitimate — retinue growth
is the reward loop, not loot. Revisit only after loot v0 (task-006) has been played."`
