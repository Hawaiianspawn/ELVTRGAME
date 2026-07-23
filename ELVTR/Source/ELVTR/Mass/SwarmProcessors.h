#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SwarmProcessors.generated.h"

// Execution chain (all PrePhysics):
//   GridBuild -> BroodSteering / RetinueFollow -> Combat -> Integrate -> Death / Contact
// Combat and Death live in SwarmCombatProcessors.h.

UCLASS()
class USwarmGridBuildProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USwarmGridBuildProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

UCLASS()
class UBroodSteeringProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UBroodSteeringProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

UCLASS()
class URetinueFollowProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	URetinueFollowProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

UCLASS()
class USwarmIntegrateProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USwarmIntegrateProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};

/** Counts brood in contact range of the hero — a gameplay-relevant query at full scale. */
UCLASS()
class USwarmContactProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USwarmContactProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};
