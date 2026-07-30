---
id: 098
title: Open the armour gate so more than three weapon archetypes are worth taking
status: done
agent: gameplay-director
model: sonnet
owns:
  - "docs/data/hero-builds.json"
  - "docs/design/hero-build-variety.md"
  - "docs/sim/DIFFERENTIATION.md"
resources: []
depends-on: []
epic: sim-irons-out-fun
evidence: >
  A build-side data change in `docs/data/hero-builds.json` plus a before/after
  re-run of `py Scripts/sim/differentiation.py --seeds 25` committed into
  `docs/sim/DIFFERENTIATION.md`, showing how many of the 9 reachable weapon
  archetypes clear `brood_elite`/`brood_boss` armour before and after, and what
  happened to rank-1 share, the @0.5x competitive band, and
  `cleave_melee_sweep`'s rank-1 frequency in both point-target scenarios. The
  wave scenario is re-run and reported alongside, caveated per LIMITATIONS §1.
score: {feel: 3, risk: 2, cost: 2}
source: user
teammate: armour-gate
decided: "2026-07-30 done"
---

## Why now

`docs/sim/DIFFERENTIATION.md` (task-091, done 2026-07-29) already measured this
and named the structural cause outright: **6 of the 9 reachable weapon
archetypes — 7 of all 10 counting the un-rollable `shotgun_spread` — sit at or
effectively at `chip_floor` against `brood_elite` / `brood_boss` armour. Only
`siege_artillery`, `arcing_aoe_lobber` and `cleave_melee_sweep` clear it
meaningfully.** That is scenario-independent, and it is why
`cleave_melee_sweep` takes rank 1 in 22/25 elite seeds and 25/25 wave seeds
despite its cleave mechanic being *completely inert* against a single target.

The owner's stated reason for the whole hero-build layer is that characters will
be unique. Two thirds of the weapon space currently reduces to chip damage
against the enemies the run is built around. Nothing breaks while this stays
undone — it just means the build variety measured in task-091 is mostly
decorative, and every downstream measurement (task-097's decision branches
included) is run on a space where the answer is already narrowed to three
weapons.

The measurement exists, the finding is named, and nobody has acted on it. That
is the whole reason this is `feel: 3` rather than another measurement task.

## Done when

- The gate is opened from the **build side**, in `docs/data/hero-builds.json`.
  **Do not change enemy armour in `docs/data/entity-tiers.json`, and do not
  change `chip_floor` in `docs/data/scenarios/combat-model-constants.json`** —
  the first feeds the *validated* point-target model that reproduces
  `entity-tiers.md` §7's table exactly, and the second is locked by task-076.
  Moving either would trade a measured, trusted number for an untraceable one
  and would break `drift_check.py`'s committed baseline. The levers available on
  the build side are weapon damage/swing values, and which chassis and weapons
  can reach an armour-penetration modification (`piercing_rounds` currently
  reaches only `cleave_melee_sweep`'s two legal chassis — that concentration is
  named in DIFFERENTIATION.md as the thing that closes cleave's remaining power
  gap to `siege_artillery`).
- Every changed number carries a written rationale in
  `docs/design/hero-build-variety.md` — what it was, what it is, and the design
  reason. A number changed because it made the sim output look better, with no
  design reason behind it, is exactly the fudge factor task-063 and
  `LIMITATIONS.md` were explicit about refusing.
- **Before/after, same seeds, committed.** Append a task-098 section to
  `docs/sim/DIFFERENTIATION.md` with `py Scripts/sim/differentiation.py --seeds
  25` run on the unchanged data and again on the changed data, reporting for
  both point-target scenarios and the wave scenario: how many archetypes clear
  the armour gate, average rank-1 share, average builds competitive @0.5x and
  @0.8x, and rank-1 weapon frequency. Do not delete or rewrite task-091's
  existing sections — append.
- **The bar is that the space widens without inverting.** More archetypes
  clearing the gate and a lower rank-1 share is the goal; a new single dominant
  weapon replacing `cleave_melee_sweep` is a failure, and so is flattening every
  weapon to interchangeable. If the change cannot widen the space without one of
  those, say so and hand back the finding rather than shipping a worse
  monoculture.
