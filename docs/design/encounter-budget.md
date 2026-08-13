# Encounter budget — per-floor density, composition, and room-type spend

**What this is:** the per-floor encounter budget table called for by
`SYSTEMS.md` §4 ("Full encounter/density budget tables per wave remain to be
tuned in play") and `RTS-VERTICAL-SLICE.md` §4's still-open checklist item
("Encounter budget table per floor (density, wave composition, spike/breather
rhythm — hand-tuned, no director AI)"). It answers four things: how much of a
floor's locked population lands in which procgen room, how that population
*arrives over time* inside the main fight (a mechanic that does not exist yet
— see §2), an optional risk-room bonus pocket, and a proof that all of it fits
inside the measured entity ceiling.

**Extends:** `docs/design/entity-tiers.md` (task-002, stat blocks — read only,
unchanged) and `docs/design/scaling-curve.md` / `docs/data/scaling-curve.json`
(task-003, which this doc depends on directly: the locked 250/450/700
population, the floor-by-floor Fodder/Soldier composition percentages, and the
Elite/Boss instance schedule are all consumed here, not re-decided). Also
extends `GDD.md` §9 (procgen: every floor needs ≥1 arena, ≥1 decision event,
≥1 optional risk room) and reads `docs/GATE1-FUN-PROTOTYPE.md` for the shipped
wave/breather structure this doc's floor-1 row deliberately preserves
unchanged, and `docs/perf/BUDGETS.md` for the measured entity ceiling.

**Does not change:** any locked population total, composition percentage, or
Elite/Boss instance count — those stay exactly as `scaling-curve.json` states
them. This doc only adds a spend rule (which room gets how much of the locked
number) and a timing rule (in what order and how fast it arrives).

---

## 1. Reading the shipped baseline correctly, first — correcting my own first draft

Before adding anything, it's worth stating plainly what `Spike1GameMode.cpp`
already does, because it's easy to mis-read it (this doc did, on first pass —
see below). `BeginWave()` spawns a wave's **entire** population in one
`SwarmSpawn::SpawnBrood()` call at t=0 — every brood entity in a wave is
*created* on frame one. There is no per-wave "trickle N entities in over T
seconds" scripting anywhere in `Spike1GameMode`.

**That is not the same claim as "every brood is in contact from frame one,"
and conflating the two was this doc's own error in an earlier pass, corrected
after checking `SwarmCommands.cpp`'s spawn implementation directly.**
`SpawnSwarm()` places brood in a **rank formation**, not a point cloud: the
front rank spawns at `Swarm.BroodSpawnRadiusMin` (2500uu from the hero) and
leads the wave; later ranks step outward by `Swarm.BroodFormation.RankSpacing`
(140uu) and — moving at `Swarm.BroodSpeed` (320uu/s) — physically arrive later.
This is real, shipped, cited arrival-timing data that was previously in no
committed doc. §2a below gives the numbers. `scaling-curve.md` §3's discarded
Lanchester model attributed gate-1's survivability to "encounter pacing (spawn
rate and arrival timing over a wave)" — that diagnosis was closer to right
than this doc's own first draft gave it credit for: the timing isn't a
scripted spawn-*rate*, it's a geometric **travel-time** stagger baked into the
existing rank-formation spawn, and it had simply never been computed and
written down anywhere before now.

This matters for how to read §2's pulse schedule: floors 2–3's pulses are a
**new, additional** layer of arrival pacing (spawning separate batches at
different times), stacked on top of a rank-arrival stagger that **already
exists inside every single pulse, floor 1 included**. Floor 1 isn't
"un-paced" — it has exactly the rank-arrival stagger §2a computes, and nothing
more.

---

## 2. Spike/lull pacing inside the main Arena — new, and floor-1 is the control

