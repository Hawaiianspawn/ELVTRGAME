---
id: 095
title: Make the knight you see the knight that fights — bind the team variant index to a per-sub-type stat block
status: done
agent: claude
model: sonnet
owns: ["ELVTR/Source/ELVTR/Mass/SwarmCombat.h", "ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp", "ELVTR/Source/ELVTR/Mass/SwarmCombatProcessors.cpp", "ELVTR/Config/SwarmExecOnPlay.canonical.txt", "docs/perf/knight-subtype-binding.md"]
resources: ["unreal-editor"]
depends-on: [85, 88]
epic: unique-knights
evidence: A PIE capture at gameplay density where the retinue shows several different knight looks AND a `Swarm.KnightSubtypeReport` log proving each look is fighting on its own stat row — plus a second capture with one sub-type's DPS driven to an extreme through its CVar, showing that specific look, and only that look, changing behaviour on screen. Frame-time at 1k/10k/40k stated before/after so the per-entity stat lookup is shown to cost nothing measurable.
score: {feel: 3, risk: 2, cost: 2}
source: user
teammate: knight-stat-binding
decided: "2026-07-30 done"
---

## Why this exists
Owner, 2026-07-30: *"Each unique character should have its own base stats."*

`task-085` puts ten knight looks on screen. `task-088` writes their stat blocks into
`docs/data/unit-types.json`. **Neither one connects the two, and after both land every knight
still fights identically.** That is the whole of this task and it is the half of the owner's ask
that nothing else covers.

The gap is visible in the code in two lines:

- `SwarmFragments.h:196` — `SwarmUnitId::Pack(uint8 UnitIndex, EUnitType Type)` packs the type
  into **one bit**. `EUnitType` (`SwarmCombat.h:46`) has exactly two members, Spearmen and
  Archers, and `NumUnitTypes = 2`.
- `SwarmProcessors.cpp:463,472` — melee damage reads `SwarmCombatTuning::RetinueTargetsPerHit()`
  and `RetinueDPS()` as **flat scalars**, one number for the entire melee retinue.

Meanwhile the variant index — the field that decides which sprite a body wears — lives at
`SwarmFragments.h:257` (`VariantShift = 21`, 4 bits, 0-15) and is derived per entity from its
phase by `VariantFromPhase` against a weight table. It is a **render-side cosmetic choice with
no combat meaning whatsoever**.

## The design, settled — do not invent a fragment
**Index the stat table by the variant the entity already has.** The look and the stat block come
from the same number, so they cannot disagree, and no new per-entity state is stored.

**Stats arrive as comma-separated CVar lists, one per stat, indexed by variant** — the exact
pattern `Swarm.BroodVariantWeights` already uses and that `SwarmRenderActor.cpp:737` already
parses:

    Swarm.KnightSubtypeHP       "159,145,124,115,107,..."
    Swarm.KnightSubtypeDPS      "37.5,31.1,27.1,28.9,25.5,..."
    Swarm.KnightSubtypeEngage   "99.9,107.6,107.6,79.1,80.7,..."
    Swarm.KnightSubtypeTargets  "8,6,6,10,9,..."

Values come from `task-088`'s adopted table in `docs/data/unit-types.json`, transcribed with the
derivation recorded — `unit-types.json` stays the spec, CVars stay the live tunable source of
truth, which is the arrangement that file's own `note` already describes.

**Rejected, and why, so nobody tries them:**

- **A new per-entity stat fragment.** It is a class-layout change, and Live Coding reports
  success then crashes the next PIE on exactly those (see `docs/` and the recorded incident).
  It also stores what is already derivable for free.
- **Widening `EUnitType` to ten members.** `SwarmUnitId::Pack` gives the type one bit inside a
  `uint8` shared with `UnitIndex`; widening it touches allocation, formation, stance and
  telemetry code that has nothing to do with this. The melee/ranged split is a *role* axis and
  should stay two-valued; sub-types are a second, orthogonal axis.
