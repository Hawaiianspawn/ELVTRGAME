# Squad Actors — costing the seven as promoted Actors vs. Mass entities

**What this is:** a costing document for `docs/OPEN-DECISIONS.md` Q25 (Mass entities or
promoted Actors?) and `docs/PREFLIGHT.md` §3 Q25 — what each side of the choice already gets
for free, what promotion would buy, and what it would cost, all traced to the running code.
**No verdict.** Q25 is the owner's; this is the input to that call.

**Extends:** `docs/design/entity-tiers.md` §5 (the promoted-Actor pattern this doc costs
against) and `docs/design/slice-a7.md` §3, §6.1, §10 (what the seven are today and what the
ability kit bolted onto them).

---

## 1. What each side IS today, from the running code

### 1a. The seven — seven command handles, one body each

`USwarmSubsystem::MaxSquads` is 8 (`SwarmSubsystem.h:46`): a shared command-handle budget the
war and the seven both draw from. `NamedSoldiers` is 7 (`SwarmSubsystem.h:59`) — handles 0–6.
Each of the seven is one ordinary Mass entity sitting alone in a handle that could hold a
hundred. Nothing about a handle cared how many bodies were in it (`slice-a7.md` §3), so every
per-handle facility below was already built for the war and the seven inherited it unchanged:

| Facility | Where it lives | What it gives a named soldier |
|---|---|---|
| Per-unit stance | `SetStance` / `GetUnitStance` / `UnitStanceAnchor` (`SwarmSubsystem.h:243–263`) | Follow/Charge/Hold/Rally addressed to one handle without sweeping the others |
| Live standing count | `SquadStanding[MaxSquads]`, `GetSquadStanding` (`SwarmSubsystem.h:1034`, `:733`) | alive/dead for that one body |
| Centroid | `SquadCentroidSum`, `GetSquadCentroid` (`:1035`, `:745`) | a point in space to aim a camera, an ability zone, or a boss's `ResolveTarget` at |
| Pooled HP / MaxHP | `SquadHP`/`SquadMaxHP`, `GetSquadHP`/`GetSquadMaxHP` (`:1036–1037`, `:753–754`) | for one body in the handle, this *is* that soldier's own health bar |
| Sticky type | `SquadType`, `GetSquadType`, set once by `AssignRecruit` (`:776–778`, `:913`) | Spearman/Archer, which the roster maps to melee/ranged archetypes |
| Adaptation rung | `SquadVariant`/`SquadTier`, `SetSquadRung` (`:1056–1058`, `:808`) | which of `upgrades.json`'s tier rows the soldier fights at, **live** — `SwarmCommands.cpp:941–943` calls `SetSquadRung` then `ReStatUnit` to re-write the standing body's HP fragment in place, no respawn |
| Kill credit | `WaveKilledBySquad`/`RunKilledBySquad` (`:1079–1080`, `:191/:195`) | per-soldier kill count, attributed by the same combat-claim pass every Mass entity's kills go through |
| Silhouette | `SetSquadRung` → `SwarmRenderPack::VariantFor` (`SevenRoster.h` header comment) | sprite, formation detachment, melee reach and cleave move together — one call |
| Per-soldier ability state | `FAbilityState::SoldierReadyAt[NamedSoldiers]`, `RaiseUnit`, `FocusUnits` bitmask (`SwarmSubsystem.h:337–411`, task-144) | per-soldier cooldowns and channel targets, **already built without any Actor** |
| Roster identity | `SevenRoster::Get(i)` (`SevenRoster.h`) → consumed by `Spike1GameMode.cpp:384–389` (spawn), `SquadAbilities.cpp` (verb binding), `KindledHud.cpp:128` (UMG cards) | name, archetype, verb, look — read off a static table, not off any per-Actor state |

**The one thing none of this buys**, because none of it needed to: the seven's combat *output*
(closing distance, striking fodder, taking a blow) runs through the exact same shared Mass
processors as every Brood and every garrison soldier — `SwarmProcessors.cpp` (steering,
separation, formation slotting, stance interpretation) and `SwarmCombatProcessors.cpp` (the
claim/strike/`BlowsClaimed` system). **A named soldier fights because it is a Mass entity, not
because anything above knows it is named.** The roster table is a skin and a bookkeeping layer
on top of ordinary swarm bodies.

