# task-180: relic threshold retune

Kill economy is `Game.gain_magic` at `godot/scripts/Game.gd:134`: every kill's `magic` value
(from `units.json`) is multiplied by `1 + relic_bonus("gather")` before it lands in both `magic`
and `magic_ever`. Only two of the five relics carry a `gather` field: `ember_ring` (+0.25) and
`witch_lamp` (+0.50) — `iron_hymn`, `echo_bell`, `green_tooth` don't touch income. A hall only
ends when `Battle.gd:646-647` finds the spawn queue empty and no enemy alive, so every listed
enemy is assumed to die (100%) for this arithmetic. Hero-death retries (`Battle.gd:641-645`)
restore `Game.magic` to the hall's checkpoint but never roll back `magic_ever`, so a run with
retries reaches these thresholds *sooner* in wall-clock terms than a clean run — noted here, not
correctable from data alone (see task-179's 1355 AFTER reading, which was already mid-Hall2, not
a Hall-1 number).

## Raw per-hall income (no relics owned — `count x magic` per enemy, `units.json`)

| Hall | Enemies (count x magic) | Hall total |
|---|---|---|
| 1 | ooze 56x8=448 | **448** |
| 2 | undead 32x9=288, mace_undead 16x10=160, archer_undead 24x8=192, ooze 24x8=192 | **832** |
| 3 | undead 48x9=432, mace_undead 24x10=240, staff_undead 24x9=216, archer_undead 16x8=128, bow_undead 16x8=128 | **1144** |
| 4 | armored 72x19=1368, undead 20x9=180, staff_undead 10x9=90, bow_undead 10x8=80 | **1718** |

Raw (no-relic) cumulative lifetime magic at hall boundaries:

| Hall | start | end |
|---|---|---|
| 1 | 0 | 448 |
| 2 | 448 | 1280 |
| 3 | 1280 | 2424 |
| 4 | 2424 | 4142 |

## Placing the five thresholds

`ember_ring` is the only relic active before any of the others can unlock, so its "at" is read
straight off the raw curve: middle of Hall 1 = 448 / 2 = 224 -> round to tens -> **220**.

Once `ember_ring` is owned, every further kill is worth x1.25, so actual `magic_ever` (call it
`M`) outruns the raw curve from R=220 onward:

```
M(R) = 220 + (R - 220) * 1.25          for R >= 220, before witch_lamp
```

`iron_hymn`, `echo_bell`, `green_tooth` carry no `gather`, so they don't change the multiplier —
their thresholds are just points read off that same M(R) line. Target raw positions spread
through Halls 2-3 (raw range 448-2424, width 1976) at the quartiles:

- R2 = 448 + 1976*0.25 = 942 (in Hall 2)
- R3 = 448 + 1976*0.50 = 1436 (in Hall 3)
- R4 = 448 + 1976*0.75 = 1930 (in Hall 3)

M(942) = 220 + (942-220)*1.25 = 220 + 902.5 = 1122.5 -> **1120** (`iron_hymn`)
M(1436) = 220 + (1436-220)*1.25 = 220 + 1520 = 1740 -> **1740** (`echo_bell`)
M(1930) = 220 + (1930-220)*1.25 = 220 + 2137.5 = 2357.5 -> **2360** (`green_tooth`)

`witch_lamp` (first half of Hall 4, raw range 2424-4142): target raw position at 25% in =
2424 + 1718*0.25 = 2854.

M(2854) = 220 + (2854-220)*1.25 = 220 + 3292.5 = 3512.5 -> **3510** (`witch_lamp`)

## Chosen values

| relic | at | rationale |
|---|---|---|
| ember_ring | 220 | raw midpoint of Hall 1 (224), rounded |
| iron_hymn | 1120 | M(R) at the first Hall2/3 quartile (R=942, inside Hall 2) |
| echo_bell | 1740 | M(R) at the Hall2/3 midpoint (R=1436, inside Hall 3) |
| green_tooth | 2360 | M(R) at the third Hall2/3 quartile (R=1930, inside Hall 3) |
| witch_lamp | 3510 | M(R) at 25% into Hall 4 (R=2854), first half of the hall |

## Sanity check: actual (bonus-adjusted) magic_ever at hall boundaries

Using `M(R)` through Hall 3, then switching to the x1.75 multiplier (`ember_ring` + `witch_lamp`)
once R passes 2854:

- end Hall 1 (R=448): M = 220 + (448-220)*1.25 = **505**
- end Hall 2 (R=1280): M = 220 + (1280-220)*1.25 = **1545**
- end Hall 3 (R=2424): M = 220 + (2424-220)*1.25 = **2975**
- end Hall 4 (R=4142): M = 3510 + (4142-2854)*1.75 = 3510 + 2254 = **5764**

All five thresholds land inside the hall they were targeted for once the multiplier is applied
(220 in Hall1's 0-505; 1120 in Hall2's 505-1545; 1740 and 2360 both in Hall3's 1545-2975; 3510 in
Hall4's first half, 2975-4369.5 of 2975-5764). By the end of Hall 2 (M=1545), only `ember_ring`
(220) and `iron_hymn` (1120) have unlocked — `echo_bell` (1740), `green_tooth` (2360) and
`witch_lamp` (3510) are all still ahead, satisfying the "no green_tooth, no witch_lamp by 30s
probe (mid-Hall2)" bar with margin.
