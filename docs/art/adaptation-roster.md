# Adaptation roster — rung-to-atlas mapping

Art half of Adaptation (`C:\Users\Hawaiian_spawn\.claude\plans\rustling-inventing-hummingbird.md`).
Decides which existing atlas looks are which ladder's rungs and hands back the
`variant_index` values the data block keys on. **No art generated, no PixelLab credits
spent, no JSON hand-edited.** The binding atlas is `docs/data/art/requests/team-units.json`
(24 rows: 0 = retinue base, 1-10 = spearmen/knight looks, 11-23 = archer looks — see that
file's `frame_map_note` and `docs/design/retinue-melee-subtypes.md`'s index table, both
verified against the tree before this was written).

Two systems share the phrase "stats" below and must not be conflated:

- **Adaptation's own stat spine** is `docs/data/upgrades.json` `tier_ladder.tiers[]`
  (`freed`/`militia`/`veteran`/`bannerman`) — not touched by this doc, not restated here.
- **`melee_subtypes`** (`docs/data/unit-types.json`, task-088) is an already-shipped,
  *unrelated* per-look combat variance system for the ten spearmen knight silhouettes. It
  has no archer equivalent. This doc cites its derived `max_hp` numbers only as a **measured
  bulk proxy** for picking which look escalates to which — never as the rung's actual
  combat stat. Archers have no such table, so archer rungs are argued from raw silhouette
  measurement instead (bbox / opaque / asymmetry), per the CLI note at the bottom.

## The ladders

**4 branches, 16 rungs total** — 1 spearmen, 3 archer. Both unit types share `index 0`
(spearmen) / `index 11` (archers) as every branch's **freed** rung: the pre-split "improvised
weapons" baseline reads as un-individuated on purpose, so branches only diverge once a
soldier is actually equipped (militia+). This is also what makes the atlas budget work —
1 shared + 4 unique = 5 spearmen looks used of 11 on disk; 1 shared + 9 unique = 10 archer
looks used of 13 on disk.

### Spearmen — `spearmen-line` (single branch)

The ten knight silhouettes never fork into distinct *registers* the way the archer block
does (`knight-melee-v1`/`v2` `family.json` axis notes: proportion/topology only, one
sword-and-shield identity throughout) — so one ladder, not several. `docs/design/adaptation.md`'s
worked example already assumes exactly this shape.

| Rung | Tier | `variant_index` | Slug (atlas prefix) | Disk path | Bulk proxy (`max_hp`, melee_subtypes) |
|---|---|---|---|---|---|
| 1 | `freed` | **0** | `retinue` | `RawArt/Renders/unit-retinue-colour/raw/rotations/` | n/a — no subtype row (task-088 addendum) |
| 2 | `militia` | **1** | `knight-v1` / `v1_narrowguard` | `RawArt/Renders/knight-melee-v1/raw/v1_narrowguard/` | 114.5 |
| 3 | `veteran` | **3** | `knight-v3` / `v3_shieldbreak` | `RawArt/Renders/knight-melee-v1/raw/v3_shieldbreak/` | 139.9 |
| 4 | `bannerman` (captain) | **7** | `knight-v8` / `v8_heavycloak` | `RawArt/Renders/knight-melee-v2/raw/v8_heavycloak/` | 164.5 |

