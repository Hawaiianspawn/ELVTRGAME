# Camera Scale — handoff into the build session

**Written:** 2026-07-26 · Branch `flame-spotlight` (tagged `v0.1.0-spike1`) · Companion:
[CAMERA-SCALE.md](CAMERA-SCALE.md)

This document stands alone. It is written for a session that was not present when the camera work
was scoped, and it assumes only that you have read `CAMERA-SCALE.md`.

---

## 1. The prerequisite is cleared

`CAMERA-SCALE.md` §5 says, correctly, **do not retire `UUnitCamProjector` until the main window
actually renders sprites**, and records that as of 2026-07-26 the emitter drew zero particles. **That
is no longer true.** The Niagara sprite path renders.

**Root cause:** the `Swarm` emitter in `NS_Swarm` was running as **`SimTarget: GPUComputeSim`**.
Setting it to **`CPUSim`** made hundreds of sprites appear immediately. CPU sim is also the correct
target — the swarm is simulated entirely on the CPU and positions are pushed every frame via
`SetNiagaraArrayPosition`, so the GPU has nothing to simulate.

**The emitter graph was never at fault**, contrary to what `docs/perf/niagara-sprite-refactor.md` §2
and `docs/GATE1-FUN-PROTOTYPE.md` §3a both claimed. Read live through
`NiagaraToolsets.NiagaraToolset_System`, every suspected layer was already correct: spawn count
linked to `User.Count`, burst disabled, `Particles.Position`/`SubImageIndex` bound through
`Engine.ExecIndex`, renderer on `M_Swarm` at `SubImageSize 8×4`, emitter bounds `Fixed ±100000`, no
compile warnings, system properly assigned on `SwarmRenderActor_0`. Both docs now carry dated
corrections at the top of those sections.

**Evidence:** `ELVTR/Saved/Screenshots/WindowsEditor/SwarmDebugShot00021.png` (first successful
render) and `SwarmDebugShot00023.png` (re-run from the saved asset — cleaner, individual sprites
rather than clumps).

### What changed on disk

| File | State |
|---|---|
| `ELVTR/Content/Spike1/NS_Swarm.uasset` | **modified, saved, NOT committed.** `SimTarget: CPUSim`; ParticleUpdate `SetVariables` module disabled. |
| `ELVTR/Saved/SwarmExecOnPlay.txt` | gitignored. Restored to the owner's tuning values with `Swarm.DebugRender 0` appended. |
| `docs/perf/niagara-sprite-refactor.md` | correction box at §2; new §9 with post-fix measurements. |
| `docs/GATE1-FUN-PROTOTYPE.md` | §3a sprite-path status rewritten; old diagnosis folded into a `<details>` block. |

Commit `NS_Swarm.uasset` early — it is the only unversioned part of the fix, and it is LFS-tracked,
so `git checkout` on it silently restores the broken state.

---

## 2. What the measurement changed about the plan

Full table in `docs/perf/niagara-sprite-refactor.md` §9. The short version, retinue 100, frame ms:

| brood | debug-box (UnitShading 1) | **sprite path** |
|---|---|---|
| 1000 | 14.62 | **3.87** |
| 2000 | 40.66 | **7.21** |
| 5000 | 132.33 | **15.92** |

The 60fps ceiling moved from **~1,930 total entities to ~5,100**. More importantly the *shape*
changed: at 5000 brood, draw is 2.81ms inside a 15.92ms frame and `game` is 15.91ms. **The render
bridge no longer dominates; the game thread does.**

Two things follow that the camera session should know:

1. **`CAMERA-SCALE.md` §2's "do not propose this refactor as a performance win" still holds, and is
   now even more true.** Collapsing to one viewport was always an architecture argument, not a
   frame-time one. The frame-time problem that did exist has been fixed by other means.
2. **The next perf target is the combat pass, not rendering** — the uncapped per-neighbour
   `GetSafeNormal` + K-insertion-sort that `niagara-sprite-refactor.md` §1 deferred as "invisible
   behind a much bigger cost." It is no longer invisible. This is a separate track from the camera.

---

## 3. Emitter work deliberately left unfinished

**`InitializeParticle.Lifetime` is still the `0.5` placeholder, with
`InterpolatedSpawnMode: Interpolation`.** Roughly 30 particle generations therefore coexist, which is
real overdraw — the §9 numbers are already paying it. The intended model (`ELVTR/SETUP-EDITOR.md` §3)
is one generation per frame.

