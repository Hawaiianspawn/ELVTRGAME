# World & Enemy Design

**Version:** 0.2 · Companion to `GDD.md` §6a/§9 and `CLASSES.md` · Last updated: 2026-07-09

---

## 1. The Setting — the Undervault *(working name)*

A buried mountain-kingdom: a great city carved downward, district under district,
lit for a thousand years by lamp-halls and ward-light. Then something at the bottom
woke, the lights went out floor by floor, and the kingdom drowned in the dark.

**Now:** the surviving descendants hold a camp at the broken gates. Heroes descend
into their own ancestors' city to take it back — floor by floor, run by run.

Why this setting (it's load-bearing, not just flavor):
- **A city, not a cave** justifies everything the classes need: prisons full of
  people (Vanguard), dormant civic guardians in every district (Relickeeper),
  kennels/menageries gone feral (Pathfinder), and the souls of a whole civilization
  adrift in the dark (Lampbearer). *The dungeon is full of things to save because
  people lived here.*
- **Descending districts** = natural biome structure and difficulty curve.
- **"Take it back"** = the world-flag meta-loop has a face: every flag is a piece of
  the kingdom re-lit, and the camp at the gates visibly grows as flags accumulate.

### The Gatecamp *(working name)* — the hub
The survivors' camp at the entrance. **Grows with world flags, never with power:**
rescued smiths open stalls, guided-home souls quiet the camp's nights, liberated
districts send supplies. It's the between-runs screen made physical — proof the
world remembers. (v1 scope: a single screen with 4–6 upgrade states driven by flags.)

---

## 2. The Enemy — the Hollow Crown *(working name)*

At the bottom of the Undervault sits the thing that killed it: a king who would not
die, and the crown that granted the wish by hollowing him out. The Hollow Crown
doesn't rule the dark — it *is* the absence the dark pours out of. It doesn't want
treasure or conquest. It wants **quiet**: every lamp out, every soul still, every
door sealed.

- **The antagonist is an ambience, then a boss.** For most of v1 the Crown is felt
  through the three factions (below) — two it hollowed, one it let in — plus the
  darkness budget of each floor and whispers at decision events. A final
  confrontation is post-v1.
- **Meta-arc:** world flags measurably weaken its grip — relit halls stay lit,
  rested souls stop haunting, liberated districts resist re-occupation. The
  meta-game *is* the war of light vs. quiet.
- Tone guardrail: the kingdom's enemies are tragic, not edgy — its own king,
  soldiers, and dead, taken. Fighting *them* is rescue work by other means. The
  one deliberate exception is the Unwitnessed (§3a): the alien horror the tragedy
  let in — nothing to mourn, nothing to save. Tragedy and horror, kept distinct.

---

## 3. Enemy Factions

Three factions, each a different face of the fall — and each deliberately
stress-testing a different part of the game. Legion and Quiet standings are world
flags (§6a); the Unwitnessed have no standing, only **sealed breaches**.

### 3a. The Unwitnessed — *the vast* (tests: scale itself)

> *Name candidates:* **the Unwitnessed** (working) · the Vast · the Nameless · the
> Below · the Firstborn · the Old Dark · and the holy-inversion set (Berserk-style):
> the Miracles · the Revelation. "Unwitnessed" wins for now: no one who saw one
> lived to give it a name.
>
> **[REVISIT — parked 2026-07-09]** This faction is a first draft and the owner
> wants another pass: final name, titan designs/variety, and how far to push the
> horror. Come back before content lock; don't build titan content beyond Spike 1
> until then.

**What:** not the kingdom's dead — the things *beneath* the kingdom. When the Crown
wished for quiet, the deepest doors stopped being held, and what came up is older
than the mountain and wrong in ways eyes refuse: too many joints, geometry that
doesn't stay counted, silhouettes that read as terrain until the terrain moves.
Things man was never meant to see — and mostly, briefly, doesn't.

**This faction breaks the tragedy rule on purpose.** The Legion and the Quiet are
the kingdom, taken — you mourn them. The Unwitnessed are the *outside* that the
fall let in. There is nothing to save, nothing to soothe, no oath-stone. The other
factions fear them too: Legion checkpoints barricade against the breaches.

- **Structure — titan + brood (Mass Entity spec):** each Unwitnessed is a
  **colossal full-Actor titan** (multi-tile, room-scale and up, visible across the
  floor) that continuously **sheds a brood** — masses of lesser spawn running pure
  boid steering: flow like liquid, swarm noise and light, no retreat. The brood is
  the tide; the titan is its source. Kill spawn forever, or kill the source.
  - Titans are *walking events*: screen presence, tremor on approach, terrain
    damage. A floor rarely holds more than 1–2.
  - Titan death sends its brood into a brief frenzy, then dissolution.
- **Tech note:** the brood is the **Spike 1 benchmark** (pure steering + count,
  1,000+ entities); titans are the showcase for few-full-Actor bosses. One faction
  proves both ends of the entity architecture.
- **Counterplay by class:** Vanguard's line dams the brood while the party works
  the titan; Relickeeper's wards block charge lanes and Unearth digs a chip-damage
  turret line; Pathfinder's Mark Quarry shines — titans have marked weak points;
  Lampbearer keeps the long fight survivable, and lamplight *reveals* weak points
  the dark hides.
- **The seal (instead of a save):** breaches can be **sealed** — a multi-step
  in-run objective at the chasm the titans climb from (world flag: *breach sealed*
  — titan pressure in that district drops across future runs).
- **The temptation:** a slain titan's heart can be **harvested** (immense run-power
  boon, the game's biggest) or **burned at the breach** to complete a seal. Power
  now vs. a healed world, at maximum stakes — and harvesting is the kind of act
  the Quiet notices. **[Hybrid/corruption hook, see GDD §5]**

### 3b. The Still Legion — *the ranks* (tests: formation vs. formation)
**What:** the kingdom's own army, still at their posts, hollowed. They keep order in
the dead city: patrol routes, shield lines, prisoner columns. They don't hate —
they *administrate*. It's worse.

- **Horde behavior (Mass Entity spec):** formation slots — shield fronts, pike
  seconds, crossbow ranks; rotate on flank, hold chokepoints, escort prisoner
  columns between floors. The mirror-match for the Vanguard and the game's
  tactical mid-weight fight.
- **They hold the prisoners.** Rescue sites (all four classes' growth, §CLASSES)
  are overwhelmingly Legion-guarded: the faction you must break to grow.
- **Counterplay:** flanks and breaks — Pathfinder marks officers (formations decay
  when officers fall), Vanguard charges the corner, Relickeeper's Unearth digs a
  turret line into their route, Lampbearer's Daybreak fears whole ranks.
- **The save:** some hollowed soldiers can be *recalled* at their old oath-stones —
  a decision event: a recalled veteran is an elite recruit for the run, or released
  to rest (world flag: their unit stops mustering on this floor).

### 3c. The Quiet — *the dark itself* (tests: the light, and nerve)
**What:** where the Crown's silence pools: snuffers, hush-maws (ambushers that hunt
by sound), and the Unlit — lost souls that were never guided home, now hostile.
Few, elite, terrifying.

- **The mechanism — soul-flames:** in the Undervault, the dead rise as a small
  floating flame — a soul, briefly its own, adrift. Left unclaimed too long, a
  flame either turns hostile on its own or is eaten by something that hunts them.
  A Lampbearer's lamp exists for exactly this: find and capture a flame before
  either happens. This is the mechanism behind three things that previously had
  none: **the Guided** (`CLASSES.md` §4) *are* captured flames, safely kept;
  **the Unlit** are flames nobody reached in time, turned; **snuffers** are not
  generic "lamp-eating wraiths" — they are the things that eat unclaimed flames,
  full stop, and a snuffer stalking a brazier and a snuffer stalking a fresh
  death are the same hunger. (This does not extend to players' own downed/revive
  state — that stays a separate, mundane mechanic. **[Parked — maybe later]**)
- **Horde behavior (Mass Entity spec):** inverse of the Blightbloom — low count,
  full-Actor elites; stalk light sources, extinguish braziers (undoing Lampbearer
  work mid-run), phase between dark rooms, retreat from Sanctuary light.
- **The pressure faction:** the Quiet is why darkness matters and why the Lampbearer's
  fantasy lands — but per the class guardrail, they must be killable by lightless
  parties (harder, never impossible).
- **The save (the cruel one):** the Unlit *are* rescueable souls — every Unlit killed
  is a soul destroyed; every one soothed (Lampbearer Kindle-channel, or any class at
  cost) is a soul saved. The game's sharpest routine decision: DPS or mercy, in
  combat, repeatedly. World flag: floors whose Unlit were mostly soothed vs. mostly
  destroyed *remember which*.

### Faction interplay
- Factions contest each other: Unwitnessed broods overrun Legion barricades; the
  Quiet pools in the wake of titans (silence follows where they've passed). Floors
  seed 2 factions with a border — players can bait them together (emergent-tactics
  budget, cheap to build: they already target "nearest non-allied").
- **Escalation:** the deeper the floor, the larger the titans and the more the mix
  tilts toward the Quiet. Floor 1 is a war; the last floor is a silence broken only
  by things too vast to be quiet.

---

## 4. Biomes (v1: three, descending)

| # | Biome (working) | District | Dominant factions | Procgen shape | Feel |
|---|---|---|---|---|---|
| 1 | **The Highgates** | Gates, barracks, prisons | Still Legion (+Blight edges) | Rooms & corridors, patrol loops, big gate-arenas | Occupied city — break the order |
| 2 | **The Sunken Works** | Cisterns, waterworks, mines — the breach district | The Unwitnessed (+Legion barricades) | Open caverns, chasms, flooded halls, breach arenas | Face what came up — titan country |
| 3 | **The Vesper Halls** | Temple district, reliquaries, lamp-halls | The Quiet (+dormant guardians everywhere) | Dark labyrinth, light wells, shrine rooms | Dread and awe — carry the light |

- Each biome is the home turf of one faction and the showcase of (roughly) one
  class's fantasy — Highgates/Vanguard, Sunken Works/Pathfinder+Relickeeper (mark
  the titan, hold the breach), Vesper Halls/Lampbearer — while staying fully
  playable by all.
- Arena guarantee (§9 GDD) per biome: gate plazas, breach chasms, lamp-hall naves.
  The Sunken Works' arenas must be *titan-scale* — the procgen's largest rooms.

---

## 5. Named NPC seeds (v1: keep to ~5)

World-flag anchors (§6a) — people whose state persists across runs:

1. **Warden-Captain Bree** *(working)* — last officer of the true Legion, holding a
   stairwell for years. Recruit her (elite Vanguard-style NPC for the run) or hold
   her post *with* her (flag: the stairwell becomes a safe room in all future runs).
2. **Maro the Chainwright** — a smith kept enslaved by the Legion to forge shackles.
   Freed: opens the Gatecamp forge (flag). His asks get harder each run he's helped.
3. **The Last Lamplighter** — a Guided soul that remembers being someone. The
   Lampbearer's personal arc seed: guide him home (flag: his route of braziers stays
   lit forever) or keep him — he's the strongest wisp in the game.
