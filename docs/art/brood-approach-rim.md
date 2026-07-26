# The brood approach-rim — sprite-path readability spec

**Status:** spec only, nothing built. **Owner decision this implements (2026-07-26):**
keep `Swarm.BroodLightFloor` at 0 (full-dark horror budget preserved); add a rim/contour
mechanism instead. **Target:** the Niagara/Mass sprite path (owner committed 2026-07-26,
task #13) — explicitly NOT `TickDebugRender`, which is being deleted.
**Depends on:** `docs/RENDERING-LIGHTING.md` §2.1 (index-space quantization), §4a
(frame-selection-not-shading), §4b.5/G5 (the sprite path has no light-floor mechanism at
all today), `docs/data/art/palette.json` (`light_shift`/`dim_shift` remap, already
speced for exactly this class of effect). Read those before touching the implementation.

## Intent

Fiction: a bearer's light is the only thing that makes anything real. Brood approaching
from the true dark are, by design, not there yet — that's the horror budget the owner
just re-confirmed. But a player cannot be killed by something they had no way to parse
(design law #6). The rim is the one beat of shape-only warning that reconciles the two:
not "the tide is visible," just "something is there, before it matters yet."

Gameplay: give the sprite path *some* distance-based readability signal before a brood
crosses into the lit pool, since — unlike the debug renderer's per-pixel `Lerp` — the
sprite path has no per-unit brightness mechanism today. This is not cosmetic. It is
currently the *first* readability mechanism the sprite path will ever have for brood
(§4b.5/G5 is an open failure with nothing else queued against it).

## Correction to the literal brief: value-flip, not an outline

I was asked to spec a "thin Steel-only rim/contour." I'm specing a **full-silhouette
value swap** instead (whole body Dark → whole body Steel, same shape, no interior
line), and want to be explicit that this is a deliberate deviation, not a
simplification for convenience.

**Why a literal outline fails at this size.** At the camera settings already measured
in `RENDERING-LIGHTING.md` (§`WorldDitherScale` note: ~2400uu across ~860px ≈ 2.8uu/px)
and the brood body half-extent (14uu, ~28uu across), a brood resolves to **roughly
10px on screen** — a ~4.8x downscale from the 48px authored cell. A 1–2px contour
authored at cell resolution lands at 0.2–0.4px on screen: below one pixel, at the mercy
of exactly where each unit's sub-pixel offset happens to land under `Nearest` filtering.
Some units would show a fragment of rim, others none, and — because that offset changes
continuously as a unit walks — it would **flicker per-unit as it moves**, which is
precisely the shimmer failure `aesthetic-direction.md` §2.4 already warns about for
fine detail on movers. A contour is the wrong tool at 10px; it would read as noise,
not signal, at the exact density (hundreds of brood) this spec was asked to survive.

**A whole-body value swap has no thin geometry to lose.** At 10px, these sprites already
read as near-solid color blobs (confirmed by eye against the committed
`RawArt/Renders/soldier-roster-v1-sheet.png` reference — detail resolves at the 48px
authoring scale, not at horde-view scale). Swapping the whole fill from Dark to Steel is
robust to any downscale ratio, needs no sub-pixel alignment to survive, and still reads
as exactly the intended signal: *this shape is one value brighter than the ground it's
standing on.* Same fictional read ("catching the edge of the light before it's real"),
delivered by a mechanism that actually survives to swarm scale.

If a literal contour is wanted anyway for a closer camera state (e.g. the Unit Cam
panel, which frames far fewer units far larger), that's a separate, smaller spec — this
one is for the main view at horde density and the math above is why it isn't that.

## Mechanism

**No new sprite frames.** `docs/data/art/palette.json` already specs exactly the
primitive this needs: `light_shift` (`dark→steel→bone→pale`) and `dim_shift` (the
inverse), stated explicitly as *"a 4-entry value remap driven by one scalar on the
Unlit+Masked material, not a second texture."* That line was written for the leash-dim
case (retinue outside the leash render one value down); this is its mirror — brood
inside the approach band render one value **up**. Same mechanism, opposite sign,
same material. Don't build a second one.

Recommend one per-particle scalar, `ShiftAmount ∈ {-1, 0, +1}`, computed CPU-side in the
same loop that already builds `SubImageScratch` (`SwarmRenderActor.cpp`'s Niagara push),
and pushed as a second float array the same way `SubImages` is pushed today
(`UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat` — precedented in the
same function, not a new pattern). `M_Swarm` remaps the sampled texel's value index by
`ShiftAmount` before output. Retinue keep using it for the existing dim-on-leash-break
case (`-1`); brood use it for this case (`+1`). One mechanism, two teams, opposite signs.

**Trigger — a hard threshold, not a curve.** `ShiftAmount = +1` for a brood when
`Swarm.FlameRadius <= Dist < BandOuter`, else `0`. This is a genuinely different
question from "how does it relate to `Swarm.FlameFalloff`," and the honest answer is:
**it doesn't, on purpose.** `FlameFalloff` shapes a *continuous* per-pixel curve inside
the post-process; a per-particle material scalar is necessarily a *discrete* state, and
collapsing a curve into "on/off" is the whole reason this has to be a threshold. The two
mechanisms compose rather than share math: the rim gives advance notice in the region
where the post-process's own screen-space light-lift (§2.1) is still negligible;
`FlameFalloff` then shapes how suddenly that natural lift finishes the reveal once the
brood actually crosses into the pool. A steeper falloff (heavier dark, more sudden pool
edge) makes the rim's warning *more* valuable, not less — they're complementary, not
coupled.

- **Inner bound: `Swarm.FlameRadius`** (live, default 900uu). Below this, `ShiftAmount`
  goes back to `0` and the *natural* post-process lift is left to do the work — no
  authored frame or material shift needed inside the pool, because the screen-space
  light-lift already brightens everything there regardless of team (see the hole this
  leaves, below).
- **Outer bound: reuse `SwarmLeash::Radius`** (2000uu), not a new invented number.
  Thematically this ties the rim to the exact concept `FLAME-FOUNDATION.md` §3a already
  names — "the leash made visible" — so a brood picks up its one-step brightening at
  exactly the boundary that matters to the player's own line, not an arbitrary distance.
  Practically it costs nothing new: the constant already exists and is already the
  leash's own edge-of-relevance number. Expose it as an overridable CVar
  (`Swarm.BroodRimOuter`, `0` = fall back to `SwarmLeash::Radius`) matching the
  `0 = don't override` convention `Swarm.BroodSize`/`Swarm.RetinueSize` already use.
- **Beyond the outer bound:** `ShiftAmount = 0`, body stays Dark, matches ground. This
  is the true horror-budget zone the owner asked to keep — a brood several seconds from
  mattering gets zero signal, which is correct: design law #6 is about *imminent*
  unparseable threats, not about the tide being visible from the spawn ring.

**This requires reverting brood's authored body value for the affected rows.** Load-bearing,
not optional: a Steel rim on a Steel body has zero contrast and does nothing. The
shipped `T_Swarm_2bit` body is currently light-shifted Dark→Steel (§4b.5's documented
workaround for the *debug renderer's* lack of a floor). That workaround's reason to
exist goes away the moment this mechanism ships — the rim *is* the new floor mechanism,
and it only works if the base body is Dark so `+1` has somewhere to go. Revert the
brood body fill to Demichrome Dark for the frames used outside the pool. This also
restores `npc-silhouette-brief.md` (c)'s original silhouette-regularity mechanism
(flat Dark body, zero internal value modeling) rather than leaving the workaround as
the permanent state — worth a line in the canon proposals below since it undoes an
open, previously-unresolved owner question rather than leaving it hanging.

## Palette table

| Value | Hex | Role here |
|---|---|---|
| Demichrome Dark | `#211e20` | Body fill, rim-off state (deep dark and beyond-band). Matches ground — the intended near-invisibility. |
| Demichrome Steel | `#555568` | Body fill, rim-on state (`Swarm.FlameRadius` ≤ dist < outer bound). One step up the existing `light_shift` ladder — **no new hex.** |
| Demichrome Bone / Pale | — | **Not used by brood in either state.** Keeps brood strictly below retinue and below honest light at every distance this spec touches, and keeps the mechanism from ever competing with the flame's own brightness hierarchy (GDD pillar 4 / the brightest value belongs to honest light first). |

**Stays honest against the ramp, unlike the two existing exceptions.** `FlameCoreColor`
and `HitFlashColor` are genuine off-ramp fifth values, explicitly declared exceptions
(one cited to an owner call, one a programmer extrapolation I flagged as needing one —
see my Q2 follow-up). This mechanism introduces **no new hex** — it's a state-driven
remap between two values already in the locked four, using a remap the palette data
already specs for a different case. Nothing here needs an owner exception.

**This is a universal enemy-state marker, not a body-identity choice — canon note.**
`npc-silhouette-brief.md`'s three-way disjointness model separates factions by *which
values dominate the body at rest*. A brood that gains a temporary Steel state during
approach is not claiming Steel as an identity value the way the old "Legion" language
did (dark body + Steel rim + Bone band, at rest) — it's a transient signal, off at rest
(deep dark) and off again once lit (in-pool). If a second hostile silhouette type is
ever added under the current (faction-less) canon, it should be able to use the same
approach-rim convention without it reading as "now two things look like Legion" —
disjointness still rides on the *at-rest* value pattern per the existing audit, this
mechanism sits outside that axis by design.

## Silhouette guide

Legend per `palette.json`: `.` transparent, `#` Dark, `S` Steel, `b` Bone, `@` Pale.
Same silhouette, two states — this is a fill swap, not a redraw:

```
rim-off (deep dark / beyond band)     rim-on (approach band)
  . . # # # # . .                       . . S S S S . .
  . # # # # # # .                       . S S S S S S .
  . # # # # # # .                       . S S S S S S .
  . . # # # # . .                       . . S S S S . .
  . . # . . # . .                       . . S . . S . .
  . . # . . # . .                       . . S . . S . .
```

**Reads as:** "a shape, not yet a person" in rim-off — the same silhouette as the ground
it stands on, only legible by motion/occlusion (or not legible at all, by design).
In rim-on: "a shape one shade off the dark" — visible, still clearly subordinate to
anything Bone or Pale in frame, no detail resolved, no threat read beyond "there."

**Horde-scale check.** At 700 brood with a realistic spread across the approach band:
reads as a scatter of small solid dim-gray shapes at irregular positions and (thanks to
`Swarm.BroodSeparation`) real gaps of true Dark between them — not a wall, not a solid
field, not a moiré pattern, because there is no sub-pixel geometry in this mechanism for
density to turn into noise. The risk named in the brief (hundreds of rims overlapping
into a field of noise) is specifically a risk of the *contour* implementation, which is
why this spec doesn't use one. A value-swap mechanism has no equivalent failure mode —
worst case at high density is "a lot of dim shapes," which is exactly the intended read
of an oncoming tide, not a rendering failure.

## Sheet layout

**None needed.** This is the point of routing it through the material remap instead of
new baked frames — `T_Swarm_2bit`'s existing 8×4 grid, row/column map, and SubUV
indexing are all untouched. Only two changes outside the texture:
1. Revert the brood body fill in the existing sheet (rows 0–1, `RowBroodWalk0/1`) from
   Steel-dominant back to Dark, per the "requires reverting" note above.
2. Add the `ShiftAmount` per-particle float array and the material-side remap node
   (shared with retinue's `dim_shift`, opposite sign).

**Fallback, if a per-particle material scalar turns out not to be practical in the
current Niagara setup:** duplicate `RowBroodWalk0/1` into two new rows with the fill
pre-baked Steel instead of Dark, and use `SwarmAnim`'s free bit (`1 << 7` — every other
bit 0–6 is already spoken for; see `SwarmFragments.h`) to select between them exactly
like `TeamBit`/`FrameBit` already select rows today. That takes the sheet from 8×4 to
8×8 (next power of two, pipeline-legal), costs 2 new rows of real art (not 4 — retinue
untouched), and leaves 2 rows spare. I'd only reach for this if the material approach is
blocked; it's strictly more expensive for an identical visual result.

## Animation notes

No new animation. Brood keep their existing 2-frame walk cycle in both rim states —
this is a value swap on an existing silhouette, not new art. Attack (still a lunge via
position, no dedicated frame) and hit (still not decoded on the sprite path at all,
per `SwarmSheet`'s own header note) are unaffected and outside this spec's scope.

## Related gap this spec does not fix — flagging for task #13

The debug renderer's `Swarm.BroodLightCeil` (brood capped below retinue's brightness at
every distance, so a soldier beside a brood always reads as the lit one) has **no
equivalent on the sprite/material path** — the post-process's screen-space light-lift
doesn't distinguish team, so a brood deep in the pool could in principle reach the same
value as an adjacent soldier once cutover happens. Same category of problem as this
spec (per-team distinction needs per-unit material work the sprite path doesn't have
yet), but a different mechanism and out of scope here. Naming it so it doesn't get
missed when task #13 scopes what has to exist before cutover.

## Depends on

- **#5 (flipbooks vs. flat-shaded 3D):** assumes flipbooks on instanced quads (the
  current architecture). A flat-shaded 3D path would deliver this differently
  (a real material light term rather than a remap) and this spec would need re-deriving
  under it.
- **#6:** N/A — resolved, strict global palette, unaffected either way.

## Canon proposals

1. **Reverting brood's authored body to Dark for the non-lit state resolves the open
   question in `docs/RENDERING-LIGHTING.md` §4b.5** ("the shipped brood is not
   Dark-dominant, which is a deliberate deviation... and wants an owner ruling"). This
   spec's mechanism is the missing piece that lets that reversion happen without
   reintroducing the invisibility problem it was worked around for. Recommend closing
   that open question in favor of the revert, contingent on this spec shipping.
2. **The approach-rim as a named, reusable "enemy state" convention** (not a body
   identity) is proposed for whatever silhouette-language document eventually replaces
   the discarded WORLD.md-era faction framework — worth a line so a future second
   hostile type inherits the convention deliberately rather than by accident.