The finalisation is: set `InterpolatedSpawnMode` to `NoInterpolation`, then bring `Lifetime` down to
just over one frame. Both are known-safe values; the hazard is ordering, see §5.

This is a tuning refinement, not a blocker. The camera work can proceed on top of it as-is.

---

## 3a. Landed after this document was written — the first camera slice

The army-scale camera is **built**, in `SpikeHeroPawn.cpp`, behind `Emberkeep.Cam.Scale` (default
`0`, so nothing changes until it is switched on). Dials are in `Saved/SwarmExecOnPlay.txt` under a
`@tab Camera` section, so they appear on the breadboard.

- **§4.1 (driver) — answered by reuse.** The weighted total is copied from `UnitCamProjector.cpp`
  (retinue ×10, brood ×0.25, over `ScaleBodies`, shaped by `ScaleCurve`), so the two views cannot
  disagree about how much army there is. `ScaleBodies` is calibrated to
  `RetinueCap(120) × RetinueWeight(10) = 1200`: a full retinue on its own pins the scalar at 1 and
  yields the shipped top-down framing. **Raising it above the retinue's own weight makes the map
  view unreachable in normal play** — that was a real bug, caught by the owner, and it is why the
  CVar's help text spells the calibration out.
- **§4.4 (ortho → perspective) — much smaller than feared.** `TickCamera` already swapped projection
  live, and the HUD-bias code already derived vertical extent for both modes. So: no matrix
  blending. A hard cut at `ScaleSwapAt` with the FOV solved at the seam,
  `Fov = 2·atan(Width / (2·Dist))`, so both projections frame the same world width and only parallax
  changes across the cut.
- **§4.2 and §4.3 are now live dials, not paper decisions** — `ScaleStages` (0 continuous, N
  quantised) and `ScaleRatchet` (one-way vs reversible).

**Verified:** both ends of the axis, by screenshot — `SwarmDebugShot00026.png` (full army: ortho,
pitch -90, shipped framing) and `SwarmDebugShot00025.png` (forced to the alone end: genuinely
perspective, ground receding to a horizon).

**NOT verified, do not assume:** the **transition itself** has never been watched — both shots are
separate runs at fixed scalars, so the seam's real quality is unproven. `ScaleStages` and
`ScaleRatchet` are written but never exercised. Both need a real play session, not a screenshot.

**What this surfaced, and it is now the live blocker:** §4.5 is no longer theoretical. At the close
end the frame sits inside `Swarm.FlameRadius 900` and blows out to white, and `DitherWorldAnchor 1`
makes the world-anchored dither enormous on screen. The flame and the dither are both tuned to a
fixed zoom, and moving the camera is what breaks them. That — not projection — is the next problem.

**Landed 2026-07-26 (this session) — the flame-pool half of §4.5.** `Swarm.FlameScaleWithView`
(default 0 = world-fixed, today's behaviour) scales `FlameRadius`/`FlameCoreRadius` by the live view
width over `Swarm.FlameScaleReferenceWidth` (default 2400, the shipped `OrthoWidth`) when set to 1 —
same A/B-behind-a-CVar idiom as `ScaleStages`/`ScaleRatchet`, not a baked-in pick. Implementation in
`SwarmRenderActor.cpp`'s `TickFlame`, sharing a new `GetLiveViewWidthUU` helper with
`Swarm.DitherZoomCompensate` rather than a third copy of the ortho/perspective width math (that
construction now exists in three places total: this helper, `DitherZoomCompensate`'s old inline copy
now routed through it, and `SpikeHeroPawn::TickCamera`'s HUD-bias extent, which still has its own copy
since it lives in a different translation unit).

Verified by screenshot, camera hand-dialled to bypass the retinue-casualty dependency (the "alone"
end normally needs a dead retinue to reach; the same Width/Dist/Pitch triple reproduces it without
that): `SwarmDebugShot00034.png` (dial 0, close end, 700uu framing — reproduces the blowout exactly,
almost the entire frame is pure white) vs. `SwarmDebugShot00035.png` (dial 1, same framing — pool is
back to a bounded, falling-off shape with visible dark ground either side). Shipped framing (2400uu,
ortho, pitch -90) checked both ways too: `SwarmDebugShot00030.png`/`00032.png` (dial 0) and
`SwarmDebugShot00036.png` (dial 1) are visually identical — the compensation is a no-op at the
framing it's calibrated against, as designed.

Not yet decided: world-fixed vs. screen-proportional is still the owner's call (this CVar is the A/B,
not the answer). §4.4 (ortho -> perspective) and the dither's own zoom coupling are unaffected by
this change.

