# The Castle — five gated layers, and the war that takes them

**Version:** 0.1 · **Status:** geometry and the four framing decisions locked by owner
2026-08-13. Everything numbered below is **proposed and unmeasured** unless it says
otherwise.
**Companion:** `GDD.md`, `SYSTEMS.md`, `docs/design/adaptation.md`,
`docs/narrative/FLAME-FOUNDATION.md`

---

> ## What this is
>
> The spec for the single container the game now happens in: a five-layer castle,
> each layer behind its own gate, under a siege that the defenders lose one gate at
> a time. It also specs the war those gates are the clock for — the autonomous
> simulation of both armies, why that simulation is *supposed* to stalemate, and
> what breaks a stalemate.
>
> ## What it supersedes, and what it does not
>
> This replaces **`GDD.md` §9's arena ladder** — "a run is 5–8 escalating arena
> stages with beats between them" — with a spatial ladder inside one authored place.
> It does **not** silently rewrite §9; per project convention the superseded text
> stays where it is and this doc is the reason it is superseded. §11's reconciliation
> table lists every doc that now disagrees with a live decision, and none of them
> have been edited.
>
> It **does not** touch a single measured number. Every population figure, DPS value,
> armour constant, surround cap and frame budget in `docs/design/entity-tiers.md`,
> `docs/design/scaling-curve.md`, `docs/design/encounter-budget.md` and `docs/perf/*`
> is read here and changed nowhere. Where this doc needs a number that does not
> exist, it says so and does not invent one.

---

## 0. The four decisions this is built on

Owner call, 2026-08-13, in response to the pivot. Recorded before anything is
derived from them, so a later session can tell decision from inference.

| # | Decision | What it kills |
|---|---|---|
| **D1** | **The player commands 7 soldiers.** The crowd does not retire — it becomes *the war*. Hundreds of allied NPCs and enemy entities fight the same battle autonomously around a squad of 7 under direct command. | The 100–120-unit player-commanded retinue as the expression of player power. Command scope shrinks; entity count does not. |
| **D2** | **One-way collapse.** The castle only ever falls inward. Layers are lost and not retaken. The player's agency is *how long*, and *at what cost*. | Any counter-attack / front-oscillation layer. Also kills "grind until the war is won." |
| **D3** | **Two ratchets: the squad and the keep.** The 7 are named, persist, and adapt. The castle's fortifications, garrison and gates persist too, on a slower clock. | The single automatic kills→army-level ratchet as the *only* progression (`GDD.md` §3). Kills may still pay something; that is open (Q3). |
| **D4** | **Concentric *and* tall, with distinct districts.** Five nested rings that also climb. Each ring is its own place with its own narrative and its own defensive problem. Crossing a gate is a hard transition — a load seam the game does not have to apologise for. | A uniform five-ring donut. Also kills "the layers are difficulty tiers of the same space." |

**The one-line pitch that falls out of these:** *the simulation grinds to a
stalemate at every gate; the enemy breaks stalemates with monsters that got
that way by eating your army; you are the seven people sent to kill those
monsters before the gate goes.*

---

## 1. Why stalemate is the design and not the bug

This is the load-bearing idea and it is worth being blunt about, because the
instinct of every future tuning pass will be to "fix" it.

Two simulated armies of comparable strength, meeting at a fortified line, do not
produce a decisive result. They produce a grind. The shipped combat model already
demonstrates the shape at small scale: the zero-input baseline **narrowly loses**
at wave 3 by 4–13 brood (`docs/GATE1-FUN-PROTOTYPE.md` §3,
`docs/data/economy.json`). A line that close to even, held by two autonomous
sides, is a stalemate by construction.

`FLAME-FOUNDATION.md` §4.1 names the risk this creates —**"stand in the circle"
stagnation**, where the dominant strategy is to do nothing and let the army
grind. It has been the project's first open risk since 2026-07-22 and nothing has
answered it.

**This pivot answers it.** Stalemate stops being the failure mode and becomes the
*floor* — the thing that happens everywhere you are not. Standing still is no
longer a winning strategy because standing still does not stop a gate from
falling somewhere else. The mechanism:

1. **The sim's default output is a held line.** Fronts grind. Neither side
   advances. This is correct, expected, and readable at a glance.
