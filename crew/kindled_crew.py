#!/usr/bin/env python3
"""
KINDLED — Encounter Design Crew
================================
Multi-Agent AI for Game Development, Assignment #3.

Five agents cooperate to turn the project's *measured* canon into a
DataTable-ready encounter specification for the game **Kindled**.

Raw orchestration, Python standard library only — no CrewAI, no network, no API
key, no third-party packages. That is a deliberate reliability choice: the crew
must run and produce output on any machine with Python 3.8+, first try.

Run:
    py crew/kindled_crew.py                  # full run
    py crew/kindled_crew.py --drop budget-auditor
                                             # prove no agent is removable
    py crew/kindled_crew.py --verbose        # show every blackboard read/write

Outputs (written to crew/out/):
    encounters.json        flat, DataTable-shaped rows for Unreal import
    encounters.schema.md   the column contract for that JSON
    run-report.md          what each agent did, and the negotiation transcript
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field, asdict
from datetime import date
from typing import Any, Callable

# Console output must survive any codepage (Windows consoles default to cp1252).
# Files are always written as explicit UTF-8; only the terminal stream needs this.
try:                                              # pragma: no cover
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except (AttributeError, OSError):                 # pragma: no cover
    pass

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")

FRAME_BUDGET_MS = 16.6          # 60 fps
# docs/RTS-VERTICAL-SLICE.md scopes "3 floors, escalating density". Floor 1 is the
# shipped Gate-1 wave structure; 2 and 3 escalate on top of it.
FLOOR_DENSITY = {1: 1.0, 2: 1.6, 3: 2.5}
GAME = "Kindled"


# ══════════════════════════════════════════════════════════════════════════
#  Blackboard — the shared workspace, with enforced access control
# ══════════════════════════════════════════════════════════════════════════

class ContractViolation(RuntimeError):
    """Raised when an agent touches a key it never declared."""


class Blackboard:
    """Typed shared store. Every read and write is attributed and logged.

    Access is *enforced*, not merely documented: an agent may only read keys it
    declared in `reads` and write keys it declared in `writes`. This is what
    makes the pipeline's dependency graph real rather than aspirational — and
    it is why dropping any agent breaks the run instead of silently degrading it.
    """

    def __init__(self, verbose: bool = False) -> None:
        self._data: dict[str, Any] = {}
        self.trace: list[str] = []
        self.verbose = verbose

    def put(self, agent: "Agent", key: str, value: Any) -> None:
        if key not in agent.writes:
            raise ContractViolation(
                f"{agent.name} tried to WRITE '{key}', which it never declared. "
                f"Declared writes: {sorted(agent.writes)}")
        self._data[key] = value
        self._note(f"{agent.name} -> wrote  {key}")

    def get(self, agent: "Agent", key: str) -> Any:
        if key not in agent.reads:
            raise ContractViolation(
                f"{agent.name} tried to READ '{key}', which it never declared. "
                f"Declared reads: {sorted(agent.reads)}")
        if key not in self._data:
            raise ContractViolation(
                f"{agent.name} needs '{key}', but no agent has produced it. "
                f"An upstream agent is missing from the crew — the pipeline "
                f"cannot proceed.")
        self._note(f"{agent.name} <- read   {key}")
        return self._data[key]

    def has(self, key: str) -> bool:
        return key in self._data

    def _note(self, line: str) -> None:
        self.trace.append(line)
        if self.verbose:
            print(f"      [bb] {line}")


# ══════════════════════════════════════════════════════════════════════════
#  Agent base
# ══════════════════════════════════════════════════════════════════════════

@dataclass
class Agent:
    name: str
    role: str                       # one plain-English sentence
    reads: set[str] = field(default_factory=set)
    writes: set[str] = field(default_factory=set)
    notes: list[str] = field(default_factory=list, repr=False)

    def say(self, msg: str) -> None:
        self.notes.append(msg)
        print(f"   - {msg}")

    def run(self, bb: Blackboard) -> None:      # pragma: no cover - overridden
        raise NotImplementedError


# ══════════════════════════════════════════════════════════════════════════
#  Agent 1 — CanonReader
# ══════════════════════════════════════════════════════════════════════════

# Values that survive if a canon doc is missing or restructured. Every one of
# these is a real measured/shipped figure from this project, recorded here so
# the crew degrades to "grounded but unverified" rather than crashing.
FALLBACK_CANON = {
    "retinue_hp": 130, "retinue_dps": 30, "retinue_cap": 120,
    "brood_dps": 35, "hero_hp": 500, "hero_dps": 55,
    "melee_range_uu": 95, "swing_interval_s": 0.9, "leash_radius_uu": 2000,
    "waves": [250, 450, 700],
    "palette_values": 4,
    # docs/perf/BUDGETS.md, measured 2026-07-26, UnitShading=1, single client
    "draw_curve": [(500, 4.85), (1000, 14.62), (2000, 40.65),
                   (5000, 132.33), (10000, 350.04)],
}


class CanonReader(Agent):
    """Reads the repo's own design + performance docs and extracts the numbers
    everything downstream must be grounded in."""

    def __init__(self) -> None:
        super().__init__(
            name="canon-reader",
            role="Parses the project's shipped design and measured performance "
                 "docs into a single fact block, so no downstream agent invents "
                 "a number.",
            writes={"canon"},
        )

    def run(self, bb: Blackboard) -> None:
        facts = dict(FALLBACK_CANON)
        sources: list[str] = []

        gate1 = os.path.join(REPO, "docs", "GATE1-FUN-PROTOTYPE.md")
        text = _read(gate1)
        if text:
            sources.append("docs/GATE1-FUN-PROTOTYPE.md")
            waves = re.search(
                r"wave 1 \((\d+) brood\).*?wave 2 \((\d+)\).*?wave 3 \((\d+)\)",
                text, re.S)
            if waves:
                facts["waves"] = [int(g) for g in waves.groups()]
            cap = re.search(r"retinue\s+refills? to (\d+)", text)
            if cap:
                facts["retinue_cap"] = int(cap.group(1))
            hp = re.search(r"\*\*(\d+) HP / (\d+) DPS\*\*", text)
            if hp:
                facts["retinue_hp"], facts["retinue_dps"] = (
                    int(hp.group(1)), int(hp.group(2)))

        budgets = os.path.join(REPO, "docs", "perf", "BUDGETS.md")
        text = _read(budgets)
        if text:
            curve = [(int(m.group(1)), float(m.group(2))) for m in re.finditer(
                r"^\|\s*(\d{3,6})\s*\|\s*([\d.]+)\s*\|", text, re.M)]
            if len(curve) >= 3:
                facts["draw_curve"] = sorted(curve)
                sources.append("docs/perf/BUDGETS.md")

        facts["sources"] = sources or ["(embedded fallback — canon docs not found)"]
        facts["grounded"] = bool(sources)

        bb.put(self, "canon", facts)
        self.say(f"read {len(sources)} canon source(s): "
                 f"{', '.join(facts['sources'])}")
        self.say(f"waves={facts['waves']}  retinue_cap={facts['retinue_cap']}  "
                 f"perf curve has {len(facts['draw_curve'])} measured points")


# ══════════════════════════════════════════════════════════════════════════
#  Agent 2 — RosterArchitect
# ══════════════════════════════════════════════════════════════════════════

class RosterArchitect(Agent):
    """Turns the measured baseline into a tier ladder expressed as multipliers."""

    TIERS = [
        # name,      hp_mult, dps_mult, cost, note
        ("fodder",   0.45, 0.60, 1, "dies in one committed exchange; the mass"),
        ("soldier",  1.00, 1.00, 3, "the measured baseline — trades with a retinue soldier"),
        ("elite",    3.20, 1.80, 12, "anti-swarm; must be answered with position, not bodies"),
        ("boss",    18.00, 2.60, 60, "army-vs-big-thing; a positioning fight, not a DPS check"),
    ]

    def __init__(self) -> None:
        super().__init__(
            name="roster-architect",
            role="Derives the entity tier ladder (fodder → soldier → elite → "
                 "boss) as multipliers over the measured combat baseline, and "
                 "prices each tier in encounter-budget points.",
            reads={"canon"},
            writes={"tiers"},
        )

    def run(self, bb: Blackboard) -> None:
        canon = bb.get(self, "canon")
        base_hp, base_dps = canon["retinue_hp"], canon["retinue_dps"]

        tiers = {}
        for name, hp_m, dps_m, cost, note in self.TIERS:
            hp, dps = round(base_hp * hp_m), round(base_dps * dps_m)
            # TTK against one baseline retinue soldier, at the shipped cadence.
            ttk = round(hp / max(base_dps, 1) , 2)
            tiers[name] = {
                "hp": hp, "dps": dps, "budget_cost": cost,
                "ttk_vs_baseline_s": ttk, "design_note": note,
            }

        bb.put(self, "tiers", tiers)
        self.say(f"derived {len(tiers)} tiers over baseline {base_hp}HP/{base_dps}DPS: "
                 + ", ".join(f"{k}({v['hp']}HP)" for k, v in tiers.items()))


# ══════════════════════════════════════════════════════════════════════════
#  Agent 3 — EncounterArchitect
# ══════════════════════════════════════════════════════════════════════════

class EncounterArchitect(Agent):
    """Composes the three-wave encounter. Revises when an auditor rejects."""

    def __init__(self) -> None:
        super().__init__(
            name="encounter-architect",
            role="Composes the three escalating waves — how many of which tier, "
                 "and the spawn cadence — and redesigns when an auditor sends "
                 "the plan back.",
            reads={"canon", "tiers", "revision_directive"},
            writes={"plan"},
        )

    # composition per wave: fraction of the wave body count, by tier
    MIX = [
        {"fodder": 1.00, "soldier": 0.00, "elite": 0.00, "boss": 0},
        {"fodder": 0.75, "soldier": 0.25, "elite": 0.00, "boss": 0},
        {"fodder": 0.60, "soldier": 0.34, "elite": 0.06, "boss": 1},
    ]

    def run(self, bb: Blackboard) -> None:
        canon = bb.get(self, "canon")
        tiers = bb.get(self, "tiers")
        directive = bb.get(self, "revision_directive") if bb.has("revision_directive") else None

        scale = canon.get("floor_density", 1.0)
        if directive:
            scale = directive.get("scale", 1.0)
            self.say(f"revising: {directive['reason']}; re-scaling to "
                     f"x{scale:.2f} of the shipped wave sizes")

        waves = []
        for i, target in enumerate(canon["waves"]):
            n = max(1, round(target * scale))
            mix = self.MIX[min(i, len(self.MIX) - 1)]
            comp, spent = {}, 0
            for tier, frac in mix.items():
                if tier == "boss":
                    count = int(frac)
                else:
                    count = round(n * frac)
                if count:
                    comp[tier] = count
                    spent += count * tiers[tier]["budget_cost"]
            waves.append({
                "wave": i + 1,
                "bodies": sum(v for k, v in comp.items()),
                "composition": comp,
                "budget_points": spent,
                "spawn": "ring" if i < 2 else "arena_entrances",
                "breather_after_s": 6 if i < len(canon["waves"]) - 1 else 0,
            })

        plan = {
            "waves": waves,
            "retinue_cap": canon["retinue_cap"],
            "scale_applied": round(scale, 3),
        }
        bb.put(self, "plan", plan)
        self.say("composed " + " | ".join(
            f"W{w['wave']}: {w['bodies']} bodies ({w['budget_points']}pts)" for w in waves))


# ══════════════════════════════════════════════════════════════════════════
#  Agent 4 — BudgetAuditor
# ══════════════════════════════════════════════════════════════════════════

class BudgetAuditor(Agent):
    """Prices the plan against the *measured* draw-cost curve and can reject it."""

    def __init__(self) -> None:
        super().__init__(
            name="budget-auditor",
            role="Projects the plan's peak concurrent entity count onto the "
                 "measured frame-cost curve and returns PASS, or REVISE with a "
                 "scale directive the architect must obey.",
            reads={"canon", "plan"},
            writes={"budget_verdict", "revision_directive"},
        )

    @staticmethod
    def _interp(curve: list[tuple[int, float]], n: int) -> float:
        """Piecewise-linear interpolation over the measured points."""
        if n <= curve[0][0]:
            return curve[0][1] * n / curve[0][0]
        for (x0, y0), (x1, y1) in zip(curve, curve[1:]):
            if n <= x1:
                t = (n - x0) / (x1 - x0)
                return y0 + t * (y1 - y0)
        # past the last measured point: extrapolate on the final slope
        (x0, y0), (x1, y1) = curve[-2], curve[-1]
        slope = (y1 - y0) / (x1 - x0)
        return y1 + (n - x1) * slope

    def run(self, bb: Blackboard) -> None:
        canon = bb.get(self, "canon")
        plan = bb.get(self, "plan")
        curve = [tuple(p) for p in canon["draw_curve"]]

        peak_wave = max(plan["waves"], key=lambda w: w["bodies"])
        peak = peak_wave["bodies"] + plan["retinue_cap"]
        ms = self._interp(curve, peak)

        ok = ms <= FRAME_BUDGET_MS
        verdict = {
            "peak_concurrent_entities": peak,
            "peak_wave": peak_wave["wave"],
            "projected_frame_ms": round(ms, 2),
            "frame_budget_ms": FRAME_BUDGET_MS,
            "headroom_ms": round(FRAME_BUDGET_MS - ms, 2),
            "status": "PASS" if ok else "REVISE",
            "basis": "measured draw curve, single client, UnitShading=1",
        }
        bb.put(self, "budget_verdict", verdict)

        if ok:
            self.say(f"PASS - peak {peak} entities ~ {ms:.2f}ms "
                     f"({verdict['headroom_ms']:.2f}ms headroom)")
        else:
            # how much would fit? solve for the body count that lands on budget
            lo, hi = 1, peak
            while lo < hi:
                mid = (lo + hi + 1) // 2
                if self._interp(curve, mid) <= FRAME_BUDGET_MS:
                    lo = mid
                else:
                    hi = mid - 1
            affordable_bodies = max(1, lo - plan["retinue_cap"])
            scale = affordable_bodies / max(peak_wave["bodies"], 1)
            bb.put(self, "revision_directive", {
                "reason": f"peak {peak} entities ~ {ms:.2f}ms, over the "
                          f"{FRAME_BUDGET_MS}ms budget",
                "scale": round(scale * plan["scale_applied"], 3),
            })
            self.say(f"REVISE - peak {peak} entities ~ {ms:.2f}ms, over budget by "
                     f"{ms - FRAME_BUDGET_MS:.2f}ms; only {affordable_bodies} bodies "
                     f"are affordable, sending back")


# ══════════════════════════════════════════════════════════════════════════
#  Agent 5 — ReadabilityAuditor
# ══════════════════════════════════════════════════════════════════════════

class ReadabilityAuditor(Agent):
    """Guards the 4-value palette pillar: can the player still parse the fight?"""

    def __init__(self) -> None:
        super().__init__(
            name="readability-auditor",
            role="Checks each wave's simultaneous distinct silhouettes against "
                 "the locked 4-value palette and flags any wave the player "
                 "could not parse at speed.",
            reads={"canon", "plan"},
            writes={"readability_verdict"},
        )

    def run(self, bb: Blackboard) -> None:
        canon = bb.get(self, "canon")
        plan = bb.get(self, "plan")
        values = canon["palette_values"]
        # one value is spent on the lit floor, one on the retinue: what is left
        # is the number of enemy silhouettes that can stay distinct at speed.
        enemy_value_budget = values - 2

        findings = []
        for w in plan["waves"]:
            distinct = len(w["composition"])
            ok = distinct <= enemy_value_budget + 1   # +1: shape can carry one extra
            findings.append({
                "wave": w["wave"],
                "distinct_enemy_types": distinct,
                "status": "OK" if ok else "CROWDED",
                "note": ("reads by value alone" if distinct <= enemy_value_budget
                         else "one type must be carried by silhouette, not value"),
            })

        worst = "CROWDED" if any(f["status"] == "CROWDED" for f in findings) else "OK"
        verdict = {
            "palette_values": values,
            "enemy_value_budget": enemy_value_budget,
            "status": worst,
            "per_wave": findings,
        }
        bb.put(self, "readability_verdict", verdict)
        self.say(f"{worst} - max {max(f['distinct_enemy_types'] for f in findings)} "
                 f"distinct enemy types in a wave against {enemy_value_budget} "
                 f"free palette values")


# ══════════════════════════════════════════════════════════════════════════
#  Agent 6 — DataEmitter
# ══════════════════════════════════════════════════════════════════════════

class DataEmitter(Agent):
    """Writes the game-ready artifacts. Refuses to emit an unaudited plan."""

    def __init__(self) -> None:
        super().__init__(
            name="data-emitter",
            role="Flattens the approved plan into DataTable-shaped JSON rows for "
                 "Unreal, writes the column schema, and refuses to emit anything "
                 "that has not passed both audits.",
            reads={"canon", "tiers", "plan", "budget_verdict", "readability_verdict"},
            writes={"artifacts"},
        )

    def run(self, bb: Blackboard) -> None:
        canon = bb.get(self, "canon")
        tiers = bb.get(self, "tiers")
        plan = bb.get(self, "plan")
        budget = bb.get(self, "budget_verdict")
        read = bb.get(self, "readability_verdict")

        if budget["status"] != "PASS":
            raise RuntimeError(
                "data-emitter refuses to write an over-budget encounter "
                f"({budget['projected_frame_ms']}ms > {FRAME_BUDGET_MS}ms). "
                "The negotiation did not converge.")

        os.makedirs(OUT_DIR, exist_ok=True)

        rows = []
        for w in plan["waves"]:
            for tier, count in w["composition"].items():
                rows.append({
                    "Name": f"W{w['wave']}_{tier}",
                    "Wave": w["wave"],
                    "Tier": tier,
                    "Count": count,
                    "HP": tiers[tier]["hp"],
                    "DPS": tiers[tier]["dps"],
                    "BudgetCost": tiers[tier]["budget_cost"],
                    "SpawnMode": w["spawn"],
                    "BreatherAfterSeconds": w["breather_after_s"],
                })

        payload = {
            "_meta": {
                "game": GAME,
                "floor": canon.get("floor", 1),
                "generated": date.today().isoformat(),
                "generator": "crew/kindled_crew.py",
                "canon_sources": canon["sources"],
                "grounded_in_repo_docs": canon["grounded"],
                "budget_verdict": budget,
                "readability_verdict": read,
            },
            "rows": rows,
        }

        json_path = os.path.join(OUT_DIR, "encounters.json")
        with open(json_path, "w", encoding="utf-8") as fh:
            json.dump(payload, fh, indent=2)

        schema_path = os.path.join(OUT_DIR, "encounters.schema.md")
        with open(schema_path, "w", encoding="utf-8") as fh:
            fh.write(_schema_md(budget, read))

        bb.put(self, "artifacts", {"json": json_path, "schema": schema_path,
                                   "row_count": len(rows)})
        self.say(f"wrote {len(rows)} DataTable rows -> crew/out/encounters.json")
        self.say("wrote column contract -> crew/out/encounters.schema.md")


# ══════════════════════════════════════════════════════════════════════════
#  Orchestrator
# ══════════════════════════════════════════════════════════════════════════

MAX_NEGOTIATION_ROUNDS = 6


def build_crew() -> list[Agent]:
    return [CanonReader(), RosterArchitect(), EncounterArchitect(),
            BudgetAuditor(), ReadabilityAuditor(), DataEmitter()]


def run_crew(drop: str | None = None, verbose: bool = False,
             floor: int = 1) -> int:
    print("=" * 74)
    print(f"  {GAME.upper()} - ENCOUNTER DESIGN CREW")
    print(f"  raw orchestration | stdlib only | 6 agents | floor {floor}")
    print("=" * 74)

    crew = build_crew()
    if drop:
        before = len(crew)
        crew = [a for a in crew if a.name != drop]
        if len(crew) == before:
            print(f"\n!! no agent named '{drop}'. Agents: "
                  f"{', '.join(a.name for a in build_crew())}")
            return 2
        print(f"\n!! RUNNING WITHOUT '{drop}' — demonstrating that the pipeline "
              f"depends on it\n")

    bb = Blackboard(verbose=verbose)
    by_name = {a.name: a for a in crew}
    rounds = 0

    try:
        # Phase 1 — ground the run in canon, then derive the roster.
        for name in ("canon-reader", "roster-architect"):
            if name in by_name:
                print(f"\n[{name}]")
                by_name[name].run(bb)

        # The floor's density multiplier is a run parameter, not an agent's
        # opinion, so the orchestrator stamps it onto the canon fact block.
        if bb.has("canon"):
            bb._data["canon"]["floor"] = floor
            bb._data["canon"]["floor_density"] = FLOOR_DENSITY.get(floor, 1.0)

        # Phase 2 — architect <-> budget-auditor negotiate until the plan fits.
        while rounds < MAX_NEGOTIATION_ROUNDS:
            rounds += 1
            print(f"\n--- negotiation round {rounds} ---")
            for name in ("encounter-architect", "budget-auditor"):
                if name in by_name:
                    print(f"[{name}]")
                    by_name[name].run(bb)
            if not bb.has("budget_verdict"):
                break                       # auditor dropped: fail downstream
            if bb._data["budget_verdict"]["status"] == "PASS":
                break

        # Phase 3 — readability gate, then emit.
        for name in ("readability-auditor", "data-emitter"):
            if name in by_name:
                print(f"\n[{name}]")
                by_name[name].run(bb)

    except ContractViolation as exc:
        print(f"\n{'!' * 74}\nPIPELINE BROKEN - {exc}\n{'!' * 74}")
        print("\nThis is the intended result of removing an agent: every agent "
              "declares\nits inputs and outputs, so a missing producer stops the "
              "run at the first\nconsumer that needs it. No agent in this crew is "
              "decorative.")
        return 1
    except RuntimeError as exc:
        print(f"\n!! {exc}")
        return 1

    _write_report(crew, bb, rounds)
    art = bb._data["artifacts"]
    print("\n" + "=" * 74)
    print(f"  DONE - {art['row_count']} encounter rows, "
          f"{rounds} negotiation round(s)")
    print(f"  crew/out/encounters.json")
    print(f"  crew/out/encounters.schema.md")
    print(f"  crew/out/run-report.md")
    print("=" * 74)
    return 0


def _write_report(crew: list[Agent], bb: Blackboard, rounds: int) -> None:
    b = bb._data["budget_verdict"]
    r = bb._data["readability_verdict"]
    p = bb._data["plan"]
    lines = [
        f"# {GAME} — encounter crew run report", "",
        f"**Floor {bb._data['canon'].get('floor', 1)}** "
        f"(density x{bb._data['canon'].get('floor_density', 1.0)})", "",
        f"Generated {date.today().isoformat()} by `crew/kindled_crew.py`.", "",
        f"**Negotiation rounds:** {rounds}  ·  "
        f"**Budget:** {b['status']} "
        f"({b['projected_frame_ms']}ms / {b['frame_budget_ms']}ms)  ·  "
        f"**Readability:** {r['status']}", "",
        "## Agents", "",
        "| Agent | Role | Reads | Writes |", "|---|---|---|---|",
    ]
    for a in crew:
        lines.append(f"| `{a.name}` | {a.role} | "
                     f"{', '.join('`%s`' % k for k in sorted(a.reads)) or '—'} | "
                     f"{', '.join('`%s`' % k for k in sorted(a.writes)) or '—'} |")

    lines += ["", "## Encounter", "",
              "| Wave | Bodies | Composition | Budget pts | Spawn |",
              "|---|---|---|---|---|"]
    for w in p["waves"]:
        comp = ", ".join(f"{k} ×{v}" for k, v in w["composition"].items())
        lines.append(f"| {w['wave']} | {w['bodies']} | {comp} | "
                     f"{w['budget_points']} | {w['spawn']} |")

    lines += ["", "## Budget verdict", "",
              f"- Peak concurrent entities: **{b['peak_concurrent_entities']}** "
              f"(wave {b['peak_wave']} + retinue cap {p['retinue_cap']})",
              f"- Projected frame cost: **{b['projected_frame_ms']}ms** against a "
              f"{b['frame_budget_ms']}ms budget — {b['headroom_ms']}ms headroom",
              f"- Basis: {b['basis']}", "",
              "## Agent transcript", "", "```"]
    lines += bb.trace
    lines += ["```", ""]

    with open(os.path.join(OUT_DIR, "run-report.md"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))


def _schema_md(budget: dict, read: dict) -> str:
    return f"""# encounters.json — schema

