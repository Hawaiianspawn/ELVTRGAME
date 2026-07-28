# encounters.json — schema

Produced by `crew/kindled_crew.py` for **Kindled**. The `rows` array is flat and
typed so it imports directly as an Unreal **DataTable**: one unique `Name` key
per row, scalar columns only, no nesting.

| Column | Type | Notes |
|---|---|---|
| `Name` | string | DataTable row key, `W<wave>_<tier>` |
| `Wave` | int | 1-indexed wave this row belongs to |
| `Tier` | string | `fodder` / `soldier` / `elite` / `boss` |
| `Count` | int | bodies of this tier spawned in the wave |
| `HP` | int | derived as a multiplier over the measured retinue baseline |
| `DPS` | int | as above |
| `BudgetCost` | int | encounter-budget points, for future spend-per-room tuning |
| `SpawnMode` | string | `ring` or `arena_entrances` |
| `BreatherAfterSeconds` | int | recovery window before the next wave; `0` on the last |

`_meta` carries provenance: which repo docs the numbers came from, and both
audit verdicts, so a row can always be traced back to the measurement that
justified it.

## Audit state at generation

- **Budget** — PASS: peak 821
  concurrent entities, projected 11.12ms against a
  16.6ms frame budget (measured draw curve, single client, UnitShading=1).
- **Readability** — CROWDED: 2 palette values
  free for enemies after the lit floor and the retinue take theirs.
