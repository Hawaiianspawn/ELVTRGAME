---
name: cvars
description: Curate and sync ELVTR's important tuning console variables for live play. Regenerates Saved/SwarmExecOnPlay.txt, emits a paste list, refreshes the Console Variables Editor preset via the editor-Python script Scripts/populate_cvar_preset.py, and opens the in-engine Breadboard panel (Emberkeep.Breadboard) whose fields are parsed from that same file. Use when the user runs /cvars or asks to refresh/expose the tuning CVars, add a CVar to the tuning set, open the breadboard, or populate the Console Variables Editor panel.
---

# cvars — the ELVTR tuning-CVar surface

Keeps one **canonical set** of the console variables worth tuning live, and pushes it
to the two places the game can consume it. The set is defined by the list in
§"Canonical set" below; the *values and help text* are read fresh from source each
run so this never drifts from the code.

## Hard constraints (learned 2026-07-23 — do not re-discover)

Before promising anything, know what MCP **cannot** do here:

- The only CVar-related MCP tool is **read-only** (`EditorAppToolset.SearchCVars`).
  There is **no set-CVar tool** and **no tool that edits the Console Variables Editor
  panel**.
- The MCP Python sandbox (`ProgrammaticToolset`) allows only `re/json/copy/math/
  datetime/time` and **cannot `import unreal`** — so the plugin's own API is unreachable.
