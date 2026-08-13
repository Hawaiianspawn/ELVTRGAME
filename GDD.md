# Game Design Document — *Kindled*

**Version:** 0.5 (living document) · Companion docs: `CLASSES.md`, `docs/narrative/FLAME-FOUNDATION.md`, `docs/design/castle-layout.md`
**Last updated:** 2026-08-13
**Engine target:** Unreal Engine 5.8

> ### Standing owner decisions, and what is still stale
>
> **-1. The 2026-08-13 castle pivot — MARKED, not absorbed.** The game is now a
> **five-layer castle siege defended by seven named soldiers**, with the crowd as
> the autonomous war around them. Canon: `docs/design/castle-layout.md` (D1–D4) +
> `docs/design/intro-and-zones.md`; every open/closed decision:
> `docs/OPEN-DECISIONS.md` (the register). §1–§4, §7–§9 and §11 below carry dated
> supersession markers pointing there; the superseded text stays in place per
> project convention. A full rewrite of this document against the pivot has not
> happened — read the markers first.
>
> **0. The 2026-07-31 direction pass — absorbed.** Genre spine, meta-progression, run shape, win/loss,
> command granularity and the performance picture were all re-decided on 2026-07-31 and are
> written into §1–§4, §9, §10 and the §12 log below. Superseded decisions keep their dated
> record in place; nothing was rewritten silently.
>
> **1. Title: _Kindled_ (2026-07-27).** Replaces *Emberkeep*, which came from the discarded
> canon. The participle is the point: it names the state of everyone who is *not* the player —
> they have been kindled, by you, and go out without you. (Q10 below is updated; scattered
> references elsewhere in the repo are not.)
>
> **2. Single-player first (2026-07-27).** 1P is the design target. **Co-op is a later
> multiplier on a proven loop, not a v1 requirement.** This is the largest scope reduction the
> project has taken and it changes the concept's go/no-go gate — see §10. Sections still
> written for a 1–4 player party are marked **[MP — deferred]** inline; they are preserved
> rather than deleted because co-op is postponed, not abandoned.
>
> **Also stale:** `WORLD.md` is superseded in full (narrative reset, 2026-07-22). Everything
> in §6a about world flags belongs to the discarded canon — as did the faction and biome names
> §9 carried, until procedural generation was retired outright on 2026-07-31 and took them with
> it (see §9). Current narrative canon is `docs/narrative/FLAME-FOUNDATION.md`.

---

## 1. High Concept

> **Superseded in shape 2026-08-13 (castle pivot).** The crowd stays — as the
> **war**: hundreds of allies and enemies fighting autonomously around a squad of
> **seven named soldiers** under your command, inside a five-layer castle that
> falls one gate at a time. The pitch: *the sim stalemates at every gate; the
> enemy breaks stalemates with monsters that got that way by eating your army;
> you are the seven sent to kill them before the gate goes.* See
> `docs/design/castle-layout.md` §0–§1. The paragraphs below are the 07-31 record.

An **incremental roguelite with crowds**. You command a hero and an army that carries
between runs and only ever grows. The hook: **massive entity counts** — hundreds-to-thousands
of units and enemies on screen at once. A run is 20–30 minutes long, and each one leaves the
army permanently larger than it started (§3).

You carry the only fire in a pitch-dark world, and the army gathers in your light because
outside it they die (`docs/narrative/FLAME-FOUNDATION.md`).

**One-liner:** *A roguelite where you don't just build a character — you build an army that
outlives the run, and you are the only thing keeping it alive.*

**Supersedes the original high concept 2026-07-31**, kept here as the record: *"A top-down
single-player dungeon crawler with roguelike structure and procedural level generation,
rendered in a 2-bit art style... Runs escalate to extreme power levels."* Three terms
retire — the genre spine is now the roguelite meta shape (§3), player power no longer resets
with the run (§3 meta loop), and the game ships in **full colour**: the 2-bit four-value
palette was superseded 2026-07-28 (`docs/art/aesthetic-direction.md`). The level-generation
framing is resolved separately in §9.

---

## 2. Design Pillars

> **Amended 2026-08-13 (castle pivot):** pillar 1 inverts — you are *seven*, and
> the many are the war around you (your output routes through the squad, register
> Q13 = C). Pillar 2's ratchet splits in two: squad and keep (D3). Pillars 3 and 4
> stand. Pillar 5's variety now comes from siege dynamics, not stage composition.

1. **You are many.** The player's power is expressed through their retinue as much as
   their hero. Growth means more bodies, better bodies, and smarter formations.
2. **High power scaling.** Runs go from "a handful of peasants with sticks" to
   screen-filling, absurd late-run power. The fantasy is escalation. **Amended
   2026-07-31:** the escalation is no longer only *within* a run — the army is a
   permanent ratchet across runs (§3 meta loop), so what the player feels is a
   run-shaped curve riding on a line that only ever goes up.
3. **Meaningful decisions on the journey.** Each player faces run-altering choices —
   not just stat pickups, but decisions with tradeoffs, consequences, and (in co-op)
   social weight.
4. **Readable at scale.** Silhouette, contrast, and motion carry the visuals — that is
   what keeps a thousand-entity battle legible. *Was "the 2-bit style isn't just a
   resource saver"; the four-value palette was superseded 2026-07-28 and the game ships
   in full colour (`docs/art/aesthetic-direction.md`), so legibility is carried by shape
   and value contrast, not by palette size.*
5. **Every run is its own run.** **Amended 2026-07-31 with the procgen retirement (§9):**
   variety comes from *what you fight and what you choose*, never from generated geometry.
   Stage composition, the beats between stages (shop / rescue / boss), decision events and
   class/retinue combinations make each run play differently on the same authored arenas.
   Scoped honestly: this pillar promises *emergent variety*, not authored per-run narrative,
   and the world-flag callbacks it used to promise died with `WORLD.md` (§6a).

---

## 3. Core Loop

> **Superseded 2026-08-13 (castle pivot).** The session is a **siege** (register
> Q1 = A): the war compresses through five layers and ends when the Crown falls or
> **another bearer answers your flame** (Q6 = C — relief, not conquest). Between
> sieges the keep is repaired with what the last siege earned. **Two ratchets**
> replace the single kills→army-level ratchet (D3): squad-credited kills pay the
> seven's mid-fight rung climbs, war outcome pays keep repair (Q3 = C). A wipe is
> cheap; a bad siege is expensive. Light fragments are gone (Q32 = A); gold and
> the stash survive via the quartermaster (Q30 = B, §8). The text below is the
> 07-31 record.

### Genre spine — DECIDED 2026-07-31: incremental roguelite with crowds
The retention model is the **roguelite meta shape** (Hades, Risk of Rain): the run is the
loop, and a **permanent ratchet carries between runs**. The ratchet is the army — monotonic,
never falling (meta loop below) — so a lost run still leaves the player stronger than it
found them. Crowds are the differentiator, not the genre: what the player increments is
bodies on screen.
The other genres this doc borrows from remain **support, not co-equal**:
- The **RTS / Pikmin-lite** command layer (§4) is how army power is expressed
  moment-to-moment. It serves the run; it is not a separate strategy game.
- The **horde-survivor** power fantasy (§7) is the *feel* of late-run escalation.
When these pull against each other, the **ratchet wins**: a change that tightens a single
run but flattens the between-run curve is the wrong trade. Onboarding, market category, and
the vertical-slice reward loop all resolve against this spine.

