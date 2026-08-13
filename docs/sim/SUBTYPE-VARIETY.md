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

---

# task-094 — extending to all TEN kept knight silhouettes

`task-087` closed the same day as `task-086` with five more kept
knight-melee-v2 silhouettes (`v7_barestance`, `v8_heavycloak`,
`v10_bracedstaff`, `v11_midguard`, `v13_maceraised`). Owner, 2026-07-30:
*"I would like to see a fair bit of unique knights in the scene. That also
includes their specific spec sheet on the simulation side. Each unique
character should have its own base stats."* Ten judged, kept silhouettes now
exist; this section extends the stat derivation and the comparison to all
ten. **The seven-way section above is left exactly as it was measured and is
NOT edited — it is the frozen record of what task-086 found with five
silhouettes and a 5-sample normalization population.** Everything below uses
`docs/data/scenarios/retinue-subtypes.json` as it stands today.

## What changed in the method, and what didn't

Same formula, verbatim, from the same `derivation_method` block: mass→max_hp,
solidity→targets_per_hit, aspect→engage_range, asymmetry→dps, against the
same spearmen baseline (130 HP / 30 DPS / 95 engage / 8 targets). **The one
thing that necessarily changes is the normalization population.**
`norm(axis, x) = (x - mean) / (max - min)` is relative to whatever population
it's computed against — at five silhouettes that was 5 samples, at ten it is
now 10. This reshuffles the derived stats for the ORIGINAL five candidates
too (e.g. `v1_narrowguard`'s `max_hp` moves from 107.0 at n=5 to 114.5 at
n=10) — not a re-tune, the literal, uniform consequence of the same formula
seeing a larger population. See `retinue-subtypes.json`'s own
`normalization_population_note`.

The two synthetic alt-hypothesis probes from task-086
(`v2_lanceout_alt_sweep`, `v4_overhead_holes_swap`) are kept, unchanged in
method, and now explicitly flagged `"is_probe": true` in the data file.
`Scripts/sim/variety.py --mode subtypes` still fields **all 12** (10 real +
2 probes) in one run — the mechanism is unchanged from task-086 — but
`print_subtype_report()` now prints the **10 real silhouettes in their own
ranked table**, with the 2 probes broken out into a second table underneath,
per task-094's brief: the ten-way comparison must not be polluted by two
synthetic rows.

## Primary result — equal split, both scenarios (10 real silhouettes)

```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1
```
```
Result: retinue_wiped  (elapsed 11.7s)
Retinue: 120 start -> 0.0 survivors
Enemy (unchanged): 250 start -> 20.2 survivors

 # candidate                        hp    dps  engage tgt/hit eff.cleave  count  surv%  dmg/unit shr-of-12 est kills
 1 v8_heavycloak                 164.5   30.0   100.3       9       7.80  10.00   0.0%    180.33    13.1%     30.06
 2 v3_shieldbreak                139.9   35.9    98.9        8       7.59  10.00   0.0%    167.95    12.2%     27.99
 3 v13_maceraised                145.8   36.7    92.6        8       6.65  10.00   0.0%    158.96    11.5%     26.49
 4 v2_lanceout                   133.2   30.1   104.6        7       7.00  10.00   0.0%    122.10     8.8%     20.35
 5 v10_bracedstaff               122.6   32.6   110.3        6       6.00  10.00   0.0%    103.00     7.5%     17.17
 6 v4_overhead                   122.6   26.6   104.7        7       7.00  10.00   0.0%     98.05     7.1%     16.34
 7 v11_midguard                  126.0   29.9    87.5        9       5.94  10.00   0.0%     96.31     7.0%     16.05
 8 v6_simplecolumn               118.5   28.2    81.8       10       5.19  10.00   0.0%     74.30     5.4%     12.38
 9 v7_barestance                 112.5   24.7    86.1        8       5.75  10.00   0.0%     68.23     5.0%     11.37
10 v1_narrowguard                114.5   25.1    83.2        9       5.37  10.00   0.0%     65.94     4.8%     10.99
```

```
py Scripts/sim/variety.py --mode subtypes --scenario floor1-swarm-wave
```
```
Result: retinue_wiped  (elapsed 2.7s)
Retinue: 40 start -> 0.0 survivors  (includes the unchanged Archers row)
Enemy (unchanged): 250 start -> 144.8 survivors

 # candidate                        hp    dps  engage tgt/hit eff.cleave  count  surv%  dmg/unit shr-of-12 est kills
 1 v8_heavycloak                 164.5   30.0   100.3       9       7.80   2.67   0.0%    322.00    12.5%     11.65
 2 v3_shieldbreak                139.9   35.9    98.9        8       7.59   2.67   0.0%    310.40    12.1%     11.23
 3 v13_maceraised                145.8   36.7    92.6        8       6.65   2.67   0.0%    292.65    11.4%     10.59
 4 v2_lanceout                   133.2   30.1   104.6        7       7.00   2.67   0.0%    225.14     8.7%      8.15
 5 v10_bracedstaff               122.6   32.6   110.3        6       6.00   2.67   0.0%    196.39     7.6%      7.11
 6 v4_overhead                   122.6   26.6   104.7        7       7.00   2.67   0.0%    185.54     7.2%      6.72
 7 v11_midguard                  126.0   29.9    87.5        9       5.94   2.67   0.0%    181.25     7.0%      6.56
 8 v6_simplecolumn               118.5   28.2    81.8       10       5.19   2.67   0.0%    142.51     5.5%      5.16
 9 v7_barestance                 112.5   24.7    86.1        8       5.75   2.67   0.0%    131.88     5.1%      4.77
10 v1_narrowguard                114.5   25.1    83.2        9       5.37   2.67   0.0%    127.08     4.9%      4.60
```

