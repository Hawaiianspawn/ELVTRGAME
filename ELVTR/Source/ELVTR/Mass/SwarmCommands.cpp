// Console commands for the Spike 1 benchmark:
//   Swarm.SpawnBrood <N>    - spawn N brood in a ring around the hero
//   Swarm.SpawnRetinue <N>  - spawn N retinue in formation slots around the hero
//   Swarm.Clear             - destroy all swarm entities

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "MassMovementFragments.h"
#include "SwarmFragments.h"
#include "SwarmSubsystem.h"

namespace
{
	struct FSwarmSpawnParams
	{
		bool bBrood = true;
		int32 Count = 0;
	};

	void SpawnSwarm(UWorld* World, const FSwarmSpawnParams& Params)
	{
		if (!World || Params.Count <= 0)
		{
			return;
		}
		UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
		USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>();
		if (!MassSubsystem || !Swarm)
		{
			return;
		}

		FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

		TArray<const UScriptStruct*> Composition = {
			FTransformFragment::StaticStruct(),
			FMassVelocityFragment::StaticStruct(),
			FSwarmAnimFragment::StaticStruct(),
			FSwarmJitterFragment::StaticStruct(),
			FSwarmTag::StaticStruct()
		};
		if (Params.bBrood)
		{
			Composition.Add(FBroodTag::StaticStruct());
		}
		else
		{
			Composition.Add(FRetinueFollowFragment::StaticStruct());
			Composition.Add(FRetinueTag::StaticStruct());
		}

		const FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(Composition);

		TArray<FMassEntityHandle> Entities;
		Entities.Reserve(Params.Count);
		const TSharedRef<FMassEntityManager::FEntityCreationContext> CreationContext =
			EntityManager.BatchCreateEntities(Archetype, Params.Count, Entities);

		const FVector Center = Swarm->GetAttractor();
		FRandomStream Rand(FPlatformTime::Cycles());

		for (int32 Index = 0; Index < Entities.Num(); ++Index)
		{
			FMassEntityView View(EntityManager, Entities[Index]);

			FVector SpawnLocation;
			if (Params.bBrood)
			{
				// Ring well outside the play area so the tide visibly converges.
				const float Angle = Rand.FRandRange(0.f, 2.f * PI);
				const float Radius = Rand.FRandRange(2500.f, 4000.f);
				SpawnLocation = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
			}
			else
			{
				// Concentric formation rings: ring r holds 8*r slots at 110uu spacing.
				int32 Ring = 1;
				int32 SlotsBefore = 0;
				while (Index >= SlotsBefore + Ring * 8)
				{
					SlotsBefore += Ring * 8;
					++Ring;
				}
				const int32 SlotInRing = Index - SlotsBefore;
				const float Angle = (2.f * PI * SlotInRing) / (Ring * 8);
				const FVector2D Offset(FMath::Cos(Angle) * Ring * 110.f, FMath::Sin(Angle) * Ring * 110.f);

				View.GetFragmentData<FRetinueFollowFragment>().SlotOffset = Offset;
				SpawnLocation = Center + FVector(Offset.X, Offset.Y, 0.f);
			}

			View.GetFragmentData<FTransformFragment>().GetMutableTransform().SetTranslation(SpawnLocation);

			FSwarmJitterFragment& JitterFragment = View.GetFragmentData<FSwarmJitterFragment>();
			JitterFragment.SpeedScale = Rand.FRandRange(0.85f, 1.15f);
			JitterFragment.Phase = Rand.FRandRange(0.f, 10.f);

			if (!Params.bBrood)
			{
				View.GetFragmentData<FSwarmAnimFragment>().Bits = SwarmAnim::TeamBit;
			}
		}

		Swarm->TrackSpawned(Entities);
		UE_LOG(LogTemp, Display, TEXT("Swarm: spawned %d %s (total tracked: %d)"),
			Entities.Num(),
			Params.bBrood ? TEXT("brood") : TEXT("retinue"),
			Swarm->GetTrackedEntities().Num());
	}

	int32 ParseCount(const TArray<FString>& Args, int32 Default)
	{
		return Args.Num() > 0 ? FCString::Atoi(*Args[0]) : Default;
	}

	FAutoConsoleCommandWithWorldAndArgs GSpawnBroodCmd(
		TEXT("Swarm.SpawnBrood"),
		TEXT("Spawn N brood entities in a ring around the hero. Usage: Swarm.SpawnBrood 1000"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SpawnSwarm(World, FSwarmSpawnParams{ true, ParseCount(Args, 1000) });
			}));

	FAutoConsoleCommandWithWorldAndArgs GSpawnRetinueCmd(
		TEXT("Swarm.SpawnRetinue"),
		TEXT("Spawn N retinue entities in formation around the hero. Usage: Swarm.SpawnRetinue 100"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SpawnSwarm(World, FSwarmSpawnParams{ false, ParseCount(Args, 100) });
			}));

	FAutoConsoleCommandWithWorldAndArgs GClearCmd(
		TEXT("Swarm.Clear"),
		TEXT("Destroy all swarm entities."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (!World)
				{
					return;
				}
				UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
				USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>();
				if (!MassSubsystem || !Swarm)
				{
					return;
				}
				FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
				for (const FMassEntityHandle& Handle : Swarm->GetTrackedEntities())
				{
					if (EntityManager.IsEntityValid(Handle))
					{
						EntityManager.DestroyEntity(Handle);
					}
				}
				Swarm->GetTrackedEntities().Reset();
				UE_LOG(LogTemp, Display, TEXT("Swarm: cleared."));
			}));
}
