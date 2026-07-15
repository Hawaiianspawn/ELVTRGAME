# Art Brief Template

Copy everything below the `═══` line into `brief-<id>-<slug>.md`
(e.g. `brief-001-warden-captain-bree.md`) and fill it in. IDs are zero-padded
three digits, incrementing from the highest existing `brief-*.md`; start at
`001` if none exist. One brief per *subject* — one thing needing its own
sprite/asset (a character and the site she stands in are two subjects).

Written by: `narrative-director` (or the user). Consumed by: `pixel-art-director`.
The brief says **what it is and what it must communicate**; the art director owns
everything pixel-level (palettes, hex values, sheet layout). Palette hints are
moods only — never hex values.

═══════════════════════════════════════════════════════════════════════════

---
id: 001
title: <short name of the visual subject>
status: pending        # pending | in-progress | done | blocked
from: narrative-director
priority: normal       # low | normal | high
faction: none          # Still Legion | the Quiet | the Unwitnessed | none/friendly
biome: Highgates       # Highgates | Sunken Works | Vesper Halls | Gatecamp
class-ties: none       # Vanguard | Relickeeper | Pathfinder | Lampbearer | none
spec:                  # filled by pixel-art-director when done: ../art/<file>.md
---

## Subject
One paragraph: what this is in the fiction.

## Mood
The feeling it must carry (tragic / administrative-cold / lamp-warm / alien-wrong…).

## Narrative excerpt
A short quote from the narrative deliverable that anchors the visual.

## Readability needs
Gameplay-terms requirements, e.g.:
- must read as rescueable (not hostile) inside a horde
- must show a damaged state at half health
- must be distinguishable from <other unit> at gameplay zoom

## Source
Link to the narrative doc this came from: `../narrative/<file>.md`
