# Game Design Document — *Emberkeep*

**Version:** 0.4 (living document) · Companion docs: `CLASSES.md`, `WORLD.md`
**Last updated:** 2026-07-09
**Engine target:** Unreal Engine 5.8

---

## 1. High Concept

A top-down multiplayer dungeon crawler with roguelike structure and procedural level
generation, rendered in a 2-bit art style. The hook: **massive entity counts**. Every
player commands not just a hero, but a growing retinue of subjects/soldiers that fight
alongside them. Runs escalate to extreme power levels, with hundreds-to-thousands of
units and enemies on screen at once.

**One-liner:** *A co-op roguelike dungeon crawler where you don't just build a
character — you build an army.*

---

## 2. Design Pillars

1. **You are many.** The player's power is expressed through their retinue as much as
   their hero. Growth means more bodies, better bodies, and smarter formations.
2. **High power scaling.** Runs go from "a handful of peasants with sticks" to
   screen-filling, absurd late-run power. The fantasy is escalation.
3. **Meaningful decisions on the journey.** Each player faces run-altering choices —
   not just stat pickups, but decisions with tradeoffs, consequences, and (in co-op)
   social weight.
4. **Readable at scale.** The 2-bit style isn't just a resource saver — it's what keeps
   a thousand-entity battle legible. Silhouette, contrast, and motion carry the visuals.
5. **Every run is its own run.** Procedural generation + decision events +
   class/retinue combinations make each run structurally different — new shapes, new
   forks, new army — with occasional authored callbacks from the world flags (§6a).
   Scoped honestly: the v1 persistence layer is "flags, not simulation," so this
   pillar promises *emergent variety with callbacks*, not authored per-run narrative.

---

## 3. Core Loop

### Genre spine — DECIDED 2026-07-21: roguelike run; retinue growth is the progression axis
This is a **roguelike first**. The load-bearing loop is the run — enter, grow your
retinue through decision events, win or die, reset — and **retinue growth (not loot,
not the persistent world) is the primary progression axis and the run's reward.**
The other genres this doc borrows from are explicitly **support, not co-equal**:
- The **RTS / Pikmin-lite** command layer (§4) is how retinue power is expressed
  moment-to-moment. It serves the run; it is not a separate strategy game.
- The **horde-survivor** power fantasy (§7) is the *feel* of late-run escalation,
  not the retention model.
- The **persistent world** (§6a) is **seasoning** — a between-run flavor layer,
  deliberately minimal, never the core reward.
When these pull against each other, the roguelike run wins. Onboarding, market
category, and the vertical-slice reward loop all resolve against this spine.

### Moment-to-moment (seconds)
Move → position your retinue → fight → collect drops → push deeper.

### Run loop (minutes)
Enter floor → explore procedurally generated level → combat encounters + decision
events → grow retinue and power → floor boss / exit → next floor (harder, denser).

### Meta loop (hours) — **DECIDED: Hybrid — knowledge + persistent world state**
Run ends (victory or death) → **no power carries over** → but the decisions made
during the run leave **persistent marks on the world** that future runs encounter →
new run in a world shaped by past runs.

- Player power fully resets each run (roguelike purity in the power curve).
- The *world* remembers: factions you aided or betrayed, sites you razed or saved,
  named NPCs you spared or doomed reappear changed in later runs (see §6a).
- Class/content unlocks may still exist as light discovery gates ("meet the
  Beastmaster in a run to unlock her"), but never as stat inflation.

This makes the meta-loop itself an expression of the "meaningful decisions" pillar:
the reward for a run isn't +5% damage, it's a changed world.

---

## 4. The Niche: Massive Entity Retinues

The differentiator. Each player's class comes with **subjects/soldiers** — autonomous
units that follow, fight, and grow with the player.

### Retinue fundamentals
- Units follow the player by default (formation/leash behavior).
- Units are gained through play: rescued, recruited, summoned, bred, or converted,
  depending on class.
- Units have simple individual behavior but aggregate into meaningful tactics
  (flanking, shielding the hero, swarming).
- Retinue size scales hard over a run — early game: 3–10 units; late game: hundreds
  per player.

### Player control model — **DECIDED: Stance commands**
Units auto-fight, plus a small set of broad orders issued to the whole retinue
(Pikmin-lite). No unit selection, no targeting micromanagement.

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

### Design tensions to watch
- **Hero relevance:** the hero must stay the star even when the army does the killing.
  Hero abilities should be force multipliers (buffs, formations, ults) not just DPS.
- **Screen readability:** friendly vs. enemy mobs must read instantly in 2-bit. Reserve
  a color/value channel per faction.
- **Multiplayer density:** 4 players × hundreds of units × enemy hordes = the tech
  problem that defines this project (see §10).
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
  `SYSTEMS.md` §6 for the mechanics and the open procgen follow-up — does arena
  generation guarantee a chokepoint narrow enough for this to matter?). The
  Relickeeper's Bulwark (`CLASSES.md` §2) makes the identical claim and inherits
  the identical fork; unresolved, since the Relickeeper isn't built in the sim yet.

