# The war ledger — is it authoritative, or derived?

**This is:** options for `docs/OPEN-DECISIONS.md` Q24, drafted for an owner call. No option
below is a recommendation and none is taken. **Extends:** `docs/design/castle-layout.md`
§5.1 (the four-number ledger), §6.2 (marks as a report on a war the player wasn't in), §7
(fidelity bands), §9 (the minute-to-minute loop).

---

## 1. Why this is here

`castle-layout.md` §5.1 defines a front as four numbers — `Pressure`, `Strength`,
`Integrity`, `Momentum` — and a derived `State`. §7 defines three fidelity bands — Present,
Adjacent, Distant — each simulating the war at a different cost. **Neither section says
which one is real.** Are the four numbers a summary of what entities are doing, or are the
entities a rendering of what the four numbers say? `docs/PREFLIGHT.md` §3 (Q24) calls this
Tier-0-equivalent: it decides whether `castle-layout.md` §7's own rule — a boss's marks must
be earned identically in all three bands — is even something the design can deliver, and
getting it wrong means beat A9's "the boss remembers what it did to you" is a promise the
game breaks the first time a player checks it.

`docs/design/slice-a7.md` §8 built steps 4–6 of the §9 loop (arrive at a fight in progress,
read the marks, commit the seven) and explicitly left steps 1–3 undone: **"the war has no
ledger, so nothing calls you anywhere."** §4 of the same doc built three of the six marks —
Quilled, Ram, Sated — by console command, and named the other three — **Wearing**,
**Unblinded**, **Column-fed** — as "deliberately absent rather than stubbed," because each
needs a war-sim event that does not exist: killing a named squad, surviving inside a
bearer's light, catching a routing column after a fall. Every option below has to say how it
produces (a) something that calls the player somewhere, and (b) those three events.

### A cross-cutting risk none of the three options escape cleanly

`docs/PREFLIGHT.md` §1 draws a line through the sim harness that matters here: the
**point-target** model (army/hero DPS vs. one Elite/Titan/Boss) is validated — it reproduces
`docs/design/entity-tiers.md` §7's own table exactly. The **wave-attrition** model
(swarm-vs-swarm survivor counts, i.e. what a front's `Pressure` vs. `Strength` actually
resolves to) is not — `docs/sim/LIMITATIONS.md` §1 states the harness does not currently
reproduce the one measured baseline it's checked against (GATE1's 110-of-120 survivors; the
model predicts a full wipe). Any option that computes a front's state, or a mark event, from
simulated entity combat is a wave-attrition claim and inherits that gap. Any option that
computes it from a formula over the ledger's own four numbers does not need the harness to
be right about entities — but then nothing has validated *that* formula either, because
nothing currently checks a ledger formula against anything. This risk is sized differently
per option below; it is called out where it bites hardest.

---

## 2. Option A — Derived

**The ledger is a read-only view computed from what entities are doing.** No independent
front state exists; `Pressure`, `Strength`, `Integrity`, `Momentum` are read off the Mass
sim's own counts and blows at query time.

### Mechanics

A front's state is not stored — it's a function over whatever entities are currently
resolved for that gate: body counts by side, aggregate DPS, gate HP. `Breaking` is a
threshold crossed by that function, evaluated every tick the front has entities to read.
**When the player is looking at it**, this is exactly the Present-band Mass sim already
shipped (`docs/perf/war-test-1000.md`'s 500-vs-500 choke fight, unchanged). **When the
player is not looking at it**, the front still needs entities to derive a number *from* —
which is where this option runs into `castle-layout.md` §7's own text: Distant band is
"ledger only — no entities, four numbers per front, resolved statistically." A front with no
entities has nothing for Derived to read. `docs/PREFLIGHT.md` §3 states this outright: **"A ·
Derived… is impossible for Cold, which has no entities to derive from. Fails on its own
terms."**

### Mark-accretion path

An "event" under this option is a literal entity-level occurrence, timestamped in the Mass
sim: a specific squad's tracked entities all reach zero HP in the same window (candidate for
Wearing); the boss occupies a query inside a bearer's leash radius for N consecutive ticks
without taking a killing blow (candidate for Unblinded); a set of routing entities (tagged
as fleeing after a `Fallen` transition) enter the boss's aggro radius and are consumed
(candidate for Column-fed). All three are expressible **only where entities exist to
generate them** — Present and, at reduced fidelity, Adjacent. **None of the three can be
produced by a Distant/Cold front**, because there is nothing simulated there to catch the
event. That is the same failure as the state derivation above, restated for marks: under
pure Derived, a boss that formed entirely offscreen arrives with no marks, which is exactly
the outcome §6.2 says would make the marks "cosmetic."

