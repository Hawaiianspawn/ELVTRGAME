# Battle camera framing — task-132

**Status:** decision-pending, owner call · **Mockup:** [`composition-framing.html`](mockups/composition-framing.html)
(published Artifact — see handoff message) · **Touches:** `Kindled.Cam.OrthoWidth`,
`Kindled.Cam.ScalePitchFull`, `Swarm.BroodSpawnRadiusMin/Max` · **Companion doc:**
`docs/design/CAMERA-SCALE.md`

> **Corrected 2026-07-31:** the first pass solved the depth axis only. The brood spawn arc also
> spreads sideways, and lateral gets no relief from the camera's pitch the way depth does — it binds
> harder in every option below. Every number in §2 changed; the four options and the D
> recommendation did not.

## 1. The problem, measured

| Value | Source | Number |
|---|---|---|
| `Kindled.Cam.Ortho` | `SpikeHeroPawn.cpp` | `1` (orthographic) |
| `Kindled.Cam.OrthoWidth` | `SpikeHeroPawn.cpp` | `2400` uu across the view |
| `Kindled.Cam.ScalePitchFull` | `SpikeHeroPawn.cpp` | `-55`° (`Cam.Scale` defaults off, so this hand dial rules) |
| Ground depth visible | derived | `(OrthoWidth / 16:9) / sin(55°)` ≈ **1648uu** |
| `Swarm.SpriteSize` | `SwarmRenderActor.cpp` | `48` uu base billboard size |
| `Swarm.BroodSpawnRadiusMin` | `SwarmCommands.cpp` | `2500` uu |
| `Swarm.BroodSpawnRadiusMax` | `SwarmCommands.cpp` | `4000` uu |
| `Swarm.BroodSpawnArc` | `SwarmCommands.cpp:56` | `120`° width of arrival (shipped, owner-set 2026-07-27, was 360) |
| `Swarm.BroodSpawnFaceCamera` | `SwarmCommands.cpp:71` | `1` — the arc's centre bearing tracks `Kindled.Cam.Yaw`, so it's always centred on screen |
| Retinue formation footprint | `SwarmFormation.cpp`, `Formation.Columns 12`, `Spacing 42.4`, `RankSpacing 110`, cap 120 | ≈ 500 × 1100uu |

The brood doesn't spawn in a ring around the bearer, it spawns on a **120° forward arc that tracks
the camera** — this is a shipped, deliberate CVar, not a modelling assumption. Its own comment
explains why: at `RadiusMin` 2500uu, 120° puts ~90uu between neighbouring spawn columns, comfortably
above the 60uu `Swarm.BroodSeparation`, while leaving the retinue's flanks and rear (the other 240°)
clear of spawns entirely. That arc is what makes containing it a two-axis problem: depth gets
foreshortening relief from the -55° pitch, lateral gets none — `OrthoWidth` is the screen-width
number outright. At `RadiusMax` 4000uu, the arc's sideways swing alone (`2 × 4000 × sin(60°)` =
6928uu) needs almost 2× the width that depth math implies (5825uu). **Lateral binds harder than
depth in every option below.**

At today's settings (`OrthoWidth` 2400), the frame only actually contains a ring out to **~1386uu**
(lateral-bound — depth alone would say 1648uu, sideways swing is stricter). Against the real ring of
2500–4000uu, the near edge is already **1114uu past the frame**, and the far edge is **2614uu past
it**. A wave closes nearly all of its approach off-camera and simply appears already close.

## 2. Four options, same population, one shared scale

Two containment formulas, one per axis — `OrthoWidth` must satisfy both, so it's the max of the two:

- `depthRequired(radius) = radius × sin(55°) × 1.778`
- `lateralRequired(radius) = 2 × radius × sin(60°)` — half of `Swarm.BroodSpawnArc`'s 120° is 60°
- `OrthoWidth = max(depthRequired, lateralRequired)` — solving forward (option A)
- `containedRadius(OrthoWidth) = min(groundDepth(OrthoWidth), OrthoWidth / (2×sin(60°)))` — solving
  backward (options B, C)

Sprite height is unaffected by the lateral fix — it only ever reads off `OrthoWidth`:
`spritePx(OrthoWidth) = 48 × (1920 / OrthoWidth)`.

| # | Option | OrthoWidth | Depth-bound / lateral-bound | Spawn min/max | Soldier height @1920×1080 | Touches gameplay CVars |
|---|---|--:|--:|--:|--:|:--:|
| — | **Today (reference)** | 2400uu | 1648 / **1386uu** | 2500 / 4000uu — **escapes by 2614uu** | 38.4px | — |
| A | **Widen the view** | **6928uu** | 5825 / **6928uu** | 2500 / 4000uu (unchanged) | **13.3px** | No |
| B | **Tighten the arena** | 2400uu | 1648 / **1386uu** | 866 / 1386uu | 38.4px | **Yes** |
| C | **Hybrid** | 4000uu | 2747 / **2309uu** | 1443 / 2309uu | 23.0px | **Yes** |
| D | **Fit-to-bbox** | 2400–6928uu, dynamic | — | 2500 / 4000uu (unchanged) | 38.4–13.3px, dynamic | No |

**Bold** marks the binding axis — it's lateral everywhere, every time: `lateralRequired` is always
1.19× `depthRequired` at these fixed angles (120° arc, -55° pitch), so a 120° arc simply swings wider
than a -55° pitch buys back. A widens
`OrthoWidth` to 6928uu, the width the arc's sideways spread demands at `RadiusMax`. B holds
`OrthoWidth` at today's 2400 and pulls `BroodSpawnRadiusMin/Max` in to the ~1386uu the lateral bound
actually allows (not the looser 1648uu depth alone would suggest), keeping their 2500:4000 (0.625)
ratio. C widens to 4000uu and pulls the spawn ring in to the resulting 2309uu lateral-bound radius.
D never touches the spawn CVars; it solves `OrthoWidth` each wave from the live unit bounding box
(retinue + hero + any spawned brood, padded 10%), clamped to the same 2400–6928uu range A and today
already establish as ceiling and floor.

