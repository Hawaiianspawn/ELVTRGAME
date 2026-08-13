---
name: variants
description: Build a family of unit variants that provably read apart — pick a base character, choose a silhouette axis, generate states through PixelLab, then measure aspect/solidity/asymmetry and publish a contact sheet for the owner's verdict. Use when the user runs /variants or asks for variety, states, variants, more units of a type, a roster, enemy tiers, or wants existing variants judged for whether they actually look different.
---

# variants — a family that reads apart

`/sprite` takes one request to one imported texture on the ramp. **This skill is the other
job:** given a base character that already looks right, produce N siblings that a player can
tell apart at panel scale, and prove it with numbers before anyone spends a review cycle.

Read `.claude/skills/sprite/SKILL.md` §"Hard constraints" first. It owns the PixelLab mode
rules (v3 ignores `proportions`, `standard` honours them, the v3-reference style lever, the
retention rule). This file does not repeat them; it adds what governs *families*.

**`Scripts/art/variantpipe.py` codifies steps 2-8 of "The loop" below (task-081).** Write the
family as `docs/data/art/families/<family>/family.json` against
`docs/data/art/family.schema.json` -- base character, the named axis, what's held constant,
and per-variant slug/silhouette_target/edit_description -- then run:

```
py Scripts/art/variantpipe.py plan   <family>     # emits mcp__pixellab__* tool+kwargs to run
py Scripts/art/variantpipe.py fetch  <family> <variant> --url URL [--url URL ...]
py Scripts/art/variantpipe.py judge  <family> [--json] [--cull]   # §Judging below, automated
py Scripts/art/variantpipe.py report <family>                    # the contact sheet, verdicts included
```

`judge` only auto-rejects the unambiguous cases in §Judging; everything else it can't decide
(lost a prop, stopped reading as the unit type) it leaves for you to look at on the contact
sheet -- it never claims to replace step 8's eyeballing, only to make it cheap.

## The one idea

**Vary the silhouette, not the texture.** Everything else follows from it.

Measured 2026-07-28 on the shipped knight group: states 01, 02, 05 and 06 have an
**identical 1,093-pixel outline**. Only 03 and 04 move it, by ~3%. That group looks varied
on a contact sheet and collapses into one figure at panel scale, which is why Knight 06 came
back byte-identical to 05 — the axis had run out. The brood group is worse: three of its
states (`A_black_ghost_with_sharp`, `Baby_face`, `made_of_marble`) share an identical
1,021-pixel outline, and all eleven sit inside aspect 0.86–0.91. Eleven "different
creatures", one silhouette.

Interior detail is the part a small, dark sprite carries **least**. Spend the variance where
it survives.

## Order of axes — spend them in this order

1. **Aspect** (width/height). The strongest and cheapest. Push to a real spread; 0.50–1.56
   across eight brood forms is a 3.1× range and every one is separable.
2. **Topology**, once aspect is crowded. Change the *kind* of outline, not the ratio: a
   jagged top edge with air gaps between spikes; a body split by a notch; an asymmetric
   form with a front and a back. Asymmetry 0.69 on one form against 0.00–0.29 on all its
   siblings is instant separation at any size.

   **A notch is not a hole.** `silhouette_report.py` counts enclosed background regions, and
   only a gap the background *cannot flow into* changes topology. Measured on the Vanguard:
   a wide straddle and a shield held out to one side produced **zero** holes — both gaps open
   to the sprite's edge — while arms akimbo and a spear raised overhead produced two each.
   If you want a hole, the gap must be closed on every side.

   **And a hole must be big enough to survive downsampling.** On an 88 px canvas with ~35 px
   of content, a figure's natural gaps run 10–20 px. Akimbo's 10 px holes close to ~2 px at
   panel scale and disappear; Overhead's 19 px arch is about the floor. **For a reliable
   hole, hold an object away from the body** — the ranged Kite's detached drone — rather than
   relying on an armpit.

   Topology moves *shape*, not *ratio*: the Vanguard topology batch spread aspect only 1.5×
   against the primitive batch's 3.2×. That is not a failure, it is the axis doing its job —
   three of those states sat at aspect 1.16–1.18 and still read completely apart, separated by
   asymmetry 0.60 and by hole count. **Which is the argument for never judging on aspect
   alone.**
