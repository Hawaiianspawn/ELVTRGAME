#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Mass/SwarmCombat.h"
#include "StressWarSide.generated.h"

class UBattlegroundCommander;
class USwarmSubsystem;

/**
 * One side of the StressWar: a team id, its companies (each a UBattlegroundCommander over
 * a few squad handles), a reserve pool of bodies not yet on the field, and the side-level
 * decisions — where the companies charge, and who gets topped up from the reserve.
 * Side -> company -> handle is the whole hierarchy; the subsystem knows nothing of it.
 */
UCLASS()
class UStressWarSide : public UObject
{
	GENERATED_BODY()
public:
	/** Claims handles [FirstHandle, FirstHandle + Companies*2): per company one Spearmen +
	 *  one Archers handle of BodiesPerHandle each, spawned in formation around HomeZone. */
	void Muster(USwarmSubsystem& Swarm, uint8 InTeamId, int32 FirstHandle, int32 Companies,
		int32 BodiesPerHandle, int32 InReserve, int32 MeleeLook, int32 ArcherLook,
		const FVector& InHomeZone, const FVector& EnemyZone);

	/** Side-level decision: every company charges THROUGH the enemy centroid (Overshoot uu past
	 *  it along the line of advance, so blocks grind instead of parking on the point); thinned
	 *  handles are refilled from the reserve at HomeZone. Call on the decision cadence. */
	void Decide(USwarmSubsystem& Swarm, const FVector& EnemyCentroid, int32 EnemyStanding, float Overshoot, float ReinforceFloor);

	int32 LiveStanding(const USwarmSubsystem& Swarm) const;
	FVector LiveCentroid(const USwarmSubsystem& Swarm) const;
	int32 GetReserve() const { return Reserve; }
	uint8 GetTeamId() const { return TeamId; }
	const TArray<int32>& GetHandles() const { return Handles; }

private:
	uint8 TeamId = 0;
	FVector HomeZone = FVector::ZeroVector;
	int32 Reserve = 0;
	int32 StartPerHandle = 0;
	UPROPERTY()
	TArray<TObjectPtr<UBattlegroundCommander>> Companies;
	TArray<int32> Handles;
};
