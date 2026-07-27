# Camera Scale — perspective as the measure of your army

**Created:** 2026-07-26 · Status: **direction, not yet built** · Companion: `docs/perf/niagara-sprite-refactor.md`, `docs/RENDERING-LIGHTING.md` §4d

---

## 1. The premise

> The camera tells you whether you are an army or a man.

Command hundreds and the view is a wide battlefield — you read formations, fronts, the shape of the
tide. Lose them and the camera descends toward the one body left, until you are playing a character
who is alone in the dark holding the only flame.

This is not a UI affordance or a zoom convenience. It is the fiction, expressed as projection:
attrition is the core of the game, and the camera is what makes attrition *felt* rather than read
off a counter. The same run should physically change genre as it goes wrong.

---

## 2. What exists today, honestly

This section matters more than the target, because the current state is not what it looks like.

**The main window is already a map.** `Emberkeep.Cam.Ortho 1`, `Pitch -90` (straight down),
`OrthoWidth 2400`, centred on the hero with `HudBias` shifting him into the unobstructed strip. It is
geometrically an overhead orthographic slice — there is no perspective camera in the world at all.

**The units in it are not art.** They are `DrawDebugSolidBox` calls — immediate-mode debug
primitives, `BodyHeight 0.5`, flat slabs seen from directly above. Untextured. One box per unit
(two before `Swarm.UnitShading` defaulted to 0).

**Everything that reads as style is post-processing.** `M_PP_Demichrome` quantises the frame to the
locked 4-value ramp with a Bayer dither; `MPC_Flame` drives the spotlight falloff from the hero's
position. The boxes contribute shape; the look is applied on top of them.

**The Unit Cam panel is the only place the actual pixel art appears.** `UUnitCamProjector` draws
`T_Swarm_2bit` atlas cells as billboards, projected by pure maths (a virtual perspective camera
behind the hero, 1/depth forced-perspective scaling) over the same live Mass buffers the Niagara
bridge reads. No scene capture. Real per-unit frames, walk/attack/hit poses, per-team value ranges.

**Evidence:** `ELVTR/Saved/Screenshots/WindowsEditor/SwarmDebugShot00016.png` — a live run with
`Swarm.DebugRender 0` (debug boxes off, sprite path only), 120 retinue and 222 brood alive. The main
window shows **nothing but the flame gradient**. The bottom panel shows soldiers.

### The inversion

The small panel is doing the *rendering*. The main window is doing the *lighting*. Neither is doing
both, and that split is why the architecture feels like two systems fighting rather than one view.

That — not performance — is the real argument for collapsing to one viewport. There is no second
scene render to reclaim: `Emberkeep.UI.ViewCam` is `0`, so `AViewCamCapture` never spawns, and
`AUnitPortraitStage` is unreferenced dead code. The Unit Cam is already capture-free. **Do not
propose this refactor as a performance win — it isn't one.** The frame cost is the debug-box
renderer in the main window, and its fix is the Niagara sprite path, tracked separately.

---

## 3. The target

One viewport. The camera interpolates along an army-size axis:

| Army state | Camera |
| --- | --- |
| Full strength, hundreds | wide orthographic, high, near top-down — the battlefield |
| Attrition setting in | descending, `OrthoWidth` tightening, pitch lifting off -90 |
| A handful left | low angled perspective — an RTS shot that has come down to eye level |
| Alone | character camera behind the bearer — a different game |

The world and combat staging change with it. That is expected and desirable; it is not a rendering
detail bolted onto the existing mode.

---

## 4. Open questions — the agenda, not obstacles

Answer these before building. They are genuinely open.

1. **What drives the interpolation?** Raw headcount is the obvious answer and probably the wrong one.
   `Emberkeep.UnitCamProj.Size*` already solves a near-identical problem with a *weighted* total
   (`SizeRetinueWeight 10`, `SizeBroodWeight 0.25`, `SizeBodies 1500`, plus a `SizeCurve` shaping
   dial) to size the panel. That weighting is worth reading before inventing a new one.
2. **Continuous or staged?** A smooth lerp risks a camera that is always subtly moving and never
   settles. Discrete stages with transitions risk feeling like cutscenes. Neither is obviously right.
3. **Does it ratchet back up?** Breathers refill the retinue to `RetinueCap`. Does the camera rise
   again, or is the descent one-way within a run? One-way is a stronger dramatic statement and a
   worse feedback loop; reversible is fairer and less meaningful.
4. **Ortho → perspective is not one lerpable projection.** You cannot naively blend an orthographic
   and a perspective matrix and get something sane in the middle. This needs a real strategy —
   faking ortho with a very narrow FOV at great distance, or a hard swap hidden inside a camera move.
   Treat this as the primary technical risk.
5. **What happens to the flame pool?** `Swarm.FlameRadius` is 900 uu against an `OrthoWidth` of 2400.
   Change the framing and the pool's proportion of the screen changes with it — the spotlight is
   tuned to a fixed zoom. Also `Swarm.DitherWorldAnchor 1` anchors the dither to world space, so
   zooming changes dither density on screen.
6. **What is the HUD for at each scale?** The muster band and squad wings assume you command an army.
   At one-survivor scale they are describing nothing.

---

## 5. What is being deleted, and what must not be lost

Retiring `UUnitCamProjector` also retires **the only sprite-rendering path currently working.**

**Do not delete it until the main window actually renders sprites.** Sequence matters: the Niagara
path must be standing up first (`docs/perf/niagara-sprite-refactor.md`).

> **Update 2026-07-26 — this prerequisite is now met.** The emitter renders. The cause was
> `SimTarget: GPUComputeSim` on the `Swarm` emitter, fixed to `CPUSim`; the `Engine.ExecIndex`
> bindings were correct all along, as this section already suspected. See
> [CAMERA-SCALE-HANDOFF.md](CAMERA-SCALE-HANDOFF.md) for what changed, the post-fix measurements,
> and the editor-driving notes. Retiring `UUnitCamProjector` is now safe *in principle* — but the
> list below of what must not be lost still applies, and retiring it is not required to start.

Worth reading before rewriting, in `ELVTR/Source/ELVTR/UI/UnitCamProjector.h/.cpp`:

- `FUnitCamBillboard` — deliberately POD, so the projection can read live Mass UObjects on the game
  thread and hand Slate nothing but numbers. No lifetime hazard across the UMG/Slate seam. Any new
  camera doing per-unit work should copy that discipline.
- The 1/depth forced-perspective scaling — Doom-sprite maths, already tuned.
- `T_Swarm_2bit` cell selection per unit, including walk/attack/hit frames.
- The close-up shading model (directional term, per-team value ranges, banded light tiers) written
  specifically because distance alone read as a flat grey crowd. That lesson will re-apply.

`AViewCamCapture` (real `SceneCapture2D`, currently disabled) and `AUnitPortraitStage` (dead,
unreferenced, carries `bCaptureEveryFrame = true`) can both go, but neither is urgent and neither is
load-bearing.

---

## 6. Relationship to other docs

- Supersedes the two-viewport arrangement described in `docs/RENDERING-LIGHTING.md` §4d **once built** —
  until then §4d still describes what ships.
- Depends on `docs/perf/niagara-sprite-refactor.md`. That work is a hard prerequisite, not a parallel
  track.
- `GDD.md` §4's hero-relevance tension is the design reason this idea is good: the camera descending
  as the army dies is hero relevance expressed by the view rather than by tuning numbers.
