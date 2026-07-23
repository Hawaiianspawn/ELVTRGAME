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

### Combat model — continuous attrition, not swings
Each unit counts enemies inside `MeleeRange` via the existing spatial grid and
bleeds `DPS × Attackers × dt`. No damage events, no cross-entity random-access
writes, so every combat pass stays chunk-local. `MaxAttackersPerUnit = 4` caps
how badly being surrounded hurts — that cap is the single most important dial in
the model, because it decides whether numbers or positioning wins a fight.

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
leash warning has no visual · dead units vanish with no death animation.