**DECIDED (this doc) — floor 1 is left exactly as shipped: one pulse, the
entire 250 at once.** It is gate-1's own measured calibration point
(`GATE1-FUN-PROTOTYPE.md` §3, "lost by default, lost narrowly, low variance")
and changing its arrival shape would invalidate that baseline for no gain —
floor 1 is meant to feel like the fodder rushing you all at once anyway
(`entity-tiers.md` §6: fodder's identity is "chaos," not "organized"). Floors
2 and 3 introduce **pulses**: the floor's locked population arrives in 2–3
batches with a short **Lull** between them, rather than in one lump.

**Naming note, load-bearing for implementation.** `Spike1GameMode.h` already
has `ERunPhase::Breather` meaning "wave cleared, growth site reached." The gap
between pulses *within* one floor's still-active Arena is a different,
smaller-grain thing and is called **Lull** here specifically so it doesn't
collide with that enum or with `SYSTEMS.md` §4/§7's "breather" language, which
stays exactly what it already means (the growth-site beat, unchanged). Lull
has no HUD panel, no Ember grant, no retinue refill — it's a few seconds of
"the tide recedes," a legibility beat, not a decision beat. This mirrors the
existing two-grains relationship `SYSTEMS.md` §4 already established for the
economy/decision-event axis (continuous grain vs. authored-event grain) —
Lull is the continuous grain of *pacing*, the growth-site breather is the
authored-beat grain of it.

### The schedule

Composition within a pulse holds the floor's overall Fodder/Soldier-melee/
Soldier-ranged ratio (`scaling-curve.json floor_roster`) — pulses reshuffle
*timing*, not *mix*. Full per-pulse numbers: `docs/data/encounter-budget.json`
`pulse_schedule[]`.

| Floor | Pulses | Split | Elite trigger |
|---|---|---|---|
| 1 | 1 | 250 (100%) | — |
| 2 | 2 | 270 (60%) → 180 (40%) | 1 Elite, start of pulse 2 |
| 3 | 3 | 210 (30%) → 245 (35%) → 245 (35%) | 1 Elite start of pulse 2, 1 more start of pulse 3 |

**Elite placement is the one deliberate exception to "pulses don't reshuffle
composition."** Both floor-2 and floor-3's first Elite arrive at the *start*
of the pulse that follows the first Lull — after the player has already
absorbed an opening wave and had a moment to breathe, not at t=0 while still
getting oriented. Floor 3's second Elite arrives at the start of the third
(final, heaviest) pulse — the fight gets hardest right as the player is about
to reach the boss room, which is the intended shape (`RTS-VERTICAL-SLICE.md`
§5: the boss is "positioning + stances, not a DPS check," so the *approach* to
it should be where the DPS-check pressure peaks, not the boss itself).

`Swarm.EliteLeadSeconds` (proposed, default **1.5s**): an embedded Elite spawns
this many seconds *before* the rest of its pulse's swarm bodies. Its own
1.8s `SwingInterval` telegraph (`entity-tiers.json`) needs a moment where it's
the only new thing on screen, or its first swing reads as indistinguishable
from the fodder arriving around it — Design Law 6 applied to arrival timing,
not just to the swing pose itself.

`Swarm.PulseLullSeconds` (proposed, default **3.0s**) sits between pulses. It
is shorter than `BreatherSeconds`'s sibling concept because nothing mechanical
happens during it — no growth site, no refill, no HUD panel — it's purely "the
crowd thins, there's a beat of quiet, then it doesn't." A first-pass number,
not measured; it should be judged by feel once pulses exist on screen, same
status as every other timing constant in this file.

### Why this validates (and sharpens) an existing caveat rather than creating a new one

`scaling-curve.md` §4 already flagged that its Elite/Boss TTK table is a
"clean 1-on-1... lower bound, not a prediction" once an Elite is embedded in a
live swarm. The pulse schedule makes the *mechanism* for that concrete: when
floor 2's Elite spawns at the start of pulse 2, some of the retinue is still
finishing off pulse-1 stragglers and hasn't fully regrouped onto the Elite yet
— the committed-army-at-t=0 assumption that TTK table used doesn't hold in the
pulsed encounter shape. This doc doesn't re-run that sim (it isn't the
deliverable here), it just confirms *why* the caveat scaling-curve.md already
stated is real once arrival timing is specced, and hands the "needs a
concurrent-spawn model" ask forward with a concrete mechanism attached instead
of a general worry.

---

## 2a. Rank-arrival timing — the real, shipped arrival curve (added after `docs/sim/LIMITATIONS.md` §1–§2)

