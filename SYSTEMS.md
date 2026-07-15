# Gameplay Systems — Decision Record

**Version:** 0.1 (skeleton) · Companion docs: `GDD.md`, `CLASSES.md`, `WORLD.md`
**Owner:** gameplay-director agent (this is the one canon file it edits directly)
**Last updated:** 2026-07-14

This file is the source of truth for gameplay-system decisions: scaling curves, loot
rules, entity tiers, encounter budgets, pacing. It records **decisions and rationale**;
detailed specs live in `docs/design/`, tuned numbers in `docs/data/`. Nothing here
overrides `GDD.md` — conflicts are canon proposals back to the user.

---

## 1. Entity tiers

*Not yet designed.* Working taxonomy (from GDD §10): fodder → soldier → elite →
titan → boss. Fodder/soldier are Mass Entity; elite/titan/boss are promoted Actors.

## 2. Scaling & difficulty

*Not yet designed.* Constraints locked by GDD §7: exponential-feeling layered
multipliers, soft caps only, co-op scales by density not HP.

## 3. Loot

*Not yet designed.* GDD §8 direction: run-scoped, VS-style stacking/evolutions,
drops feed both hero and retinue.

## 4. Encounter budgets & procgen rules

*Not yet designed.* GDD §9 constraints: every floor has ≥1 arena, ≥1 decision
event, ≥1 optional risk room; arenas sized for horde fights.

## 5. Pacing director

*Not yet designed.* Intent: L4D-style intensity manager (spike → breather → spike)
reading party state and world flags.

## 6. Retinue tuning

*Not yet designed.* Attrition/replenishment rates per class are the central balance
dial. Identity-level changes belong to `CLASSES.md` (canon proposals only).

---

## Decision log

| Date | Decision | Rationale | Spec / data |
|---|---|---|---|
| — | *(none yet)* | | |
