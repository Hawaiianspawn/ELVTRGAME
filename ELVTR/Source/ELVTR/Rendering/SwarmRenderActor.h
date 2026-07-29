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

	/**
	 * Renderer configurations to sweep, each one measured across EVERY entry in
	 * BenchmarkBroodCounts — so the harness answers "which renderer scales best", not just
	 * "how does today's renderer scale". Format per entry:
	 *
	 *     Name|command;command;command
	 *
	 * The commands are exec'd once at the top of the config, before the first count runs.
	 * An entry with no '|' is treated as a name with no setup. Empty array = one unnamed
	 * config using whatever CVars the exec file / command line already set, which is the
	 * pre-2026-07-27 behaviour.
	 *
	 * Defaults are the one-camera comparison the owner asked for (docs/perf/one-camera-bench.md):
	 * the isolation baseline, the Niagara renderer, and the Unit Cam projector standing alone as
	 * a full-screen "simulated camera" with no world render underneath it.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm|Benchmark")
	TArray<FString> BenchmarkConfigs = {
		// Every row states every switch explicitly rather than relying on defaults. Configs are
		// exec'd in sequence into one running session, so an unstated CVar keeps whatever the
		// PREVIOUS config set it to — the classic way a sweep like this silently measures the
		// wrong thing.
		TEXT("SIM-ONLY|Swarm.DebugRender 2;Emberkeep.UnitCamProj.Enable 0;Swarm.SimLOD.Stride 1"),
		// Range 2400 matches the main view's ground coverage (GetLiveViewWidthUU), so the
		// projector is culling the same population the viewport rows have to draw. Left at the
		// 1400 default it would frame less ground, win on cost, and the comparison would be a
		// measurement of two different shots rather than of two renderers.
		TEXT("UNITCAM-FULL|Swarm.DebugRender 2;Emberkeep.UnitCamProj.Enable 1;Emberkeep.UnitCamProj.Fullscreen 1;Emberkeep.UnitCamProj.Range 2400;Swarm.SimLOD.Stride 1"),
		TEXT("UNITCAM+NIAGARA|Swarm.DebugRender 0;Emberkeep.UnitCamProj.Enable 1;Emberkeep.UnitCamProj.Fullscreen 1;Emberkeep.UnitCamProj.Range 2400;Swarm.SimLOD.Stride 1"),
		// The LOD ladder. Stride 1 is the control and must reproduce run 1's VIEWPORT-NIAGARA
		// column; if it doesn't, the machine moved under us and the 2/4 rows mean nothing.
		TEXT("NIAGARA-LOD1|Swarm.DebugRender 0;Emberkeep.UnitCamProj.Enable 0;Swarm.SimLOD.Stride 1"),
		TEXT("NIAGARA-LOD2|Swarm.DebugRender 0;Emberkeep.UnitCamProj.Enable 0;Swarm.SimLOD.Stride 2"),
		TEXT("NIAGARA-LOD4|Swarm.DebugRender 0;Emberkeep.UnitCamProj.Enable 0;Swarm.SimLOD.Stride 4")
	};

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
	/** Exec the current config's setup commands and reset the count sweep to its first entry. */
	void BenchStartConfig();
	/** Append one measured row to Saved/SwarmBench.csv, creating it with a header if absent. */
	void BenchWriteCsvRow(const FString& Row);
	/** Display name of the config currently being swept, or "default" when none are configured. */
	FString BenchConfigName() const;

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

	TArray<float> SubImageScratch;
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
	int32 BenchConfigIndex = 0;
	bool bBenchCsvStarted = false;
	float BenchTimer = 0.f;
	int32 BenchFrames = 0;
	double BenchFrameMs = 0.0;
	double BenchGameMs = 0.0;
	double BenchRenderMs = 0.0;
	double BenchGpuMs = 0.0;
};
