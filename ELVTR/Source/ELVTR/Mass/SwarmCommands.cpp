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
	// How far out the brood tide appears and converges from. Under the flame
	// (a ~900uu light pool) these spawn deep in the dark and emerge as they close.
	// There is no distance culling on the renderer, so this ring is the only
	// "how far out do enemies appear" number.
	TAutoConsoleVariable<float> CVarBroodSpawnRadiusMin(
		TEXT("Swarm.BroodSpawnRadiusMin"), 2500.f,
		TEXT("Inner radius (uu) of the ring brood spawn in, measured from the hero."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodSpawnRadiusMax(
		TEXT("Swarm.BroodSpawnRadiusMax"), 4000.f,
		TEXT("Outer radius (uu) of the brood spawn ring. Keep >= BroodSpawnRadiusMin."), ECVF_Default);

	// A full 360 ring is the "surrounded" case and it is the only one the spike could
	// stage. An arc is what lets a wave arrive as a FRONT — which is the situation the
	// stances are actually about, since Hold only means something if there is a
	// direction to hold against.
	TAutoConsoleVariable<float> CVarBroodSpawnArc(
		TEXT("Swarm.BroodSpawnArc"), 360.f,
		TEXT("Width in DEGREES of the arc brood spawn along. 360 = surrounded on all\n")
		TEXT("sides; ~90 = one flank; ~30 = a column down one approach. [0..360]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSpawnArcCenter(
		TEXT("Swarm.BroodSpawnArcCenter"), 0.f,
		TEXT("Bearing in degrees the spawn arc is centred on, world +X = 0, CCW positive.\n")
		TEXT("Ignored while Arc is 360. [-180..180]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSpeedJitter(
		TEXT("Swarm.BroodSpeedJitter"), 0.15f,
		TEXT("Per-brood speed variation, +/- this fraction of Swarm.BroodSpeed, rolled at\n")
		TEXT("spawn. This is what strings the tide out into a ragged arrival instead of one\n")
		TEXT("rigid wall — 0 makes the whole wave land on the same frame. [0..0.8]"), ECVF_Default);

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
			FSwarmStrikeFragment::StaticStruct(),
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

		const float MaxHP = Params.bBrood ? SwarmCombatTuning::BroodMaxHP() : SwarmCombatTuning::RetinueMaxHP();
		const float SpawnRadiusMin = CVarBroodSpawnRadiusMin.GetValueOnGameThread();
		const float SpawnRadiusMax = CVarBroodSpawnRadiusMax.GetValueOnGameThread();

		// Arc, in radians, as a half-width either side of the centre bearing.
		const float ArcHalf = FMath::DegreesToRadians(
			FMath::Clamp(CVarBroodSpawnArc.GetValueOnGameThread(), 0.f, 360.f) * 0.5f);
		const float ArcCenter = FMath::DegreesToRadians(CVarBroodSpawnArcCenter.GetValueOnGameThread());

		// Retinue keeps the original fixed +/-15%: your line's raggedness isn't a dial
		// anyone has asked to move, and the formation slots already stagger it.
		constexpr float RetinueSpeedJitter = 0.15f;
		const float SpeedJitter = Params.bBrood
			? FMath::Clamp(CVarBroodSpeedJitter.GetValueOnGameThread(), 0.f, 0.95f)
			: RetinueSpeedJitter;

		for (int32 Index = 0; Index < Entities.Num(); ++Index)
		{
			FMassEntityView View(EntityManager, Entities[Index]);

			FVector SpawnLocation;
			if (Params.bBrood)
			{
				// Ring well outside the play area so the tide visibly converges.
				const float Angle = ArcCenter + Rand.FRandRange(-ArcHalf, ArcHalf);
				const float Radius = Rand.FRandRange(SpawnRadiusMin, SpawnRadiusMax);
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
			JitterFragment.SpeedScale = Rand.FRandRange(1.f - SpeedJitter, 1.f + SpeedJitter);
			JitterFragment.Phase = Rand.FRandRange(0.f, 10.f);

			FSwarmHealthFragment& HealthFragment = View.GetFragmentData<FSwarmHealthFragment>();
			HealthFragment.MaxHP = MaxHP;
			HealthFragment.HP = MaxHP;

			// Desynchronise the swing clocks. Reuse the jitter phase that already
			// staggers the walk cycle — spawning a wave with every unit's swing at
			// zero would make the whole front line strike on the same frame, which
			// reads as one pulsing organism rather than a mêlée.
			View.GetFragmentData<FSwarmStrikeFragment>().SwingTime =
				FMath::Fmod(JitterFragment.Phase, SwarmCombatTuning::SwingInterval());

			if (!Params.bBrood)
			{
				FSwarmAnimFragment& AnimFragment = View.GetFragmentData<FSwarmAnimFragment>();
				AnimFragment.Bits = SwarmAnim::TeamBit;
				AnimFragment.SquadId = (uint8)USwarmSubsystem::SquadIdForSlot(Params.SlotBase + Index);
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
