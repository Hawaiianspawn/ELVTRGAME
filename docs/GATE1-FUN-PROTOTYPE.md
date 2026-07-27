# Gate 1 — Fun Prototype

**Built:** 2026-07-22 · **Answers:** `docs/RTS-VERTICAL-SLICE.md` §3 gate 1
> "Stances (with leash) feel good at 50–200 units; hero feels like a commander,
> not a camera."

Built on top of the Spike 1 swarm — same Mass Entity sim, same Niagara sprite
bridge, same map (`Content/Spike1/L_Spike1`). Spike 1 remains intact: the
benchmark harness and `Swarm.*` console commands still work.

---

## 1. How to play

PIE on `L_Spike1`, or launch standalone. No input assets — keys are polled
directly in `SpikeHeroPawn.cpp`.

| Key | Action |
|---|---|
| WASD | move the hero |
| 1 | **Follow** — formation rings around the hero, engage what comes close |
| 2 | **Charge** — surge at the **cursor** position, wide engage range, +25% speed |
| 3 | **Hold** — anchor the formation at the hero's current spot; short engage range |
| 4 | **Rally** — collapse tight onto the hero (45% slot radius), short engage range |
| R | restart the run |

Console: `Swarm.Stance Follow|Charge|Hold|Rally` does the same thing headlessly.

**Run structure:** 3s deploy → wave 1 (250 brood) → breather (6s, retinue
refills to 120) → wave 2 (450) → breather → wave 3 (700) → win. Hero death
loses. HUD shows phase, hero HP, live retinue/brood counts, current stance, and
how many units are currently leash-broken.

---

## 2. What was added

| File | Role |
|---|---|
| `Mass/SwarmCombat.h` | **new** — health fragment, `ESwarmStance`, all combat + leash tunables |
| `Mass/SwarmCombatProcessors.{h,cpp}` | **new** — melee attrition pass + death pass |
| `Mass/SwarmSpawn.h` | **new** — shared spawn entry points (impl in `SwarmCommands.cpp`) |
| `Mass/SwarmDebug.{h,cpp}` | **new** — nearest-neighbour spacing measurement + `Swarm.SpacingReport` |
| `Rendering/SwarmRenderActor.{h,cpp}` | debug-box renderer that bypasses Niagara (§3a) |
| `Mass/SwarmSubsystem.h` | grid entries carry team; stance/anchor state; hero HP; live counts |
| `Mass/SwarmProcessors.cpp` | stance-aware retinue steering + leash; brood target nearest retinue |
| `Spike/Spike1GameMode.{h,cpp}` | wave director, reinforcements, win/lose |
| `Spike/SpikeHeroPawn.{h,cpp}` | stance input, hero damage, prototype HUD |

Processor chain is now:
`GridBuild → BroodSteering / RetinueFollow → Combat → Integrate → Death / Contact`

### Combat model — discrete blows, victim-pull
**Amended 2026-07-25 (§3b).** Originally a continuous bleed of `DPS × Attackers × dt`.
It is now a discrete swing cadence: each unit runs its own swing clock and a blow
removes `DPS × SwingInterval` at once, so average throughput is unchanged but the
damage arrives in visible steps. Every unit still counts *its own* attackers via the
spatial grid and damages *itself* — there are still no cross-entity random-access
writes, and every combat pass stays chunk-local.

`MaxAttackersPerUnit = 4` caps how badly being surrounded hurts — the single most
important dial in the model, because it decides whether numbers or positioning wins
a fight. It caps **bodies allowed to engage**, not blows landing on a given frame;
see §3b for why that distinction is load-bearing.

### Leash — `docs/RTS-VERTICAL-SLICE.md` §2, implemented as specced
Per-unit latch in `FRetinueFollowFragment::bLeashBroken`. Breaks past
`Radius` (2000uu), re-anchors only inside `Radius × 0.85` so a unit sitting on
the boundary can't flicker. A broken unit ignores the global stance and behaves
as **Follow** until it is back inside. `LeashWarnBit` is set at 80% of radius —
available to the sprite/HUD layer so breaking never feels random. The HUD
reports the live broken count.

---

## 3. Measured baseline — **zero input**

Three standalone runs, hero never moved and never issued a stance. This is the
floor the design sits on: what the sim does with no player at all.

| Run | Wave 1 survivors | Wave 2 survivors | Result |
|---|---|---|---|
| 1 | 103 / 120 | 4 | LOST wave 3 — 13 brood left |
| 2 | 100 / 120 | 7 | LOST wave 3 — 4 brood left |
| 3 | 97 / 120 | 12 | LOST wave 3 — 8 brood left |

