---
id: 055
title: Pack every PixelLab character state per type, so a rank of spearmen has visual variety
status: proposed
agent: claude
owns: ["ELVTR/Content/Sprites/Units/**", "RawArt/Renders/knight/**", "RawArt/Renders/archer-proxy/**", "RawArt/Sheets/T_Soldier_Knight*.png", "RawArt/Sheets/T_Soldier_Archer*.png", "docs/data/art/provenance.json"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: [46]
evidence: A PIE capture of a rank where soldiers of the SAME type visibly differ — some face-shielded knights, some not — while every one of them still reads unmistakably as that type.
score: {gate: 2, risk: 1, cost: 2}
source: user
teammate: ""
decided: ""
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