- **Fifty individual CVars.** Ten sub-types times five stats. Unmanageable on the Breadboard and
  in `SwarmExecOnPlay`, and the list-CVar precedent already exists.

## The variant index this binds to — landed by task-085, 2026-07-30
`task-085` closed with the team atlas packed and the ordering recorded in
`docs/data/art/team-variants.json` and `docs/perf/niagara-sprite-path.md` §2. **These are the
exact numbers the stat table is indexed by:**

    0  retinue (the plain base, NOT a knight keep)
    1  v1_narrowguard      6  v7_barestance
    2  v2_lanceout         7  v8_heavycloak
    3  v3_shieldbreak      8  v10_bracedstaff
    4  v4_overhead         9  v11_midguard
    5  v6_simplecolumn    10  v13_maceraised

**Index 0 is the retinue base, not a knight**, and it needs a stat row like everything else —
today's flat `Retinue*` values are the obvious one. `SwarmSheet::Team::Variants = 11`, and the
render int32's variant field (bits 21-24) is **shared with the enemy side**, with `TeamBit`
deciding which table applies. Read the team table only when `TeamBit` is set.

Live weight table: `Swarm.TeamVariantWeights`, default `20,8,8,8,8,8,8,8,8,8,8` — deliberately
near-flat and explicitly *not* owner-judged yet.

## Fewer stat rows than looks is a legal outcome
`task-094` is expected to name pairs of silhouettes whose profiles are indistinguishable, and
`task-088` may well adopt fewer sub-types than there are looks. **Build for a mapping, not for
ten.** A `Swarm.KnightSubtypeMap` list — variant index → stat row — costs one more parse and
means three stat rows can back ten looks without touching the atlas. If `task-088` adopts a
1:1 mapping, the map is the identity list and nothing is wasted.

## Done when
- Each knight look on screen fights on its own stat row, and `Swarm.KnightSubtypeReport` logs
  the live count per sub-type beside the table it came from — the same shape as
  `Swarm.BroodVariantReport` (`SwarmRenderActor.cpp:742`), which exists because a histogram
  without its table proves nothing.
- Driving one sub-type's DPS to an extreme through its CVar visibly changes **that look only**.
  This is the proof the binding is real rather than a table nothing reads.
- Frame time at 1k/10k/40k before/after. A list index in the melee loop should cost nothing;
  state it measured rather than assumed.
- Archers are untouched and still read their own `Archers*` tuning. The sub-type axis applies to
  the melee retinue only.
- `docs/perf/knight-subtype-binding.md` records the mapping, the CVar names, and — explicitly —
  that the variant index now has combat meaning, because the next person to change the weight
  table needs to know it moves stats too.

## Scope fence
- Not the atlas, `SwarmSheet`, the pack loop, or the team weight table — `task-085` owns all of
  it and closes before this starts.
- Not the numbers. `docs/data/unit-types.json` and `docs/design/**` are `task-088`'s, adopted
  before this runs. Transcribe them; do not re-balance them.
- Not `Scripts/sim/**` or `docs/sim/**`.
- No PixelLab credits, no new art.

