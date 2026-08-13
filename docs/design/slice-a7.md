# Slice A7 — you, seven soldiers, and one marked boss, inside a war that fights without you

**Built:** 2026-08-13 · **Status:** runnable prototype on `L_Spike1`
**Answers:** the first vertical slice of the castle pivot (`docs/design/castle-layout.md` D1,
§6, §9) · **Extends:** `docs/GATE1-FUN-PROTOTYPE.md`, which is otherwise unchanged and still
accurate about everything below the command layer.

> **The one thing to read if you read nothing else.** Every prototype expedient in this build
> is listed in §6, and every open decision this deliberately did **not** answer is listed in
> §7. Nothing here is a verdict. `docs/OPEN-DECISIONS.md` was not edited and must not be
> updated from this document — an open question is closed by an owner call, never by an
> implementation.

---

## 1. What it is

Three things sit on top of the Gate 1 prototype, and nothing that was already there was
rebuilt:

1. **The war.** The 128-body retinue stops being the player's army. It becomes the autonomous
   garrison: one shared command handle, an anchored hold on a line in front of the bearer,
   exempt from the leash, never swept by a broadcast order. It is fighting when you arrive and
   it keeps fighting if you walk away.
2. **The seven.** Seven named soldiers on the seven remaining command handles, one body each,
   with their own names, health, silhouettes and orders — drawn across `CLASSES.md`'s four
   archetypes per Q29 = A.
3. **One marked boss.** The `entity-tiers.md` §3 baseline stat block as a promoted Actor,
   reusing the hero grid-bridge exactly as that doc's §5 instructs, carrying three of
   `castle-layout.md` §6.1's marks — Quilled, Ram, Sated — each of which changes behaviour and
   is visible on the silhouette before it acts.

---

## 2. How to run it

PIE on `L_Spike1`, or standalone. The build is current; no rebuild needed.

```powershell
# standalone, boss on wave 1, all three marks, roster printed to the log every 6s
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" `
  "C:\Projects\ELVTRGAME\ELVTR\ELVTR.uproject" L_Spike1 -game -windowed -ResX=1600 -ResY=900 `
  -ExecCmds="Kindled.Boss.AutoWave 1,Kindled.Seven.LogInterval 6"
```

### Keys

Unchanged from Gate 1, with one meaning changed: **1/2/3/4 now order the seven and only the
seven.** The war does not hear them.

| Key | Action |
|---|---|
| WASD | move the bearer |
| 1 / 2 / 3 / 4 | Follow / Charge (at cursor) / Hold / Rally — **to all seven** |
| Num1 / Num2 / Num3 | camera: close / army / strategic |
| R | restart the run |

### Console

| Command | What it does |
|---|---|
| `Swarm.UnitStance <0-6> Follow\|Charge\|Hold\|Rally` | order **one** named soldier. 0-6 are the seven; 7 is the garrison and warns |
| `Kindled.Seven.Report` | log each of the seven — archetype, look, rung, health, order, kills |
| `Kindled.Boss.Spawn [quilled,ram,sated\|none]` | spawn the boss now, carrying those marks |
| `Kindled.Boss.Marks quilled,sated \| none` | re-mark the standing boss live — the A/B for whether a mark reads |
| `Kindled.Boss.Report` | HP, marks, and how many soldiers actually landed a blow last frame |
| `Kindled.Boss.Clear` | remove the boss |
| `Kindled.Adapt <0-6> <ladder> <rung>` | move one of the seven up the adaptation tree (already existed) |

### Dials worth knowing

| CVar | Default | What it decides |
|---|---|---|
| `Kindled.War.Standoff` | 700 | how far in front of the bearer the garrison holds. **Keep it below `Swarm.BroodSpawnRadiusMin` (1200)** or the wave spawns behind your own front |
| `Kindled.Boss.AutoWave` | 3 | 1-based wave the boss arrives on; 0 = never |
| `Kindled.Boss.AutoMarks` | `quilled,ram,sated` | what it arrives carrying |
| `Kindled.Seven.LogInterval` | 0 | seconds between automatic roster + boss log lines; 0 = off |
| `Kindled.Boss.SurroundCap` | 45 | `entity-tiers.md` §4's 35–55 estimate at its midpoint — a safety clamp, not the mechanism |
| `Kindled.Boss.Draw` | 1 | 0 hides the boss's debug body; it still fights |

---

## 3. How it was built — and what was reused rather than written

The single most useful decision in this slice was not writing things.

### The command layer already existed