2. **The enemy breaks stalemates with individuals, not with mass.** A front does
   not fall because more brood arrived. It falls because *something* arrived that
   a line of soldiers cannot answer (§6).
3. **The player is the only counter to that something.** Seven soldiers under
   direct command are the only force on the board that can be pulled off a line
   and sent at a specific target. The garrison cannot be; it is holding.
4. **Failing to answer costs a gate**, permanently, and the war compresses one
   ring inward (§4).

So the player's job description is exact: **you are the fire brigade in an
attrition war.** Not the army. The thing the army sends.

> **Falsification test for this whole document.** If a playtest shows the player
> can ignore a Breaking front and the layer holds anyway, the war ledger (§5) is
> not doing its job and the stalemate premise is decorative. If the player can
> *never* reach a Breaking front in time, the ladder is too fast. Both are
> measurable and neither has been measured.

---

## 2. Geometry — concentric, tall, and five distinct places

**Not a donut.** The five rings are nested in plan *and* stacked in elevation:
each layer inward is also a layer upward. Two things this buys that a flat
concentric castle does not:

- **The player can see the whole war.** From the Ascent and above, the lower
  rings are visible below — including the ring the enemy currently holds. A gate
  that fell twenty minutes ago is a lit ruin you can look down at. The war's
  history is legible from the keep without a UI element.
- **Ranged units get real elevation value** and the layers get genuinely
  different defensive problems (§3) instead of the same problem at five
  difficulties.

**Rough proportions** (proposed, unmeasured — these are shape, not spec):

| Layer | Plan radius | Floor elevation | Rise over the layer below |
|---|---|---|---|
| L1 Outworks | widest | 0 m | — |
| L2 The Works | ~0.70× L1 | +6 m | shallow ramp |
| L3 The Ascent | ~0.45× L1 | +34 m | **the climb** — switchbacks |
| L4 Lantern Court | ~0.28× L1 | +48 m | short stair |
| L5 The Crown | ~0.15× L1 | +62 m | the last stair |

The vertical budget is deliberately lumpy. **L3 carries most of the height**,
which is what makes it the hardest layer to take by force and therefore the layer
the enemy will not try to take by force (§3.3, §6).

### 2.1 The gate as a transition

Every gate is a **hard seam**: an authored transition when the player crosses it,
in the plainest video-game sense — the gate closes, the view changes, the next
layer resolves in. This is a design decision and an engineering one at the same
time.

- **Design:** the transition is where the layer's identity gets established, and
  where a fall gets its moment. A gate closing behind you with the layer you just
  lost on the far side is the game's strongest single image and it is free.
- **Engineering:** the seam is a **streaming boundary**. Five layers of full-
  fidelity crowd simultaneously is not a thing this project has any measurement
  to support, and §7's fidelity bands depend on there being a hard line between
  "the layer you are in" and "everywhere else."

**Constraint, carried forward from a measurement that exists:** entity spawn is
metered, never batched. The only spawn measurement the project owns is a
**23.46 ms single-frame spike for 250 entities** (`GDD.md` §9). A layer
transition that materialises a garrison in one `BatchCreateEntities` call puts
that spike exactly where the player is being asked to read a new space. Layer
population drips in behind the transition or is resident before it.

---

## 3. The five layers

Names are **provisional**. `FLAME-FOUNDATION.md` §5 keeps the world deliberately
unwritten, and these are functional labels that can survive being renamed. What
is *not* provisional is each layer's **defensive problem** — that is the design,
and it is what makes the layer a distinct place rather than a difficulty tier.

### 3.1 — L1 · The Outworks · **the Great Gate**

**Ground level. The widest ring, the longest wall, and the fewest defenders per
metre of it.**

Open killing ground outside; the enemy's camp beyond that, visible, growing.
Inside the wall: the lists, the mustering yards, the outer ditch.

- **Defensive problem — frontage.** There is more wall than there are bodies to
  hold it. The garrison cannot be strong everywhere and the sim will visibly
  thin some stretches to reinforce others. The player's first real read of the
  war is watching a wall be too long.
- **Where the war looks biggest.** Highest concurrent entity count of any layer,
  by design. This is the layer that justifies the crowd tech.
- **The gate:** a double gatehouse with a **killing corridor** between two
  portcullises. Taking the Great Gate means taking it twice.