### 1b. The boss — the promoted-Actor bridge, as built

`ASpikeBossActor` (`SpikeBossActor.h/.cpp`) is the one thing in the codebase actually built on
`entity-tiers.md` §5's instruction to reuse the hero grid-bridge. Its authoritative state is
`USwarmSubsystem::FBossState` (`SwarmSubsystem.h:286–297`) — `Location`, `HP`, `MaxHP`, `Marks`,
`bAlive`, `bStriking`, `BlowDamage`, `ReachSq`, `TargetsPerHit` — the same shape as the hero's
own bridge fields it mirrors (header comment, `SwarmSubsystem.h:269–284`).

What actually carries the weight, and is the one line the seven's Mass-entity approach has no
equivalent of: **the boss publishes itself into the spatial grid every frame as an ordinary
enemy `FGridEntry`** (`SpikeBossActor.cpp` "publish" block, `:299–309`). That one call gives it,
for free, everything `SwarmCombatProcessors.cpp` already does for any grid entry: the retinue
find it as nearest-enemy and close on it, their swing clocks advance against it, they claim
their blow against it out of the same conserved `BlowsClaimed` budget every other striker uses,
it gets knockback and flash, brood correctly ignore it (same-team test). **Not one line of that
was written by hand.**

What *is* hand-written, and is the whole cost of the bridge:

- **Damage in** — `SpikeBossActor::Tick` consumes `ConsumePendingBossDamage()` (`:206–218`),
  fed by a hand-written retinue-claim branch in `SwarmCombatProcessors.cpp` that is a
  near-verbatim copy of the existing brood-vs-hero branch (header comment, `:17–20`).
- **Damage out to the bearer** — the bearer is a Pawn, not a Mass entity, so nothing in the
  grid can hit him; the boss hands damage over directly via `AddPendingHeroDamage`
  (`SpikeBossActor.cpp:311–329`), the mirror of the hero's own bridge.
- **Its own AI** — `ResolveTarget` (`:156–188`) is the entirety of its behaviour: one hand-rolled
  decision (Ram → the objective; Quilled → nearest archer centroid; else → nearest line
  centroid), a hand-rolled walk-until-contact rule (`:248–284`), and its own swing clock
  (`:286–297`). It does not follow Follow/Charge/Hold/Rally. It has no formation slot, no
  steering, no separation force. **It is a single autonomous target, not a squad member.**
- **Its own draw** — `Draw()` (`:338–425`), debug-box body plus three mark decorations, since
  there is no sprite for it.

`ASpikeBossActor` demonstrates, concretely: *an Actor can be struck by, and can strike, the Mass
grid.* It does **not** demonstrate: *an Actor can hold a stance, steer with a formation, or
generate its own combat output the way a line soldier does.* That distinction is the hinge the
rest of this document turns on.

---

## 2. What promotion buys

