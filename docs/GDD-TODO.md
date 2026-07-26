# GDD Fleshing-Out Tracker

**Purpose:** a living checklist of what still needs writing/deciding in the design
docs, triaged **assignment-rubric-first → vertical-slice → full-v1**.

**Scoring constraint (drives Part A):** the GDD is submitted for **Assignment #1 —
First Draft of GDD**, due **21 July 2026, 11:59 PM ET**. Course:
*Multi-Agent AI for Game Development*. The rubric — not design completeness — is the
scoring driver. Key consequence: two required sections (**AI Architecture**,
**Technical Strategy**) are about the **development-time agent system** (the
`.claude/agents/*` subagents + `unreal-mcp` + `pixellab` MCP), described *through
their effect on this specific game* — the current docs never surface this.

Source docs (all rich, none scored yet): `GDD.md`, `CLASSES.md`, `WORLD.md`,
`SYSTEMS.md`, `docs/RTS-VERTICAL-SLICE.md`, `docs/SPIKE1-RESULTS.md`.

---

## Rubric → current-state gap map

| Criterion (pts) | Requires | Current state | Gap |
|---|---|---|---|
| **Game Specificity** /3.0 | Named game, core loop, **win/loss** | Core loop solid (GDD §3); **no name**, **no win/loss** | 🔴 BLOCKER ×2 |
| **Player Experience Clarity** /2.5 | What player *sees/does* | Present but scattered across 4 docs | Assemble player-facing pass |
| **Agent Role Clarity** /2.0 | Each **dev agent** named + 1-sentence role | Agents exist in `.claude/agents/`; GDD never describes them | Missing section |
| **Technical Feasibility** /1.5 | Semester scope, ≥1 constraint, **token budget** | Great tech constraints (GDD §10); **no token budget / API constraints** | Missing |
| **Presentation** /1.0 | 4 sections present, single PDF | Content spread across many `.md` files | Assemble + export PDF |

**Anti-slop gate (from the brief):** generic/placeholder framing, or MAS concepts not
grounded in specific player-facing mechanics, = **zero** on Game Specificity + Player
Experience Clarity regardless of theoretical correctness. Every agent role must be tied
to a concrete thing the player sees or does.

---

## Part A — Assignment-critical (must-fix to score; due tonight)

Organized by the four required sections + presentation.

### Executive Summary
- [ ] Write a standalone Executive Summary (does not exist as one). Source: GDD §1 High
  Concept + the one-liner *"A co-op roguelike dungeon crawler where you don't just build
  a character — you build an army."*

### Game Mechanics (player-facing actions + loop)
- [ ] 🔴 **Name the game** — resolve GDD Open Q #10 / `WORLD.md` W1. Parked candidates:
  *Undervault*, *Hollow Crown*, *Lamplight* + noun. **Blocks Game Specificity /3.0.**
- [ ] 🔴 **State the win/loss condition explicitly** — currently only implicit. Draft:
  *loss = hero death / run ends; per-run win = clear the floor boss and exit; meta-win =
  accumulate world flags that weaken the Hollow Crown's grip.* **Blocks Game Specificity.**
- [ ] Assemble the player-facing mechanics section from GDD §3 (core loop), §4 (retinue +
  4 stances: Follow/Charge/Hold/Rally + leash rule), §6 (per-player decision events).
  Frame as *what the player sees and does*, not what the system computes (anti-slop gate).

### AI Architecture (dev agents, described via gameplay effect)
- [ ] New section — name each development agent + one plain-English role tied to a
  player-facing outcome:
  - [ ] `narrative-director` → writes the decision events, faction lore, and NPC barks
    the player reads at event sites. (`.claude/agents/narrative-director.md`)
  - [ ] `pixel-art-director` → defines the 2-bit readability rules (silhouette/value per
    faction) that keep 1,000-entity battles legible. (`.claude/agents/pixel-art-director.md`)
  - [ ] `gameplay-director` → owns scaling curves, enemy stat blocks, encounter budgets —
    the numbers that make the power fantasy land. (`.claude/agents/gameplay-director.md`,
    `SYSTEMS.md`, `docs/data/`)
  - [ ] MCP tooling: `unreal-mcp` (builds/edits the UE5.8 project directly) and `pixellab`
    (generates the 2-bit sprites/tilesets). Source: memory `unreal-mcp-setup`,
    `pixellab-retention-rule`.

### Technical Strategy (agent roles, token budget, API constraints)
- [ ] New section:
  - [ ] Agent-roles table (dev agent → responsibility → canon files it owns).
  - [ ] **Token budget** — per-agent budgets and why they're plausible for the semester.
  - [ ] **API constraints** — model choice, context limits, MCP round-trip costs.
  - [ ] Semester-realistic scope — lean on GDD §10 spikes + the RTS vertical slice.

### Presentation
- [ ] Assemble the four required sections into one submission document.
- [ ] Export to **PDF**.
- [ ] Proofread for major errors; confirm all four required sections present.

---

## Part B — Vertical-slice design depth (near-term production; not scored tonight)

- [ ] Fill `SYSTEMS.md` (currently a skeleton — every section "not yet designed"):
  - [ ] §1 entity tier stat blocks (fodder → soldier → elite → titan → boss)
  - [ ] §2 one scaling curve across the slice's 3 floors
  - [ ] §4 encounter budget table per floor (density, wave composition, spike/breather)
  - [ ] §6 Vanguard retinue tuning (growth rate, attrition, per-floor cap)
  - [ ] loot v0 (unit orbs + healing + ~4–6 stacking items — not the real loot system)
- [ ] Create `docs/data/` and `docs/design/` — referenced by `SYSTEMS.md` and the
  gameplay-director agent but **do not exist yet**.
- [ ] Fill `docs/SPIKE1-RESULTS.md` — all numbers blank, no machine, no verdict; complete
  after running Spike 1 (the project's biggest technical risk).
- [ ] Art-test decisions (gate all sprite production): flipbooks vs. flat-shaded 3D
  (GDD Q#5), strict 4-color vs. per-faction palettes (GDD Q#6).
- [ ] RTS slice §4 design prerequisites — all currently unchecked (mirror of the
  `SYSTEMS.md` items above; keep in sync).

---

## Part C — Full-v1 backlog (parked)

- [x] ~~**Base Camp Loot Manager**~~ — **CUT 2026-07-21** per the GDD review board
  (`gdd-review-kit`): reopened the §6a host-world rule for a stretch goal layered on
  a loot system that doesn't exist yet. Removed from `GDD.md` §8. Revisit only if/when
  the loot system *and* the Gatecamp hub are both real — this entry stays as the
  historical record of the idea, not an active backlog item.
- [ ] **Unwitnessed faction revisit** — final name, titan designs/variety, horror level
  (GDD Q#13 / WORLD W1b). Parked at owner's request until content lock; don't build titan
  content beyond Spike 1 until resolved.
- [ ] Loot & itemization system (GDD §8 — deferred by design; VS-style, feeds hero + retinue).
- [ ] Naming pass: final class names (CLASSES C1), setting/hub/antagonist names (WORLD W1).
- [ ] S8 "The Silent Bell" world flag — effects "design later" (WORLD §7).
- [ ] Pacing director — L4D-style intensity manager (SYSTEMS §5).
- [ ] Remaining open questions: CLASSES C2–C11, WORLD W2–W7, GDD Open Questions log.

---

*Tracker created 2026-07-21. Update checkboxes as items land; move parked items up when
they enter production.*