**Rank order is byte-identical across both scenarios**: `v8_heavycloak >
v3_shieldbreak > v13_maceraised > v2_lanceout > v10_bracedstaff > v4_overhead
> v11_midguard > v6_simplecolumn > v7_barestance > v1_narrowguard`, with no
exceptions (contrast the seven-way result, which had one rank swap between a
real candidate and its own probe). **The spread from rank 1 to rank 10 is
~2.5-2.7x** (180.33/65.94 = 2.735x on gate1; 322.00/127.08 = 2.534x on
floor1) — down from the seven-way ~3.5x, exactly the compression the
task-094 brief predicted: five more candidates drawn from the same shape-axis
population fill in the middle of the band rather than extending it.

The 2 probes, reported separately (not part of the ten-way ranking):

```
 --- PROBES ---                       hp    dps  engage tgt/hit eff.cleave  count  surv%  dmg/unit shr-of-12
 1 v2_lanceout_alt_sweep (gate1)    133.2   30.1   104.6       9       8.49  10.00   0.0%    148.04    10.7%
 2 v4_overhead_holes_swap (gate1)   104.2   30.6   104.7       7       7.00  10.00   0.0%     95.85     7.0%
 1 v2_lanceout_alt_sweep (floor1)   133.2   30.1   104.6       9       8.49   2.67   0.0%    272.97    10.6%
 2 v4_overhead_holes_swap (floor1)  104.2   30.6   104.7       7       7.00   2.67   0.0%    186.99     7.3%
```

## Robustness — 5 randomized headcount splits, `gate1-calibration-wave1`

```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 1
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 2
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 3
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 4
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --seed 5
```

`damage_per_unit_committed` for the 10 real silhouettes, ranked, one column
per seed (headcount splits vary wildly seed to seed, e.g. `v10_bracedstaff`
gets 0.59 of the 120 total at seed 1 and 11.12 at seed 2):

```
 rank candidate            seed1   seed2   seed3   seed4   seed5
  1   v8_heavycloak       174.97  201.40  183.04  180.43  173.27
  2   v3_shieldbreak      168.08  176.48  169.28  169.42  162.53
  3   v13_maceraised      158.29  165.94  160.50  159.90  153.66
  4   v2_lanceout         122.56  128.88  122.80  123.64  118.23
  5   v10_bracedstaff     103.86  109.16  103.36  105.36   99.67
  6   v4_overhead          98.86  103.91   98.39  100.30   94.88
  7   v11_midguard         96.65  101.76   96.65   97.97   93.22
  8   v6_simplecolumn      75.30   78.97   74.56   76.47   71.89
  9   v7_barestance        69.53   72.67   68.40   70.72   65.93
 10   v1_narrowguard       67.11   70.22   66.15   68.23   63.76
```

**The rank order is identical across all 5 seeds AND matches both
deterministic equal-split runs exactly — zero exceptions, in every one of
the 7 runs measured (2 scenarios + 5 seeds).** This is a stronger robustness
result than the seven-way one, which had a single rank swap (between
`v4_overhead` and its own probe `v4_overhead_holes_swap`). That swap still
happens at ten — see below — but because probes are now reported in their
own table, it no longer touches the ten-way real-silhouette ranking at all.

Reproducibility check (byte-identical on repeat):
```
py Scripts/sim/variety.py --mode subtypes --scenario gate1-calibration-wave1 --json
```
run twice, output identical.

## Answering the three questions task-094 asked

**1. Does the ten-way spread still separate, or compress into a band inside
the noise?** It separates, but it compresses. ~2.5-2.7x rank-1-to-rank-10
spread (down from ~3.5x at seven), and unlike the seven-way result the
ranking is now **densely packed with several adjacent pairs closer than 3%
apart** — see question 2. The ten-way spread is real but noticeably tighter,
exactly because five more candidates were drawn from the same four-axis
shape population rather than a wider one.

**2. Which pairs are indistinguishable — named, numerically.** Two pairs,
consistently, across every one of the 7 measured runs (both scenarios, all 5
seeds), never once swapping order but by a margin small enough that shipping
both as distinct stat blocks would not be a difference a player can feel:

- **`v4_overhead` and `v11_midguard`** (ranks 6/7): gap = 1.8% (gate1 equal
  split), 2.3% (floor1 equal split), 1.8-2.3% across all 5 seeds. **The
  tightest pair in the whole ten-way comparison.**
