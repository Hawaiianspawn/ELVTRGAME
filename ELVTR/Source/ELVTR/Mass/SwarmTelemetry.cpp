#include "SwarmTelemetry.h"

#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SwarmSubsystem.h"

namespace
{
	/** Silence between deaths/damage that means the engagement is over. */
	constexpr double IdleEndSeconds = 2.5;

	/** A fight can't be declared from a single stray hit. */
	constexpr double MinFightSeconds = 0.5;

	/** Rows are buffered and flushed in batches; a per-sample file write would
	 *  show up in the very frame times the perf group is trying to measure. */
	constexpr int32 FlushEveryRows = 64;

	FString CsvEscape(const FString& In)
	{
		return In.Contains(TEXT(",")) ? FString::Printf(TEXT("\"%s\""), *In) : In;
	}
}

const TCHAR* LexToString(ESwarmFightOutcome Outcome)
{
	switch (Outcome)
	{
	case ESwarmFightOutcome::BroodCleared:	return TEXT("BroodCleared");
	case ESwarmFightOutcome::RetinueWiped:	return TEXT("RetinueWiped");
	case ESwarmFightOutcome::HeroDown:		return TEXT("HeroDown");
	case ESwarmFightOutcome::Stalemate:		return TEXT("Stalemate");
	default:								return TEXT("InProgress");
	}
}

//----------------------------------------------------------------------
// Subsystem plumbing
//----------------------------------------------------------------------
TStatId USwarmTelemetrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USwarmTelemetrySubsystem, STATGROUP_Tickables);
}

bool USwarmTelemetrySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Game and PIE only: recording fights in an editor preview world would
	// interleave junk rows into the sweep file.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FString USwarmTelemetrySubsystem::TelemetryDir()
{
	return FPaths::ProjectSavedDir() / TEXT("SwarmTelemetry");
}

//----------------------------------------------------------------------
// Tick: detect, sample, resolve
//----------------------------------------------------------------------
void USwarmTelemetrySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	const int32 AliveRetinue = Swarm->GetAliveRetinue();
	const int32 AliveBrood = Swarm->GetAliveBrood();

	const int64 TotalKills = Swarm->GetTotalKilledRetinue() + Swarm->GetTotalKilledBrood();
	const double TotalDamage = Swarm->GetTotalDamageToRetinue() + Swarm->GetTotalDamageToBrood()
		+ Swarm->GetTotalHeroDamage();

	// "Something happened" — a death or applied damage since the last tick.
	const bool bActivity = (TotalKills != LastTotalKills) || (TotalDamage > LastTotalDamage + UE_KINDA_SMALL_NUMBER);
	LastTotalKills = TotalKills;
	LastTotalDamage = TotalDamage;

	// Swarm.Clear / ResetRunState zeroes the run totals underneath us. Diffing a
	// live fight against a stale baseline would emit negative kill counts into
	// the sweep file, so abandon the recording rather than write a poisoned row.
	if (bRecording
		&& (Swarm->GetTotalKilledRetinue() < BaseKilledRetinue || Swarm->GetTotalKilledBrood() < BaseKilledBrood))
	{
		UE_LOG(LogTemp, Warning, TEXT("SwarmFight %d: ABANDONED (run state reset mid-fight)"), Current.Index);
		FlushSampleRows();
		bRecording = false;
		return;
	}

	if (!bRecording)
	{
		// Start on first contact between two populated sides. Damage is the
		// signal rather than proximity: units standing near each other because
		// the spawn ring overlaps is not a fight.
		if (bAutoDetect && bActivity && AliveBrood > 0 && (AliveRetinue > 0 || Swarm->IsHeroAlive()))
		{
			StartFight(/*bManual=*/false);
		}
		else
		{
			return;
		}
	}

	if (bActivity)
	{
		LastActivityTime = Now;
		if (Current.TimeToFirstBlood < 0.0 && TotalKills > BaseKilledRetinue + BaseKilledBrood)
		{
			Current.TimeToFirstBlood = Now - Current.StartTime;
		}
	}

	// Stance dwell time, so a summary row can be read against what was ordered.
	const int32 StanceIndex = FMath::Clamp(static_cast<int32>(Swarm->GetStance()), 0, 3);
	Current.StanceSeconds[StanceIndex] += DeltaTime;

	Current.PeakHeroContacts = FMath::Max(Current.PeakHeroContacts, Swarm->GetHeroContactsThisFrame());
	Current.PeakLeashBroken = FMath::Max(Current.PeakLeashBroken, Swarm->GetLeashBrokenCount());

	if (SampleInterval > 0.f && Now >= NextSampleTime)
	{
		NextSampleTime = Now + SampleInterval;
		SampleNow(Now);
	}

	// --- resolution -------------------------------------------------------
	const double Elapsed = Now - Current.StartTime;
	if (Elapsed < MinFightSeconds)
	{
		return;
	}

	if (AliveBrood == 0)
	{
		EndFight(ESwarmFightOutcome::BroodCleared);
	}
	else if (!Swarm->IsHeroAlive())
	{
		EndFight(ESwarmFightOutcome::HeroDown);
	}
	else if (AliveRetinue == 0)
	{
		EndFight(ESwarmFightOutcome::RetinueWiped);
	}
	else if (Now - LastActivityTime > IdleEndSeconds)
	{
		EndFight(ESwarmFightOutcome::Stalemate);
	}
}

