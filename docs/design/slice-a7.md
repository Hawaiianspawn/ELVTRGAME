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

Input is **Enhanced Input** as of task-144 — one C++-built mapping context, no polled keys
(§10.5). 1/2/3/4 still order the seven and only the seven; the war does not hear them.

| Key | Action |
|---|---|
| WASD | move the bearer |
| 1 / 2 / 3 / 4 | Follow / Charge (at cursor) / Hold / Rally — **to all seven** |
| **F** | **flip Q23 A ↔ B, live, mid-fight.** The whole comparison on one key |
| **Q** (hold) | **Q26 = D verb wheel** — point, release to arm. Q23 = A only |
| **LMB** | target the armed verb (Q23 = A) / order the selected soldier (Q23 = B) |
| **Z X C V B N M** | **Q26 = B** — select one of the seven. Q23 = B only |
| **E** | **Q26 = A** — one click, the cursor decides the verb and who casts it. Q23 = B only |
| Num1 / Num2 / Num3 | camera: close / army / strategic |
| R | restart the run |

### Console

| Command | What it does |
|---|---|
| `Swarm.UnitStance <0-6> Follow\|Charge\|Hold\|Rally` | order **one** named soldier. 0-6 are the seven; 7 is the garrison and warns |
| `Kindled.Seven.Report` | log each of the seven — archetype, look, rung, health, order, kills |
| `Kindled.Ability.Use <0-6\|focus\|screen\|raise\|rally> [X Y]` | fire one verb without a keyboard — the headless half of the input scheme |
| `Kindled.Ability.Report` | which Q23 shape is live, every verb + cooldown, what is standing, and casts per Q26 scheme |
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

### Dials — the ability kit (task-144)

Every tuning value in the kit is a CVar, none is a constant. Both `ELVTR/Saved/SwarmExecOnPlay.txt`
and `ELVTR/Config/SwarmExecOnPlay.canonical.txt` carry the block, in step.

| CVar | Default | What it decides |
|---|---|---|
| **`Kindled.Ability.Mode`** | **0** | **The switch. 0 = Q23 A (fixed kit on the bearer, verb wheel). 1 = Q23 B (verb in the soldier, select-then-order + direct target). `[F]` flips it live** |
| `Kindled.Ability.PlayerRange` | 1600 | Q23 = A only: how far from the bearer a soldier still counts as in reach. A cast with nobody in reach refuses |
| `Kindled.Ability.Cooldown` | 12 | Seconds between casts — per **verb** under A, per **soldier** under B. The dial for the rotation failure `ability-kit.md` §5 names |
| `Kindled.Ability.FocusSeconds` | 6 | Mark Quarry duration |
| `Kindled.Ability.ScreenSeconds` / `ScreenRadius` / `ScreenScale` | 8 / 700 / 0.35 | Ward Circle: how long, how wide, and damage taken inside it |
| `Kindled.Ability.RaiseSeconds` / `RaiseRate` | 5 / 90 | Kindle: channel length and HP/s |
| `Kindled.Ability.RallySeconds` / `RallyRadius` / `RallyHaste` | 8 / 900 / 1.6 | Banner Slam: how long, how wide, and the swing-**clock** multiplier inside it |
| `Kindled.Ability.PickRadius` | 400 | Q26 = A only: how close the cursor must be to mean a thing. Widen → guesses more, narrow → refuses more |
| `Kindled.Ability.Draw` | 1 | 0 hides the wheel and the zone rings. Every verb still works |

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

> **Where the tracked copy actually is.** `docs/design/SwarmExecOnPlay.txt` does not exist and
> never did — the real version-controlled copy is
> **`ELVTR/Config/SwarmExecOnPlay.canonical.txt`**. It was out of sync by this one line when
> this slice landed; **task-144 verified both copies now read `Swarm.HeroDPS 0`** and added the
> ability-kit block to both in step. Diff them before trusting any "restored the tuning" claim —
> that is what the canonical copy exists for.

---

## 6. What was faked, and every prototype expedient

Listed so none of it is mistaken for a decision.