> **Supersedes the 2026-07-21 spine (Q15), kept verbatim as the record of a decision that
> was made and then unmade:**
>
> **Genre spine — DECIDED 2026-07-21: roguelike run; retinue growth is the progression axis.**
> This is a **roguelike first**. The load-bearing loop is the run — enter, grow your
> retinue through decision events, win or die, reset — and **retinue growth (not loot,
> not the persistent world) is the primary progression axis and the run's reward.**
> The other genres this doc borrows from are explicitly **support, not co-equal**:
> - The **RTS / Pikmin-lite** command layer (§4) is how retinue power is expressed
>   moment-to-moment. It serves the run; it is not a separate strategy game.
> - The **horde-survivor** power fantasy (§7) is the *feel* of late-run escalation,
>   not the retention model.
> - The **persistent world** (§6a) is **seasoning** — a between-run flavor layer,
>   deliberately minimal, never the core reward.
> When these pull against each other, the roguelike run wins. Onboarding, market
> category, and the vertical-slice reward loop all resolve against this spine.

### Moment-to-moment (seconds)
Move → position your retinue → fight → collect drops → hold the arena.

### Run loop (minutes) — **DECIDED 2026-07-31: 20–30 minutes, 5–8 stages**
Deploy → clear an arena stage → take the beat between stages (shop / rescue / boss) →
next stage, denser and harder → the final stage's boss. A run is **20–30 minutes across
5–8 stages**, and the ladder extends past 8 as the frontier (difficulty rule, meta loop
below). There is **no exploration layer**: no floors, no corridors, no generated map to
walk. Dungeon crawling is cut and the procedural generator retires with it — full stage
structure and the reasoning are in §9. *Session length had never been stated anywhere in
this document; "floor" is renamed to "stage" throughout.*

- **Win:** kill the final stage's boss.
- **Loss:** hero death, immediately, at any point, with no checkpoint.
- **What a loss costs — DECIDED 2026-07-31: stage progress, and nothing else.** A wipe
  restarts the run at stage 1. The army and its level, gold, items and the stash all
  survive untouched; only the ladder resets. Losing costs the time, never the ratchet.
- Retinue reaching zero is **not** a loss condition — units degrade rather than dying
  out (§7), and the run continues. This closes an already-flagged open question that
  both the leash rule (§4) and degrade-not-die upkeep (§7) depend on.

### Meta loop (hours) — **DECIDED 2026-07-31: the army *is* the meta-progression**
Run ends (win or wipe) → **the army carries** → the next run starts stronger than the last.
There is no separate meta-currency and no allocation panel.

- **Full persistent army.** The army carries between runs and only ever grows. Player
  power does not reset at the run boundary.
- **Army level = f(lifetime kills)** — monotonic, never falls. Kills level the army
  **automatically**; the player never spends anything on army power. The kill economy
  already ships in C++ (per-squad and per-hero attribution, `Mass/SwarmSubsystem.h`).
- **Kills pay army *size*. Adaptation pays army *shape*** (added 2026-08-08, Q31). Every
  character template has an evolution ladder; the player picks a branch and the shops stock
  the rest, and the top rung is a captain fielding its own small retinue. This is the only
  player-facing spend on army power, and it is deliberately on the *shape* axis so the size
  ratchet above stays fully automatic. Spec: `docs/design/adaptation.md`.
- **Kills pay by tier, not flat.** A fodder kill and a titan kill must not pay the same;
  payout scales with the target's tier (`docs/data/entity-tiers.json`). The lifetime kill
  count is surfaced in the HUD as a rising number — a ratchet the player can't watch turn
  isn't doing the job.
- **Three currencies, one per timescale.** *Embers are deleted (2026-07-31).*

  | Currency | Buys | Timescale |
  |---|---|---|
  | **Kills** | army level | permanent, automatic |
  | **Gold** | items at shops | earned in-run; the purse and the item stash persist |
  | **Light fragments** | healer units | in-run only |

- **Light fragments** are gathered through a mission; enough of them in one place convert
  into a **healer unit**. Boss fights drop special fragments that act as **modifiers**
  instead of converting into bodies. *Open (2026-07-31): how fragments are gathered —
  kill drops, placed caches, both, or rescue-only — was not decided, and is not to be
  inferred (Q24).*
  **Superseded 2026-08-13 (castle pivot; `docs/OPEN-DECISIONS.md` Q32 = A): fragments
  retire entirely.** Healing is the Guided-light archetype among the seven (Q29) plus
  the garrison's triage anchors; Q24 closes with the currency.
- **Win / loss** are stated in the run loop above: a wipe costs stage progress only, and
  everything persistent survives it.
- **Difficulty — DECIDED 2026-07-31: both levers.** Stages scale at roughly **60% of army
  level**, so early stages are outgrown slowly rather than instantly dead, **and** the stage
  ladder extends past 8 as the frontier. This requires endless stage generation.
