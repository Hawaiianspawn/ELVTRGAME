# Protagonist prototypes — four directions for the flame bearer

**Subject:** the protagonist — the flame bearer, the player character · **Round type:**
prototype / selection, not a shipping spec · **Cell:** 48×48 (locked, owner decision
2026-07-25), plus a 128px concept pass — see §3 ·
**Requests:** `../data/art/requests/protagonist-01.json` … `protagonist-04.json` ·
**Anchors:** all four **generated** (`anchor.strategy: "text-only"`), 1 generation each
per resolution.

**Binds to:** `docs/narrative/FLAME-FOUNDATION.md` (§1 premise, §3a leash-as-light, §3c the
hero's job is the light) · `docs/art/aesthetic-direction.md` (2026-07-12 RESET — Direction A,
global 4-value ramp, no fifth value, no palette swaps) · `docs/art/vanguard.md` (prototype 01
*is* this document's answer, restated at protagonist scale) ·
`docs/art/npc-silhouette-brief.md` (the three-way disjointness model) ·
`docs/art/soldier-roster-v1.md` (§3a the Dark-dominant enemy reservation; §10 free-vs-caged
light) · `docs/art/soldier-style-depth-test.md` (the measured 2026-07-25 batch) ·
`docs/data/art/palette.json` · `ELVTR/SETUP-EDITOR.md`.

**Deliberately does NOT bind to:** `WORLD.md` (superseded in full), `docs/art/hero-palettes.md`
§1–§2 (void), or any faction, biome or NPC name — current canon names none
(FLAME-FOUNDATION §5) and this round does not invent one.

---

## 1. Intent

**Fiction.** FLAME-FOUNDATION §1: the world is pitch dark, you bear the only light, people
gather to you because outside your circle the dark takes them, and they treat you as a god.
This round does not design the protagonist. It asks the one question that has to be answered
before the protagonist can be designed: **how does the only light in the world attach to a
person, and what does that make them look like to the crowd standing in it.**

Four answers are staked out. They are not four styles of the same figure — they are four
different *relationships* between a body and a fire, and each one buys something and costs
something that the others do not.

**Gameplay.** Under a strict global palette there is no colour channel. Class, threat and
identity ride on silhouette, value-pattern, and how the single bright value is spent. So each
prototype is a **shape claim plus a bright-placement claim**, and the two together are the
whole design. The selection criterion is not which looks best at rest; it is which one the
owner can find in a 500-unit press, and which one still means something when it is the thing
an army is standing inside.

**This character may be rigged and may become a 3D model**, so each prototype below carries a
one-line **Rig** verdict. That is a real selection criterion in this round, not decoration: a
held prop and a body silhouette translate; a 2-value cloth flip and a pixel-glow trick do not.

**What this round deliberately is not.** No pixel maps, no sheet, no animation, no import. The
anchors are generated rather than authored precisely because the owner needs to *look at* four
rendered candidates; an authored ASCII anchor comes back as a greybox blockout, which is the
right tool for proving a sheet and the wrong tool for choosing a protagonist. The cost of that
choice is stated in §11.

---

## 2. The palette

Global ramp, no exceptions, no swaps (`aesthetic-direction.md` 2026-07-12 reset; GDD #6
resolved to strict global palette). All four prototypes draw from exactly this table.

| Hex | Name | Role across this round |
|---|---|---|
| `#211e20` | Demichrome Dark | outline, seam, recess, **eye notch on all four**, belt, boot, the cage/rim that contains every fire in this set |
| `#555568` | Demichrome Steel | worked metal: plate, helm, pole, bowl rim, brazier ribs, hat brim |
| `#a0a08b` | Demichrome Bone | skin, cloth, robe, rope, wood — the *person* value |
| `#e9efec` | Demichrome Pale | **the flame, and nothing else.** The brightest value belongs to honest light first; on all four prototypes it is spent only on fire |

Transparency is **not a value**: alpha is binary (0/255), Unlit + Masked, per
`palette.json.mask`. The quantizer hard-thresholds at 128; partial alpha is the most common
silent breakage.

**Reservations and rulings that apply to every prototype here:**

- **Eyes are Dark notches on all four. Never Pale.** A pale eye-dot is the registered tell of
  the amorphous void register (`npc-silhouette-brief.md` (c)); on the protagonist it is the
  single worst confusion available in a hue-less game.
- **Every fire in this set is *caged*** — bounded by a hard Dark rim or rib on all sides, never
  touching transparency. `soldier-roster-v1.md` §10 draws the free-vs-caged line to keep the
  Lampbearer's `point_halo` unforgeable, and `quantize` now audits it mechanically
  (`stats["pale_uncaged"]`). Prototype 04 is the first hero-scale subject that *wants* to break
  this; it is caged anyway, and the alternative is raised in §13.
- **Prototype 01 takes `shape_carriers.rectangle_flip`**, which `palette.json` registers to the
  Vanguard, "banners only". This is flagged, not quietly taken — see §13 proposal 1.
- **No prototype is Dark-dominant.** `soldier-roster-v1.md` §3a holds Dark-dominance in reserve
  for a future enemy register; the protagonist is the last sprite in the game that should spend
  it.
- **No faction value.** Current canon names no factions; nothing is reserved for one here.

**Pale budget, stated per prototype** so the QC pass has a number to check rather than an
adjective:

| | 01 Standard | 02 Ember Core | 03 Tender | 04 Brazier |
|---|---|---|---|---|
| Pale as % of opaque px | ≤ 9% | ≤ 3% | ≤ 4% | ≤ 10% |
| Contiguous or split | one rectangle | one caged block | one caged block | split between ribs |

**Light-shifted variant: none emitted** (`light_shift_variant: false` on all four). A bearer is
never outside its own light — it renders at full ramp always and is the calibration reference
for every dimmed unit around it (`vanguard.md` §7). The state the protagonist *does* need is
the guttering one, which is `palette.json.dim_shift` applied by the material, not a second
texture.

---

## 3. Two resolutions, and what each column is for

Owner decision, this round: **each prototype is generated twice — once at 48px, once at
128px.** Both use `standard` mode, which is the only mode that honours `proportions` (chibi)
and `shading` (flat); 128 is inside standard's range, so the house style holds at both sizes
and the two passes differ only in pixel budget.

| Pass | Size | What it is for | What it proves |
|---|---|---|---|
| **Concept** | 128px | **The read the owner judges.** 48px chibi is a poor frame in which to choose a character who may be rigged or modelled — at 48 the face is four pixels and the difference between "hood" and "helmet" is one row. | That the *design* is worth having: proportion, carriage, the relationship between body and fire, whether this is a person you want to be. |
| **Gameplay** | 48px | **The confirmation.** Same composed description, the locked cell. | That the design **survives the locked cell** — that its carrier is still legible when the whole figure is 48 pixels tall, which is the only size the game ever draws. |

The 48px column is the veto, not the illustration. A prototype that is beautiful at 128 and
becomes a grey lump at 48 has failed this round, because 48 is what ships. Read them as a pair
and in that order: **judge at 128, confirm at 48.** If the two disagree, the 48 wins.

**Pipeline note, on the record.** `pixellab.size` stays **48** in every request — that is the
shipping value the pipeline manages, and `pack` refuses to resample. The 128px pass is run as
**the same composed description with `size` overridden at the MCP call**, fetched into a
separate `concept128` stage so it can never be mistaken for a packable frame. That is a
deliberate deviation from the request's declared params and is documented here rather than
left as an undocumented call-site edit. `budget.max_generations: 2` on each request covers the
pair.

---

## 4. The set at a glance

Each prototype is built as a **letter-shape**, because overall silhouette mass is the coarsest
and most rotation-proof channel available and is one of the things text prompting reliably
lands. Nothing in this set rides on asymmetry, uneven shoulders or mismatched limbs — the
measured 2026-07-25 finding is that a generator regresses those to a uniform soldier, and this
round is designed against that failure mode.

| # | Name | Letter | How the flame attaches | Head | Cloak | Stance | `value_dominance` | `pale_usage` | Pale altitude |
|---|---|---|---|---|---|---|---|---|---|
| **01** | **The Standard** *(incumbent)* | **I** | held high on a dead-vertical pole | open-faced flat-topped helm | no | narrow column, feet close | `steel` | `banner` | **above** the crown, detached across a transparent gap |
| **02** | **The Ember Core** | **A** | embedded — a barred fire-window in the breastplate; hands empty | **bare, no headgear at all** | no | wide-planted legs under a narrow torso | `steel` | `halo` | mid-chest, **inside** the body outline |
| **03** | **The Tender** | **O** | carried low in a bowl, both hands, arms closing a loop | deep pointed hood, face recessed | **yes — ankle robe** | narrow, slight forward lean | `bone` | `halo` | waist, forward of the body |
| **04** | **The Brazier** | **T** | back-mounted ribbed basket riding across the shoulders | wide flat brim hat | no | wide top, narrow waist and legs | `mixed` | `halo` | shoulder line, wide, welded to the outline |

Four letters — **I, A, O, T** — is the whole readability mechanism at horde scale. At 12px the
face is gone, the values are two blobs, and the letter is the only thing left.

**01 is the incumbent and the set is honest about it.** `vanguard.md` already specced the
standard-borne flame in full: Steel-dominant, geometry-first, one large Pale rectangle riding
above the crowd, `reads_as` unchanged and quoted verbatim below. Slot 01 restates that answer
at protagonist scale so the other three are measured against a real, already-argued design
rather than against nothing. Its request carries `class_tie: "Vanguard"` for that reason; 02–04
carry `none`. If 01 wins, the protagonist and the Vanguard converge and `vanguard.md` becomes
the spec. If it loses, it stays the Vanguard's answer and the protagonist takes a different
one — those are different outcomes and the round should not blur them.

---

## 5. Pairwise disjointness audit

Model: `npc-silhouette-brief.md`'s three axes — **silhouette × value dominance × pale usage.**
Two subjects are safely disjoint if they disagree on any two; they disambiguate under motion
and occlusion if they disagree on all three. Four subjects means only three dominance values
are available to spend (`dark` is reserved, `pale` is impossible), so exactly one collision is
unavoidable — it is placed on the pair whose silhouettes are furthest apart.

| Pair | Shared axis (the risk) | Resolved by |
|---|---|---|
| **01 × 02** | **both `steel`-dominant** — the deliberate collision | Silhouette is the maximum available contrast in this whole set: 01's identity is *a prop leaving the outline vertically*; 02's is that **nothing leaves the outline at all**. I versus A, and column-with-a-mast versus planted-wedge. Pale separates them on both shape and altitude: one large detached rectangle above the crown, versus one small caged block inside the torso. 2 of 3 axes, and the two that disagree are the two coarsest. |
| **01 × 03** | — | All three: steel vs bone, I vs O, above-crown vs at-waist. |
| **01 × 04** | **both put bright above the crowd line** — the genuinely risky pair, because that altitude is the single most valuable piece of screen real estate in the game | Three checkable properties, in order of survivability. (a) **The gap.** 01's Pale is a rectangle *detached* from the body — transparent pixels between the flag and the head. 04's is *welded* to the shoulder outline with no gap anywhere. Gap-versus-no-gap survives to 8px. (b) **Aspect.** 01's is a compact rectangle, roughly 3:2. 04's is a long horizontal bar, wider than the figure's own shoulders. (c) **Dominance**, steel vs mixed. If these two ever collide in playtest the fix is 04's basket width, never 01's flag. |
| **02 × 03** | **both put bright below the crown**, so neither can be found by scanning a skyline | Dominance (steel vs bone), letter (A vs O), head (bare vs deep hood — presence/absence of headgear, which is a thing the generator lands), and **containment**: 02's fire is inside the body outline with both hands empty; 03's is in a held object *forward* of the body with both hands on it. 3 of 3, but note that neither of them wins the horde-scale test on the bright alone — see §7 and §8. |
| **02 × 04** | — | All three: steel vs mixed, A vs T (exact inversion — wide base/narrow top against wide top/narrow base), chest vs shoulder-line. |
| **03 × 04** | — | All three: bone vs mixed, O vs T, waist vs shoulder-line. |

### 5a. Against everything already specced

| | Silhouette | Value dominance | Pale usage |
|---|---|---|---|
| **01 Standard** | narrow column, flat shoulder line, vertical pole | steel | one large rectangle, above the crowd |
| **02 Ember Core** | wide-planted wedge, no protrusions | steel | one caged block, inside the torso |
| **03 Tender** | closed loop, hooded, robed taper | bone | one caged block, at the waist |
| **04 Brazier** | wide ribbed bar over a narrow body | mixed | split between ribs, on the shoulder line |
| Soldier roster 01–06 (`soldier-roster-v1.md`) | ragged/regular congregation shapes | bone / mixed / steel | none, except 05's caged pot |
| 05 Flame-tender (the retinue unit) | hooded, squat, caged bright at **chest** height | steel | halo, caged, ≤16px |
| A Dark-dominant enemy register, if one is ever named | clean, regular, repeated | **dark** (reserved, unspent) | none |

**One collision needed explicit resolution: 03 Tender versus soldier-05 Flame-tender.** Both
are hooded, both carry a caged fire in a vessel, and both are `halo`. They are *the same
fiction at two ranks*, which is the most dangerous kind of neighbour. Separation, if 03 wins
this round: (a) **dominance** — 03 is `bone` (unarmoured, cloth, a keeper), 05 is `steel`
(43.4%, chosen optically to give its 4×4 core a field to burn against); (b) **vessel size and
grip** — 05's pot is held at chest height on one side with a bundle hump breaking the other;
03's bowl is wide, held at the waist, **in both hands**, with the arms closing a visible loop
around it; (c) **the robe** — 03's silhouette tapers to a single unbroken hem at the feet, 05
has legs and boots. If 03 wins, `soldier-05` should be re-examined and probably re-pitched
before both ship: a congregation that contains a smaller copy of its own god is a confusion,
not a hierarchy. Raised in §13.

---

## 6. Prototype 01 — The Standard *(incumbent)*

**Reads as:** *a walking rectangle under a pale flag — the one straight-edged figure in the
crowd.* (verbatim, `vanguard.md` §4b — the incumbent is quoted, not rewritten)

**Carrier:** the detached rectangle above the crown. A dead-vertical Steel pole gripped at hip
height, rising clear of the head, carrying one axis-aligned Pale rectangle with four straight
edges and a transparent gap between it and the body.

**Head signature:** an open-faced helm with a flat top — no visor, no crest, no plume. The face
stays open and Bone. A faceless bearer is one step from the regular/no-face/Dark-heavy
mechanism held in reserve for enemies, and the protagonist cannot afford that read.

**Value reasoning.** `steel` dominant, Pale ≤9%, Bone rationed to face and hands. Steel is
worked metal and this figure is *equipped* — the bearer as commander, the one who was already
armoured when the fire found them. Dark stays contour only: outline, helm rim, eye notches,
belt, boots. What must be **absent** to hold Steel dominance under the 4-value collapse: no
cast shadow, no doubled outline, no internal outlining between plates. A 50%-Steel design with
heavy internal linework quantizes to a 55%-Dark one.

**At 500 units this reads as:** the single bright rectangle floating above a crowd line, found
in well under a glance. This is the strongest horde-scale performance in the set and it is not
close.

**Rig:** *partial.* Pole and hand are trivially riggable — a rigid prop on a hand socket. The
**2-value flag flip does not translate**: it is a pixel trick, and in 3D the banner becomes
either cloth simulation (expensive, and it will fight the flat-unlit rule the moment it
self-shadows) or a stiff plane that looks wrong the instant the character turns.

**What it buys:** instant findability, and a class grammar that already exists in the codebase
and in `CLASSES.md`. **What it costs:** it takes a registered carrier (§13.1); and it makes a
fictional claim the other three do not — a light held *aloft* is a **signal**. The crowd stands
under it, not in it. That is a subtly different premise from FLAME-FOUNDATION §1, where people
gather into the circle because outside it the dark takes them. Worth deciding on purpose.

---

## 7. Prototype 02 — The Ember Core

**Reads as:** *an empty-handed figure planted wide, lit from a slot cut in its own chest.*

**Carrier:** the caged core inside the body. A small square barred iron window set into the
centre of the breastplate with fire behind it, framed hard on all four sides by Steel and Dark.
**Nothing leaves the silhouette** — no prop, no pole, no cloak, no backpack. The body is a wide
planted base under a narrow hard-edged torso: an **A**.

**Head signature:** **bare.** No helmet, no hat, no hood — cropped hair, blunt face, heavy brow.
This is the only bare head in the set and it is deliberate on two counts: headgear
presence/absence is a differentiator a generator reliably lands, and at 48px the helmet dome is
the shape that survives the collapse (and quantizes to Bone, stealing the eye). Removing it
entirely forces the read down onto the chest, which is where this prototype's whole identity
lives.

**Value reasoning.** `steel` dominant, Pale ≤3% — the smallest bright in the set. The fire is
small because it is *inside* something; a fire you carry in your chest is not a bonfire, it is
an ember, and the armour has to read as thick enough to contain it. Steel gives the Pale a
0.59 luma gap to burn against (Bone would give 0.31 and swallow it). What must be **absent**:
any fire licking past the frame, any rays, any halo dither. The moment light escapes the cage
this prototype becomes the Lampbearer.

**At 500 units this reads as:** **a problem.** Say it plainly, because it is the finding this
prototype exists to produce: a chest-height bright is occluded by the front rank of the
congregation standing in front of you. The bearer disappears into their own crowd. The wide
planted stance is a partial mitigation — an A-shape reads as a distinct mass even when the
bright is hidden — but it is a *second-order* tell, not a beacon. If this direction wins, the
game needs to solve findability some other way (a floor ring, a camera-anchored marker, a
cleared radius around the hero) and that is a systems cost, not an art fix.

**Rig:** **best in the set.** The carrier is body geometry. No prop, no cloth, no pixel trick —
an emissive slot in a breastplate translates unchanged to any renderer, at any resolution, and
survives every animation you will ever put on it. Both hands are free, which means this is the
only prototype whose kit is not already spent before the kit exists.

**What it buys:** the strongest reading of the premise in the set. *You cannot put it down.*
The flame is not equipment; it is a wound or an organ. It also buys total freedom on hands,
weapons and future abilities, and the cleanest path to 3D. **What it costs:** horde-scale
findability, which is the pillar-4 requirement (*readable at scale*) and therefore the most
expensive thing any prototype in this set can be asked to pay.

---

## 8. Prototype 03 — The Tender

**Reads as:** *a hooded figure bent closed around a bowl of fire it is carrying in both hands.*

**Carrier:** the closed loop. A wide shallow hard-rimmed bowl held at waist height in **both**
hands, arms coming forward and closing so that arms, bowl and torso form one continuous ring —
an **O**. The fire sits down inside the rim and never rises above it.

**Head signature:** a deep pointed hood with the face recessed into a Dark cavity, eyes as Dark
notches catching at the back of it. Hoods are a generator staple and land reliably; the recess
is what makes it read as a hood rather than a helm.

**Value reasoning.** `bone` dominant — cloth, robe, bare hands, no plate. This is the only
prototype whose body is made of the *person* value, and that is the fictional claim: a tender
is not armoured, because a tender's job is not fighting. Pale ≤4%, caged in the bowl. Dark is
contour plus the hood cavity and the bowl rim. What must be **absent**: armour plate,
pauldrons, mail, a belt of kit — every one of those pushes toward `steel` and turns a keeper
into a soldier.

**At 500 units this reads as:** a hunched pale shape with a low ember at its middle, inside the
crowd rather than above it. Like 02, the bright loses to occlusion — but unlike 02 the
*silhouette* carries a second, independent tell: a closed hooded loop is a shape nothing else
in the game makes, and it survives at 12px in a way a chest slot does not. This is the
middle-performing prototype at horde scale: worse than 01 and 04, materially better than 02.

**Rig:** **very riggable.** A rigid bowl on a two-handed IK grip is one of the most stable rigs
you can build — the prop constrains both arms, which means the pose is consistent from every
angle and every animation inherits it for free. The one soft cost is the robe, which wants
cloth simulation in 3D.

**What it buys:** the most sympathetic protagonist of the four. A bearer who *tends* rather than
commands, hands full, body bent around the thing everyone else needs — this is the only one of
the four that makes being worshipped look like a burden, which is exactly the discomfort
FLAME-FOUNDATION §4.2 names as probably the best thing in the premise. **What it costs:** both
hands, permanently. This body can never hold a weapon, which forecloses a large part of the
design space before it is explored. And it is the prototype closest to an existing retinue unit
(§5a).

---

## 9. Prototype 04 — The Brazier

**Reads as:** *a narrow figure under a wide iron crossbar of fire — top-heavy, the light riding
on the shoulders.*

**Carrier:** the wide ribbed bar. A broad horizontal brazier basket on a back frame, riding
across the shoulders and projecting well past them on both sides, with a flat top edge. Thick
vertical Dark iron ribs cross the fire so Pale appears **only between the ribs** — split, never
one contiguous field. The body beneath is a narrow waist and narrow legs: a **T**, and the exact
inversion of 02.

**Head signature:** a wide flat-brimmed iron hat. It doubles the prototype's own thesis — this
is the *wide* one, and the brim restates that at head scale. It is also the only brim in the
set (`soldier-roster-v1.md` treats the full-width brim bar as the strongest "helmet" tell at
16px; here it is spent deliberately and on the one subject whose identity is width).

**Value reasoning.** `mixed` — Steel ribs and brim, Bone tunic and skin, Dark cage and contour,
Pale between the ribs at ≤10%, the largest bright budget in the set. Mixed is honest here: this
prototype's identity is **100% silhouette**, and spending a dominance claim on it would waste
one that 01 and 03 need more. What must be **absent**: any flame escaping the cage, any plume,
any rays. The ribs are load-bearing, not decoration.

**At 500 units this reads as:** a wide bright bar at head height, countable across the field
and impossible to confuse with a banner because it has no gap under it. Second-best horde
performance, and arguably the most *distinctive* — 01 is found faster, but 04 is recognised
faster, because no other shape in the game is a horizontal bright bar.

**Rig:** **yes, as specced — and this is the reason it is specced this way.** A rigid brazier on
a spine socket attaches cleanly and animates for free. The version of this prototype that does
*not* rig is the tempting one: flame as a genuinely radiant shape, a fan or cloak of light
spilling past the body's outline. In 3D that is a particle system and a light, both of which the
flat-unlit rule forbids; in 2D it is Pale touching transparency, which is **free light** — the
Lampbearer's `point_halo` — and `quantize`'s `pale_uncaged` audit will flag it. Caging the
radiance behind ribs is what makes this direction buildable at all, in either renderer.

**What it buys:** the horde-scale legibility of 01 without taking the banner carrier, plus the
single most unusual silhouette in the game. The light is *worn*, not held — closer to the
premise than 01's signal, and closer to findable than 02's ember. **What it costs:** the largest
Pale spend in the game on a single subject, which devalues every other bright on screen; a
permanently top-heavy figure that will look wrong doing anything athletic; and it is the one
prototype that needs a carrier ruling before revision 2 (§13.1).

---

## 10. What this round outputs

Not a sheet. Per prototype:

- **`concept128`** — the 128px standard-mode generation, quantized. **This is the image the
  owner looks at.** Retained under the retention rule, never packed.
- **48px anchor** — the same composed description at the locked cell, quantized. The veto pass
  (§3).
- **`output` in the request:** `cell: 48`, `grid: [1, 1]`, `frame_map: {"0": "south.idle"}`,
  `texture: T_Proto_Protagonist_0N`, `content_path: /Game/Sprites/Prototypes`.

The 1×1 grid is deliberate and honest: `south.idle` is the only frame key guaranteed to resolve
from an anchor-only stage, so pack cannot fail on a missing cell. `n_directions: 8` still
returns all eight rotations for the same single generation; the other seven are downloaded and
retained for whichever prototype wins, and re-packed at that prototype's revision 2 as a proper
4×4 sheet with the walk flip.

**Nothing here is imported into Unreal.** A prototype texture in `/Game/` that nobody chose is
how a placeholder becomes canon by accident.

---

## 11. Animation notes

**None this round, and the omission is the method.** `animations: []`, `portrait.enabled:
false`. Animation is a per-direction generation cost that only pays off once the subject is
chosen, and every animation frame inherits the anchor — so animating four candidates is
spending four times to learn nothing the south frame does not already tell you.

Two things about motion do belong in the selection decision, and they are stated here as
predictions to be checked after the choice, not as frames:

- **01** marches on a two-beat and its motion language is the flag riding a rigid pole
  (`vanguard.md` §6). Its walk is the cheapest of the four because the carrier does not move
  relative to the body.
- **02, 03 and 04** each constrain the walk differently: 02's empty hands make it the most
  flexible, 03's two-hand grip locks the arms out of the cycle entirely (which is cheap and
  also means the walk carries no upper-body information), and 04's shoulder-mounted mass should
  *not* bob, or the bar smears at gameplay zoom — which means 04's walk is the one that needs
  the most care and probably a locked upper body.

