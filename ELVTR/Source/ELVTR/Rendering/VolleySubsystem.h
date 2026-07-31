#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VolleySubsystem.generated.h"

class UNiagaraSystem;

/**
 * Volley cue subsystem (task-129). Archers engage from 750uu
 * (Swarm.ArchersEngageRange) and, until this existed, nothing whatever happened on
 * screen when one shot -- a brood simply took damage and flashed 750uu away.
 * docs/design/squad-group-system.md §2.2 specced the missing half in one line: "the
 * visual is a volley (a cheap arcing Niagara trail or ribbon triggered on SwingBit,
 * purely cosmetic, no gameplay state)". This is that line and nothing more.
 *
 * Deliberately a near-copy of UBloodSubsystem, down to the tick shape: each frame it
 * scans the swarm's already-published render arrays (USwarmSubsystem::GetRenderPositions
 * / GetRenderAnimBits) and fires a short arcing streak of arrow sprites (NS_Volley,
 * ELVTR/Content/Gore/) from any archer wearing SwarmAnim::SwingBit. Reads the sim's
 * output only -- no fragment, no processor, no new render-buffer bit, no gameplay state
 * added on its account. An archer is identified exactly the way SwarmRenderActor's pack
 * loop already does it: SwarmSquad::UnitType(SwarmRenderPack::Squad(Bits)) ==
 * EUnitType::Archers, so sprite, stats and cue cannot disagree about who is an archer.
 *
 * WHY THE CUE FIRES AT A SHARED CENTROID, NOT AT A TARGET: there is no per-shot victim
 * in the render buffer, and putting one there is the per-entity uniqueness cost Design
 * Law 5 rules out at horde scale (squad-group-system.md §2.2 says so in as many words).
 * So the same pass that finds the archers also averages the position of every non-team
 * entity, and every cue that frame flies at that ONE brood centroid, clamped to the
 * archer's own engage range. At horde density this reads correctly because the brood
 * genuinely do arrive as a mass -- see docs/perf/volley-vfx.md for the honest account of
 * where it stops reading.
 *
 * WHY SwingBit NEEDS A RATE, not just a cap: SwingBit is a POSE window, not an event.
 * USwarmSwingProcessor holds it for StrikeAt*0.5 .. StrikeAt + Interval*0.18 -- about
 * 0.32s of every 0.9s swing at shipped defaults, so roughly nineteen consecutive frames
 * at 60fps carry the same single shot. The render buffer has no stable per-entity id to
 * edge-detect that one shot without adding one, and this subsystem is explicitly scoped
 * not to (that buffer belongs to SwarmSubsystem.h, out of bounds here). Rather than
 * emit nineteen cues per arrow, each swinging archer rolls Volley.CueRate * DeltaSeconds
 * each frame -- a per-second rate, so it is frame-rate independent and defaults to about
 * one cue per shot in expectation. It is expectation, not a guarantee: a given shot can
 * roll two cues or none. Volley.MaxPerFrame is the hard ceiling on top of that, the same
 * idiom as Blood.MaxBurstsPerFrame, and the doc records what both look like at wave-3
 * density rather than pretending it is exactly-once-per-shot.
 *
 * A subsystem, not an actor, for the same reason UBloodSubsystem is one: an actor would
 * need placing in L_Spike1.umap, which belongs to task-059 (parked, not cancelled). The
 * NiagaraComponent is created at runtime instead, one per cue, via
 * UNiagaraFunctionLibrary::SpawnSystemAtLocation (bAutoDestroy=true) -- fire-and-forget,
 * nothing persistent for this subsystem to own or clean up.
 */
UCLASS()
class UVolleySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- UTickableWorldSubsystem -------------------------------------------
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Loads NS_Volley on first use; cached for the subsystem's lifetime. Logs once and
	 *  leaves the feature a clean no-op if the asset can't be found, rather than crashing. */
	UNiagaraSystem* GetVolleySystem();

	void SpawnCue(UWorld* World, UNiagaraSystem* System, const FVector& Location,
		const FRotator& Rotation, int32 ArrowCount, float Lifetime, float SpeedScale);

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> VolleySystem;

	bool bTriedLoadVolleySystem = false;

	/**
	 * Archers that rolled a cue this frame, filled during the one scan and drained
	 * immediately after it. A member rather than a local purely to keep its allocation
	 * across frames -- it is bounded by Volley.MaxPerFrame, so this is a few hundred
	 * bytes that never grow. It exists at all because the brood centroid is not known
	 * until the scan has finished, and walking 40k render entries a second time to
	 * spawn would cost far more than remembering two dozen positions.
	 */
	TArray<FVector> FiringScratch;
};
