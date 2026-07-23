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
| Many bearers, united | **1–4 player co-op** is the fiction, not a mode | Spike 2 target |
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
4. **Uniting flames — co-op mechanic or run objective or both?** Overlapping light
   from two players should probably *do something* (brighter, larger, safer). If it
   does, co-op gains a mechanic no other game in the genre has. If it doesn't,
   "many bearers" is just flavor for the lobby.
5. **Does the dark have monsters, or is the dark itself the enemy?** Both is likely,
   but the ratio decides whether this is a horde game with a light gimmick or a
   survival game about a fire. Not decided.

---

## 5. What is deliberately not written yet

No faction names. No biome names. No NPC roster. No world flags. No decision-event
templates. No antagonist. **No game title** — `Emberkeep` came from the old canon and
is not assumed to survive.

Those get written once the prototype answers §4.1 and §4.4, because those two answers
determine what the world needs to *contain*. Writing them now would repeat exactly the
mistake this reset exists to undo.
