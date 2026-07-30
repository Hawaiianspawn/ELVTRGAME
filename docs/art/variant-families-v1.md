# Variant families v1 — six enemy registers, specced as approve/reject gates

**What this is:** six candidate enemy/NPC *families*, each derived from a distinct genre
register, specced only far enough for the owner to decide **which to investigate**. This is a
gate document, not an art spec. Nothing here is buildable as written and nothing here should
be built until a family is approved — the per-family spec comes after, in the shape of
`soldier-roster-v1.md`.

**Version:** 0.2 · **Status:** APPROVED 2026-07-30 (owner) — **F1 and F2 advance. F3 rejected on
measurement, see §2 F3. F4 held, F5/F6 skipped.** · **Date:** 2026-07-30

**Source of the registers.** The genre split comes from *VERSUS* (story ONE, art Kyoutarou
Azuma), where thirteen human worlds each lose to a *different* kind of enemy — demons, aliens,
titans, machines living and dead, nature turned abominable, and rival humans — before the worlds
collide. The useful part for us is not the plot (see `docs/narrative/STORY-STRUCTURE.md`,
shelved) but the observation that **each enemy category owns a different silhouette axis**. Six
registers is a menu to pick from, not a roster to build.

**Binds to:** `docs/art/soldier-roster-v1.md` (the disjointness model and the authored-anchor
method) · `docs/art/npc-silhouette-brief.md` (silhouette × value dominance × pale usage) ·
`docs/art/aesthetic-direction.md` (**including the 2026-07-28 amendment — see §1**) ·
`docs/narrative/FLAME-FOUNDATION.md` §4.5 (open: does the dark have monsters).

**Names nothing.** FLAME-FOUNDATION §5 still defers factions, biomes and NPC names. Every
family below is described by its *mechanism*. The bracketed labels are handles for this
document only.

---

## 1. Two things changed under the existing model — read before judging any family

**(a) Colour is back, so the disjointness model has a fourth axis it did not have.**
`aesthetic-direction.md`'s 2026-07-28 amendment supersedes the strict 4-value global palette;
the game ships at `Quantize 0`, full colour. Every disjointness argument in
`soldier-roster-v1.md` and `npc-silhouette-brief.md` was written against **three** axes
(silhouette / value dominance / pale usage) *because hue did not exist*. It does now.

This cuts both ways and the second half matters more:

- Families are now **cheaper** to keep disjoint — hue can carry a family read that used to cost
  a whole silhouette claim.
- But **hue is the first thing the dark takes.** Outside the leash, `dim_shift` collapses
  everything toward value, so any family whose identity rides on hue **stops being identifiable
  exactly where the game is most dangerous.** Silhouette must still carry the read alone. Hue is
  a bonus channel, never the carrier.

**Treat every "reads as" claim below as a silhouette claim that must survive greyscale.**

**(b) The enemy slot is reserved and still unspent.** `soldier-roster-v1.md` §3a holds a
standing reservation: *Bone-dominant and irregular is the player's side; enemies take
Dark-dominant, or regular, or both.* All six friendly soldiers deliberately declined to spend
it. **Every family below is a candidate for spending that reservation, and only one or two
should.** A family that is neither Dark-dominant nor regular will fight the player's own
congregation for read.

---

## 2. The six families

Ranked by recommendation. `Cost` is generations to a decidable prototype, not to ship.

### F1 — [Mirror] · rival humans · **recommended, cheapest**

**Claim:** the enemy has *your* silhouette. Another bearer's congregation — the same ordinary
people, standing in a light that is not yours.

**Silhouette axis:** none new. Reuses the six authored soldier anchors unchanged.

**Disjointness mechanism:** spends the enemy reservation on **regularity**, not darkness. Where
your congregation is ragged and individual, theirs is *aligned* — same six bodies, uniform
spacing, matched props, no variation between neighbours. The tell is formation, not anatomy.
Plus hue as the second channel (their fire is a different colour than yours), with the explicit
understanding from §1a that hue vanishes at range.

**Why it's first:** it is nearly free, and it is the only family whose art cost is a palette
pass rather than a generation batch. It also does narrative work no other family can — it makes
"they think you are a god" land by showing the same devotion pointed elsewhere.

**What kills it:** if playtest can't tell friend from foe in a melee, this family is unshippable
and no amount of tuning saves it. **That is the whole experiment** — this family is *designed*
to be confusable, and the question is whether formation alone is enough separation. Test at
horde scale before anything else.

**Cost:** 0 generations. Recolour + a formation rule in the spawner.

---

### F2 — [Broken] · machines living and dead · **recommended, best cost-to-variety**

**Claim:** two sub-registers where one is the damaged state of the other. Intact: articulated,
bilaterally symmetric, upright, mechanical regularity. Broken: the same body, partial —
missing a limb, dragging mass, symmetry destroyed.

