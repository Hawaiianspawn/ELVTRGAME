# Does the hero-build space actually differentiate builds? (task-091)

**Verdict: SCENARIO-DEPENDENT, with a residual data-driven skew underneath
that the swarm scenario amplifies from mild to severe.** Neither of the two
explanations task-091 opened with is the whole story on its own — the
measurement below separates them and finds a third factor neither named.

`Scripts/sim/differentiation.py` is a thin driver over `variety.py` (WAVE
side, `variety.run()` unmodified) plus a small point-target counterpart built
from the same `sample_roster()`/`evaluate_synergy_rules()` functions
(`variety.py` itself only runs `Kind='wave_attrition'` scenarios and is not
touched here — task-090 owns it). 25 seeds (1-25), `--roster 20
--count-per-build 2` throughout (variety.py's own defaults, ~40 total rolled
retinue headcount, matching `floor1-swarm-wave`'s real retinue size).

```powershell
py Scripts/sim/differentiation.py --selftest   # same seed -> identical roster, both scenario kinds
py Scripts/sim/differentiation.py --seeds 25   # the full measurement below
py Scripts/sim/differentiation.py --seeds 25 --json   # per-seed, per-build detail
```

## The numbers, side by side

"Competitive with rank 1" bands: **0.5x** (within 2x of rank 1's own
damage/dps — the loosest band that still means something) and **0.8x**
(within 25% — a stricter check reported alongside it, so no single arbitrary
cutoff is doing all the work). Both computed identically for wave (on
`damage_dealt`) and point-target (on `group_dps`).

| | **floor1-swarm-wave** (WAVE-ATTRITION, NOT VALIDATED — see caveat below) | **floor2-elite-point-target** (VALIDATED) | **floor3-boss-point-target** (VALIDATED) |
|---|---|---|---|
| avg rank-1 share of roster damage | **31.3%** | 10.3% | 9.9% |
| range across 25 seeds | 17.8% - 42.1% | 8.5% - 11.9% | 7.9% - 12.5% |
| avg builds competitive @0.5x (of 20) | **2.64** | 9.08 | 9.72 |
| avg builds competitive @0.8x (of 20) | 1.88 | 2.08 | 2.24 |
| rank-1 weapon frequency (25 seeds) | `cleave_melee_sweep` 25/25 | `cleave_melee_sweep` 22/25, `siege_artillery` 3/25 | `cleave_melee_sweep` 13/25, `siege_artillery` 12/25 |

Reproduces the lead's own seed-3/seed-7 numbers exactly on this same
25-seed run: seed 7 rank 1 = `Beastcaller`/`Cleave Melee Sweep` at 41.9%;
seed 3 ranks 1+2 both `Cleave Melee Sweep` (`Line Breaker` 32.8% +
`Beastcaller` 28.1% = 60.9% of the whole roster). Full per-seed table in
`--json` output.

## What this says about the two competing explanations

**(2), the scenario explanation, is real and does most of the work.** Rank-1
share triples going from point-target to wave (≈10% → 31%), and the number
of builds within 2x of the top build drops by more than 3x (≈9.4 → 2.64).
That is exactly the shape explanation (2) predicted: a 250-enemy swarm
rewards `targets_per_shot` (cleave hits up to 8 simultaneous enemies, every
other weapon in the roster caps at 1-4) in a way a single point-target
mechanically cannot — **verified directly in `combat_model.py`**:
`steady_state_dps()` (the only function `army_ttk_vs_point_target()` calls
per group) never reads `targets_per_hit` at all. Cleave's namesake mechanic
is *completely inert* in the point-target model. Its rank-1 share there is
not a cleave effect.

**But (1) isn't fully clean either — there's a residual, scenario-independent
skew this measurement surfaces that the framing didn't name.**
`cleave_melee_sweep` still takes rank 1 more often than any single
competitor in BOTH point-target scenarios (22/25 and 13/25), where its
cleave mechanic contributes nothing.

**Correction to an earlier draft of this doc:** that draft attributed this to
`cleave_melee_sweep` being uniquely legal on 2 chassis. Checked by hand
against `hero-builds.json` and that claim was wrong — cleave is mid-pack on
legality breadth, not an outlier:

| weapon | legal on N chassis |
|---|---|
| `rapid_skirmish_blaster` | 3 |
| `chain_bounce_shot` | 3 |
| `arcing_aoe_lobber` | 2 |
| `beam_continuous` | 2 |
| `cleave_melee_sweep` | 2 |
| `dual_strike_melee` | 2 |
| `precision_longbow` | 1 |
| `turret_autocannon` | 1 |
| `siege_artillery` | 1 |

The real driver is an **armor gate**, and it's a much bigger effect than any
chassis-legality count. Blow ≈ `damage_per_shot x accuracy` (`rate_of_fire`
cancels out of `blow x rate_of_fire = dps x swing_interval`); effective blow
is `max(blow - armor, chip_floor)` (`chip_floor` = 3.0,
`armor_chip_floor()`). Against `brood_elite`'s Armor 12 / `brood_boss`'s
Armor 14:

| weapon | blow | clears Elite (12)? | clears Boss (14)? |
|---|---|---|---|
| `siege_artillery` | 154.0 | yes (142) | yes (140) |
| `arcing_aoe_lobber` | 44.0 | yes (32) | yes (30) |
| `cleave_melee_sweep` | 27.0 | yes (15) | yes (13) |
| `precision_longbow` | 16.2 | barely (4.2) | **no — floors (2.2 < 3.0)** |
| `chain_bounce_shot` | 11.9 | no — floors | no — floors |
| `dual_strike_melee` | 9.0 | no — floors | no — floors |
| `turret_autocannon` | 7.2 | no — floors | no — floors |
| `beam_continuous` | 6.0 | no — floors | no — floors |
| `rapid_skirmish_blaster` | 4.2 | no — floors | no — floors |

**Only 3 of the 9 reachable weapon archetypes clear armor meaningfully against
either target — `siege_artillery`, `arcing_aoe_lobber`, `cleave_melee_sweep`
— and 6 of 9 (7 of all 10, counting the un-rollable dead-data
`shotgun_spread`) sit at or effectively at `chip_floor` regardless of how
often they're drawn.** `rapid_skirmish_blaster` and `chain_bounce_shot` are
the two MOST-drawn weapons in the whole table (18.75% each, legal on 3
chassis apiece — the widest legality of any weapon) and neither has ever
taken rank 1 in either point-target scenario across these 25 seeds: **wide
legality is worth nothing once a weapon can't clear armor.** This is the
headline finding, not chassis count, and it's scenario-independent (it's a
property of the weapon table against these two targets' Armor values, not of
which scenario is run).