- **What its fall costs:** the mustering ground and the outer wells. Reinforcement
  rate to every layer above drops. Nothing else — L1 is designed to be lost, and
  losing it should feel like the war starting rather than the war going wrong.

### 3.2 — L2 · The Works · **the Cart Gate**

**Still low. The ring tightens and the ground breaks up.** Forge, granary,
stables, cisterns, the tanneries. Roofs, alleys, stacked stores.

- **Defensive problem — cover, and the loss of sightlines.** The enemy gets to
  fight through structure. Ranged banks that dominated the open Outworks lose
  most of their value here, and the sim's ranged position-search (§5.3) starts
  failing to find scores worth taking. The player feels their best asset stop
  working.
- **Supply lives here.** Losing the Works starves everything above it.
- **The gate:** a *working* gate — wide enough for carts, built for logistics, and
  never designed to be defended. It is the weakest gate in the castle and everyone
  knows it.
- **What its fall costs:** supply. Upkeep degrades across all remaining layers
  (`GDD.md` §7's degrade-don't-die model is the existing mechanism —
  this doc does not redefine it, it points a new cause at it).

### 3.3 — L3 · The Ascent · **the Stair Gate**

**Here the castle goes vertical.** Switchback ramps, a span over a cut, murder
holes, a gate at the top of the climb rather than the bottom.

- **Defensive problem — *the ground is too good*.** This is the inversion and it
  is the most important layer in the document. Terrain favours the defender so
  heavily that a conventional assault cannot take the Ascent at any sane cost.
  The sim will show that: fronts here sit at Holding almost permanently.
- **Which is exactly why the enemy does not send soldiers.** The Ascent is the
  layer the enemy takes with a **boss** (§6), and the player should learn that
  lesson *here*, on the layer where the stalemate is most total and the break,
  when it comes, is unmistakably not made of mass.
- **The gate:** at the top of the climb. Everything below it is approach.
- **What its fall costs:** the castle's vertical advantage, in one stroke. Every
  layer above is now a short fight from a held position.

### 3.4 — L4 · The Lantern Court · **the Lantern Gate**

**High, enclosed, ceremonial. Where the wounded are brought and the fire is
tended.**

- **Defensive problem — it is full of people who are not soldiers.** Healers,
  the injured pulled up from below, the congregation. Fighting here costs
  non-combatants, and the sim should let the player watch that happen.
- **This is where the healers are.** Losing the Lantern Court does not just cost
  a ring — it costs the triage anchors the whole defence has been routing its
  wounded to (§5.2). It is the single most expensive layer to lose and it is
  positioned second-from-last so the player loses it while still expected to fight.
- **The gate:** the Lantern Gate. Narrow, ornamental, structurally serious.
- **What its fall costs:** healing, army-wide. After the Lantern Court, attrition
  is one-way.

### 3.5 — L5 · The Crown · **the Crown Gate**

**The top and the centre at once. The hub.**

- **The player walks this layer freely.** It is not an arena; it is the space
  between sorties — where the squad is re-formed, where the keep's persistent
  state is visible and spent (§8), where the war can be looked at from above.
- **Defensive problem — there is nowhere left.** The Crown has no defensive
  problem to solve, which is the point. Every problem was in the four layers
  below and they are gone.
- **The gate:** the Crown Gate is visible from every layer below it. The player
  looks up at it for the whole siege and then stands behind it.
- **What its fall costs:** the siege.

---

## 4. Falling inward — the rules

**D2: one-way.** Within a siege, a fallen gate never reopens.

1. **A layer falls when its gate's front reaches `Fallen`** (§5.1). Not when the
   player dies, not on a timer, not on a script.
2. **The fall is announced before it happens.** A front at `Breaking` is the
   game's only real alarm, and it is what turns the player from wherever they are
   toward wherever it is. There is a window. Its length is open (Q5).
3. **Falling back is orderly and costly.** Surviving defenders on a fallen layer
   route to the next gate inward. Some do not make it. The number that does not
   is the layer's fall cost in bodies, and it feeds the enemy (§6 — a boss that
   eats a routing column arrives at the next gate stronger).
4. **The player is never trapped.** If the player's squad is on a layer when it
   falls, they get out. Losing the squad to a fall would make the alarm punishing
   to respond to, which is backwards.
5. **The enemy consolidates before pushing.** A newly taken layer is not
   immediately pressure on the next gate. The lull between a fall and the next
   front forming is the siege's breathing room and the only place the run's
   pacing beats can live.

