# The two combat models — derivations and status

`Scripts/sim/combat_model.py` implements two structurally different models,
because the game has two structurally different fight shapes. Both are
built on the same discrete-swing primitive the shipped combat model actually
uses (`GATE1-FUN-PROTOTYPE.md` §3b), not a continuous-DPS approximation:

```
blow            = DPS x SwingInterval
EffectiveBlow   = max(blow - Armor, ArmorChipFloor)
steady-state DPS = EffectiveBlow / SwingInterval
```

## 1. Point-target model — army vs. one big single-location entity

**Status: validated.** Matches `entity-tiers.md` §7's own published table
exactly (see `docs/sim/VALIDATION.md`'s bonus check) and is the same method
that doc already used, not a new invention.

Elite/Titan/Boss are one entity with one `Location`; only finitely many
melee bodies can physically be in contact with it at once
(`entity-tiers.json`'s `SurroundCapEstimate`, a Fermi estimate the gameplay
director already committed). Ranged attackers are not — every archer in the
army contributes simultaneously (`entity-tiers.md` §4's own finding).

```
per_group_dps = min(count, SurroundCapEstimate) x steady_state_dps   [melee]
per_group_dps = count x steady_state_dps                              [ranged, uncapped]
TTK = target.MaxHP / sum(per_group_dps for all groups, + hero)
```

This is a closed-form snapshot (no time axis, no decline in attacker count
as the target loses HP, because the target is the only thing dying) — exactly
matching how `entity-tiers.md` §7 computed its own N=50/120/250 table.

## 2. Wave-attrition model — pooled two-sided swarm fight

**Status: NOT validated.** Fixes the flaw that made `scaling-curve.md` §7's
discarded model wrong *in kind*, but still cannot reproduce
`GATE1-FUN-PROTOTYPE.md`'s measured survival number. See
`docs/sim/VALIDATION.md` for the actual numbers and
`docs/sim/LIMITATIONS.md` for the diagnosis. Documented here anyway, in full,
because a documented failed model is exactly what task-063 asked for over a
silently-discarded one.

### What was wrong with the discarded model, precisely

`scaling-curve.md` §7: *"a pooled two-sided Lanchester attrition sim (each
side's total DPS applied proportionally against the other's total HP pool,
armor-mitigated per matchup)"* — i.e. **Lanchester's Square Law**: every
attacker on both sides can hit anything on the other side, every tick, no
matter the population size. That's the right law for ranged/area fire, and
the wrong one for a melee brawl where only finitely many bodies can be in
contact with the line at once. It predicted a 100% wipe on floor 1 against a
measured 110-of-120 survival — not a close miss, a different regime.

### The fix attempted here: a frontage-capped ("Linear Law") model

Lanchester's **Linear Law** (the historically-standard model for ancient/melee
combat, as opposed to the Square Law for ranged fire) caps *how many pairs of
combatants can be simultaneously engaged* rather than letting whole
populations interact freely. This harness estimates that cap from the same
Fermi circle-packing technique `entity-tiers.json`'s `SurroundCapEstimate`
already uses for a single point target, generalized to a distributed
ring/blob formation:

```
exposed_frontage(N, formation_spacing, engaged_spacing):
    radius     = formation_spacing * sqrt(N / pi)          # disk-packing footprint
    perimeter  = 2 * pi * radius
    return min(N, perimeter / engaged_spacing)              # bodies that fit on that edge
```

`formation_spacing` (86uu) is `GATE1-FUN-PROTOTYPE.md` §3a's own **measured**
retinue ring-formation spacing at rest (`Swarm.SpacingReport`). `engaged_spacing`
(45uu default, sensitivity range 25-51uu) is that same section's **measured**
mid-combat compressed spacing. The *estimate itself* (how those two measured
numbers translate into a simultaneous-contact count) is NOT measured — same
epistemic status as `SurroundCapEstimate`, and stated as a range for the same
reason.

Per discrete tick (`dt` = the shared `SwingInterval`, 0.9s):

