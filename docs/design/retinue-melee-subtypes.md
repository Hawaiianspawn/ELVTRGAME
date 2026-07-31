# Retinue melee sub-types — decision

This is the task-088 decision on whether the retinue's melee identity axis splits
into sub-types. It extends `SYSTEMS.md` §1 (entity tiers) and §6 (retinue tuning),
and turns `docs/data/unit-types.json`'s single `spearmen` row into nine.

## Decision

**Yes, ship a split — at 9 stat rows for the atlas's 11 team looks** (index 0
plus the 10 kept knight silhouettes), not 11. Two adjacent pairs in task-094's
ten-way ranking are close enough that shipping them as separate stat blocks
would be a difference no player can feel; every other row is separated by a
measured, stable margin. This keeps the owner's stated preference for "a fair
bit of unique knights, each with its own base stats" — 9 distinct combat
identities is real variety — while not pretending two numerically identical
fighters are different units.

**Addendum, same day:** `task-085` landed the team atlas as 11 looks, not 10 —
atlas index 0 is the pre-split base retinue look, `Swarm.TeamVariantWeights`
default ~20% of the army. It isn't one of task-094's ten measured silhouettes
(no shape data, nothing to derive from), so it isn't part of the 8-row knight
split above; it ships as its own 9th row, carrying `spearmen`'s existing Gate 1
combat block unchanged, so it reads as the baseline rather than an invented
eleventh knight identity.

**The caveat travels with every number below.** All of it derives from
`Scripts/sim/variety.py --mode subtypes`, a wave-attrition harness that
`docs/sim/LIMITATIONS.md` §1 documents does NOT reproduce GATE1's measured
109-111-of-120 survival at committed defaults — it predicts a full wipe, on every
candidate, in every run task-094 measured. The ranking and the gap percentages
below are a **relative comparison of these ten profiles against each other inside
one shared, imperfect model** — real enough to justify a design decision on
identity and row count, not evidence that any individual HP/DPS number is
balanced. The values that follow are gameplay-director's own tuning pass off that
ranking (rounded, two rows blended, two disagreements resolved by design judgment)
— they are not copied from `docs/data/scenarios/retinue-subtypes.json`, which
stays sim-director's candidate/experiment file.

## Row count and why

task-094 measured, across 2 scenarios and 5 randomized headcount splits (7 runs,
zero rank swaps in all 7): a stable order

```
v8_heavycloak > v3_shieldbreak > v13_maceraised > v2_lanceout > v10_bracedstaff
> v4_overhead > v11_midguard > v6_simplecolumn > v7_barestance > v1_narrowguard
```

with adjacent-pair gaps ranging 1.8%–23.2%. Two pairs sat far enough below every
other adjacent gap, consistently across all 7 runs, that I'm treating them as one
stat identity each:

- **`v4_overhead` / `v11_midguard`** (ranks 6/7) — 1.8–2.4% apart in every run.
  The tightest pair measured. **Merged.**
- **`v7_barestance` / `v1_narrowguard`** (ranks 9/10) — 3.3–3.8% apart in every
  run. **Merged.**

task-094 also flagged a third, softer pair: **`v10_bracedstaff` / `v4_overhead`**
(ranks 5/6), 4.8–5.9% apart — stable, but roughly double the gap of the two
pairs above, and explicitly reported as a near-miss rather than a confident call.
**I'm not merging it.** Reasons:

1. The gap is consistently about 2x the size of the two pairs I did merge — every
   other adjacent gap in the ten-way table that's "clearly separated" starts at
   5.4%, and 4.8–5.9% sits right at that boundary, not inside the collapsed band.
2. `v4_overhead` is already the anchor of the other merge (with `v11_midguard`).
   Folding `v10_bracedstaff` in too would make a single stat row span three
   consecutive ranks (5, 6, 7) — measured pairwise closeness between 5↔6 doesn't
   establish that 5 and 7 are indistinguishable, and they aren't: `v10_bracedstaff`
   vs. `v11_midguard` (ranks 5 vs. 7) sit ~6.7% apart on the primary gate1 run,
   comparable to gaps I'm treating as real everywhere else in the table.
