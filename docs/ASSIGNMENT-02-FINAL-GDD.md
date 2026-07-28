KINDLED
Game Design Document — Final Draft
Multi-Agent AI for Game Development — Assignment #2
Aaron Low — 27 July 2026

> Supersedes the Assignment #1 first draft (`docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md`,
> 21 July 2026), submitted under the working title *Emberkeep*.
>
> **Submitted PDF:** `docs/Kindled-GDD-Assignment-02.pdf`
> **Render source:** `docs/assets/kindled-gdd-a02.html` — this markdown is the readable
> repo record; the HTML is what the PDF is generated from. Re-render with:
>
> ```bash
> "/c/Program Files/Google/Chrome/Application/chrome.exe" \
>   --headless=new --disable-gpu --no-sandbox --no-pdf-header-footer \
>   --print-to-pdf="docs/Kindled-GDD-Assignment-02.pdf" \
>   "file:///C:/Projects/ELVTRGAME/docs/assets/kindled-gdd-a02.html"
> ```
>
> **Figures** live in `docs/assets/img/` and are referenced relatively, so the HTML renders
> them for the PDF and this markdown shows them in any viewer.
>
> **Canon status.** `GDD.md` was reconciled on 2026-07-27 — title (§12 Q10), the
> single-player decision (§12 Q20), the redefined entity gate (§10), and `[MP — deferred]`
> markers on the co-op sections. The `Emberkeep.*` **CVar namespace in `ELVTR/Source/` was
> deliberately NOT renamed** (142 references plus `SwarmExecOnPlay.txt` and the Breadboard
> parser key off it): the game is *Kindled*, the CVar prefix is still `Emberkeep.`.

> *You are the only light. Everything that lives, lives inside your circle — and they
> think you are a god.*

---

## 1. Executive Summary

**Kindled** is a single-player roguelike in which you carry the only fire in a pitch-dark
world, and an army gathers in your light because outside it they die.

The world is not at night — it is *dark*, and the dark takes what it touches. Power exists
only where it passes through fire, so a **flame-bearer** is the one place anything can
happen. People gather to you, fight for you, and need you to live; this is not loyalty, it
is physics. They do not think you are a person — a bearer is a god to the people around
them, and you can neither ask for that nor decline it. Other bearers are scattered in the
dark, each with a small congregation in a small circle of light. Alone, a flame holds a
room. **United, flames do what no single fire can** — and finding them is what a run is
*for*. They are people you locate in the dark and unite with, not other players in a lobby
(§2.3).

In play: a top-down roguelike run where you command a growing retinue — three to ten units
early, hundreds by late run — through four broad **stances** rather than unit selection or
targeting. It renders in a locked four-value 2-bit palette chosen so a several-hundred-unit
battle stays legible by silhouette and value alone. Retinue growth, not loot, is the
progression axis and the run's reward.

| | |
|---|---|
| **Win condition — per run** | Survive three escalating waves and kill what the last one brings. The fire is still burning when the floor is clear. |
| **Loss condition** | Hero death. The flame goes out, everyone standing in it is in the dark, and the run ends immediately and finally. |

The rule the whole design hangs on is the **leash**: every unit has a maximum distance from
the bearer, and that distance is drawn on the ground as the reach of your light. Inside it,
lit floor and full-value units. Outside, the dark ground state and units dimmed a palette
step. A unit that strays too far drops whatever order you gave and returns — not
disobedience, but *running back to the light*. You may anchor a line at a chokepoint, but
you can never walk away from it.

**Project status — playable, measured, not yet passing its own gate.** The Gate 1 prototype
(all four stances, the leash, a three-wave run, win and lose states) was built 2026-07-22
and has been tuned against measured runs since. It is calibrated so that a **zero-input run
loses** — narrowly and repeatably, three runs out of three — so everything the player adds
is the margin between defeat and victory. The concept's go/no-go gate, entity count at frame
budget, is *not* passed: §5 states exactly why and what it costs.

