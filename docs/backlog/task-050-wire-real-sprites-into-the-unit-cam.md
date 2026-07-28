---
id: 050
title: Draw the real sprite set in the Unit Cam — six soldier variants, the hero, and an archer proxy
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp", "ELVTR/Source/ELVTR/UI/UnitCamProjector.h", "ELVTR/Content/Sprites/**", "RawArt/Renders/archer-proxy/**", "RawArt/Sheets/T_Soldier_Archer.png", "docs/data/art/provenance.json"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: A PIE capture (docs/AGENT-TEAMS.md §8 recipe, 1920x1080) of the Unit Cam panel showing visibly DIFFERENT soldier sprites side by side, the bearer drawing T_Hero_Vanguard, and the archer proxy present and distinguishable from the melee variants.
score: {gate: 3, risk: 2, cost: 2}
source: user
teammate: unit-cam-assets
decided: "2026-07-27 done"
---

## Why now
The owner, 2026-07-27: *"The performance is not accurate as we are not rendering UnitCam
assets"*, then *"lets get the unit cam assets in"*.

**The art already exists and is simply not wired in.** `ELVTR/Content/Sprites/` holds
`T_Soldier_01`–`06` (six palette-validated militia variants — volunteer, line infantry,
veteran, horn-caller, flame-tender, shield-heavy), `T_Hero_Vanguard`, and
`T_Unit_Retinue`. The Unit Cam draws none of them: `UUnitCamProjector` loads only
`T_Swarm_2bit`, the 8×4 combined atlas with one generic retinue row, so every soldier in
the panel is the same sprite.

This blocks two other things. task-021 (measure the cost boundary) was **parked** because
its numbers cannot be representative while the panel draws placeholders. And the owner's
core question — whether the Unit Cam can show real units by default — cannot be judged
against art that isn't there.

The archer is a **proxy, deliberately**. Owner: *"use this one for now. we may have to swap
but we can get up the structure with this proxy."* Build the structure so the swap is easy.

## SCOPE CHANGED 2026-07-27, mid-task
The first pass wired the six 48px militia variants and the owner rejected the result:
*"we degraded with the units again. I only want my high resolution units for the retinue.
So the archer model and the knight model we had."* The militia sheets are 48px cells against
88–92px PixelLab characters, and the resolution drop was visible in the panel.

**The retinue is now exactly two high-resolution units:** the knight (PixelLab character
`1c935515`, 8dir 88×88, already carrying walk/attack/attack_v2/hit animations in south) and
the archer proxy (`b3163cdf`, 8dir 92×92). The six militia variants are **removed from the
panel** — files stay in the project, nothing draws them.

What survives from the first pass: the stable-assignment mechanism (`SizeBucket` re-hashed
off `JitterFragment::Phase` — now choosing between two units, not seven), the
load-by-content-path design that makes this redirect cheap, and the archer import.

## Addendum — panel lighting, landed after this task closed
The owner reviewed the packing fix, judged the panel good enough, and **chose to skip** the
full-colour lighting fix; task-050 was closed and task-052 dispatched to the editor. A
lighting change was already in flight against an earlier instruction and landed afterwards:

- `Emberkeep.UnitCamProj.FullColorFloor` (0.55) — a separate floor from
  `Swarm.UnitLightFloor` (0.28), which still governs everyone else.
- `Emberkeep.UnitCamProj.FullColorDimStrength` (0.5) — scales the *amplitude* of the shared
  distance/facing falloff rather than removing it. Worst case ≈0.775 against 1.0 at full
  light, so a ~22% flame-distance gradient survives. Banding, front/back shading and flame
  radius are all still computed as before.

Scoped to the knight/archer sprite sets only; brood, hero, `T_Swarm_2bit` and world
rendering are untouched. Both values are in `Saved/SwarmExecOnPlay.txt`, not just code.

**The owner reviewed it after the fact and chose to keep it** (2026-07-27) — it reads
visibly better at typical distance than the version they had accepted. Recorded here because
it landed outside the task's own lifecycle, and a change that arrives after close should not
quietly become part of the record without saying so.

## Done when
- The Unit Cam retinue draws **only the knight and the archer**, both at high resolution.
  Assignment stays deterministic and stable per soldier.
- **The resolution mismatch is resolved deliberately** — the panel's 48px cell geometry does
  not fit 88–92px characters. Downsampling them recreates exactly the degradation the owner
  objected to, so that is almost certainly the wrong answer. Whatever is chosen is stated
  and justified.
- **Both units read as the same army.** The archer proxy currently quantizes ~69%
  Dark-dominant, which `soldier-roster-v1.md` reserves so friendlies never read as enemies.
  Knight and archer must sit together as one side.
- The six militia variants are no longer drawn; their assets are left in place.
- The bearer draws **`T_Hero_Vanguard`**.
- An **archer proxy** is present, sourced from the owner's PixelLab character
  (`b3163cdf-6434-4390-8417-721a80797dcf`, "Merle (Pathfinder)"), quantized to the locked
  4-value Demichrome ramp and packed to the atlas's frame format. It must be
  **distinguishable at panel scale** from the melee variants — the bow is the read.
- The swap path is obvious: replacing the proxy later is a texture swap plus a manifest
  entry, not a code change.
- Existing framing, yaw clamp, selection and Army View behaviour are unchanged. This task
  changes **what is drawn**, not how it is framed.
- Evidence per `evidence:` above.

## Spawn prompt
```
You are executing task-050 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight
branch.

GOAL, from the owner: "lets get the unit cam assets in". The Unit Cam panel is drawing
placeholder art while the real sprite set sits unused in the project. Fix that.

THE SITUATION, verified before this task was written:
- ELVTR/Content/Sprites/Units/ holds T_Soldier_01 through T_Soldier_06 — six real,
  palette-validated militia variants (192x96 each, 48px cells). Source sheets are in
  RawArt/Sheets/. docs/art/soldier-roster-v1.md §3 describes what each variant IS:
  01 volunteer (braced haft), 02 line infantry (dome helm + vertical shaft), 03 veteran
  (stepped crown, mail skirt), 04 rally caller (upswept horn), 05 flame-tender (caged
  bright in an iron pot), 06 shield-heavy (door-shield wider than the body).
- ELVTR/Content/Sprites/Heroes/T_Hero_Vanguard.uasset (192x192) is the bearer.
- ELVTR/Content/Sprites/Units/T_Unit_Retinue.uasset (192x48) is the generic retinue strip.
- ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp currently loads ONLY T_Swarm_2bit (the 8x4
  combined atlas, brood + one generic retinue row) into SwarmAtlas, slices it once into
  CellBrushes, and every billboard indexes that. That is why every soldier looks identical.

WHAT TO BUILD:
1. Per-variant soldier rendering. Each retinue soldier in the panel draws one of the six
   variants instead of the shared row. The assignment must be DETERMINISTIC AND STABLE for a
   given soldier — deriving it from a per-entity value that does not change is essential. If
   you derive it from anything that gets renumbered (see the warning below), units will
   flicker between variants as people die, which will look like a bug and will be blamed on
   your change.
2. The bearer draws T_Hero_Vanguard rather than a cell of the swarm atlas.
   Emberkeep.UnitCamProj.HeroCell (default 16) currently picks a cell of the 8x4 sheet;
   decide whether that dial survives, and say what you did with it.
3. The archer proxy — see the section below.
4. Keep it tunable: if you add dials, follow the TAutoConsoleVariable prose-doc-comment style
   already in that file. Every dial there explains its tradeoff in plain language.

CRITICAL WARNING ON STABLE ASSIGNMENT: do NOT derive the variant from SquadId or from a
formation slot index. SwarmSubsystem.h computes SquadIdForSlot(Slot) = Slot / SquadTargetSize
off the DENSE formation-repack index, and NeedsFormationRepack() fires whenever
AliveRetinue != PackedRetinueCount — i.e. on ANY casualty anywhere. Those indices renumber
constantly during a fight. This is a known, documented defect (docs/design/squad-group-system.md
§1.3); task-046 will fix it but has not been built. Until then, find something genuinely
stable per soldier, or derive the variant from a hash of something immutable. If nothing
stable exists to hang it on, SAY SO in your handback rather than shipping a flickering panel —
that is a real finding and it strengthens the case for task-046.

THE ARCHER PROXY:
The owner supplied a PixelLab character and was explicit that it is temporary:
"use this one for now. we may have to swap but we can get up the structure with this proxy."
  character_id: b3163cdf-6434-4390-8417-721a80797dcf   (named "Merle (Pathfinder)")
  8 directions, 92x92, low top-down, COMPLETED, no animations.

  - DOWNLOAD ONLY. Use mcp__pixellab__get_character to read it and fetch its rotation PNGs.
    DO NOT call animate_character, create_character, or any other generating call — those
    cost real credits and the owner has not approved spending any on this task. If you
    conclude the proxy genuinely needs generated animation frames, STOP and say so; do not
    spend.
  - Save the raw download under RawArt/Renders/archer-proxy/ FIRST and never delete it —
    that is a standing project rule for every PixelLab generation, keep/reject decision or not.
  - It does NOT match the project format and must be converted: it is 92x92 and full-colour;
    the atlas is 48px cells on the LOCKED 4-value Demichrome ramp with alpha strictly 0 or
    255, content bbox <= 48x48. Use the /sprite skill's quantize step — do not hand-roll a
    palette snap. docs/art/aesthetic-direction.md is the locked ramp.
  - It has NO animation frames, but the atlas format is action frames (walk0, walk1, attack,
    hit), not directions. For a proxy, reusing a single rotation across the action cells is
    acceptable — say clearly in your handback that the archer does not animate yet and what
    it would take.
  - MAKE THE SWAP CHEAP. The owner expects to replace this. Replacing it should be a texture
    swap plus a manifest entry, not a code edit. If your design would require touching C++ to
    swap the archer art, redesign it.
  - NAMING: the character is called "Merle". That name is RETIRED — the owner reversed the
    fixed-hero-name decision and classes are identified by role only. Do not carry "Merle"
    into asset names, code, or docs. Name it for its role (e.g. T_Soldier_Archer).
  - Record the generation in docs/data/art/provenance.json per the /sprite skill's manifest
    convention, including that it is a proxy and where it came from.

CONSTRAINTS:
- DO NOT change framing, the yaw clamp, selection behaviour, or Army View. task-045 shipped
  those and they are settled. This task changes WHAT IS DRAWN, not how it is framed.
- Palette law: everything drawn must sit on the locked 4-value Demichrome ramp. The panel is
  UMG and draws AFTER post-processing, so nothing quantizes it for you — a sprite that is
  off-ramp stays off-ramp on screen. Demichrome::Pale() and friends are in
  ELVTR/Source/ELVTR/UI/EmberkeepPalette.h.
- ADDING A UPROPERTY VIA LIVE CODING REPORTS SUCCESS THEN CRASHES THE NEXT PIE. Use
  `pwsh Scripts/ue-relaunch.ps1` for layout changes; Scripts/ue-iterate.ps1 picks the path.
- unreal-mcp is on PORT 9000, not 8000. MCP asset edits are IN-MEMORY until save_assets([]).
  Importing textures without saving means they vanish on editor close.
- If you change Saved/SwarmExecOnPlay.txt, RESTORE IT. Several values are owner-tuned and
  deliberate (Swarm.Formation.Spacing 42.4, the UnitCamProj Fov/Height/Pitch block,
  Swarm.UnitShading 0).

EVIDENCE — read docs/AGENT-TEAMS.md §8 FIRST, it was written for exactly this. Swarm.DebugShotAfter
now captures at a fixed 1920x1080 from an UNFOCUSED PIE window, driven over plain HTTP via
Scripts/ue-mcp.ps1's Invoke-McpTool. Its traps are documented there (DebugRender must be 0 or
you capture no units; SlateInspectorToolset ejects PIE on click-in). Note the capture is the
RAW SCENE without the demichrome pass — fine for proving WHICH sprite is drawn, NOT valid for
judging palette. If you need to prove palette compliance, do it by inspecting the texture, not
from the capture.

Hand back a capture showing: visibly DIFFERENT soldier sprites side by side in the panel, the
bearer as T_Hero_Vanguard, and the archer proxy present and telling apart from the melee
variants at panel scale. If the variants are not distinguishable at that size, that is an
important finding — report it rather than cropping to hide it.

The screenshot is not the last step of this task; it IS the task. Several teammates on this
project have written their code, gone idle without building or capturing, and had to be sent
back. If you are about to report without an image, you are not done.

YOU OWN: ELVTR/Source/ELVTR/UI/UnitCamProjector.cpp/.h, ELVTR/Content/Sprites/**,
RawArt/Renders/archer-proxy/**, RawArt/Sheets/T_Soldier_Archer.png, docs/data/art/provenance.json.

DO NOT TOUCH: UnitCamDirector.* , ELVTR/Source/ELVTR/Mass/**, ELVTR/Source/ELVTR/Rendering/**,
ELVTR/Content/PostProcess/**, docs/design/**, docs/art/** (specs are the art director's),
GDD.md, CLASSES.md, SYSTEMS.md, or any docs/backlog/ file.

HAND BACK: how you made variant assignment stable (and whether you found anything genuinely
stable to hang it on), what you did with HeroCell, how the archer proxy was converted and how
someone swaps it later, your capture, whether the six variants actually read as different at
panel scale, and anything you had to fake.
```