- `AssetTools` can **duplicate/load/save/delete** assets but **cannot create** one from
  a class, and `write_file` is text-only (can't emit a binary `.uasset`).
- Typing a CVar in the normal console does **not** add a row to the Console Variables
  Editor panel. That panel is fed only by its **+ Console Variable** button or by
  **loading a preset asset**.

Net: the panel cannot be populated purely from MCP. The reliable outputs are the exec
file and the paste list; the panel gets one-click loading only after a one-time seed
(§Panel preset).

## Canonical set

The important tuning CVars, grouped. (Debug/capture CVars — `Swarm.DebugRender`,
`DebugPlainView`, `DebugShotAfter`, `SpacingLogInterval` — are deliberately **excluded**;
add one here only if the owner asks.)

- **Lighting:** `Swarm.Flame`, `Swarm.FlameRadius`, `Swarm.FlameCoreRadius`,
  `Swarm.FlameFalloff`, `Swarm.FlameIntensity`, `Swarm.FlameFlicker`,
  `Swarm.FlameStiffness`, `Swarm.FlameDamping`
- **Dither:** `Swarm.DitherWorldAnchor`, `Swarm.WorldDitherScale`, `Swarm.DitherBandWidth`,
  `Swarm.DitherThreshold1`, `Swarm.DitherThreshold2`, `Swarm.DitherThreshold3` — the last
  four route through `MPC_Flame` into `M_PP_Demichrome`
- **Unit shading:** `Swarm.UnitShading`, `Swarm.UnitBackShade`, `Swarm.UnitLightFloor`
- **Body size:** `Swarm.BroodSize`, `Swarm.RetinueSize`, `Swarm.BodyHeight` — defined in
  `Rendering/SwarmRenderActor.cpp`, added 2026-07-26. These **override** the placed
  `ASwarmRenderActor`'s `BroodDebugPointSize`/`RetinueDebugPointSize` UPROPERTYs, which were
  the only way to resize a unit and needed you to select an actor in the level. A CVar value
  of **0 means "don't override"**, so the level keeps its design-time default and these stay
  a live experiment on top of it — the exec file writes explicit positive values so the
  breadboard rows show real numbers rather than a sentinel. They drive the **debug-box**
  renderer, which is the shipping path while `Swarm.DebugRender` is 1. Pair `BroodSize` with
  `Emberkeep.UnitCamProj.BroodScale` (the panel counterpart) and with `Swarm.BroodSeparation`
  — separation holds bodies apart with no idea how big they are, so size outgrowing it
  interpenetrates
- **Per-unit size variation:** `Swarm.BroodSizeJitter` (0.2), `Swarm.RetinueSizeJitter` (0) —
  same file, added 2026-07-26. Each body draws at 1 ± the amplitude, fixed for its lifetime.
  The per-entity roll rides in **bits 8-11 of the render buffer's anim int32**
  (`SwarmRenderPack` in `Mass/SwarmFragments.h`), derived from `FSwarmJitterFragment::Phase`
  rather than stored — chosen over a fragment field + a parallel `TArray<float>` because both
  of those are class-layout changes and would force a closed-editor rebuild every time this
  surface moved. Only the **amplitude** is a CVar, never baked into the buffer, so dragging
  the dial retunes the whole horde on that frame with no respawn. Two standing hazards:
  every consumer must keep masking a single bit or casting `(uint8)` before `CellForBits`,
  or it reads a size as an anim state; and the **Niagara sprite path ignores the roll**
  entirely (it needs a per-particle size array and a graph edit), so this is debug-boxes +
  Unit Cam only — invisible today because `Swarm.DebugRender` is 1, a trap the day it isn't
- **Combat (HP/DPS/melee):** `Swarm.BroodMaxHP`, `Swarm.BroodDPS`, `Swarm.RetinueMaxHP`,
  `Swarm.RetinueDPS`, `Swarm.MeleeRange`, `Swarm.MaxAttackersPerUnit`, `Swarm.HeroMaxHP`,
  `Swarm.HeroDPS`, `Swarm.HeroMeleeRange` — defined in `SwarmCombatProcessors.cpp`
- **Cleave, per team:** `Swarm.RetinueTargetsPerHit` (8), `Swarm.BroodTargetsPerHit` (1) —
  defined in `SwarmCombatProcessors.cpp`. How many enemies one blow lands on (the
  attacker's K nearest). The retinue dial is the cleave-powerup dial. Per team because a
  single shared K cannot express the output-unbounded / intake-bounded asymmetry the old
  model had by accident, and a *constant* K removes a stabiliser that keeps runs landing
  near a knife edge — see `docs/GATE1-FUN-PROTOTYPE.md` §3b. Added 2026-07-25
- **Hit reaction (swing cadence / flash / knockback):** `Swarm.SwingInterval`,
  `Swarm.SwingStrikeAt`, `Swarm.SwingLunge`, `Swarm.HitFlashTime`,
  `Swarm.KnockbackDistance`, `Swarm.KnockbackTime` — defined in
  `SwarmCombatProcessors.cpp`. These are game-*feel* dials rather than balance dials:
  `SwingInterval` is the only one that touches throughput, and only by changing how
  damage is parcelled (see `docs/GATE1-FUN-PROTOTYPE.md` §3b). Added 2026-07-25
- **Horde movement:** `Swarm.BroodSpeed`, `Swarm.BroodAggroRange`, `Swarm.BroodSeparation`,
  `Swarm.BroodSeparationWeight`, `Swarm.BroodSeparationCap`, `Swarm.BroodWalkHz` — defined in
  `Mass/SwarmProcessors.cpp`. Constexpr in `namespace SwarmTuning` until 2026-07-26, which
  meant the only tunable thing about the brood was how hard it hit. `BroodAggroRange` is the
  load-bearing one: it decides whether the tide is *stopped* by your line (a front forms) or
  *flows past* it for the flame, which is the difference between Hold meaning something and
  not. It is capped in practice by the 3x3 grid reach (~600uu at `GridCellSize` 200), so
  larger values read the same — raising it past that needs a wider neighbour query, not a
  bigger number. The retinue equivalents stay compile-time on purpose: your line is what the
  *stances* are meant to move
- **Horde arrival:** `Swarm.BroodSpawnRadiusMin`, `Swarm.BroodSpawnRadiusMax`,
  `Swarm.BroodSpawnArc`, `Swarm.BroodSpawnArcCenter`, `Swarm.BroodSpeedJitter` — defined in
  `Mass/SwarmCommands.cpp`. The arc pair (added 2026-07-26) is what lets a wave arrive as a
  FRONT rather than a full encirclement; 360 (the old hard-coded behaviour) is the
  surrounded case and was the only one the spike could stage
- **Unit Cam (projection close-up, §4d):** `Emberkeep.UnitCamProj.Focus`,
  `Emberkeep.UnitCamProj.FollowSpeed`, `Emberkeep.UnitCamProj.SoldierScale`,
  `Emberkeep.UnitCamProj.BroodScale`, `Emberkeep.UnitCamProj.FootAnchor`,
  `Emberkeep.UnitCamProj.BroodTint`, `Emberkeep.UnitCamProj.NearFade`,
  `Emberkeep.UnitCamProj.NearPlane`, `Emberkeep.UnitCamProj.Fov`, `Emberkeep.UnitCamProj.Dist`,
  `Emberkeep.UnitCamProj.Height`, `Emberkeep.UnitCamProj.Pitch`, `Emberkeep.UnitCamProj.Yaw`,
  `Emberkeep.UnitCamProj.AutoLook`, `Emberkeep.UnitCamProj.LookLerp`,
  `Emberkeep.UnitCamProj.CombatScan`, `Emberkeep.UnitCamProj.CastFocusSpeed`,
  `Emberkeep.UnitCamProj.CastZoom`, `Emberkeep.UnitCamProj.Range`, `Emberkeep.UnitCamProj.Scale`,
  `Emberkeep.UnitCamProj.SizeMax`, `Emberkeep.UnitCamProj.SizeMin`,
  `Emberkeep.UnitCamProj.SizeBodies`, `Emberkeep.UnitCamProj.SizeRetinueWeight`,
  `Emberkeep.UnitCamProj.SizeBroodWeight`, `Emberkeep.UnitCamProj.SizeCurve`,
  `Emberkeep.UnitCamProj.Aspect`,
  `Emberkeep.UnitCamProj.ThreatTint`, `Emberkeep.UnitCamProj.Hero`,
  `Emberkeep.UnitCamProj.HeroScale`, `Emberkeep.UnitCamProj.HeroCell`
  — split across two files since 2026-07-25: the **direction** dials (`Focus`, `FollowSpeed`,
  `Yaw`, `AutoLook`, `LookLerp`, `CombatScan`, `CastFocusSpeed`, `CastZoom`) live in
  `UI/UnitCamDirector.cpp` beside the camera manager they steer; the **lens/panel/hero-proxy**
  dials stay in `UI/UnitCamProjector.cpp` (owner added to the surface 2026-07-24).
  **`FootAnchor` is a correctness dial, not a taste one** (added 2026-07-26): the point a body
  projects to is its GROUND contact — the sim is 2D, every transform sits on the floor plane —
  so the old centre-anchored sprite was drawn half-buried, and every size multiplier
  (`SoldierScale`, `HeroScale`, `BroodScale`, `Swarm.BroodSizeJitter`) grew a unit downward
  through the floor exactly as much as upward. That is what "scaling sinks them through the
  floor" means. 1 plants the feet; 0 reproduces the old look for an A/B, and re-framing
  `Pitch`/`Height` after moving it is expected, since 1 lifts every body half its height
- **Game camera (the shot):** `Emberkeep.Cam.HudBias`, `Emberkeep.Cam.HudBiasLerp`,
  `Emberkeep.Cam.Ortho`, `Emberkeep.Cam.OrthoWidth`, `Emberkeep.Cam.Fov`, `Emberkeep.Cam.Dist`,
  `Emberkeep.Cam.Pitch`, `Emberkeep.Cam.Yaw`, `Emberkeep.Cam.YawInput`,
  `Emberkeep.Cam.OffsetX/Y/Z`, `Emberkeep.Cam.Lerp` — defined in `Spike/SpikeHeroPawn.cpp`.
  `TickCamera` owns the camera transform every frame, so the pawn's `CameraHeight` UPROPERTY
  no longer moves the in-game shot and `Dist` replaces it. Defaults reproduce the old
  hard-coded constructor shot exactly (ortho, pitch -90, 1200uu). **`YawInput` is load-bearing
  for `Yaw`**: movement is world-axis WASD, so without it a yawed camera leaves W pushing
  sideways relative to the screen. Added 2026-07-25
- **HUD command rectangle:** `Emberkeep.UI.Muster.WingRatio`, `Emberkeep.UI.ViewCam`,
  `Emberkeep.UI.Rect.Split`, `Emberkeep.UI.Rect.ViewOnTop` — defined in `UI/EmberkeepHud.cpp`;
  `Emberkeep.UI.ViewCam.Res`, `Emberkeep.UI.ViewCam.Rate` in `UI/ViewCamCapture.cpp`.
  WingRatio sizes the retinue wings flanking the cam; the rest drive the optional split centre
  column. All pair with the `SizeMax`/`SizeMin`/`Aspect` framing dials above, since the whole
  rectangle scales off the cam. **The split ships OFF (`ViewCam 0`) — the Unit Cam alone at full
  height is the primary layout.** The minimap that occupied the top half was tried and rejected
  2026-07-26 (pulled back far enough to show the brood spawn ring, the lit pool became a dot in
  a black field). Keep the dials on the surface: the split machinery is sound and cheap when
  off, so a future second panel reuses it. **`ViewCam.Rate` is a perf dial, not a look dial** —
  that feed is a real second render of the scene (added 2026-07-25)

To add/remove a CVar from the tuning surface, edit the lists above — that is the whole
point of this file being the single source of truth.

## What to do when invoked

1. **Read current defaults from source.** The CVar definitions live in seven files —
   lighting/dither/unit-shading in `Rendering/SwarmRenderActor.cpp`, combat in
   `Mass/SwarmCombatProcessors.cpp`, horde movement in `Mass/SwarmProcessors.cpp`,
   horde arrival (spawn ring/arc/jitter) in `Mass/SwarmCommands.cpp`,
   Unit Cam lens/panel in `UI/UnitCamProjector.cpp`, Unit Cam direction in
   `UI/UnitCamDirector.cpp`, the HUD rectangle in `UI/EmberkeepHud.cpp`.
   Grep each for the CVar names in the canonical set; take the default value and first
   line of help text. If a name isn't found in any of them, flag it (renamed or removed)
   rather than emitting a stale line.

2. **Regenerate `ELVTR/Saved/SwarmExecOnPlay.txt`.** Write every CVar as
   `Name <default>   ; <one-line help>`, grouped with `#` headers, preserving the
   existing "ACTIONS" block at the bottom (spawn/stance/report, commented out). This
   file is exec'd line-by-line at BeginPlay by `ASwarmRenderActor`; its parser strips
   `#`/`;` comments (added 2026-07-23), so inline notes are safe. Values here **do**
   apply at play — this is the reliable path. If the owner has passed specific values
   (e.g. "use FlameRadius 1150"), write those instead of the defaults and say so.

   **Never revert an owner-tuned value to the source default.** Where the existing file
   already deviates from source, that is a deliberate look/balance choice — keep the file's
   value and mark it `; (owner-tuned, src <default>)`. As of 2026-07-25 those are
   `WorldDitherScale 8`, `DitherBandWidth 0.5`, `FlameShadows 1`, `FlameShadowStrength 0.4`.
   Sections outside the canonical set that the file already carries (flame shadows, debug
   /render/capture) are kept as configured — `Swarm.DebugRender 1` in particular is load-
   bearing for the current renderer. Only take the source default when the *code* default
   changed on purpose that session.

   **Every section title MUST end in an `@tab <Name>` marker.** That marker is the only
   thing that puts a section on a breadboard tab; a section without one falls into a
   catch-all "Other" tab, which is the panel telling you this step was skipped. The tabs are
   **Player · Unit · Horde · Camera · Debug** (added 2026-07-25). Format — the marker sits on
   the *title* line, after the text, and the parser strips it before displaying the title:

   ```
   # ============================================================
   # RETINUE COMBAT — your soldiers' side of the exchange   @tab Unit
   # ============================================================
   ```

   Which tab a new section takes: the **subject** it tunes, not the system it lives in.
   Bearer/flame → `Player`; your soldiers, shared melee resolution, per-unit shading and
   hit feel → `Unit`; brood stats and spawning → `Horde`; both cams and the HUD rectangle →
   `Camera`; render/capture/debug knobs → `Debug`. **A section is the smallest unit a tab can
   hold**, so a section mixing subjects (the old combined `ENEMY / COMBAT` block did) must be
   split into one section per subject before it can be tabbed — that split is why bearer,
   retinue, brood and shared-melee are now four sections instead of one.

   To move a section between tabs, edit the marker. Nothing in C++ names these tabs, so no
   rebuild is involved and the tab bar picks up whatever the file declares.

   **Preserve the `# @tabs Player, Unit, Horde, Camera, Debug` line in the file header.** It
   fixes the left-to-right order of the tab bar. Without it the bar falls back to whichever
   tab a section happens to claim first, which is an accident of reading order — dither sits
   early in the file because it belongs beside lighting, and that alone would push Debug into
   second place. Keeping the two orders separate is what lets the file stay grouped for
   reading while the bar stays grouped for browsing, so **do not reorder sections to fix the
   bar** — edit that one line instead.

3. **Emit the paste list.** Print the bare `Name Value` lines (no comments) in a code
   block so the owner can seed the panel or paste into the console quickly.

4. **Refresh the panel presets via editor Python.** The panel *can* be populated
   programmatically — not through MCP, but through the ConsoleVariablesEditor plugin's
   `BlueprintCallable` API, which editor Python reaches. `Scripts/populate_cvar_preset.py`
   builds **several presets** (§Panel preset) — one per canonical group plus an
   "everything" preset — so the owner can load just the dials for what they're testing.
   Update `VALUES`/`PRESETS` in that script to match the canonical set, then have the owner
   run it. Do **not** try the MCP `set_properties` route — it cannot reach the row list.

5. **Open the breadboard.** Type `Emberkeep.Breadboard` into the editor's status-bar console
   (SlateInspector `Snapshot` the main window, find the console textbox next to the "Cmd"
   label, `Type` with `submit:true` — the same route used to run the preset script). The
   panel is the in-engine face of the file written in step 2, so opening it last means the
   owner sees exactly what was just generated, with live fields. Skip only if the editor
   isn't running.

## Breadboard (the in-engine panel)

`Window > Tools > Breadboard`, or the `Emberkeep.Breadboard` console command. Lives in the
editor-only `ELVTREditor` module (`ELVTR/Source/ELVTREditor/Breadboard/`).

**Its data source is `Saved/SwarmExecOnPlay.txt` itself** — it parses the `# ===` banners into
collapsible groups and each `Name Value ; help` line into a field, with the help text as the
tooltip. There is deliberately **no second CVar table in C++**: this file stays the single
source of truth, so a `/cvars` run in step 2 is what changes the panel's contents.

- Editing a field **sets the live CVar immediately** (drag a spinbox and the running game
  changes under your hands). The `live N` column shows what the CVar actually holds, tinted
  when it has drifted from the field.
- **Save to file** writes the fields back as a surgical rewrite: only value tokens change, so
  banners, help text, alignment, owner-tuned markers and the commented-out ACTIONS block all
  survive. This is what makes a value stick — live edits are stomped at the next BeginPlay,
  when the render actor re-execs the file.
- **Pull live** copies the running values into the fields, so a console-tuning session can be
  captured and saved. **Apply all** pushes every field out. **Reload** re-reads the file.
- A row whose name is red isn't a registered CVar in this build — a stale line the code
  renamed or removed. That is the panel's drift check; fix it in the file, not in the panel.
- Optional slider bounds: write a `[min..max]` token anywhere in a row's help comment and the
  panel uses it for that dial's slider range. Without one it brackets the file value.
- The same panel can be hosted in an EditorUtilityWidget: drag **ELVTR Breadboard** (palette
  category "ELVTR") onto an Editor Utility Widget canvas.

## Panel preset (the one-click panel UX)

The Console Variables Editor panel loads **preset assets** (`ConsoleVariablesAsset`).
Two facts, both verified 2026-07-23:

- **MCP cannot touch the row list.** No tool creates a `ConsoleVariablesAsset` from a
  class; and on a seeded asset the `SavedCommands` array is **not reflection-accessible**
  (`list_properties` shows only `variableCollectionDescription`;
  `get_properties(["SavedCommands"])` errors "could not be read"). `set_properties`
  therefore can't write rows — `SavedCommands` is a bare `UPROPERTY()`, no Blueprint flag.
- **Editor Python CAN**, via `unreal.ConsoleVariablesEditorFunctionLibrary` (the plugin's
  BlueprintCallable library in
  `Engine/Plugins/Editor/ConsoleVariablesEditor/.../ConsoleVariablesEditorFunctionLibrary.h`).
  Key calls: `load_preset_into_console_variables_editor(asset, REPLACE_EXISTING)`,
  `add_validated_command_to_current_preset("Name Value")` per row,
  `remove_command_from_current_preset("Name")` (clears a row — how each preset is stripped
  to exactly its set), `copy_current_list_to_asset(asset)`, then
  `EditorAssetLibrary.save_loaded_asset(asset)`; verify with
  `get_list_of_commands_from_preset(asset)` (returns a `(bool, [names])` tuple in Python).

**The presets** (assets under `/Game/SwarmControls`, built by the script):

| Asset | Contents | Load it to test… |
|---|---|---|
| `SwarmConsoleVariablesAsset` | everything (all tuning CVars) | anything / the full surface |
| `CVP_Lighting` | flame + dither + unit shading | the spotlight look |
| `CVP_Combat` | HP/DPS/melee + hit reaction | fight balance |
| `CVP_Horde` | brood stat block + movement + arrival | how the tide behaves |
| `CVP_UnitCam` | the 13 `Emberkeep.UnitCamProj.*` dials | the Unit Cam / camera work |

The subset presets mirror the canonical groups above. To add a **scenario** preset (a
named config with non-default values), add an entry to `PRESETS` with its own baked values.

Workflow:

- **One-time seed (done):** `/Game/SwarmControls/SwarmConsoleVariablesAsset` exists; the
  script duplicates it to create any missing `CVP_*` assets, then clears + fills each.
- **Populate/refresh:** edit `VALUES`/`PRESETS` in `Scripts/populate_cvar_preset.py`, then
  run it in the editor console: `py "C:/Projects/ELVTRGAME/Scripts/populate_cvar_preset.py"`.
  It rebuilds every preset to exactly its set, saves, and logs a per-preset row count.
- **Claude can run it directly** (verified 2026-07-24) without the owner typing anything,
  via the SlateInspector toolset: `Snapshot` the main editor window, find the status-bar
  console textbox (next to the "Cmd" label — it was `tb2`), `Type` the `py "..."` command
  with `submit:true`, then confirm with `LogsToolset.GetLogEntries(category:"", pattern:"CVE preset")`
  — the script logs `added N/N, asset now holds M rows: [...]` per preset.
- **Load (owner):** Console Variables Editor → Presets → pick the preset for what you're
  testing; its rows appear with sliders.

MCP still can't *run* the script (its sandbox blocks `import unreal`), so the owner
triggers it with `py`. The exec file + paste list remain the no-editor-needed fallback.

## Reporting

State plainly which outputs were produced: the exec file (always), the paste list
(always), the preset (only if it existed and the read-back verified), and whether the
breadboard was opened. Never claim the panel was populated when only the exec file was
written — they are different things.
