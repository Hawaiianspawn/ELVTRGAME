# Niagara sprite path — refactor proposal

**Status:** proposal, for owner/team-lead sign-off. **Decision already made** (owner,
2026-07-26): commit to the Niagara/Mass sprite path, stop investing in the debug-box
renderer. This document scopes the "how" and the cost, and states plainly what it does
not yet know.

**Author:** performance-director. Read-only for this document — no source or asset
edits. Numbers are from a live in-editor measurement run by team-lead
(`-SwarmBench` harness, `SwarmRenderActor.cpp:448-527`), full table in
[BUDGETS.md](BUDGETS.md). Everything else here is static analysis of the current
source tree and the project's own dated findings in `docs/RENDERING-LIGHTING.md` and
`ELVTR/SETUP-EDITOR.md`.

## 1. Why: the measured case

Retinue held at 100, brood swept, `Swarm.DebugRender 1` (at the time of this measurement
pass, the only renderer that drew anything — the Niagara path drew nothing then; fixed
2026-07-26, see the RESOLVED box in §2). A = `Swarm.UnitShading 1` (default two-box
shading). B = `Swarm.UnitShading 0` (flat single box, the cheapest the debug-box
renderer can be).

| brood | A draw ms | B draw ms | A/B ratio | A frame ms | B frame ms |
|---|---|---|---|---|---|
| 500 | 4.85 | 3.09 | 1.57x | 4.85 | 3.10 |
| 1000 | 14.62 | 7.59 | 1.93x | 14.62 | 7.59 |
| 2000 | 40.65 | 18.42 | 2.21x | 40.66 | 18.43 |
| 5000 | 132.33 | 57.15 | 2.32x | 132.33 | 57.15 |
| 10000 | 350.04 | 135.47 | 2.58x | 350.02 | 135.47 |

At 10000 brood, GPU sits at 23.11ms (A) / 13.71ms (B) — the frame cost is not the
hardware. It is CPU-side draw *submission*: `DrawDebugSolidBox` is one immediate-mode
primitive per entity (two per entity under `UnitShading 1`), and that cost scales
with entity count, not with what the box looks like once submitted.

**This is not a headroom problem, it's a ceiling problem.** Interpolating the two
nearest measured points (B, 1000 brood -> 7.59ms; B, 2000 brood -> 18.43ms — linear
interpolation, not separately measured), the flat-shaded debug-box renderer crosses
the 16.6ms/60fps budget at roughly **~1930 total entities** (100 retinue + ~1830
brood). That's above the letter of the Spike-1 bar ("1,000+ units," GDD §10) but far
below the entity counts GDD §10's actual gate implies — 4 players' retinues plus
enemy hordes at late-run scale — and this is single-client; the real gate measures 4
clients under the aggregate-replication model, which this branch doesn't test at all.
**Turning off directional shading buys a multiplier, not a fix.** See §4 for what that
multiplier is still worth as an interim measure.

Finding #2 from the earlier diff review (uncapped per-neighbour `GetSafeNormal` +
K-insertion-sort in the combat pass) is **not isolated by this data** — the game
thread stays under 27ms in every row above, so whatever that pass costs, it isn't
the current constraint. It doesn't disappear, it's just invisible behind a much
bigger cost. Revisit it once the render bridge stops dominating (tracked in
[BUDGETS.md](BUDGETS.md)).

## 2. What actually blocks the emitter today

> **RESOLVED 2026-07-26 — this section's diagnosis was wrong. Read this box before §2.**
>
> The emitter drew nothing because the `Swarm` emitter's **`SimTarget` was `GPUComputeSim`**.
> Setting it to **`CPUSim`** made hundreds of sprites render immediately (evidence:
> `Saved/Screenshots/WindowsEditor/SwarmDebugShot00021.png`, and `00023.png` from the saved asset).
> CPU sim is also the correct target: the swarm is simulated entirely on the CPU and positions are
> pushed per frame via `SetNiagaraArrayPosition`, so there is nothing for the GPU to simulate.
>
> **The emitter graph was never at fault.** Read live via `NiagaraToolsets.NiagaraToolset_System`,
> every layer this section suspected was already correct: `SpawnPerFrame.Spawn Count` linked to
> `User.Count`, `SpawnBurst_Instantaneous` disabled, `Particles.Position`/`SubImageIndex` bound
> through `Engine.ExecIndex`, renderer on `M_Swarm` with `SubImageSize 8×4` (matching `SwarmSheet`),
> emitter `CalculateBoundsMode: Fixed ±100000`, zero compile warnings, system correctly assigned on
> the level's `SwarmRenderActor_0`.
>
> Consequently **§8's recommendation #1 is void** — the one-hour "can MCP reach the module graph"
> test was already run on 2026-07-23 (`docs/UNIT-CAM-HANDOFF.md`, a doc this proposal does not
> cite) and the answer was yes. The toolset reads *and writes* the module stack.
>
> Post-fix measurements are in §9.

