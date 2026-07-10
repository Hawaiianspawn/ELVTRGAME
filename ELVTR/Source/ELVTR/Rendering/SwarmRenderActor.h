#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwarmRenderActor.generated.h"

class UNiagaraComponent;

/**
 * Bridges Mass -> Niagara. Place one in the map with NS_Swarm assigned.
 * Each tick, pushes the swarm's packed positions + anim bits into the
 * Niagara system's user parameter arrays:
 *   User.Positions (Vector array), User.AnimBits (Int32 array)
 */
UCLASS()
class ASwarmRenderActor : public AActor
{
	GENERATED_BODY()

public:
	ASwarmRenderActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Swarm")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
};
