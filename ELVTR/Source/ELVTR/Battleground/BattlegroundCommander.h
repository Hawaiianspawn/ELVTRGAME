#pragma once

#include "CoreMinimal.h"
#include "Mass/SwarmCombat.h" // ESwarmStance
#include "UObject/Object.h"
#include "BattlegroundCommander.generated.h"

class USwarmSubsystem;
class UBattlegroundDirector;

/**
 * One army's AI commander (docs/design/battleground.md §2.1) — a plain UObject, no body,
 * only orders. Issues the same four verbs every stance system in this project already
 * uses (Follow/Charge/Hold/Rally), addressed per unit through USwarmSubsystem's existing
 * SetUnitStance API (SwarmSubsystem.h:225-263) — no new order-issuing mechanism.
 *
 * Two are instanced by ABattlegroundGameMode, one per team (§2.1: "instanced twice with a
 * TeamId"). Only the AI team's instance ever has Decide called on it — the player's own
 * team's instance exists purely as bookkeeping (which handles are "mine", where "home" is)
 * for the Battleground.Order console command, per §4: "the player's army gets one
 * commander-shaped thing fewer than the enemy — no AI decision loop on their own side."
 */
UCLASS()
class UBattlegroundCommander : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(uint8 InTeamId, const TArray<int32>& InUnitHandles,
		const FVector& InHomeZone, const FVector& InEnemyZone);

	/**
	 * Called on a ~1.5-2s cadence (§2.1's decision cadence) by the game mode's Tick, never
	 * per-frame. Reads read-only squad state (GetSquadStanding/GetSquadCentroid/GetSquadHP)
	 * and issues stance orders through the shared USwarmSubsystem API — the entirety of
	 * what a commander does (Design Law 5: it never addresses an individual soldier, only
	 * ever a typed handle).
	 */
	void Decide(USwarmSubsystem& Swarm, UBattlegroundDirector& Director, const FVector& EnemyCentroid);

	/** Player took manual control (Battleground.Order): stop autopilot decisions for this side. */
	void SetManual(bool bInManual) { bManual = bInManual; }
	bool IsManual() const { return bManual; }

	/**
	 * The director's one scripted break, applied to THIS commander only (§3.3: the
	 * back-channel must never override the player's own side, and Battleground has exactly
	 * one AI seat — see the game mode, which never calls this on the player's commander).
	 * Forces every one of this army's units onto Charge, aimed at EnemyCentroid, overriding
	 * whatever Decide would otherwise have picked this tick — §3.3's one sanctioned
	 * override: order TIMING only, the same four verbs, nothing hidden.
	 */
	void ForceBreakCharge(USwarmSubsystem& Swarm, const FVector& EnemyCentroid);

	uint8 GetTeamId() const { return TeamId; }
	const TArray<int32>& GetUnitHandles() const { return UnitHandles; }
	const FVector& GetHomeZone() const { return HomeZone; }

	int32 GetLiveStanding(const USwarmSubsystem& Swarm) const;
	FVector GetLiveCentroid(const USwarmSubsystem& Swarm) const;

private:
	uint8 TeamId = 0;

	UPROPERTY()
	TArray<int32> UnitHandles;

	FVector HomeZone = FVector::ZeroVector;
	FVector EnemyZone = FVector::ZeroVector;

	/** Last stance this commander itself issued, so Decide only logs on a change. */
	ESwarmStance LastStance = ESwarmStance::Hold;
	bool bCommitted = false; // set by ForceBreakCharge; Charge is sticky until Rally
	bool bManual = false;
};
