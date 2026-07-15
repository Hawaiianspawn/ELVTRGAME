#include "SwarmRenderActor.h"

#include "Engine/Engine.h"
#include "Mass/SwarmFragments.h"
#include "Mass/SwarmSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "RenderTimer.h"
#include "RHI.h"

ASwarmRenderActor::ASwarmRenderActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after Mass PrePhysics processing

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	RootComponent = NiagaraComponent;
	NiagaraComponent->SetAutoActivate(true);
}

void ASwarmRenderActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	// Debug hook for MCP-driven sessions (no interactive console): run any
	// console commands listed in Saved/SwarmExecOnPlay.txt at BeginPlay.
	FString ExecFileContents;
	if (FFileHelper::LoadFileToString(ExecFileContents, *(FPaths::ProjectSavedDir() / TEXT("SwarmExecOnPlay.txt"))))
	{
		TArray<FString> ExecLines;
		ExecFileContents.ParseIntoArrayLines(ExecLines);
		for (const FString& Line : ExecLines)
		{
			if (!Line.TrimStartAndEnd().IsEmpty())
			{
				BenchExec(Line);
			}
		}
	}

	const bool bArmed = bRunBenchmark || FParse::Param(FCommandLine::Get(), TEXT("SwarmBench"));
	if (World && World->IsGameWorld() && bArmed)
	{
		if (GEngine)
		{
			GEngine->bSmoothFrameRate = false; // uncapped frame rate so timings are real
		}
		BenchExec(TEXT("t.MaxFPS 0"));
		BenchExec(TEXT("r.VSync 0"));
		BenchStep = 0;
		BenchStartStep();
	}
}

void ASwarmRenderActor::BenchExec(const FString& Cmd)
{
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), *Cmd);
	}
}

void ASwarmRenderActor::BenchStartStep()
{
	if (BenchStep >= BenchmarkBroodCounts.Num())
	{
		BenchExec(TEXT("Swarm.Clear"));
		BenchPhase = EBenchPhase::Off;
		UE_LOG(LogTemp, Display, TEXT("SwarmBench: DONE"));
		return;
	}

	BenchExec(TEXT("Swarm.Clear"));
	BenchExec(FString::Printf(TEXT("Swarm.SpawnRetinue %d"), BenchmarkRetinueCount));
	BenchExec(FString::Printf(TEXT("Swarm.SpawnBrood %d"), BenchmarkBroodCounts[BenchStep]));
	BenchPhase = EBenchPhase::Settle;
	BenchTimer = 0.f;
}

void ASwarmRenderActor::BenchTick(float DeltaSeconds)
{
	if (BenchPhase == EBenchPhase::Off)
	{
		return;
	}

	BenchTimer += DeltaSeconds;

	if (BenchPhase == EBenchPhase::Settle)
	{
		if (BenchTimer >= BenchmarkSettleSeconds)
		{
			BenchPhase = EBenchPhase::Sample;
			BenchTimer = 0.f;
			BenchFrames = 0;
			BenchFrameMs = BenchGameMs = BenchRenderMs = BenchGpuMs = 0.0;
		}
		return;
	}

	// Sample phase: GGameThreadTime / GRenderThreadTime / GPU cycles are the
	// previous frame's thread times, updated once per frame by the engine.
	++BenchFrames;
	BenchFrameMs += DeltaSeconds * 1000.0;
	BenchGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
	BenchRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
	BenchGpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());

	if (BenchTimer >= BenchmarkSampleSeconds && BenchFrames > 0)
	{
		const double Inv = 1.0 / BenchFrames;
		UE_LOG(LogTemp, Display,
			TEXT("SwarmBench: brood=%d retinue=%d frame=%.2fms game=%.2fms draw=%.2fms gpu=%.2fms fps=%.1f"),
			BenchmarkBroodCounts[BenchStep], BenchmarkRetinueCount,
			BenchFrameMs * Inv, BenchGameMs * Inv, BenchRenderMs * Inv, BenchGpuMs * Inv,
			1000.0 / (BenchFrameMs * Inv));
		++BenchStep;
		BenchStartStep();
	}
}

void ASwarmRenderActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BenchTick(DeltaSeconds);

	const USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || !NiagaraComponent)
	{
		return;
	}

	const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();
	SubImageScratch.Reset(AnimBits.Num());
	for (const int32 Bits : AnimBits)
	{
		const float Frame = (Bits & SwarmAnim::FrameBit) ? 1.f : 0.f;
		const float Team = (Bits & SwarmAnim::TeamBit) ? 2.f : 0.f;
		SubImageScratch.Add(Frame + Team);
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
		NiagaraComponent, FName(TEXT("Positions")), Swarm->GetRenderPositions());
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName(TEXT("SubImages")), SubImageScratch);
	NiagaraComponent->SetVariableInt(FName(TEXT("Count")), Swarm->GetRenderPositions().Num());
}