3. **Interior / kit** last, and only as a tiebreaker between two shapes that already differ.

Never open with 3. That is the mistake the knight and brood groups both made.

### Simplify the silhouette rather than loading it up

Counter-intuitive and measured on the Vanguard, 2026-07-28. Two batches off the same base:

| Approach | aspect spread | rotation drift |
|---|---|---|
| **Adding** gear — pauldrons, tower shield, banner, pike | 1.6× | up to 0.48 |
| **Stripping** to one nameable primitive each | **3.2×** | as low as 0.05 |

Adding gear makes the outline *busier*, not more distinct, and every protrusion is a new thing
to foreshorten as the sprite turns. Ask instead for one primitive per variant — square, dome,
column, bar — with the weapon stowed, limbs tucked, and **no gaps**. Simple shapes differ more
legibly than complicated ones and they hold through all eight facings.

**Solidity then tells you which primitive you actually got**, because each has a known ideal:
rectangle 1.00, half-disc 0.785, triangle 0.50. Measured returns were bar 0.98, square 0.96,
dome 0.81, triangle 0.55 — five of six within a few points of target. A triangle reading 0.55
is correct, not gappy. Read solidity against the intended shape, never as a bare number.

### But there is a ceiling, and it is the point of the unit

Push simplification all the way to a pure primitive and the unit **stops being a unit**. Of
the six above, Dome came back as a plain metal egg, Bar as a framed plaque with gold trim that
was not even in the base palette, and Triangle as a shield or arrowhead. Three of six were
furniture. Square and Column survived because they kept a **visible head and legs**.

That is the `archer-04` failure in a new costume: distinctiveness bought by destroying the
type read is worth nothing. **The rule: simplify to the simplest shape that still keeps a head
and legs.** If you cannot name the unit from its outline alone, it has gone too far — reject it
however good its aspect number is.

## Which tool — this is the whole decision

| Want | Use | Gets you | Costs you |
|---|---|---|---|
| Big silhouette range, style irrelevant | `create_character` **standard** + custom `proportions` | Real mass control: 2.4× width spread | Wrong palette, wrong canvas, **drops props** — 3 of 6 lost their weapon |
| Family cohesion, style must match | `create_character_state` on the approved base, `use_color_palette_from_reference=True` | Inherits palette, lighting, canvas, prompt fidelity | On a **skeletal** base, silhouette range collapses (2.4× → 1.2×) |
| A new individual in an existing style | `create_character` + v3 reference image | See `/sprite` — not a state | — |

**The asymmetry that decides it, and it is not obvious:**

- **Skeletal (humanoid) base** → states can only *pad* the body with gear. Range caps hard.
  Measured: Bulwark and Swarmcaller rebuilt as states went from 2.4× width spread down to
  1.2×. Take the style anyway — a unit that does not match the roster is unusable however
  good its outline, and a tamer outline still reads.

  **A humanoid will not go squat. Stop asking.** Three separate attempts on the Vanguard —
  "shieldwall crouch" (got 1.02 against a 1.30 target), "heavy skirt plates" (0.82 against
  1.00), "squat immovable heavy" (0.89 against 1.30) — all undershot, and the last came back
  with solidity identical to the base's across all eight facings. The same wording made the
  ooze hit 1.56 first time. The skeleton will not compress vertically.

  What *does* work on a humanoid, both measured on the same base:
  - **Narrow** — tuck the arms in, drop the shield, extend upward. Pikeman reached aspect
    0.58 at 28 px wide, and held 0.46–0.58 across every rotation.
  - **Wide via a held prop** — extend outward with something rigid rather than inflating the
    body. A spear held level got 53 px, the widest knight produced all session, at solidity
    0.45 and a rotation drift of just 0.10.

  So on a skeletal base the axis is **narrow ↔ wide-by-extension**, never tall ↔ squat.
- **Amorphous base (ooze, blob, cloud, swarm)** → there is no skeleton to preserve, so
  states deliver the **full** range *and* the inherited surface. Measured: 3.1× aspect
  spread on the brood with correct palette and canvas. **For amorphous units, states beat
  proportion knobs outright** — there is no trade to make.

