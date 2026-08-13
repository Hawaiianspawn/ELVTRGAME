# Run structure — start to boss, three floors

**What this is:** the run-structure spec called for by `docs/RTS-VERTICAL-SLICE.md`
§5's tech checklist ("Run structure: start → 3 floors → boss → victory/death
screen," task-024). It answers the shape questions nothing else in the repo
answers yet: what a floor transition is from the player's side, whether the
retinue persists across floors and at what cost, what gates the boss, and what
a wipe does.

**Extends:** `docs/GATE1-FUN-PROTOTYPE.md` (the shipped wave/breather/win-lose
structure this doc reconciles, §1), `docs/design/scaling-curve.md` (the locked
250/450/700 population curve and the retinue growth-site schedule this doc
sequences into floors, §2–3), `docs/design/encounter-budget.md` (the per-floor
pulse/Lull pacing and room-type budget this doc's floor template is built
directly on top of, §2–3), `GDD.md` §3 (core loop: run → floors → boss/exit)
and §9 (procgen floor baseline), and `SYSTEMS.md` §4/§6/§7 (growth sites,
retinue economy, upkeep).

**Does not change:** any locked population number, pulse schedule, room-type
budget, or economy number. This doc adds exactly one new layer — the top-level
run/floor state machine and the transition rules between its states — over
numbers all already decided elsewhere.

---

## 1. Reconciling the shipped shape with the target shape

`GATE1-FUN-PROTOTYPE.md` ships **3s deploy → wave 1 (250) → breather (6s,
refill to 120) → wave 2 (450) → breather → wave 3 (700) → win**, hero death
loses. The target (`RTS-VERTICAL-SLICE.md` §1, §5) is **start → 3 floors →
boss → victory/death screen**. These are not the same shape stacked one level
deeper — three things about gate-1 are placeholders that get replaced outright,
and three survive untouched.

**Replaced:**
- **"Wave" becomes "Floor," and a floor is more than its fight.** Gate-1's wave
  is a bare field with brood spawning around the hero. A floor
  (`encounter-budget.md` §3) is a small room graph: entrance corridor →
  optional Risk Room → (floors 1/3 only) Decision Site → mandatory Arena. The
  Arena is where gate-1's wave logic lives on, largely unchanged in kind
  (§2 below) — but it's now one room in a floor, not the whole floor.
- **The flat refill-to-120 is replaced by the growth-site economy**, already
  DECIDED and specced (`SYSTEMS.md` §4/§6/§7, `docs/data/growth-sites.json`).
  §3 below states what this means for run structure specifically: the
  retinue's headcount is no longer reset to a fixed number between floors, it
  *carries forward* and is topped up only by what the player buys.
- **Win condition moves from "clear wave 3" to "kill the boss."** The boss
  (`entity-tiers.md` §3, stat-block only; full fight design is a separate
  deliverable) is new content, not a relabeling of wave 3 — floor 3's Arena
  still has to be cleared first (§4 below), the boss is a second, distinct gate
  behind it.

**Survives, unchanged in kind:**
- **Hero-death-loses**, globally, at any point in the run. Nothing in this doc
  touches that rule or adds a second loss condition (§5 states why).
- **The Arena's internal combat model** — discrete swing cadence, leash,
  stances, the pulse/Lull pacing `encounter-budget.md` §2 specs on top of it.
  This doc treats the Arena as a room type it schedules, not a system it
  redesigns.
- **A hand-authored, non-director rhythm.** `SYSTEMS.md` §5: "pacing is
  hand-authored... no director AI yet." The floor template below is exactly
  that — a fixed room sequence per floor, the same status gate-1's fixed
  3-wave list already had.

**Was always a placeholder, and stays one:** the win/lose HUD line
(`GATE1-FUN-PROTOTYPE.md` §5: "win/lose is a HUD line, not a screen"). §6
below specs what the victory/death screen needs to *show*, not how it renders
— that's implementation, out of this doc's scope.

