# The introduction, and the zone model that carries it

**Version:** 0.1 · **Status:** four framing decisions locked by owner 2026-08-13.
**Companion:** `docs/design/castle-layout.md` (the five layers this moves through),
`docs/design/CAMERA-SCALE.md` (§3 of which this retires), `docs/perf/BUDGETS.md`
and `docs/perf/one-camera-bench.md` (every measured number below comes from these).

---

> ## What this is
>
> Two things that turn out to be one thing.
>
> **Part A** is the opening — the player's first five minutes, starting on the second
> layer among wounded soldiers, ending with the Great Gate falling on script.
>
> **Part B** is the **zone model**: how the castle is cut into simulation units, what
> the transition between them is, and what it costs. The opening is the first thing
> that has to survive that model, which is why they are specced together.
>
> ## The headline, because it inverts the usual trade
>
> **Cutting the castle into zones is what buys the mass, not what costs it.** One Live
> zone spends the *entire* frame budget instead of five zones sharing it. Measured
> (`docs/perf/one-camera-bench.md`, 2026-07-28, standalone `-game`): Niagara holds
> **2.31 ms at 1,000 units and 8.32 ms at 10,000**, with 100% of frame cost being Mass
> sim on the game thread at ~0.75 ms per 1,000. Against a 16.6 ms budget, a Live zone
> of several thousand is affordable with room left over — **including during the slide,
> when two zones are briefly live at once** (§B5).
>
> ## What it does not change
>
> No measured number is edited. `castle-layout.md`'s five layers, gates, front ledger
> and boss-marks model are read here and changed nowhere; this doc refines §2.1
> (the gate as a transition) and §7 (fidelity bands) into something implementable and
> contradicts neither.

---

## 0. The four decisions

Owner call, 2026-08-13, second pass. Recorded before anything is derived from them.

| # | Decision | Consequence |
|---|---|---|
| **D5** | **The intro ends in a scripted loss.** The first boss cannot be killed and the Great Gate falls regardless of play. | Carries a real risk of teaching "your intervention doesn't matter." §A7 is entirely about defusing that, and the defusal is where the intro gets its hook. |
| **D6** | **The incoming zone is already live and fighting during the slide.** Not frozen, not idling. | The pre-warm model (§B4) becomes mandatory rather than an optimisation. This is also the single best payoff for simulating offscreen at all. |
| **D7** | **The army-size camera descent is retired.** One framing, orthographic, zoom on player input. | `CAMERA-SCALE.md` §3's target and §4's Q1–Q4 die with it — **including its stated "primary technical risk"** (blending ortho into perspective). A risk deleted, not solved. |
| **D8** | **The healer-mages are the castle's, not yours.** Garrison field medics, autonomous. | The opening image is the war working without you, which is the only setup that makes "so what am I for?" land. |

---

# PART A — The opening

**Target length: about five minutes.** No tutorial text, no popups, no tooltip prompts.
Every mechanic below is taught by watching something happen.