### Costs

- **Disqualified on its own terms for Cold**, per PREFLIGHT's own text quoted above — this
  is not a hypothetical cost, it's a hole in the option as stated.
- **Even restricted to Present + Adjacent**, deriving state and events from live entities
  for every front the player could conceivably reach is a wave-attrition claim end to end,
  and inherits the unvalidated-harness risk in full: nothing currently confirms that a
  derived `Breaking` threshold, computed off entity counts, corresponds to the state the
  design wants players to read as "the line cannot hold this."
- **Frame cost, if pushed toward "simulate more fronts to avoid the Cold hole":**
  `docs/perf/war-test-1000.md` measured one 500-vs-500 front at Present fidelity costing
  3.0–3.4ms GameThread against a 16.6ms/frame budget, with headroom to roughly 4× that
  population before the sim becomes the wall again (the bench's own honest ceiling,
  ~13k–20k entities total, still stands per that doc). `castle-layout.md` §7 already spends
  "the whole existing budget" on the Present band alone. Extending Present-grade derivation
  to cover more than the layer the player is on is not free-form headroom — it is spending
  down the same budget the shipped combat model already fills.

### What to watch for in a prototype

Run the war-test scenario (`Kindled.WarTest`) as two or three concurrent fronts instead of
one, at Present fidelity, with the player's camera on none of them. If GameThread cost stays
inside budget, the "impossible for Cold" objection weakens to "expensive for Cold" rather
than a hard wall, and the option is worth re-examining with a diluted-fidelity variant. If it
blows the budget with two fronts, the option is dead beyond what PREFLIGHT already says.

---

## 3. Option B — Authoritative always

**The ledger is the sim; entities are a rendering of it.** The four numbers are the true
state at all times, in all bands, and any visible entity combat is decoration that reports
what the ledger already decided.

### Mechanics

A front's state lives in four stored numbers, updated by formula (reallocation rates,
pressure decay, whatever tuning `castle-layout.md` §5.1 leaves open) regardless of whether
the player is present. **When the player is looking at it**, the Present-band Mass sim still
runs and still looks like a fight — but the fight's outcome does not feed back into
`Pressure`/`Strength`; the ledger's formula was already going to produce whatever state it
produces. **When the player is not looking at it**, nothing changes about how the state is
computed — it's the same formula, same cost, uniform across all three bands. This is the
option `docs/PREFLIGHT.md` §3 calls "cheap, uniform, trivially consistent across bands," and
also the one it says "kills the game, because what the player does in a Live fight then
cannot move the front."

### Mark-accretion path

