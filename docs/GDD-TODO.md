# GDD Fleshing-Out Tracker

**Purpose:** a living checklist of what still needs writing/deciding in the design
docs, triaged **assignment-rubric-first → vertical-slice → full-v1**.

> **Status note (added 2026-07-29):** `docs/backlog/INDEX.md` is now the live work
> queue for ongoing design/production tasks — check there first for what's actually
> being worked. This file remains the assignment-scoped record: originally for GDD
> Assignment #1 (due 21 July, working title *Emberkeep*), and since superseded by
> Assignment #2, submitted 27 July as `docs/ASSIGNMENT-02-FINAL-GDD.md` /
> `docs/Kindled-GDD-Assignment-02.pdf` under the final title **Kindled**
> (`GDD.md:9,481`). Part A below was re-verified against current canon on 2026-07-29
> — most of it is now done, via the Assignment #2 submission this tracker predates.
> Part B was re-checked against `SYSTEMS.md`, `docs/design/`, and `docs/data/`, which
> now exist and are populated — this file's own claim that they didn't was stale.

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
| **Game Specificity** /3.0 | Named game, core loop, **win/loss** | ✅ RESOLVED 2026-07-29 — name is **Kindled** (`GDD.md:481`), win/loss stated (`docs/ASSIGNMENT-02-FINAL-GDD.md:56`) | Closed |
| **Player Experience Clarity** /2.5 | What player *sees/does* | ✅ Assembled (`docs/ASSIGNMENT-02-FINAL-GDD.md:197`) | Closed |
| **Agent Role Clarity** /2.0 | Each **dev agent** named + 1-sentence role | ✅ Written (`docs/ASSIGNMENT-02-FINAL-GDD.md:329-350`) | Closed |
| **Technical Feasibility** /1.5 | Semester scope, ≥1 constraint, **token budget** | Constraints + scope done (`:382-418`); **token budget line item still missing** | Partial — token budget open |
| **Presentation** /1.0 | 4 sections present, single PDF | ✅ Assembled + PDF exported (`docs/Kindled-GDD-Assignment-02.pdf`) | Closed, proofread unconfirmed |

**Anti-slop gate (from the brief):** generic/placeholder framing, or MAS concepts not
grounded in specific player-facing mechanics, = **zero** on Game Specificity + Player
Experience Clarity regardless of theoretical correctness. Every agent role must be tied
to a concrete thing the player sees or does.

---

## Part A — Assignment-critical (must-fix to score; due tonight)

Organized by the four required sections + presentation.

