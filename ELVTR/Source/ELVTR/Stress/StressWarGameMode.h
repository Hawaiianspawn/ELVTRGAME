#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StressWarGameMode.generated.h"

class UStressWarSide;

/**
 * StressWar testbed (docs/design/stress-war.md): two IDENTICAL formed armies, PerSide bodies
 * each, on an open field, run by a two-level war manager: side (UStressWarSide) over
 * companies (UBattlegroundCommander) over squad handles. Each side charges the other's live
 * centroid, refills thinned handles from a reserve, and the mode writes one CSV row per
 * decision tick to Saved/StressWar.csv. No hero, no brood, no director: the only question
 * this level asks is what the sim costs at 2 x PerSide and where it falls over.
 *
 * Play: `open L_StressWar` (World Settings pins this mode), or
 * `open L_Spike1?game=/Script/ELVTR.StressWarGameMode`. Cost: `stat unit`, `stat Swarm`.
 *
 * Handles: team 0 takes 0..MaxSquads/2-1, team 1 the rest (USwarmSubsystem::AssignRecruit's
 * own TeamId split). Each company = 1 Spearmen + 1 Archers handle.
 */
UCLASS()
class AStressWarGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStressWarGameMode();
	/** URL options override the defaults: `?PerSide=500?Reserve=0?Companies=2?MaxSeconds=60`. */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 PerSide = 5000;

	/** Companies per side; each takes 2 handles. Capped by MaxSquads/2/2. */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 CompaniesPerSide = 2;

	/** Bodies per side held back and fed in as handles thin out. */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 Reserve = 2500;

	/** A handle is refilled once its standing drops under this fraction of its start. */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	float ReinforceFloor = 0.6f;

	/** Distance between the two deployment centres, uu. */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	float DeploymentDistance = 8000.f;

	/** Files per handle block. Bodies-per-handle / Columns = ranks. */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 Columns = 40;

	/** Handle blocks abreast before the next wraps behind (Swarm.Formation.GroupsPerRow). */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 BlocksAbreast = 2;

	/** How far past the enemy centroid a company aims its charge, uu (see UStressWarSide::Decide). */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	float ChargeOvershoot = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	float DecisionIntervalSeconds = 2.f;

	/** Match ends (CSV closed) after this long, or when a side has no bodies left anywhere. */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	float MaxMatchSeconds = 180.f;

	/** Team-atlas looks so the two sides read apart (docs/data/art/team-variants.json). */
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 TeamAMeleeLook = 1;
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 TeamBMeleeLook = 7;
	UPROPERTY(EditDefaultsOnly, Category = "StressWar")
	int32 ArcherLook = 0;

private:
	void WriteCsv(const FString& Row);
	void EndMatch(const TCHAR* Why);

	UPROPERTY()
	TObjectPtr<UStressWarSide> SideA = nullptr;
	UPROPERTY()
	TObjectPtr<UStressWarSide> SideB = nullptr;

	float Countdown = 0.f;
	double StartTime = 0.0;
	bool bCsvStarted = false;
	bool bEnded = false;
};