Boots straight into play — no menu in front of it (task-115's shipped intent).

## A1 · The Infirmary Yard — L2, the Works · ~20s

Cold open. Fixed overhead ortho on a walled yard one layer *behind* the fighting.

Thirty-odd soldiers are down, and rendered **dim** — the existing degrade-don't-die
visual state (`GDD.md` §7; unfed and spent units literally lose value,
`FLAME-FOUNDATION.md` §2). Two garrison mages move between them. Two more soldiers are
carried in and set down while you watch.

Sound is the whole point of this beat: **the war is audible, muffled, from below and
outward.** You are not in it. You can move; nothing asks you to.

## A2 · The heal · ~10s

A mage plants and casts. A wide circle of light crosses the yard — **area, not target**.
Inside it the dim soldiers come back to full value and stand up.

**They do not look at you.** They form up and walk out toward the Cart Gate.

This ten seconds carries more teaching than any tutorial string would:

| Shown | Learned |
|---|---|
| A circle, not a beam | healers work in areas |
| Dim → full value on stand-up | "down" is a state, not a death — units degrade, they don't die out |
| Nobody waits for an order | the army is autonomous and self-directing |
| They walk *out* | the line is somewhere else, and it is short of bodies |
| None of it needed you | **the war works without you** |

## A3 · The seven · ~10s

The yard empties. Seven do not leave.

They turn and look at you. Everyone else files past. They fall in behind you.

No roster screen, no names on the HUD, no "you have acquired a squad." The distinction
that matters for the rest of the game — *these seven are mine, those hundreds are not* —
is established purely by who looked.

This is also `FLAME-FOUNDATION.md` §1's uncomfortable clause ("they do not think you are
a person") shown rather than written, and it costs one animation.

## A4 · Out of the yard — the first slide · ~1s

You walk out. The camera slides one screen (§B5).

The new zone is the Works proper: forges, alleys, stacked stores, roofs. **It is already
running when it arrives** (D6) — and the first thing visible in it is an archer bank on a
roof *relocating*: their firing positions have stopped scoring and they are moving rather
than shooting badly (§B, and `castle-layout.md` §5.2).

That is L2's defensive problem — **cover kills your ranged advantage** — demonstrated
sixty seconds into the game and never stated.

## A5 · The Cart Gate — the war, at last · ~30s

Slide again. The entity count jumps by an order of magnitude.

A held line. Grinding. Neither side moving. **The soldiers from A2 are in it** — that is
the payoff, roughly ninety seconds after the setup, and it is the first time the player
sees the sim's default output: a stalemate that reads as stable rather than as broken
(`castle-layout.md` §1).

## A6 · The alarm · ~5s

Below and outward, the Great Gate's front goes **Breaking**.

First hard gate transition, and it runs the wrong way: down and out. **This is the only
time in the game the war moves outward.** Everything after this is inward and up.

## A7 · The thing at the Great Gate · ~3 min

It is already through the outer portcullis, standing in the killing corridor.

It carries two marks and they are legible before it does anything (`castle-layout.md` §6.1):
**Ram** — it is carrying the door it broke — and **Quilled**, shafts still in it, because
the wall's archers already tried this. Between them they say *ranged has been tried and
melee is what is left*, which is the read the whole marks system exists to deliver.

**It has no health bar.** Not a slow one — none. This is the honest signal, and the rule
it establishes is worth keeping for the whole game: **the first boss that shows a health
bar is the first boss you can kill.** A bar that barely moves is exactly the "bigger HP
bar" failure mode `GDD.md` §9 names as the thing to design against.

A warden on the wall gives the objective in one shouted line, and it is not *kill it*:

> **You can't stop it. Get them out.**

> **Two Tier-0 decisions landed on this beat, 2026-08-13.**
>
> **Q7 = A — the castle has its own light**, so the Outworks beyond a fallen gate are the
> only ground in the opening the castle no longer holds. **A7 is therefore the first and
> only place in the intro where the leash matters** (`LeashRadius`, break latch,
> `LeashWarnBit` — shipped). That is not a coincidence to be smoothed over; it is the beat
> teaching what your flame is for, on the one piece of ground where the answer is visible.
>
> **Q13 = C — your entire output is the seven.** "Commit the squad" is literal here: you
> have no independent attack worth using, so intercepting the Ram is something the seven do
> and you direct. The ability kit that makes this possible does not exist yet
> (`docs/OPEN-DECISIONS.md` Q23), and **A7 is the beat that will expose whether it works.**

### What you are actually scored on

The Outworks are emptying. Wounded, non-combatants and a broken line are streaming from
the Great Gate toward the Cart Gate. **Your seven exist to buy that column time and keep
the boss off its back.**

So the loss is scripted and the *play is not*. The gate was always going to fall — the
game says so out loud — and how many people get through the Cart Gate is entirely yours.

### Why this defuses D5

A scripted loss teaches helplessness only when the scripted thing is also the *only*
thing being measured. Here it isn't:

- **The stated objective is achievable and the player achieves it, or doesn't.**
- **The failure is attributed correctly.** You didn't lose the gate; the war lost the
  gate. You were one squad, one layer behind, arriving late — which is precisely the
  job description the rest of the game hands you.
- **The consequence is legible and it is yours.** Which brings us to A9.

## A8 · The Great Gate falls · ~20s

On script. You get out — the player is never trapped by a fall (`castle-layout.md` §4.4),
and being punished for answering an alarm would be backwards.

The Cart Gate closes behind the last of the column. Slide, and **rise** — the first
inward transition, the direction of travel for the rest of the game.

## A9 · The Crown, and the hook · ~20s

The camera pulls up to the Crown and shows the whole thing at once: four gates still
standing, the host outside, the Outworks lit and lost below.

And down there, the thing that took it — **visibly larger than when you fought it.**

It ate whatever you did not save. It gains **Column-fed** (`castle-layout.md` §6.1) in
proportion to the people who did not reach the Cart Gate, and the mark is on its
silhouette from now on.

> ### The proposal this creates — **OWNER CALL NEEDED (Q9)**
>
> **The intro's boss does not die in the intro, and it comes back.** The player meets it
> again later, at a gate that matters, carrying the marks it earned on them personally.
>
> If that is taken, the scripted loss stops being a tutorial concession and becomes the
> strongest thing in the opening: *the game's recurring antagonist is exactly as strong
> as you let it become, in the first five minutes, before you knew that was what was
> happening.*
>
> This is a proposal, not a decision. It is written here because A9 sets it up whether or
> not it is taken, and because if it is **not** taken, A9 should be re-cut — a visible
> consequence that never returns is a promise the game breaks.

---

# PART B — Zones, seams, and the slide

## B1 · The zone is the simulation unit

A **layer** (L1–L5) is a place. A **zone** is a chunk of one, and it is what the
simulation is scoped to. A layer holds 1..N zones — the Outworks are the widest ring and
want several; the Crown is one.

Two kinds of boundary:

- **Hard seams — gates.** Between layers. Slide plus a change in elevation. The moment
  `castle-layout.md` §2.1 describes.
- **Soft seams — necks.** Within a layer: an arch, a turn in an alley, a stair landing.
  Same mechanism, smaller gesture, no elevation change.

Both are the same code path. Only the framing differs.

## B2 · Fixed framing makes zones authorable in screens

D7 retires the camera descent, and the thing that falls out of it is more useful than the
descent was: with one fixed orthographic framing (today `Cam.Ortho 1`, `Pitch -90`,
`OrthoWidth 2400` — `CAMERA-SCALE.md` §2), **the screen is a known rectangle in world
space.** Zones can be authored as whole multiples of it, which is exactly how the games
this transition is borrowed from did it.

- **Within a zone the camera scrolls freely.** You need to see a front, not a screen.
- **At a seam it slides exactly one screen.** That is the effect.
- **Authoring reference is the default zoom.** Player zoom is an affordance, not an
  authoring unit — see Q11.

## B3 · Three states

Refines `castle-layout.md` §7's fidelity bands into something with a trigger and a cost.

| State | Which zones | What runs | Notes |
|---|---|---|---|
| **Live** | exactly one (two during a slide) | full Mass sim, VFX, audio, per-unit sprite frames | gets the whole entity budget |
| **Warm** | every zone one seam away | Mass sim under `Swarm.SimLOD.Stride`; no VFX, no audio, no per-unit frames | entities are **resident** — this is what makes D6 possible |
| **Cold** | everything else | the front ledger only (`castle-layout.md` §5.1) — four numbers, no entities | still produces boss marks (§B7) |

`Swarm.SimLOD.Stride` already exists and already ships (added 2026-07-28; Stride 4 raised
the 60 fps ceiling from ~21,000 to ~34,000). **Warm is not new tech.** It is the existing
LOD lever pointed at a zone instead of at a distance band.

## B4 · Pre-warm — the whole trick

**Nothing may be allocated during a slide.** The only spawn measurement the project owns
is a **23.46 ms single-frame spike for 250 entities**, and a hitch at the exact moment the
camera is moving is the worst place in the game to put one.

So Cold→Warm promotion happens **while the player is still walking**, triggered by a
pre-warm volume placed well before the seam, and dripped across frames.

Sizing it, from that same measurement — ~0.094 ms per entity created:

| Promotion budget | Entities per frame | Time to warm a 2,000-entity zone |
|---|---|---|
| 1.0 ms/frame | ~10 | ~3.3 s |
| 1.5 ms/frame | ~16 | ~2.1 s |

**So a pre-warm volume needs roughly 3–4 seconds of walking ahead of its seam.** That is a
level-authoring constraint, and it is the main thing the zone graph has to respect.

> **Inferred, not measured:** the per-entity figure is derived by dividing a batch
> measurement, and drip-spawning may not scale linearly. Worth a direct measurement before
> anyone authors to it (Q12).

**Fallback for the player who sprints.** If a seam is reached before promotion completes,
**hold the slide** for the remaining frames rather than finishing the promotion in one.
A brief hitch in camera start is invisible; a 23 ms frame is not.

## B5 · The slide

~0.7 s, eased, exactly one screen of camera translation. Player input locked or
auto-walked through it.

| Phase | Zone A (leaving) | Zone B (arriving) |
|---|---|---|
| before | Live | Warm — resident, ticking, **already fighting** |
| during | Live | promoted to Live; both rendered, both simulating |
| after | Live → Warm | Live |
| +8 s dwell | Warm → Cold | — |

**Both zones are live during the slide, and B is mid-battle when it arrives** (D6). Units
in it are already in contact — no pop, no spin-up, nothing waiting for the camera. This is
the single clearest statement the game can make that the war did not pause for you, and it
is the visible return on simulating what the player cannot see.

**Demotion is lazy, deliberately.** Zone A is never deallocated during the slide — the
frame that is already carrying two Live zones does not also get a teardown. And the 8 s
dwell before Warm→Cold is hysteresis: a player pacing back and forth across a seam must
not thrash the promotion path.

## B6 · The budget, against measured numbers

All from `docs/perf/one-camera-bench.md`, 2026-07-28, standalone `-game`, `-SwarmBench`,
Niagara path, single client.

| Entities | Frame ms (measured) |
|---|---|
| 500 | 1.75 |
| 1,000 | **2.31** |
| 2,000 | 3.06 |
| 5,000 | 5.02 |
| 10,000 | 8.32 |
| 20,000 | 15.90 |

Against 16.6 ms:

| Situation | Cost |
|---|---|
| Live zone, 2,000 | **3.06 ms** — measured |
| Live zone, 5,000 | **5.02 ms** — measured |
| **Slide peak** — two Live zones at 2,000 each | **~6.1 ms** — measured, summed |
| Two Warm zones at 2,000, Stride 4 | **~0.8 ms** — *inferred* from ~0.75 ms/1,000 sim ÷ 4 |
| Cold zones, any number | negligible — four numbers each |

**The worst moment in the model costs about a third of the budget.** That is the answer to
"I still want the option for mass amounts of units": a zone that wants to be a spectacle
can be authored at 10,000 (8.32 ms measured) and still slide, and one that wants to be a
corridor costs nearly nothing.

> **What these numbers do not include, stated plainly.** They were measured on a flat
> field: no level geometry, no navmesh, no gates, no audio, no UI, single client. A real
> castle will cost something that is not in this table. The *shape* of the argument
> survives — sim dominates, sim is cheap, headroom is large — but the specific millisecond
> figures are a floor, not a forecast.

## B7 · The rule that keeps Cold honest

Restated from `castle-layout.md` §7 because it is the thing most likely to be quietly
dropped during implementation:

**A boss's marks must be earned identically in all three states.** A front that resolves
in the Cold ledger has to produce the same marks the Live sim would have. Otherwise §A9's
consequence — and the whole claim that a boss is a report on a war you missed — is false,
and the player finds out the first time a distant fall yields an unmarked boss.

---

## C · The camera decision (D7) and what it deletes

`CAMERA-SCALE.md` §3's target — camera descending from wide ortho to a character cam as
the army dies — **retires.** Its premise was a player commanding hundreds and losing them;
the player now commands seven, and a descent driven by squad losses would fire constantly.

**Retired with it, from that doc's §4:**

- **Q1** what drives the interpolation
- **Q2** continuous or staged
- **Q3** does it ratchet back up
- **Q4 — ortho → perspective blending.** That doc calls this "the primary technical risk."
  D7 deletes it rather than solving it. This is the largest single risk reduction in the
  pivot so far and it should be counted as one.

**Survives and still needs answering:** §4's **Q5** (the flame pool is tuned to a fixed
`OrthoWidth` — `FlameRadius` 900 against 2400 — and `DitherWorldAnchor 1` ties dither
density to world space, so *any* player zoom changes both). Carried forward as Q11.