Among the 3 armor-clearing survivors, per-unit dps ranks `siege_artillery`
(21.3 vs Elite, 21.0 vs Boss) > `cleave_melee_sweep` (16.67 / 14.44) >
`arcing_aoe_lobber` (12.8 / 12.0 — its huge blow is undercut by the
slowest rate_of_fire in the roster, 0.4). Draw probability (chassis picked
uniformly 1/8, then a weapon uniformly from that chassis's own
`legal_weapons` list) inverts that ranking: `cleave_melee_sweep` 12.5%
(`line_breaker` 1/16 + `beastcaller` 1/16) > `arcing_aoe_lobber` 10.4%
(`archer` 1/24 + `siege_artillerist` 1/16) > `siege_artillery` 6.25%
(`siege_artillerist` 1/16 only) — cleave is the MOST-drawn of the three
weapons that actually clear armor, not because of any legality-count
extreme (`arcing_aoe_lobber` ties it at 2 chassis) but because both of its
chassis happen to be 2-weapon chassis rather than `archer`'s 3-weapon one.
`arcing_aoe_lobber` never wins rank 1 despite decent frequency and clearing
armor, because its slow cadence caps its dps below both rivals.

`piercing_rounds` (`armor_penetration_flat` 4) is available on BOTH of
`cleave_melee_sweep`'s legal chassis (`line_breaker`, `beastcaller`) and
closes most of the remaining gap to `siege_artillery`: with it rolled,
cleave's Elite dps rises from 16.67 to ~21.1 (`docs/sim/VARIETY.md`'s own
measured +26.7% at Armor 12), essentially matching siege's 21.3 baseline.
`siege_artillerist`'s own modification pool (`focus_optic`,
`reinforced_plating`) has no damage or armor-penetration option at all —
siege can never close that gap the way a lucky cleave roll can.

`siege_artillery` only overtakes `cleave_melee_sweep` in rank-1 frequency
when target armor rises from Elite's 12 to Boss's 14 (3/25 -> 12/25): the
extra 2 armor points cost `cleave_melee_sweep` proportionally more of its
smaller blow (even with piercing: 21.1 -> 18.9) than they cost
`siege_artillery`'s enormous one (21.3 -> 21.0, barely moved). That is the
harness behaving correctly, not a bug — exactly the "big single hits are
less armor-sensitive than many small hits" relationship the design already
encodes; it just isn't enough to overturn cleave's frequency edge until
armor climbs further.

**Net read:** in the wave scenario, cleave's data-level edge (one of only 3
armor-clearing weapons, the most-drawn of those 3, with an available
armor-pen modification) COMPOUNDS with a real scenario mechanic
(targets_per_shot 8 vs. 250 enemies, inert in point-target) to produce the
reported ~31% rank-1 share and a genuinely thin competitive band (2.64/20).
In the point-target scenarios, only the data-level edge survives, and it
produces a much milder effect: rank 1 still wins somewhat more than its
"fair share" (10% vs. a ~5% uniform baseline for 20 builds) but roughly half
the roster (9-9.7 of 20) stays within 2x of it — real differentiation, not a
monoculture, in that scenario shape.

