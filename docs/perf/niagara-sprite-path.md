# The Niagara sprite path — what NS_Swarm decodes, and where each number lives

Written 2026-07-29 (task-059), split 2026-07-30 (task-085). Two emitters, two
renderers, two draw calls, one per side — team (retinue + ten knights) and enemy (nine
brood looks). This file is the answer to "the horde is drawing the wrong frame" and to
"which of the places that have to agree did I forget". Read §1-§2 before changing either
atlas.

## 0. Why two atlases (task-085)

The single atlas was never about material cost — it was about draw calls. But task-059
measured the Niagara render cost at `draw_ms 0.001` regardless of brood count, with
`game_ms` (the Mass sim) climbing 11→29ms across 1k→40k brood — rendering is free, the
sim is the entire frame. So going from one draw call to two is not a regression, and
§7's bench table below confirms it: `draw_ms` still rounds to 0.00 at 1k/10k/40k with
both emitters live.

What splitting buys is churn isolation: before task-085, changing the enemy roster meant
repacking the ONE texture that also held the team, then moving `SwarmSheet::Rows`, then
the one Sub UV field — three things that had to agree, and a mismatch was silent
(exactly the trap `SwarmFragments.h` warned about since 2026-07-26, and what nearly sank
task-059). Split into `SwarmSheet::Team` / `SwarmSheet::Enemy`, each with its own grid,
texture, material and emitter, and the two sides can never drift against each other —
only against themselves.

**Design, settled — do not re-litigate:** two emitters inside `NS_Swarm`, not two
renderers on one emitter (both would process every particle, needing a visibility hack
to hide the wrong half) and not one material sampling two textures via a dynamic
parameter (the renderer's built-in SubUV decodes against ONE grid; two grids means
hand-rolling UV math in the shader to save 0.001ms of already-free render cost).

## 1. FOUR places have to agree about EACH grid — and there are now two grids

| # | Team | Enemy |
|---|---|---|
| 1 | `SwarmFragments.h` — `SwarmSheet::Team::Variants`/`Rows` | `SwarmSheet::Enemy::Variants`/`Rows` |
| 2 | `docs/data/art/requests/team-units.json` `output.grid` + `frame_map` | `.../enemy-units.json` |
| 3 | imported texture `T_Team_2bit` | `T_Enemy_2bit` |
| 4 | `NS_Swarm` → emitter **Team** → Sprite Renderer → `SubImageSize` = **{8, 34}** | emitter **Swarm** → `SubImageSize` = **{8, 18}** |

The team grid went 8×22 → **8×34** in task-126 (the six archer keeps). The enemy grid is
untouched — that independence is the whole point of the split.

`Scripts/art/check_brood_variants.py` checks 1 against 2, for BOTH sides, and prints
what each Sub UV field has to hold. It cannot read 4 — verify that one by reading it
back from the asset (`NiagaraToolset_System.GetRendererData`), never by assuming a write
took (`SetRendererData`/`set_properties` return success on writes that did not land).

A mismatch is SILENT: every unit on that side simply wears the wrong cell.

**Emitter naming note:** the enemy-side emitter is still literally named `Swarm` inside
`NS_Swarm` — it is the original pre-split emitter, repointed (material's texture swapped
`T_Swarm_2bit` → `T_Enemy_2bit`, `SubImageSize` narrowed 8×20 → 8×18) rather than
recreated, so it kept its already-wired `User.Positions/SubImages/Colors/Sizes/Count`
parameters and needed no graph rewiring. The team-side emitter is named `Team` and is
new.

## 2. The atlas layouts

**Enemy (`T_Enemy_2bit`, 8×18) — unchanged since task-059 apart from losing the trailing
retinue rows:**

