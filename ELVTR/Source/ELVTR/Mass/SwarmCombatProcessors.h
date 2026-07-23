#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SwarmCombatProcessors.generated.h"

// Slots into the chain as:
//   GridBuild -> Steering -> Combat -> Integrate -> Death / Contact

/**
 * Continuous melee attrition. Each unit counts enemies inside MeleeRange via the
 * shared grid and bleeds HP; brood additionally trade damage with the hero.
 * Chunk-local writes only — no cross-entity random access.
 */
UCLASS()
class USwarmCombatProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USwarmCombatProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/** Destroys entities whose HP hit zero (deferred; flushed at phase end). */
UCLASS()
class USwarmDeathProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USwarmDeathProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};
