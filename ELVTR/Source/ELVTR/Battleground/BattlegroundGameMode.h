#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BattlegroundGameMode.generated.h"

class UBattlegroundCommander;
class UBattlegroundDirector;

/**
 * Battleground testbed (docs/design/battleground.md) — a field battle between two formed
 * armies, each a mirror of the v1 Spearmen/Archers roster (squad-group-system.md §1.1),
 * run by one AI commander per side plus a shared back-channel director. A separate level
 * and a separate game mode: no upkeep, no Adaptation, no five-layer castle geometry (§0's
 * canon note) — the underlying Mass mechanism (typed squad handles, per-type formation,
 * per-handle stance) is exactly what this needs, borrowed as-is from the castle.
 *
 * §0 canon check, restated here because it changes this class's own numbers: the spec's
 * headline "150 Spearmen + 40 Archers per side" was derived against
 * USwarmSubsystem::TypeLegibilityCeiling = 80. The shipped constant is now 16 (2026-08-04
 * owner call) — at 16, 150/40 would need roughly 13 handles per side, not 3, blowing well
 * past the shared 8-handle budget the spec itself calls load-bearing. SpearmenPerSide/
 * ArchersPerSide below are chosen instead to satisfy the spec's actual DELIVERABLE (§5's
 * Build scope row: "2 Spearmen + 1 Archer unit x2 teams = 6/8 handles claimed") against
 * today's real ceiling, at roughly the same 75/25 split ratio the spec itself argues for.
 * Flagged in the handback, not silently reconciled.
 */
UCLASS()
class ABattlegroundGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABattlegroundGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Per side. See the class comment for why these are 30/10, not the spec's stale 150/40. */
	/** Ally army = MeleeUnits + RangedUnits formations of UnitWidth x UnitDepth bodies each
	 *  (owner: "6x4 unit formations; multiple formations is your army"). One squad handle per
	 *  formation, so MeleeUnits + RangedUnits <= USwarmSubsystem::MaxSquads. */
	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	int32 MeleeUnits = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	int32 RangedUnits = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	int32 UnitWidth = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	int32 UnitDepth = 4;

	/** Enemy = the Ooze brood, spawned in ranks (Swarm.BroodFormation.*) and hunting the army. */
	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	int32 OozeCount = 192;

	/** How far apart the two deployment zones sit, uu, along the field's long axis (§1.1:
	 *  "far enough... neither army's formation spawns inside the other's... engage
	 *  distance, close enough that a Charge order closes the gap in a readable few
	 *  seconds"). */
	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	float DeploymentZoneDistance = 3000.f;

	/** Commander decision cadence, seconds (§2.1: "recommend 1.5-2s"). */
	UPROPERTY(EditDefaultsOnly, Category = "Battleground")
	float CommanderDecisionIntervalSeconds = 1.75f;

	/** Team 0 is the player's army by convention; team 1 is always the AI. */
	static constexpr uint8 PlayerTeamId = 0;
	static constexpr uint8 EnemyTeamId = 1;

	UBattlegroundCommander* GetPlayerCommander() const { return PlayerCommander; }
	UBattlegroundCommander* GetEnemyCommander() const { return EnemyCommander; }
	UBattlegroundDirector* GetDirector() const { return Director; }

private:
	void StartMatch();

	UPROPERTY()
	TObjectPtr<UBattlegroundCommander> PlayerCommander = nullptr;

	UPROPERTY()
	TObjectPtr<UBattlegroundCommander> EnemyCommander = nullptr;

	UPROPERTY()
	TObjectPtr<UBattlegroundDirector> Director = nullptr;

	float DecisionCountdown = 0.f;
	double MatchStartTime = 0.0;
	int32 ShotsTaken = 0;
};