### 4.1 What a siege *is*, and what comes after — **DECIDED 2026-08-13 (Q1 = A)**

D2 and D3 pull against each other: the castle only falls inward, but the keep
persists across sieges. The resolution — now an owner call rather than the
recommendation this section used to carry:

> A **siege** is the session — the war compresses through the five layers and
> ends when the Crown falls, or when the defence holds to a relief condition.
> Between sieges the castle is repaired and reinforced, and **how deep the enemy
> got last time determines what state it starts in.** The keep ratchet (D3) is
> what you spend on that repair. Losing L1 and L2 in siege N means siege N+1
> opens with those rings damaged, cheaply held, or given up outright.

One-way collapse holds *within* a siege; the keep ratchet sits on the between-siege
clock. *Alternatives considered and not taken: successive castles as the front
retreats across a region; one unbroken siege with no session boundary at all.*

**Three consequences, none of them optional:**

1. **A wipe is cheap; a bad siege is expensive.** Losing costs the siege. Losing
   *early* costs the next one too, because the castle opens damaged. That is the
   ratchet growing teeth without punishing a loss directly.
2. **Repair is a spend, so the keep ratchet needs a source.** It now has a sink and
   no income — which promotes the shipped kill-attribution economy from housekeeping
   to load-bearing (`docs/OPEN-DECISIONS.md` Q3).
3. **Layer state is save state.** Which gates stand at siege start is the persistent
   set, and it changes wholesale exactly once per siege (Q19).

> **Deliberately still unwritten: the relief condition.** "Ends when the Crown falls
> **or when the defence holds to a relief condition**" is a placeholder, and Q1 did
> not fill it. **What actually ends a siege in the player's favour is undecided (Q6),
> and "survive N waves" is not to be inferred as the default.**

---

## 5. The war simulation

Both armies are simulated. The player commands seven of the entities in it.

### 5.1 The front ledger

Each gate carries a **front** — the contested band in front of it. A front is
four numbers and a state, and it is the entire strategic layer.

| Field | What it is |
|---|---|
| `Pressure` | enemy strength committed to this front |
| `Strength` | defender strength holding it |
| `Integrity` | the gate's own structural state |
| `Momentum` | signed drift — which way it has been going, and how fast |

```
State =  Holding    when Pressure ≈ Strength          (the default — §1)
         Straining  when Pressure > Strength, slowly
         Breaking   when Pressure >> Strength, or Integrity is failing
         Fallen     terminal, one-way
```

**Fronts drift slowly and stalemate is the attractor.** Reinforcement flows
toward `Straining` fronts automatically; the sim self-corrects back toward
Holding. This is deliberate — it means ordinary combat outcomes cannot decide the
war, and therefore something extraordinary has to.

**What actually moves a front to `Breaking`:** a pressure spike the ledger cannot
absorb by reallocating. In practice that means a boss (§6), a gate's `Integrity`
failing to something built to break gates, or the compounding cost of layers
already lost (fewer bodies to reallocate).

> **Not decided, do not infer:** the units `Pressure` and `Strength` are counted
> in, the reallocation rate, and the thresholds between states. Picking these is a
> tuning pass against the sim harness (`docs/sim/README.md`), not a design call,
> and the harness has never run a multi-front scenario.

### 5.2 What allied units do without being told

Named explicitly because the pivot brief calls these out, and because *the player
never orders them* — legibility of autonomous behaviour is the whole readability
budget.

- **Line soldiers** advance to contact and hold. They do not pursue past the
  front band. A line that chases is a line that is not a line, and the player
  needs to be able to trust the shape of a held front at a glance.
- **Healers** path to the densest wounded cluster within reach and establish a
  **triage anchor** — a held position that pulls wounded toward it rather than
  chasing them individually. Anchors are why L4 matters (§3.4): they are
  hierarchical, and the Lantern Court is the root.
- **Ranged units seek firing positions.** This is a scored search, not a
  formation slot, and it is where the layer geometry pays off:

  ```
  Score(pos) = w1·TargetsInLOS
             + w2·Elevation           (L3+ is why the castle is tall)
             + w3·CoverFromReturnFire
             - w4·DistanceToFallback
             - w5·Crowding            (keeps banks from stacking on one parapet)
  ```

  A ranged unit that cannot find a position above a floor score **relocates**
  rather than firing badly. This is the mechanism that makes L2's cover problem
  (§3.2) *visible*: in the Works, scores collapse, archers visibly stop shooting
  and start moving, and the player reads "my ranged advantage is gone here"
  without being told.

