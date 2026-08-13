#include "Meta/ArmyProgressionSubsystem.h"

#include "ArmyProgression.h"
#include "ArmyProgressionSaveGame.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Mass/SwarmSubsystem.h"

const FString& UArmyProgressionSubsystem::SaveSlotName()
{
	static const FString SlotName = TEXT("ArmyProgression");
	return SlotName;
}

void UArmyProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadFromDisk();
}

void UArmyProgressionSubsystem::Deinitialize()
{
	SaveNow();
	Super::Deinitialize();
}

void UArmyProgressionSubsystem::LoadFromDisk()
{
	if (const UArmyProgressionSaveGame* Loaded = Cast<UArmyProgressionSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName(), 0)))
	{
		LifetimeKills = FMath::Max<int64>(0, Loaded->LifetimeKills);
	}
}

void UArmyProgressionSubsystem::SaveNow()
{
	UArmyProgressionSaveGame* SaveGame = Cast<UArmyProgressionSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UArmyProgressionSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return;
	}
	SaveGame->LifetimeKills = LifetimeKills;
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName(), 0);
}

void UArmyProgressionSubsystem::AddLifetimeKills(int64 Kills)
{
	if (Kills > 0)
	{
		LifetimeKills += Kills;
	}
}

int32 UArmyProgressionSubsystem::GetArmyLevel() const
{
	return ArmyProgression::ComputeLevelFromLifetimeKills(LifetimeKills);
}

TStatId UArmyProgressionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UArmyProgressionSubsystem, STATGROUP_Tickables);
}

void UArmyProgressionSubsystem::Tick(float DeltaSeconds)
{
	TimeSinceAutosave += DeltaSeconds;
	if (TimeSinceAutosave >= AutosaveIntervalSeconds)
	{
		TimeSinceAutosave = 0.f;
		SaveNow();
	}

	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	const int64 CurrentTotal = Swarm->GetTotalKilledRetinue() + Swarm->GetTotalKilledBrood();
	const int64 Delta = (CurrentTotal >= LastObservedRunKills)
		? (CurrentTotal - LastObservedRunKills)
		: CurrentTotal; // counters reset under us (new world) -- the whole reading is newly-earned
	AddLifetimeKills(Delta);
	LastObservedRunKills = CurrentTotal;
}

namespace
{
	// Same shape as the codebase's other Kindled.*.Report commands (Spike1GameMode.cpp) --
	// the only readout of this system until a HUD ratchet display exists.
	FAutoConsoleCommandWithWorld CmdArmyReport(
		TEXT("Kindled.Army.Report"),
		TEXT("Prints the persisted army's lifetime kills and derived ArmyLevel."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			const UArmyProgressionSubsystem* Army = GameInstance ? GameInstance->GetSubsystem<UArmyProgressionSubsystem>() : nullptr;
			if (!Army)
			{
				UE_LOG(LogTemp, Warning, TEXT("Kindled.Army.Report: no UArmyProgressionSubsystem"));
				return;
			}
			UE_LOG(LogTemp, Display, TEXT("Army: LifetimeKills=%lld ArmyLevel=%d"),
				Army->GetLifetimeKills(), Army->GetArmyLevel());
		}));
}
