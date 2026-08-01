# Direction 2026-07-31 — invalidation ledger

**Status:** owner decision record + downstream invalidation sweep. Every decision below
was answered directly by the owner on 2026-07-31. This document is the *ledger*, not the
edit: it records what was decided and names every file, line and key that the decisions
break. It authorises no rewrite of `GDD.md` or `SYSTEMS.md` by itself — superseded rows
there get a dated supersession marker in the style Q4/Q20 already use, never a silent
overwrite.

**Sweep scope:** `docs/data/*.json`, `docs/data/*.schema.md`, `docs/design/*.md`,
`docs/backlog/*.md`, `docs/backlog/INDEX.md`. Files outside that glob that the decisions
also hit are listed in §6 so they are not lost.

> **Citation verification pass, 2026-07-31.** Every `file:line` below was re-opened
> against the working tree at `build-space-differentiates` before this file was written.
> Where a drafted citation was wrong it has been corrected in place; where a claim could
> not be verified it has been deleted. Two whole-file corrections came out of that pass and
> are stated up front, because they change what is left to do:
>
> - **`GDD.md` has already been amended** with D1–D17 and C1/C3/C4/C6 by a concurrent
>   session. Its §6 entry below records the applied state and the new line numbers, not
>   pending work. Do not re-apply.
> - **`SYSTEMS.md` has been amended by this pass**, so its pre-edit line numbers (`:3`,
>   `:45`, `:179-183`) no longer resolve. It is cited by **section**, not by line, from
>   here on.

---

## 1. The decision ledger, in brief

### Genre + progression

| ID | Decision | Supersedes |
|---|---|---|
| D1 | Genre is an **incremental roguelite with crowds** — roguelite meta shape (Hades / Risk of Rain), runs are the loop, a permanent ratchet carries between them. | Q15 (decided 2026-07-21) and the old genre spine. **Applied**: `GDD.md`:83-95 states the new spine; :97-112 keeps the 2026-07-21 spine verbatim as the superseded record. |
| D2 | **Full persistent army.** It carries between runs and only ever grows. | The "player power fully resets each run" line. **Applied**: `GDD.md`:139-140. |
| D3 | The persistent army **is** the meta-progression. Army level = f(lifetime kills), monotonic, never falls. No separate meta-currency. | Closes the slot `SYSTEMS.md` §7 left "deliberately unwritten". **Applied** in `SYSTEMS.md` §7 ("Meta-progression — the persistent army") and `GDD.md`:135-143. |
| D4 | **Kills auto-level the army.** No allocation panel, no player spend on army power. Kill economy already ships (`SwarmSubsystem.h`:159-186, `CreditKills` / `RunKilledBySquad`). **Consequence, owner-accepted: the growth-site triangle is deleted** — `SYSTEMS.md` §7–8, `growth-sites.json`, `upgrades.json` retire. |
| D5 | **Embers are deleted.** Three currencies, one per timescale: kills → army level (auto, persistent); gold → items (shops, stash persists); fragments → healer units (in-run, threshold conversion). |
| D6 | **Light fragments** are gathered through a mission; enough in one place convert to a **healer unit**. Boss fragments are **modifiers**, not bodies. Gather method NOT specified → O1, filed in `GDD.md` at **two separate anchors**: the Q24 row in the §12 open-questions log (:768) and the inline flag inside §3's meta loop (:158-160). |

### Run shape

| ID | Decision |
|---|---|
| D7 | Run length **20-30 minutes, 5-8 stages**. The GDD had never stated a session length; **now written in** at `GDD.md`:117-124. |
| D8 | **Dungeon crawling is cut.** A run is escalating arenas with beats between them (shop / rescue / boss). `GDD.md` §1's dungeon-crawler framing and the whole of §9 procgen retire. |
| D9 | **Loss costs stage progress only.** Army, gold, items, stash survive; the run restarts from stage 1. `GDD.md`:128-130. |
| D10 | **WIN = kill the final-stage boss. LOSS = hero death, immediately, no checkpoint** (`GDD.md`:126-127). Retinue reaching zero is **not** a loss (:131-133) — this closes an already-flagged open question. |
| D11 | **Difficulty uses both levers:** stages scale ~60% with army level, AND the stage ladder extends past 8 as the frontier. Needs endless stage generation. `GDD.md`:162-164. |
| D12 | **Shops are separate venues on gold.** Merchant, secret shop, more types. **Item stash persists between runs.** Overturns Loot v0's no-second-economy rule. Stock it from Loot v0's 6 stacking items + Supply/recruit goods — do **not** build task-034. |
| D13 | **Rescue is mid-fight, pressure not countdown.** Captives break free during the arena; the dark takes the ones you do not reach. No UI clock, no new room type, no procgen. This is the reason to move the light circle that `FLAME-FOUNDATION.md`:132-136 calls unsolved. |

### Army + command

| ID | Decision |
|---|---|
| D14 | **Command by type, not by group** — "all archers" / "all spearmen" / "all healers". Handle count scales with the number of unit TYPES, so it never folds. Supersedes the MaxSquads=8 group model (`squad-group-system.md`:376-390, `SwarmSubsystem.h`:46-54), which measurably breaks at ~730 retinue. |
| D15 | **Capstone = boss with adds.** Melee is surround-capped (Boss 35-55 concurrent attackers); ranged is not. Adds give melee mass something to hold while ranged works the boss. The boss fight is currently a stat block, not a design — it still needs writing. |
| D16 | **Headcount is answered by measurement, not by a doc.** Owner declined to name a target. task-108 is **APPROVED** and must include a **RETINUE sweep past 120** — every measurement the project owns pins the friendly army at 100-120. |
| D17 | **Performance is priority 1 of 1.** Named lever: **camera-distance adaptive sim LOD** — at distance the detail (projectiles especially) is not perceptible, so sim fidelity can fall off with camera distance. Correct axis: the sim is 100% of frame cost. |

### Corrections shipping in the same edit