**The anchor is generated, so nothing here is guaranteed.** The measured tradeoff
(`SKILL.md`, 2026-07-25): a generated anchor comes back off-palette and may drop its defining
prop, where an authored ASCII anchor comes back 100% on-palette and cannot lose the prop — but
comes back as a greybox. This round accepts the first failure mode to avoid the second, because
a greybox cannot answer "which of these is the protagonist". The consequence to plan for is
that **any prototype may come back as a generic soldier**, and if one does, that is a failed
generation and not a failed direction. Re-roll once before drawing conclusions; the reasons to
re-roll are the stage-D list — under 3 values used, a dominant value contradicting the table in
§4, or Pale where the spec says there should be none.

---

## 12. Depends on

**GDD #5 (sprite flipbooks vs. flat-shaded 3D): Neither, for everything that matters here.**

This is unusual for a spec in this project and it is the point of the round. Every claim in
§4–§9 — letter-shape, headgear, where the fire attaches, cage-versus-free, value dominance,
Pale budget — is a statement about **shape and value**, and shape and value survive either
answer to #5. That is deliberate: the owner is choosing a character, not a rendering technique,
and a protagonist decision that only holds under one answer to an unresolved question would be
a bad decision.

The **Rig** lines in §6–§9 are the place where #5 does bite, and they bite *in favour of
resolving it later*: 02 and 03 translate cleanly to a 3D pipeline, 01's flag does not, and 04's
brazier does only in its caged form. If the owner already knows they want a 3D-rigged
protagonist, that is a thumb on the scale toward 02 and 03 — and the round should say so rather
than pretend the two decisions are independent.

