#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwarmRenderActor.generated.h"

class UNiagaraComponent;
class UMaterialParameterCollection;

/**
 * Bridges Mass -> Niagara. Place one in the map with NS_Swarm assigned.
 * Each tick, pushes the swarm's packed positions + sprite frames into the
 * Niagara system's user parameter arrays:
 *   User.Positions (Position array), User.SubImages (Float array), User.Count (int)
 * SubImage index = walk frame (bit 0) + 2 * team (bit 3) — decoded here so the
 * Niagara graph stays a dumb array lookup.
 */
UCLASS()
class ASwarmRenderActor : public AActor
{
	GENERATED_BODY()

public:
	ASwarmRenderActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Swarm")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	/**
	 * MPC_Flame — the bearer's spotlight (docs/RENDERING-LIGHTING.md §4b).
	 *
	 * The light is folded into the existing demichrome post-process rather than
	 * added as a second pass, so its attenuation shifts luminance *before* the
	 * Bayer threshold quantises it. That ordering is what makes falloff scatter
	 * into the dark instead of banding into concentric rings (§4b.4).
	 *
	 * This actor drives it because it already ticks in TG_PostUpdateWork after
	 * Mass has run, and already reads the subsystem — so the light sees the same
	 * hero position the swarm steered against this frame, not last frame's.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm|Flame")
	TObjectPtr<UMaterialParameterCollection> FlameCollection;

	/**
	 * Debug view: one solid debug box per entity, straight from the render
	 * buffers, bypassing Niagara entirely.
	 *
	 * Debug draw rather than instanced meshes on purpose. Instanced meshes need
	 * a material flagged `bUsedWithInstancedStaticMeshes`, and every stock engine
	 * material lacks it — UE silently substitutes the *default lit* material, so
	 * the units render black in a dark level with only a log warning to say why.
	 * Debug draw is unlit, takes an explicit colour, and needs no asset at all.
	 *
	 * Specifically DrawDebugSolidBox, not DrawDebugPoint: points silently draw
	 * nothing in a -game session (verified), boxes render correctly in both.
	 *
	 * Toggle with `Swarm.DebugRender 0|1`; while on, the Niagara component is
	 * hidden so the two renderers can't be mistaken for one another.
	 */
	/** Half-extent in world units. Formation spacing is ~86uu, so keep under ~40. */
	UPROPERTY(EditAnywhere, Category = "Swarm|Debug")
	float RetinueDebugPointSize = 22.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Debug")
	float BroodDebugPointSize = 14.f;

	/**
	 * Self-driving Spike 1 benchmark. When set (or the game runs with
	 * -SwarmBench), steps through BenchmarkBroodCounts: clear, respawn
	 * retinue + brood, wait BenchmarkSettleSeconds for convergence, then
	 * average frame/game/render/GPU ms over BenchmarkSampleSeconds and log
	 * one "SwarmBench:" line per count, ending with "SwarmBench: DONE".
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	bool bRunBenchmark = false;

	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	TArray<int32> BenchmarkBroodCounts = { 500, 1000, 2000, 5000, 10000 };

	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	int32 BenchmarkRetinueCount = 100;

	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	float BenchmarkSettleSeconds = 8.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	float BenchmarkSampleSeconds = 5.f;

private:
	enum class EBenchPhase : uint8 { Off, Settle, Sample };

	void BenchExec(const FString& Cmd);
	void BenchStartStep();
	void BenchTick(float DeltaSeconds);

	void TickDebugRender();
	void TickSpacingLog(float DeltaSeconds);
	void TickFlame(float DeltaSeconds);

	TArray<float> SubImageScratch;
	float FlameSeed = 0.f;

	// Spring-arm lag state for the flame (see TickFlame). Not a UPROPERTY — plain
	// per-instance runtime state, reset each PIE because the actor is recreated.
	FVector SmoothedFlamePos = FVector::ZeroVector;
	bool bFlameInitialized = false;

	float SpacingLogTimer = 0.f;
	float PlayTime = 0.f;
	bool bDebugShotTaken = false;
	int32 LastPlainViewState = -1;

	EBenchPhase BenchPhase = EBenchPhase::Off;
	int32 BenchStep = 0;
	float BenchTimer = 0.f;
	int32 BenchFrames = 0;
	double BenchFrameMs = 0.0;
	double BenchGameMs = 0.0;
	double BenchRenderMs = 0.0;
	double BenchGpuMs = 0.0;
};