- **Consequence, owner-accepted:** the growth-site allocation triangle is **deleted** —
  `SYSTEMS.md` §4's growth-site decision, §7's allocation-triangle subsection and §8's Ember
  purchase route retire with it, as does `docs/data/growth-sites.json` in full. `SYSTEMS.md`
  §7 and §8 themselves stay live (§7 reshaped, §8's catalog unchanged), and
  `docs/data/upgrades.json`'s 6-item catalog survives — only its Ember prices and
  growth-site purchase route die. The tuning passes that made that allocation non-inert
  (task-097, task-101) are superseded, not struck from the history.

This closes the slot `FLAME-FOUNDATION` §5 and `SYSTEMS.md` left *deliberately unwritten*:
meta-progression is the army, and there is nothing else.

> **Supersedes the hybrid meta loop decided 2026-07-09 (Q1), kept verbatim as the record of
> a decision that was made and then unmade:**
>
> **Meta loop (hours) — DECIDED: Hybrid — knowledge + persistent world state.**
> Run ends (victory or death) → **no power carries over** → but the decisions made
> during the run leave **persistent marks on the world** that future runs encounter →
> new run in a world shaped by past runs.
>
> - Player power fully resets each run (roguelike purity in the power curve).
> - The *world* remembers: factions you aided or betrayed, sites you razed or saved,
>   named NPCs you spared or doomed reappear changed in later runs (see §6a).
> - Class/content unlocks may still exist as light discovery gates ("meet the
>   Beastmaster in a run to unlock her"), but never as stat inflation.
>
> This makes the meta-loop itself an expression of the "meaningful decisions" pillar:
> the reward for a run isn't +5% damage, it's a changed world.

---

## 4. The Niche: Massive Entity Retinues

> **Superseded for the commanded force 2026-08-13 (castle pivot, D1).** The player
> commands **seven** (mixed across the four archetypes, Q29 = A); command-by-type,
> the stance set at army scale, and the headcount question all retire with the
> 100–120 retinue. **The crowd does not retire — it becomes the war**, and every
> measurement below stands for it. The leash survives on the seven and on fallen
> (dark) ground (Q7 = A); Q27's leash-at-thousands worry is moot. Order issuing for
> the seven is register Q26; the channel kit is Q23.

The differentiator. Each player's class comes with **subjects/soldiers** — autonomous
units that follow, fight, and grow with the player.

### Retinue fundamentals
- Units follow the player by default (formation/leash behavior).
- Units are gained through play: rescued, recruited, summoned, bred, or converted,
  depending on class.
- Units have simple individual behavior but aggregate into meaningful tactics
  (flanking, shielding the hero, swarming).
- Retinue size scales hard over a run — early game a handful of units; the late-run
  ceiling is **deliberately unwritten**. **Amended 2026-07-31:** the army is
  **persistent** (§3 meta loop) — it is not rebuilt from scratch each run, so a run's
  growth stacks on whatever the last run left standing.
- **Headcount is answered by measurement, not by a target — DECIDED 2026-07-31.** The
  "hundreds per player" figure this bullet used to carry was never measured. Every
  friendly-army measurement the project owns sits at **100–120 units**, and only because
  that is the auto-fight harness's own cap (`StartingRetinue`/`RetinueCap` 120 —
  MEASURED 2026-07-27, `docs/perf/squad-aggregation.md` §1). The late-run number is
  therefore *unknown*, not merely un-stated. **OPEN:** task-108 must sweep **retinue**
  past 120, not just brood, and set it; until it lands, treat every end-of-run headcount
  in this repo as UNVERIFIED.

### Player control model — **DECIDED: Stance commands**
Units auto-fight, plus a small set of broad orders issued to the whole retinue
(Pikmin-lite). No unit selection, no targeting micromanagement.

**Command granularity — DECIDED 2026-07-31: command by TYPE, not by group.** Orders go
to *"all archers"*, *"all spearmen"*, *"all healers"*. Handle count scales with the
number of unit **types** the army fields — a small number that grows only when the
roster grows — so the command layer never folds no matter how large the army gets.

This **supersedes the fixed 8-handle group model** (`MaxSquads = 8`,
`ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h:46`; `docs/design/squad-group-system.md`
§4.2-§4.3, which kept 8 on a "bound the handles, not the power" rule). That model breaks
by its own arithmetic: at roughly **730 retinue** the spearman pool alone consumes all
eight handles and archers are force-folded into one oversized unit, regardless of how
few archers there are. That figure is a projection from the 80-per-unit legibility
ceiling and is **UNVERIFIED** — no sim run has reached it. It does not need verifying to
be disqualifying: under a persistent army that only ever grows (owner decision,
2026-07-31), a handle budget that binds at any fixed headcount is a ceiling on the
premise. Typing the handles removes the ceiling instead of raising it.

**v1 stance set (working):**
- **Follow** *(default)* — stay in formation around the hero, engage what comes close.
- **Charge** — surge toward the aimed direction/target area, aggressive engage.
- **Hold** — anchor at current position; make a wall, hold a chokepoint.
- **Rally/Defend** — collapse tightly onto the hero, prioritize intercepting threats
  to the hero.

Design rules:
- One button/wheel, instant to issue mid-combat; stances are broad *intents*, the
  Mass Entity AI interprets them per unit type (archers "hold" differently than pikemen).
- Classes may **modify or replace** a stance rather than add new ones (e.g., the
  Pathfinder's Charge is *Loose the Pack* — the pack hunts marked targets
  independently, even off-screen). Same verbs everywhere, different flavor per
  class — keeps the UI universal. Per-class reflavors are specced in `CLASSES.md`.
- Stances are also the networking-friendly choice: replicating one intent enum per
  retinue is cheap; the swarm interprets it locally (see §10).
- **Leash rule (DECIDED 2026-07-19):** the retinue's home is the hero. Every unit
  has a leash radius; a unit past it — including one on **Hold** — breaks stance
  and returns to Follow. You can anchor a chokepoint, but you must stay in the
  fight with your troops (hero relevance enforced by rule, not tuning). Class
  reflavors may *explicitly* override the leash as a designed exception (e.g.,
  the Pathfinder's *Loose the Pack* above); leashed is the default. Tunables and
  break/warning behavior: `docs/RTS-VERTICAL-SLICE.md` §2.
  **OPEN 2026-07-31 (Q27) — does the leash survive scale?** The rule was written for a
  retinue of tens-to-hundreds trailing one hero, and its whole justification is "hero
  relevance enforced by rule, not tuning." Nobody has asked whether *every unit past
  radius reverts to Follow* still reads as hero relevance — rather than as a rubber
  band — for an army in the thousands commanded by type. Flagged, not decided; the owner
  has not been asked.

### Design tensions to watch
- **Hero relevance:** the hero must stay the star even when the army does the killing.
  Hero abilities should be force multipliers (buffs, formations, ults) not just DPS.
- **Screen readability:** friendly vs. enemy mobs must read instantly. Reserve a
  colour/value channel per faction. *Was "in 2-bit"; the four-value palette was superseded
  2026-07-28 and the game ships in full colour (§10).*
- **Entity density:** one hero × a growing army × enemy hordes is still the tech problem
  that defines this project (see §10) — but it is a **simulation** problem, not a
  rendering one. MEASURED 2026-07-28: the renderer costs **0.036 µs per unit drawn** and
  **100% of the frame cost is the Mass sim on the game thread**
  (`docs/perf/one-camera-bench.md` §1, §6). *Was "4 players ×" until the single-player
  decision of 2026-07-27.*
- **Leash vs. Hold-wall — DECIDED 2026-07-26: Hold is porous by design, not a
  barricade.** Resolved toward the sim: a held line pins the retinue's formation to
  the spot it was issued at (leash rule unchanged — units past radius, incl. on
  Hold, still revert to Follow), but nothing about Hold raises how far brood will
  divert to engage it — the tide bites whatever's near its own path and flows past
  what isn't. A hard blocking wall would need per-unit pathing obstruction, which
  is exactly the individual special-casing §10's Mass Entity constraints rule out
  at horde scale, and cuts against §7's "soft costs over hard numeric caps." The
  Vanguard's Shield Wall (`CLASSES.md` §1) is rewritten to match: a positioning
  tool that reads as a wall only where level geometry already makes it one (see
  `SYSTEMS.md` §6 for the mechanics and the open follow-up — does an arena contain a
  chokepoint narrow enough for this to matter? *Amended 2026-07-31: that was a procgen
  question until §9 retired generation; it is now an arena-**authoring** question.*). The
  Relickeeper's Bulwark (`CLASSES.md` §2) makes the identical claim and inherits
  the identical fork; unresolved, since the Relickeeper isn't built in the sim yet.

---

## 5. Classes

> **Superseded 2026-08-13 (castle pivot; `docs/OPEN-DECISIONS.md` Q29 = A):** there
> is no class select. The four identities survive as the archetypes of the
> seven-soldier squad and evolve mid-fight up the tiered adaptation tree — see the
> `CLASSES.md` banner. Kept below as the record.

Each class defines: the hero's kit, the **type of retinue** they field, and how the
retinue grows. Classes should feel like different *games*, not different stat sheets.

**Tone decision (2026-07-09): we are the good guys.** Heroes descend to liberate the
dungeon, not plunder it. Unifying retinue theme: **your army is what you save** —
every class grows by rescuing, restoring, or rallying something the dungeon has taken.

### v1 roster — **DECIDED: 4 classes** (full designs in `CLASSES.md`)

| Class (working name) | Role | Retinue identity | Growth verb | Axis |
|---|---|---|---|---|
| **Vanguard** | Melee anchor | Liberated soldiers & militia in ranks | Rescue & rally | **The many** — high count, disciplined |
| **Relickeeper** | Fortifier / control | Awakened ancient guardians (stone) | Excavate & awaken | **The tough** — mid count, durable, auras |
| **Pathfinder** | Ranged skirmisher | Small named hunting pack | Bond & train | **The few** — low count, elite, mobile |
| **Lampbearer** | Healer / vision | Guided souls & light-wisps | Kindle & guide | **The light** — high count, fragile, sustaining |

The four occupy distinct corners of the count-vs-quality space so the massive-entity
fantasy shows four faces. Co-op composition pitch: *legion holds, fortress endures,
pack deletes, light sustains* — complementary, never required. Support splits
cleanly: Relickeeper is prevention (wards, walls), Lampbearer is restoration
(healing, revives, vision).

Design rule of thumb (for future classes): vary the retinue along **count vs.
quality**, **permanent vs. expendable**, and **passive growth vs. active growth**
axes so classes cover distinct strategic space. v2 candidates parked in `CLASSES.md`.

### Class ↔ retinue binding — **DECIDED: Locked start, hybrid later**
- Every class **starts** locked to its signature retinue type — full identity at
  character select and in the early run.
- **Late-run events and discoveries** can splice off-class units into your army: the
  Relickeeper who awakens a corrupted guardian without purifying it; the Pathfinder
  who bonds the dire beast that killed her hound. With the good-guys tone, hybrid
  events double as *temptation* decisions — dark help at a cost.
- Hybridization is always an **earned, in-run decision with a cost** (a §6 decision
  event, never a menu option) — so it reinforces the decisions pillar rather than
  diluting class identity.
- Balance guardrail: off-class units keep their own behavior but scale off *your*
  class's growth stats at a penalty, so hybrids are spice, not the meta-optimal core.

---

## 6. Meaningful Decisions

Every player gets recurring, personal, run-shaping choices. These are the narrative
spine of a run.

### Decision types
- **Fork events:** e.g., save the caged prisoners (gain units, alert the stage) vs.
  loot the vault quietly. The rescue beat (§9) is this type made mechanical.
- **Sacrifice offers:** trade retinue lives, hero HP, or loot for power spikes.
  **Class-aware pricing (DECIDED 2026-07-21):** retinue permanence differs by class —
  Vanguard Veterans permadeath, Pathfinder pack is "found again," Relickeeper Awakened
  self-repair, Lampbearer Guided regenerate — so a flat "sacrifice N units" template
  is a real loss for one class and a rounding error for another. Sacrifice templates
  price in a **common currency of loss** (banked progression / upkeep supply / ~~world
  standing~~ — world standing died with the flags, 2026-07-22), not raw unit count, so the
  choice is comparably meaningful for all four.
- **Path choices:** branching next-stage offers with telegraphed risk/reward (an elite
  stage vs. a shop beat vs. a harder stage that pays more). **Amended 2026-07-31:** these
  are choices *between* stages, not level exits — floors retired with §9.
- **Moral/faction choices:** decisions that shift how the dungeon reacts to you across
  the rest of the run (persistent consequence within a run).

### Single-player resolution — **DECIDED 2026-07-27**
Every decision belongs to the bearer, alone, and resolves instantly. There is no party, no
vote, and no negotiation layer. This *simplifies* the decision system rather than weakening
it: a sacrifice offer is a conversation between the player and their own congregation, and
the discomfort of that (they will pay the price gladly) is the point — §6a of
`FLAME-FOUNDATION`.

### Multiplayer wrinkle — **[MP — deferred 2026-07-27]**
*Kept for when co-op returns; not in scope.* Decisions were **per-player where possible**:
a decision event targets one player, and their choice can affect the whole party — social
texture through credit, blame, negotiation.

**Resolution rule — DECIDED 2026-07-21, superseded for v1: vote on weight.**
- **Self / current-run-only** (affects only the deciding player or just this run):
  the owning player decides alone, resolved instantly.
- **Party-wide or persistent** (consequence lands on the party or writes a world
  flag): settled by **party vote**, resolved the moment the vote closes. Passes on
  **simple majority**; absent / downed / disconnected players **abstain**; a tied
  or failed vote keeps the world exactly as it stands, so the run always keeps
  moving forward.
Cheap choices resolve instantly; heavy choices resolve fairly, through the party.

### 6a. Persistent World State — **DISCARDED 2026-07-22 (narrative reset); [MP — deferred] throughout**

> **This entire section is stale.** The 15 world flags, 2 faction standings, 5 named NPCs and
> 8 site states belong to the discarded `WORLD.md` canon. **Meta-progression was written
> 2026-07-31: it is the persistent army (§3 meta loop), which closes the slot
> `FLAME-FOUNDATION` §5 left deliberately unwritten — the ratchet is the army, not the
> world.** The
> multiplayer rules below are additionally deferred by the single-player decision. Kept
> verbatim as the record of a decision that was made and then unmade.

Decisions don't just shape the current run — they scar the world. This is our
meta-progression (§3): the world changes instead of your stats.

**How it works (v1 — deliberately minimal):**
- The world tracks a small set of **world flags** per save: faction standings
  (Still Legion, the Quiet — see `WORLD.md`; the Unwitnessed have no standing,
  only sealed breaches), **5 named NPCs** (alive/dead/changed), and **8 site
  states** (landmark locations: sealed / saved / held / claimed).
- Procedural generation reads these flags: a faction you betrayed seeds hostile
  elites into its territory; a site you saved becomes a mid-run safe room; a spared
  NPC turns up later as a recruitable elite or a decision-event vendor.
- **No power inheritance.** World state changes *what you encounter*, never your
  starting stats.

**Scope guardrails for v1:** flags, not simulation. A world flag is an enum the
generator and event system read — no persistent economy, no evolving settlements.
**v1 list locked at 15 flags** (2 standings + 5 NPCs + 8 sites — see `WORLD.md`
§7); nothing gets added without removing something. Decision-event templates
(8 for v1) are specced in `WORLD.md` §8.

**Multiplayer rule — DECIDED 2026-07-21: the party owns the scar.**
Every world-altering decision carries the whole party's agreement: a shared NPC's
fate, a faction's standing, or a site's ruin always reflects everyone's will, settled
by **party vote** (§6 resolution rule). Self/current-run-only choices belong to the
deciding player alone, resolved instantly.

**World-write target — DECIDED 2026-07-21: all present players' worlds.** When a
vote passes, the persistent outcome writes to **every present player's world**, host
and guests alike — so a co-op session leaves every participant a mark on a save they
own, closing the guest-earns-nothing gap. The host's world stays the **shared stage**
the run is generated from.

---

## 7. Power Scaling

Target curve: **exponential-feeling**. By late run, players should be doing things
that would trivialize the early game by orders of magnitude.

- Layered multipliers: hero stats × retinue count × retinue quality × synergy effects.
- Enemy density scales alongside — the answer to player power is *more enemies*, which
  the flat unlit sprite path and entity tech make affordable (§10; rendering is measured
  free). *Was "the 2-bit style"; the four-value palette was superseded 2026-07-28.*
- Breakpoints and "run-defining" pickups (à la Vampire Survivors evolutions / Risk of
  Rain item stacking) create spike moments, not just smooth growth.
- Anti-cap philosophy: prefer soft costs over hard numeric caps. The primary soft
  cost is **retinue upkeep** (specced below); screen chaos and swarm-punishing elites
  back it up.

**Retinue upkeep — DECIDED 2026-07-21 (spec; tunables in `docs/RTS-VERTICAL-SLICE.md`
§2):** Retinue size is *governed, not capped*. Every active unit draws **upkeep**
from a per-run supply pool whose capacity is bought at shop beats (§9). When demand
outruns supply, units don't die — they **degrade** (reduced stats / dimmed /
"unfed"), and newly recruited units enter degraded until supply recovers. One
mechanic, three jobs:
- gives the exponential curve above a real governor — you can't bank hundreds of
  full-strength units without continuously feeding them;
- bounds the entity budget (§10) — degraded units stay in the cheap Mass tier and
  are first demoted from "promoted Actor" status, capping live Actor promotions;
- lets negative world flags (§6a) bite as *supply pressure* instead of pure added
  difficulty, so the world-ratchet has something to push back against. *(Third job is
  dead: the world flags were discarded 2026-07-22. The first two stand.)*

**Amended 2026-07-31 — where supply comes from.** The degrade-not-die rule, the uniform
per-unit demand and the capacity/demand formula are all unchanged. Only the purchase route
moves: growth sites are deleted with the allocation triangle (§3 meta loop), and **Supply
capacity is now the merchant's headline good, bought with gold at a shop beat** (§9). That
makes the shops the thing that unlocks scale, and costs zero re-tuning. It also keeps the
retinue-reaching-zero rule intact — units degrade, they do not die out, and a starved army
is never a loss condition (§3).

> **Superseded 2026-08-13 (castle pivot; `docs/OPEN-DECISIONS.md` Q31 = A): upkeep
> belongs to the war now.** Degrade-don't-die survives unchanged in kind, but it
> governs the *garrison* — the Works' fall is the starvation lever
> (`docs/design/castle-layout.md` §3.2). The seven draw no upkeep, and the
> merchant-headline-good purchase route above is dead with it (the quartermaster's
> replacement goods are register Q30).

---

## 8. Loot & Itemization — **[LATER, placeholder]**

> **Amended 2026-08-13 (castle pivot; `docs/OPEN-DECISIONS.md` Q30 = B):** gold and
> the persistent stash survive; the shops' venue moves from stage beats (dead with
> §9's arena ladder) to **the castle's quartermaster, whose stock is the layers you
> still hold** — lose the Works, lose the stores. Stocks items for the seven and
> off-branch adaptation rungs. Q26 (gold's rate/sources) and Q34 (rung price)
> remain open.

Deferred by design, but reserving the slot. Direction notes:
- Vampire Survivors–style: frequent drops, stacking/synergizing effects, evolution
  combos.
- Loot should feed **both** hero and retinue (e.g., a banner item that upgrades all
  soldiers vs. a weapon for the hero) — this is our twist on the formula.
- Rarity tiers. **Amended 2026-07-31 with shops (§9): the item stash persists between
  runs**, which overturns the old "no persistent gear inventory" rule outright.
- **Base Camp Loot Manager — CUT 2026-07-21.** A cross-player donation cache +
  bot-proxy feature that reopened the host-world rule (§6a) for a parked stretch
  goal, layered on a loot system that doesn't exist yet. Removed from canon to stop
  scope gravity; revisit only if/when the loot system *and* the Gatecamp hub are
  real. (History preserved in `docs/GDD-TODO.md` Part C and this project's design
  session log.)

---

## 9. Stage Structure — **PROCEDURAL GENERATION RETIRED 2026-07-31 · ARENA LADDER SUPERSEDED 2026-08-13**

> **The arena ladder below is superseded by the castle** (`docs/design/castle-layout.md`,
> which says so explicitly): the ladder is now spatial and inward — five layers, one-way
> collapse, fronts and gates instead of stages and beats. Beats re-home: shop → the
> quartermaster (Q30 = B), rescue → mid-fight pressure survives in kind (routing columns,
> the intro's withdrawal), boss-with-adds → **the boss is the stalemate-breaker**, its
> strongest form yet. The spawn-spike constraint (23.46ms/250) carries forward verbatim
> into the gate-seam streaming rules (castle-layout §2.1).

> **This section was "Procedural Generation" until 2026-07-31. It is retired here rather
> than deleted, so that nobody re-derives it.** Dungeon crawling is cut: a run is
> escalating arenas with beats between them. Retired with it, explicitly: floor-based
> room-and-corridor and open-cavern layouts; the prefab-room-library + graph-based layout
> with constraint rules; the per-floor baseline (≥1 arena, ≥1 decision event, ≥1 optional
> risk room, ≥1 growth site) and its deferred party-size scaling; tile-level noise; and
> flag-then-seed deterministic generation with its shareable community seeds.
>
> **Why it was cut.** The generator only ever had to produce one thing the retinue
> actually needed — an open space big enough for a horde fight, which the old text itself
> called "a *requirement*, not a nicety". Everything else it produced was corridor between
> those fights, and corridor is where a thousand-unit army is at its worst. A hand-authored
> arena delivers the one requirement for free, and run-to-run variety comes from
> population, composition and beats rather than from geometry. **task-025 (procgen room
> graph) dies with this section**, as does Spike 3 (§10). The three v1 biome names it
> carried (**the Highgates**, **the Sunken Works**, **the Vesper Halls**) were retired
> `WORLD.md` nouns from the discarded canon (narrative reset, 2026-07-22) and go with it.
> History is in git.
>
> **Not retired by this:** *stage* generation. The ladder below extends indefinitely as a
> difficulty curve, and a stage is a population, a composition and a beat — not a
> floorplan. Generating one costs no level geometry.

### The ladder
A run is a sequence of **arena stages**: each stage is one escalating fight in a single
open space, with a **beat** between stages. A run is **20–30 minutes across 5–8 stages**
(§3); density and composition climb with every rung, the last rung is a boss, and killing
it wins the run. Stages scale at roughly **60% of army level**, and the ladder extends past
8 as the frontier (§3 meta loop) — which is what "endless stage generation" has to mean
here. Nothing is walked to between stages: a beat is where the run stops, not a room the
player explores.

A wipe costs the ladder and nothing else: the run restarts at stage 1 with the army,
gold, items and stash intact (§3).

### Beats — DECIDED 2026-07-31

**Shops.** Separate venues, on their own currency — **gold**, from drops — deliberately
*not* competing with army growth, which is paid for in kills rather than in anything the
player spends (§3). A shop beat is therefore a pure "what do I want" choice with no
opportunity cost against getting stronger, which is the whole reason it can be a shop at
all. **Merchant** first, a **secret shop** second, further shop types after those.
**The item stash persists between runs** (§8), so the shops are the between-run ratchet's
second half rather than a per-run vending machine.

> **Scope guardrail, stated because this is the obvious place to overbuild: do not build a
> full itemisation system to stock these.** Shops open on `docs/design/loot-v0.md`'s six
> stacking run-items plus goods the retinue economy already prices — **Supply capacity is
> the merchant's headline good** (§7), plus recruits. Loot v0's "never a second economy"
> rule is knowingly overturned by gold — that is the decision, not an oversight. Gold's
> drop rate and sources are open (Q26).

**Rescue.** Mid-fight, and **pressure, not a countdown.** Captives break free during an
arena stage; the dark and the brood take the ones you do not reach in time, so saving
several means *getting there*. There is **no UI clock, no new room type and no generation**
behind it — a rescue is layered onto a stage that was going to be fought anyway. Its real
job is that it makes the player *move the light*: a reason to take the circle somewhere
costly is exactly the counterweight `docs/narrative/FLAME-FOUNDATION.md` §4.1 flags as
unsolved, and this is the first mechanic that supplies one.

> **Implementation constraint, not a design note:** rescued arrivals must **drip in over N
> frames**, never land as one `BatchCreateEntities` call. The only spawn measurement the
> project owns is a **23.46ms single-frame spike for 250 entities** — a visible hitch at
> the exact moment the player is being judged on reaction speed is the worst possible place
> for one.

**Boss — a boss with adds, never a boss alone.** Melee is surround-capped (a Boss admits
roughly **35–55 concurrent attackers**, `docs/design/entity-tiers.md` §4) and ranged is
not, so a lone boss does not pay off a massive army: most of it stands and watches while
a fraction plus every archer does the work. Adds fix the *shape* rather than the numbers —
they give melee mass something to hold and kill while ranged units work the boss, so an
army that doubled has twice as much to do instead of twice as much to queue behind.

> **Still to write:** the boss fight is a stat block (`docs/design/entity-tiers.md` §3),
> not a design — no phases, no mechanics, no add schedule. Add composition, add cadence
> and whatever gates a phase are the open half of this decision, and "bigger HP bar" is
> the failure mode to design against.

---

## 10. Technical Direction

**Engine:** UE5.8. **The defining technical challenge is simulated entity count on one
client** — and it is now a measured question, not an assumed one (below). Multiplayer is
deferred (2026-07-27) and is not a term in this problem.

### Resourcing posture — DECIDED 2026-07-21: pre-resourcing, gated
This document is **pre-resourcing**: team size, disciplines, budget, and ship window
are **TBD**. The v1 scope tables (§11) describe an *intended* scope, **not a
committed plan** — nothing here is costed until a team exists. To keep that honest,
the concept-defining tech risk carries a hard gate (below).

### Entity architecture — important note
- **Niagara** is the right call for *rendering and VFX at scale* (GPU-instanced
  visuals, hit effects, mass death effects), but Niagara alone doesn't give you
  gameplay entities — no per-unit AI, collision-driven gameplay, or replication.
- UE's **Mass Entity framework (MassEntity / MassAI)** is purpose-built for exactly
  this: data-oriented simulation of thousands of units with LOD'd processing, and it
  pairs with Niagara or ISM/Nanite instancing for rendering.
- **Working plan:** Mass Entity for unit simulation (movement, targeting, health,
  simple AI) + Niagara/instanced static meshes for representation + full Actors only
  for heroes, bosses, and "promoted" elite units. Prototype this first — it is the
  project's biggest risk.

**Entity-count spike gate — REDEFINED 2026-07-27, PASSED 2026-07-28.** The gate was: one
hero × late-run retinue + enemy hordes holding **60fps (16.6ms)** on a single client, no
replication term. It supersedes the 2026-07-21 gate ("full 4-player target load under the
aggregate-replication model"), which the single-player decision retired along with the
replication risk.

**MEASURED 2026-07-28** — standalone `-game` Development, 1080p, `t.MaxFPS 0`,
`r.VSync 0`, Niagara sprite path, 1,000 brood + 100 retinue: **2.31ms — 433fps**, a
**7.2× margin** on the 16.6ms budget (`docs/perf/one-camera-bench.md` §1). The gate is
passed and the concept's go/no-go is closed. There are no surviving fallbacks to name
because there is no failure to fall back from.

**The 14.62ms figure this section used to carry is dead.** It measured the **debug-box
renderer** (`DrawDebugSolidBox` — one immediate-mode draw call per entity per frame, an
O(N) draw path), MEASURED in-editor 2026-07-26 (`docs/perf/BUDGETS.md`); the same load
re-measured standalone at **9.44ms** on 2026-07-28. It was a diagnostic, never a
renderer, and `Swarm.DebugRender 0` has been the shipping default since 2026-07-27. Every
"we can't hit the gate" claim still in this repo traces back to it.

What the gate did **not** answer is where the ceiling actually is.

### The entity ceiling — an honest range, not the 34,000 figure

**Do not quote 34,000 as fact.** It comes from a single standalone run at
`Swarm.SimLOD.Stride 4` (MEASURED 2026-07-28, `one-camera-bench.md` §8: 30,000 →
14.73ms, 40,000 → 19.06ms). **Four in-editor `-SwarmBench` sweeps on 2026-07-30/31 did
not reproduce it.** Each crosses the 16.6ms budget far earlier:

| sweep (in-editor, retinue 100) | first point | second point | crosses 16.6ms near |
|---|---|---|---|
| `task126` | 12.41ms @10k brood | 20.15ms @20k brood | ~15,500 entities |
| `task128` | 11.26ms @10k brood | 18.16ms @20k brood | ~17,800 entities |
| `task129` (volley off) | 14.09ms @10k brood | 21.21ms @20k brood | ~13,600 entities |
| `task130` | 13.70ms @20k brood | 18.48ms @30k brood | ~26,200 entities |

*Frame times MEASURED 2026-07-30/31, `docs/perf/evidence/task126|128|129|130/SwarmBench-*.csv`.
The crossing column is **UNVERIFIED** — linear interpolation between two measured rows,
not a measured point. Totals are brood + 100 retinue.*

**Working figure: ~13,000–20,000 total entities at 60fps, pending task-108.** Write it as
a range, never as a single number. The range is drawn from the three sweeps measured at
10k/20k brood; **`task130`'s ~26,200 crossing sits outside it and is deliberately excluded**
— a single unreproduced sweep, interpolated from a different load pair (20k/30k), and the
range is owner-directed not to widen on one run. task-108 resolves whether it holds. `docs/data/wave-scaling.schema.md:32` hard-codes 34,000
as the divisor for every headroom claim in canon, so **every one of those percentages is
UNVERIFIED** — including the late-wave "39.4% headroom", which is arithmetic that has
never been run against a measurement.

**The gap is confounded but unresolved.** The confounds are real: standalone `-game` vs.
in-editor; a harness measurement floor of roughly **±0.9ms** (MEASURED 2026-07-31,
`docs/perf/volley-vfx.md` — six deltas came back negative, and added work cannot make a
frame faster); and `gpu_ms` varying **2–6×** between otherwise identical passes in the
CSVs above. Confounds explain how the numbers *could* differ. They do not say which one is
right, and nothing here licenses quoting the friendlier figure. task-108 resolves it, and
its sweep must cover **retinue** past 120 as well as brood (§4).

### Performance is priority 1 of 1 — DECIDED 2026-07-31

The owner's named lever is **camera-distance adaptive sim LOD**: at distance the detail is
not perceptible — projectiles in particular — so simulation fidelity can fall off with
camera distance. That is the correct axis, because the sim is 100% of frame cost.

**Rendering is measured free. Do not build render-side imposters or sprite LOD.**
MEASURED 2026-07-28 (`one-camera-bench.md` §1, §6): the renderer's delta against a
sim-only baseline is inside run-to-run noise from 500 to 20,000 entities, and **two of six
deltas are negative**. Cost per unit drawn is **0.036 µs**, and it lands on the GPU, which
never exceeded **5.5ms** against a 19ms game thread across the sweep. Frustum-culling the
Niagara push would optimise 0.036 µs/unit.

The performance budget is spent **sim-side**, in this order:

1. **Make `QueryNeighbors`' traversal abortable.** `SeparationForce`'s `NeighborCap`
   early-returns from its lambda, but `USwarmSubsystem::QueryNeighbors` keeps walking
   every entry in all 9 buckets — the cap bounds the *math*, not the *walk*. A contained
   change to the dominant pass.
2. **Move the passes to `ParallelForEachEntityChunk`.** The code was deliberately written
   chunk-local for exactly this and nobody has priced it. **UNVERIFIED** — there is no
   measurement of what parallelising buys.
3. **Camera-distance adaptive `SimLOD` stride** — the owner's lever. Today's
   `Swarm.SimLOD` strides on distance from the *bearer* against a fixed `NearRadius`
   (1600uu), and its own operating rule is that `NearRadius` must stay above the visible
   half-width — which is precisely the camera coupling, currently hand-tuned instead of
   derived. Fixed-stride saving MEASURED 2026-07-28: **+6% at 1,000 entities, +39% at
   20,000, +58% at 40,000** (`one-camera-bench.md` §5, §8).

### Sprite rendering — **the 4-value palette is SUPERSEDED 2026-07-28**
The 2-bit / four-value palette this subsection used to spec is **dead canon**. The game
ships in **full colour** (`Quantize 0`; `docs/art/aesthetic-direction.md`), and 3D is
shelved. Nothing below depends on a value count.

- **Niagara-instanced sprites, settled by measurement, not by art test.** One emitter, one
  instanced draw, CPU sim — the `[DECIDE via art test]` fork between flipbook quads and
  flat-shaded 3D is closed. Flipbook quads on Niagara won.
- Flat **unlit** rendering; cheap by design. `M_Swarm` is Unlit + Masked — no light
  touches a sprite. Budget goes to simulation, not shading.
- Units are exempt from the demichrome post-process via `CustomStencil`
  (`Swarm.UnitStencil`), so authored sprite colour reaches the screen unmodified. The
  consequence, stated because it is live: an off-palette pixel in a sheet now renders
  as-is instead of being quantized into legality, which makes `/art-coverage`'s off-ramp
  check more load-bearing, not less.

### Multiplayer — **[MP — deferred 2026-07-27]**
**Single-player is the design target.** Co-op returns as a later multiplier on a proven
loop, not as a v1 requirement. Nothing below is in scope; it is kept because the design is
deliberately written so co-op stays *possible*, and those affordances cost nothing now:

- Stances replicate as **one intent enum per army**, not per unit (§4) — the cheap shape.
- The leash clusters units on their hero (§4), which is exactly what makes replication
  relevancy and off-screen LOD tractable later.
- When it returns: co-op, 1–4 players, listen-server first; replicate authoritative
  *aggregate* state (group positions/counts/seeds) with cosmetic client-side simulation,
  and full replication only for heroes, elites, and bosses. **Do not build toward this
  now** — the single-client gate above has to pass first.

### Early technical milestones
1. ~~**Spike 1 — The Thousand:**~~ — **PASSED 2026-07-28.** 1,000+ Mass Entity units with
   follow/attack AI at 60fps, single client, Niagara-rendered: MEASURED **2.31ms /
   433fps** (`docs/perf/one-camera-bench.md` §1). The open question is no longer "does
   1,000 hold" but "where does it stop" — see the ceiling range above, which task-108
   resolves.
2. ~~**Spike 2 — The Thousand, Networked**~~ — **CUT 2026-07-27** with the single-player
   decision. Revisit only after the slice loop is proven fun on one client.
3. ~~**Spike 3 — Procedural floor**~~ — **CUT 2026-07-31** with the procgen retirement
   (§9). Arenas are hand-authored; there is no floor graph left to generate.
4. **Vertical slice:** 1 class (Vanguard), 1 arena set, a short stage ladder, 1 boss,
   single-player. *Was "1 biome, 3 floors" until §9 retired 2026-07-31.*
   Full definition, gates, and bill of materials: `docs/RTS-VERTICAL-SLICE.md`.

---

## 11. Scope Guardrails (v1 targets)

> **Superseded 2026-08-13 (castle pivot):** this table predates the pivot and has
> not been redrawn. The v1 shape is now: one castle (five layers), seven soldiers
> across four archetypes with no class select, the war sim, the intro
> (`docs/design/intro-and-zones.md`), and the first slice per register Q28. Redraw
> this table once Q23/Q14/Q15/Q16 close.

**Revised 2026-07-27** for the single-player decision and the narrative reset, and
**2026-07-31** for the persistent army, the stage ladder and the procgen retirement.

| In (v1) | Later | Out (for now) |
|---|---|---|
| **Single-player** | **1–4 player co-op** — a multiplier on a proven loop | PvP |
| 1 class (Vanguard) for the slice; 4 designed | 6+ classes | Modding/community content |
| Arena stage ladder + beats (shop / rescue / boss), 1 arena set in the slice | More arena sets, more shop types | Procedural level generation (retired, §9); console ports |
| Stance-based retinue control (4 stances) | Class-specific stance variants | Persistent economy/settlement sim |
| Retinue upkeep (degrade, don't die) | Fancy loot/evolution system | ~~World flags~~ — discarded with the lore |
| The leash, rendered as light | Full itemisation behind the shops (task-034) | |
| **Persistent army as meta-progression** (kills → army level); gold shops with a persisting stash | | Separate meta-currency / meta-shop / allocation screen |

---

## 12. Open Questions Log

| # | Question | Decision / Lean | Status |
|---|---|---|---|
| 1 | Meta-progression model | **SUPERSEDED: the persistent army is the meta-progression (Q22).** Power no longer resets between runs, and the world-flag half of the hybrid died with `WORLD.md` (2026-07-22). | ⏸ Superseded 2026-07-31 |
| 22 | Meta-progression model, and what carries between runs | **Full persistent army. The army carries between runs and only ever grows; army level = f(lifetime kills), monotonic, fully automatic — no allocation panel, no player spend on army power, no separate meta-currency.** This supersedes §3's "player power fully resets each run" and "the reward for a run isn't +5% damage". Kills pay by target tier, not flat. Consequence, owner-accepted: the growth-site triangle retires (`SYSTEMS.md` §4's growth-site decision, §7's allocation-triangle subsection, §8's Ember purchase route, and `docs/data/growth-sites.json` in full — §7 and §8 stay live, and `upgrades.json`'s 6-item catalog survives minus its Ember prices), and Embers are deleted — three currencies remain, one per timescale: kills → army level (persistent), gold → items (stash persists), fragments → healer units (in-run). **AMENDED 2026-08-08 (Q31):** "no player spend on army power" stands for army **size** — kills still level the army automatically, with no panel. It is amended for army **shape**: an Adaptation is buyable at a shop. The original sentence is kept above rather than rewritten. | ✅ Decided 2026-07-31 · amended 2026-08-08 |
| 2 | Retinue control model | **Stance commands: Follow/Charge/Hold/Rally (§4)** | ✅ Decided 2026-07-09 |
| 3 | Classes locked to retinue type? | **Locked start, hybrid via in-run events (§5)** | ✅ Decided 2026-07-09 |
| 4 | Max party size | **SUPERSEDED: single-player first (§10, Q20). Co-op returns later at 1–4.** | ⏸ Superseded 2026-07-27 |
| 20 | Player count for v1 | **Single-player. Co-op is a later multiplier on a proven loop, not a v1 requirement.** Rationale: it retires the replication risk outright, cuts the entity gate by ~4×, and answers `FLAME-FOUNDATION` §4.4 — uniting flames becomes a *run objective* found in single-player, not a co-op mode. | ✅ Decided 2026-07-27 |
| 5 | Sprite flipbooks vs. flat-shaded 3D | **CLOSED by measurement, not by art test: Niagara-instanced flipbook quads, CPU sim (§10).** 3D is shelved. | ✅ Closed 2026-07-28 |
| 6 | Strict 4-color global palette vs. per-faction palettes | **SUPERSEDED: the game ships in full colour (Quantize 0) — `docs/art/aesthetic-direction.md`.** The 2-bit four-value palette is dead canon wherever it still appears. | ⏸ Superseded 2026-07-28 |
| 7 | Loot system design (Vampire Survivors–style) | **SUPERSEDED: items get a venue and a currency of their own (Q23).** The deferral rested on retinue growth being the run's only reward loop; Q21/Q22 move growth onto a separate automatic track, so the slot can no longer stay empty. | ⏸ Superseded 2026-07-31 |
| 23 | Where items come from, and on what currency | **Shops as separate venues on gold dropped in-run — merchant, secret shop, further types later — deliberately not competing with army growth. The item stash persists between runs.** Owner-accepted consequence: this overturns Loot v0's no-second-economy rule. Scope guardrail: stock it from Loot v0's 6 stacking run-items plus Supply/recruit goods — **do not** build task-034's full itemisation for it. With Embers deleted (Q22), Supply capacity becomes the merchant's headline good, so the shops are what unlocks scale. | ✅ Decided 2026-07-31 |
| 8 | Co-op world writes | ~~Party vote on any world-scarring decision~~ — **MOOT.** The world flags were discarded (2026-07-22) and the party was deferred (2026-07-27); the question no longer has either term. | ⏸ Moot 2026-07-27 |
| 15 | Genre spine (roguelike vs. RTS vs. horde-survivor vs. persistent-world) | **SUPERSEDED: incremental roguelite with crowds (Q21).** The 2026-07-21 spine — "roguelike first", the horde-survivor fantasy as "the *feel* of late-run escalation, not the retention model", "when these pull against each other, the roguelike run wins" — is retired, along with §3's reading of it. | ⏸ Superseded 2026-07-31 |
| 21 | Genre spine for v1 | **Incremental roguelite with crowds.** The retention model *is* the roguelite meta shape (Hades / Risk of Rain): the run is the loop, and a permanent ratchet — the army (Q22) — carries between them. Run shape: 20–30 minutes over 5–8 escalating arenas with beats between them; dungeon crawling and procedural floors are cut (§9 retires). WIN = kill the final-stage boss. LOSS = hero death, immediately, no checkpoint; the run restarts at stage 1 with army, gold, items and stash intact, so a wipe costs stage progress only. Retinue reaching zero is **not** a loss. | ✅ Decided 2026-07-31 |
| 16 | Retinue upkeep / power-curve governor | **Degrade-not-die upkeep economy (§7)** — the governor, the uniform demand and the degrade formula stand unchanged. **Amended 2026-07-31:** supply is no longer replenished at growth sites (deleted with the triangle, Q22); **Supply capacity is bought with gold at the merchant** (Q23). | ✅ Decided 2026-07-21 · amended 2026-07-31 |
| 17 | Sacrifice-event pricing across classes | **Common currency of loss, not raw unit count (§6)** | ✅ Decided 2026-07-21 |
| 18 | Decision resolution rule (blocking vs. not) | **SIMPLIFIED: every decision is the bearer's alone and resolves instantly (§6).** The vote rule is preserved for co-op's return. | ✅ Re-decided 2026-07-27 |
| 19 | Pre-resourcing posture + entity-count go/no-go | **Team/budget/timeline TBD. Gate REDEFINED 2026-07-27 (single client, 60fps, no replication term) and PASSED 2026-07-28: 2.31ms / 433fps, a 7.2× margin (§10).** Spike 2 cut, Spike 1 passed, Spike 3 cut with §9. The open successor is the *ceiling*, not the gate: ~13,000–20,000 total entities pending task-108 — 34,000 is not a fact. | ✅ Passed 2026-07-28 |
| 9 | v1 class roster | **4 classes: Vanguard / Relickeeper / Pathfinder / Lampbearer** (see `CLASSES.md`; working names) | ✅ Decided 2026-07-09 |
| 10 | Game name | **Kindled** — the participle, naming the state of everyone who is *not* the player: kindled by you, and out without you. Replaces *Emberkeep*, which named a place in the discarded canon. | ✅ Re-decided 2026-07-27 |
| 12 | World/setting design (factions, biomes, antagonist, NPCs) | ~~**See `WORLD.md`** — Undervault / Hollow Crown / 3 factions / 3 biomes / 5 NPCs (working names)~~ — **SUPERSEDED.** `WORLD.md` is discarded in full by the narrative reset; current canon is `docs/narrative/FLAME-FOUNDATION.md`. Nothing has been re-drafted against it, and the biome names went with §9's retirement. | ⏸ Superseded 2026-07-22 |
| 13 | Unwitnessed faction (name, titan variety, horror level) | ~~First draft in `WORLD.md` §3a~~ — **SUPERSEDED.** The Unwitnessed belong to the discarded canon; any replacement antagonist is drafted from `docs/narrative/FLAME-FOUNDATION.md`, and none has been. | ⏸ Superseded 2026-07-22 |
| 14 | v1 world-flag list + decision-event templates | ~~**15 flags / 8 templates — `WORLD.md` §7–8**~~ — **SUPERSEDED.** The world flags were discarded with the lore (§6a, §11) and `WORLD.md` is dead canon; meta-progression is answered instead by the persistent army (Q22). | ⏸ Superseded 2026-07-22 |
| 11 | Final class names (per-class candidates in `CLASSES.md`) | Working names in use | Naming pass later |
| 24 | How light fragments are gathered | ~~Undecided. The conversion is decided — enough fragments in one place become a **healer unit**, and boss fragments drop as **modifiers** rather than bodies (Q22) — but the gathering method is not.~~ **SUPERSEDED: fragments retire entirely with the castle pivot** (`docs/OPEN-DECISIONS.md` Q32 = A) — healing is the Guided-light archetype among the seven plus the garrison's triage anchors. | ⏸ Superseded 2026-08-13 |
| 25 | Which wave curve is canon | Undecided, and the two live curves conflict: `SYSTEMS.md` §2 locks **250 / 450 / 700** as a dated decision record, while `docs/data/wave-scaling.json` proposes **120 / 400 / 20,000** with retinue 60 → 120 → 600. `encounter-budget.json`, `scaling-curve.json` and `retinue-vanguard.json` all derive from the old curve, so picking the new one re-derives three files. The direction needs one picked; deliberately not picked unilaterally. **Recontextualised 2026-08-13:** "waves" are now front populations in the war sim — the curve question survives as *per-layer population and pressure tuning* (castle pivot; the front ledger's units are register-flagged as not inferable). | Open |
| 26 | Gold's drop rate and sources | Undecided. Gold survives the pivot (register Q30 = B — the quartermaster, war-coupled stock), but neither the rate nor which drops pay it has been set. | Open |
| 27 | Does the leash survive an army in the thousands, commanded by type? | **MOOT — the castle pivot removed both terms** (2026-08-13): the commanded force is seven, and the leash's new job is decided (register Q7 = A — it governs the seven and fallen dark ground; `LeashRadius` needs a squad-scale retune, not a redesign). | ⏸ Moot 2026-08-13 |
| 28 | Puzzle combat as a later direction | **RECORDED, not scoped.** Runs could later become puzzle combat: stat guidance where certain unit types dominate certain factions, making stash and army composition a puzzle to solve rather than a pure power check. Not specced, no tasks filed. | ⏸ Recorded 2026-07-31 — future direction |
| 29 | Command granularity (how many handles, and what they address) | **Command by unit TYPE, not by group** — "all archers" / "all spearmen" / "all healers" (§4). Handle count scales with the number of unit types, so it never folds. **Supersedes the fixed `MaxSquads = 8` group model** (`docs/design/squad-group-system.md` §4.2-§4.3, `SwarmSubsystem.h:46`), which folds recruits into unit 0 at roughly 730 retinue — a projection, UNVERIFIED, and disqualifying anyway under a persistent army that only grows (Q22). **AMENDED 2026-08-08 (Q31):** an Adaptation **branch** is a type under this rule and earns a handle; a **rung** is a look-and-stat move inside a type and does not. A captain plus its retinue is one handle, not one per body. **SUPERSEDED 2026-08-13:** the commanded force is seven (castle pivot D1); how orders are issued to them is register Q26, blocked on the Q23 kit. | ⏸ Superseded 2026-08-13 |
| 31 | Army **shape** between runs — how a unit changes, not how many | **Adaptation.** Every character template has an evolution ladder; a rung is `(unit_type, tier, variant_index)` keyed on §1's existing four-tier ladder, so no second stat ladder exists. **The player picks from a branch, and Adaptations are also shop stock** — the shop offers rungs on branches the pick did not grant. **The top rung is a captain** fielding its own retinue of ≤ 8 (`bannerman` reused; its existing "item/event reward only" line makes the captain the one rung the shop cannot sell). **One command handle per branch, not per rung** (amends Q29). Friendly side only for v1; the rung triple is unchanged for enemies. Fills the promotion slot D4 vacated — amends Q22 on the *shape* axis only. Spec: `docs/design/adaptation.md`. | ✅ Decided 2026-08-08 |
| 32 | Rung count per Adaptation ladder | Undecided. Default is the four tiers that already exist; a fifth rung means inventing an HP/DPS row, which is the inference this project bans. Owner not asked. | Open |
| 33 | How many captains a run supports, and whether captain retinue draws Supply upkeep | Undecided. The ≤ 8 retinue cap is a legibility call (half `TypeLegibilityCeiling`, `SwarmSubsystem.h:59`); neither the captain count nor the upkeep question was put to the owner. | Open |
| 34 | Adaptation shop price | Undecided, and **blocked on Q26** — nothing can be priced before gold's rate and sources exist. Recorded as an explicit `null` in `unit-types.json` rather than guessed. | Open |
| 30 | End-of-run headcount target | Deliberately **not a design question** — answered by measurement (§4). Every friendly-army figure the project owns sits at 100–120, and only because that is the harness cap; task-108 must sweep retinue past 120 and set it. Until then every end-of-run headcount in this repo is UNVERIFIED. **Re-scoped 2026-08-13:** the commanded force is fixed at seven; the sweep now prices the *allied garrison* (the war), where the measurements still bind. | ⏸ Deferred to measurement 2026-07-31 · re-scoped 2026-08-13 |

---

*Next revision should (reset 2026-08-13 for the castle pivot): absorb the pivot into
§1–§4/§9/§11 properly once the register's intro-blocking set (Q23/Q14/Q15/Q16) closes;
spec the boss fight beyond its stat block — now the stalemate-breaker, the most
load-bearing content in the game; and re-derive the wave curve (Q25) as per-layer
front populations. The classes' kits question became the Q23 channel kit (Q29 = A).*