## Two traps that cost real generations

- **`size` is not the canvas.** The canvas comes back ~40% larger. Asking `size 92` returned
  **132×132**, unusable next to a roster of 92 px sheets. For a 92 px canvas request
  `size 64`. Check the returned size before you queue seven more.
- **`standard` treats every description detail as soft guidance.** It is the only mode with
  `proportions`, and it will silently drop the weapon, the drones, the prop that defines the
  unit — and add a glowing visor you explicitly banned. If a prop is load-bearing, standard
  mode cannot be trusted to keep it.

## The loop

1. **Pick the base.** An already-approved character whose style is the target. Confirm its
   canvas size and `group_id` with `get_character`. Everything inherits from here.

   **Prefer a base without a large dominant prop.** Whatever the parent always carries becomes
   a constant in every child and eats the range. Measured: four states off the plain Vanguard
   spread 2.1× in aspect; four states off a tower-shield variant of the same soldier spread
   **1.3×** — at the do-not-separate threshold — because the ~20 px shield is present in all
   of them and anchors the mass. The narrow state could not go narrow; the wide state only
   separated because its blade reached past the shield. If a prop-carrying variant must be the
   seed, plan to **remove the prop** in the states that need to move.
2. **Name the axis before writing prompts.** Write the variance table: which variable each
   variant moves, what every variant holds constant, and how many the axis supports before
   they stop separating. If you cannot say what makes variant 5 different from variant 1 in
   one measurable phrase, do not queue it. **This table IS `family.json`'s `axis` and
   `constant` fields** (task-081) — write it there, not just in chat, so the next family
   starts from a record instead of zero.
3. **One `edit_description` per variant**, each stating its silhouette target in plain
   geometric terms ("two and a half times wider than tall", "air gaps between the spikes",
   "the two sides must be clearly different, not mirrored"). Always
   `use_color_palette_from_reference=True` for a family. Always ban glow explicitly —
   `aesthetic-direction` reserves light for the flame. These go in `family.json`'s
   per-variant `edit_description` and (where the target is a number) `silhouette_target`;
   `variantpipe.py plan` reads them back out as the `mcp__pixellab__create_character_state`
   kwargs to run.
4. **Download every rotation first**, before judging anything:
   `RawArt/Renders/<family>/raw/<variant>/rotations/*.png`. Standing project rule, and it
   means an upstream delete cannot lose your evidence. `variantpipe.py fetch <family>
   <variant> --url ...` does this and records the urls in the family's manifest.json.
5. **Measure.** `py Scripts/art/silhouette_report.py RawArt/Renders/<family>/raw`
   Reports aspect, solidity, asymmetry, luma, colours — and flags any pair sharing an
   identical outline, which is the failure this skill exists to prevent. `--json` emits the
   same numbers machine-readably; `variantpipe.py judge` is built on it.
6. **Check rotations before any verdict** — not just on suspicious variants:
   `... --all-directions`. Everything in step 5 is a single frame, and a single frame will
   lie to you.

   **Worked example, 2026-07-28.** A variant briefed as a long low ridge measured aspect
   **0.86** from the south and was written off as a failure. Across its rotations it runs
   **0.59 to 2.60** — from east and west it is exactly the 2.5× log it was asked for. South
   is the view *down its length*, where a log is correctly stubby. It was the best variant in
   the batch and nearly got rerolled for free. (This is brood-ooze's `state05_ridge` --
   `docs/data/art/families/brood-ooze/family.json` records its target as `2.5`, and
   `variantpipe.py judge` checks a numeric target against the whole 8-direction band, never
   the south value alone, for exactly this reason.)

   Read the drift both ways: on a form meant to be uniform, large drift is a defect; on a
   directional form, **the drift is the feature** — it is the only thing in that family whose
   read changes with facing. `judge`'s band-containment reject (§Judging) refuses to use a
   variant with drift over 0.45 as the *reference* another variant gets compared against, for
   the same reason: its huge natural range would otherwise swallow siblings that plainly
   read as different shapes.
