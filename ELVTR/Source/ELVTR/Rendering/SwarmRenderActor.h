#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwarmRenderActor.generated.h"

class UNiagaraComponent;

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

	TArray<float> SubImageScratch;

	EBenchPhase BenchPhase = EBenchPhase::Off;
	int32 BenchStep = 0;
	float BenchTimer = 0.f;
	int32 BenchFrames = 0;
	double BenchFrameMs = 0.0;
	double BenchGameMs = 0.0;
	double BenchRenderMs = 0.0;
	double BenchGpuMs = 0.0;
};
