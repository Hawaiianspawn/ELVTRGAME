# Seven vs Boss — measuring castle-layout.md §6.3

task-143. Point-target scenarios only (`docs/sim/LIMITATIONS.md` §3's evidence
bar), all against `brood_boss` (MaxHP 6000, Armor 14, SurroundCapEstimate 45,
range 35-55), `HeroPresent: false` throughout — Q13=C retires `HeroDamage` as
the player's own damage, so adding a separately-damaging Hero fighter would
double-count a mechanic the pivot removed.

## Headline

**§6.3's claim — "seven specialists inside the cap are not meaningfully
worse than seventy" — does not survive a same-tier control test, at any cap
value in the Boss's documented 35-55 range.** It only becomes true at
`SurroundCapEstimate <= 7`, which is below the documented range and which
`entity-tiers.md` §4 already flags as an unmeasured Fermi estimate. Read as
literally as possible (same troops, only the headcount differs), mass wins
by 5.0x-7.9x across the whole documented band.

## 1. Baseline pair (same tier, isolates the cap mechanic alone)

7 Militia Spearmen vs 70 Militia Spearmen vs the slice Boss, clean.

```
py Scripts/sim/scenario_runner.py seven-militia-vs-boss-point-target
py Scripts/sim/scenario_runner.py seventy-militia-vs-boss-point-target
```

| Squad | Engaged | TTK |
|---|---|---|
| Seven Militia | 7/7 | 59.34s |
| Seventy Militia | 45/70 (cap-limited) | 9.23s |

Seventy is **6.4x faster** — exactly the 45/7 engaged-count ratio, because
at equal tier the model is pure headcount-up-to-cap. Not close.

## 2. Cap sweep (§4's stated 35-55 range, plus below it)

```
py Scripts/sim/sweep.py seven-militia-vs-boss-point-target  --axis "entity-tiers:tiers[Name=brood_boss].SurroundCapEstimate=7,10,20,35,45,55,70"
py Scripts/sim/sweep.py seventy-militia-vs-boss-point-target --axis "entity-tiers:tiers[Name=brood_boss].SurroundCapEstimate=7,10,20,35,45,55,70"
```

| SurroundCapEstimate | Seven TTK | Seventy TTK |
|---|---|---|
| 7 | 59.34s | 59.34s |
| 10 | 59.34s | 41.54s |
| 20 | 59.34s | 20.77s |
| 35 | 59.34s | 11.87s |
| 45 (committed default) | 59.34s | 9.23s |
| 55 | 59.34s | 7.55s |
| 70 | 59.34s | 5.93s |

Seven's TTK is flat — 7 is always ≤ cap, so the cap never binds for it.
Seventy's falls monotonically. **The flip point is exactly cap = 7**: at
cap = 7 both scenarios land on the identical 59.34s, because seventy is
capped down to the same 7 engaged bodies as the seven. At every cap value
in the documented 35-55 range, seventy wins by 5.0x-7.9x — the answer does
not turn on where exactly within 35-55 the real number lands, it is stable
and unfavorable across the whole documented band. It only flips outside
that band, at cap ≤ 7, which is not what §4 states.

**This makes the in-engine cap measurement (`PREFLIGHT.md` §4 P2) more
load-bearing for §6.3's claim than it was for the wave-attrition question
task-068 already tested** — task-068 found arrival timing didn't matter to
its outcome either way; here, whether the real cap is 7-ish or 45-ish is the
entire difference between the claim holding and failing.

