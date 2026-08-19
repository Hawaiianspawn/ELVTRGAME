#include "StressWarGameMode.h"

#include "StressWarSide.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"

AStressWarGameMode::AStressWarGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr; // no hero: fixed ortho camera below
}

void AStressWarGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	auto Opt = [&Options](const TCHAR* Key, int32& Out)
	{
		if (UGameplayStatics::HasOption(Options, Key)) { Out = UGameplayStatics::GetIntOption(Options, Key, Out); }
	};
	Opt(TEXT("PerSide"), PerSide);
	Opt(TEXT("Reserve"), Reserve);
	Opt(TEXT("Companies"), CompaniesPerSide);
	int32 MaxSeconds = (int32)MaxMatchSeconds;
	Opt(TEXT("MaxSeconds"), MaxSeconds);
	MaxMatchSeconds = (float)MaxSeconds;
}

void AStressWarGameMode::BeginPlay()
{
	Super::BeginPlay();

	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		UE_LOG(LogTemp, Error, TEXT("StressWar: no USwarmSubsystem"));
		return;
	}

	SwarmSpawn::ClearAll(GetWorld());
	Swarm->ResetRunState();
	Swarm->SetFieldBattle(true);

	// Same CVar-at-console-priority idiom as BattlegroundGameMode (SetByCode is ignored).
	auto SetCVar = [](const TCHAR* Name, const FString& Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) { V->Set(*Value, ECVF_SetByConsole); }
		else { UE_LOG(LogTemp, Warning, TEXT("StressWar: no CVar %s"), Name); }
	};
	const FString Cols = FString::FromInt(FMath::Max(1, Columns));
	const int32 HandlesPerSide = USwarmSubsystem::MaxSquads / 2;
	const int32 Companies = FMath::Clamp(CompaniesPerSide, 1, HandlesPerSide / UStressWarSide::HandlesPerCompany);
	const int32 PerHandle = FMath::Max(1, PerSide / (Companies * 2)); // 2 troop handles per company
	// Row pitch = one detachment's depth plus a gap, so wrapped rows never grow into each
	// other. A handle deeper than GroupDepthCap (16) ranks opens sibling detachments.
	const int32 Ranks = FMath::Min(16, FMath::DivideAndRoundUp(PerHandle, FMath::Max(1, Columns)));
	const FString RowPitch = FString::SanitizeFloat(Ranks * 52.f + 200.f);
	SetCVar(TEXT("Swarm.Formation.Shape"), TEXT("1"));
	SetCVar(TEXT("Swarm.Formation.Columns"), Cols);
	SetCVar(TEXT("Swarm.Formation.Archers.Shape"), TEXT("1"));
	SetCVar(TEXT("Swarm.Formation.Archers.Columns"), Cols);
	SetCVar(TEXT("Swarm.Formation.Spacing"), TEXT("48"));
	SetCVar(TEXT("Swarm.Formation.RankSpacing"), TEXT("52"));
	SetCVar(TEXT("Swarm.Formation.Archers.Spacing"), TEXT("48"));
	SetCVar(TEXT("Swarm.Formation.Archers.RankSpacing"), TEXT("52"));
	SetCVar(TEXT("Swarm.Formation.GroupGap"), TEXT("110"));
	// 0 = every one of a side's handles abreast in ONE row. Rows behind the front are what
	// held the two armies apart: a block parks at anchor + slot, and a wrapped row's slot is
	// a whole GroupRowPitch further along, so with 3 abreast the armies stalled 4200uu apart
	// after the first clash and stopped killing. One row per side: gap ~1600uu, even
	// attrition, both reserves spent (measured 2026-08-19).
	const int32 Abreast = (BlocksAbreast > 0) ? BlocksAbreast : Companies * UStressWarSide::HandlesPerCompany;
	SetCVar(TEXT("Swarm.Formation.GroupsPerRow"), FString::FromInt(Abreast));
	SetCVar(TEXT("Swarm.Formation.GroupRowPitch"), RowPitch);
	SetCVar(TEXT("Swarm.Formation.GroupDepthCap"), TEXT("16"));
	SetCVar(TEXT("Swarm.Formation.FaceCamera"), TEXT("0"));
	SetCVar(TEXT("Swarm.Formation.Yaw"), TEXT("0"));
	SetCVar(TEXT("Swarm.RetinueSizeJitter"), TEXT("0"));
	// Mirror the two armies' STATS while keeping their looks apart: the shipped
	// Swarm.KnightSubtypeMap sends look 1 to row 8 (HP 114 / DPS 25) and look 7 to row 1
	// (HP 165 / DPS 30), so side A was fighting side B two weight classes down -- that, not
	// the war manager, is what the first runs' 2:1 result measured. Every look on row 0 here.
	SetCVar(TEXT("Swarm.KnightSubtypeMap"), TEXT("0,0,0,0,0,0,0,0,0,0,0"));

	const FVector ZoneA(-DeploymentDistance * 0.5f, 0.f, 0.f);
	const FVector ZoneB(+DeploymentDistance * 0.5f, 0.f, 0.f);

	SideA = NewObject<UStressWarSide>(this);
	SideA->Muster(*Swarm, 0, 0, Companies, PerHandle, Reserve, TeamAMeleeLook, ArcherLook, LeadHPScale, ZoneA, ZoneB);
	SideB = NewObject<UStressWarSide>(this);
	SideB->Muster(*Swarm, 1, HandlesPerSide, Companies, PerHandle, Reserve, TeamBMeleeLook, ArcherLook, LeadHPScale, ZoneB, ZoneA);
	Swarm->SetAttractor(FVector::ZeroVector);

	if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(FVector(0.f, 0.f, 3000.f), FRotator(-90.f, 90.f, 0.f)))
	{
		Cam->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
		Cam->GetCameraComponent()->SetOrthoWidth(DeploymentDistance * 2.f);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			PC->SetViewTarget(Cam);
		}
	}

	StartTime = GetWorld()->GetTimeSeconds();
	Countdown = DecisionIntervalSeconds;
	bCsvStarted = false;
	bEnded = false;
	UE_LOG(LogTemp, Display, TEXT("StressWar: %d vs %d fielded (%d companies x [lead + 2 x %d] per side, %d handles of %d), +%d reserve each, zones %.0fuu apart"),
		PerHandle * Companies * 2, PerHandle * Companies * 2, Companies, PerHandle,
		Companies * UStressWarSide::HandlesPerCompany * 2, USwarmSubsystem::MaxSquads, Reserve, DeploymentDistance);
}

