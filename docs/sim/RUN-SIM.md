# `Scripts/sim/run_sim.py` — chaining waves into one run (task-096)

**Read `docs/sim/LIMITATIONS.md` §1 before trusting a single number out of
this file's output.** Everything below inherits it unchanged: the
wave-attrition model does not reproduce `GATE1-FUN-PROTOTYPE.md`'s own
measured ~110-of-120 wave-1 survival at the harness's committed defaults,
and predicts a full wipe instead. `run_sim.py` chains that same model
across multiple waves — chaining does not fix it, or make any individual
wave's number more trustworthy. Everything this file reports is a
**relative comparison between runs of one imperfect model**, never an
absolute claim about a played run.

## What this closes from `LIMITATIONS.md` §4

Two of the four gaps `LIMITATIONS.md` §4 lists as "not modeled at all" —
**multi-wave carryover** and **supply/degrade** — now have a real,
re-runnable mechanism. `docs/sim/LIMITATIONS.md` itself is locked this task
(task-076) and has not been edited to reflect that; whoever next has write
access to it should update §4 to point here instead of continuing to list
both as fully unmodeled.

## Running it

```powershell
py Scripts/sim/run_sim.py run-slice-three-wave           # human table
py Scripts/sim/run_sim.py run-slice-three-wave --json    # full result
py Scripts/sim/run_sim.py --selftest                     # the two required checks
```

## The run-file format — `docs/data/scenarios/run-<name>.json`

A run file lives in the same directory as ordinary scenarios (so it's a
plain, hand-editable JSON file, same convention as every other file there)
but is **not** a `scenario_runner.py`-runnable `Kind` — it's read by
`data_loader.load_scenario()` (the generic path-and-parse function every
`docs/data/scenarios/*.json` file already goes through) and consumed only
by `run_sim.py`.

| Field | Type | Meaning |
|---|---|---|
| `Name`, `DisplayName`, `Description`, `SourceRefs`, `Notes` | — | Same convention as `scenarios.schema.md`. |
| `Kind` | `"run_chain"` | Not read by `run_sim.py` at all. Exists only so `scenario_runner.py --all` (which iterates every file `data_loader.list_scenarios()` returns) fails with a readable `Unknown scenario Kind 'run_chain'` message instead of a bare `KeyError` on `scenario["Kind"]`. Both `scenario_runner.py` and `list_scenarios()` are locked this task and can't be taught to skip `run-*.json` files directly — this is the cheapest mitigation available without touching either. |
| `StartingComposition` | array of `{UnitType, Tier, Count}` | Wave 1's actual starting retinue — same row shape as a scenario's `Retinue.Composition`. Should equal the first chained wave's own composition (see the carryover rule below for why). |
| `Waves` | array of scenario names | Existing `wave_attrition` scenario files, run in order. Each wave's `Enemy.Composition` is read from that scenario **unmodified** — enemy population is a fresh spawn each wave, not carried. Each wave's own `Retinue.Composition` is used only as the **target shape** for redistributing survivors into it (see below) — its absolute `Count`s are never used directly past wave 1. |
| `Stops` | array of `{AfterWaveIndex, GrowthSiteId}` | A growth-site breather after the wave at that index (0-based) completes, **only if the retinue was not wiped that wave**. Grants `economy.json`'s `embers.income.growth_site_grant` Embers. `GrowthSiteId` is informational (traces to `growth-sites.json`'s `slice_placement` ids) — not looked up or validated against that file. |

## Survivor carryover — the rule, and why

Wave N's `simulate_wave_attrition` result already reports one field for
this: `retinue_survivors`, a **pooled scalar** (the model splits damage
proportional to alive-count share across whatever groups a wave started
with — it does not, and structurally cannot, hand back a clean
per-tier breakdown that means anything once the NEXT wave's scenario
authors a different set of rows). `run_sim.py` takes that scalar at face
value — "survivors come back as a pooled count, not identified units" —
and re-distributes it into wave N+1's own authored `Retinue.Composition`
shape using **weakest-first casualties**:

- Order wave N+1's rows by tier rank (`upgrades.json`'s `tier_ladder.tiers`
  list order: freed < militia < veteran < bannerman).
- Walk **strongest to weakest**, giving each row its full authored `Count`
  as long as the running survivor total allows.
- The **weakest** row absorbs whatever's left — including dropping to 0,
  cascading to the next-weakest row, if the shortfall is large enough.
- (Symmetrical, if survivors ever exceed the authored total: the surplus
  goes to the **strongest** row.)

This is the task's own stated default ("weakest-first unless you can
justify better") and is not measured against anything — it is the more
defensible of the two obvious choices, not a proven one. The alternative
(cut every row by the same proportion) would imply a Veteran dies exactly
as often as a Freed unit, which nothing in `upgrades.json`'s tier ladder
supports either (Veteran is the tier explicitly described there as holding
formation under fear — routing less, not equally).

**Fractional survivor counts are not rounded.** They're carried through as
float. `combat_model.WaveGroup.count`/`alive_count` are already float —
the model treats "count" as a continuous HP-pool proxy, not an integer
headcount — so this is consistent with an existing convention in the
harness, not a new one, and avoids an arbitrary rounding direction
compounding across a multi-wave chain.

`run-slice-three-wave.json`'s three chained scenarios
(`gate1-calibration-wave1/2/3`) all share the exact same single-row shape
(`spearmen`/`militia`), so this redistribution logic is present but inert
for that particular committed run — every survivor scalar maps onto the
one row that exists. The mechanism is general on purpose (any future run
chaining scenarios with genuinely different tier mixes needs it), but it
is untested against a mixed-tier chain by this task.

## Supply / degrade — where it sits in the tick order

Read directly from `docs/data/economy.json`'s `supply` block, never
hardcoded:

