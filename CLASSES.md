# Class Design — v1 Roster

**Version:** 0.4 (4-class roster, role-only identities) · Companion to `GDD.md` §5 · Last updated: 2026-07-11

**Tone decision:** *We are the good guys.* Heroes descend into the dungeon to liberate
it, not to plunder it. This drives the unifying retinue theme:

> **Your army is what you save.**
> Every class grows its retinue by rescuing, restoring, or rallying something the
> dungeon has taken — people, guardians, creatures. Recruitment *is* heroism, and it
> naturally feeds the decision system (§6) and world flags (§6a): who you save is who
> fights for you, and the world remembers.

Each class is defined across the same seven slots so they stay comparable:
**fantasy · hero kit · retinue · growth · stance reflavors · scaling · decision hooks.**

---

## Roster overview

| | **Vanguard** (melee) | **Relickeeper** (fortifier) | **Pathfinder** (ranged) | **Lampbearer** (healer) |
|---|---|---|---|---|
| Count vs. quality | **High count**, disciplined | **Mid count**, durable | **Low count**, elite | **High count**, fragile |
| Retinue | Liberated soldiers & militia | Awakened ancient guardians | Hunting pack & scouts | Guided souls & light-wisps |
| Growth verb | **Rescue & rally** | **Excavate & awaken** | **Bond & train** | **Kindle & guide** |
| Army feel | A marching legion | A slow, unstoppable escort | A fast, precise strike team | A drifting constellation |
| Hero role | Front-line anchor | Fortification / control | Skirmisher / marksman | Sustain / vision |

The four classes occupy distinct corners of the count-vs-quality space, so the
"massive entity" fantasy shows four faces: *the many*, *the tough*, *the few*,
*the light*. Party pitch: **legion holds, fortress endures, pack deletes, light
sustains.** Any class works solo; the four together sing — never required.

With four classes the support role splits cleanly:
- **Relickeeper** = *prevention* — walls, wards, damage reduction, battlefield shape.
- **Lampbearer** = *restoration* — healing, revives, vision, attrition recovery.

### Name candidates (working name in bold)

- Melee: **Vanguard** · Banneret · Bulwark · Warden-Captain · Liberator
- Fortifier: **Relickeeper** *(chosen 2026-07-09)* · alternates considered:
  Antiquarian · Reliquarian · Wardkeeper · Awakener · Lorewarden · Custodian ·
  Runekeeper · Delver · Chronicler · Archeologist · Curator
- Ranged: **Pathfinder** · Huntmaster · Trailwarden · Falconer · Outrider
- Healer: **Lampbearer** *(chosen 2026-07-09)* · Lightwarden · Candlekeeper ·
  Luminary · Dawnkeeper · Shepherd of Souls

### Hero identities (role-only — revised 2026-07-11)

Each class is played as a fixed individual with a visible face, but **no proper
name** — the role *is* the identity (reversing an earlier same-day "named hero"
decision). The retinue and cast around each hero turn over too often in this game
for a proper name to hold; the class handle already carries the weight. Full
fiction lives in `docs/narrative/`, sprite/portrait specs in `docs/art/`; the
identity blocks below are the citable summary.

---

## 1. VANGUARD — *the many*

**Fantasy:** You are the shield at the front and the banner they follow. Every cell
door you break open, every conscript you free, your line grows longer. By the late
run you are marching a liberated army through the dark.

### The hero

*Fiction: `docs/narrative/hallam.md` · sprite/portrait: `docs/art/hallam.md` · palette: `docs/art/hero-palettes.md`*

- **Who:** 38, a quarryman's son from the Gatecamp; nine years in a Legion pen, where
  the ledger listed him as a row and a mark. He answers with names — he keeps the
  count, and the count of people still in the pens is not zero.
- **The face:** big, heavy-boned, deliberate; broken nose set crooked, beard greying
  early at the jaw; a chain-gall scar ring at the throat, visible in every frame
  including the portrait. Legion-pattern helm with the visor unbolted and gone —
  *"a banner needs a face under it."*
- **His light — the Roll:** a Legion muster-flag turned inside out, scoured of the
  crown, restitched with the name of every person he brought out alive; the names of
  those he didn't are folded under the hem. Art: **Roll-Gold `#f0b84a`**, cloth and
  thread only — planted, the banner is the one flapping gold rectangle on screen.
