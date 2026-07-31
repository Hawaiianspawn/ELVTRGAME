---
id: 059
title: Give the brood nine looks on one draw call — variant axis in the atlas, chosen by a display-weight table
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/Mass/SwarmFragments.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h", "ELVTR/Content/Spike1/**", "ELVTR/Content/Swarm/**", "ELVTR/Content/Sprites/Swarm/**", "RawArt/Sheets/T_Swarm*.png", "docs/data/art/requests/swarm-units.json", "docs/data/art/brood-variants.json", "docs/data/art/provenance.json", "docs/perf/niagara-sprite-path.md"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [57]
epic: ""
evidence: A PIE capture of the viewport horde at gameplay density where the brood visibly shows several different ooze looks at once and at varying sizes, plus a second capture at a deliberately skewed weight table (one skin at weight 100) proving the weights actually drive the mix, with the draw-call count unchanged from today and a before/after frame-time row.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: brood-variety
decided: "2026-07-29 done"
model: opus
---

## Why now
Owner, 2026-07-28, on the goal of tightening the visuals: *"if there is a better way to
render those sprites via niagara and the simulation space feel free to research."*

There is, and the research is already done — see §Findings. This also resolves the decision
`task-055` stalled on. That task packed real PixelLab character states, imported them, and
wired them correctly — into the **Unit Cam**, which the game no longer ships. The owner's
call (2026-07-28) was **give the viewport atlas a state axis**. That work falls outside
`task-055`'s `owns:` set, so it is filed here rather than widening that task.

## Findings — the research, so the teammate does not redo it

**1. The packed render int32 has free bits, and a state index costs nothing.**
`SwarmRenderPack` (`Mass/SwarmFragments.h:202`) already rides four things in one
`TArray<int32>`: anim bits 0-7, size roll 8-11, world facing 12-16, squad byte 17-20.
**Bits 21-31 are free.** A state index rides there for no new array, no new fragment field,
no class-layout change — exactly the reasoning that put size, facing and squad there. This
is the cheap half of the job and it is already the house pattern.

**2. Growing the SubUV atlas is the wrong axis.** `SwarmSheet` is `Columns 8, Rows 4`
(8 facings × brood walk0/1, retinue walk0/1). Adding states multiplies rows —
states × teams × frames — against a single texture with a hard size ceiling, and every new
state means repacking one giant sheet. **A `Texture2DArray` with one slice per state** keeps
the SubUV decode 2D (facing × frame), adds a slice index as a separate per-particle float,
holds at ONE draw call and ONE renderer, and lets each state be authored and imported
independently. Evaluate this first; it is the likely answer.

**3. Half the current atlas is already wasted.** `SwarmFragments.h:60` records that the
brood has no real rotations — *"every column packs the same south frame"*. Rows 0-1 are
eight copies of one image. Reclaiming that is free space for states before anything grows.

**4. `SizeScale` is computed, packed, and then thrown away.** `SwarmRenderPack::SizeScale`
turns the 4-bit size roll into a per-entity multiplier, and the Niagara bridge explicitly
ignores it — `SwarmRenderActor.cpp:1338`: *"This path also IGNORES the size roll —
`Swarm.BroodSizeJitter` does nothing to Niagara sprites, which need a per-particle size
array and a graph edit to honour it. Every brood sprite is the same size until that
exists."* That is per-unit size variety sitting fully built and unused, for the cost of one
more array push and one graph edit. Cheapest visual-variety win on the board.

**5. The per-frame respawn is the fragile part.** The graph currently drives
`SpawnPerFrame` from `User.Count` with a positive lifetime, and interpolated spawn kills
short lifetimes. That is spawn churn standing in for a particle pool. The standard
array-driven pattern — a **persistent pool** sized to the cap, each particle reading its slot
by `ExecutionIndex` and killing itself when its index exceeds `Count` — removes the churn and
the lifetime trap together. Worth doing while the graph is open.

**6. Cost is not the reason to do any of this.** The one-camera bench measured **433fps with
Niagara rendering effectively free and the Mass sim at 100% of the frame**. So the
motivation here is **visual capability, not speed**, and the extra arrays are affordable.
Do not sell any of this as an optimisation, and do not trade visual range for a frame-time
win nobody needs.

**7. Do NOT return the emitter to GPU sim.** `NS_Swarm` drew nothing for a long stretch
because the emitter was `GPUComputeSim`; `CPUSim` fixed it. The emitter graph was never at
fault. Several perf docs still carry the retracted claim — do not trust them on this.

**On simulation space:** confirm the emitter's sim space and state it in the handback rather
than changing it speculatively. Positions arrive via `SetNiagaraArrayPosition` (LWC-aware
`Position` array) in world terms; if the emitter is Local and the actor is not at origin
that is a latent bug worth recording even if nothing currently trips it.

## Update 2026-07-28, later the same day — four things moved under this task

Written by the session that pinned the eye-level camera. **All four change the starting
conditions of §Findings; read them before acting on it.**

**A. The 48px cell lock is gone — the atlas is now 448×224, 8×4 @ 56px.** Forced, not
chosen: the incoming retinue character's content measures 41×49 and `pixelpipe pack` refuses
to scale pixel art (correctly). Scaling destroys the art; regenerating cannot reproduce a
character authored in the web Character Creator. So `swarm-units.json` `output.cell` went
48 → 56. **Nothing in C++ read that number** — `SwarmSheet` knows Columns/Rows only, and the
material's SubUV split is ratio-based — so this moved `T_Swarm_2bit` and nothing else.
Consequence for finding #2: the "hard size ceiling" argument is now *stronger*, since each
cell costs 36% more texels than the numbers in that finding assume.

**B. The retinue sheet is a different character.** `unit-knight` (Hallam) is out;
PixelLab `243c7684-552a-4a73-9be7-046a8c5a9221` is in, at the owner's direction, packed at
`quantize: false`. **It has no walk animation** — `south.walk0` and `south.walk1` both point
at the static south rotation, so the facing-the-camera view has no leg motion where the
knight had a real 2-frame walk. One `animate_character` call fixes it; it was skipped to keep
the pack at 0 credits. If this task repacks the atlas, that regression is worth closing in
the same pass.

**C. The 4-value palette lock is SUPERSEDED — the game ships in colour.** Owner call,
recorded as a direction change in `docs/art/aesthetic-direction.md` (AMENDMENT 2026-07-28);
`Emberkeep.Quantize` is now 0. Any state art this task packs should be authored and judged
**in colour**, not against the demichrome ramp. Note the knock-on already flagged in that
amendment: the flame thresholds and light floors were all tuned against the *posterised*
output and have not been retuned, so the raw scene currently reads oddly. Do not tune sprite
brightness against an unretuned scene and conclude the sprites are wrong.

**D. The owner named the variety source.** For brood variety it is the **nine numbered
brood-ooze states**, already on disk and needing no generation:
`RawArt/Renders/brood-ooze/raw/state00_base` … `state08_slug` (8 rotations each). The wider
group has 37 states and the retinue group 28; the owner scoped this to the nine plus the one
retinue character. Finding #3 still holds and is the way in — brood rows 0-1 are eight copies
of one south frame, so there is free space before anything grows.

**E. THE MISSING PER-PARTICLE COLOUR ARRAY IS THE REAL HEADLINE — it has three symptoms,
not one.** The bridge pushes exactly `Positions`, `SubImages`, `Count`
(`SwarmRenderActor.cpp:1467-1471`). Everything below is downstream of that single gap, and
all three are owner-reported, not theoretical:

  1. **No hit flash.** Already documented at `SwarmFragments.h:54` — *"HIT is lost on the
     sprite path… Niagara has no per-particle colour array here."* `HitFlashBit` is still
     written by the strike pass every frame and still read by the debug renderer and the Unit
     Cam; nothing on the shipping path draws it. Owner noticed, 2026-07-28: *"hit reacts do
     not appear to be showing at least the white blink."*
  2. **No distance gradient, and units read blown out.** The `Atten`/`Lit` maths exists and is
     good (`SwarmRenderActor.cpp:1355-1359`) but lives in the **debug-box** renderer, so it is
     dead code at `Swarm.DebugRender 0`. Consequence: `Swarm.UnitLightFloor`,
     `BroodLightFloor`, `BroodLightCeil` and `UnitBackShade` currently do **nothing** on the
     shipping path. Owner: *"is there any way to gradient them into existence? the light is
     blown out."*
  3. **No per-unit size variety** — finding #4 above, same fix.

**Do the colour array first.** It is one array push plus one graph edit, it closes two
owner-reported defects immediately, and the state axis rides the same graph visit. The
"per-unit distance fade" carved out in §Scope fence as a separate task should be **folded back
into this one** — it is not separable work, it is the same array.

**F. Half the atlas is now duplicate rows, and finding #3 is outdated in a new way.**
Finding #3 said brood rows 0-1 were eight copies of one south frame. That is fixed — the
brood has eight real facings as of 2026-07-28. But **neither team has a walk animation now**,
so `walk0` and `walk1` are identical images: **row 1 duplicates row 0, and row 3 duplicates
row 2. Sixteen of thirty-two cells are redundant.** That is a bigger reclaim than finding #3
described, and it is the cheapest place to put a second variant — but note it costs the walk
axis, which is currently free only because no walk animation exists. Generating one walk
animation reclaims that cost and the space disappears again, so do not design around it as
permanent.

**G. The emitter, read from the live asset 2026-07-28 (so nobody has to open it to plan).**
`GetSystemSummary` on `/Game/Spike1/NS_Swarm.NS_Swarm`:

  - **User variables — exactly three, matching the bridge:** `User.Count`
    (`NiagaraInt32`), `User.Positions` (`NiagaraDataInterfaceArrayPosition`),
    `User.SubImages` (`NiagaraDataInterfaceArrayFloat`). Nothing else is exposed, which
    confirms finding E from the asset side rather than only from the C++ side.
  - **One emitter, `Swarm`, `bEnabled: true`, `simTarget: CPUSim`** — finding #7 holds, the
    emitter is where the fix left it. Do not touch this.
  - **One renderer: `NiagaraSpriteRendererProperties`.** Single renderer, so the
    "draw calls unchanged" bar in §Done when is currently met by construction — any design
    that adds a second renderer breaks it.

New user variables needed, following the existing naming: `User.Colors`
(`ArrayColor` — closes hit flash *and* distance fade), `User.Sizes` (`ArrayFloat` — closes
the size roll), and a state index (`ArrayFloat`) if the `Texture2DArray` route is taken.

**H. THE `owns:` SET IS WRONG AND MUST BE FIXED BEFORE EDITING.** This task must edit
`NS_Swarm`, which lives at **`/Game/Spike1/`** — i.e. `ELVTR/Content/Spike1/**`. The `owns:`
list claims `ELVTR/Content/Swarm/**` and `ELVTR/Content/Sprites/Swarm/**`, **neither of which
contains it.** The same applies to the sprite material if it is also under `Spike1/`. Widen
`owns:` to include `ELVTR/Content/Spike1/**` (and re-run the lock check, because that path may
collide with another task) before touching the asset — an undeclared edit is exactly what the
lock machinery exists to catch.

## Update 2026-07-29 — C++ half LANDED, graph half fully specified

**The bridge side is done, compiled and in the binary.** `SwarmRenderActor.cpp` now builds and
pushes two more arrays beside `SubImages`:

  - `Colors` (`SetNiagaraArrayColor`) — `HitFlashBit` → `FLinearColor::White`, else the same
    `Atten`/`Lit` per-team distance model the debug renderer uses, so the two paths cannot
    disagree about how a unit is lit.
  - `Sizes` (`SetNiagaraArrayFloat`) — `SwarmRenderPack::SizeScale(Bits, jitter)`.

Both use **file-static** scratch arrays (`ColorScratch`, `SizeScratch`), not members: a member
is a class-layout change and Live Coding cannot apply those. Pushing arrays the emitter does
not yet read is harmless — an unbound User array is ignored — so this half is already live and
verifiable independently.

**BUILD NOTE: Live Coding is unusable on this module.** A patch compile crashed the editor in
`FAutoConsoleObject::FAutoConsoleObject` → `dynamic initializer for 'CVarMusterWingRatio'`
(`EmberkeepHud.cpp:27`) — the patch DLL re-runs static `TAutoConsoleVariable` initialisers and
re-registers CVars the base module owns. It named a file the edit never touched. Use
`Stop-Editor; Build-Editor; Start-Editor; Wait-Mcp` (`Scripts/ue-mcp.ps1`); the full rebuild
takes ~7s.

**The graph edit is now fully specified — read from the live asset, no discovery left.**

`NS_Swarm` particle flow (from `GetEmitterTopology`):

  - `ParticleSpawnScript` → `InitializeParticle`, then a Set-Parameters module
    **`SetVariables_0830B4C9465A68FDAA5AC6AD1BB43F88`** which sets exactly two things.
  - `ParticleUpdateScript` → `ParticleState` (enabled); `ScaleColor`,
    `SolveForcesAndVelocity` and the *update* Set-Parameters module
    `SetVariables_005C077A4C24F3177C71339D45B09288` are all **DISABLED**.

So position/subimage are written **at spawn only** — which is why the emitter leans on
`SpawnPerFrame` churn (finding #5). The existing bindings, from `GetModuleInputValues`:

| Input | Dynamic input asset |
|---|---|
| `Particles.Position` | `/Niagara/DynamicInputs/Arrays/SelectPositionFromArray` |
| `Particles.SubImageIndex` | `/Niagara/DynamicInputs/Arrays/SelectFloatFromArray` |

**Therefore the remaining work is mechanical:** add two User parameters —
`User.Colors` (`NiagaraDataInterfaceArrayColor`) and `User.Sizes`
(`NiagaraDataInterfaceArrayFloat`), matching the names the C++ already pushes — then add
`Particles.Color` and the sprite-size parameter to that same spawn Set-Parameters module,
bound to `SelectColorFromArray` and `SelectFloatFromArray` respectively, indexed the same way
`Position`/`SubImageIndex` already are. Copy the existing binding pattern rather than
inventing one.

**Gotcha carried forward:** MCP asset edits are in-memory until `save_assets([])`. Do not
declare the graph edit done without a save and a re-read.

## Update 2026-07-29 — UNPARKED and rescoped: the owner asked for this out loud, with a weight table

Owner, 2026-07-29: *"can we get npc variety into our system, lets not create any new stuff but
utilize some of the good skins we have. Example the Oozes have some variance in appearence.
They will be match to display weight."*

That is this task, plus one layer it did not have. **Four things are now settled that were
open when it parked — all four measured, so do not re-derive them.**

**I. THE ART IS MEASURED AND IT FITS. No repack of the cell size, no generation, no credits.**
All nine states (`state00_base` … `state08_slug`) have all 8 rotations, in full colour, 88×88
source. Measured max content bbox across all 72 frames:

| state | max content | | state | max content |
|---|---|---|---|---|
| `state00_base` | 40×44 | | `state05_ridge` | 52×44 |
| `state01_sump` | 53×44 | | `state06_crown` | 40×47 |
| `state02_bell` | 43×45 | | `state07_twin` | 40×45 |
| `state03_stalk` | 26×45 | | `state08_slug` | 43×47 |
| `state04_wedge` | 47×44 | | *(shipping brood)* | 53×44 |

**Every one fits the existing 56px cell** (worst case 53×47). `pixelpipe pack` will not have to
scale anything, so Update A's forced-cell episode does not repeat. Width runs **26px → 53px, a
2.0× spread** — real separation, not the 1.2× collapse that skeleton-based variants suffer
(`variant-families-workflow`). **The nine are good as they are. Do not re-measure them with
`/variants` and do not regenerate any of them to "improve the spread".**

**II. GROW ROWS. Finding #2 is OVERRULED — do not build a `Texture2DArray`.** That finding
argued from a "hard size ceiling" without numbers. Here are the numbers: keeping the walk axis,
`Rows = variants×2 + retinue 2 = 20`, so the atlas is **448×1120** at 8×20 — under 2K in both
dimensions, one texture, one renderer, one draw call, ~2MB uncompressed. The ceiling is not
close and never was. A `Texture2DArray` costs a new material decode, a third per-particle
array, and a new import path to buy headroom nothing needs. **Rows is a `Rows = 4` → `Rows = 20`
constant, a `CellFor` signature, and a `frame_map`.** Take it.

**Keep the walk axis even though it is currently dead** (`walk0 == walk1` for both teams today,
Update F). Row = `Variant*2 + WalkFrame`. The ten duplicate rows cost ~1MB of texels and nothing
else; collapsing them saves that 1MB and forces a *second* layout change the day any
`animate_character` walk lands. Update F already warned not to design around the duplication as
permanent — this is that warning honoured.

**III. THE VARIANT PICK IS FREE — no new fragment, no class-layout change, no Live Coding trap.**
`SwarmProcessors.cpp:1050` already derives the size bucket from `Jitter[i].Phase` via
`SwarmRenderPack::BucketFromPhase` — *"Derived from the jitter phase rather than stored, so no
fragment grows a field."* **The variant index derives from the same `Phase`, the same way**, and
rides bits 21-24 of the existing packed int32 (bits 21-31 are free, finding #1). `Phase` is a
per-entity `FRandRange(0, 10)` set once at spawn, so the look is stable for that entity's life
and costs zero storage. This is the single most important line in this update: **if you find
yourself adding a fragment field, you have taken a wrong turn.**

**IV. The display-weight table — the layer the owner added, and the reason not to pre-judge
the art.** Owner's chosen reading, confirmed 2026-07-29: *each skin gets a weight controlling
how often it appears.* Write `docs/data/art/brood-variants.json` — a variant id, a source path,
and an integer weight per row — and have the pick map `Phase` through the **cumulative** weight
array rather than a uniform 0-8. Consequences worth stating because they are the point:

  - **Pack all nine regardless.** A skin that reads badly in the horde drops to `weight: 0` and
    vanishes with no repack, no reimport, no rebuild. That is why there is no "which nine ship"
    gate ahead of this task and no contact-sheet approval step — the weights *are* the gate, and
    they are tunable after seeing the thing move.
  - **Expose one CVar** (`Swarm.BroodVariantWeights` or equivalent) so the mix can be skewed live
    on the Breadboard without a reload. The second required capture depends on it.
  - Ship sensible defaults, not a flat distribution — a common base look with a couple of rare
    ones reads as variety; nine-way-equal reads as noise. The numbers are the owner's to tune;
    getting them arguable is enough.

**Rescored** `feel 1 → 2` (a visual layer that reads in play, `TEMPLATE.md` rubric) and
`risk 3 → 2` (the C++ half landed, the graph edit is fully specified from the live asset, the
art is measured — the remaining unknown is MCP asset-write reliability, not the design).

**Retinue variety is NOT in this task.** The `T_Soldier_Knight_02..06` and
`T_Soldier_Archer_02..05` sheets on disk are the friendly-side equivalent and they ride these
exact same variant bits once this lands — but they are packed 8×4 per-character sheets, not raw
rotation sets, so wiring them needs their raw renders located first. File it as a follow-up
after this closes. Do not widen scope to reach it.

## Done when
- The **brood in the viewport** shows several different ooze looks at once at gameplay density.
- **The weight table demonstrably drives the mix** — skewing one skin to weight 100 changes what
  the horde is made of, on screen.
- All nine states packed into the atlas, `weight: 0` available as the way to retire a bad one.
- **Draw calls unchanged.** One renderer, one draw call. If the design costs N draw calls,
  it is the wrong design; say so and stop rather than shipping it.
- Per-unit size variety is live — `Swarm.BroodSizeJitter` visibly does something.
- A before/after frame-time row at gameplay density, so the claim that this is free is
  measured rather than asserted.
- `docs/perf/niagara-sprite-path.md` records the chosen decode, the bit layout, and the sim
  space finding.

## Why this is not a sibling of the `scene-tightening` fan
It writes `SwarmRenderActor.cpp` and `.h`, and so does `task-057`. Overlapping `owns:` means
the cut is wrong, not that the declaration should be loosened — so this stands outside the
epic as sequential follow-on work with `depends-on: [57]`. It is the same subject matter and
a different file-ownership story, which is exactly the line the epic machinery draws.

## Scope fence
- **The flat-shaded 3D route is shelved** (owner, 2026-07-28: *"the 3d perception can be
  shelved for now but we will potentially come back to it when we get what we are looking
  for"*). Stay on sprites. Do not propose or prototype mesh renderers. `task-015` is the
  filed instance of that question and it is not this task.
- Not the Unit Cam. `UnitCamProjector.*` is untouched.
- Not new sprite generation — no PixelLab credits. The states are already packed and
  imported; this is about reaching them.
- Not the per-unit distance fade. Real open defect, separate task.

## Spawn prompt
```
You are giving Emberkeep's brood nine different looks on one draw call, chosen by a
display-weight table (C:\Projects\ELVTRGAME).

Owner, 2026-07-29: "can we get npc variety into our system, lets not create any new stuff but
utilize some of the good skins we have. Example the Oozes have some variance in appearence.
They will be match to display weight."

READ docs/backlog/task-059-niagara-sprite-path-state-axis-and-variety.md IN FULL BEFORE YOU
TOUCH ANYTHING. It carries several sessions of research and four measured decisions. The
sections below the "Update 2026-07-29" heading OVERRULE parts of the older §Findings — where
they disagree, the 2026-07-29 update wins. In particular:

  * THE ART IS ALREADY MEASURED AND IT FITS. Nine states, RawArt/Renders/brood-ooze/raw/
    state00_base .. state08_slug, 8 rotations each, full colour, worst-case content 53x47
    inside the existing 56px cell. GENERATE NOTHING. Spend NO PixelLab credits. Do not
    re-measure with /variants and do not "improve" any state.
  * GROW ROWS, DO NOT BUILD A Texture2DArray. Finding #2 in the older text argues the other
    way and it is OVERRULED with numbers in update §II: 8x20 @ 56px is 448x1120, nowhere near
    any ceiling. Keep the walk axis: Row = Variant*2 + WalkFrame, Rows 4 -> 20.
  * THE VARIANT PICK ADDS NO FRAGMENT FIELD. SwarmProcessors.cpp:1050 already derives the size
    bucket from Jitter[i].Phase. Derive the variant index from that SAME Phase and ride bits
    21-24 of the existing packed int32. IF YOU ARE ADDING A FRAGMENT FIELD YOU HAVE TAKEN A
    WRONG TURN -- and it would also hit the Live Coding class-layout crash (see GOTCHAS).
  * THE WEIGHTS ARE THE GATE. Pack all nine. docs/data/art/brood-variants.json holds a weight
    per variant; the pick maps Phase through the CUMULATIVE weights, not a uniform 0-8. A skin
    that reads badly drops to weight 0 with no repack. Expose a CVar so the mix can be skewed
    live. Ship defaults with a common base and a couple of rare looks -- not nine-way-equal.

THREE THINGS MUST AGREE ABOUT THE LAYOUT and SwarmFragments.h:44 says so explicitly: the C++
constants (SwarmSheet::Columns/Rows), the packed sheet (docs/data/art/requests/swarm-units.json
output.grid + frame_map), and the sprite renderer's Sub UV field in the ASSET. Change one and
you must change all three, or the horde decodes garbage. Verify the Sub UV value by reading it
back from the asset, not by assuming your write took.

The remaining Niagara graph work is ALSO fully specified from the live asset in the
"Update 2026-07-29 — C++ half LANDED" section — User.Colors and User.Sizes need adding and
binding in the spawn Set-Parameters module, copying the existing SelectPositionFromArray /
SelectFloatFromArray binding pattern. The C++ already pushes both arrays. Do that in the same
graph visit as the variant work; it closes the missing hit flash and the blown-out lighting the
owner reported, and it is why Swarm.BroodSizeJitter currently does nothing.

Older context that still holds, unchanged:

Owner, 2026-07-28: "if there is a better way to render those sprites via niagara and the
simulation space feel free to research." The research is DONE and written up in
docs/backlog/task-059-niagara-sprite-path-state-axis-and-variety.md §Findings. READ IT FIRST
and do not redo it. Summary of what you are inheriting:

  1. SwarmRenderPack (Mass/SwarmFragments.h:202) already rides anim/size/facing/squad in one
     TArray<int32>. BITS 21-31 ARE FREE — a state index rides there for no new array and no
     class-layout change. That is the house pattern; follow it.
  2. [SUPERSEDED by update §II — this item argued for a Texture2DArray and it is OVERRULED.
     Grow rows. Left here only so you recognise it if you meet the claim in another doc.]
  3. [SUPERSEDED — the brood grew eight real facings on 2026-07-28. What IS duplicated now is
     walk0 == walk1 for both teams (update §F), and update §II says KEEP that duplication
     rather than reclaiming it. SwarmFragments.h:60 still carries the stale comment; fixing
     that comment is in scope.]
  4. SwarmRenderPack::SizeScale is computed, packed, and THROWN AWAY by the bridge
     (SwarmRenderActor.cpp:1338). Per-unit size variety is fully built and unused — one more
     array push and one graph edit. Cheapest win here; do it.
  5. The graph drives SpawnPerFrame from User.Count with a positive lifetime, and
     interpolated spawn kills short lifetimes. That is spawn churn standing in for a pool.
     Move to a PERSISTENT POOL sized to the cap, each particle reading its slot by
     ExecutionIndex and killing itself when index > Count.
  6. COST IS NOT THE MOTIVATION. The one-camera bench measured 433fps with Niagara rendering
     effectively FREE and the Mass sim at 100% of the frame. This is about visual capability.
     Do not sell it as an optimisation and do not trade visual range for a frame-time win
     nobody needs.
  7. DO NOT return the emitter to GPU sim. NS_Swarm drew nothing for a long stretch because
     the emitter was GPUComputeSim; CPUSim fixed it. The graph was never at fault. Several
     perf docs STILL CARRY THE RETRACTED CLAIM — do not trust them on this point.

SIM SPACE: confirm the emitter's simulation space and STATE IT in your handback rather than
changing it speculatively. Positions arrive via SetNiagaraArrayPosition (LWC-aware Position
array) in world terms; if the emitter is Local and the actor is not at origin, that is a
latent bug worth recording even if nothing currently trips it.

WHERE THE ART COMES FROM: RawArt/Renders/brood-ooze/raw/state00_base .. state08_slug, already
on disk with 8 rotations each. Extend the composite sources + frame_map in
docs/data/art/requests/swarm-units.json and repack with pixelpipe, exactly the way the two
existing brood/retinue sources are declared there. quantize STAYS FALSE (the game ships in
colour). Record the pack in docs/data/art/provenance.json — it currently has NO brood-ooze
record at all. You are NOT generating art. No PixelLab credits are to be spent.

DONE WHEN:
  - The brood in the viewport shows several different ooze looks at once at gameplay density.
  - THE WEIGHT TABLE DEMONSTRABLY DRIVES THE MIX — skew one skin to weight 100 and the horde
    visibly becomes mostly that skin. Capture both states.
  - DRAW CALLS UNCHANGED — one renderer, one draw call. If your design costs N draw calls it
    is the wrong design: say so and stop rather than shipping it.
  - Swarm.BroodSizeJitter visibly does something.
  - A before/after frame-time row at gameplay density — measured, not asserted.
  - docs/perf/niagara-sprite-path.md records the decode, the bit layout, the weight-table
    format and the sim space.

DO NOT TOUCH:
  - The flat-shaded 3D route. SHELVED by the owner 2026-07-28: "the 3d perception can be
    shelved for now but we will potentially come back to it." Stay on sprites. Do not
    propose or prototype mesh renderers. task-015 is the filed instance of that question.
  - UnitCamProjector.* — the Unit Cam is disabled, not deleted.
  - ELVTR/Content/PostProcess/** — task-057's territory, already landed.
  - ELVTR/Source/ELVTR/UI/** — task-058's territory.
  - RETINUE variety. T_Soldier_Knight_02..06 and T_Soldier_Archer_02..05 exist on disk and
    ride these same variant bits once you land them, but they are packed 8x4 per-character
    sheets rather than raw rotation sets. FOLLOW-UP TASK, not this one. Do not widen scope.
  - The nine ooze states themselves. Do not edit, requantize, recolour or regenerate the
    source PNGs. They are the input, measured and accepted.

GOTCHAS:
  - MCP asset edits are IN-MEMORY until save_assets([]). You are editing NS_Swarm. And a
    set_properties that returns true is NOT proof the write landed — READ THE VALUE BACK and
    compare before you recompile or save.
  - Live Coding is UNUSABLE on this module — see the BUILD NOTE above. Use
    Stop-Editor; Build-Editor; Start-Editor; Wait-Mcp (Scripts/ue-mcp.ps1), ~7s.
  - This task parked on 2026-07-28 when the editor lock went to task-060 (blood), which has
    since LANDED. BloodSubsystem now exists on the render path — reconcile with it rather than
    assuming SwarmRenderActor.cpp looks the way this file's line numbers say. Re-read before
    editing; every line number quoted in this task may have moved.
  - The tree may carry uncommitted work from a concurrent session in SwarmRenderActor.cpp.
    Build on top of it. Do not revert, stash or tidy it, and do not attribute it to anyone.

You hold the unreal-editor and mcp-9000 locks. Deliver ON-SCREEN EVIDENCE from a runnable
build — TWO PIE captures at gameplay density: the default weight mix showing several ooze
looks at once, and a skewed table proving the weights drive it. Not a diff plus "it works".
```