The 1×1 sheet and the SubUV frame map in §10 do assume flipbooks, but this round packs no real
sheet, so that dependency is deferred to the winner's revision 2. Recommendation unchanged:
flipbooks on instanced quads, pending the art test.

**GDD #6:** resolved (strict global palette, 2026-07-12). Not re-litigated.

---

## 13. Canon proposals

**1. A protagonist carrier ruling is needed before any winner reaches revision 2.**
`palette.json.shape_carriers` registers the four bright carriers to the four *classes*:
`rectangle_flip` → Vanguard, `dot_cluster` → Relickeeper, `thin_contour` → Pathfinder,
`point_halo` → Lampbearer. This round exposes that the registry has no entry for the thing that
matters most — **the protagonist**. Prototype 01 takes the Vanguard's rectangle outright;
prototype 04 pushes on the Lampbearer's halo and is caged specifically to avoid taking it.
Proposed rule, extending `vanguard.md` canon proposal 2: *the protagonist's flame is rendered in
the registered carrier of whichever class the player is bearing it as; a shape used by the
protagonist is thereby spent for that class and unavailable to any unit-scale sprite of any
other.* If the owner instead wants a protagonist silhouette that is class-independent — which
is what prototypes 02, 03 and 04 quietly assume — then the registry needs a fifth entry and one
of the four classes needs a new carrier. **This is a fiction decision as much as an art one and
it should be made when the winner is picked, not after.**

**2. Promote free-versus-caged light from a spec section to `palette.json`.**
`soldier-roster-v1.md` §10 defines it, `quantize` already computes `stats["pale_uncaged"]`
against it, and this round is the first time a **hero-scale** subject depends on it (prototype
04 is only buildable because of it). A rule that a script enforces and a prose file defines will
drift. Proposed addition under `shape_carriers`:
`"caged_light": {"rule": "Pale enclosed by Dark on all sides, never adjacent to transparency; available to any subject", "free_light": "Pale adjacent to transparency; reserved to the Lampbearer's point_halo"}`.

**3. If prototype 03 wins, `soldier-05` (Flame-tender) must be re-pitched before both ship.**
They are the same fiction — hooded figure, caged fire in a vessel — at two ranks, and a
congregation containing a smaller copy of its own god reads as a confusion rather than a
hierarchy. §5a lists the three separations that would have to hold. This is not a blocker on
this round; it is a consequence to price into choosing 03.

**No faction, biome or NPC is named anywhere in this document.** FLAME-FOUNDATION §5 holds, and
this round does not need one: the question "how does the light attach to a person" is answerable
without knowing who else is in the world.