- **Reads as:** *a Legion shield with an open face and a rolled-up flag, standing
  where the line is thickest.* Widest hero; 1px off-axis banner pole (never reads as
  an officer crest); warm face inside a Legion silhouette — the Bree trick at hero scale.

### Hero kit (working)
- **Weapon:** sword & tower shield; short-reach, high-stagger melee.
- **Signature — Banner Slam:** plant the banner; retinue in radius gains attack speed
  and fights to the death (no retreat/flee behavior) while it stands.
- **Mobility — Shield Rush:** short charge that knocks enemies aside and *bodyblocks*
  for the hero's escort — pushes a lane open for the retinue behind.
- **Ultimate — The Muster:** every friendly unit on screen rallies to the banner and
  delivers one synchronized charge. The class's "screen-wide moment."

### Retinue: the Liberated
- Freed prisoners, conscripts, and militia. Individually weak, strong in ranks.
- Unit ladder: **Freed** (improvised weapons) → **Militia** (armed) → **Veteran**
  (survives 2+ floors; better stats, holds formation under fear) → **Bannerman**
  (rare; carries a mini-banner that buffs nearby Liberated).
- Formation behavior is the identity: they form ranks, hold walls, brace for charges.

### Growth: Rescue & Rally
- Primary source: **rescue sites** (cells, slave pens, besieged survivors) seeded by
  the generator. Freeing them = recruiting them.
- Secondary: survivors of hard fights **promote** (Veteran system) — the Vanguard is
  the class that cares whether individual units *live*, opposite of a swarm class.
- Tension: rescues are noisy/defended — growth always costs a fight or a §6 decision.

### Stance reflavors
- **Charge → Advance the Line:** ranks move as a wall, shields up — slower than a
  generic charge but units take reduced damage while advancing.
- **Hold → Shield Wall:** the defining stance. A braced line that blocks enemy
  pathing entirely; enemies must break it or go around.
- **Rally → To the Banner:** collapses on the *banner* if planted, else on the hero.

### Scaling & decision hooks
- Scales on **count + formation multipliers** (each rank of a wall buffs the rank
  behind it) — the "legion" curve; late-run Vanguard is the biggest army on screen.
- Decision hooks: *Do you rescue the prison block (big recruit, raises the alarm) or
  the armory (upgrade every current unit)?* · *Veterans of a fallen co-op partner's
  retinue can be folded into yours — take them, or let them carry your ally's banner
  as an honor guard NPC?* · World flag: settlements you liberate stay liberated (§6a).

### 2-bit readability
- Reads as *geometry*: straight lines and rectangles of units. Formation shapes are
  the visual signature; a Bannerman's flag is a 2-value flip animation for cheap flair.

---

## 2. RELICKEEPER — *the tough* (the fortifier)

**Fantasy:** The dungeon was not always a tomb. You read the old walls, restore the
broken wards, and wake the guardians who defended this place before it fell. Your
"spells" are restorations; your army is the dungeon's own immune system, remembered.

*Role note (4-class split):* the Relickeeper is **prevention, not healing** — walls,
wards, and battlefield shape. Healing belongs to the Lampbearer (§4). The Relickeeper
is the class that decides *where* fights happen; the Lampbearer decides how long the
party survives them.

### The hero

*Fiction: `docs/narrative/edda.md` · sprite/portrait: `docs/art/edda.md` · palette: `docs/art/hero-palettes.md`*

- **Who:** 64, the last apprentice of the Vault-Tenders — the hereditary guild that
  kept the kingdom's wards fed and its guardians on shift, and died at its posts in
  the fall. Not a wizard: a maintainer. To her the Undervault is a neglected building,
  she has the keys, and broken means fixable — all of it is *catching up on the backlog.*
- **The face:** small, straight-backed, mason's forearms; white hair cropped practical
  under a tender's cap; round work-spectacles, one lens ground from ward-glass and
  faintly warm in the right light, which she denies. Dry, appraising, unhurried — a
  professional estimating a job everyone else calls impossible.
- **Her light — the waking-ink:** the Vault-Tenders' own pigment, ember-ash bound in
  wax; she carries her mother's last jar and renders more from every shrine she
  restores. Her marks are banked fires; the Crown's sigils are cold geometry. Art:
  **Waking Ember `#e87d3a`**, glyph and seam pixels only — never at rest on the
  Relickeeper herself; the ink stays in the jar until a mark needs making.