| # | Expedient | Why | What it costs / upgrade path |
|---|---|---|---|
| 1 | **The seven are Mass entities on existing command handles, not promoted Actors.** | **Q25 is open and this does not answer it.** This is the cheap path and the *only* reason it was taken: every per-unit facility the seven need — stance, rung, kill credit, standing, centroid, health — already exists and already works on a handle holding one body. | If Q25 lands on promoted Actors, the roster table and the spawn path go; the boss already demonstrates the Actor bridge those seven would use. |
| 2 | **The seven's roster is a fixed authored table** (`Spike/SevenRoster.h`). | A slice needs seven concrete bodies to point at, and "fixed roles" is the cheapest of Q2's three options. **Not because fixed roles won the argument.** | If Q2 lands on survivors or on loadout, this table is what gets deleted. Nothing else assumes its shape. |
| 3 | ~~**The seven have no abilities.**~~ **SUPERSEDED by task-144 — see §10.** | The kit exists now, and it exists as **both** of Q23's first two shapes behind one switch, precisely so the register's rule ("must not be settled by whatever gets prototyped first") still holds with code on the ground. | Q23 and Q26 are still open. §10.8 lists what the kit itself fakes. |
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
| **Q23** | The squad-channelled ability kit | **STILL OPEN, with both shapes built (§10).** A *and* B are reachable in one session on `Kindled.Ability.Mode` / the `F` key. Building one would have closed it by accident; building both is what keeps it an owner call. |
| **Q26** | How orders are issued | **STILL OPEN, with three of four schemes built (§10.5).** D (wheel) pairs with A; B (select-then-order) and A (direct target) are both live under B. C is not separately built, and §10.5 says why. |
| **Q25** | Are the seven Mass entities or promoted Actors? | **Open.** This slice picks Mass entities as a prototype expedient (§6.1) and says so in the code. The boss demonstrates the *other* path working in the same build, so neither option is now cheaper by default. |
| **Q2** | Composition of the seven | **Open.** The roster is a fixed table because that was cheapest to build (§6.2). Task-144 added a verb column to it, which §10.3 shows is doubled three ways — a *roster* finding for Q2, not a verdict on it. |
| **Q14** | Is seven a cap, a floor, or a field size over a bench? | **Open.** Seven is hard-coded as the field size and nothing implies which. |
| **Q15** | Can your seven be downed and revived? | **Open.** They die and stay dead for the run because nothing revives anything (§6.9). Kindle heals a *standing* soldier and revives nothing — deliberately, so this stays open (§10.8 row 11). |
| **Q4, Q5, Q24** | Enemy planner, the `Breaking`→`Fallen` window, ledger authority | **Untouched.** All three need the war ledger, which is out of scope. |

---

## 8. Known gaps and handoffs

- ~~**`ELVTR/Config/SwarmExecOnPlay.canonical.txt` is out of sync by one line.**~~ **Closed.**
  Both copies now read `Swarm.HeroDPS 0`, and task-144's ability-kit block went into both in step.
- ~~**The HUD is on-screen debug text, not UMG.**~~ **Closed by task-144 (§10.6):** both were
  extended. `SquadCard` / `MusterPanel` / `KindledHud` now describe the seven by name with a verb
  chip each and the garrison as one `THE WAR` card. The prototype debug HUD is still the one you
  see by default; the UMG band is on `Kindled.UI.Hud` / `Kindled.UI.AutoShow`.
- **No capture path in this project can film the HUD or the boss.** `Swarm.DebugShotAfter` goes
  through a `SceneCaptureComponent2D`, which renders world primitives — not Slate text and not
  debug draws. `Kindled.Seven.Report` and `Kindled.Boss.Report` exist because of this; on a
  headless run the log is the only readout.
- ~~**Archers rarely strike the boss while brood are alive.**~~ **This turned out to be every
  soldier's problem, not the archers', and task-144's FOCUS verb is the answer** — see §10.1.
  A soldier's reach is the distance to its *K-th nearest* enemy, so anyone standing in a live
  wave hits the brood at its elbow rather than the boss behind it. Under a mark, that soldier's
  blow reaches the boss anywhere inside its own engage range.
- **The war has no ledger, so nothing calls you anywhere.** The loop in `castle-layout.md` §9
  steps 1-3 (read the fronts, one goes `Breaking`, cross gates to reach it) does not exist. This
  slice is steps 4-6.

---

## 10. The ability kit — both shapes of Q23, three of Q26's schemes (task-144)

> **Q23 and Q26 ARE STILL OPEN.** `docs/OPEN-DECISIONS.md` was not edited, no box is ticked,
> and nothing below is a verdict. Both of Q23's first two shapes are built and reachable in one
> session; three of Q26's four schemes are built and reachable in one session. The point of the
> build is that the owner can play the comparison instead of reading an argument about it.
> A player's report — explicitly not a decision — is §10.7.

### 10.1 The four verbs

Named by `docs/design/ability-kit.md` §1, sourced from `CLASSES.md`'s hero kits per Q29 = A.
One implementation each; both addressings reach the *same* four effects.

