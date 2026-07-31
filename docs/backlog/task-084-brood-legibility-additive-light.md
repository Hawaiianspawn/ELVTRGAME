---
id: 084
title: Make the brood visible — the colour path is multiplicative and the ooze art is black, so no light value can lift it
status: done
agent: claude
owns: ["ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.cpp", "ELVTR/Source/ELVTR/Rendering/SwarmRenderActor.h", "docs/RENDERING-LIGHTING.md", "docs/perf/niagara-sprite-path.md"]
resources: ["unreal-editor"]
depends-on: []
epic: ""
evidence: A PIE capture at gameplay density where the nine brood variants are individually legible against the floor at contact range AND still fade toward the pool edge, with the retinue still reading as the brighter team beside them. The same two captures task-059 could not produce (default mix vs. one skin at weight 100) become gettable, and getting them is the proof.
score: {feel: 2, risk: 2, cost: 2}
source: user
teammate: brood-legibility
decided: "2026-07-29 done"
model: opus
---

## Why now
Task-059 landed the whole brood-variety mechanism and **proved it works without being able to
show it.** Nine ooze skins are in the atlas, the weight table demonstrably drives which rows
reach the renderer, and it costs one draw call — but a human cannot see any of it, because the
brood renders black on a black floor. That is the last thing standing between the owner and the
feature they asked for on 2026-07-29.

Owner, 2026-07-29, on seeing the task-059 captures: *"what happening wwith the light"*.

## The finding — established twice, independently, so do not re-derive it

**The colour path is a MULTIPLIER and the brood art has no headroom.** Confirmed from the code
by the lead and empirically by task-059's teammate, which is why it is stated as fact:

  - `SwarmRenderActor.cpp:1436` — `Atten = 1 - (D/FlameRadius)^FlameFalloff`, then
    `Lit = Lerp(Floor, Ceil, Atten)`, pushed as `FLinearColor(Lit, Lit, Lit, 1)`.
  - Brood window is `BroodLightFloor 0.0` → `BroodLightCeil 0.7`. Retinue is
    `UnitLightFloor 0.28` → `1.0`.
  - **The ooze body measures RGB 0-35 per channel** and `M_Swarm` is **Unlit**. Multiplying
    near-black by anything in `[0, 0.7]` is still near-black, at every distance. The hit flash
    reads only because it *replaces* the colour with white rather than scaling it.

**Therefore `Swarm.BroodLightFloor` is NOT the dial, and its own help text is wrong.** It claims
raising it "buys back the old always-visible tide". That was true when a demichrome pass
quantised the result to a 4-value ramp; the game ships at `Emberkeep.Quantize 0` now, so it is
stale advice. Fix the help text as part of this.