## Caveat specific to this measurement — read before trusting the point-target absolute numbers

`army_ttk_vs_point_target()`'s melee surround-cap (`SurroundCapEstimate`, 20
for Elite / 45 for Boss) is applied **per `ArmyGroup`**, not pooled across
all melee groups on a side (`engaged = min(g.count, cap)` per group — see
`combat_model.py`). The model's normal callers (`scenario_runner.py`) only
ever build ONE melee `ArmyGroup` (all Spearmen together), so this never
matters there. This measurement instead splits the melee roster into up to
20 separate 2-count `ArmyGroup`s (one per rolled build) — every one of them
sits at `count=2 << cap`, so **the surround cap never binds for any build
here**, unlike the scenario's own intended reading where the whole melee
block saturates well below its headcount. This does not distort the
RELATIVE ranking this doc reports (every melee build is capped-or-not the
same way, uniformly), but it means the point-target `total_group_dps` /
implied TTK figures in a raw `--json` dump of this tool run HOTTER than a
real "40 melee units vs. one Elite" fight would be — do not quote a TTK out
of this tool's point-target mode as a scenario TTK; use
`scenario_runner.py`'s own point-target scenarios for that (those keep the
retinue as intended, unsplit, cap-respecting groups). Not a
`combat_model.py` bug — that function's cap-per-group behavior is correct
for its one documented use pattern; it is this tool's own roster-splitting
that puts it outside that pattern. Flagged rather than worked around,
per this task's own instruction not to touch `combat_model.py`.

## Wave-attrition trust caveat

Every wave-attrition number above is a **RELATIVE** comparison inside one
model run, per `docs/sim/LIMITATIONS.md` §1: that model does not reproduce
`GATE1-FUN-PROTOTYPE.md`'s measured ~110-of-120 wave-1 survival at the
harness's committed defaults (every roll here also ends `retinue_wiped`, same
as every prior `variety.py` run — see `docs/sim/VARIETY.md`'s own evidence
section). The 31.3% rank-1 share / 2.64-of-20 competitive-band numbers are
trustworthy as *rankings between builds fighting the same imperfect model*,
not as an absolute claim about what share of damage a build would deal in a
played run. Point-target numbers are VALIDATED (`docs/sim/VALIDATION.md`
reproduces `entity-tiers.md` §7's table exactly) and carry no such caveat,
subject to the surround-cap-per-group note above.

## What this means for "characters will be unique in the game"

Reading the two model shapes as proxies for two real gameplay moments:

- **Fighting a single big target (Elite/Titan/Boss)** — the shape
  `floor2-elite-point-target`/`floor3-boss-point-target` stand in for — the
  build space already differentiates reasonably well: no single build eats
  more than ~12% of the roster's damage on average, and about half the
  roster stays within 2x of whichever build tops that fight. This is not a
  monoculture.
- **Fighting a swarm** — the shape `floor1-swarm-wave` stands in for — one
  weapon (`cleave_melee_sweep`) is winning by a wide and growing margin (up
  to 60.9% of a roster's ENTIRE damage output split across just its top two
  builds on seed 3), and only 2-3 of 20 rolled builds stay competitive.
  Whether that reads as "differentiated" or "monoculture" depends entirely
  on how much of the game is actually fought as a swarm fight versus a
  point-target fight — a question this harness cannot answer (it's a
  scenario-mix / level-design question, not a combat-model question).

This is not a clean monoculture verdict, so no single data change is being
named as a fix per task-091's own instruction (that applies only to a
monoculture verdict). The one data-level observation worth flagging to the
gameplay director, without prescribing a number: **6 of the 9 reachable
weapon archetypes (7 of all 10, counting the un-rollable `shotgun_spread`)
sit at or effectively at `chip_floor` against `brood_elite`/`brood_boss`
armor — only `siege_artillery`, `arcing_aoe_lobber` and `cleave_melee_sweep`
clear it meaningfully.** That armor gate, not chassis-legality count
(`cleave_melee_sweep` is mid-pack there — 5 other weapons are legal on as
many or more chassis), is the scenario-independent structural fact behind
`cleave_melee_sweep`'s point-target rank-1 rate: it's simply the most-drawn
of the three weapons that survive the gate (12.5% vs. `arcing_aoe_lobber`'s
10.4% and `siege_artillery`'s 6.25%), and the only one of the three with an
armor-penetration modification (`piercing_rounds`, on both its legal
chassis) available to close the remaining power gap to `siege_artillery`.
That (not `targets_per_shot`, which is inert against a single target) is
what makes `cleave_melee_sweep` outperform even where its cleave mechanic
does nothing, and it would still be present even if the swarm scenario's
concentration problem were fixed some other way.
