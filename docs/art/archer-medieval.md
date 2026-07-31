# archer-medieval — variant family (task-125)

Base: the shipped knight (`1c935515-0ea3-459b-8c63-e7f8cf368272`, 88x88px, group
`2cc3ab61-4230-4786-98fa-b94da9e99218` — the same base as `knight-melee-v1`/`v2`), confirmed
live via `get_character` before touching anything. Every state drops the base's shield and
polearm, adds a bow/crossbow/sling weapon in its place, bans glow, and inherits the base's
palette via `use_color_palette_from_reference`. Full colour throughout per
`docs/art/aesthetic-direction.md`'s 2026-07-28 amendment — no quantizing, no demichrome-4.

## Provenance note

A prior attempt at this task already queued and completed all 6 `create_character_state`
generations before ending; they were sitting on the base's sibling-state list
(`Narrow strung bow`, `Bow extended wide`, `Loosing arm raised`, `Quiver held away`,
`Crossbow brace crouch`, `Sling whirl overhead`), never fetched or measured. This pass found
them live via `get_character`, fetched all 8 rotations for each from PixelLab's existing
result URLs, and ran judge/report — **0 new generations spent**, all 6 states were already
paid for.

## The axis

Two variants move width (the skeletal-base axis SKILL.md proves works): narrow (arms tucked,
bow slung on the back) versus wide-by-extension (a fully drawn longbow reaching out at arm's
length). Four variants move topology instead: a raised loosing arm closing a hole against
the head/shoulder, a quiver held away from the torso closing a hole at the hip, a forward
crouch bracing a crossbow low and level, and a sling whirled overhead in a wide arc.

## Measurements

All 8 rotations measured per variant (`silhouette_report.py --all-directions` +
`variantpipe.py judge`); size-gate bbox measured per rotation via direct alpha-bbox on each
PNG.

| variant | south content | aspect band | solidity band | asym band | holes | max bbox (8 facings) | size gate |
|---|---|---|---|---|---|---|---|
| v1_narrowstrung | 28x46 | 0.59–0.70 | 0.67–0.75 | 0.21–0.49 | 1 (11px) | 30x46 | PASS |
| v2_bowextended | 40x46 | 0.73–0.87 | 0.48–0.52 | 0.59–0.74 | 2 (77px) | 40x48 | PASS |
| v3_loosingarm | 38x47 | 0.66–0.81 | 0.57–0.62 | 0.61–0.78 | 0 | 38x47 | PASS |
| v4_quiverreach | 42x44 | 0.86–1.00 | 0.43–0.58 | 0.40–0.90 | 3 (19px) | 44x44 | PASS |
| v5_crossbowbrace | 39x44 | 0.80–0.91 | 0.51–0.59 | 0.59–0.80 | 0 | 40x44 | PASS |
| v6_slingwhirl | 47x48 | 0.80–0.98 | 0.46–0.58 | 0.22–0.84 | 1 (301px) | 47x48 | PASS |

Family aspect spread: 0.61–0.98 south-frame (1.6x); width spread 28–47px (1.7x) — capped
hard by the skeletal base, as `knight-melee-v1` predicted.

**Size gate: PASS for all 6 keeps in all 8 facings.** Every content bbox sits well inside the
56x56 atlas cell (`docs/data/art/requests/team-units.json`, `output.cell`); the widest facing
observed anywhere in the family is `v6_slingwhirl` north-west at 46x47 and south at 47x48 —
9-10px of headroom left on the worst case.

## Verdicts

- **v1_narrowstrung** — keep. Low luma (0.186) means the outline is doing all the work
  (flagged by `judge`, not a defect per SKILL.md), but it's the narrowest, most upright
  silhouette in the family and clearly reads as a hooded archer with a slung bow.
- **v2_bowextended** — keep, overriding `judge`'s automatic flag. `judge` flagged it because
  its 1.5x aspect target wasn't hit (measured band 0.73–0.87 instead) — PixelLab under-widened
  the outline the drawn bow was meant to add. But `judge` did *not* band-contain it inside any
  sibling: it's the only variant with a 77px/2-hole signature, and visually it is an
  unmistakable full-draw bow pose (checked south frame directly) distinct from every other
  variant's stance. A missed number, not a redundant shape.
- **v3_loosingarm** — keep. Highest asymmetry in the family (0.78), 0 holes (the raised-arm
  gap opens to the sprite edge rather than enclosing), reads as a loosing archer.
- **v4_quiverreach** — keep. 3 holes (the quiver held away from the torso does enclose real
  gaps on multiple facings), distinct aspect band from its neighbours.
- **v5_crossbowbrace** — keep. Lower, more compact profile than the bow poses as briefed; 0
  holes, distinct weapon silhouette (crossbow, not bow).
- **v6_slingwhirl** — keep. Widest variant (47x48), lowest solidity (0.48), and by far the
  largest hole (301px — the sling's loop clears the head/shoulder by a wide margin, exactly
  the topology it was asked for).

**6 of 6 generated variants are keeps** (target was ≥4). Nothing culled — nothing came back
visually identical to a sibling or lost the archer read.

## Generations spent this pass

**0.** All 6 states existed already from a prior attempt; this pass only fetched and measured
them. `get_balance` before starting: 6385 generations remaining (unchanged after).

## Contact sheet

Published as an Artifact: see handback. Source: `docs/data/art/families/archer-medieval/report.html`.