| ID | Correction |
|---|---|
| C1 | `GDD.md` §10's perf claim was **wrong**. The gate **PASSED**: 2.31ms / 433fps measured standalone 2026-07-28 on the Niagara sprite path — a 7.2x margin on the 16.6ms gate. 14.62ms is the DEAD debug-box renderer. **APPLIED**: `GDD.md`:597, :602 ("the 14.62ms figure this section used to carry is dead"), :703-704, and Q19 at :754 now reads "Gate REDEFINED 2026-07-27 … and PASSED 2026-07-28". |
| C2 | **Rendering is measured free** — 0.036 µs/unit; renderer delta vs. sim-only is within noise 500→20,000 entities, two of six deltas negative. **Do not build render-side imposters or sprite LOD.** Perf pre-authorisation is spent SIM-SIDE, in order: (1) abortable `QueryNeighbors` traversal so `SeparationForce`'s NeighborCap bounds the *walk*; (2) `ParallelForEachEntityChunk` (`SwarmSubsystem.h`:155-158 names this as the exact reason the credit write was kept chunk-local); (3) camera-distance adaptive SimLOD stride (D17). |
| C3 | **The 34,000-entity ceiling is not trustworthy.** One standalone run, 07-28; four in-editor sweeps 07-30/31 did not reproduce it, crossing 16.6ms somewhere between ~13,000 and ~26,000. Honest range: **~13,000-20,000 total entities at 60fps pending task-108**, gap marked confounded (standalone vs. in-editor, ±0.9ms harness floor, gpu_ms varying 2-6x between identical passes) but **unresolved**. **APPLIED in `GDD.md`** (Q19, :754: "~13,000–20,000 total entities pending task-108 — 34,000 is not a fact"); still outstanding everywhere in `docs/data/` — see §2.7. |
| C4 | **Strip the dead palette language.** The game ships **full colour** (Quantize 0, superseded 2026-07-28, `docs/art/aesthetic-direction.md`). **APPLIED in `GDD.md`** — pillar 4 at :67-71 now carries the dated supersession explicitly. |
| C5 | **Fix live-looking `WORLD.md` references** the supersession banner does not cover: the §9 biome names (Highgates / Sunken Works / Vesper Halls), `SYSTEMS.md`'s companion-doc header (**done this pass**), `docs/narrative/README.md`:4. |
| C6 | **Kills pay by tier, not flat.** Scale payout with the target's tier from `entity-tiers.json`. Surface the kill counter in the HUD as a rising number. **APPLIED in `GDD.md`**:144-147; the data-side column does not exist yet (§2.8). |
| C7 | **Keep Supply/upkeep as the size governor**, but with Embers deleted, Supply capacity becomes the **merchant's headline good**, bought with gold. Zero re-tuning of `DPS × clamp(capacity/demand, 0.4, 1.0)`. |
| C8 | **Rescue arrivals drip in** over N frames, not one `BatchCreateEntities` call. The only spawn measurement the project owns is a **23.46ms single-frame spike for 250 entities**. |
| C9 | `FLAME-FOUNDATION.md`:132-136 claims "the zero-input baseline *nearly wins*". It **LOSES** — narrowly and repeatably, wave 3 by 4-13 brood (`economy.json`:48). Same warning, opposite sign. **Still outstanding** — the narrative doc is unedited. |
| C10 | `wave-scaling.json`:23 argues from "`economy.json` supply.start_capacity (60)"; task-101 moved it to **120** the same day (`economy.json`:9). The no-degrade-at-wave-1 argument no longer holds as written. **STATUS: NOT YET APPLIED (re-verified 2026-07-31).** `wave-scaling.json`:23 still reads "RetinueStart 60 exactly equals economy.json supply.start_capacity (60)" and `economy.json`:9 still reads 120 — the false equality is live on disk. This ledger records only; the data edit is not made here. |
| C11 | `INDEX.md`:6 ranks by `(feel × risk × unblocks) ÷ cost` with **no performance term**. "Priority 1 of 1" that is not in the ranking rule gets silently re-derived away. Add a performance term. **STATUS: NOT YET APPLIED (re-verified 2026-07-31).** `INDEX.md`:6 and `Scripts/backlog.py`:102 (`SCORE_KEYS = ["feel", "risk", "cost"]`) are both unchanged, so the ranking rule still has no performance term. The generator change is the owner's call and is **not** made by this ledger — see §4.5. |
| C12 | **Un-park task-105.** Under this direction it is the single highest-value backlog item — the only task that puts a small starting army and a massive ending one on screen. **STATUS: NOT YET APPLIED (re-verified 2026-07-31) — owner action required.** task-105 is still `status: parked` (`decided: "2026-07-30 parked"`) and task-108 is still `status: proposed` with an empty `decided:` against D16's APPROVED. Backlog status is the owner's verdict, not this ledger's; recorded here, not landed. |
| C13 | Two live wave curves sit unretired: `SYSTEMS.md` §2 locks 250/450/700 as a dated decision record, `wave-scaling.json`:22-26 proposes 120/400/20,000 with retinue 60→120→600. `encounter-budget.json`, `scaling-curve.json` and `retinue-vanguard.json` all derive from the OLD curve. **Flagged as an open question in `SYSTEMS.md` §2 and O2 below; NOT picked.** |

### Recorded, not built

**F1.** Runs can later become **puzzle combat** — stat guidance where certain unit types
dominate certain factions, making stash and army composition a puzzle to solve rather than
a pure power check. Record in the GDD's forward-looking section. Do not spec, do not file
tasks.

---

## 2. Invalidated data files — `docs/data/*.json`

### 2.1 `docs/data/economy.json` — partially retired

| file:line | What breaks | Required change |
|---|---|---|
| `economy.json`:2 | `$schema_note` points at "SYSTEMS.md §7 and docs/design/retinue-economy.md (task-101 reconciliation)" as the decision home for a triangle that D4 deletes. | Repoint to this document. Restate the file's remaining job: Supply/upkeep only. |
| `economy.json`:6-23 | `supply` block survives **in mechanism** but not in framing — :8's note derives `start_capacity=120` from "the GATE1 refill-to-cap retinue convention" and :11's `upkeep_note` calls uniform upkeep "the core of the triangle". | Keep :9 `start_capacity: 120`, :10 `upkeep_per_unit: 1`, :12-22 `degrade` **unchanged** (C7: zero re-tuning). Rewrite :8 and :11 so capacity is the merchant's headline good bought with gold, not a Provision lane. Delete "the core of the triangle" from :11. |
| `economy.json`:25-33 | The whole `embers` block. :26 declares Embers "the SPEND currency for the RECRUIT / PROMOTE / PROVISION triangle and for items / hero nodes"; :28 `per_brood_killed: 0.1`; :29 `growth_site_grant: 10`; :30-31 `veteran_survives_wave` + the `oath_ledger` hook. | **DELETE the entire block (D5).** Replace with a `gold` block whose drop rate and sources are explicitly **unanswered** (O4) rather than invented, and a `fragments` block whose gather method is explicitly **unanswered** (O1). |
| `economy.json`:35-42 | `scaling_curve.waves` restates 250/450/700 as the canonical curve and :36 calls them "the gate-1 values". `wave-scaling.json` proposes 120/400/20,000 against the same slot. | **Do not resolve.** Add a conflict marker at :35 pointing at O2 and at `wave-scaling.json`:22-26. This is C13's designated flag site because `scaling-curve.json`:2 and :7 both cite *this* key as the lock. |
| `economy.json`:44-49 | `slice_targets` assumes a run-scoped 40-unit start (:45), 2 growth sites (:47), and a growth-site framing throughout. | Delete :47 `growth_sites: 2` (D4). Reframe :45 `start_units` as *the first run's* start — under D2 the army persists and only ever grows, so the start count is a floor, not a constant. |
| `economy.json`:48 | **This line is right and the narrative doc is wrong.** `design_intent` states the gate-1 zero-input baseline "LOSES wave 3 by 4-13 brood" — the measurement C9 says `FLAME-FOUNDATION.md`:132-136 inverts. The rest of the sentence ("the economy layer must be the margin… If a single dominant allocation always wins, the triangle has failed") is dead with D4. | **Preserve the 4-13 brood finding verbatim.** It is the citation C9's correction rests on. Delete the triangle clause after it. |

### 2.2 `docs/data/growth-sites.json` — **retires in full (D4)**

| file:line | What breaks | Required change |
|---|---|---|
| `growth-sites.json`:1-39 | The entire file is the growth-site allocation panel D4 deletes. | **Retire the file.** Do not delete from history — task-097 and task-101 tuned it and those measurements stay citable (see §4). |
| `growth-sites.json`:6-16 | :9-13 the five Ember-priced actions (`recruit` 12, `promote` 15, `provision` 16, `item` 20, `hero` 18); :7 "Player arrives with an Ember pool and allocates"; :15 the budget note that task-101 re-costed. | All dead: D4 removes the venue, D5 removes the currency. **`provision` is the one action with a successor** — it becomes the merchant's Supply-capacity good, bought with gold (C7); carry the `+25 Supply capacity` effect forward, not the 16-Ember price. |
| `growth-sites.json`:18-33 | `slice_placement` — growth-A after wave 1, growth-B after wave 2, with :22 `embers_on_arrival_est: 33` / :29 `52` derived from :23/:30's `brood killed × 0.1 + 10 grant` arithmetic. | Dead. **Four downstream sites cite these two numbers** — `retinue-vanguard.json`:58-59, `run-structure.md`:155-160, `run-structure.md`:305-307, `encounter-budget.json`:70-72. |
| `growth-sites.json`:35-38 | `relation_to_decision_events` ties the triangle to `GDD.md` §6 fork/sacrifice events as "the same axis, two grains". | With the triangle gone, §6's decision events lose their stated mechanical partner. Flag for whoever next touches §6 — this ledger does not re-scope decision events. |

### 2.3 `docs/data/upgrades.json` — **retires as a growth-site file; the item catalog survives as shop stock**