- `py Scripts/sim/validate.py` and `py Scripts/sim/drift_check.py` both still
  pass. If `drift_check` flags, **stop and report** — a hero-build data change
  should not move the committed sweep baseline, and if it does, that is a real
  finding about coupling, not something to refresh away.
- The wave-scenario numbers are reported with the `LIMITATIONS.md` §1 caveat
  attached — that model does not reproduce GATE1's measured survival and every
  roll wipes; wave numbers here are relative comparisons only. The point-target
  numbers are the validated ones and carry the argument.

## Spawn prompt

```
You are the gameplay-director. Open the armour gate in the hero-build space so
more than three weapon archetypes are worth taking, and prove it with a
before/after run of the committed simulation harness.

Read first:
  docs/sim/DIFFERENTIATION.md    -- task-091's measurement. The finding you are
                                    acting on is in its final section: 6 of 9
                                    reachable weapon archetypes sit at or
                                    effectively at chip_floor against
                                    brood_elite / brood_boss armour, and that
                                    armour gate -- not chassis legality -- is
                                    why cleave_melee_sweep wins
  docs/data/hero-builds.json     -- the build space; the file you are changing
  docs/design/hero-build-variety.md -- where the rationale goes
  docs/data/entity-tiers.json    -- READ ONLY. brood_elite armour 12,
                                    brood_boss 14, brood_titan 20
  docs/sim/LIMITATIONS.md        -- §1 and §3, the trust boundaries
  Scripts/sim/differentiation.py -- READ ONLY. The tool you run

HARD CONSTRAINT -- you write exactly three files:
  docs/data/hero-builds.json
  docs/design/hero-build-variety.md
  docs/sim/DIFFERENTIATION.md   (APPEND a task-098 section; do not rewrite or
                                 delete task-091's existing sections)
Do NOT edit docs/data/entity-tiers.json, do NOT edit chip_floor in
docs/data/scenarios/combat-model-constants.json (task-076 holds a lock on that
file), and do NOT edit any file under Scripts/sim/. Lowering enemy armour or
the chip floor would trade a validated, cited number -- the point-target model
reproduces entity-tiers.md §7's table exactly -- for an untraceable one, and
would move drift_check.py's committed baseline. Fix this from the BUILD side.

The levers you do have: weapon damage and swing values in hero-builds.json, and
which chassis/weapons can reach an armour-penetration modification.
DIFFERENTIATION.md notes piercing_rounds currently reaches only
cleave_melee_sweep's two legal chassis -- that concentration is part of the
cause.

Method:
1. Run `py Scripts/sim/differentiation.py --seeds 25` on the UNCHANGED data and
   record the baseline: archetypes clearing the gate, avg rank-1 share, avg
   builds competitive @0.5x and @0.8x, rank-1 weapon frequency -- for
   floor2-elite-point-target, floor3-boss-point-target and floor1-swarm-wave.
2. Make the data change. EVERY changed number gets a written design rationale in
   docs/design/hero-build-variety.md: old value, new value, why. A number changed
   because it made the output look better, with no design reason, is the exact
   fudge factor this harness refuses -- do not do it.
3. Re-run the same command on the changed data. Append both tables to
   docs/sim/DIFFERENTIATION.md as a task-098 section.
4. Run `py Scripts/sim/validate.py` and `py Scripts/sim/drift_check.py`. Both
   must pass. If drift_check flags, STOP and report it -- a hero-build change
   should not move the sweep baseline, and if it does that is a real coupling
   finding, not something to refresh away.

The bar: MORE archetypes clear the gate and rank-1 share DROPS, without a new
single weapon replacing cleave_melee_sweep at the top and without flattening
every weapon into interchangeability. If you cannot widen the space without one
of those two outcomes, hand back that finding instead of shipping a worse
monoculture. A negative result here is a real deliverable.

Report wave-scenario numbers with the LIMITATIONS.md §1 caveat attached (that
model does not reproduce GATE1's measured survival; every roll wipes; wave
numbers are relative only). The point-target numbers are the validated ones --
lead with those.

Hand back: the before/after table, every number you changed with its rationale,
the validate/drift_check results, and your verdict on whether the space actually
widened.
```
