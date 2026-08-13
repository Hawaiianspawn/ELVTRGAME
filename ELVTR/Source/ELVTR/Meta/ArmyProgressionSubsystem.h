#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ArmyProgressionSubsystem.generated.h"

/**
 * Cross-run army meta-progression: army carries between runs, level scales
 * automatically from lifetime kills, monotonic and never falls.
 *
 * Lives on the GAME INSTANCE, not the world, specifically so it survives
 * ASpike1GameMode::RestartRun and any level travel -- USwarmSubsystem is a
 * UWorldSubsystem and gets torn down with its world; this does not.
 *
 * LifetimeKills only ever grows: each tick it reads USwarmSubsystem's run-scoped
 * GetTotalKilledRetinue()+GetTotalKilledBrood() (both already monotonic within a run
 * -- see SwarmSubsystem.h) and folds the delta in. If a new world's counters read
 * LOWER than what was last observed (a fresh SwarmSubsystem after a level load), the
 * whole new reading is treated as newly-earned kills rather than a negative delta --
 * so a world swap can only add to LifetimeKills, never subtract it. ArmyLevel is a
 * pure read-time function of LifetimeKills (ArmyProgression.h), so it inherits that
 * same never-falls guarantee for free instead of needing its own clamp.
 */
UCLASS()
class ELVTR_API UArmyProgressionSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

	int64 GetLifetimeKills() const { return LifetimeKills; }
	int32 GetArmyLevel() const;

	/**
	 * Folds Kills into the persisted lifetime total; never decreases it. Exposed
	 * directly (not just reached via Tick's SwarmSubsystem polling) so anything else
	 * that earns lifetime credit later doesn't need a live SwarmSubsystem to do it.
	 */
	void AddLifetimeKills(int64 Kills);

	/** Writes the current LifetimeKills to disk now. Auto-called on an interval and
	 *  on Deinitialize; exposed so a caller (e.g. an end-of-run hook) can force it. */
	void SaveNow();

	static const FString& SaveSlotName();

private:
	void LoadFromDisk();

	int64 LifetimeKills = 0;

	/** Last CurrentTotal read from a USwarmSubsystem, used to derive Tick's delta. */
	int64 LastObservedRunKills = 0;

	float TimeSinceAutosave = 0.f;

	static constexpr float AutosaveIntervalSeconds = 30.f;
};
