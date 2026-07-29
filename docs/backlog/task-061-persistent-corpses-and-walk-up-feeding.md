---
id: 061
title: Amend the feeding spec — corpses persist to end of round and any unit can walk up and eat
status: done
agent: gameplay-director
owns: ["docs/design/feeding-distraction.md", "docs/data/feeding.json", "docs/data/feeding.schema.md"]
resources: []
depends-on: [53]
epic: feeding-distraction
evidence: docs/design/feeding-distraction.md §5 rewritten for persistent corpses and walk-up claiming, with a re-run of the §8 density arithmetic showing what fraction of each side is feeding at wave-3 peak once bodies litter the field — plus the claim-radius and corpse-budget numbers task-054 needs, and docs/data/feeding.json updated to match.
score: {feel: 1, risk: 2, cost: 2}
source: user
teammate: persistent-corpses
decided: "2026-07-28 done"
---

## Why now

`task-053` delivered a spec that answered the corpse-persistence question with a firm
**no**. `docs/design/feeding-distraction.md` §5: *"There is no persistent corpse entity,
and there are no late joiners."* The killer set resolves at the death frame and is final;
the body is a render-only artifact that fades. §5 and the rejected-alternatives list both
turn that down on cost — walk-up eating needs a new per-frame spatial query to find corpses
with open slots, on top of everything already running.

The owner has now read that and overridden it: bodies persist until the round ends, and any
unit that comes near one can claim a slot and eat. That is a design decision, not a defect
in `task-053` — the spec settled the question with its reasoning shown, and the owner made a
different call with that reasoning in hand. But `task-054` must not be built against a §5
that the owner has already rejected, so the spec has to be amended before the build starts.

The real risk this task retires is not the query cost. It is §8. That section found a
scale-invariant **~31% retinue self-stall** from kill-triggered feeding alone, and it is the
reason `RetinueFeedChance ≈ 0.35` / `BroodFeedChance ≈ 0.85` exist at all. Walk-up eating on
a field littered with bodies is a strictly larger pull off the line — units get distracted by
deaths they had nothing to do with. Those two load-bearing numbers almost certainly do not
survive, and finding what replaces them is the work.

## Done when

`docs/design/feeding-distraction.md` is amended — not rewritten from scratch, and with the
superseded §5 reasoning preserved as the record of why the call changed — such that:

- **§5 is replaced.** Corpses are persistent entities that lie on the field until the round
  ends. Say what a corpse actually is at the sim level and how cheap it must be: it does not
  steer, fight, or collide, so state the minimum it carries — position, remaining slots, and
  whatever the renderer needs.
- **Walk-up claiming is specified with a number.** A claim radius, and what happens when
  several units are in range of several bodies. Be explicit about whether a unit actively
  seeks out corpses or only notices one it happens to be standing near — those are very
  different mechanics and only the second is cheap.
- **The three-slot cap is re-settled for late arrivals.** §2 resolved the killer set at the
  death frame precisely because there were no late joiners. With walk-up eating there are.
  Say whether the killers still get first claim, whether slots free up when a feeder is
  killed mid-meal, and whether a body can be eaten by both sides over its lifetime.
- **§8 is re-run.** This is the load-bearing part. Redo the density arithmetic with
  walk-up eating included and report what fraction of each side is feeding at wave-3 peak.
  If `RetinueFeedChance 0.35` / `BroodFeedChance 0.85` no longer hold the line, give the
  numbers that do. If the honest finding is that the owner's version stalls both armies into
  a stalemate, **say so plainly with the arithmetic** and propose the smallest change that
  keeps their intent — a claim cooldown, a corpse budget, a shorter feed on stale bodies.
  Do not quietly retune it into the old design.
- **Corpse population is bounded.** Bodies accumulate for a whole round at wave-3 kill
  rates. Give the expected peak count and a cap-and-cull rule for when it is exceeded, so
  `task-054` is not left inventing one.
- **What happens at round end** to bodies nobody ate, and what a mid-meal feeder does when
  the round ends under it.
- **The §11 handoff to `task-054` is updated** so the build task inherits a coherent spec,
  and the §13 decision log records the override with its date and reason.
- `docs/data/feeding.json` and its schema doc updated to match, still importing cleanly as
  a UE DataTable.

## Spawn prompt