**This subsection answers a direct request, not a self-assigned extension.**
`docs/sim/LIMITATIONS.md` §1 names "arrival/spawn-pacing timing" as one of two
undisentangled candidates for why the sim harness's wave-attrition model
cannot reproduce `GATE1-FUN-PROTOTYPE.md`'s measured ~110-of-120 wave-1
survival, and states plainly: "no committed data file anywhere in the repo
says at what rate brood arrive over the course of a wave." §2 above computed
one *new* arrival mechanism (pulses); this subsection computes the arrival
timing that **already exists in the shipped game today**, inside every single
spawn call, pulses or not — the number LIMITATIONS.md was actually asking for.

### The mechanism, cited exactly

`SwarmCommands.cpp::SpawnSwarm()` places a wave's brood in **ranks**, not a
point cloud: rank 0 (`Swarm.BroodFormation.Columns` = 60 brood) spawns at
`Swarm.BroodSpawnRadiusMin` (2500uu from the hero) and leads the wave; each
later rank steps outward by `Swarm.BroodFormation.RankSpacing` (140uu).
Every brood then closes on the hero at `Swarm.BroodSpeed` (320uu/s, ±6%
`Swarm.BroodSpeedJitter`). **None of these five numbers were picked for this
doc** — they are the CVar defaults already shipped and already what
`GATE1-FUN-PROTOTYPE.md`'s measured baseline ran against.

```
retinue_radius(N)  = FormationSpacingUU x sqrt(N / pi)         [86uu, GATE1 §3a measured]
rank_radius(k)     = BroodSpawnRadiusMin + k x RankSpacing
travel(k, N)       = max(0, rank_radius(k) - retinue_radius(N) - FodderEngageRange)
ArrivalSeconds     = travel / (BroodSpeed x (1 +/- BroodSpeedJitter))
```

`retinue_radius` uses the exact same disk-packing formula
`Scripts/sim/combat_model.py`'s `exposed_frontage()` already uses — not a new
Fermi technique invented for this doc.

### The number, for the harness's own calibration fixture

Applied to `docs/data/scenarios/gate1-calibration-wave1.json` (120 retinue,
250 Fodder, the harness's own committed validation target — I did not create
or edit this scenario):

| Rank | Bodies | Arrival (nominal) | Arrival (±6% jitter) |
|---|---|---|---|
| 0 (front) | 60 | **5.85s** | 5.52–6.23s |
| 1 | 60 | 6.29s | 5.94–6.69s |
| 2 | 60 | 6.73s | 6.35–7.16s |
| 3 | 60 | 7.17s | 6.76–7.62s |
| 4 (back) | 10 | 7.60s | 7.17–8.09s |

**No brood is in melee contact range before ~5.85 seconds.** The harness's
`simulate_wave_attrition` currently treats the full 250-strong enemy
population as alive and eligible to engage from `t=0` — this data says the
true figure for the first ~5.85s is **zero**, and the full population isn't
present until ~7.6s. Over a fight the harness's own 27-cell sweep
(`docs/sim/VALIDATION.md`) puts in the 10–20+ second range at its committed
defaults, a "no incoming damage for the first ~6 seconds, then a ~1.75s
ramp-in" curve is not a rounding error — full per-rank breakdown for the
calibration fixture plus every pulse in §2's schedule:
`docs/data/encounter-budget.json` `rank_arrival_context[]` /
`rank_arrival_timing[]`.

### What this is, and isn't, an answer to

**This is real, cited, shipped-default data — not a number chosen to make a
model land on a desired answer**, the exact thing LIMITATIONS.md §1 warns
against. Every input traces to a named CVar default; if those defaults are
retuned, the numbers go stale and must be recomputed from the formula above,
not hand-edited. I am **not** claiming this closes the harness's gap — the
harness's own best untested cell only reaches ~53 of 120 survivors against a
measured 109–111, and this data doesn't say how much of the remaining
distance a ~6-second contact-free window closes versus LIMITATIONS.md's
candidate #2 (`MaxAttackersPerUnit`'s pooled-vs-per-entity transfer). Stating
that plainly rather than asserting either way, per the same house convention
`entity-tiers.md` §7 and `scaling-curve.md` §3 both already use for open
questions.

### Checked whether the harness can consume this today — it cannot, without a code change outside my file boundary