| Verb | Source kit | What it actually does in the sim | Where it lives in code |
|---|---|---|---|
| **FOCUS** | **Mark Quarry** (Pathfinder) | The marking soldiers' blows reach the **boss** anywhere inside their own engage range, brood in the way or not — and they are ordered to Charge it | `SwarmCombatProcessors.cpp`, the boss-claim gate |
| **SCREEN** | **Ward Circle** (Relickeeper) | A ground zone; retinue **and the bearer** inside take `ScreenScale` (0.35) of incoming damage | `SwarmCombatProcessors.cpp` at the one place HP falls, + `AddPendingHeroDamage` |
| **RAISE** | **Kindle** (Lampbearer) | Heal over time on one of the seven, clamped to MaxHP | `SwarmCombatProcessors.cpp`, retinue branch |
| **RALLY** | **Banner Slam** (Vanguard) | A ground zone; retinue inside swing `RallyHaste` (1.6x) faster **and are leash-exempt** while it stands | swing clock in `SwarmProcessors.cpp`; leash in the retinue-follow pass |

**Why FOCUS is the load-bearing one.** In this sim what stops a soldier hitting the boss is not
range, it is **priority**: `StrikeReachSq` is the distance to that soldier's *K-th nearest*
enemy, so a soldier standing in a live wave reaches the brood at its elbow and never the boss
two bodies behind it. §8 above recorded that as an archer quirk; it is every soldier's, and it
is why a boss fight inside a wave felt like nothing you did mattered. Focus is the answer, and
it **redirects** a swing rather than adding one — the claim still spends from that soldier's own
`BlowsClaimed` budget.

**Reposition is deliberately not a fifth verb.** `ESwarmStance::Hold` already anchors a unit on
the ground it was called on, which is Shield Wall's mechanical text verbatim. A fifth verb would
have been a second way to press the same button.

### 10.2 Q23 = A — a fixed kit on the bearer (`Kindled.Ability.Mode 0`)

All four verbs are the bearer's, always. Each applies to whichever of the seven are inside
`Kindled.Ability.PlayerRange` (1600uu) — the register's own phrasing. Cooldowns are **per verb**.

A cast with nobody in reach **refuses and says why**. That is shape A's stated cost ("the seven
risk becoming interchangeable") turned into something that happens to you rather than something
that was written down.

### 10.3 Q23 = B — the verb lives in the soldier (`Kindled.Ability.Mode 1`)

Each of the seven carries one verb; the bearer has none of his own. Cooldowns are **per soldier**.
The binding is `ability-kit.md` §2 B's own suggested one, not a new proposal:

| | Vanguard | Relickeeper | Pathfinder | Lampbearer |
|---|---|---|---|---|
| soldiers | Ash, Rook | Cairn, Slate | Wren, Kite | Ember |
| verb | RALLY (Banner Slam) | SCREEN (Ward Circle) | FOCUS (Mark Quarry) | RAISE (Kindle) |

**The shape of that table is itself the evidence `ability-kit.md` §5 asks for.** Three verbs are
doubled and Kindle is held by exactly one body, so **losing Ember loses raise for the run** while
losing Wren still leaves Kite able to mark. It is not balanced and is not meant to be — it is the
"is the choice hollow?" question made concrete. The HUD prints `RAISE LOST` on a downed carrier,
and the mode-A capture below caught `[1] Rook VANGUARD DOWN — RALLY LOST` for real.

### 10.4 Three mechanical differences between the shapes

Not cosmetic. These are what the comparison is about.

1. **Who marks.** Under A, focus is cast by whoever is in the bearer's reach. Under B it is cast
   by **the pack** — `CLASSES.md`'s "the entire pack focus-fires it". Same verb, different number
   of soldiers turning onto the boss, visible on screen without reading a number.
2. **Where a zone lands.** Under A the Ward Circle and the banner go **at the cursor** — the
   player's own spells, and a soldier's position is irrelevant to them. Under B they are
   inscribed **where the casting soldier stands**, so positioning the seven is how you aim them.
   That is the difference between commanding a kit and commanding people, and it is the clearest
   thing in the two captures: the circles sit on the bearer in A and up on the front line in B.
3. **What running out looks like.** Under A you run out of cooldown. Under B you run out of the
   soldier.

**Flipping the mode does not reach backwards.** Effects already standing keep behaving as the
shape that cast them, and both cooldown sets are kept live at once, so a mid-fight flip never
reads the other shape's clock.

