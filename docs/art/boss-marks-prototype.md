# Brood boss — base silhouette + the six castle-pivot marks (PROTOTYPE)

**Status: prototype-support spec, not final canon.** This backs a readability test —
does a mark read on the outline before it reads as anything else — not a shipped
boss design. Full fight design (phases, arena, per-phase telegraphs) stays out of
scope, per `docs/design/entity-tiers.md` §3's own boundary on the `brood_boss` row.

**Canon read:** `docs/design/castle-layout.md` §6 (marks table, §6.1–6.4),
`docs/data/entity-tiers.json` `brood_boss` (MaxHP 6000, Armor 14, EngageRange 250,
SurroundCap 35–55, `SwingInterval` 2.0s), `docs/design/entity-tiers.md` §Boss ("positioning
+ stances, not a DPS check"; Armor deliberately lower than Titan's despite ~2× the HP so
toughness reads as HP/phases, not an armor wall), `docs/data/art/requests/brood-elite.json`
+ `docs/art/brood-elite.md` (the family register this boss must read as the apex of).

**Family opt-in.** The brief asks this boss to read as the apex of the existing
brood-elite register, which ships on `canon.palette: "demichrome-4"` — that ramp is no
longer the game-wide default (`docs/art/aesthetic-direction.md` 2026-07-28 amendment;
full colour ships elsewhere), but it survives as a named, explicit opt-in
(`docs/data/art/palette.json` `palettes.demichrome-4.superseded`). This spec opts in
deliberately, for family consistency with the sibling asset, not because the ramp is
still the default.

**Cell-size correction.** The brief proposed roughly 64px, reasoning the boss is bigger
than the elite's 48px cell. That's no longer available: 48×48 is a hard pipeline lock
(owner decision 2026-07-25), `pixelpipe.py validate` rejects anything else for a
non-`ui` subject, and it is not a per-spec judgment call anymore. This spec designs the
"biggest thing on the field" read through footprint, stance and outline complexity at
48px, not a bigger canvas. If 48px turns out to be a genuine ceiling once this is on
screen, that's a `## Canon proposals` item for later, not something to route around here.

---

## 1. Boss base silhouette

**The three brood tiers now need a three-way disjointness axis, same audit model as
`docs/art/npc-silhouette-brief.md`'s three-way check (silhouette × value dominance ×
pale usage) — here run over the brood family instead of the NPC one.**

| | Walkers (9 variants, task-059) | Elite (`brood-elite.md`) | **Boss (this spec)** |
|---|---|---|---|
| Base contact | continuous smooth dome, ground-hugging | single narrow root (≤⅓ cell width) | **wide, jagged, multi-point** — several thick stub-limbs planted separately, no continuous base curve |
| Verticality | never past half cell height | rears past ¾ cell height | **low and broad — shorter than the elite's rear, taller than a walker, widest footprint of the three** |
| Signature topology | none (variant axis lives in the low aspect) | one enclosed negative-space hole through the upper mass | **no hole — reserved to the elite; boss's tell is footprint width + limb count, not an interior gap** |

**Reads as:** *the thing that plants itself and does not move — you go through it,
not around it.* This is the visual translation of `entity-tiers.md`'s own boss identity
("positioning + stances, not a DPS check") and of `EngageRange` 250 / `SurroundCap`
35–55 both being the largest of any tier: the boss's melee-reachable perimeter is
physically the biggest circle on the field, and the silhouette should look like it —
broad and grounded, not tall and rearing.