I read `Scripts/sim/combat_model.py::simulate_wave_attrition` and
`scenario_runner.py` before writing a scenario file. **`simulate_wave_attrition`
has no arrival-time parameter at all** — every `WaveGroup`'s `count` is
treated as fully alive and contending for frontage from `t=0`, regardless of
what a `docs/data/scenarios/*.json` file says. Writing a scenario file with
this doc's arrival numbers in it would not change the model's behavior in any
way — the harness would silently ignore them and produce the same already-known
full-wipe result, which would misrepresent tested-and-passed as though arrival
timing had actually been exercised. I did not write one for that reason.

**What would make this data usable**, for whoever owns `Scripts/sim/`
(sim-director, not me — `combat_model.py`/`scenario_runner.py`/
`docs/data/scenarios/` are outside this doc's file boundary): split each
`WaveGroup` into per-rank sub-groups carrying an `ArrivalSeconds` field (the
`rank_arrival_timing[]` rows above are already shaped for exactly this — one
row per rank, per context), and gate a sub-group's contribution to
`enemy_melee_alive`/frontage/damage in the per-tick loop behind
`t >= ArrivalSeconds`. `docs/sim/LIMITATIONS.md` §2 already anticipated this
exact shape: "it plugs into `simulate_wave_attrition`'s per-tick loop as an
arrival-rate term without restructuring." I'm handing over the data half of
that; the loop change is a `Scripts/sim/` edit I'm not making.

---

## 3. Room-type budget spend

GDD §9: every floor needs ≥1 arena, ≥1 decision event, ≥1 optional risk room;
`RTS-VERTICAL-SLICE.md` §5's bill of materials adds corridors and (floor 3
only) a boss room, totalling "~12–15" room prefabs across the slice with "≥3
arenas." Full table: `docs/data/encounter-budget.json` `room_types[]`.

| Room type | Population source | Per floor | Combat? |
|---|---|---|---|
| **Arena** | Locked total (`scaling-curve.json`) | 1 | Yes — hosts the pulse schedule above |
| **Corridor** | none | 2–3 (a procgen layout concern, not budgeted here) | No |
| **Decision site** | none | Floors 1 & 3 only (see below) | No |
| **Risk room** | Bonus, on top of the locked total | 1 | Optional, player's choice |
| **Boss room** | none (Boss is a `PromotedActor`, not population) | Floor 3 only, 1 | Yes — isolated, no concurrent swarm |

**DECIDED (this doc) — decision sites land on floors 1 and 3, not 2.** The
slice builds exactly 2 authored decision events (`RTS-VERTICAL-SLICE.md` §5:
"one fork, one sacrifice" — the narrative content behind them is stale per
`WORLD.md`'s supersession, but the *slot count* isn't). Floor 2 is already the
curve's steepest escalation step (`scaling-curve.md` §1: "the single biggest
composition swing," ×1.8 population) and introduces the first Elite — giving
it a decision beat too would stack three new pressures (harder swarm, new
enemy type, authored choice) on one floor while floor 1 and 3 carry one new
pressure each. Floor 1's fork is the low-stakes intro beat; floor 3's
sacrifice sits *before* the Arena, the penultimate-stakes choice right ahead
of the escalation-into-boss sequence. This is a room-graph placement call, not
a narrative one — the actual event content stays the narrative-director's to
write once faction/biome canon exists.

**Sanity check against the stated room-prefab budget.** 3 Arenas + 3 Risk
Rooms + 2 Decision Sites + 1 Boss Room = 9 named "encounter" rooms; the
remaining ~3–6 of the stated "~12–15" total are corridors, which lines up with
2–3 corridors per floor. Not a proof, just a consistency check that this
doc's room count doesn't quietly blow the bill-of-materials budget.

### Risk room — optional, bonus, legible

**DECIDED — Risk Room population is additive, never sliced from the locked
total**, and is **pure Fodder only**, sized at roughly 8% of the floor's
locked population (Floor 1: +20, Floor 2: +35, Floor 3: +55 —
`docs/data/encounter-budget.json` `risk_room_budget[]`). Two reasons for the
pure-Fodder restriction: it keeps the room's difficulty legible from its size
alone before the player commits (no Soldier/Elite ambush hiding behind a
small-looking pocket), and it means clearing it is a pure quantity, not
composition, decision — appropriate for an optional side room that shouldn't
need the same read-the-room care as the mandatory Arena. Reward is a
guaranteed Loot v0 roll (`docs/data/loot-v0.json`) plus the standard per-kill
Ember rate (`economy.json`), not a new reward table.

