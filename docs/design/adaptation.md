# Adaptation — unit evolution ladders

**Status:** decision record + spec. Decided by the owner 2026-08-08.
**Companion data:** `docs/data/unit-types.json` (`adaptation` block),
`docs/data/unit-types.schema.md`, `docs/data/upgrades.json` (`tier_ladder`, the stat spine),
`docs/data/scenarios/adaptation-ladder.json`.
**Art mapping:** `docs/art/adaptation-roster.md`.

This document is the decision home. `SYSTEMS.md` §7 and `GDD.md` §12 point here; they carry
the dated entries, this carries the design.

---

## 1. The decision (2026-08-08, owner)

| ID | Decision |
|---|---|
| **A1** | Every character template has an evolution ladder. The term is **Adaptation** — project vocabulary from here on. |
| **A2** | **The player picks from a branch, AND Adaptations can be found in the shop.** Both, not either. |
| **A3** | An Adapted unit is a **new commandable type with its own command handle** — at **one handle per branch, not per rung** (owner, same session, after the handle budget was priced). |
| **A4** | The **top rung of a ladder is a captain**: an individual unit that fields its own small army. Owner's framing: *"It starts with individual teammates for the beginning, then they get their own armies (just smaller for visibility)."* Small retinue counts are a **legibility requirement**, not a balance accident. |
| **A5** | The visual tiers come from art that already exists: *"We have a lot of similar looking characters, those will be the Adaptations."* |
| **A6** | **Friendly side only for v1.** The data shape reserves the enemy case so it drops in later without a schema change. |
| **A7** | `RawArt/Renders/undead-simple/`'s 22 orphan sprites are **out of scope** for this pass. |

### This fills a vacant slot — it does not overturn D4

`SYSTEMS.md`:259-260, closing out the retired growth-site triangle, reads:

> …**Supply capacity are now merchant goods bought with gold**; **promotion's home is open**;
> **hero nodes' route is open** (§8).

D4 (`docs/design/DIRECTION-2026-07-31.md`:38) deleted the growth-site **venue**; D5 deleted the
Ember **currency**. Between them they left promotion homeless. **Adaptation is promotion's
home.**

The split that keeps this clean:

| Axis | Owner | Mechanism | Status |
|---|---|---|---|
| Army **size** | D3 / D4, untouched | kills → army level, automatic, monotonic, persistent | unchanged by this document |
| Army **shape** | **Adaptation** | branch pick + shop | new, 2026-08-08 |

One genuine tension, recorded rather than merged: `GDD.md`:746 (Q22) says *"no allocation
panel, no player spend on army power, no separate meta-currency."* Buying an Adaptation with
gold **is** spending on army power. The owner decided both, so Q22 takes a dated amendment in
place — it stands for army **size** and is amended for army **shape**. Its original sentence
is not deleted. See §7.

---

## 2. What a rung is

```
rung = (unit_type, tier, variant_index)
```

| Key | Resolves to | Notes |
|---|---|---|
| `unit_type` | `docs/data/unit-types.json` `types` — `spearmen` \| `archers` | |
| `tier` | `docs/data/upgrades.json` `tier_ladder.tiers[]` — `freed`/`militia`/`veteran`/`bannerman` | **The stat spine. Not restated anywhere.** |
| `variant_index` | the atlas index — `docs/data/art/requests/team-units.json`, and `docs/data/art/team-variants.json`'s `index_note` | the existing binding key |

All three keys already exist and are already composed by
`Scripts/sim/data_loader.py:96-159` (`retinue_fighter(unit_type, tier)`), and
`docs/data/scenarios/scenarios.schema.md`:31 already accepts `{UnitType, Tier, Count}`. That
is the entire reason for choosing this triple: **the sim can validate a ladder the day it is
written, and no new table is needed.**

No second HP/DPS ladder is invented. `upgrades.json`'s four tiers are the stat ladder.

---

## 3. Rung order

**Rank is `rungs[]` array order.** There is no `rank`, `level`, or `tier_number` field.

Two reasons, both from the existing tree:

- Nothing on disk encodes magnitude today — no manifest, no `family.json`, no roster entry
  carries a rank field. This document does not add the first one.
- The convention already has precedent: `Scripts/sim/decisions.py:77-78` reads
  `tier_ladder`'s **key order** as promotion order.

