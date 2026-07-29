---
id: 079
title: Design the combinatorial hero-build variety layer — axes, weapon archetypes, and composition synergies
status: done
agent: gameplay-director
model: sonnet
owns:
  - "docs/design/hero-build-variety.md"
  - "docs/data/hero-builds.json"
  - "docs/data/hero-builds.schema.md"
resources: []
depends-on: []
epic: hero-variety
evidence: >
  `docs/data/hero-builds.json` defines the hero-side build space as multiplying
  axes (chassis x weapon archetype x projectile x ability/modification x
  origin-world), plus a data-expressed chassis constraint table that keeps a
  roll themed to the character, and states BOTH the raw product and the legal
  post-constraint build count — the legal count landing in the hundreds; every
  weapon archetype row carries rate_of_fire, range, min_range,
  damage_per_shot, targets_per_shot, aoe_radius and accuracy; every unique type
  fills in one shared measurement base stat block in stated units so any two
  builds compare directly; every synergy rule is expressed as a
  machine-checkable condition over a roster's axis values (no prose-only
  buffs); `docs/data/hero-builds.schema.md` documents every field and unit;
  `docs/design/hero-build-variety.md` shows two contrasting builds on the
  shared stat block and states which numbers are cited from existing data and
  which are declared prototype dials.
score: {feel: 3, risk: 2, cost: 2}
source: user
teammate: hero-build-variety
decided: "2026-07-29 done"
---

## Why now

The owner brought this as a new game-design goal (2026-07-29): hero-side units
should have **hundreds** of individual varieties while enemies stay at 6-8
types, and the fun is meant to come from the randomization and the synergies
between what you rolled. Nothing in the repo supports that today. `grep`ped
2026-07-29: there is **no weapon system anywhere in `docs/design/` or
`docs/data/`** — the only "weapon" mentions are art-request prop lists.
`docs/data/unit-types.json` has exactly two hero-side types (spearmen,
archers), each with one flat combat block and no notion of a per-unit loadout.

So the whole variety axis the design goal rests on is missing at the data
layer, and the simulation harness (`Scripts/sim/`) has nothing to roll. Until
this file exists, task-080 has no build space to sample and the "top 10
performers" report has nothing to rank.

The enemy side is deliberately **out of scope**. `docs/data/entity-tiers.json`
already carries the enemy tier ladder and the owner's instruction is that it
stays small (6-8 types). Do not touch it.

## Done when

`docs/data/hero-builds.json` exists and a reader can compute the size of the
build space from it without guessing:

- **Axes, not a list.** Hundreds of builds come from multiplying a handful of
  axes, not from enumerating hundreds of rows. Name each axis, its members, and
  its cardinality; state the product. A hand-written list of 300 builds is a
  wrong answer to this task.
- **Weapon archetypes are mechanical, flavour is a label.** The owner's
  decision (2026-07-29, verbatim): *"We are going to not lock ourselves to just
  medieval, we may even have enemies be of different worlds as well. As
  everyone gets merged together."* So each archetype row is defined by its
  numbers and carries a list of flavour names spanning worlds — one archetype
  can read as a ballista bolt or a laser lance. Do not pick a single setting.
- **Origin-world is an axis and it is where synergy lives.** The cross-world
  merge is the narrative premise (`docs/narrative/FLAME-FOUNDATION.md`), and it
  gives synergy rules a natural key: a roster's distribution across
  origin-worlds is something a rule can read.
- **Every synergy rule is checkable, not prose.** A rule states a condition
  over the rolled roster's axis values and the modifier it applies. Something
  a script can evaluate — `{"when": {"same_origin_count": ">=3"}, "grants":
  {"rate_of_fire_mult": 1.15}}` shape, exact form your call. A buff described
  only in English is not done.
- **Provenance is explicit.** Where a number can be cited (spearman/archer
  combat blocks in `unit-types.json`, the tier ladder in `upgrades.json`, the
  shared swing interval and chip floor in
  `docs/data/scenarios/combat-model-constants.json`), cite it. Everything else
  is a declared PROTOTYPE DIAL in the same voice `unit-types.json` and
  `economy.json` already use. Do not present invented numbers as measured.
- **Abilities and modifications are their own axis.** Owner, 2026-07-29: *"The
  abilities and modifications is important aspect of the character."* Not a
  footnote on the weapon row — a first-class axis that multiplies the space and
  that synergy rules can read.
- **One comparable base stat block per unique type.** Owner: *"We should have a
  measurement base stat for all unique types."* Every type declares the same
  named measurement fields in the same units, so any two builds anywhere in the
  space can be put side by side without a translation step. Task-080 ranks
  builds against each other; that ranking is meaningless if two types measure
  different things.