## Spawn prompt
```
You are binding Kindled's knight LOOKS to their STAT BLOCKS in Mass (C:\Projects\ELVTRGAME).
Read docs/backlog/task-095-bind-knight-look-to-stat-block.md IN FULL FIRST. The design is
SETTLED in it -- do not re-litigate it and do not invent a per-entity fragment.

Owner, 2026-07-30: "Each unique character should have its own base stats."

THE GAP, in two lines of shipped code. task-085 already put ten knight looks on screen and
task-088 already wrote their stat blocks into docs/data/unit-types.json -- but nothing connects
them, so every knight still fights identically:
  - SwarmFragments.h:196 SwarmUnitId::Pack packs EUnitType into ONE BIT. EUnitType
    (SwarmCombat.h:46) is Spearmen|Archers, NumUnitTypes = 2.
  - SwarmProcessors.cpp:463 and :472 read SwarmCombatTuning::RetinueTargetsPerHit() and
    RetinueDPS() as FLAT SCALARS -- one number for the whole melee retinue.
  - The variant index (SwarmFragments.h:257, VariantShift 21, 4 bits) that picks which sprite a
    body wears is derived per entity from its phase via VariantFromPhase and is purely cosmetic.

THE SETTLED DESIGN: index the stat table BY THE VARIANT THE ENTITY ALREADY HAS. Look and stats
come from the same number so they cannot disagree, and NO new per-entity state is stored.
Stats arrive as COMMA-SEPARATED CVAR LISTS, one per stat, indexed by variant -- the exact
pattern Swarm.BroodVariantWeights already uses and that SwarmRenderActor.cpp:737 already parses:
    Swarm.KnightSubtypeHP / .DPS / .Engage / .Targets
Values come from task-088's adopted table in docs/data/unit-types.json. Transcribe them with the
derivation recorded. unit-types.json stays the spec; CVars stay the live tunable source of truth,
which is the arrangement that file's own note already describes.

REJECTED, do not try:
  - A new per-entity stat fragment. It is a CLASS-LAYOUT CHANGE. Live Coding reports success on
    those and then CRASHES THE NEXT PIE. It also stores what is derivable for free.
  - Widening EUnitType to ten members. Pack gives the type one bit inside a uint8 it shares with
    UnitIndex; widening it drags in allocation, formation, stance and telemetry. Melee/ranged is
    a ROLE axis and stays two-valued. Sub-types are a second, orthogonal axis.
  - Fifty individual CVars. Unmanageable on the Breadboard and in SwarmExecOnPlay.

FEWER STAT ROWS THAN LOOKS IS LEGAL AND LIKELY. task-094 names silhouette pairs whose profiles
are indistinguishable, and task-088 may adopt fewer sub-types than there are looks. BUILD FOR A
MAPPING, NOT FOR TEN: a Swarm.KnightSubtypeMap list (variant index -> stat row) costs one more
parse and lets three stat rows back ten looks without touching the atlas. A 1:1 adoption just
makes it the identity list.

DONE WHEN:
  - Each look fights on its own stat row, and Swarm.KnightSubtypeReport logs the live count per
    sub-type BESIDE THE TABLE IT CAME FROM -- same shape as Swarm.BroodVariantReport
    (SwarmRenderActor.cpp:742), which exists because a histogram without its table proves nothing.
  - Driving one sub-type's DPS to an extreme through its CVar visibly changes THAT LOOK ONLY.
    That capture is the proof the binding is real rather than a table nothing reads.
  - Frame time at 1k/10k/40k before/after, MEASURED not assumed.
  - Archers untouched, still on their own Archers* tuning. Sub-types are melee-retinue only.
  - docs/perf/knight-subtype-binding.md records the mapping and the CVar names and states
    explicitly that the variant index NOW HAS COMBAT MEANING -- the next person to change the
    weight table needs to know it moves stats too.

SCOPE FENCE:
  - Not the atlas, SwarmSheet, the pack loop or the team weight table. task-085 owns all of it
    and is closed before you start.
  - Not the numbers. docs/data/unit-types.json and docs/design/** are task-088's and are adopted
    before you run. TRANSCRIBE, do not re-balance.
  - Not Scripts/sim/** or docs/sim/**. No PixelLab credits, no new art.

GOTCHAS that cost previous sessions real time:
  - Live Coding is unusable on this module: a patch compile re-runs CVar static initialisers and
    crashed in a file the edit never touched. Use Stop-Editor; Build-Editor; Start-Editor;
    Wait-Mcp (Scripts/ue-mcp.ps1), ~7s.
  - A recurring "rebuild ELVTR modules?" dialog is usually a false alarm (BuildId matches) but it
    BLOCKS MCP until dismissed.
  - Big changes hand over as a RUNNABLE BUILD or on-screen evidence, never a diff plus "it works".

Hand back with: the two captures, the frame-time rows, the adopted mapping, and anything you had
to assume. Do not mark the task done -- the lead does that.
```