```
You are executing task-061 in Emberkeep (C:\Projects\ELVTRGAME), on the flame-spotlight branch.

READ docs/design/feeding-distraction.md IN FULL FIRST. You are amending it, not replacing
it. It is good work and most of it stands.

WHAT CHANGED
That spec's §5 says: "There is no persistent corpse entity, and there are no late joiners."
The killer set resolves at the death frame and is final; the corpse is a render-only
artifact that fades. §5 and the rejected-alternatives list turn down persistent corpses on
cost grounds — walk-up eating needs a new per-frame spatial query to find bodies with open
slots.

The owner has read that reasoning and overridden it. Their words: "downed units persistent
state until the round is over. Those can get eaten by the enemy like our gameplay plan."

Asked directly to choose between (a) persistent bodies that anyone can walk up and eat,
(b) persistent bodies where only the killers eat, at the moment of the kill, and (c) build
persistence now and decide walk-up eating later — and shown that (a) was the expensive one
the spec had rejected — the owner chose (a) explicitly.

So: corpses persist to end of round, and any unit that comes near one can claim a slot and
eat it. Build the design around that. This is a change of design input, not a correction of
your predecessor's work — preserve the superseded §5 reasoning in the document as the record
of why the call changed, and log the override in §13 with today's date.

WHAT YOU OWN
  docs/design/feeding-distraction.md
  docs/data/feeding.json
  docs/data/feeding.schema.md
Nothing else. Do not touch GDD.md, SYSTEMS.md, CLASSES.md, or any source file.

THE PART THAT ACTUALLY MATTERS — §8
Your predecessor's §8 found a scale-invariant ~31% retinue self-stall from kill-triggered
feeding alone, and that finding is the entire reason RetinueFeedChance 0.35 and
BroodFeedChance 0.85 exist. The small army has the high kill rate, so it is the side that
pays.

Walk-up eating is strictly worse for that: units now get pulled off the line by deaths they
had nothing to do with, on a field that accumulates bodies for a whole round. Re-run the
arithmetic honestly. Report the fraction of each side feeding at wave-3 peak.

If the owner's version stalls both armies into a stalemate, SAY SO, with the numbers, and
propose the smallest change that preserves their intent — a claim cooldown, a corpse budget,
a decaying feed duration on stale bodies. Do not quietly retune the chances until the
mechanic collapses back into the old kill-triggered design. The owner is entitled to know
the cost of what they picked; they are not well served by a spec that agrees with them and
then fails in the build.

ALSO SETTLE, because task-054 will otherwise invent answers:
- What a corpse IS at the sim level, and the minimum it carries. It must not steer, fight,
  or collide.
- A claim RADIUS as a number, and whether units actively seek corpses or only notice one
  they are already standing near. Only the second is cheap; if you want the first, justify
  the query cost against §10's existing per-frame budget.
- Whether the killers still get first claim, whether slots free when a feeder dies mid-meal,
  and whether both sides can eat the same body over its lifetime.
- Expected peak corpse count at wave-3 kill rates, and a cap-and-cull rule for exceeding it.
- What happens at round end to uneaten bodies and to a feeder mid-meal.

CARRY FORWARD UNCHANGED unless your §8 re-run forces otherwise: the feed duration curve
clamp(MaxHP × 0.05, 1.5, 8.0)s (§3), full duration each rather than a shared race (§4),
feeders stay vulnerable/targetable/immobile/non-damaging (§1), the small self-heal on
completion (§6), and the symmetric mechanic with per-side fiction — "The Rite" vs "The Feed"
(§7). The MaxHP-as-armor-proxy note and its handoff to task-002 also stand.

CANON WARNINGS
- WORLD.md is SUPERSEDED by the 2026-07-22 narrative reset. Current canon is
  docs/narrative/FLAME-FOUNDATION.md. Do not cite WORLD.md.
- docs/perf/niagara-sprite-refactor.md §2 and §8.1 still carry a RETRACTED claim about GPU
  sim. Do not repeat it if you cite that file for render cost.

UPDATE THE §11 HANDOFF to task-054 so the build task inherits a coherent spec. Note in
passing that task-060 (blood particles) can key a distinct death burst off the corpse spawn
once corpses have positions — that is currently out of scope there purely because deaths are
counted but never positioned.

HAND BACK
A summary of what §5 now says, the re-run §8 numbers with the feed-chance values that
survive them, and — stated plainly — whether you believe the owner's walk-up version is
tactically sound at wave-3 density or whether it needs the guard rails you are proposing.
That verdict is the most useful thing you can give them.
```
