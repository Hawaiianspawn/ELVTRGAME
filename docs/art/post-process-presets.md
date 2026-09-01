# Battle post-process presets

Evidence probe for task-168: a togglable full-screen post-process stack on the Godot
battle scene, so the owner can see stylistic options on screen before any one of them
gets adopted. One `canvas_item` shader (`godot/assets/shaders/post.gdshader`) reading
`hint_screen_texture`, mounted by `godot/scripts/Post.gd` on a `CanvasLayer` at layer
10 — above the world/props/fx layer (1) so those get processed, below the HUD
text/army-panel layer (20) so those don't. Cycle with `[` / `]` in the battle scene;
the current preset name is appended to the on-screen HUD readout.

Probe: `godot --path godot -- --probe=battle,12,post=N` applies preset `N` after a
1s settle and saves `user://probe_battle_postN.png`. Contact sheet and per-preset
captures below are in `godot/RawArt/Renders/post-process/` (seed/time held constant
across all six, `battle,10,post=N`).

Contact sheet: `godot/RawArt/Renders/post-process/contact.png`

| # | Name | What it does | PROBE line | Unit readability |
|---|------|--------------|-----------|-------------------|
| 0 | off | Pass-through — `Post.gd` hides the `ColorRect` entirely, shader never runs. | `PROBE battle fps=281 process_ms=6.2 physics_ms=0.0 units=73 objects=509` | baseline |
| 1 | dither | Quantizes to `steps` (default 6) levels per channel with a 4x4 ordered Bayer dither at the threshold, so bands don't hard-step. | `PROBE battle fps=315 process_ms=5.0 physics_ms=0.0 units=74 objects=502` | Neutral — silhouettes stay crisp, only flat gradients (wall, floor, glow falloff) pick up visible dither texture. |
| 2 | glow | 12-tap ring blur of pixels over a brightness threshold, added back over the scene, plus a corner vignette. | `PROBE battle fps=305 process_ms=4.3 physics_ms=0.0 units=73 objects=491` | Better for mood, neutral for readability — the lanterns and hero's flame bloom convincingly; unit silhouettes are untouched since blur only picks up already-bright pixels. |
| 3 | grade | Cool-green tint mixed into shadow luminance, warm lift blended in near screen centre (where the hero stands). **Shipped default since 2026-08-31 (owner call)** — Post.gd starts on it; `[`/`]` still cycle, 0 = off. | `PROBE battle fps=312 process_ms=4.4 physics_ms=0.0 units=75 objects=468` | Neutral — subtle at this contrast; sells the necromancer-hall cold/hero-warm split without changing shapes. |
| 4 | crt | Mild barrel curvature on the sample UV + per-row scanline darkening + RGB aperture-grille phosphor mask (1.25 gain compensates). | `PROBE battle fps=295 process_ms=6.0 physics_ms=0.0 units=72 objects=513` | Worse — the scanline darkening drops overall scene brightness enough that back-rank units read darker/flatter than the other presets; fine for a stylized vignette but costs the crowd some legibility. |
| 5 | retro | The whole cabinet in one: barrel curvature + edge chromatic aberration + demichrome-4 dither per channel (the offsets re-fringe the greys) + scanlines + phosphor mask. | `PROBE battle fps=190 process_ms=7.2 physics_ms=0.0 units=156 objects=883` | Strongest stylization of the set; aberration fringes and scanlines cost fine text some sharpness at the frame edges. |
| 6 | dusty4 | The dither preset on the [dusty4](https://lospec.com/palette-list/dusty4) ramp (night purple / slate blue / sage green / bone white) instead of demichrome-4: same 8x8 Bayer + luma quantize model. Dark step deepened from the published `#372a51` to `#241b36` per owner call. | | Same readability behavior as dither; the green midtone plays into the necromancer glow, the deep purple floor sits near-black without going pure black. |

The Sobel "edges" preset (old #4) was cut 2026-08-31 — owner call: it webbed the packed
crowd. Presets after it renumbered (crt 5→4, retro 6→5, dusty4 7→6); `--probe=battle,N,post=N`
numbers follow the new table.

FPS note: baseline (off) measured 281 fps in this run; every preset measured 295-315
fps, i.e. no preset dropped more than 10% below the off baseline (these are uncapped
headroom numbers with normal wave-to-wave variance, not a regression). `process_ms`
stayed in the 4-6ms band across all six.
