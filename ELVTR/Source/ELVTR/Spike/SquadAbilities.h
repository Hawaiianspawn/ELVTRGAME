#pragma once

#include "CoreMinimal.h"
#include "Mass/SwarmCombat.h" // ESquadVerb

class USwarmSubsystem;

/**
 * The squad-channelled ability kit — task-144, docs/design/ability-kit.md, and the two open
 * owner calls it exists to make playable rather than arguable.
 *
 * WHAT THIS DOES NOT DO: close Q23 or Q26. Both remain open in docs/OPEN-DECISIONS.md and
 * neither register entry is ticked. This file implements BOTH of Q23's first two shapes and
 * THREE of Q26's four schemes behind one live switch, so the owner can play the comparison.
 * Nothing here prefers one. Where a choice had to be made to get anything on screen at all,
 * the choice is stated in a comment and repeated in docs/design/slice-a7.md §10.
 *
 * ── The two addressings (Q23), on Kindled.Ability.Mode ────────────────────────────────────
 *
 *   Mode 0 — Q23 = A, a fixed kit on the player.
 *       All four verbs belong to the bearer, all four are always his, and each applies to
 *       whichever of the seven are inside Kindled.Ability.PlayerRange — the register's own
 *       phrasing. Cooldowns are per VERB. A cast with nobody in reach REFUSES, which is the
 *       shape's stated cost ("the seven risk becoming interchangeable") turned into something
 *       that happens to you rather than something that was written down.
 *
 *   Mode 1 — Q23 = B, the kit lives in the soldiers.
 *       Each of the seven carries one verb (SevenRoster.h's Verb column) and the bearer has no
 *       kit of his own. Cooldowns are per SOLDIER. Spending a verb spends that soldier's
 *       availability, which is B's whole premise: you are choosing WHO acts.
 *
 * Three differences between the modes are mechanical, not cosmetic, and they are what the
 * comparison is actually about:
 *
 *   1. WHO MARKS. Under A, focus is cast by whoever is in the bearer's reach. Under B it is
 *      cast by the pack — CLASSES.md's "the entire pack focus-fires it". So the same verb
 *      turns a visibly different number of soldiers onto the boss depending on the shape.
 *   2. WHERE A ZONE LANDS. Under A the Ward Circle and the banner are placed at the CURSOR:
 *      they are the player's spells and a soldier's position is irrelevant to them. Under B
 *      they are inscribed WHERE THE CASTING SOLDIER STANDS, so positioning your seven is how
 *      you aim them. That is the difference between commanding a kit and commanding people.
 *   3. WHAT RUNNING OUT LOOKS LIKE. Under A you run out of cooldown. Under B you run out of
 *      the soldier — and with one Lampbearer on the roster, losing Ember loses raise for the
 *      rest of the run.
 *
 * ── The order schemes (Q26) ───────────────────────────────────────────────────────────────
 *
 * PREFLIGHT.md §3 pairs Q23 = A with D (verb wheel) and Q23 = B with A or C. ability-kit.md §3
 * works the pairings through against the actual verb list and adds a finding: select-then-order
 * (Q26 = B) is at least as coherent with Q23 = B as either, and arguably the most literal fit,
 * since B's premise IS "you are choosing who acts". So:
 *
 *   Mode 0 gets D  — hold Q for the wheel, release to arm the verb, LMB to target it.
 *   Mode 1 gets B  — ZXCVBNM selects a soldier, LMB orders them to act.
 *   Mode 1 ALSO gets A — E is one button whose meaning is resolved by what is under the
 *                        cursor (ResolveDirectTarget below). Both are live on the same
 *                        keyboard at once so the comparison needs no second flip, and
 *                        FAbilityState::CastsByScheme records which one actually got used.
 *
 * Q26 = C (contextual single button) is NOT separately implemented. Under Q23 = B it is the
 * same input as A with the resolution rule doing more of the work, and one honest
 * implementation of that resolution is worth more than two that differ by a comment. Said
 * plainly here and in the handback rather than left to be discovered.
 */
namespace SquadAbilities
{
	/** Which of Q26's schemes delivered a cast. Indexes FAbilityState::CastsByScheme. */
	enum class EOrderScheme : uint8
	{
		Wheel = 0,				// Q26 = D — verb chosen first, then a target
		SelectThenOrder = 1,	// Q26 = B — soldier chosen first, then "act"
		DirectTarget = 2,		// Q26 = A — one click, meaning resolved from what it landed on
		Console = 3,			// not a scheme: Kindled.Ability.Use, for headless evidence
	};

	/** The verb soldier UnitIndex carries under Q23 = B. None for the garrison or a bad index. */
	ESquadVerb VerbFor(int32 UnitIndex);

	/**
	 * Fire Verb. Under Q23 = A pass Caster = INDEX_NONE (the bearer owns it); under Q23 = B
	 * pass the soldier acting, and Verb is ignored in favour of theirs.
	 *
	 * CursorPoint is the ground point the order was aimed at — where a zone lands under Q23 = A,
	 * and which soldier Kindle picks under both. Returns false and logs the reason on refusal;
	 * a refusal is evidence (ability-kit.md §5 asks for exactly this), so it is never silent.
	 */
	bool Cast(UWorld* World, ESquadVerb Verb, int32 Caster, const FVector& CursorPoint, EOrderScheme Scheme);

	/**
	 * Q26 = A. What one click at CursorPoint means, and who answers it. Returns the soldier
	 * that should act (INDEX_NONE if nobody can) and writes the verb that resolution picked.
	 *
	 * The rule is deliberately small and is documented in slice-a7.md, because its AMBIGUITY is
	 * the thing being tested: ability-kit.md §5 says to watch for wrong-verb activations under
	 * this scheme, and a resolution rule elaborate enough to never misfire would answer Q26 by
	 * making the question disappear.
	 */
	int32 ResolveDirectTarget(const USwarmSubsystem& Swarm, const FVector& CursorPoint, ESquadVerb& OutVerb);

	/** Nearest STANDING one of the seven to P, or INDEX_NONE. MaxDist 0 = no limit. */
	int32 NearestNamed(const USwarmSubsystem& Swarm, const FVector& P, float MaxDist = 0.f);

	/** Seconds until this cast would be allowed; 0 = ready. Caster INDEX_NONE reads the
	 *  per-verb clock (Q23 = A), otherwise the per-soldier one (Q23 = B). */
	float ReadyIn(const USwarmSubsystem& Swarm, ESquadVerb Verb, int32 Caster, float Now);

	/** Standing zones + the focus/raise tethers, in world debug draw. No-op while
	 *  Kindled.Ability.Draw is 0. The verb wheel is drawn by the pawn, which owns its state. */
	void DrawActiveZones(UWorld* World);

	/** One line per soldier + the live actives, for the log. Shared by Kindled.Ability.Report
	 *  and Kindled.Seven.Report so the two can never drift. */
	void LogReport(UWorld* World);
}