//----------------------------------------------------------------------
// Fight lifecycle
//----------------------------------------------------------------------
void USwarmTelemetrySubsystem::StartFight(bool bManual)
{
	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}
	if (bRecording)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwarmFight: already recording fight %d — ignoring start"), Current.Index);
		return;
	}

	const double Now = World->GetTimeSeconds();

	Current = FSwarmFightRecord();
	Current.Index = ++FightCounter;
	Current.StartTime = Now;
	Current.StartRetinue = Swarm->GetAliveRetinue();
	Current.StartBrood = Swarm->GetAliveBrood();
	Current.HeroHPStart = Swarm->GetHeroHP();

	BaseKilledRetinue = Swarm->GetTotalKilledRetinue();
	BaseKilledBrood = Swarm->GetTotalKilledBrood();
	BaseDamageToRetinue = Swarm->GetTotalDamageToRetinue();
	BaseDamageToBrood = Swarm->GetTotalDamageToBrood();
	BaseHeroDamage = Swarm->GetTotalHeroDamage();

	LastActivityTime = Now;
	NextSampleTime = Now;
	PendingRows.Reset();

	SampleFilePath = TelemetryDir() / FString::Printf(TEXT("fight_%03d_%s.csv"),
		Current.Index, *FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")));

	bRecording = true;
	WriteSampleHeader();

	UE_LOG(LogTemp, Display, TEXT("SwarmFight %d: START (%s) retinue=%d brood=%d stance=%s"),
		Current.Index, bManual ? TEXT("manual") : TEXT("auto"),
		Current.StartRetinue, Current.StartBrood, LexToString(Swarm->GetStance()));
}

void USwarmTelemetrySubsystem::EndFight(ESwarmFightOutcome Outcome)
{
	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!bRecording || !Swarm)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();

	Current.Duration = Now - Current.StartTime;
	Current.Outcome = Outcome;
	Current.EndRetinue = Swarm->GetAliveRetinue();
	Current.EndBrood = Swarm->GetAliveBrood();
	Current.HeroHPEnd = Swarm->GetHeroHP();

	Current.KilledRetinue = Swarm->GetTotalKilledRetinue() - BaseKilledRetinue;
	Current.KilledBrood = Swarm->GetTotalKilledBrood() - BaseKilledBrood;
	Current.DamageToRetinue = Swarm->GetTotalDamageToRetinue() - BaseDamageToRetinue;
	Current.DamageToBrood = Swarm->GetTotalDamageToBrood() - BaseDamageToBrood;
	Current.HeroDamageTaken = Swarm->GetTotalHeroDamage() - BaseHeroDamage;

	SampleNow(Now);	// final row, so the timeline ends where the summary says it does
	FlushSampleRows();
	AppendSummaryRow();

	bRecording = false;
	LogSummary();
}