- **Reads as:** *a small stonemason with a chisel-staff, foreman to walking
  fortress-blocks, leaving lit letters on the world.* Smallest hero, squarest stance,
  sentinel cadence; the Graver rides her back on a strap, because it is a mason's tool.

### Hero kit (working)
- **Tool:** the Graver — a chisel-staff; mid-range strikes inscribe runes on enemies
  and terrain (modest damage, marks stack into rune detonations). It **resonates**
  near dormant guardians and hidden stonework — the class's exploration sense.
- **Signature — Mend Stone:** channel onto an Awakened unit: repair its cracks;
  *over-mending* temporarily upgrades it (gilded state). Guardians only — players
  are the Lampbearer's job.
- **Field — Ward Circle:** inscribe a zone: allies inside take reduced damage and
  Liberated/pack units won't rout. The party's "hold this ground" button.
- **Ultimate — Remembrance:** the floor briefly *remembers what it was* — all Awakened
  units surge to full power, broken terrain features re-form as cover, enemies of the
  dungeon's old enemy faction are staggered. Scales with secrets found this floor.

### Retinue: the Awakened
- Stone sentinels, rune-plinths, animated reliquaries — the dungeon's original
  guardians, restored.
- Mid count, **very durable, slow**. Sentinels taunt and block; rune-plinths anchor
  ward zones; reliquaries are walking auras. The retinue is a moving fortress the
  party fights from.
- Units don't flee and don't rout — they are stone. They *crack* instead (visible
  damage states), and a cracked unit that survives the floor self-repairs.

### Growth: Excavate & Awaken
- Primary source: **dormant guardians** embedded in the level (statues, sealed
  alcoves, buried shrines) — found by exploring, sensed by the Graver's resonance.
  The generator guarantees a baseline; thorough exploration finds more.