7. **Hand over a contact sheet.** Each variant renders next to its flat outline, because
   for a dark unit the outline is what a player actually receives. Per project convention,
   big work is handed over as on-screen evidence, never as a written "it works".

   Two routes, same measurements:
   - **`py Scripts/art/forge.py --family <family>`** — the live page. Prefer this whenever
     the owner is at the machine: verdicts are clicked rather than typed back in chat, they
     land in the manifest as structured data, and the owner can re-roll a near-miss without
     spending a turn. Anything you generated over MCP and landed with `fetch` is already
     on it — same folder, same layout, no sync step.
   - **`variantpipe.py report <family>`** → `--out sheet.html` → the `Artifact` tool — the
     static sheet, for an async handover or a record to link from a task.
8. **Report with the numbers and name the failures.** State which variants landed, which
   undershot their target, and which to reroll or drop. A variant that missed is a finding,
   not something to quietly omit. `judge` automates the unambiguous half of this (§Judging);
   it flags rather than decides everything it can't check by measurement alone (lost a prop,
   stopped reading as the unit type) — that verdict is still yours to make off the sheet.

   **Read `variants.<slug>.owner` before re-deciding anything.** `verdict` in that manifest
   is judge's measurement call and it is rewritten on every run; `owner` is the human's and
   it is not. Where the two disagree, the owner's stands — a variant judge calls redundant
   can still be the one that reads best in a crowd. Never overwrite the `owner` block, and
   never present a judge verdict as the owner's.

## Judging — thresholds that have held up

- **aspect spread** across the family: 2.5×+ is good, under 1.3× means they will not separate.
  **Not a gate** — archer-scifi spreads only 1.5× and separates on asymmetry instead (below).
  `variantpipe.py judge` never scores aspect spread alone for this reason (task-081).
- **identical opaque counts**: an automatic fail. Two variants with the same outline are one
  variant. `judge` auto-rejects this (keeping whichever name looks like the tracked
  `state00`/`t0`/`p0`-style convention over a raw PixelLab auto-name, when exactly one of a
  tied group is tracked).
- **asymmetry** above ~0.4 marks a form with a front and a back; 0.00–0.10 is another blob.
- **solidity** well below its siblings means a genuinely gappy shape — worth more than any
  aspect change of the same magnitude. `judge` scores aspect, solidity, asymmetry and hole
  count together (as a per-variant band across all 8 rotations), and auto-rejects a variant
  whose whole band sits inside a non-directional sibling's on every one of those axes.
- **luma mean** under ~0.20 on a dark panel means only rim highlights reach the player. Not
  automatically wrong (the brood is meant to be near-black), but it makes the outline
  load-bearing, so weight the silhouette column accordingly. `judge` notes this, it does not
  reject or flag on it.
- Missing rotations, or a returned canvas that doesn't match the family's declared size, are
  also automatic `judge` rejects. Everything else — undershot a stated target, lost its prop,
  stopped reading as the unit type — comes back `flag`ged for the owner, never auto-rejected;
  no metric here can see those failures (task-081 §Findings 3).
- **A variant something under `docs/data/art/**.json` names as a `source` (or `state`) is
  never cullable**, whatever the measurement says. Measured 2026-07-29: brood-ooze's
  `state00_base` genuinely bands-inside `pale_blue_skin_big_e` and would otherwise auto-reject
  — but `docs/data/art/brood-variants.json` (task-059) consumes it at index 0, weight 14, so
  culling it would point a live document of record at a moved path. `judge` downgrades this
  to `keep` with both facts stated ("mechanically redundant but externally consumed") rather
  than silently skipping the cull, so the reason stays visible to the next reader too.

## Costs actually observed

| Operation | Generations |
|---|---|
| `create_character_state` @ 88–92 px | **~20–40** (Gemini tier, scales with canvas) |
| `create_character` standard | 1 |

A batch of six states is 120–240 generations; six standard characters is 6. Prototype the
*axis* in standard mode when only shape matters, then rebuild the winners as states for
style. Say the estimate out loud before queueing a batch.

## Do not

- Do not queue a batch before the base's canvas size is confirmed.
- Do not judge a family from one frame if anything in it is asymmetric.
- Do not report "they look varied". Report the aspect spread.
- Do not pack or import anything under this skill. It ends at the owner's verdict; `/sprite`
  owns packing, quantizing and UE import.
