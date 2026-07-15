---
id: 005
title: The Pathfinder — reworked hero identity (orphan of the Fall, war-scarred, gender-unreadable)
status: pending
from: narrative-director
priority: high
faction: none/friendly
biome: all (hero crosses every biome; Gatecamp is home base)
class-ties: Pathfinder
spec:
---

## Subject

The Pathfinder's fiction changed substantially (owner decision, 2026-07-11) and the
existing sprite/portrait spec (`../art/merle.md`, delivered under brief-004, status
`done`) is now stale on every point of physical description. This brief supersedes
brief-004's Pathfinder portions and should feed a fresh pass on `../art/merle.md`.

Old: 22, Gatecamp-born trapper, royal-hunt kennel lineage, claw-scar through one
eyebrow. New: a teenager (age genuinely uncertain, "sixteen, seventeen"), an orphan
of the Fall who grew up feral in the collapse-lands outside the Gatecamp before
being taken in. War-scarred: a burn or blast-scar runs from scalp to jaw on one
side, old enough to have healed smooth, young enough that the bone underneath sits
subtly wrong. This scarring is the deliberate mechanism for the character's gender
being unreadable at a glance — not androgynous features, not an unstated design
choice, a specific described wound that obscures the facial landmarks a glance
normally sorts by. No proper name — referred to by role only, per the 2026-07-11
no-names decision (see `../narrative/merle.md`).

## Mood

Taut, watchful, feral-adjacent — carried over from the old read — but younger and
harder-used. Not androgynous-elegant; not mysterious-for-mystery's-sake. The scar
should read as *survived*, not as a stylistic flourish: matter-of-fact, a little
rough, the kind of old wound a person stops noticing on themself. Tragic-not-edgy
still applies — this is a war injury on a child who lived, not a horror image.

## Narrative excerpt

> War-scarred in a way that reads before it explains: a burn or blast-scar runs
> from scalp to jaw on one side, old enough to have gone smooth and pale, young
> enough that the brow and cheekbone underneath sit slightly wrong — reset by
> whoever had rough field skill and no time. It takes the read of the face with
> it: the jawline, the brow, the places a glance uses to sort a person into a
> box, are simply not there to sort. This is not androgyny and it is not a
> mystery being coyly withheld — it is a specific old wound, worn without
> apology […] "Pick whichever helps you remember to duck when I say duck."

## Readability needs

- Must still read, at gameplay zoom, as the class's signature silhouette: smallest,
  lightest, fastest hero; the only curve (bow arc) in the hero row. That silhouette
  language from the old spec (`../art/merle.md` §3) is **not** retired — only the
  face/age/backstory grounding is.
- The scar must be legible at portrait resolution as an old healed wound (not fresh
  gore, not a horror-register injury — see tone rule 1) and should visibly disrupt
  jaw/brow silhouette cues at portrait zoom, since that disruption is the actual
  in-fiction mechanism for the gender-unreadable read. At hero-sprite (gameplay)
  zoom it doesn't need to carry that same legibility burden — the portrait is where
  this detail has to do its work.
- Must not read as older than "teens/early 20s" despite the scarring and the
  hardened manner — avoid the visual shorthand that reads scarring as automatically
  aging a face.
- Hair grows in patches, cropped short and uneven on purpose to make the unevenness
  read as a choice rather than neglect.
- Still needs a valid answer to the existing spec's mark-temperature question
  (`../art/hero-palettes.md` §4, still open) — unaffected by this rework, flagging
  for continuity only.

## Source

`../narrative/merle.md` (full replacement fiction) · supersedes the "who/face"
grounding of `../art/merle.md` (sprite/portrait spec, brief-004) · `CLASSES.md` §3
(silhouette/kit language, unchanged) · companion brief-007 (the Pack retinue rework).