**The run is lost by default and lost narrowly** — within 4–13 brood of a win,
consistently. That is the intended calibration: everything a player adds
(repositioning, Rally before a flank lands, Hold on a chokepoint, 55 hero DPS
aimed somewhere useful) is the margin between loss and victory. Low variance
across runs, so it is a usable A/B harness for tuning.

Reproduce:
```powershell
# ~95s per run; grep the standalone log
Select-String -Path ELVTR\Saved\Logs\ELVTR.log -Pattern 'Run:'
```

### Tuning history (why the numbers are what they are)
1. **Flat `+40` reinforcements per wave** → army death-spiralled, wave 3 was
   unwinnable before it started. Replaced with **refill to a cap** (120), so
   losses cost you the wave, not the run.
2. **Retinue at 100 HP / 22 DPS** → traded ~1:1 with brood, so raw numbers
   decided every fight and stances were irrelevant. Raised to **130 HP / 30 DPS**:
   a soldier is now worth several brood, and *being surrounded* is what kills it
   rather than being outnumbered in aggregate. That is the distinction the
   stances exist to control.
3. **Hero at 120 DPS** → soloed the tail of wave 3 after the army was dead, i.e.
   exactly the "hero relevance" failure GDD §4 warns about. Cut to **55 DPS**.
   The hero is tanky (500 HP, max 8 attackers) but cannot replace the line.

---

## 3a. Debug view (added 2026-07-22)

The hand-built `NS_Swarm` Niagara asset renders the whole army stacked on one
point — every particle reads `Positions[0]`, so its per-particle array lookup is
not bound to `Engine.ExecIndex` (see `SETUP-EDITOR.md` §3.4). The C++ bridge is
correct: `SwarmRenderActor.cpp` pushes the full array and `Count` every tick.

Rather than debug the Niagara graph blind, there is now a renderer that bypasses
it entirely — **on by default** until the sprite pipeline is verified.

| Console var | Default | Effect |
|---|---|---|
| `Swarm.DebugRender` | `1` | Solid debug boxes per entity — white retinue, dark red brood. `0` returns to Niagara sprites. |
| `Swarm.DebugPlainView` | `0` | `1` disables post-process materials (drops the demichrome dither). Off by default — judge the game as it looks. |
| `Swarm.SpacingReport` | — | One-shot nearest-neighbour spacing stats per team. |
| `Swarm.SpacingLogInterval` | `0` | Seconds between automatic spacing reports. |
| `Swarm.DebugShotAfter` | `0` | Screenshot N seconds after BeginPlay — lets a scripted run capture what is actually drawn. |

Only one renderer is ever active; the Niagara component is hidden while debug
render is on, so a broken sprite setup can't be mistaken for a broken sim.

### Sprite path status 2026-07-26 — **FIXED. The emitter graph was not the fault.**

The cause was the `Swarm` emitter's **`SimTarget` being `GPUComputeSim`**; switching it to
**`CPUSim`** made the sprites render. The module stack, the `Engine.ExecIndex` array bindings, the
renderer properties and the Sub UV were all already correct.

Two claims in the superseded section below are wrong and should not be repeated:
- *"Fixing it needs a human in the Niagara editor: UE 5.8's Python exposes no emitter members at
  all."* — false. `NiagaraToolsets.NiagaraToolset_System` reads **and writes** the emitter/module
  stack from a live editor session (this is how the fix was applied).
- *"`NS_Swarm` Sub UV verified **4×2** on disk"* — it is **8×4**, matching `SwarmSheet::Columns = 8`
  / `Rows = 4` in `ELVTR/Source/ELVTR/Mass/SwarmFragments.h`.

`Swarm.DebugRender` can now be set to `0`. Measurements: `docs/perf/niagara-sprite-refactor.md` §9.

<details><summary>Superseded 2026-07-25 diagnosis, kept for the record</summary>

### Sprite path status 2026-07-25 — **fault isolated to the NS_Swarm emitter graph**

`Swarm.DebugRender 0` currently renders **nothing**: mid-combat with 219 units alive
(`RETINUE 104 / BROOD 115`) the world view showed only the flame pool — zero sprites.
Every other layer was eliminated by measurement, so the emitter graph is the only
remaining candidate:

| Layer | Evidence | Verdict |
|---|---|---|
| Mass sim + render buffers | `SwarmSpacing: 120 distinct retinue positions of 120` | good |
| C++ → Niagara bridge | pushes `Positions`/`SubImages`/`Count` every tick | good |
| Component visibility | `SwarmDebug: debugRender=0 niagaraVisible=1` | good |
| `T_Swarm_2bit` texture | imported, read-back `[OK]`, 4 palette values | good |
| `NS_Swarm` Sub UV | verified **4×2 on disk** in a fresh process | good |
| Unit Cam drawing the same texture | renders units correctly (it bypasses Niagara) | good |
| **`NS_Swarm` emitter graph** | — | **the fault** |

No Niagara errors are logged; the emitter simply produces no visible particles. This is
the §3a problem above, still live — the spawn lifecycle and/or the `Engine.ExecIndex`
array binding. `Swarm.DebugRender` is therefore **left at 1**, with the reason recorded in
`Saved/SwarmExecOnPlay.txt` so nobody flips it and concludes the atlas is broken.

Fixing it needs a human in the Niagara editor: UE 5.8's Python exposes **no emitter
members at all** on `unreal.NiagaraSystem`, so the module stack cannot be inspected or
edited from script (renderer *properties* can be reached by subobject name — see the
`sprite` skill — but the graph cannot).

</details>

### Two UE gotchas worth remembering
- **Instanced static meshes were the first attempt and silently failed.** Every
  stock engine material lacks `bUsedWithInstancedStaticMeshes`, so UE substitutes
  the *default lit* material and logs only a warning — units rendered black in a
  near-black level. Fixing it properly needs a project-authored material asset.
- **`DrawDebugPoint` draws nothing in a `-game` session.** `DrawDebugSolidBox`
  works in both editor and game. Verified by screenshot, not assumed.

### Separation is confirmed — measured, not eyeballed
`Swarm.SpacingReport` reads the same buffers handed to the renderer, so it
measures the sim independently of what is on screen. Every report showed **120 of
120 retinue at distinct positions** — they never stack.

| State | min | median | max |
|---|---|---|---|
| Formation (at rest) | 84uu | **86uu** | 86uu |
| Mid-combat | 25–39uu | 43–51uu | 343uu |

86uu at rest is exactly correct: ring `r` holds `8r` slots at radius `110r`, so
arc spacing is `2π·110/8 = 86uu`. Units compress to ~45uu when engaged and
recover. **No collision is involved** — spacing is entirely the steering
separation force in `SwarmProcessors.cpp` (`SeparationRadius = 60uu`, weight 1.4,
capped at 6 neighbours), which is why it costs nothing and never jitters.

## 3b. Hit reactions — swing cadence, flash, knockback (added 2026-07-25)

The continuous model answered the balance question but had **no impact moment**: HP
slid downward and units vanished, so there was nothing for an attack pose, a flash, or
a shove to belong to. Fighting read as proximity, not as blows.

### The mechanism
Each unit runs its own swing clock in `FSwarmStrikeFragment` (`Mass/SwarmFragments.h`),
advancing only while something is in reach:

```
windup ──────► STRIKE ──────► recover ──► (wrap)
0            0.35×interval              interval
             blow lands here
```

The strike is **edge-triggered**, so a blow lands exactly once regardless of frame rate.
Clocks are desynchronised at spawn from the existing `FSwarmJitterFragment::Phase`, or
a whole wave would strike on the same frame and read as one pulsing organism.

**How this stays victim-pull.** `FGridEntry` gained a single `bStriking` bool. Grid build
publishes it at the top of the frame; the combat pass reads it while walking the
neighbourhood it already walked. A victim can therefore tell which of its neighbours are
*connecting right now* and apply its own damage. No pass writes to another entity, so the
property that made the original model cheap is intact — the cost is one extra bool test
per neighbour entry.

**Knockback is a separate channel.** Steering *overwrites* `FMassVelocityFragment` every
frame, so an impulse stored there is gone before it moves anything. `Impulse` lives in the
strike fragment and is added on top of steering velocity by the integrate pass, decaying
exponentially with time constant `KnockbackTime` — which makes total displacement exactly
`KnockbackDistance` and lets repeated hits stack additively with no special-casing.

**The lunge is applied to the published render position, not the transform.** A pose must
not let a unit reach further or shove a neighbour, and it keeps the renderers dumb — they
have no idea what a unit is fighting and don't need to.

### The one thing that is easy to get wrong
`MaxAttackersPerUnit` had to move from capping *blows* to capping *bodies*. Blows are now
spread across frames, so capping simultaneous strikers caps almost nothing — a mob of ten
would land ten blows per interval and kill as fast as ten, silently deleting the most
important dial in the model. Capping bodies preserves the old guarantee exactly: incoming
damage is still at most `MaxAttackersPerUnit × enemy DPS`. Same reasoning applies to
`MaxHeroAttackers = 8`.