1. `exposed_retinue = exposed_frontage(retinue melee alive, ...)`.
2. `engaging_enemy_melee = min(enemy melee alive, exposed_retinue x MaxAttackersPerUnit)`
   — GATE1 §3b: *"incoming damage is still at most MaxAttackersPerUnit x enemy
   DPS"* per exposed victim. `MaxAttackersPerUnit = 4` is the shipped default
   (`combat-model-constants.json`). This is an INCOMING-only bound.
3. Damage **into** the retinue = `engaging_enemy_melee` bodies, each landing
   their own group's blow, apportioned across enemy subgroups by population
   share.
4. Damage **out of** the retinue uses a bound derived *independently*, not
   `engaging_enemy_melee` — see "Two different physical limits, not one"
   below, which explains why that independence is load-bearing, not a style
   choice.
5. Ranged subgroups on both sides (retinue Archers, `brood_soldier_ranged`)
   are added on top, fully uncapped — same "ranged isn't surround-capped"
   rule as the point-target model.
6. Damage is applied to each side's pooled HP, split across subgroups
   proportional to current alive-count share (a "well-mixed target
   selection" simplification — present in the discarded model too, and NOT
   the part that was wrong in kind).

### Two different physical limits, not one — a bug found in review, and the fix

An earlier version of step 4 capped outgoing cleave demand by reusing
`engaging_enemy_melee` (step 2's INCOMING bound). That is wrong: "how many
enemies can hit me" (`MaxAttackersPerUnit` — an adjacency/elbow-room limit
at point-blank contact) and "how many enemies can I reach with a weapon"
(`TargetsPerHit` within `EngageRange`, 95uu — a longer, geometrically
distinct reach; GATE1's own combat model is geometric Kth-nearest-within-range
targeting, §3b) are different physical quantities. Reusing one for the other
made `exposed_retinue` cancel algebraically out of the ratio whenever the
frontage cap bound, collapsing the model's outcome to the CONSTANT
`MaxAttackersPerUnit / TargetsPerHit` — deleting `TargetsPerHit`'s effect
entirely. Full algebra and the sweep this produced (now retracted) is in
`docs/sim/VALIDATION.md`.

The fix: `combat_model.melee_reach_per_exposed_unit(engage_range,
engaged_spacing, facing_fraction)` derives the outgoing cleave bound on its
own terms —

```
melee_reach_per_exposed_unit(R, engaged_spacing, facing_fraction):
    area      = pi * R^2 * facing_fraction     # R = EngageRange, 95uu for Spearmen
    footprint = engaged_spacing^2
    return area / footprint                     # enemies simultaneously within weapon reach
```

`facing_fraction` (default 0.5, new Fermi input, `combat-model-constants.json`)
accounts for roughly half of that circle being the exposed unit's own
formation neighbors, not enemies. `reach = min(TargetsPerHit, this
estimate)` — so a unit's cleave is capped by whichever is smaller: its own
design stat, or how many bodies actually fit within its own weapon range
locally. The aggregate cleave demand (`exposed_retinue x reach`, summed
across subgroups) is THEN capped against the whole living enemy population
(`enemy_melee_alive`) as a final sanity bound — you plainly can't hit more
enemies than exist — but that population-level cap is now the ONLY place
`MaxAttackersPerUnit` and this calculation could ever interact, and they
don't, by construction. `validate.py`'s cleave-sensitivity guard (check 4,
required/gating) exists specifically to catch this regressing.

### Why this still (mostly) fails — see VALIDATION.md for the real sweep

With cleave genuinely decoupled, the model is now capable of the retinue
*winning* — and does, in 15 of a real 27-cell sweep (`docs/sim/VALIDATION.md`).
At the harness's own committed defaults (`MaxAttackersPerUnit=4`,
`EngagedSpacingUU=45`, `MeleeContactFacingFraction=0.5` — the shipped CVar
default plus this harness's stated midpoint estimates), it still loses, and
even its single best untested cell in the sweep (survivors ~53 of 120) only
reaches about half the measured 109-111. Two live, undisentangled
candidates for the remaining gap (see `docs/sim/LIMITATIONS.md` for the full
discussion, stated there as genuinely open, not resolved here):

