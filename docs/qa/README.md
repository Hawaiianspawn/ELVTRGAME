# Adversarial QA agent — Kindled (Godot build)

Assignment #09. The agent lives inside the game as an autoload, `godot/scripts/Adversary.gd`,
and is inert unless launched with `--adversary=<scene>,<seconds>,<seed>`.

    pwsh Scripts\godot-run.ps1 -Adversary battle,60,7

That runs the real game binary against the real project folder (same code the web export ships),
lets the agent loose for 60 s, then copies `adversary_<seed>.json` / `.csv` / `.png` here and folds
the engine's own `SCRIPT ERROR` lines into the JSON as `engine_errors`.

## How the agent tries to break things

Every 0.6 s it picks one of 15 behaviors (seeded RNG, so a run is repeatable) and holds it:

| behavior | what it does |
|---|---|
| `move_random`, `wall_hug`, `corner_hold` | holds WASD through the real input map, including 2–4 s pinned into a wall or corner |
| `cursor_warp`, `siphon_hold` | mouse off-screen / hold RMB siphon with the cursor anywhere |
| `spell_spam`, `swap_spam` | cast Z/X/C or cycle the army (Q/E) **every frame** |
| `teleport_hero` | writes the hero far outside the hall, then watches the clamp |
| `magic_flood`, `magic_drain` | 1e6 magic (relic thresholds) / 0 magic then cast everything (cost gate) |
| `horde` | drops 60 enemies right on the front line |
| `launch_all` | knocks every unit on the field into the air |
| `kill_front`, `kill_hero` | instakill the whole front line / set hero HP to 0 (lose path) |
| `jump_phase` | dev-jumps to a random phase mid-fight (scene swap while things are in flight) |

"Broken" is defined as a rule the game's own code claims and then fails to keep. Checked every frame:
hero inside its clamp, `Game.magic >= 0`, unit `hp` in `0..max_hp`, `dead` flag matches `hp`, `front[]`
lanes point at live units that agree on their lane, `pool >= 0`, every unit between the hall walls
(`|wx| <= HALL_HALF`) and inside `0..SPAWN_D+200` depth, `air_h >= 0`, no NaN, transient states end
inside a budget (`RETREAT` 12 s, `ADVANCE` 15 s, charge 10 s, airborne 15 s), `< 1500` units,
`fps >= 20`, and `Engine.time_scale` back to 1 within a second. Repeats of the same
(type, system, unit type) are folded into one finding with a `count`.

Report fields: `error_type`, `location` {scene, wave, system, unit_type, team, state, wx, wd},
`game_context` {behavior, magic, hero_hp, army_type, units, enemies, fps, held_inputs}, `detail`, `t`, `count`.

## What it found (seeds 7, 11, 23 — 60 s each)

1. **Rank block sits through the hall wall** — `Army.slots` (`godot/scripts/Army.gd:36`) staggers odd rows by
   half a file: `-180 + 5*72 + 36 = 216`, past `HALL_HALF = 200`. Every wave, every ally type in the outer
   file (veteran, hammer, halberdier, vet_ranged) stands 9–21 units inside the drawn wall. Reproduced in all
   three runs from `t=0.8`. Fix: subtract the stagger from `half` (or shrink `RANK_HALF` by `dx/2`).
2. **Hit-stop exploit** — `Battle._hitstop` (`godot/scripts/Battle.gd:382`) sets `Engine.time_scale = 0.05`
   and every army swap re-arms it. Mashing Q/E holds the whole world at 5 % speed for as long as the player
   keeps mashing; the agent measured it held for the full spam tick every time. Fix: rate-limit swaps, or
   don't re-arm hit-stop while one is live.
3. **Slash tween outlives its unit** — `SCRIPT ERROR: Invalid access to property or key 'team' on a base object
   of type 'Nil'` at `Battle.gd:332`, 20× in seed 7 (plus 77 `Lambda capture ... was freed`). The slash
   effect's tween callbacks capture attacker `a` and target `t`; when either dies/gets freed mid-clip
   (kill_front, wave reset, swap) the lambda runs on null. Fix: `if not is_instance_valid(a): return` at the
   top of the callback.
4. **Perpetual juggling** — seed 23, wave 3: seven unit types (both teams) airborne for 15 s+. `Unit.take`
   adds `POP` on every hit while airborne and `_slash`/`_vortex` re-launch airborne foes, so a crowd under a
   whirl field never lands. Allies caught in it can't retreat or advance (`air_h > 0` branch skips movement).
5. **Enemy walks behind the lens** — seed 7: a `mace_undead` at depth `-0` after a horde. Enemies that push
   past the hero keep walking toward `wd = 0` and beyond instead of being recalled or hitting the hero.

Not found (the invariants held): hero clamp (teleporting 3000 units out snaps back next frame), magic never
went negative through the cost gate, no `pool` underflow, no duplicate/dead `front[]` refs, no NaN,
no fps collapse even with 4× hordes on top of each other.

## Was I surprised?

Yes, twice. The wall bug is in the very first second of a normal game and had been in every screenshot for
weeks — the walls are drawn at `HALL_HALF` and the outer file just stands in them; nobody looked because the
crowd reads fine. And the hit-stop exploit was found by accident: the agent's own clock crawled because the
game slowed *itself* down under swap spam, which is exactly the kind of thing a player mashing keys will do
on the first try. The slash-tween null access was the expected one — captured object lifetimes in tween
callbacks are a known Godot foot-gun — but 20 hits in a minute says it fires in ordinary play too, not just
under abuse. The floating crowd was not on the list at all; the 15 s budget was a guess and it tripped.
