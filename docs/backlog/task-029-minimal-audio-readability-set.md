---
id: 029
title: Spec the minimal audio set — hits, deaths, stance confirmations
status: proposed
agent: gameplay-director
owns: ["docs/design/audio-minimal.md"]
resources: []
depends-on: []
evidence: An audio spec framed as readability tooling, naming each cue, what it disambiguates, and how it survives 700 simultaneous sources.
score: {gate: 1, risk: 1, cost: 1}
source: docs/RTS-VERTICAL-SLICE.md:113
decided: ""
---

## Why now
The slice plan frames this precisely and the framing is the whole value:
*"readability tools, not polish"* (`docs/RTS-VERTICAL-SLICE.md:113`). Stance
confirmations in particular are a control-feedback problem, not an aesthetic one — the
player needs to know a stance took effect without reading the HUD, which is exactly the
question task-008 is going to ask about whether the hero feels like a commander.

Cheap, small, and it makes the Gate 1 feel questions easier to answer honestly. But it is
genuinely low priority against the design gaps, and the score says so.

## Done when
- Each cue named, with what it disambiguates — not "hit sound" but "which of the two
  things a player can't visually distinguish does this separate?"
- An answer for density: 700 units dying is 700 potential death cues. Voice limiting,
  pooling, or distance culling has to be part of the spec, or the cue set is unshippable
  at the density the game targets.
- Stance confirmations treated as control feedback with a latency expectation.
- Explicitly not polish. No music, no ambience, no mixing philosophy.

## Spawn prompt
```
You are the gameplay-director for Emberkeep (C:\Projects\ELVTRGAME).

Spec the minimal audio set per docs/RTS-VERTICAL-SLICE.md:113. That line frames the whole
task: hit/death and stance confirmations, as "readability tools, not polish". Hold that
framing — this is not an audio-design document, it is a legibility document that happens
to use sound.

Read: docs/RTS-VERTICAL-SLICE.md, docs/GATE1-FUN-PROTOTYPE.md (the four stances and their
input handling), and GDD.md §4.

For each cue, state what it DISAMBIGUATES — not "hit sound" but which two things a player
currently cannot tell apart that this separates.

Handle density explicitly: 700 units dying means 700 potential death cues. Voice limiting,
pooling, or distance culling must be part of the spec, or the set is unshippable at the
density this game targets. Design law 6 (readable danger at 500 units) applies to ears as
well as eyes.

Treat stance confirmations as control feedback with a stated latency expectation — the
player must know a stance took without reading the HUD.

Out of scope, do not write it: music, ambience, mixing philosophy, anything that is polish
rather than legibility.

Write ONLY docs/design/audio-minimal.md. Do not edit SYSTEMS.md, GDD.md, source, or content.
```
