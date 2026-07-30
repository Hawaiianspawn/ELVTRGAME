---
id: 090
title: Constrain weapon×projectile pairing so a beam can't fire an arrow
status: done
agent: sim-director
model: sonnet
owns: ["docs/data/hero-builds.json", "docs/data/hero-builds.schema.md", "docs/design/hero-build-variety.md", "Scripts/sim/variety.py"]
resources: []
depends-on: []
epic: build-space-differentiates
evidence: A `weapon_projectile_legality` table in hero-builds.json, variety.py rolling against it, a corrected `legal_count`, and a before/after roll on the same seed showing the incoherent pairings gone
score: {feel: 1, risk: 1, cost: 1}
source: user
teammate: weapon-projectile
decided: "2026-07-29 done"
---

## Why now
`Scripts/sim/variety.py:66-67` draws `weapon_id` and `projectile_id` independently from
the chassis's own `legal_weapons` / `legal_projectiles` lists, with nothing coupling the
two. Measured on the committed data at seed 7 and seed 3, that produces builds like
**Continuous Beam firing a Fletched Arrow** and **Cleave Melee Sweep firing a Shrapnel
Spread** — a laser cannon shooting an arrow, and a melee sweep launching a shrapnel cone.
Every variety report the harness prints, including task-080's top-10 table, is ranking a
population that contains these. The `legal_count: 999` figure in `hero-builds.json` is
also overstated for the same reason: it multiplies the two axes as if independent.

Nothing is on fire — this is sim-space only, nothing runtime-reads it. But it is the
cheapest correctness fix available on the build space, and task-091 measures that space,
so it should be measured after this lands rather than before.

## Done when
- `hero-builds.json` carries an explicit weapon→legal-projectile mapping (a
  `weapon_projectile_legality` block, or per-weapon `legal_projectiles` on each
  `weapon_archetypes` row — pick one, state which and why in the file's own note).
  A `melee_instant` weapon accepts only `melee_swing`; a beam accepts only `hitscan`
  travel types; a lobber accepts only `ballistic_arc`. Derive the rule from the
  `travel_type`/`resolve_type` fields that already exist rather than inventing a
  parallel taxonomy.
- `variety.py`'s roll picks a weapon, then picks a projectile legal **for that weapon**
  intersected with the chassis's `legal_projectiles`. If that intersection is ever
  empty for a legal chassis/weapon pair, that is a data bug — fail loudly, do not
  silently fall back.
- `reachable_build_count.legal_count` and `per_chassis` are recomputed under the new
  constraint and the note explains that the previous 999 counted impossible pairings.
  Show the arithmetic; do not hand-wave it.
- `calibration_builds` still reproduce `unit-types.json`'s Spearmen and Archers rows
  **exactly**. This is the non-negotiable check — if constraining pairings makes either
  calibration build illegal, the constraint is wrong, not the calibration.
- `py Scripts/sim/validate.py` shows no NEW failures. Check 3 (GATE1 wave-attrition)
  already fails on `master`; it is expected to still fail and that is not this task's
  problem.
- `docs/design/hero-build-variety.md` records the decision and supersedes its own
  "every combination is reachable" framing where that is now wrong.

## Spawn prompt

```
You are executing task-090. You are the sim-director for Kindled (the game is named
Kindled, NOT Emberkeep — Emberkeep is a discarded working title that still appears in
~97 files including some skill and task files; never take the name from those).

THE BUG, measured, not hypothesised:

Scripts/sim/variety.py lines 66-67 roll weapon_id and projectile_id independently:

    "weapon_id": rng.choice(chassis["legal_weapons"]),
    "projectile_id": rng.choice(chassis["legal_projectiles"]),

Nothing couples them, so incoherent builds are legal. Reproduce it yourself before
changing anything:

    py Scripts/sim/variety.py --scenario floor1-swarm-wave --seed 7
    py Scripts/sim/variety.py --scenario floor1-swarm-wave --seed 3

At seed 7, rank 8 is "Archer / Continuous Beam / Fletched Arrow" — a laser cannon
firing an arrow. At seed 3, rank 1 is "Beastcaller / Cleave Melee Sweep / Shrapnel
Spread" — a melee sweep firing a shrapnel cone. Save that before-output; the handback
needs a before/after on the same seed.

YOUR JOB:

1. Add an explicit weapon→legal-projectile constraint to docs/data/hero-builds.json.
   Derive it from the `travel_type` and `resolve_type` fields the `projectiles` table
   ALREADY carries — do not invent a second taxonomy alongside them. A melee weapon
   takes only `melee_instant` projectiles; a continuous beam takes only `hitscan`; an
   arcing lobber takes only `ballistic_arc`; and so on. You decide the exact shape
   (a top-level `weapon_projectile_legality` block, or per-weapon `legal_projectiles`
   on each `weapon_archetypes` row) — but state which you chose and why in the file's
   own note field, the way every other block in that file documents itself.

2. Make variety.py's roll respect it: pick the weapon first, then pick from
   (weapon's legal projectiles ∩ chassis's legal_projectiles). If that intersection
   is EMPTY for any legal chassis/weapon pair, raise — do not silently fall back to
   the chassis list, and do not quietly drop the build. An empty intersection means
   the data is wrong and you want to know.

3. Recompute `reachable_build_count.legal_count` and `.per_chassis` under the new
   constraint. The current 999 multiplies weapons × projectiles as if independent, so
   it counts builds that cannot exist. Show your arithmetic in the note. A scratch
   script is fine (that is what the existing note says was used) but the NUMBER in the
   file must be right, not estimated.

4. HARD CHECK, non-negotiable: `calibration_builds.spearmen` and
   `calibration_builds.archers` must remain exactly legal and their `build_stat_block`
   values must still match docs/data/unit-types.json's `types.spearmen.combat` and
   `types.archers.combat` exactly. Those two builds are the anchor that keeps this
   whole space honest. If your constraint makes either illegal, YOUR CONSTRAINT IS
   WRONG — fix the constraint, never the calibration.

5. Run `py Scripts/sim/validate.py`. Check 3 (GATE1 wave-attrition reproduction)
   ALREADY FAILS on master — see docs/sim/LIMITATIONS.md §1. It is expected to still
   fail. Do not try to fix it, and absolutely do not tune combat-model-constants.json
   to make it pass; LIMITATIONS.md §1 explicitly forbids that. Only NEW failures are
   yours.

6. Update docs/design/hero-build-variety.md: record the decision, and correct its own
   "every combination is reachable" framing where that is now false.

DO NOT TOUCH: docs/data/unit-types.json, docs/data/entity-tiers.json,
docs/data/scenarios/**, Scripts/sim/combat_model.py, Scripts/sim/validate.py,
SYSTEMS.md, GDD.md, or anything under ELVTR/. You own exactly these four files:
docs/data/hero-builds.json, docs/data/hero-builds.schema.md,
docs/design/hero-build-variety.md, Scripts/sim/variety.py.

A sibling task (task-091) measures whether this build space actually differentiates
and it runs AFTER you close. Do not do its work — no spread analysis, no multi-seed
study, no new scenarios. Fix the pairing, correct the count, keep calibration exact.

HAND BACK: the before/after roll on seed 7 AND seed 3 side by side showing the
incoherent pairings gone; the old and new legal_count with the arithmetic; explicit
confirmation that both calibration builds still match unit-types.json exactly; and
validate.py's output. If you found a second incoherence class beyond weapon×projectile
(for example a modification that cannot apply to its own chassis), name it but do NOT
fix it — say so and leave it for a follow-up task.
```
