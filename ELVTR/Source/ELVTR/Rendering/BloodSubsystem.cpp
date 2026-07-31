#include "BloodSubsystem.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Mass/SwarmFragments.h"
#include "Mass/SwarmSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Stats/Stats.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	TAutoConsoleVariable<int32> CVarBloodEnable(
		TEXT("Blood.Enable"),
		1,
		TEXT("Master on/off for the blood particle subsystem. 0 skips the render-array scan\n")
		TEXT("entirely, so the feature costs nothing when off."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarBloodParticlesPerHit(
		TEXT("Blood.ParticlesPerHit"),
		6,
		TEXT("Red pixel particles spawned in one burst for each struck unit found this frame.\n")
		TEXT("Per-burst count, not a global total -- see Blood.MaxBurstsPerFrame for the dial\n")
		TEXT("that actually bounds total cost."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarBloodMaxBurstsPerFrame(
		TEXT("Blood.MaxBurstsPerFrame"),
		24,
		TEXT("GLOBAL cap: the most struck units this subsystem will spawn a burst for in one\n")
		TEXT("frame, regardless of how many are flagged. Without this, cost scales with the\n")
		TEXT("kill/hit rate and has no ceiling -- at horde density many units can be mid-flash\n")
		TEXT("simultaneously (SwarmAnim::HitFlashBit stays set for the whole Swarm.HitFlashTime\n")
		TEXT("window, not one frame). Units beyond the cap simply don't bleed that frame; which\n")
		TEXT("ones are skipped follows the render buffer's publish order, same idiom as\n")
		TEXT("Swarm.MaxAttackersPerUnit."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarBloodLifetime(
		TEXT("Blood.Lifetime"),
		0.3f,
		TEXT("Seconds a blood particle lives. Sub-second by design -- the owner does not need\n")
		TEXT("it to live long. Drives NS_Blood's User.Lifetime; NS_Blood's own spawn-time\n")
		TEXT("modules are already configured off interpolated spawning specifically so a short\n")
		TEXT("lifetime like this actually renders (interpolated spawn silently kills particles\n")
		TEXT("that die within their first partial frame -- see NS_Blood's emitter properties)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarBloodSize(
		TEXT("Blood.Size"),
		6.f,
		TEXT("Sprite half-size in uu -- kept pixel-scale to match the sprites, not a fx-scale\n")
		TEXT("splat. Drives NS_Blood's User.Size."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarBloodSpeedScale(
		TEXT("Blood.SpeedScale"),
		1.f,
		TEXT("Multiplier over NS_Blood's authored spray-velocity range (an outward, slightly\n")
		TEXT("upward random box, +-70uu/s horizontal, 20-150uu/s vertical). 1 = as authored,\n")
		TEXT("0 = particles drop straight at the hit point. Drives NS_Blood's User.SpeedScale."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarBloodHeightOffset(
		TEXT("Blood.HeightOffset"),
		15.f,
		TEXT("Z lift, uu, above the raw published (ground-plane) position -- task-110: that raw\n")
		TEXT("position is where NS_Swarm's sprite pivot renders the unit's feet, so this alone\n")
		TEXT("is the whole story now. Lifts the spawn up the body toward roughly where a blow\n")
		TEXT("lands. 0 = spawns at the unit's feet."),
		ECVF_Default);
}

TStatId UBloodSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBloodSubsystem, STATGROUP_Tickables);
}

bool UBloodSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Game and PIE only -- same reasoning as USwarmTelemetrySubsystem: no reason to scan
	// and spawn bursts in an editor preview world.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UNiagaraSystem* UBloodSubsystem::GetBloodSystem()
{
	if (!bTriedLoadBloodSystem)
	{
		bTriedLoadBloodSystem = true;
		BloodSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Gore/NS_Blood.NS_Blood"));
		if (!BloodSystem)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("BloodSubsystem: /Game/Gore/NS_Blood not found -- blood particles disabled."));
		}
	}
	return BloodSystem;
}

void UBloodSubsystem::SpawnBurst(UWorld* World, UNiagaraSystem* System, const FVector& Location, const FRotator& Rotation, int32 ParticleCount)
{
	// bAutoActivate=false: the burst needs its User.* parameters written before the sim
	// first ticks, so activation is deferred to an explicit Activate() below, after every
	// parameter is set. bAutoDestroy=true: this is genuinely fire-and-forget -- the
	// subsystem owns nothing about the spawned component past this call, matching the
	// "no persistent component" reasoning in the class doc comment.
	//
	// Rotation orients the spawn: NS_Blood's AddVelocity module reads its random spray box
	// in LOCAL space (bound to Engine.Owner.SystemLocalToWorld), so whatever rotation we
	// hand the component here is exactly the direction the burst sprays in. The caller
	// derives it from the struck unit's own published facing, so blood sprays with the
	// fight line instead of along fixed world axes for every hit.
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, System, Location, Rotation, FVector(1.f),
		/*bAutoDestroy=*/true, /*bAutoActivate=*/false);
	if (!Comp)
	{
		return;
	}

	Comp->SetVariableInt(FName("User.Count"), ParticleCount);
	Comp->SetVariableFloat(FName("User.Lifetime"), FMath::Max(CVarBloodLifetime.GetValueOnGameThread(), 0.01f));
	Comp->SetVariableFloat(FName("User.Size"), FMath::Max(CVarBloodSize.GetValueOnGameThread(), 0.1f));
	Comp->SetVariableFloat(FName("User.SpeedScale"), FMath::Max(CVarBloodSpeedScale.GetValueOnGameThread(), 0.f));

	// Match the units' own flame-lift exemption (Swarm.UnitStencil, SwarmRenderActor.cpp)
	// rather than adding a second, independent dial that could drift from it. Blood is a
	// bright, saturated red -- left un-exempted it would take the flame's additive lift
	// and white-core clip near the hero exactly like ground does, so blood next to a unit
	// would be graded differently from the unit it came out of. Read live (not cached at
	// startup) so toggling the swarm's stencil CVar for an A/B moves blood with it.
	static IConsoleVariable* UnitStencilCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("Swarm.UnitStencil"));
	const int32 StencilValue = UnitStencilCVar ? UnitStencilCVar->GetInt() : 0;
	Comp->SetRenderCustomDepth(StencilValue != 0);
	Comp->SetCustomDepthStencilValue(StencilValue);

	Comp->Activate(true);
}

void UBloodSubsystem::Tick(float DeltaSeconds)
{
	if (CVarBloodEnable.GetValueOnGameThread() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	UNiagaraSystem* System = GetBloodSystem();
	if (!System)
	{
		return;
	}

	const TArray<FVector>& Positions = Swarm->GetRenderPositions();
	const TArray<int32>& RenderAnimBits = Swarm->GetRenderAnimBits();
	const int32 Count = FMath::Min(Positions.Num(), RenderAnimBits.Num());

	const int32 ParticlesPerHit = FMath::Max(CVarBloodParticlesPerHit.GetValueOnGameThread(), 0);
	const int32 MaxBursts = FMath::Max(CVarBloodMaxBurstsPerFrame.GetValueOnGameThread(), 0);
	const float HeightOffset = CVarBloodHeightOffset.GetValueOnGameThread();

	// GetRenderPositions() is the RAW ground-plane position (Z=0) the sim publishes. task-110:
	// NS_Swarm's Sprite Renderer pivot is (0.5, 0.0) on both emitters now, so that raw position
	// IS where the sprite's feet render -- no ground correction needed here any more. (Used to
	// re-derive SwarmRenderActor's Swarm.SpriteGroundOffset/GroundScale, an absolute-uu or
	// per-sprite-size Z shift that faked the same anchoring in world space; deleted along with
	// that CVar rather than re-pointed at the pivot, since the pivot makes it dead weight.)

	int32 BurstsThisFrame = 0;
	for (int32 i = 0; i < Count && BurstsThisFrame < MaxBursts; ++i)
	{
		// Symbolic unpack only -- SwarmRenderPack::Anim masks to the low 8 bits before
		// anyone reads a bit, so this never risks reading the size/facing/squad payload
		// riding in the same int32 as an anim bit. Never hand-roll this shift; see
		// SwarmFragments.h's own warning on SwarmRenderPack.
		const uint8 Anim = SwarmRenderPack::Anim(RenderAnimBits[i]);
		if ((Anim & SwarmAnim::HitFlashBit) == 0)
		{
			continue;
		}

		FVector Location = Positions[i];
		Location.Z += HeightOffset;

		// Orient the spray with the struck unit's own published facing rather than always
		// along world axes -- see SpawnBurst's comment for how this reaches the Niagara
		// side. SwarmFacing's index is measured south-first, counter-clockwise from atan2(Y,
		// -X); inverting that back to a world direction gives Yaw = 180 - index*DegreesPerStep.
		const int32 FacingIndex = SwarmRenderPack::Facing(RenderAnimBits[i]);
		const float Yaw = 180.f - (float)FacingIndex * SwarmFacing::DegreesPerStep;
		const FRotator Rotation(0.f, Yaw, 0.f);

		SpawnBurst(World, System, Location, Rotation, ParticlesPerHit);
		++BurstsThisFrame;
	}
}