## 4. The camera agenda is unchanged

`CAMERA-SCALE.md` §4's six open questions are still the agenda and are still genuinely open. Nothing
learned during the emitter fix answers any of them. Two are worth re-flagging because the sprite path
now touches them:

- **§4.5, the flame pool.** `Swarm.FlameRadius` is 900uu against `OrthoWidth` 2400, and
  `Swarm.DitherWorldAnchor 1` anchors dither to world space — so changing framing changes both the
  pool's screen proportion and the dither density. Now that units are sprites lit by the flame's
  distance falloff, `niagara-sprite-refactor.md` §3b's **base dither-anchor fix** applies from the
  moment any sprite renders, which is now. That decision (object/sprite-UV anchoring vs. exempting
  sprites from the post-dither pass) is live, and the owner has previously leaned toward *exempt
  units from post-dither / object-anchor*.
- **§4.4, ortho → perspective.** Still the primary technical risk, still unaddressed.

`UUnitCamProjector` is now safe to retire *in principle* — but read `CAMERA-SCALE.md` §5's list of
what must not be lost first, especially the POD `FUnitCamBillboard` discipline and the close-up
shading model. Retiring it is not required to start.

---

## 5. Working notes for driving the editor

Earned in this session; each cost real time.

- **`unreal-mcp` serves on port 9000**, hosted *inside* the editor. Its tools only register with a
  Claude session if the editor was already running when that session started. If it missed the
  window, drive the server directly over HTTP: `initialize` → `notifications/initialized` →
  `tools/call`. Responses are SSE-framed (`data:` lines), not plain JSON. A working client is worth
  keeping around.
- **Asset edits made over MCP are in-memory only.** Save with
  `editor_toolset.toolsets.asset.AssetTools.save_assets` — but `asset_paths` by name **fails** for
  every path form tried ("Asset does not exist"). Pass `[]` to save all dirty assets, then verify by
  hashing the `.uasset` against the git/LFS oid.
- **Do not call `SetStackInputData` right after changing `SimTarget`.** It crashed the editor:
  `Assertion failed: UserPtrIdx < NumUserPtrs` (`VectorVM.h:347`), because the script had not
  recompiled. Change SimTarget → save → let it recompile → then edit inputs. The crash lost every
  unsaved change.
- **`Swarm.Flame 0` only *freezes* the light, it does not disable it** (see the CVar's own comment).
  For a genuinely flat diagnostic scene also set `Swarm.FlameShadows 0`; the large black quad in the
  main viewport is the 64-bin radial shadow buffer, **not** the swarm. It tracks lighting CVars and
  ignores Niagara changes entirely — do not read it as evidence about particles.
- **`Saved/SwarmExecOnPlay.txt` is the way to set CVars at BeginPlay** (MCP cannot set CVars —
  `SearchCVars` is read-only). Every non-comment line is exec'd by `ASwarmRenderActor`; later lines
  override earlier ones, so a temporary block appended at the end is a clean override. It is
  gitignored. Back it up before editing and restore it after.
- **`-SwarmBench` only arms from the command line or the actor's `bRunBenchmark`**, so benchmarking
  means a standalone `-game -SwarmBench` launch, not PIE.
- **Check for stray editors before building.** A second editor for an unrelated project was running
  throughout this session; historically a stray instance locks `UnrealEditor-ELVTR.dll` and produces
  a bogus "won't build."

---

## 6. Reading order for the next session

1. `CAMERA-SCALE.md` — the direction and the six open questions. Still current apart from §5's
   "emitter draws zero particles," which §1 above supersedes.
2. This document.
3. `docs/perf/niagara-sprite-refactor.md` §9 — measurements. **Skip §2 and §8.1**, they are corrected
   at the top but the body is stale.
4. `docs/RENDERING-LIGHTING.md` §4d — still describes what ships until the camera lands.
5. `docs/UNIT-CAM-HANDOFF.md` — the 2026-07-23 session that first got sprites rendering. Not cited by
   any of the three docs above, and it was right about things they got wrong.
