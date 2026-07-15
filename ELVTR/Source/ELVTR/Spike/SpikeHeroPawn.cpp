#include "SpikeHeroPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Mass/SwarmSubsystem.h"
#include "Materials/MaterialInterface.h"
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
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DemichromeMat(
		TEXT("/Game/PostProcess/M_PP_Demichrome.M_PP_Demichrome"));
	if (DemichromeMat.Succeeded())
	{
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(
			FWeightedBlendable(1.f, DemichromeMat.Object));
		Camera->PostProcessBlendWeight = 1.f;
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

	FVector Input = FVector::ZeroVector;
	if (PC->IsInputKeyDown(EKeys::W)) { Input.X += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S)) { Input.X -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D)) { Input.Y += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A)) { Input.Y -= 1.f; }

	if (!Input.IsNearlyZero())
	{
		AddActorWorldOffset(Input.GetSafeNormal() * MoveSpeed * DeltaSeconds);
	}

	if (USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->SetAttractor(GetActorLocation());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,
				FString::Printf(TEXT("Swarm entities: %d"), Swarm->GetRenderPositions().Num()));
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Yellow,
				FString::Printf(TEXT("Hero contacts (cumulative): %lld"), Swarm->GetHeroContacts()));
		}
	}
}