**South frame (the anchor — this is the frame every rotation and every mark
inherits, so it carries the whole spec's weight).** Six thick blunt stub-limbs,
irregular spacing, planted at the base in a jagged ring wider than the body above it —
this is the family's one negative-space-free base yet, so it can't rely on that tell,
disjointness has to come from raw width and limb count instead. Body rises in a single
broad low mound, no rearing curve, roughly half cell height at its crown. One subtle,
currently-unmarked central dorsal ridge runs the spine, present but flat at rest — it's
the site Sated will swell later (§2), so it needs to exist now, quietly. Amorphous
tar-flesh surface, torn-membrane texture, zero bilateral symmetry, no face, no straight
lines anywhere on the unmarked base (the marks are what introduce foreign, angular
shapes — see §2's Ram and Wearing).

**Horde-scale check.** At 500 units on screen, this reads as the single widest dark
silhouette in the crowd — no other brood tier occupies this much footprint at this low
a profile. A player should be able to tell "boss" from base shape alone, at a glance,
before any mark registers, the same way the elite's rearing spike already reads apart
from the walker carpet.

**Zone map — not a pixel/anchor grid, a readability schematic for where each mark in
§2 is allowed to live on the outline:**

```
              [ A: CROWN/BROW ]
   [C: SHOULDER]  [D: DORSAL RIDGE]  [C: SHOULDER]
[E: FLANK]   (body mass, unmarked at rest)   [E: FLANK]
   [B: FRONT-GRIP, forelimbs]      [F: REAR-DRAG, trailing]
        (jagged multi-point limb base, all round)
```

Six marks, six zones, one-to-one. §3 covers the one pair (C/D) close enough to need an
explicit split rule.

## 2. Per-mark silhouette deltas

Each entry: what changes on the **outline** only (interior detail stays flat, per the
simplify-outline-not-interior rule), the canon "reads as" line, and — where the mark's
canon description collides with a family law — the resolution, not just the flag.

### Quilled — Zone E (flank/hide)
**Reads as:** *shafts still in it.* Outline change: 5–9 thin, perpendicular tick-marks
of irregular length and angle protrude from the flank silhouette, breaking its smooth
edge into a bristled one. Value: **steel** (foreign metal, not body). Zero pale spend.
No canon-law tension — nothing here touches face or light.

### Ram — Zone B (front-grip)
**Reads as:** *carrying the door.* Outline change: a large flat rectangular slab is
gripped against the front two limbs, wide enough to visibly extend the front outline.
This is the **only permitted straight edge on the body** — deliberately, since a broken
gate-door is a foreign, man-made object and the family's own language (torn, amorphous,
no straight lines) is what makes a straight edge read as *carried* rather than *grown*.
Value: **steel**. No pale.

### Sated — Zone D (dorsal ridge)
**Reads as:** *visibly swollen, lit from inside.* **Tension:** the family's palette law
(`brood-elite.md` §2 slot 4) forbids self-illumination outright. **Resolution:** this is
never emitted light. The dorsal ridge balloons into a taut, rounded bulge — organic,
not angular, distinct from Wearing's angular Zone C — and the membrane over its crown
thins enough that a **bone-to-pale value patch** shows through, matte and reactive to
ambient/directional light only, the identical device already established for the
elite's crown mark ("catching light only, never emitting"). The *swelling* carries the
"sated" read; the patch only sells "thin enough to show what's under it," not a light
source.

### Wearing — Zone C (shoulder ridge)
**Reads as:** *wearing your dead.* Outline change: angular fragments — a helm crest, a
broken shield edge, a pauldron shape — jut from both shoulder flanks. Second permitted
set of straight/geometric intrusions (after Ram), smaller and more numerous, clustered
rather than one slab. Value: **steel**. No pale. See §3 for the Sated/Wearing zone
split, since both claim "the back" in the canon table's own language.

### Unblinded — Zone A (crown/brow)
**Reads as:** *eyes that do not flinch.* **Tension:** the family's palette law is
explicit — no face, no eyes (`brood-elite.md` prompt `must_avoid`). **Resolution:**
2–4 bare pale eye-dots, asymmetric, no shared spacing, embedded directly in the crown
mass with **no lids, no brow ridge, no mouth, no bilateral arrangement** — eyes without
a face around them, which is a different claim than a face. **Also:** no halo-dither.
Point-plus-halo is the Lampbearer's reserved shape-carrier
(`docs/data/art/palette.json` `shape_carriers.point_halo`); putting a soft glow around
these dots would misread as a friendly light source on an enemy, which is the one
failure mode worse than the tension it would be solving. Bare dots only.

### Column-fed — Zone F (rear-drag)
**Reads as:** *dragging what it caught.* Outline change: a low, trailing mass of
debris/dragged bulk extends from the rear at ground level — unlike every other mark,
this one builds the silhouette *outward and low*, not upward, which is also how it
stays visually distinct from Sated's upward dorsal bulge two zones away. Value:
**dark/steel mix**, reusing existing body values — spends **no pale**, which matters
for the compound check in §3.

## 3. Compounding rules

**Zone ownership (recap):** A=crown/Unblinded, B=front-grip/Ram, C=shoulder/Wearing,
D=dorsal/Sated, E=flank/Quilled, F=rear-drag/Column-fed. Six marks, six zones, no
zone is shared by two marks' *primary* silhouette change.

**The one real collision: C and D.** Wearing (shoulder) and Sated (dorsal) both sit on
"the back," which is exactly what the canon table's own wording risks — a swollen back
carrying broken kit could read as one shapeless lump. Resolved by **shape language, not
just position**: Sated is round, organic, centered on the spine; Wearing is angular,
geometric, flanking it on both sides. A boss with both should read as *a round swollen
back with angular debris jutting from the shoulders* — two legible adds, not a blob.
This is the pairing worth checking first if a generated sprite comes back muddy.

**Pale budget.** Only two marks spend pale at all — Sated (a small patch) and
Unblinded (2–4 bare dots) — and they occupy zones D and A, at opposite ends of the
body. Every other mark (Quilled, Ram, Wearing, Column-fed) spends zero pale. A
worst-case stack that includes both Sated and Unblinded still keeps pale as a minority,
scarce value, matching the family law that pale is reserved and never a body-filling
colour.

**Three-stack test — the one to actually generate.** Canon §6.1's own worked example
is a boss that "took the Great Gate, ate the column that routed from it, and then
survived the Works' archers" — Ram + Column-fed + Quilled. Zones B, F, E: front, rear,
flank — maximally spread, zero adjacency, the easiest three-stack to read and the right
one to burn a generation on first. A harder combination worth a second pass once the
easy one passes — Sated + Wearing + Unblinded (D+C+A, all upper-body) — is the real
stress test of the zone system, since it's the only three-mark stack that's spatially
clustered rather than spread. Not required for this prototype's gate, but flagged so
whoever runs the follow-up knows which combination will actually test the design.

## 4. Generation notes — prototype pass

**Scope: south-facing static states only, no rotation, no animation.** This gates
whether the marks read before any 8-direction spend, per the brief.

**States to generate against one approved anchor** (base + six single marks + the one
three-stack from §3): `base`, `quilled`, `ram`, `sated`, `wearing`, `unblinded`,
`column-fed`, `compound-ram-column-fed-quilled`. Eight south-facing frames.

**Schema note.** `sprite-request.schema.json`'s `prompt` block carries exactly one
`description` — there's no first-class "states" array to hold seven more prompts
alongside it. The linked request below specs the **base/anchor only** (the frame every
mark inherits, per the pipeline's own "one frame defines the whole subject" rule);
whoever runs the states pass should compose each mark's prompt directly from §2's
outline deltas above, not invent new appearance language. This is a tooling gap worth
a schema follow-up (a `states` array mirroring `animations`), not a blocker here.

**Outline choice: full black outline, not the house-default selective outline.** The
general house guidance is to prefer selective outlining to avoid over-accumulating
Dark on quantize. This subject is deliberately Dark-dominant to match its family
(`value_dominance: dark`, same as the elite) — a full black outline isn't fighting that
target here the way it would on a subject meant to read Steel- or Bone-dominant, so
matching the sibling asset's choice is the right call, not an oversight.

**Body template caveat.** PixelLab's `quadruped`/`dog` template is the closest
available skeleton (same as the elite used for four pseudopods) but models four limbs,
not six — none of the five body templates do. The six-stub-limb read has to come from
the prompt text and gets checked at the human review gate, not guaranteed by the
`body_type` param.

**Telegraph, explicitly deferred.** `entity-tiers.json`'s 2.0s `SwingInterval` and
Design Law 6 (a windup must read as a distinct beat at horde scale) both apply to this
boss, but a windup pose is an animation-state concern, not a static silhouette one —
out of scope for this pass, flagged so it isn't silently dropped once animation work
starts.

**Machine-readable request:** `docs/data/art/requests/brood-boss-marks-prototype.json`.

## 5. Depends on

Assumes GDD `#5`'s flipbook/SubUV side. `GDD.md`'s own decision log marks `#5` **closed**
2026-07-28 in favor of Niagara-instanced flipbook quads over flat-shaded 3D — this
spec's cell/grid/frame_map vocabulary throughout only makes sense under that side, so
it isn't "Neither." (Noting the closure since this agent's standing brief still lists
`#5` as open/unresolved; `GDD.md` is the more current source here and this spec follows it.)

## 6. Canon proposals

None. No faction, biome, or NPC is invoked — the boss stays exactly as unnamed as its
siblings (`brood_boss`, `WorkingNameOnly: true`), and every mark reading above is
resolved within existing family law (no self-illumination, no face) rather than by
asking for an exception to either.