- **`v7_barestance` and `v1_narrowguard`** (ranks 9/10): gap = 3.4% (gate1),
  3.6% (floor1), 3.3-3.5% across all 5 seeds. **Second-tightest, also
  consistent.**

A third pair is borderline and worth naming, though the gap is
noticeably wider and this call is softer than the two above:
**`v10_bracedstaff` and `v4_overhead`** (ranks 5/6) sit 4.8-5.9% apart across
all 7 runs — small and remarkably stable, but roughly double the gap of the
two pairs above, so this is reported as a near-miss, not a confident
indistinguishable call.

Every other adjacent pair in the ranking is separated by 5.4-23.2%
(`v13_maceraised`→`v2_lanceout` and `v11_midguard`→`v6_simplecolumn` are the
two widest jumps, both >21% in every run) — clearly separated, not noise.

Separately, the two **probes** (not part of the ten-way ranking, but a
useful cross-check): `v4_overhead` and its own probe `v4_overhead_holes_swap`
swap relative order seed-to-seed against `v11_midguard` (seed 1: `v4_overhead`
98.86 > `v4_overhead_holes_swap` 97.13 > `v11_midguard` 96.65; seed 2:
`v4_overhead` 103.91 > `v11_midguard` 101.76 > `v4_overhead_holes_swap`
101.01) — the same "budget-neutral hp/dps trade is genuinely tied" finding
task-086 reported at seven, still holding at ten, now correctly isolated to
the probes table rather than contaminating the real ranking.

**3. Does the rank order hold across both scenarios and all 5 seeds the way
it did at seven?** Yes, and more completely: **zero rank swaps** among the
10 real silhouettes across all 7 measured runs (at seven, there was one swap,
and it was between a real candidate and its own synthetic probe — see
question 2's probe note, which reproduces at ten too but no longer pollutes
this table). The rank order compressing (question 1) did NOT make it
unstable — it stayed exactly as ordered every single time, just with several
adjacent gaps small enough to be a difference nobody would notice in play.

## The caveat, stated inline (not a footnote)

Same as the seven-way section above, unchanged: every number in every table
here is a **WAVE-ATTRITION** result. `docs/sim/LIMITATIONS.md` §1 states this
harness's wave-attrition model does **not** reproduce
`GATE1-FUN-PROTOTYPE.md`'s measured 109-111-of-120 wave-1 survival at the
harness's own committed defaults — it predicts a full wipe there, and
(consistent with that) every run in this section also ends `retinue_wiped`
(see the `validate.py` run below — check 3 fails exactly as before, this
task changed nothing about that). **These tables rank the 10 real silhouette
candidates RELATIVE TO EACH OTHER inside one shared, imperfect model — never
an absolute claim about how any of them would feel in the real game.**

## Assumptions made in this extension

- The two synthetic probes (`v2_lanceout_alt_sweep`, `v4_overhead_holes_swap`)
  were NOT re-derived against a new alt-hypothesis for any of the five new
  v2 silhouettes. Nobody asked for that, and inventing new probes unasked
  would be exactly the kind of formula-tuning-by-another-name task-094's
  brief forbids. `v13_maceraised` has `holes=1` (like `v4_overhead`'s `2`)
  and gets no equivalent swap candidate for the same reason.
- `holes` stays out of the normalized 4-axis formula. At n=10 there are now
  two nonzero `holes` samples instead of one, which is technically enough to
  define a normalizable range — but adding a 5th axis term now, only because
  population growth happened to make it arithmetically possible, would be
  inventing a better formula mid-task. See
  `retinue-subtypes.json`'s `holes_excluded_from_the_normalized_formula`.
- `v9_spearlevel` (rejected — its measured band sits fully inside
  `v10_bracedstaff`'s) and `v12_pikereach` (flagged — aspect target outside
  the measured rotation band) are excluded, per
  `docs/data/art/families/knight-melee-v2/manifest.json`'s own verdicts.
  Only `keep`-verdict silhouettes get a stat candidate, same rule task-086
  applied to `knight-melee-v1`'s `v5_widecross` (flagged, excluded).

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
All shown in full above. Files touched: `docs/data/scenarios/retinue-subtypes.json`
(five new candidates added, existing five recomputed against the n=10
normalization population, two probes flagged `is_probe`),
`Scripts/sim/variety.py` (`run_subtypes()` threads `is_probe` through the
ranked-candidate dict, `print_subtype_report()` splits real silhouettes and
probes into two tables), and this file. `Scripts/sim/data_loader.py`,
`combat_model.py`, `scenario_runner.py`, `validate.py`,
`docs/data/scenarios/combat-model-constants.json`, every existing scenario
file, and `docs/data/unit-types.json`/`docs/design/**`/`SYSTEMS.md` are all
untouched, per task-094's scope fence. No `retinue-subtypes-v2.json` file was
created — `data_loader.load_retinue_subtypes()` reads exactly one file, and
that file is off limits to edit for this task, so the five new candidates
were added directly to the existing `retinue-subtypes.json` instead of
requiring an unowned second data-loader read path.