**Anti-rule, stated so nobody re-derives it wrong:**

> `variant_index` is **NOT** the rank. Atlas index order is arbitrary-by-history (task-085,
> then task-126, then task-128) and is frozen — `Scripts/art/atlas.py:306-309` warns that
> inserting anywhere but the end silently re-points every later stat row. Reordering
> `rungs[]` changes rank. Reordering the atlas changes nothing but stat bindings, and breaks
> them.

---

## 4. Ladder length

Default is **the four tiers that already exist**.

Rung count per template is **not decided**. A fifth rung means inventing an HP/DPS row, and
inventing balance numbers is exactly the inference this project bans. Marked `JUDGMENT CALL`,
filed as **O6**.

---

## 5. The captain rung

**The captain is `bannerman`, reused. Not a new tier.**

`upgrades.json`'s existing `bannerman` row is already captain-shaped, without a single edit:

```json
{ "id": "bannerman", "hp": 160, "dps": 35, "upkeep": 1, "rare": true, "stretch": true,
  "traits": ["aura: +15% attack speed to Liberated within 400uu",
             "SLICE STRETCH — item/event reward only, not a purchasable tier"] }
```

Three things fall out of that for free:

1. Its trait is an **aura over nearby friendlies** — a captain affects the bodies around it,
   which is what a captain is for.
2. `rare: true` means captains are meant to be uncommon. A2's "found in the shop" does not
   flood the field with them.
3. **"not a purchasable tier" makes the captain rung the one rung the shop cannot sell.**
   Captains are earned; everything below them is buyable. That was not designed, it was
   already written — and it resolves A2's shop half without a new rule.

### Captain retinue size

A captain fields its own retinue. Capped at **≤ 8 bodies**.

Derivation, cited rather than invented: no measurement the project owns gives a captain
retinue size. Every friendly-army measurement pins the whole retinue at 100-120, and the
honest total-entity ceiling is ~13,000-20,000 (C3). Neither answers this. The only
**legibility** number the project owns is `TypeLegibilityCeiling = 16`
(`ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h:59`, lowered from 80 on 2026-08-04 on an explicit
owner call that "every unit is a full 8x2 mini retinue"). Half of that ceiling keeps a
captain's clump legible even when two captains overlap, which is the case A4's "just smaller
for visibility" is protecting.

`JUDGMENT CALL` · `PROTOTYPE DIAL`. Open: how many captains a run supports, and whether
captain retinue draws Supply upkeep (`docs/data/economy.json` `upkeep_per_unit: 1`) — **O7**.

---

## 6. Command handles — the honest section

**One handle per branch, not per rung.** A captain plus its retinue is **one** handle, not one
per body.

The reasoning: D14 already says handle count scales with the number of unit **types**. An
Adaptation **branch** is a new type under D14; a **rung** is a look-and-stat move *within* a
type. Four rungs × two templates would consume the entire budget on day one.

### What the runtime cannot express today

All verified against the working tree. **All of it is out of scope for this document** — the
spec states a target state, and the prerequisite is filed as its own backlog task.

> **AMENDED 2026-08-09 — item 4 is LANDED; items 1, 2, 3 and 5 still stand.** A unit's rung
> is now assignable: `USwarmSubsystem::SetSquadRung(UnitIndex, WithinBlockVariant, TierIndex)`,
> with every one of the five phase-roll call sites routed through the new
> `SwarmRenderPack::VariantFor`. Console surface is `Kindled.Adapt <0-7> <ladder> <rung>`,
> reading the ladder table from the `Kindled.Adaptation.Ladders` CVar (transcribed from
> `unit-types.json`) and the stat spine from `Swarm.TierHP` / `Swarm.TierDPS` (transcribed
> from `upgrades.json` `tier_ladder` — no second ladder, §2 holds).
>
> Storage is **per unit, not per soldier** — eight ints on the subsystem — which is what §6's
> own "one handle per branch" rule already implied and is why no fragment grew a field. The
> §6 fear that this was "the one that has no cheap answer" was aimed at per-entity storage;
> per-branch storage was always in budget. It still cost a full editor-closed rebuild
> (item 5, unchanged).
>
> What did NOT land: a branch is still an **existing** command handle, not a new `EUnitType`
> (items 1-2), dispatch is still by slot index (item 3), and the captain rung assigns
> bannerman's look and stats but **fields no retinue** (A4 is its own pass). Reach and cleave
> stay bound to the look, not the tier — the tier ladder has no such columns and adding them
> would be the second stat ladder §2 forbids.