`USwarmSubsystem` already carried eight command handles with per-unit stance, per-unit
adaptation rung, per-unit kill credit, per-unit standing count and per-unit centroid. The
seven are seven of those handles holding one body each; the war is the eighth holding a
hundred and twenty-eight. Nothing about a handle cared how many bodies were in it, so nothing
had to change to make that true.

Three small things did change, and they are the whole of "the war becomes the war":

- `SetStance` (the broadcast order behind keys 1-4) now writes handles 0-6 and stops. Left
  alone, the player's order would sweep the garrison along and the front would follow the
  bearer around the map.
- The garrison is **exempt from the leash** (`SwarmProcessors.cpp`). The leash means "you must
  stay in the fight with your troops", which is a statement about the bearer's own squad. Left
  leashed, the entire line would latch broken the moment the player walked 2000uu away, drop
  to Follow, and come running after the flame. Q7 = A supplies the fiction for free: the castle
  has fixed light of its own, so the garrison does not need yours.
- The garrison holds an **anchored** Hold at a fixed world point, captured once at run start,
  on the bearing the tide arrives from. That one line is the difference between "your army,
  orbiting you" and `castle-layout.md` §9 step 4's "a fight already in progress".

### The boss reuses the hero bridge, plus one line that carries most of the weight

`entity-tiers.md` §5 says to reuse `HeroMeleeRangeSq` / `FindOwnGridEntry` /
`SwarmCombatProcessors.cpp` rather than invent a second Actor-vs-Mass path. The boss does
that — its authoritative state lives on `USwarmSubsystem::FBossState` exactly as the hero's
does — **and is also published into the spatial grid each frame as an ordinary enemy entry.**

That one `AddToGrid` call gives it, with no further code:

- the retinue find it as their nearest enemy and close on it;
- their swing clocks advance against it, so they actually swing;
- they take its blow **victim-side**, out of the same conserved `BlowsClaimed` budget that caps
  every other striker in the sim, so one boss swing cannot pay out to fifty people;
- they get knockback and hit flash from it;
- they turn to face it;
- brood correctly ignore it (same-team test), and brood steering never targets it.

The hero's own bridge could never have any of that, because the hero has no enemies in the grid
to be found *by*. Only damage **in** to the boss had to be hand-written, and that is a
near-verbatim copy of the existing brood-vs-hero branch: on the frame a soldier's blow lands,
it spends one claim from its own budget and the boss takes
`max(BlowDamage - Armor, ArmorChipFloor)` — `entity-tiers.md` §2.2's formula, applied
victim-side, unchanged.

### The seven's looks cost nothing

A named soldier's silhouette is assigned through `SetSquadRung` — the adaptation handle that
already existed. One assignment moves the sprite, the formation detachment, the melee reach and
the cleave together (`SwarmRenderPack::VariantFor`), so a named soldier cannot end up looking
like something it does not fight like. No new art, no new atlas rows, no new field on any
fragment.

---

## 4. The three marks

Each one had to (a) change behaviour rather than a number and (b) read on the silhouette
*before* it acts. Both halves are implemented; the silhouette half is debug-drawn (§6).

| Mark | Behaviour | Silhouette | Counter it creates |
|---|---|---|---|
| **Quilled** | +40 Armor against **archers only**, and it walks at the nearest archer unit instead of the nearest line | eight pale shafts radiating from the body | bring melee. At the shipped archer blow (27) this floors a bow to the chip floor while barely touching a spear |
| **Ram** | ignores the line entirely and beelines the objective; blow against a soldier cut to 0.35x, against the objective 1.5x | a wide slab held out in front, on its heading | intercept it away from what it is aimed at |
| **Sated** | regenerates 120 HP/s after 3s without taking a blow | body swells 1.28x, bright core lit inside it | burst that outruns regeneration — chip-and-disengage never finishes it |

Marks **compound**, which is §6.1's actual claim: `Kindled.Boss.Spawn quilled,ram` arrives as a
gate-breaker you cannot shoot, and looks like both before it does anything. The default
auto-spawn carries all three.

A windup tell is on the body itself: it brightens through the 2s windup and snaps back on the
recovery, so a boss swing is a beat you can see coming rather than a slower version of a
soldier's jab (Design Law 6).

**Nothing accretes a mark.** `castle-layout.md` §6.2's claim — that a boss is a report on a war
you were not in — needs the offscreen war simulation, which is out of this slice's scope. Marks
are set by console. That is a prototype surface, not a position on how a boss earns one.

