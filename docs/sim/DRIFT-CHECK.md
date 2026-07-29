# `Scripts/sim/drift_check.py` — the recurring half of the sim harness (task-071)

## Why this exists

task-069 (`sweep.py`) made a sweep reproducible on demand — one command,
re-runnable by anyone. task-070 gave it 10 committed scenarios to sweep.
Neither answers a question the owner actually wants answered on a cadence:
**did today's committed data say something different from what it said
yesterday?** That is a distinct job from either of the first two — running
a sweep on demand tells you what the data says *right now*; this tells you
whether that answer *moved*, and if so, exactly what moved it.

A scheduled run that prints numbers nobody reads is worse than nothing.
What makes a recurring check worth its cost is a committed **baseline**
(what the sweep said last time it was deliberately captured) plus a stated
**threshold** for what counts as drift worth flagging.

## What's baselined

`docs/sim/baseline.json` — committed, 7 entries, 55 cells total, generated
by `py Scripts/sim/drift_check.py --refresh --yes` from the 7 sweep
commands defined in `Scripts/sim/drift_check.py`'s `SWEEP_DEFINITIONS`
list. Every entry is a real `sweep.py` invocation (same axis-path language,
same anti-fitting guard — see `docs/sim/SWEEPS.md`), so every baselined
number is traceable to the exact command in its own `"command"` field.

| id | scenario | axis family | model | trust |
|---|---|---|---|---|
| A | `floor1-swarm-wave` | 1 (upgrades: Militia HP) | wave_attrition | drift-only |
| B | `floor1-swarm-wave` | 2 (scenario: Spearmen count) | wave_attrition | drift-only |
| C | `floor2-ranged-wave` | 1 (entity-tiers: ranged enemy DPS) | wave_attrition | drift-only |
| D | `gate1-calibration-wave1` | 3 (constants, DIAGNOSTIC) | wave_attrition | drift-only |
| E | `floor2-elite-point-target` | 1 (entity-tiers: Elite Armor) | point_target | **trustworthy model** |
| F | `floor3-boss-point-target` | 1 (upgrades: Militia DPS) | point_target | **trustworthy model** |
| G | `floor1-swarm-wave` | 1 (unit-types: Spearmen cleave) | wave_attrition | drift-only |

A/B/D/G reproduce `docs/sim/SWEEPS.md`'s own worked examples where
possible (A and B are byte-for-byte the same commands as that file's
Demonstrations 1 and 2). This is a **curated subset**, not exhaustive
coverage of all 10 scenarios x all 4 axis families — chosen to hit every
axis family (`entity-tiers`/`unit-types`/`upgrades`/`constants`/`scenario`)
at least once, both model kinds, and both trust levels, while keeping the
whole check fast enough to run on every invocation without a caller
choosing a subset. `Scripts/sim/drift_check.py`'s `SWEEP_DEFINITIONS` is a
plain Python list — extending coverage later is adding one dict, then
`--refresh --yes`.

## The most important thing this file has to say

**Entries A/B/C/D/G are drift-only, never correctness.** Per
`docs/sim/LIMITATIONS.md` §1, the wave-attrition model does not currently
reproduce its one measured baseline (GATE1's 109-111-of-120 survival) — at
committed defaults it predicts a full wipe. Baselining those five entries'
numbers records "this is what the harness currently says," not "this is
what actually happens." A clean drift-check pass on those entries means
**the committed data hasn't changed since the baseline was captured** — it
does NOT mean the numbers are right. `drift_check.py` prints each entry's
`trust` string on every DRIFT DETECTED line specifically so this
distinction can't be missed mid-incident. Only E and F (point-target,
`docs/sim/LIMITATIONS.md` §3) carry any correctness claim at all, and even
that is bounded (clean-fight lower bound, Fermi `SurroundCapEstimate`).

## Running it

```powershell
py Scripts/sim/drift_check.py                  # CHECK — the schedule runs this
py Scripts/sim/drift_check.py --refresh         # PREVIEW a new baseline, never writes
py Scripts/sim/drift_check.py --refresh --yes   # WRITE docs/sim/baseline.json
```

`--check` (the default, no flag needed) exits 0 if every baselined cell is
within tolerance, 1 if anything drifted or if the baseline's structure
(which cells exist) no longer matches `SWEEP_DEFINITIONS`. It never writes
`docs/sim/baseline.json` under any flag combination — there is no way to
invoke check mode and have it also refresh the file it's comparing
against. That's deliberate: **if a failing run could update its own
baseline, it would not be a baseline, it would be a diary.** Refreshing
requires `--refresh`, and even `--refresh` alone only *previews* the diff
against the current committed baseline — writing requires the additional
`--yes`. Two flags, one of them named for what it does, is the amount of
friction it takes to make "oops, the scheduled job overwrote the baseline"
structurally impossible rather than merely discouraged.

