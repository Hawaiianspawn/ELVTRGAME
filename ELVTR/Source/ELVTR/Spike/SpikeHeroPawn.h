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

	UPROPERTY(EditAnywhere, Category = "Spike")
	float CameraHeight = 1200.f;

private:
	/** Ground point under the cursor — Charge aims here. */
	bool GetCursorGroundLocation(FVector& OutLocation) const;

	void TickStanceInput(const APlayerController& PC);
	void TickHeroCombat(float DeltaSeconds);
	void DrawHUD() const;

	/** True on the frame Key transitions up->down. */
	bool ConsumeKeyPress(const APlayerController& PC, const FKey& Key, bool& bWasDown) const;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UCameraComponent> Camera;

	// Edge-detection latches for the polled keys.
	bool bWasDownFollow = false;
	bool bWasDownCharge = false;
	bool bWasDownHold = false;
	bool bWasDownRally = false;
	bool bWasDownRestart = false;
};