**Silhouette axis:** **symmetry, and its violation.** No other family in this list owns
symmetry-as-identity, and none owns *asymmetry produced by damage* rather than by poverty
(which is what variant 01's raggedness already claims).

**Disjointness mechanism:** spends the reservation on **regularity** (the intact form is the
most regular silhouette in the game — more regular than a veteran rank). The broken form
inherits the reservation by association: it is visibly the *same thing*, wrecked.

**Why it's second:** this is the cheapest roster doubler available. The broken variant is a
PixelLab **state** on the intact character — inherits canvas, palette and lighting, per the
established states workflow — and the silhouette delta is genuine (removed mass, broken
symmetry), not a recolour. One character purchases two families' worth of read.

**"Machine" is the mechanism, not the fiction.** Articulated-symmetric-upright doesn't require
chrome or sci-fi. Reframe it as anything made-and-repeated and the silhouette claim is
unchanged. **Do not let this family drag hard-surface sci-fi rendering into the game** — that's
F3's failure mode, below.

**Collision check run 2026-07-30 — the named risk was wrong, and the real one reshapes the
claim.** This spec said to measure the intact form against the *veteran* (soldier03). Measured,
the roster says otherwise:

| | aspect | solidity | asymmetry |
|---|---|---|---|
| soldier06 shield-heavy | 0.74 | **0.83** | **0.05** |
| soldier03 veteran | 0.61 | 0.70 | 0.35 |
| knight `type0_base` (nearest existing intact-regular form) | 0.77 | 0.73 | 0.26 |

**Soldier06 already sits at asymmetry 0.05 and the roster's highest solidity.** "The most
regular silhouette in the game" is taken, by a friendly, with no headroom above it. The veteran
was never the threat — it measures 0.35, nowhere near.

**Revised claim, and it is the stronger one: F2's identity is the *pair*, not either pose.**
The family is one body appearing at **both ends of the asymmetry axis** — intact near 0, broken
high — and no friendly does that. Every soldier sits at one fixed point on that axis (0.05 to
1.67 across the six, each static). A unit the player has seen *whole* and then meets *wrecked*
is a read nothing in the roster can make, and it costs one state.

This also means the intact half must **not** chase symmetry past soldier06. It should sit
mid-range and let the broken half travel. Target the delta, not the endpoint.

**What kills it:** if the broken state's asymmetry lands inside soldier01's (1.67) or
soldier04's (1.28) territory *and* shares their bone-dominance. Those two are the roster's
irregular friendlies. The broken form must be irregular **and** regular-derived — visibly a
wrecked made-thing, not a ragged person.

**Cost:** 1 character + 1 state. Low, but **not zero — this is the first credit spend in the
plan and needs sign-off.**

---

### F3 — [Bloom] · nature made abominable · **REJECTED 2026-07-30, on measurement**

> **The gate below ("check it against `brood-ooze` before generating anything") was run first
> and F3 failed it.** `silhouette_report.py RawArt/Renders/brood-ooze/raw` returns **13
> variants, aspect spread 0.50–1.56 (3.1×), width spread 22–53px (2.4×)** — the brood already
> occupies the range F3's entire case rested on. A second blob family would buy silhouette
> territory that is measured, built and on disk. Cost of the check: zero generations.
>
> **Reconsider only if** the brood is ever narrowed or retired. The claim below is not wrong —
> blob topology *does* have the widest ceiling — it is just already cashed.

**Original case, kept for the record:**

**Claim:** mass without structure. No head, no limbs, no held object — a thing whose outline is
its entire identity.

**Silhouette axis:** **topology.** This is the only family that can leave humanoid topology
entirely, and topology is the strongest of the three variation levers (aspect → topology →
interior).

**Disjointness mechanism:** spends the reservation on **Dark-dominance** and flat internal
value. Note this is the *existing* void mechanism from `npc-silhouette-brief.md` (c) — so F3
is less a new family than a formalisation of one already half-specced, which lowers its cost
and raises its risk of redundancy with the brood.

**Why it's third:** blob topology is where measured silhouette range is widest — a skeleton caps
how far a variant can travel, and a blob has no skeleton. If the goal is *variety per unit of
art budget*, this family has the highest ceiling of the six.

**What kills it:** redundancy. If it lands on the brood, it bought nothing. **Check it against
`brood-ooze` and the existing void register before generating anything** — this family must
prove it is a different thing, not a second helping.

**Also gated on FLAME-FOUNDATION §4.5.** If the dark turns out to *be* the enemy rather than to
contain monsters, this family's fiction needs rewriting even though its silhouette claim holds.

**Cost:** 3–4 generations to judge range. Medium.

---

### F4 — [Weight] · titans · **investigate only if F1–F3 leave a gap**

**Claim:** scale, plus the posture that comes with it — hunched, low-browed, arms forward,
head below the shoulder line.

**Silhouette axis:** aspect ratio and posture. The weakest lever of the three, used alone.

**Why it's fourth:** "bigger" is not a silhouette claim, it's a scale multiplier, and at horde
zoom a scaled-up humanoid is still a humanoid. The *posture* half is real — head-below-shoulders
inverts the read of every friendly unit — but posture alone is thin for a whole family, and
`soldier-roster-v1.md` variant 06 already owns "head sunk below the silhouette's top edge."

**What kills it:** the 06 collision, and the fact that a big unit is probably an
**entity-tier** question (elite/boss) rather than a variant-family question. Check
`docs/design/entity-tiers.md` before treating this as art work at all — it may already be
answered somewhere that isn't the art pipeline.

**Cost:** 1–2 generations. Low, but likely wasted.

---

### F5 — [Horned] · infernal demons · **skip — already occupied**

**Claim:** horns, asymmetry, limb count off human.

**Why it's skipped:** this is the register the brood already sits closest to, and it is the
default fantasy answer. It would produce recognisable enemies quickly and add nearly nothing
the existing void/brood register doesn't cover. Its main axis (limb-count asymmetry) is F3's
territory and its secondary axis (horns above the crown) collides with variant 04's rally horn,
which is the only over-crown silhouette break in the game.

**Reconsider if:** F3 is rejected. Then this becomes the fallback non-humanoid register.

**Cost:** irrelevant unless reconsidered.

---

### F6 — [Clean] · advanced aliens · **skip — costs a whole visual register**

**Claim:** hard-edge geometry, no organic taper, carried tools rather than grown parts.

**Why it's skipped:** the contrast that makes this work in a sci-fi crossover is *"a clean thing
in a ragged world"* — which requires the game to first establish that everything else is ragged,
and then spend an entire rendering register on the exception. That's a large purchase for one
enemy type, and it's the single most likely thing on this list to make the game look like a
different game.

**The idea underneath it is worth keeping, though:** *an enemy whose visual language obeys none
of the world's rules* is exactly the third-force concept from the shelved
`STORY-STRUCTURE.md` §2 ("doesn't act through fire"). If that antagonist is ever approved, F6
is its art answer — **and only then.**

**Cost:** high, and front-loaded into style work rather than sprites.

---

## 3. Gate summary

| | Family | Spends reservation via | Silhouette axis | Cost | Recommend |
|---|---|---|---|---|---|
| F1 | Mirror | regularity (formation) | none — reuses roster | 0 gens | **APPROVED** |
| F2 | Broken | regularity | symmetry + its violation | 1 char + 1 state | **APPROVED** |
| F3 | Bloom | Dark-dominance | topology (no skeleton) | 3–4 gens | **REJECTED** — brood owns the range |
| F4 | Weight | — (unclear) | aspect + posture | 1–2 gens | Hold |
| F5 | Horned | — | limb asymmetry | — | Skip |
| F6 | Clean | — | hard geometry | high | Skip unless third force approved |

F1 and F2 spend the reservation differently — formation regularity vs mechanical regularity —
and own different axes, so they are coherent together. F2 and F5 would collide, which is a
second reason F5 stays skipped.

**With F3 gone, no approved family spends the reservation on Dark-dominance.** That channel
returns to unspent, where `soldier-roster-v1.md` §3a left it. Worth noticing rather than
patching: it means both approved enemy families read as *regular*, and regularity is now
carrying the entire friend/foe distinction. If playtest breaks F1, it likely breaks F2 too —
they share a mechanism. Named in §5.5.

---

## 4. Method — applies to whichever families are approved

1. **Measure, never eyeball.** `Scripts/art/silhouette_report.py` decides whether a family is
   distinct, and it decides it against the *existing* roster as well as within the family.
   No verdict from the south frame alone.
2. **Vary aspect first, then topology, then interior.** Interior detail is the last resort and
   the first thing the collapse eats.
3. **Rotation is where families die, not the anchor.** `soldier-roster-v1.md` §6.1 is the
   standing lesson: a rotator infers rigidity from **aspect ratio and terminals**, not from
   the word describing the object. Any held or protruding part must be specced as numbers —
   minimum thickness, maximum length, hard terminals — so a rotated frame is checkable
   mechanically. Every family spec must record its carrier as measurements.
4. **Authored anchors cost nothing.** `pixelpipe.py authored` renders a pixel map at 0
   generations. Never have an agent hand-type the ASCII grid — a director writes the metaprompt
   as named variables and a Python drawer emits it.
5. **48×48 cell**, locked 2026-07-25. Requests land in `docs/data/art/requests/`.

---

## 5. Open

1. **§1a is the real question under all six.** Now that hue exists, how much family identity is
   allowed to ride on it? The position taken above — *hue is a bonus, silhouette carries the
   read alone* — is a proposal, not canon, and it is conservative. If the owner wants hue to
   carry more, F5 and F6 get considerably cheaper and this whole ranking changes.
2. **Does the enemy reservation survive colour?** `soldier-roster-v1.md` §3a's
   Dark-dominant-means-enemy rule was written when value was the only channel. It may be
   over-restrictive now, and it is currently constraining three of six families.
3. **F4 may not be an art question.** Resolve against `entity-tiers.md` before spending art
   budget on it.
4. **No art brief attached.** Briefs get written per-family, after approval — writing them
   against six unapproved candidates is how the last reset happened.
5. **Regularity now carries friend/foe alone** (see §3). Both approved families are "regular";
   neither is Dark-dominant. F1 is the harder case and should be tested first — if formation
   regularity fails to separate F1 from the player's own congregation at horde scale, F2's
   mechanical regularity is unlikely to rescue it, and one of the two needs a different
   reservation. Do not build both before that test returns.