> The weights are unwritten on purpose. No measurement exists.

### 5.3 The enemy has a plan

The enemy is not a spawn schedule pointed at the player. It runs the same ledger
from the other side and it wants the Crown.

- It **commits** to fronts, and committing to one means not committing elsewhere.
- It **probes** — cheap pressure on several gates to find the one whose
  `Strength` is thinnest, which is exactly the frontage problem L1 is built
  around (§3.1).
- It **holds its bosses back.** Bosses are the instrument that breaks a
  stalemate, and spending one on a front that was going to hold anyway is a
  waste. The enemy sends them at fronts that are already `Straining`, or at the
  Ascent, where nothing else will work.
- It **exploits a fall** — a routing column (§4.3) is food, and the enemy knows
  it (§6).

> **Open (Q4):** whether the enemy plan is a real planner (goal decomposition
> over the ledger) or a small authored strategy set with weighted selection. The
> second is dramatically cheaper and probably indistinguishable for a first
> vertical slice. Not decided.

---

## 6. Bosses, and why each one is different

**The pivot's strongest idea, and the reason the offscreen simulation is worth
running at all.**

A boss is not a stat block that got scaled up. A boss is an enemy that has
**accreted marks from what it has done** — and it does those things in the
simulated war whether or not the player was watching.

### 6.1 Marks

A boss carries a set of marks. Each is earned by a specific interaction, each
changes behaviour and not just numbers, and **each is visible on the
silhouette.**

| Mark | Earned by | What it does | Reads as |
|---|---|---|---|
| **Quilled** | surviving sustained fire from a ranged bank | armour that scales against ranged specifically; seeks ranged positions | shafts still in it |
| **Ram** | breaking a gate | heavy damage to structure; *prefers* gates over bodies | carrying the door |
| **Sated** | consuming a triage anchor or healer cluster | regeneration between engagements | visibly swollen, lit from inside |
| **Wearing** | killing a named squad | takes their kit and silhouette | wearing your dead |
| **Unblinded** | holding inside a bearer's light without dying | the light stops deterring it | eyes that do not flinch |
| **Column-fed** | catching a routing column after a fall | flat mass and reach increase | dragging what it caught |

**Marks compound.** A boss that took the Great Gate, ate the column that routed
from it, and then survived the Works' archers arrives at the Ascent as a
gate-breaking, regenerating, ranged-immune problem — and it looks like all three
of those things before it does anything.

### 6.2 Why this is worth the simulation cost

This is the answer to "why simulate everything, including what the player cannot
see."

**Because the boss that arrives at your gate is a report on the last twenty
minutes of a war you were not in.** If the sim is faked, every boss is a
designer's list and the marks are cosmetic. If the sim is real, the player can
look at a boss and know what it ate — and, more importantly, know that the fight
they *skipped* two layers ago is the reason this one is hard.

It also makes the player's triage decisions carry forward. Letting the Works fall
cheaply to save bodies is a defensible call that comes back wearing your archers.

### 6.3 The counter — and the job of the seven

Seven soldiers under direct command against a monster that a hundred autonomous
soldiers cannot stop. The reason that is not absurd:

- **The line cannot disengage.** A held front holding is the only thing keeping
  the layer. Pulling the garrison off it to swarm a boss loses the gate to the
  ordinary brood, immediately. The squad is the only force that can be *spent
  somewhere specific*.
- **Melee is surround-capped.** A boss admits roughly **35–55 concurrent
  attackers** (`docs/design/entity-tiers.md` §4, existing measurement). Mass does
  not convert into damage against a boss; the surplus queues. Seven specialists
  inside the cap are not meaningfully worse than seventy, and they are far better
  than seventy if the seven counter the marks.
- **Marks are counterable, and the counters are squad-shaped.** Quilled wants
  melee, not the archers it is armoured against. Ram wants to be intercepted away
  from the gate. Sated wants burst that outruns regeneration. Reading the
  silhouette and picking the answer is the squad's tactical layer, and it is the
  thing the autonomous sim explicitly cannot do.