4. **The Kennel Matron's Hound** — the feral pack-mother of the old royal kennels.
   Pathfinder can bond her (apex pack member) or free her litter to the Gatecamp
   (flag: hounds guard the camp; all classes get a one-hound escort option).
5. **The Doorwarden** — a colossal dormant guardian fused into the Vesper Halls'
   great door. Waking him is a multi-run relic-fragment project (Relickeeper arc;
   flag-chain), and he is the key to whatever is behind that door (post-v1 hook).

---

## 6. How this feeds the systems

- **World flags (§6a budget ~10–15):** 2 faction standings (Legion, Quiet) + 5
  named NPCs + ~6 site states (breaches sealed ×2–3, stairwell held, brazier-route
  lit, kennels freed, district liberated) = right at budget. **Locked in §7 below
  (15 flags).**
- **Decision events:** every faction carries a signature dilemma (recall vs. release
  a soldier; soothe vs. destroy the Unlit; harvest the titan's heart vs. burn it to
  seal the breach). Event templates come from factions, not from a separate pool.
- **Tech spikes:** Unwitnessed brood = Spike 1 (mass steering, 1,000+); Still
  Legion = Spike 3+ (formation slots); the Quiet + titans = few full Actors (cheap,
  ship-shaped early). One world, both ends of the entity architecture proven.
