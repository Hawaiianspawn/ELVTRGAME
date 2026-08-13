# The one-camera bench — which renderer, and how many units

**Measured 2026-07-28**, standalone `-game` Development (not PIE, not the editor), single
client, `1920x1080` windowed, `t.MaxFPS 0`, `r.VSync 0`. Harness: `-SwarmBench` with the
config sweep added the same day (`SwarmRenderActor.h`'s `BenchmarkConfigs`), which measures
every renderer configuration across every entity count in one launch and writes
`Saved/SwarmBench.csv`. Retinue held at 100 throughout; brood swept 500 → 20,000.

Question this run exists to answer, from the owner: **as a one-camera game, is the camera the
rendered viewport or a full-screen "simulated" Unit Cam — and how many entities does each
stretch to?** Plus: what LOD, if any, is worth building.

Everything below supersedes the in-editor numbers in [BUDGETS.md](BUDGETS.md) for the debug-box
path (they agree closely — 136.8ms here vs 135.5ms there at 10,000 — which is the main reason to
trust this harness) and is the **first measured baseline the Niagara sprite path has ever had**.

---

## 1. The headline

**The GDD §10 gate is passed, with a 7x margin, and it is not close.**

| | 1,000 brood + 100 retinue | vs 16.6ms budget |
|---|---|---|
| Debug boxes (the path the gate was failing on) | 9.44ms — **106 fps** | fails at ~1,700 units |
| **Niagara sprites** | **2.31ms — 433 fps** | **7.2x headroom** |

And the second finding, which is the one that actually changes what to build next:

> **Rendering is free. 100% of the frame cost is the Mass sim on the game thread.**

Niagara's frame time sits on top of a sim-only baseline at every count measured, within
run-to-run noise:

| brood | SIM-ONLY (no renderer at all) | VIEWPORT-NIAGARA | renderer cost |
|---|---|---|---|
| 500 | 1.670ms | 1.753ms | +0.08 |
| 1,000 | 2.248ms | 2.309ms | +0.06 |
| 2,000 | 3.069ms | 3.064ms | −0.01 |
| 5,000 | 4.997ms | 5.019ms | +0.02 |
| 10,000 | 8.534ms | 8.320ms | −0.21 |
| 20,000 | 15.238ms | 15.898ms | +0.66 |

Two of those deltas are *negative*, which is the clearest possible statement that what is being
measured is noise rather than a renderer. The Niagara path is one emitter, one draw call, and
a CPU push loop whose cost disappears next to the simulation feeding it.

**So the entity ceiling is a simulation number, not a rendering number.** The sim costs a
near-linear **~0.75ms per 1,000 entities**, and crosses 16.6ms at roughly **21,000 entities**.
That is the real cap, and no renderer choice moves it.

---

## 2. Full results — run 1

`Saved/SwarmBench-run1.csv`. `frame` is wall-clock; `game`/`draw`/`gpu` are the engine's
thread timers. Read `frame ≈ game` everywhere except the debug-box rows as "game-thread bound".

| config | 500 | 1,000 | 2,000 | 5,000 | 10,000 | 20,000 |
|---|---|---|---|---|---|---|
| SIM-ONLY | 1.67 | 2.25 | 3.07 | 5.00 | 8.53 | 15.24 |
| VIEWPORT-BOX | 3.40 | 9.44 | 21.19 | 59.93 | 136.84 | 355.07 |
| **VIEWPORT-NIAGARA** | **1.75** | **2.31** | **3.06** | **5.02** | **8.32** | **15.90** |
| NIAGARA+VIEWCAM | 1.68 | 2.28 | 2.98 | 5.13 | 8.42 | 15.98 |

*(All values ms/frame. The two Unit Cam rows from run 1 are withheld deliberately — see §4.)*

### Niagara vs. debug boxes

| brood | box | niagara | speedup |
|---|---|---|---|
| 1,000 | 9.44 | 2.31 | **4.1x** |
| 5,000 | 59.93 | 5.02 | **11.9x** |
| 10,000 | 136.84 | 8.32 | **16.4x** |
| 20,000 | 355.07 | 15.90 | **22.3x** |

The gap widens with count because the debug renderer's cost *is* per-entity draw submission
(`DrawDebugSolidBox`, one immediate-mode primitive each), while Niagara's is one instanced
draw regardless. The debug-box renderer was never a renderer; it was a diagnostic, and the
project has been measuring its own frame budget against it for weeks.

### The minimap's second scene render is, surprisingly, also free

`NIAGARA+VIEWCAM` turns on `AViewCamCapture` — a genuine second full render of the world from
a second POV, at 640px, which the class's own header calls "the cost this class exists to pay".
It lands within ±0.11ms of `VIEWPORT-NIAGARA` at every count.

**Why, and the caveat that matters:** it is not that the second render is cheap, it is that it
lands on the **GPU**, and the GPU is nowhere near the constraint here — 4.1ms of GPU work
against a 15.9ms game-thread frame at 20,000 units. There is so much GPU headroom that a second
pass hides inside it entirely. This result will not survive a scene with real lighting, real
materials, or a GPU-bound target platform. **Do not read this as "the minimap is free forever."**
Read it as "the minimap is free while the game thread is the bottleneck," which is today, and
re-measure it the first time that stops being true. `Emberkeep.UI.ViewCam.Enable 0` was added in
this pass as the switch to price it again.

---

## 3. What this means for the light

The owner's standing offer was *"I am willing to simulate light if that means it's cheaper."*

**It doesn't need to be cheaper, because lighting is not costing anything measurable.** The
flame is already a simulated light in the sense that matters: a material-parameter-collection
position + falloff evaluated in the demichrome post-process, plus (on the debug path only) a
CPU-side per-unit attenuation. GPU across the whole sweep never exceeds 4.2ms.

So the answer is: **keep the light you have, spend nothing to fake it further.** There is no
frame time to win there. If the flame gets more expensive later — real shadow casting, a second
light, per-unit light buckets from `niagara-sprite-refactor.md` §3a — re-measure, because that
work lands on the GPU where the headroom currently is, not on the game thread where it isn't.

---

## 4. The Unit Cam question — run 1's numbers were wrong, and why

Run 1's `UNITCAM-FULL` / `UNITCAM+NIAGARA` rows are **not trustworthy and are excluded above.**
Stating this plainly rather than quietly dropping them, because the failure is instructive:

The bench needs the Unit Cam *off* for the viewport-only rows (the combat HUD auto-shows the
panel even during a benchmark run, so without a switch every renderer row would silently carry
a projector's cost). The first version of that switch set the widget to
`ESlateVisibility::Collapsed`. **A collapsed UMG widget is dropped from Slate's tick, so
`NativeTick` stopped running — and the code that would turn the panel back on lives in
`NativeTick`.** Once any config disabled it, no later config could re-enable it.

The tell was in the data: `UNITCAM+NIAGARA` came out *cheaper* than `UNITCAM-FULL` and landed
exactly on `VIEWPORT-NIAGARA`, which is impossible if it were drawing anything.

Fixed by zero-sizing the widget instead of collapsing it — it keeps ticking, so it can come
back, while giving Slate nothing to lay out or paint. Re-measured in run 2 (§5).

---

## 5. The LOD that was worth building — `Swarm.SimLOD`

Because rendering is free and the sim is everything (§1), the only LOD worth building is a
**simulation** LOD. Render-side culling would have optimised a cost that does not exist.

**What it does** (`SwarmProcessors.cpp`, `UBroodSteeringProcessor`): a brood further than
`Swarm.SimLOD.NearRadius` (default 1600uu) from the bearer re-steers only once every
`Swarm.SimLOD.Stride` frames instead of every frame. Steering is where the money is — two spatial
grid queries per brood per frame (`FindNearestEnemy` + `SeparationForce`). Integration is *not*
strided, so skipped units keep gliding on their last velocity and movement stays smooth; what
lags is only the *response* to a new neighbour, by up to N frames.

`NearRadius` 1600 is set against the shipped shot: the view is at most 2400uu wide
(`Emberkeep.Cam.ScaleWidthFull`, and it only narrows as the army dies), which puts the furthest
visible corner ~1,470uu from the bearer once `Emberkeep.Cam.HudBias` is counted. **At full army
the LOD only touches brood that are off-screen**, still walking in from the 2500-4000uu spawn ring.

**One honest exception**, since `Emberkeep.Cam.Scale` is on in the shipped tuning: below
`Cam.ScaleSwapAt` the camera swaps to a shallow perspective (pitch −22°), which sees further
down-field than any ortho width. A late-run shot *can* have strided units in frame. They are a
few pixels at that depth and a 67ms avoidance lag does not read there — but "off-screen only" is
true of the full-army top-down shot, not of every shot the game can produce.

### Measured (`Saved/SwarmBench-run2.csv`, ms/frame)

| brood | Stride 1 (off) | Stride 2 | Stride 4 | Stride 4 saving |
|---|---|---|---|---|
| 500 | 1.82 | 1.77 | 1.81 | −1% |
| 1,000 | 2.39 | 2.51 | 2.45 | +2% |
| 2,000 | 3.05 | 3.04 | 2.98 | −2% |
| 5,000 | 5.04 | 4.58 | 4.37 | **−13%** |
| 10,000 | 8.43 | 7.26 | 6.40 | **−24%** |
| 20,000 | 15.76 | 12.69 | 10.72 | **−32%** |

Monotonic in both stride and count, which is what a real effect looks like. The ±2% at 500-2,000
is noise, and it is noise *for a reason*: at low counts the whole wave has converged onto the
bearer by sample time, so almost nothing is outside `NearRadius` and there is nothing to skip.

**The measured saving is a lower bound.** The benchmark settles for 8s before sampling, and brood
march at 320uu/s from a 2500uu ring — so by the time the sample window opens, the front ranks
have arrived and only the deep back ranks are still far. A real fight with waves continuously
walking in from the dark has a permanently larger far-population than this benchmark does.

### What it buys

| | 60fps ceiling | at 20,000 |
|---|---|---|
| No LOD | ~21,000 entities | 15.76ms (63fps) |
| **Stride 4** | **~33,000 entities** | **10.72ms (93fps)** |

**Recommended default: `Swarm.SimLOD.Stride 4`.** It is a ~55% raise in the entity ceiling for a
behaviour change nothing on screen can see, because everything it touches is off-screen by
construction. Stride 2 is the conservative option if a wider camera is ever wanted without
re-tuning `NearRadius`.

**The one rule for it:** `NearRadius` must stay above the visible half-width. Zoom the camera out
past `Emberkeep.Cam.OrthoWidth` 3200 without raising `NearRadius` and the LOD starts striding
units the player can watch — which is where a 4-frame lag in collision response would begin to
read as units sliding through each other.

---

## 6. The one-camera answer: rendered viewport, not the Unit Cam

Measuring the Unit Cam honestly took three attempts, and the first two were wrong in ways worth
recording (§4 for the widget-lifetime bug; here for the framing one).

**The trap:** the projector looks free. In its normal configuration it tracks a sim-only baseline
at every count. But `Emberkeep.UnitCamProj.CountLog`, added in this pass, shows why:

| config | bodies on field | billboards actually drawn |
|---|---|---|
| Army View (`SelectedSquad -1`) | 20,000 | ~8 squad blocks |
| Per-body, `Range 2400`, 40° lens | 20,100 | **339 - 1,092** |
| Culls opened (`Range 20000`, 150° lens) | 20,100 | ~8,700 |

The shipping Unit Cam is cheap because it is a **close-up panel drawing about 5% of the field** —
range-culled, frustum-culled, and filtered to one squad's soldiers. That is exactly right for what
it is. It is not a measurement of "the Unit Cam as the game's camera", and reporting it as one
would have compared a renderer that draws everything against one that draws a twentieth.

### With the culls opened so it draws the battle (`UNITCAM-DRAWALL`, run 4)

| brood | SIM-ONLY | VIEWPORT-NIAGARA | UNITCAM drawing the field | cam − niagara |
|---|---|---|---|---|
| 500 | 1.68 | 1.75 | 1.63 | −0.12 |
| 1,000 | 2.38 | 2.31 | 2.37 | +0.06 |
| 2,000 | 3.02 | 3.06 | 3.31 | +0.25 |
| 5,000 | 5.00 | 5.02 | 5.89 | +0.87 |
| 10,000 | 8.58 | 8.32 | 10.59 | **+2.27** |
| 20,000 | 15.18 | 15.90 | 17.55 | +1.65 * |

\* understated — at 20,000 the frustum still culled it to ~8,700 drawn, so this row prices 8,700
billboards, not 20,000.

**Cost per unit actually drawn:**

| | per unit | lands on |
|---|---|---|
| Niagara sprites | **0.036 µs** | GPU (1.7-4.1ms used, huge headroom) |
| Unit Cam billboards | **0.229 µs** | **game thread (already the bottleneck)** |

**Niagara is ~6x cheaper per unit drawn, and it spends the abundant resource instead of the
scarce one.** That second half is the real argument and it does not depend on the exact ratio:
every billboard the projector draws is `FSlateDrawElement::MakeBox` plus a projection and a
depth sort on the game thread — the same thread that is 100% of the frame cost. Niagara's work is
one instanced draw on a GPU sitting at a quarter utilisation.

### 60fps ceilings

| camera | ceiling |
|---|---|
| Unit Cam, drawing the field | **~16,000 entities** |
| **Rendered viewport (Niagara)** | **~21,000 entities** |
| **Rendered viewport + `SimLOD.Stride 4`** | **~34,000 entities** |

**Recommendation: the one camera is the rendered viewport.** It is architecturally the wrong place
to put per-unit cost in a game that is game-thread bound, and it caps out ~24% lower than the
viewport before any LOD, and ~53% lower after.

### DECIDED, owner, 2026-07-28: one-camera mode is the default

`Emberkeep.UI.Cams` now defaults to **0**. The player's viewport is the only camera; the HUD band
becomes a single centred muster shelf. The Unit Cam is **disabled, not deleted** — every line of
`UUnitCamProjector` is intact and one CVar away.

What "disabled" means precisely: at 0 the second-camera machinery is never *built*, not merely
hidden. `RebuildBand` returns before constructing the cam, `SyncWingsToCam` early-outs on the null
cam, and `AViewCamCapture` is never spawned — so nothing projects, captures, or ticks.

Three things that had to be true for this to be a real mode rather than a preview, all now
verified on screen:

| Concern | Resolution |
|---|---|
| Do squads get lost with one panel instead of two wings? | No — `SetSquads` already pools the **whole** roster into the single panel when cams are off. |
| Does the company readout survive? | It does now. The centre column's company strip is built inside the cam branch, so the muster-only path had `SetShowCompany(false)` pointing at a strip that doesn't exist there — it would have silently dropped "Vanguard Company N/M · S squads" off the HUD. The panel carries its own readout on this path. |
| Does the camera's HUD bias still line up with a shorter band? | Yes, unchanged — `PublishHudOcclusion` **measures** the band's real height rather than assuming a cam-sized one. |

Two fixes the mode needed, both because the muster-only layout was written as a preview and had
never been the shipping HUD:

- **The shelf had nothing to size it.** Each muster panel is normally scaled to fit the cam
  standing beside it; with no cam the cards rendered at natural size, about 40% of the screen.
  Now bounded by `Emberkeep.UI.BandHeight` (default 190px, down-only scaling).
- **A `UScaleBox` reports its child's *unscaled* desired width**, so a box given only a height
  reserved the full-size panel's width while painting shrunk content, leaving dead ground inside
  the band frame. Both axes are now sized from the measured panel and the forced scale.

`Emberkeep.UI.Cams` is watched live in `NativeTick`, so flipping it rebuilds the band immediately.
That is deliberate: the auto-show runs on a next-tick timer that `-ExecCmds` and most startup paths
lose the race against, so a launch-time set would otherwise appear to do nothing. Verified both
directions.

---

## 7. What to do, in order

1. ~~**Set `Swarm.DebugRender 0`**~~ — **done.** Niagara is the source default. The debug-box
   renderer is 4-22x more expensive and is what every stale "we can't hit the gate" claim in this
   repo was measured against.
2. ~~**Set `Swarm.SimLOD.Stride 4`**~~ — **done.** +55% entity ceiling, invisible by construction (§5).
3. ~~**One-camera mode**~~ — **done, owner call 2026-07-28.** `Emberkeep.UI.Cams 0` (§6).
4. **Update GDD §10** — the gate is passed at 433fps, not failing. `BUDGETS.md` now carries a
   supersession header; the GDD is canon and is the owner's to change.
5. **Don't build render-side LOD.** Frustum-culling the Niagara push would optimise 0.036 µs/unit.
   The remaining lever is all sim-side.
6. **Next sim win, if the ceiling ever needs raising again:** `SeparationForce`'s `NeighborCap`
   early-returns from its lambda but `USwarmSubsystem::QueryNeighbors` keeps walking every entry
   in all 9 buckets — the cap bounds the *math*, not the *traversal*. Making the walk itself
   abortable is a contained change to the dominant pass. Not needed today.

## 8. Run 5 — viewport capability at the shipping config, and what the light costs

Measured 2026-07-28 after one-camera mode landed. All rows Niagara + one-camera.
`Saved/SwarmBench-run5-viewport.csv`. Counts extended to 40,000 so the ceiling is **measured
rather than extrapolated**.

| config | 1,000 | 5,000 | 10,000 | 20,000 | 30,000 | 40,000 |
|---|---|---|---|---|---|---|
| **SHIPPING** (LOD 4) | 2.40 | 4.50 | 6.52 | 10.74 | 14.73 | **19.06** |
| NO-LOD (stride 1) | 2.54 | 5.01 | 8.02 | 14.92 | 22.19 | 30.19 |
| NO-FLAME (`Swarm.Flame 0`) | 2.65 | 4.29 | 6.34 | 10.10 | 14.67 | 19.27 |
| PLAIN-VIEW (no post-process at all) | 2.53 | 4.21 | 6.17 | 10.12 | 14.44 | 18.95 |

**Ceiling confirmed: ~34,000 entities at 60fps** (30,000 → 14.73ms, 40,000 → 19.06ms). The
earlier extrapolation was right. 40,000 runs at 52fps.

**The sim LOD's saving grows with count** — +6% at 1,000, +39% at 20,000, **+58% at 40,000.**
Exactly as predicted: the further the population spreads, the more of it is outside `NearRadius`.

### The light costs nothing — this retires the "simulate light to save processing" idea

| turning off | mean frame delta |
|---|---|
| the flame lift (`Swarm.Flame 0`) | **−0.11ms** (one of six deltas is *positive* — this is noise) |
| the **entire** demichrome post-process | **−0.26ms** |

Out of a 19ms frame at 40,000, the whole styled look costs about **1.3%**. There is no
processing to win back here.

**And there is no real-time lighting on the units to switch off in the first place.** `M_Swarm`
is **Unlit + Masked** and samples `T_Swarm_2bit` directly — no light touches a sprite. The light
is *already* simulated: `lum = saturate(lum + atten * FlameIntensity)`, a screen-space additive
lift in the post-process, not a light source. GPU across the sweep never exceeds 5.5ms against a
19ms game thread.

### So why do the units blow out?

That same `saturate`. The lift is applied to the **whole screen** and then quantized to the four
values; near the core `atten` → 1, so any pixel already bright — and the sprites are Bone/Steel
dominant by design — clips to Demichrome Pale. Ground and units alike.

This is a known, already-documented gap, not a new bug: the debug renderer had `Swarm.BroodLightCeil`
holding units below full brightness at every distance, and
[niagara-sprite-refactor.md](niagara-sprite-refactor.md) §6 flagged that **the sprite path has no
equivalent, "since the post-process light-lift doesn't know about team."** It still doesn't.

**The fix is a ceiling, not an exemption — and the distinction matters.** Exempting units from the
lift entirely (the literal "turn the lighting off for units") would also stop them *fading in from
the dark*: `Swarm.BroodLightFloor` is 0 precisely so a distant brood draws black and resolves as it
walks into the light. That fade is the flame premise made visible. Removing it to fix the bright
end would trade a real designed effect for a clipping bug.

### BUILT 2026-07-28 — the spotlight no longer touches units

Owner call: *"spotlight should not affect the units or entities"*, and *"they read as normal
sprites and not washed out."* Done, and verified on screen — units standing inside the white core
now render as legible sprites instead of white blobs.

How it works, three pieces:

1. **`r.CustomDepth=3`** (Enabled with Stencil) in `DefaultEngine.ini`.
2. **`ASwarmRenderActor`** stamps `Swarm.UnitStencil` (default 1) onto the Niagara component via
   `SetRenderCustomDepth` / `SetCustomDepthStencilValue`, driven on change. **No Niagara asset
   edit was needed** — this is a component property.
3. **`M_PP_Demichrome`** gained a `UnitStencil` input fed by a `PPI_CustomStencil` SceneTexture,
   and **returns the authored colour immediately** for unit pixels — before the lift, the Bayer
   dither, and the thresholds.

Set `Swarm.UnitStencil 0` for an instant A/B against the old behaviour.

**Sprites are authored ON the palette already** — they never needed lighting to land on a legal
value, they needed to be left alone so they land on the one the artist picked.

### It took two passes, and the first one was only half right

Worth recording, because the second failure is not obvious and would cost someone an afternoon.

**Attempt 1 — skip only the lift.** Fixed the wash-out and immediately produced the opposite
failure: units read as near-black silhouettes. Cause: the *thresholds were tuned assuming the lift
was there*. `Threshold1` is 0.40, but Demichrome Steel (`#555568`) has luma **0.34** — so with the
lift removed, a Steel-bodied brood fell below the first threshold and quantized **down to Dark**.
The lift had been carrying it over the line.

**Attempt 2 — bypass the whole pass.** The quantizer exists to turn a continuous-tone world into
four values. A sprite is *already* four values. Running it through can only mis-bin it:

| | result |
|---|---|
| lift on, quantize on | clips to Pale — white blobs |
| lift off, quantize on | Steel 0.34 < Threshold1 0.40 → drops to Dark — black silhouettes |
| **both bypassed** | **the authored value, exactly** |

**Watch for:** units are now the only thing on screen that is *not* dithered or snapped to the
ramp. Any off-palette pixel in a sprite sheet will now render as-is instead of being quantized
into legality — so `/art-coverage`'s off-ramp check gets more load-bearing, not less.

**The known consequence, now live:** units no longer fade with distance, because the lift was the
only thing that was dimming them. That is expected and is the reason the owner framed it as
*"original colors first, then get a layer about distance from"* — the distance read moves onto the
sprite itself, where it can shift a unit along the 4-value ramp instead of washing it out. That
second layer is **not built yet**; it is `art`'s already-specced per-particle `ShiftAmount`
(`docs/art/brood-approach-rim.md`) and is the next piece of work.

**Not re-measured.** The stencil adds a custom-depth pass for the swarm — GPU-side, where there
was 14ms of headroom, so it is very unlikely to matter, but it is unmeasured and should not be
quoted as free.

## 9. Caveats, stated plainly

- **One machine, one scene, single client.** The level is nearly empty — no real lighting, no
  materials beyond the demichrome post, no other actors. The GPU headroom that makes both the
  minimap and Niagara look free is a property of *this scene*, and is the first thing that will
  change as the game fills in. Re-run then; the harness makes it a single command.
- **Frame-time only.** No memory, no load time, no hitching/percentile analysis — these are mean
  ms over a 5s window after an 8s settle, so a stutter that averages out is invisible here.
- **The LOD saving is a lower bound** (§5) and the 20,000 Unit Cam row is understated (§6).
- **Not a packaged build.** Standalone `-game` Development off editor binaries, which removes PIE
  and editor overhead but is not a cook. A packaged Shipping build would be faster still, so the
  ceilings above are conservative.