```
      col:   0     1     2     3     4     5     6     7
             S    SE     E    NE     N    NW     W    SW
  rows 0-1   brood variant 0 (walk0, walk1)   state00_base
  rows 2-3   brood variant 1                  state01_sump   <- shipped alone before task-059
  rows 4-5   brood variant 2                  state02_bell
  rows 6-7   brood variant 3                  state03_stalk
  rows 8-9   brood variant 4                  state04_wedge
  rows 10-11 brood variant 5                  state05_ridge
  rows 12-13 brood variant 6                  state06_crown
  rows 14-15 brood variant 7                  state07_twin
  rows 16-17 brood variant 8                  state08_slug
```

**Team (`T_Team_2bit`, 8×34) — task-085 landed rows 0-21, task-126 appended rows 22-33.
TWO SUB-TABLES share this one grid. Variant 0 is the pre-split retinue look, kept at
index 0 so the pre-split default appearance did not move; variants 1-10 are the ten
judged knight keeps from `knight-melee-v1`/`knight-melee-v2`; variants 11-16 are the six
`archer-medieval` keeps (task-081's silhouette pipeline judged separation on all of
them — not re-judged at pack time):**

```
  --- SPEARMAN block (Swarm.TeamVariantWeights, 11 entries) ---
  rows 0-1   variant 0  retinue        (the pre-split look, pale grey-green)
  rows 2-3   variant 1  knight-v1      v1_narrowguard
  rows 4-5   variant 2  knight-v2      v2_lanceout
  rows 6-7   variant 3  knight-v3      v3_shieldbreak
  rows 8-9   variant 4  knight-v4      v4_overhead
  rows 10-11 variant 5  knight-v6      v6_simplecolumn
  rows 12-13 variant 6  knight-v7      v7_barestance
  rows 14-15 variant 7  knight-v8      v8_heavycloak
  rows 16-17 variant 8  knight-v10     v10_bracedstaff
  rows 18-19 variant 9  knight-v11     v11_midguard
  rows 20-21 variant 10 knight-v13     v13_maceraised
  --- ARCHER block (Swarm.ArcherVariantWeights, 6 entries) ---
  rows 22-23 variant 11 archer-v1      v1_narrowstrung
  rows 24-25 variant 12 archer-v2      v2_bowextended
  rows 26-27 variant 13 archer-v3      v3_loosingarm
  rows 28-29 variant 14 archer-v4      v4_quiverreach
  rows 30-31 variant 15 archer-v5      v5_crossbowbrace
  rows 32-33 variant 16 archer-v6      v6_slingwhirl
```

**Seventeen looks index through a four-bit field, and that is deliberate.** The render
int32's variant field (bits 21-24) stops at 15, so it carries the WITHIN-BLOCK index —
0-10 for spearmen, 0-5 for archers — and `SwarmRenderActor.cpp`'s pack loop adds
`SwarmSheet::Team::ArcherVariantBase` (11) when the entity's squad byte says
`EUnitType::Archers`. Which block an entity draws from is decided by unit type, not by
its weight roll: an archer can never draw a knight row and a spearman can never draw an
archer row. Widening `VariantMask` was rejected — the offset is one line in the bridge
and it lets either block grow past sixteen looks with no repack of the int32.

