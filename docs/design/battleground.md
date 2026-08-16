# Battleground — a dedicated field-battle testbed, two AI commanders, one back-channel

**What this is:** the design spec for a new level, **the Battleground** — a large open-field
fight between two formed armies, each run by its own AI commander, with a hidden channel
between the commanders whose job is to keep the fight dramatically readable for whichever side
the player is leading. **Extends:** `GDD.md` §4 (stances), §10 (Mass Entity constraints);
`SYSTEMS.md` §5 (pacing director, undesigned) and §6 (retinue tuning); `docs/design/
squad-group-system.md` (typed units, `MaxSquads`); `docs/design/squad-actors.md` §1 (per-handle
facilities). **Not** a redesign of the castle — separate level, separate game mode, no upkeep,
no Adaptation, no five-layer geometry.

---

## 0. Canon check before speccing (read this before the rest)

Two files this brief cited turned out to be **stale on arrival** — flagging per
[[canon-moves-dont-propagate]] before building on them:

- The brief describes `URetinueFormationProcessor` as forming "the retinue" and implies the
  player commands a growing crowd. **The castle pivot (2026-08-13) already changed this in the
  running code.** `Spike1GameMode.h:50-60` — the 128-body Mass pool is now **the garrison, not
  the player's army**; the player commands **seven named soldiers**, one handle each
  (`SwarmSubsystem.h:59-61`, `docs/design/castle-layout.md` D1). `URetinueFormationProcessor`
  still forms bodies into typed ranks — it just forms the garrison's ranks now, and the seven's
  bodies sit alongside them in the same handle space.
- This does not block the Battleground level — it's a separate testbed, and the underlying
  Mass mechanism (typed squad handles, per-type formation, per-handle stance) is exactly what
  this spec needs regardless of which game mode currently owns it. It does mean **"the retinue"
  is not a live noun to design against** — everything below is specced against the handle
  mechanism directly (`USwarmSubsystem`, `SwarmFormation`, `EUnitType`), not against a "player
  army" concept that no longer means what it used to.
- `docs/design/squad-group-system.md`'s v1 roster — **Spearmen and Archers**, exactly two types
  — is current and is what this level fields for both sides (§1.1 of that doc; stat and
  formation tables reused verbatim below).

---

## 1. What the level creates on load

### 1.1 The place

`ELVTR/Content/Battleground/L_Battleground.umap` — one flat, open field, no castle geometry,
no gates, no rings. This satisfies the same load-bearing constraint the old arena ladder
named before the castle superseded it: an authored open space sized for a horde fight is "a
*requirement*, not a nicety" (`GDD.md` §9) — that reasoning outlives the section it was written
in. Two **deployment zones** at opposite ends of the field, far enough apart that neither
army's formation spawns inside the other's `Swarm.BroodAggroRange`-equivalent engage distance,
close enough that a Charge order closes the gap in a readable few seconds, not a march.

### 1.2 The two armies

Both armies field the same v1 roster as the Vanguard retinue — **Spearmen and Archers**
(`squad-group-system.md` §1.1) — mirrored. This is deliberate, not a placeholder: a mirror
matchup is the cleanest way to prove the two-commander system on its own terms, with no
composition asymmetry muddying whether a bad outcome came from the commanders or from unequal
armies. Per-type formation and stance reflavors are reused **verbatim**, both sides:

| Field | Spearmen | Archers | Source |
|---|---|---|---|
| Shape | Block | Block | `squad-group-system.md` §1.7 |
| Columns / Spacing / RankSpacing | 12 / 42.4 / 110 | 20 / 55 / 70 | same |
| Forward (push from army's own spawn line, toward the enemy) | 250 | 40 | same |
| Stance reflavor | Advance the Line / Shield Wall / To the Banner | Volley Advance / Loose from Cover / Fall Back | `squad-group-system.md` §1.8 |
| Combat | `MeleeRange` ~95uu, `TargetsPerHit` 8 (cleave) | `EngageRange` 750uu, `MinEngageRange` 150uu, `TargetsPerHit` 1 | §2.2/§2.3 of same |
| Body tier | Militia (130 HP / 30 DPS) | Militia | `SYSTEMS.md` §1 |

**Recommended v1 headcount: 150 Spearmen + 40 Archers per side (190/side, 380 total).**
Reasoning, not a guess: `squad-group-system.md` §4.1's allocation formula —
`WantedUnits(type) = ceil(pool / 80)` — gives each army **2 Spearmen units + 1 Archer unit = 3
handles**, so both armies together cost **6 of the shared 8 handles** (§2 below), leaving 2
spare before any fold. It also sits comfortably under the measured entity ceiling (`GDD.md`
§10: 13,000–20,000 total entities hold 60fps) with enormous headroom, and mirrors the 80/20
Spearmen/Archer growth-source split the squad-group-system doc already uses elsewhere, so no
new ratio is invented. **This is a starting dial, not a cap** — §5's Build scope evidence
includes confirming the level still holds frame budget if headcount is raised later.

Facing: each army's Spearmen form the front rank (`Forward` pushes them toward the enemy),
Archers sit behind them at their own smaller `Forward` offset — identical to how the squad-
group-system doc already screens Vanguard archers behind Vanguard spearmen. Same geometry,
now facing an equivalent enemy formation instead of an aggroing tide.

---

## 2. The commander model

### 2.1 One commander per army, ordering through the existing stance API

Each army gets one commander — a plain `UObject` (not a Pawn; it has no body, only orders),
instanced twice with a `TeamId`. It issues the same four verbs every stance system in this
project already uses — Follow / Charge / Hold / Rally — addressed **per type**, via the
existing per-unit stance API: `SetStance`/`SetUnitStance` write `UnitStance[UnitIndex]` +
`UnitStanceAnchor[UnitIndex]` (`SwarmSubsystem.h:225-263`), read by the shared stance-
interpretation processor every Mass entity already goes through. **No new order-issuing
mechanism** — a commander is a thing that decides *when* to call the same function a player's
stance-wheel input already calls.

**Decision cadence:** not per-frame. A commander ticks on an interval (recommend 1.5–2s,
matching the `LookLerp`/pacing cadence elsewhere in this codebase rather than inventing a new
one) and reads read-only squad state already exposed for exactly this purpose:
`GetSquadStanding`, `GetSquadCentroid`, `GetSquadHP`/`GetSquadMaxHP` per handle
(`SwarmSubsystem.h:733-754`). This satisfies Design Law 5 for free — the commander never
touches an individual soldier, only ever addresses a typed handle, exactly the granularity the
game's whole command system is already built around.

### 2.2 A second FORMED team — the actual gap in the sim

**Confirmed by reading the code, not assumed:** there is exactly one "formed, stance-driven"
side today (`URetinueFormationProcessor`) and one "boid" side (`UBroodSteeringProcessor`,
`SwarmProcessors.h:24-34` — steering only, no stance, no formation). Battleground needs **two**
formed sides. Three ways to get there:

**Option A — double `MaxSquads` to 16, one bank of 8 per army.** Rejected. Every per-handle
array in `USwarmSubsystem` (`SquadStanding`, `SquadCentroidSum`, `SquadHP`, `SquadType`,
`UnitStance`, `SquadVariant`, `SquadTier`, `WaveKilledBySquad`, ~12 fields,
`SwarmSubsystem.h:1034-1080`) would double, and `MaxSquads = 8` is stated as a **hard cap**
(`SwarmSubsystem.h:46`) other systems reason about (the muster HUD's 4-card-per-wing layout,
Design Law 2's "bound the handles"). This is shared engine code — a testbed level should not
widen a constant the castle's own canon leans on, even though `USwarmSubsystem` is a
`UWorldSubsystem` and each map gets its own instance at runtime.

**Option B — repurpose Brood as the second army.** Rejected outright. `UBroodSteeringProcessor`
has no stance interpretation and no formation slotting (`SwarmProcessors.h`) — it is boids, not
ranks. The brief's explicit requirement is "both armies fight in formations"; Brood cannot do
that without becoming a second `URetinueFormationProcessor`-shaped system, which is Option C
with extra steps.

**Option C — recommended. Reuse the existing `bRetinue`/`TeamBit` flag as a generic two-team
flag, and range-partition the shared 8 handles between the two armies exactly the way
`squad-group-system.md` §4.1 already partitions handles between Spearmen and Archers.**
`Anim[i].Bits & SwarmAnim::TeamBit` already computes a per-entity boolean team membership
(`SwarmCombatProcessors.cpp:771`), and every targeting/claim comparison in that file already
keys off it (`Entry.bRetinue == bRetinue` at line 870, the retinue/brood split throughout).
**It is already a team bit, not a "formed vs. boid" bit** — today's codebase just only ever
populates one side of it with formed bodies. Battleground populates *both* sides with formed
bodies and both call `SetStance`. Cost against the 8-handle budget: **zero new handles.**
§1.2's 6-of-8 usage (3 handles/army: 2 Spearmen units + 1 Archer unit) fits inside the existing
cap with headroom, using the *same* `AssignRecruit`/formation/stance plumbing every other
formed body in the project uses — no parallel command system, no new fragment.

**What genuinely has to change in C++** (named here, not designed — the build task owns
implementation):
- `AssignRecruit` (`SwarmSubsystem.h:913`) assumes one friendly pool claiming handles
  fill-lowest-first. It needs a `TeamId` parameter so Battleground's game mode can route
  Army A's recruits into handles 0-2 and Army B's into 3-5 (or an equivalent split), instead of
  both armies fighting over the same handle range.
- The targeting/claim comparisons in `SwarmCombatProcessors.cpp` (lines 771-773, 870, and the
  retinue/brood branches around 972-1164) currently read `bRetinue` as "am I the one formed
  side" in several places. Under two formed teams those need to mean "is this the *other*
  team," which for a strict two-value bool is the same bit reinterpreted — but every call site
  that assumed "not retinue = brood, has no `SquadId`, no upkeep, no kill-credit" needs an
  audit pass, because Battleground's "not Team A" bodies are fully typed, upkeep-less-but-not-
  brood combatants, not boid fodder.

This is a real, scoped Mass change — not a new system, a generalization of an existing bit
that the code already names `TeamBit`.

---

## 3. The back-channel

### 3.1 What it is, reframed by a quick sim (§6)

The brief's framing — "enforce a good experience," in the owner's words — was checked against
a toy attrition model before speccing the mechanism (§6). **The finding reshapes what the
back-channel should actually do.** Two symmetric formed armies (identical stats, identical
`TargetsPerHit=8` cleave, small random noise) do not spontaneously snowball — they grind to a
near-simultaneous mutual collapse in a handful of exchanges, with essentially no decisive
margin. This is the *exact* stalemate behavior `castle-layout.md` §1 already documents as the
sim's honest default output at any scale ("two simulated armies of comparable strength...
produce a grind"). **The back-channel's real job is not preventing a runaway blowout — a
mirror matchup barely produces one on its own. It's manufacturing a legible arc (rising
tension, a visible push, a climax) out of a fight whose natural default is an anticlimactic
tie**, which matches how `castle-layout.md` itself resolves stalemate: not by tuning the grind,
but by scripting a **discrete break event** ("the enemy breaks stalemates with individuals, not
with mass," §1).

### 3.2 What the two commanders exchange

A small shared state, owned by the level (a `UBattlegroundDirector` member on the game mode,
not a new subsystem — this is scoped to one level, not a project-wide director):

- **Tension curve target** — a 0–1 authored value over the match's expected length (a slow
  build, not a random walk), read by both commanders as a shared clock for "how aggressive
  should orders be right now."
- **Casualty pacing** — recent KIA rate per side (`GetSquadStanding` deltas over the last few
  ticks), so both commanders know whether the fight is currently a grind or a swing.
- **The break schedule** — the actual lever, per §3.1's finding: a shared timer/condition for
  *which* side gets a scripted push (a Charge order timed to land as a visible clash) and when,
  so the two AI's Charge/Hold/Rally choices don't fire independently and cancel each other into
  more grind. This is an **order-timing coordination**, not a stat nudge.
- **No-snowball guard** — a floor, not a buff: if one side's live headcount ratio crosses a
  threshold (proposed 70/30, unmeasured — flag for playtest), the losing commander is nudged
  toward **Rally** (fall back, consolidate, buy time) rather than **Hold** (attrite in place),
  which is an order choice already in its vocabulary, not a new mechanic.

### 3.3 What it may override, and what it must never fake

**May override:** either commander's own locally-decided stance choice, for the sake of timing
— e.g. holding a Charge order back a beat so it lands as a coordinated clash instead of firing
mid-tick against a Hold. This is coordination between two AI decision processes the player
never has to see negotiate; only the resulting orders (which the player *does* see, as
formation movement) are observable.

**Must never fake, on either side, ever:**
- **No hidden DPS/HP multipliers.** The toy sim (§6) shows a continuous output clamp barely
  moves the needle and, worse, actively *dampens toward tie* — the wrong direction for
  producing a readable climax, and the kind of invisible thumb-on-the-scale a player would
  rightly call unfair if they ever profiled the numbers. The back-channel's only lever is
  **which order fires when** — the same four verbs, the same stance system, fully visible in
  formation behavior. This also keeps the design consistent with Design Law 2 (soft costs, not
  hidden caps) and Design Law 6 (readable danger) — an invisible multiplier is unreadable by
  definition.
- **No invisible bodies.** Reinforcement, if it exists in this level at all, must arrive the
  same way `GDD.md` §9 already mandates for the castle — dripped in over frames, never a
  silent `BatchCreateEntities` nobody saw land (`GDD.md` §9's 23.46ms spike constraint applies
  here too; not re-litigated, just inherited).
- **Never overrides the player's own side.** If the player is leading one army (§4), the
  back-channel governs the *two AI commanders'* mutual pacing only — it must never intercept or
  veto a stance order the player issues. The moment it does, "leading" becomes decorative.

### 3.4 Relationship to `SYSTEMS.md` §5 and task-036

**This is a scoped instance of that undesigned director, not a separate system.** `SYSTEMS.md`
§5 names the intent — an L4D-style intensity manager reading run state to spike and breathe —
and states plainly it is "not yet designed." The Battleground back-channel is the same idea,
narrowed to the smallest possible surface: two armies, one clash, one climax, instead of a
castle's five fronts. Building it here first, at this scope, is a legitimate way to learn what
the real director needs before committing to the harder multi-front version — worth a pointer
entry in `SYSTEMS.md` §5 when this spec is reviewed (not written there by this doc).

---

## 4. Player role — leads one army

**Recommended: the player commands one army through the existing stance system; the other
army is fully AI (§2).** Not an observer mode. Three reasons, not a survey:

1. **The level's stated purpose is to run "the formation/GM process at army scale."** A GM
   process has someone at the helm. An observer-only version tests whether two AI commanders
   can fight each other, which is a real but much smaller question than the one this level
   exists to answer.
2. **Zero new input surface.** The player issues Follow/Charge/Hold/Rally to their own
   Spearmen/Archer handles through the exact same stance-wheel and muster-card path the seven
   already use (`SwarmSubsystem.h:225-263`). This level costs nothing on the input side.
3. **It's the only version that exercises the back-channel's actual stated purpose.** The
   owner's own framing — "enforce a good experience for the player watching/leading one side" —
   names leading directly. A pure-observer build cannot test whether the back-channel keeps a
   player-led fight fair and dramatic, because there is no player decision in the loop for it
   to protect.

Concretely: the player's army gets **one commander-shaped thing fewer** than the enemy — no AI
decision loop on their own side, just their own stance input feeding the same
`UnitStance`/`UnitStanceAnchor` array the AI commander writes to on the other team. The
back-channel still reads the player's side's live state (casualties, centroid) for pacing
purposes — it just never writes orders there.

---

## 5. Build scope

Exact artifacts, and the PIE evidence that proves each:

| Artifact | Proves | Evidence |
|---|---|---|
| `ELVTR/Content/Battleground/L_Battleground.umap` | Level loads, both armies spawn in formation | Screenshot/description: two Block formations, ~190 bodies each, facing each other across the field at match start |
| `ABattlegroundGameMode` (`ELVTR/Source/ELVTR/Battleground/`) | Match setup — recruits both armies via `AssignRecruit(Type, TeamId)`, sets deployment zones, starts both commanders | Log line at `BeginPlay` confirming handle allocation matches §1.2 (2 Spearmen + 1 Archer unit, ×2 teams = 6/8 handles claimed) |
| Commander class (one `UObject`, instanced ×2 with `TeamId`) | AI army fights without player input | Log/debug overlay showing the **enemy** commander's `UnitStance` values changing autonomously over a fight (not stuck on Follow) |
| `UBattlegroundDirector` (back-channel state) | Two commanders coordinate, not just act independently | Debug readout of the shared tension/break-schedule value over the match, plus at least one observed case of the break schedule firing a coordinated Charge that both armies' formations visibly close on together |
| Mass change: `AssignRecruit` `TeamId` param + `SwarmCombatProcessors.cpp` targeting audit (§2.2) | Two-team combat is mutually exclusive and correct | A captured frame/log showing Army A strikers only ever claim blows against Army B `SquadId`s (and vice versa) — no friendly-fire claims, verified via kill-credit or HP deltas on the correct side only |
| Player stance path (no new code — reused) | Player-led side responds identically to the seven's existing stance input | Screenshot/log: player toggles Charge on their own Spearmen mid-fight, formation advances per §1.2's `Forward` value, exactly as the castle's existing stance system already does |
| Perf sanity | This level doesn't reopen the entity-count question | A `stat unit` or existing bench-style capture at the §1.2 headcount (380 total), confirming frame time inside the 16.6ms budget — not a new gate, just a confirmation |

---

## 6. Simulation notes

**Simulated (scratch Python, not committed):**

1. **Handle-budget arithmetic** — `WantedUnits(type) = ceil(pool/80)` (`squad-group-system.md`
   §4.1) applied to both armies at three headcounts (120/side, 190/side, 280/side). Result:
   190/side (§1.2's recommendation) costs 6 of 8 shared handles; 280/side is the point where the
   shared budget is fully consumed (8/8) with zero headroom — a real ceiling, useful if this
   level is ever scaled up.
2. **Toy attrition sim** — two symmetric 190-body armies (Militia stats, `TargetsPerHit=8`
   cleave both sides per `GATE1-FUN-PROTOTYPE.md` §3b's shipped defaults), 20 seeded runs, with
   and without a continuous output clamp. Headline: **the unclamped symmetric fight already
   resolves in ~9 ticks with essentially no decisive margin (avg final gap 9.9 bodies out of
   190) — no spontaneous snowball to prevent.** Adding a continuous ±15% output clamp *reduced*
   the average margin further (to 4.2) rather than producing a readable swing — it dampens
   toward tie, which is the opposite of a climax. This directly drove §3's redesign of the
   back-channel from "prevent a blowout" to "schedule a break," and directly grounds the "must
   never fake with a hidden multiplier" rule in §3.3.

**Not simulated:** the no-snowball guard's 70/30 threshold (§3.2) — flagged as an unmeasured
placeholder dial, same epistemic status as every other prototype number in this repo; the
tension-curve shape and the break-schedule's actual timing function (needs a playtest to feel
right, not a model); and frame cost at 380 total entities (reasoned from `GDD.md` §10's
measured 13,000–20,000-entity ceiling, not independently re-measured this session — the Build
scope table above asks for that confirmation as PIE evidence, not claimed here).

---

## Canon proposals

- **`SYSTEMS.md` §5** should get a pointer to this doc once reviewed: the Battleground
  back-channel is the first concrete instance of the pacing-director concept §5 names as
  undesigned, scoped down to one level. Worth mining when the full multi-front director is
  eventually built — not written into `SYSTEMS.md` by this doc per the file-write restriction.
- **`squad-group-system.md`'s `bRetinue`/`TeamBit`** is currently documented (in code comments,
  not in that spec's prose) as a "formed vs. brood" distinction. Once Battleground repurposes it
  as a generic two-team flag, that doc's own language should probably gain one clarifying
  sentence — flagged here, not edited, per this doc's own scope.

## Narrative requests (→ narrative-director)

- **Who is Army B, fictionally?** Mechanically it's a mirror of the player's own Spearmen/
  Archer roster — same stats, same formation, opposite side of the field. Current narrative
  canon (`FLAME-FOUNDATION.md`) names no factions yet, so this needs a hook: is this a
  training/proving-ground fight against a drilled mirror force (a "what if the dark had an
  army that fought like yours" idea), or a distinct hostile faction that happens to fight in
  ranks rather than as a tide? Either reading changes whether Army B should look like a dark
  mirror of the Liberated militia or something visually distinct.
- **The back-channel is invisible to the player by design (§3.3) — the *climax* it schedules is
  not.** A player-facing hook for "why did the enemy suddenly commit everything to one push"
  would sell the coordinated Charge as an enemy commander's decision rather than a scripted
  timer — a line of in-fiction reasoning (a banner signal, a horn, a visible officer figure who
  gives the order) would let the mechanic read as diegetic AI behavior instead of an obviously
  gamey beat. Gameplay readability need this must satisfy: the climax must be legible as "the
  enemy just decided something" within a couple of seconds, at 380-entity scale, without a UI
  callout — a physical tell (a raised banner, a horn cue) is the natural silhouette/audio
  answer and should travel into whatever art/audio brief follows.