- **Name pass:** the game name likely lives here — *Undervault*, *The Vesper Halls*,
  *Lamplight* + a noun, *Hollow Crown* as subtitle material. Park for the naming pass.

## 7. The v1 World-Flag List (draft — the ~15)

The complete set the save file tracks. Rules from GDD §6a: flags are enums the
generator/event system read; effects change **what you encounter, never your
stats**; in co-op they live in the host's world.

### Faction standings (2)
| Flag | Moves when… | World effect |
|---|---|---|
| **F1 · Legion at Rest** (0–100) | Hollowed soldiers released at oath-stones; officers laid to rest | Higher: fewer/weaker Legion musters, more lightly-guarded rescue sites; oath-stones more common. Low: Legion patrols reinforce. |
| **F2 · Souls at Rest** (0–100) | Unlit soothed vs. destroyed | Higher: fewer Unlit spawn, Gatecamp nights go quiet, Vesper Halls shrines pre-lit. Low: more Unlit, whispers at events. |

### Named NPCs (5)
| Flag | States | World effect |
|---|---|---|
| **N1 · Warden-Captain Bree** | unmet → met → recruited / **post held** / fallen | Post held: her stairwell is a permanent safe room every run. Fallen: her post spawns as a Legion strongpoint. |
| **N2 · Maro the Chainwright** | enslaved → freed → **forge open** (quest chain) | Forge open: Gatecamp vendor (run-scoped purchases only). |
| **N3 · The Last Lamplighter** | adrift → kept → **guided home** | Guided home: his brazier route stays lit across all runs. Kept: strongest wisp in the game, route stays dark. |
| **N4 · The Hound-Matron** | feral → bonded / **litter freed** | Litter freed: hounds guard the Gatecamp; any class may take a one-hound escort. Bonded: Pathfinder apex pack option. |
| **N5 · The Doorwarden** | dormant → fragments 1–3 → **awakened** | Multi-run relic chain; awakened = the great door opens (post-v1 content gate). |