- Secondary: **relic fragments** from bosses/secrets assemble into rarer guardian
  types (the class's "collect the set" hook — future loot-system synergy, §8).
- Tension: awakening takes a channel in dangerous places — the party must cover you.

### Stance reflavors
- **Charge → Unearth:** guardians surge forward and *dig in* where they stop,
  becoming temporary cover/turret line rather than chasing.
- **Hold → Bulwark:** sentinels interlock into literal wall segments.
- **Rally → Procession:** guardians orbit the hero; healing wisps prioritize players.

### Scaling & decision hooks
- Scales on **quality + aura stacking**: fewer units, but each reliquary/wisp aura
  multiplies the *whole party's* output — the co-op force-multiplier class.
- Decision hooks: *A guardian is fused with something dark — awaken it anyway (strong
  off-class hybrid unit, §5 hybrid rule) or purify it (weaker, but a world flag: the
  shrine stays cleansed)?* · *Spend relic fragments now or carry them (lost on death)
  toward the set-piece guardian?*
- World-flag natural: restored shrines/sites persist across runs — the Relickeeper
  is the class that most visibly *changes the world* (§6a).

### 2-bit readability
- Reads as *mass*: big, slow, blocky silhouettes vs. the Vanguard's thin lines.
  Damage = visible cracking (dither patterns), a perfect 2-bit trick. Rune marks
  use a reserved palette value so inscriptions read through crowds.

---

## 3. PATHFINDER — *the few* (the "Range" slot)

**Fantasy:** You walk ahead of the army. You, a hawk, a brace of hounds, and the
handful of scouts you've trained — and that is enough, because you never fight fair.
Marks, traps, and lines of sight are your weapons; the pack executes.

### The hero

*Fiction: `docs/narrative/merle.md` · sprite/portrait: `docs/art/merle.md` · palette: `docs/art/hero-palettes.md`*

- **Who:** young — no one, including them, is entirely sure how young; call it
  sixteen, seventeen. An orphan of the Fall, what they call **the great end
  war**. Not raised at the Gatecamp — kept alive feral in the collapse-lands
  above the Undervault, alongside a stray hawk and two feral war-dogs, until a
  patrol found the group holding off a hush-maw with thrown stone and nerve
  alone and recommended "recruitment, not rescue." Their hunt-law is scripture:
  *nothing you mark is ever left wounded in the dark.*
- **The face:** war-scarred in a way that reads before it explains — a burn or
  blast-scar runs scalp to jaw on one side, old enough to have gone smooth,
  young enough that the brow and cheekbone underneath sit slightly wrong. It
  takes the read of the face with it: the features a glance uses to sort a
  person by gender simply aren't there to sort. They don't correct anyone's
  guess, and don't confirm one either. *"Pick whichever helps you remember to
  duck when I say duck."* They/them.
- **Their light — waylight:** pale lamp-lichen that grows only where honest
  light has burned long, cut with tallow into marking-paste. Not inherited —
  taught to the whole feral group by one of the other strays, a scrap of
  pre-Fall trail-lore in her head. They've paid it forward on every quarry
  since. Art: **Waylight `#d9f0b8`**, contour and pip only, never on any
  friendly sprite — the Pathfinder carries no bright at rest; their light is
  spent entirely on others.
- **Reads as:** *the small quick shape ahead of the army, and the pale outline
  on the thing that is about to die.* The one friendly sprite that never
  marches in step; their bow arc is the only curve in the hero row.

### Hero kit (working)
- **Weapon:** longbow (or repeating crossbow — art test); charged shots pierce.
- **Signature — Mark Quarry:** tag an enemy: the entire pack focus-fires it, and it's
  revealed through walls. The class's core loop — you *choose what dies*.
- **Utility — Snare Line:** place a tripline between two points; enemies crossing it
  are rooted. Turns corridors into kill zones; scales beautifully with procgen.
- **Ultimate — The Hunt Is Called:** every marked enemy on screen is simultaneously
  attacked by spectral echoes of the pack; each kill during the Hunt refreshes it once.

### Retinue: the Pack
- **Low count, elite, named — found family, not inherited stock.** **Relay**, an
  orphaned Legion signal-hawk (scout/reveal); **Latch** and **Ash**, feral
  mongrel descendants of Legion war-dogs (chase & pin); human scouts — fellow
  war-orphans the Pathfinder found or who found them, the roles deliberately
  blurred; rare exotic bonds (see growth).
- Every unit is individually visible, individually named, and *individually mourned* —
  the emotional inversion of the Vanguard's crowd. A name is the one thing the
  Pathfinder still gives away for free. Pack members that fall can be found
  again... changed (decision event).
- Cap is small (≈6–12 late run) but each member takes upgrades like a mini-hero.

### Growth: Bond & Train
- Primary source: **bonding events** — wounded beasts to heal, wild creatures to feed,
  captive animals at rescue sites (shares sites with Vanguard in co-op: he takes the
  people, you take the kennels — automatic co-op texture).
- Secondary: **training** — pack members level per floor survived and per marked
  target killed; deep individual upgrade trees instead of unit count.
- Elite capture: some minibosses can be *spared at the kill* and bonded — the
  class's hybrid mechanic (§5 hybrid rule) and its biggest decisions.

### Stance reflavors
- **Charge → Loose the Pack:** pack sprints ahead independently, hunting marked
  targets first — the only retinue that operates off-screen and reports back
  (hawk reveals map as it flies).
- **Hold → Ambush:** the pack goes prone/hidden; first strike from hidden crits.
- **Rally → At Heel:** tight orbit, no engagement at all — the only true stealth
  stance in the game; enables a scouting/bypass playstyle.

### Scaling & decision hooks
- Scales on **multipliers, not bodies**: mark damage, crit chains, per-member upgrade
  stacking. Late-run Pathfinder deletes elites/bosses while the Vanguard holds the line
  — clean co-op complementarity.
- Decision hooks: *The dire beast that killed your hound can be bonded — take the
  monster that took from you?* · *Send the hawk to scout the far exit (information)
  or keep it in the fight (power)?* · World flag: species you bond become allied
  fauna in that biome across runs.

### 2-bit readability
- Reads as *motion*: few sprites, high animation budget each. The pack darts while
  armies march. Marks are a reserved palette-value outline — instantly readable
  through any horde.

---

## 4. LAMPBEARER — *the light* (the healer)

**Fantasy:** Someone has to carry the last light into a place like this. The dungeon
is full of the lost — souls, strays, the dim remainders of everyone it swallowed —
and they turn toward your lamp like moths. You didn't recruit an army. You lit one.

*Role note:* the party's **restoration** — healing, revives, vision, attrition
recovery. Where the Relickeeper decides where fights happen, the Lampbearer decides
how long the party can keep fighting them. Support = light is also *information*:
this class owns vision and reveals.

### The hero

*Fiction: `docs/narrative/noll.md` · sprite/portrait: `docs/art/noll.md` · palette: `docs/art/hero-palettes.md`*

- **Who:** mid-twenties. Does not carry her own lamp — carries her mentor's, the
  Vesper Halls tender who trained her and died with it in reach. She was not the
  chosen successor; no Lampbearer chooses her replacement. She was simply the one
  standing there when a snuffer caught the mentor first, and the mentor's last
  reflex was shoving the lamp into her hands.
- **The face:** features gone quietly severe from Vesper Halls damp and years of
  bad light; the pallor of someone who reads by lamp more than sun. Wears the
  mentor's old tender's wrap without having earned the rank it once implied.
  Steady, burn-flecked hands; a stillness that reads as calm until you notice
  she's always listening — to the lamp at her hip.
- **Her light — the Borrowed Lamp:** never relit, because she was never the one
  who lit it. It holds her mentor's flame the same way it holds every flame she
  catches below: the mentor's soul is captured inside, aware, present, able to
  be heard — an ongoing voice, not a silent relic. Art: **Watch-Lamp `#ffe9c2`**,
  flame + halo + wisp points only, shared by every honest lamp in the game (the
  one-flame rule); the lamp renders lit and upright in every state, including
  hero-down.
- **Reads as:** *one warm light walking like a woman, with a sky of small lights
  around her and a room that believes her.* The only bare head in the hero row;
  her presence shifts the room's palette one value brighter.

### Hero kit (working)
- **Tool:** the Lantern-Staff — a cone of revealing light: modest holy damage,
  **reveals hidden glyphs, secrets, traps, and weak points**, and dims/staggers
  creatures of the dark at close range.
- **Signature — Kindle:** channel onto a player or unit: heal over time; *overheal*
  becomes a temporary light-shield. On a downed player, Kindle revives faster than
  the standard pick-up — the Lampbearer is the best rescuer.
- **Field — Sanctuary:** plant the lamp: a standing circle of daylight. Allies inside
  regenerate; the Guided orbit its rim as a living lantern-wall; enemies of the dark
  won't cross the light without being burned. (Complements, not duplicates, Ward
  Circle: Ward reduces damage, Sanctuary restores it — stacking them is a co-op play.)