An "event" here is a statistical draw over the ledger's own numbers, not anything that
happened to a specific entity. Wearing needs the ledger to know a *named* squad was present
and lost at a front — which the four generic numbers don't carry, so this option needs at
least a fifth field (named-squad-at-front) to express it at all, otherwise Wearing has
nothing to attach to. Unblinded (dwelling inside a bearer's light without dying) is a
condition about the *player's* leash, which the ledger doesn't track when the player isn't
there — under this option "surviving inside a bearer's light" would have to become "surviving
N ticks while `Pressure` exceeds `Strength`" or similar, a redefinition of what the mark
means rather than an implementation of the one `castle-layout.md` §6.1 specified. Column-fed
(catching a routing column) is the one this option handles most naturally — a `Fallen`
transition already produces a computable body-loss number in the ledger's own terms, so a
mark keyed to "this boss was credited with front N's routing loss" is a direct formula, no
extension needed. **Net: Column-fed is cheap under B, Wearing needs a schema change, and
Unblinded needs its definition weakened to something the ledger can see.**

### Costs

- **The disqualifying one, in PREFLIGHT's own words:** "it kills the game, because what the
  player does in a Live fight then cannot move the front." This is not a tuning problem —
  it's the option removing the thing beat A7's entire loop (`castle-layout.md` §9 step 5:
  "read the boss's marks, pick the answer, commit the seven") is built to matter to. If the
  ledger was always going to resolve a front a given way, committing the seven is theatre.
- **Cheap and uniform is real, and worth stating plainly since it's the only option's actual
  strength:** no wave-attrition harness gap applies, because nothing here claims to predict
  entity combat outcomes — the ledger's formula is the only thing that has to be right, and
  it can be tuned directly against the sim harness's sweep tools (`sweep.py`,
  `drift_check.py`) without touching the entity-count validation problem PREFLIGHT §1
  raises at all.
- **The Wearing/Unblinded gaps above are schema and definition costs, not performance
  costs** — cheaper to fix than A's frame-budget wall, but they are still changes to what
  §6.1 said each mark means, and that's a design cost this option incurs that A and C don't.

### What to watch for in a prototype

Instrument a Live front so the player's on-screen kills, deaths and boss-marks-applied are
logged alongside the ledger's own `Pressure`/`Strength` delta for the same window. Play a
fight where the player clearly wins the visible engagement (kills far outpace losses) and
check whether the ledger's state moves toward `Holding` in response, or whether it was
already going to land there regardless of the log. If the ledger's trajectory is
indistinguishable with and without the player's recorded actions spliced in, that is B's
named failure mode confirmed directly rather than argued.

---

## 4. Option C — Hybrid, by band

**Live:** entities are authoritative and write back to the ledger. **Warm:** the ledger is
authoritative and drives entities to match it. **Cold:** ledger only. Conversion happens at
the two band-transition points — promotion (Cold → Warm) and demotion (Live → Warm).

### Mechanics

A front's state is *two different representations depending on band*, reconciled at the
seams. **When the player is looking at it** (Live), the front behaves exactly as Present-band
Mass sim does today, and the fight's outcome is written back into the ledger's four numbers
— this half is Option A's mechanics, scoped to one band. **When the player is not looking at
it**, Warm fronts run the ledger's formula and *drive* a reduced-fidelity entity population
to be consistent with it (Adjacent-band tick per `castle-layout.md` §7), and Cold fronts have
no entities at all and are pure ledger, same as Option B scoped to one band. The two seams —
promotion has to *synthesise* an entity population consistent with a ledger state that never
had entities; demotion has to *reconcile* a specific entity population back into four numbers
— are, per `docs/PREFLIGHT.md` §3, "where this will actually break," and neither conversion
is lossless.

### Mark-accretion path

An event's definition **changes by band**, which is the option's central complexity. In Live,
an event is exactly Option A's: a real entity occurrence (a squad's entities zeroed out, the
boss dwelling inside the leash query, routing entities consumed), captured and attached to
the boss as a mark token. In Cold, an event is exactly Option B's: a statistical draw over the
ledger's numbers at a `Fallen` transition (Column-fed is again the natural one; Wearing again
needs the ledger to carry a named-squad field; Unblinded again needs redefinition against
ledger-visible conditions). **In Warm, marks have to come from whichever side is currently
authoritative for that tick** — a Warm front's ledger-driven state can itself trigger a
`Fallen` transition and produce a Cold-style mark event, or the reduced-fidelity Adjacent
entities it's driving toward could, in principle, generate a Live-style event if the
promotion synthesis populated them with enough fidelity to do so. This means Wearing and
Unblinded can be expressed in all three bands under C, but through **two structurally
different code paths that have to agree on what a mark means** — which is precisely the
promotion/demotion conversion problem stated above, applied to marks specifically rather than
to the four numbers generally. `castle-layout.md` §7's rule (marks earned identically in all
three bands) is a constraint on exactly these two conversion functions, not on the bands
themselves, per PREFLIGHT's own framing.