### Sites (8)
| Flag | Earned by | World effect |
|---|---|---|
| **S1–S3 · Breaches Sealed** (Sunken Works ×3) | Burning a titan heart at the breach | Each seal: titan pressure drops in that district's floors; sealed chasm rooms generate as crossable. |
| **S4 · Highgate Plaza Liberated** | Clearing + holding the gate arena event | Runs open with the plaza friendly; Gatecamp visibly expands into it. |
| **S5 · The Kennels Freed** | Kennel event chain | Feral beast spawns in Highgates become neutral/bondable. |
| **S6 · The Great Cistern Restored** | Sunken Works purification event | Flooded halls drain — new room types generate; Gatecamp gains water (cosmetic growth). |
| **S7 · Vesper Naves Relit** | Lamplighter route + brazier events | Vesper Halls generate with standing light wells; the Quiet concedes those rooms. |
| **S8 · The Silent Bell** | Secret: ring the temple bell at the bottom of Vesper | One-way flag, effects deliberately undocumented for players — the mystery flag. **[Design later]** |

**Budget check:** 2 + 5 + 8 = 15 flags. At cap — nothing gets added to v1 without
removing something.

---

## 8. Decision-Event Templates (v1: eight)

Anatomy of every event: **trigger site** (procgen-placed) → **the choice** (2–3
options, one player owns the decision per GDD §6) → **run effect** (immediate) →
**flag effect** (which of §7 it moves, if any). Events are drawn from factions and
NPCs — no generic pool.

| # | Template | Owner | The choice | Moves flag |
|---|---|---|---|---|
| E1 | **The Oath-Stone** | Any | Recall the hollowed veteran (elite recruit this run) vs. release them (Legion at Rest ↑) | F1 |
| E2 | **Soothe or Strike** | Any (in combat) | The Unlit can be fought or soothed mid-fight — soothing takes time under pressure | F2 |
| E3 | **The Titan's Heart** | Party vote | Harvest (biggest run boon in the game, the Quiet notices) vs. burn at the breach (seal progress) | S1–S3 |
| E4 | **The Prisoner Column** | Any | Intercept now (fight + recruits) vs. shadow it to the pens (harder fight, bigger rescue) | — |
| E5 | **The Rescue Site** | Class-flavored | Loud breach (fast, alarms) vs. quiet route (slow, risks the prisoners) — yields people/guardians/beasts/souls by class | S4/S5 feeders |
| E6 | **The Lost's Plea** | Lampbearer-weighted | Gathered souls beg for rest at a shrine: release (F2 ↑, lose the units) vs. keep burning | F2 |
| E7 | **The Dark Bargain** | Individual, private | A whisper offers off-class power (hybrid units, titan-touched boons) at a hidden cost — the temptation track | F2 ↓, hybrid |
| E8 | **The Keeper's Ask** | Party | A named NPC asks something costly this run for a permanent flag (Bree's stairwell, Maro's chain, the Lamplighter's route) | N1–N5 |

Design rules:
- **E2 is the texture** (small, constant, in-combat); **E3/E8 are the tentpoles**
  (rare, loud, party-visible). A floor should average 1–2 events, never zero.
- E7 is deliberately *private* — other players see the outcome, not the offer.
  This is the co-op social-texture engine (credit, blame, suspicion).
- Every template must be authorable as data (trigger + options + effects) — the
  event *system* is one piece of tech, the events are content.

---

## Open questions (world-level)

| # | Question | Lean | Status |
|---|---|---|---|
| W1 | Setting/hub/antagonist final names | Undervault / Gatecamp / Hollow Crown as working names | Naming pass |
| W1b | **Unwitnessed faction — full revisit** (name, titan variety, horror level) | Parked at owner's request | ⏸ Revisit before content lock |
| W2 | Is the Hollow Crown fightable in v1? | No — ambience + hosts only; final fight post-v1 | Decide at content lock |
| W3 | Soothing the Unlit: Lampbearer-only or all classes at cost? | All classes at cost (mercy is everyone's option) | Playtest |
| W4 | Faction-vs-faction combat: full sim or scripted border skirmishes? | Simple "nearest non-allied" targeting, no extra sim | Tech spike |
| W5 | Does the Gatecamp have gameplay (vendors) or is it purely a flag mirror? | Flag mirror + 1–2 flag-unlocked vendors max | Scope check |
| W6 | Titan scale ceiling in top-down 2-bit (how big before camera/readability breaks?) | Multi-room; camera pulls back for titan arenas | Art + tech test |
| W7 | Harvesting titan hearts: corruption consequences (Quiet aggression? visual taint?) | Yes — ties into hybrid/temptation system | Design with decision events |
