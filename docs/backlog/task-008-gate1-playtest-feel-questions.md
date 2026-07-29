---
id: 008
title: Play Gate 1 and answer its five open feel questions
status: proposed
agent: claude
owns: ["docs/GATE1-FUN-PROTOTYPE.md"]
resources: ["unreal-editor"]
depends-on: []
evidence: The five questions in GATE1-FUN-PROTOTYPE.md §"open" answered from an actual play session, each with the tuning value tried and what it felt like.
score: {feel: 3, risk: 2, cost: 1}
source: docs/GATE1-FUN-PROTOTYPE.md:377
decided: ""
---

## Why now
Gate 1's stated pass condition is *"stances (with leash) feel good at 50–200 units; hero
feels like a commander, not a camera"* — a feel judgment, and the five questions that
decide it are still unanswered at lines 377–386. The prototype is built and playable.
Nobody has sat down and answered whether it is fun.

Every downstream tuning task (002–006) is guessing at targets until someone says what
the current build actually feels like. This is one session with the game and it unblocks
judgment across the whole design board.

## Done when
Each of the five is answered from play, not from reading code:
- Does **Hold** feel like a real tool at `LeashRadius = 2000uu`?
- Does the leash *break* read clearly, or does the army feel like it wanders off?
- Is **Charge** distinct from Follow in practice, or does auto-engage collapse them?
- Does the hero feel like a commander at 55 DPS — or like a camera with a sword?
- Is 120 retinue the right count to *read* on screen at this camera height?

Each answer records the value tried and the felt result. Where a different value felt
better, say the number. This is an owner-facing report — the answers are input to
tuning, not permission to retune source.

## Spawn prompt
```
You are playtesting Gate 1 of Emberkeep (C:\Projects\ELVTRGAME) and reporting how it feels.

Read docs/GATE1-FUN-PROTOTYPE.md first — it documents controls, run structure, and the
five open questions at lines 377-386 that you are here to answer.

PIE on Content/Spike1/L_Spike1. Controls are polled directly in SpikeHeroPawn.cpp:
WASD move, 1/2/3/4 = Follow/Charge/Hold/Rally, R restart. Console: Swarm.Stance <name>.
Tuning CVars are exposed — see the /cvars skill and Saved/SwarmExecOnPlay.txt.

You own the Unreal editor for this task; no other task may drive it concurrently.

Play several runs. Vary the values the questions name (LeashRadius, hero DPS, retinue
count) using CVars, not source edits. Then answer all five questions in
docs/GATE1-FUN-PROTOTYPE.md, each with: the value tried, what it felt like, and a
recommended value if the default felt wrong.

Write ONLY docs/GATE1-FUN-PROTOTYPE.md. Do not edit ELVTR/Source or ELVTR/Content — if
a change is needed, recommend it. Report honestly: "it did not feel good" is a valid and
useful answer, and a gate that fails should be reported as failed.
```
