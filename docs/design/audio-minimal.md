# Audio — minimal legibility set (slice)

**What this is:** the spec for `docs/RTS-VERTICAL-SLICE.md`:120's still-unchecked line,
*"Audio minimal: hit/death, stance confirmations (readability tools, not polish)."* This is
a legibility document that happens to use sound — every cue below exists because a player
currently cannot tell two states apart by eye, not because a fight needs to feel good.
Music, ambience, mixing, and anything that would read as polish are out of scope on
purpose and are not addressed here.

**Extends:** `GDD.md` §4 (stances, leash rule, hero-relevance tension) and Design Law 6
(readable danger at 500 units — applied here to ears). Reads `docs/RTS-VERTICAL-SLICE.md`
§2 (leash tunables), `docs/GATE1-FUN-PROTOTYPE.md` §2/§3b (stance input model, the
swing/strike/flash combat model), `docs/design/scaling-curve.md` (locked floor
populations 250/450/700), and `docs/design/loot-v0.md` (the two new battle-drop types).
Cites `ELVTR/Source/ELVTR/Rendering/BloodSubsystem.{h,cpp}` (task-060) as the shipped
precedent for a global per-frame voice/burst cap, and
`ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp`'s `USwarmDeathProcessor` as the reason
a positional death cue cannot ship today.

**Does not touch:** `SYSTEMS.md`, `GDD.md`, any file under `ELVTR/Source` or
`ELVTR/Content`. This doc is the whole deliverable.

---

## 1. The cue set — what each one disambiguates

