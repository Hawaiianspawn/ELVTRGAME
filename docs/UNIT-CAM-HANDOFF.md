# Unit Cam — session handoff (2026-07-23)

Investigation into the "unit cam" (the second in-frame camera that should show enemies
emerging from the dark). It went deep; this is the honest state so you can pick it up or
replace it with your own camera-emulation method. **Nothing here is committed.**

---

## TL;DR

- The unit cam **is** a real second camera (`AUnitPortraitStage` → `SceneCaptureComponent2D`
  → render target → `UEmberkeepCamFeed` panel). It renders; it just had nothing useful to show.
- Three hard walls, all **confirmed live** (PIE + screenshots), in priority order:
  1. **SceneCaptures can't see `DrawDebug` primitives.** The swarm's default renderer is debug
     boxes, which are viewport-only → invisible to any capture. Proven: filled the main view
     with the box-swarm, unit-cam panel stayed pure black.
  2. **So the Niagara sprite path must actually render** (real primitives). It was broken;
     I got it rendering *real art* but only partially (see below).
  3. **The capture films `SCS_FinalColorLDR`** (the demichrome 2-bit pass), so up close it
     collapses to a single flat value — flat **Pale** in the flame core (hero cam), flat
     **Dark** `(36,36,36)` in the darkness (unit cam). The 2-bit look that's right for the main
     view is useless for a close-up capture. **This is the core design fork.**

If your new method avoids `SceneCapture` of the final demichrome image, walls #1 and #3 may
not apply to you — worth weighing.

---

## What I changed

### 1. `ELVTR/Content/Spike1/NS_Swarm.uasset` — SAVED to disk (binary, uncommitted)

The Niagara array→sprite bridge was fine all along (`Particles.Position/SubImageIndex` ←
`User.Positions/SubImages` via `SelectPositionFromArray`/`SelectFloatFromArray`, indexed by
`Engine.ExecIndex`, Direct Set + Clamp). The break was the **spawn lifecycle**. Net diff from
git HEAD:

| Thing | HEAD (broken) | Now | Why |
|---|---|---|---|
| `SpawnPerFrame.Spawn Count` | `Max_Int` (garbage default) | `User.Count` (linked) | **The real fix.** Spawn one particle per live entity each frame. |
| `InitializeParticle.Lifetime` | `2.0` | `0.5` | Short lifetimes (0 / 0.02) render **nothing** — the emitter uses **interpolated spawn**, which ages a particle partway through its own spawn frame and kills it before it draws. `0.5` is a **placeholder** — it causes heavy overdraw (~30×Count/frame). |
| `ParticleUpdateScript` `SetVariables` | enabled | disabled | Redundant in the ephemeral model; with any frame-overlap it clamp-stacks the overflow onto the last array index. |

`SpawnBurst_Instantaneous` stays disabled (I toggled it during debugging; net unchanged).

**The minimal, clean fix is really just: `SpawnPerFrame.Spawn Count = User.Count` + a sane
positive lifetime.** Everything else was thrash.

**Still open on the sprite path:** only a handful of sprites draw even though `User.Count`≈72
reaches the component. Spawn-count → rendered-count is lossy — suspect GPU max-particle-count
or `SpawnPerFrame` per-frame semantics. And `Lifetime 0.5` must be finalized (best route:
**disable interpolated spawning** on the emitter, then a tiny lifetime renders exactly one
generation with no overdraw). This is gated behind non-default `Swarm.DebugRender 0`, so it
doesn't affect normal play (which uses debug boxes).

### 2. `ELVTR/Source/ELVTR/UI/UnitPortraitStage.cpp` — edited on disk, **Live-Coding compiled only (not a real build, uncommitted)**

Added 6 CVars and a block in `Tick()` that re-frames the capture **behind the unit, looking
forward (+X) into the dark**, every frame, so the shot is tunable live without a rebuild:

```
Emberkeep.UI.UnitCam.SubjectFwd  250   ; how far ahead the stand-in sits (local +X)
Emberkeep.UI.UnitCam.Dist        320   ; how far BEHIND the stand-in the camera sits
Emberkeep.UI.UnitCam.Height      150   ; camera height
Emberkeep.UI.UnitCam.Pitch       -12   ; look-down degrees
Emberkeep.UI.UnitCam.Side          0   ; over-the-shoulder offset
Emberkeep.UI.UnitCam.FOV          55
```

The header was **not** touched (kept it Live-Coding-safe — no UPROPERTY/layout change). The
hero pawn never rotates (`AddActorWorldOffset` only), so local +X is a fixed world direction;
aiming at the live enemy front would be a later pass.

### 3. No net change
- `Saved/SwarmExecOnPlay.txt` — restored to the original `/cvars` preset.
- Left a few test screenshots in `Saved/Screenshots/WindowsEditor/` (gitignored).

---

## Reasoning / logic worth keeping

- **Why the cam looked "broken" but wasn't:** the hero cam (identical pipeline) renders a
  bright picture, proving capture→panel works. The unit cam was black because it filmed dark,
  empty space with no real unit primitives in frame.
- **Why debug boxes will never work:** `DrawDebugSolidBox` draws into the world line-batcher /
  HUD pass, which SceneCaptures don't sample. Not tunable — it's architectural.
- **Why FinalColorLDR is the real problem:** the demichrome PPV quantizes to 4 values. A
  close-up capture sees mostly one region (all-lit or all-dark) → one flat value. Options to
  fix: give the capture its **own** `PostProcessSettings`/exposure, or use a capture source
  that shows the **emissive sprites on black** (units glow as they emerge — arguably the best
  "enemies from the dark" read), and/or make brood more emissive.

## If you go with your own camera-emulation method

Weigh it against these constraints:
- If it **doesn't** use `SceneCapture` of the final image, you dodge walls #1 and #3 entirely
  — likely more efficient and gives direct art control (e.g. compose the panel from the sim's
  positions/sprites yourself, or render a dedicated cheap view).
- Either way you still need the units as **real, visible things** in whatever you render. The
  Niagara fix (SpawnPerFrame = User.Count + sane lifetime) is **independent of the camera** and
  worth keeping regardless.

## Revert

- Camera code: `git checkout ELVTR/Source/ELVTR/UI/UnitPortraitStage.cpp` then rebuild.
- Niagara asset: `git checkout ELVTR/Content/Spike1/NS_Swarm.uasset` — **but** that restores the
  broken `Max_Int`/stacking state, so only do this if you also don't want the sprite fix. The
  sprite fix is worth keeping even if the camera approach changes.

## Environment notes
- unreal-mcp editor server is on **port 9000** (not 8000) — read from
  `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`, `ServerPortNumber`.
- `NiagaraToolsets.NiagaraToolset_System` + `EditorToolset.EditorAppToolset` (StartPIE /
  CaptureEditorImage) drove all the live inspection. `Swarm.DebugShotAfter N` writes a real
  game-viewport screenshot to `Saved/Screenshots/` — the honest way to see what renders.
