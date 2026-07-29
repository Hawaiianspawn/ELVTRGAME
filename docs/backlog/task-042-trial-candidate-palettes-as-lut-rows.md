---
id: 042
title: Trial rust-gold and eulbink as LUT rows against the locked demichrome
status: proposed
agent: pixel-art-director
owns: ["docs/art/palette-trials.md"]
resources: ["unreal-editor"]
depends-on: [43]
evidence: Screen captures of the same 700-unit wave under each candidate ramp at gameplay zoom, with a recommendation and the readability cost of each stated.
score: {feel: 2, risk: 2, cost: 2}
source: user
decided: ""
---

## Why now
The owner asked to see rust-gold-8 and eulbink against the locked ramp — *"it would be
good to have this be base to be on tone right."* This is the task that answers it, and it
**Re-pointed 2026-07-26 from task-041 (Phase B) to task-043 (Breadboard dial).** Phase B
is not needed to *preview* a ramp: `M_PP_Demichrome` quantizes scene luminance and every
sprite renders through it, so the post pass alone recolours the world and all units. Once
task-043 puts the four entries on `MPC_Flame` behind an `Emberkeep.Palette` int CVar,
judging a candidate is dragging one Breadboard row in PIE. Phase B stays the requirement
for *hue-separated* palettes and per-faction LUT rows — not for tone.

Also corrected: the full 8- and 7-colour sets are not drop-ins, but curated **4-cuts**
are, and **eulbink-4 separates better than the incumbent** (0.235 vs 0.218). Judge the
cuts in the table below, not the full sets.

Measured against the quantizer's own luma model (Rec.709, gamma sRGB) on 2026-07-26:

| Cut to judge | 4 values | Min luma gap |
|---|---|---|
| demichrome (incumbent) | `#211e20` `#555568` `#a0a08b` `#e9efec` | **0.218** |
| eulbink-4 | `#252446` `#0098db` `#0ce6f2` `#ffffff` | **0.235** |
| rust-gold-4 | `#331c17` `#725956` `#bb7f57` `#f6cd26` | 0.168 |

*(Full sets, for the record: eulbink-7 min gap 0.051, rust-gold-8 min gap 0.001 — the
latter has two luma-identical pairs that collapse under a luminance quantizer. The 4-cuts
above are the usable forms.)*

**Each candidate answers a different open question, and neither is a free swap.**

**Rust-gold** would hand back the warm/cold friend-foe channel that
`aesthetic-direction.md:39` says plainly was surrendered by the lock: *"there is no more
warm/cold friend-foe channel, no faction-reserved third slot."* Its structure — 2 neutrals
+ 4 rust + 2 gold — is a natural fit for the per-faction LUT rows §2.1 makes possible, and
it is thematically dead-on for a game about carrying the only fire. Its risk is the same
thing: hue-carried separation is exactly what a 4-value near-neutral ramp proved it does
*not* need, and reintroducing it re-opens Direction B's discipline problem.

**Eulbink** would close open decision **L11** for free. `RENDERING-LIGHTING.md:705` records
that the flame pool *"reads warm, not white"* because it grades through Bone `#a0a08b`, the
palette's only warm mid — and that fixing it *"needs a second palette exception (a
white-biased ramp for lit areas)."* Eulbink ramps blue→cyan→white, so a white pool falls
out of the ramp instead of out of an exception. Its risk is tonal: its own description is
"cold, retro," and this is a game about fire in the dark.

## Done when
- Each candidate reduced to a usable ramp and the reduction justified — eulbink's bottom
  three blues sit within 0.10 luma and cannot all survive as distinct values; say which
  are kept and why.
- Same 700-unit wave captured under each ramp at true gameplay zoom. Gate 1 wave 3 gives
  that count.
- Judged on the thing that actually matters: **can you still tell friend from foe, and
  read a threat, at density?** Demichrome's 0.218 minimum gap is the number to beat, and
  it is a wide margin. A candidate that looks better in a screenshot and worse in a wave
  loses.
- L11 explicitly re-tested under eulbink — does the pool read white without an exception?
- One recommendation, with the readability cost stated honestly. **"Keep demichrome" is a
  valid and likely outcome**; this task exists to answer the question with evidence, not
  to justify a change.
- `## Canon proposals` for any amendment to `aesthetic-direction.md`. This task does not
  edit the direction doc.

## Spawn prompt
```
You are the pixel-art-director for Emberkeep (C:\Projects\ELVTRGAME).

The owner asked to see two candidate palettes against the locked ramp:
  https://lospec.com/palette-list/rust-gold-8
  https://lospec.com/palette-list/eulbink
This depends on task-041 (Phase B index+LUT). Confirm it landed — if palettes are not yet
LUT rows, stop and say so rather than editing three places by hand.

Hexes, and their Rec.709 luma under the project's own model (docs/data/art/palette.json
luma_model), measured 2026-07-26:

  demichrome-4:  #211e20 .121  #555568 .339  #a0a08b .622  #e9efec .931   min gap 0.218
  eulbink:       #201533 .100  #252446 .152  #203562 .203  #1e579c .313
                 #0098db .488  #0ce6f2 .724  #ffffff 1.000                min gap 0.051
  rust-gold-8:   #202020 .125  #331c17 .128  #563226 .223  #393939 .224
                 #725956 .369  #ac6b26 .454  #bb7f57 .537  #f6cd26 .791   min gap 0.001

Read docs/art/aesthetic-direction.md (Direction A is LOCKED — you are testing candidates,
not overturning it), docs/data/art/palette.json, docs/RENDERING-LIGHTING.md (especially
open decision L11 at line ~705: the pool reads warm, not white), and
docs/art/palette-exceptions.md.

Reduce each candidate to a usable ramp and JUSTIFY the reduction — eulbink's bottom three
blues are within 0.10 luma and cannot all survive as distinct values.

Then capture the SAME 700-unit wave under each ramp at true gameplay zoom (Gate 1 wave 3
reaches that count; see docs/GATE1-FUN-PROTOTYPE.md). You hold the unreal-editor lock.

Judge on density readability, not on screenshots. Demichrome's 0.218 minimum luma gap is
the number to beat and it is a wide margin — a candidate that photographs better and reads
worse in a wave has lost. Also re-test L11 under eulbink: does the pool read white without
needing a palette exception?

Give ONE recommendation. "Keep demichrome" is a fully valid outcome and you should say so
plainly if that is what the captures show — do not manufacture a case for change because a
trial was requested.

Write ONLY docs/art/palette-trials.md. Do NOT edit aesthetic-direction.md, palette.json,
or any art spec — put amendments in `## Canon proposals`. No mcp__pixellab__* calls.
```