**What task-059 already tried and ruled out** — do not repeat any of it:
  - `Swarm.DebugPlainView 1` (post-process off) — floor still renders black.
  - `Swarm.UnitStencil 0` (drops the units' demichrome-lift exemption) — no change.
  - Flame cranked to `FlameRadius 3200`, `CoreRadius 2600`, `Intensity 1.6`, `Falloff 1` —
    floor still black, so there is nothing for a black silhouette to read against.

**Two candidate fixes. This is a LOOK decision and it is the owner's — bring them a comparison,
do not just pick one.**

  1. **An additive term for the brood** — a rim/fill floor added rather than multiplied, so a
     black body can surface. Keeps the art untouched and keeps the "fades in on approach"
     intent. Cheapest, and it is the route `swarm-units.json`'s brood note predicted: *"the fix
     is the per-unit distance/light layer, not another manual shift."*
  2. **Lighter brood art** — what was done in July 2026 (a manual Dark→Steel lift) and then
     retired when the floor colour changed. It works, but it costs the pure-black silhouette
     that makes the tide read as a mass, and it means regenerating or editing nine states.

Prefer (1) and present (2) as the alternative with a side-by-side. If (1) needs a Niagara
graph change to add an additive input, say so before building it — the graph is now known to
read `User.Colors` on `InitializeParticle`, and `AddUserVariables` cannot create a new array
param from MCP (see GOTCHAS).

## The second half — the flame defaults are stale and this is the moment to retune

Every floor/ceiling/falloff default was tuned against the **posterised 4-value** output that no
longer exists (`docs/art/aesthetic-direction.md` AMENDMENT 2026-07-28, colour gate superseded).
They were also **dead code until 2026-07-29** — `User.Colors` did not exist, so nothing applied
them and the horde drew at flat authored albedo. Task-059 switched four CVars live at once at
values nobody has ever actually seen applied.

Consequence to check and state: `Swarm.FlameRadius 900` against a camera that sees ~1200uu
half-width, so **the light ends visibly inside the frame** — that hard horizon in the task-059
captures where the horde stops existing is `Atten = 0` at the pool edge, not fog and not draw
distance. Whether that reads as intended or as a bug is a judgement call for the owner; measure
it and ask rather than silently widening the pool.

## Done when
- The nine brood variants are individually legible at contact range in a PIE capture at
  gameplay density, and still fade toward the pool edge rather than arriving fully formed.
- The retinue still reads as the brighter team beside a brood at every distance (that split is
  deliberate — `CVarSwarmBroodLightCeil`'s comment explains why).
- **Task-059's two blocked captures become gettable** — default weight mix showing several ooze
  looks at once, and one skin at weight 100 visibly dominating. This is the real bar.
- `Swarm.BroodLightFloor`'s help text no longer claims it can buy back visibility.
- `docs/RENDERING-LIGHTING.md` records the additive-vs-multiplicative decision and why.

## Scope fence
- **Not the variety mechanism.** Task-059 closed it and it is proven. Do not touch the atlas,
  `SwarmSheet`, the variant bits, the weight table, or `swarm-units.json`.
- Not retinue variants (the `T_Soldier_Knight_02..06` / `Archer_02..05` follow-up).
- Do not "fix" this by reverting to a manual Dark→Steel lift on the source PNGs without the
  owner explicitly choosing option (2) first.

## Spawn prompt
```
You are making Emberkeep's brood visible (C:\Projects\ELVTRGAME). Read
docs/backlog/task-084-brood-legibility-additive-light.md IN FULL FIRST — it contains a finding
established twice independently, and a list of five things already tried and ruled out. Do not
re-derive either.

THE FINDING, so you do not waste a session on the wrong dial: the per-particle colour path is a
MULTIPLIER (SwarmRenderActor.cpp:1436, Lit = Lerp(Floor, Ceil, Atten), pushed as a grey
FLinearColor) and the ooze body measures RGB 0-35 with M_Swarm Unlit. Multiplying near-black by
anything in [0, 0.7] is still near-black. Swarm.BroodLightFloor CANNOT fix this and its help
text saying otherwise is stale (it was written when a demichrome pass quantised the output; the
game ships at Emberkeep.Quantize 0 now). ALREADY TRIED AND RULED OUT: DebugPlainView 1,
UnitStencil 0, and flame cranked to FlameRadius 3200 / CoreRadius 2600 / Intensity 1.6 /
Falloff 1. The floor renders black in all of them.

YOUR JOB IS TO BRING THE OWNER A LOOK DECISION, NOT TO PICK ONE SILENTLY. Two routes:
  (1) An ADDITIVE term for the brood so a black body can surface, art untouched. Preferred.
  (2) Lighter brood art. Works, but costs the pure-black mass silhouette and means editing nine
      states. This was done in July 2026 and deliberately retired. DO NOT do this unless the
      owner explicitly picks it.
Build (1), and show (2) as a side-by-side comparison so the owner can choose.

SECOND HALF, and do not skip it: every flame floor/ceiling/falloff default was tuned against a
POSTERISED 4-value image that no longer exists, AND was dead code until 2026-07-29 because
NS_Swarm had no User.Colors parameter. Nobody has ever seen these values actually applied. In
particular Swarm.FlameRadius is 900 against a camera seeing ~1200uu half-width, so the light
ends INSIDE the frame — that hard horizon where the horde stops existing is Atten = 0, not fog.
Measure it and ASK the owner whether that reads as intended; do not silently widen the pool.

DONE WHEN:
  - The nine brood variants are individually legible at contact range at gameplay density, and
    still fade toward the pool edge rather than popping in fully formed.
  - The retinue still reads as the brighter team beside a brood at every distance.
  - TASK-059'S TWO BLOCKED CAPTURES BECOME GETTABLE: default weight mix showing several ooze
    looks at once, and one skin at Swarm.BroodVariantWeights 100 visibly dominating. THIS IS
    THE REAL BAR — task-059 proved the mechanism by logging atlas rows because it could not
    photograph it. You are the task that makes the photograph possible.
  - Swarm.BroodLightFloor's help text no longer claims it can buy back visibility.
  - docs/RENDERING-LIGHTING.md records the additive-vs-multiplicative decision and why.

DO NOT TOUCH:
  - The variety mechanism. task-059 closed it and it is PROVEN — the atlas, SwarmSheet,
    the variant bits, brood-variants.json and swarm-units.json are all off limits.
  - Retinue variants (T_Soldier_Knight_02..06 / Archer_02..05) — a separate follow-up.
  - The nine ooze PNGs, unless the owner has explicitly chosen route (2).

GOTCHAS:
  - MCP AddUserVariables CANNOT create an Array DataInterface user parameter — it returns
    success and silently does nothing, because the type needs an instance only the editor UI
    makes. If you need a new array param, add it via the User Parameters "+" by hand, then MCP
    can wire the rest. NS_Swarm already reads User.Colors and User.Sizes on InitializeParticle.
  - MCP asset edits are IN-MEMORY until save_assets([]). A set_properties returning true is NOT
    proof the write landed — read the value back and compare.
  - Live Coding is unusable on this module (a patch compile re-runs CVar static initialisers and
    crashes in a file the edit never touched). Use Stop-Editor; Build-Editor; Start-Editor;
    Wait-Mcp (Scripts/ue-mcp.ps1), ~7s.
  - Swarm.Clear trips the game mode's wave-cleared path and immediately spawns +120
    reinforcements, so a brood-only frame is not stageable that way. Swarm.RunAfter works as
    delayed exec.
  - Restore ELVTR/Saved/SwarmExecOnPlay.txt byte-exact if you move any dial for a shot.
  - The tree is shared with concurrent sessions. Build on uncommitted work you find, do not
    revert it, and do not attribute it to anyone.

You hold the unreal-editor lock. Deliver ON-SCREEN EVIDENCE from a runnable build — the two
captures task-059 could not get. Not a diff plus "it works".
```