3. Owner's stated preference is for more distinct identities, not fewer. Where the
   evidence is genuinely ambiguous (this pair, unlike the other two) I'm resolving
   the ambiguity toward keeping `v10_bracedstaff` distinct rather than collapsing
   it, since the numbers don't force the merge and the design intent points away
   from it.

That leaves **8 knight rows**: 6 solo (`heavycloak`, `shieldbreak`, `maceraised`,
`lanceout`, `bracedstaff`, `simplecolumn`) and 2 merged pairs (`line_standard`,
`line_light`) — plus the unrelated `retinue_base` row for atlas index 0 (see
addendum above), **9 rows total**.

## The two flagged formula-vs-intuition disagreements

**`v3_shieldbreak`'s dps direction.** The uniform asymmetry→dps formula reads
`v3_shieldbreak` as one of the highest-DPS candidates of the ten, not the
"more HP, less DPS" defensive read task-086's brief guessed at a glance from the
name. **Resolved toward the formula: `v3_shieldbreak` ships as a high-DPS row.**
"Shieldbreak" itself is not actually a defensive name — breaking a shield is an
offensive act — so the intuitive guess and the silhouette's own name were never
as aligned as the brief's first pass assumed. The formula's read (an off-center,
committed, weapon-presented stance = offensive commitment) fits the name at least
as well as the discarded guess, and overriding a uniformly-applied, tested formula
without a concrete reason would just be hand-tuning wearing a design justification.

**`v4_overhead`'s two-handed hp/dps trade.** The primary 4-axis formula has no
term for the "two-handed, commits fully, less time to guard" read the silhouette's
enclosed-arm hole suggests; the holes-driven `v4_overhead_holes_swap` probe that
tested it (+15% dps / −15% hp) landed statistically tied with `v11_midguard` in
the seed sweep, same as unmodified `v4_overhead` does. **Resolved by not adopting
the probe's swapped numbers**, for two reasons: it's explicitly a probe
(`is_probe: true`), never meant to ship as-is, and it's moot for the stat block
either way now that `v4_overhead` merges with `v11_midguard` — the "two-handed"
read is a real, worth-keeping visual and animation identity, not a stat-row
identity, since nothing in the data supports it surviving as a separate row.
`line_standard`'s numbers use the primary-formula `v4_overhead` value (blended
with `v11_midguard`, see below), not the swap.

## The mapping — all 11 atlas looks to their stat row

Keyed by **atlas index**, per `docs/data/art/team-variants.json`'s
`variants[].index` / `SwarmSheet::Team` (verified against that file and
`docs/perf/niagara-sprite-path.md` §2 directly, 2026-07-30) — this is the
binding key `task-095` reads. The silhouette name/id is carried alongside for
readability only; it is not what Mass looks up.

| Index | Silhouette (`retinue-subtypes.json` candidate / `team-variants.json` id) | Stat row | Row shared with |
|---|---|---|---|
| 0 | `retinue` (base, pre-split look) | `retinue_base` | — solo, not derived (see addendum) |
| 1 | `v1_narrowguard` | `line_light` | `v7_barestance` (3.3–3.8% apart, every run) |
| 2 | `v2_lanceout` | `lanceout` | — solo |
| 3 | `v3_shieldbreak` | `shieldbreak` | — solo |
| 4 | `v4_overhead` | `line_standard` | `v11_midguard` (1.8–2.4% apart, every run) |
| 5 | `v6_simplecolumn` | `simplecolumn` | — solo |
| 6 | `v7_barestance` | `line_light` | `v1_narrowguard` |
| 7 | `v8_heavycloak` | `heavycloak` | — solo |
| 8 | `v10_bracedstaff` | `bracedstaff` | — solo (see near-miss note above) |
| 9 | `v11_midguard` | `line_standard` | `v4_overhead` |
| 10 | `v13_maceraised` | `maceraised` | — solo |

## The shipped dials (my tuning, `docs/data/unit-types.json`)