**DECIDED — Risk Room is reached only before the Arena's first pulse
triggers**, via a side corridor off the entrance path. This avoids any
question of whether the Arena's pulse clock keeps running while the player is
elsewhere (it simply hasn't started) — the simplest rule that still gives the
room's name ("risk") real meaning: time spent here is time not spent
preparing/positioning for the harder fight ahead, without needing a director
AI to arbitrate a live interruption. Consistent with `SYSTEMS.md` §5's
"pacing is hand-authored... no director AI yet" constraint.

---

## 4. Entity ceiling check — **PASSES with wide margin, stated explicitly**

Every peak-concurrency number below is a **worst case**: it assumes zero
deaths since the encounter began (a Lull ran short, nothing from the prior
pulse died) and, pessimistically, that the Risk Room's bonus pocket is
concurrent with the Arena even though §3 designs it not to be. Full table:
`docs/data/encounter-budget.json` `peak_concurrency_check[]`.

| Floor | Swarm | Retinue (worst of both growth scenarios) | Elites | Risk-room bonus (worst case) | **Total peak** | % of measured 20k Niagara sweep | % of 34k SimLOD4 60fps ceiling |
|---|---|---|---|---|---|---|---|
| 1 | 250 | 40 | 0 | 20 | **310** | 1.55% | 0.91% |
| 2 | 450 | 60 | 1 | 35 | **546** | 2.73% | 1.61% |
| 3 | 700 | 90 | 2 | 55 | **847** | 4.24% | 2.49% |
| 3 (boss room, separate) | 0 | 90 | — | — | **91** | 0.46% | 0.27% |

Even floor 3's worst-case peak (847) sits under **5%** of both measured
ceilings in `docs/perf/BUDGETS.md` — the 20,000-sprite Niagara sweep (never
GPU-bound in that data) and the 60fps-with-`Swarm.SimLOD.Stride 4` ceiling
(~34,000). **The encounter budget does not need to trade density for
headroom anywhere in the 3-floor slice** — there is no scenario in this doc
that comes close to the measured constraint. Stated plainly per the brief's
own instruction to say so explicitly rather than assume it.

---

## 5. What this doc does not resolve

**scaling-curve.md §3's headcount-vs-population gap is untouched by this
doc, and pulsing doesn't fix it.** That finding — the growth-site economy's
40/50/60-90 retinue never gets near gate-1's calibrated 120-count survival
band — is about *how many bodies the economy produces*, not about *when
enemies arrive*. Spreading a floor's population into pulses changes the shape
of the fight (a Lull gives a thin army a moment to regroup, which likely
*helps* at the margin) but doesn't change the fact that floor 1 alone starts
at a population/army ratio already past gate-1's own measured collapse point.
This doc isn't positioned to close that gap (same authorization boundary
scaling-curve.md stated: it belongs to whoever next revises `SYSTEMS.md` §2 or
§7), and a pulse schedule tuned to hide it would be tuning around a
known-bad number instead of fixing it. Flagged forward, not quietly patched
over.

**No encounter-budget change compensates for melee's surround cap.**
`entity-tiers.md` §4 / `scaling-curve.md` §4's finding — melee DPS against a
single Elite/Boss point-target is capped, all army-size scaling against those
targets comes from Archers — is orthogonal to pulse timing and unaffected by
anything in this doc.

---

## 6. Narrative requests