1. **`EUnitType` is one bit, not an index.** `SwarmCombat.h:46-53` defines exactly
   `{Spearmen, Archers}` with `NumUnitTypes = 2`; `SwarmSquad::UnitType()`
   (`SwarmFragments.h:266`) is literally `(Packed & TypeBit) ? Archers : Spearmen`. A third
   type invalidates the pack format plus roughly twenty `bArcher ? A : B` ternaries across
   `SwarmProcessors.cpp`, `SwarmCombatProcessors.cpp`, `SwarmCommands.cpp`,
   `SwarmRenderActor.cpp` and `SwarmFormation.cpp`.
2. **All 8 command handles are consumed.** `MaxSquads = 8` (`SwarmSubsystem.h:46`),
   `TypeLegibilityCeiling = 16` (`:59`). At the shipped `RetinueCap = 128`
   (`ELVTR/Source/ELVTR/Spike/Spike1GameMode.h:64`) that lands exactly 6 Spearmen units + 2
   Archers, all eight full. `AssignRecruit` (`SwarmSubsystem.h:568-617`) silently folds
   recruits into a wrong-shaped unit when exhausted.
3. **D14 is not implemented.** Dispatch is by slot index — `SetUnitStance(UnitIndex, …)`
   (`SwarmSubsystem.h:231-238`), consumed at `SwarmProcessors.cpp:1090-1091`. Comments at
   `SwarmProcessors.cpp:847-850` already assert command-by-type as done while the adjacent
   line reads `GetUnitStance(UnitIndex)`. The only per-unit order surface that exists is the
   console command `Swarm.UnitStance` (`SwarmCommands.cpp:427-450`); the muster HUD is
   display-only (`ELVTR/Source/ELVTR/UI/SquadCard.h:30` has a `bSelected` flag and no click
   handler).
4. **A unit's look cannot be assigned — the hardest one.**
   `SwarmRenderPack::VariantFromPhase` (`SwarmFragments.h:406-423`) recomputes the variant
   every frame from the entity's spawn phase against a live CVar weight table. No fragment
   stores it. *"This soldier adapted into look X"* has nowhere to live.
5. **Any fix is a full editor-closed rebuild.** Live Coding reports success and then crashes
   the next PIE for this module (`SwarmFragments.h:328-331`).

### The one constraint this puts on the design, today

Because the fix is expensive and deferred, the ladder is authored so it costs nothing extra
when the fix lands:

> **Every rung must land on an atlas index that already exists**, or inside free 4-bit
> capacity — 3 free archer slots, 5 free spearman slots. `py Scripts/art/atlas.py check --all`
> must come back byte-identical.

Adaptation adds **zero repack debt**. That is a hard acceptance criterion, not a preference.

---

## 7. The shop

Specifiable now, without inventing a number:

- Adaptations are shop stock. The row reuses `upgrades.json` `items.catalog`'s existing shape
  with `kind: "adaptation"` — no new catalog.
- **The stock rule needs no number:** *the shop offers rungs on branches the player's pick did
  not grant.* That is exactly A2's "both, not either" — the pick is the free path, the shop is
  the paid second path, and they cannot collide by construction.
- The captain rung is not purchasable (§5).

**Price is an explicit `null`** with a note citing **O4** (`GDD.md`:770, Q26 — gold's drop rate
and sources are undecided). `docs/design/DIRECTION-2026-07-31.md`:341 forbids inventing it.
There is no shop spec doc and no shop data file in the repo at all; this document does not
create one. Filed as **O8**, blocked on O4.

### The Q22 amendment

`GDD.md`:746 (Q22) gets a dated amendment clause in place, in the `✅ Decided … · amended …`
form the file already uses elsewhere:

- Q22's *"no player spend on army power"* **stands for army size** — kills still pay size,
  automatically, with no panel.
- It is **amended for army shape** — an Adaptation is buyable at a shop.
- The original sentence is **not deleted**, per the project's supersession rule.

---

## 8. Art

A5's premise holds. 103 of 122 measured variants carry a `keep` verdict, across roughly five
PixelLab body groups, and — the part that matters — **every `family.json` already declares an
`axis` and a `constant`**, which is a written "same character, one thing varies" statement.