| file:line | What breaks | Required change |
|---|---|---|
| `upgrades.json`:2 | `$schema_note` frames all three blocks as growth-site purchases against SYSTEMS.md §1/§3/§8. | Rewrite: the file becomes the **shop catalog** (D12), priced in gold. |
| `upgrades.json`:7 | `tier_ladder` note: "PROMOTE (depth) spends Embers to move units up a tier" and — the load-bearing breakage — **"All tiers are RUN-SCOPED (reset each run — roguelike purity, GDD §3)."** | **Directly contradicted by D2/D3.** The army persists and only ever grows. Delete the run-scoped sentence; delete the PROMOTE/Embers mechanism. The HP/DPS ladder at :9-12 can survive as the tier stat reference (`wave-scaling.schema.md`:42 keys into it). |
| `upgrades.json`:16-26 | `items.catalog` — the 6 stacking run-items. D12 names these as the shop's starting stock, so the catalog **survives**, but :17's acquisition framing ("3 offered per growth site; take 1") dies. | Re-home to the merchant (D12). :21 `iron_rations` (+15 Supply capacity) becomes a natural second Supply good alongside C7's headline capacity purchase. |
| `upgrades.json`:24 | `oath_ledger` — "each Veteran that survives a wave yields +2 Embers (turns on `embers.income.veteran_survives_wave`)". | **Delete the item.** Its entire effect is on a currency D5 removes, and its target key (`economy.json`:31) is being deleted. Not portable to gold without a new design call — do not invent one. |
| `upgrades.json`:28-43 | `hero_nodes` — :29's "Bought with Embers at a growth site — so a hero node DIRECTLY competes with retinue growth for the same currency. That competition IS the point." | The competition is gone by construction: D4 makes army power automatic and unspendable. Either re-home hero nodes to the shop on gold, or park them. **Owner has not been asked** — do not silently move them. The ability catalog at :30-43 is fine. |

### 2.4 `docs/data/scaling-curve.json` — derives from the old curve and the deleted economy

| file:line | What breaks | Required change |
|---|---|---|
| `scaling-curve.json`:2, :7-8 | `$schema_note` and `population_by_floor_note` both declare 250/450/700 "LOCKED from SYSTEMS.md §2 / economy.json" — the exact lock C13 says is now contested. | Add the O2 conflict marker; do not renumber. |
| `scaling-curve.json`:19-31 | `floor_roster` is keyed on `Floor` 1/2/3 — the dungeon-floor model D8 cuts. | Rekey to **stages** (D7: 5-8 of them, D11: extending past 8). The 3-row shape cannot express an endless ladder. |
| `scaling-curve.json`:37 | `floor3_boss_01` scheduled `"EncounterMode": "isolated_arena"`. | **D15 overturns this.** The capstone is a boss **with adds**; an isolated arena is exactly the "boss alone doesn't pay off a massive army" case D15 rejects. Needs an adds schedule alongside the boss row. |
| `scaling-curve.json`:40-48 | `retinue_growth_curve` — every row's growth is growth-site output, and `SupplyCapacity` runs 60 → 85 → 110 (:41, :42, :43) / 60 → 60 → 85 (:45, :46, :47). | **C10 collision at file scale.** `economy.json`:9 is now 120, so every capacity in this table is below the run's opening capacity. :47's `DegradeMultiplier: 0.944` — the only non-1.0 degrade in the file, and the whole point of the recruit_max scenario — is arithmetic against a superseded 85. Re-derive against 120 or retire the table. |
| `scaling-curve.json`:50-59 | `elite_boss_ttk_sim` — army sizes 50-90, `MeleeBodiesCapped: 20` for Elite (:51-55) and **45 for Boss (:57-58)**. | The `MeleeBodiesCapped` column is the **evidence for D15** and should be promoted, not deleted. The `ArmyN` values (50-90) are the exact 100-120 pin D16 orders task-108 to sweep past. |

### 2.5 `docs/data/encounter-budget.json` — largest single casualty of D8

| file:line | What breaks | Required change |
|---|---|---|
| `encounter-budget.json`:2 | `$schema_note` states the file's job as "(a) procgen room-type budget spend, (b) main-arena arrival pacing, (c) an optional risk-room bonus pocket…". | (a) and (c) die with D8/D13. (b) — pulse/lull arrival pacing — **survives and gets more important**, because C8's drip-in requirement is the same mechanism. |
| `encounter-budget.json`:41-56 | `room_types` — every row. :43/:48/:52 Corridors, :44/:53 DecisionSites, :45/:49/:54 RiskRooms, :55 BossRoom, all framed as a procgen room graph. | **Delete the room graph (D8).** Keep only the Arena rows (:42, :47, :51) as stage population sources. |
| `encounter-budget.json`:45, :49, :54, :69-73 | The Risk Room is the repo's only rescue venue — a side room entered before the fight, cleared for BonusFodder. **D13 replaces it outright**: captives break free *mid-arena*, under pressure, with no side room and no clock. | Delete `room_types` RiskRoom rows and the whole `risk_room_budget` table. The captive counts (20 / 35 / 55) are the only rescue-volume numbers the repo owns — carry the *magnitudes* into the new mid-fight rescue spec, not the room. |
| `encounter-budget.json`:70-72 | Each `RewardNote` prices a Risk Room in Embers: "standard per-kill Ember rate (economy.json, 0.1/kill = +2 Embers)" / "+3.5" / "+5.5". | Dead currency (D5) in a dead room (D13). |
| `encounter-budget.json`:7-8 | `entity_ceiling_niagara_measured: 20000` and `entity_ceiling_60fps_simlod4: 34000`. | **C3.** The 34,000 must stop being stated as a constant. Replace with the honest ~13,000-20,000 range and a confounded-but-unresolved marker. |
| `encounter-budget.json`:76-79 | `peak_concurrency_check` — every row carries `PctOf34kSimLOD4Ceiling` (0.91 / 1.61 / 2.49 / 0.268). | All four are arithmetic against the untrusted divisor. Recompute against the range, or drop the column until task-108 lands. |
| `encounter-budget.json`:82-89 | `rank_arrival_context` — `RetinueN` 40 / 60 / 90 across the pulse rows, plus 120 for the gate1 calibration (:83). | The 100-120 pin D16 names. Nothing in this table reaches the army size D1/D2/D11 imply. |
| `encounter-budget.json`:10-16 | `pulse_lull_seconds` (3.0) and `elite_lead_seconds` (1.5), both flagged JUDGMENT CALL. | **Survives and is promoted.** C8's "drip in over N frames rather than one `BatchCreateEntities` call" is the same lever at a finer grain — the 23.46ms/250-entity spike is the measurement this block now has to answer to. |
| `encounter-budget.json`:18 | `risk_room_access_note` — the whole "scout the side room before committing to the fight" framing. | Delete (D13). |

### 2.6 `docs/data/retinue-vanguard.json` — rescue model replaced, growth-site inputs orphaned

| file:line | What breaks | Required change |
|---|---|---|
| `retinue-vanguard.json`:11-13 | `rescue_conversion_rate: 0.25`, defined at :13 as "Fraction of a cleared Risk Room's `BonusFodderCount` converted to rescued Freed-tier retinue". | **D13 replaces the entire input.** There is no cleared Risk Room to take a fraction of; rescue is now a mid-fight reach-them-in-time problem. The 0.25 dial has no domain left. |
| `retinue-vanguard.json`:42-46 | `rescue_and_rally` — all three rows compute `RescuedFreed` from `encounter-budget.json risk_room_budget[]`. | Recompute from the mid-fight captive model once it exists. Until then the table has no source. |
| `retinue-vanguard.json`:48-53 | `three_floor_ledger` (40 → 35 → 26 → 13, rows :49-51) and the **"STARVE"** verdict at :53. | The verdict is **scoped to one run under a deleted economy**. Under D2/D3 the army carries between runs and only ever grows, so "starve across the slice" is no longer the failure mode it was filed as. Do not silently delete the finding — re-scope it: *within* a single run attrition still bites, and that is a real tuning input for D9's stage-progress-only loss. |
| `retinue-vanguard.json`:55-66 | `growth_site_context` — the whole block, explicitly ":56 sourced from docs/data/growth-sites.json". :58-59 cite `EmbersOnArrival` 33 / 52; :63's `NextFloorEntering: 86` is the number the comparison ledger turns on. | **Orphaned by D4** — its only source file retires. Delete the block. |
| `retinue-vanguard.json`:68-71 | `per_floor_soft_cap` — ":69 Governed entirely by the existing Supply/upkeep degrade mechanic… Practical per-floor ceiling = wherever Supply capacity (Provision-funded, unchanged) runs out". | **The rule survives (C7), the funding source does not.** Replace "Provision-funded" with "merchant-purchased, gold-funded". :70's worked example (28 rescued units added to demand for zero cost) still holds and gets *more* pointed, since rescued bodies are now free but capacity costs gold. |
| `retinue-vanguard.json`:7-9, :30-40 | `starting_headcount: 40` and the `danger_index`/`expected_losses` chain built on `encounter-budget.json` pulse rows. | Survives arithmetically but sits on the old 250/450/700 curve (O2) and on floors that D8 turns into stages. |