//----------------------------------------------------------------------
// Sampling
//----------------------------------------------------------------------
void USwarmTelemetrySubsystem::SampleNow(double Now)
{
	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	PendingRows.Add(FString::Printf(
		TEXT("%.3f,%s,%d,%d,%lld,%lld,%.1f,%.1f,%.1f,%d,%d,%lld"),
		Now - Current.StartTime,
		LexToString(Swarm->GetStance()),
		Swarm->GetAliveRetinue(),
		Swarm->GetAliveBrood(),
		Swarm->GetTotalKilledRetinue() - BaseKilledRetinue,
		Swarm->GetTotalKilledBrood() - BaseKilledBrood,
		Swarm->GetHeroHP(),
		Swarm->GetTotalDamageToRetinue() - BaseDamageToRetinue,
		Swarm->GetTotalDamageToBrood() - BaseDamageToBrood,
		Swarm->GetHeroContactsThisFrame(),
		Swarm->GetLeashBrokenCount(),
		Swarm->GetHeroContacts()));

	if (PendingRows.Num() >= FlushEveryRows)
	{
		FlushSampleRows();
	}
}

void USwarmTelemetrySubsystem::WriteSampleHeader()
{
	// Tuning constants go in the file as comments: a timeline you can't attribute
	// to a balance pass is a timeline you'll re-record.
	const FString Header = FString::Printf(
		TEXT("# ELVTR swarm fight %d — recorded %s\n")
		TEXT("# retinue HP=%.0f DPS=%.0f | brood HP=%.0f DPS=%.0f | hero HP=%.0f DPS=%.0f\n")
		TEXT("# melee=%.0f maxAttackersPerUnit=%d leash=%.0f\n")
		TEXT("t,stance,aliveRetinue,aliveBrood,killedRetinue,killedBrood,heroHP,dmgToRetinue,dmgToBrood,heroContactsNow,leashBroken,heroContactsTotal\n"),
		Current.Index, *FDateTime::Now().ToString(),
		SwarmCombatTuning::RetinueMaxHP, SwarmCombatTuning::RetinueDPS,
		SwarmCombatTuning::BroodMaxHP, SwarmCombatTuning::BroodDPS,
		SwarmCombatTuning::HeroMaxHP, SwarmCombatTuning::HeroDPS,
		SwarmCombatTuning::MeleeRange, SwarmCombatTuning::MaxAttackersPerUnit,
		SwarmLeash::Radius);

	FFileHelper::SaveStringToFile(Header, *SampleFilePath);
}

void USwarmTelemetrySubsystem::FlushSampleRows()
{
	if (PendingRows.Num() == 0)
	{
		return;
	}

	FString Block;
	for (const FString& Row : PendingRows)
	{
		Block += Row + TEXT("\n");
	}
	PendingRows.Reset();

	FFileHelper::SaveStringToFile(Block, *SampleFilePath,
		FFileHelper::EEncodingOptions::ForceAnsi,	// no repeated BOM mid-file
		&IFileManager::Get(), FILEWRITE_Append);
}

void USwarmTelemetrySubsystem::AppendSummaryRow() const
{
	const FString Path = TelemetryDir() / TEXT("fights.csv");

	// Header written once, on first creation.
	if (!IFileManager::Get().FileExists(*Path))
	{
		const FString Header =
			TEXT("timestamp,fight,outcome,duration,")
			TEXT("startRetinue,startBrood,endRetinue,endBrood,")
			TEXT("killedRetinue,killedBrood,exchangeRate,")
			TEXT("dmgToRetinue,dmgToBrood,heroDamageTaken,heroHPStart,heroHPEnd,")
			TEXT("timeToFirstBlood,peakHeroContacts,peakLeashBroken,")
			TEXT("secFollow,secCharge,secHold,secRally,")
			TEXT("retinueHP,retinueDPS,broodHP,broodDPS,heroHP,heroDPS,meleeRange,maxAttackersPerUnit\n");
		FFileHelper::SaveStringToFile(Header, *Path);
	}

	const FString Row = FString::Printf(
		TEXT("%s,%d,%s,%.2f,%d,%d,%d,%d,%lld,%lld,%.3f,%.1f,%.1f,%.1f,%.1f,%.1f,%.2f,%d,%d,")
		TEXT("%.2f,%.2f,%.2f,%.2f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%d\n"),
		*CsvEscape(FDateTime::Now().ToString()),
		Current.Index, LexToString(Current.Outcome), Current.Duration,
		Current.StartRetinue, Current.StartBrood, Current.EndRetinue, Current.EndBrood,
		Current.KilledRetinue, Current.KilledBrood, Current.ExchangeRate(),
		Current.DamageToRetinue, Current.DamageToBrood, Current.HeroDamageTaken,
		Current.HeroHPStart, Current.HeroHPEnd,
		Current.TimeToFirstBlood, Current.PeakHeroContacts, Current.PeakLeashBroken,
		Current.StanceSeconds[0], Current.StanceSeconds[1],
		Current.StanceSeconds[2], Current.StanceSeconds[3],
		SwarmCombatTuning::RetinueMaxHP, SwarmCombatTuning::RetinueDPS,
		SwarmCombatTuning::BroodMaxHP, SwarmCombatTuning::BroodDPS,
		SwarmCombatTuning::HeroMaxHP, SwarmCombatTuning::HeroDPS,
		SwarmCombatTuning::MeleeRange, SwarmCombatTuning::MaxAttackersPerUnit);

	FFileHelper::SaveStringToFile(Row, *Path,
		FFileHelper::EEncodingOptions::ForceAnsi,	// no repeated BOM mid-file
		&IFileManager::Get(), FILEWRITE_Append);
}

