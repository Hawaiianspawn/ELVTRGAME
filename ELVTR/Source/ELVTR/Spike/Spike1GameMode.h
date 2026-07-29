#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Spike1GameMode.generated.h"

UENUM()
enum class ERunPhase : uint8
{
	Deploying,	// short beat before wave 1 so the formation can settle
	WaveActive,
	Breather,	// wave cleared, reinforcements arrive
	Won,
	Lost
};

/**
 * Gate 1 fun-prototype run structure: survive N waves of brood, then win.
 * Hero death loses. Reinforcements between waves are the only healing.
 *
 * Deliberately a hand-authored wave list, not a director — the point of this
 * prototype is to judge whether stances feel good, and a fixed rhythm makes
 * two playthroughs comparable.
 */
UCLASS()
class ASpike1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpike1GameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Wipe the field and start over (bound to R on the hero pawn). */
	void RestartRun();

	// --- HUD accessors ---------------------------------------------------
	ERunPhase GetPhase() const { return Phase; }
	int32 GetWaveIndex() const { return WaveIndex; }
	int32 GetWaveCount() const { return WaveBroodCounts.Num(); }
	float GetPhaseTimer() const { return PhaseTimer; }
	bool IsRunOver() const { return Phase == ERunPhase::Won || Phase == ERunPhase::Lost; }

	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TArray<int32> WaveBroodCounts = { 250, 450, 700 };

	UPROPERTY(EditDefaultsOnly, Category = "Run")
	int32 StartingRetinue = 120;

	/**
	 * Breathers refill the retinue back up to this cap rather than granting a
	 * flat bonus. A flat grant can't keep pace with per-wave attrition — the
	 * army death-spirals and the last wave is lost before it starts. Refilling
	 * to a cap means losses cost you the *wave*, not the run, and the cap keeps
	 * the ceiling flat so later waves still escalate.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	int32 RetinueCap = 120;

	UPROPERTY(EditDefaultsOnly, Category = "Run")
	float DeploySeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run")
	float BreatherSeconds = 2.f;

	/**
	 * Grace period after a wave spawns before "no brood left" counts as a clear.
	 * The live counts come from the render pass, so they read zero for a frame
	 * or two after BatchCreateEntities.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	float WaveClearGraceSeconds = 2.f;

private:
	void BeginWave();
	void EnterPhase(ERunPhase NewPhase);

	ERunPhase Phase = ERunPhase::Deploying;
	int32 WaveIndex = 0;
	float PhaseTimer = 0.f;

	/** Keeps mid-run reinforcements out of already-occupied formation slots. */
	int32 RetinueSlotCursor = 0;
};