### 2.7 `docs/data/wave-scaling.json` + `wave-scaling.schema.md` — the 34k divisor, C3 and C10

| file:line | What breaks | Required change |
|---|---|---|
| `wave-scaling.schema.md`:32 | **The single most load-bearing stale line in `docs/data/`.** `HeadroomPctVs34kCeiling` is *defined* as `(34000 - TotalMassEntities) / 34000 * 100` — "the measured `Swarm.SimLOD.Stride 4` 60fps ceiling (`one-camera-bench.md` §8, run 5, measured to 40,000, not extrapolated)". Every headroom claim in canon divides by this. | **C3.** Rename the column off the hard-coded number and redefine against the honest ~13,000-20,000 range, with the confound stated (standalone vs. in-editor, ±0.9ms harness floor, gpu_ms varying 2-6x between identical passes) and marked **unresolved** pending task-108. |
| `wave-scaling.json`:7-8 | `ceiling_60fps_simlod_stride4: 34000` with a note asserting "**Measured, not extrapolated**". | The assertion is the problem: one standalone run, four in-editor sweeps failed to reproduce. Restate as a single unreplicated observation. |
| `wave-scaling.json`:9-10 | `ceiling_60fps_no_lod: 21000` and its note — "the late wave's headroom against the 34k figure collapses to ~2% against this lower figure". | The 21,000 figure is *closer* to the in-editor 13,000-26,000 crossing than the 34,000 is. Under C3's range the late wave is not 39.4% clear; it may be over the line. |
| `wave-scaling.json`:23 | wave-1 note: "**RetinueStart 60 exactly equals economy.json supply.start_capacity (60)**: upkeep demand == capacity, no degrade multiplier triggers at wave 1." | **C10 verbatim.** `economy.json`:9 is now **120** (task-101, same day). The equality is false and the no-degrade-at-wave-1 argument no longer holds as written. Either restate (60 demand against 120 capacity — comfortably under, degrade still doesn't trigger, for a different reason) or re-derive `RetinueStart`. **NOT YET APPLIED — re-verified on disk 2026-07-31, the "(60)" claim is still there.** |
| `wave-scaling.json`:23-25 | `HeadroomPctVs34kCeiling` 99.47 / 98.47 / **39.41**. | All three are the untrusted divisor. The 39.41 is the figure task-108 exists to replace. |
| `wave-scaling.json`:25 | wave3_late: `RetinueStart: 600`, `EnemyPopulation: 20000`, `TotalMassEntities: 20600`, `RetinueMechanism: "growth_site_purchase (exceeds what economy.json's current simulated scenarios reach)"`. | The **mechanism is deleted** (D4) — retinue 600 now arrives by persistent army growth (D2/D3) plus mid-fight rescue (D13), which is a *better* story for this number, not a worse one. Rewrite the mechanism string. |
| `wave-scaling.json`:19 | `surround_cap_handoff_note` — flags `entity-tiers.json`'s SurroundCapEstimate finding forward "if a longer/co-op run ever pushes army size past ~120", noting wave3's 600 is exactly that scenario. | **Promote from flagged-aside to D15 canon.** This note is the reasoning D15 states as a decision. |
| `wave-scaling.json`:50-56 | `elite_boss_schedule` — `wave3_boss_01` at `"EncounterMode": "isolated_arena"` (:55). | Same as `scaling-curve.json`:37: **D15 requires adds.** |
| `wave-scaling.schema.md`:42 | `Tier` column documented as "Key into `upgrades.json`'s tier ladder". | Orphaned reference if `upgrades.json` retires wholesale (D4). Keep the tier ladder alive as a stat reference, or repoint. |
| `wave-scaling.schema.md`:70-71 | Conventions: "see the design doc's own supersession section before treating any number here as replacing `SYSTEMS.md`", and the all-at-once peak-concurrency assumption. | The supersession is now **owner-level, not doc-level** (O2 is the open question, C13 the conflict). The all-at-once assumption is what C8 forbids for rescue arrivals specifically. |
| `wave-scaling.json`:2 | `$schema_note` claims outright to be "superseding SYSTEMS.md line 45's locked 250/450/700 curve". | **Nothing adjudicated that claim.** It is now explicitly logged as contested — `SYSTEMS.md` §2 carries the conflict marker, O2 carries the question. |

### 2.8 `docs/data/entity-tiers.json` + `entity-tiers.schema.md` — C6 needs a new column

| file:line | What breaks | Required change |
|---|---|---|
| `entity-tiers.json`:18, :37, :56, :75, :95, :115 | Six tier rows (`brood_fodder`, `brood_soldier_melee`, `brood_soldier_ranged`, `brood_elite`, `brood_titan`, `brood_boss`) and **no kill-value column anywhere in the file**. A fodder kill and a titan kill pay the same. | **C6: add a per-tier kill-payout weight.** `GDD.md`:144-147 already states the decision; the data column to satisfy it does not exist. |
| `entity-tiers.schema.md`:17-36 | The tier column table documents `MaxHP`/`Armor`/`DPS`/`SwingInterval`/`EngageRange`/`TargetsPerHit`/`MoveSpeedScale`/`SurroundCapEstimate`/`SurroundCapRange`/`TelegraphWindup`/`Notes` — no payout column. | Add the C6 column with its units and range; it is the input the D3 army-level function reads. |
| `entity-tiers.json`:129-130 | `brood_boss`: `SurroundCapEstimate: 45`, `SurroundCapRange: "35-55, Fermi estimate not measured"`. | **This is the number D15 cites.** It is now load-bearing design canon and must be measured, not left Fermi — see `entity-tiers.md`:301-304's own request. |
| `entity-tiers.json`:132 | `brood_boss` Notes: "BASELINE STAT BLOCK ONLY — full phase/arena/mechanic design belongs to the separate 'Boss & elite design' scope area". | Still true and now urgent (D15: "the boss fight is currently a stat block, not a design; note that it still needs writing"). |

### 2.9 `docs/data/loot-v0.json` + `loot-v0.schema.md` — D12 overturns the no-second-economy rule

| file:line | What breaks | Required change |
|---|---|---|
| `loot-v0.json`:67-72 | `stacking_items_reference` — `"source_of_truth": "docs/data/upgrades.json items.catalog"` (:70) and `"acquisition": "growth-site 'item' action (docs/data/growth-sites.json site.actions), offer-3-take-1"` (:71). | Both source files retire (D4). Repoint acquisition to the **merchant** (D12); keep the 6 catalog ids at :69 as the shop's opening stock. |
| `loot-v0.json`:37-46, :48-55 | `kindling_ember_working` and `rally_ember_working` — both `WorkingNameOnly: true` (:39, :50). | **Name collision with a deleted currency (D5).** Nothing mechanical breaks, but shipping "Ember" pickups after Embers are deleted re-implies the dead economy. Rename on the next art/UI pass; the working-name flag already licenses it. |
| `loot-v0.json`:58-64 | `drop_sources` has UnitOrb / KindlingEmber / BattleBuff columns and **no gold row and no fragment row**. | D5 and D6 both need drop plumbing here. **Do not invent rates** — file O1 (fragment gather method) and O4 (gold rate/sources) instead. |
| `loot-v0.json`:33 | Unit Orb "adds +1 to Supply demand like any other Freed unit (economy.json upkeep_per_unit=1), so orbs stay inside the single existing definition of what a unit costs". | **Survives intact under C7** — the one part of the Loot v0 economy story that the new direction strengthens. |
| `loot-v0.schema.md`:72-73 | Documents `whetstone` as "a permanent, Ember-bought +DPS stack" against `rally_ember_working`'s free temporary one. | Ember-bought → gold-bought (D5/D12). |

---

## 3. Invalidated design docs — `docs/design/*.md`

### 3.1 `docs/design/squad-group-system.md` — the command model D14 supersedes

| file:line | What breaks | Required change |
|---|---|---|
| `squad-group-system.md`:33 | "`USwarmSubsystem` already has the right shape for a **squad**: `MaxSquads` (8)…" — the premise of the whole document. | **D14 supersedes the group model.** Handles now scale with the number of unit **types**, not a fixed 8. |
| `squad-group-system.md`:43-48 | The up-front flag: "splitting one pool into two makes the existing 8-squad-handle ceiling bind *sooner*, not later — §4.2 simulates this and finds the model breaks at a lower total retinue (~730)". | This is the **evidence for D14**, not a caveat any more. Promote it. |
| `squad-group-system.md`:338-364 | §4.1's replacement formula: `WantedUnits(type) = ceil(Pool(type) / 80)`, `Units(Archers) = clamp(…, 0, MaxSquads - Units(Spearmen))` (:351-353), and the "Spearmen claim first" policy (:356-364). | Under command-by-type there is no budget to split: one handle per type, N types. The clamp, the claim order and the overflow-fold rule all go. |
| `squad-group-system.md`:366-390 | §4.2's simulation table — :376's breaking row (**retinue 728: Spearmen 8 units, Archers folded into one 146-body unit, over ceiling**) and :378-390's headline ("Spearmen alone consume the entire 8-unit budget once their own pool passes ~580… the two-type split makes the 8-handle ceiling bind *sooner*"). | **The measured failure D14 cites.** Keep the finding as the reason for the supersession; retire the model it measures. |
| `squad-group-system.md`:392-398 | §4.3 "`MaxSquads` stays 8 … Raising it, or reserving a *guaranteed minimum* per type … is the natural next lever if §4.2's ~730 ceiling proves too low". | Superseded. D14 takes neither of the two flagged levers — it changes the axis. |
| `squad-group-system.md`:719-750 | §11 assumptions **1** (:719, v1 types are exactly Spearmen and Archers), **2** (:722, squad = unit), **5** (:729-732, the `ceil(pool/80)` Spearmen-claims-first formula) and **10** (:748-750, `MaxSquads` stays 8). | 5 and 10 are dead. 1 needs widening: D5/D6 introduce **healers** as a commandable type, so v1 is at least three types, and D14's handle count is a function of the roster. |
| `squad-group-system.md`:134-143 | §1.4 recruitment: type is "generator-tagged" — "Each growth site the procgen layer places is tagged with which type it yields". | **Two dead dependencies in one sentence** (D4 growth sites, D8 procgen). Type assignment needs a new source. |
| `squad-group-system.md`:555 | "`Emberkeep.UI.ViewCam` still defaults to 0". | Stale CVar name — `Kindled.*` since the 2026-07-31 rename. Already task-117's territory; noted so it is not re-published. |
| `squad-group-system.md`:96-107, :199-232 | §1.2's per-unit ownership table and §1.7/§1.8's per-type formation and stance reflavors. | **Survive and get stronger under D14** — a per-type command model is exactly what these tables describe. The archers-behind-spearmen `Forward` gap (250 vs 40) is unaffected. |

### 3.2 `docs/design/run-structure.md` — D8, D9, D10, D13 rewrite it

| file:line | What breaks | Required change |
|---|---|---|
| `run-structure.md`:1-8 | Title "**Run structure — start to boss, three floors**" and the framing "start → 3 floors → boss → victory/death screen". | **D7: 5-8 stages, 20-30 minutes.** **D11: the ladder extends past 8 as the frontier**, so the count is a starting shape, not a fixed list. `GDD.md`:117-124 now states this; this doc has not caught up. |
| `run-structure.md`:36-41 | "**'Wave' becomes 'Floor,' and a floor is more than its fight.** A floor is a small room graph: entrance corridor → optional Risk Room → (floors 1/3 only) Decision Site → mandatory Arena." | **D8 cuts this outright.** A run is escalating arenas with beats between them (shop / rescue / boss). No room graph, no corridors, no procgen. |
| `run-structure.md`:42-46 | "The flat refill-to-120 is replaced by the growth-site economy, already DECIDED and specced (`SYSTEMS.md` §4/§6/§7, `growth-sites.json`)." | Growth sites are deleted (D4). Between-stage replenishment is now: persistent army (D2), mid-fight rescue (D13), and fragment→healer conversion (D6). |
| `run-structure.md`:47-51 | "Win condition moves from 'clear wave 3' to '**kill the boss**'." | **Confirmed by D10** — one of the few lines the direction ratifies rather than breaks. It is now stated canon at `GDD.md`:126. |
| `run-structure.md`:87-104 | The floor-template diagram — corridors, Risk Room, Decision Site, per-floor pulse counts, ":102 [Floors 1 & 2 only] Growth site". | Replace wholesale with the stage template: arena → beat (shop / rescue / boss). |
| `run-structure.md`:106-110 | The per-floor table (Population / Elites / Decision site / **Growth site after** / Boss). | Drop the growth-site column; extend rows to 5-8 stages (D7) with a stated generation rule past the authored ladder (D11). |
| `run-structure.md`:141-176 | §3 "Does the retinue persist across floors, and at what cost?" — answered "Yes", with the cost stated as **Embers** (:155-160, quoting 12/15/10/20/18 and the ~30-52 pool) and **Supply demand** (:161-166). | The question is now trivially answered at a level above: **the army persists between whole runs** (D2). The Ember channel is deleted (D5); the Supply channel survives verbatim (C7). Rewrite the section around D9's within-run/between-run split. Note :157's "Provision (10E)" is itself pre-task-101 and was already stale before this direction. |
| `run-structure.md`:168-175 | The Risk Room framing — "a player weighing 'risk retinue now for Embers to spend at the growth site'". | Both halves dead (D13 room, D5 currency). |
| `run-structure.md`:177-191 | The headcount-gap caveat: "the growth-site economy never gets the retinue near gate-1's own measured 120-count survival calibration". | The premise (growth-site economy) is deleted, but **the gap is real and gets worse**: D11 wants stages scaling ~60% with army level and D16 wants headcount measured past 120. Re-scope, don't drop. |
| `run-structure.md`:195-213 | §4 "What gates the boss?" — clearing Floor 3's Arena, with the finding built on ":198 no growth site between Floor 3's Arena and the boss" and growth-B being ":208 the last economic decision that has any bearing on the boss fight". | The finding evaporates with growth sites (D4). What replaces it: **D15** — the boss is a boss-with-adds capstone, and what gates it is the stage ladder, not an economy beat. |
| `run-structure.md`:226-233 | §5 "**Unchanged from gate-1: hero death ends the run, immediately, at any point.**" | **Ratified by D10** — keep verbatim; `GDD.md`:127 now states it as canon. |
| `run-structure.md`:234-245 | "**Retinue reaching zero is not itself a loss condition**, and this doc flags rather than resolves that gap… is genuinely open." | **D10 CLOSES this open question.** Retinue zero is NOT a loss (`GDD.md`:131-133). Both the leash and the degrade-not-die rule depend on the closure. Replace the flag with the dated answer. |
| `run-structure.md`:246-252 | "**No mid-run checkpoint** … a wipe on floor 3 sends the player back to floor 1's entrance, matching the roguelike frame GDD §3 states plainly ('run ends… reset')." | **D9 keeps the restart and deletes the reset.** Army, gold, items and stash all survive; only stage progress is lost. The GDD §3 quote this line leans on is exactly what D2 superseded. |
| `run-structure.md`:262-283 | §6's `ERunPhase` target table — `FloorExplore` (:272), `ArenaActive` (:273), **`GrowthSite`** (:274), `BossActive` (:275), `Won`, `Lost`. | `FloorExplore` dies with D8. `GrowthSite` dies with D4 and is replaced by a **Shop** state (D12). A **Rescue** beat is not a state at all — D13 puts it *inside* `ArenaActive`, which is the design's whole point. |
| `run-structure.md`:296-307 | Handoffs to ":296 whoever revises `SYSTEMS.md` §2 or §7" and ":305 whoever next revises `growth-sites.json`'s Ember arrival estimates". | The second handoff has no recipient (D4). |
| `run-structure.md`:370-379 | §10's canon proposal about `GDD.md` §9's "≥1 growth site per floor" wording being stale. | **Overtaken: D8 retires the whole of §9.** The one-line fix this doc proposed is no longer the right edit. |

### 3.3 `docs/design/loot-v0.md` — D12's overturned rule

| file:line | What breaks | Required change |
|---|---|---|
| `loot-v0.md`:17-18 | "`SYSTEMS.md` §7 (Supply/Embers economy — **this doc's Unit Orb is additive to it, not a second economy**)". | **D12 explicitly overturns the no-second-economy rule.** Shops are a separate venue on their own currency, deliberately not competing with army growth. |
| `loot-v0.md`:79-84 | The principle in full: "**Additive to the Supply/Ember economy, never a second economy.** … none of the three drop types can be banked, saved, or converted into Embers — they are a pure, small, real-time bonus layered on top of the growth-site triangle, **not a parallel currency competing with it**". | Rewrite. Gold is precisely a parallel currency, and D12's reasoning is that it *doesn't* compete because army power isn't purchasable at all (D4). State that reasoning — the rule changes for a stated reason, not by drift. |
| `loot-v0.md`:44-47 | The two-grain table — :47 "Growth-site items \| breather \| **Embers** \| curated, offer-3-take-1 \| `upgrades.json`". | Row becomes: shop items \| shop venue \| **gold** \| curated \| shop catalog. |
| `loot-v0.md`:62-65 | "**Run-scoped, no persistent gear** (Design Law 7). All three drop types are consumed instantly on pickup — there is no inventory, no carry-over between floors, **no state that survives a run.**" | **D12: item stash persists between runs.** The three *battle drops* can stay consumable, but the blanket "no state survives a run" is now false at the file's own framing level, and Design Law 7 itself is in tension with D2/D12 — flag it, don't quietly reword it. |
| `loot-v0.md`:22-25 | "**Does not touch:** `docs/data/upgrades.json`, `docs/data/economy.json`, `docs/data/growth-sites.json`, `SYSTEMS.md`, `GDD.md`…" | Two of those three data files retire and the third loses half its content; this doc's clean-boundary claim no longer describes reality. |
| `loot-v0.md`:254 | "**They spend no Embers and offer no choice.**" | Dead currency. |
| `loot-v0.md`:271-278 | §7's Rally-Ember-vs-Whetstone non-competition argument, which turns on Whetstone costing Embers (:276 "because Rally Ember costs no Embers"). | Re-derive on gold, or drop — the argument's structure survives the currency swap intact. |
| `loot-v0.md`:3-9 | Scope: this is "`Loot v0` … *not the real loot system*", explicitly not redesigning the deferred full system. | **Ratified by D12**, which names this catalog as the shop's starting stock. *(Corrected: this doc does not itself mention task-034 anywhere — the "do NOT build task-034" instruction is the owner's, in D12, not a quote from here.)* |

