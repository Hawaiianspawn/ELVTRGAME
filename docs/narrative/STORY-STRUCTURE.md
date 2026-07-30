# Story Structure — Third-Party Escalation & Party of Co-Leads

**What this is:** a proposal filling two slots `FLAME-FOUNDATION.md` leaves open — the
antagonist (§5) and the party/godhood tension (§4.4) — using two structural devices from
*VERSUS* (story ONE, art Kyoutarou Azuma) the owner selected. **Extends** `FLAME-FOUNDATION.md`
§1, §4.4, §4.5, §5; `GDD.md` §4, §5, §6, §9; `docs/design/run-structure.md` §4–5.

**Version:** 0.1 · **Status:** SHELVED 2026-07-30 (owner) — not rejected, not reviewed. Parked
because the VERSUS reference was being mined for unit archetypes, not plot. Also note the brief
behind §1–2 was based on a wrong reading of VERSUS (single tier-above invader); the real
structure is thirteen worlds each with its own enemy. Re-verify before unshelving.
**Does not edit** `FLAME-FOUNDATION.md`.
No faction, biome, or NPC names below — §5's deferral still stands. "The third force" is a
descriptive placeholder, not a working name.

---

## 1. The two devices, translated

- **Third-party escalation:** the dark and the bearers have never needed a name for their war —
  it's just the premise (§1). A third thing arrives that is a *tier above*: not a bigger dark,
  a different ruleset. It doesn't replace the dark as the threat; it stacks on top of it.
- **Party over protagonist:** the four v1 classes stop being a character-select menu and become
  co-leads you *find*, per §4.4's answer that uniting flames is a run objective. This is the
  natural single-player reading the owner flagged — worked through in §3.

---

## 2. The third force

**One line:** everything in this world that's dangerous is dangerous because it's *outside your
light*. The third force is dangerous because your light doesn't register it as a threat to keep
out at all — it doesn't need to pass through fire to act, and fire is the one law §1 says
everything else obeys. That's the tier difference: not more HP, not more of the swarm — a thing
your one tool has no default answer for.

It also explains something §1 states but never accounts for: *why are the other bearers
scattered and alone* if uniting is strictly better? Because something has been finding lone
flames and putting them out, one at a time, faster than bearers could find each other. That's
why they're scattered. Finding and uniting them (§4.4) stops being a nice-to-have power spike
and becomes the only real answer to something that's already been doing exactly that to
everyone else.

**The dark stays exactly as scary as it already is.** Nothing here dims the brood or softens the
leash-break stakes — the third force is a second, worse problem stacked on the first, not a
replacement for it. And because it's defined only by its relationship to fire, not by what
"the dark" turns out to be, it's compatible with §4.5 either way — *whether* the dark has
monsters or is itself the enemy, the third force's threat doesn't change shape. One row below
(hybridization) does assume monsters exist; flagged in §6.

| Fiction | Mechanic it explains | Status in code |
|---|---|---|
| The third force doesn't act through fire, the one rule everything else obeys | A new enemy category that light doesn't neutralize — the one thing standing in the circle doesn't fix | Unbuilt; slots into `entity-tiers.md`'s existing tier system as a new category, not a new system |
| It hunts and snuffs isolated single flames | Explains why bearers are scattered (§1) and makes uniting them urgent, not optional | Fiction only — reinforces the existing run objective (§4.4), no new mechanic |
| Dark creatures flee it toward your fire, briefly | In-fiction justification for the hybridization decision events GDD §5 already specs ("dark help at a cost") | Unbuilt — hybridization events are already unbuilt in `GDD.md` §5; this just gives them a reason |
| United flames do what no single fire can, *specifically against this* | Makes the run objective (§4.4) necessary, not just stronger | Still unbuilt; now has an in-fiction stake instead of being a bare power spike |

---

## 3. Party over protagonist — resolving the godhood tension

**The tension, stated plainly:** §1's whole premise is *you are the only light*. Four co-leads
on screen at once reads like four gods, which is either a plot hole or a different game.

**Resolution: co-leads don't merge into one shared light — they stay separate gods, walking
together.** When a found bearer joins, their congregation does not fold into yours. You now
have two circles of light near each other, not one bigger circle. Each bearer is still the only
god to their own people — that's *more* uncomfortable than a single diluted godhood, not less
(tone rule 3), because it makes visible something that was already true: your fire was never
special to anyone outside your own light, and now you're standing next to proof of it.