**On the disputed pacing constants.** `Spike1GameMode.h` currently ships
`DeploySeconds = 1.f` / `BreatherSeconds = 2.f`; `GATE1-FUN-PROTOTYPE.md`
documents 3s/6s. Which is intentional is unsettled and not this doc's call.
Nothing below asserts either number — where a deploy or breather duration
needs naming, it's cited as "`GATE1-FUN-PROTOTYPE.md`'s documented Ns,
currently contradicted by the shipped default" and the *structural* role
(a settle beat before combat starts; a decision beat between floors) is what's
specced, not the second-count.

---

## 2. The floor transition, from the player's side

Every floor is the same template; only the numbers plugged into it differ
(`scaling-curve.json`, `encounter-budget.json`). Floor 3 adds two rooms floors
1–2 don't have.

```
Entrance corridor
   │
   ├─ optional: Risk Room (pure Fodder, +Embers, +loot roll)   ← must be entered
   │                                                              before the Arena's
   │                                                              first pulse fires
   ├─ (floors 1 & 3 only): Decision Site (fork / sacrifice)
   │
   ▼
Arena (mandatory) — pulse schedule per encounter-budget.md §2
   │  Floor 1: 1 pulse, 250 at once (unchanged from gate-1)
   │  Floor 2: 2 pulses (270→180), 1 embedded Elite at pulse 2
   │  Floor 3: 3 pulses (210→245→245), embedded Elites at pulses 2 & 3
   │
   ▼
[Floors 1 & 2 only] Growth site — spend Embers, then proceed
[Floor 3 only]      no growth site — straight into the boss gate (§4)
```

| Floor | Population | Elites (embedded) | Decision site | Growth site after | Boss |
|---|---|---|---|---|---|
| 1 | 250 | 0 | fork (low-stakes intro) | growth-A | — |
| 2 | 450 | 1 | — | growth-B | — |
| 3 | 700 | 2 | sacrifice (before the Arena) | — (see §4) | isolated boss room |

**Decision-site ordering.** `encounter-budget.md` §3 states floor 3's sacrifice
sits *before* the Arena, "the penultimate-stakes choice right ahead of the
escalation-into-boss sequence." Floor 1's fork isn't given an explicit
before/after in that doc — I'm reading it the same way for structural symmetry
(a floor's authored choice precedes its fight, the same "decide, then commit"
shape both times), but that's this doc's assumption, not a re-statement of
something already decided. Flagging it as such rather than presenting it as
settled.

**Risk Room timing is load-bearing for why floors don't have a "pause the
fight" problem.** `encounter-budget.md` §3: the Risk Room is reachable only
before the Arena's clock starts, so there's never a question of whether a
pulse schedule keeps running while the player is elsewhere — it hasn't
started. The corollary for run structure: once a player commits to the Arena
(triggers pulse 1), the floor's optional detours are behind them. A floor,
from the player's seat, is "explore the side room if you want it, then commit."

