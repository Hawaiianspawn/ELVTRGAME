# RTS Vertical Slice — Definition & Bill of Materials

**Version:** 0.1 · Companion docs: `GDD.md` (§10 milestone 4), `SYSTEMS.md`, `CLASSES.md`, `WORLD.md`
**Created:** 2026-07-19

---

## 1. What the slice proves

> A 30–45 minute co-op run of the real game feels good from floor 1 to boss kill.

Scope line from GDD §10: **1 class, 1 biome, 3 floors, 1 boss, 2 decision events, co-op.**
Everything below serves that sentence; anything that doesn't is cut or faked.

---

## 2. Stance model for the slice — leash rule **(DECIDED 2026-07-19)**

The retinue's home is the hero. Stances redirect units *near* the hero; they never
detach the army from the player.

- **Leash radius:** every retinue unit has a max distance from its hero. Inside it,
  stances behave as specced (GDD §4). When the hero moves far enough that a unit
  (including one on **Hold**) exceeds the leash, that unit **breaks stance and
  returns to Follow**, pathing back to formation.
- **Hold is a promise with a range:** you can anchor a chokepoint, but you have to
  stay in the fight with your troops. Defending a gate while the hero wanders off
  is deliberately impossible — hero relevance (GDD §4 design tensions) enforced by
  rule, not tuning.
- **Class reflavors may explicitly override the leash** as a designed feature —
  e.g., the Pathfinder's *Loose the Pack* hunts marked targets off-screen
  (GDD §4). Override is the exception and must be stated in the class spec;
  default for all units and stances is leashed.
- Side benefits (why this is also the right engineering call): units clustered on
  heroes simplifies replication relevancy, off-screen sim LOD, and camera framing
  on Steam Deck.

**Tunables (prototype dials, working values):**

| Param | Working value | Notes |
|---|---|---|
| `LeashRadius` | ~1.25 screens (~2000 uu) | Generous enough that Hold-the-choke works while hero fights nearby |
| `LeashHysteresis` | ~15% band | Re-anchor only after unit is well inside radius; prevents flicker at the edge |
| Leash warning | at 80% of radius | Held units flash / audio tick before they break — breaking must never feel random |
| Break behavior | drop to Follow, path to formation | Units re-engage per Follow rules on the way back |

---

## 3. Gates (sequenced — each can invalidate work done after it)

1. **Fun prototype verdict** — stances (with leash) feel good at 50–200 units;
   hero feels like a commander, not a camera. See stance/hero/replay tests.
   **Built 2026-07-22 — playable, awaiting verdict:** `docs/GATE1-FUN-PROTOTYPE.md`
   (all 4 stances + leash, 3-wave run, win/lose; zero-input baseline loses wave 3
   by 4–13 brood across 3 runs).
2. **Spike 1 measured** — fill `docs/SPIKE1-RESULTS.md`, including a
   **Steam-Deck-budget row** (game thread ≤ ~10 ms or power-limited profile).
   Output: the entity ceiling that sizes every encounter below.
3. **Art test (GDD Open Question #5)** — flipbooks vs. flat-shaded 3D. Gates all
   sprite production.
4. **Spike 2 — networking**: 2 players, heroes fully replicated, swarms as
   aggregate state. Co-op cannot be bolted on later.

Spike 3 (procgen) is partially deferrable — see §5 Tech.

---

## 4. Design prerequisites (SYSTEMS.md v0 — currently empty sections)

- [ ] Entity tier stat blocks for slice roster (fodder / soldier / elite / boss)
- [ ] One scaling curve across 3 floors (first act of the exponential fantasy)
- [ ] Encounter budget table per floor (density, wave composition, spike/breather
      rhythm — hand-tuned, no director AI)
- [ ] Vanguard retinue tuning: growth rate, attrition, per-floor cap
- [ ] Loot v0: unit orbs + healing + ~4–6 stacking items (not the real loot system)

---

## 5. Bill of materials

### Content

| Thing | Slice count |
|---|---|
| Class | 1 — **Vanguard** (full kit: hero abilities, stance reflavors, 2–3 retinue unit types) |
| Biome | 1 — **the Highgates** (plazas/courtyards = arena rooms inside the city fiction) |
| Floors | 3, escalating density |
| Enemy roster | ~5: melee fodder, melee soldier, ranged, 1 anti-swarm elite, 1 boss |
| Boss | 1 — army-vs-big-thing; positioning + stances, not a DPS check |
| Decision events | 2 from `WORLD.md` §8 — one fork, one sacrifice; ≥1 per-player |
| Room prefabs | ~12–15 (≥3 arenas, decision sites, corridors, boss room) |

### Tech

- [ ] Mass sim + **aggregation** (squad-as-entity, renders as N sprites) — built in
      from the start; this is the Steam Deck perceived-scale strategy
- [ ] Leash system per §2 (radius, hysteresis, warning, break-to-Follow)
- [ ] Replication 2–4 players: heroes/elites/boss full, swarms aggregate
- [ ] Stance UI + minimal HUD (retinue count, HP, stance indicator) —
      **controller-first** (Deck target)
- [ ] Run structure: start → 3 floors → boss → victory/death screen
- [ ] Procgen: small room-graph generator over the prefab library
      (≈ Spike 3-lite). Fallback if it drags: 3 hand-assembled floors with
      randomized encounters. **Non-negotiable either way:** arena-sizing
      constraint (GDD §9).

### Art & audio

- [ ] Highgates tileset (post art-test)
- [ ] Vanguard hero + retinue sprites; 5 enemy sprites — walk/attack/death each
- [ ] Faction palette separation **proven at horde scale** (Pillar 4 gets tested
      here, not asserted)
- [ ] Audio minimal: hit/death, stance confirmations (readability tools, not
      polish), 1 music loop

---

## 6. Explicitly faked or absent

Persistent world flags (stub one flag write at run-end; render nothing from it) ·
the other 3 classes · meta menus · difficulty options · host-world rules ·
hybridization events · the real loot/evolution system · pacing director AI.

---

## 7. Schedule shape & risks

Fun prototype → Spike 1 measurement (overlap fine, same map) → art test →
Spike 2 → slice production, with §4 design numbers running ahead of content.

**Top risks:**
1. Sprite production starting before the art-test decision (long pole, gated late).
2. Netcode discovered late (hence Spike 2 before production).
3. Leash tuning: if `LeashRadius` is too small, Hold feels useless; too big and
   the hero-relevance rule evaporates. It's a first-class fun-prototype dial.