Solo rows use the ten-way primary-formula `combat` block from
`retinue-subtypes.json`, rounded to whole numbers to match `unit-types.json`'s
existing house convention (`spearmen`/`archers` are whole-number dials; the
candidate file's one-decimal precision is sim-harness precision, not a shipped
dial's). Merged rows use the arithmetic mean of their two silhouettes' primary
combat blocks, then rounded the same way — a plain, defensible way to give one
identity to two silhouettes that the harness can't tell apart, without
privileging either one's exact numbers over the other's.

| Row | max_hp | dps | engage_range | targets_per_hit |
|---|---|---|---|---|
| `retinue_base` | 130 | 30 | 95 | 8 |
| `heavycloak` | 165 | 30 | 100 | 8† |
| `shieldbreak` | 140 | 36 | 99 | 8 |
| `maceraised` | 146 | 37 | 93 | 8 |
| `lanceout` | 133 | 30 | 105 | 7 |
| `bracedstaff` | 123 | 33 | 110 | 6 |
| `line_standard` | 124 | 28 | 96 | 8 |
| `simplecolumn` | 119 | 28 | 82 | 8† |
| `line_light` | 114 | 25 | 85 | 8 |

† `heavycloak`/`simplecolumn` originally derived at 9/10 — respecced to 8 by task-099, see
"Ceiling reconciliation" below.

`min_engage_range` (0) and `move_speed_scale` (1.0) are uniform across all 9 rows
— no measured shape axis feeds either one in the source formula, so there is
nothing to derive a variation from; inventing one would be adding a stat axis the
data doesn't support. `line_light`'s `targets_per_hit` is the one place I rounded
against the arithmetic mean (8.5 → 8, not 9): it's the bottom-tier row by every
other stat, and rounding up would nudge it toward `line_standard`/`simplecolumn`'s
band for no reason the data gives me.

Formation, `stance_reflavor`, and `growth_source_weight` are **not** varied per
row — all 9 inherit `spearmen`'s existing values unchanged. Nothing in this
decision or its source data touches formation/behavior, and Mass Entity design
law (GDD §10, this director's design law #5) keeps soldier-tier behavior shared
and data-cheap; only the combat stat block is a per-row dial.

## Ceiling reconciliation (task-099)

`task-095` wired these nine rows into Mass and hit a pre-existing constraint the numbers
above didn't know about: the shipped combat loop's nearest-target array is fixed at 8, and
every `TargetsPerHit` accessor clamps to `1..8` before use
(`SwarmCombatProcessors.cpp:253-260`). `heavycloak` (derived 9) and `simplecolumn` (derived
10) were both landing above that ceiling and being silently clamped to 8 in play —
`unit-types.json` was carrying numbers the engine never actually used.

**Resolved: respec both rows to `targets_per_hit: 8`.** This isn't a new derivation pass —
8 is the nearest legal value under the ceiling to each row's derived rank, and it's exactly
the value the combat loop was already producing. Nothing about the live game changes;
the data file now says what ships instead of disagreeing with it. I did not consider
touching the formula or re-deriving other rows to "make room" above 8 for these two —
`unit-types.json`'s own house rule is that this formula gets applied uniformly and read off,
not hand-tuned per disagreement, and the two `retinue-subtypes.json` disagreements already
flagged (`shieldbreak`'s dps direction, `overhead`'s holes probe) were left unresolved by
the formula's own author for the same reason.

**What this costs.** The axis had little spread even before this fix: of the nine rows,
five already land at 8 from the raw formula (`retinue_base`, `shieldbreak`, `maceraised`,
`line_standard`, `line_light`) — `heavycloak` and `simplecolumn` were the *only* spread at
the top of the range. The live spread, unchanged by this fix since it only formalizes what
already shipped, is `8, 8, 8, 8, 8, 8, 8, 7, 6` — seven of nine rows tied, with `lanceout`
(7) and `bracedstaff` (6) carrying the axis's entire remaining differentiation.
`targets_per_hit` cannot distinguish `heavycloak`/`simplecolumn` from the pack anymore; it
already couldn't, in play, since `task-095` shipped.

Their identity isn't lost, it just doesn't live on this axis: `heavycloak` carries the
highest `max_hp` of all nine rows (165 vs. next-highest `maceraised` at 146) — the "big and
tanky" read survives entirely on HP. `simplecolumn` carries the shortest `engage_range` of
all nine (82 vs. next-shortest `line_light` at 85) and ties `line_light` for the lowest dps
(28) — and `retinue-subtypes.json`'s own `v6_simplecolumn` derivation note already flagged
that its short `engage_range` caps its effective cleave well below its nominal
`targets_per_hit` via `melee_reach_per_exposed_unit()`, independent of this engine ceiling.
Neither row goes flat; they're differentiated on three axes instead of four.

**Why not raise the ceiling (option 3).** Both rows land at their nearest legal clamp with
zero gameplay change from what's already shipped — there's no live problem this fix leaves
unsolved that would justify opening a fixed-size nearest-target array in the combat loop.
If a future pass wants genuine top-of-range spread on this axis (a row that reads as
"sweeps meaningfully more targets than the rest of the roster"), raising the ceiling is a
real follow-on with its own perf question on the Mass sim's nearest-K search — not implied
or requested by this fix, and not something two already-clamped rows justify opening today.

**Sim divergence — flagging for sim-director, not fixing.** `Scripts/sim/` carries no
8-target ceiling and will keep evaluating `v8_heavycloak`=9 and `v6_simplecolumn`=10 as
derived in `retinue-subtypes.json`, unclamped. Now that the shipped data also reflects the
ceiling, the sim harness's model diverges from the shipped game on `targets_per_hit`
specifically for those two candidates: model reads 9/10, game runs 8/8. This doesn't
retroactively change task-088's row count or merge decisions above — those were driven by
overall damage-per-unit gaps across all four axes, not `targets_per_hit` in isolation — but
any future sim-driven ranking that leans on this axis specifically should account for the
ceiling. `docs/sim/**` and `Scripts/sim/**` are sim-director-owned; not touched here.

## What this is NOT deciding

- **Not a balance pass.** These are relative identities lifted from one
  wave-attrition harness that currently predicts a full retinue wipe on every
  scenario it's run against (`docs/sim/LIMITATIONS.md` §1). Real balance needs the
  same zero-input-baseline treatment `GATE1-FUN-PROTOTYPE.md` §3 gave the
  original `spearmen` numbers, once these ship into the actual Mass sim.
- **Not resolving whether `v10_bracedstaff`/`v4_overhead` truly separates in
  play.** That call is made on a 4.8–5.9% wave-attrition gap; it may read as
  identical once stances, positioning, and the real per-entity combat model are
  in play, or it may not. Flagging it rather than pretending the near-miss
  decision above is more certain than it is.

## Assumptions made

- Rounding solo rows to whole numbers and merged rows to the arithmetic mean of
  their pair, then whole numbers, are both my own tuning choices — not specified
  by task-088 or task-094, and not present in the candidate file (which reports
  one-decimal precision throughout).
- No new formation, stance behavior, or move-speed variation was introduced per
  row; only `combat` differs. This wasn't asked for either way — treating it as
  out of scope follows the Mass Entity data-cheap-soldier constraint by default.
- `v9_spearlevel` and `v12_pikereach` are excluded from this mapping because
  task-094 already excluded them from the ten (rejected/flagged verdicts in the
  `knight-melee-v2` manifest) — nothing changes that here.

## Simulation notes

Not simulated further here. This decision reads task-094's completed sim output
(`docs/sim/SUBTYPE-VARIETY.md`'s ten-way section, `docs/data/scenarios/retinue-subtypes.json`)
directly, per task-088's scope fence (`docs/sim/**` and `Scripts/sim/**` are
sim-director-owned; this task does not re-run the harness). The row-count and
merge decisions above are arithmetic on task-094's already-reported per-run
damage-per-unit-committed numbers (percentage gaps recomputed directly from the
published tables in `SUBTYPE-VARIETY.md`, not a new sim run) plus design
judgment on the two flagged disagreements — not a new measurement.

## Canon proposals

None. This stays inside `spearmen`'s existing `melee_line` role — no new stance,
no CLASSES.md identity change, no GDD conflict. It's a stat/identity split within
an already-canon unit type.

## Narrative requests

None. These are stat and combat-identity variants of ten already-designed
`knight-melee-v1`/`v2` silhouettes inside the existing retinue archetype, not new
entities — no new fiction, faction, or biome is implied.