---

## 5. Classes

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
- **Fork events:** e.g., save the caged prisoners (gain units, alert the floor) vs.
  loot the vault quietly.
- **Sacrifice offers:** trade retinue lives, hero HP, or loot for power spikes.
  **Class-aware pricing (DECIDED 2026-07-21):** retinue permanence differs by class —
  Vanguard Veterans permadeath, Pathfinder pack is "found again," Relickeeper Awakened
  self-repair, Lampbearer Guided regenerate — so a flat "sacrifice N units" template
  is a real loss for one class and a rounding error for another. Sacrifice templates
  price in a **common currency of loss** (banked progression / upkeep supply / world
  standing), not raw unit count, so the choice is comparably meaningful for all four.
- **Path choices:** branching floor exits with telegraphed risk/reward (elite floor
  vs. treasure floor vs. shortcut).
- **Moral/faction choices:** decisions that shift how the dungeon reacts to you across
  the rest of the run (persistent consequence within a run).

### Multiplayer wrinkle
Decisions are **per-player where possible** ("each person will have their
opportunities"): a decision event targets one player, and their choice can affect the
whole party. This creates social texture — credit, blame, negotiation.

**Resolution rule — DECIDED 2026-07-21: vote on weight.**
- **Self / current-run-only** (affects only the deciding player or just this run):
  the owning player decides alone, resolved instantly.
- **Party-wide or persistent** (consequence lands on the party or writes a world
  flag): settled by **party vote**, resolved the moment the vote closes. Passes on
  **simple majority**; absent / downed / disconnected players **abstain**; a tied
  or failed vote keeps the world exactly as it stands, so the run always keeps
  moving forward.
Cheap choices resolve instantly; heavy choices resolve fairly, through the party.

### 6a. Persistent World State — **DECIDED: yes, this is the meta-game**

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
  the 2-bit style and entity tech make affordable.
- Breakpoints and "run-defining" pickups (à la Vampire Survivors evolutions / Risk of
  Rain item stacking) create spike moments, not just smooth growth.
- Anti-cap philosophy: prefer soft costs over hard numeric caps. The primary soft
  cost is **retinue upkeep** (specced below); screen chaos and swarm-punishing elites
  back it up.

**Retinue upkeep — DECIDED 2026-07-21 (spec; tunables in `docs/RTS-VERTICAL-SLICE.md`
§2):** Retinue size is *governed, not capped*. Every active unit draws **upkeep**
from a per-run supply pool replenished only at growth/supply sites. When demand
outruns supply, units don't die — they **degrade** (reduced stats / dimmed /
"unfed"), and newly recruited units enter degraded until supply recovers. One
mechanic, three jobs:
- gives the exponential curve above a real governor — you can't bank hundreds of
  full-strength units without continuously feeding them;
- bounds the entity budget (§10) — degraded units stay in the cheap Mass tier and
  are first demoted from "promoted Actor" status, capping live Actor promotions;
- lets negative world flags (§6a) bite as *supply pressure* instead of pure added
  difficulty, so the world-ratchet has something to push back against.

---

## 8. Loot & Itemization — **[LATER, placeholder]**

Deferred by design, but reserving the slot. Direction notes:
- Vampire Survivors–style: frequent drops, stacking/synergizing effects, evolution
  combos.
- Loot should feed **both** hero and retinue (e.g., a banner item that upgrades all
  soldiers vs. a weapon for the hero) — this is our twist on the formula.