**Not retired:** everything in `CAMERA-SCALE.md` §2 (the honest account of what the
renderer actually is today) and §5 (what must not be lost from `UUnitCamProjector`). Those
belong to the Niagara sprite path and are untouched by this.

---

## D · Risks

1. **D5 still reads as helplessness.** §A7's mitigation is a design argument, not a
   measured result. **Test it directly:** if playtesters describe the opening as "I lost"
   rather than "I got them out," the mitigation failed and the warden's line, the scoring
   readout and A9's consequence all need to be louder.
2. **The missing health bar reads as a bug.** Mitigated by the rule being consistent
   forever after, but the first-time read is untested.
3. **The slide exposes the simulation boundary.** With B already live, the far edge of the
   incoming zone is where entities stop existing. Mitigate by siting seams at genuine
   visual occluders — a gate arch, a turn, a stair landing — so the slide travels *through*
   something that blocks sight.
4. **Pre-warm volumes mis-sized by authors.** §B4's fallback (hold the slide) contains it,
   but a level with seams three seconds apart will thrash.
5. **A fight the player walks away from resolves statistically.** That is the design, and
   it is also the most likely place for a player to catch the sim being two different
   things (§B7).

---

## E · Open questions — do not answer by inference

Continues `castle-layout.md` §10's numbering. **The full register is
`docs/OPEN-DECISIONS.md`** — it consolidates these with §10's and with Q13–Q23, and it is
where verdicts get recorded. Tier 0 (Q13, Q1, Q7) closed 2026-08-13; **Q23, opened by
Q13's answer, is the one that most directly blocks building this document's Part A.**