### Costs

- **Inherits A's wave-attrition risk, but only for the Live band** — the band the player is
  actually watching, where the validated point-target model (not wave-attrition) is closer
  to what's actually being asked (`docs/PREFLIGHT.md` §1: "seven soldiers versus one marked
  boss is a point-target problem… answerable in the harness today"). This is a smaller
  exposure than A's, because A has to defend deriving state for fronts nobody is watching;
  C only has to defend it for the one front that is, by construction, on screen.
  - **Note, stated exactly rather than left implicit:** this reduced exposure holds only for
    the *boss-vs-seven* fight itself. If a Live front's *background* state (the ordinary line
    holding or not, independent of the boss) is also derived from entity wave-attrition
    counts, that piece of Live is still a wave-attrition claim and still inherits the harness
    gap in full — Q24 does not choose that away, only Q13/§6.3's point-target framing does.
- **Avoids B's disqualifying failure for the band it matters most in** — a Live fight's
  outcome does move the front, because Live is authoritative and writes back. The player's
  actions are never decorative while they're the one watching.
- **The stated cost is the conversion functions, and PREFLIGHT is specific about why they're
  hard rather than merely fiddly:** promotion has no entities to draw from and has to invent
  a population that reads as consistent with a number; demotion has a real, particular
  population and has to compress it losslessly into four numbers, which it cannot do without
  discarding information (which specific soldier died, in what order, to what) that marks
  like Wearing need in order to know *which* named squad to attach to a boss.
- **Frame cost is bounded more tightly than A's**, because only the Live band pays Present
  fidelity — `docs/perf/war-test-1000.md`'s 3.0–3.4ms figure applies to at most one front at a
  time under this option (the one the player is on), not to however many fronts exist across
  five layers simultaneously. Warm's Adjacent tick is specced in `castle-layout.md` §7 as "a
  small fraction" of Present cost and Cold as "negligible," though neither figure is measured
  (§7 says so directly).

### What to watch for in a prototype

`docs/PREFLIGHT.md` §3 already names this option's falsification test and it doesn't need
restating differently: **leave a Live fight at roughly even, come back. If the ledger
resolved it somewhere the Live sim plainly would not have, the simulation is two different
things and the player will find that out before you do.** Concretely: instrument a demotion
(player leaves a front mid-fight) and the subsequent promotion (player returns), and diff the
synthesized entity population and any marks attached against what a continued, uninterrupted
Live sim of the same encounter would have produced over the same wall-clock window. A large
divergence is the promotion/demotion cost above showing up as a measurement rather than a
prediction.

---

## 5. What this document is not

- **Not a decision.** No box above is ticked; `docs/OPEN-DECISIONS.md` Q24 stays open until
  an owner call closes it there, per that register's standing rule.
- **Not new measurement.** Every number cited traces to `docs/perf/war-test-1000.md` or
  `docs/perf/one-camera-bench.md`, both already on record. Nothing here was simulated for
  this document.
- **Not a ruling on the three absent marks' definitions.** Where an option is described as
  needing a schema change (a named-squad field) or a redefinition (Unblinded against
  ledger-visible conditions instead of the leash), that is this document naming a cost the
  option incurs, not a proposal to make that change.

## 6. Simulation notes

Not simulated. This document evaluates three architectural shapes against existing measured
data (`docs/perf/war-test-1000.md`'s 500-vs-500 front cost, `docs/perf/one-camera-bench.md`'s
entity ceiling) and existing design text (`docs/PREFLIGHT.md` §3, `castle-layout.md` §5.1/§7);
it proposes no new tuned numbers, thresholds, or formulas, so there is nothing here a Monte
Carlo or sweep would validate. The frame-cost claims above are read directly off those two
perf docs, not re-derived.
