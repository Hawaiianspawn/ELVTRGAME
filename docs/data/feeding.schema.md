# feeding.json — schema

Companion data file to `docs/design/feeding-distraction.md`. Unlike
`unit-types.json`/`squads.json` (nested per-instance config, explicitly *not*
flat DataTables per their own schema notes), this file is a **flat, row-per-CVar
table** — one row per tuning dial, a single typed `Value` column per row — per
`task-053`'s explicit requirement that it "import cleanly as a UE DataTable."

**v0.1 (2026-07-27):** new file, introduced alongside the feeding/distraction
mechanic spec.

## Row shape (`rows[]`)

One row per `Swarm.Feeding.*` CVar. Every row shares the same struct shape so
the whole table imports as one UE DataTable row struct.

| Field | Type | Notes |
|---|---|---|
| `Name` | string (row key) | The literal CVar name, e.g. `Swarm.Feeding.ChompRate`. Matches the `Swarm.Feeding.*` naming group task-054 should register (mirrors the existing `Swarm.BroodFormation.*` nesting precedent in `SwarmCommands.cpp`). |
| `ValueType` | enum string | `Bool` \| `Int` \| `Float` — documents how to interpret `Value`, since the table stores everything in one numeric column for DataTable-import simplicity. `Bool` is 0/1, `Int` is an integer-valued float. |
| `Value` | float | The tuned default. Always numeric regardless of `ValueType` (see above). |
| `Team` | enum string | `Any` \| `Retinue` \| `Brood` \| `Hero` — which side the dial applies to. Most dials are `Any` (shared mechanics); the feed-chance dials are deliberately split per-team (spec §8/§9) even though the underlying rule is symmetric. |
| `Description` | string | One-line rationale, cross-referenced to the spec section that derives the value. Not consumed by code — a design-intent record, same role `unit-types.json`'s `stance_reflavor`/`note` fields play. |

## Values not yet in this table

Per-tier `MaxHP` inputs to the `FeedDuration()` formula (`Swarm.RetinueMaxHP`,
`Swarm.BroodMaxHP`, and any future per-type/tier values from `task-002`'s armor
work or `unit-types.json`'s typed-unit model) are **not duplicated here** —
they're read live from the existing combat CVars/data files at the point
`FeedDuration()` is evaluated, so there is exactly one source of truth for each
number. This table only owns the feeding-specific dials layered on top.