| Q | Question | Why it is not inferable |
|---|---|---|
| **Q8** | Zone count and authored size per layer | Needs the level, not a doc. §B4's 3–4 s pre-warm lead is the binding constraint. |
| **Q9** | **Does the intro's boss return?** (§A9) | A proposal, and a strong one, but it commits the game to a named recurring antagonist — a real content decision. If **no**, A9 must be re-cut. |
| **Q10** | Does the withdrawal count persist — and does it feed the keep ratchet (D3)? | It is the first number the game asks the player to care about. Whether it is a stat, a modifier, or just a mark on a boss changes what the intro *is*. |
| **Q11** | Player zoom range, and what happens to the flame pool and dither at other zooms | Inherited live from `CAMERA-SCALE.md` §4 Q5. D7 did not answer it, only narrowed it. |
| **Q12** | What a Warm zone actually costs, and whether drip-spawn scales linearly | Both are inferences off measured batch numbers in §B4/§B6. The `-SwarmBench` harness can settle them. |

---

## F · Reconciliation

No file below was edited.

| Doc | Status after this one |
|---|---|
| `docs/design/castle-layout.md` §2.1, §7 | **Refined, not contradicted.** This is the implementable version. |
| `docs/design/CAMERA-SCALE.md` §3, §4 Q1–Q4 | **Retired by D7.** §2 and §5 stand. §4 Q5 carried forward as Q11. |
| `docs/RENDERING-LIGHTING.md` §4d | Still describes what ships; unaffected — the two-viewport question belongs to the Niagara path, not here. |
| `docs/perf/BUDGETS.md`, `one-camera-bench.md` | **Read only.** Every figure in §B6 is quoted, none altered. |
| task-115 (menu that boots into play) | **Consumed.** A1 is what it boots into. |
| `docs/GATE1-FUN-PROTOTYPE.md` | Survives as evidence. The wave/breather rhythm is now a front's rhythm. |

---

## G · Decision log

- **2026-08-13** — D5 scripted-loss intro · D6 incoming zone live during the slide ·
  D7 camera descent retired · D8 mages are the castle's. Owner.
- **2026-08-13** — The withdrawal, not the gate, is the intro's real objective (§A7).
  Written as the defusal of D5's helplessness risk. **Untested.** This doc.
- **2026-08-13** — "The first boss with a health bar is the first boss you can kill"
  adopted as a global rule (§A7). This doc.
- **2026-08-13** — Zone model: Live / Warm / Cold with pre-warm promotion and lazy
  demotion, budgeted against 2026-07-28 measurements (§B). This doc.
- **2026-08-13** — **Q13 = C, Q1 = A, Q7 = A** (owner). Q13 makes the seven the player's
  entire output and opens **Q23**, the ability kit A7 depends on. Q7 makes the fallen
  Outworks the one place in the opening the leash bites. Q1 gives A9's consequence a
  boundary to persist across. Recorded in `docs/OPEN-DECISIONS.md`;
  `castle-layout.md` §4.1, §6.4 and §10 carry the detail.
