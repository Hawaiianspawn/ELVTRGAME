#include "Spike1GameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"
#include "SpikeHeroPawn.h"

ASpike1GameMode::ASpike1GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ASpikeHeroPawn::StaticClass();
}

void ASpike1GameMode::BeginPlay()
{
	Super::BeginPlay();
	RestartRun();
}

void ASpike1GameMode::RestartRun()
{
	SwarmSpawn::ClearAll(GetWorld());

	if (USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->ResetRunState();

		// The formation spawns around the attractor, and the hero pawn may not
		// have ticked yet on the first run — seed it here so wave 1 isn't
		// deployed around the world origin.
		if (const APawn* Hero = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Swarm->SetAttractor(Hero->GetActorLocation());
		}
	}

	WaveIndex = 0;
	RetinueSlotCursor = 0;

	SwarmSpawn::SpawnRetinue(GetWorld(), StartingRetinue, RetinueSlotCursor);
	RetinueSlotCursor += StartingRetinue;

	EnterPhase(ERunPhase::Deploying);
	UE_LOG(LogTemp, Display, TEXT("Run: restarted (%d retinue, %d waves)"), StartingRetinue, WaveBroodCounts.Num());
}

void ASpike1GameMode::EnterPhase(ERunPhase NewPhase)
{
	Phase = NewPhase;
	PhaseTimer = 0.f;
}

void ASpike1GameMode::BeginWave()
{
	const int32 Count = WaveBroodCounts.IsValidIndex(WaveIndex) ? WaveBroodCounts[WaveIndex] : 0;
	SwarmSpawn::SpawnBrood(GetWorld(), Count);
	EnterPhase(ERunPhase::WaveActive);
	UE_LOG(LogTemp, Display, TEXT("Run: wave %d/%d — %d brood"), WaveIndex + 1, WaveBroodCounts.Num(), Count);
}

void ASpike1GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	PhaseTimer += DeltaSeconds;

	// Hero death ends the run from any phase.
	if (!IsRunOver() && !Swarm->IsHeroAlive())
	{
		EnterPhase(ERunPhase::Lost);
		UE_LOG(LogTemp, Display, TEXT("Run: LOST on wave %d — %d retinue / %d brood still standing"),
			WaveIndex + 1, Swarm->GetAliveRetinue(), Swarm->GetAliveBrood());
		return;
	}

	switch (Phase)
	{
	case ERunPhase::Deploying:
		if (PhaseTimer >= DeploySeconds)
		{
			BeginWave();
		}
		break;

	case ERunPhase::WaveActive:
		// Grace period: live counts come from the render pass, which reads zero
		// for a frame or two right after the batch spawn.
		if (PhaseTimer >= WaveClearGraceSeconds && Swarm->GetAliveBrood() == 0)
		{
			SwarmSpawn::CompactTracked(GetWorld());
			++WaveIndex;

			if (WaveIndex >= WaveBroodCounts.Num())
			{
				EnterPhase(ERunPhase::Won);
				UE_LOG(LogTemp, Display, TEXT("Run: WON with %d retinue alive"), Swarm->GetAliveRetinue());
			}
			else
			{
				// Refill to the cap. Re-seat the slot cursor at the survivor
				// count so the formation re-packs from the inside out instead
				// of marching ever-outward as the cursor accumulates.
				const int32 Survivors = Swarm->GetAliveRetinue();
				const int32 Reinforcements = FMath::Max(0, RetinueCap - Survivors);
				RetinueSlotCursor = Survivors;
				SwarmSpawn::SpawnRetinue(GetWorld(), Reinforcements, RetinueSlotCursor);
				RetinueSlotCursor += Reinforcements;

				EnterPhase(ERunPhase::Breather);
				UE_LOG(LogTemp, Display, TEXT("Run: wave cleared — %d survivors, +%d reinforcements"),
					Survivors, Reinforcements);
			}
		}
		break;

	case ERunPhase::Breather:
		if (PhaseTimer >= BreatherSeconds)
		{
			BeginWave();
		}
		break;

	case ERunPhase::Won:
	case ERunPhase::Lost:
	default:
		break;
	}
}