### 3.4 `docs/design/entity-tiers.md` — C6 and D15

| file:line | What breaks | Required change |
|---|---|---|
| `entity-tiers.md`:242-256 | §4 "melee is surround-capped against a single big target; ranged isn't", with the ranges stated as Fermi estimates — "**Elite 15-27, Titan 28-45, Boss 35-55**" (:251-252). | **D15 is built on this section.** "Boss 35-55 concurrent attackers" is quoted directly in the decision. It moves from a flagged finding to the load-bearing reason the capstone needs adds. |
| `entity-tiers.md`:258-265 | "**Ranged attackers … are not subject to this cap** … melee's contribution to an Elite/Titan/Boss kill is **flat** past the surround cap (identical whether the army is 50 or 250), while the fight only gets faster with army size because the **archer** contribution keeps growing." | The exact mechanism D15 names: a boss alone does not pay off a massive army. Promote to canon. |
| `entity-tiers.md`:267-280 | The `growth_source_weight` tension (0.8 Spearmen / 0.2 Archers is weighted *away* from the sub-type that scales against big targets), and the flagged future fix — ":276-279 a future multi-hitpoint boss design that gives melee a way to matter past its own surround cap". | **D15 chose the other answer:** adds, not weak points. Record the decision against this flag so it stops reading as open. |
| `entity-tiers.md`:229-238 | §3's Boss row — "**stat block only** … Full fight design (phases, arena, mechanics) is out of scope for this doc". | D15 confirms and escalates: the boss fight still needs writing. Note :234 already says its toughness "should read as HP/phases/**adds**" — D15 picks that branch. |
| `entity-tiers.md`:301-304 | "`SurroundCapEstimate` (§4) is a design assumption to validate with a real measurement … before it's trusted for real balance." | Now blocking: D15 rests on it. Fold into task-108's D16 sweep or file separately. |
| `entity-tiers.md`:284-307 | §5's handoffs, including ":306 whoever tunes `unit-types.json` `growth_source_weight` next". | Still open; `SYSTEMS.md` §1's taxonomy slot is unfilled and now also needs the C6 kill-payout column. |