## The threshold: 0.02, absolute, same value for every numeric metric

`combat_model.py` has no randomness anywhere (checked: no `random` import,
no wall-clock read). Given identical `docs/data/*.json` content, two runs
produce **byte-identical** output — there is no run-to-run noise for a
threshold to filter. That changes what the threshold's job actually is
here versus in a system with real measurement noise:

- It is **not** absorbing sampling variance (none exists).
- It **is** absorbing the model's own presentation rounding:
  `combat_model.simulate_wave_attrition`'s result dict rounds
  `retinue_survivors`/`enemy_survivors`/`elapsed_seconds` to 2 decimals;
  `scenario_runner.run_point_target` rounds `ttk_seconds` the same way.
  The smallest possible nonzero delta from that rounding is 0.01.

**0.02 — exactly 2x that rounding floor — is the threshold**, for every
continuous metric this file tracks (`retinue_survivors`, `enemy_survivors`,
`elapsed_seconds`, `ttk_seconds`), no per-metric tuning. Set any lower and
it stops meaning anything different from the bare 0.01 floor; set it
higher and it starts hiding changes on the same order as the effects this
harness exists to detect — `docs/sim/SWEEPS.md` Demonstration 1's own
HP-breakpoint table moves `enemy_survivors` by single digits per 100-HP
step near the knee, and this repo's demonstrated perturbation (below)
produced deltas from 4.05 up to 63.47 — both comfortably clear of 0.02, and
both comfortably below what a double-digit threshold would have swallowed.

**The `result` field is categorical, not numeric, and is exempt from
0.02 entirely: ANY change flags.** `retinue_wiped` / `enemy_wiped` /
`timed_out` flipping is the single most load-bearing kind of finding this
whole harness produces (`docs/sim/SWEEPS.md`'s own framing: "the retinue
actually WINS in 15 of 27 cells") — it must never be filtered by a
tolerance built for continuous metrics.

## Refreshing the baseline — a deliberate, explicit act

`--refresh` alone always previews; nothing is written without also passing
`--yes`. When something in `docs/data/*.json` or `docs/data/scenarios/*`
changes on purpose (a gameplay-director balance pass, a new
`SWEEP_DEFINITIONS` entry, a harness math fix), the flow is:

1. `py Scripts/sim/drift_check.py --refresh` — read the full diff. Every
   line names the entry, the cell's overrides, the metric, old value, new
   value, and delta (or `[QUALITATIVE FLIP]` for a `result` change).
2. Confirm every line is an **expected** consequence of whatever changed —
   not a surprise in some entry nobody meant to touch.
3. `py Scripts/sim/drift_check.py --refresh --yes` — write it, then commit
   `docs/sim/baseline.json` in the same change as whatever data edit caused
   the diff, so the baseline and the data it describes move together in
   git history. (`docs/data/entity-tiers.json`, `unit-types.json`,
   `upgrades.json` are git-tracked; `docs/data/scenarios/**` and this
   baseline file are not yet — see the demonstration below for why that
   matters operationally.)

## Demonstration — perturb, catch, revert, confirm clean

Full transcript, run this session against the current committed baseline.

**1. Baseline created from clean committed data:**

```
$ py Scripts/sim/drift_check.py --refresh --yes
=== drift_check --refresh: no committed baseline exists yet — this would CREATE docs/sim/baseline.json ===

WROTE C:\Projects\ELVTRGAME\docs\sim\baseline.json.

$ py Scripts/sim/drift_check.py
=== drift_check: comparing fresh sweep results against docs/sim/baseline.json ===

[A-floor1-militia-hp-breakpoint] OK — 6 cells, no drift beyond tolerance.
[B-floor1-spearmen-count-breakpoint] OK — 7 cells, no drift beyond tolerance.
[C-floor2-ranged-dps-band] OK — 4 cells, no drift beyond tolerance.
[D-gate1-frontage-model-sensitivity] OK — 27 cells, no drift beyond tolerance.
[E-floor2-elite-armor-band] OK — 4 cells, no drift beyond tolerance.
[F-floor3-boss-militia-dps-band] OK — 4 cells, no drift beyond tolerance.
[G-floor1-spearmen-cleave-band] OK — 3 cells, no drift beyond tolerance.
======================================================================
RESULT: CLEAN. No drift beyond tolerance in any baselined sweep.
EXIT: 0
```

**2. Perturbed `docs/data/entity-tiers.json`'s `brood_soldier_melee.DPS`
from its shipped `42` to `84` (doubled).** Chosen deliberately: it feeds
`floor1-swarm-wave`'s enemy population (entries A, B, G all run that
scenario) but is NOT itself an axis any of those three entries sweeps —
demonstrating drift detection on data an entry *reads but doesn't
override*, not just re-confirming an axis's own swept value. `floor2-
ranged-wave` (C), `gate1-calibration-wave1` (D), and the two point-target
scenarios (E, F) don't reference `brood_soldier_melee` at all, so they
should stay clean — a check on whether the tool over-fires as well as
whether it fires at all.

**3. Ran the check — FAILED, correctly, only on the three entries that
actually read the perturbed value:**

```
$ py Scripts/sim/drift_check.py
=== drift_check: comparing fresh sweep results against docs/sim/baseline.json ===