- **Projectile behaviour is explicit and varies per build.** Owner: *"Their
  projectile can be different."* Projectile type, travel behaviour, and whether
  it lands as a point hit or an area effect are fields, not flavour text.
- **Rolls stay themed to the character's visual read.** Owner: *"We should be
  able to get somewhat themed consistently to their character view. Example RPG
  archer would be shooting explosives that do damage in an area."* So the axes
  are **not** a free cross product — a chassis constrains which weapons,
  projectiles and abilities it can roll, and the file states those constraints
  as data. The reachable-build count must be computed **after** the constraints,
  not from the raw product, and must still land in the hundreds.
- **The two existing hero types must survive.** A spearman and an archer as
  they exist today should both be expressible as points in the new build space,
  so the harness's current numbers stay reachable and comparable.

`docs/design/hero-build-variety.md` carries the reasoning: why these axes, what
the intended spread of outcomes is, which rows you expect to be strong and why,
and the open questions you could not settle from the repo.

## Spawn prompt

```
You are the gameplay-director agent. Design the hero-side build variety layer
for Kindled/ELVTR. This is a design + data task: you write three files and no
code.

You own exactly these paths and must not write anything else:
  docs/design/hero-build-variety.md
  docs/data/hero-builds.json
  docs/data/hero-builds.schema.md

Do NOT touch: docs/data/entity-tiers.json, docs/data/unit-types.json,
docs/data/upgrades.json, docs/data/squads.json, SYSTEMS.md, GDD.md, anything
under Scripts/, anything under ELVTR/Source/. Read them freely — writing them
is another task's job.

THE GOAL, from the owner directly (2026-07-29):

  "A new game design goal is to give specific rules and buffs based on their
  variety, so example we have a rpg archer, he would likely shoot an rpg, the
  next a laser cannon, stats rate of fire, range, etc. This is the randomizing
  and synergies that makes it fun."

  "We are focusing on making many individual character varieties for the
  heroes, but not the enemies. Enemies get like 6-8 types, the Hero units on
  our side should get 100's."

  "We are going to not lock ourselves to just medieval, we may even have
  enemies be of different worlds as well. As everyone gets merged together."

  "The abilities and modifications is important aspect of the character. We
  should have a measurment base stat for all unique types. Their projectile can
  be different. We should be able to get somewhat themed consistently to their
  character view. Example RPG archer would be shooting explosives that do
  damage in an area."

  Scope, also from the owner: "We dont need the editor right now this is all in
  json and simulation space." Nothing here needs to be runtime-readable by
  Unreal yet. JSON that Python can read is the whole target.

WHAT THIS MUST BE

1. COMBINATORIAL, NOT ENUMERATED — BUT CONSTRAINED, NOT A FREE PRODUCT.
   Hundreds of hero builds come from multiplying axes: chassis x weapon
   archetype x projectile x ability/modification x origin-world. State each
   axis's members and cardinality.

   Then constrain it. The owner requires rolls to stay themed to what the
   character visibly is: "We should be able to get somewhat themed consistently
   to their character view. Example RPG archer would be shooting explosives
   that do damage in an area." So a chassis declares which weapons,
   projectiles and abilities it can legally roll — as DATA in the file, not as
   advice in the prose — and an archer chassis rolling a shield-wall ability is
   an illegal combination the constraint table must exclude. Compute the
   reachable-build count AFTER the constraints and report both numbers (raw
   product vs. legal builds). The legal count must still land in the hundreds;
   if your constraints crush it into the dozens, loosen them and say what you
   loosened. If you find yourself typing a 300-row list, you have taken the
   wrong shape.

2. WEAPON ARCHETYPES DEFINED BY NUMBERS, FLAVOUR AS A LABEL LIST. Every
   archetype row carries at minimum: rate_of_fire (shots/sec),
   range, min_range, damage_per_shot, targets_per_shot, aoe_radius, accuracy.
   Each row also carries several flavour names from different worlds (an
   arcing-AoE row might list "rocket", "greek fire pot", "mortar shell") —
   because the owner explicitly refused to lock the setting. Archetypes must
   actually differ in kind, not just scale: a long-range slow single-target
   weapon and a short-range fast cleaving one should produce genuinely
   different fight shapes, not the same DPS with different words.

2b. PROJECTILE IS ITS OWN AXIS. Owner: "Their projectile can be different."
   Give projectiles their own rows — what travels, how it travels, and whether
   it resolves as a point hit, a pierce, or an area effect — so the same weapon
   archetype can throw different things and a chassis can be themed by what it
   visibly fires. Say which projectile fields the sim can consume and which are
   presentation-only; task-080 has to map every field you define onto a model
   whose primitives are listed further down, and a field with no mapping is
   decoration.

2c. ABILITIES AND MODIFICATIONS ARE A FIRST-CLASS AXIS. Owner: "The abilities
   and modifications is important aspect of the character." Design them as an
   axis that multiplies the space and that synergy rules can read — not as
   flavour on a weapon row. Distinguish, and say which is which: a
   MODIFICATION alters the build's own numbers (a scope that trades rate of
   fire for range), an ABILITY is a conditional or periodic effect. Where an
   ability cannot be represented in a DPS-and-HP attrition model, say so
   explicitly in the design doc rather than encoding a number the sim will
   silently ignore.

2d. ONE COMPARABLE BASE STAT BLOCK FOR EVERY UNIQUE TYPE. Owner: "We should
   have a measurment base stat for all unique types." Define one named set of
   measurement fields, in stated units, that EVERY type fills in — so any two
   builds in the space can be compared directly with no translation. This is
   load-bearing for task-080, which ranks builds against each other; if two
   types measure different things the ranking is meaningless. Include a worked
   example in the design doc showing two very different builds side by side on
   that same block.

3. ORIGIN-WORLD AS THE SYNERGY KEY. The cross-world merge is current narrative
   canon (read docs/narrative/FLAME-FOUNDATION.md — and note WORLD.md is
   SUPERSEDED, do not cite it). A roster's distribution across origin-worlds is
   the natural thing a synergy rule reads.

4. MACHINE-CHECKABLE SYNERGY RULES. Each rule is a condition over the rolled
   roster's axis values plus the modifiers it grants. A downstream Python
   script (task-080) will evaluate these directly, so they must be data, not
   English. Pick a concrete shape and document it in the schema file. Include
   at least one anti-synergy or a rule with a cost, so the optimal answer is
   not simply "maximise every buff".

5. THE CURRENT TWO TYPES STAY REACHABLE. docs/data/unit-types.json's spearmen
   (130hp / 30dps / 95 engage / 8 targets) and archers (70hp / 18dps / 750
   engage / 150 min / 1 target) must each be expressible as a point in your
   build space. The harness's existing numbers are the only calibration this
   layer has; do not orphan them.

6. HONEST PROVENANCE. Cite what you can: docs/data/unit-types.json,
   docs/data/upgrades.json (tier ladder),
   docs/data/scenarios/combat-model-constants.json (shared swing interval,
   armor chip floor), docs/design/entity-tiers.md, docs/design/scaling-curve.md.
   Mark everything else as a PROTOTYPE DIAL in the same explicit voice
   docs/data/unit-types.json and docs/data/economy.json already use. Do not
   dress a guess as a measurement.

WHAT TO READ FIRST
  docs/data/unit-types.json           the two hero types that exist today
  docs/data/upgrades.json             the retinue tier ladder
  docs/data/entity-tiers.json         the enemy side (READ ONLY, stays 6-8)
  docs/data/squads.json               how units are allocated today
  docs/design/squad-group-system.md   the typed-unit design this extends
  docs/design/entity-tiers.md         the enemy stat-block reasoning
  docs/sim/README.md + docs/sim/MODEL.md
       what the Python harness can actually consume. Your archetype fields
       should be consumable by a model whose primitives are hp, dps,
       swing_interval, engage_range, min_engage_range, targets_per_hit and
       armor — see Scripts/sim/data_loader.py's `retinue_fighter()` for the
       exact flat fighter shape. Inventing a field no model can use is how
       this ends up decorative.
  docs/sim/LIMITATIONS.md
       what the harness is NOT trustworthy for. Read §1 before you assume a
       wave-attrition survivor count validates anything.
  docs/data/unit-types.schema.md      the schema-doc voice to match

HAND BACK
  - the three file paths
  - the axis table with cardinalities, the RAW product, and the LEGAL build
    count after the chassis coherence constraints
  - the archetype count and a one-line read on how they differ in kind
  - the shared base stat block, with two contrasting builds shown on it
  - the synergy rules, and which one is the anti-synergy
  - which projectile/ability fields the sim can consume and which cannot be
    represented in a DPS-and-HP attrition model
  - which numbers are cited vs. prototype dials
  - the open questions you could not settle from the repo
Then stop. Do not edit any status field in docs/backlog/ — the lead closes it.
```