- **Ultimate — Daybreak:** for a few seconds the floor is lit as it was in daylight —
  full map vision of the floor, party healed in waves, dark-aligned enemies feared,
  and every Guided soul blazes to double strength. The scarier the floor's darkness,
  the better this feels.

### Retinue: the Guided
- **Captured soul-flames** — the newly dead of the Undervault, caught before the
  dark claims them (WORLD.md §3c) — a drifting constellation of small lights
  around the hero.
- **High count, fragile, non-martial.** The Guided don't hold lines or deal real
  damage: they *sustain*. Wisps carry heal-pulses to wounded allies (visually: a
  light darts from the flock to the injured), soul-lights extend the hero's vision
  radius, censer-moths cleanse debuffs.
- The inversion that makes it work at scale: every other retinue converts count into
  damage — the Guided convert count into **throughput of care**. A late-run
  Lampbearer with hundreds of lights is healing an entire 4-player army through a
  horde fight. That is this class's massive-entity spectacle.
- The Guided are fragile but not tragic: a wisp that "dies" gutters out and re-lights
  at the next Sanctuary (regenerating retinue — attrition, not permadeath).

### Growth: Kindle & Guide
- Primary source: **the lost** — souls adrift in dark rooms, survivors huddled where
  the light died. Reach them with the lamp and they join the constellation. Darkness
  is literally this class's resource map: the *worst* places hold the most lost.
- Secondary: **braziers and beacons** — dead light-fixtures seeded by the generator.
  Relighting one grants wisps, permanently lights that room, and (in flagged sites)
  can persist as a §6a world flag: *the lamps of this hall stay lit across runs.*
- Tension: the lost are found in the dark, and the Lampbearer must *walk into it* to
  reach them — the healer is structurally the class that strays from the party.

### Stance reflavors
- **Charge → Flare:** the constellation surges forward as a wave of light — brief
  burn damage and blind, the class's only real offense. Leaves the hero unlit
  (vulnerable) until it returns.
