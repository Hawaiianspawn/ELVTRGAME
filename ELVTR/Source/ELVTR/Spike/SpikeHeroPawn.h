#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SpikeHeroPawn.generated.h"

class UCameraComponent;
class UStaticMeshComponent;

/**
 * Minimal WASD hero. Polls keys directly (no input assets needed for a spike).
 * Owns three jobs beyond movement:
 *  - publishes its location to USwarmSubsystem as the attractor
 *  - issues stance orders (1 Follow / 2 Charge / 3 Hold / 4 Rally, R restart)
 *  - applies incoming brood damage and draws the prototype HUD
 */
UCLASS()
class ASpikeHeroPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpikeHeroPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Spike")
	float MoveSpeed = 600.f;

	/**
	 * Seeds the camera's constructor placement only. At runtime TickCamera owns the transform
	 * from Emberkeep.Cam.Dist, so editing this in the details panel will not move the in-game
	 * shot — change the CVar (or its line in Saved/SwarmExecOnPlay.txt) instead.
	 */
	UPROPERTY(EditAnywhere, Category = "Spike")
	float CameraHeight = 1200.f;

private:
	/** Ground point under the cursor — Charge aims here. */
	bool GetCursorGroundLocation(FVector& OutLocation) const;

	void TickStanceInput(const APlayerController& PC);
	void TickHeroCombat(float DeltaSeconds);

	/**
	 * Drive the whole camera transform from the Emberkeep.Cam.* dials — projection, pitch/yaw,
	 * distance, focus offset — plus the up-axis bias that keeps the HUD from pushing the hero
	 * out of the visible strip. Owns the camera every frame, so the constructor's values only
	 * survive as the editor preview before BeginPlay.
	 */
	void TickCamera(float DeltaSeconds, const class USwarmSubsystem* Swarm);

	void DrawHUD() const;

	/** True on the frame Key transitions up->down. */
	bool ConsumeKeyPress(const APlayerController& PC, const FKey& Key, bool& bWasDown) const;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UCameraComponent> Camera;

	/**
	 * The hero's own swing clock, in seconds, wrapping on Swarm.SwingInterval.
	 *
	 * The hero is a pawn, not a Mass entity, so it needs its own copy of the cadence
	 * every unit runs — otherwise the one attack the player actually feels would be
	 * the only one still bleeding damage continuously, with no blow to see land.
	 * Plain runtime state, not a UPROPERTY: the pawn is recreated each PIE.
	 */
	float HeroSwingTime = 0.f;

	/** Smoothed camera offset along its up-axis compensating for the HUD's occluded band. */
	float CameraHudBias = 0.f;

	/** Where the camera actually is, so Emberkeep.Cam.Lerp has something to ease from. */
	FVector CameraLoc = FVector::ZeroVector;
	FRotator CameraRot = FRotator::ZeroRotator;
	/** False until the first TickCamera places it — stops Lerp swooping in from the origin. */
	bool bCameraPlaced = false;

	// Edge-detection latches for the polled keys.
	bool bWasDownFollow = false;
	bool bWasDownCharge = false;
	bool bWasDownHold = false;
	bool bWasDownRally = false;
	bool bWasDownRestart = false;
};