The other three marks in §6.1 — **Wearing**, **Unblinded**, **Column-fed** — are deliberately
absent rather than stubbed. Each needs a war-sim event that does not exist (killing a named
squad, surviving inside a bearer's light, catching a routing column after a fall).

---

## 5. `HeroDamage` is retired (Q13 = C)

`Swarm.HeroDPS` is **0**, in the C++ default *and* in `ELVTR/Saved/SwarmExecOnPlay.txt` — which
overrides C++ defaults at BeginPlay, so changing one without the other would have proved
nothing. It was 55.

**The bridge is untouched and still runs every frame.** The bearer is still a body in the grid,
still gets mobbed, still takes blows, still dies and still loses the run. `HeroMeleeRangeSq`,
`FindOwnGridEntry` and the pawn's swing clock all remain, and `entity-tiers.md` §1 still points
at that path as the precedent every promoted Actor reuses — the boss is the first thing to
actually reuse it. Only the number retired.

> **Sync warning, and a stale premise in the task brief.** The brief named
> `docs/design/SwarmExecOnPlay.txt` as the version-controlled copy of the exec file. **That file
> does not exist.** The real tracked copy is **`ELVTR/Config/SwarmExecOnPlay.canonical.txt`**,
> which is outside this task's owned paths and was therefore **not** edited. It still says
> `Swarm.HeroDPS 55`. Anyone restoring the working copy from canonical will silently un-retire
> the bearer's damage. The one-line fix is in §8.

---

## 6. What was faked, and every prototype expedient

Listed so none of it is mistaken for a decision.

| # | Expedient | Why | What it costs / upgrade path |
|---|---|---|---|
| 1 | **The seven are Mass entities on existing command handles, not promoted Actors.** | **Q25 is open and this does not answer it.** This is the cheap path and the *only* reason it was taken: every per-unit facility the seven need — stance, rung, kill credit, standing, centroid, health — already exists and already works on a handle holding one body. | If Q25 lands on promoted Actors, the roster table and the spawn path go; the boss already demonstrates the Actor bridge those seven would use. |
| 2 | **The seven's roster is a fixed authored table** (`Spike/SevenRoster.h`). | A slice needs seven concrete bodies to point at, and "fixed roles" is the cheapest of Q2's three options. **Not because fixed roles won the argument.** | If Q2 lands on survivors or on loadout, this table is what gets deleted. Nothing else assumes its shape. |
| 3 | **The seven have no abilities.** | **Deliberate, and the most important omission here.** Q23 is open, is the largest piece of undesigned content in the project, and the register says it must not be settled by whatever gets prototyped first. Giving them verbs would have answered it by accident. | Today an archetype is a label, a body and a reach. task-144 owns the kit. |
| 4 | **`HPScale` 4x on each of the seven** (760 HP vs a veteran's 190). | There is no stat block for a named soldier anywhere in canon, because Q2/Q14 are open. At a plain veteran rung they die inside a 700-brood wave before you can read their names, which makes the question this slice exists to ask unanswerable. | A legibility number. Delete it the moment the seven have a real stat block. |
| 5 | **The bearer stands in for the gate that Ram breaks.** | §6.1's Ram breaks a *gate*; the brief rules gates out of scope. The **behaviour** is the specced one — ignore bodies, go for the thing that matters — but what it is aimed at is a stand-in. | When gates exist, change `ResolveTarget`'s Ram branch and nothing else. |
| 6 | **The boss is debug-drawn, not a sprite.** | The enemy atlas has no boss row and authoring one is a PixelLab task with its own request file, palette pass and import. A box plus three attachments carries the read today. | An enemy-atlas variant plus per-mark overlay frames. **No sim change needed** — the boss is already an Actor that draws itself. |
| 7 | **The garrison handle's recorded *type* is its first recruit's roll**, while its bodies are mixed Spearmen and Archers. | A handle carries one type; the war should be mixed. The mix is what matters and is real; the handle's label is cosmetic. | Cosmetic only — it affects a HUD label and a log string, not combat, formation or targeting. |
| 8 | **One arena, one line, one tide.** | The brief rules out zones, fronts, a ledger, gates and streaming. | No front ledger (§5.1) exists, so nothing yet says a front is *Breaking* and nothing calls the player anywhere. |
| 9 | **A named soldier lost mid-run is not replaced.** | **Not a ruling on Q15.** Nothing revives them because no revive mechanic exists — that is the absence of an answer, not one. | Reinforcements refill the garrison only. R restarts the run. |
| 10 | **`Kindled.Boss.SurroundCap` never binds in practice.** | The real cap is geometric (only so many bodies fit inside 250uu) and blows are spread over a 0.9s cadence, so only a fraction strike on any frame. The clamp is a safety valve against a pathological pile-up. | `Kindled.Boss.Report` prints the live attacker count against the cap. If it sits pinned at 45, `entity-tiers.md` §4's estimate is wrong and wants the real measurement §5 asks for — not a bigger clamp. |

### One measured surprise worth recording

The boss one-shot the bearer on its first run and never moved from the world origin.
`ASpikeBossActor` had **no root component**, so `SpawnActor`'s location was discarded,
`SetActorLocation` silently returned false, and `GetActorLocation` reported the origin. It cost
a run to find, because everything downstream behaved perfectly on the wrong position — it hit
the two nearest bodies for exactly its Ram body blow and killed the bearer with exactly one Ram
objective blow, both arithmetically correct. A bare `USceneComponent` fixed it.

That run also surfaced a real tension, which is flagged rather than quietly tuned: the boss's
base blow is 220 against a bearer whose 500 HP was set when he had 55 DPS and was expected to
tank brood. `Kindled.Boss.RamObjectiveScale` went 2.5 → 1.5 so Ram is two blows and not one,
which makes §6.3's "intercept it" counter actually playable. **The bearer's own HP against
post-pivot threats is not retuned here** — that is a tuning call, not this task's.

---

## 7. Open decisions this deliberately did not close

`docs/OPEN-DECISIONS.md` was **not edited**, and must not be updated from this file.

| Q | Question | Status after this slice |
|---|---|---|
| **Q23** | The squad-channelled ability kit | **Untouched, on purpose.** The seven have no verbs. This is the one that would have been closed by accident. |
| **Q25** | Are the seven Mass entities or promoted Actors? | **Open.** This slice picks Mass entities as a prototype expedient (§6.1) and says so in the code. The boss demonstrates the *other* path working in the same build, so neither option is now cheaper by default. |
| **Q26** | How are orders issued? | **Open.** Addressing one soldier stays on the shipped `Swarm.UnitStance` console surface. No input scheme was invented. |
| **Q2** | Composition of the seven | **Open.** The roster is a fixed table because that was cheapest to build (§6.2). |
| **Q14** | Is seven a cap, a floor, or a field size over a bench? | **Open.** Seven is hard-coded as the field size and nothing implies which. |
| **Q15** | Can your seven be downed and revived? | **Open.** They die and stay dead for the run because nothing revives anything (§6.9). |
| **Q4, Q5, Q24** | Enemy planner, the `Breaking`→`Fallen` window, ledger authority | **Untouched.** All three need the war ledger, which is out of scope. |

---

## 8. Known gaps and handoffs

- **`ELVTR/Config/SwarmExecOnPlay.canonical.txt` is out of sync by one line.** It still says
  `Swarm.HeroDPS 55`; the working copy says `0`. That file is outside this task's owned paths
  and was not edited. Apply:
  `Swarm.HeroDPS 0   ; RETIRED 2026-08-13 — castle-layout.md §6.4 / Q13 = C`
- **The HUD is on-screen debug text, not UMG.** `UI/KindledHud` and the `SquadCard` /
  `MusterPanel` widgets still describe an army, not seven individuals — the change §6.4 asks
  for ("the HUD's job changes") is done in the prototype HUD only. Whoever owns the UMG pass
  now has a real data source: `GetSquadHP` / `GetSquadMaxHP` per handle.
- **No capture path in this project can film the HUD or the boss.** `Swarm.DebugShotAfter` goes
  through a `SceneCaptureComponent2D`, which renders world primitives — not Slate text and not
  debug draws. `Kindled.Seven.Report` and `Kindled.Boss.Report` exist because of this; on a
  headless run the log is the only readout.
- **Archers rarely strike the boss while brood are alive.** An archer's `TargetsPerHit` is 1, so
  its reach is the distance to its single nearest enemy — usually a brood, not the boss. That is
  the shipped targeting model behaving correctly, not a bug, but it does mean Quilled's counter
  reads best once the tide thins or on a `Kindled.Boss.Spawn` with no wave up.
- **The war has no ledger, so nothing calls you anywhere.** The loop in `castle-layout.md` §9
  steps 1-3 (read the fronts, one goes `Breaking`, cross gates to reach it) does not exist. This
  slice is steps 4-6.
