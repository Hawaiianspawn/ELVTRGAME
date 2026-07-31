---
id: 076
title: Add a seeded variance layer to the combat model so a config yields a distribution, not a single point estimate
status: done
agent: sim-director
model: opus
owns:
  - "Scripts/sim/combat_model.py"
  - "Scripts/sim/scenario_runner.py"
  - "Scripts/sim/validate.py"
  - "docs/sim/MODEL.md"
  - "docs/sim/VALIDATION.md"
  - "docs/sim/LIMITATIONS.md"
  - "docs/data/scenarios/combat-model-constants.json"
resources: []
depends-on: []
epic: sim-pipeline
evidence: >
  `py Scripts/sim/scenario_runner.py <name> --trials 200 --seed 1` prints a
  distribution (n, mean, median, p5, p95, min, max) instead of one number;
  `py Scripts/sim/validate.py` gains and passes an identity check proving
  variance-off output is bit-identical to the committed pre-variance numbers,
  a same-seed reproducibility check, and an order-independence check; `py
  Scripts/sim/drift_check.py` still passes against the UNCHANGED
  docs/sim/baseline.json.
score: {feel: 1, risk: 3, cost: 2}
source: user
teammate: variance-layer-retry
decided: "2026-07-31 done"
---

## Why now

The model is fully deterministic — there is no RNG anywhere in
`combat_model.py`. That means running the same configuration twice is pointless,
every published number is a single point estimate with no stated spread, and
"thread multiple tests" has nothing to thread beyond parameter combinations.
The owner asked for both combinations and randomized trials (2026-07-29), and
trials are the half that does not exist.

This is also the task with real risk in the epic: it edits the one model that
currently *passes* its validation checks and reproduces `entity-tiers.md` §7's
table exactly. The safety property is therefore identity, not accuracy — with
variance off, output must be bit-identical to today, and `drift_check.py` must
stay green against the existing committed baseline.

## Done when

1. `combat_model.py` supports optional, explicitly-enabled, seeded variance on
   named sources, each source individually toggleable and each defaulting OFF.
2. Seeding is derived, not streamed — the same (scenario, overrides, trial
   index, root seed) always produces the same result, in any execution order,
   in any process.
3. `scenario_runner.py` exposes `run(name, seed=None)` and
   `run_trials(name, trials, root_seed)` returning per-trial results plus a
   stdlib-computed summary, and a `--trials/--seed` CLI.
4. `validate.py` gains three checks: identity (variance off == committed
   numbers), reproducibility (same seed == same result), order-independence.
   All three gate the exit code.
5. `MODEL.md`, `VALIDATION.md` and `LIMITATIONS.md` state what each variance
   source is, whether its magnitude is cited or invented, and what a spread
   from this layer may and may not be used to argue.

## Spawn prompt