## 3. What each option costs the read of the fight

**A — Widen.** The tide is always fully visible arriving, which is a real win for "sense of a tide
arriving." But it is a flat, permanent tax: a soldier is ~35% of today's screen size even in a lull
with three brood left on the field and nobody approaching. Formation legibility for the retinue
block survives (it is still a readable rectangle), but the individual reads the team has already
tuned for — the close-up shading model, hit flashes, per-unit poses — lose most of their screen
budget all the time, not just during a wave.

**B — Tighten.** Best possible soldier and formation legibility, unconditionally — the frame never
changes. The cost lands on the fight's pacing, not its picture, and lands harder than the depth-only
read suggested: approach distance drops from 1500uu to **~520uu**, which sits entirely inside the
archer engage range (750uu from `Swarm.Formation.Archers.*`). The wave doesn't just spawn close to a
firefight, it spawns already inside one. The "tide arriving" beat — the thing the wide spawn ring
exists to deliver — is essentially gone, and this is a balance change to `BroodSpawnRadiusMin/Max`,
not a camera change, so it isn't mine to land unilaterally.

**C — Hybrid.** A smaller version of both A's and B's costs: soldiers read at ~57% of today's size at
all times, approach distance is cut to ~866uu — just past the 750uu archer range, so the tide gets a
brief, real closing beat before contact instead of none. It resolves neither problem, it just makes
each one survivable, which is a legitimate compromise if the owner wants one shot that never moves —
but it still edits gameplay CVars and still pays a permanent size tax.

**D — Fit-to-bbox.** Pays A's size cost only while the wave is actually big enough to justify it, and
returns to B's full-size shot the moment it isn't — a lull, a mop-up, the run's final survivor all
get the close shot for free. It is the only option that changes nothing about how or where the brood
spawns. Its cost is entirely in the doing: without the smoothing below, `OrthoWidth` will visibly
zoom in and out on every spawn burst and every batch of deaths, which reads as motion sickness, not
drama.

## 4. Recommendation

**D, fit-to-bbox, camera-only.** It is the only option that does not force a permanent trade against
either soldier readability or wave pacing — it pays each cost only when the fight in front of the
camera actually calls for it — and it needs no balance change to `BroodSpawnRadiusMin/Max`, which
keeps this a UI/camera decision rather than a systems one. It is also the natural first rung of
`docs/design/CAMERA-SCALE.md`'s "the camera tells you whether you are an army or a man" premise: that
doc's own open question #1 already points at a *weighted live population* driving the shot, and a
clamped bounding-box solve is a concrete, buildable version of exactly that, without pre-answering
that doc's still-open staged-vs-continuous and one-way-ratchet questions.

Named so it cannot pump between waves:

- **OrthoWidth clamp:** 2400uu (floor, today's shot) – **6928uu** (ceiling — the widest the shot can
  go, sized to the arc's lateral swing at `RadiusMax`; the AABB solve, not "ring contained," is what
  the camera actually tracks frame to frame).
- **Solve target:** the live unit AABB (hero + retinue + spawned brood), padded 10%.
- **Smoothing:** exponential interpolation toward the solved width, ~1.2s time constant — no hard
  cuts.
- **Re-solve deadband:** ignore AABB changes smaller than ±5% of the current `OrthoWidth` before
  retargeting, so a single unit spawning or dying doesn't retrigger a solve.

These four numbers are motion/interaction parameters, which is why they're specified here rather
than left open — but the camera code that reads the live AABB and the decision of where that solve
lives (camera actor vs. a swarm telemetry query) is an engineering call, not a UI one.

## 5. Mockup

`docs/ui/mockups/composition-framing.html`, published as an Artifact. Look at: the shared-scale
diagram row — options A/B/C/D are drawn as real rectangles at their true relative size, so the
"today" reference card's dashed-edge escape (now visibly sideways as well as short) and A's much
larger footprint are visible side by side rather than argued in prose. Each solved card states both
its depth-bound and lateral-bound `OrthoWidth`, so which axis actually decided the number is never
hidden. Option D shows its two live extremes (small bbox / full-ring bbox) side by side rather than
one static frame, since it's the only option that isn't a fixed number.

## 6. Handoffs

- **→ gameplay-director:** if B or C is the owner's pick instead of D, the `BroodSpawnRadiusMin/Max`
  retune in §2's table is a balance call, not mine to land — I've named the numbers that fit the
  frame on both axes, not verified they still feel like a wave building, especially B's ~520uu
  approach sitting entirely inside archer range.
- **→ engineering (camera):** if D is picked, the AABB-solve + smoothing/deadband values in §4 are a
  starting proposal, not a spec — they need to be checked against real per-frame AABB noise before
  landing in `SpikeHeroPawn.cpp`.
- **→ pixel-art-director:** not blocking this decision — flagged only so a resize of the main render
  target's effective sprite scale (13.3–38.4px depending on option) is on the radar for the next
  readability pass at horde scale, same as any other framing change would be.

## 7. Canon proposals

None. This spec proposes CVar values and a camera-solve strategy; it does not change
`docs/design/CAMERA-SCALE.md`'s open questions or `docs/art/aesthetic-direction.md`'s full-colour
ruling, both of which it treats as binding.