![The bearer's flame lighting a formation of 120 retinue](assets/img/fig-1-formation.png)

**Figure 1 — the game as it runs today.** The bearer's flame is the bright core; the dithered
falloff around it is the lit floor, which is also the leash radius (§3.2). 120 retinue hold
formation inside it while brood approach from the dark at the top of frame. The panel along
the bottom is the Unit Cam: a close-up of the line, flanked by six squad-strength readouts.
*Debug-box renderer — the sprite path is repaired but unmeasured (§5.1).*

---

## 2. What Changed Since the First Draft

Three inputs drove this revision: a narrative reset, three corrections that review and
measurement forced, and my own judgement calls. The largest change is first.

### 2.1 The narrative reset — the whole world was discarded

Assignment #1 described the *Undervault*: a kingdom a thousand years dark, the Hollow Crown,
three factions, five named NPCs, fifteen persistent world flags and eight decision-event
templates. On 2026-07-22, **all of it was discarded.** `WORLD.md` is superseded in full and
kept unedited as history; the replacement is `docs/narrative/FLAME-FOUNDATION.md`.

The reason, stated plainly: **the set dressing had outrun the prototype.** I had eight event
templates written and none built, fifteen world flags specified and one stubbed write
planned — while the actual playable build carried an unanswered question that no amount of
faction lore could address, namely whether the leash reads as a rule the player understands
or whether the army simply feels like it disobeys. The fiction was accumulating faster than
the thing it was supposed to describe.

The replacement premise (§1) is deliberately short and mechanics-first, and it ships with a
rule every future piece of lore must pass: **if a piece of fiction does not explain a
mechanic that already exists or is already needed, it is decoration and it waits.** The
premise was written against that test:

| Fiction | Mechanic it explains | Status in code |
|---|---|---|
| Outside the light, the dark takes you | The leash — radius, break latch, warn bit | **Built** |
| A god is obeyed in intent, not in orders | Stances; no unit selection, no micro | **Built** — 4 stances |
| Fire must be fed or it dims | Upkeep — units degrade rather than die | Specced |
| You are the only light source | Hero relevance without hero damage | Unblocks the 55-DPS problem |
| Many bearers, united | The run objective — find the other fires, bring them together | Unbuilt; the run's spine (§2.3) |
| They die for you gladly | Sacrifice events priced by what it costs *you* | Specced |

**What the reset bought immediately**

- **The leash became visible instead of a rule.** Rendering the leash radius as the lit
  floor answers the prototype's open question directly, and costs one floor decal plus a
  per-unit value shift the renderer already has the data for.
- **Hold and Charge acquired a price.** Both push units toward the edge of the leash — which
  is precisely where the fiction says it is dangerous to stand. That is not a bug, it is the
  game. Three of the four stances now have a cost the player can feel; previously they were
  movement modes.
- **The hero stopped needing damage.** The first draft worried openly that a 55-DPS hero was
  "a camera with a health bar." The hero is now *the thing everyone is standing in*:
  abilities concern where the light reaches, how bright, how long, and at what cost.

**What the reset cost — stated, not hidden**

The first draft's **meta-win condition went with the lore.** Progression across runs was
"your decisions write world flags that visibly weaken the Hollow Crown's grip." There is no
Hollow Crown and there are no flags. Meta-progression is now *deliberately unwritten*,
because the two prototype questions that determine what a persistent world would need to
contain — does standing in the circle stagnate into a dominant strategy, and does overlapping
light between two players actually do something — are unanswered. Writing it now would repeat
the exact mistake the reset exists to undo. Per-run win and loss (§1) are unaffected and are
what the build implements today.

### 2.2 Three corrections review and measurement forced

The first two contradict sentences in the submitted first draft.

**a. The Shield Wall claim was wrong.** Assignment #1 said "the Vanguard's Hold is a literal
Shield Wall that blocks enemy pathing." A blocking wall needs per-unit pathing obstruction —
exactly the individual special-casing Mass Entity rules out at horde scale. **Corrected
2026-07-26 toward the simulation:** Hold pins the formation where it was issued, but nothing
about it changes how far an enemy will divert to engage. The tide bites what is near its path
and flows past what is not. Shield Wall is now a positioning tool that reads as a wall only
where geometry already makes it one.

**b. A balance claim I made was false, and measurement caught it.** Moving from continuous
damage to discrete swings, I wrote that average DPS was unchanged. Wave-1 survivors fell from
97–103 to 60–62. The cause: `MaxAttackersPerUnit` — called "the single most important dial in
the model" in my own design — **never bounded a damage rate at all.** It capped the first N
enemies in *grid iteration order*, and the grid rebuilds every frame as units move, so that set
churned and far more than N attackers landed blows per interval. Fixed with geometry rather
than counters: each attacker publishes the distance to its K-th nearest enemy, and a victim
takes a blow if inside it — one compare, no cross-entity writes. That fix then exposed a second
thing: a *constant* K removed a stabiliser the broken model had by accident, and fights went
**bimodal**. Hence the shipped defaults, which restore the stabiliser deliberately.

**c. The renderer cannot hold the bar it was assumed to hold.** At 1,000 enemies the debug-box
renderer costs **14.62 ms on draw alone** against a 16.6 ms frame — and 350 ms at 10,000. Draw
time *is* frame time at every count tested. Since the Spike 1 floor is "1,000+ units at 60 fps,"
this renderer cannot pass it. Now the project's binding technical fact rather than an
assumption (§5). Source: `docs/perf/BUDGETS.md`.

### 2.3 What I changed by my own judgement

- **The title.** *Emberkeep* came from the discarded canon — it named a place, and the place
  no longer exists. **Kindled** names a *state* instead, and specifically the state of
  everyone who is not the player: they have been kindled, by you, and they will go out
  without you. The participle was chosen over the bare verb deliberately. *Kindle* would have
  named what the bearer does; *Kindled* names what it does to the people standing in the
  light, which is the part of the premise the game is actually about — and it clears the
  search collision the verb form carries.
- **The game is single-player.** The first draft opened "1–4 player co-op" and treated that as
  the premise rather than a mode. It is now **single-player first, with co-op as a later
  multiplier on a proven loop** — and this is the largest scope reduction the project has taken,
  larger than cutting three classes. It does three things at once. It *retires* the replication
  risk the first draft called "the second-biggest risk," rather than deferring it with a cost.
  It cuts the entity gate by roughly 4× (§5.1). And it *answers* a question the premise had left
  open — whether uniting flames was a co-op mechanic or a run objective. It is the run objective:
  the other bearers are found in the dark over the course of a run. That is the better version,
  because the objective is now always present instead of contingent on having three friends
  online.
- **The roster narrows to one class.** The first draft scoped four classes for v1 and one for
  the slice; this draft commits the semester to the **Vanguard** alone and says so everywhere
  (§3.6, §5).
- **Ad-hoc dispatch was replaced with a scored, lock-checked backlog.** The reason is a
  coordination failure specific to this project rather than a general tidiness urge — §4.2.

---

## 3. Game Mechanics — What the Player Sees and Does

### 3.1 The moment

Top-down. You move one hero with WASD and a visible army moves with you. There is no unit
selection, no drag-box, and no targeting: the entire command vocabulary is four keys,
issuable instantly mid-fight, that state an *intent* the units interpret for themselves. That
is not a simplification of an RTS — it is the fiction and the input model being the same
decision. A god is obeyed in intent, not in orders.

| Key | Stance | What the army does |
|---|---|---|
| 1 | **Follow** | Formation rings around you; engage whatever comes close. The default. |
| 2 | **Charge** | Surge at the cursor. Wide engage range, +25% speed — thrown past the edge of your light, on faith. |
| 3 | **Hold** | Anchor the formation where it stands; short engage range. Asking people to stand at the edge of the light, where the dark presses. |
| 4 | **Rally** | Collapse tight onto you — 45% slot radius, short engage. Everyone inside the fire. |

*Built and playable. The same four are drivable headlessly via console for scripted
measurement runs.*

### 3.2 The leash — the rule everything else is built on

Every unit has a maximum distance from the hero, roughly 1.25 screens. Cross it and the unit
latches "broken": it ignores your stance and behaves as Follow until it is well back inside —
a hysteresis band, so a unit sitting exactly on the boundary cannot flicker between states. A
warning fires at 80% of the radius, because breaking must never feel random, and the HUD
reports how many units are currently outside.

Under the flame premise this stops being an invisible rule and becomes the most legible thing
on screen: inside is lit ground and full-value units, outside is the dark ground state and
units dimmed one palette step. The player reads their own reach at a glance.

The consequence is the design's load-bearing restriction: **you cannot park the army on a
chokepoint and walk away.** Anchoring a line always means standing in the fight beside it.
Hero relevance is enforced by rule rather than by tuning numbers until it happens to be true.

### 3.3 What a fight feels like

Damage arrives in visible blows, not as a slide. Each unit runs its own swing clock — windup,
strike about a third of the way in, recover — and the strike is edge-triggered so a blow lands
exactly once regardless of frame rate. The clocks are deliberately desynchronised when units
spawn; without that, an entire wave strikes on the same frame and the army reads as one
pulsing organism rather than a crowd of individuals.

A struck unit flashes and is shoved 35 units of distance. That number is sized against
measured spacing — 86uu at rest, compressing to about 45uu when engaged, with a 95uu melee
reach — so the shove is big enough to see and small enough that the line does not blow apart
and the shoved unit can close again. The flash is deliberately exempt from light attenuation:
dimmed correctly, it would quantise below the palette's lowest threshold and become invisible
at the edge of the fire, which is exactly where fighting starts.

### 3.4 The run, and how it is calibrated

Three seconds to deploy, then wave one, a breather in which your retinue refills toward its
cap, wave two, another breather, wave three. Clear the last wave and you win the floor. Die
and the run ends.

| Zero-input baseline (hero never moves, no stance ever issued) | Wave 1 survivors | Wave 2 survivors | Result |
|---|---|---|---|
| Original continuous-damage model, 3 runs | 97–103 / 120 | 4–12 | Lost wave 3, by 4–13 enemies |
| **Shipped defaults**, 4 runs | 109–111 / 120 | 16–24 | Lost wave 3, by 131–145 enemies |

![Mid-run: retinue down to 78 of 120, six squad panels depleted](assets/img/fig-2-combat.png)

**Figure 2 — the same run, mid-fight.** Retinue 78 of 120, hero at 248/500, brood 49 still
alive. The six squad readouts have gone ragged — Shield down to 5/20, Vets 11/20 — which is
what attrition looks like before the line breaks. Losing units is expected and mournable; it
is why they have no fixed names.

**The run is lost by default, in every run, with tight variance.** That is the intended
calibration and it is the reason this build is usable as an A/B harness: everything a player
adds — repositioning, Rally before a flank lands, Hold on a choke, the hero's own damage aimed
somewhere useful — is the margin between losing and winning. The remaining honest gap is that
the current margin is 131–145 enemies rather than the original 4–13, so a player has more to
claw back than the calibration intends. Closing it is another tuning pass, and it should be
judged by feel as much as by the number, because the model underneath it changed (§2.2b).

### 3.5 Growth and upkeep — the progression axis

Retinue growth *is* the reward for a run; there is no loot system and its absence is a design
position rather than an omission. Units are gained by rescuing and rallying what the dark has
taken.

Size is **governed, not capped**. Every active unit draws upkeep from a per-run supply pool
replenished only at supply sites. When demand outruns supply, units do not die — they
**degrade**: dimmer, weaker, visibly unfed, and newly recruited units arrive degraded until
supply recovers. Fire must be fed or it dims. One mechanic does three jobs: it gives the
exponential power curve a real governor, it bounds the entity budget by keeping degraded units
in the cheap simulation tier, and it gives pressure on the player somewhere to land that is
not simply "more enemies." *Specced; not yet built.*

### 3.6 The Vanguard — the one class this semester ships

*The many.* You are the shield at the front and the banner they follow. Every cell door you
break open, every conscript you free, your line grows longer, and by late run you are marching
a liberated army through the dark. High count, disciplined; growth verb *rescue and rally*; a
retinue of liberated soldiers and militia that reads as a marching legion.

Three further classes are designed and parked — a fortifier who awakens stone guardians, a
skirmisher with a small elite pack, a healer trailing a constellation of kindled souls —
chosen to occupy distinct corners of count-versus-quality so the massive-entity fantasy shows
four faces. None of them are in scope for the remaining weeks (§5).

![The shipping sprite atlas: brood and retinue, eight rotations each](assets/img/fig-3-units.png)

**Figure 3 — the actual shipping atlas** (`T_Swarm_2bit`, 8 rotations × 4 rows, imported and
running). Brood read as a dark hooded mass with no discernible kit; retinue read pale,
armoured, and *equipped* — shield, spear, banner. That separation is doing the work described
in §3.7: at two hundred units you are not reading faces, you are reading light shape against
dark shape.

### 3.7 How it looks, and why that is a systems decision

A locked four-value palette, flat and unlit, with a post-process dither. This is not only a
production saving: at several hundred units, silhouette and value are the only channels that
survive, so the palette is what keeps friend, foe and boss separable at a glance without
health bars. The render budget goes to entity count rather than to shading. Exceptions to the
four values exist, are deliberate, and are documented individually — the flame's white core
and the hit-flash above are the two current ones, each with a stated reason.

![Six soldier archetypes at hero scale and again at crowd scale](assets/img/fig-4-roster.png)

**Figure 4 — the readability test the art has to pass.** Six authored soldier anchors, 100%
on the locked four-value ramp, shown at hero scale (top) and again at the size they actually
occupy in a crowd (bottom). The top row is where you design; *the bottom row is where you
judge.* Volunteer, line infantry, veteran, rally-caller, flame-tender and shield-heavy have to
stay distinguishable at that second size or the silhouette language has failed — which is why
every art spec is reviewed at gameplay zoom with hundreds of units moving, never as a single
sprite on a canvas.

---

## 4. The Development Crew

The game is built by one person directing a crew of specialised agents, two MCP tool servers
that give those agents real hands on the engine and the art pipeline, and a task system that
keeps them from overwriting each other. Every canon file has **exactly one** agent with write
access; everyone else treats it as read-only.

### 4.1 Roles — what each agent produces, and in what format

| Agent | What it produces | Output format & where it lands |
|---|---|---|
| **narrative-director** | The premise, decision-event text, and the fiction that has to justify itself against §2.1's test. Ends any visual work by writing an art brief rather than describing an image. | Markdown prose in `docs/narrative/`; numbered briefs in `docs/briefs/` |
| **pixel-art-director** | Silhouette and value rules per faction, palette tables, sprite-sheet layouts, readability reviews. Writes specs only — never image files. | Markdown specs in `docs/art/`, each ending in an acceptance checklist |
| **gameplay-director** | Stat blocks per entity tier, the scaling curve across floors, encounter budgets, retinue tuning — the numbers behind "a handful of militia" becoming an army. | Schema-validated JSON in `docs/data/` that must import cleanly as an Unreal DataTable, plus a companion spec in `docs/design/` |
| **performance-director** | Frame-time measurement and the optimisation plan behind it: what we hold today versus what the design needs, never what we hope to hold. | Measured tables in `docs/perf/BUDGETS.md`, each row citing the harness that produced it |
| **ui-director** | HUD and menu layout, screen flow, widget specs — and a viewable version of each before anything is built in engine. | Specs in `docs/ui/` plus a self-contained HTML mockup published to a URL |
| **host** (backlog) | Sweeps the repository for undone and newly stale work, and turns a stated goal into one scored, lock-checked task. It proposes and ranks; it never approves and never spawns. | One task file per unit of work in `docs/backlog/`, each carrying a paste-ready spawn prompt |
| **Lead session** | Reads the canon, briefs each director with only the section it owns, integrates the results, and carries every dispatch and hand-back. The only role that spawns anything. | Commits, and the merged state of the canon |
| **GDD Review Board** | Six isolated-context reviewers — systems, narrative, player psychology, feasibility, adversarial QA, business — critique the canon independently, cross-examine each other's findings, then a moderator synthesises. | A ranked, ratifiable edit plan; the party-vote rule in the first draft exists because this board argued for it |
| **unreal-mcp** *(tool server)* | Exposes the live Unreal 5.8 editor — actors, Blueprints, Niagara systems, materials — over HTTP, so an agent builds in-editor instead of describing a build. | Direct edits to the project; this is how the stances, the leash and the swarm exist at all |
| **pixellab** *(tool server)* | Generates sprites, portraits and animations to the art director's written spec. | PNGs into a holding folder, promoted to game content only after a human keep/reject call |

### 4.2 The two coordination rules that actually bite — and how they were mechanised

Both rules come from failures on this project, not from theory. Agents run as parallel
sessions with their own context windows, which creates exactly two ways to lose work:

- **Two agents editing one file overwrite each other; there is no merge.** So every task now
  declares the file globs it *owns*, and validation fails if two simultaneously active tasks
  claim overlapping paths. The concrete case: five design specs each wanted to write to the
  same systems document, so the shared write was split out into its own task that depends on
  all five and runs after them. Specs fan wide; the file they converge on is a single join.
- **There is one Unreal editor.** Two agents building or play-testing at once fight over the
  same process. So tasks also declare the resources they need — the editor, the MCP port,
  art-generation credits — and those are treated as global mutexes. Two agents can never be
  approved into driving the editor simultaneously. Credits are locked because they cost real
  money.

Dispatch is gated on both checks and refuses any task that is not approved with its
dependencies closed. The refusal is the point: **if it refuses, no tokens have been spent.**
In practice this caps the crew at three or four live threads, and the cap comes from the
single editor rather than from ambition.

> **Two failure modes worth recording, because both cost time.** An agent treats its spawn
> prompt as binding — telling a teammate mid-run that a restriction is lifted does not reliably
> override an instruction in its original prompt, and arguably shouldn't. The fix is to spawn a
> fresh one with the right scope, not to negotiate. Separately, an agent may honestly report
> that it has no access to a tool and then successfully call that same tool minutes later,
> because tools can be surfaced lazily rather than declared up front. Do not design around an
> agent's first answer about its own capabilities.

---

## 5. Scope and Constraints

### 5.1 Named constraints

| Constraint | What it means in practice |
|---|---|
| **One developer, one editor** | All in-engine work serialises through a single live Unreal process. Directors write specs in parallel; exactly one thing builds at a time. Previously held in my head, now enforced by the resource mutex in §4.2. |
| **The render bridge is over budget** *(the binding one)* | 14.62 ms of draw at 1,000 units against a 16.6 ms frame. The single-player decision *redefined* this gate — it is now one client at the late-run entity budget, with no replication term, roughly a quarter of the old bar — and the gate **still does not pass**. That is the clearest statement of where the project actually stands. The Niagara sprite path was repaired on 2026-07-26 but has no measured baseline yet, so the honest statement is that the replacement is untested rather than proven. |
| **Art generation is separately metered** | Sprite generation draws on a credit balance independent of language-model usage, so art volume is a hard-costed budget line. Every generation is retained before any keep/reject decision, so a rejected asset can still be compared against. |

### 5.2 What one person can actually finish in the remaining weeks

The first draft projected a semester of one class, one biome, three floors, one boss, two
decision events and two-to-four-player replication. **Four of those six are now out**, and one
of them — the replication — is out permanently rather than postponed, because the game is
single-player (§2.3). That correction is the scope lesson of this revision, and the sequencing
below is ordered by what invalidates what.

| # | In scope | Why it is first |
|---|---|---|
| 1 | Measure the repaired sprite path and record a GO / ADJUST / KILL verdict on the entity gate | Nothing downstream is trustworthy until this is a number rather than a hope. A KILL invalidates the concept rather than trimming it, and the fallbacks — fewer entities, single-player mass, or a faked crowd — are already named. |
| 2 | Play the prototype and answer its five open feel questions | Is Hold a real tool at this leash radius, or is the leash so tight that anchoring a choke is pointless? Does a leash break read, or does the army feel disobedient? Is Charge distinct from Follow in practice? Does the hero feel like a commander at reduced damage? Does 120 units read at this camera height? None are settled by any number above. |
| 3 | Render the leash as light | The cheapest thing the reset unlocked, and the direct fix for question 2 — a floor decal and a value shift on data the renderer already has. |
| 4 | Sprites in place of debug boxes for the Vanguard roster | Gated on 1. The current renderer is both over budget and visually placeholder; one change addresses both. |
| 5 | One hand-assembled arena floor with a boss, replacing ring-spawn | Turns a wave harness into a run. Hand-built deliberately — procedural generation is out. |

**Explicitly out, and why.** *The other three classes* — one class is enough to prove the
command layer, and four is four times the art. *Procedural generation* — the fallback to
hand-assembled floors is already written into the slice definition, and the one-developer
constraint says I get to build one thing at a time. *Persistent world state* — discarded with
the lore in §2.1 and correctly not rewritten yet. *Loot* — deferred by design, because retinue
growth is the reward loop. *Networked co-op* — not a semester cut but a **design decision**
(§2.3): the game is single-player, and four-client replication of aggregate swarm state is not
a thing this project has to prove at all. The design keeps the door open at zero cost rather
than walking through it — a stance is one intent enum per army rather than per unit, and the
leash clusters units on their hero, which is exactly what would make replication relevancy
tractable if co-op ever returns.

---

## 6. Appendix — Build Status

What separates this document from the first draft is that most of the following column three
can be checked by running the project rather than by trusting the prose.

| System | State | Evidence |
|---|---|---|
| Four stances + hero movement | **Built** | Playable on keys 1–4; also drivable from console for scripted runs |
| Leash — radius, hysteresis, warn bit, break-to-Follow | **Built** | Per-unit latch; live broken count on the HUD |
| Mass Entity swarm — grid, steering, separation | **Built** | Measured: 120 of 120 units at distinct positions, 86uu median spacing at rest, no collision system involved |
| Discrete-swing combat, hit flash, knockback | **Built** | Tuned across five measured model variants (§2.2b) |
| Three-wave run with win and lose states | **Built** | Zero-input baseline loses repeatably (§3.4) |
| Four-value palette + post-process dither | **Built** | Locked ramp; documented exceptions |
| Niagara sprite rendering | **Repaired, unmeasured** | Root cause was an emitter simulating on GPU where it needed CPU; no performance baseline yet |
| Leash rendered as light | Specced | §2.1 — next build item |
| Upkeep economy | Specced | §3.5 |
| Decision events | Specced | Templates discarded with the lore; the resolution rule survives |
| Procedural generation, loot, persistence, networking | Out of scope | §5.2 |

---

*All measurements cited were produced by the project's own in-engine benchmark harness on the
current development branch, single client.*