### 3.5 Other design docs hit in passing

| file:line | What breaks | Required change |
|---|---|---|
| `scaling-curve.md`:11-13 | "**Extends:** `SYSTEMS.md` §2 (locks the enemy-population numbers, 250/450/700, as canon…), §7 (the Supply/Embers economy…)". | Both anchors move: O2 contests the curve, D5 deletes Embers. |
| `scaling-curve.md`:1, :26-30 | Title "the vertical slice's **three floors**" and §1's floor roster. | Floors → stages (D8/D7). |
| `encounter-budget.md`:8, :18 | ":8 which procgen room" / ":18 extends `GDD.md` §9 (procgen: every floor needs ≥1 arena, ≥1 decision event…)". | §9 retires (D8). |
| `encounter-budget.md`:310-320 | The peak-concurrency table and ":317-320 Even floor 3's worst-case peak (847) sits under **5%** of both measured ceilings … the 60fps-with-`Swarm.SimLOD.Stride 4` ceiling (**~34,000**)". | **C3** — the reassurance is computed against the untrusted divisor. |
| `encounter-budget.md`:383-386 | Handoff "To whoever builds the procgen room-graph generator". | No recipient (D8, task-025 dies). |
| `wave-scaling-three-act.md`:143-152 | The 34,000 ceiling and the **39.4% headroom arithmetic**, written out as a code block: `34,000 − 20,600 = 13,400; 13,400/34,000 = 39.41%`. | **C3's named target.** Replace with the honest range and defer the number to task-108. |
| `wave-scaling-three-act.md`:179-195 | ":179-180 Against the **no-LOD** ceiling (~21,000), headroom collapses to **400 entities (1.9%)** — this wave is barely inside that ceiling at all", and :186-195's choice of 20,000 "as the largest round figure that keeps both margins comfortable". | Under C3's ~13,000-20,000 range neither margin is comfortable; the late wave may be over the line. |
| `wave-scaling-three-act.md`:287 | Open item: "Whether procgen arena geometry (`encounter-budget.md` §3) can host a 20,000-population Arena at all". | Dead premise (D8). |
| `retinue-economy.md`:6-14, :49-59 | The task-101 reconciliation itself — `start_capacity` 60 → **120** (:14) and the `provision` 10 → **16 Embers** re-costing (:54). | **The 120 survives and is exactly what C10 says `wave-scaling.json`:23 missed.** The provision re-costing is superseded by D4/D5 (see §4). |
| `retinue-tuning-vanguard.md`:14, :22, :26 | Frames itself on ":14 §7 (the Supply/Embers economy, unchanged…)", on ":22 nothing resets the retinue between floors except losses taken and Embers spent", and ":26 Does not change: the Supply/Embers economy". | Both premises move (D5, D9). |
| `audio-minimal.md`:37, :121-123 | Cue table row ":37 Pickup — Kindling Ember" and §-body volumes (":123 Kindling Embers 4–19 per floor mean"). | Survives mechanically; the **name** re-implies a deleted currency (D5) and the "per floor" framing becomes per stage (D8). No new cue is authorised here — D6's fragments and D5's gold will need their own, which is a new pass, not a rename. |
| `hero-build-variety.md`:571-576 | "`squad-group-system.md` §4.2 already found the `MaxSquads=8` handle ceiling binds sooner once a SECOND type (Archers) exists (~730 total retinue…)" — flagged as the next stress test. | Superseded by D14; the stress test does not need running against a retired model. |
| `hero-build-variety.md`:582-585 | "**Growth-site tagging for chassis/origin-world** … belongs to the procgen encounter-rules scope area, a separate deliverable." | Two dead dependencies (D4, D8); the deliverable it defers to will not exist. |
| `CAMERA-SCALE.md`:25, :52, :80 / `CAMERA-SCALE-HANDOFF.md`:89 | `Emberkeep.Cam.Ortho`, `Emberkeep.UI.ViewCam`, `Emberkeep.UnitCamProj.Size*`, `Emberkeep.Cam.Scale`. | Stale CVar prefix (`Kindled.*` since 2026-07-31). Already task-117; listed so this sweep is complete. |