**Rule: a ladder is built inside one family's axis, and inherits that family's `constant` as
its identity guarantee.** No new art metadata is introduced. The per-rung mapping lives in
`docs/art/adaptation-roster.md`.

### The duplicate ledger

Thirteen byte-identical groups exist on disk — same files, matching to the pixel across all
thirteen measured fields:

| Group | Evidence |
|---|---|
| All 5 `archer-proxy` variants == 5 `pathfinder-line` entries | shared PixelLab character UUID `b3163cdf`, corroborated in `docs/data/art/provenance.json` |
| `archer-scifi/state06_sharpshooter` == `pathfinder-line/Replace_the_bow_with` | identical 13-field measure, 1033 px |
| `knight-mass/v0_base` == `knight-primitive/p0_base` == `knight-topology/t0_base` == `knight-types/type0_base` | all 1093 px — the exact number `Scripts/art/silhouette_report.py:3-8` names as the founding failure |
| `brood-ooze-colour/rotations` == `brood-ooze/state01_sump` | already documented in that family's own note |

**No tool in this repo compares sprites across families** — `judge_family()`
(`Scripts/art/variantpipe.py:260`) builds its candidate set from one family's `raw_root` only.
That is why none of this was ever caught. Filed as its own backlog task.

Nothing is deleted, no variant weight is zeroed, and no atlas row moves. The ledger becomes a
**selection constraint**:

> **No two rungs on one ladder may be byte-identical twins.**

### Rung assignment is an owner verdict, not a metric

`docs/data/art/families/broken-machine/family.json`'s `checks_run[]` records a visual pass
correctly **overruling the better-measuring pair** — the numerically superior candidate read
as two different creatures. Measurement bounds the choice; it does not make it.

So rung picks are recorded through the `owner` verdict block in
`docs/data/art/families/<f>/manifest.json`, which is built and currently used on **0 of 122
variants**, and rung intent through `py Scripts/art/roster.py set <slug> expectation "…"`,
populated on **2 of 137**. Both layers exist. Neither gets a parallel replacement.

### Out of scope

`RawArt/Renders/undead-simple/raw/`'s 22 sprites are named as a ladder already
(`01_monolith` → `11_monolith_col` → `21_mono_bandaged`, `24_bulk_juggernaut`,
`28_hunch_dragarm`) and sit outside every system — no `family.json`, no manifest, no roster
entry, no atlas row, invisible to `Scripts/art/roster.py`'s `discover_variants`. They are the
obvious **enemy** ladder candidate and are deferred (A7).

Warning for whoever picks them up: **the tier naming is partly false.**
`06_monolith_v2 == 21_mono_bandaged` and `08_wedge_v2 == 25_wedge_void == 26_wedge_horned` are
pixel-identical; only three of the `2x_` entries actually move the silhouette. Do not carry
those names in as real rungs.

---

## 9. Verification and trust boundary

Full command list in the plan and in §10. The two things that matter for reading any result
out of this system:

- **The ladder scenario is `Kind: "point_target"`, deliberately not `wave_attrition`.** Only
  the point-target model passes validation and reproduces `docs/design/entity-tiers.md` §7;
  the wave-attrition model does not reproduce GATE1's measured survival numbers and is
  directional only (`docs/sim/README.md:11-16`, `docs/sim/LIMITATIONS.md`). A ladder claim
  must not rest on the unvalidated model.
- **Adjacent rungs must separate by ≥ 5.4%.** `docs/sim/SUBTYPE-VARIETY.md`:401-403 reports
  every clearly-separated adjacent pair in the ten-way melee ranking at 5.4-23.2%. Anything
  tighter is the same rung and must merge.
- **TTK must fall strictly monotonically across the *power* rungs** — `freed` → `militia` →
  `veteran`. Non-monotonic there is a bug in the rung↔tier mapping, not a balance finding.
  **The captain rung is explicitly exempt.** See below.

### Measured 2026-08-08 — the ladder separates, and the captain rung inverts

First run of `adaptation-ladder`, swept over `Tier`, 120 spearmen against `brood_elite`:

| Rung | TTK | Δ vs prior |
|---|---|---|
| `freed` | 6.75s | — |
| `militia` | 2.70s | **60.0% faster** |
| `veteran` | 1.42s | **47.4% faster** |
| `bannerman` (captain) | 2.08s | **46.5% SLOWER** |