[A-floor1-militia-hp-breakpoint] DRIFT DETECTED (floor1-swarm-wave):
  wave_attrition — DRIFT-ONLY signal, not a correctness claim. [...]
  cell 0 (upgrades:tier_ladder.tiers[id=militia].hp=130):
    enemy_survivors: 138.62 -> 151.14  (delta=12.5200, tolerance=0.02)
  cell 1 (upgrades:tier_ladder.tiers[id=militia].hp=200):
    enemy_survivors: 92.21 -> 112.87  (delta=20.6600, tolerance=0.02)
    elapsed_seconds: 4.5 -> 3.6  (delta=0.9000, tolerance=0.02)
  cell 2 (upgrades:tier_ladder.tiers[id=militia].hp=300):
    enemy_survivors: 30.41 -> 64.66  (delta=34.2500, tolerance=0.02)
    elapsed_seconds: 8.1 -> 5.4  (delta=2.7000, tolerance=0.02)
  cell 3 (upgrades:tier_ladder.tiers[id=militia].hp=400):
    enemy_survivors: 3.96 -> 23.89  (delta=19.9300, tolerance=0.02)
    elapsed_seconds: 10.8 -> 7.2  (delta=3.6000, tolerance=0.02)
  cell 4 (upgrades:tier_ladder.tiers[id=militia].hp=600):
    retinue_survivors: 4.23 -> 0.0  (delta=4.2300, tolerance=0.02)
    enemy_survivors: 0.0 -> 4.91  (delta=4.9100, tolerance=0.02)
    elapsed_seconds: 300.6 -> 9.9  (delta=290.7000, tolerance=0.02)
    result: 'enemy_wiped' -> 'retinue_wiped'  [QUALITATIVE FLIP]
  cell 5 (upgrades:tier_ladder.tiers[id=militia].hp=900):
    retinue_survivors: 12.78 -> 5.39  (delta=7.3900, tolerance=0.02)

[B-floor1-spearmen-count-breakpoint] DRIFT DETECTED (floor1-swarm-wave):
  [... 6 of 7 cells drifted, including a second QUALITATIVE FLIP at
  Count=200: 'enemy_wiped' -> 'retinue_wiped' ...]

[C-floor2-ranged-dps-band] OK — 4 cells, no drift beyond tolerance.
[D-gate1-frontage-model-sensitivity] OK — 27 cells, no drift beyond tolerance.
[E-floor2-elite-armor-band] OK — 4 cells, no drift beyond tolerance.
[F-floor3-boss-militia-dps-band] OK — 4 cells, no drift beyond tolerance.

[G-floor1-spearmen-cleave-band] DRIFT DETECTED (floor1-swarm-wave):
  cell 0 (unit-types:types.spearmen.combat.targets_per_hit=4):
    enemy_survivors: 185.33 -> 192.62  (delta=7.2900, tolerance=0.02)
  cell 1 (unit-types:types.spearmen.combat.targets_per_hit=8):
    enemy_survivors: 138.62 -> 151.14  (delta=12.5200, tolerance=0.02)
  cell 2 (unit-types:types.spearmen.combat.targets_per_hit=12):
    enemy_survivors: 138.62 -> 151.14  (delta=12.5200, tolerance=0.02)

======================================================================
RESULT: DRIFT DETECTED. One or more committed data files (or the harness's
own math) produced a different sweep result than the committed baseline.
See lines above for exactly what moved and by how much.
EXIT: 1
```

Exactly the four entries that don't touch `floor1-swarm-wave` (C, D, E, F)
stayed clean — the tool did not over-fire on scenarios the perturbation
never reached.

**4. Reverted `brood_soldier_melee.DPS` from `84` back to `42`, confirmed
by direct read (not `git diff` — see the note below on why):**

```
$ py -c "... print DPS for brood_soldier_melee ..."
DPS after revert: 42
```

**5. Re-ran the check — CLEAN again:**

```
$ py Scripts/sim/drift_check.py
=== drift_check: comparing fresh sweep results against docs/sim/baseline.json ===