### Executive Summary
- [x] Executive Summary — written and shipped: `docs/ASSIGNMENT-02-FINAL-GDD.md:34`
  (also present in the Assignment #1 draft, `docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md:8`).
  Superseded framing note: the one-liner this bullet was drafted against is itself
  stale — current framing is *"you carry the only fire in a pitch-dark world"*
  (`docs/ASSIGNMENT-02-FINAL-GDD.md:36`), not the co-op-army pitch quoted here.

### Game Mechanics (player-facing actions + loop)
- [x] **Name the game** — resolved. `GDD.md:481` (§12 Q10, re-decided 2026-07-27): the
  game is **Kindled**, replacing the working title *Emberkeep* (which came from the
  discarded `WORLD.md` canon). Also stated at `GDD.md:9` and
  `docs/ASSIGNMENT-02-FINAL-GDD.md:1`. **Correction to this task's own spawn prompt:**
  it asserted the name was "Emberkeep" per `GDD.md:433` — that line is actually part of
  §10's entity-gate discussion, not the naming question, and the name itself is stale.
  No longer blocks Game Specificity.
- [x] **State the win/loss condition explicitly** — resolved and stated as a table:
  `docs/ASSIGNMENT-02-FINAL-GDD.md:56-58` — *loss = hero death, run ends immediately;
  per-run win = survive three escalating waves and clear what the last one brings.*
  Matches the loop already implicit in `GDD.md:82` ("floor boss / exit → next floor").
  This bullet's own draft (meta-win via world flags weakening the Hollow Crown) is
  itself stale — world flags were discarded in the 2026-07-22 narrative reset
  (`docs/narrative/FLAME-FOUNDATION.md:8`) and meta-progression is currently
  "unwritten by choice" (`GDD.md` §11 Scope Guardrails). No longer blocks Game
  Specificity.
- [x] Player-facing mechanics section assembled: `docs/ASSIGNMENT-02-FINAL-GDD.md:197`
  ("§3. Game Mechanics — What the Player Sees and Does"), built from the current core
  loop, stances, and decision events.

### AI Architecture (dev agents, described via gameplay effect)
- [x] Section written: `docs/ASSIGNMENT-02-FINAL-GDD.md:329` ("§4. The Development
  Crew"), roles table at `:336-350` — covers two more agents than scoped here
  (`performance-director`, `ui-director`) plus the `host` backlog agent.
  - [x] `narrative-director` — `docs/ASSIGNMENT-02-FINAL-GDD.md:340`
  - [x] `pixel-art-director` — `docs/ASSIGNMENT-02-FINAL-GDD.md:341`
  - [x] `gameplay-director` — `docs/ASSIGNMENT-02-FINAL-GDD.md:342`
  - [x] MCP tooling (`unreal-mcp`, `pixellab`) — `docs/ASSIGNMENT-02-FINAL-GDD.md:348-349`

### Technical Strategy (agent roles, token budget, API constraints)
- [ ] Section exists — mostly written, two sub-items genuinely open. Section at
  `docs/ASSIGNMENT-02-FINAL-GDD.md:382` ("§5. Scope and Constraints").
  - [x] Agent-roles table — the §4.1 crew table (`:336-350`) doubles as this: agent →
    output → where it lands, in place of a separate "canon files it owns" column.
  - [ ] **Token budget** — re-verified 2026-07-29, genuinely open. The doc mentions
    "context windows" (`:354`) and "no tokens have been spent" (`:368`) in passing but
    states no per-agent budget figures or semester-plausibility rationale.
  - [ ] **API constraints** — re-verified 2026-07-29, partially open. MCP round-trip
    cost is discussed qualitatively (the single-editor mutex, `:356-370`), but model
    choice and context-limit numbers aren't itemized anywhere in the doc.
  - [x] Semester-realistic scope — `docs/ASSIGNMENT-02-FINAL-GDD.md:392` ("§5.2 What
    one person can actually finish in the remaining weeks"): cuts 4 of the original 6
    scope items and explains the sequencing.

### Presentation
- [x] Four required sections assembled into one submission document:
  `docs/ASSIGNMENT-02-FINAL-GDD.md` (Assignment #2, supersedes the Assignment #1
  version at `docs/ASSIGNMENT-01-GDD-FIRST-DRAFT.md`).
- [x] Exported to **PDF**: `docs/Kindled-GDD-Assignment-02.pdf` (confirmed present on
  disk); render source + regeneration command recorded at
  `docs/ASSIGNMENT-02-FINAL-GDD.md:9-18`.
- [ ] Proofread for major errors — re-verified 2026-07-29, **can't confirm from the
  repo either way**; no record of a proofreading pass. Left open rather than assumed
  — the doc was already submitted for grading 2026-07-27, and a wrong tick here would
  repeat exactly this tracker's original problem.

---

## Part B — Vertical-slice design depth (near-term production; not scored tonight)

- [x] Fill `SYSTEMS.md` — **no longer a skeleton.** `SYSTEMS.md` was last updated
  2026-07-24 and every section below has a recorded decision:
  - [x] §1 entity tier stat blocks — `SYSTEMS.md:19-37` (Liberated ladder DECIDED
    2026-07-24), data in `docs/data/entity-tiers.json`.
  - [x] §2 scaling curve — `SYSTEMS.md:39-51` (250→450→700 brood, DECIDED
    2026-07-24), `docs/data/scaling-curve.json`.
  - [x] §4 encounter budget table per floor — answered by `docs/design/
    encounter-budget.md` (written specifically to close this item — see its own
    header) and `docs/data/encounter-budget.json`. Note: `SYSTEMS.md:84` itself still
    carries the stale "remains to be tuned in play" line; the spec exists but hasn't
    been folded back into `SYSTEMS.md`'s prose yet (tracked by
    `docs/backlog/task-038-fold-settled-specs-into-systems-md.md`, still open).
  - [x] §6 Vanguard retinue tuning — answered by `docs/design/
    retinue-tuning-vanguard.md` and `docs/data/retinue-vanguard.json`
    (`docs/backlog/task-005-vanguard-retinue-tuning.md`, status **needs-review**: the
    tuning pass is written and complete, awaiting the owner's sign-off before it's
    folded into `SYSTEMS.md` §6 as canon — a review-queue item, not a design gap).
  - [x] loot v0 — `SYSTEMS.md:53-64` (6 stacking run-items, DECIDED 2026-07-24),
    `docs/data/loot-v0.json`.
- [x] **Correction: `docs/data/` and `docs/design/` already exist and are heavily
  populated** — this bullet's claim that they "do not exist yet" was itself stale
  (flagged 2026-07-29, per lead). `docs/data/` holds 25+ files (economy, upgrades,
  entity tiers, scaling curve, encounter budget, scenarios, art requests/provenance…);
  `docs/design/` holds 12 spec docs (entity-tiers, scaling-curve, encounter-budget,
  run-structure, loot-v0, retinue-tuning-vanguard, feeding-distraction, audio-minimal,
  hero-build-variety, squad-group-system, CAMERA-SCALE, CAMERA-SCALE-HANDOFF).
- [ ] Fill `docs/SPIKE1-RESULTS.md` — **re-verified 2026-07-29, still genuinely open.**
  Every number in the table is blank, machine/build fields are unfilled, and no
  GO/ADJUST/KILL box is checked. Matches
  `docs/backlog/task-007-fill-spike1-results-and-verdict.md`, still `proposed`
  (not started).
- [ ] Art-test decisions — **re-verified 2026-07-29, still open** — and Q6 needs a
  canon update this task isn't authorized to make (editing `GDD.md` is out of scope).
  - Flipbooks vs. flat-shaded 3D (GDD Q#5): still "Needs art test" — `GDD.md:471`;
    `docs/backlog/task-015-gdd-q5-flipbooks-vs-3d-art-test.md` still `parked`.
  - Strict 4-color vs. per-faction palettes (GDD Q#6): `GDD.md:472` still frames this
    as an open 4-color question, but it's arguably **moot** — the strict 4-value
    palette gate was superseded 2026-07-28 in favor of full colour
    (`docs/art/aesthetic-direction.md:8-24`, owner call, live in-editor). `GDD.md`
    §12 hasn't been updated to reflect that yet
    (`docs/backlog/task-016-gdd-q6-palette-strategy.md`, still `proposed`).
- [ ] RTS slice §4 design prerequisites — `docs/RTS-VERTICAL-SLICE.md:77-82` itself
  still shows every box unchecked, but the underlying work for 4 of its 5 items has
  actually landed (see the `SYSTEMS.md` ticks above: entity tiers, scaling curve,
  encounter budget, and loot v0 are done; Vanguard retinue tuning is needs-review).
  That file is outside this task's edit scope (`docs/GDD-TODO.md` only per the spawn
  prompt), so its checkboxes stay stale until someone syncs them — noted here rather
  than fixed.

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
they enter production. Corrected against current canon 2026-07-29 (see status note at
top) — `docs/backlog/INDEX.md` is the live queue going forward.*
