# Kindled — encounter crew run report

**Floor 1** (density x1.0)

Generated 2026-07-27 by `crew/kindled_crew.py`.

**Negotiation rounds:** 1  ·  **Budget:** PASS (11.12ms / 16.6ms)  ·  **Readability:** CROWDED

## Agents

| Agent | Role | Reads | Writes |
|---|---|---|---|
| `canon-reader` | Parses the project's shipped design and measured performance docs into a single fact block, so no downstream agent invents a number. | — | `canon` |
| `roster-architect` | Derives the entity tier ladder (fodder → soldier → elite → boss) as multipliers over the measured combat baseline, and prices each tier in encounter-budget points. | `canon` | `tiers` |
| `encounter-architect` | Composes the three escalating waves — how many of which tier, and the spawn cadence — and redesigns when an auditor sends the plan back. | `canon`, `revision_directive`, `tiers` | `plan` |
| `budget-auditor` | Projects the plan's peak concurrent entity count onto the measured frame-cost curve and returns PASS, or REVISE with a scale directive the architect must obey. | `canon`, `plan` | `budget_verdict`, `revision_directive` |
| `readability-auditor` | Checks each wave's simultaneous distinct silhouettes against the locked 4-value palette and flags any wave the player could not parse at speed. | `canon`, `plan` | `readability_verdict` |
| `data-emitter` | Flattens the approved plan into DataTable-shaped JSON rows for Unreal, writes the column schema, and refuses to emit anything that has not passed both audits. | `budget_verdict`, `canon`, `plan`, `readability_verdict`, `tiers` | `artifacts` |

## Encounter

| Wave | Bodies | Composition | Budget pts | Spawn |
|---|---|---|---|---|
| 1 | 250 | fodder ×250 | 250 | ring |
| 2 | 450 | fodder ×338, soldier ×112 | 674 | ring |
| 3 | 701 | fodder ×420, soldier ×238, elite ×42, boss ×1 | 1698 | arena_entrances |

## Budget verdict

- Peak concurrent entities: **821** (wave 3 + retinue cap 120)
- Projected frame cost: **11.12ms** against a 16.6ms budget — 5.48ms headroom
- Basis: measured draw curve, single client, UnitShading=1

## Agent transcript

```
canon-reader -> wrote  canon
roster-architect <- read   canon
roster-architect -> wrote  tiers
encounter-architect <- read   canon
encounter-architect <- read   tiers
encounter-architect -> wrote  plan
budget-auditor <- read   canon
budget-auditor <- read   plan
budget-auditor -> wrote  budget_verdict
readability-auditor <- read   canon
readability-auditor <- read   plan
readability-auditor -> wrote  readability_verdict
data-emitter <- read   canon
data-emitter <- read   tiers
data-emitter <- read   plan
data-emitter <- read   budget_verdict
data-emitter <- read   readability_verdict
data-emitter -> wrote  artifacts
```