```
You are executing task-076. Read docs/backlog/task-076-seeded-variance-layer-distributions.md
first, then docs/sim/MODEL.md in full, docs/sim/LIMITATIONS.md in full,
docs/sim/VALIDATION.md, docs/sim/DRIFT-CHECK.md, and
Scripts/sim/combat_model.py in full.

You are the sim-director. Give the combat model an optional seeded variance
layer so a configuration can be run N times and yield a distribution instead of
a single point estimate. You own EXACTLY these paths:

    Scripts/sim/combat_model.py
    Scripts/sim/scenario_runner.py
    Scripts/sim/validate.py
    docs/sim/MODEL.md
    docs/sim/VALIDATION.md
    docs/sim/LIMITATIONS.md
    docs/data/scenarios/combat-model-constants.json

DO NOT TOUCH, for any reason: Scripts/sim/sweep.py, Scripts/sim/drift_check.py,
Scripts/sim/data_loader.py, docs/sim/baseline.json, docs/sim/README.md,
docs/sim/SWEEPS.md, docs/sim/DRIFT-CHECK.md, docs/sim/PIPELINE.md,
docs/data/experiments/**, docs/data/scenarios/*.json OTHER than
combat-model-constants.json, .gitignore, .claude/agents/sim-director.md, or
anything under ELVTR/ or docs/design/. A parallel teammate (task-076's sibling,
task-075) is creating Scripts/sim/batch.py, runstore.py, report.py and
docs/data/experiments/** in the same window; task-077 wires your trials API into
their batch runner afterwards. Do not write their files and do not write the
wiring yourself.

Stdlib only — `random`, `hashlib`, `statistics`. No numpy, no new dependency.
The harness's "plain stdlib, no install step" property is load-bearing.

## 1 — THE SAFETY PROPERTY, before any feature work

Variance defaults OFF, globally and per-source. With no seed and no trials
argument, every existing entry point must produce output BIT-IDENTICAL to what
it produces today. Establish this before you change behaviour:

  1. First, capture today's baseline. Run and save to a scratch file OUTSIDE
     the repo (use the job tmp dir, not a committed path):
       py Scripts/sim/scenario_runner.py --all --json
       py Scripts/sim/validate.py
       py Scripts/sim/drift_check.py
  2. Do the work.
  3. Re-run all three and diff. `scenario_runner.py --all --json` must be
     byte-identical. `drift_check.py` must pass against the UNCHANGED
     docs/sim/baseline.json.

If drift_check.py reports drift at any point, you have changed the default
numeric path. Fix that — do NOT refresh the baseline. Refreshing it is a
deliberate owner-level act (docs/sim/DRIFT-CHECK.md) and is explicitly not
yours in this task; baseline.json is not in your owned paths.

## 2 — deterministic derived seeding (get this right first)

Do NOT use a single shared RNG stream advanced across cells or trials, and do
not use the `random` module's global state. Both make results depend on
execution order, which breaks under the process pool task-075 is building and
would make every persisted artifact unreproducible.

Instead derive each trial's seed:

    seed_for(root_seed, scenario_name, overrides, trial_index)
      = int.from_bytes(sha256(canonical_json([root_seed, scenario_name,
                                              sorted(overrides.items()),
                                              trial_index])).digest()[:8])

and construct a FRESH `random.Random(seed)` instance per trial, passed
explicitly down the call chain into the model. Never a module global. Document
the derivation in MODEL.md so it is reproducible from the doc alone.

Consequence you must verify explicitly: computing trials [0,1,2,3] in any order,
in any number of processes, yields the same per-trial results. That is check 3
in §4.

## 3 — what may actually vary, and the citation rule

This is the part where it would be easy to do real damage. Each variance source
must be one of:

  (a) CITED — its magnitude traces to a shipped CVar default or a committed
      data file. `Swarm.BroodSpeedJitter` is the model case: it is a real
      shipped CVar with a documented ±6% bracket, already referenced in
      docs/sim/LIMITATIONS.md §1 and behind encounter-budget.json's
      rank_arrival_timing[]. Arrival-time jitter is therefore a legitimate,
      cited variance source.
  (b) HARNESS-INVENTED — no citation exists (e.g. per-unit swing-phase
      desynchronisation, damage roll). These are permitted, but each must
      default to OFF INDIVIDUALLY, be named in combat-model-constants.json with
      a per-field note saying plainly that its magnitude is invented and not
      measured, and be flagged in output as diagnostic — the same register
      combat-model-constants.json already uses for its measured-vs-estimated
      fields, and the same treatment sweep.py gives family-3 axes.

Add the variance dials to combat-model-constants.json in their own block
(e.g. `variance_model`), each with the enable flag, the magnitude, and the
cited-or-invented note. Follow that file's existing `$schema_note` convention.

THE FITTING PROHIBITION — read docs/sim/LIMITATIONS.md §1's closing paragraph
before you touch a magnitude. That section forbids tuning EngagedSpacingUU /
MaxAttackersPerUnit / MeleeContactFacingFraction to force validation check 3 to
pass, because a passing check with no citation behind its value is worse than an
honest documented failure. A variance layer creates a NEW and more tempting
version of the same trap: with enough spread, GATE1's measured 109-111
survivors falls inside *some* distribution's tail, and check 3 can be declared
"passing within variance". Do not do this. Do not select a variance magnitude
because it makes check 3 pass, do not reframe check 3 as passing because a
measured value lands in a tail, and do not add any code that searches for a
magnitude by proximity to a target. If you find that a cited variance source
happens to widen the distribution toward the measured value, report that as an
observation with the magnitude's citation attached, and leave the check's
verdict exactly as it is. Adding variance does not close the §1 gap and must not
be presented as closing it.

## 4 — the three new validate.py checks

All three gate the exit code (unlike check 3, which is reported but
non-gating — preserve that existing behaviour exactly).

  Check: IDENTITY. With variance off, a set of committed scenario results
  matches expected values that you capture in §1 step 1 and embed as literals.
  This is the regression wall protecting the validated point-target model.

  Check: REPRODUCIBILITY. run_trials(name, 8, root_seed=1234) called twice
  returns identical per-trial results.

  Check: ORDER-INDEPENDENCE. The same 8 trials computed in a shuffled order —
  and, separately, computed via a ProcessPoolExecutor with 4 workers — return
  the same per-trial results as the serial in-order computation. (Use the pool
  in the check itself; guard it with `if __name__ == "__main__":` since this
  machine is Windows/spawn.)

Update docs/sim/VALIDATION.md with the actual committed output of the full
suite, matching how that file already records check results — real pasted
numbers, not a description of them.

## 5 — the trials API (task-077 wires this; get the signature right)

Expose in scenario_runner.py, and treat these signatures as a contract another
task depends on:

    run(name, seed=None) -> dict
        Existing behaviour when seed is None. Bit-identical to today.

    run_trials(name, trials, root_seed=None, overrides=None) -> dict
        {
          "scenario": ..., "kind": ..., "trials": <int>, "root_seed": ...,
          "variance_sources_enabled": ["arrival_jitter", ...],
          "diagnostic_invented_variance": <bool>,
          "results": [<per-trial result dict>, ...],
          "summary": {"<numeric field>": {"n":…, "mean":…, "median":…,
                                          "p5":…, "p95":…, "min":…, "max":…,
                                          "stdev":…}}
        }

`summary` covers the numeric result fields the existing result dicts already
carry (retinue_survivors, enemy_survivors, elapsed_seconds for wave_attrition;
ttk_seconds for point_target). Percentiles via `statistics.quantiles` — state
the interpolation method you used in MODEL.md, since p5/p95 on small n is
method-sensitive and a reader comparing two runs needs to know.
`diagnostic_invented_variance` is true whenever any enabled source is
category (b) from §3.

CLI: `--trials N` and `--seed S` on scenario_runner.py. With --trials absent or
1 and no --seed, behaviour and output are unchanged. With --trials > 1, print
the distribution summary, and print a DIAGNOSTIC banner if any enabled variance
source is invented rather than cited.

## 6 — docs

MODEL.md: a new section on the variance layer — each source, its magnitude, its
citation or its explicit lack of one, the seed derivation, the percentile
method, and why seeding is derived rather than streamed.

LIMITATIONS.md: a new section stating what a spread out of this layer may and
may not be used to argue. Be blunt: the spread reflects only the sources
modelled here, is not a confidence interval on the real game, cannot be used to
declare check 3 passing, and inherits every §3/§4 limitation the point estimates
already had. Do not weaken or rewrite any existing section of that file —
add to it. §1's account of the check-3 gap stands exactly as written.

Hand back: the §1 before/after diff result (state explicitly that
scenario_runner --all --json was byte-identical and that drift_check passed
against the unchanged baseline), the full validate.py output with all new
checks, one wave_attrition and one point_target distribution at --trials 200,
the list of variance sources with each one's cited-or-invented status, and a
plain statement of whether any enabled source is invented. If you concluded that
some intended variance source could not be grounded and left it off, say so —
that is a good outcome, not a shortfall.
```