void USwarmTelemetrySubsystem::LogSummary() const
{
	UE_LOG(LogTemp, Display,
		TEXT("SwarmFight %d: %s in %.1fs | retinue %d->%d (lost %lld) brood %d->%d (killed %lld) ")
		TEXT("| exchange %.2f brood per retinue | hero %.0f->%.0f (took %.0f) | peak mobbed %d | first blood %.2fs"),
		Current.Index, LexToString(Current.Outcome), Current.Duration,
		Current.StartRetinue, Current.EndRetinue, Current.KilledRetinue,
		Current.StartBrood, Current.EndBrood, Current.KilledBrood,
		Current.ExchangeRate(),
		Current.HeroHPStart, Current.HeroHPEnd, Current.HeroDamageTaken,
		Current.PeakHeroContacts, Current.TimeToFirstBlood);
}

//----------------------------------------------------------------------
// Console commands
//----------------------------------------------------------------------
namespace
{
	USwarmTelemetrySubsystem* GetTelemetry(UWorld* World)
	{
		return World ? World->GetSubsystem<USwarmTelemetrySubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GFightStartCmd(
		TEXT("Swarm.Fight.Start"),
		TEXT("Begin recording a fight now, without waiting for auto-detection."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (USwarmTelemetrySubsystem* Telemetry = GetTelemetry(World))
				{
					Telemetry->StartFight(/*bManual=*/true);
				}
			}));

	FAutoConsoleCommandWithWorldAndArgs GFightStopCmd(
		TEXT("Swarm.Fight.Stop"),
		TEXT("Close the current fight recording and write its summary row."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (USwarmTelemetrySubsystem* Telemetry = GetTelemetry(World))
				{
					Telemetry->EndFight(ESwarmFightOutcome::Stalemate);
				}
			}));

	FAutoConsoleCommandWithWorldAndArgs GFightAutoCmd(
		TEXT("Swarm.Fight.Auto"),
		TEXT("Enable/disable automatic fight detection. Usage: Swarm.Fight.Auto 0|1"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmTelemetrySubsystem* Telemetry = GetTelemetry(World);
				if (!Telemetry)
				{
					return;
				}
				const bool bEnabled = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : true;
				Telemetry->SetAutoDetect(bEnabled);
				UE_LOG(LogTemp, Display, TEXT("SwarmFight: auto-detect %s"), bEnabled ? TEXT("on") : TEXT("off"));
			}));

	FAutoConsoleCommandWithWorldAndArgs GFightSampleHzCmd(
		TEXT("Swarm.Fight.SampleHz"),
		TEXT("Timeline sample rate in Hz (0 disables per-sample rows). Usage: Swarm.Fight.SampleHz 4"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmTelemetrySubsystem* Telemetry = GetTelemetry(World);
				if (!Telemetry || Args.Num() == 0)
				{
					return;
				}
				Telemetry->SetSampleHz(FCString::Atof(*Args[0]));
			}));

	FAutoConsoleCommandWithWorldAndArgs GFightStatusCmd(
		TEXT("Swarm.Fight.Status"),
		TEXT("Log the running (or most recent) fight summary."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (const USwarmTelemetrySubsystem* Telemetry = GetTelemetry(World))
				{
					UE_LOG(LogTemp, Display, TEXT("SwarmFight: recording=%d auto=%d"),
						Telemetry->IsRecording() ? 1 : 0, Telemetry->IsAutoDetectEnabled() ? 1 : 0);
					Telemetry->LogSummary();
				}
			}));
}