The hero runs the same cadence from `SpikeHeroPawn` (it is a pawn, not an entity), so the
one attack the player actually feels lands like everyone else's.

### Fixed along the way
`AttackBit` was self-perpetuating — `UpdateAnimBits` re-derived it from its own previous
value, so a unit that engaged once read as attacking forever. Invisible while nothing
decoded the bit; it would have left veterans swinging at nothing. Now consumed at the end
of the integrate pass and re-observed each frame.

### Rendering
Debug boxes only, this pass. A struck unit draws as **one solid white full-size box** —
no directional split, no distance falloff. The collapse of the two-tone shading is what
sells it, and it is the only thing that works on retinue, who are already near-white. The
flash is deliberately **light-exempt**: attenuated by `UnitLightFloor` it would quantise
below `Threshold3` and be invisible at the edge of the pool, which is exactly where
fighting starts. Same class of palette exception as the flame's white core. The unit-cam
close-up (`UI/UnitCamProjector.cpp`) flashes too.

`SwingBit` and `HitFlashBit` (anim bits 5 and 6) are written every frame and already reach
the Niagara bridge. Decoding them is a wider `SubImage` index over a wider `T_Swarm_2bit`
— an attack frame per team — per `docs/RENDERING-LIGHTING.md` §4a. **No sim change needed
when that sheet exists.**

### Tuning

| Console var | Default | Effect |
|---|---|---|
| `Swarm.SwingInterval` | `0.9` | Seconds between blows. One blow = DPS × this. Very short values collapse back toward the old continuous bleed. |
| `Swarm.SwingStrikeAt` | `0.35` | Fraction of the interval where the blow lands. Before it is windup (the tell), after is recovery. |
| `Swarm.SwingLunge` | `12` | uu a unit leans toward its target while swinging. Cosmetic — render position only. |
| `Swarm.HitFlashTime` | `0.10` | Seconds a struck unit flashes white. |
| `Swarm.KnockbackDistance` | `35` | **uu a struck unit is shoved.** `0` disables. |
| `Swarm.KnockbackTime` | `0.10` | Seconds to spend that distance. Shorter = sharper pop for the same distance. |

`KnockbackDistance = 35` is sized against the measured spacing in §3a (86uu at rest, ~45uu
compressed, `MeleeRange` 95uu): big enough to see, small enough that the line does not blow
apart and the shoved unit can close again.

### Balance status — **MEASURED 2026-07-25, and the model changed twice as a result**

My original claim here — that average DPS was unchanged and `SwingInterval` only affected
how damage was *parcelled* — **was false, and measurement caught it.** Wave-1 retinue
survivors (zero input, 3 runs each) are the cleanest metric:

| Model | `SwingInterval` | Wave 1 survivors | Outcome |
|---|---|---|---|
| §3 baseline, continuous | — | **97–103** / 120 | lost wave 3, 4–13 brood left |
| discrete, attacker-count cap | 0.05 | 119–120 | lost wave **2** |
| discrete, attacker-count cap | 0.9 | **60–62** | lost wave 3, **167–171** brood left |
| geometric targeting, K=1 | 0.9 | 68–92 | lost wave 2, 260–301 brood left |
| geometric targeting, K=4 | 0.9 | 73–80 | lost wave 2, 211–234 brood left |

Knockback was A/B'd out and exonerated (126–204 brood left with it disabled — same order).

**Bug 1: `MaxAttackersPerUnit` never bounded a damage rate.** It capped the first N enemies
in *grid iteration order*, and the grid is rebuilt every frame as units move, so that set
churned — over one swing interval far more than N attackers each landed a blow, and the
error grew with the interval. That is the whole 97→60 regression. Capping *bodies* instead
of *blows* did not fix it, because the bodies were never the same bodies twice.

**Fix: geometry, not counters.** Each attacker publishes the squared distance to its
**Kth-nearest** enemy (`Swarm.TargetsPerHit`, default 1). A victim takes a blow if it is
inside that radius — one compare against a distance it already has. Exact, cheap, no
hitboxes, no cross-entity writes, and a striker delivers exactly K blows so total damage is
conserved. `MaxAttackersPerUnit` survives only as a per-frame safety clamp, and
`MaxHeroAttackers` is gone (it had the identical flaw).

