---
id: 041
title: Build Phase B — the index + palette-ID LUT pipeline
status: proposed
agent: claude
owns: ["ELVTR/Content/PostProcess/**", "ELVTR/Source/ELVTR/Rendering/**", "ELVTR/Source/ELVTR/UI/EmberkeepPalette.h", "docs/RENDERING-LIGHTING.md", "Scripts/art/pixelpipe.py"]
resources: ["unreal-editor", "mcp-9000"]
depends-on: []
evidence: The whole game re-coloured by editing one LUT row — demichrome and a candidate ramp swapped live on screen, with no sprite regenerated and no shader recompiled.
score: {feel: 1, risk: 3, cost: 4}
source: docs/RENDERING-LIGHTING.md:21
decided: ""
---

## Why now
Owner decision, 2026-07-26: palette should be something you change in the post-process,
not something baked into art. `docs/RENDERING-LIGHTING.md` §2.1 specced exactly that on
2026-07-10 and it was never built — Phase A shipped luminance-based, *"no index buffer
yet."*

Today the palette lives in **three** places, so a swap is three edits plus re-quantizing
every sheet:

| Where | What holds the hexes |
|---|---|
| World | `M_PP_Demichrome`'s Custom node LUT step |
| Sprites | **Baked into the textures.** `pixelpipe.py` quantizes to literal hexes; `M_Swarm` is Unlit+Masked sampling `T_Swarm_2bit` directly |
| UI | `EmberkeepPalette.h:20-23`, hardcoded — UMG draws after post and is never quantized |

The spec's own promise, lines 38–40: *"Per-faction palettes are LUT rows; a strict global
palette is a LUT with one row. The code is identical; the choice becomes art data."*

**What forced this now.** Two candidate palettes were measured against the quantizer's
own luma model (Rec.709, gamma sRGB) on 2026-07-26:

| Palette | Min luma gap | Verdict |
|---|---|---|
| demichrome-4 (locked) | **0.218** | evenly spread — this is why it survives horde density |
| eulbink-7 | 0.051 | monotonic; a real ramp, tight at the dark end |
| rust-gold-8 | **0.001** | **impossible under a luminance quantizer** |

Rust-gold is not a value ramp. `#202020`/`#331c17` are luma 0.125/0.128; `#563226`/`#393939`
are 0.223/0.224. Those pairs land in the same bucket and collapse to one colour. It
separates by **hue**, and a luminance pass is structurally blind to that. Phase B is not
a nicety for it — it is the only way it can ever render.

**Cheaper than it looks.** Because `pixelpipe.py` already enforces exactly four known
hexes, converting the existing sprite library to indices is a deterministic remap —
hex→index lookup, **zero PixelLab generations, no re-authoring.** The expensive-sounding
part of Phase B is already paid for by the quantize discipline.

## Done when
- Scene renders `(value index, palette ID)` into two channels with wide separation,
  quantized on read so it survives the pipeline (§2.1).
- The post pass converts attenuation to a **value-step shift**, Bayer threshold deciding
  the rounding — replacing the current luminance lift, without regressing what Phase A
  already got right (`§4b.7` finding 1: light must **lift**, not scale).
- A LUT texture maps `(palette ID, shifted index) → RGB`. Demichrome is row 0.
- `pixelpipe.py` emits index PNGs alongside the RGB ones; the existing library is
  converted by remap, not regenerated.
- `EmberkeepPalette.h` reads the same source rather than holding a fourth copy of the
  hexes. UMG draws after post, so it needs the LUT applied CPU-side — say how.
- **Proof:** swap demichrome for a candidate ramp by editing one LUT row, live, with no
  sprite regenerated and no shader recompiled. That demo is the deliverable.
- Reserved values can be declared **light-exempt** in the LUT (§2.1) — the mechanism that
  lets marks read through darkness.

## Watch for
- **L2 is the real unknown** and is flagged open in §5: does the index encoding survive
  the tonemapper? Lean is wide-separation encode + quantize-on-read, with a custom pass
  only if it bands. Test this **first** — if indices don't survive, the rest is moot.
- Adding a `UPROPERTY` via Live Coding reports success then crashes the next PIE. Class
  layout changes need a full editor-closed rebuild.
- MCP `set_properties` on a Custom node's `code` silently no-ops and returns true. Read
  the code back and compare length before recompiling or saving.
- This does **not** change the locked palette. It changes how cheaply the locked palette
  could be changed. Direction A stays locked until the owner says otherwise.

## Spawn prompt
```
You are implementing Phase B of Emberkeep's rendering plan (C:\Projects\ELVTRGAME).

Read docs/RENDERING-LIGHTING.md in full first — §2.1 is the spec, §4b.7 is what Phase A
measured, §5 lists the open decisions (L2 is yours to answer). Also read
docs/data/art/palette.json and docs/art/palette-exceptions.md.

Goal, in the spec's own words: "Per-faction palettes are LUT rows; a strict global palette
is a LUT with one row. The code is identical; the choice becomes art data."

Today the palette is baked in three places: M_PP_Demichrome's Custom node, the sprite
textures themselves (pixelpipe.py quantizes to literal hexes; M_Swarm is Unlit+Masked
sampling T_Swarm_2bit directly), and ELVTR/Source/ELVTR/UI/EmberkeepPalette.h:20-23.

DO THIS FIRST, before building anything: answer open decision L2 — does a wide-separation
index encoding survive the tonemapper intact? If it does not, stop and report; every other
part of this task depends on it and a custom pass changes the cost completely.

Then: index+paletteID buffer, value-step shift in the post pass (keep §4b.7 finding 1 —
light must LIFT the value, not scale it), LUT texture with demichrome as row 0, pixelpipe
emitting index PNGs, and EmberkeepPalette.h reading one source instead of holding a fourth
copy.

Converting the existing sprite library is a deterministic hex→index remap because
pixelpipe already guarantees exactly four known hexes. Do NOT regenerate any sprite and do
NOT call any mcp__pixellab__* tool — this task holds no credits lock.

You hold the unreal-editor and mcp-9000 locks.

Gotchas that will bite: Live Coding cannot add a UPROPERTY (reports success, crashes the
next PIE — needs a full editor-closed rebuild). MCP set_properties on a Custom node's
`code` silently no-ops and returns true — always read back and compare length before
recompiling or saving.

Deliver a RUNNABLE BUILD demonstrating the payoff: swap demichrome for another ramp by
editing one LUT row, live, no sprites regenerated, no shader recompiled. On-screen
evidence, not a diff plus "it works".

This does NOT change the locked palette. Direction A stays locked. You are making a change
cheap, not making it.
```