### 6.4 What the player is, in that fight — **DECIDED 2026-08-13 (Q13 = C)**

> **The player's entire output is the seven.** You are embodied, you carry the
> flame, and you can be hit — but you have no independent attack worth using. Your
> abilities act *through* the squad: focus fire, reposition, screen, raise.

This closes the hero-relevance tension `GDD.md` §4 has carried since before the
pivot, and it closes it from both directions at once. **You cannot become
irrelevant**, because every point of damage the player side deals routes through
you. **The seven cannot become decoration**, because they are the only thing you
have.

**What it changes in work that already ships:**

- **`HeroDamage` retires as the player's damage.** The hero Actor and its
  grid-bridge (`HeroMeleeRangeSq`, `FindOwnGridEntry`, `SwarmCombatProcessors.cpp`)
  **stay** — the hero is still a body in the grid that can be struck, and
  `entity-tiers.md` §1 still points at that bridge as the precedent for promoted
  Actors. **The pattern survives; the number it carried does not.**
- **§6.3's mark-reading becomes *the* tactical layer, not one of several.** A boss's
  silhouette now tells you which of your seven to spend, and that is the whole
  decision. If the answer were the same regardless of the marks, the marks would be
  decoration.
- **The seven cannot be interchangeable.** Each has to be a distinct verb, or "who
  do I spend" is not a question. This lands hard on Q2 below and on Q14.
- **The HUD's job changes** — from describing an army to describing seven
  individuals and what each can currently do. Nothing has specced that.

> **Opened by this decision — `docs/OPEN-DECISIONS.md` Q23: the ability kit does not
> exist.** It is now the largest piece of undesigned content in the project, because
> it *is* the player. Whether the kit lives on the player, in the soldiers, or both
> is open, and **it should not be settled by whatever gets prototyped first.**

> **Open (Q2):** the composition of the 7 — fixed roles, player-chosen loadout, or
> whoever survived the last siege. D3 says they are named and persist, which
> implies the third at least partly, but the slot structure is not decided.
> **Q13 = C raises the stakes here:** if the seven are your only output, they have to
> be genuinely distinct. This stopped being a flavour question.

---

## 7. Simulating five layers at once — fidelity bands

The pivot says all entities are monitored and simulated. Five layers of full-
fidelity crowd is not something this project has any measurement to support, and
inventing one would be the exact failure mode `GDD.md` §10 spends a page warning
about (the retired 34,000-entity figure).

**Three bands, with the gate seams (§2.1) as the boundaries:**

| Band | Where | Fidelity | Cost |
|---|---|---|---|
| **Present** | the layer the player is on | full Mass sim — the shipped combat model, unchanged | the whole existing budget (`docs/perf/BUDGETS.md`) |
| **Adjacent** | the layer inward and the layer outward | Mass sim at a reduced tick, no per-unit VFX, squad-aggregated | small fraction |
| **Distant** | everything else | **ledger only** (§5.1) — no entities, four numbers per front, resolved statistically | negligible |

**The band promotes when the player crosses a seam**, which is what the
transition is buying time for.

**The rule that keeps this honest:** a boss's marks must be earned identically in
all three bands. If a Distant front resolves statistically, it still has to
produce the same marks the Present sim would have. Otherwise §6.2's whole claim —
that the boss is a report on a real war — is false, and the player will find out
the first time a Distant fall produces a boss with no marks on it.

> **Nothing here is measured.** The reduced tick rate, the Adjacent budget, and
> whether ledger-resolution can reproduce Present-band mark outcomes within
> tolerance are all open, and the sim harness (`docs/sim/`) is the place to answer
> them. This table is a proposal shaped by existing budgets, not a result.

---

## 8. The two ratchets (D3)

Stated, not designed — the detail belongs in a follow-up and inferring it here
would be the mistake `FLAME-FOUNDATION.md` §5 exists to prevent.

**The squad ratchet.** The 7 are named individuals who persist between sieges,
level, and climb the evolution ladder already specced in
`docs/design/adaptation.md` — which was written for the friendly side and
survives this pivot largely intact, at a much smaller headcount. A death is the
loss of *that* soldier, which is a thing the 120-unit retinue could never make
the player feel.

**The keep ratchet.** Fortifications, gate integrity, garrison strength and
standing orders persist and improve. This is what the player spends on between
sieges, and per §4.1's recommended shape it is what determines the state the next
siege opens in.