- Rarity tiers, run-based (no persistent gear inventory) to keep the roguelike frame.
- **Base Camp Loot Manager — CUT 2026-07-21.** A cross-player donation cache +
  bot-proxy feature that reopened the host-world rule (§6a) for a parked stretch
  goal, layered on a loot system that doesn't exist yet. Removed from canon to stop
  scope gravity; revisit only if/when the loot system *and* the Gatecamp hub are
  real. (History preserved in `docs/GDD-TODO.md` Part C and this project's design
  session log.)

---

## 9. Procedural Generation

- **Floor-based dungeon**, top-down, room-and-corridor or open-cavern layouts varying
  by biome. v1 biomes (see `WORLD.md` §4): **the Highgates** (occupied city,
  corridors/patrols), **the Sunken Works** (breach district — chasms and
  titan-scale arenas), **the Vesper Halls** (dark temple labyrinth).
- Generation must produce: combat arenas sized for horde fights (big open spaces are a
  *requirement*, not a nicety — retinues need room), decision-event sites, secrets,
  and a floor objective (boss/exit).
- Likely approach: prefab room library + graph-based layout (rooms as nodes, corridors
  as edges), with constraint rules. Site counts **scale with party size**: a floor
  baseline of ≥1 arena, ≥1 decision event, ≥1 optional risk room, **plus ≥1 growth
  site per player** so a 4-player party isn't starved and a solo player isn't flooded.
  Where two players share a class (same yield type), a contested site either **splits
  yield or duplicates** — decided per site type in `docs/RTS-VERTICAL-SLICE.md` — so
  "shared sites, class-specific yields" (`CLASSES.md`) stops silently assuming a
  one-of-each party. Tile-level noise/variation on top.
