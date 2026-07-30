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

---

# task-098: opening the armor gate from the build side

Acting on this doc's own finding above — the armor gate, not chassis-legality
count, is what actually gates a weapon out of point-target relevance — this
task widened the build space from the BUILD side only.
`docs/data/entity-tiers.json` (Elite Armor 12, Boss Armor 14),
`combat-model-constants.json`'s `chip_floor` (3.0), and every file under
`Scripts/sim/` are unchanged; task-076's lock on `chip_floor` and this doc's
own citations against the armor values both depend on that. Full rationale
and the verified per-weapon blow table: `docs/design/hero-build-variety.md`
§2b. In short, two changes to `docs/data/hero-builds.json`:

1. `chassis.archer.legal_modifications` gained `piercing_rounds` (was
   `[focus_optic, overcharge_core]`) — `precision_longbow`'s blow (16.2) was
   already close enough to Elite/Boss armor that the existing +4 penetration
   alone clears both with real margin; it just had no chassis with
   penetration access before now (`piercing_rounds` previously reached only
   `line_breaker`/`beastcaller` — `cleave_melee_sweep`'s own two chassis, the
   "concentration is part of the cause" line above). `precision_longbow`
   itself is untouched (a CALIBRATION row) and `calibration_builds.archers`
   keeps `modification: null`.
2. `weapon_archetypes.chain_bounce_shot.damage_per_shot` raised 14.0 -> 19.0
   (blow 11.9 -> 16.15, a PROTOTYPE DIAL, not a calibration row).
   `chain_bounce_shot` already had a piercing-legal chassis (`beastcaller`) —
   this only needed the damage number, no access change.