[A-floor1-militia-hp-breakpoint] OK — 6 cells, no drift beyond tolerance.
[B-floor1-spearmen-count-breakpoint] OK — 7 cells, no drift beyond tolerance.
[C-floor2-ranged-dps-band] OK — 4 cells, no drift beyond tolerance.
[D-gate1-frontage-model-sensitivity] OK — 27 cells, no drift beyond tolerance.
[E-floor2-elite-armor-band] OK — 4 cells, no drift beyond tolerance.
[F-floor3-boss-militia-dps-band] OK — 4 cells, no drift beyond tolerance.
[G-floor1-spearmen-cleave-band] OK — 3 cells, no drift beyond tolerance.
======================================================================
RESULT: CLEAN. No drift beyond tolerance in any baselined sweep.
EXIT: 0
```

**Confirmed reverted.** `docs/data/entity-tiers.json`'s
`brood_soldier_melee.DPS` is `42` (its original committed value), and the
drift check confirms it independently by matching the baseline exactly.

**Why "confirm by direct read," not `git diff`:** `docs/data/entity-
tiers.json` turned out to be untracked by git entirely
(`git ls-files docs/data/entity-tiers.json` returns nothing, unlike
`upgrades.json`/`unit-types.json`, which ARE tracked) — so `git diff`
against it is silently a no-op regardless of what the file contains, not
evidence of anything. The task brief's warning about `docs/data/
scenarios/**` being untracked turned out to extend to at least this one
file outside that directory too. The original value (`42`) was recorded
before editing and the revert was verified by reading the field back
directly, exactly as the brief's safety instruction required for files
`git checkout` can't restore.

## Cadence recommendation

**`drift_check.py` costs nothing but CPU to execute — it's plain
deterministic Python, no LLM call inside it.** The full 7-entry, 55-cell
check above runs in about a second. That means the expensive part of
"running this on a schedule" isn't the script — it's whether an *agent*
gets spun up to invoke it and interpret the result, and that's what the
brief is right to ask to see priced before picking a frequency.

**Recommendation: don't schedule an agent to run this at all — gate it on
the event that can actually produce drift.** Every drift source this file
can detect is a change to a committed file (`docs/data/entity-tiers.json`,
`unit-types.json`, `upgrades.json`, `docs/data/scenarios/combat-model-
constants.json`, or a `docs/data/scenarios/*.json` scenario file, or a
`Scripts/sim/` code change). None of those happen on a clock — they happen
when the gameplay director (or this director) commits an edit. A
time-based cadence (daily/weekly) will spend a full agent invocation
re-confirming "nothing changed" on every single firing where no edit
landed since the last one, which given how infrequently `docs/data/*.json`
actually changes (this repo's history shows edits landing alongside
specific backlog tasks, not continuously) will be the overwhelming
majority of firings.

The cheaper design: **wire the plain script (`py Scripts/sim/
drift_check.py`, exit-code gated) into whatever already runs when
`docs/data/**` changes** — a pre-commit or pre-push hook, or a CI step on
push, is the natural fit, and costs zero incremental agent tokens per
check regardless of how often it fires, because nothing about running a
Python script and reading an exit code requires a model call. Reserve an
*agent* invocation for the moment the exit code is actually 1 — at that
point a short session reads the failure block above (which already names
the entry, cell, metric, old/new value, and delta) and reports it to
whoever needs to see it; that's the only point an agent's judgment is
doing anything a shell script's exit code check couldn't.

If the owner wants a calendar-based backstop in addition (to catch a
change that landed without going through whatever hook/CI path exists —
e.g. a scenario file edited by hand without a commit, or a hook bypassed
with `--no-verify`), **weekly** is the floor I'd recommend for a
schedule that DOES spin up an agent unconditionally: `docs/data/*.json`
changes at roughly a per-task cadence in this repo's own history, not a
daily one, so daily would spend an agent invocation confirming "still
clean" on most days; weekly still catches drift within days of it landing
rather than months. Each such firing is one short agent turn — read this
script's output (a few dozen lines on a clean pass, more on a failure),
state PASS/FAIL, and stop; not a research task, not multi-turn, but not
free either, and it recurs indefinitely at whatever the owner's account
prices a short agent turn at. **I'm not in a position to state that price
in dollars from inside this session — the owner should check whatever
their plan's actual per-invocation cost is before committing to any
recurring number, weekly or otherwise; the "hook/CI plus fire-only-on-
failure" design above is the way to make that number as close to zero as
this check can get.** I have not created any hook, CI step, or scheduled
job myself — that's the owner's call, per the brief.

## Layout addition

```
Scripts/sim/
  drift_check.py         -- this file's tool (task-071)

docs/sim/
  DRIFT-CHECK.md          -- this file
  baseline.json           -- committed expected sweep results (task-071)
```