**This ordering is combat-meaningful, not just a packing choice.** task-095 binds
per-sub-type combat stats to exactly this index (docs/data/art/team-variants.json is
the document of record — reorder there AND repack AND update task-095's stat table
together, or a knight's stats land on a different skin). `docs/data/art/team-variants.json`
`index_note` says the same thing.

`Row = Variant*2 + WalkFrame` on both sides, decoded by `SwarmSheet::Team::CellFor` /
`SwarmSheet::Enemy::CellFor` (`Mass/SwarmFragments.h`) — the same formula, different
variant counts, so there is exactly one place either decode can be wrong.

**walk0 == walk1 in every row on both sheets** — no source character has a generated
walk. Kept anyway for the same reason as before the split: collapsing it forces a second
layout change the day an `animate_character` walk lands.

**Rows are not a power of two, and that is fine** — `SubImageSize` is a pair of floats
and the decode is a ratio; `pixelpipe.py validate` only enforces power-of-two on
COLUMNS (a real Niagara constraint), not rows (folklore, disproved 2026-07-29).

## 3. The packed render int32

Unchanged shape from task-059, but bits 21-24 are now a SHARED field between two
independent tables:

```
  bits 0-7    anim byte (SwarmAnim: walk frame, attacking, flip, team, leash, swing, hit flash)
  bits 8-11   size bucket   — per-entity 0-15 roll
  bits 12-16  world facing  — 32 steps, view-relative column resolved per camera
  bits 17-20  squad byte    — unit index + type, retinue only
  bits 21-24  VARIANT, as a WITHIN-BLOCK index — enemy (0-8), team spearmen (0-10) or
              team archers (0-5). TeamBit (already in bits 0-7) picks the side; on the
              team side the squad byte's unit type picks the block
  bits 25-31  free
```

All three counts fit the same 4 bits (max 15), so neither task-085 (eleven-way team
variety) nor task-126 (a six-way archer block on top, seventeen team rows total) needed a
width change — the archer ROW OFFSET lives in the pack loop instead of in this field.

Nothing above bit 7 is visible to a consumer that masks a single bit or casts to
`uint8` first. The `(uint8)` cast on `CellFor` is load-bearing.

## 4. How a variant is chosen — three independent display-weight tables

`SwarmRenderPack::VariantFromPhase(Phase, Cum, Num)` (unchanged mechanism, shared code,
called with one of three tables depending on `TeamBit` and unit type):

1. `FSwarmJitterFragment::Phase` is a per-entity `FRandRange(0, 10)` fixed at spawn.
2. `Frac(Phase * 0.3819660887)` maps it to 0..1.
3. Mapped through a **cumulative** weight array — weights control frequency, a weight of
   0 retires a look.

`USwarmIntegrateProcessor::Execute` parses ALL THREE CVars once per pass and picks
per-entity on `TeamBit` plus `SwarmSquad::UnitType`. Changing any of them reskins a horde
that is already standing, on the next frame, with no respawn — same as before the split.

| side | CVar | entries | atlas order | defaults documented in |
|---|---|---|---|---|
| enemy | `Swarm.BroodVariantWeights` | 9 | base,sump,bell,stalk,wedge,ridge,crown,twin,slug | `docs/data/art/brood-variants.json` |
| team, spearmen | `Swarm.TeamVariantWeights` | 11 | retinue,v1,v2,v3,v4,v6,v7,v8,v10,v11,v13 | `docs/data/art/team-variants.json` |
| team, archers | `Swarm.ArcherVariantWeights` | 6 | narrowstrung,bowextended,loosingarm,quiverreach,crossbowbrace,slingwhirl | `docs/data/art/team-variants.json` |

**The archer table drives looks only.** Archers short-circuit task-095's knight sub-type
stat binding at `SwarmProcessors.cpp`'s `bArcher` branch and at
`SwarmCombatProcessors.cpp`'s — they read the `Swarm.Archers*` CVars and never reach
`KnightSubtypeRowFor`. Skewing `Swarm.ArcherVariantWeights` is combat-neutral by
construction, unlike `Swarm.TeamVariantWeights`.

**task-127: the mechanism above was proven correct but invisible at default weights.**
Archers are ~20% of recruits split six ways over this table, so any one archer look is
~3% of the army, and the tell (a thin, dark bow arc) is below the size/contrast threshold
that reads in a mass of hundreds — see `docs/perf/evidence/task126/01-...png` (invisible
at default weights) against `02-...png` (visible once skewed). `Swarm.ArcherSizeScale`
(`SwarmRenderActor.cpp`, default 1.4, range [1,3]) is the fix: a per-particle size
multiplier applied to archers only, in the pack loop, on top of `Swarm.RetinueSizeScale`.
Stacks with the existing `bArcher` unit-type test the archer row offset already computes
(both now share one `bArcher` local per entity rather than deriving it twice). Purely a
render-size dial — does not touch `Swarm.Archers*` combat stats, the weight tables, or
the atlas. See `docs/perf/evidence/task127/` for the before/after capture.

**task-130: rung 1 alone still read "mostly by robe colour", not as a block.** Spent the
next two rungs of the same ladder, in order, both:

- **Rung 2, contrast.** `Swarm.ArcherColorLift` (`SwarmRenderActor.cpp`, default 0.15,
  range [0,1]) — an additive brightness lift on the SAME channel `Swarm.BroodAdd` rides
  (`M_Swarm_Team`'s `Emissive = SubUV.RGB * ParticleColor.RGB + ParticleColor.A`, task-084),
  applied to archers only, in the pack loop's existing per-particle colour branch. Verified
  the material actually reads it BEFORE writing the CVar: `get_property_input`/
  `get_expression_inputs` on `M_Swarm_Team` read back `MP_EmissiveColor <-
  Add(Multiply(ParticleSubUV.RGB, ParticleColor.RGB), ParticleColor.A)` — the node is there
  and wired to both terms, so this isn't the material-silently-discarding-colour trap again.
  Chosen as an ADD rather than a bump to `Lit` because `Lit` saturates to 1.0 near the flame
  (where the army actually stands), so a multiplicative lift would read as nothing exactly
  where it matters; the additive term always shows. Rides the same `Swarm.RawNear` camera-
  distance fade as `Swarm.BroodAdd`, so a close-up archer still shows authored robe colour
  rather than a flat lift.
- **Rung 3, mix — a BALANCE CHANGE, not a render dial.** `Swarm.ArcherGrowthWeight`
  (`SwarmCombatProcessors.cpp`) default raised **0.2 -> 0.4**: archers were 20% of recruits
  split six ways (~3% per look), too thin for a block even with size + contrast fixing how
  ONE archer reads. 0.4 roughly doubles the ranged share of the army; Spearmen still claim
  the majority (0.6). This changes gameplay, not just the look — Archers carry
  `Swarm.ArchersMaxHP` 70 against a Spearman's 130 and fight at range instead of melee
  cleave, so a bigger archer share measurably softens the line's HP total. Flagged for the
  owner to argue with, not buried as a free lever.

Both rungs were spent (not just rung 2) because the maths said rung 2 alone couldn't clear
the bar: brightening one archer doesn't create more of them, and the "block" ask is a
population-density read as much as a per-unit-legibility one. `docs/perf/evidence/task130/`
has the before/after pair at DEFAULT settings, same camera and density as task-127's
baseline, plus three camera-diagnostic captures (see the framing note below).

**Framing finding, task-130 (owner ask, not part of the ladder):** the shipped pinned
eye-level camera (`Kindled.Cam.Dist 323`, `Pitch -8.2`, `Fov 45.6`, `OffsetZ 54`) crops the
retinue tight against the bottom edge — only the front rank or two are ever in frame, no
matter how big or bright an archer draws. **Pulling `Dist` back alone does not fix this**
(`docs/perf/evidence/task130/05-diagnostic-dist-only-insufficient.png`, `Dist 900` at the
shipped `Pitch -8.2`): at a grazing eye-level angle the ground plane is nearly edge-on to
the lens, so distance alone just shrinks everyone inside the same thin band rather than
revealing depth. `Pitch` has to open up too — `04-diagnostic-moderate-pitch.png`
(`Dist 900, Pitch -20, Fov 55, OffsetZ 30`) gets two ranks in frame with archers still
identifiable by bow silhouette; `03-diagnostic-overhead-full-retinue.png`
(`Dist 1700, Pitch -45, Fov 60, OffsetZ 0`) gets the whole formation in frame and makes the
archer block unmistakable — the front several ranks are visibly bow-carrying, distinct from
the shield-and-spear ranks behind.

That capture also settles a formation-geometry question the brief raised: **archers are
NOT hidden behind the spear line.** `Swarm.Formation.Archers.Forward` defaults to 40uu
against `Swarm.Formation.Forward`'s ~150-250uu for Spearmen — archers sit CLOSER to the
bearer (and so closer to a camera positioned behind him) than Spearmen do, not farther.
In every diagnostic capture the front-most, camera-nearest rank is the archers, not the
spearmen. The original "archer is invisible" problem (task-126/127) was never occlusion;
it was too few of them, too small, too low-contrast, in a camera crop too tight to show
more than one or two ranks regardless of who stood where. These `Kindled.Cam.*` values are
diagnostic only — the shipped exec-file camera block was restored byte-for-byte afterward
(`ELVTR/Saved/SwarmExecOnPlay.txt` vs the lines above); this task does not own the camera
rig and did not retune it.

`check_brood_variants.py` fails if either compiled CVar default and its weights file
disagree. `Swarm.BroodVariantReport` / `Swarm.TeamVariantReport` log the live histogram
plus the weight string it came from — both fire automatically alongside
`Swarm.DebugShotAfter`.

**Team's default weight table is a first cut, not owner-judged like the brood's.**
`20,8,8,8,8,8,8,8,8,8,8` — a slight edge for the pre-split retinue look, the ten knights
split the rest evenly. It exists to prove the mechanism (several looks visible at once,
a skewed table changes the mix); retune once the owner has seen the army on screen.

## 5. What the bridge pushes, and what the graph does with it

`ASwarmRenderActor::Tick` splits `AnimBits` by `SwarmAnim::TeamBit` into two sets of
scratch arrays (`Team*Scratch`/`Enemy*Scratch`, file-static except the two SubImage
arrays, which are actor members because `TakeDebugShot` reads them back — see the header
doc comment) and pushes each side to its own five User parameters:

| side | Position | SubImage | Color | Size | Count |
|---|---|---|---|---|---|
| enemy (unchanged names) | `User.Positions` | `User.SubImages` | `User.Colors` | `User.Sizes` | `User.Count` |
| team (new) | `User.TeamPositions` | `User.TeamSubImages` | `User.TeamColors` | `User.TeamSizes` | `User.TeamCount` |

Every array is read by its emitter's graph through the same pattern on both sides — a
`Select*FromArray` dynamic input with **Array Sampling Mode = Direct Set**, **Direct
Array Index = `Engine.ExecIndex`**, **Direct Array Mode = Clamp**:

- `InitializeParticle` → `Position Mode = Simulation Position` (position comes from the
  `SetVariables` module below, not here) → `Sprite Size Mode = Uniform`, `Uniform Sprite
  Size` = `SelectFloatFromArray(<side>Sizes)` → `Color Mode = Direct Set`, `Color` =
  `SelectLinearColorFromArray(<side>Colors)`.
- A `SetVariables` (Set Parameters) module right after `InitializeParticle` sets
  `Particles.Position` = `SelectPositionFromArray(<side>Positions)` and
  `Particles.SubImageIndex` = `SelectFloatFromArray(<side>SubImages)`.
- `EmitterUpdateScript` → `SpawnPerFrame`, `Spawn Count` = linked `User.<side>Count`.

Copy that pattern; do not invent a second one. `M_Swarm` (enemy) / `M_Swarm_Team`
(team) are both `Emissive = SubUV.RGB * ParticleColor.RGB + ParticleColor.A` (task-084;
RGB multiplies for distance dimming, A adds for the brood's near-black-body legibility
fix) — `M_Swarm_Team` is a straight duplicate of `M_Swarm` with the `ParticleSubUV`
node's texture repointed, nothing else differs.

### Building a from-scratch emitter (what task-085 actually did for `Team`)

There was no reusable standalone Emitter asset to duplicate — the original `Swarm`
emitter was built directly inside `NS_Swarm`, not inherited from one, and
`NiagaraToolset_System.AddEmitter` requires a template EMITTER ASSET (not an
in-system emitter instance; passing `/Game/Spike1/NS_Swarm.NS_Swarm:Swarm` as a
template fails). The route that worked:

1. `AddEmitter` with `templateEmitter = /Niagara/DefaultAssets/Templates/CascadeConversion/CompletelyEmpty.CompletelyEmpty`
   — a genuinely empty emitter (no modules, no renderer).
2. `AddModule` three times: `SpawnPerFrame` (EmitterUpdateScript),
   `InitializeParticle` (ParticleSpawnScript), `ParticleState` (ParticleUpdateScript) —
   all standard-library module assets under `/Niagara/Modules/...`, same paths the
   reference emitter uses.
3. `AddSetParametersModule` on `ParticleSpawnScript` for `Particles.Position` +
   `Particles.SubImageIndex`, then `SetStackInputData` to assign each a
   `SelectPositionFromArray`/`SelectFloatFromArray` dynamic input.
4. `SetStackInputData` for every `InitializeParticle` static switch (`Lifetime Mode`,
   `Color Mode`, `Position Mode`, `Sprite Size Mode` — all "Direct Set" or "Simulation
   Position"/"Uniform" to match the reference) and its two dynamic inputs (`Color`,
   `Uniform Sprite Size`).
5. Each dynamic input's own sub-inputs (`Array Sampling Mode`, `Direct Array Index`,
   `Direct Array Mode`, and the array-linking input — named `Color Selection Array`,
   `Float Selection Array` or `Position Array` depending on type) are a SECOND level of
   `SetStackInputData` calls, addressed with a two-element `inputNameStack` (e.g.
   `["Uniform Sprite Size", "Float Selection Array"]`). `GetDynamicInputChain` on the
   REFERENCE emitter's equivalent input is how these sub-input names were discovered —
   they are not documented anywhere else and differ by array type.
6. `AddRenderer` (`NiagaraSpriteRendererProperties`), then `SetRendererData` with a
   JSON patch setting `Material` and `SubImageSize` — `SetRendererData` merges rather
   than replacing the whole property blob, confirmed by reading the renderer back
   afterward. `bSubImageBlend` defaults to `true` on a fresh renderer and had to be
   explicitly set `false` to match the reference (hard-cut SubUV, no blend).
7. `GetStackIssues` flagged one real error along the way — `SpawnPerFrame` has an
   unmet `EmitterState` dependency on a from-scratch stack (the reference emitter had
   one because non-empty templates normally carry it). `ApplyStackIssueFix` with the
   offered fix ID added it. `GetStackIssues` also reports transient "compile in
   flight" errors if called too soon after an edit — check
   `GetSystemCompileState().bIsCompiling` first, or just retry.

Every value above was copied from the live `Swarm` emitter via `GetEmitterInputValues`
/ `GetDynamicInputChain` rather than guessed — do the same for any future third
emitter, since the enum names (`ENiagara_ColorInitializationMode::NewEnumerator1` etc.)
are not self-describing from the schema alone.

`Swarm.NiagaraEnsureArrays` (editor-only console command,
`SwarmRenderActor.cpp`) is the only way to create the Array-DataInterface User
parameters themselves (`TeamPositions`/`TeamSubImages`/`TeamColors`/`TeamSizes`, plus
the plain-int `TeamCount`) — MCP `AddUserVariables` still silently no-ops on
data-interface types, unchanged from task-059's finding.

## 6. Simulation space — WORLD, and CPUSim on both emitters

Both emitters are `bLocalSpace: false`, `SimTarget: CPUSim`, `InterpolatedSpawnMode:
Interpolation`, `CalculateBoundsMode: Fixed` with generous fixed bounds — copied
verbatim from the working `Swarm` emitter's settings onto `Team`. **CPUSim is not
negotiable**: `NS_Swarm` drew nothing for a long stretch pre-task-059 because the
original emitter was `GPUComputeSim`.

## 7. Cost (task-085, two emitters)

Measured 2026-07-30, `Saved/SwarmBench.csv`, single config (`Swarm.DebugRender 0`,
Unit Cam off, `SimLOD.Stride 1`), 100 retinue, both emitters live simultaneously (team
weights default, enemy weights default):

| brood | game ms | draw ms | gpu ms |
|---|---|---|---|
| 1 000 | 9.63 | 0.00 | 4.55 |
| 10 000 | 16.60 | 0.00 | 7.54 |
| 40 000 | 38.88 | 0.00 | 6.10 |

**Reading: no cost from the second emitter.** `draw_ms` rounds to 0.00 at every count —
consistent with task-059's `0.001` finding for one emitter; two `NiagaraSpriteRendererProperties`
draw calls (one per side) is still nothing next to the Mass sim, which is what
`game_ms` climbing 9.6→38.9ms actually reflects. `frame_ms`/`fps` are INVALID for the
same reason task-059 flagged — PIE window unfocused during an MCP-driven run throttles
to 3fps regardless of `t.MaxFPS`; `game_ms`/`draw_ms`/`gpu_ms` are real per-frame work
and unaffected.

**Draw calls: 1 → 2, exactly as expected and pre-approved** (task-085's premise
correction reversed task-059's "draw calls unchanged" bar — see the task file). One
`NiagaraSpriteRendererProperties` per emitter, each its own material and texture.

### 7b. Cost after task-126 (archer rows, still two emitters)

Re-measured 2026-07-31, `Saved/SwarmBench.csv` config `task126`
(`Swarm.DebugRender 0`, Unit Cam off, `SimLOD.Stride 1`, 100 retinue), second pass of two
in one session:

| brood | game ms (task095 row) | game ms (task126) | draw ms | gpu ms |
|---|---|---|---|---|
| 1 000 | 3.84 | 9.51 | 0.00 | 1.94 |
| 10 000 | 10.56 | 12.41 | 1.94 | 3.28 |
| 40 000 | 32.86 | 35.09 | 2.19 | 8.59 |

**Draw calls are UNCHANGED at 2** — `GetSystemSummary` on `NS_Swarm` still reports exactly
two emitters (`Swarm`, `Team`), one `NiagaraSpriteRendererProperties` each. task-126 added
rows to an existing atlas, not a third emitter or a third texture.

**Read the game_ms deltas with care — they are not attributable to this change.** The
task095 rows come from a different session on a different day, and the first pass of this
same session (log only, CSV is rewritten per run) read 12.81 / 17.57 / 41.50 with
`draw_ms 0.00` and `gpu_ms` 12.4–14.9 — a 2–6× spread on `gpu_ms` between two passes of
identical code minutes apart. Run-to-run variance dominates. What task-126 actually adds
per frame is one `SwarmSquad::UnitType` test on TEAM entities in the integrate pass, one in
the render pack loop, and one extra `ParseVariantTable` per pass; the bench holds retinue
at 100, so the added work is bounded by 100 entities and cannot account for milliseconds at
40 000 brood. The brood path is untouched.

**Adding one enemy variant, after the split:** repack `enemy-units.json` with the new
source added as an eleventh composite source (or twelfth row pair), bump
`SwarmSheet::Enemy::Variants`, re-import `T_Enemy_2bit`, update the enemy emitter's
`SubImageSize` and `docs/data/art/brood-variants.json`'s weight table — all of it
confined to the enemy side. The team atlas, its emitter, its material and its texture
are never touched. (Same shape in the other direction for a team addition.) This is the
whole point of the split, and is now the only thing changing for a roster update.

## 7c. Evidence (task-126, archers)

Captured 2026-07-31 by `Swarm.DebugShotAfter 5` on `L_Spike1`, 140 retinue spawned on top
of the level's own wave, in `docs/perf/evidence/task126/`.

| file | what it shows |
|---|---|
| `01-default-weights-archers-among-spearmen.png` | default weights, both sub-tables live. Bow-carrying bodies among the shield-and-sword knights. Paired log: `TeamVariantReport: 74 spearmen (atlas rows 0-21) \| weights "20,2,18,14,14,2,2,6,10,18,16" \| v0=5 … v10=14` and `TeamVariantReport: 25 archers (atlas rows 22-33) \| weights "16,16,16,16,16,16" \| v0=7 (28.0%) v1=3 v2=3 v3=6 v4=4 v5=2`; `Team renderer was handed 113 cells this frame, atlas rows: 0 1 2 … 21 22 23 25 27 28 29 30 31 32 33` — both blocks drawing at once |
| `02-skewed-bowextended-vs-midguard.png` | the same field with `Swarm.TeamVariantWeights 0,0,0,0,0,0,0,0,0,100,0` and `Swarm.ArcherVariantWeights 0,100,0,0,0,0`. Every spearman is `v11_midguard`, every archer is `v2_bowextended`, and the two read apart at a glance. `Team renderer was handed 110 cells this frame, atlas rows: 18 19 24 25` — exactly variant 9's row pair (9*2) and archer table index 1's (`(11+1)*2`), nothing else. **This is the proof the row offset is applied per unit type and not per weight roll** |
| `SwarmBench-task126.csv` | the raw bench rows behind §7b |

**Watch out when re-running these:** CVars persist across PIE sessions inside one editor
process, so a weights line set by one run's exec block is still set for the next run unless
that run re-states it. A "default weights" shot taken straight after a skewed one is not a
default-weights shot.

## 7a. Evidence on disk (task-085)

| file | what it shows |
|---|---|
| `docs/perf/evidence/task085/01-default-weight-mix.png` | PIE, 120 retinue + 300 brood spawned, default team weights (`20,8,8,8,8,8,8,8,8,8,8`). Several distinct knight silhouettes visible at once — shields, polearms, bare heads, different heights. Paired log: `TeamVariantReport: 102 team \| weights "20,8,8,8,8,8,8,8,8,8,8" \| v0=20 (19.6%) v1=7 (6.9%) v2=11 (10.8%) v3=8 (7.8%) v4=7 (6.9%) v5=5 (4.9%) v6=13 (12.7%) v7=5 (4.9%) v8=12 (11.8%) v9=5 (4.9%) v10=9 (8.8%)` |
| `docs/perf/evidence/task085/02-skewed-v3-weight-100.png` | same field, `Swarm.TeamVariantWeights 0,0,0,100,0,0,0,0,0,0,0` — army visibly collapses to one repeated shield+sword silhouette. Paired log: `TeamVariantReport: 101 team \| v3=101 (100.0%)`, every other bucket 0; `Team renderer was handed 104 cells this frame, atlas rows: 6 7` — exactly variant 3's row pair (3*2=6,7), nothing else |

Both captures also logged `Enemy renderer was handed N cells, atlas rows: ...` in the
same frame, confirming the two sides decode independently in one screenshot's worth of
evidence.

## 7b. How to prove the decode when you cannot photograph it

Unchanged mechanism from task-059, doubled: `Swarm.DebugShotAfter` logs the distinct
atlas rows EACH renderer was handed in the same frame it captures — `LogRows` in
`SwarmRenderActor.cpp` runs once for `TeamSubImageScratch` against
`SwarmSheet::Team::Rows` and once for `EnemySubImageScratch` against
`SwarmSheet::Enemy::Rows`. That is the check that actually discriminates, because it
reads what reached each renderer rather than what the sim believes — see §7a's skewed
row output (`atlas rows: 6 7`, exactly variant 3, nothing else) for a worked example.

## 8. Brood photographability — resolved pre-split (task-084), unaffected by task-085

`Swarm.BroodAdd`, `Swarm.RawNear`, `M_Swarm`'s `ParticleColor` node and everything else
in the additive-light-floor fix documented in the pre-task-085 version of this file
carried across the split unchanged — `M_Swarm` still has that graph, it just samples a
different texture now. `docs/RENDERING-LIGHTING.md` §4e has the full history and is
still current. The retinue's own legibility (pale grey-green against the black floor)
was always fine and needed none of that machinery, which is why the team-side evidence
in §7a is a plain screenshot with no additive-light discussion.
