#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwarmRenderActor.generated.h"

class UNiagaraComponent;
class UMaterialParameterCollection;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Bridges Mass -> Niagara. Place one in the map with NS_Swarm assigned.
 *
 * Each tick, splits the swarm's packed entities by SwarmAnim::TeamBit and pushes each
 * side into its OWN set of the Niagara system's user parameter arrays (task-085 — two
 * emitters inside NS_Swarm, one per side, each with its own Sub UV grid):
 *   Team:  User.TeamPositions, User.TeamSubImages, User.TeamColors, User.TeamSizes, User.TeamCount
 *   Enemy: User.Positions, User.SubImages, User.Colors, User.Sizes, User.Count (unchanged names —
 *          this is the emitter that already existed before the split)
 * SubImage index is decoded here (SwarmSheet::Team::CellFor / ::Enemy::CellFor) so the
 * Niagara graph stays a dumb array lookup on both emitters.
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
	 * task-048: Swarm.DebugShotAfter's capture path. NOT FScreenshotRequest — that call still
	 * exists in engine code but is USELESS for an agent-driven PIE session, because it is
	 * fulfilled inside UGameViewportClient::Draw(), which only runs when Slate actually paints
	 * the game viewport. An unfocused/occluded PIE window can sit for a full auto-fight with
	 * the game thread ticking normally (spawns, combat, everything) while Draw() is simply never
	 * called, so the request sits queued forever and no file ever lands — confirmed empirically
	 * (task-047 ran a full fight to completion with zero screenshot output).
	 *
	 * A SceneCaptureComponent2D sidesteps this entirely: CaptureScene() issues its own render
	 * command straight to a render target, independent of Slate window paint/focus/occlusion.
	 * It also fixes two problems FScreenshotRequest never solved: resolution is the render
	 * target's, not the on-screen window's (so it's sharp regardless of how small/tiled the PIE
	 * viewport is), and — unlike the MCP CaptureViewport tool, which renders the persistent
	 * EDITOR world and so shows editor-only gizmo icons and zero swarm (verified 2026-07-28,
	 * task-048) — this component lives IN the PIE actor, so it captures the actual game world
	 * the swarm renders into.
	 *
	 * Every shot copies the live PlayerCameraManager's cached view (location/rotation/FOV or
	 * OrthoWidth) onto this component rather than re-deriving ASpikeHeroPawn's Ortho/Pitch/Yaw/
	 * Dist/HudBias math a second time — one source of truth for "what the player is actually
	 * seeing", same idiom as GetLiveViewWidthUU above.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Swarm|Debug")
	TObjectPtr<USceneCaptureComponent2D> DebugCaptureComponent;

	/**
	 * Self-driving Spike 1 benchmark. When set (or the game runs with
	 * -SwarmBench), steps through BenchmarkBroodCounts: clear, respawn
	 * retinue + brood, wait BenchmarkSettleSeconds for convergence, then
	 * average frame/game/render/GPU ms over BenchmarkSampleSeconds and log
	 * one "SwarmBench:" line per count, ending with "SwarmBench: DONE".
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	bool bRunBenchmark = false;

	// 20000 is deliberately past any count the design asks for: the gate is 1,000 (GDD §10) and
	// the interesting question is not "does it pass" but "where does each renderer actually
	// break", which a sweep that stops at the target can never show.
	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	TArray<int32> BenchmarkBroodCounts = { 1000, 5000, 10000, 20000, 30000, 40000 };

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
	/** Append one measured row to Saved/SwarmBench.csv, creating it with a header if absent. */
	void BenchWriteCsvRow(const FString& Row);

	void TickSpacingLog(float DeltaSeconds);
	void TickFlame(float DeltaSeconds);

	/** Fires once from TickSpacingLog when Swarm.DebugShotAfter's timer elapses. See
	 * DebugCaptureComponent's doc comment for why this replaced FScreenshotRequest. */
	void TakeDebugShot();

	// Lazily (re)created in TakeDebugShot at Swarm.DebugShotWidth/Height's current size — not a
	// UPROPERTY on the actor's own declaration list above because it is pure runtime scratch,
	// same reasoning as SmoothedFlamePos below.
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> DebugCaptureRT;

	// task-085: one SubImage array per side, kept as members (not file-static like the other
	// per-tick scratch below) because TakeDebugShot reads them back after Tick to log which
	// atlas rows each emitter was actually handed. Splitting this pair IS a class-layout
	// change, so unlike the file-static arrays in SwarmRenderActor.cpp this needs the full
	// editor-closed rebuild (Stop-Editor; Build-Editor; Start-Editor) rather than Live Coding.
	TArray<float> TeamSubImageScratch;
	TArray<float> EnemySubImageScratch;
	float FlameSeed = 0.f;

	// Damped-spring state for the flame (see TickFlame). Not a UPROPERTY — plain
	// per-instance runtime state, reset each PIE because the actor is recreated.
	FVector SmoothedFlamePos = FVector::ZeroVector;
	FVector FlameVel = FVector::ZeroVector;
	bool bFlameInitialized = false;

	float SpacingLogTimer = 0.f;
	float PlayTime = 0.f;
	bool bDebugShotTaken = false;
	int32 LastPlainViewState = -1;
	/** Last CustomStencil value pushed to the swarm component; -1 forces the first write. */
	int32 LastUnitStencil = -1;

	EBenchPhase BenchPhase = EBenchPhase::Off;
	int32 BenchStep = 0;
	bool bBenchCsvStarted = false;
	float BenchTimer = 0.f;
	int32 BenchFrames = 0;
	double BenchFrameMs = 0.0;
	double BenchGameMs = 0.0;
	double BenchRenderMs = 0.0;
	double BenchGpuMs = 0.0;
};