> **Open (Q3):** what happens to the automatic kills→army-level ratchet
> (`GDD.md` §3, shipped in C++ — per-squad and per-hero attribution in
> `Mass/SwarmSubsystem.h`). It is built and it works. It may feed the squad
> ratchet, feed the keep ratchet, feed both, or retire. **Not decided.** Note
> that the kill-attribution code is live and cheap to keep pointed somewhere.

---

## 9. What the player actually does, minute to minute

Sanity check on the whole design — if this loop is not good, nothing above matters.

1. **Stand in the Crown, or wherever the squad was left.** Read the war: which
   fronts are Holding, which are Straining.
2. **A front goes `Breaking`.** Something arrived that the line cannot answer.
3. **Cross gates to reach it** — each crossing a transition, each layer a
   different place with a different problem.
4. **Arrive in a fight already in progress** — hundreds of entities, a held line,
   and a thing in the middle of it that is killing the line.
5. **Read the boss's marks off its silhouette.** Pick the answer. Commit the seven.
6. **Win:** the front sags back to Holding, the gate stands, the line closes up
   around you. **Lose:** the gate goes, you get out, and the war is one ring
   smaller for the rest of the siege.
7. **Repeat, inward, with fewer layers and worse bosses**, until the Crown.

---

## 10. Open questions — do not answer by inference

Numbered for the backlog. Project convention: an open question is closed by an
owner call, not by a later doc quietly assuming an answer.

| Q | Question | Why it is not inferable |
|---|---|---|
| ~~Q1~~ | What a siege is, and what persists between sieges (§4.1) | **CLOSED 2026-08-13 — A: the siege is the session.** §4.1 is now a decision. The *relief condition* inside it is still open (Q6). |
| **Q2** | Composition of the 7 — fixed roles, chosen loadout, or survivors (§6.3) | D3 implies persistence, not structure. **Q13 = C makes it load-bearing** — your only output cannot be interchangeable. |
| **Q3** | Fate of the automatic kills→army-level ratchet (§8) | It is shipped code. Retiring it and repointing it are both defensible and cost differently. **Promoted by Q1 = A** — repair is a spend, so the keep ratchet now has a sink and no source. |
| **Q4** | Real planner vs. authored strategy set for the enemy (§5.3) | Cost difference is large; slice-indistinguishable. An engineering call the owner should make. |
| **Q5** | The `Breaking`→`Fallen` window — how long the player has to respond | This is *the* pacing number for the entire game and there is no basis for it yet. Measure it. |
| **Q6** | Whether the war can be won, or only survived | D2 says the castle only falls. It does not say the *war* is unwinnable. Tone-defining. **Q1 = A left the relief condition explicitly unwritten; this is now where it lives.** |
| ~~Q7~~ | Whether a bearer's light still works the way `FLAME-FOUNDATION` says | **CLOSED 2026-08-13 — A: the castle has its own light.** See below. |
| **Q13** | What the player is (§6.4) | **CLOSED 2026-08-13 — C: the player's entire output is the seven.** **Opened Q23** — the ability kit. |

### Q7, closed — the castle has its own light

The leash is *built* (`SwarmCombat.h`, `SwarmProcessors.cpp`, `LeashRadius`, break
latch, `LeashWarnBit`) and the narrative foundation rests on "outside the light, the
dark takes you." That could not be literally true while the garrison held fronts
across five layers without the player present.

**Resolved 2026-08-13: the castle has fixed light of its own** — braziers, the
Lantern Court, the gate-fires. Your flame is *portable* light in a place that
already has light. `FLAME-FOUNDATION.md` needs no retraction.

**Four things follow, and three of them are free:**

1. **The leash survives unchanged in kind**, governing seven units instead of 120.
   Nothing retires. But `LeashRadius` was tuned for a congregation and is almost
   certainly wrong for a squad — **a tuning pass, not a redesign.**
2. **A fallen layer goes dark.** The castle's light is the castle's; when a layer
   falls, its fires go with it. So the ground behind the enemy is lethal dark — which
   supplies the *in-fiction* reason the collapse is one-way, at no extra rule.
3. **Your flame is the only light on ground the castle has lost.** Which means
   **beat A7 at the Great Gate is the first and only place in the opening where the
   leash matters**, because it is the only ground the castle no longer holds
   (`docs/design/intro-and-zones.md` §A7).