Retinue body (the captain's own ≤8-unit escort): `unit_type: spearmen, tier: militia,
variant_index: 1` — same look as the militia rung, so the captain visibly commands
soldiers of its own gear tier, not the base freed look.

### Archers — 3 branches, one per kit register

`pathfinder-line/family.json`'s own `register` note says these twelve read as "firearms,
an RPG launcher, shoulder cannons, a mounted turret, a cyborg energy bow and a mage staff" —
a real fork, unlike spearmen. 9 of the 12 non-base looks partition cleanly into three
3-look registers with no overlap; the remaining 3 (`bowextended`, `crystalstaff`, `arcbow`)
don't cohere into a fourth and are named as a gap below rather than forced.

Every branch's freed rung is shared: `variant_index 11` (`archer-hoodedbow` /
`Merle_Pathfinder`, the group's own base state).

**`archer-scout`** (woodland/stealth register):

| Rung | Tier | `variant_index` | Slug | Disk path |
|---|---|---|---|---|
| 1 | `freed` | **11** | `archer-hoodedbow` / `Merle_Pathfinder` | `pathfinder-line/raw/Merle_Pathfinder/` |
| 2 | `militia` | **13** | `archer-ghilliecloak` / `Can_we_have_one_cove` | `pathfinder-line/raw/Can_we_have_one_cove/` |
| 3 | `veteran` | **15** | `archer-stoutbeard` / `dwarf_version_which` | `pathfinder-line/raw/dwarf_version_which/` |
| 4 | `bannerman` (captain) | **14** | `archer-hollowmask` / `One_with_Hollow_blac` | `pathfinder-line/raw/One_with_Hollow_blac/` |

**`archer-gunner`** (firearms register, escalates by footprint/reach):

| Rung | Tier | `variant_index` | Slug | Disk path |
|---|---|---|---|---|
| 1 | `freed` | **11** | `archer-hoodedbow` | (shared, above) |
| 2 | `militia` | **20** | `archer-twincannon` / `ArtilleryCannons_on` | `pathfinder-line/raw/ArtilleryCannons_on/` |
| 3 | `veteran` | **18** | `archer-riflecrouch` / `Replace_the_bow_with` | `pathfinder-line/raw/Replace_the_bow_with/` |
| 4 | `bannerman` (captain) | **21** | `archer-domedhelm` / `Bump_helmet_ontop_of` | `pathfinder-line/raw/Bump_helmet_ontop_of/` |

**`archer-siege`** (heavy ordnance register) — **revised 2026-08-08, see §4**: veteran and
bannerman swapped from the first pass. `turretnest` is the bigger, more imposing silhouette
(and was the original bannerman pick on that basis alone), but it's a seated ground
emplacement — the wrong read for a rung that specifically has to lead a moving ≤8-unit
retinue. `rocketshoulder`'s striding, weapon-presenting pose reads as a soldier, not
furniture, so it takes the captain slot instead. This costs the branch its monotonic
footprint growth (see §4) — named, not hidden.

| Rung | Tier | `variant_index` | Slug | Disk path |
|---|---|---|---|---|
| 1 | `freed` | **11** | `archer-hoodedbow` | (shared, above) |
| 2 | `militia` | **23** | `archer-carbinesuit` / `Lets_so_secret_servi` | `pathfinder-line/raw/Lets_so_secret_servi/` |
| 3 | `veteran` | **22** | `archer-turretnest` / `Can_you_make_a_Turre` | `pathfinder-line/raw/Can_you_make_a_Turre/` |
| 4 | `bannerman` (captain) | **19** | `archer-rocketshoulder` / `RPG_launcher` | `pathfinder-line/raw/RPG_launcher/` |

Retinue body for all three archer branches: `unit_type: archers, tier: militia,
variant_index: 11` — the shared base archer look, since archers have no per-branch militia
identity distinct enough from the base to justify a second unique retinue sprite per branch
at this budget (see gaps, §5).

## Rung separation, argued from measurement

**Spearmen — `melee_subtypes`' own ten-way ranking is the strongest evidence in this
doc, and it's already both cross-family and cross-run measured.** `docs/sim/SUBTYPE-VARIETY.md`
(task-094, 2 scenarios x 5 seeds, zero rank swaps) ranks the ten knight silhouettes by
`damage_per_unit_committed`, derived from real per-rotation mass/solidity/asymmetry/aspect
measurement, not eyeballed:

```
v8_heavycloak(180.33) > v3_shieldbreak(167.95) > ... > v1_narrowguard(65.94)
```

- militia (`v1_narrowguard`, rank 10) → veteran (`v3_shieldbreak`, rank 2): **60.8% gap** —
  not remotely close.
- veteran (`v3_shieldbreak`, rank 2) → bannerman (`v8_heavycloak`, rank 1): **6.9% gap** —
  the two ARE rank-adjacent in the real ten-way table, and 6.9% clears
  `SUBTYPE-VARIETY.md`'s own cited "clearly separated" floor (5.4%, `docs/sim/SUBTYPE-VARIETY.md:401`)
  with room to spare.
- freed (`retinue`, index 0) carries no subtype row at all — it's qualitatively distinct
  (unarmed baseline vs. three fully-kitted knights), not sim-measured against the other
  three. Flagged, not hidden.

Monotonic in the bulk proxy: 114.5 → 139.9 → 164.5. Good escalation, and it matches the
prose: `v8_heavycloak`'s own `family.json` `edit_description` calls it explicitly *"the
bulkiest, heaviest-looking knight in the group... doubling the visible bulk."*

**Archers — no ten-way sim table exists (archers have no `melee_subtypes` equivalent), so
this rests on `pathfinder-line/family.json`'s `bbox_8rot` (genuinely 8-direction, the
measurement the team-lead brief asked for) plus south-frame opaque/asymmetry as
corroboration only** — flagged per-branch where the corroboration is thin, not glossed over:

| Branch | Rung | bbox (8-rot max, px) | footprint area | south-frame opaque px |
|---|---|---|---|---|
| `archer-siege` | militia `carbinesuit` | 39×46 | 1794 | 874 |
| | veteran `turretnest` | 53×50 | 2650 (+47.7%) | 1269 (+45.2%) |
| | bannerman `rocketshoulder` | 46×46 | 2116 (−20.1%) | 1045 (−17.7%) |
| `archer-gunner` | militia `twincannon` | 46×46 | 2116 | 1239 |
| | veteran `riflecrouch` | 53×46 | 2438 (+15.2%) | 1033 (−16.6%) |
| | bannerman `domedhelm` | 53×50 | 2650 (+8.7%) | 927 (−10.3%) |
| `archer-scout` | militia `ghilliecloak` | 46×47 | 2162 | 1152 |
| | veteran `stoutbeard` | 48×47 | 2256 (+4.3%) | 1178 (+2.3%) |
| | bannerman `hollowmask` | 46×46 | 2116 (−6.2%) | 1090 (−7.5%) |

**`archer-siege` was the strongest bulk case in the first pass, and no longer is, on
purpose** (see §4): militia→veteran (`carbinesuit`→`turretnest`) still climbs hard on both
axes (+47.7% footprint, +45.2% mass), but veteran→bannerman (`turretnest`→`rocketshoulder`)
*drops* on both (−20.1%, −17.7%) because the captain rung was re-picked for pose over size.
The bulk escalation this table shows is real for rungs 1-3; the captain swap trades it away
deliberately for a better "leads a moving retinue" read — flagged, not smoothed over.

**`archer-gunner` escalates on a different, still-real axis**: footprint (reach) grows
monotonically (+15%, +9%) while opaque mass actually *falls* (−17%, −10%). Worth stating
plainly rather than smoothing over — `domedhelm`'s twin muzzles and `riflecrouch`'s leveled
rifle both claim more silhouette *space* than `twincannon`'s denser blob without being
"bigger" in raw pixel count. Escalation-by-reach is a legitimate, different grammar from
escalation-by-bulk, but it is a softer, less immediately readable signal than siege's.

**`archer-scout` is the weakest quantitative case in this doc, flagged as such rather than
forced**: bbox area is flat within ±6% across all three rungs, and opaque count is nearly
flat too (1152/1178/1090). This branch's real separation is the one `pathfinder-line/family.json`'s
own `separation_rationale` already names as the family's actual axis — **kit/headgear
register**, not size: a foliage ghillie cloak, a squat bearded build, and a bone-white
hollow-eyed mask read as three different soldiers at a glance even though none of the four
scalar measurements move much. That's a real, established mechanism in this family (the
whole reason it exists — see `family.json` line 11), just not one the numeric table can
show the way it shows siege's. Owner verdict territory, per `broken-machine/family.json`'s
precedent that a correct visual call can outrank a tighter metric.

## The selection constraint, applied

No two rungs on any one ladder are byte-identical twins. The known duplicate groups (team-lead
brief, confirmed against `docs/data/art/provenance.json`) are: all 5 `archer-proxy` variants
== `pathfinder-line` entries (shared character `b3163cdf`), and
`knight-mass/v0_base == knight-primitive/p0_base == knight-topology/t0_base ==
knight-types/type0_base`. **Neither duplicate group intersects this doc's 14 chosen
variant_index values** — `knight-mass`/`knight-primitive`/`knight-topology`/`knight-types`
are a different family from `knight-melee-v1`/`v2` (the family this atlas actually sources
from) and never entered consideration; `archer-proxy` never entered consideration either
since `pathfinder-line` is the atlas's actual archer source and the two are the *same*
files under two names, not two different looks to choose between. Within each of the 4
ladders, all 4 rungs point at 4 distinct atlas indices — verified directly against the
table in §1, not asserted.

## The captain rung

Two different, both-legitimate visual grammars for "reads as a leader," applied per-branch
based on which one the branch's own art actually supports — not a single rule stamped on
all four:

- **Bulk.** `spearmen-line` (`v8_heavycloak`) puts the single largest/heaviest look in the
  branch at bannerman — independently the #1-ranked-by-size look in its measured table
  above, and its own description explicitly calls itself the biggest thing in the family
  ("doubling the visible bulk").
- **Salience.** `archer-scout` (`hollowmask`) and `archer-gunner` (`domedhelm`) instead pick
  the highest-*contrast* look — a bone-white mask with one hollow eye, and the block's
  heaviest, most enclosed head. `hollowmask`'s own `look` text calls it "the highest-contrast
  head in the block, a bright disc where every other look is dark" — a "point of glow" read,
  the same crowd-legibility grammar this project already uses for Lampbearer (`CLASSES.md`
  §4) and for reserved-value marks generally: a leader needs to be *findable* in a crowd, and
  for a ranged unit that isn't reliably the bulkiest silhouette, high local contrast does
  that job better than size.
- **Pose, the third grammar `archer-siege` needed.** The first pass put `turretnest` at
  bannerman on bulk alone (it's the single biggest look in the whole 13-look archer block,
  tied-widest with `domedhelm`) — but `turretnest` is a seated, ground-mounted emplacement
  (`family.json` `look`: "the only look with ground furniture"), and neither bulk nor
  salience rescues that: a captain has to visibly *lead* a moving ≤8-unit retinue, and
  furniture doesn't lead anything. **Revised: `rocketshoulder` is the captain instead**
  ("a launcher tube shouldered diagonally with a fat warhead clearing the head... a thick
  diagonal crossing the whole silhouette") — a standing, striding soldier presenting a
  dramatic weapon, not the biggest or highest-contrast look available, but the only one of
  the three ordnance-register looks that reads as an individual rather than a position.
  `turretnest` moves to veteran, where "biggest and most dug-in in the branch" is a fine
  read for a seasoned gun crew and doesn't need to lead anyone. Named as a genuine third
  grammar (pose over bulk or salience), not a forced fit — and named as the softest of the
  three grammars in this doc, since "dramatic weapon" is a weaker leadership signal than
  either raw size or a lit contrast marker.

## Gaps and orphans

**Spearmen — 7 of 11 atlas looks belong to no ladder in this v1 pass**: `v2_lanceout`,
`v4_overhead`, `v6_simplecolumn`, `v7_barestance`, `v10_bracedstaff`, `v11_midguard`,
`v13_maceraised`. Brief for a future second branch, not built here: a **reach/polearm**
register — `v2_lanceout` (couched lance) and `v10_bracedstaff` (two-handed chest-braced
polearm) already read as a distinct weapon identity from the sword-and-shield line above;
`v13_maceraised`'s raised overhead weapon (second-highest bulk proxy of all ten, 145.8)
would be the natural bannerman for it. Not assigned here because a second spearmen branch
wasn't asked for and forcing one in without a stated need would be exactly the
"invent a fork nobody asked for" mistake the archer block's *real* fork is the counter-example
to.

**Archers — 3 of 13 looks belong to no branch**: `archer-v2`/`v2_bowextended` (the one
surviving knight-armored archer, `archer-medieval` family), `archer-crystalstaff` (mage
staff), `archer-arcbow` (cyborg energy bow). One-line brief: these don't cohere into a
fourth register on their own — `crystalstaff` and `arcbow` are both plausibly "arcane," but
two looks isn't a 3-rung branch, and `v2_bowextended`'s steel-helmeted, sword-and-board-adjacent
read doesn't fit an arcane register on the same "genuinely forks" standard the three shipped
branches meet. A fourth archer branch needs one more owner-supplied or generated look in this
register before it's assignable — not invented here.

**Archer captain retinue reuses the freed look** (`variant_index 11`) rather than each
branch getting its own unique retinue sprite — flagged as a budget call, not a design one:
archers have no equivalent of spearmen's "retinue body = militia rung's own look" because
this doc kept every archer branch's militia rung branch-specific and distinct, and spending
a second unique look per branch just for the retinue body wasn't asked for.

## The roster.py CLI this doc's expectations depend on does not exist yet

`Scripts/art/roster.py:384` defines `set_field()` (the function the team-lead brief's
`py Scripts/art/roster.py set <slug> expectation "..."` maps to) but `main()`
(`Scripts/art/roster.py:473-506`) only wires `--seed` and `--json` — there is no `set`
subcommand in the tree today. Per the hard constraint ("edit no JSON except via the roster
CLI"), this doc does not hand-edit `docs/data/art/roster.json` to work around that gap.
Flagged to team-lead; the 14 calls below are the intent to run the moment `set` is wired,
recorded here so the mapping-to-expectation work doesn't have to be redone:

```
py Scripts/art/roster.py set shipped/unit-retinue-colour/raw/rotations expectation "Adaptation freed rung, all 4 spearmen+archer ladders share this look via their own unit_type's index (spearmen index 0 / archers index 11) -- unarmed baseline, docs/art/adaptation-roster.md"
py Scripts/art/roster.py set knight-melee-v1/v1_narrowguard expectation "spearmen-line militia rung (variant_index 1) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set knight-melee-v1/v3_shieldbreak expectation "spearmen-line veteran rung (variant_index 3) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set knight-melee-v2/v8_heavycloak expectation "spearmen-line bannerman/captain rung (variant_index 7) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/Merle_Pathfinder expectation "shared freed rung for all 3 archer branches (variant_index 11) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/Can_we_have_one_cove expectation "archer-scout militia rung (variant_index 13) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/dwarf_version_which expectation "archer-scout veteran rung (variant_index 15) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/One_with_Hollow_blac expectation "archer-scout bannerman/captain rung (variant_index 14) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/ArtilleryCannons_on expectation "archer-gunner militia rung (variant_index 20) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/Replace_the_bow_with expectation "archer-gunner veteran rung (variant_index 18) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/Bump_helmet_ontop_of expectation "archer-gunner bannerman/captain rung (variant_index 21) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/Lets_so_secret_servi expectation "archer-siege militia rung (variant_index 23) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/RPG_launcher expectation "archer-siege bannerman/captain rung (variant_index 19) -- docs/art/adaptation-roster.md"
py Scripts/art/roster.py set pathfinder-line/Can_you_make_a_Turre expectation "archer-siege veteran rung (variant_index 22) -- docs/art/adaptation-roster.md"
```

## Depends on

Neither GDD open question #5 (flipbooks vs. flat-shaded 3D) — this doc assigns existing
static rotation sheets to rungs; it does not touch how frames are rendered.