### 10.5 The order schemes, and the input surface (task-137 closed against this)

`SpikeHeroPawn` no longer polls keys. Input is **Enhanced Input**: one `UInputMappingContext`
with ~20 `UInputAction`s, bound with lambdas in `SetupPlayerInputComponent`. That migration was
required rather than optional — the Q26 = D wheel is a *hold*, which needs a real
Started/Completed pair and not two `IsInputKeyDown` reads compared against last frame's bool.

**The assets are built in C++, not in `ELVTR/Content/Input`.** `NewObject`'d into the pawn at
possession, so there is no `.uasset` and no editor round-trip to change a binding. Deliberate
stopping point, not an oversight: content assets buy designer-editable rebinding, which nothing
has asked for, and cost an editor session per change. Moving to real assets replaces
`BuildInputMap()` and touches nothing else, because every binding already goes through a
`UInputAction*`. **`ELVTR/Content/Input/` is therefore still empty.**

| Q23 | Q26 scheme | Input | Why this pairing |
|---|---|---|---|
| A | **D · verb wheel** | hold `Q`, point, release to arm, `LMB` to target | `PREFLIGHT.md` §3 and `ability-kit.md` §3 both name D as A's fit: A's verbs target three different kinds of thing, so the verb has to be chosen before a target can mean anything |
| B | **B · select-then-order** | `Z X C V B N M` select, `LMB` order | `ability-kit.md` §3's own finding — the most literal fit for B, whose premise *is* "you are choosing who acts" |
| B | **A · direct target** | `E` at the cursor | `PREFLIGHT.md` §3 names A as B's fit. Built too, live at the same time, so the two need no second flip to compare |

**Q26 = C (contextual single button) is not separately implemented.** Under Q23 = B it is the
same input as A with the resolution rule doing more of the work, and one honest implementation of
that resolution is worth more than two that differ by a comment. Stated here rather than left to
be discovered.

**The direct-target resolution rule, in full** — small on purpose, because its *ambiguity* is
what §5 says to watch:

1. cursor within `PickRadius` of the boss → FOCUS
2. cursor near one of the seven, below 90% HP → RAISE
3. cursor near one of the seven, healthy → SCREEN
4. open ground → RALLY

Then the carrier of that verb nearest the cursor acts, preferring one that is ready. Rule 3 is
the known-ambiguous one: "near a soldier" and "open ground" are a hair apart at any pick radius,
so screen and rally trade places under small cursor movements. Left visible, not smoothed.

**The evidence channel for "which scheme did your hand actually reach for":**
`Kindled.Ability.Report` prints casts and refusals per scheme, with the console counted
separately so a scripted run cannot be mistaken for clicks.

### 10.6 The HUD (castle-layout.md §6.4)

Both HUDs were extended; neither was replaced.

- **The prototype HUD** (`SpikeHeroPawn::DrawHUD`, what you actually see) gains a **"what this
  one can currently do"** column per soldier — `FOCUS READY` / `SCREEN 10.8s` / `RALLY LOST` under
  Q23 = B, `in reach` / `out of reach` under Q23 = A — plus a live `KIT` line for standing
  effects, a `>` marker on the selected soldier, and two lines naming **both** open options
  rather than presenting one as "the controls".
- **The UMG side** (`SquadCard` / `MusterPanel` / `KindledHud`) now describes seven individuals:
  cards carry the soldiers' **real names** off `SevenRoster` with the garrison as one `THE WAR`
  card, read each unit's **own** order instead of the last broadcast one, and draw a second chip
  for the verb and its cooldown. The five invented squad names ("Shield", "Vets", "Spearmen"…)
  are gone — they described a company that no longer exists.

### 10.7 A player's report — **not a decision**

Measured and watched in the captures below, at the shipped defaults.

- **B reads better, and it is the HUD that makes it read.** Under A the per-soldier line can only
  say `in reach` / `out of reach`; under B it says what each one *does* and when. §6.4's sentence
  about the HUD's job turns out to be describing shape B — under A there is genuinely less to say
  about an individual, because under A an individual carries nothing.
- **B's zones being anchored on the caster is the single best thing in the build.** It makes
  where you put a soldier matter, which is the only place in the slice where the seven feel like
  people standing somewhere rather than four buttons.
- **The hollowness `ability-kit.md` §5 predicts is real and visible at this roster.** Three verbs
  doubled and one held alone means the "choice" is often between two carriers of the same verb.
  That is a *roster* problem (Q2), not a shape problem, and it would be fixed by seven distinct
  verbs rather than by choosing A.
