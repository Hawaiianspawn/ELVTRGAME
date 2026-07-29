---
id: 055
title: Pack every PixelLab character state per type, so a rank of spearmen has visual variety
status: done
agent: claude
owns: ["ELVTR/Content/Sprites/Units/**", "RawArt/Renders/knight/**", "RawArt/Renders/archer-proxy/**", "RawArt/Sheets/T_Soldier_Knight*.png", "RawArt/Sheets/T_Soldier_Archer*.png", "docs/data/art/provenance.json", "ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [46]
evidence: A PIE capture of a rank where soldiers of the SAME type visibly differ — some face-shielded knights, some not — while every one of them still reads unmistakably as that type.
score: {feel: 2, risk: 1, cost: 2}
source: user
teammate: character-states
decided: "2026-07-28 done"
---

## Why now
The owner, correcting terminology mid-session: *"sorry variants I mean states. This gives us
variety of the character type."*

PixelLab characters live in **groups** containing multiple **states** — the same character
re-rendered with a variation. Verified: the knight (`1c935515`) is one of **6 states** in
group `2cc3ab61-4230-4786-98fa-b94da9e99218`; a sibling (`37636e11`, "Add a face shield th")
is the same armoured knight at the same 88×88 with 8 directions, wearing a face shield. Same
silhouette family, different soldier. The archer sits in a group too.

This gives the two-axis model the Unit Cam actually wants:

| Axis | Determines | Source |
|---|---|---|
| **Type** | spearman / archer | the typed-unit layer (task-046) |
| **State** | which visual variation | the stable per-soldier hash |

Which restores the variety the six militia variants used to provide, except it now rides on
the type system instead of fighting it: a rank is visually varied *and* unambiguously one
type. task-046 builds the selection mechanism; **this task supplies the art it selects from.**

**It costs nothing in generations.** Every state already exists, and `get_character`'s
download bundles a whole group at once. The owner chose to pack all of them.

## Owner approval — 2026-07-28
All states reviewed and **approved** from the contact sheet
(`https://claude.ai/code/artifact/9e15430a-28ee-46aa-a544-62ec3b2997e0`, all 25 at 2× integer
scale). Owner: *"These look good I approve."*

| Group | States | Packed today |
|---|---|---|
| Vanguard / knight | 6 | Knight 01 only |
| Archer | 5 | Archer 01 only (proxy) |
| Brood / enemy | 14 | none |

**The brood group is a different kind of thing and needs its own decision before packing.**
Its 14 states read as genuinely *different creatures* (ooze, ghost, undead, marble, tongues),
where the knight's 6 are one figure re-kitted. So "state" is doing two jobs: cosmetic variety
within a retinue unit type, and potentially *distinct enemy types* for the brood. Pack the
knight and archer groups under this task; treat the brood as its own question.

**Animation is a separate spend, gated on this approval.** The schema wants 8 directions ×
2 walk frames = 16 frames (`asset-matrix.json` global open question #2). Template mode is
1 generation per direction, so **8 generations per state** — ~88 for everything. This task
packs only; it generates nothing.

## Amendment — 2026-07-27, scope widened mid-flight

The spawn prompt below says **DO NOT TOUCH `ELVTR/Source/**`** because task-046 owned the
selection code. **That instruction is superseded.** task-046 closed as `done` on 2026-07-27,
releasing its `owns:` lock; nothing else holds `Source/**`. The teammate correctly refused to
edit it and escalated instead.

The task as originally written had a defect: its `evidence:` bar — a PIE capture of visible
per-soldier variety — was **unreachable from inside its `owns:` set**. Wiring a packed sheet
into a live option is one line per state in `SpearmenStatePaths()` / `ArcherStatePaths()`
(`UnitCamProjector.cpp:70-79`), the exact spot task-046's own doc comment at line 525 names as
*"the entire cost of turning it on."* Without it, `NativeTick` never loads the new textures and
no capture can exist.

`owns:` is therefore widened to include `ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp`, scoped to
those two function bodies only. Re-validated clean: 55 tasks, 0 errors, 0 warnings.

**Owner's wiring decision — 8 of 9 states go live:**

| Array | Entries |
|---|---|
| `SpearmenStatePaths()` | `T_Soldier_Knight`, `_02`, `_03`, `_04`, `_05` |
| `ArcherStatePaths()` | `T_Soldier_Archer`, `_02`, `_03` |

Three are packed but deliberately **not wired**, and are **not deleted** — sheets, raw
downloads and provenance entries all stay:

- **Knight 06** — byte-identical to Knight 05 across all 8 source rotations, verified
  programmatically (max pixel diff 0). It cannot contribute variety.
- **Archer 04** (rocket launcher) and **Archer 05** (mage staff with glowing orb) — neither
  reads as an archer. Wiring them would produce confusion *between* types, which is the one
  failure mode this task exists to prevent.

`provenance.json` records each of the three as packed-but-unwired with the reason.

## Blocked at "wired, uncaptured" — 2026-07-28

Everything in this task is finished **except the evidence**. Packed, imported, verified,
provenance recorded, and wired: `SpearmenStatePaths()` holds 5 entries, `ArcherStatePaths()`
holds 3. Zero PixelLab generations spent.

The capture cannot be taken because **the editor's `unreal-mcp` listener is down** — pid 41048
is alive and responding but bound only to :1985, nothing on :9000. This is *not* the
blocked-dialog false alarm (that leaves the socket bound and hangs the call; this refuses the
connection outright).

Root cause is a **concurrent session** working in this same tree on a renderer-comparison
benchmark harness (`SwarmRenderActor.cpp/.h`, `ViewCamCapture.cpp`, plus `CVarProjEnable` /
`CVarProjFullscreen` added to `UnitCamProjector.cpp` — which task-055 is also editing). Owner
confirmed the second session and ruled: **nobody restarts the editor.**

`SwarmRenderActor.h` adds new `UPROPERTY` fields, so that workstream needs a full editor-closed
rebuild regardless — and that same rebuild is what restores the MCP listener. So the unblock is
free once the other session lands; racing it for the process would only break both.

**To finish:** rebuild with the editor closed, PIE, capture a rank showing per-soldier variety
within one type. Recipe below. Note that teammates do not survive `/resume` — re-spawn rather
than messaging the existing name.

### Capture recipe

Setup, verified against source (`character-states`, from reading — **not** observed live, it was
blocked from PIE the whole time):

1. `StartPIE` via `EditorToolset.EditorAppToolset` (`bSimulate=false`,
   `playMode=PlayMode_InViewPort`). The Unit Cam is embedded in the combat HUD by default
   (`UEmberkeepHud::RebuildBand`) — no toggle needed to make the panel appear.
2. `Swarm.SpawnRetinue 100` — each recruit rolls Spearmen vs Archers by `ArcherWeight`
   (`SwarmCommands.cpp:251`). Spearmen claim unit slots ascending from 0, Archers descending
   from `MaxSquads-1`, so squad 0 *should* be knights. **Unverified** — if squad 0 is empty or
   Archer, try 1, 2… or read `GetSquadType`.
3. `Emberkeep.UnitCamProj.SelectedSquad 0` — **required.** Default is `-1` = Army View, which
   draws aggregate count blocks, not individual sprites (`UnitCamProjector.cpp:1152`). No amount
   of wired states shows variety in Army View. `>= 0` switches to per-soldier billboards, the
   path `SpriteSetForSoldier` actually runs, and the camera auto-focuses that squad's centroid.
4. Confirm `Emberkeep.UnitCamProj.SoldierVariants` is `1` (default). At `0` the retinue draw
   from the flat shared atlas and none of this task's work is visible.
5. Check `Saved/SwarmExecOnPlay.txt` for values the concurrent bench session may have left —
   especially `Emberkeep.UnitCamProj.Enable` (0 collapses the panel entirely). That file is
   **gitignored**, so `git diff` proves nothing; diff it against
   `ELVTR/Config/SwarmExecOnPlay.canonical.txt` before and after, per `docs/AGENT-TEAMS.md` §8c.

### ⚠ The capture path is an open problem — all three documented routes fail

`character-states` recommended the §8b MCP tools. **That is wrong, and §8 itself says so.** The
correction, because whoever picks this up will otherwise burn a session on it:

| Route | Sees the PIE world? | Sees the UMG panel? | Verdict |
|---|---|---|---|
| `Swarm.DebugShotAfter` → `DebugCaptureComponent->CaptureScene()` | yes, at 1920×1080 | **no** | Scene capture renders the 3D scene only. The Unit Cam is a Slate/UMG widget painted by the HUD *after* the scene — it cannot appear. |
| MCP `CaptureViewport` | **no** | n/a | Renders the *persistent editor world*, not the transient PIE world (§8, wall 2). Structurally cannot see the swarm. |
| MCP `CaptureEditorImage` (desktop) | yes, composited | yes | But §8 wall 3 measured the game view at ~380×230 inside a 1280×446 grab — **units land at single-digit pixels**, far too coarse to judge helmet silhouettes. |

So the one route that can see the panel is the one documented as too low-res for exactly this
kind of detail judgement. The task's evidence bar may be unreachable with current tooling,
independent of the MCP outage.

**The plausible unlock is the concurrent session's own new CVar.**
`Emberkeep.UnitCamProj.Fullscreen 1` blows the Unit Cam up to fill the viewport. That is a large
enough panel that `CaptureEditorImage`'s resolution problem may stop biting — the §8 wall-3
measurement was taken against a small docked panel, not a full-viewport one. Try that
combination first. If it works, it is worth writing back into `docs/AGENT-TEAMS.md` §8 as the
recipe for capturing *any* Slate-drawn evidence, which is currently a documented dead end.

**Never** use a raw desktop-region grab (§8b) — one caught another session's windows on this
shared machine.

### What the shot must show

The Spearmen squad framed close enough that helmet silhouettes are individually legible: several
visibly face-shielded / round-helmed / chibi-helmed knights standing among plain-helm ones, all
still unmistakably knights. If individuals are too small to tell apart at default
`CVarProjDist`/`Height`/`Pitch`/`Fov`, tighten the framing dials before concluding the states
are illegible — that is a framing problem, not a states problem.

If the five knight states genuinely do not read as distinct at panel scale, **that is the
finding.** Report it; do not hunt for a flattering angle.

## Done when
- Every state in the knight's group and the archer's group is downloaded, packed and
  imported — full colour, no quantization, matching task-050's established format.
- Sheets follow the **existing** conventions exactly: cropped to alpha bbox and centred, no
  downscaling, the 56×60 cell geometry, and whatever grid task-046's selection mechanism
  expects. Do not invent a second format.
- Assets are named by **role and index**, never by PixelLab's state names — those are prompt
  fragments ("Add a face shield th", "Make more chibi helm (copy)"), not names.
- Raw downloads land under `RawArt/Renders/` first and are never deleted.
- `provenance.json` records every state: which group, which source id, and that these are
  full colour by owner direction (the ramp departure already recorded there).
- Evidence per `evidence:` above.

## Spawn prompt
```
You are executing task-055 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

GOAL, from the owner: "sorry variants I mean states. This gives us variety of the character
type." PixelLab characters live in GROUPS of STATES — the same character re-rendered with a
variation. Pack them all, so a rank of spearmen is visually varied while every soldier in it
still reads unmistakably as a spearman.

THE TWO AXES, so you understand what you are feeding:
  TYPE  (spearman / archer) -> which character GROUP a soldier draws from   [gameplay]
  STATE (within that group) -> which visual variation that soldier draws    [variety]
task-046 built the SELECTION mechanism — sprite choice is f(type, stable_hash), where the
stable hash comes from SizeBucket/JitterFragment::Phase. THIS task supplies the art it picks
from. Read how task-046 actually implemented state selection BEFORE packing anything, and
match the sheet layout it expects. If its mechanism supports N states and you pack a different
number, say so rather than silently mismatching.

WHAT TO GET:
- Knight group: 2cc3ab61-4230-4786-98fa-b94da9e99218, SIX states. One is 1c935515
  ("Hallam (Vanguard)") which is already packed as T_Soldier_Knight; another is 37636e11
  ("Add a face shield th"). Resolve the rest via get_character on any member — the response
  lists the group and the download bundles ALL states, one folder per state.
- Archer group: resolve from b3163cdf ("Merle (Pathfinder)"), which is already packed as
  T_Soldier_Archer. Its group has several sibling states.

DOWNLOAD ONLY. No animate_character, no create_character, no create_character_state, no
generation calls of any kind. Every state already exists and the owner has approved ZERO
credit spend on this task. If you believe something genuinely needs generating, STOP and say
so rather than spending.

FORMAT — follow what task-050 established, do not invent a second convention. Read
docs/data/art/provenance.json first; it documents all of this:
- FULL COLOUR. No quantize, no shadow lift, no normalize, no pale-to-bone clamp. These panel
  assets are a deliberate, owner-directed departure from the locked 4-value Demichrome ramp
  (recorded in provenance.json as `ramp_departure`). Do NOT "correct" them onto the ramp.
- No downscaling. Crop to alpha bbox and centre, unresized. PixelLab ships these with a large
  transparent margin — measured content is ~43x46 (archer) and ~35x46 (knight) inside an
  88-92px canvas, which is why the cell is 56x60 rather than 96x96. That crop is what made
  them read broad rather than narrow; preserve it.
- Check alpha is strictly 0/255. Task-050 found the source already binary, but verify rather
  than assume — a soft edge will fringe against the dark panel.
- NAMING: never carry PixelLab's state names into assets, code or docs. They are prompt
  fragments ("Add a face shield th", "Can the helmet be 60", "Make more chibi helm (copy)"),
  and "Hallam"/"Merle" are RETIRED names under the owner's role-only naming decision. Name by
  role and index.

RAW RETENTION: save every raw download under RawArt/Renders/ first and never delete it. That
is a standing project rule for all PixelLab output regardless of whether it is kept.

WATCH FOR: some states in a group are experiments rather than keepers — names like "(copy)"
and iterative prompts suggest so. The owner chose to pack ALL of them, so pack them all; but
if one is visibly broken or a near-duplicate of another at panel scale, SAY SO in your
handback so they can drop it later. Do not silently omit one.

CONSTRAINTS:
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are in-memory until save_assets([]).
- MCP AssetTools delete/move are KNOWN-UNRELIABLE on existing asset paths in this project.
  Fresh paths import fine via import_file; re-importing over an existing asset needs the
  headless -run=pythonscript commandlet fallback (real delete_asset + AssetImportTask + save).
  Both routes are documented in provenance.json. Budget for this — it bit task-050 every round.
- SAVED/SwarmExecOnPlay.txt IS GITIGNORED — "git diff is empty" proves NOTHING about it. If you
  touch it, verify with:
    diff ELVTR/Config/SwarmExecOnPlay.canonical.txt ELVTR/Saved/SwarmExecOnPlay.txt
  See docs/AGENT-TEAMS.md §8c. Values marked (owner-tuned) are deliberate and must survive.
- Capture per docs/AGENT-TEAMS.md §8. Note §8b: MCP-scoped capture tools only, never a raw
  desktop-region grab — one of those caught another session's windows on this shared machine.

EVIDENCE: a PIE capture of a rank where soldiers of the SAME type visibly differ — some
face-shielded knights among plain ones — while every one still reads unmistakably as that
type. That is the whole point: variety WITHIN a type, not confusion BETWEEN types. If the
states turn out not to be distinguishable at panel scale, that is an important finding —
report it rather than cropping to hide it.

The capture is not the last step of this task; it IS the task.

YOU OWN: ELVTR/Content/Sprites/Units/**, RawArt/Renders/knight/**,
RawArt/Renders/archer-proxy/**, RawArt/Sheets/T_Soldier_Knight*.png,
RawArt/Sheets/T_Soldier_Archer*.png, docs/data/art/provenance.json.

DO NOT TOUCH: ELVTR/Source/** (task-046 owns the selection code — if its mechanism cannot
take your sheets, SAY SO, do not edit it), ELVTR/Content/PostProcess/**, docs/art/**
(the art director's), docs/design/**, GDD.md, CLASSES.md, SYSTEMS.md, or any docs/backlog/ file.

HAND BACK: how many states you packed per type and what each one visually is, whether any are
duplicates or broken, the sheet layout you used and confirmation it matches what task-046
expects, your capture, and whether the states actually read as different at panel scale.
```

---

## Amendment — 2026-07-28, from the concurrent perf session

Two things this task needs to know. Written by the renderer/perf session it names above as the
source of its block, so the attribution in "Blocked at wired, uncaptured" is correct — it was me.

### 1. The MCP block is cleared, and `UnitCamProjector.cpp` has my edits in it

The editor was rebuilt with the editor closed and relaunched; **MCP is back on :9000.** The
`SwarmRenderActor.h` UPROPERTY rebuild that this task correctly predicted would restore the
listener has happened.

I also added **~118 lines to `ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp`**, which is inside this
task's `owns:` set. I did not check the lock first — my error. Owner ruled: **keep them, merge
around them, do not revert.** What is there:

| Added | Purpose |
|---|---|
| `Emberkeep.UnitCamProj.Enable` | master off switch (zero-sizes the panel) |
| `Emberkeep.UnitCamProj.Fullscreen` | blows the panel to the viewport, for measuring it as a camera |
| `Emberkeep.UnitCamProj.CountLog` | logs billboards actually drawn — proves a cost is real |
| tick-safety fix | early-out no longer *collapses* the widget, because a collapsed widget stops ticking and could never re-enable itself |

None of it touches `SpearmenStatePaths()` / `ArcherStatePaths()` / `SpriteSetForSoldier`, so this
task's own edits should merge cleanly.

### 2. The harder one: **this task's output is currently invisible in the game**

Owner decision the same day (`docs/perf/one-camera-bench.md` §6): **one-camera mode**, viewport
only, `Emberkeep.UI.Cams 0` by default. `RebuildBand` now returns before constructing the Unit Cam
at all. Owner, when shown a variety capture taken in the panel: *"no unitcam"*.

That matters because **every state this task packed is wired only into the Unit Cam.**
`SpearmenStatePaths()`/`ArcherStatePaths()` feed `SpriteSetForSoldier`, which exists solely in
`UnitCamProjector.cpp`. The viewport path is Niagara sampling a single shared atlas —
`T_Swarm_2bit`, and `SwarmSheet` is `Columns = 8, Rows = 4` (8 facings × brood walk0/1 +
retinue walk0/1). **There is no state axis on it.**

So the packing, importing, provenance and wiring are all real and all correct — and none of it
reaches the one camera the game now has.

**This is not a defect in the work; it is the ground moving underneath it.** The evidence bar
("a PIE capture … some face-shielded knights, some not") *was* met — I captured exactly that in
the panel, and the states genuinely read as different soldiers of one type at panel scale. The
capture was then discarded as invalid evidence, because it proves variety in a camera the game
no longer ships.

**What finishing now requires** is a decision, not more packing:

- **Give the viewport atlas a state axis.** Costs sheet space multiplicatively (states × 8
  facings × frames) and a `SwarmSheet`/`SwarmRenderPack` change to carry a state index into the
  SubImage decode. The honest option.
- **Multiple Niagara renderers**, one per state sheet — no atlas growth, but N draw calls
  instead of 1. Cheap today (rendering is free — §1 of the bench) but it gives back the single
  biggest reason Niagara won.
- **Accept it as Unit-Cam-only** and re-enable the panel for close-ups later, keeping the
  viewport horde uniform.

Owner's call. Nothing here should be packed, wired or generated further until it is made.

## Owner decision — 2026-07-28

The question above ("what finishing now requires is a decision, not more packing") was put to
the owner. **Verdict: give the viewport atlas a state axis.** Keep ONE draw call; texture
growth is cheap at 92px cells; this is the path that actually varies the horde in the camera
that ships. The "multiple Niagara renderers" and "accept Unit-Cam-only" options are declined.

**That work is not this task.** It needs `Mass/SwarmFragments.h` (SwarmSheet /
SwarmRenderPack), `Rendering/SwarmRenderActor.cpp` and the `NS_Swarm` / atlas assets — none
of which are in this task's `owns:` set. Widening `owns:` to cover it would be the wrong
move; the cut was simply between packing and decoding.

So it is filed as **`task-059`**, which also absorbs the owner's follow-up question about a
better Niagara render path, and carries the research: a state index rides free in
`SwarmRenderPack`'s unused bits 21-31, a `Texture2DArray` beats growing the SubUV atlas, and
`SizeScale` is already packed but discarded by the bridge.

**This task closes on what it delivered** — the states are generated, quantized, packed,
imported, provenance-recorded and correctly wired. That work is real and `task-059` consumes
it directly rather than redoing it.