- Seeded generation — all players in a session see the identical dungeon: the
  generator reads the **session stage's** world flags (the host's world as the
  shared stage), then applies the seed **deterministically on top of the resolved
  flag set** (DECIDED 2026-07-21: seed evaluates *after* flag injection, so "same
  seed" means "same seed + same flags"). Community seeds are therefore shareable
  **at a declared flag baseline** (e.g. a fresh/neutral world); a seed shared
  without its flag state won't reproduce across differing saves. The community-seed
  feature is scoped to neutral-baseline runs, not veteran saves — stated plainly
  rather than implied. (Note: whether the deterministic-ish swarm sim itself desyncs
  under this model is a tech-spike question, tracked under the entity-count spike
  gate in §10, not resolved here.)

---

## 10. Technical Direction

**Engine:** UE5.8. **The defining technical challenge is entity count under
multiplayer.**

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

**Entity-count spike gate — DECIDED 2026-07-21 (go/no-go).** No content or class
production begins until the Mass Entity spikes pass at the **full 4-player target
load** (not the 2-client half-target). Pass = the entity budget above (4 players ×
late-run retinue + enemy hordes) holding 60fps under the aggregate-replication model.
**A gate failure invalidates the concept, it does not merely trim it** — the defined
fallbacks to evaluate are: (a) reduced entity counts, (b) single-player-only mass, or
(c) a fake-crowd hybrid. This is the project's go/no-go before major investment.

### 2-bit rendering
- 2-bit = 4 values per palette. Options: strict global 4-color palette (Game Boy
  style) vs. 2-bit-per-layer with palette swaps per faction/biome (much more practical
  for readability at horde scale — **lean this way**).
- Flat unlit rendering; cheap by design. Budget goes to entity count, not shading.
- Sprites (Paper2D/PaperZD or flipbooks on instanced quads) vs. flat-shaded 3D posing
  as 2-bit — **[DECIDE via art test]**. Instanced flipbook quads driven by Niagara is
  the likely winner for mass units.

### Multiplayer
- Co-op, 1–4 players. **DECIDED.** Entity budget must be proven at 4 players ×
  late-run retinue sizes plus enemy hordes — this is the number Spikes 1–2 target.
- Listen server vs. dedicated — start listen-server for prototyping.
- **Replication strategy is the second-biggest risk:** you cannot naively replicate
  hundreds of units per player. Approach: replicate authoritative *aggregate* state
  (unit group positions/counts/seeds) and simulate cosmetically on clients;
  deterministic-ish local sim for the swarm, full replication only for heroes,
  elites, and bosses. Prototype alongside the Mass Entity spike.

### Early technical milestones
1. **Spike 1 — The Thousand:** 1,000+ Mass Entity units with basic follow/attack AI at
   60fps, Niagara/ISM rendered.
2. **Spike 2 — The Thousand, Networked:** same scene at the **full 4-client target
   load** (raised from 2 so the spike proves the real entity budget, not half of
   it), aggregate-replication model.
3. **Spike 3 — Procedural floor:** graph-based floor generation with arena constraints.
4. **RTS vertical slice:** 1 class, 1 biome, 3 floors, 1 boss, 2 decision events,
   co-op. Full definition, gates, and bill of materials: `docs/RTS-VERTICAL-SLICE.md`.

---

## 11. Scope Guardrails (v1 targets)

| In (v1) | Later | Out (for now) |
|---|---|---|
| 4 classes | 6+ classes | PvP |
| 1–4 player co-op | Fancy loot/evolution system | Modding/community content |
| 3 biomes, floor-based runs | More world flags, evolving sites | Console ports |
| Stance-based retinue control (4 stances) | Class-specific stance variants | Persistent economy/settlement sim |
| World flags (15: 2 standings + 5 NPCs + 8 sites) | Off-class hybrid events (few in v1, more later) | |

---

## 12. Open Questions Log

| # | Question | Decision / Lean | Status |
|---|---|---|---|
| 1 | Meta-progression model | **Hybrid: knowledge + persistent world state (§3, §6a)** | ✅ Decided 2026-07-09 |
| 2 | Retinue control model | **Stance commands: Follow/Charge/Hold/Rally (§4)** | ✅ Decided 2026-07-09 |
| 3 | Classes locked to retinue type? | **Locked start, hybrid via in-run events (§5)** | ✅ Decided 2026-07-09 |
| 4 | Max party size | **4 players co-op (§10)** | ✅ Decided 2026-07-09 |
| 5 | Sprite flipbooks vs. flat-shaded 3D for 2-bit look | Flipbooks on instanced quads | Needs art test |
| 6 | Strict 4-color global palette vs. per-faction palettes | Per-faction | Open |
| 7 | Loot system design (Vampire Survivors–style) | Deferred by design — deferral is now legitimate: retinue growth is the run reward loop (§3 genre spine), not loot | Deferred by design |
| 8 | Co-op world writes | **RESOLVED: party vote on any world-scarring/party-wide decision; no unilateral scars; passed outcomes echo to all present players' worlds (§6a)** | ✅ Decided 2026-07-21 |
| 15 | Genre spine (roguelike vs. RTS vs. horde-survivor vs. persistent-world) | **Roguelike run is primary; retinue growth is the progression axis; the rest are support (§3)** | ✅ Decided 2026-07-21 |
| 16 | Retinue upkeep / power-curve governor | **Degrade-not-die upkeep economy, fed by supply sites (§7)** | ✅ Decided 2026-07-21 |
| 17 | Sacrifice-event pricing across classes | **Common currency of loss, not raw unit count (§6)** | ✅ Decided 2026-07-21 |
| 18 | Decision resolution rule (blocking vs. not) | **Self/current-run = non-blocking solo call; party-wide/persistent = party vote (§6)** | ✅ Decided 2026-07-21 |
| 19 | Pre-resourcing posture + entity-count go/no-go | **Team/budget/timeline TBD; Spike 2 raised to full 4-client load as the gate (§10)** | ✅ Decided 2026-07-21 |
| 9 | v1 class roster | **4 classes: Vanguard / Relickeeper / Pathfinder / Lampbearer** (see `CLASSES.md`; working names) | ✅ Decided 2026-07-09 |
| 10 | Game name | **Emberkeep** — chosen from the light-in-darkness theme (soul-flames, lamp-halls, the war of light vs. the Quiet) | ✅ Decided 2026-07-21 |
| 12 | World/setting design (factions, biomes, antagonist, NPCs) | **See `WORLD.md`** — Undervault / Hollow Crown / 3 factions / 3 biomes / 5 NPCs (working names) | ✅ Drafted 2026-07-09 |
| 13 | Unwitnessed faction (name, titan variety, horror level) | First draft in `WORLD.md` §3a | ⏸ Parked — revisit before content lock |
| 14 | v1 world-flag list + decision-event templates | **15 flags / 8 templates — `WORLD.md` §7–8** | ✅ Drafted 2026-07-09 |
| 11 | Final class names (per-class candidates in `CLASSES.md`) | Working names in use | Naming pass later |

---

*Next revision should: name the game, detail the first three classes (kits, retinue
growth, stance variants), and draft the v1 world-flag list.*