void AStressWarGameMode::WriteCsv(const FString& Row)
{
	const FString Path = FPaths::ProjectSavedDir() / TEXT("StressWar.csv");
	if (!bCsvStarted)
	{
		bCsvStarted = true;
		FFileHelper::SaveStringToFile(TEXT("t,standingA,standingB,reserveA,reserveB,gap_uu,frame_ms,game_ms,render_ms,gpu_ms\n"), *Path);
	}
	FFileHelper::SaveStringToFile(Row + TEXT("\n"), *Path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

void AStressWarGameMode::EndMatch(const TCHAR* Why)
{
	if (bEnded) { return; }
	bEnded = true;
	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	UE_LOG(LogTemp, Display, TEXT("StressWar: END (%s) at t=%.0fs: A %d standing / %d reserve, B %d standing / %d reserve. CSV: %s"),
		Why, GetWorld()->GetTimeSeconds() - StartTime,
		Swarm ? SideA->LiveStanding(*Swarm) : -1, SideA->GetReserve(),
		Swarm ? SideB->LiveStanding(*Swarm) : -1, SideB->GetReserve(),
		*(FPaths::ProjectSavedDir() / TEXT("StressWar.csv")));
}

void AStressWarGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bEnded || !SideA || !SideB) { return; }
	Countdown -= DeltaSeconds;
	if (Countdown > 0.f) { return; }
	Countdown = DecisionIntervalSeconds;

	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm) { return; }

	const int32 StandingA = SideA->LiveStanding(*Swarm);
	const int32 StandingB = SideB->LiveStanding(*Swarm);
	const FVector CentroidA = SideA->LiveCentroid(*Swarm);
	const FVector CentroidB = SideB->LiveCentroid(*Swarm);

	SideA->Decide(*Swarm, CentroidB, StandingB, ChargeOvershoot, ReinforceFloor);
	SideB->Decide(*Swarm, CentroidA, StandingA, ChargeOvershoot, ReinforceFloor);

	const double T = GetWorld()->GetTimeSeconds() - StartTime;
	const float FrameMs = DeltaSeconds * 1000.f;
	const float GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
	const float RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
	const float GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
	WriteCsv(FString::Printf(TEXT("%.1f,%d,%d,%d,%d,%.0f,%.2f,%.2f,%.2f,%.2f"),
		T, StandingA, StandingB, SideA->GetReserve(), SideB->GetReserve(),
		FVector::Dist2D(CentroidA, CentroidB), FrameMs, GameMs, RenderMs, GpuMs));
	UE_LOG(LogTemp, Display, TEXT("StressWar: t=%.0fs A=%d(+%d) B=%d(+%d) gap=%.0fuu frame=%.2fms game=%.2fms render=%.2fms gpu=%.2fms"),
		T, StandingA, SideA->GetReserve(), StandingB, SideB->GetReserve(),
		FVector::Dist2D(CentroidA, CentroidB), FrameMs, GameMs, RenderMs, GpuMs);

	if ((StandingA + SideA->GetReserve()) <= 0 || (StandingB + SideB->GetReserve()) <= 0) { EndMatch(TEXT("a side is spent")); }
	else if (T >= MaxMatchSeconds) { EndMatch(TEXT("time")); }
}
