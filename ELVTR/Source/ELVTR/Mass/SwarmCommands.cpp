// Spawn implementation (SwarmSpawn.h) + the console commands that drive it:
//   Swarm.SpawnBrood <N>    - spawn N brood in a ring around the hero
//   Swarm.SpawnRetinue <N>  - spawn N retinue in formation slots around the hero
//   Swarm.Clear             - destroy all swarm entities
//   Swarm.Stance <name>     - Follow | Charge | Hold | Rally

#include "SwarmSpawn.h"

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "MassMovementFragments.h"
#include "SwarmCombat.h"
#include "SwarmFragments.h"
#include "SwarmSubsystem.h"

namespace
{
	struct FSwarmSpawnParams
	{
		bool bBrood = true;
		int32 Count = 0;
		int32 SlotBase = 0;
	};

	/** Concentric rings: ring r holds 8*r slots at 110uu spacing. */
	FVector2D FormationSlot(int32 Index)
	{
		int32 Ring = 1;
		int32 SlotsBefore = 0;
		while (Index >= SlotsBefore + Ring * 8)
		{
			SlotsBefore += Ring * 8;
			++Ring;
		}
		const int32 SlotInRing = Index - SlotsBefore;
		const float Angle = (2.f * PI * SlotInRing) / (Ring * 8);
		return FVector2D(FMath::Cos(Angle) * Ring * 110.f, FMath::Sin(Angle) * Ring * 110.f);
	}

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
			FSwarmHealthFragment::StaticStruct(),
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

		const float MaxHP = Params.bBrood ? SwarmCombatTuning::BroodMaxHP : SwarmCombatTuning::RetinueMaxHP;

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
				const FVector2D Offset = FormationSlot(Params.SlotBase + Index);
				View.GetFragmentData<FRetinueFollowFragment>().SlotOffset = Offset;
				SpawnLocation = Center + FVector(Offset.X, Offset.Y, 0.f);
			}

			View.GetFragmentData<FTransformFragment>().GetMutableTransform().SetTranslation(SpawnLocation);

			FSwarmJitterFragment& JitterFragment = View.GetFragmentData<FSwarmJitterFragment>();
			JitterFragment.SpeedScale = Rand.FRandRange(0.85f, 1.15f);
			JitterFragment.Phase = Rand.FRandRange(0.f, 10.f);

			FSwarmHealthFragment& HealthFragment = View.GetFragmentData<FSwarmHealthFragment>();
			HealthFragment.MaxHP = MaxHP;
			HealthFragment.HP = MaxHP;

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
				SwarmSpawn::SpawnBrood(World, ParseCount(Args, 1000));
			}));

	FAutoConsoleCommandWithWorldAndArgs GSpawnRetinueCmd(
		TEXT("Swarm.SpawnRetinue"),
		TEXT("Spawn N retinue entities in formation around the hero. Usage: Swarm.SpawnRetinue 100"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SwarmSpawn::SpawnRetinue(World, ParseCount(Args, 100), 0);
			}));

	FAutoConsoleCommandWithWorldAndArgs GClearCmd(
		TEXT("Swarm.Clear"),
		TEXT("Destroy all swarm entities."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SwarmSpawn::ClearAll(World);
			}));

	FAutoConsoleCommandWithWorldAndArgs GStanceCmd(
		TEXT("Swarm.Stance"),
		TEXT("Set the retinue stance. Usage: Swarm.Stance Follow|Charge|Hold|Rally"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm || Args.Num() == 0)
				{
					return;
				}
				const FString& Name = Args[0];
				ESwarmStance Stance = ESwarmStance::Follow;
				if (Name.Equals(TEXT("Charge"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Charge; }
				else if (Name.Equals(TEXT("Hold"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Hold; }
				else if (Name.Equals(TEXT("Rally"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Rally; }

				Swarm->SetStance(Stance, Swarm->GetAttractor());
				UE_LOG(LogTemp, Display, TEXT("Swarm: stance = %s"), LexToString(Stance));
			}));
}

namespace SwarmSpawn
{
	void SpawnBrood(UWorld* World, int32 Count)
	{
		SpawnSwarm(World, FSwarmSpawnParams{ true, Count, 0 });
	}

	void SpawnRetinue(UWorld* World, int32 Count, int32 SlotBase)
	{
		SpawnSwarm(World, FSwarmSpawnParams{ false, Count, SlotBase });
	}

	void ClearAll(UWorld* World)
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
	}

	void CompactTracked(UWorld* World)
	{
		UMassEntitySubsystem* MassSubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
		USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!MassSubsystem || !Swarm)
		{
			return;
		}
		const FMassEntityManager& EntityManager = MassSubsystem->GetEntityManager();
		Swarm->GetTrackedEntities().RemoveAllSwap(
			[&EntityManager](const FMassEntityHandle& Handle)
			{
				return !EntityManager.IsEntityValid(Handle);
			},
			EAllowShrinking::No);
	}
}