**What a floor transition *feels* like, concretely:** clear the Arena (last
pulse and any embedded Elites dead) → the growth-site panel opens (floors 1
and 2 only) → spend Embers across the triangle (`SYSTEMS.md` §7) → walk
through a corridor into the next floor's entrance. No loading-screen framing
implied or required; this is a room-to-room transition like the rest of the
floor's rooms, not a distinct meta-state, except that it's the one room type
that pauses time to let the player make an unhurried allocation choice
(the growth site has never had a combat clock running against it, per its own
spec — `growth-sites.json`'s panel is a camp beat, not a timed one).

---

## 3. Does the retinue persist across floors, and at what cost?

**Yes — and this isn't a new decision, it's the structural sentence nobody had
written yet.** `scaling-curve.md` §2's floor-by-floor growth table only makes
sense under persistence (Floor 2's starting count is Floor 1's survivor count
plus whatever growth-A bought; Floor 3's is Floor 2's plus growth-B), and
`SYSTEMS.md` §4 already says the growth site "replaces the gate-1 flat
refill-to-120, which was a non-decision." Nobody had stated the floor-
transition rule itself as a plain sentence before now: **nothing resets your
retinue between floors except combat losses you already took and whatever you
choose to spend Embers replacing.**

**The cost, specifically — two channels, both already decided elsewhere and
just being named here in transition terms:**
- **Embers.** Recruit costs 12 Embers for +10 Freed (`growth-sites.json`),
  competing against Promote (15E), Provision (10E), items (20E), and hero
  nodes (18E) for the same ~30–52 Ember pool (`growth-sites.json`
  `slice_placement`). Replacing losses is never free and is never the only
  thing worth doing with that pool — that opposition is the point
  (`SYSTEMS.md` §7's "triangle").
- **Supply demand.** Recruiting past Supply capacity doesn't just fail to help
  — every unit in the army degrades (`SYSTEMS.md` §7: `DPS × capacity/demand`,
  floored at 0.4×). A floor transition where the player over-recruits to patch
  losses can make the *whole* retinue weaker walking into the next floor, not
  just fail to strengthen it. This is the concrete mechanism behind
  `scaling-curve.md`'s recruit-max scenario crossing into degrade by floor 3.

**Entering a Risk Room before the Arena spends this same currency in a third,
earlier way.** Its Fodder-only population (§2's table) can cost you units
before the floor's mandatory fight even starts, in exchange for a guaranteed
loot roll and extra Embers (`encounter-budget.md` §3). That's a real
floor-transition-adjacent choice this doc surfaces for the first time: a
player weighing "risk retinue now for Embers to spend at the growth site" is
making the same breadth/depth/sustain-shaped tradeoff `SYSTEMS.md` §7
describes for the growth site itself, one room earlier.

**The headcount-gap caveat carries forward unchanged, and matters most right
here.** `scaling-curve.md` §3 is the single most important open finding this
doc inherits: as currently tuned, the growth-site economy never gets the
retinue near gate-1's own measured 120-count survival calibration — floor 1
alone starts at a population/army ratio already past where gate-1's zero-input
baseline was being wiped. This doc doesn't change that (it isn't authorized
to touch `scaling-curve.json`/`economy.json`/`growth-sites.json`, and neither
was `scaling-curve.md` itself), but it's the thing that would make "does the
retinue persist and at what cost" theoretical rather than functional: a
run-structure spec that assumes persistence works as a meaningful tradeoff
is standing on a growth curve that its own upstream doc says currently loses
every floor it's tested against. Restating `scaling-curve.md` §6's handoff
here because it's this doc's single biggest dependency: **whoever next revises
`SYSTEMS.md` §2 or §7 needs to close that gap before floor transitions can be
tuned by feel rather than by table.**

---

## 4. What gates the boss?

**Clearing Floor 3's Arena** — its 3rd pulse and both embedded Elites dead —
opens the boss room. There is deliberately **no growth site between Floor 3's
Arena and the boss** (§2's table): the two growth sites in the slice sit at
the two floor *transitions* (1→2, 2→3); the boss isn't a fourth floor, it's
Floor 3's capstone, so there's nothing to transition into a growth site for.

**That absence is the real answer to "what gates the boss," stated plainly
because nothing else in the repo has said it yet:** the boss fight is the one
encounter in the run where the player enters with exactly whatever they
finished Floor 3's Arena holding — no chance to patch losses, no fresh Embers,
no Provision top-up first. Whatever growth-B was spent on two floors earlier
is the last economic decision that has any bearing on the boss fight. This
raises the stakes of growth-B specifically (it's effectively "spend for floors
2 *and* 3 *and* the boss," not just floor 3) — worth flagging to whoever tunes
Ember arrival amounts, since `growth-sites.json` currently sizes growth-B's
~52-Ember arrival estimate around clearing floor 3's Arena, not around also
walking into the boss with nothing left in reserve.

**The boss room is isolated** (`encounter-budget.md` §3: "no concurrent
swarm") — once the gate opens, the fight is the hero + whatever retinue
survived, against the boss alone, per `entity-tiers.md` §3's stat-block
baseline. Full phase/mechanic design is out of this doc's scope (that's the
separate "Boss & elite design" deliverable, already flagged by
`scaling-curve.md` §4 and `entity-tiers.md` §5 as needing phase gates or
damage-immune windows so the raw-HP TTK numbers there — 7.8–13.7s — don't
read as "just a bigger DPS check").

---

## 5. What happens on a wipe

**Unchanged from gate-1: hero death ends the run, immediately, at any point.**
Not just inside the Arena — a hero could in principle die in a Risk Room or
the boss room too (Decision Sites and corridors are non-combat by design,
§2, so no death risk lives there today). This doc doesn't add a second
condition and deliberately doesn't invent one:

- **Retinue reaching zero is not itself a loss condition**, and this doc
  flags rather than resolves that gap. Gate-1 never needed to define this
  because the hero-solo-DPS problem was already why hero damage was cut to 55
  (`GATE1-FUN-PROTOTYPE.md` §3's tuning history, point 3) — a hero with zero
  retinue is not equipped to solo anything past Fodder (`entity-tiers.md` §7:
  hero-solo TTK vs. Elite is 21.6s, vs. Titan/Boss 97–152s). In practice a
  zero-retinue hero likely stalls rather than dies outright against most
  pulse compositions, which would read as the run limping rather than
  formally ending. Whether that needs an explicit "retinue wiped + can't
  recover" loss state, or is left to fail forward into hero death naturally,
  is genuinely open — flagged for whoever next plays a full run past a bad
  floor-1 wipe, not decided here.
- **No mid-run checkpoint.** `Spike1GameMode::RestartRun()` is described as
  wiping the field and starting over — a full run restart, not a floor
  retry. This doc extends that unchanged: a wipe on floor 3 sends the player
  back to floor 1's entrance, matching the roguelike frame GDD §3 states
  plainly ("run ends... reset"). A "restart at last floor" mode is a difficulty
  option, which `RTS-VERTICAL-SLICE.md` §6 explicitly fakes/omits from the
  slice.
- **The world-flag stub still fires on a loss, same as a win.**
  `RTS-VERTICAL-SLICE.md` §6: "stub one flag write at run-end; render nothing
  from it." Nothing in this doc changes that it's a stub — a wipe is still a
  run-end event for that write's purposes, it just carries whatever
  (currently nothing) a loss writes differently from a win, which is future
  scope, not this doc's.

---

## 6. The phase state machine — what changes from `ERunPhase`

`Spike1GameMode.h`'s `ERunPhase` (`Deploying, WaveActive, Breather, Won,
Lost`) is shaped for one floor's worth of waves on one map. This doc doesn't
edit that file (out of scope), but the target shape needs more states than
that enum has, and naming them here is the useful handoff:

| Target state | Gate-1 equivalent | New? |
|---|---|---|
| `Deploying` | `Deploying` | same — one settle beat, run start only |
| `FloorExplore` (Risk Room / Decision Site, optional/authored, non-combat) | *none* | **new** — floors 1–3 all need a non-combat, player-paced state before the Arena's clock starts |
| `ArenaActive` (pulse-driven combat) | `WaveActive` | renamed + pulse-aware; `encounter-budget.md` §7's own handoff already calls for `BeginWave()` to become pulse-aware |
| `GrowthSite` (breather, Embers, triangle) | `Breather` | renamed + no longer a flat refill; floors 1→2 and 2→3 only, **not** present after floor 3 (§4) |
| `BossActive` (isolated, no concurrent swarm) | *none* | **new** |
| `Won` | `Won` | same, now triggers on boss kill instead of wave-3 clear |
| `Lost` | `Lost` | same, unchanged trigger (hero death) |

The `FloorExplore`/`ArenaActive`/`GrowthSite` sequence repeats per floor (with
`GrowthSite` skipped after floor 3, and `BossActive` inserted only after floor
3's `ArenaActive`) rather than being a flat list the way gate-1's 3-wave array
is. This is a structural note for whoever next touches `Spike1GameMode` (or
its floor-aware successor), not a code change this doc makes.

---

## 7. Handoffs

**To whoever implements the floor-aware game mode.** §6's state table is the
target; `encounter-budget.md` §7's own handoff ("`BeginWave()` needs to become
pulse-aware") is the Arena-internal half of the same change. Floor
entrance/Risk-Room/Decision-Site sequencing (§2) needs a room-graph or
equivalent gate before `ArenaActive` can begin — this doc specs the *rule*
("must enter Risk Room before pulse 1"), not the trigger mechanism.

**To whoever revises `SYSTEMS.md` §2 or §7 next.** §3's headcount-gap caveat
is this doc's largest open dependency, restated because it bites hardest
exactly at the floor-transition seam this doc specs — not a new finding, the
same one `scaling-curve.md` §6 already flagged forward.

**To the "Boss & elite design" deliverable.** §4's "no growth site before the
boss" finding raises the stakes of growth-B specifically — worth factoring in
if that deliverable ever proposes a pre-boss buffer beat.

**To whoever next revises `growth-sites.json`'s Ember arrival estimates.** §4:
growth-B currently sizes its ~52-Ember estimate around clearing floor 3's
Arena; it's also, as of this doc, the last spend before the boss.

**To the retinue-tuning deliverable (`docs/design/retinue-tuning-vanguard.md`,
task-005), not yet written.** This doc assumed no floor-transition-specific
attrition or replenishment rule beyond what the already-decided growth-site
economy (`SYSTEMS.md` §6/§7) provides — i.e., persistence with no extra
floor-boundary penalty or bonus. If task-005 specs per-class
attrition/replenishment rates that interact with floor transitions
differently (e.g., a class-specific "some units don't make the walk to the
next floor" rule), that supersedes the plain-persistence assumption in §3
here, not the reverse.

**To whoever owns `SYSTEMS.md` next.** §1–§6 above are a candidate for a new
§4/§9-adjacent entry ("the floor-transition rule") once this spec is reviewed
— this doc doesn't edit `SYSTEMS.md` itself (out of scope for this task).

---

## 8. Narrative requests

No new entities. Two framing requests, both about what the *structure* should
feel like once faction/biome canon exists (`docs/narrative/FLAME-FOUNDATION.md`):

- **The growth site is where the fire is fed, not a generic shop panel.**
  `FLAME-FOUNDATION.md` §3c: hero abilities should be "about the light —
  where it reaches, how bright, how long, at what cost," and Design Law 8
  treats light as a resource throughout. Nothing about Embers/Supply is
  currently tied to fire imagery (`SYSTEMS.md` §7 specs them as generic
  currency/capacity). Mechanically the ask is nothing — the numbers are
  right — but the growth-site *beat*, sitting between two Arenas, is a
  natural place for "you are refueling the only light in the dark" to land
  visually/narratively, once there's a faction/biome to hang it on.
- **The boss gate (§4) is the one moment in the run with no economic safety
  net left.** What a player should feel walking from Floor 3's Arena straight
  into the boss room, with growth-B two floors behind them and nothing ahead:
  this is close to `FLAME-FOUNDATION.md` §1's "the fight you can only win once
  you have [united the fires]" framing, even though the vertical slice's boss
  isn't that fight yet (co-op/uniting-flames is deferred, §4.4 of that doc).
  Worth flagging to the narrative director as a possible hook for what this
  specific boss room means fictionally, once biome/faction naming exists —
  not a request to answer now.

---

## 9. Simulation notes

**Not simulated.** This doc adds a state-machine and sequencing layer over
numbers (population, pulses, Ember costs, Supply demand) that are already
locked and already simulated in `scaling-curve.md` §7 and
`encounter-budget.md` §8. Nothing here introduces a new tuned curve of its own
— the one open quantitative question this doc surfaces (§3's headcount gap)
is a restatement of `scaling-curve.md` §3's already-simulated finding, not a
new one, so re-running that sim here would only reproduce the same result
those two docs already report. If a future revision changes floor-transition
rules in a way that affects survivability (e.g., adding a growth site before
the boss, per §4's flag), that change should be simulated against
`scaling-curve.md`'s existing ratio-calibration method before being adopted,
not asserted by feel.

---

## 10. Canon proposals

- **`GDD.md` §9's floor baseline states "≥1 growth site" per floor.** As
  actually placed (`growth-sites.json` `slice_placement`, restated in §2's
  table here), the slice has exactly 2 growth sites for 3 floors — at the two
  floor *transitions*, not one per floor — and floor 3 explicitly has none
  (§4). `scaling-curve.md` §2 already reads this as "one growth site per floor
  transition," a narrower rule than GDD §9's literal wording. I'm following
  the more specific, already-implemented reading rather than GDD §9's
  wording, and flagging the wording itself as stale and worth a one-line fix
  next time GDD §9 is touched — not resolving it here, since GDD.md is
  read-only for this doc.
