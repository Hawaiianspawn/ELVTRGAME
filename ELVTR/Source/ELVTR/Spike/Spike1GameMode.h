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
	int32 StartingRetinue = 128;

	/**
	 * Breathers refill the retinue back up to this cap rather than granting a
	 * flat bonus. A flat grant can't keep pace with per-wave attrition — the
	 * army death-spirals and the last wave is lost before it starts. Refilling
	 * to a cap means losses cost you the *wave*, not the run, and the cap keeps
	 * the ceiling flat so later waves still escalate.
	 *
	 * 128 = 8 units x 16 (the 8x2 "mini retinue", owner call 2026-08-04): every
	 * command handle claimed, every unit full at muster. Was 120, which left
	 * the 16-body units ragged.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	int32 RetinueCap = 128;

	UPROPERTY(EditDefaultsOnly, Category = "Run")
	float DeploySeconds = 1.f;

	/**
	 * Long enough to READ the end-of-wave board, not just long enough to respawn
	 * reinforcements (docs/ui/end-of-wave-showcase.md §9, which asks for 6-8s).
	 *
	 * 2s was a fast-internal-test-loop placeholder from before that panel existed; a
	 * five-row table cannot be read in it. Of the two answers §9 offers — raise the
	 * number, or gate the transition on an explicit A-press with a timer backstop — this
	 * is the raise, deliberately: the dismiss half needs an input binding on the hero pawn
	 * and a W_WaveBoard widget, neither of which exists yet, and shipping the binding
	 * ahead of the widget would be a control bound to nothing. Adding the A-press later
	 * only means calling EnterPhase(WaveActive) early — this value stays as its backstop.
	 *
	 * Counts up on PhaseTimer, so task-115's pause menu freezing PhaseTimer freezes this
	 * read-time budget too: summoning the menu can't burn the board's dwell down.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	float BreatherSeconds = 7.f;

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
