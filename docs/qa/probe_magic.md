# task-179: siphon removal — probe magic readings

`powershell -File Scripts/godot-run.ps1 -Probe battle,30`, HUD `MAGIC n (lifetime n)` read from
`user://probe_battle.png` each run.

| Run | MAGIC (lifetime) | Notes |
|---|---|---|
| BEFORE (siphon mechanic live) | 51 (51) | Brief expected 0 — "nothing collects unattended" since the probe never presses RMB. Measured 51 instead: `Battle.gd:656` (`p.distance_to(hero) < 22.0`) collected motes unconditionally, not gated on `siphoning`. A mote drifting on its own downward drift (`vel = Vector2(0, -22.0)`) could still pass within 22 units of the stationary hero and get credited. Recorded as measured, not force-fit to the stated expectation. |
| AFTER (kill-credit, siphon removed) | 1355 (1355) | Kills credit `Game.gain_magic` directly at death (`Battle.gd:_on_died`); no RMB or cursor proximity involved. All five relics already unlocked (ember_ring, iron_hymn, echo_bell, green_tooth, witch_lamp) within the 30s window. |

Ambient trickle income (the old `magic_rate` ground-vein motes) is gone by design — the task called
for kill-credit only, no replacement passive income.

## Per-enemy economy (magic = round(6 + 0.08 * hp), exactly what a mote used to be worth on that kill)

| type | hp | magic |
|---|---|---|
| ooze | 30 | 8 |
| undead | 40 | 9 |
| mace_undead | 50 | 10 |
| staff_undead | 40 | 9 |
| bow_undead | 25 | 8 |
| archer_undead | 25 | 8 |
| horse_undead | 70 | 12 |
| armored | 160 | 19 |

Per-kill payout is unchanged from the old mote value; the only economy change is that a kill now
credits magic to the hero immediately and unconditionally instead of dropping a mote that had to be
walked over/siphoned (and could otherwise drift away and be lost).
