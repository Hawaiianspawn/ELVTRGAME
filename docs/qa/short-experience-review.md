# Short experience QA — title -> battle (4 halls) — task-186

Scope: `scenes/Main.tscn` -> `scenes/battle/`, four halls (waves 0-3) with jump-phase
transitions, ending on hall 4's necromancer. Reviewed against `godot/README.md` (live
truth), not the retired-Unreal GDD/SYSTEMS.md.

## Method

- Adversary: seeds 31 and 37, `Scripts/godot-run.ps1 -Adversary battle,240,<seed>`, both
  ran the full 240s cleanly on the first try. `jump_phase` fired 22-26 times per run,
  cycling through all four halls many times over (waves picked uniformly at random
  each time). A third seed (41) was cut short by owner call to close the pass early —
  two clean 240s runs was judged enough.
- Stills: `Scripts/godot-run.ps1 -Probe "battle,<secs>,wave=<N>"` for each hall, one
  longer hold (18s) on hall 1 to see it clear. Copied to `docs/qa/short-experience-hall{1,2,3,4}.png`
  and `short-experience-hall1-clash.png`. All four halls are covered.
- Full reports: `docs/qa/adversary_{31,37}.{json,csv,png}` (+ per-finding shots).

**Shared-tree note:** `godot/scripts/Game.gd` and `godot/scripts/Battle.gd` changed
after these stills were captured — owner-requested live edits by the team lead
(a focus-loss audio mute in `Game.gd`; the tank pulled to depth 150 with
`TANK_FOOTPRINT_R` 110 in `Battle.gd`), not part of this review. The four hall stills
below predate that change and show the old tank depth (190). `Unit.gd`, `World3D.gd`,
`assets/sprites/manifest.json`, `assets/sprites/atlas.png`, `Scripts/art/godot_pack.py`,
and `assets/sfx/gatling_shot.wav` were already modified by an unrelated peer session
before this task started and are also not this review's doing.

## Findings

**1. HIGH — Probe.gd:394 crashes and hangs the process (tooling bug, not the game).**
`wds[0]` / `wds[-1]` are indexed without checking `wds` (ally units currently in
`Unit.State.RANK`) is non-empty. When a probe's hold timer expires at a moment where
every ally is in FIGHT/ADVANCE/RETREAT instead of RANK, this throws "Out of bounds
get index '0'", aborts `_run()` before it reaches `img.save_png()` or `get_tree().quit()`
(Probe.gd:399-403), and leaves a zombie Godot window that has to be killed externally.
Reproduced live on the first hall-4 probe attempt (`battle,20,wave=3`): confirmed via
`godot.log` — `SCRIPT ERROR: Out of bounds get index '0' (on base: 'Array') at:
_run (res://scripts/Probe.gd:394)`, process then sat open for minutes doing nothing.
Hall 4's heavier composition (9 lanes, 160hp armored units bogging down fights) makes
allies less likely to be idling in RANK at any given instant, so this hits hall-4
captures hardest, but it can happen in any hall.

**2. MEDIUM — halberdier depth residual-state bug. Repro: seeds 31 and 37, both at
240s.** Both adversary runs logged the *same* finding: an ally **halberdier**, and
only a halberdier, parked at wd -1 to -28 (behind the camera plane; ally floor is
wd >= 0) for a huge share of each run — 36,958 to 68,235 per-frame check hits per run
(seed 31: `RETREAT` state, wd=-28; seed 37: `RANK` state, wd=-1), spanning from a few
seconds in to nearly the run's end each time. Suspect cause: `Unit.gd:214-261`, the
charge/return-leg block that runs *instead of* the normal state machine while
`charge_to > 0` (including the `RETREAT` recall-at-wd<70 check at line 316-320) —
`charge_to` only clears at line 259 once the return leg's `_step(_charge_from, ...)`
reports `not _moving`. If that return leg's target or completion check goes stale
(plausible under concurrent army-swap pressure — `swap_spam` was active in both
finding contexts), a halberdier neither reaches home nor gets recalled, and settles
at a corrupted depth near zero indefinitely.

**3. LOW/cosmetic — airborne ally briefly renders above the frame.** `veteran` allies
launched high enough (air_h 17-67) occasionally have their sprite top clip above
y=0 for a few frames (`Unit.air`, 49-91 hits per 240s run — rare, not sustained).
Minor launch/juggle-height vs. camera-frame mismatch, not a stuck state.

**4. LOW — three of the adversary's fourteen behaviors are dead weight against the
current build.** `move_random`, `wall_hug`, and `corner_hold` hold `move_up/down/left/right`,
but nothing in `Battle.gd` reads those actions to move `hero_wx`/`hero_wd` (the hero
rides mounted on the tank now — grep for `Input.is_action` in `Battle.gd` finds only
the `cast` action). The only thing that can move the hero at all is the adversary's
own `teleport_hero` behavior, and nothing ever clamps it back afterward — so the
"hero clamp" invariant exists purely to catch the adversary's own artificial teleports,
with three of fourteen behaviors (~21% of tick time) contributing nothing.