- **Hold → Vigil:** the Guided form a fixed lantern-ring: a standing zone of healing
  and vision. The "set up the field hospital" button.
- **Rally → Huddle:** all lights collapse tight around the hero and nearby players —
  maximum regeneration, near-zero vision beyond the huddle. The desperate-last-stand
  stance, and it *looks* like one.

### Scaling & decision hooks
- Scales on **throughput**: wisp count × heal-pulse rate × overheal shielding. Late
  run, the Lampbearer turns the party effectively unkillable *while the lights last*
  — so enemies that snuff lights (darkness elites) are the natural counter-pressure.
- Decision hooks: *Guide the gathered souls home at a shrine — releasing them (lose
  the units, gain a permanent world flag: this floor's souls rest, its haunts are
  gone) — or keep them burning for the run?* This is the game's meta-loop tension
  (§3/§6a) embodied in one class: **power now vs. a healed world.** The
  discipline holds for every stranger's soul without exception — and she has
  never once applied it to the one soul riding in her own lamp, who has asked.
  · *A dying survivor can be saved as a Guided light or escorted (slow,
  dangerous) to a rescue site to fight for the Vanguard instead — whose army
  grows?*
- World flags: relit beacon halls; floors whose souls were laid to rest.

### 2-bit readability
- Reads as *glow*: single-pixel bright points with 1px halo dither on the darkest
  palette value — the cheapest sprites in the game, hundreds nearly free (ideal
  Niagara case: pure visual particles carrying almost no gameplay state each).
- The Lampbearer's presence changes the *palette itself*: rooms shift one value
  brighter inside lamp radius. Vision-as-class-identity, rendered for pennies.

---

## Cross-class design notes

- **Co-op composition is the pitch:** legion holds, fortress endures, pack deletes,
  light sustains. Any solo class works; the four *sing*. Never require the quartet.
- **Shared rescue sites, class-specific yields** (people / guardians / beasts /
  souls) — one procgen system feeds all four growth mechanics. Big scope win.
- **Faction readability rule (from GDD §2):** each class's retinue gets a consistent
  silhouette language — lines (Vanguard), blocks (Relickeeper), darts (Pathfinder),
  points of glow (Lampbearer) — so 2-bit players parse a 4-player battle by *shape*,
  before color.
- **Darkness as a shared system:** the Lampbearer makes light/dark a first-class
  mechanic. Design floors with real darkness so the lamp matters — but never make a
  Lampbearer *required* (baseline hero vision stays sufficient).
- **v2 candidates** (parked): High Priest–style converter, Tinkerer (wants the loot
  system live first), and a "fallen" mirror-class unlocked by dark hybrid choices.

## Open questions (class-level)

| # | Question | Lean | Status |
|---|---|---|---|
| C1 | Final class names | Vanguard / Relickeeper / Pathfinder / Lampbearer | Working names set — revisit at naming pass |
| C2 | Pathfinder weapon: bow vs. repeating crossbow | Bow | Art/feel test |
| C3 | Pack death: permanent vs. "found again, changed" | Found again, changed | Prototype the feels |
| C4 | Veteran promotion visible on-unit in 2-bit? | Yes — helmet pixel-tier | Art test |
| C5 | Do Awakened persist as world-flag site defenders after a run? | Yes, at flagged sites | Tie to §6a flag budget |
| C6 | Guided wisps: pure Niagara particles vs. lightweight Mass entities? | Niagara-first, promote to Mass only for units with gameplay effects | Tech spike |
| C7 | Does darkness reduce baseline vision for all classes, or only hide secrets? | Hides secrets/enemies at range; never blinds | Playtest |
| C8 | Pathfinder growth verb: keep "Bond & Train" or rename to foreground found-family (e.g. "Take In & Train")? | Kept "Bond & Train" — still fits | Flagged by narrative pass, open |
| C9 | Waylight provenance: taught by a fellow orphan, or invented by the Pathfinder alone? | Taught by a fellow orphan | Flagged by narrative pass, open |
| C10 | Does the Lampbearer's mentor get promoted to a full WORLD.md §5 named NPC (own flag/E8 hook)? | No — stays unnamed, off the 15-flag budget | Flagged by narrative pass, open |
| C11 | Does the mentor's soul ever get released (capstone E6 variant)? | Left unresolved by design, on purpose | Flagged by narrative pass, open — revisit if a capstone content pass is scoped |