This stays single-player. The player controls exactly one bearer — the one whose decisions the
run answers to (§6 stays unchanged: **decisions are that bearer's alone, even with co-leads
present**; found bearers are companions, not additional votes). A found bearer's kit runs
autonomously, the same shape as an existing Elite-tier unit doing its own scripted signature
move — not a second playable hero, which would be a real control-scheme and camera build, not a
narrative decision. This is additive to, not a replacement for, the existing off-class
hybridization mechanic (GDD §5): hybridization splices *individual units* into your retinue at a
penalty; a co-lead brings a whole *separate* bearer, retinue, and light with them.

| Fiction | Mechanic it explains | Status in code |
|---|---|---|
| Found bearers keep their own congregation, never fold into yours | Multiple simultaneous light/leash sources on the field at once | Unbuilt — new. `LeashRadius` today is single-source (`SwarmCombat.h`) |
| A god's intent, not orders, moves a congregation (§1) — extended to more than one congregation | One stance order broadcasts to every present retinue; each interprets it per its own type, exactly like archers vs. pikemen already do (GDD §4) | Cheap extension of the built stance system — no new input layer |
| A found bearer is a god, but not one that takes orders from you | Found-bearer kit runs autonomous, elite-tier scripted behavior, not player input | Unbuilt; same shape as `entity-tiers.md`'s existing Elite framework |
| Losing your own flame ends the run; losing a found bearer's flame ends *their* congregation | Extends `run-structure.md` §5's hero-death-loses rule with one clause, doesn't replace it | Unbuilt — a one-line addition to an existing rule, not a new loss condition |
| The bearer decides alone, even walking beside another god | `GDD.md` §6's single-player decision resolution, unchanged | Already decided (2026-07-27) — reaffirmed against any "party votes" read |

---

## 4. Does this help or hurt §4.1 (stand-in-the-circle stagnation)?

**Helps — but only if it's felt during floors, not just at the boss.** A third-force presence
that only shows up in a final cutscene does nothing for the zero-input baseline problem
`GATE1-FUN-PROTOTYPE.md` already flagged as nearly winning. The cheap version: read the
**already-scheduled embedded Elites** on floors 2 and 3 (`run-structure.md` §2's table — 1 elite
on floor 2, 2 on floor 3) as, at least in part, the third force's agents, with one new
behavioral rule — *this one ignores the "inside light = safe" status* — layered on the existing
elite-design slot rather than a new spawn system. That directly taxes standing still on the
exact floors where the exploit would otherwise be safest, using slots the run structure already
has open. No new schema; one new elite behavior to design later.

## 5. What this does to §4.2 (is being worshipped in tone)

Deepens it, doesn't resolve it. Standing next to another god who cannot save your people any
more than you can save theirs is a sharper version of the same discomfort — "they will die for
me and I cannot make them stop" now has a second person on screen who understands it exactly,
and can't help either. Keep this unresolved; it's the seed tone rule 3 already asks for.

---

## 6. Open — deliberately open

1. **Multi-source leash is the single biggest engineering fork this doc leaves open.** Is "two
   overlapping circles of light" actually v1, or does a found bearer instead just strengthen
   *your* one leash (bigger radius, not a second source)? The fiction in §3 assumes the former;
   the latter is cheaper and still defensible, but reads as "reinforcement," not "co-lead."
2. **Does losing a found bearer have a run-wide mechanical cost** (permanently smaller total
   army for the rest of the run) or is it fictional weight only? Undecided here.
3. **§2's hybridization row assumes the dark has creatures that can flee toward you.** If §4.5
   resolves toward "the dark itself is the enemy, no monsters," that row doesn't work and should
   be cut — the rest of §2 doesn't depend on it either way.
4. **Naming.** Nothing above is named. An art brief for the third force's "light doesn't protect
   you from this one" visual tell is warranted, but only once the antagonist concept itself is
   approved — writing one against an unapproved proposal would be building on a name that might
   not survive review.

---

## 7. Canon proposals

- `FLAME-FOUNDATION.md` §5: fill the antagonist slot with §2 above, once approved.
- `FLAME-FOUNDATION.md` §4.4: append §3's resolution (found bearers as autonomous co-leads,
  separate light sources, no vote) as the answer to "what uniting a fire actually does" —
  currently listed as still needing design.
- `run-structure.md` §5: add the one-clause extension to hero-death-loses from §3's table,
  once §6.2 (run-wide cost of losing a found bearer) is settled.