Per `docs/GATE1-FUN-PROTOTYPE.md` §3a (2026-07-25 measurement pass), every other
layer of the bridge is confirmed working:

| Layer | Evidence | Verdict |
|---|---|---|
| Mass sim + render buffers | spacing report: 120/120 distinct retinue positions | good |
| C++ -> Niagara bridge | pushes `Positions`/`SubImages`/`Count` every tick | good |
| Component visibility | debug log confirms Niagara component visible, debug off | good |
| `T_Swarm_2bit` texture | imported, read back OK, 4 palette values | good |
| `NS_Swarm` Sub UV renderer property | verified 8x4 on disk (`SETUP-EDITOR.md` §3, set + read back via MCP 2026-07-26) | good |
| Unit Cam (bypasses Niagara, draws the same texture) | renders correctly | good |
| ~~**`NS_Swarm` emitter graph**~~ | zero particles visible with debug render off | ~~**the fault**~~ — superseded, see the RESOLVED box above |

> **Historical — the next two paragraphs are the pre-fix theory and the plan to test it.
> Both are moot.** The particle-spawn module's `Engine.ExecIndex` bindings were fine —
> read live via `NiagaraToolsets.NiagaraToolset_System` after the fault was found
> elsewhere, per the RESOLVED box above. Kept below as the record of how this proposal
> reasoned about the bug before the actual cause (`SimTarget: GPUComputeSim`) was found;
> nobody needed to spend the hour this section proposes.

