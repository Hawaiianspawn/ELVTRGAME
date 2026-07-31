# Kindled — encounter crew run report

**Floor 3** (density x2.5)

Generated 2026-07-30 by `crew/kindled_crew.py`.

**Negotiation rounds:** 3  ·  **Budget:** PASS (16.6ms / 16.6ms)  ·  **Readability:** CROWDED

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
| 1 | 341 | fodder ×341 | 341 | ring |
| 2 | 614 | fodder ×460, soldier ×154 | 922 | ring |
| 3 | 956 | fodder ×573, soldier ×325, elite ×57, boss ×1 | 2292 | arena_entrances |

## Budget verdict

- Peak concurrent entities: **1076** (wave 3 + retinue cap 120)
- Projected frame cost: **16.6ms** against a 16.6ms budget — 0.0ms headroom
- Basis: measured draw curve, single client, UnitShading=1

## Negotiation

What each agent said, in order. The architect only re-scales because the auditor sent the plan back with a reason.

```
canon-reader         read 2 canon source(s): docs/GATE1-FUN-PROTOTYPE.md, docs/perf/BUDGETS.md
canon-reader         waves=[250, 450, 700]  retinue_cap=120  perf curve has 6 measured points
roster-architect     derived 4 tiers over baseline 130HP/30DPS: fodder(58HP), soldier(130HP), elite(416HP), boss(2340HP)
--- negotiation round 1 ---
encounter-architect  composed W1: 625 bodies (625pts) | W2: 1125 bodies (1687pts) | W3: 1751 bodies (4155pts)
budget-auditor       REVISE - peak 1871 entities ~ 37.29ms, over budget by 20.69ms; only 956 bodies are affordable, sending back
--- negotiation round 2 ---
encounter-architect  revising: peak 1871 entities ~ 37.29ms, over the 16.6ms budget; re-scaling to x1.36 of the shipped wave sizes
encounter-architect  composed W1: 341 bodies (341pts) | W2: 614 bodies (922pts) | W3: 957 bodies (2293pts)
budget-auditor       REVISE - peak 1077 entities ~ 16.62ms, over budget by 0.02ms; only 956 bodies are affordable, sending back
--- negotiation round 3 ---
encounter-architect  revising: peak 1077 entities ~ 16.62ms, over the 16.6ms budget; re-scaling to x1.36 of the shipped wave sizes
encounter-architect  composed W1: 341 bodies (341pts) | W2: 614 bodies (922pts) | W3: 956 bodies (2292pts)
budget-auditor       PASS - peak 1076 entities ~ 16.60ms (0.00ms headroom)
--- audit and emit ---
readability-auditor  CROWDED - max 4 distinct enemy types in a wave against 2 free palette values
data-emitter         wrote 7 DataTable rows -> crew/out/encounters.json
data-emitter         wrote column contract -> crew/out/encounters.schema.md
```

## Blackboard traffic

Every read and write, attributed. Access is enforced: an agent that touches an undeclared key raises `ContractViolation`.

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
budget-auditor -> wrote  revision_directive
encounter-architect <- read   canon
encounter-architect <- read   tiers
encounter-architect <- read   revision_directive
encounter-architect -> wrote  plan
budget-auditor <- read   canon
budget-auditor <- read   plan
budget-auditor -> wrote  budget_verdict
budget-auditor -> wrote  revision_directive
encounter-architect <- read   canon
encounter-architect <- read   tiers
encounter-architect <- read   revision_directive
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
