#include "SwarmRenderActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmDebug.h"
#include "Materials/MaterialParameterCollection.h"
#include "Mass/SwarmFragments.h"
#include "Mass/SwarmStats.h"
#include "Mass/SwarmSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "RenderTimer.h"
#include "RHI.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TAutoConsoleVariable<int32> CVarSwarmDebugRender(
		TEXT("Swarm.DebugRender"),
		1,
		TEXT("Render the swarm as mesh instances instead of Niagara sprites.\n")
		TEXT("1 = debug meshes (default while the sprite pipeline is unverified), 0 = Niagara."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarSwarmDebugPlainView(
		TEXT("Swarm.DebugPlainView"),
		0,
		TEXT("Opt-in: disable post-process materials (drops the demichrome dither) for an\n")
		TEXT("unfiltered read of the debug view. Default 0 keeps the game's look intact."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDebugShot(
		TEXT("Swarm.DebugShotAfter"),
		0.f,
		TEXT("Take one screenshot this many seconds after BeginPlay. 0 disables.\n")
		TEXT("Lets a headless/scripted run capture what the renderer is actually drawing."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmSpacingLog(
		TEXT("Swarm.SpacingLogInterval"),
		0.f,
		TEXT("Seconds between automatic nearest-neighbour spacing reports. 0 disables."),
		ECVF_Default);

	// --- the bearer's spotlight (docs/RENDERING-LIGHTING.md §4b) -------------

	TAutoConsoleVariable<int32> CVarSwarmFlame(
		TEXT("Swarm.Flame"),
		1,
		TEXT("Drive MPC_Flame from the hero each tick (the bearer's spotlight).\n")
		TEXT("0 stops writing the collection, which leaves the light frozen wherever it\n")
		TEXT("last was — useful for checking that it really is tracking the hero."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameRadius(
		TEXT("Swarm.FlameRadius"),
		900.f,
		TEXT("How far the flame reaches, in uu — the outer edge of the pool.\n")
		TEXT("DECOUPLED from SwarmLeash::Radius (owner call 2026-07-23). The spec had these\n")
		TEXT("wired together so the edge of the light would read as the leash, but the leash\n")
		TEXT("(2000uu) is wider than the camera can see (~1200uu half-width), so that edge was\n")
		TEXT("never on screen. This is the dial to tune the pool by; expect it to move."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameCoreRadius(
		TEXT("Swarm.FlameCoreRadius"),
		330.f,
		TEXT("Radius in uu of the pure-white focusing core at the flame itself.\n")
		TEXT("The core sits outside the 4-value palette on purpose; its edge is cut with the\n")
		TEXT("same Bayer threshold as the pool so it dissolves rather than ending on a clean\n")
		TEXT("circle. Colour is MPC_Flame's FlameCoreColor (white by default)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameFalloff(
		TEXT("Swarm.FlameFalloff"),
		2.f,
		TEXT("Falloff exponent. 1 = linear; higher makes the dark heavier and the pool\n")
		TEXT("edge arrive more suddenly."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameIntensity(
		TEXT("Swarm.FlameIntensity"),
		0.55f,
		TEXT("Base brightness of the flame before flicker. This is the channel the\n")
		TEXT("upkeep/fuel economy would eventually drive.\n")
		TEXT("Well under 1.0 because the light LIFTS the palette value: at 1.0 the whole\n")
		TEXT("visible area saturates to the brightest value and the falloff ramp disappears.\n")
		TEXT("0.55 specifically because Threshold3 is 0.75 — it keeps the body of the pool\n")
		TEXT("at Demichrome Bone so the pure-white core reads as the focusing point instead\n")
		TEXT("of blending into a field of Pale. Measured, see docs/RENDERING-LIGHTING.md §4b.7."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameFlicker(
		TEXT("Swarm.FlameFlicker"),
		0.06f,
		TEXT("Flicker amplitude, 0-1. Mandatory anti-vignette mechanism (§4b.1): a lens\n")
		TEXT("vignette is perfectly steady, a carried fire is not. 0 disables."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameLagSpeed(
		TEXT("Swarm.FlameLagSpeed"),
		3.5f,
		TEXT("How fast the flame catches up to the bearer, like a spring arm's lag speed.\n")
		TEXT("The light is NOT pinned to the hero — it eases toward them via VInterpTo, the\n")
		TEXT("same lag USpringArmComponent uses. Lower = laggier / more trailing; higher =\n")
		TEXT("tighter. <=0 snaps instantly (no lag). Only the LIGHT lags — steering and the\n")
		TEXT("leash still read the true hero position, so the retinue math stays exact."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherWorldAnchor(
		TEXT("Swarm.DitherWorldAnchor"),
		1.f,
		TEXT("0 = screen-anchored dither (pattern fixed to the display), 1 = world-anchored\n")
		TEXT("(pattern fixed to the ground and scrolling with it). World anchoring is the\n")
		TEXT("other mandatory anti-vignette mechanism — it is what proves the world is\n")
		TEXT("moving through the light. This CVar is the A/B for open decision L5."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmWorldDitherScale(
		TEXT("Swarm.WorldDitherScale"),
		12.f,
		TEXT("World units per dither texel when world-anchored. Lower = finer pattern.\n")
		TEXT("The camera shows ~2400uu across ~860px, i.e. ~2.8uu per pixel, so 5uu put a\n")
		TEXT("Bayer texel under 2px and the pattern read as noise. 12uu keeps texels at\n")
		TEXT("the 2x2-pixel minimum that docs/art/aesthetic-direction.md §2.4 requires of\n")
		TEXT("anything that moves."),
		ECVF_Default);

	// Retinue pure white, brood dark red. Separation is by *value* first so it
	// survives the demichrome dither, with hue as a secondary cue — the same
	// light-retinue / dark-brood convention the sprite pipeline is aiming for.
	const FColor RetinueDebugColor(255, 255, 255);
	// Brood sit low in the value range on purpose (owner 2026-07-23): they read as
	// the dark made flesh, and they are darkest at the edge of the pool where they
	// enter — the flame lifts them only as they close on the hero. Was (190,45,35).
	const FColor BroodDebugColor(130, 32, 26);

	constexpr float DebugPointZOffset = 30.f;
}

ASwarmRenderActor::ASwarmRenderActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after Mass PrePhysics processing

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	RootComponent = NiagaraComponent;
	NiagaraComponent->SetAutoActivate(true);

	// Resolved here rather than left for a designer to assign: the light is not
	// optional set dressing, it is the only light in the game, and an actor
	// placed without it would render a black level with no obvious cause.
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> FlameMPC(
		TEXT("/Game/PostProcess/MPC_Flame.MPC_Flame"));
	if (FlameMPC.Succeeded())
	{
		FlameCollection = FlameMPC.Object;
	}
}

void ASwarmRenderActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	// Per-run seed so two sessions don't flicker in lockstep.
	FlameSeed = FMath::FRandRange(0.f, 1000.f);

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

void ASwarmRenderActor::TickSpacingLog(float DeltaSeconds)
{
	PlayTime += DeltaSeconds;

	const float ShotAfter = CVarSwarmDebugShot.GetValueOnGameThread();
	if (!bDebugShotTaken && ShotAfter > 0.f && PlayTime >= ShotAfter)
	{
		bDebugShotTaken = true;
		// Direct request rather than the HighResShot console command, which
		// silently does nothing in a packaged/-game session.
		FScreenshotRequest::RequestScreenshot(TEXT("SwarmDebugShot"), /*bInShowUI=*/true, /*bAddFilenameSuffix=*/true);
		UE_LOG(LogTemp, Display, TEXT("SwarmDebug: screenshot requested at t=%.1fs"), PlayTime);
	}

	const float Interval = CVarSwarmSpacingLog.GetValueOnGameThread();
	if (Interval <= 0.f)
	{
		SpacingLogTimer = 0.f;
		return;
	}

	SpacingLogTimer += DeltaSeconds;
	if (SpacingLogTimer >= Interval)
	{
		SpacingLogTimer = 0.f;
		SwarmDebug::LogSpacingReport(GetWorld());

		UE_LOG(LogTemp, Display, TEXT("SwarmDebug: debugRender=%d niagaraVisible=%d"),
			CVarSwarmDebugRender.GetValueOnGameThread(),
			NiagaraComponent ? (int32)NiagaraComponent->IsVisible() : -1);
	}
}

void ASwarmRenderActor::TickFlame(float DeltaSeconds)
{
	if (CVarSwarmFlame.GetValueOnGameThread() == 0 || !FlameCollection)
	{
		return;
	}

	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	// The attractor is already the hero's published position, updated every tick
	// by the pawn. Reading it here rather than finding the pawn keeps one source
	// of truth for "where the bearer is" — the swarm and the light can't disagree.
	const FVector Target = Swarm->GetAttractor();

	// Spring-arm lag: the flame eases toward the bearer instead of being pinned to
	// them (owner 2026-07-23), so it trails and sloshes as the hero moves. Snap on
	// the first tick so it doesn't streak in from the world origin; snap thereafter
	// only if lag is disabled. VInterpTo is exactly what USpringArmComponent uses.
	const float LagSpeed = CVarSwarmFlameLagSpeed.GetValueOnGameThread();
	if (!bFlameInitialized || LagSpeed <= 0.f)
	{
		SmoothedFlamePos = Target;
		bFlameInitialized = true;
	}
	else
	{
		SmoothedFlamePos = FMath::VInterpTo(SmoothedFlamePos, Target, DeltaSeconds, LagSpeed);
	}
	const FVector Flame = SmoothedFlamePos;

	// Flicker: two incommensurate sines so the period never reads as a loop.
	// Cheap, and it is doing real work — see §4b.1, a steady radial gradient on a
	// hero-locked camera reads as a lens vignette rather than a carried fire.
	const float Amplitude = FMath::Clamp(CVarSwarmFlameFlicker.GetValueOnGameThread(), 0.f, 1.f);
	const float Time = World->GetTimeSeconds();
	const float Wobble = 0.6f * FMath::Sin(Time * 11.3f + FlameSeed)
					   + 0.4f * FMath::Sin(Time * 17.7f + FlameSeed * 2.1f);
	const float Intensity = CVarSwarmFlameIntensity.GetValueOnGameThread() * (1.f + Amplitude * Wobble);

	UKismetMaterialLibrary::SetVectorParameterValue(
		World, FlameCollection, FName(TEXT("FlamePosition")),
		FLinearColor(Flame.X, Flame.Y, Flame.Z, 0.f));

	const auto SetScalar = [this, World](const TCHAR* Name, float Value)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(World, FlameCollection, FName(Name), Value);
	};

	SetScalar(TEXT("FlameRadius"), CVarSwarmFlameRadius.GetValueOnGameThread());
	SetScalar(TEXT("FlameCoreRadius"), CVarSwarmFlameCoreRadius.GetValueOnGameThread());
	SetScalar(TEXT("FlameFalloff"), CVarSwarmFlameFalloff.GetValueOnGameThread());
	SetScalar(TEXT("FlameIntensity"), FMath::Max(Intensity, 0.f));
	SetScalar(TEXT("DitherWorldAnchor"), CVarSwarmDitherWorldAnchor.GetValueOnGameThread());
	SetScalar(TEXT("WorldDitherScale"), CVarSwarmWorldDitherScale.GetValueOnGameThread());
}

void ASwarmRenderActor::TickDebugRender()
{
	SWARM_SCOPE(STAT_SwarmDebugDraw, SwarmDebugDraw);

	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	const TArray<FVector>& Positions = Swarm->GetRenderPositions();
	const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();

	const int32 Num = FMath::Min(Positions.Num(), AnimBits.Num());
	for (int32 i = 0; i < Num; ++i)
	{
		const bool bRetinue = (AnimBits[i] & SwarmAnim::TeamBit) != 0;
		const float HalfSize = bRetinue ? RetinueDebugPointSize : BroodDebugPointSize;

		DrawDebugSolidBox(
			World,
			Positions[i] + FVector(0.f, 0.f, DebugPointZOffset),
			FVector(HalfSize, HalfSize, HalfSize * 0.5f),
			bRetinue ? RetinueDebugColor : BroodDebugColor,
			/*bPersistent=*/false,
			/*LifeTime=*/-1.f,
			/*DepthPriority=*/0);
	}
}

void ASwarmRenderActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SWARM_SCOPE(STAT_SwarmRenderBridge, SwarmRenderBridge);

	BenchTick(DeltaSeconds);
	TickSpacingLog(DeltaSeconds);
	TickFlame(DeltaSeconds);

	const USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || !NiagaraComponent)
	{
		return;
	}

	// One renderer at a time, so a broken sprite setup can't be mistaken for a
	// broken sim (or vice versa).
	const bool bDebugRender = CVarSwarmDebugRender.GetValueOnGameThread() != 0;
	NiagaraComponent->SetVisibility(!bDebugRender);

	// The demichrome post-process stays on by default — the debug points are
	// bright enough to read straight through the dither, and the whole point of
	// judging the game is judging it as it actually looks. Only an explicit
	// opt-in drops it. Driven on change so toggling either way self-heals.
	const int32 PlainView = CVarSwarmDebugPlainView.GetValueOnGameThread();
	if (LastPlainViewState != PlainView)
	{
		LastPlainViewState = PlainView;
		BenchExec(PlainView != 0 ? TEXT("r.PostProcessing.DisableMaterials 1")
								 : TEXT("r.PostProcessing.DisableMaterials 0"));
	}

	if (bDebugRender)
	{
		TickDebugRender();
		return;
	}

	SWARM_SCOPE(STAT_SwarmNiagaraPush, SwarmNiagaraPush);

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
