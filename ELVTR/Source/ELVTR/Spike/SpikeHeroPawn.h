#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SpikeHeroPawn.generated.h"

class UCameraComponent;
class UStaticMeshComponent;

/**
 * Minimal WASD hero for the benchmark. Polls keys directly (no input assets
 * needed for a spike). Publishes its location to USwarmSubsystem as the
 * attractor, and draws the benchmark HUD lines.
 */
UCLASS()
class ASpikeHeroPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpikeHeroPawn();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Spike")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Spike")
	float CameraHeight = 3500.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UCameraComponent> Camera;
};