4. **Q15 gains a fiction it did not have.** Permanent squad death used to contradict
   beat A2's "down is a state, not death." It no longer does: the castle's light is
   what stands the *garrison* back up, and **you fight where that light does not
   reach.** This does not decide Q15 — it removes the objection that ruled out one
   of its options.

---

## 11. Reconciliation — what now disagrees with what

No file below has been edited. This table is so that the disagreements are known
rather than discovered.

| Doc | What it says | Status after this doc |
|---|---|---|
| `GDD.md` §1, §3 (run loop) | 20–30 min, 5–8 escalating arena stages, wipe restarts at stage 1 | **Superseded in shape.** The ladder is spatial and inward. Session boundary is Q1. |
| `GDD.md` §3 (meta loop) | army level = f(lifetime kills), the only ratchet, no player spend | **Superseded by D3.** Two ratchets. Kill economy's fate is Q3. |
| `GDD.md` §4 | retinue of 100–120 under stance command, headcount by measurement | **Superseded by D1 for the *commanded* force only.** The crowd survives as the war; the measurements stand. |
| `GDD.md` §9 | procgen retired; arena ladder + beats (shop/rescue/boss) | **Superseded.** Beats need re-homing into a siege — not attempted here. |
| `docs/design/run-structure.md` | run → 3 floors → boss state machine | **Superseded wholesale.** Closest existing analogue to §4; worth reading for the transition-rule shape. |
| `docs/design/adaptation.md` | evolution ladders, captains with their own retinues | **Survives**, retargeted from an army to 7 named soldiers (§8). Captain-with-retinue rung needs re-reading against D1. |
| `docs/design/entity-tiers.md` | fodder/soldier/elite/titan/boss + Armor | **Survives fully.** §6's marks sit *on top of* the boss stat block; they do not replace it. Titan finally has an obvious home. |
| `docs/design/encounter-budget.md`, `scaling-curve.md` | per-floor pulse pacing, 250/450/700 curve | **Numbers survive; their container does not.** The curve now describes pressure on a front, not a wave in a stage. Needs a re-fit pass. |
| `docs/narrative/FLAME-FOUNDATION.md` | pitch-dark world, the leash, hero-as-light | **Q7.** Premise intact, mechanical role unclear. |
| `SYSTEMS.md` §5 (pacing director) | hand-authored rhythm, no director | **Directly challenged.** §5's ledger *is* a director. Reconcile deliberately. |
| `docs/GATE1-FUN-PROTOTYPE.md` | wave/breather/win-lose, zero-input baseline narrowly loses | **Survives as evidence**, and §1 above reinterprets that measurement as the stalemate premise rather than a balance problem. |

---

## 12. Decision log

- **2026-08-13** — D1 crowd becomes the war, 7 under command · D2 one-way
  collapse · D3 two ratchets (squad + keep) · D4 concentric + tall + distinct
  districts, gates as transitions. Owner.
- **2026-08-13** — Five layers named provisionally (Outworks / Works / Ascent /
  Lantern Court / Crown) with their defensive problems fixed as the design.
  This doc.
- **2026-08-13** — Stalemate adopted as the sim's intended default output, and
  offered as the answer to `FLAME-FOUNDATION.md` §4.1's standing-still risk (§1).
  This doc. **Untested.**
- **2026-08-13** — Boss-by-accretion (marks) proposed as the stalemate-breaker
  and as the justification for offscreen simulation (§6). This doc.
- **2026-08-13** — **Q1 = A.** A siege is the session; how deep the enemy got sets the
  state the next one opens in (§4.1, promoted from recommendation to decision). The
  relief condition inside it stays unwritten and moves to Q6. Owner.
- **2026-08-13** — **Q7 = A.** The castle has its own light; your flame is portable
  light in a place that has fixed light (§10). The leash survives at a new scale;
  a fallen layer going dark becomes the in-fiction reason collapse is one-way. Owner.
- **2026-08-13** — **Q13 = C.** The player's entire output routes through the seven
  (§6.4). `HeroDamage` retires as a player-damage number; the Actor grid-bridge stays.
  **Opened Q23** — the squad-channelled ability kit, now the largest piece of
  undesigned content in the project. Owner.