Reproduce:

```powershell
py Scripts/sim/sweep.py adaptation-ladder --axis "scenario:Retinue.Composition[UnitType=spearmen].Tier=freed,militia,veteran,bannerman"
```

**The three power rungs pass by a wide margin** — 60.0% and 47.4% against a 5.4% floor. The
ladder is not theatre.

**The captain rung inverts, and that is correct, not a defect.** `bannerman` is 160 HP / 35 DPS
against `veteran`'s 190 / 45 — it was never authored as a power rung. Its whole value is the
trait `upgrades.json` already gives it: *"aura: +15% attack speed to Liberated within 400uu"*.
**The point-target model has no aura primitive** — it sums one army's DPS against one target,
so a buff to *neighbouring* units is structurally invisible to it. The 2.08s is the captain
fighting **alone and unbuffed**, which is the one situation a captain is never in.

Three consequences, recorded so nobody re-derives them wrong:

1. **Do not "fix" the inversion by raising `bannerman`'s stats.** That would convert a support
   unit into a fourth power rung and delete the one thing making the captain rung a different
   kind of thing rather than a bigger number. It would also move a committed balance row that
   `drift_check.py` guards.
2. **The captain rung's value is unmeasured, and no instrument in this repo can measure it.**
   An aura needs the wave-attrition model (which does not reproduce GATE1 and is directional
   only) or an in-engine measurement. Stated as a gap, not resolved.
3. **This validates A4's shape.** The owner asked for a captain that fields an army, not for a
   stronger soldier. A rung whose worth is entirely in what it does to the bodies around it is
   exactly that — and the sim proving it is *worse alone* is the cleanest possible evidence
   that the captain is a different role rather than a taller stat block.

**Not claimed:** branch-choice theatre-testing. `Scripts/sim/decisions.py` — the project's
branch-divergence instrument — is hardcoded to `growth-sites.json`'s retired action table
(`_actions()` → `rs._load_growth_sites()["site"]["actions"]`,
`MODELLABLE_LANES = ("breadth","depth","sustain")`) and cannot be pointed at an Adaptation
branch today. Repointing it is a Python change and is filed separately. The tier sweep is the
evidence this pass ships.

### Known gap, left alone deliberately

**Archers have no per-tier row.** `Scripts/sim/data_loader.py:126-136` scales archer stats
from the spearmen tier ratio and stamps an `ASSUMED` note; `:149` hardcodes `armor: 0.0`.
Adding real archer tier rows would silently move every existing archer number in the repo.
Not touched here. Recorded so the next reader does not mistake it for an oversight.

---

## 10. Open questions — filed, not answered

| ID | Question | Why it is not answered here |
|---|---|---|
| **O6** | Rung count per template. | A fifth rung means inventing an HP/DPS row. Owner not asked. |
| **O7** | How many captains a run supports, and whether captain retinue draws Supply upkeep. | Owner not asked; `upkeep_per_retinue_body` is an explicit `null`. |
| **O8** | Adaptation shop price. | Blocked on **O4** — gold's drop rate and sources (`GDD.md`:770, Q26). |

---

## 11. Handoffs

- **To whoever implements the C++ prerequisite:** §6's five items are the whole list. The
  variant-storage problem (item 4) is the one that has no cheap answer — bits 25-31 of the
  render int32 are free, but no fragment field exists to feed them, and adding one is an
  editor-closed rebuild.
  **DONE 2026-08-09, and the framing above was wrong in a useful way** — see §6's amendment.
  A rung belongs to a *branch*, so it never needed per-entity storage or those spare bits at
  all; eight ints on the subsystem covered it. Items 1, 2, 3 and 5 are still open and are
  still the real cost. What is left for you is a third `EUnitType`, a handle budget past 8,
  and D14's dispatch.
- **To whoever writes the shop spec:** §7's stock rule is designed to survive O4 being
  answered any way at all. Only the price is blocked.
- **To whoever picks up enemy ladders:** the rung triple is unchanged; swap `tier` from
  `upgrades.json` to `docs/data/entity-tiers.json`. `unit-types.json`'s `adaptation.ladders[]`
  carries a `side` field for exactly this. No schema change should be needed — if one is,
  that is a defect in this spec, not in the enemy pass.