The standing theory (`SETUP-EDITOR.md` §3.3, restated as still-live in GATE1 §3a) is
that the particle-spawn module's `Set Particles.Position` / `Set
Particles.SubImageIndex` nodes — which are supposed to do a "Select [Position/Float]
From Array, Direct Set by `Engine.ExecIndex`" — aren't actually bound to
`Engine.ExecIndex` per-particle, so every particle reads the same array slot. **Nobody
has actually opened the graph and confirmed this is the specific broken node** — it's
inferred from symptoms (stacked-at-one-point in the original write-up; zero-visible in
the latest one), not diagnosed at the node level. That gap matters for the cost
estimate in §5.

**One thing worth re-checking before assuming this needs a human, because the
capability picture changed after GATE1 §3a was written:** `.claude/skills/sprite/SKILL.md`
records that the "Niagara graph is unreachable by MCP/Python" claim was wrong *twice*
(2026-07-25, then again 2026-07-26) before the correct answer was found —
`NiagaraToolsets.NiagaraToolset_System` reaches the emitter/renderer stack from a live
editor session and is verified end-to-end for renderer properties (`SetRendererData`,
used to set Sub UV to 8x4). What's verified so far is **renderer properties**, not the
**particle-spawn module graph** where the actual bug lives — the toolset's own
description claims "module creation and modification" and "dynamic-input chain
traversal via `GetDynamicInputChain`," which sounds like exactly the right shape of
tool for rebinding a dynamic input to `Engine.ExecIndex`, but nobody has pointed it at
this specific bug yet. Given the track record of this exact claim being wrong before,
I'm not asserting it can fix this — I'm flagging that a cheap, low-risk test (point
`NiagaraToolset_System` at a **duplicate** of `NS_Swarm` and try to read/rewrite the
`Set Particles.Position` module's index input) would collapse most of the scheduling
uncertainty in §5 for about an hour of work, before anyone commits to a "needs a
human" timeline.

**Two prior attempts already ruled out**, so the plan doesn't have to re-litigate them:
- **Instanced static meshes** (`GATE1-FUN-PROTOTYPE.md` §3a "gotchas") were tried first
  and silently failed — every stock engine material lacks
  `bUsedWithInstancedStaticMeshes`, so UE substitutes the default lit material and logs
  only a warning; units rendered black in a near-black level. Fixable with a
  project-authored material, but the owner's decision is Niagara, not ISM, so this is
  dead unless Niagara turns out to be a dead end too.
- **`DrawDebugPoint`** draws nothing in a `-game` session (verified by screenshot); that's
  why the interim renderer uses `DrawDebugSolidBox` and pays the cost in §1.

## 3. What's already built (not zero-cost work remaining)

The CPU-side half of the sprite bridge is done and sitting unused behind
`Swarm.DebugRender 1`'s early return in `ASwarmRenderActor::Tick`:

- **Facing resolution**: `SwarmFacing::IndexFromDir`/`ColumnFor` (`Mass/SwarmFragments.h`)
  — world facing quantised to 32 steps, computed once per entity per frame in the
  Integrate pass (`SwarmProcessors.cpp`'s `ResolveFacing`), already live and feeding the
  debug-box renderer's shading direction today.
- **Sheet decode**: `SwarmSheet::CellFor` — one shared column/row -> cell function, so the
  Unit Cam and the Niagara bridge can't drift on what a cell means.
- **Packing**: `SwarmRenderPack` — anim byte + size roll + facing index packed into the
  existing `TArray<int32>` the render buffer already carries, no new array.
- **The push loop itself**: `SwarmRenderActor.cpp`'s Niagara branch (the code after
  `if (bDebugRender) { ...; return; }`) already computes `SubImageScratch` from the packed
  buffer and calls `SetNiagaraArrayPosition`/`SetNiagaraArrayFloat`/`SetVariableInt` every
  tick — it just never executes while `Swarm.DebugRender` stays at 1.

None of this needs to be re-derived. **Cutting over is not "build the bridge," it's
"decide what the debug-box renderer did that the sprite sheet still can't."**

## 3a. The mechanism for directional shading: §4a's light-bucket plan, not a shader term

Worth naming explicitly, because it's the answer to two different questions this
proposal has to cover, not just a background fact: **how does the sprite path
reproduce the flame-lit/dark directional shading (§4a) in a way that's also
dither-safe (art's requirement, §7)?**

`docs/RENDERING-LIGHTING.md` §4a specs this as **frame selection, not shading** —
"directional light in pixel art is authored, not computed." The plan was to extend the
SubUV index with a **light bucket**: quantise the angle between a unit's facing and the
direction to the bearer into 4 buckets (lit-from-behind / front / left / right), and
select a different pre-drawn sprite frame per bucket, the same way frame selection
already picks a walk frame. §4c's dither-anchoring lean depends on this: a *baked*
frame is authored art, sprite-anchored by construction, and cannot misalign with a
world- or screen-anchored dither pattern because nothing about it is computed against
world/screen position at render time. A *computed* per-pixel or per-vertex directional
term, by contrast, is exactly the kind of thing that can drift out of sync with a
dither pattern anchored somewhere else — which is the failure mode art is asking this
proposal to rule out.

**This is the mechanism I'd recommend for satisfying art's requirement**, not a
shader-computed alternative — it's cheaper (CPU-side `atan2`, no shader work, per §4a),
it's the one the design doc already specced, and it's dither-safe by construction
rather than by careful shader math that has to stay correct forever.

**The catch, and it's the same one flagged to `art` in §7:** the plan was never built
as specced. The current `SwarmSheet` (r2, 2026-07-26) spent the column axis on full
8-direction facing instead of a 4-bucket light term, and dropped the hit-frame cell to
make room. There is no free axis left for a baked lit/dark variant on top of that. Two
ways to reconcile it, both real sheet-authoring work, not just code:
- **Fold light-bucket into the existing facing axis** — instead of 8 raw facing
  columns, encode facing *and* light state together (e.g. author each of the 8
  directions twice, once lit and once dark, doubling columns to 16, or accept coarser
  facing to make room). Changes what PixelLab has to generate and repacks the sheet.
- **Add a light-bucket row dimension** on top of the existing walk/team rows (currently
  4 rows: brood walk0/1, retinue walk0/1) — multiplies row count by the bucket count
  (4), so 4 rows becomes 16. Same sheet-size consequence from a different axis.

**Correction, now that `art`'s task #15 spec exists (`docs/art/brood-approach-rim.md`,
2026-07-26) — don't conflate this with the light-floor/rim mechanism below.** I'd
speculated the floor gap would need the same sheet-space treatment as this front/back
bucket. It doesn't: `art` deliberately routed the floor/rim mechanism through a
**material remap driven by a per-particle scalar** instead, specifically to avoid
adding sheet cells (§4c-below has the real numbers). The two are genuinely different
problems — this section is about *angle* relative to the bearer (front vs. back), the
floor/rim is about *distance* from the flame — and only this one, the angle-based
directional shading, actually needs sheet space if it's built as specced. Also worth a
factual correction: `art`'s spec describes the current sheet as "8×8"; the code
(`Mass/SwarmFragments.h`, `SwarmSheet::Rows = 4`) says **8×4** — I'm going with what I
read in source. Doesn't change either party's argument, but the row-doubling math above
should be read against 4 rows today, not 8. **Confirmed with `art` directly (2026-07-26):
their fallback (2 new rows if the per-particle remap scalar proves impractical) is this
exact 8×4 -> 8×8 row-only fallback, not a new ask — the column axis (facing) stays
untouched either way.**

## 3b. Dither anchoring has two independent sources — baking the light-bucket only fixes one

`art` corrected an assumption in my first draft (2026-07-26): baking §3a's light-bucket
into selected frames removes *one* source of dither misalignment (a rotating,
per-pixel-computed shading boundary), but there's a **second, separate source that
exists regardless of how directional shading gets built**: the plain distance-based
post-process light-lift (`atten * FlameIntensity`, §4b.2/4b.4) is a continuous,
computed, per-pixel term applied to the *whole scene*, sprites included. Any sprite
whose lifted luminance lands near a dither threshold samples the Bayer pattern at its
own screen/world position — if that anchor is world-space, two units at the same
logical brightness (same distance-to-flame) but different world positions sample
different dither texels and show visibly different dithering. This has nothing to do
with rotation or facing; it happens even with the light-bucket fully baked and even
with `Swarm.UnitShading` conceptually off.

**This means an anchor fix is needed independent of whether the light-bucket ships at
all**, as soon as *any* sprite is lit by the flame's distance falloff under the
existing post-process — which is to say, from the moment cutover happens, not from the
moment §3a's directional shading lands. That changes how I'd sequence it: this isn't
bundled with the "desired, not a hard gate" light-bucket item in §5 any more; it's
closer to the floor gap in urgency, because it's a visible-from-day-one consistency
problem on the base sprite render, not an optional feature.

Two valid fixes, per `art` — pick whichever is cheaper to build, not decided here:
1. **Object/sprite-space dither anchoring** for units (§4c's existing lean) — sample
   the Bayer pattern in sprite-UV space instead of world/screen space.
2. **Exempt sprites outright from the post-dither pass** — apply the light-lift's
   brightness shift but skip the Bayer step for sprite pixels specifically, so units
   render at a clean quantised value with no dither band to misalign in the first
   place.

Both are M_Swarm/M_PP_Demichrome-side shader work, not Mass/Niagara bridge work — added
to §5's cost table as its own item, separate from the light-bucket sheet cost.

**Two independent, composable primitives, not one system — worth being explicit about
this because they'll likely land on different timelines** (`art`'s framing): bucket
selection (§3a) picks *which* already-correct frame shows, based on facing; the
approach-rim remap (§7) shifts *that* frame's value up or down one step, based on
distance/leash state. Neither touches the dither pass — the dither-anchoring fix in
this section is a third, separate concern that both of them are downstream of once
sprites exist at all.

**Open question, not yet confirmed either way:** `art`'s floor-mechanism spec (§7)
covers *brood* only. The retinue mirror case — staying legible near the leash edge,
the sprite-path equivalent of `Swarm.UnitLightFloor` — is *assumed* already covered by
the existing `dim_shift` entry in `palette.json`, but `art` was explicit that this
wasn't audited, only inferred. I'm flagging it here rather than assuming it's solved:
someone needs to confirm the retinue leash-dim behavior actually exists on the sprite
path (or spec it, if it doesn't) before cutover, the same way brood's floor needed a
spec before it existed.

## 4. Interim: `Swarm.UnitShading 0` is a free stopgap, with a real cost

No code change, already shipped as a CVar. From §1: 1.57x-2.58x draw-ms reduction,
growing with entity count. It's a legitimate thing to flip today if the team needs
headroom before the graph is fixed.

**Cost, stated plainly, not glossed over:** it drops the front/back directional
shading — the flame-lit/dark-hemisphere read that `docs/RENDERING-LIGHTING.md` §4a
built and gated (`G1`: "does it read as a carried light or a lens vignette"). That's a
readability feature the art direction earned, not free set dressing; turning it off to
buy frame time is a visible downgrade, and per §1 it still doesn't clear the budget
past ~1930 entities. Recommend stating this to the owner as "buys time, not headroom
at the counts the game actually wants," not as a solved interim.

## 5. Sequencing and cost — honest, not padded

> **Historical — superseded by the RESOLVED box in §2.** The best/worst-case split below
> was written against a graph-rewiring diagnosis that never happened; the actual fault
> was a one-line `SimTarget` property (`GPUComputeSim` → `CPUSim`), found and fixed the
> same day this document was drafted. Kept as the record of what this proposal budgeted
> before the real cause was known — the emitter-graph line in the table after it is
> stale for the same reason.

**Best case:** the `NiagaraToolset_System` module-editing capability (§2) reaches the
broken binding directly — this could be under a day once someone (human or the
right MCP call) actually looks at the graph.

**Worst case:** it doesn't, and a human has to open `NS_Swarm` in the Niagara editor,
diagnose the module graph, and rewire it by hand. Historically this class of fix — one
or two dynamic inputs bound wrong — is a small edit *once found*; the time sink is in
diagnosis, not application, because nobody has looked at the graph yet (§2). Budget
**up to a day** for this leg if it needs a human.

Beyond "the emitter renders something," cutover-readiness needs:

| Item | Blocked on | Rough cost | Blocks cutover? |
|---|---|---|---|
| Emitter graph fix (§2) | diagnosis (human or MCP test) | half a day - 1 day | yes — nothing else matters until this renders |
| Light-floor / approach-rim mechanism | nothing — `art` has now specced it (`docs/art/brood-approach-rim.md`, §4c below) | **Small, now that the spec exists — I overestimated this earlier.** One new per-particle float array (`ShiftAmount`) pushed the same way `SubImages` already is (precedented pattern, `SwarmRenderActor.cpp`'s Niagara push loop); one signed-shift remap node in `M_Swarm`, shared with retinue's existing leash-dim case; revert brood's body fill in the existing sheet rows 0-1 from Steel back to Dark (asset edit, not new cells). No sheet resize in the primary plan. Rough guess: **half a day to a day** once the emitter itself renders. Fallback (only if the per-particle scalar proves impractical in Niagara) does cost 2 new rows (8×4 → 8×8) — art's spec has that path scoped too if needed. | **yes** — §4b.5/G5 has no sprite-path equivalent today; shipping without it either vanishes or never-fades the tide |
| Brood light-**ceiling** parity (new gap, flagged by `art` 2026-07-26, not part of #15) | unscoped — nobody has written this spec yet | unknown; likely the same class of fix as the row above (a per-team clamp on the material path) but not yet designed | **should preserve, per `art`** — the debug renderer's `BroodLightCeil` (brood held below retinue at every distance) has no sprite-path equivalent; without it a brood deep in the pool could read as bright as an adjacent soldier. Not blocking in the sense of "the tide is invisible," but is the same category of "sprite path can't yet do something the renderer being deleted could" |
| **Base dither-anchor fix** — the always-present light-lift source (§3b), independent of any feature | owner decision on §5 L5 (world/sprite-UV/screen anchor) plus a build choice between object-space anchoring and outright sprite dither-exemption (§3b) | shader-side work in `M_Swarm`/`M_PP_Demichrome`, not the Mass/Niagara bridge — unscoped in engineer-days until the anchor-vs-exempt choice is made, but structurally small (one sampling change or one pass-skip, not new data flowing from Mass) | **yes, per `art` (2026-07-26 clarification)** — this exists the moment any sprite renders under the flame post-process, not just when the light-bucket feature ships; reclassified from "bundled with the light-bucket item" to its own cutover-relevant line |
| Dither/shading anchoring — the §4a light-bucket mechanism specifically (§3a) | owner decision on `docs/RENDERING-LIGHTING.md` §5 L5 (shared with the row above) | genuinely needs sheet space if built as specced (see §3a) — doubling columns or adding a light-bucket row dimension, on top of whichever anchor is chosen. **Not a cutover blocker the way the floor or the base dither fix above are** (see re-classification below) but real cost if pursued | see re-classification below — a requirement *on the mechanism if built*, not a cutover gate itself |
| Retinue leash-dim on the sprite path — confirm, don't assume (§3b) | nobody has audited this yet | small if it turns out to already be covered by the existing `dim_shift` `palette.json` entry; a short spec + the same remap-node pattern as brood's rim if it isn't | **should confirm before cutover** — `art`'s floor spec covers brood only; retinue's leash-edge legibility (the sprite-path equivalent of `Swarm.UnitLightFloor`) is assumed solved, not verified |
| Per-unit size jitter for sprites | nothing (known gap, documented) | ~half a day (new per-particle float array + graph edit) | no — visual downgrade only (uniform size vs. today's jittered debug boxes), can ship after cutover |
| Hit-flash on sprites | nothing (deliberately deferred, `SwingBit`/`HitFlashBit` retained but undecoded) | out of scope by owner call already | no |
| 8-direction walk animation | art pipeline (`sprite` skill), ~8 PixelLab generations | non-blocking, separate track | no — non-south units are static-while-walking until this lands, a quality gap not a functional one |

**Re-classifying the front/back directional shading (§3a) against the floor/rim, now
that they're both concretely scoped:** team-lead's original brief named one hard
cutover blocker — the light-*floor* gap (§4b.5/G5, "the tide vanishes or never
fades"). That's now cheap (above). The front/back light-*bucket* (§3a) is a different,
real feature the debug renderer has and the sprite path doesn't yet — losing it at
cutover is a visible downgrade, same category as size jitter — but it isn't the kind of
correctness gap the floor was (nothing vanishes or fails to read without it, the flame
just stops looking directional). I'd treat it as **desired before cutover, not a hard
gate**, and recommend the owner make that call explicitly rather than have it default
to "blocking" by inheriting the floor's urgency. If it *is* pursued, art's dither
constraint (§7) still applies and the sheet-space cost in §3a is real.

**Total, honestly, net effect of both rounds of correction:** the floor mechanism
turned out cheap (good news); the base dither-anchor fix turned out to be a real
cutover blocker I'd originally under-scoped as "bundled with an optional feature"
(the opposite direction of correction). Net: still looking like **a few days to just
over a week** if the emitter-graph diagnosis (§2) goes well and the anchor-vs-exempt
build choice for §3b is quick to make — the graph fix and that one build choice are
now the two real remaining unknowns, not the floor work. The light-bucket sheet work
(§3a) stays costly-but-deferrable if the owner doesn't require it before cutover.
**Still not compressing the graph-fix estimate further without the one-hour test in
§2** — that remains the single biggest lever on the total, unchanged by any of this.

## 6. What we lose in the interim, what could regress at cutover

**Interim (now, however long the fix takes):**
- The `UnitShading 0` stopgap (§4) costs the directional-lighting read if the team
  takes it. Staying on `UnitShading 1` costs the frame budget instead (§1). No interim
  configuration clears the real entity-count gate — that's the whole reason for this
  refactor, not a caveat to it.

**At cutover, confirm before calling it done:**
- **Size jitter** (`Swarm.BroodSizeJitter`/`RetinueSizeJitter`) currently only affects
  the debug-box renderer and Unit Cam. Cutting to sprites with no follow-up makes every
  brood the same size again — a real, visible regression from today's look unless
  §5's size-array item ships alongside or soon after.
- **Hit flash** already doesn't read on the sprite path (deliberate, prior owner call)
  — not a new loss, but worth re-confirming the owner still accepts that at cutover
  time, since debug-box testing has been masking it.
- **Only the south walk cycle animates**; the other seven directions hold their idle
  pose while moving, per `SETUP-EDITOR.md` §1's documented gap. Debug boxes don't have
  this problem (they don't animate walk frames visually beyond the flip bit), so this
  is a **new**, cutover-introduced regression until the 8-direction walk fill lands.
- **The light-floor mechanism is no longer an open gap — it's a landed, cheap spec
  (§7).** Cutover still shouldn't happen before it's actually built (or ships with an
  explicit owner-accepted "no floor for now" call), but this is no longer the
  expensive unknown I first flagged it as.
- **Brood light-ceiling parity is a fresh, still-open gap** (art flagged it 2026-07-26,
  outside #15's scope): the debug renderer keeps brood dimmer than retinue at every
  distance (`BroodLightCeil`); the sprite/material path has no equivalent, since the
  post-process light-lift doesn't know about team. Nobody has specced a fix yet — worth
  tracking as its own item, not folded silently into the floor work.

## 7. Requirements from `art` (direct coordination + task #15 spec, `docs/art/brood-approach-rim.md`, 2026-07-26)

Confirmed directly with `art`, superseding my earlier guesses in the first draft of
this section:

- **Dither anchoring is a hard requirement, not a preference**, carried unchanged from
  team-lead's original brief: instancing fixes draw-call count (§1) but not dither
  misalignment on its own. The sprite path must either stop rotating the shading
  boundary or anchor units' dither in object/sprite space — §4c already leans
  sprite-UV for units. Applies to whichever directional-shading mechanism gets built
  (§3a) — costed there.

- **The light-floor mechanism is specced, and it is not a baked-sheet rim.** `art`
  corrected the literal brief ("thin Steel-only rim/contour") to a **full-silhouette
  value swap** instead, with the math for why: at the measured camera settings a brood
  resolves to roughly 10px on screen, and a 1-2px authored contour lands sub-pixel at
  that scale — it would flicker per-unit as a function of sub-pixel offset under
  `Nearest` filtering, not read as signal. A whole-body Dark-to-Steel swap has no thin
  geometry to lose and survives any downscale ratio.
  - **Mechanism:** one per-particle `ShiftAmount ∈ {-1, 0, +1}` float, computed
    CPU-side in the same loop that builds `SubImageScratch` today and pushed the same
    way (`SetNiagaraArrayFloat` — precedented, not a new pattern). One signed-shift
    remap node in `M_Swarm`, reusing the primitive `docs/data/art/palette.json` already
    specs for retinue's leash-dim case (`dim_shift`, one value down) — this is that
    mechanism's mirror (one value up for brood), same material, opposite sign, shared
    rather than duplicated.
  - **Trigger:** a hard threshold, not a curve — `+1` when
    `Swarm.FlameRadius <= Dist < BandOuter` (outer bound defaults to reusing
    `SwarmLeash::Radius`, overridable via a new `Swarm.BroodRimOuter` CVar), `0`
    outside that band. Deliberately discrete, not coupled to `Swarm.FlameFalloff`'s
    continuous curve — the two compose rather than share math.
  - **No new hex, no new sheet cells in the primary plan.** The remap moves between two
    values already in the locked four (Dark -> Steel). Fallback if the per-particle
    scalar proves impractical in Niagara: 2 new rows (8x4 -> 8x8, next power of two),
    using `SwarmAnim`'s one remaining free bit (`1 << 7`).
  - **Requires an asset edit**, not new art: revert brood's existing sheet rows
    (`RowBroodWalk0`/`1`) from the current Steel-dominant fill back to Dark. This is
    load-bearing — a Steel rim on an already-Steel body has no contrast — and it
    resolves an open question already on record in `docs/RENDERING-LIGHTING.md` §4b.5
    (the shipped brood body isn't Dark-dominant, and that wanted an owner ruling),
    contingent on this mechanism shipping to replace the reason the workaround existed.
  - Costed in §5. This is the item I originally guessed would need new sheet space and
    a 1-3 day unscoped cost — it doesn't, and it's cheaper. Correcting that here rather
    than leaving the wrong estimate standing.

- **Brood light-ceiling parity is flagged but explicitly not part of #15** — see §6.
  Naming it here so it doesn't quietly fall out of scope between two specs that each
  assumed the other covered it.

## 8. Recommendation

1. **Void, per the RESOLVED box in §2 — kept as the record of the original plan.**
   ~~Spend the one hour from §2 testing whether `NiagaraToolsets.NiagaraToolset_System`
   can read/rewrite the particle-spawn module's index binding on a **duplicate** of
   `NS_Swarm` (never the live asset) before scheduling anything further — it's the
   cheapest possible reduction of the biggest unknown in this document.~~ The graph was
   never broken; the fix was a one-line `SimTarget` change. No module-graph test was
   ever run or needed.
2. In parallel, decide whether `Swarm.UnitShading 0` ships as an interim default. It's
   free and real, but per §4 it is not a fix — say so to whoever signs off on it.
3. `art`'s task #15 has landed (`docs/art/brood-approach-rim.md`) and the light-floor
   mechanism is cheap and buildable once the emitter renders — build it alongside the
   graph fix rather than treating it as a follow-on.
4. **The §5 L5 dither-anchor decision now gates a real cutover blocker, not just an
   optional feature** (§3b) — resolve it, plus the object-space-anchor-vs-dither-exempt
   build choice, before cutover, not after. This is separate from and more urgent than
   the front/back light-bucket mechanism (§3a), which still costs real sheet space and
   can reasonably wait for an explicit owner call on whether it's required before
   cutover or can ship after (§5's re-classification).
5. Confirm retinue's leash-dim behavior on the sprite path (§3b) rather than assuming
   the existing `dim_shift` `palette.json` entry already covers it — `art` was explicit
   this wasn't audited, only brood's side was specced.
6. Get a spec written for brood light-ceiling parity (§6/§7) before cutover, even if
   it's the last thing implemented — it's currently the only "what the sprite path must
   preserve" item with no owner and no spec.
7. Once the emitter renders *something*, re-run the §1 benchmark against it before
   declaring victory. Expect the shape of the win to be structural, not incremental —
   GPU-instanced sprite rendering doesn't pay a per-instance CPU submission cost the way
   `DrawDebugSolidBox` does, so I'd expect the fix to escape the "draw ms = frame ms"
   regime in §1 rather than just improve its slope. That's a prediction grounded in how
   Niagara rendering is structured, **not a measured number** — treat it as the reason
   to expect a big win, not as a substitute for re-measuring once there's something to
   measure.

## 9. Measured after the fix (2026-07-26)

Standalone `-SwarmBench` run against the **sprite path** (`Swarm.DebugRender 0`, `SimTarget: CPUSim`,
ParticleUpdate `SetVariables` disabled), retinue held at 100. Compared against the debug-box
baselines from §1 (A = `UnitShading 1`, B = `UnitShading 0`); all figures are frame ms.

| brood | A frame ms | B frame ms | **sprite frame ms** | vs A | sprite draw ms | sprite game ms |
|---|---|---|---|---|---|---|
| 500 | 4.85 | 3.10 | **2.56** | 1.9x | 2.52 | 2.05 |
| 1000 | 14.62 | 7.59 | **3.87** | 3.8x | 3.73 | 2.80 |
| 2000 | 40.66 | 18.43 | **7.21** | 5.6x | 5.41 | 6.06 |
| 5000 | 132.33 | 57.15 | **15.92** | 8.3x | 2.81 | 15.91 |

The 10000 row was not captured — the game window was closed during that step (clean exit, no crash).

**§8.7's prediction held: the win is structural, not a change of slope.** Draw ms has decoupled from
frame ms. At 5000 brood, draw is 2.81ms inside a 15.92ms frame while `game` is 15.91ms — the render
bridge no longer dominates and **the game thread is now the constraint**.

Two consequences for planning:
- The 60fps ceiling moved from **~1,930 total entities** (§1's interpolated figure for the flat
  debug-box renderer) to **~5,100** (100 retinue + 5000 brood at 15.92ms).
- **Finding #2 is no longer invisible.** §1 deferred the uncapped per-neighbour `GetSafeNormal` +
  K-insertion-sort in the combat pass because "the game thread stays under 27ms in every row." It is
  now the thing being waited on, and is the correct next target.

**Caveat on these numbers:** `InitializeParticle.Lifetime` is still the `0.5` placeholder with
`InterpolatedSpawnMode: Interpolation`, so roughly 30 particle generations coexist. These figures are
therefore paying real overdraw; finalising the lifecycle should improve them further.

## 9.1 Lifecycle finalisation attempt (2026-07-26) — half landed, half blocked by a toolset bug

The intended finalisation was two changes: `InterpolatedSpawnMode` -> `NoInterpolation`, then
`InitializeParticle.Lifetime` down from `0.5` to just over one frame. Only the second one landed.

**`Lifetime` 0.5 -> 0.05: done, saved, verified by screenshot.** Set via
`NiagaraToolset_System.SetStackInputData` and confirmed on disk — `is_dirty` flipped true, the save
changed the `.uasset` hash, and `git status` shows it modified. `-SwarmBench` re-run below confirms
brood sprites still render (not "NOTHING" — the danger called out for a too-short lifetime): checked
by screenshot mid-run (`SwarmDebugShot00030.png` / cropped `shot30_crop.png`), small brood sprites
visible arcing around the flame pool's edge.

**`InterpolatedSpawnMode` -> `NoInterpolation`: BLOCKED — do not retry with `SetEmitterData`.**
`NiagaraToolset_System.SetEmitterData` accepted the write and even echoed it back on an immediate
`GetEmitterData` read — but it never actually stuck: `AssetTools.is_dirty` stayed `false`,
`GetSystemCompileState` kept reporting `ParticleSpawnScriptInterpolated` as the emitter's active spawn
script throughout, and a later `GetEmitterData` call reverted to `InterpolatedSpawnMode: Interpolation`
with nothing else touching the asset in between. This is the same *class* of bug as the
Custom-node-code silent no-op documented elsewhere for material graphs — a reflection-based property
write that bypasses whatever hooks Niagara needs (`PreEditChange`/`PostEditChange`/dirty-marking,
and for this specific field, a script recompile) to actually take effect. `SetStackInputData` (used
for `Lifetime` above) does **not** have this problem — it correctly dirties the package.

**This blocked attempt is also the likely cause of an editor crash mid-session** (PID changed
across the attempt, `LiveCodingConsole` PID changed too, and the relaunched editor showed UE's own
"1 asset editor was open when the editor quit unexpectedly" dialog — a genuine unclean-exit signal,
not a scripted restart). Causation isn't fully certain — the crash surfaced on a *second*,
full-property-blob retry of the same `SetEmitterData` call, one call after a partial-blob attempt
that hadn't crashed — but the timing correlation is tight enough that **`SetEmitterData` on
`InterpolatedSpawnMode` should be treated as a third confirmed crash-risk pattern** in this Niagara
toolset, alongside `SetStackInputData` immediately after a `SimTarget` change (`CAMERA-SCALE-HANDOFF.md`
§5). No data was lost — `Lifetime` was already saved to disk before this happened — but flagging it so
the next session doesn't rediscover it the same way. A UI-driven fix (toggling "Interpolated Spawning"
in the emitter's Properties panel via `SlateInspectorToolset`) was attempted but the Niagara System
Overview's emitter-stack rows are custom-painted, not exposed as clickable refs in the generic Slate
accessibility tree, and this was not pursued further given the crash risk already observed.

**Re-run `-SwarmBench` with only the `Lifetime` fix landed** (retinue 100, standalone `-game
-SwarmBench`, same build as §9's baseline — no C++ rebuild between the two runs):

| brood | §9 sprite frame ms (Lifetime 0.5) | **this run (Lifetime 0.05)** | game ms | draw ms | gpu ms | fps |
|---|---|---|---|---|---|---|
| 500 | 2.56 | **2.07** | 2.04 | 1.69 | 0.90 | 483.0 |
| 1000 | 3.87 | **2.58** | 2.57 | 1.59 | 1.27 | 387.6 |
| 2000 | 7.21 | **5.40** | 5.40 | 1.65 | 1.29 | 185.2 |
| 5000 | 15.92 | **14.82** | 14.82 | 1.84 | 1.75 | 67.5 |
| 10000 | not captured | **29.98** | 29.98 | 1.91 | 3.41 | 33.4 |

The 10000 row §9 was missing is captured here. Improvement across every row despite
`InterpolatedSpawnMode` staying stuck on `Interpolation` — cutting the coexisting-generation count via
`Lifetime` alone was worth doing even without the mode flip (roughly 20-33% off frame time at
500-2000 brood, smaller but still real gains higher up). `draw` stays flat at ~1.6-1.9ms across the
whole sweep and `frame` == `game` at every row — confirms §9's finding again: **the render bridge
is not the bottleneck at any brood count tested; the game thread (sim + combat) is.** The 60fps
ceiling is still around the same ~5,100 total entities (5000 brood row: 14.82ms, comfortably under
16.6ms; 10000 brood: 29.98ms, well over).

**Net for the next session:** `Lifetime` is finalised. `InterpolatedSpawnMode` is not, and stayed on
the placeholder `Interpolation` mode for these numbers — so there is still headroom to capture once
someone finds a safe way to flip it (most likely: drive the emitter Properties checkbox through the
Niagara System editor UI directly rather than through `SetEmitterData`, with the System Overview
graph's custom-painted rows worked around some other way — or a human does it by hand).
