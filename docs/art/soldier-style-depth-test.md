# Soldier style-depth test — 6 render treatments, one subject

**Run:** 2026-07-25 · **Cost:** 6 generations (1 each, budget was 2 each) · **Status:** results in, **no anchor approved**
**Requests:** `docs/data/art/requests/style-soldier-01.json` … `-06.json`
**Raw + quantized frames:** `RawArt/Renders/style-soldier-0N/r1/` (retention rule — never deleted)
**Contact sheets:** `RawArt/Renders/style-soldier-contact-sheet.png`, `…-gameplay-scale.png`

## What was being tested

Whether **rendered visual depth** can survive the locked 4-value Demichrome quantize.
`aesthetic-direction.md` Direction A specifies flat unlit art; this batch asks whether
that is a real constraint or just an untested assumption.

**The control is strict.** All six use a byte-identical composed description — the
existing, already-specced retinue militia (`docs/art/retinue-militia.md`, the subject and
`canon` block inherited unchanged from `unit-retinue.json`). The *only* variable is the
render-treatment param block:

| Cell | shading | detail | outline | style_mode |
|---|---|---|---|---|
| 01 | flat | low | selective | flat — **CONTROL**, the locked look |
| 02 | basic | low | selective | depth |
| 03 | medium | medium | selective | depth |
| 04 | detailed | medium | selective | depth |
| 05 | detailed | high | single color black outline | depth |
| 06 | detailed | high | lineless | depth |

`prompt.style_mode` was added to the schema and composer for this test. The composer
previously **hardcoded** `"flat unlit pixel art … dither for any intermediate tone"` plus
`no soft shading / no colour gradients / no anti-aliasing` into every prompt, so asking
for `detailed shading` would have measured a prompt-vs-param contradiction rather than
depth. `style_mode: "depth"` drops that clause and those three negatives. It does **not**
touch the palette clause, and it is **experiment-only** — no shipping asset may use it
without an owner exception.

## Measured result — quantized south frame

| Cell | dark% | steel% | bone% | pale% | distinct values |
|---|---|---|---|---|---|
| 01 **control** | 49.6 | 24.7 | 25.7 | 0.0 | 3 |
| 02 | 43.0 | 32.2 | 24.8 | 0.0 | 3 |
| 03 | **60.7** | 19.6 | 19.6 | 0.0 | 3 |
| 04 | 46.2 | 26.3 | 26.6 | 0.9 | 4 |
| 05 | 55.2 | 24.0 | 20.8 | 0.0 | 3 |
| 06 | 52.4 | **35.9** | 11.7 | 0.0 | 3 |

**On-palette before quantize: 0.0% in all six.** This is the third independent
confirmation of the skill's standing constraint — no tool forces a palette, the quantize
pass is mandatory, not polish.

## Findings

1. **Depth does not survive the ramp. The constraint is real, not an assumption.**
   Every cell above `flat shading` spends its extra pixels on speckle rather than form.
   More shading did not produce readable mid-tones; it produced **more Dark** — cell 03
   is 60.7% Dark against a spec asking for Bone dominance. The extra rendered information
   has nowhere to go in a 4-value ramp and collapses into the darkest bin.

2. **The control won.** Cell 01 (flat / low / selective) has the most balanced spread
   (49.6 / 24.7 / 25.7) and the cleanest silhouette after quantize. The locked Direction A
   settings are the correct ones, and this batch is the evidence.

3. **All six fail `canon.value_dominance: "bone"`** — every frame came back Dark- or
   Steel-dominant. The bone-dominance target in `unit-retinue.json` is not reachable
   through the generate-then-quantize route; it is only reachable via the authored/
   reference path where value assignment is authored rather than inferred.

4. **Cell 06 (lineless) is the one interesting outlier.** Dropping the outline entirely
   is the only treatment that pushes mass into **Steel (35.9%)** — the game's armour
   value — instead of into black outline. But Bone collapses to 11.7%, so the skin/face
   channel nearly disappears. Worth remembering if an armour-heavy unit ever needs to
   read as Steel-dominant; not usable as-is.

5. **Cell 05 confirms the outline rule.** `single color black outline` + high detail is
   the second-worst Dark reading (55.2%), matching the skill's existing note that full
   black outlining is the biggest driver of Dark dominance. `selective outline` stays
   correct.

6. **The subject's identity carriers did not land in any cell.** The prompt's
   `must_include` items — uneven shoulder line, half skullcap with bare scalp, split
   cloth/plate torso, one booted and one wrapped leg, off-vertical club — are absent or
   illegible in all six. PixelLab returned six broadly *uniform* armoured soldiers with
   matching helmets, which is close to the opposite of the "motley, never uniform" brief.
   The club survives only as detached speckle.

   This is the more important finding than the depth result: **the ragged-militia design
   is not reachable by text prompt at all**, at any style setting. It needs the authored
   anchor route.

7. **At gameplay scale the helmet is the only thing that reads.** At 1x and 2x
   (`…-gameplay-scale.png`) the body dissolves in all six and the helmet dome survives as
   the single legible shape — and it quantizes to *Bone*, the pale-mid value, so the eye
   is pulled to the helmet rather than to the face or the silhouette. Any soldier design
   that relies on body detail to distinguish itself will not survive the crowd.

## What this changes

- **Direction A's flat-shading rule is confirmed by measurement.** Do not revisit it for
  gameplay-scale sprites without new evidence.
- **`style_mode: "depth"` stays in the pipeline as an experiment-only lever**, defaulting
  to `flat`. It is now the mechanism for asking this question again cheaply if the
  quantizer or the palette ever changes.
- **The next soldier attempt should be authored, not generated.** Per stage C, an art
  spec's ASCII silhouette guide costs 0 generations, is on-palette by construction, and
  cannot come back missing the subject's defining prop — which is exactly the failure
  mode finding 6 describes.

## Not done

No anchor was approved. The stage-D human gate was not passed for any of the six, so
nothing was rotated, animated, packed, or imported. These six exist only as raw and
quantized anchor frames on disk.
