#include "SpikeHeroPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmSubsystem.h"
#include "Spike1GameMode.h"
#include "UObject/ConstructorHelpers.h"

ASpikeHeroPawn::ASpikeHeroPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	RootComponent = BodyMesh;
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.2f));
	}

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetRelativeLocation(FVector(0.f, 0.f, CameraHeight));
	Camera->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(2.f * CameraHeight); // matches prior perspective framing at 90-deg FOV
}

void ASpikeHeroPawn::BeginPlay()
{
	Super::BeginPlay();

	// Charge aims at the cursor, so the cursor has to be visible.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
	}

	if (USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->SetAttractor(GetActorLocation());
	}

	// Framing check: compare against the retinue bbox in the spacing report to
	// tell "army is too small" apart from "camera is too wide".
	if (Camera)
	{
		UE_LOG(LogTemp, Display, TEXT("SwarmDebug: camera ortho=%.0fuu height=%.0fuu projection=%s"),
			Camera->OrthoWidth, CameraHeight,
			Camera->ProjectionMode == ECameraProjectionMode::Orthographic ? TEXT("ortho") : TEXT("perspective"));
	}
}

bool ASpikeHeroPawn::ConsumeKeyPress(const APlayerController& PC, const FKey& Key, bool& bWasDown) const
{
	const bool bIsDown = PC.IsInputKeyDown(Key);
	const bool bPressed = bIsDown && !bWasDown;
	bWasDown = bIsDown;
	return bPressed;
}

bool ASpikeHeroPawn::GetCursorGroundLocation(FVector& OutLocation) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	FVector WorldOrigin, WorldDirection;
	if (!PC || !PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection)
		|| FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	// Everything in the prototype lives on the Z = hero plane.
	const float PlaneZ = GetActorLocation().Z;
	const float T = (PlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (T < 0.f)
	{
		return false;
	}

	OutLocation = WorldOrigin + WorldDirection * T;
	return true;
}

void ASpikeHeroPawn::TickStanceInput(const APlayerController& PC)
{
	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	if (ConsumeKeyPress(PC, EKeys::One, bWasDownFollow))
	{
		Swarm->SetStance(ESwarmStance::Follow, GetActorLocation());
	}
	else if (ConsumeKeyPress(PC, EKeys::Two, bWasDownCharge))
	{
		// Aim at the cursor; fall back to straight ahead if the deproject fails.
		FVector Target;
		if (!GetCursorGroundLocation(Target))
		{
			Target = GetActorLocation() + FVector(1000.f, 0.f, 0.f);
		}
		Swarm->SetStance(ESwarmStance::Charge, Target);
	}
	else if (ConsumeKeyPress(PC, EKeys::Three, bWasDownHold))
	{
		// Hold anchors where the hero stands when the order is given.
		Swarm->SetStance(ESwarmStance::Hold, GetActorLocation());
	}
	else if (ConsumeKeyPress(PC, EKeys::Four, bWasDownRally))
	{
		Swarm->SetStance(ESwarmStance::Rally, GetActorLocation());
	}

	if (ConsumeKeyPress(PC, EKeys::R, bWasDownRestart))
	{
		if (ASpike1GameMode* GameMode = GetWorld()->GetAuthGameMode<ASpike1GameMode>())
		{
			GameMode->RestartRun();
		}
	}
}

void ASpikeHeroPawn::TickHeroCombat(float DeltaSeconds)
{
	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm || !Swarm->IsHeroAlive())
	{
		return;
	}

	const float Damage = Swarm->ConsumePendingHeroDamage();
	if (Damage > 0.f)
	{
		Swarm->SetHeroHP(Swarm->GetHeroHP() - Damage);
		if (Swarm->GetHeroHP() <= 0.f)
		{
			Swarm->SetHeroAlive(false);
		}
	}
}

void ASpikeHeroPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	const bool bAlive = Swarm && Swarm->IsHeroAlive();

	if (bAlive)
	{
		FVector Input = FVector::ZeroVector;
		if (PC->IsInputKeyDown(EKeys::W)) { Input.X += 1.f; }
		if (PC->IsInputKeyDown(EKeys::S)) { Input.X -= 1.f; }
		if (PC->IsInputKeyDown(EKeys::D)) { Input.Y += 1.f; }
		if (PC->IsInputKeyDown(EKeys::A)) { Input.Y -= 1.f; }

		if (!Input.IsNearlyZero())
		{
			AddActorWorldOffset(Input.GetSafeNormal() * MoveSpeed * DeltaSeconds);
		}
	}

	TickStanceInput(*PC);
	TickHeroCombat(DeltaSeconds);

	if (Swarm)
	{
		Swarm->SetAttractor(GetActorLocation());
	}

	DrawHUD();
}

void ASpikeHeroPawn::DrawHUD() const
{
	const USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!GEngine || !Swarm)
	{
		return;
	}

	const ASpike1GameMode* GameMode = GetWorld()->GetAuthGameMode<ASpike1GameMode>();

	// --- run banner ------------------------------------------------------
	if (GameMode)
	{
		FString Banner;
		FColor BannerColor = FColor::White;

		switch (GameMode->GetPhase())
		{
		case ERunPhase::Deploying:
			Banner = TEXT("DEPLOYING — form up");
			BannerColor = FColor::Cyan;
			break;
		case ERunPhase::WaveActive:
			Banner = FString::Printf(TEXT("WAVE %d / %d"), GameMode->GetWaveIndex() + 1, GameMode->GetWaveCount());
			BannerColor = FColor::Orange;
			break;
		case ERunPhase::Breather:
			Banner = FString::Printf(TEXT("WAVE CLEARED — reinforcements in %.0fs"),
				FMath::Max(0.f, GameMode->BreatherSeconds - GameMode->GetPhaseTimer()));
			BannerColor = FColor::Green;
			break;
		case ERunPhase::Won:
			Banner = TEXT("*** THE LINE HELD — VICTORY ***   [R] restart");
			BannerColor = FColor::Green;
			break;
		case ERunPhase::Lost:
			Banner = TEXT("*** THE HERO HAS FALLEN ***   [R] restart");
			BannerColor = FColor::Red;
			break;
		}
		GEngine->AddOnScreenDebugMessage(1, 0.f, BannerColor, Banner);
	}

	// --- hero ------------------------------------------------------------
	const float HPFraction = Swarm->GetHeroHP() / FMath::Max(1.f, Swarm->GetHeroMaxHP());
	GEngine->AddOnScreenDebugMessage(2, 0.f,
		HPFraction > 0.5f ? FColor::Green : (HPFraction > 0.25f ? FColor::Yellow : FColor::Red),
		FString::Printf(TEXT("HERO  %3.0f / %3.0f"), Swarm->GetHeroHP(), Swarm->GetHeroMaxHP()));

	// --- army ------------------------------------------------------------
	GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Cyan,
		FString::Printf(TEXT("RETINUE %4d      BROOD %4d"), Swarm->GetAliveRetinue(), Swarm->GetAliveBrood()));

	// --- stance + leash ---------------------------------------------------
	const int32 Broken = Swarm->GetLeashBrokenCount();
	GEngine->AddOnScreenDebugMessage(4, 0.f, Broken > 0 ? FColor::Yellow : FColor::White,
		FString::Printf(TEXT("STANCE  %s%s"),
			LexToString(Swarm->GetStance()),
			Broken > 0 ? *FString::Printf(TEXT("   (%d leashed back to Follow)"), Broken) : TEXT("")));

	GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Silver,
		TEXT("[WASD] move   [1] Follow  [2] Charge (at cursor)  [3] Hold  [4] Rally   [R] restart"));
}
