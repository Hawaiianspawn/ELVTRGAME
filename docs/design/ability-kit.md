# The ability kit — options for Q23 and Q26

**What this is:** drafted options for an owner call on Q23 (what the squad-channelled
ability kit is) and Q26 (how orders are issued to it) — `docs/OPEN-DECISIONS.md`. No
option is ticked here; that is the owner's call, not this document's.
**Extends:** `docs/OPEN-DECISIONS.md` Q23/Q26, `docs/design/castle-layout.md` §6.3–§6.4,
`docs/PREFLIGHT.md` §3 Q26. Source material is `CLASSES.md`'s four hero kits, per
Q29 = A.

---

## 1. The verbs, mechanically

Q13 = C ("your abilities act *through* the squad — focus fire, reposition, screen,
raise") is a sketch, not a spec. Below is every verb the four class hero kits already
define, transcribed from `CLASSES.md` as written, not paraphrased. Each is tagged
against the sketch's four words where it fits, and flagged where it doesn't.

### Maps onto the Q13 sketch

| Sketch word | Source verb | Mechanical text (`CLASSES.md`) |
|---|---|---|
| **focus fire** | **Mark Quarry** (Pathfinder, signature) | "tag an enemy: the entire pack focus-fires it, and it's revealed through walls. The class's core loop — you *choose what dies*." Targets one enemy. No cost or cooldown stated. |
| **reposition** | **Shield Wall** (Vanguard, Hold-stance reflavor) | "anchors the line to the ground it was called on — the formation stops tracking the hero and holds that spot... a **positioning tool, not a barricade**... soldiers fight whatever comes within their own reach, but nothing about Hold makes brood target the wall over the flame." Targets a ground position; affects the whole retinue currently in Hold. |
| **screen** | **Ward Circle** (Relickeeper, field) | "inscribe a zone: allies inside take reduced damage and Liberated/pack units won't rout. The party's 'hold this ground' button." Targets an area. |
| **raise** | **Kindle** (Lampbearer, signature) | "channel onto a player or unit: heal over time; *overheal* becomes a temporary light-shield. On a downed player, Kindle revives faster than the standard pick-up." Targets one ally. Channeled (implies a duration the caster is committed to). |

### In the source kits, not in the sketch

These exist in `CLASSES.md` and are legitimate candidates for the kit, but Q13 = C's
four words don't cover them. Listed so a shape isn't chosen against an incomplete
verb list.

| Verb | Source | Mechanical text |
|---|---|---|
| **Banner Slam** | Vanguard, signature | "plant the banner; retinue in radius gains attack speed and fights to the death (no retreat/flee behavior) while it stands." Buff + morale-lock, radius around a planted point. |
| **The Muster** | Vanguard, ultimate | "every friendly unit on screen rallies to the banner and delivers one synchronized charge." Screen-wide, one-shot. |
| **Shield Rush** | Vanguard, mobility | "short charge that knocks enemies aside and *bodyblocks* for the hero's escort — pushes a lane open for the retinue behind." Player-body-relative, not squad-targeted. |
| **Mend Stone** | Relickeeper, signature | "channel onto an Awakened unit: repair its cracks; *over-mending* temporarily upgrades it (gilded state). Guardians only." Single-target repair/upgrade, restricted to one archetype. |
| **Remembrance** | Relickeeper, ultimate | "the floor briefly *remembers what it was* — all Awakened units surge to full power, broken terrain features re-form as cover, enemies of the dungeon's old enemy faction are staggered." |
| **Snare Line** | Pathfinder, utility | "place a tripline between two points; enemies crossing it are rooted." Battlefield-control, not squad-targeted at all. |
| **The Hunt Is Called** | Pathfinder, ultimate | "every marked enemy on screen is simultaneously attacked by spectral echoes of the pack; each kill during the Hunt refreshes it once." The closest thing in the source material to a burst-damage verb (see §4). |
| **Sanctuary** | Lampbearer, field | "plant the lamp: a standing circle of daylight. Allies inside regenerate... enemies of the dark won't cross the light without being burned." Overlaps functionally with Ward Circle (damage reduction vs. regen — the kits themselves note this as a deliberate stack, not a duplicate). |
| **Daybreak** | Lampbearer, ultimate | "the floor is lit as it was in daylight — full map vision, party healed in waves, dark-aligned enemies feared, every Guided soul blazes to double strength." |

**Two structural things worth naming before either question can be answered honestly:**

1. **The four signature verbs target three different things.** Mark Quarry targets
   an enemy. Kindle targets an ally. Shield Wall and Ward Circle target a ground
   position. Any control scheme (Q26) has to resolve targeting for all three kinds
   with one input model — this is not a detail, it constrains every option in §3.
2. **None of the transcribed verbs is a pure burst-damage effect.** Mark Quarry
   sustains focus fire; The Hunt Is Called is the closest analogue (simultaneous
   strikes on every marked enemy, refreshing on kill) but is gated behind an
   ultimate and behind the Pathfinder's own mark system, not the boss-mark system
   in `castle-layout.md` §6.1. This resurfaces as a concrete gap in §4.

---

## 2. Q23's three shapes, against those verbs

### A — A fixed kit on the player

Four or five of the verbs above (most naturally: focus fire, reposition, screen,
raise, plus optionally rally) live on the player and apply to *whichever soldiers are
in range* — the register's own phrasing. The seven are targets of the kit, not
holders of it.

- **Verbs it implies:** the sketch's four, cast as player-owned spells rather than
  soldier-owned actions. Banner Slam/Muster-style buffs fit naturally (they are
  already radius-around-a-point effects with no soldier-identity requirement).
  Mend Stone does not fit — it is explicitly restricted to one archetype
  (Awakened/guardian), which under A the player can't guarantee is nearby.
- **What it does to the seven's distinctness:** distinctness lives entirely in
  passive stats and silhouette — what the player can *command* is uniform across
  all seven regardless of archetype. A guardian and a hunter receive the same
  "screen" or "raise" if they're in range; nothing about the verb changes with who
  it lands on.
- **Stated cost (register):** "the seven risk becoming interchangeable again, which
  is the failure Q13 = C was chosen to avoid."

### B — The kit lives in the soldiers; the player spends them

Each of the seven carries one verb as their unique action — plausibly one
archetype-to-verb binding: line/Vanguard → Shield Wall (reposition), guardian/
Relickeeper → Ward Circle (screen), hunter/Pathfinder → Mark Quarry (focus fire),
light/Guided → Kindle (raise). The player's own kit is small or empty; the player's
role is choosing *who acts and when*, not casting.

- **Verbs it implies:** the same sketch verbs, but each is now scoped to a specific
  soldier (or a small set of soldiers sharing an archetype) rather than available on
  demand from the player. Spending the verb plausibly costs that soldier's
  availability for something else (a cooldown, or removing them from the line while
  they act) — not specified anywhere in canon; would need its own design pass.
- **What it does to the seven's distinctness:** maximal. Distinctness is the entire
  point of this shape — each of the seven is mechanically, not just cosmetically,
  different.
- **Stated cost (register):** "closest to an RTS with seven units; needs each
  soldier to be genuinely distinct or the choice is hollow."

### C — Both; a small player kit modifies what soldiers do

Soldiers carry the base verb as in B. The player's own kit is a layer of amplifiers
or redirects on top — e.g., a player action that extends a currently-active Ward
Circle's radius, or retargets an in-progress Mark Quarry to a different enemy.

- **Verbs it implies:** whatever B implies, plus a second, smaller vocabulary of
  modifiers that only do something when a base verb is already active or
  addressable. This makes player agency conditional on squad state in a way A and B
  are not — there's nothing to amplify if no soldier's verb is currently running.
- **What it does to the seven's distinctness:** inherits B's distinctness at the
  soldier layer; adds a second axis of player skill on top of it.
- **Stated cost (register):** "most design surface, most likely to be legible only
  to the designer."

---

## 3. Q26's four schemes crossed with them

- **A — Direct target.** Click a thing; the appropriate soldier (or verb) acts on
  it.
- **B — Select then order.** RTS-conventional: select a unit, then issue it an
  order.
- **C — Contextual single button.** One input; meaning resolved by what's under the
  cursor and who's available.
- **D — Radial / verb wheel.** Explicit verb selection, then a target.

`PREFLIGHT.md` §3 already states: "a kit that lives in the soldiers (Q23 = B) wants
A or C; a fixed player kit (Q23 = A) wants D." Working through each pairing against
§1's actual verb list:

| | A. Direct target | B. Select-then-order | C. Contextual button | D. Verb wheel |
|---|---|---|---|---|
| **Q23 = A** (fixed kit) | Weak. A's own verbs target three different kinds of thing (enemy / ally / ground, §1). One click can't disambiguate "cast Mark Quarry on this enemy" from "cast Kindle on this ally" from "cast Ward Circle on this ground" without a verb already chosen. | Weak/redundant. There is no soldier-level unit for the player to *select* under A — the ability is the player's, not a specific soldier's, so this scheme's core action (pick who acts) has nothing to bind to. | Marginal — a single button could work if the game infers "heal if hovering a wounded ally, else screen," but that guesses at intent across three target types rather than the player choosing. | **Coherent.** The verb is chosen first (wheel), then the target is picked with whatever type that verb needs. This is the natural fit — noted in `PREFLIGHT.md`. |
| **Q23 = B** (kit in soldiers) | **Coherent** — noted in `PREFLIGHT.md`. Click an enemy/location; the soldier whose verb answers that target acts. | **Also coherent, and arguably the most literal fit** — see the finding below. Select soldier N, then order them to act; this is B's own "each of the seven is a verb" premise made into an input scheme directly, rather than inferred through a click. | **Coherent** — noted in `PREFLIGHT.md`. One button, resolved by cursor + soldier availability. | Weak. Each soldier under B has exactly one verb, so a wheel with one live option per soldier isn't offering a choice — it's a soldier-picker wearing a verb-picker's UI. |
| **Q23 = C** (both) | Needs two-stage input regardless of column: address the soldier/verb being modified, then apply the modifier. No single-scheme cell here is a clean fit — C structurally wants a soldier-addressing step (B- or C-shaped) *and* a modifier-application step (A- or D-shaped) layered together. | Partial — covers the soldier-addressing half. | Partial — covers the soldier-addressing half. | Partial — could cover the modifier-application half once a base verb is already targeted. |

**A finding, not a verdict:** `PREFLIGHT.md` names A and C as B's fits and doesn't
mention B (select-then-order). Working through it directly here shows
select-then-order is at least as coherent with Q23 = B as either — arguably more
literal, since B's whole premise is "you are choosing who acts," and
select-then-order is the scheme built around exactly that action. Its cost is tempo
(see §5), not incoherence, which is a different objection than the one `PREFLIGHT.md`
raises for the other pairings.

---

## 4. The counter-check

`castle-layout.md` §6.3 states the marks are "counterable, and the counters are
squad-shaped," and names three: **Quilled** ("wants melee, not the archers it is
armoured against"), **Ram** ("wants to be intercepted away from the gate"), **Sated**
("wants burst that outruns regeneration"). Working each Q23 shape against these three
— genuinely different answer per mark, or the same answer three times.

### Shape A (fixed player kit)

- **Quilled** wants a *composition* answer — commit melee, hold ranged back. A's
  verbs (§2) act on "whichever soldiers are in range," not on a chosen subset by
  archetype. There is no verb in the fixed kit that says "only my melee-capable
  soldiers act here" — the kit structurally cannot express soldier-type selection,
  because it was designed not to route through soldier identity at all.
- **Ram** is answerable — "reposition" (Shield Wall-style anchor) can plausibly
  intercept a Ram-boss away from the gate, since it's a ground-targeted verb the
  player controls directly.
- **Sated** wants burst; the fixed kit's closest verb is focus fire (Mark Quarry),
  which is sustained-fire, not burst, and — critically — is the *same* verb a player
  would reach for against Quilled too, since neither mark's counter is expressible
  as anything more specific than "make my soldiers attack this thing."

**Finding:** under A, Quilled and Sated collapse toward the same answer (focus fire,
because nothing else discriminates), and only Ram gets a genuinely distinct response
(reposition). That is one clearly distinct answer out of three, not three. This
directly supports — rather than merely repeats — the register's stated worry that A
risks flattening the marks read; tracing it mark-by-mark against the actual verb text
is what turns that worry into a specific, falsifiable claim.

### Shape B (kit lives in soldiers)

- **Quilled**: genuinely different — commit the melee-archetype soldier (Vanguard-
  line), hold the ranged-archetype soldier back. This is a real compositional choice
  B enables and A cannot.
- **Ram**: commit the reposition-holding soldier (Shield Wall) or a mobility soldier
  (Shield Rush) to physically get between the boss and the gate. Also genuinely
  distinct from the Quilled answer.
- **Sated**: **no clean answer exists in the current source material.** None of the
  four hero kits' signature/field verbs is a burst effect (§1). The Hunt Is Called
  is the nearest candidate, but it is a Pathfinder ultimate gated on that class's own
  mark system, not a general squad verb any of the seven could plausibly carry as
  their one action.

**Finding:** B gives two clearly distinct, squad-shaped answers (Quilled, Ram) and
currently has **no answer at all for Sated** — not because Q23's shape is wrong, but
because the source material (`CLASSES.md`'s four kits) has no burst verb in it. This
gap exists independent of which shape is chosen; it is a content gap in the source
kits, and it will surface under B or C regardless of which one the owner picks.

### Shape C (both)

Inherits B's per-soldier verbs, so Quilled and Ram read the same as under B. Its
extra layer is a plausible place to *close* the Sated gap — a player-level "amplify"
modifier could convert a soldier's sustained damage into effective burst for a
window, which B alone cannot do without inventing a new base verb. This is a
possibility, not a resolution: the specific modifier verb that would do this doesn't
exist in any canon document and would need its own design pass before it could be
claimed as C's answer to Sated.

---

## 5. What to watch in the prototype

The most concrete section — what would confirm or kill each option in play, not in
argument.

**Q23 = A (fixed kit).** Watch whether the player's verb choice ever changes with
which mark is on screen, or whether one verb (most likely "focus fire") gets pressed
against every boss regardless of its marks. Convergence on one dominant button across
different marked bosses is the kill signal §4 predicts in the abstract — this is
where it would show up concretely. Also watch for a player trying to *hold back* a
specific soldier type against Quilled and finding no way to do it — under A that
choice has nowhere to live; if a tester reaches for it anyway, that confirms the gap.

**Q23 = B (kit lives in soldiers).** Watch whether players can say, unprompted,
"which of my seven do I send at this" and get it right without being told — that's
the shape's whole thesis working. Watch for the specific failure mode the register
names: does spending a soldier collapse into "whoever's cooldown is up" (rotation)
instead of a mark-driven read? If most marks only have one legal responder among the
seven, the "choice" is hollow the same way A's is, just reached by a different route.
Specifically watch what happens against a Sated boss, given §4's gap — does a
playtester notice there's no good answer and get frustrated, or does the first slice
simply not include a Sated encounter yet, in which case this observation doesn't
fire and the gap stays theoretical until it's tested?

**Q23 = C (both).** Watch whether a tester can describe what the player-level layer
*does*, separately from the soldier verb it's modifying. If nobody can explain
"amplify" without also re-explaining the base verb underneath it, that is the
register's "legible only to the designer" cost made concrete, not hypothetical.
Watch input friction directly: does the two-stage input (address a soldier or verb,
then apply the modifier) cost a beat of hesitation that reads as sluggish against a
boss admitting 35–55 concurrent attackers (`entity-tiers.md` §4) — a fight whose pace
assumes continuous engagement, not a pause to compose an action.

**Q26 = A (direct target).** Watch for wrong-verb activations where a click is
ambiguous between enemy/ally/ground (§1's three target types). Every misfire here is
evidence against this scheme regardless of which Q23 shape it's paired with.

**Q26 = B (select-then-order).** Watch whether the selection step reads as
deliberate strategy (intended, per `PREFLIGHT.md`'s "reads as strategy") or as the
fight visibly continuing without the player while they click a portrait. Time the
gap between selecting and ordering against how long a front stays at a state where
that gap matters.

**Q26 = C (contextual button).** Watch for the single button firing the wrong
soldier's verb because "who's available" resolved ambiguously — this scheme's entire
risk surface is exactly that resolution being wrong, and it will show up as visibly
incorrect casts, not as a subjective feel complaint.

**Q26 = D (verb wheel).** Watch whether the game needs to slow or pause time for the
wheel to be usable, and if it does, watch whether that reads as contradicting Q13 =
C's framing of the player as embodied and in danger during the fight — a wheel that
requires stopping the world to use is a different fantasy than "a fighter whose
entire output is the seven."

---

## What this document does not do

No option above is ticked. Q23 and Q26 remain open in `docs/OPEN-DECISIONS.md`. This
document does not edit that register, `castle-layout.md`, `GDD.md`, `CLASSES.md`, or
any source file — it is the input to an owner call, not the call itself.

**Not resolved by canon and not inferred here, per `castle-layout.md` §6.4:** whether
the player has any direct attack for self-defence, cooldown/resource structure for
the kit, and whether Q23's "targeting" question (individual soldiers vs. the squad as
a unit) is settled by whichever shape is chosen — it may not be; a shape (A/B/C)
constrains but does not by itself answer that targeting question.
