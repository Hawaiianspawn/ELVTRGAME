# task-181 tank loop QA

Checked: siege cut clean (`Game.SCENES` has no "siege", `Jump.PHASES` is 4 halls, `Main.gd`
routes `advance` -> battle); hall 1 opens on the `gate_open.png` swing then the fodder rush
(waves.json wave 1: 120 ooze/undead, burst=8 every 0.3s); LMB gatling screen-picks a grounded
enemy and launches it; RMB cannon arcs to the cursor and blast-damages/launches a cluster;
an upgrade chest pop grants the next upgrade in the cycle (gatling rate first).

`godot-run.ps1 -Probe "battle,10,tank"`:
    PROBE tank gatling ok: launched undead air_h=1.3
    PROBE tank cannon ok: 14/100 killed or launched
    PROBE tank chest ok: gatling_rate_mult 1.00 -> 1.25
    PROBE tank ok: siege removed, gatling/cannon/chest all landed
    PROBE battle fps=565 process_ms=3.3 units=51 objects=360

Cannon note: CANNON_DMG=40 one-shots hall-1 fodder (ooze hp 30, undead hp 40) before the
`launch()` call runs, and `launch()` correctly no-ops on an already-dead unit -- so the
self-check counts a kill as "affected" too, not just a launch. `launch(230)` only visibly
juggles a survivor against tougher enemies (later halls).

`godot-run.ps1 -Adversary "battle,30,1"`, run twice: 1 finding both times (pre-existing,
not mine -- `boundary_break @Battle.depth`, an ally halberdier's `wd` goes to -1 in RANK
state during `swap_spam`; lives in Unit.gd/Army.gd, both off-limits this task) plus 1 benign
engine error (`RenderingServer::get_singleton() is null` at shutdown teardown, seen in every
probe run this session, unrelated to this change). `gatling_spam`/`cannon_spam`/`upgrade_flood`
(replacing `spell_spam`/`magic_flood`/`magic_drain`) all ran with no new findings.
