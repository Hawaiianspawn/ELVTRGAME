---
id: 101
title: Reconcile supply capacity with retinue size, and make provision bind
status: proposed
agent: gameplay-director
model: ""
owns:
  - "docs/data/economy.json"
  - "docs/data/growth-sites.json"
  - "docs/design/retinue-economy.md"
resources: []
depends-on: []
epic: ""
evidence: >
  A reconciled `start_capacity` (or an explicit written decision that a 120-unit
  retinue is *meant* to open degraded, with the design reason), plus a re-run of
  `py Scripts/sim/run_sim.py run-slice-three-wave` and `py Scripts/sim/decisions.py`
  showing whether the run now reaches growth-A at all and whether `provision`
  stops being byte-identical to spending nothing.
score: {feel: 3, risk: 2, cost: 1}
source: task-096 + task-097 handbacks, 2026-07-30
---

## Why now

`docs/data/economy.json` sets `supply.start_capacity` to 60. GATE1's retinue
convention — and every wave scenario built on it — is 120 units. Upkeep demand
is therefore **2x capacity before a single blow lands**, so the retinue fights
the entire run at the floor-adjacent 0.50 DPS degrade multiplier from tick zero.
Two documents written independently; task-096 was the first thing that ever
combined them.

Task-097 then showed this is worse than "makes a losing wave lose harder": with
`MaxAttackersPerUnit=1` — a combat configuration `docs/sim/VALIDATION.md`
already has on record as winning outright — the run **still wipes**, purely on
the supply collision. It can sink an independently-winning fight on its own.

Downstream, it makes the game's first real decision inert. `docs/sim/DECISIONS.md`
returns **THEATRE** at both growth-site stops: the run wipes before growth-A is
reached, so all five allocation branches are byte-identical. Force the run
survivable and `provision` is *still* exactly inert — byte-identical to spending
nothing — because retinue headcount crashes to ~32% of capacity by wave 2
regardless of branch, so the capacity raise never binds on any tested path.

## Done when

- `start_capacity` and the retinue-size convention agree, **or** there is a
  written decision that opening degraded is intentional, with the design reason
  stated. Either is a valid answer; silence is not.
- `provision` (+25 capacity, 10 embers) either binds somewhere in a producible
  run, or is re-costed / re-scoped / cut with a stated reason. A lane that is
  provably byte-identical to spending nothing is not a lane.
- `growth-sites.json`'s greedy afford-check is looked at: task-097 found the
  `triangle` allocation *underperforms `promote` alone while spending more*,
  because the greedy check drops the priciest action to fit two cheaper ones.
  Either the ordering rule changes or the costs do.
- Re-run `py Scripts/sim/run_sim.py run-slice-three-wave` and `py
  Scripts/sim/decisions.py` and report whether the THEATRE verdict moves.
  **Do not edit anything under `Scripts/sim/`** — read `docs/sim/RUN-SIM.md`
  and `docs/sim/DECISIONS.md` first, and note that every wave number inherits
  `docs/sim/LIMITATIONS.md` §1.
- The one signal worth preserving through any retune, from task-097: with
  headcount held *exactly* equal entering wave 2 (38.2 both), `promote` exits
  with 12x the survivors of `hoard` (8.48 vs 0.66). Quality-over-quantity is
  real and measurable in this model — do not flatten it while fixing the rest.