Raw headcount parity does not make seven "not meaningfully worse" — closing
the gap needs a quality/tier difference (§6.3's own "specialists" framing).
Not measured here — that's a tuning question, not this task's scope.

## 3. Mark counters

**Quilled** (armor that "scales against ranged specifically," melee answers
it) — `army_ttk_vs_point_target` applies the target's Armor identically to
every attacking role; the harness has no per-attacker-type Armor. No literal
role-conditional mark is expressible. What the existing flat-Armor mechanic
*can* show: bumping the Boss's one Armor value and comparing an all-melee
squad's degradation to an all-ranged squad's, since flat subtraction already
blunts a low-Blow attacker (Archer) harder than a high-Blow one
(`entity-tiers.md` §2.2's documented mechanism).

```
py Scripts/sim/sweep.py seven-melee-veteran-vs-boss-point-target  --axis "entity-tiers:tiers[Name=brood_boss].Armor=14,20,28,36,44"
py Scripts/sim/sweep.py seven-ranged-veteran-vs-boss-point-target --axis "entity-tiers:tiers[Name=brood_boss].Armor=14,20,28,36,44"
```

| Armor | Melee (7 Veteran Spearmen) TTK | Ranged (7 Veteran Archers) TTK |
|---|---|---|
| 14 (baseline) | 29.11s | 74.90s |
| 20 | 37.63s | 179.40s |
| 28 | 61.71s | 257.14s (chip-floored) |
| 36 | 171.43s | 257.14s (chip-floored) |
| 44 | 257.14s (chip-floored) | 257.14s (chip-floored) |

Direction confirmed: melee degrades far more slowly (ranged is already
chip-floored by Armor 28; melee doesn't floor until Armor >= 44). **This is
illustrative sensitivity, not a Quilled implementation** — no
Quilled-specific Armor magnitude is committed anywhere in `docs/data`, and
because the mechanism is one flat Armor value, high enough Armor eventually
floors melee too (the Armor 44 row): **a single flat Armor value cannot
express a mark that hits ranged only**, only one that hits ranged worse.

**Ram** ("prefers structure over bodies, interception answers it") — NOT
expressible. This is a targeting/pathing mechanic (what the boss attacks —
a gate vs. a body), not reducible to DPS/HP/Armor/TargetsPerHit;
`docs/sim/LIMITATIONS.md` §4 already lists chokepoints/positioning as
entirely outside this harness. No proxy invented.

**Sated** ("regeneration between engagements") — also NOT expressible, for
two independent reasons: (1) no regen-rate number is committed anywhere in
`docs/data` — inventing one would be exactly the fitting shortcut the
harness's own guard rules forbid — and (2) "between engagements" describes
downtime across *separate* fights over time; `army_ttk_vs_point_target` is a
single continuous closed-form snapshot with no time axis at all
(`entity-tiers.md` §4's own stated assumption: "fought clean, full army
committed at t=0"). There is no gap in this model for regen to happen in,
independent of the missing number.

## 4. Composition (uniform 7 vs split across archetypes)

```
py Scripts/sim/scenario_runner.py seven-melee-veteran-vs-boss-point-target
py Scripts/sim/scenario_runner.py seven-mixed-composition-vs-boss-point-target
```

| Squad | TTK |
|---|---|
| Uniform (7 Veteran Spearmen) | 29.11s |
| Split (4 Veteran Spearmen + 2 Veteran Archers, 1 slot omitted) | 42.65s (+47%) |

SIMPLIFICATION (stated in the scenario's own `Notes`): only 2 of the 4
surviving archetypes (Vanguard ~ melee Spearmen, Pathfinder ~ ranged Archers)
have any committed combat stat block. Relickeeper has no numbers at all and
is not represented even at zero. Lampbearer is represented as the missing
7th body — `CLASSES.md` §4 states its zero-damage design intent explicitly
("The Guided don't hold lines or deal real damage"), so omitting it from the
DPS sum isn't an invented number, just an honest gap — but this means the
scenario is a 6-damaging-body test standing in for a 7-body roster, not a
full 4-archetype composition.

**Read on Q13=C's distinctness requirement:** at least in this harness,
distinctness has a real, measurable boss-TTK cost — a specialized 7 is
slower against a single point target than a uniform 7 of the same tier. It
is not free, and not purely a legibility argument. Whether that cost is
worth paying is a design call this doc does not make.

## Clean-fight caveat

Every number above assumes the fight is clean (`entity-tiers.md` §4's own
stated lower-bound assumption — full squad committed at t=0, nothing else
competing). §6.3's actual fiction has the seven arriving mid-fight, into an
already-engaged line, possibly mid-Breaking. This harness cannot express
that (no arrival/concurrent-engagement primitive for point-target, the same
gap `LIMITATIONS.md` §3 already names) — every TTK above is optimistic for
the seven specifically, not what a real encounter would produce.

## Verdict

§6.3 is contradicted at the harness's committed defaults and across its
entire documented cap range. Whether and how to revise §6.3 is the gameplay
director's / owner's call, not made here.

## Scenarios

- `docs/data/scenarios/seven-militia-vs-boss-point-target.json`
- `docs/data/scenarios/seventy-militia-vs-boss-point-target.json`
- `docs/data/scenarios/seven-melee-veteran-vs-boss-point-target.json`
- `docs/data/scenarios/seven-ranged-veteran-vs-boss-point-target.json`
- `docs/data/scenarios/seven-mixed-composition-vs-boss-point-target.json`

`py Scripts/sim/validate.py` passes every gating check (1, 2, 4, 5, 6, 7).