**Bug 2, the one that still needs an owner call: the old model was implicitly
infinite-cleave.** Every unit dealt full DPS to *every* enemy in range at once, so a
retinue with 4 brood on it was doing 120 DPS, not 30. Attacker output was unbounded while
victim intake was capped at 4 — and that asymmetry is *precisely* why 120 retinue could
nearly beat 700 brood. Geometric targeting bounds output and lets geometry bound intake,
reversing the asymmetry against whoever is outnumbered. Raising K lifts both teams equally,
so it cannot restore the old point.

**Resolution (owner, 2026-07-25): option 2 — `TargetsPerHit` is now per-team.**
`Swarm.RetinueTargetsPerHit` (cleave, and the powerup dial) and
`Swarm.BroodTargetsPerHit` (1 — a brood commits to one target).

### The tuning sweep, and the structural thing it found

| Retinue K | RetinueDPS | BroodDPS | Wave 1 survivors | Outcome |
|---|---|---|---|---|
| — (baseline §3) | 30 | 14 | 97–103 | lost wave 3, **4–13** brood |
| 2 | 30 | 14 | 116–119 | lost wave 3, 182–359 brood |
| 3 | 30 | 14 | 119–120 | lost wave 3, 194–237 brood |
| 4 | 30 | 14 | **120** | **WON**, 9–21 retinue alive |
| 4 | 22 | 14 | 117–120 | lost wave 3, 199–312 brood |
| **8** | **30** | **35** | **100–110** | lost wave 3, 119–201 brood |

**A constant K removes a stabiliser the old model had by accident.** When output scales
with how surrounded you are, a shrinking force hits proportionally harder — negative
feedback that keeps runs landing near a knife edge, which is exactly why §3 could report a
4–13 brood margin three times running. Cap cleave at a constant and that feedback is gone,
so attrition runs away and fights go **bimodal**: the line either holds easily or
collapses. K=3 losing with ~220 brood while K=4 wins outright is one integer step across
that tipping point, and DPS 22→30 at K=4 does the same thing. Neither is a dial you can
calibrate a knife edge with.

Hence the defaults: **`RetinueTargetsPerHit` 8** — "swings at everyone adjacent", which
restores the scaling stabiliser deliberately instead of accidentally — and **`BroodDPS`
raised 14 → 35**, because 14 was tuned when a brood implicitly hit two or three soldiers at
once and a single-target brood at 14 was harmless (zero wave-1 losses).

### Shipped defaults, measured (4 zero-input runs, 2026-07-25)

| | Wave 1 survivors | Wave 2 survivors | Result |
|---|---|---|---|
| §3 baseline (old model) | 97–103 / 120 | 4–12 | lost wave 3, **4–13** brood |
| **shipped now** | **109–111** / 120 | 16–24 | **lost wave 3, 131–145 brood** |

**Lost by default in every run, with tight variance** — the property §3 depends on, and the
early-game shape is back. The remaining gap is the late game: the margin is 131–145 brood
rather than 4–13, so a player has more to claw back than the original calibration intended.
Closing it is another pass on `RetinueDPS` / `RetinueMaxHP` or the wave-3 count, and it
should be judged by feel as much as by the number now the model underneath has changed.

**Gotcha that cost a measurement here:** `Saved/SwarmExecOnPlay.txt` **overrides C++ CVar
defaults** at BeginPlay — that is its job. Changing a default in code and re-running proves
nothing until the exec file is updated too. Three runs were read as "the new defaults win
comfortably" when they were still running `BroodDPS 14`; the tell was wave-1 survivors at
119–120 instead of ~110. Change both, or change the exec file alone.

## 4. Open questions this prototype exists to answer

Play it and judge — these are *not* settled by the numbers above:

- [ ] Does **Hold** feel like a real tool at `LeashRadius = 2000uu`, or is the
      leash so tight that anchoring a choke is pointless? (Flagged as a
      first-class risk in vertical-slice §7.3.)
- [ ] Does the leash *break* read clearly, or does the army feel like it
      disobeys? The warn bit is set but nothing renders it yet.
- [ ] Is **Charge** distinct from Follow in practice, or does auto-engage make
      them the same order?
- [ ] Does the hero feel like a commander at 55 DPS — or like a camera with a
      health bar? (GDD §4 "hero relevance" tension, the reason the DPS was cut.)
- [ ] Is 120 retinue the right count to *read* on screen at this camera height?

## 5. Known gaps (deliberate)

Single player only · no ranged units, elites, or boss · no loot or abilities ·
no unit tiers — one retinue type, one brood type · brood spawn as a ring around
the hero rather than from arena entrances · win/lose is a HUD line, not a screen ·
leash warning has no visual · dead units vanish with no death animation · the attack
pose exists as a lunge on a debug box, not yet as a sprite frame (§3b).
