# Spike 1 Results — "The Thousand"

**Goal:** 1,000+ Mass entities with follow/attack steering + Niagara sprite
rendering at ≥60fps (game thread <8ms). Stretch: 5,000.

**Machine:** _(CPU / GPU / RAM — fill in)_
**Build:** UE 5.8, Development Editor, date: _(fill in)_

## Numbers

| Entities (brood + 100 retinue) | Mode | Frame ms | Game ms | Draw ms | GPU ms | FPS | Verdict |
|---|---|---|---|---|---|---|---|
| 500 | PIE | | | | | | |
| 1,000 | PIE | | | | | | |
| 1,000 | Standalone | | | | | | |
| 2,000 | PIE | | | | | | |
| 5,000 | PIE | | | | | | |
| 5,000 | Standalone | | | | | | |
| 10,000 | PIE | | | | | | |

## Observations

- _(convergence behavior, hitches, grid hot spots, Niagara upload cost…)_

## Insights hotspots

- _(top 3 game-thread costs at 5k from Unreal Insights)_

## Verdict

- [ ] **GO** — architecture holds, proceed to Spike 2 (networking)
- [ ] **ADJUST** — holds after optimization pass (list changes)
- [ ] **KILL / RETHINK** — numbers don't support the design target

_(Copy the verdict + headline numbers into GDD.md §10 when done.)_