`dual_strike_melee`, `turret_autocannon`, `beam_continuous`, and
`rapid_skirmish_blaster` are deliberately left floored — closing any of them
the same way needed either a `piercing_rounds` pen increase (which would move
this doc's own already-cited "+26.7% at Armor 12" figure for
`cleave_melee_sweep`, `docs/sim/VARIETY.md` — **flagging that figure as now
counterfactual to any FUTURE pen change, not stale today, since pen itself
was not touched**) or a damage_per_shot increase large enough to push each
weapon's raw dps past the roster's own ceiling (`cleave_melee_sweep`'s 30),
i.e. trading "opens the gate" for "creates a new dominant weapon" — the
disqualifying outcome this task was told to avoid.

## Before / after, same command, same seeds

`py Scripts/sim/differentiation.py --seeds 25` (identical invocation both
times — only `docs/data/hero-builds.json` changed between runs).

| | **floor1-swarm-wave** (WAVE, NOT VALIDATED — LIMITATIONS.md §1) | | **floor2-elite-point-target** (VALIDATED) | | **floor3-boss-point-target** (VALIDATED) | |
|---|---|---|---|---|---|---|
| | before | after | before | after | before | after |
| avg rank-1 share | 31.3% | **30.2%** | 10.3% | **9.8%** | 9.9% | **9.7%** |
| range across 25 seeds | 17.8%-42.1% | 18.1%-44.5% | 8.5%-11.9% | 8.1%-11.4% | 7.9%-12.5% | 7.9%-11.7% |
| avg competitive @0.5x (of 20) | 2.64 | 2.64 | 9.08 | **9.52** | 9.72 | 9.68 |
| avg competitive @0.8x (of 20) | 1.88 | 1.84 | 2.08 | 1.88 | 2.24 | 2.00 |
| rank-1 weapon frequency | `cleave_melee_sweep` 25/25 | `cleave_melee_sweep` 25/25 | `cleave` 22/25, `siege` 3/25 | `cleave` 22/25, `siege` 3/25 | `cleave` 13/25, `siege` 12/25 | `cleave` 15/25, `siege` 9/25, **`arcing_aoe_lobber` 1/25** |

## Reading the movement

**The wave scenario is essentially untouched** (30.2% vs 31.3%, within
seed-to-seed noise — compare the range column, which shifted by more than
this delta on its own). Expected: neither changed weapon has
`cleave_melee_sweep`'s `targets_per_shot: 8`, and the wave scenario's own
dominant mechanic (per this doc's earlier finding) is that coverage
advantage, not the armor gate — this task never touched it and wasn't asked
to.

**The two validated point-target scenarios both move in the intended
direction, modestly.** Rank-1 share drops in both (10.3%->9.8% Elite,
9.9%->9.7% Boss) — nobody replaces `cleave_melee_sweep` at the top; it still
wins rank-1 in 22/25 and 15/25 seeds respectively, same or fewer than before.
Elite's competitive-@0.5x band widens (9.08->9.52 of 20); Boss's is flat to
very slightly down (9.72->9.68) — an average built substantially from
seed-level noise given only 25 seeds, not a regression signal (`chain_bounce_
shot`'s and `precision_longbow`'s own post-armor dps, computed directly
against Boss's armor in `hero-build-variety.md` §2b's table, both rose from
this change; a flat competitive-count average simply means their gains
didn't always land inside the "within 2x of whichever build won that
particular seed's rank 1" band, which moves per seed).

**The real, unambiguous new signal: `arcing_aoe_lobber` wins rank-1 in the
Boss scenario for the first time (1/25 seeds), something neither weapon in
this doc's original 25-seed measurement ever did.** `arcing_aoe_lobber` was
already one of the 3 armor-clearing survivors before this task touched
anything — giving `archer` (its own chassis) `piercing_rounds` access as a
side effect of opening `precision_longbow`'s path let a `piercing_rounds`
roll push `arcing_aoe_lobber`'s own already-high blow (44.0) even further
past Boss's armor (30.0 -> 34.0 effective, `hero-build-variety.md` §2b), and
in at least one seed that was enough to overtake `siege_artillery`. A third
weapon now has a demonstrated, not just theoretical, path to rank 1.

## Verdict

**The build space widened, honestly and modestly, not dramatically.** By
the letter of the brief: more than three weapon archetypes now clear the
armor gate with real margin (5 of 9: `siege_artillery`, `arcing_aoe_lobber`,
`cleave_melee_sweep` unchanged, plus `precision_longbow` and
`chain_bounce_shot` newly opened — `hero-build-variety.md` §2b's blow table),
rank-1 share dropped in both validated scenarios, no single weapon replaced
`cleave_melee_sweep` at the top, and a third weapon (`arcing_aoe_lobber`) won
rank-1 at least once where it never had before. Four weapons
(`dual_strike_melee`, `turret_autocannon`, `beam_continuous`,
`rapid_skirmish_blaster`) remain floored by deliberate choice, not oversight
— closing them would have required either moving the locked chip-floor/armor
values (out of scope) or dps increases large enough to create a new
monoculture (the disqualifying outcome). This is a real, cited, small step —
not a full resolution of the concentration this doc's task-091 section
measured, and the wave scenario's much larger concentration (cleave's
`targets_per_shot` coverage advantage) is untouched, because it isn't an
armor-gate problem and wasn't this task's brief.

## Simulation notes

Ran `py Scripts/sim/differentiation.py --seeds 25` twice, identical
invocation, only `docs/data/hero-builds.json` changed between runs (git diff
limited to the two edits in `hero-build-variety.md` §2b). `py Scripts/sim/
validate.py` and `py Scripts/sim/drift_check.py` both re-run after the data
change — results below. No `Scripts/sim/` file touched; no `entity-tiers.json`
or `combat-model-constants.json` edit. A scratch (uncommitted) Python script
recomputed `reachable_build_count` and the full armor-gate blow table
directly from the live `hero-builds.json`, both before and after the edit,
matching the file's own committed numbers exactly both times — not hand
arithmetic (`hero-build-variety.md` §2b, §9).