```
demand    = alive_retinue_units * upkeep_per_unit
multiplier = 1.0                                    if demand <= capacity
           = clamp(capacity / demand, min_multiplier, 1.0)   otherwise
```

`capacity` is `supply.start_capacity` (60) for the whole run — nothing in
this task changes it (the `provision` growth-site action that raises
capacity is a **spend**, and this task explicitly does not spend embers;
see task-097). `alive_retinue_units` is the count **going into** that wave
(post-carryover from the previous wave, pre-fight) — degrade is computed
once per wave, before that wave's combat runs, and the resulting
`dps_multiplier` is applied to every retinue fighter's `dps` (via a local
copy of the fighter dict `data_loader.retinue_fighter()` returns — nothing
in `combat_model.py` or `data_loader.py` itself is touched) before the wave
is simulated. **The multiplier is not applied to the Hero** — `economy.json`'s
supply section describes upkeep/degrade purely in terms of recruited
retinue units ("recruiting raises demand"); the Hero is never named as an
upkeep-consuming unit there, so this driver does not invent that it
should be.

## Ember income — where it comes from, and what's deferred

Per wave: `(enemy_start - enemy_survivors) * embers.income.per_brood_killed`
(0.1). Plus, at any `Stops` entry whose `AfterWaveIndex` matches a wave that
was **not** a retinue wipe, `embers.income.growth_site_grant` (10). Both
read from `economy.json`, banked in a running total, reported per wave and
as a final total. **This task does not spend the banked embers** — no
`recruit`/`promote`/`provision`/`item`/`hero` action from `upgrades.json` or
`growth-sites.json`'s `site.actions` is applied to anything; task-097 owns
that.

## `docs/data/economy.json` / `docs/data/growth-sites.json` have no `data_loader.py` accessor

`data_loader.py`'s own module docstring states it is "the ONLY place this
harness touches `docs/data/*.json`." Neither `economy.json` nor
`growth-sites.json` has a loader function there, and `data_loader.py` is on
this task's do-not-touch list (task-076's lock forbids adding one, and
`growth-sites.json` is read by nothing in this task beyond this doc's own
citations, since `Stops` only needs a `GrowthSiteId` string, not a
looked-up row). `run_sim.py`'s `_load_economy()` reads `economy.json`
directly with plain stdlib `json`, mirroring `data_loader._load_json`'s own
pattern rather than silently working around the stated invariant. This is a
real gap in that invariant's coverage, not something quietly patched over
— flagged here and in the task-096 handback.

## The actual committed run — `run-slice-three-wave.json`, and its result

Chains `gate1-calibration-wave1` → `wave2` → `wave3` (the three committed
GATE1 density-calibration fixtures), starting at 120 `spearmen`/`militia`,
with growth-site stops after wave 1 (`growth-A`) and wave 2 (`growth-B`).
Actual output, `py Scripts/sim/run_sim.py run-slice-three-wave`:

```
   w scenario                   ret_start  ret_surv enemy_start enemy_surv         result  demand   cap  mult  embers+ embers_tot
   0 gate1-calibration-wave1        120.0       0.0       250.0      132.2  retinue_wiped   120.0  60.0  0.50    11.78      11.78

  RUN ENDED EARLY: retinue wiped on wave 0 -- remaining waves not run.

  Final embers banked: 11.78
```

Wave 1 wipes and the chain stops there — waves 2 and 3 never run, matching
the task's own expectation ("If the chained run wipes on wave 1 — which
given LIMITATIONS §1 is the likely outcome — report that as the result").
This is a faithfully-produced wipe, not something to route around by
picking a friendlier constant.

**A second, separable finding, worth reading carefully rather than folding
into "the model is unvalidated":** this wave is *worse* than the
un-degraded `gate1-calibration-wave1` fixture on its own (`132.2` enemy
survivors here vs. `19.24` enemy survivors reported by a plain
`scenario_runner.py gate1-calibration-wave1` run) — not because chaining
introduced a new bug, but because `economy.json`'s `start_capacity` (60)
and GATE1's own 120-retinue convention collide immediately: `demand=120`
is already 2x `capacity=60` before a single blow lands, so the retinue
fights wave 1 at the `0.50` DPS multiplier from the very first tick. Every
scenario this harness has run before this task assumed full-strength
DPS — this is the first time a committed run has combined GATE1's
fixed-120 convention with the economy model's own capacity number, and the
combination degrades the retinue by half on wave 1 of a run that
`economy.json`'s own `slice_targets.design_intent` says should be "the
economy layer['s]... margin" for turning a narrow loss into a win. That
tension is between two already-committed design numbers
(`GATE1-FUN-PROTOTYPE.md`'s 120-retinue convention vs. `economy.json`'s
60-capacity number), not a harness defect — see the task-096 handback for
the recommended next step (a gameplay-director finding, not something this
task resolves).

## `--selftest`

```
selftest OK (a): run_one_wave('floor1-swarm-wave', multiplier=1.0) is byte-identical to
  scenario_runner.run('floor1-swarm-wave').
selftest OK (b): run_chain('run-slice-three-wave') is identical across two independent calls.
```

(a) proves this driver has not silently changed the combat math — under
capacity (`floor1-swarm-wave`'s 40 units vs. `economy.json`'s 60-unit
capacity), `run_sim.py`'s own wave-runner reproduces `scenario_runner.py`'s
result field-for-field, including the tick log. (b) is this driver's
determinism check in the shape the task asked for: `run_sim.py` has no RNG
anywhere (unlike `variety.py`/`differentiation.py`), so "same seed run
twice" reduces to "call `run_chain()` twice with identical inputs and diff
the output" — guarding against a shared-mutable-state bug (e.g. a fighter
dict edited in place and reused across calls) that a single run would never
surface.
