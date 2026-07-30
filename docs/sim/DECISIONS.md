# Does the growth-site allocation actually decide anything? (task-097)

**Verdict, committed run: THEATRE at both stops — not a small effect, a
zero one.** Under `run-slice-three-wave.json`'s own committed inputs
(`Scripts/sim/decisions.py`'s **PRIMARY** condition — the harness's default
constants, `economy.json`'s real `start_capacity`, unmodified), the retinue
is wiped on wave 1 before either growth-site stop is ever reached. All 5
allocation branches this file tests are **byte-identical through and past
the point they'd diverge**, because there is no point where they diverge —
`Stops` only apply "if the retinue was not wiped that wave"
(`docs/sim/RUN-SIM.md`), and it always is. Growth-A's allocation is inert by
construction, not by measurement; growth-B, gated behind surviving wave 2,
is inert for the same reason at one more remove.

This is the deliverable the task brief itself anticipated ("if every branch
wipes identically... reporting exactly that... IS the correct
deliverable"), and this doc also carries the SECONDARY/TERTIARY hypothetical
sensitivity checks the brief explicitly allowed for, clearly separated from
the committed answer below.

```powershell
py Scripts/sim/decisions.py               # human table, all three conditions
py Scripts/sim/decisions.py --json        # full result as JSON
py Scripts/sim/decisions.py --selftest    # reproducibility check, exit 1 on failure
```

**Read `docs/sim/LIMITATIONS.md` §1 before trusting any number below.**
Every number in this file inherits it unchanged through `run_sim.py`: the
wave-attrition model does not reproduce `GATE1-FUN-PROTOTYPE.md`'s own
measured ~110-of-120 wave-1 survival at the harness's committed defaults,
and predicts a full wipe instead. Everything here is a **relative
comparison between branches of one unvalidated model**, never an absolute
claim about a played run.

## The premise this task was handed, corrected before use

The brief asked for "25 seeds, same seed, same enemy side, only the
allocation differs," modeled on `differentiation.py`'s own precedent.
Checked against the actual code before building on it (per this file's own
standing instruction to re-verify a brief's premise, not assume it):
**there is no RNG anywhere this measurement touches.** `run_sim.py`'s own
module docstring states it plainly ("No RNG anywhere in this driver"),
`combat_model.py` has no `random` import, and `differentiation.py`/
`variety.py`'s seeded RNG rolls a **hero-build roster** — orthogonal to
growth-site spend, and not even present in these scenarios
(`HeroPresent: false` on all three chained GATE1 fixtures). Growth-site
effects themselves (recruit/promote/provision, as this file models them)
are deterministic transforms on the retinue composition and a capacity
scalar.

Consequence, stated precisely rather than hedged: **the "seed-to-seed
variance of one fixed branch" this task asked to compare against is not
measured to be small — it is provably exactly zero**, for every branch,
every condition, by construction. `decisions.py --selftest` demonstrates
this directly (each branch, in each of the three conditions below, run
twice with identical inputs, asserted byte-identical — `run_sim.py`'s own
`--selftest (b)` established the same thing for the unmodified chain). This
means step 4's literal "spread vs. noise ratio" degenerates: dividing by a
proven zero is meaningless, so **any nonzero branch spread trivially
"beats" it** — the honest test that survives this correction is narrower:
not "is the signal bigger than the noise," but "does branch choice change
the outcome at all, and is that change big enough in absolute terms to
matter." Both are answered explicitly below, per condition, rather than
folded into a single misleading ratio.

## Three conditions, not one

| | MaxAttackersPerUnit | supply.start_capacity | Reaches growth-A? | Reaches growth-B? |
|---|---|---|---|---|
| **PRIMARY** — committed | 4 (shipped default, unmodified) | 60 (economy.json, unmodified) | No | No |
| **SECONDARY** — hypothetical | 1 (diagnostic override) | 60 (unmodified) | No | No |
| **TERTIARY** — hypothetical | 1 (diagnostic override) | 120 (diagnostic override) | Yes | Yes |

The `MaxAttackersPerUnit=1` override is not invented for this task — it is
the specific, already-cited cell from `docs/sim/VALIDATION.md`'s 27-cell
sweep ("at `MA=1` the retinue wins in all 9 (ES, FF) combinations"),
applied via the exact mechanism `sweep.py` already established for family-3
constants (monkeypatch `data_loader._load_json` for one call's duration,
never write `docs/data/*.json` to disk — see `decisions.py`'s
`diagnostic_constants_override`, ~15 lines, not a new technique). The
`start_capacity=120` override is the task brief's own named example ("a run
where supply capacity is not already 2x-oversubscribed at t=0"), using an
already-committed number (`GATE1-FUN-PROTOTYPE.md`'s 120-retinue
convention) rather than an invented one. **Neither of these is the
committed data. Both are clearly labeled hypothetical everywhere this
file and `decisions.py`'s own output print them.**

**Why SECONDARY exists and still wipes:** it isolates the two already-known
problems from each other. `MA=1` alone is enough to flip
`gate1-calibration-wave1` from a wipe to an `enemy_wiped` win **at full DPS**
(checked directly: `py Scripts/sim/sweep.py gate1-calibration-wave1 --axis
"constants:wave_attrition_model.MaxAttackersPerUnit=1"` → 120 → 38.21
survivors, `enemy_wiped`). But run through `decisions.py` at `economy.json`'s
real `start_capacity=60`, the wave-1 demand-at-t=0 problem this task
inherited from task-096's handback (finding (b): 120 retinue vs 60 capacity
= 0.50 DPS multiplier before a blow lands) **halves that same favorable
`MA=1` cell's DPS and sinks it too** — confirmed directly (same command,
`compute_degrade` forced to `capacity=120` first shows `dps_multiplier=1.0`
and the fight wins; forced back to the real `60` and it doesn't). **This is
a new, separable finding, not a restatement of (b):** the economy's t=0
oversubscription is not just "worse than the un-degraded fixture" (task-096's
own finding) — it is potent enough to sink even a combat-model cell that is
independently known to win outright. Growth-A is unreachable under
SECONDARY for exactly the same structural reason as PRIMARY: the wipe
happens before the stop.

**Only TERTIARY — stacking both hypothetical fixes — reaches either stop at
all.** Every number past this point in this document is TERTIARY unless
stated otherwise, and none of it is the committed answer.

## Reading `growth-sites.json` — the assumptions this measurement had to make explicit

The file doesn't state these; `decisions.py` picks one reading and holds to
it consistently, documented here rather than left implicit:

- **Each of the 5 actions may be picked at most once per stop.** The panel
  is read as 5 discrete choices, not a repeatable shop — consistent with
  the file's own "3 offered [items], take 1" convention for the `item` lane
  and the `budget_note`'s "two of three, or two lanes plus a partial"
  framing (singular picks, not multiples of one lane).
- **Unspent Embers carry forward** to the next stop, matching `run_sim.py`'s
  own running-total design for the field it doesn't spend.
- **A branch is a fixed policy** (a target action set) applied identically
  at every stop it can afford, not an independent per-stop choice — five
  branches (`hoard`, `recruit_only`, `promote_only`, `provision_only`,
  `triangle`) rather than the full per-stop combinatorial product. This
  simplification is named, not hidden: it means growth-A's and growth-B's
  effects are read from the same 5 runs, not independently varied stops
  (see "Isolating growth-A from growth-B," below, for how this doc still
  separates the two as far as the data allows).
- **Affordability is greedy, costliest-first-dropped:** if a branch's full
  target set doesn't fit the bank on hand, the single most expensive
  action is dropped and the check repeats until it fits (or the set is
  empty). This is `decisions.py`'s own tie-break, not a stated rule in the
  data file, and it visibly matters (see `triangle`'s growth-A row below,
  which drops `promote` for being unaffordable alongside the other two).

## Two of five lanes are unpriced, not silently dropped

`item` (20 Embers, "choose 1 of 3 offered") and `hero` (18 Embers, "buy 1
hero ability node") — the `budget_note`'s own "real temptations" — have no
representable effect in this harness. Neither is invented a damage number
here. `docs/sim/LIMITATIONS.md` §4 already flags 5 of 6 hero abilities as
inert in `variety.py` for the same structural reason (no primitive in a
pooled attrition model for burst windows, auras, stealth); the `item`
catalog (`upgrades.json`) is worse off still — its pool isn't even resolved
to a specific offered triple, only a category. **Two of five lanes,
including both of the file's own named real temptations, cannot be priced
by this harness.** That is itself the finding for those two lanes, not a
gap papered over with a guess.

## The measurement — TERTIARY (the only condition either stop is reached)

Actual output, `py Scripts/sim/decisions.py`:

```
=== TERTIARY -- HYPOTHETICAL (MaxAttackersPerUnit=1 AND supply capacity=120, see DECISIONS.md) ===
  branch           target                       waves_surv final_ret   killed  embers
  hoard            (none)                                2       0.0    709.5   90.94
  recruit_only     recruit                               2       0.0    778.8   73.88
  promote_only     promote                               2       0.0    859.0   75.90
  provision_only   provision                             2       0.0    709.5   70.94
  triangle         promote,provision,recruit             2       0.0    797.4   40.74
  spread(final_retinue) = 0.00  (0.0 - 0.0)
  spread(total_killed)  = 149.56  (709.5 - 859.0)
    [hoard          wave 0 stop growth-A target=(none)                   chosen=(none)                   cost=  0.0 embers_after= 35.00]
    [hoard          wave 1 stop growth-B target=(none)                   chosen=(none)                   cost=  0.0 embers_after= 90.00]
    [recruit_only   wave 0 stop growth-A target=recruit                  chosen=recruit                  cost= 12.0 embers_after= 23.00]
    [recruit_only   wave 1 stop growth-B target=recruit                  chosen=recruit                  cost= 12.0 embers_after= 66.00]
    [promote_only   wave 0 stop growth-A target=promote                  chosen=promote                  cost= 15.0 embers_after= 20.00]
    [promote_only   wave 1 stop growth-B target=promote                  chosen=promote                  cost= 15.0 embers_after= 60.00]
    [provision_only wave 0 stop growth-A target=provision                chosen=provision                cost= 10.0 embers_after= 25.00]
    [provision_only wave 1 stop growth-B target=provision                chosen=provision                cost= 10.0 embers_after= 70.00]
    [triangle       wave 0 stop growth-A target=promote,provision,recruit chosen=recruit,provision        cost= 22.0 embers_after= 13.00]
    [triangle       wave 1 stop growth-B target=promote,provision,recruit chosen=promote,recruit,provision cost= 37.0 embers_after= 31.00]
```

Full per-wave breakdown (`--json`, `tertiary_hypothetical_MA1_capacity120`):

| branch | wave0 (vs 250) surv | wave1 start / (vs 450) surv | wave2 start / (vs 700) enemy surv | result |
|---|---:|---:|---:|---|
| hoard | 38.21 | 38.2 → 0.66 | 0.7 → 690.55 | wiped wave 2 |
| recruit_only | 38.21 | 48.2 → 1.23 | 11.2 → 621.23 | wiped wave 2 |
| promote_only | 38.21 | 38.2 → 8.48 | 8.5 → 540.99 | wiped wave 2 |
| provision_only | 38.21 | 38.2 → 0.66 | 0.7 → 690.55 | wiped wave 2 |
| triangle | 38.21 | 48.2 → 1.23 | 11.2 → 602.63 | wiped wave 2 |

## Primary metric: `total_killed`, not `final_retinue` — and why

The task named all three fields; `total_killed` is the one this file
actually uses for the spread/verdict computation, because **`final_retinue`
saturates to a floor of 0.0 for every branch** — all 5 branches lose wave 3
against a 700-enemy population no combination of these 5 policies gets
remotely close to holding, so `final_retinue`'s spread is exactly zero
regardless of whether the branches differ anywhere else. `total_killed` is
continuous across the whole chain and does not get zeroed out by the
eventual wipe — it is the metric that actually shows the branches
differing. `waves_survived` is reported (2 of 3 for all 5 branches — degrade
never binds in this hypothetical, so every branch wins waves 1-2 outright
and loses wave 3) but, like `final_retinue`, does not discriminate here.

## What actually differentiates, and why (an emergent, checked finding)

- **`provision` is measurably inert, exactly — `provision_only` and `hoard`
  produce byte-identical `total_killed` (709.45 both) and byte-identical
  per-wave survivor counts.** Not "small effect" — zero, checked directly
  in the per-wave table (`degrade.dps_multiplier` is `1.0` in every single
  wave, for every branch, including `hoard` at the unmodified capacity=120
  baseline). The mechanism: by wave 2, the retinue has already crashed to
  ~38 of 120 (32%) regardless of branch, so demand never approaches even
  the *unmodified* 120 capacity again, let alone needs `provision`'s extra
  +25/+50. **`provision`'s entire mechanic (raise the ceiling) only pays
  off when demand is near or above capacity for multiple waves — casualties
  in this model are steep enough, every wave, that this never holds past
  wave 1.** This is a data-level observation, not a prescription: something
  about how fast the retinue count falls (relative to how much headroom
  `provision` buys, and how long that headroom needs to matter) makes this
  lane structurally unable to bind in a chain shaped like this one — worth
  the gameplay director's attention, no replacement number implied.
- **`recruit` (+10 raw Freed bodies, 90hp/20dps) is a real, positive,
  moderate lever** — `recruit_only` kills 69.3 more brood than `hoard`
  (778.8 vs 709.5) for 24 Embers spent across both stops (vs `hoard`'s 0).
- **`promote` (up to 20 units, one tier up, zero upkeep cost) is the
  standout lever** — `promote_only` kills 149.6 more brood than `hoard`
  (859.0 vs 709.5, the FULL measured spread) for only 30 Embers spent, the
  cheapest of the three single actions to use twice. The mechanism traces
  cleanly: promoting Militia (130hp/30dps) to Veteran (190hp/45dps, +46%/
  +50%) costs no extra Supply demand (`upgrades.json`: uniform 1 upkeep per
  unit across tiers — the file's own stated "quality lane" framing), and
  `redistribute_survivors`' weakest-first casualty rule (`run_sim.py`)
  then protects the promoted, now-strongest row disproportionately across
  the next wave's losses — the two mechanics compound rather than sitting
  side by side.
- **`triangle` (spend on all three where affordable) UNDERPERFORMS
  `promote_only` alone** (797.4 vs 859.0 total killed) **despite spending
  more Embers overall** (59 vs 30) and banking far fewer (40.74 vs 75.90
  final). Traced directly to the affordability rule: at growth-A, the full
  triangle (37 Embers) doesn't fit the 35-Ember bank, so the greedy
  costliest-first drop removes `promote` (the most expensive single action,
  15) — `triangle` never gets its single strongest lever at the stop where
  applying it compounds the longest. This is exactly what "spreading Embers
  across the triangle instead of concentrating them" would predict, checked
  directly rather than assumed.

## Isolating growth-A from growth-B, as far as this data allows

The five branches apply the *same* target set at both stops, so their
`total_killed` spread conflates both stops' contributions. Two things this
data *can* still isolate cleanly, because they land strictly between the
two stops (before growth-B has fired at all):

- **Headcount effect of growth-A, in isolation:** wave-1 (`gate1-
  calibration-wave2`) starting count is 38.2 for every branch except
  `recruit_only`/`triangle` (48.2 — exactly `hoard`'s 38.2 + `recruit`'s
  flat +10). `promote`/`provision` change nothing about raw headcount by
  design (tier reassignment and capacity respectively, not +bodies) — this
  is a clean, single-cause read, not a mixed one.
  - So the same instrument that showed count is the one to check for the
  quality effect: `promote_only` and `hoard` enter wave 1 with the
  IDENTICAL 38.2 headcount, yet `promote_only` exits wave 1 with 8.48
  survivors against `hoard`'s 0.66 — **a ~12x difference in survivors driven
  entirely by growth-A's quality choice, with headcount held exactly equal.**
  That is growth-A's clean, isolated signal: at this stop, quality
  (`promote`) dominates quantity (`recruit`) for surviving the very next
  wave, and `provision` contributes nothing measurable.
- **growth-B's incremental contribution** is visible in the same table one
  column later (wave-2 starting counts: 0.7 / 11.2 / 8.5 / 0.7 / 11.2) and
  in the final `total_killed` deltas, but by this point both stops have
  already fired, so growth-B's isolated marginal effect (holding growth-A's
  choice fixed) is not separated by these 5 runs — a genuine limitation of
  the "one policy across the whole run" branch design, named rather than
  finessed. Resolving it would need the full per-stop combinatorial product
  (up to 8 legal target sets × 2 stops = up to 64 combined branches for
  a factorial read) — out of scope for this task's 5-branch design; flagged
  as the natural next step if a stop-by-stop-isolated read is wanted later.

## Verdicts

| Stop | PRIMARY (committed) | SECONDARY (hypothetical) | TERTIARY (hypothetical) |
|---|---|---|---|
| **growth-A** | **THEATRE** — unreachable, zero branches differ, by construction (wave 1 always wipes first) | **THEATRE** — unreachable for a *new* reason: even a combat-model cell independently known to win outright (`MA=1`) is sunk by the economy's own t=0 oversubscription | **REAL DECISION** — `promote` vs `hoard`/`provision`, headcount held exactly equal, produces a ~12x difference in wave-1 survivors; `provision` measurably contributes nothing |
| **growth-B** | **THEATRE** — unreachable at one more remove (gated behind surviving wave 2, which never happens) | **THEATRE** — same as growth-A | **REAL DECISION, with the same lane weighting** — `promote` remains the strongest lever, `provision` remains measurably inert, `triangle`'s affordability collision demonstrates spend ORDER (not just spend total) materially changes the outcome |

**The committed row is the one that matters for the actual game as shipped
today: THEATRE at both stops.** The TERTIARY column describes what the
*mechanism* would do if two separately-flagged, already-cited problems
(the frontage-model calibration gap, `LIMITATIONS.md` §1; the economy/GATE1
retinue-count collision, task-096's finding (b)) were both fixed — it is
not a claim about the current build, and neither of those two fixes is
this task's to make.

## A further, unprompted caveat on the TERTIARY numbers themselves

`growth-sites.json`'s own `slice_placement` estimates ~33 Embers on arrival
at growth-A and ~52 at growth-B ("~230/~420 brood killed x 0.1 + 10 grant"
— i.e., assuming a near-miss, NOT a total wipe of the enemy side).
TERTIARY's actual Embers income is higher across the board (35 at growth-A
for every branch; 68-90 banked before growth-B's own spend, depending on
how much growth-A already spent) because `MA=1` doesn't just let the
retinue survive — it lets it **wipe the entire enemy population outright**
each of the first two waves (`enemy_survivors: 0.00` both times, every
branch). That is a much more lopsided win than the design's own Ember-pacing
assumption anticipates. **This hypothetical is useful for exercising the
allocation mechanism at all, but its Embers economy runs hotter than
`growth-sites.json`'s own stated pacing — a further reason not to read its
absolute numbers (Embers banked, specific survivor counts) as tuned,
only the relative ranking between branches it produces.**

## Handoff to the gameplay director

Two observations, neither prescribing a replacement number:

1. **`provision`'s capacity-raise mechanic never binds in any run this
   harness can currently produce** — committed defaults wipe before it
   fires at all; the one hypothetical where it does fire shows it
   contributing exactly zero, because retinue headcount falls well below
   even the *unraised* capacity within one wave, every time. Whatever
   `provision` is meant to protect against (a retinue staying close to its
   recruited headcount across multiple waves) does not happen in this
   harness's casualty curve, at any parameter combination tested so far.
2. **The economy/GATE1 collision task-096 already flagged (60 capacity vs
   120 committed retinue-count convention) is potent enough to sink even a
   combat-model cell independently known to win outright** (`MA=1` alone:
   wins; `MA=1` + real 60-capacity degrade: wipes). This sharpens finding
   (b) from "makes an already-losing wave worse" to "can flip an
   independently-winning wave into a loss on its own" — worth weighing
   alongside whatever resolution task-096's finding gets.

## What this task did NOT do

No `docs/data/*.json` file was edited (`combat-model-constants.json`
included — its `MaxAttackersPerUnit=1`/`start_capacity=120` values used
above are in-memory overrides only, applied via
`decisions.py`'s `diagnostic_constants_override`, never written to disk —
verify with `git status` / `git diff` on `docs/data/**` if in doubt). No
constant was tuned to make a branch "look better" — `MA=1` and
`capacity=120` were chosen once, before any branch was run, for being
already-cited/already-committed numbers, and used identically across all
five branches and both isolated-effect reads above. `combat_model.py`,
`scenario_runner.py`, `validate.py`, `data_loader.py`, `variety.py`,
`differentiation.py`, `sweep.py`, `drift_check.py`, and `run_sim.py` are
all unedited — `py Scripts/sim/validate.py` and `py Scripts/sim/
drift_check.py` both still pass (checked after this task's changes).