---

## 4. The backlog

### 4.1 Tasks that die

| Task | Current state | Why it dies |
|---|---|---|
| **task-025** — Spec the procgen room-graph generator | `status: proposed` (`task-025…md`:4), owns `docs/design/procgen-room-graph.md`, `docs/data/room-types.json` (:6); source `docs/RTS-VERTICAL-SLICE.md:102` (:11) | **D8 cuts dungeon crawling.** Its own *Why now* (:15-24) already says building the generator first means building it against three unknowns; the direction removes the generator's reason to exist entirely. Its deliverables at :26-30 (arena sizing rules, encounter-budget spend per room type, decision-event and risk-room placement) are all D8/D13 casualties. **Close as superseded, not rejected** — it was never wrong, its premise was. |
| **task-034** — Full loot & itemization system | `status: proposed` (:4), score `{feel: 3, risk: 1, cost: 4}` (:10) | **D12 names it directly: "Do NOT build the full task-034 itemisation system."** Note the framing inversion at :19-26: the task currently argues for parking because "building it now would create a second progression system competing with the one the game is actually about", citing Q7 and Q15. **D1 supersedes Q15 and D12 sanctions the second economy** — so the *reason* recorded in the file is now wrong even though the *verdict* (don't build it) is right. Rewrite the Why-now before closing, or the next sweep re-derives the old reasoning. |

### 4.2 Tasks superseded (kept in history, not deleted)

| Task | Current state | Why superseded |
|---|---|---|
| **task-097** — Measure whether the growth-site allocation is a real decision or theatre | `status: done` (:4), `decided: "2026-07-30 done"` (:26), epic `sim-irons-out-fun` (:12) | It measured, and made non-inert, an allocation panel **D4 deletes**. The tooling it shipped (`Scripts/sim/decisions.py`, `docs/sim/DECISIONS.md`, :8-9) is generic branch-divergence measurement and **survives** — point it at the shop (D12) instead. Only the growth-site subject is superseded. |
| **task-101** — Reconcile supply capacity with retinue size, and make provision bind | `status: done` (:4), `decided: "2026-07-30 done"` (:22), owns `economy.json`, `growth-sites.json`, `retinue-economy.md` (:7-10) | **Split verdict.** Its `start_capacity` 60 → 120 fix **survives and is load-bearing** — it is the reason C10 exists. Its `provision` 10 → 16 Ember re-costing is superseded by D4/D5 along with the lane it re-priced. Record both halves; do not blanket-supersede the task or the 120 goes with it. |

### 4.3 Tasks un-parked

| Task | Current state | Verdict |
|---|---|---|
| **task-105** — Wire per-wave retinue and composition into Spike1GameMode, and play the three-act curve | `status: parked` (:4), `decided: "2026-07-30 parked"` (:22), epic `three-act-waves` (:12), score `{feel: 3, risk: 3, cost: 3}` (:19) | **C12: UN-PARK.** Under this direction it is the single highest-value backlog item — the only task that puts a small starting army and a massive ending one on screen, which is the D1 incremental pitch made visible. Its evidence line (:13-18: "does early / mid / late read as three different fights, and does the late wave hold frame rate at its specced population") now answers two owner priorities at once (D1 and D17). **Note the index drift:** `INDEX.md`:45 shows the `three-act-waves` epic as "3/3 closed … complete" while 105 is parked — un-parking will correct that row on reindex. **STATUS: NOT YET APPLIED (re-verified 2026-07-31) — the file is still `status: parked`, `decided: "2026-07-30 parked"`.** Un-parking is the owner's verdict and this ledger does not edit backlog files; the C12 verdict is recorded here, not landed. |

### 4.4 Tasks approved

| Task | Current state | Verdict |
|---|---|---|
| **task-108** — Measure frame time at the three-act populations, and price the Elite/Boss actors | `status: proposed` (:4), agent `performance-director` (:5), score `{feel: 1, risk: 3, cost: 1}` (:18), currently ranked #7 in the audit queue (`INDEX.md`:25) | **D16: APPROVED**, with a scope addition. Its stated evidence (:12-17) already promises to replace "task-102's arithmetic headroom claim with a measurement" — that is **C3's fix**. It must **additionally include a RETINUE sweep past 120** (D16), because every measurement the project owns pins the friendly army at 100-120: `encounter-budget.json`:82-89 (RetinueN 40/60/90, 120 at the calibration row), `scaling-curve.json`:41-47 (TotalUnits 40-90), `retinue-vanguard.json`:49-51 (40 → 35 → 26 → 13). Without that sweep O3 stays unanswerable. Note its `feel: 1` score is exactly the ranking failure C11 describes. **STATUS: NOT YET APPLIED (re-verified 2026-07-31) — the file is still `status: proposed` with an empty `decided:`, and carries no RETINUE-sweep line.** The APPROVED verdict is the owner's and is recorded here, not landed; this ledger does not edit backlog files. |

### 4.5 `docs/backlog/INDEX.md` — C11's missing performance term

| file:line | What breaks | Required change |
|---|---|---|
| `INDEX.md`:6-9 | "Score is `(feel × risk × unblocks) ÷ cost`, where **`feel` is how much this changes the moment-to-moment feel or gameplay** (owner, 2026-07-28)." **No performance term.** | **C11.** D17 makes performance priority 1 of 1; a priority absent from the ranking rule gets silently re-derived away by the next sweep. Add a performance term. **STATUS: NOT YET APPLIED (re-verified 2026-07-31)** — `INDEX.md`:6 and `Scripts/backlog.py`:102 both still score without one. |
| `INDEX.md`:3-4 | `<!-- GENERATED by Scripts/backlog.py reindex — DO NOT HAND-EDIT. -->` | **C11 cannot be applied in INDEX.md.** The fix belongs in three places outside this file: `Scripts/backlog.py`:102 (`SCORE_KEYS = ["feel", "risk", "cost"]`), `Scripts/backlog.py`:202-206 (`total()`), and `Scripts/backlog.py`:809 (the header string INDEX.md:6 is generated from). All three verified 2026-07-31. |
| `docs/backlog/TEMPLATE.md` | The scoring rubric — `total = (feel × risk × unblocks) ÷ cost`, the 1/2/3 tables for `feel`/`risk`/`cost`, and the "`feel` is the primary axis (owner, 2026-07-28)" note. | Needs the new term's rubric row, and a note recording that `feel` primary (2026-07-28) now sits alongside performance (2026-07-31), the same dated-supersession style D1 requires of Q15. |
| `INDEX.md`:25 | task-108 sits at #7 with `1×3×1÷1 = 3.0` — the lowest-scoring row in the audit queue. | Illustrates C11 concretely: the project's stated priority-1 task ranks last because performance has no term. Re-score after the generator change. |
| `INDEX.md`:45 | `three-act-waves` epic reads "3/3 closed … complete" while task-105 is parked. | Corrects itself on reindex after C12. |

### 4.6 Backlog tasks that need a premise re-check before dispatch

Not verdicts — these carry premises the direction moved, and per the standing rule
("`waves`/`dispatch` check locks, never whether the prose is still true") each brief must
be re-verified before it is sent out.

| Task | Status | Stale premise |
|---|---|---|
| task-024 — Run structure, three floors and boss | `done` | Produced `run-structure.md`, which §3.2 above rewrites at fifteen separate anchors. |
| task-004 — Encounter budget table | `done` | Produced `encounter-budget.json`; its room-graph half dies with D8, its risk-room half with D13. |
| task-003 — Scaling curve, three floors | `done` | Locked 250/450/700 downstream; now contested (O2). |
| task-005 — Vanguard retinue tuning | `done` | Produced `retinue-vanguard.json`, whose rescue model D13 replaces. |
| task-006 — Loot v0, slice-scoped | `done` | Its no-second-economy rule is overturned (D12) — the v0 *scope* is ratified. |
| task-044 / task-046 / task-021 | `done` | All three build the MaxSquads=8 group model D14 supersedes (`SwarmSubsystem.h`:46-54). |
| task-036 — Runtime pacing director | `parked` (`decided: "2026-07-29 parked"`) | `depends-on: [4, 24]` — both of its blockers are being rewritten; the "encounter budget + floor structure to modulate" it waits for will not arrive in the shape it assumes. |
| task-026 — Highgates tileset | `proposed` | "Highgates" is a **superseded `WORLD.md` biome name** (C5). Re-name before dispatch, or it re-publishes retired canon. Its evidence line also cites "the locked value register" (C4 — full colour since 2026-07-28). |
| task-102 / task-103 | `done` | Produced the 39.4% headroom claim C3 retracts. |

---

## 5. Still open — file these, do not answer them

| ID | Question | Why it must not be invented |
|---|---|---|
| **O1** | **How fragments are gathered** — kill drops with auto-vacuum, placed caches, both, or rescue-only. | D6 specifies the *conversion* (enough fragments in one place → a healer unit) and the *boss variant* (fragments as modifiers, not bodies). It does not specify gathering. The owner did not answer. Filed in `GDD.md` at **two separate anchors, not one**: the **Q24 row** in the §12 open-questions log (:768, "How light fragments are gathered … Not to be filled in by inference"), and the **inline flag** inside §3's meta loop (:158-160, "*Open (2026-07-31): how fragments are gathered … (Q24)*"). The inline flag is a pointer, not the question's home — an earlier draft of this ledger cited only :158-159 for "Q24" and was wrong. Also filed in `SYSTEMS.md`'s open-questions table (O1). `loot-v0.json`:58-64 is where the drop plumbing would land; leave it empty. |
| **O2** | **Which wave curve wins** (C13). | `SYSTEMS.md` §2 locks 250/450/700 as a dated decision record; `wave-scaling.json`:22-26 proposes 120/400/20,000 with retinue 60→120→600, and its `$schema_note` (:2) claims the supersession unilaterally. `encounter-budget.json`, `scaling-curve.json` and `retinue-vanguard.json` all derive from the OLD curve. **Do NOT pick it unilaterally** — filed with the conflict spelled out. |
| **O3** | **End-of-run headcount target.** | Deliberately deferred to task-108's measurement (D16). The owner declined to name a number and named the lever instead (D17). |
| **O4** | **Gold's drop rate and sources.** | D12 establishes gold as the shop currency. Nothing states where it comes from. |
| **O5** | **Whether the leash survives** an army in the thousands commanded by type. | Already filed as `GDD.md` Q27 (:764) and flagged inline at :259. The owner has not been asked. D10's closure of the retinue-zero question means both the leash *and* the degrade-not-die rule now hang off it. Flag; do not resolve. |
| — | **Where hero ability nodes are earned** now that Embers and growth sites are gone. | The catalog (`upgrades.json`:30-43) stands; the purchase route (:29) does not. Gold at a shop, or a hero track off the same kill ratchet — owner not yet asked. |

---

## 6. Outside the sweep glob, but hit by these decisions

Listed so nothing is silently dropped. This ledger does not edit any of them.

- **`GDD.md` — ALREADY AMENDED; do not re-apply.** A concurrent session landed the full
  pass, and **kept editing it after this ledger was first written** — the line numbers below
  were **re-read 2026-07-31 at 22:27** and every anchor is quoted by its heading or opening
  words so a later reader can re-find it after the next drift. Verified anchors: the new
  genre spine at :83-95 with the 2026-07-21 spine kept verbatim as the superseded record at
  :97-112 (D1); run loop, session length and win/loss at :117-133 (D7/D9/D10), including
  retinue-zero-is-not-a-loss at :131-133; meta loop at :135-172 (D2/D3/D5/D6/D11) with the
  three-currency table at :150-154 and the growth-site-triangle consequence at :166-172;
  C6's kill-payout-by-tier at :144-147; pillar 4's palette supersession at :67-71 (C4);
  pillar 2 and 5's amendments at :60-63 and :72-77 (D2/D8); §9 "Stage Structure —
  **PROCEDURAL GENERATION RETIRED 2026-07-31**" at :488 (D8); the perf correction at :601,
  :606, :709-713 and Q19 at :761 (C1); C3's honest-range section "### The entity ceiling —
  an honest range, not the 34,000 figure" at :615-648, carrying the four in-editor sweeps
  and the ~13,000–20,000 working figure; and D17 + C2's sim-side spend order at
  "### Performance is priority 1 of 1" :650-677. O5 is filed as Q27 (:771, flagged inline at
  :263). **O1 is filed as Q24 in two places — the open-questions row at :768 and the inline
  meta-loop flag at :158-160; the inline flag is not Q24 itself** (see O1 in §5).
  **F1 IS filed** — corrected 2026-07-31: it landed as **Q28 in the §12 open-questions log**
  (:772, "Puzzle combat as a later direction … **RECORDED, not scoped**", status "⏸ Recorded
  2026-07-31 — future direction"). An earlier version of this bullet said F1 was *not* done
  because it looked only at the §11 scope table (:736, "The leash, rendered as light" /
  "Full itemisation behind the shops (task-034)"), which is not where it went. **Do not
  re-apply F1 and do not report it missing.**
- **`SYSTEMS.md` — AMENDED BY THIS PASS.** Cited by section from here on, since the line
  numbers moved. Applied: header drops `WORLD.md` as a companion doc (C5); §2 carries the
  two-curve OPEN CONFLICT marker (C13/O2); §3 amends Loot v0's storefront; §4 retires the
  growth site with the 2026-07-24 record kept verbatim; §5's breather becomes a shop stop;
  §6 retires the Recruit action and adds the command-by-type decision (D14); §7 is reshaped
  to Supply + three currencies with the Embers and triangle blocks retired-in-place, the
  kill ratchet, the shops, and the meta-progression slot closed (D3/D4/D5/C7); §8 retires
  the hero-node purchase route; a new open-questions table carries O1/O2/O4 and the hero-node
  route; seven new dated rows in the decision log (all seven re-counted 2026-07-31).
  **Added since this bullet was first written:** the data-file list carries a PENDING banner
  (§ header, "The three files below are not yet marked on disk … Treat every retirement here
  as **PENDING**"), which is `SYSTEMS.md` making the same recorded-vs-landed distinction this
  ledger now marks on C10/C11/C12 — `economy.json`, `upgrades.json` and `growth-sites.json`
  are retired *in this record only*, not on disk. Section numbers deliberately unchanged —
  `GDD.md`, `economy.json`, `upgrades.json` and `growth-sites.json` all cite them by number.
- **`docs/narrative/FLAME-FOUNDATION.md`**:132-136 — the "zero-input baseline *nearly wins*"
  claim **C9 inverts**; it loses, wave 3 by 4-13 brood (`economy.json`:48). Verified
  unedited. The same lines are where D13's rescue design lands the counterweight that doc
  calls "**Unsolved. This is the first thing to test.**"
- **`docs/narrative/README.md`**:4 — still names `WORLD.md` as source of truth alongside
  `GDD.md`/`CLASSES.md` (C5). *Corrected:* the hero list at :13-19 uses legacy **filenames**
  (`hallam.md`, `edda.md`, `merle.md`, `noll.md`) but already carries an explicit
  "role only, no proper names per 2026-07-11 decision" header — the filenames are the stale
  part, not the prose.
- **`Scripts/backlog.py`**:102, :202-206, :809 — the only place C11 can actually be
  implemented. All three verified.
- **`ELVTR/Source/ELVTR/Mass/SwarmSubsystem.h`**:46-54 — `MaxSquads = 8`,
  `SquadTargetSize = 20`, `TypeLegibilityCeiling = 80`; superseded by D14. :159-186 —
  `CreditKills` / `WaveKilledBySquad` / `RunKilledBySquad` / `ResetWaveKills`, the shipped
  kill economy D3/D4 build on. Note `ResetWaveKills`'s comment (:181-182) says "the run
  totals keep climbing… The run side resets in `ResetRunState`" — so the **lifetime**
  accumulator D3 needs does not exist yet, and adding one is a class-layout change (full
  editor-closed rebuild, not Live Coding). :155-158 also records that the passes were kept
  chunk-local specifically to survive a move to `ParallelForEachEntityChunk` — C2's item (2).
- **`docs/perf/one-camera-bench.md`** §5/§6/§8 — the source of the 21,000 and 34,000 figures
  C3 retracts as trustworthy.
- **`docs/perf/BUDGETS.md`** — carries the same two ceilings, cited by
  `encounter-budget.md`:317-320.
- **`docs/GATE1-FUN-PROTOTYPE.md`** — the measured 4-zero-input-run baseline every
  calibration in `docs/data/` anchors on; survives untouched and stays the calibration point.
