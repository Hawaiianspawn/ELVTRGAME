#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BattlegroundDirector.generated.h"

/**
 * The back-channel between the two Battleground commanders (docs/design/battleground.md
 * §3) — a scoped instance of SYSTEMS.md §5's undesigned pacing director, narrowed to one
 * level: two armies, one clash, one climax.
 *
 * Owned by ABattlegroundGameMode as a plain member (§3.2: "owned by the level... not a
 * new subsystem"). A UObject rather than a bare struct only so it can be a UPROPERTY the
 * game mode's GC keeps alive without a manual lifetime — it has no other reflected
 * surface, and it never talks to Mass or the world directly; the game mode reads its
 * output (GetTensionCurveTarget, ShouldRally) and calls Update once per decision cycle.
 *
 * What this must NEVER do (§3.3, load-bearing): no hidden DPS/HP multiplier, no invisible
 * bodies. Its only lever is WHICH ORDER FIRES WHEN, and even that is exercised entirely
 * by the caller — this class only ever answers "is it time," never writes a stance itself.
 */
UCLASS()
class UBattlegroundDirector : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Authored length of the arc this director paces toward, seconds. Unmeasured — same
	 * epistemic status as every other prototype dial in battleground.md (§6): long enough
	 * for both armies to close the deployment-zone gap and grind for a few exchanges
	 * before the break, short enough to stay a legible one-scene testbed run.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	float ExpectedDurationSeconds = 50.f;

	/**
	 * Seconds into the match, as a fraction of ExpectedDurationSeconds, that the one
	 * scripted break fires (§3.1: "manufacture a legible arc... out of a fight whose
	 * natural default is an anticlimactic tie"). 0.4 lands it after both lines have closed
	 * and traded a few exchanges — an escalation, not the opening move — and comfortably
	 * before the ~9-tick grind-out §6's toy sim measured for a symmetric fight. Unmeasured
	 * placeholder, same status as ExpectedDurationSeconds.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	float BreakAtFraction = 0.4f;

	/**
	 * §3.2's no-snowball guard: transcribed verbatim from battleground.md's own proposed,
	 * explicitly-unmeasured 70/30 threshold — once the LOSING side's share of the two
	 * live standing counts falls below (1 - this), that side is nudged toward Rally.
	 */
	static constexpr float NoSnowballWinnerShare = 0.7f;

	/** Call once, at BeginPlay, after both armies have spawned. */
	void StartMatch(double NowSeconds, int32 StartingA, int32 StartingB);

	/**
	 * Call once per commander decision cycle with THIS tick's live standing per team.
	 * Returns true on the ONE tick the scripted break fires — the caller is the one that
	 * actually issues the order; this class only decides WHEN and logs WHY.
	 */
	bool Update(double NowSeconds, int32 StandingA, int32 StandingB);

	/**
	 * §3.2's floor: is TeamId's live line thin enough, relative to the other side's, that
	 * its commander should be nudged toward Rally instead of whatever it would otherwise
	 * pick? An ORDER-CHOICE nudge, not a stat change — see the class's own doc comment.
	 */
	bool ShouldRally(uint8 TeamId) const;

	float GetTensionCurveTarget() const { return TensionCurveTarget; }
	bool HasBreakFired() const { return bBreakFired; }

private:
	double MatchStartSeconds = 0.0;
	float TensionCurveTarget = 0.f;
	int32 LastStandingByTeam[2] = { 0, 0 };
	int32 StartingStandingByTeam[2] = { 0, 0 };
	bool bBreakFired = false;
};