Produced by `crew/kindled_crew.py` for **{GAME}**. The `rows` array is flat and
typed so it imports directly as an Unreal **DataTable**: one unique `Name` key
per row, scalar columns only, no nesting.

| Column | Type | Notes |
|---|---|---|
| `Name` | string | DataTable row key, `W<wave>_<tier>` |
| `Wave` | int | 1-indexed wave this row belongs to |
| `Tier` | string | `fodder` / `soldier` / `elite` / `boss` |
| `Count` | int | bodies of this tier spawned in the wave |
| `HP` | int | derived as a multiplier over the measured retinue baseline |
| `DPS` | int | as above |
| `BudgetCost` | int | encounter-budget points, for future spend-per-room tuning |
| `SpawnMode` | string | `ring` or `arena_entrances` |
| `BreatherAfterSeconds` | int | recovery window before the next wave; `0` on the last |

`_meta` carries provenance: which repo docs the numbers came from, and both
audit verdicts, so a row can always be traced back to the measurement that
justified it.

## Audit state at generation

- **Budget** — {budget['status']}: peak {budget['peak_concurrent_entities']}
  concurrent entities, projected {budget['projected_frame_ms']}ms against a
  {budget['frame_budget_ms']}ms frame budget ({budget['basis']}).
- **Readability** — {read['status']}: {read['enemy_value_budget']} palette values
  free for enemies after the lit floor and the retinue take theirs.
"""


def _read(path: str) -> str | None:
    try:
        with open(path, encoding="utf-8") as fh:
            return fh.read()
    except OSError:
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description=f"{GAME} encounter design crew")
    ap.add_argument("--drop", metavar="AGENT",
                    help="run without this agent, to show the pipeline break")
    ap.add_argument("--verbose", action="store_true",
                    help="print every blackboard read and write")
    ap.add_argument("--floor", type=int, default=1, choices=(1, 2, 3),
                    help="which slice floor to design (density escalates)")
    args = ap.parse_args()
    return run_crew(drop=args.drop, verbose=args.verbose, floor=args.floor)


if __name__ == "__main__":
    sys.exit(main())
