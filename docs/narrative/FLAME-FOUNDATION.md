# The Flame — Narrative Foundation

**Version:** 0.1 · **Status:** core premise locked 2026-07-22 (owner) · **Supersedes:** `WORLD.md` in full
**Companion:** `GDD.md`, `docs/GATE1-FUN-PROTOTYPE.md`

> **This is a total reset.** Per owner directive 2026-07-22, the Undervault / Hollow
> Crown / Still Legion / the Quiet / the Unwitnessed / the 5 named NPCs / the 15 world
> flags / the 8 event templates are **all discarded**. Nothing in `WORLD.md` is canon.
> It is kept unedited as history (project convention) and carries a superseded banner.
>
> This document is deliberately **short and mechanics-first**. It establishes the
> premise and what the premise *demands of the game*. It does not name factions,
> biomes, or NPCs. Those get written after the fun is found, not before — the whole
> reason for the reset is that set dressing outran the prototype.

> **AMENDMENT 2026-07-31: the army's arms-and-kit register is OPEN, and that is a
> decision, not an oversight.** Owner call, made against the twelve-look Pathfinder
> group. Firearms, mechanical siege pieces, emplaced turrets, powered/masked helms and
> overt mage kit all ship on player units. A future session must not narrow these back
> to bows-and-leather on the grounds that the setting "is medieval."
>
> **This amendment exists because nothing here ever banned them.** The prohibition
> ("no guns, no energy weapons, no glow") was written into `task-125`'s brief as an
> *inference* from §1's pitch-dark premise and was never a line in this document. The
> inference was wrong, it cost a generation pass aimed at the wrong register, and
> writing the decision down is how that stops repeating. §5 below still holds: the
> world is deliberately unwritten, and an unwritten world does not imply a medieval one.
>
> **What earns its place, per §2's test:** the looks do mechanical work, so they are
> not decoration. Unit type has to be legible in a mass of hundreds at a 56px cell —
> the player reads *what a squad is* off silhouette alone, at a glance, with no unit
> selection to fall back on (§2, "a god is obeyed in intent"). A visibly distinct arms
> register per unit type is the readability mechanism, and it is the reason the render
> path carries per-type sub-tables at all.
>
> **What this does NOT license:** it does not reopen the palette
> (`docs/art/aesthetic-direction.md`'s 2026-07-28 amendment governs colour, unchanged);
> it does not move the chibi gameplay register; and it does not make the setting
> science-fiction. The premise in §1 is untouched — the world is still dark, the flame
> is still the only light, and the congregation still follows it.
>
> **One real tension, named rather than papered over:** §1 says power does not exist
> except as it passes through fire, and several of these looks emit their own light (a
> cyan-lit crossbow, a mage's lit orb). Either that emitted light is kit *lit by the
> bearer's flame* rather than a second power source, or §1's clause is about the
> world's ambient dark and not about props. **Not decided.** Flagged so it gets an
> owner call rather than a silent revert of the art.

---

## 1. The Premise

The world is pitch dark. Not night — dark. The dark is the condition of everything,
and it takes what it touches.

**You bear a flame.** It is the only light, and it is a *focus*: power does not exist
in this world except as it passes through fire. To carry flame is to be the only
place where anything can happen.

**People gather to you.** They fight for you, and they need you to live. Step away and
they are in the dark. This is not loyalty — it is physics.

**They do not think you are a person.** A bearer is a god to the people around them.
You did not ask for this and cannot decline it. They will die for you gladly and you
cannot make them stop.

**There are other bearers.** Scattered, alone, each with their own small congregation
in their own small circle of light. Alone, a flame holds a room. **United, flames do
what no single fire can.** The run is the journey to find them and bring the fires
together — and the fight you can only win once you have.

**One-liner:** *You are the only light. Everything that lives, lives inside your
circle — and they think you are a god.*

---

## 2. Why this premise and not a prettier one

Every clause above is doing mechanical work that the game already needs. This is the
test any future lore must pass.

| Fiction | Mechanic it explains | Status in code |
|---|---|---|
| Outside the light, the dark takes you | **The leash** (`LeashRadius`, break latch, `LeashWarnBit`) | **Built.** `SwarmCombat.h` / `SwarmProcessors.cpp` |
| Fire must be fed or it dims | **Upkeep — degrade, don't die** (GDD §7); unfed units literally dim | Specced, not built |
| You are the only light source | **Hero relevance** (GDD §4 tension) without needing hero DPS | Unblocks the 55-DPS problem |
| A god is obeyed in *intent*, not orders | **Stances**, no unit selection, no micro (GDD §4) | **Built.** 4 stances |
| Many bearers, united | **The run objective** — find the other fires and bring them together (see §4.4) | Unbuilt; co-op deferred 2026-07-27 |
| They die for you gladly | **Sacrifice events** price differently — the cost is that they *want* to | GDD §6, unbuilt |

**The rule going forward:** if a piece of lore doesn't appear in that right-hand
column, it is decoration and it waits.

---

## 3. The three things this changes immediately

### 3a. The leash becomes visible, and stops being a rule
The prototype's open question — *"does the leash break read clearly, or does the army
feel like it disobeys?"* (`GATE1-FUN-PROTOTYPE.md` §4) — is answered by rendering the
leash radius as **the lit floor**. Inside: lit ground, units at full value. Outside:
the dark ground state, units dimmed to a lower palette value.

A unit that breaks leash is not disobeying. It is **running back to the light**, which
is the correct and sympathetic read, and it costs one floor decal plus a per-unit
value shift the renderer already has the data for (`bLeashBroken`, `LeashWarnBit`).

### 3b. Hold and Charge get their real meaning
Both stances currently push units toward the edge of the leash, which is where the
fiction says it is *dangerous* to be. That is not a bug — that is the game.

- **Hold** = asking people to stand at the edge of your light, where the dark presses.
- **Charge** = throwing them past it entirely, on faith.

Both should carry a visible cost the player can feel, and the reason to do it anyway
is that the fire needs room. This is the tension the stance system has been missing:
right now stances are movement modes, and this gives three of the four a *price*.

### 3c. The hero's job is redefined
The hero is not a damage dealer with 55 DPS. The hero is **the thing everyone is
standing in.** Hero abilities should be about the *light* — where it reaches, how
bright, how long, at what cost — not about swinging harder. This is the force-multiplier
direction GDD §4 already asked for, now with a reason.

---

## 4. Open — and deliberately open

These are real design risks in the premise. Named here so they get playtested, not
written around.

1. **"Stand in the circle" stagnation.** If light = safety = leash, the dominant
   strategy is to never move and let the army grind. The prototype already shows
   the zero-input baseline *nearly wins* — that's a warning. Fire needs to demand
   movement: fuel is elsewhere, the fire dims where it sits, the dark closes in on
   a static flame. **Unsolved. This is the first thing to test.**
2. **Is being worshipped in tone?** The good-guys framing (GDD §5) is intact — you
   are saving people — but a congregation that throws itself into the dark for you
   is genuinely uncomfortable. That discomfort is probably the best thing in the
   premise and the seed of every sacrifice and temptation event. Confirm the owner
   wants to sit in it before building on it.
3. **What "focusing element" buys mechanically.** Power passing through fire is a
   strong image and currently has no system attached. Candidate jobs: it's how
   abilities are cast, it's what upgrades apply to, it's what the retinue is made
   of. **Pick one, later.**
4. **Uniting flames — co-op mechanic or run objective or both?** ✅ **ANSWERED
   2026-07-27 (owner): run objective.** The game is single-player first, so the other
   bearers are people you *find in the dark* over the course of a run and unite with —
   not other players in a lobby. This is the better version of the idea: the objective is
   always present instead of contingent on having friends online, and "many bearers"
   stops being flavor.

   What still needs designing: what uniting a fire actually *does* (brighter, larger,
   longer-lasting, a second congregation folded into yours?), and what it costs. The
   co-op reading is preserved for later — overlapping light between two players doing
   something real is a mechanic no other game in the genre has — but it is a multiplier
   on a proven loop, not a v1 requirement.
5. **Does the dark have monsters, or is the dark itself the enemy?** Both is likely,
   but the ratio decides whether this is a horde game with a light gimmick or a
   survival game about a fire. Not decided.

---

## 5. What is deliberately not written yet

No faction names. No biome names. No NPC roster. No world flags. No decision-event
templates. No antagonist.

**The title is settled: the game is `Kindled`** (`GDD.md`, owner 2026-07-27; rename
propagated through code and docs 2026-07-31). `Emberkeep` came from the old canon and
did not survive.

**Unwritten is not the same as implied.** Nothing above licenses an assumption about
period, technology or arms — see the 2026-07-31 amendment at the head of this file.

Those get written once the prototype answers §4.1 and §4.4, because those two answers
determine what the world needs to *contain*. Writing them now would repeat exactly the
mistake this reset exists to undo.
