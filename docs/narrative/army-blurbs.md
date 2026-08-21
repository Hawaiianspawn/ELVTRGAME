This is swap-in bark text for Kindled's four player army types, wired into Battle.gd's `_set_army` toast and the panel's idle line.
Extends `docs/narrative/FLAME-FOUNDATION.md` §1 (the flame as focus, the congregation gathered to it) — no canon changes.

Setting: a junior battle-mage captain's first command, pushing down a dark, ruined, unlived-in castle
hall toward a necromancer's green light. Voice: gruff, dry veterans who have done this before and are
sizing up the new captain — never jokey-cute.

## Veterans (`veteran`) — ability: Whirl

**Tag:** VETERANS

**Swap-in (said stepping up):**
- We've done this before, captain. Stay behind the spin.
- That green light isn't natural, captain. We've walked into worse.
- Steel moves before you think it. Just point us.

**Idle:** Veterans hold the center, waiting on your word.

## Halberdiers (`halberdier`) — ability: Sweep

**Tag:** HALBERDIERS

**Swap-in (said stepping up):**
- Line's up, captain. Give the word and we go through the rank.
- Long steel, long reach. Nothing stays standing after.
- We charge as one, captain, or we don't charge at all.

**Idle:** Halberdiers hold the flanks, reach longer than steel should.

## Hammer Knights (`hammer`) — ability: Slam

**Tag:** HAMMER KNIGHTS

**Swap-in (said stepping up):**
- Ground shakes when we swing, captain. Stand behind it.
- We don't dodge. We break what's standing in front of us.
- One good hit and the floor finishes the job.

**Idle:** Hammer Knights anchor the line, slow and unmovable.

## Veteran Ranged (`vet_ranged`) — ability: Volley

**Tag:** VETERAN RANGED

**Swap-in (said stepping up):**
- Arrows are nocked, captain. Just point us at the dark.
- Four volleys coming. Make the first one count.
- We shoot over your head — try not to duck into it.

**Idle:** Veteran Ranged loose arrows over the front line.

```json
{
  "veteran": {
    "tag": "VETERANS",
    "swap": [
      "We've done this before, captain. Stay behind the spin.",
      "That green light isn't natural, captain. We've walked into worse.",
      "Steel moves before you think it. Just point us."
    ],
    "idle": "Veterans hold the center, waiting on your word."
  },
  "halberdier": {
    "tag": "HALBERDIERS",
    "swap": [
      "Line's up, captain. Give the word and we go through the rank.",
      "Long steel, long reach. Nothing stays standing after.",
      "We charge as one, captain, or we don't charge at all."
    ],
    "idle": "Halberdiers hold the flanks, reach longer than steel should."
  },
  "hammer": {
    "tag": "HAMMER KNIGHTS",
    "swap": [
      "Ground shakes when we swing, captain. Stand behind it.",
      "We don't dodge. We break what's standing in front of us.",
      "One good hit and the floor finishes the job."
    ],
    "idle": "Hammer Knights anchor the line, slow and unmovable."
  },
  "vet_ranged": {
    "tag": "VETERAN RANGED",
    "swap": [
      "Arrows are nocked, captain. Just point us at the dark.",
      "Four volleys coming. Make the first one count.",
      "We shoot over your head - try not to duck into it."
    ],
    "idle": "Veteran Ranged loose arrows over the front line."
  }
}
```

## Canon proposals

None.