- **`ability-kit.md` §4's burst gap is real and it bit in play.** During the wheel capture the
  Sated boss regenerated to 6000/6000 while the fight stalled. `RallyHaste` is the nearest thing
  the four kits have to burst, and it is a *rate*, not a burst. Nothing in this build closes that
  gap and nothing here should be read as claiming it does.
- **If forced to pick, I would pick B with select-then-order** — B because A cannot express
  "hold this soldier back" at all, which is the counter Quilled wants; select-then-order because
  in play the `E` direct-target key repeatedly resolved rule 3 into the verb I did not mean, and
  a scheme whose failure is invisible is worse than one that is a beat slower. **This is a
  player's report on one session, not a call. Q23 and Q26 close when the owner says so.**

### 10.8 What was faked or left out, in the kit specifically

| # | Expedient | Why | Upgrade path |
|---|---|---|---|
| 11 | **Kindle has no overheal shield and revives nothing.** | The shield half needs a shield mechanic the sim does not have; the revive half is **Q15, which is open and must not be answered here**. | A shield fragment; Q15's answer. |
| 12 | **Mark Quarry can only mark the boss.** | It is the only body in the sim with an identity to point at. A mark on an interchangeable brood would be a damage buff wearing a verb's name. | Any promoted enemy Actor works the same way. |
| 13 | **Banner Slam's "fights to the death" is leash-exemption.** | Nothing in this sim routs, so there is no flee behaviour to suppress. The leash is the only thing that ever drags a soldier off a fight against your order — so a bannered soldier is exempt from it, exactly as the garrison is. | A real morale/rout state. |
| 14 | **Kindle's channel does not commit the caster.** | `CLASSES.md` calls Kindle channeled, but no channel-interrupt mechanic exists. The duration is real; the commitment is not. | An interrupt condition. |
| 15 | **The zones and the wheel are debug draws.** | No capture path here can film Slate or debug draws (§8), so a debug circle is the only thing a screenshot can catch. | A decal or Niagara ribbon; no sim change — nothing reads the drawing. |
| 16 | **Q26 = C is not separately built**, and `ELVTR/Content/Input/` has no assets. | Both stated in §10.5 with the reasoning. | Both are additive. |

### 10.9 Evidence

Whole-window captures of the floating PIE window — the only channel that catches the HUD and the
debug draws (§8). **`slice-a7-kit-modeA.png` and `slice-a7-kit-modeB.png` are the same boss**, a
`QUILLED+SATED` instance, seconds apart, flipped with one command.

| File | What it shows |
|---|---|
| `docs/design/slice-a7-kit-modeA.png` | **Q23 = A.** `KIT FOCUS 4.8s (7 marking) SCREEN 6.8s RAISE Wren 3.8s RALLY 6.9s` — all four standing at once. Ward Circle and banner **centred on the bearer**; seven red mark-tethers to the boss; `Slate … out of reach` showing the reach test bite |
| `docs/design/slice-a7-kit-modeB.png` | **Q23 = B**, same boss ~20s later. Per-soldier verbs and cooldowns (`FOCUS READY`, `SCREEN 10.8s`, `RALLY READY`…); the two zones have **moved up to the front line** where Cairn and Ash are standing |
| `docs/design/slice-a7-kit-wheel.png` | **Q26 = D** open: the wheel at the cursor with `FOCUS` / `SCREEN` / `RALLY` spokes and the hovered one lit. Also caught two downed Pathfinders reading `FOCUS LOST`, and a Sated boss back at 6000/6000 — §4's burst gap, live |

Log evidence, from the same session:

```
Ability: FOCUS  (Mark Quarry) — Q23=A via Q26=D wheel,             by the bearer — 1 marking the marked boss
Ability: RAISE  (Kindle)      — Q23=B via Q26=B select-then-order, by Ember — Wren, 90 HP/s for 5s
Ability: RALLY  (Banner Slam) — Q23=B via Q26=A direct-target,     by Rook  — banner at (1962, 7), swing x1.60 + no leash for 8s
  casts/refusals by scheme — wheel(Q26=D) 0/0 | select-then-order(Q26=B) 3/0 | direct-target(Q26=A) 1/0 | console 0/0
```

### 10.10 The one line for flipping between them

```
Kindled.Ability.Mode 0    # Q23 = A — fixed kit on the bearer, verb wheel on Q
Kindled.Ability.Mode 1    # Q23 = B — the verb lives in the soldier, ZXCVBNM + LMB, or E
```

or press **`F`** in game, which does the same thing and logs which shape went live.