| Capability | What an Actor gives that a handle cannot | Needed by | Already substantially covered without promotion? |
|---|---|---|---|
| Per-body animation state | A real state machine / anim graph per body, not a static sprite-variant swap | Nothing shipped needs this yet; the four archetypes read fine off `SwarmRenderPack::VariantFor` | **No** — genuinely new capability |
| Hit reactions beyond flash | A body that staggers, plays a directional hit anim, or interrupts its own action on a big blow | Nothing open decision currently asks for this | **No** — the seven get the shared knockback+flash every Mass entity gets, nothing more |
| Downed/revive states (Q15) | An actual third state (standing / downed / dead) with a pose and a revive interaction, instead of `SquadStanding` being a boolean | **Q15**, open — right now a named soldier at 0 HP is gone; nothing distinguishes "downed" from "dead" because nothing revives anything (`slice-a7.md` §6 row 9, §10.8 row 11) | **No** — this is a real gap either way; Mass has no partial-alive fragment state today and would need one added regardless of Q25's answer |
| Per-soldier ability channels with interrupts | A place to hang an interrupt condition (an Actor's own tick can check "am I still casting, did I take a blow") | Q23 = B's Kindle channel — flagged in `slice-a7.md` §10.8 row 14 as not committing the caster because "no channel-interrupt mechanic exists" | **Partially** — the per-soldier cooldowns and targets (`FAbilityState`) already exist on the subsystem without any Actor. What promotion would add is specifically *interruptibility*, not the channel state itself |
| UMG binding | Actor-native binding (a widget bound to a specific Actor instance) | Nothing — `SquadCard`/`MusterPanel` already read named-soldier state (`KindledHud.cpp:128`, `Spike1GameMode.cpp:244`) off subsystem accessors, no Actor required | **Yes, already done** — this line item is close to free either way and should not weigh on Q25 |

**Read plainly:** three of the five capabilities the register names as promotion's payoff are
either already achieved by the handle approach (UMG, most of the ability kit's per-soldier
state) or are gaps that exist regardless of Q25 and would need new work under *either* answer
(downed/revive). The two capabilities that are *cleanly* Actor-only are animation depth and
hit-reaction depth — and nothing currently open asks for either.

---

## 3. What promotion costs

The boss precedent is cheap because it only had to solve *being fought*. Promoting the seven
means solving *fighting*, which the boss's build never had to touch. Below is the migration
seam, file by file, framed against what currently runs for free.

| Seam | What runs today (free, shared) | What promotion requires |
|---|---|---|
| **Roster spawn path** | `Spike1GameMode.cpp:384–389` — `SetSquadRung` then `SwarmSpawn::SpawnNamed` per roster row, into a Mass handle | Replace with `SpawnActor<AKindledSoldierActor>` per row, seeded with a `FBossState`-shaped per-soldier struct (own `HP`/`MaxHP`/`Location`) |
| **Stance orders** | `SetStance` writes `UnitStance[0..6]` (`SwarmSubsystem.h:243–251`); `SwarmProcessors.cpp`'s shared stance-interpretation processor reads it for every Mass entity, named or not | **Rebuilt per Actor.** The boss precedent has *no* stance interpretation to reuse — `ResolveTarget` is a single hard-coded behaviour, not four. Each promoted soldier's `Tick` would need to read its own `UnitStance` and implement Follow/Charge/Hold/Rally itself |
| **Steering, separation, formation slotting** | `SwarmProcessors.cpp` — every Mass entity, named or not, gets pathing-toward-target, neighbour separation, and a formation slot for free, because it's iterated by the same batch processor as the other ~120 bodies in frame | **Rebuilt per Actor**, or the promoted seven opt out of formation entirely and free-walk (a visible behaviour change, not just an implementation detail) |
| **Combat output (striking)** | `SwarmCombatProcessors.cpp`'s claim system: any Mass entity in reach of an enemy grid entry can claim and land a blow, same code for Brood, garrison, and the seven alike | **New code.** The boss precedent only proves an Actor can be a *victim* in the grid and hand-write one outgoing attack against one target (the bearer). A promoted soldier needs to find and strike *whichever* enemies are in its own reach, every frame — effectively a per-Actor instance of what `SwarmCombatProcessors.cpp` already does in batch for free |
| **Adaptation rungs** | `SetSquadRung` + `ReStatUnit` (`SwarmCommands.cpp:941–943`) rewrites a *standing* Mass entity's HP fragment live, no respawn | Live re-stat becomes direct field writes on the Actor's own struct — mechanically simple, but one more thing that has to be hand-duplicated rather than inherited |
| **Kill credit** | `WaveKilledBySquad`/`RunKilledBySquad`, incremented by the same combat-claim pass every entity's kills flow through (`SwarmSubsystem.h:182–183`) | Needs its own write path once the promoted soldier's kills stop flowing through that shared pass |
| **HP surface** | `SquadHP`/`SquadMaxHP`, pooled per handle, read by the HUD (`Spike1GameMode.cpp`) and by every ability-zone check (`ScreenScaleAt`, `IsRalliedAt`) | Those read sites move from `GetSquadHP(Index)` to reading the Actor directly — a mechanical rename, not a new problem, **provided** the Actor is still discoverable by index the same way |
| **Ability kit's per-soldier verbs** | `FAbilityState` already lives on the subsystem, addressed by `UnitIndex`, not by Actor (`SwarmSubsystem.h:337–411`) | **No change needed** — this system was built to be addressable by index and does not care whether index 3 is a Mass handle or an Actor pointer |

**Frame cost.** Seven Actors is trivially cheap in isolation — `entity-tiers.md` §5 already
calls "eight Actors is nothing" and the boss is the working proof. The measured Mass-sim cost
(`docs/perf/one-camera-bench.md` §1) is ~0.75ms per 1,000 *Mass* entities, game-thread-bound,
because Mass batches fragments through cache-friendly SoA processors — which is the entire
reason Design Law 5 keeps Fodder/Soldier off the Actor path at all (`entity-tiers.md` §1). An
`AActor::Tick` goes through the general-purpose actor tick manager instead — heavier per body,
virtual-dispatch, not batched — which is *why* the line is drawn where it is architecturally,
not a claim this document re-derives. At **seven** bodies that difference does not register
against a frame that is already spending single-digit milliseconds on ~100–250 Mass bodies. It
would start to register if Q14 ever answers "seven grows" past a few dozen promoted bodies —
see §5.

---

## 4. The hybrid worth naming

The seam above supports one hybrid cleanly, because it is exactly what the boss already does in
reverse: **keep the handles authoritative for combat, animation-state, and the shared Mass
systems (steering, formation, striking), and give each of the seven a thin Actor shell that
exists only for what handles cannot express** — specifically UMG binding (already free either
way, §2) and, if Q15 lands on a real downed state, a place to drive that state's presentation.

This is not a new architecture; it is the boss's own bridge run the other direction. The boss
publishes an Actor's state *into* the grid so the grid's systems can act on it. A hybrid seven
would keep the grid's systems acting on a handle as today, and layer an Actor **on top** purely
for what needs Actor-native facilities (a `UWidgetComponent`, a state machine for downed/revive
presentation) — reading its position and HP off `GetSquadCentroid`/`GetSquadHP` every frame
rather than owning either. **This only holds if the seven stay fighters that follow orders
through the shared Mass systems** — the moment a soldier needs its *own* combat AI (the boss's
actual reason for existing), the hybrid collapses back into full promotion, because that is
precisely the half of the boss's cost that has no cheap version.

---

## 5. What to watch for

The observable that would say the Mass-handle path has hit its ceiling is not frame time —
seven or even several dozen promoted Actors cost nothing measurable (§3). It is **feature
pressure on facilities Mass genuinely cannot express**, and the two concrete tripwires already
visible in the shipped slice are:

1. **Q15 lands on a real downed state.** `SquadStanding` is boolean today; there is no partial
   state to build a "downed, draggable, revivable" presentation on top of without either (a)
   adding a genuinely new per-entity Mass fragment for it, or (b) an Actor to hold that state
   machine. This is the single most likely trigger for revisiting Q25, because it is the one
   capability gap in §2 that is not already substantially covered.
2. **The ability kit's Kindle channel gets a real interrupt** (`slice-a7.md` §10.8 row 14). The
   moment "does not commit the caster" needs to become "does," something has to observe the
   caster taking a blow mid-channel and react — cheap on an Actor's own tick, awkward as a new
   field threaded through the shared Mass combat pass for exactly one soldier's benefit.

Neither of these is a frame-cost ceiling. Both are "the handle has no honest place to put this
state" ceilings, and either one recurring a second and third time across future content would be
the signal that promotion has stopped being optional.

---

## 6. What this document is not

- **Not a verdict.** Q25 is the owner's call, made against this costing.
- **Not a recommendation weighted toward either answer.** §2 and §3 are written to be read
  independently of each other; the "hybrid" in §4 is named because the seam supports it, not
  because it is being proposed as a compromise position.
- **Not a measurement.** §3's frame-cost paragraph reasons from `docs/perf/one-camera-bench.md`
  (measured) and from `entity-tiers.md`'s stated architectural rationale (not independently
  re-measured this session — no engine access in this pass). If Q14 ever pushes the promoted
  count into the dozens, the next step is a real `-ActorBench`-shaped measurement, the same
  shape as `-SwarmBench`, spawning N promoted Actors and reading actual `AActor::Tick` cost
  rather than reasoning from the architecture alone.