- **Arrival/spawn-pacing timing** — still real, still undocumented anywhere
  in committed data (`scaling-curve.md` §3/§4's own flagged gap). Nothing in
  this fix touches it.
- **The pooled/geometric approximation may not transfer `MaxAttackersPerUnit`
  cleanly.** In the real per-entity sim it bounds one specific victim's
  simultaneous attacker count at an instant; in this pooled model it's
  being used as an aggregate rate multiplier across an entire exposed
  perimeter every tick, which is not obviously the same thing statistically
  — plausibly over-stating incoming damage relative to what the real
  spatial sim produces.

This harness cannot currently distinguish between those two explanations
with the data it has. Both are stated as open, not resolved, on purpose.

### 3. Arrival gating (task-068) — candidate (1), tested, and its result

`docs/data/encounter-budget.json` §2a added real, shipped-code-derived
per-rank arrival timing (`rank_arrival_timing[]`, cited off
`Swarm.BroodSpawnRadiusMin`/`BroodFormation.*`/`BroodSpeed` CVar defaults —
see that file's `design_constants.rank_arrival_source_cvars`). This gives
`simulate_wave_attrition` a real, non-invented number for "when does a rank
of brood actually reach contact range" instead of the previous "every
`Composition.count` is alive and contending for frontage from `t=0`"
assumption.

**The mechanism.** Every `WaveGroup` now carries an `arrival_seconds` field
(default `0.0` — already on the field at t=0, exactly the old behavior; opt-in
per row). `WaveGroup.has_arrived(t)` gates a group out of everything in the
per-tick loop that isn't "the group still exists": `exposed_frontage`'s
melee-alive input, the `engaging_enemy_melee` incoming-damage bound, the
`contact_scale` outgoing-damage cap, and the proportional damage-application
split. A not-yet-arrived group keeps its `hp_pool` untouched and still counts
toward `total_alive()` (the stop condition and final survivor report), because
it hasn't been fought yet, not because it's already dead. No new Fermi
estimate, no new free dial — `arrival_seconds` is data, not a fitted
parameter, and defaults to zero everywhere it isn't explicitly set (see
`docs/data/scenarios/scenarios.schema.md`'s `ArrivalSeconds` section).

`gate1-calibration-wave1.json`'s single 250-count `brood_fodder` row is now 5
per-rank rows (`brood_fodder_rank0..4`, counts 60/60/60/60/10) carrying
`ArrivalSeconds` `5.85/6.29/6.73/7.17/7.60` — `rank_arrival_timing[]`'s
`gate1_calibration_wave1_rank0..4` rows, verbatim, nominal (no jitter).

**The result — a real time-shift, not a survival change.** Re-running
`gate1-calibration-wave1` (`docs/sim/VALIDATION.md` has the exact numbers):
the retinue takes **zero** damage before ~5.85s (confirmed in the tick log —
`dmg->ret = 0.0` through t=5.4s), then the fight resolves to the **same**
outcome as the ungated model: retinue fully wiped, ~19-23 of 250 enemy
survivors (vs. 19.73 ungated) — just roughly 7 seconds later (11.7s elapsed
vs. 4.5s). Checked across the full `BroodSpeedJitter` ±6% bracket
(Fast/Nominal/Slow arrival times): retinue survivors are 0.00 in all three,
enemy survivors land in a tight 20.19-22.50 band. **This is robust, not a
coincidence of the nominal timing.**

**Why, mechanistically, arrival gating can't close this gap in this model.**
At this population scale, `exposed_retinue * MaxAttackersPerUnit` (74.21 x 4
= ~296.8) already exceeds the entire 250-strong enemy population — so
`engaging_enemy_melee = min(enemy_melee_alive, exposed_retinue *
MaxAttackersPerUnit)` is bounded by `enemy_melee_alive` itself, not by the
frontage cap, for any rank count above ~0. The frontage cap literally never
binds on the incoming side at N=250 vs a 120-retinue formation with the
shipped `MaxAttackersPerUnit=4`. That means once ANY meaningful fraction of
the enemy has arrived, the model's incoming-damage rate is essentially "full
arrived-population DPS," identical in kind to the t=0 case — arrival timing
changes *when* that rate applies, not what it converges to. The model has no
mechanism by which delaying contact changes the eventual outcome, because
nothing in it depends on elapsed time or accumulated fatigue — only on
current alive-counts. A model built this way is structurally incapable of
letting arrival pacing alone turn a loss into a win, regardless of how
accurate the arrival numbers themselves are.

**What this means for `docs/sim/LIMITATIONS.md`'s two candidates.** This is
evidence, not proof, but it points one way: candidate (1) (arrival timing) is
now tested with real cited data and found not to move the check-3 result at
all, across its full measured jitter range. Candidate (2)
(`MaxAttackersPerUnit`'s pooled-vs-per-entity transfer) is untouched by this
change and remains the more promising open explanation — see
`docs/sim/LIMITATIONS.md` §1 for the current state of both.

## 4. The variance layer (task-076) — turning a point estimate into a distribution

Everything above this section is, and remains, fully deterministic:
`combat_model.py` has no randomness anywhere in `ttk_1v1`,
`army_ttk_vs_point_target`, or `simulate_wave_attrition` — those three
functions were not touched by task-076 and do not take an `rng` parameter.
Running the same configuration twice gives byte-identical output, same as
before this section existed. What task-076 adds is a small, OPTIONAL layer
that perturbs a trial's *inputs* before handing them to that unchanged core,
so a caller can run one configuration N times and get a distribution instead
of one number — never by adding noise inside the validated math itself.

### Why perturb inputs, not the core

`combat_model.jitter_arrival_seconds()` and `combat_model.jitter_fighter_dps()`
are pure sampling helpers: given an explicit `random.Random` and a magnitude,
they return a perturbed COPY of one input value (an `ArrivalSeconds` float,
or a fighter dict's `dps`), never mutate anything, and are only ever called
by `scenario_runner.py`'s trial-construction code — `simulate_wave_attrition`
and `army_ttk_vs_point_target` never see an `rng` at all. This means the
"no seed -> bit-identical" safety property (`docs/sim/LIMITATIONS.md`'s new
variance-layer section states the stakes) holds **by construction**: an
unseeded call never constructs a `random.Random` in the first place, so
there is nothing for the jitter helpers to be called with, regardless of
what `combat-model-constants.json`'s `variance_model` block says.

### The two variance sources, and their citation status

Both live in `docs/data/scenarios/combat-model-constants.json`'s
`variance_model` block, each with an `enabled` flag (both `false` in the
committed file — see that block's own `$schema_note` for why, and
task-076's brief §1/§3 for the requirement), a `magnitude`, and a `status`
of `"cited"` or `"invented"`:

1. **`arrival_jitter` — CITED.** `Swarm.BroodSpeedJitter`
   (`docs/data/encounter-budget.json`'s `rank_arrival_source_cvars`) is a
   real shipped +/-6% per-brood speed jitter at spawn, already the source of
   that file's own `ArrivalSecondsFast`/`ArrivalSecondsSlow` columns and
   referenced in this doc's §3 arrival-gating account above. That file's own
   `rank_arrival_formula` divides travel time by `BroodSpeed * (1 +/-
   jitter)`, i.e. `ArrivalSeconds` scales by `1 / (1 + jitter)` —
   `combat_model.jitter_arrival_seconds()` reproduces that exactly (not a
   linear +/- approximation), confirmed against the file's own precomputed
   numbers to rounding (see that function's docstring). Only applies to
   `wave_attrition` scenarios (`point_target` has no arrival/time axis —
   it's a closed-form snapshot, §1 above) and only to groups whose
   `ArrivalSeconds > 0` (a group already at t=0 has nothing to jitter).
2. **`damage_roll_jitter` — INVENTED, no citation.** No shipped CVar or
   committed data file describes swing-to-swing damage-roll variance for
   this game. `combat_model.jitter_fighter_dps()` samples one multiplicative
   DPS factor per fighter GROUP for the whole trial (not per-swing — the
   discrete-swing model has no per-hit event loop to attach a finer-grained
   roll to), magnitude +/-10%, this harness's own unmeasured guess. Applies
   to both scenario kinds. Any trial that enables it is flagged
   `diagnostic_invented_variance: true` in `run_trials()`'s output and
   prints a DIAGNOSTIC banner on the CLI — see
   `docs/sim/LIMITATIONS.md`'s variance-layer section for what that flag
   means and doesn't mean.

Per task-076 §3: an invented source's `enabled` flag must default to
`false`. A cited source isn't required to (it isn't a fitted number), but
this harness keeps both `false` in the committed file anyway — the
strictest reading of "variance defaults OFF, globally and per-source."
Turning a source on is a deliberate edit to that committed file (same
convention every other dial in it already uses), not a CLI flag.

### Seed derivation — derived, never streamed

```
seed_for(root_seed, scenario_name, overrides, trial_index)
  = int.from_bytes(
      sha256(canonical_json(
          [root_seed, scenario_name, sorted(overrides.items()), trial_index]
      )).digest()[:8],
      "big",
    )
```

`canonical_json` is `json.dumps(obj, sort_keys=True, separators=(",", ":"))`
— sorted keys, no incidental whitespace, so the same logical payload always
hashes to the same bytes regardless of dict insertion order.
`scenario_runner.compute_trial(name, trial_index, root_seed, overrides)`
constructs a FRESH `random.Random(seed)` from this per trial and threads it
explicitly down into `_run_wave_attrition_trial`/`_run_point_target_trial`
— never a module-global `random` instance, never the `random` module's own
global state.

**Why derived, not a shared stream advanced across trials:** a shared,
advancing RNG makes trial *i*'s result depend on every trial computed before
it in THIS run, in THIS process, in THIS order — which is fine for a single
serial loop but breaks the moment trials are computed out of order or on
separate workers (a `ProcessPoolExecutor`, which task-075's planned batch
runner uses). Deriving each trial's seed purely from its own identity
(`root_seed`, scenario, overrides, trial index) makes `compute_trial` a pure
function of those four inputs — safe to call from any process, in any
order, and guaranteed to agree with every other process's answer for the
same inputs. `validate.py`'s order-independence check (check 7) proves this
directly: the same 8 trials computed serially, in reversed order, and via a
4-worker process pool all produce identical per-trial results.

### Percentile method

`run_trials()`'s summary reports `p5`/`p95` via
`statistics.quantiles(values, n=100, method="inclusive")`, taking index 4
(p5) and index 94 (p95) of the 99 returned cut points. `"inclusive"` is
stdlib's name for the conventional linear-interpolation percentile
definition (numpy's default, Excel's `PERCENTILE.INC`) — stated explicitly
because percentile choice is method-sensitive at the small trial counts this
harness's own checks use (e.g. n=8), and a reader comparing two runs needs
to know which convention produced the numbers, not just that "a percentile"
was computed.

### The trials API

```
run(name, seed=None) -> dict
    seed=None: identical to every pre-task-076 call. seed given: constructs
    ONE random.Random(seed) directly (no seed_for derivation needed for a
    single explicit seed) and applies whichever sources are enabled.

run_trials(name, trials, root_seed=None, overrides=None) -> dict
    {
      "scenario", "kind", "trials", "root_seed",
      "variance_sources_enabled": [...],       # enabled AND applicable to this kind
      "diagnostic_invented_variance": bool,    # true if any enabled source is "invented"
      "results": [<per-trial result dict>, ...],
      "summary": {"<field>": {"n","mean","median","p5","p95","min","max","stdev"}},
    }
```

`overrides` (optional) is a flat `{"<dot.path>": value}` dict applied to a
deep copy of the loaded scenario JSON before any trial runs — the same
dot/`[field=value]` filter path language `sweep.py`'s `scenario:` axis
family uses, reimplemented independently in `scenario_runner.py` (this task
doesn't own `sweep.py`, and the two are meant to stay decoupled). It only
ever targets the scenario file, not `combat-model-constants.json` — turning
a variance source on/off is a committed-file edit, not something a caller
threads through `overrides` at call time.

CLI: `py Scripts/sim/scenario_runner.py <name> --trials N --seed S`. `N`
absent or `1` with no `--seed` is unchanged from before this task existed.
`--seed` alone (no `--trials`, or `--trials 1`) runs one seeded `run()` call
in the normal single-result format. `--trials > 1` prints the distribution
summary instead, plus a DIAGNOSTIC banner if any enabled source is invented.