**Environment note (not a game defect):** other agents in this shared session
repeatedly invoke the same `godot-run.ps1` wrapper, whose kill-previous-instance
logic kills *any* non-editor game process bound to this project — including a long
adversary run mid-flight. That cost several retries during this pass. Not a game
defect; a side effect of several agents sharing one machine's game binary.

The pre-existing `_gatling_hit_at` probe flake (`docs/qa/tank-sprite.md`) was not seen
in these runs and isn't re-litigated here.

## Hall-by-hall read

![Hall 1](short-experience-hall1.png)
![Hall 2](short-experience-hall2.png)
![Hall 3](short-experience-hall3.png)
![Hall 4](short-experience-hall4.png)

- **Hall 1** reads clearly at a glance: hero-tank centered, army massed around it, HUD
  clean. But it clears *fast* — an 18s hold (`short-experience-hall1-clash.png`) was
  enough to see the "victory" stinger fire and the scene auto-advance into hall 2's
  caption, meaning hall 1's ~120 enemies (70 ooze + 50 undead) died and the hall
  turned over in under 20 real seconds including the transition beat. As a first
  encounter, that's barely enough time to notice there's a fight happening before
  it's over.
- **Hall 2** and **Hall 3** are visually almost identical to hall 1 from this camera:
  same blob of allies, same angle, same HUD. The only tell that you've progressed is
  the "HALL N/4" label and swapped background props (a bookshelf in hall 2, a banner
  and crates in hall 3) — the hall narrowing (`HALF_BY_WAVE`: 520 -> 400 -> 280) and
  the OutRun-style wall bend (`CURVE_A_BY_WAVE`) are both real escalation dials in the
  code, but neither reads as an obvious difficulty step in a still frame.
- **Hall 4** is the most visually distinct of the four — noticeably dimmer lighting,
  more floor debris, heavier gray silhouettes from the armored elites. That said, the
  necromancer itself isn't guaranteed to land as a climactic final beat: hall 4's
  spawn queue of 145 enemies (72 armored, 20 undead, 12 wraith, 12 bone_knight, 8
  plague_priest, 1 necromancer, etc.) is built and then `.shuffle()`d
  (`Battle.gd:265-269`), so the single necromancer could spawn anywhere from the first
  few seconds of the hall to the very last. It also carries no unique ability —
  `units.json` gives it the same generic melee attack/cooldown/range shape as every
  other unit, just scaled to 900 HP against a next-highest of 160 (armored). The
  "boss" is a stat check that might not even show up last.
- **Every hall** shares the same HUD legibility problem: the hero dialogue box
  (bottom-left) and the four-card unit-roster row both anchor the bottom edge and
  visibly truncate text mid-sentence in every single capture — "Hammer Knights anchor
  the l...", "Veteran Ranged loose arrows over..." cut off identically in all four
  stills. This is the single most consistent, easiest-to-reproduce readability issue
  found.
- The between-hall beat (`Battle._turn`, Battle.gd:860-868) is a static caption
  ("The hall turns left.") held 2.5s with the camera locked at rest (the swing/tilt
  was deliberately cut per an existing code comment) — it names the turn but doesn't
  visually sell escalation into the next hall.

## Top 5 recommended fixes

1. Guard `wds.is_empty()` before `wds[0]`/`wds[-1]` in `Probe.gd:394` — it currently
   hangs the probe tool with no PNG and no clean exit whenever no ally happens to be
   in RANK state at capture time (hits hall 4 hardest).
2. Chase the halberdier charge/retreat residual-state bug (`Unit.gd:214-261`):
   swap-cycling away from halberdier mid-charge leaves stragglers parked behind the
   camera plane for tens of thousands of frames per run, in both RETREAT and RANK.
3. Stop shuffling the necromancer into hall 4's general spawn queue
   (`Battle.gd:265-269`) — pin it to spawn last so the "boss" actually lands as a
   closing beat instead of appearing at a random point among 145 enemies.
4. Give the necromancer (`units.json`) at least one distinguishing mechanic; right
   now it's a plain 900-HP stat check with the same attack shape as every trash mob.
5. Rework the bottom HUD so the hero dialogue box and the unit-roster row stop
   fighting for the same strip — roster text truncates mid-word in every hall.

Secondary: hall 1's clear time (~18s including the transition) leaves little room to
read the fight before it's over — worth a pacing pass on the opener specifically.
Also worth a pass on `Adversary.gd`'s `move_random`/`wall_hug`/`corner_hold` (and the
hero-clamp invariant), which test hero movement inputs the current `Battle.gd` no
longer wires up to anything.