No new entities. Two feel-facing requests for whoever's picking up readability
work next (audio-minimal's task-029 is the natural owner of the first):

- **Spike/lull needs an audible tell, not just a visual one.** A pulse landing
  reads instantly by sight (a wall of new sprites), but the *lull* — the 3s
  quiet beat between pulses on floors 2–3 — needs its own audio cue (a
  recede/hush) or it just reads as "nothing happening" rather than "the
  breath before the next wave." Gameplay ask: distinguishable from the
  Elite's own pre-pulse lead-in cue (next bullet) and from the leash-warn
  audio tick already speced in `GATE1-FUN-PROTOTYPE.md` §2.
- **The Elite's 1.5s lead-in before its pulse's swarm needs its own
  telegraph**, separate from its swing windup. What a player must feel in
  that 1.5s: *this one arrived on its own, deliberately, ahead of the rest* —
  reinforcing entity-tiers.md §6's "disciplined, the opposite of the fodder's
  chaos" read, now expressed as an arrival beat rather than only a combat
  pose. Readability constraint to carry forward: must be distinguishable at
  the same horde density the swing telegraph already has to clear (Design
  Law 6).

---

## 7. Handoffs

**To whoever implements `Spike1GameMode`'s wave director.** `BeginWave()`
needs to become pulse-aware: N calls to `SwarmSpawn::SpawnBrood()` per floor
(per `pulse_schedule[]`), gated by `Swarm.PulseLullSeconds`, with the Elite's
own spawn call offset `Swarm.EliteLeadSeconds` ahead of its pulse. Floor 1
needs no change — its single pulse is the existing `BeginWave()` call
untouched.

**To whoever builds the procgen room-graph generator (`RTS-VERTICAL-SLICE.md`
§5's `[ ]` item).** `room_types[]` is the population-budget input; room
adjacency/layout (which corridor connects to which room) is this doc's
explicit non-scope — a procgen/level-design call, not a numbers one.

**To the narrative-director**, per §6 above, once faction/biome canon lands:
the two decision-site slots (floor 1, floor 3) need real content; this doc
only reserves the room and states why those two floors.

**To whoever revises `SYSTEMS.md` §2 or §7** (already an open item from
`scaling-curve.md`): §5 above restates that this doc doesn't close the
headcount gap and isn't trying to.

**To sim-director (`Scripts/sim/`, `docs/sim/`, `docs/data/scenarios/`) —
new, per `docs/sim/LIMITATIONS.md` §1–§2.** §2a's `rank_arrival_context[]` /
`rank_arrival_timing[]` is real, cited arrival-timing data — the exact gap
LIMITATIONS.md §1 names as missing and §2 anticipates plugging into
`simulate_wave_attrition`'s per-tick loop "as an arrival-rate term without
restructuring." Concretely: split each `WaveGroup` into per-rank sub-groups
carrying an `ArrivalSeconds` field, gate their contribution to
`enemy_melee_alive`/frontage/damage behind `t >= ArrivalSeconds`. The
`gate1_calibration_wave1` context is sized to attach directly to the
harness's own existing `gate1-calibration-wave1.json` fixture (unedited by
this doc) — that's the one scenario worth re-running once the code change
lands, since it's the harness's own calibration target. I did not write a
scenario file myself: `combat_model.py`/`scenario_runner.py` have no
arrival-time parameter today, so a scenario file couldn't express this data
without that code change first (§2a).

---

## 8. Simulation notes

**What was simulated:** a scratch Python script (session scratchpad, not
committed) that (1) computed each floor's pulse split by applying the floor's
locked composition percentages to each pulse's share of the locked total,
verifying the pulses sum back to the exact locked population with no drift,
(2) computed the worst-case peak-concurrency numbers in §4 from
`scaling-curve.json`'s retinue counts and Elite/Boss schedule plus this doc's
risk-room bonus figures, and (3) computed §2a's per-rank arrival timing —
`retinue_radius`/`rank_radius`/`travel`/`ArrivalSeconds` from the formula
cited there — for the harness's `gate1-calibration-wave1.json` context and
every pulse in §2's schedule (32 rank-rows total across 7 contexts). Same
house method as `entity-tiers.md` §7 and `scaling-curve.md` §7 — closed-form
arithmetic over already-decided/measured/shipped constants, no engine run.

**Checked whether the harness (`Scripts/sim/`) could run this floor directly,
per the lead's instruction to prefer that over prose where it applies — it
cannot, and not for a reason this doc can fix.** I read
`combat_model.simulate_wave_attrition` and `scenario_runner.py` before
attempting a scenario file: the model has no arrival-time input at all — every
`Composition` row in a scenario is fully alive and contending for frontage
from `t=0`, regardless of what a `docs/data/scenarios/*.json` file states.
Running my Floor 2 walkthrough (or any pulse) through the harness as-is would
therefore just reproduce the already-known, already-flagged full-wipe result
(`docs/sim/LIMITATIONS.md` §1) — a result the lead's own instruction says must
not be presented as a prediction. Writing a scenario file that the model would
silently fail to honor felt worse than not writing one, so I didn't; §2a hands
the arrival-timing *data* forward instead, in the exact shape
(`rank_arrival_timing[]`) `LIMITATIONS.md` §2 already anticipated needing, for
sim-director to wire in once `simulate_wave_attrition` gains the parameter to
consume it. The walkthrough below is therefore prose, stated as a design
narrative, not a harness output, and does not cite any survivor count.

**One floor walked end to end — Floor 2**, the most representative row (it
has both a multi-pulse Arena and an embedded Elite, unlike floor 1; it doesn't
have a decision site or boss room, unlike floor 3, so it isolates the two
mechanics this doc actually adds):

1. Player arrives at the Floor 2 Arena via the entrance corridor (having
   optionally cleared the Risk Room first — +35 Fodder, +Embers, +loot roll —
   before the Arena's clock starts).
2. **Pulse 1** is called (all 270 entities created at once — 148 Fodder / 68
   Soldier-melee / 54 Soldier-ranged, 60% of the floor's locked 450, same
   55/25/20 ratio as the floor overall) but is **not** instantly a 270-strong
   fight: per §2a's `floor2_pulse1` context, the front rank (60 bodies) isn't
   in melee range for **~6.3s**, and the back rank isn't in until **~8.1s** —
   Soldier-ranged's 700uu `EngageRange` (`entity-tiers.json`) means the ranged
   portion of the pulse starts contributing sooner than the melee rank-arrival
   numbers alone suggest, a detail this doc's closed-form rank model doesn't
   separately account for and flags rather than quietly ignores. No Elite yet.
3. Player fights pulse 1 down as ranks continue arriving. Once it clears (or
   after `PulseLullSeconds` = 3.0s, whichever the eventual implementation
   gates on — an open implementation detail, not resolved here), a 3-second
   **Lull**: no new spawns, an audio/visual recede cue (§6).
4. **Pulse 2** begins: the Elite (`floor2_elite_01`) spawns alone, `1.5s`
   ahead of the rest. For that 1.5-second window it's the only new thing on
   screen — the intended read is "something else is here now," distinct from
   "more of the same arriving."
5. 1.5s later, pulse 2's 180 remaining Mass Entities (99/45/36) land alongside
   it. The Elite is now embedded in a live, only-partially-regrouped swarm —
   exactly the condition `scaling-curve.md` §4 flagged its clean-1-on-1 TTK
   table as *not* modeling; this walkthrough is the concrete shape of that
   caveat, not a new TTK number (this doc doesn't have a concurrent-spawn sim
   to produce one).
6. Floor clears when pulse 2 and the Elite are both down. **Peak concurrency
   at any point in this sequence never exceeds 546** (§4's floor-2 row,
   worst-case) — 2.73% of the measured 20,000-sprite Niagara sweep, nowhere
   near a rendering or sim concern.
7. Player proceeds to the growth-site breather (`growth-B`, `~52` Embers on
   arrival per `growth-sites.json`, unchanged by this doc) before Floor 3.

**Assumptions, stated plainly (none measured in-engine):**
- `PulseLullSeconds` (3.0s) and `EliteLeadSeconds` (1.5s) are first-pass
  readability numbers, the same status as every other timing constant in
  `docs/data/` — not derived from a measurement, to be judged by feel once
  pulses exist on screen. **§2a's rank-arrival numbers are a different
  category** — derived from shipped CVar defaults, not this doc's judgment —
  but still a closed-form Fermi estimate, not an in-engine measurement; see
  §2a's own caveats (retinue treated as a static point-plus-radius, no
  steering/obstacle slowdown).
- The 60/40 (floor 2) and 30/35/35 (floor 3) pulse splits are a hand-picked
  escalation shape (opening pulse smaller than what follows, to leave room
  for the Elite's own beat), not derived from a target survival rate — no
  tool in this repo currently produces one (the gap `scaling-curve.md` §3
  already named: no encounter-pacing data existed before this doc, and this
  doc is a hand-authored first pass at exactly that, not a solved model).
- Peak-concurrency's "worst of both growth scenarios" retinue figures
  (`scaling-curve.json retinue_growth_curve`) inherit every assumption that
  table already stated as unmeasured (illustrative bounding scenarios, not
  optimal-play predictions).
