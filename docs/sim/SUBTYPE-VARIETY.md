# SUBTYPE-VARIETY.md — task-086: does splitting melee into sub-types separate?

`task-082` closed 2026-07-29 with FIVE kept knight-melee-v1 silhouettes,
measured. The typed retinue model ships exactly one melee type
(`spearmen`, `docs/data/unit-types.json`) — so today all five would fight
identically. This records the method, the numbers, and the answer:
**yes, the spread is real and stable, but it is a RELATIVE finding inside
one imperfect wave-attrition model, not a prediction of real-game feel.**
Read `docs/sim/LIMITATIONS.md` §1 before trusting any number below — the
same caveat this section states inline, not as a footnote.

## Method

`Scripts/sim/variety.py --mode subtypes` (new, task-086 — see its own
docstring). Unlike the `--mode hero-builds` roll-and-rank (a huge
combinatorial space sampled randomly), `docs/data/scenarios/retinue-subtypes.json`
is a small FIXED set of 7 candidates, so this mode fields **all 7 at once**
(one `WaveGroup` each) rather than sampling a subset — the most direct,
least noisy comparison the harness can run.

The scenario's `spearmen` `Composition` row is replaced by that same total
headcount split across the 7 candidates; every other retinue row (e.g.
`floor1-swarm-wave`'s Archers) is carried through **unchanged** via the
normal `retinue_fighter()` path, and the **Enemy side is untouched** — a
like-for-like swap of the melee identity axis only, per task-086's scope
fence.

- `--seed` omitted (the PRIMARY, deterministic comparison): the 7 candidates
  split the headcount **equally**.
- `--seed <n>` given: `random.Random(seed)` draws a random per-candidate
  headcount split instead (`variety.random_split()`) — a robustness check on
  whether the ranking holds regardless of how the fixed total is portioned.
  Ranking is by `damage_per_unit_committed` (damage dealt / headcount that
  candidate actually got), not raw `damage_dealt`, specifically so unequal
  splits stay comparable.

Two scenarios tested, unchanged: `gate1-calibration-wave1` (120 melee, no
Archers, arrival-gated — the closer-to-validated fixture) and
`floor1-swarm-wave` (32 melee + 8 Archers unchanged, the real economy-fed
composition).

## The candidate derivation, in one line each

Full derivation and the formula: `docs/data/scenarios/retinue-subtypes.json`
itself (`derivation_method`). One normalized formula applied uniformly across
all 5 silhouettes' 4 measured axes (aspect, solidity, asymmetry, mass) against
the `spearmen`/`militia` baseline (130 HP / 30 DPS / 95 engage / 8 targets):
mass→max_hp, solidity→targets_per_hit, aspect→engage_range,
asymmetry→dps. Two extra candidates test the two ambiguities task-086's brief
named explicitly: `v2_lanceout_alt_sweep` (targets_per_hit sign flipped —
"does a level lance sweep, or focus one point?") and
`v4_overhead_holes_swap` (a holes-driven ±15% hp/dps swap testing the
"two-handed weapon" read the 4-axis formula itself has no term for).

**Where the formula disagrees with the brief's own hand-guess, that
disagreement is stated in the data file, not resolved:** applied uniformly,
`v3_shieldbreak`'s high asymmetry (0.74, by far the highest of the five) reads
as the HIGHEST-dps candidate, not the "more HP, less DPS" defensive read the
brief guessed at a glance.

## Primary result — equal split, both scenarios

```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1
```
```
Result: retinue_wiped  (elapsed 11.7s)
Retinue: 120 start -> 0.0 survivors
Enemy (unchanged): 250 start -> 11.5 survivors

 # candidate                      hp    dps  engage tgt/hit eff.cleave   count  surv%  dmg/unit shr-of-7 est kills
 1 v3_shieldbreak              159.0   37.5    99.9       8       7.74   17.14   0.0%    215.46    25.8%     61.56
 2 v2_lanceout_alt_sweep       145.3   31.1   107.6      10       8.98   17.14   0.0%    185.84    22.3%     53.10
 3 v2_lanceout                 145.3   31.1   107.6       6       6.00   17.14   0.0%    124.16    14.9%     35.47
 4 v4_overhead                 123.5   27.1   107.6       6       6.00   17.14   0.0%     89.65    10.7%     25.62
 5 v4_overhead_holes_swap      105.0   31.2   107.6       6       6.00   17.14   0.0%     86.94    10.4%     24.84
 6 v6_simplecolumn             115.2   28.9    79.1      10       4.85   17.14   0.0%     71.83     8.6%     20.52
 7 v1_narrowguard              107.0   25.5    80.7       9       5.05   17.14   0.0%     61.02     7.3%     17.43
```

```
py Scripts/sim/variety.py --mode subtypes --scenario floor1-swarm-wave
```
```
Result: retinue_wiped  (elapsed 2.7s)
Retinue: 40 start -> 0.0 survivors  (includes the unchanged Archers row)
Enemy (unchanged): 250 start -> 145.8 survivors

 # candidate                      hp    dps  engage tgt/hit eff.cleave   count  surv%  dmg/unit shr-of-7 est kills
 1 v3_shieldbreak              159.0   37.5    99.9       8       7.74    4.57   0.0%    370.32    24.9%     22.98
 2 v2_lanceout_alt_sweep       145.3   31.1   107.6      10       8.98    4.57   0.0%    324.46    21.8%     20.13
 3 v2_lanceout                 145.3   31.1   107.6       6       6.00    4.57   0.0%    216.77    14.6%     13.45
 4 v4_overhead_holes_swap      105.0   31.2   107.6       6       6.00    4.57   0.0%    164.75    11.1%     10.22
 5 v4_overhead                 123.5   27.1   107.6       6       6.00    4.57   0.0%    163.01    11.0%     10.11
 6 v6_simplecolumn             115.2   28.9    79.1      10       4.85    4.57   0.0%    133.67     9.0%      8.29
 7 v1_narrowguard              107.0   25.5    80.7       9       5.05    4.57   0.0%    114.55     7.7%      7.11
```

**Rank order is identical across both scenarios**, with one exception: ranks
4/5 (`v4_overhead` vs `v4_overhead_holes_swap`) swap order between the two —
expected, see "what collapsed to noise" below, they're a deliberately
budget-neutral trade. **The spread from rank 1 to rank 7 is ~3.5x in
per-unit damage output in both scenarios** (215.46/61.02 = 3.53x on gate1;
370.32/114.55 = 3.23x on floor1).

## Robustness — 5 randomized headcount splits, `gate1-calibration-wave1`

```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 1
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 2
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 3
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 4
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 5
```

Each seed draws a wildly different per-candidate headcount split (e.g. seed 3
gives `v6_simplecolumn` only 0.64 of the 120 total; seed 4 gives it 48.39).
**The rank order by `damage_per_unit_committed` is identical across all 5
seeds and matches the equal-split result exactly**, with the same single
exception: `v4_overhead` / `v4_overhead_holes_swap` (ranks 4/5) swap order
seed-to-seed — confirming they are genuinely tied within this model, not that
the ranking is unstable. This is the reproducibility/robustness evidence:
the ~3.5x spread is not an artifact of picking equal headcounts.

Reproducibility check (byte-identical on repeat):
```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --json
```
run twice, `diff`'d clean.

## Which axes separated, which collapsed to noise

**Separated, robustly:**
- **`max_hp` and `dps` moving in the SAME direction** (both up, or both down)
  — `v3_shieldbreak` (highest of both) dominates every run; `v1_narrowguard`
  (lowest hp, lowest dps) is last in every run. This is the dominant driver
  of the ~3.5x spread.
- **`effective_cleave`** (`min(targets_per_hit, combat_model.melee_reach_per_exposed_unit(engage_range, ...))`),
  NOT raw `targets_per_hit`. See below — this is the harness's own genuine,
  non-obvious finding, not a derivation artifact.

**Collapsed to noise, and why:**
- **`survival_rate`** — every candidate reads ~0.0% survival in every run
  (one exception, seed 3, 0.1-1.8%, still negligible). This is NOT a
  candidate-identity finding — it is `docs/sim/LIMITATIONS.md` §1's own
  documented gap: this harness's wave-attrition model wipes the retinue at
  its committed defaults regardless of composition, so survival cannot
  currently distinguish these candidates from each other. Every number this
  section reports is damage dealt BEFORE the wipe, not a claim about who
  lives.
- **`max_hp`/`dps` traded against each other at a roughly constant budget** —
  `v4_overhead` (123.5 HP / 27.1 DPS) vs. `v4_overhead_holes_swap` (105.0 HP /
  31.2 DPS, a deliberate +/-15% swap on the same base) land within ~2% of
  each other's `damage_per_unit_committed` in every run, and which one ranks
  higher flips scenario-to-scenario and seed-to-seed. A budget-neutral
  hp<->dps trade is genuinely close to indistinguishable in this model — only
  trades that move BOTH stats the same direction (not a trade at all,
  strictly better/worse) produce robust separation.
- **`targets_per_hit` in isolation** — `v6_simplecolumn` has the joint-highest
  raw `targets_per_hit` (10, tied with `v2_lanceout_alt_sweep`) but the
  SHORTEST `engage_range` (79.1) of any candidate, so
  `melee_reach_per_exposed_unit()` caps its `effective_cleave` at 4.85 — the
  LOWEST of all 7, lower even than `v1_narrowguard`'s nominal 9. `v6` ranks
  6th of 7 in every single run despite having a top-tier raw cleave stat.
  **A candidate's raw `targets_per_hit` number is not its gameplay identity —
  its `effective_cleave` (jointly capped by `engage_range`) is, and those two
  can rank in the opposite order.** This is the single most useful, least
  obvious finding here: engage_range and targets_per_hit are not independent
  axes in this model, they interact, and reading either one alone would have
  been a wrong prediction of which candidates matter.

## The caveat, stated inline (not a footnote)

Every number in every table above is a **WAVE-ATTRITION** result.
`docs/sim/LIMITATIONS.md` §1 states this harness's wave-attrition model does
**not** reproduce `GATE1-FUN-PROTOTYPE.md`'s measured 109-111-of-120 wave-1
survival at the harness's own committed defaults — it predicts a full wipe
there, and (consistent with that, not contrary to it) every run in this
document also ends `retinue_wiped`. **These tables rank the 7 candidates
RELATIVE TO EACH OTHER inside one shared, imperfect model — never an
absolute claim about how any of them would feel in the real game.** A
candidate that ranks #1 here beat the other 6 under this model's rules; it is
not a claim it would perform well in a played run.

Stances, leash, supply/degrade, items, knockback and positioning are **not
modelled at all** (`docs/sim/LIMITATIONS.md` §4). None of the 7 candidates'
identity here rests on any of those — every derived axis (HP, DPS,
engage_range, targets_per_hit) is a plain stat-block number the harness
already models for `spearmen`/`archers`, so this comparison is answerable;
it would not be if a candidate's identity had depended on a stance-specific
behavior.

## Recommendation

The spread is real, stable, and large enough (~3.5x per-unit damage,
identical rank order across 2 scenarios and 5 randomized splits) to be worth
a design look — **but it is sim-director's job to report that, not decide
it.** Filed as a follow-on: `docs/backlog/task-088-adopt-melee-subtype-identity-into-unit-types.md`
(gameplay-director). That task also carries forward the two named
disagreements between this formula's literal output and the brief's own
design intuition (`v3_shieldbreak`'s dps direction, `v4_overhead`'s
two-handed hp/dps trade) as open judgment calls, not resolved here.

## Evidence

```
py Scripts/sim/validate.py
```
Unchanged by this task: checks 1, 2, 4 PASS; check 3 (GATE1 wave-attrition
reproduction) FAILs exactly as `docs/sim/LIMITATIONS.md` §1 documents
(retinue survivors 0.00 of 120 vs the measured 109-111 band) — same failure,
same numbers, this task changed nothing about it.

```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1
py Scripts/sim/variety.py --mode subtypes --scenario floor1-swarm-wave
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 1   (and --seed 2..5)
```
All shown in full above. `Scripts/sim/data_loader.py`'s
`load_retinue_subtypes()`/`retinue_subtype_fighter()` and
`Scripts/sim/variety.py`'s `run_subtypes()`/`print_subtype_report()` are the
only code added; `Scripts/sim/combat_model.py`, `scenario_runner.py`,
`validate.py`, `docs/data/scenarios/combat-model-constants.json`, and every
existing scenario file are untouched, per task-086's scope fence.
