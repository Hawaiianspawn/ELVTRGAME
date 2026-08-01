#pragma once

#include "CoreMinimal.h"
#include "SwarmCombat.h"
#include "Subsystems/WorldSubsystem.h"
#include "SwarmTelemetry.generated.h"

/**
 * Fight recorder — the balance instrument.
 *
 * `stat Swarm` answers "what did this frame cost". This answers "was the fight
 * fair". It watches the swarm subsystem's monotonic run totals, decides when an
 * engagement starts and ends, and writes two things:
 *
 *   Saved/SwarmTelemetry/fight_<n>_<stamp>.csv   per-sample timeline of one fight
 *   Saved/SwarmTelemetry/fights.csv              one summary row per fight, appended
 *
 * `fights.csv` carries the SwarmCombatTuning constants in every row on purpose.
 * A row without the numbers it was produced under is unattributable the moment
 * anyone edits a DPS value, and sweeping tuning passes is the whole point.
 *
 * The headline number is the exchange rate: brood killed per retinue lost. GDD
 * design law "scale by more enemies, not spongier enemies" is a claim about that
 * ratio holding as brood count rises. This is how you check it.
 */

UENUM()
enum class ESwarmFightOutcome : uint8
{
	InProgress,
	BroodCleared,	// the line held
	RetinueWiped,	// the line broke
	HeroDown,
	Stalemate,		// activity stopped with both sides alive
};

const TCHAR* LexToString(ESwarmFightOutcome Outcome);

/** One fight's worth of accumulated facts. */
struct FSwarmFightRecord
{
	int32 Index = 0;
	double StartTime = 0.0;
	double Duration = 0.0;
	ESwarmFightOutcome Outcome = ESwarmFightOutcome::InProgress;

	int32 StartRetinue = 0;
	int32 StartBrood = 0;
	int32 EndRetinue = 0;
	int32 EndBrood = 0;

	int64 KilledRetinue = 0;
	int64 KilledBrood = 0;

	double DamageToRetinue = 0.0;
	double DamageToBrood = 0.0;
	double HeroDamageTaken = 0.0;

	float HeroHPStart = 0.f;
	float HeroHPEnd = 0.f;

	/** Worst moment, not the average — the average hides the spike that killed you. */
	int32 PeakHeroContacts = 0;
	int32 PeakLeashBroken = 0;

	double TimeToFirstBlood = -1.0;
	/** Seconds spent in each stance, indexed by ESwarmStance. */
	double StanceSeconds[4] = { 0.0, 0.0, 0.0, 0.0 };

	/** Brood killed per retinue lost. The one number balance lives or dies on. */
	double ExchangeRate() const
	{
		return KilledRetinue > 0 ? static_cast<double>(KilledBrood) / KilledRetinue
								 : static_cast<double>(KilledBrood);
	}
};

UCLASS()
class ELVTR_API USwarmTelemetrySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- UTickableWorldSubsystem ------------------------------------------
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// --- control (also exposed as Swarm.Fight.* console commands) ----------
	void StartFight(bool bManual);
	void EndFight(ESwarmFightOutcome Outcome);

	void SetAutoDetect(bool bEnabled) { bAutoDetect = bEnabled; }
	bool IsAutoDetectEnabled() const { return bAutoDetect; }
	void SetSampleHz(float Hz) { SampleInterval = Hz > 0.f ? 1.f / Hz : 0.f; }

	bool IsRecording() const { return bRecording; }
	const FSwarmFightRecord& GetCurrentRecord() const { return Current; }

	/** Log the running (or last) fight as a one-line summary. */
	void LogSummary() const;

private:
	void SampleNow(double Now);
	void WriteSampleHeader();
	void FlushSampleRows();
	void AppendSummaryRow() const;
	static FString TelemetryDir();

	bool bRecording = false;
	bool bAutoDetect = true;

	FSwarmFightRecord Current;
	int32 FightCounter = 0;

	// Baselines captured at fight start; run totals are differenced against these.
	int64 BaseKilledRetinue = 0;
	int64 BaseKilledBrood = 0;
	double BaseDamageToRetinue = 0.0;
	double BaseDamageToBrood = 0.0;
	double BaseHeroDamage = 0.0;

	/** Activity watchdog: an engagement ends when nothing has happened for a while. */
	double LastActivityTime = 0.0;
	int64 LastTotalKills = 0;
	double LastTotalDamage = 0.0;

	float SampleInterval = 0.5f;	// 2 Hz — fine enough to see a line collapse
	double NextSampleTime = 0.0;

	FString SampleFilePath;
	TArray<FString> PendingRows;
};
