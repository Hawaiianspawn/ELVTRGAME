#include "SwarmRenderActor.h"

#include "Mass/SwarmSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

ASwarmRenderActor::ASwarmRenderActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after Mass PrePhysics processing

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	RootComponent = NiagaraComponent;
	NiagaraComponent->SetAutoActivate(true);
}

void ASwarmRenderActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || !NiagaraComponent)
	{
		return;
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
		NiagaraComponent, FName(TEXT("Positions")), Swarm->GetRenderPositions());
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(
		NiagaraComponent, FName(TEXT("AnimBits")), Swarm->GetRenderAnimBits());
	NiagaraComponent->SetVariableInt(FName(TEXT("Count")), Swarm->GetRenderPositions().Num());
}
