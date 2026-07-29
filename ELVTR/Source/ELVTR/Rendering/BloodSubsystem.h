#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BloodSubsystem.generated.h"

class UNiagaraSystem;

/**
 * Blood particle subsystem (task-060). Owner's own words: "add some gore particles or
 * sprites... something very simple like red pixels for blood sim. We dont have to have
 * it live for long." Told to spray on both deaths AND hits, and warned hits land
 * continuously across the line before choosing that anyway -- see
 * docs/perf/blood-particles.md for whether that survived contact with horde density.
 *
 * Each frame, scans the swarm's already-published render arrays
 * (USwarmSubsystem::GetRenderPositions / GetRenderAnimBits) for units carrying
 * SwarmAnim::HitFlashBit and sprays a short-lived burst of red pixel particles (NS_Blood,
 * ELVTR/Content/Gore/) at their position. Reads the sim's output only -- no fragment, no
 * processor, no new render-buffer bit added on its account.
 *
 * WHY HitFlashBit, not "struck this exact instant": the bit stays set for the whole
 * Swarm.HitFlashTime window (SwarmProcessors.cpp), not just the one frame the blow lands
 * -- there is no stable per-entity id in the render buffer to edge-detect a single hit
 * without adding one, and this subsystem is explicitly scoped not to (that buffer belongs
 * to SwarmSubsystem.h, out of bounds here). So one hit can spray across several
 * consecutive frames while its flash is up. Blood.MaxBurstsPerFrame bounds the cost
 * regardless; the doc records the honest read of what that looks like at wave-3 density
 * rather than pretending it is exactly-once-per-hit.
 *
 * Deaths bleed for free: the killing blow also sets HitFlashBit, so a death is not a
 * distinct, harder burst -- just an ordinary hit. A real death-position burst needs a
 * position captured at the moment of death, which the sim does not publish (see
 * USwarmDeathProcessor, SwarmCombatProcessors.cpp) and which is task-054's territory
 * (persistent corpses), not this task's.
 *
 * A subsystem, not an actor: an actor would need placing in L_Spike1.umap, which belongs
 * to task-059 (parked, not cancelled) along with the rest of Content/Spike1/**. The
 * NiagaraComponent is created at runtime instead, one per burst, via
 * UNiagaraFunctionLibrary::SpawnSystemAtLocation (bAutoDestroy=true) -- fire-and-forget,
 * nothing persistent for this subsystem to own or clean up.
 */
UCLASS()
class UBloodSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- UTickableWorldSubsystem -------------------------------------------
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Loads NS_Blood on first use; cached for the subsystem's lifetime. Logs once and
	 *  leaves the feature a clean no-op if the asset can't be found, rather than crashing. */
	UNiagaraSystem* GetBloodSystem();

	void SpawnBurst(UWorld* World, UNiagaraSystem* System, const FVector& Location, const FRotator& Rotation, int32 ParticleCount);

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> BloodSystem;

	bool bTriedLoadBloodSystem = false;
};
