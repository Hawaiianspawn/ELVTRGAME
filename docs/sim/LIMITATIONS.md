# What this harness is NOT trustworthy for yet

Read this before using any output from `Scripts/sim/` to argue a design
point. Plain statements, not hedges.

## 1. Wave-attrition survivor/casualty counts — NOT trustworthy (the headline gap)

`docs/sim/VALIDATION.md` check 3 fails at the harness's committed defaults:
the wave-attrition model (`floor1-swarm-wave.json`, `floor2-ranged-wave.json`,
`gate1-calibration-wave1.json`) cannot reproduce `GATE1-FUN-PROTOTYPE.md`'s
measured ~110-of-120 wave-1 survival there. It predicts a full retinue wipe
instead.

**This section was rewritten after a review caught a structural bug in an
earlier version of the model** (cleave capacity was wrongly derived from the
same bound as the incoming-attacker cap, which cancelled `TargetsPerHit`'s
effect on the outcome to a constant — see `docs/sim/VALIDATION.md`'s "The bug
that invalidated the original check-3 sweep" for the full account). The
original version of this section reported the failure as robust "across the
harness's entire documented parameter range" — that claim was wrong; the
sweep behind it could not have shown anything else, by construction. Fixed
now, and the honest picture is more specific and more interesting than
either "always fails" or "now it works":

**With the bug fixed, a real 27-cell sweep (`EngagedSpacingUU` x
`MaxAttackersPerUnit` x `MeleeContactFacingFraction`, `docs/sim/VALIDATION.md`)
shows the retinue actually WINNING in 15 of 27 cells** — the model is
capable of the correct qualitative outcome, driven almost entirely by
`MaxAttackersPerUnit`: strict values (1) always win, the shipped default (4)
always loses. **At the harness's actual committed defaults, it still
loses, and even its single best untested cell only reaches about half
(53/120) of the measured 109-111 survivors.** Two live candidates were named
for that remaining gap; **candidate (1) has since been tested (task-068) and
found not to close it. Candidate (2) is untested and, by elimination, now
the stronger explanation of the two — not proof, still needs an in-engine
measurement.**

1. **Arrival/spawn-pacing timing — TESTED (task-068), does NOT close the
   gap.** This candidate previously read "still real, still undocumented in
   any committed data file." That data now exists:
   `docs/data/encounter-budget.json`'s `rank_arrival_timing[]` derives real
   per-rank arrival times from shipped `Swarm.BroodSpawnRadiusMin`/
   `BroodFormation.*`/`BroodSpeed` CVar defaults (not estimated, not
   invented — cited exactly, see that file's `design_constants`). Wired into
   `simulate_wave_attrition` via a per-`WaveGroup` `arrival_seconds` gate
   (`docs/sim/MODEL.md` §3), and run against `gate1-calibration-wave1.json`
   (now split into 5 per-rank rows carrying the real timing): **the retinue
   takes zero damage before the front rank arrives (~5.85s) and elapsed time
   to resolution nearly triples (4.5s -> 11.7s), but the final outcome is the
   same fight, delayed — retinue still fully wiped, enemy survivors ~19-23 of
   250 (vs. 19.73 ungated), robust across the full ±6% `BroodSpeedJitter`
   bracket.** Mechanistic reason, checked directly: at N=250 vs. this
   120-retinue formation, `exposed_retinue x MaxAttackersPerUnit` (~296.8)
   already exceeds the entire enemy population, so the incoming-damage bound
   is set by `enemy_melee_alive` itself, not the frontage cap, for
   essentially any nonzero rank count — arrival timing changes *when* the
   full-population damage rate applies, not what it converges to, because
   nothing in this model depends on elapsed time or accumulated fatigue, only
   current alive-counts. Full numbers: `docs/sim/VALIDATION.md`'s task-068
   section.
2. **`MaxAttackersPerUnit` may not transfer cleanly from the real per-entity
   sim into this pooled/geometric approximation.** In the real sim it bounds
   one specific victim's simultaneous attacker count at an instant; here it's
   used as an aggregate rate multiplier across an entire exposed perimeter
   every tick. Those aren't obviously the same statistical quantity, and this
   harness has no way to check which one is closer to the truth without an
   in-engine measurement to compare against. **Now the more promising open
   candidate of the two**, purely by elimination — candidate (1)'s own
   arrival data was real and still didn't move the number.

**This harness previously could not distinguish between (1) and (2); it now
has real evidence against (1) contributing meaningfully to this model's gap,
but that is not the same as confirming (2) — it just narrows what's left
untested.** Whether arrival timing matters in the REAL per-entity sim (which
has geometry, positioning, and player input this pooled model still lacks —
see §4 below) is a separate, still-open question this harness cannot answer
either way; what's closed here is narrower and more specific: arrival timing
alone cannot rescue *this pooled model's* prediction at these population
scales, because of how its damage-rate math is structured, not because
arrival timing is unimportant in general.

**Do not tune `EngagedSpacingUU`, `MaxAttackersPerUnit`, or
`MeleeContactFacingFraction` in `combat-model-constants.json` to force check
3 to pass.** All three are the documented/cited/midpoint values; picking a
value specifically because it makes a scenario land on 110 would be exactly
the "tuned fudge factor" task-063 was explicit about not wanting — even
though, per the 27-cell sweep, some untested or off-default combinations
technically could be found that pass. That would trade one bad number for a
worse one: a passing check with no citation behind the value that produced it.

## 2. What the wave-attrition model IS still useful for

- **Illustrating the mechanism** — that a concurrency/frontage cap is the
  right *kind* of fix (Linear Law vs Square Law), and that `TargetsPerHit`
  (cleave) and `MaxAttackersPerUnit` (adjacency) need genuinely independent
  derivations rather than sharing one — both real, demonstrated findings now,
  not assumptions.
- **Relative comparisons within itself** — e.g., toggling ranged-enemy
  presence on/off, composition mixes, or `TargetsPerHit` values, and reading
  the *direction* of the effect (now real and checked — `validate.py`'s
  cleave-sensitivity guard — rather than accidentally fixed at zero).
- **Arrival gating is now built and wired in (task-068), not just a future
  scaffold.** `simulate_wave_attrition` takes a per-`WaveGroup`
  `arrival_seconds` gate; `docs/data/encounter-budget.json`'s
  `rank_arrival_timing[]` supplies real, shipped-CVar-derived numbers for it;
  `gate1-calibration-wave1.json` uses them. Section 1 above has the actual
  result: it's real and does something (zero damage before ~5.85s, elapsed
  time nearly triples) but does not close the check-3 gap at this
  population scale. The remaining natural next step is testing candidate (2)
  (`MaxAttackersPerUnit`'s pooled-vs-per-entity transfer) against an
  in-engine measurement — still unbuilt, still needs that measurement to
  exist first, same caveat as before.

## 3. The point-target model — trustworthy, but only for what it already covers

`army_ttk_vs_point_target` passes its validation checks exactly and
reproduces `entity-tiers.md` §7's own table. But it inherits every
assumption that table already stated as unmeasured:

- `SurroundCapEstimate` per tier is a Fermi estimate (`entity-tiers.md` §4),
  not an in-engine measurement.
- It assumes the target is fought **clean** — full army committed at t=0,
  nothing else competing for attention. `entity-tiers.md` §4 point 1 already
  flags this as a **lower bound**, not a prediction, once Elite/Titan/Boss
  are embedded in a live swarm (per `scaling-curve.md` §1's decision that
  they always are). This harness does not fix that either — it would need
  the same arrival-timing/concurrent-engagement data as the wave model.
- Archer tier-scaling is an assumption, not committed data — see
  `data_loader.retinue_fighter`'s docstring and
  `docs/design/entity-tiers.md` §7's own flagged, unresolved note.

## 4. What this harness does not model at all

None of these are bugs — they're outside task-063's scope, listed so a
reader doesn't assume silence means "handled":

- **Stance play** (Follow/Charge/Hold/Rally), leash mechanics, and the leash
  radius as a lit-floor resource — GDD's whole "light is a resource" design
  law is entirely absent. Every scenario here is closer to
  `GATE1-FUN-PROTOTYPE.md` §3's *zero-input baseline* than to a played run.
- **Supply/degrade** (`economy.json`'s DPS-multiplier penalty for
  over-recruiting past capacity) — scenario retinue counts are treated as
  full-strength, not degraded.
- **Items, hero ability nodes, Whetstone-style stacking buffs** —
  `scaling-curve.md` §2 explicitly flags these as "exactly where the rest of
  the exponential feel is meant to come from" and un-modeled there too; this
  harness doesn't add them either.
- **Knockback, telegraph/windup timing, positioning, chokepoints** — anything
  that isn't reducible to a DPS/HP/Armor/TargetsPerHit number.
- **Multi-wave carryover** — each scenario is a single, independent
  engagement. It does not chain wave 1's survivors into wave 2's starting
  count the way a real run (or `GATE1-FUN-PROTOTYPE.md`'s "refill to a cap"
  rule) would.
- **Per-unit ability effects in the wave model** — burst windows, regeneration,
  stealth and aura-radius effects have no primitive to attach to in a pooled
  attrition model. Measured 2026-07-29 against `docs/data/hero-builds.json`:
  5 of its 6 abilities are inert in `variety.py` for this reason, and they are
  the dominant remaining source of tied rows in its ranking (see
  `docs/sim/VARIETY.md`). This is the largest gap between what the hero-build
  data expresses and what the harness can compare.

## 5. Armor in the wave-attrition model is applied over a MIXED victim pool

Added 2026-07-29, when armor stopped being hardcoded to zero in
`simulate_wave_attrition()` (it had been passed `victim_armor=0.0` on all four
`steady_state_dps` call sites, which silently discarded the enemy's `Armor` —
`brood_soldier_melee` 6, `brood_elite` 12, `brood_titan` 20, `brood_boss` 14 —
and systematically **overstated** the retinue's damage output in every wave
fight).

The fix carries its own approximation, and it is a real one. The model pools HP
and splits each side's damage across that side's subgroups proportional to
alive-count share, so an attacking group has no single identifiable victim whose
Armor to subtract — it is hitting a mixture. `combat_model.mixed_victim_armor()`
therefore uses the alive-count-weighted **mean** Armor over exactly the pool
that absorbs the damage, reusing the well-mixed-target assumption the tick loop
already makes.

**Why that is not exact:** `effective_blow()` floors every blow at
`chip_floor`, so averaging a mixed pool's armor is not the same as applying each
subgroup's armor separately and summing. It is close where armor values are
similar across the pool and diverges where one subgroup is far tougher than the
rest — a `brood_titan` at Armor 20 mixed with `brood_fodder` at Armor 0 is the
worst case, because the average understates how much the Titan absorbs and
overstates how much the Fodder does. Removing the approximation means splitting
damage per victim subgroup **before** applying armor, which is a restructuring
of the tick loop rather than a parameter change.

Practical read: trust armor's *direction and rough magnitude* in wave results,
not its precise value in a mixed-tier fight. Point-target results are unaffected
— that model always applied a single named target's armor and still does.
