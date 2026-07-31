# `Scripts/sim/fight_metrics.py` — reading fights.csv (task-107)

`USwarmTelemetrySubsystem` (`ELVTR/Source/ELVTR/Mass/SwarmTelemetry.h/.cpp`) has
been appending one row per fight to `Saved/SwarmTelemetry/fights.csv` since it
was written. Nothing read it — zero consumers anywhere in the repo before this
task. It's the one instrument in this project that **measures** balance
instead of predicting it (see `docs/sim/LIMITATIONS.md` §1 for why the
wave-attrition sim model can't be trusted for absolute numbers); this file
reads it.

`fight_metrics.py` only reads `fights.csv`. It never touches
`SwarmTelemetry.h`/`.cpp`. If a column this task wanted doesn't exist, that's
reported below as a finding, not silently added — see **Known gaps**.

## Running it

```powershell
py Scripts/sim/fight_metrics.py                    # ELVTR/Saved/SwarmTelemetry/fights.csv
py Scripts/sim/fight_metrics.py --file PATH.csv     # any other capture
py Scripts/sim/fight_metrics.py --selftest          # the one runnable check
```

There is no `pip install` step; stdlib only (`csv`, `statistics`), matching
the rest of `Scripts/sim/`.

## Grouping

Two levels, both read straight off columns already in the row — nothing
recomputed or inferred:

1. **Tuning constants** (`retinueHP`, `retinueDPS`, `broodHP`, `broodDPS`,
   `heroHP`, `heroDPS`, `meleeRange`, `maxAttackersPerUnit`) — the
   `SwarmCombatTuning` values `AppendSummaryRow()` embeds in every row
   specifically so a row survives a later balance pass being attributable.
   Runs from different tuning never pool.
2. **Encounter shape** (`startRetinue`, `startBrood`) within a tuning group —
   `fights.csv` has no stable "which design wave was this" id (see Known gaps
   #2), but a 3-straggler mop-up fight and a full 250-brood wave sharing the
   same tuning are still not the same run. Pooling them would make the
   median/spread and outcome-mix numbers this task exists to produce
   meaningless, which is the exact failure the task's own scope notes warn
   against ("a single median across runs with real variance is the same
   mistake as a single-point sim estimate"). This grouping level is not
   explicitly named in task-107's "Done when" list but is required to make
   that list's own guarantees hold against the real capture — see the
   demonstration below, where the 250/450/700-brood GATE1 waves land in
   their own groups automatically.

Within a leaf group: **median and range** (min–max), never a bare mean.
`n=1` groups print the single value tagged `(n=1)` instead of a median that
would imply a distribution that isn't there. Range, not stdev, because with
groups this small (many real leaf groups are n<5) a standard deviation is
noisier than the range itself and implies a normal-ish spread that isn't
warranted.

## Columns — measured vs. derived

All columns below are exactly what `AppendSummaryRow()` writes
(`SwarmTelemetry.cpp`); `fight_metrics.py` invents no new fields.

| Column | Measured / Derived | What it is |
|---|---|---|
| `timestamp`, `fight`, `outcome`, `duration` | measured | wall time, per-session fight counter (resets each capture, see Known gaps #2), `ESwarmFightOutcome`, fight length in seconds |
| `startRetinue`, `startBrood`, `endRetinue`, `endBrood` | measured | alive counts at fight start/end |
| `killedRetinue`, `killedBrood` | measured | run-total deltas over the fight window |
| `exchangeRate` | **derived, by the recorder itself** | `KilledBrood / KilledRetinue`, except when `KilledRetinue == 0` it falls back to `KilledBrood` outright (`FSwarmFightRecord::ExchangeRate()`, `SwarmTelemetry.h:71-75`) — a zero-retinue-lost fight reports its brood kill count directly rather than dividing by zero. `fight_metrics.py` reads this column as-is rather than recomputing it from `killedRetinue`/`killedBrood`, so it inherits that exact convention. |
| `dmgToRetinue`, `dmgToBrood`, `heroDamageTaken` | measured | raw damage totals over the fight window |
| `heroHPStart`, `heroHPEnd` | measured | — |
| `timeToFirstBlood` | measured, **with a sentinel** | seconds to the first kill on either side. Defaults to `-1.0` and is only ever overwritten on an actual kill (`FSwarmFightRecord::TimeToFirstBlood`, `SwarmTelemetry.h:66`) — a fight can end (`RetinueWiped`/`HeroDown`/`Stalemate`) with zero kills on either side and this stays `-1.0`. `fight_metrics.py` excludes `-1.0` rows from the time-to-first-blood spread and reports the excluded count separately (`"N fight(s) drew no blood"`) rather than letting a sentinel drag a median toward zero. |
| `peakHeroContacts`, `peakLeashBroken` | measured | worst single-frame values, not averages, per the header comment ("the average hides the spike that killed you") — **not reported by `fight_metrics.py`**, out of task-107's explicit scope (exchange rate, survivors, duration, time-to-first-blood, outcome mix) |
| `secFollow`, `secCharge`, `secHold`, `secRally` | measured | seconds spent in each `ESwarmStance` (`SwarmCombat.h`) — **not reported**, same scope reason. See Known gaps #4 for what the real capture shows here. |
| `retinueHP`, `retinueDPS`, `broodHP`, `broodDPS`, `heroHP`, `heroDPS`, `meleeRange`, `maxAttackersPerUnit` | measured (config, not outcome) | the tuning group key — see Grouping above |

## Demonstrated against a real capture

`ELVTR/Saved/SwarmTelemetry/fights.csv` on this machine is real played/tested
data, not a toy — and it's **live**: another session was actively playing
and generating new rows in it while this doc was written, so the exact row
count below is a snapshot, not a frozen number. As of the snapshot this
doc's numbers are pulled from: **4880 rows across 3 tuning groups**. Not
committed to the repo (build output, and too large to be a "small,
demonstration fixture" per the task's own rule) — reproduce with
`py Scripts/sim/fight_metrics.py`; expect the row count and the small
mop-up-fight groups to differ slightly on a re-run against a growing file.
The three headline GATE1-wave groups quoted below (n=163/123/89) were stable
across two snapshots taken minutes apart during this task — the growth was
landing in the small continuation-fight groups, not these.

The dominant tuning group (`retinueHP=130, retinueDPS=30, broodHP=60,
broodDPS=14, heroHP=500, heroDPS=55, meleeRange=95, maxAttackersPerUnit=4`,
matching `combat-model-constants.json`'s current defaults) contains the three
GATE1-convention wave sizes as their own encounter-shape groups automatically,
with no wave-id column needed to separate them:

```
  encounter startRetinue=120 startBrood=250 (n=163)
    exchange rate (brood/retinue): 11.905 [range 3.906-250.000, n=163]
    retinue survivors:             99.000 [range 56.000-120.000, n=163]
    enemy survivors:                0.000 [range 0.000-11.000, n=163]
    duration (s):                   9.420 [range 1.770-44.040, n=163]
    time to first blood (s):        0.590 [range 0.010-4.100, n=163]
    outcome mix:                     BroodCleared=141/Stalemate=22 (of 163)

  encounter startRetinue=120 startBrood=450 (n=123)
    exchange rate (brood/retinue): 3.956 [range 1.263-50.000, n=123]
    retinue survivors:             7.000 [range 0.000-111.000, n=123]
    enemy survivors:                0.000 [range 0.000-301.000, n=123]
    duration (s):                   10.570 [range 3.490-26.670, n=123]
    time to first blood (s):        0.330 [range 0.030-1.430, n=123]
    outcome mix:                     BroodCleared=64/RetinueWiped=28/HeroDown=11/Stalemate=20 (of 123)

  encounter startRetinue=120 startBrood=700 (n=89)
    exchange rate (brood/retinue): 5.867 [range 3.072-140.000, n=89]
    retinue survivors:             7.000 [range 0.000-115.000, n=89]
    enemy survivors:                17.000 [range 0.000-363.000, n=89]
    duration (s):                   9.450 [range 2.600-28.330, n=89]
    time to first blood (s):        0.380 [range 0.010-1.330, n=89]
    outcome mix:                     BroodCleared=26/RetinueWiped=21/HeroDown=28/Stalemate=14 (of 89)
```

Reading this straight (no cherry-picking): the outcome mix is exactly the
kind of thing a mean would hide. The 250-brood wave clears almost every time
(141/163 `BroodCleared`, 22 `Stalemate`, **zero** `RetinueWiped`/`HeroDown`).
The 450-brood wave is a real coin-flip fight (64 clear / 28 wiped / 11 hero
down / 20 stalemate). The exchange rate also swings hard within a single
group — the 250-brood group's own range is 3.906 to 250.0, a ~64x spread —
which is exactly why this reader refuses to print a bare mean for it: a
single "11.9" would erase that a nontrivial share of those 163 runs were
lopsided one-kill Stalemates (`killedRetinue=0` → `exchangeRate` falls back
to raw `killedBrood`, sometimes as high as 250) sitting in the same group as
tight even fights.

There are also 60+ small encounter-shape groups under the same tuning
(`startBrood` values like 1, 2, 3, 5, 6, 9, 21, 38, 64, 94, 99, ...,
`startRetinue=0`) — these read as mop-up/continuation fights recorded
immediately after a big wave left a handful of units alive on each side, not
fresh design waves. They're reported the same way (median/range/outcome mix,
`n=1` called out honestly), several as small as `n=1`; run the command above
for the full listing.

Two other tuning groups exist in the same file
(`retinueDPS=22` at n=16, `broodDPS=35` at n=4159) — both kept fully separate
by the grouping, per the task's core requirement.

## Known gaps (findings, not fixed here)

Per the task's hard rule, the C++ recorder is not touched. These are reported
for whoever owns `SwarmTelemetry.h`/`.cpp` next (task file: "the recorder is
owned by whoever takes the N-run loop task"):

1. **`SwarmLeash::Radius` is cited but not carried in `fights.csv`'s tuning
   key.** `WriteSampleHeader()` writes it into the per-sample file's header
   comment (`SwarmTelemetry.cpp:287`) right alongside melee range and
   max-attackers-per-unit, but `AppendSummaryRow()`'s tuning columns
   (`SwarmTelemetry.cpp:341-344`) stop at `maxAttackersPerUnit` and never
   include it. If leash radius is ever retuned independently of the 8
   columns this reader groups by, `fights.csv` summary rows produced under
   the new leash value are indistinguishable from rows produced under the
   old one — the exact "a row without the numbers it was produced under is
   unattributable" failure the file's own header comment (`SwarmTelemetry.h:18-21`)
   says the tuning columns exist to prevent.
2. **`fight` is a per-session counter, not a stable wave id.** It restarts
   at 1 every time `USwarmTelemetrySubsystem` is created (a new PIE/game
   session), so `fight=2` in one capture and `fight=2` in another aren't
   necessarily the same design wave. This reader works around it by grouping
   on `(startRetinue, startBrood)` instead, which recovers the right
   structure for the fixed-size GATE1 waves (250/450/700 always land
   together) but is a heuristic, not a real identifier — two genuinely
   different design waves that happen to start with the same counts would
   still pool.
3. **No seed/spawn-layout column.** Nothing in `fights.csv` records what
   made two same-tuning, same-encounter-shape fights land on opposite sides
   of an outcome (e.g. the 450-brood group's near-even split above). Can't
   currently separate "this tuning is genuinely a coin flip" from "spawn
   layout got unlucky that run" — see also `docs/sim/LIMITATIONS.md` §1 on
   arrival/spawn-pacing being a real, still-undata'd gap in the *simulated*
   model; this is the same gap's shadow in the *measured* data.
4. **Stance columns (`secFollow/secCharge/secHold/secRally`) are present but
   read almost entirely `Follow` in this capture** — at the same snapshot
   (4880 rows), `secFollow` is nonzero in 4797 of them, versus 22
   (`secCharge`), 17 (`secHold`), 57 (`secRally`). Not a missing column, just
   a fact about what this particular capture set exercised: whoever wants
   exchange-rate data broken out **by stance** needs a capture session that
   actually spends meaningful time in Charge/Hold/Rally first, or the stance
   breakdown will be almost all `Follow` by construction.
5. **`outcome` in this capture skews `HeroDown` (3156 of 4880 rows, ~65% at
   this snapshot), more than `BroodCleared` (1048), `Stalemate` (487), and
   `RetinueWiped` (189) combined.** Reporting this as a raw fact about the
   data, not diagnosing it — that's a gameplay-director-facing finding about
   whatever session(s) produced this file, not something this reader tool
   interprets or corrects.
