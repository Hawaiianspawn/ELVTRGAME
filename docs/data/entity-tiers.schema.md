# `entity-tiers.json` — schema

Companion data file for `docs/design/entity-tiers.md`. One row (`tiers[]` entry)
per enemy entity type. Friendly retinue tiers (Freed/Militia/Veteran/Bannerman)
are a separate table (`docs/data/upgrades.json`) — not duplicated here.

## `design_constants`

| Field | Type | Meaning |
|---|---|---|
| `swing_interval_shared` | float, seconds | Shared cadence for Fodder/Soldier tiers (matches `Swarm.SwingInterval` default). Elite/Titan/Boss override it per-row. |
| `armor_chip_floor` | float, damage | Minimum damage a landed blow always deals regardless of Armor. Proposed CVar `Swarm.ArmorChipFloor`. |
| `reference_blow_source` / `reference_blow_value` | string / float | Which attacker (and its computed `Blow = DPS x SwingInterval`) the `EffectiveHP` conversion in `entity-tiers.md` §2.3 is expressed against. |

## `tiers[]` row columns

| Column | Type | Units / range | Meaning |
|---|---|---|---|
| `Name` | string | unique, snake_case | DataTable row key. `brood_*` prefix — working/mechanical ID, not a lore name (narrative hasn't named the faction; see `entity-tiers.md` §6 Narrative requests). |
| `DisplayName` | string | — | Human-readable label for editor/debug UI. Still a working name. |
| `WorkingNameOnly` | bool | — | Always `true` in this file. A flag so nothing downstream (UI, VO hooks) mistakes these for shippable names before the narrative-director's pass. |
| `Tier` | enum | `Fodder`\|`Soldier`\|`Elite`\|`Titan`\|`Boss` | GDD §10 / SYSTEMS.md §1 taxonomy. |
| `ActorType` | enum | `MassEntity`\|`PromotedActor` | Fodder/Soldier are Mass Entity (shared archetype, no per-unit uniqueness — Design Law 5). Elite/Titan/Boss are promoted Actors. |
| `Role` | string | free text, stable vocabulary | Behavior archetype: `melee_swarm`, `melee_line`, `ranged_line`, `anti_swarm_elite`, `screen_wall`, `boss_baseline`. |
| `MaxHP` | float | HP | Same field as `FSwarmHealthFragment::MaxHP` (Fodder/Soldier) or its promoted-Actor equivalent. |
| `Armor` | float | flat damage, ≥0 | **Not a percentage.** Subtracted from every incoming blow before the chip floor is applied: `EffectiveBlow = max(AttackerBlow - Armor, ArmorChipFloor)`. See `entity-tiers.md` §2 for why flat-subtraction (not %) is the load-bearing choice. `0` = no armor stat in play, mathematically identical to today's shipped (armor-less) model. |
| `DPS` | float | damage/second | Same meaning as the shipped `Swarm.*DPS` CVars — steady-state output; `Blow = DPS x SwingInterval` is derived, not stored, to avoid a second source of truth. |
| `SwingInterval` | float | seconds | Own attack cadence. Fodder/Soldier share the design constant (0.9s); Elite/Titan/Boss get longer, telegraph-legible intervals. Does **not** change the entity's own steady-state DPS — only how it's parcelled (same identity `GATE1-FUN-PROTOTYPE.md` §3b established). |
| `EngageRange` | float | uu | Melee reach (`Swarm.MeleeRange`-equivalent) or ranged engage distance, per `Role`. |
| `MinEngageRange` | float | uu | Ranged-only "won't shoot into its own scrum" band floor (`unit-types.json` pattern). `0` for melee roles. |
| `TargetsPerHit` | int | 1-8 | Cleave — how many victims one blow spreads across (`Swarm.*TargetsPerHit` pattern). |
| `MoveSpeedScale` | float | multiplier of `Swarm.BroodSpeed` | `1.0` = base Brood speed. |
| `SurroundCapEstimate` | int or null | attacker count | Elite/Titan/Boss only — a **Fermi estimate**, not measured, of how many melee attackers can be in simultaneous physical contact with this entity (`entity-tiers.md` §4). `null` for Fodder/Soldier, where the question doesn't apply (they aren't single large-collision targets). |
| `SurroundCapRange` | string or absent | — | Sensitivity range around the point estimate, stated plainly as unmeasured. |
| `TelegraphWindup` | string | free text | Design-law-6 readability note: what makes the entity's strike legible at horde scale. Elite/Titan/Boss carry the load here; Fodder/Soldier explicitly carry none (their danger is numbers, not a single blow). |
| `Notes` | string | free text | Rationale, provenance (shipped/new), and explicit scope flags (e.g. boss phase design is out of scope for this file). |

## Conventions carried over from sibling tables

- All values are **PROTOTYPE DIALS**, same status as every other file in `docs/data/` — re-tune in play, not committed balance.
- Flat, typed columns, no nesting inside a row — importable as a single UE DataTable keyed on `Name`.
- A future third source of `Armor` (a friendly-side armored identity, e.g. Relickeeper's Bulwark) should reuse this exact column rather than inventing a second stat — see `entity-tiers.md`'s handoff section.
