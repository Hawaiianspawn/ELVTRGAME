# Sci-fi soldier silhouette variants (task-056)

**Status: rev 1 ON HOLD (superseded by rev 2's approach) · rev 2 GENERATED, compromised by a
crop bug (see "Rev 2" section) · rev 2b DIAGNOSED, NOT GENERATED — the fix is written up below
but `RawArt/Renders/soldier-scifi-variants/rev2b/` does not exist on disk. A prior pass through
this doc claimed the fix was "confirmed and applied as rev 2b"; that was false when written and
is corrected here — no rev2b generation has been queued.** Rev 1's owner verdict was "we are too
similar"; rev 1's root cause is structural (documented below) and rev 1 itself is not being
touched further. Rev 2 used the anchor-first approach the owner chose, and it surfaced a
different, self-inflicted issue in all 6 of its variants — flagged plainly, not glossed over, in
the Rev 2 section.

**Base:** PixelLab character `afa5582e-c649-49cc-96de-677e6f9869dd` ("Make brighter pallet"),
88x88, 8 directions, low top-down, mannequin template, group `8b4b9811-dcf3-4e9d-baa6-2cb3f8901d0b`
(confirmed via `get_character` before queueing anything). **Skeletal humanoid armoured soldier** —
per `.claude/skills/variants/SKILL.md`, this caps the proportion axis hard and rules out squat/
crouch entirely. What this batch adds to that finding: on a `create_character_state` call, the
cap is not just tight, it is **absolute** (see Root cause below).

All six are `create_character_state` on `afa5582e` with `use_color_palette_from_reference=True`.
No `create_character`, no `proportions` knobs, no shape-study pass — the owner's states-only
decision from the spawn prompt, not re-litigated here. Glow banned in every prompt. Every image
verified at 88x88 on generation.

Contact sheet (rendered + flat outline per variant, all measurements):
**https://claude.ai/code/artifact/96835bd9-c830-4900-9e60-feb258386f14**

Raw renders: `RawArt/Renders/soldier-scifi-variants/raw/<variant>/rotations/*.png` (all 8
directions downloaded to disk before any measurement, per the retention rule — nothing has been
deleted, nothing has been rerolled).

---

## Variance table (written before any prompt was sent)

| # | variant | axis | single variable moved | held constant | target |
|---|---|---|---|---|---|
| 1 | prop-narrow | proportion | arms tucked to sides, rifle stowed flat on back, legs together | palette, canvas, kit, camera | aspect ~0.55-0.70 |
| 2 | prop-wide | proportion | rifle gripped rigid, held fully extended horizontal to one side | same | aspect ~1.3-1.6 via extension, not inflation |
| 3 | prop-mid | proportion (anchor) | relaxed neutral guard stance, rifle held vertical in front — minimal deviation from base mass | same | aspect ~0.9-1.1, midpoint |
| 4 | topo-asymmetric | topology | front arm/leg thrust forward in a lunge, back arm/leg pulled back — front half/back half read as different shapes | same | asymmetry > 0.4 |
| 5 | topo-hole | topology | small drone held away from the body on a rigid rod, gap closed on every side | same | >=1 hole, ~19px+ |
| 6 | topo-notched | topology | torso split into two armor halves by an open vertical gap + jagged shoulder spikes | same | lower solidity, visibly notched/split |

---

## Measured results

### South frame (the print_table baseline)

| variant | content | aspect | solidity | asymmetry | holes | luma | colours |
|---|---|---|---|---|---|---|---|
| prop-mid | 29x45 | 0.64 | 0.70 | 0.14 | – | 0.313 | 58 |
| prop-narrow | 32x48 | 0.67 | 0.66 | 0.07 | 1 (10px) | 0.321 | 55 |
| prop-wide | 43x45 | 0.96 | 0.50 | 1.04 | – | 0.292 | 62 |
| topo-asymmetric | 35x49 | 0.71 | 0.56 | 0.54 | – | 0.306 | 56 |
| topo-hole | 50x45 | 1.11 | 0.50 | 1.06 | 1 (7px) | 0.305 | 59 |
| topo-notched | 40x45 | 0.89 | 0.67 | 0.31 | 1 (7px) | 0.315 | 57 |

**South-only aspect spread: 0.64-1.11, a 1.7x range. Width spread: 29-50px, also 1.7x.** This is
the single-frame view, and per `.claude/skills/variants/SKILL.md` §6 a single frame is not the
verdict — see below for the all-rotation figure, which is the one that leads. Zero pairs share an
identical opaque-pixel count on this frame, so the automatic-fail check PixelLab's own knight
group failed does not trigger here.

### All 8 rotations (`--all-directions`)

| variant | aspect range | drift | solidity range |
|---|---|---|---|
| prop-mid | 0.50-0.65 | 0.15 | 0.57-0.70 |
| prop-narrow | 0.48-0.70 | 0.22 | 0.61-0.75 |
| prop-wide | 0.86-1.20 | 0.34 | 0.39-0.50 |
| topo-asymmetric | 0.69-1.09 | 0.40 | 0.42-0.56 |
| topo-hole | 0.76-1.16 | 0.40 | 0.43-0.50 |
| topo-notched | 0.52-0.89 | 0.37 | 0.54-0.67 |

**Across every rotation of every variant, aspect ranges from 0.48 (prop-narrow, east/west) to
1.20 (prop-wide, north-east) — a 2.5x spread — and width from 22px to 55px, also 2.5x. This is
the headline number**, corrected from an earlier pass through this doc that suppressed it in
favor of the south-only figure — that was wrong, and it contradicts the exact methodology rule
this family is measured against (`.claude/skills/variants/SKILL.md` §6: a single frame lies, as
proven by a prior batch's ridge variant that measured 0.86 from the south and 0.59-2.60 across
its rotations, and was nearly rerolled as a failure when it was the best variant in that batch).

**Report both numbers — neither is suppressed — and the gap between them is itself
informative:** 2.5x across all 8 facings against 1.7x head-on says this family's separation is
**facing-dependent** — real when the camera turns, weaker in the single view a player sees most
often. That is a legitimate thing to know about the family, not a discrepancy to paper over by
picking whichever number is more convenient.

Full per-variant, per-direction detail (content px / aspect / solidity / asymmetry / holes):

```
prop-mid        south 29x45 .64/.70/.14   se 29x47 .62/.63/.52   e 29x46 .63/.57/.64
                ne 27x46 .59/.63/.41      n  28x44 .64/.69/.02   nw 30x46 .65/.58/.50
                w  23x46 .50/.70/.27      sw 26x47 .55/.69/.25
prop-narrow     south 32x48 .67/.66/.07 (1 hole 10px)  se 33x48 .69/.61/.22
                e  22x46 .48/.75/.31      ne 28x47 .60/.65/.36   n 32x46 .70/.64/.01
                nw 28x47 .60/.65/.35      w  22x46 .48/.75/.31   sw 31x48 .65/.64/.24
prop-wide       south 43x45 .96/.50/1.04  se 42x49 .86/.46/1.28  e 41x46 .89/.45/1.28
                ne 55x46 1.20/.39/.48     n  44x44 1.00/.49/1.05 nw 42x46 .91/.46/1.22
                w  41x46 .89/.44/1.30     sw 41x47 .87/.49/1.25
topo-asymmetric south 35x49 .71/.56/.54   se 48x47 1.02/.43/.69  e 49x46 1.07/.42/.50
                ne 51x47 1.09/.42/.55     n  36x52 .69/.54/.45   nw 51x47 1.09/.42/.54
                w  50x46 1.09/.42/.46     sw 49x45 1.09/.45/.71
topo-hole       south 50x45 1.11/.50/1.06 (1 hole 7px)  se 48x49 .98/.44/1.22
                e  41x46 .89/.44/1.43     ne 48x46 1.04/.44/.85  n 51x44 1.16/.47/1.11 (1 hole 6px)
                nw 45x46 .98/.45/.90      w  35x46 .76/.50/.48   sw 53x47 1.13/.43/.69
topo-notched    south 40x45 .89/.67/.31 (1 hole 7px)    se 41x49 .84/.54/.45
                e  24x46 .52/.67/.36      ne 38x46 .83/.55/.62   n 39x44 .89/.62/.42 (2 holes, 36px)
                nw 36x46 .78/.56/.57      w  27x46 .59/.59/.54   sw 41x47 .87/.56/.61
```

---

## The proportion axis failed — name it plainly

**prop-narrow measured 0.67 aspect. prop-mid measured 0.64.** Despite one being briefed narrow
and the other briefed as a relaxed midpoint, prop-narrow came back *wider* than prop-mid. Those
two are, to measurement, the same shape. The proportion trio did not produce three separated
silhouettes — it produced one narrow cluster (prop-mid, prop-narrow) and one wide outlier
(prop-wide), and the outlier's separation is discussed below under Topology, because that is
actually where it comes from.

## Root cause — this is structural, not a prompting miss

`create_character_state`'s own contract states it "keeps the source's identity, body type, and
**proportions**, with the edit applied consistently across all rotations." Proportion is
explicitly one of the three things a state is defined to preserve, not one of the things an
`edit_description` can move.

This lines up exactly with the mode table in `.claude/skills/sprite/SKILL.md`: the `proportions`
parameter exists **only** on `create_character` in `standard` mode. `v3` silently ignores it,
`pro` ignores nearly all style params, and `create_character_state` has no `proportions`
parameter at all — there is no lever to pass through even if one wanted to try. **No wording in
an `edit_description` moves proportion on a state, because the call was never built to accept a
proportion instruction.** prop-narrow's "arms tucked, taller and thinner" language was not weak
phrasing that a stronger prompt would fix — it was asking a locked door to open. The 0.67-vs-0.64
result is not noise from an imperfect prompt; it is what the base's fixed proportion envelope
looks like when read through two different but structurally equivalent poses.

This reframes prop-wide, too: it did **not** separate because "wide via extension" is a
proportion technique that worked on a state. It separated because the rifle held out to the side
is additional *held geometry outside the body*, which is a topology move (more mass in the
bounding box, via a prop, not via the skeleton's own proportions) that happens to read as
"wider." The skill's own tradeoff table says this outright: real proportion control
(2.4x width spread) exists only in `create_character` **standard** mode with custom
`proportions` — at the cost of losing this family's palette/canvas/kit inheritance. There is no
version of `create_character_state` that gets both.

## Topology delivered, and it should get the credit

- **Asymmetry ran 0.07 (prop-narrow) to 1.06 (topo-hole) on the south frame — a 15x range** —
  the single largest spread anything in this family produced, on an axis that a state call
  genuinely can move (pose, not skeleton proportion).
- **Width by extension ran 29-50px (south frame)** — real separation, delivered by held props
  (rifle, drone) reaching outside the body rather than by any change to the body itself.
- **topo-asymmetric holds asymmetry above 0.4 on all 8 facings (0.45-0.71)** — the most reliable,
  direction-independent separator measured in the whole batch, and it comes entirely from pose
  (front leg + weapon forward, back arm/leg trailing), which is exactly the kind of move a state
  edit is built to make.
- **topo-hole's own target (a hole ≥19px) was missed** — measured 6-7px, present on only 2 of 8
  facings. This is a genuine separate finding, independent of the proportion root cause: the
  drone was held too close to the body for the gap to survive downsampling. It stands as a named
  miss but **is not being rerolled right now** per the hold on new generations.
- **topo-notched partially recovers topo-hole's miss by accident**: 7px from the front, but a
  real 36px double-hole from the back (north). Its topology genuinely changes with facing.

**Honest verdict for this rev:** topology is the axis that worked in this family; proportion did
not work and, on this call, structurally cannot. On spread alone the family **clears the 2.5x
bar across rotations** (0.48-1.20) but **reads weaker head-on** (1.7x south-only, 0.64-1.11) —
a facing-dependent result, not a clean pass or a clean fail. The owner's "too similar" verdict
does not rest on a failed spread number at all: it rests on the proportion ladder collapsing
outright (prop-narrow 0.67 vs prop-mid 0.64 — indistinguishable) and on the shape language itself
reading too close together. Say that plainly rather than overstating the aspect spread as the
failure, when the real failure is the proportion trio and the silhouette family.

---

## Judging against the stated bar

| Check | Result |
|---|---|
| aspect spread >= 2.5x good / < 1.3x fail | **2.5x across all 8 rotations (clears the bar) vs 1.7x south-only (does not) — facing-dependent; the owner's "too similar" verdict rests on the proportion collapse below, not on this number** |
| zero pairs sharing identical opaque-pixel count | **pass** — no duplicates among the 6 south frames |
| at least one variant above 0.4 asymmetry | **pass** — topo-asymmetric holds > 0.4 on all 8 facings |
| every variant readable as a soldier from its outline alone | **pass** — head, torso and legs visible on every variant inspected across facings |

---

## Per-variant status (rev 1, no further action without a rev 2 direction)

| Variant | Status | Why |
|---|---|---|
| prop-narrow | On hold | Measured indistinguishable from prop-mid (0.67 vs 0.64 aspect) — the proportion axis it was meant to anchor doesn't exist on a state call (see Root cause) |
| prop-wide | On hold | Its separation is real but comes from held-prop topology, not proportion — reclassify rather than reroll |
| prop-mid | On hold | Same finding as prop-narrow — not a failed brief, a call that cannot move this variable |
| topo-asymmetric | On hold | Strongest result in the batch — asymmetry > 0.4 on every facing. No action needed |
| topo-hole | On hold | Missed its own hole-size target (6-7px vs ~19px+) — a real miss, but **not queued for reroll** per the current hold |
| topo-notched | On hold | Direction-dependent topology (7px front / 36px back) — worth keeping as reference, no action pending |

Nothing here has been packed, quantized, or imported. No new generations have been queued since
the owner's "too similar" verdict landed.

---

## Rev 2 — anchor-first, generated, with a flagged methodology problem

**Approach, per owner direction:** edit six distinct south-facing anchors off the owner's shape
reference (`edit_image`), then rotate each into 8 directions with `create_character(mode="v3",
reference_image_url=<edited anchor>)`. This sidesteps rev 1's structural wall entirely — editing
pixels directly means there is no skeleton whose "proportions" a call is contractually bound to
preserve, so the tall↔squat axis is genuinely back on the table.

**Reference:** PixelLab character `bb92dd76-98b0-4327-ba62-8747709402ff` ("Replace the bow
with"), 92x92, group `667961de-4062-4589-ac2d-c27157cc1cb2`. South frame content bbox 41x46
(aspect 0.89, solidity 0.52) — a hooded figure already holding a rifle roughly level.

Contact sheet (rev 2): **https://claude.ai/code/artifact/f2c94cfd-cc58-40bc-b05d-9f6774b9cce1**
Raw renders: `RawArt/Renders/soldier-scifi-variants/rev2/<variant>/{anchor,rotations}/*.png`.

### A real methodology problem, found and named before any verdict

The first two `edit_image` attempts, at the reference's full 92x92 size and then a 41x46
content-bbox crop, both came back **"TRUNCATED in transit"** — the base64 payload (2864 and 2536
characters respectively) was cut short before the API ever saw it, a known MCP failure mode for
inline image parameters. The fix that got a call through cleanly was quantizing the crop to a
24-color palette (`FASTOCTREE`, alpha preserved), which shrank the payload to 872 characters.

**That fix introduced a second, worse problem, and it is the finding that matters most in this
pass.** The crop I fed every `edit_image` call was trimmed to the reference's own content bbox
with **zero padding on any side** — the content already touched all four edges of the 41x46
frame before a single edit was requested. `edit_image`'s output canvas defaults to match its
input canvas, so **any edit asking for a wider or broader silhouette had nowhere to grow into**
and was silently compressed back to fit the same 41x46 box, with no visible clipping artifact
(no severed limb, no prop poking off-canvas) — it just quietly produced a tamer result than
briefed. This is confirmed, not suspected:

| Variant | Anchor canvas | Anchor content | Touches all 4 edges? |
|---|---|---|---|
| tall-narrow | 41x46 | 34x46 | No (narrower — didn't need the room) |
| asymmetric | 41x46 | 28x46 | No (narrower from south — didn't need the room) |
| wide-level | 41x46 | **41x46** | **Yes — full width AND full height** |
| squat-compact | 41x46 | **41x46** | **Yes — full width AND full height** |
| holed | 41x46 | **41x46** | **Yes — full width AND full height** |
| split-notched | 41x46 | **41x46** | **Yes — full width AND full height** |

Every variant that was briefed to grow **wider or broader** than the reference hit the exact
same 41x46 ceiling; every variant briefed to stay the same size or shrink did not. That is not
four independent misses — it is one crop-methodology bug expressing itself identically four
times. It shows up again after the v3 rotation: south-frame aspect for wide-level,
squat-compact, holed and split-notched all land at **0.89** — identical to the *reference's own*
aspect — and the all-rotation table shows the same 0.89 recurring as the *maximum* for all four.
`v3` reproduces the anchor faithfully (by design — "it preserves, it does not embellish"), so a
clipped anchor produces a clipped family.

**Corrected finding (the bug is one notch worse than first diagnosed):** verified independently
against every anchor's bbox — **height was pinned at 46px and the left edge at 0 for ALL SIX
variants**, not just the four that read as fully clipped. `tall-narrow` and `asymmetric` were not
"unclipped, room-to-move" data points; they are the two that happened to move in the *only*
direction a zero-margin frame still permits — inward, i.e. narrower. **This proves the
anchor-first technique can make a sprite NARROWER than its reference. It does NOT yet prove it
can make one TALLER, WIDER, or SQUATTER — no variant in this pass was ever able to test those
directions**, because none had vertical or rightward room to grow into. `wide-level`,
`squat-compact`, `holed`, and `split-notched` all needed exactly that room and got none. This is a
finding to fix, not a verdict to accept — and the fix applies to all six, not four.

**The fix (simpler than a re-crop, confirmed):** `edit_image` accepts explicit `width`/`height`
output parameters (32-512) — no need to touch the input at all. The 41x46 quantized crop was
solving a real problem (MCP truncating large inline base64) and stays exactly as-is; the missing
piece was never passing an output canvas larger than the input. Redo queued as **rev2b** with
`width=92, height=92` on every `edit_image` call, matching the reference's native canvas and the
92x92 output v3 already normalizes to regardless of input size. No further generations were
queued from this pass while the fix was confirmed.

### Measured results (rev 2, as generated — read with the caveat above)

South frame:

| variant | content | aspect | solidity | asymmetry | holes | luma | colours |
|---|---|---|---|---|---|---|---|
| asymmetric | 28x46 | 0.61 | 0.69 | 0.49 | – | 0.142 | 35 |
| holed | 41x46 | 0.89 | 0.66 | 0.66 | 1 (12px) | 0.176 | 37 |
| split-notched | 41x46 | 0.89 | 0.65 | 0.63 | – | 0.176 | 39 |
| squat-compact | 41x46 | 0.89 | 0.60 | 0.60 | – | 0.174 | 29 |
| tall-narrow | 34x46 | 0.74 | 0.62 | 0.24 | – | 0.192 | 17 |
| wide-level | 41x46 | 0.89 | 0.59 | 0.59 | – | 0.172 | 46 |

**South-only aspect spread: 0.61-0.89, a 1.5x range.** All 8 rotations:

| variant | aspect range | drift | solidity range |
|---|---|---|---|
| asymmetric | 0.61-0.83 | 0.22 | 0.54-0.69 |
| holed | 0.72-0.89 | 0.17 | 0.50-0.70 |
| split-notched | 0.52-0.89 | 0.37 | 0.58-0.71 |
| squat-compact | 0.70-0.89 | 0.20 | 0.52-0.63 |
| tall-narrow | 0.54-0.78 | 0.24 | 0.57-0.73 |
| wide-level | 0.65-0.89 | 0.24 | 0.51-0.66 |

**0.89 recurs as the maximum for four of the six variants across every rotation checked** — the
clipping ceiling, not a natural convergence. **All six were height-pinned at 46px** (confirmed by
bbox inspection), so even tall-narrow and asymmetric were constrained — they simply moved in the
one direction (narrower) a zero-margin frame still allows.

**What did land clean, with the corrected claim:**
- **tall-narrow** reaches aspect 0.54 (east/west) — genuinely narrower than the reference's 0.89,
  and it reads clearly as a tall, cinched hooded figure with the rifle slung on the back, from
  every angle checked (north/north-east judged first, per the rev 2 caveat that rear facings
  degrade most). This proves the anchor-first technique can move proportion **narrower** than a
  reference; it does not yet speak to taller, wider, or squatter — none of those directions had
  room in this pass (see the corrected finding above).
- **asymmetric** holds asymmetry 0.49 (south) rising to higher on some facings, content 28x46 —
  narrower from the front because the pack sits on one shoulder rather than spreading the whole
  frame; visually reads as a soldier with a heavy shoulder-mounted launcher, clearly not mirrored.
- **holed** produced one measured hole of 12px — better than rev 1's topo-hole (6-7px) but still
  under the ~19px floor for reliably surviving downsampling. Visually the lantern-on-a-pole gap is
  legible in the raw image at this size; whether it survives quantization/downsampling to game
  scale is a separate, unanswered question.
- **split-notched** shows a genuine jagged, forked hood/shoulder top edge in the raw image,
  though its south-frame hole count reads as 0 (the fork's gaps may be open to the sprite edge
  rather than fully enclosed — the same notch-vs-hole distinction rev 1 had to learn).

**Cost, reported plainly against the estimate:** each `edit_image` call cost **~20 generations**,
not the near-free step the rev 2 direction assumed. Six edits × ~20 ≈ 120, plus six v3 rotations
at 1 generation each (observed, cheap at this canvas size) ≈ **~126 generations total** — well
under rev 1's ~200, but **over double** the "~20-60" estimate. The gap is entirely in
`edit_image`'s cost, not the v3 rotation step, which came in at the cheap end as expected.

**Every variant is still readable as a soldier from its outline alone** — head, torso and legs
visible on every rotation inspected, hood-and-cloak read intact throughout.

### Rev 1 vs rev 2, side by side

| | Rev 1 (states) | Rev 2 (anchor-first, as generated) |
|---|---|---|
| South-frame aspect spread | 1.7x (0.64-1.11) | 1.5x (0.61-0.89) |
| All-rotation aspect spread | 2.5x (0.48-1.20) | not a clean comparison — 4 of 6 variants hit a crop ceiling |
| Proportion axis | Structurally cannot move (API contract) | Proven it CAN move **narrower** (tall-narrow, 0.54) — taller/wider/squatter untested, all 6 variants were height-pinned by the same crop bug |
| Topology axis | Delivered (asymmetry 15x range) | Delivered again (asymmetric, holed, split-notched all read distinctly) |
| Cost | ~200 generations | ~126 generations (vs ~20-60 estimated) |

**Read this carefully: rev 2's raw spread number (1.5x) is lower than rev 1's (1.7x), but that
comparison is not fair to the technique** — rev 1's number is real, rev 2's is suppressed by a
crop bug affecting **all six** variants (height-pinned across the board; four also width-pinned).
tall-narrow's 0.54 shows the technique can narrow past anything rev 1 produced, but it says
nothing about the taller/wider/squatter directions this pass never got to test.

**Fix diagnosed, NOT yet applied — corrected 2026-07-28.** An earlier pass through this doc
claimed "confirmed and applied as rev 2b" and pointed at a `rev2b/` results section below. Both
claims were false: `RawArt/Renders/soldier-scifi-variants/rev2b/` was never created, and no
"Rev 2b" results section ever existed past this point. The diagnosis itself holds up: `edit_image`
takes explicit `width`/`height` output parameters — the 41x46 quantized-crop input (which solves
the real base64-truncation problem) stays exactly as-is; every call would also pass `width=92,
height=92`, giving every direction of edit real room to move. All six variants would need
redoing, not just the four that read as visibly clipped, because height was pinned on all six
alike and a family needs one consistent canvas to compare fairly. Rev 2 stays on disk untouched.

**Current status: rev2b has not been generated.** The session handed this task (2026-07-28,
continuing from the point above) had no PixelLab tool access at all — every PixelLab MCP tool
(`create_character_state`, `edit_image`, `get_balance`, `get_character`, etc.) was unavailable,
confirmed by exhausting the tool-discovery search rather than assumed. No generation was queued,
no credits were spent. This is a tooling blocker, not a judgment call against running rev2b —
the diagnosis above still reads as sound and worth executing once a session with PixelLab access
picks this up. See "Rev 2b — not generated" below for the full handoff note.

## Rev 2b — not generated (tooling blocker, 2026-07-28)

**Nothing in this section is measured data. It is a handoff note.**

The fix described above (`width=92, height=92` on every `edit_image` call, crop input unchanged)
was never executed. This session picked up task-056 mid-flight specifically to run it, verified
the rev2b directory does not exist and the doc's "confirmed and applied" claim was false (both
corrected above), and then found it had no PixelLab MCP tools available to act on the diagnosis —
not a missing credential or a denied call, but no `create_character`, `create_character_state`,
`edit_image`, `get_character`, or `get_balance` tool present in the session at all.

**What is still true and does not need re-litigating by the next session:**
- The crop-ceiling bug diagnosis (all 6 rev 2 variants height-pinned at 46px; 4 of 6 also
  width-pinned at 41px) is confirmed independently against bbox data, not speculative.
- The fix (`edit_image(width=92, height=92, ...)`, same 41x46 quantized crop as input) directly
  targets the confirmed cause and does not require re-deriving the crop-quantization workaround
  for the base64-truncation problem — that part of rev 2's pipeline is unaffected and stays as-is.
- Estimated cost, extrapolated from rev 2's actual spend: ~126 generations for 6 `edit_image`
  edits + 6 v3 rotations (rev 2's observed cost at the same call shape). Confirm the live balance
  with `get_balance` before queueing, per the standing rule — this is an extrapolation, not a
  quote.
- The open question rev2b exists to answer: whether anchor-first editing can move **taller,
  wider, or squatter**, not just narrower — rev 2 only ever proved narrower, because zero
  variants had spare canvas to grow into.

**Recommendation:** run rev2b once PixelLab tool access is available. The diagnosis is sound,
the fix is a one-parameter change to a call already validated in rev 2, and the estimated cost
(~126 generations) is in line with rev 2's actual spend, not a new order of magnitude. This is
not a "the numbers already settle it, skip the regeneration" case — rev 2's own data cannot
answer the taller/wider/squatter question at all, by construction, so there is no shortcut to
a verdict without running the fixed version.