| Cue | Disambiguates | Trigger | Position |
|---|---|---|---|
| Hero-hit | hero personally took a blow *vs.* a nearby soldier did | hero's own strike-cadence lands a blow on the hero (`SpikeHeroPawn`'s hero-side of the swing model) | 2D — hero is always screen-center |
| Hero-critical | hero is in a normal fight *vs.* the run is one or two blows from ending | hero HP crosses a threshold (e.g. 25%) — a re-arming variant of Hero-hit, not a new voice | 2D |
| Retinue-strike-landed | a unit is merely engaged (windup/recover) *vs.* a blow just connected — the only landed-blow tell on screen today; a crisper reinforcing channel once the Niagara graph binding lands (§1) | `bStrikeFrame` (see §2 — **not** the render buffer's `HitFlashBit`) | 3D, gated to near-hero (§2) |
| Retinue-death (toll) | the line is thinning at a background rate *vs.* it just took a spike of losses | `USwarmDeathProcessor` destroying a Health≤0 entity, aggregated (§2) | **non-positional** — see §4 |
| Leash-warn | a held unit is holding fine *vs.* about to break and revert to Follow | `SwarmAnim::LeashWarnBit` sets at 80% of `LeashRadius` | 3D, at the unit |
| Leash-break | the army is obeying Hold *vs.* has quietly stopped obeying it | `FRetinueFollowFragment::bLeashBroken` transitions false→true | 3D, at the unit |
| Stance-Follow / Charge / Hold / Rally | the stance keypress registered *vs.* nothing happened | `USwarmSubsystem::SetStance` called (edge-triggered key press, §3) | 2D |
| Pickup — Unit Orb | a Soldier kill dropped a body for you *vs.* was just a kill | Unit Orb claimed (`loot-v0.md` §4) | 2D or tight 3D at claim point |
| Pickup — Kindling Ember | a kill dropped a heal burst *vs.* was just a kill | Kindling Ember claimed (`loot-v0.md` §5) | 2D or tight 3D at claim point |

Nine cue identities total (Hero-critical shares a voice with Hero-hit). That is the
"minimal" set — nothing here is a mood layer, and nothing plays without a specific state
transition behind it.

### Why hero-hit is not redundant with anything visual
The hero pawn is currently a bare engine cube standing in for un-built art
(`SpikeHeroPawn.cpp:258`, `:301-309`) with no hit reaction — no flash, no color change,
nothing. The only existing signal that the hero took damage is the HUD's `HERO HP` text
line (`SpikeHeroPawn.cpp:765-768`), which requires the player to be reading the HUD at the
exact moment it changes. Hero-hit is therefore not a confirmation layered on top of a
visual — for the main camera view, it is currently the **only** feedback channel for the
single most important stat in the game (hero death loses the run outright). Hero-critical
is the same signal re-armed at a low-HP threshold, because "hero took a blow" and "hero is
about to die" are different urgencies and Design Law 4 (hero relevance) makes the second
one worth a distinct sting, not a discovery made by reading a shrinking number.

### Why retinue-strike-landed is the only landed-blow tell on screen today
The state of this is more precise than either "no visual exists" or "a visual exists," and
it's worth getting exactly right because two people specced against this repo this week and
landed on both wrong versions before this one.

**The work is half-done, in C++.** `SwarmRenderActor.cpp` does push a per-particle `Colors`
array to the Niagara component every frame (`:1473`, `:1567-1568`), and a struck unit —
`HitFlashBit` set — is written into it as full `FLinearColor::White` with no distance
falloff (`:1508-1513`), the renderer's own comment there calling it "the tell that was lost
when the sheet dropped its hit cell." That logic is real and correct.

**But the array is unbound in the Niagara graph, and the code says so about itself.** The
comment directly above the push (`:1564-1566`) is the source, not an inference: *"Colors and
Sizes are new as of task-059. Pushing them is harmless before the emitter reads them — an
unbound User array is simply ignored — so the C++ half can land and be verified
independently of the graph edit."* The graph edit that would actually bind `User.Colors` to
`Particles.Color` is task-059's own stated remaining work — add the two User parameters
"matching the names the C++ already pushes... bound to `SelectColorFromArray`" — and
task-059 is **parked** before that edit was made. So the array is pushed into a parameter
nothing reads. **The white flash does not reach the screen today.**

That makes the original claim in this doc correct: retinue-strike-landed **is** currently the
only signal, at any scale, that a blow has actually connected — combat reads as two lines
leaning into each other, not an exchange of blows, exactly as first written. It becomes a
reinforcing, higher-precision channel — not the sole one — the moment task-059's graph edit
lands, for the reasons worth keeping on record: the flash will then compete for legibility
against every other simultaneously-white struck unit plus the blood spray sharing the same
`HitFlashBit` trigger (task-060), while audio shares none of that screen-pixel budget; and
the flash rides the whole `Swarm.HitFlashTime` window (0.10s, ~6 frames at 60fps) with no
per-entity edge-detection, so it reads as "white for a beat," where audio hooked to
`FSwarmStrikeFragment::bStrikeFrame` (§2) marks the true single-frame instant the blow
landed. Both readings are written here on purpose — sole-signal today, reinforcing-and-crisper
once 059 lands — rather than picking one, because which is true is a live fact about the
build, not a design decision this doc gets to make.

**Open, not resolved: the graph binding's live state is unverified in-editor as of
2026-07-29.** The repo evidence above (`:1564`'s own comment, task-059 parked before the
edit) is strong, but checking `NS_Swarm` directly would need the editor, and it is currently
held by a concurrent measurement run (task-060, mid-frame-time-capture) that shouldn't be
perturbed to answer this. If someone later confirms the binding is live, the cue becomes the
reinforcing channel rather than the sole one, and the voice-priority reasoning below (which
currently treats it as sole-signal) should be re-argued at that point, not silently kept.

**The lesson for the next reader isn't "trust the renderer over the header."** It's narrower
and more useful than that. `SwarmFragments.h:54-57`'s stated *reason* — "Niagara has no
per-particle colour array here" — is stale: the array exists now. Its *conclusion* — the hit
tell is not on the sprite path — is still correct, for a different reason than it originally
gave. Neither the header comment nor the C++ push, read alone, tells you what actually
reaches the screen; only the graph binding does, and that's the one thing in this chain that
has to be checked in the editor rather than read off a file.

### Why leash-warn/break are named gaps, not new ideas
Both `RTS-VERTICAL-SLICE.md` §2 ("Leash warning: at 80% of radius, held units flash/audio
tick before they break — breaking must never feel random") and `GATE1-FUN-PROTOTYPE.md` §4
("Does the leash break read clearly, or does the army feel like it disobeys? The warn bit
is set but nothing renders it yet") already identify this exact gap and already name audio
as part of the fix. This doc just completes it: `LeashWarnBit` and `bLeashBroken` are both
already computed every frame (`SwarmProcessors.cpp`) — no new sim state, purely a listener
on data that exists. This is the pairing most likely to fix GATE1's open "does the army
feel like it disobeys" question, because the risk that question names is specifically an
*audio-shaped* one: the player is not looking at the unit in question when it breaks.

### Why the pickups get a cue
`GDD.md` §3's own moment-to-moment loop names "collect drops" as a beat, and both battle
drops are automatic, no-menu pickups (`loot-v0.md` §1) — there is no confirmation screen,
no inventory ping, nothing. A player who is not staring at the exact pixel where a Soldier
died has no way to know a Unit Orb (a body) landed versus a Kindling Ember (a heal) versus
neither. Given `loot-v0.md` §7's own simulated volumes — Unit Orbs 0.2–6.3 per floor mean,
Kindling Embers 4–19 per floor mean, across a whole floor, not per second — this is
trivially cheap to voice discretely; see §2 for why it needs no pooling at all.

---

## 2. Density: voice limiting, distance culling, and pooling

The task brief's own framing is the right one to hold onto here: **700 units dying means
700 potential death cues**, and the same shape of problem exists for hits — `scaling-curve.md`
locks floor populations at 250 → 450 → 700, and `GATE1-FUN-PROTOTYPE.md`'s own shipped
defaults show wave 3 wiping a full 120-unit retinue (0 survivors). A discrete one-shot per
hit or per death is unshippable at that volume, and — this is the sharper point — it would
also be **unreadable** even if it were affordable: a wall of overlapping one-shots reads as
noise, not as "my line is thinning," which defeats the cue's own purpose (Design Law 6
demands a *legibility* answer, not just a *performance* one).

### Distance culling — reuse the leash radius, don't invent a new number
Combat and hit/death/leash audio is only evaluated for entities inside `LeashRadius`
(~2000uu, `RTS-VERTICAL-SLICE.md` §2) of the hero. This costs nothing new to compute — the
leash system already tests every unit against this radius every frame
(`FRetinueFollowFragment::bLeashBroken`) — and it is the correct legibility boundary, not
an arbitrary one: the leash rule already guarantees the retinue's home is the hero, so "near
the hero" is where the fight the player is actually fighting is happening. This alone cuts
most of a 700-strong floor population out of consideration, since anything outside leash
range is, by the game's own rule, not yet engaged with the player's army.

### Voice limiting — a global per-category budget with priority stealing
Even inside the leash radius, peak concurrent engagement is still large. A Fermi bound
(§5): at `Swarm.SwingInterval` = 0.9s (shipped default) and an order-of-magnitude estimate
of 150–250 units concurrently in contact at wave-3 peak, blows land at roughly
150–280/second army-wide. That is two to three orders of magnitude above any usable
concurrent-voice count. The fix is the same shape `BloodSubsystem` already shipped for the
visual half of this exact problem (`Blood.MaxBurstsPerFrame`, default **24**, "GLOBAL cap
... without this, cost scales with the kill/hit rate and has no ceiling"): a small, fixed
voice budget per cue category, with events beyond the cap in a given window simply not
sounding, rather than queuing (queuing would detach the sound from the moment it was
supposed to confirm).

Working values, deliberately far below Blood's 24 (a dropped particle burst is invisible in
a crowd; a dropped *sound* competing with five others playing at once is inaudible anyway,
so there is no reason to spend voices past the point of legibility):

| Dial | Working value | Reasoning |
|---|---|---|
| `Audio.Hit.MaxVoices` | 4 concurrent | Above ~4 simultaneous impact one-shots, a listener can no longer parse "a blow landed" as a distinct event — it reads as texture, which is exactly the ambience this doc is scoped to avoid, and that ceiling holds regardless of what else is true about the cue. Priority: Elite/Boss point-target hits > player-adjacent retinue hits > everything else inside the leash radius. **Because §1 finds retinue-strike-landed is currently the *only* landed-blow tell (not a reinforcing one — the Niagara graph binding that would make the visual flash reach the screen is unbound, task-059 parked), an event dropped by this cap is a real, uncompensated loss today, not a redundant copy silently absorbed by the flash.** The cap stays at 4 anyway — legibility, not affordability, is the binding constraint — but this is the reason Elite/Boss keeps top priority (rare, high-value, near-1-on-1 fights where losing the only tell matters most) over generic retinue hits, rather than a comfortable assumption that the flash covers the difference. Re-argue this ordering if the graph binding is later confirmed live (§1's open item). |
| `Audio.Hit.CellCooldown` | 150ms per `GridCellSize` (250uu) cell | Reuses the combat pass's own 3×3-cell neighbour scan (same idiom `loot-v0.md` §3 already reused from `feeding-distraction.md`) so one dense knot of fighting can't retrigger the same cell's sound every frame. |
| `Audio.Death.MaxVoices` | 2 concurrent | Death is rarer than hits per-unit but still spikes at wipe moments (GATE1 wave 3: full 120-unit wipe). Two voices is enough to signal "a death just happened" without the channel becoming a machine-gun. |
| `Audio.Death.AggregateWindow` | 250ms | Deaths inside one window collapse to a single trigger with a magnitude tag (1 vs. "several") rather than one sound per death — see §4 for why this is also forced by the position problem. |
| `Audio.Leash.MaxVoices` | 3 concurrent | Warn/break events are naturally bounded by geometry (only units near the leash boundary ring can trigger them at all), so this is a safety valve, not a tuned limit — mirrors the "neither is expected to bind" framing `loot-v0.md` §3 uses for its own despawn/population caps. |
| `Audio.Stance.MaxVoices` | uncapped | Player-triggered, edge-detected on key-down (§3): at most 1 trigger per keypress, physically bounded by input rate. No pooling needed. |
| `Audio.Pickup.MaxVoices` | uncapped | `loot-v0.md` §7's simulated volumes (low single digits to ~19 per floor, spread across a whole floor) never approach a density concern. No pooling needed. |

### Retinue-strike-landed specifically: the render-buffer trigger is the wrong hook
`BloodSubsystem` scans the swarm's already-published render arrays for `HitFlashBit` and
deliberately does **not** touch `SwarmSubsystem.h` or add a fragment — its own header
comment states why (no stable per-entity id in the render buffer to edge-detect a single
hit) and accepts the consequence plainly: "one hit can spray across several consecutive
frames while its flash is up." That consequence is tolerable for a particle burst (more red
pixels is not confusing) and would be actively harmful for audio: `HitFlashTime` defaults
to 0.10s, so at 60fps the same landed blow would retrigger a hit *sound* roughly six times
in a row, which does not read as "one blow" — it reads as a stutter, undermining the exact
disambiguation this cue exists to provide. Audio for this cue therefore needs a real
single-frame edge, which means hooking `FSwarmStrikeFragment::bStrikeFrame` (true for
exactly one frame per landed blow, per its own doc comment) rather than reading
`HitFlashBit` off the render buffer the way Blood does. That is a real scope difference
from the precedent this doc otherwise leans on, flagged here rather than glossed over:
**whoever implements this needs a read path into the Mass fragment layer that
`BloodSubsystem` explicitly avoided**, not a second render-buffer scan.

---

## 3. Stance confirmation — latency and identity

`ASpikeHeroPawn::TickStanceInput` polls stance keys with edge-detected `ConsumeKeyPress`
(rising edge only — a held key does not re-fire `SetStance`) and calls
`USwarmSubsystem::SetStance` directly on the same frame the key is read
(`SpikeHeroPawn.cpp:375-414`). There is no animation wind-up, no queued command, and no
network round-trip (single-player, no replication term) between input and the stance
actually changing. The audio confirmation should be hooked at the same call site and fire
on the same frame: **latency target is one frame (~16.6ms at 60fps), not "soon" or
"on the next beat."** This matters because the only other confirmation today is the HUD's
stance-name text (`EmberkeepHud.cpp:216-230`), which the player is not necessarily looking
at mid-fight — the entire point of a confirmation tone is that it must land at essentially
the same instant as the keypress, or it stops confirming and starts describing.

Four distinct one-shots (Follow/Charge/Hold/Rally), not one generic "stance changed" tone —
with only 4 possible states and at most one trigger per keypress, there is no density
reason to collapse them, and a single shared tone would re-introduce the exact ambiguity
("did it change, and to what?") the cue exists to remove.

---

## 4. What could not be spec'd — positional death is blocked by the sim

A positional retinue-death cue — one that plays *at the unit's location*, matching where
the player's eye is looking — **cannot be built on the current architecture.**
`USwarmDeathProcessor` (`SwarmCombatProcessors.cpp:511-517`) queries only
`FSwarmHealthFragment` (read-only) and `FSwarmAnimFragment` (read-only, "purely so deaths
can be bucketed by team for the fight log") before calling `DestroyEntity` — it never
requires or reads `FTransformFragment`. A dying unit's position is never captured at the
moment of death; by the time anything could look it up, the entity no longer exists. This
is the identical gap the blood particle subsystem already ran into and stated plainly
(`BloodSubsystem.h`: "Deaths bleed for free... a death is not a distinct, harder burst...
a real death-position burst needs a position captured at the moment of death, which the
sim does not publish"), and it is explicitly task-054's territory (persistent corpses),
which is proposed but not built.

**What this spec does instead, and why it's still buildable today:** Retinue-death is
non-positional (2D) and rate-aggregated (§2's `Audio.Death.AggregateWindow`). This is not a
downgrade improvised to route around the gap — it is arguably the *more correct* answer for
this specific cue regardless of position, because the thing a player needs to know is "the
line is thinning" (a rate), not "unit #4,281 died at this exact pixel" (a location the
crowd's own motion already obscures within a frame or two anyway). A player standing at the
hero, inside a formation of dozens to hundreds of units, cannot use positional death audio
for individual-unit tracking at this density even if it existed — the HUD's live retinue
count (`GATE1-FUN-PROTOTYPE.md` §1) already serves the precise, per-unit question. Once
task-054 lands and death positions exist, a positional variant becomes possible, but should
be judged against this doc's aggregate cue on its own legibility merits, not assumed to be
strictly better.

Two other things this doc could not spec, stated plainly:
- **The exact `Audio.Hit.MaxVoices` / `Audio.Death.MaxVoices` values are working numbers,
  not measured ones** (§5) — the per-floor encounter-budget table
  (`RTS-VERTICAL-SLICE.md` §4, still `[ ]`) that would give real concurrent-engagement and
  wave-duration figures does not exist yet. These dials should be re-checked once that
  table lands, the same handoff `scaling-curve.md` §6 already makes for its own ratio model.
- **Whether Elite/Boss point-target hits deserve their own always-on voice slot** (rather
  than sharing `Audio.Hit.MaxVoices` with priority) is a judgment call this doc leans toward
  (priority-first) but does not force — `scaling-curve.md` §1/§4 establishes these are rare,
  embedded, single/dual-instance fights, so the concurrency argument against a dedicated
  slot is weak, but nothing here blocks a future revision from giving them one.

---

## 5. Simulation notes

**Not a scripted simulation.** This doc sets voice-count ceilings and a stance-latency
target, not a curve to fit — the numbers needed are order-of-magnitude Fermi bounds checked
against already-shipped, already-measured quantities (`Swarm.SwingInterval` = 0.9s,
`Blood.MaxBurstsPerFrame` = 24, GATE1's wave-3 zero-input wipe, `loot-v0.md` §7's Monte
Carlo pickup volumes), not a new distribution worth a throwaway script.

**What was computed by hand, and its limits:**
- Peak hit rate: (estimated concurrent-engaged unit count) ÷ `SwingInterval`. The
  concurrent-engaged count (150–250) is an **unmeasured order-of-magnitude estimate**, not
  a simulated or profiled number — the encounter-budget table that would give a real
  concurrent-engagement figure is explicitly not built yet (`RTS-VERTICAL-SLICE.md` §4).
  The conclusion this doc draws from it (peak rate is orders of magnitude above any usable
  voice count, so a hard global cap is required regardless of the exact multiplier) holds
  even if the true concurrent count is off by 2×, which is why the doc treats it as
  sufficient for a threshold decision without needing a tighter number.
- Peak death rate is stated qualitatively, not numerically: GATE1's own zero-input wave 3
  wipes all 120 retinue, and floor 3's total population is 700, so cumulative deaths in a
  single wave run into the hundreds regardless of the exact wave duration — which is itself
  unmeasured (`RTS-VERTICAL-SLICE.md` §4's encounter-budget table would supply it). This
  doc did not compute a deaths/second figure because it would depend on that missing number
  and the qualitative conclusion (a discrete per-death cue is unshippable) doesn't need one.
- Pickup volumes are **not** re-derived here — `loot-v0.md` §7's own 20,000-run Monte Carlo
  is used directly (0.2–6.3 Unit Orbs/floor, 4–19 Kindling Embers/floor), which is why §2
  states plainly that pooling is unnecessary for either pickup cue.

**Assumptions carried forward, unverified:** the 150–250 concurrent-engagement estimate;
that `Audio.Hit.MaxVoices` = 4 and `Audio.Death.MaxVoices` = 2 are the right *feel*, not
just an affordable ceiling — that can only be judged by playing it, the same way
`GATE1-FUN-PROTOTYPE.md` §4's own open questions are stated as "play it and judge," not
settled by a number.
