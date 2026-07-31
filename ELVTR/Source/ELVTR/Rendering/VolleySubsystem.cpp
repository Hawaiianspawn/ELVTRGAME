#include "VolleySubsystem.h"

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
	TAutoConsoleVariable<int32> CVarVolleyEnable(
		TEXT("Volley.Enable"),
		1,
		TEXT("Master on/off for the archer volley cue. 0 skips the render-array scan entirely,\n")
		TEXT("so the feature costs nothing when off."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarVolleyMaxPerFrame(
		TEXT("Volley.MaxPerFrame"),
		16,
		TEXT("GLOBAL cap: the most volley cues this subsystem will spawn in one frame, no\n")
		TEXT("matter how many archers are mid-swing. Without it, cost scales with the size of\n")
		TEXT("the archer line and has no ceiling. Archers beyond the cap simply don't fire a\n")
		TEXT("visible arrow that frame -- they still deal their damage, the cue is cosmetic\n")
		TEXT("and always was. Which ones are skipped follows the render buffer's publish\n")
		TEXT("order, same idiom as Blood.MaxBurstsPerFrame and Swarm.MaxAttackersPerUnit.\n")
		TEXT("\n")
		TEXT("This is the HARD ceiling; Volley.CueRate is what normally keeps the count down.\n")
		TEXT("Hitting this cap every frame means the rate is set too high, not that the cap\n")
		TEXT("is too low."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarVolleyCueRate(
		TEXT("Volley.CueRate"),
		3.1f,
		TEXT("Cues per second per SWINGING archer. Each archer carrying SwarmAnim::SwingBit\n")
		TEXT("rolls this * DeltaSeconds each frame, so the cue count is frame-rate independent.\n")
		TEXT("\n")
		TEXT("Why a rate and not one-cue-per-shot: SwingBit is a POSE window, not an event --\n")
		TEXT("USwarmSwingProcessor holds it for about 0.32s of every 0.9s swing at shipped\n")
		TEXT("defaults (StrikeAt*0.5 .. StrikeAt + Interval*0.18), so ~19 consecutive frames at\n")
		TEXT("60fps carry ONE shot, and the render buffer has no per-entity id to edge-detect\n")
		TEXT("it with. 3.1 is 1/0.32 -- one cue per shot in EXPECTATION, not a guarantee: a\n")
		TEXT("given shot can produce two arrows or none. Retune this if Swarm.SwingInterval or\n")
		TEXT("Swarm.SwingStrikeAt move; the formula is 1 / (pose window in seconds). Raise it\n")
		TEXT("for a thicker, less accurate hail; drop it toward 1 for sparse single arrows."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarVolleyArrowsPerCue(
		TEXT("Volley.ArrowsPerCue"),
		3,
		TEXT("Arrow sprites in one cue. Per-cue count, not a global total -- Volley.MaxPerFrame\n")
		TEXT("and Volley.CueRate are the dials that bound total cost. More than one because a\n")
		TEXT("volley reads as a small clutch of shafts, not a single dot; NS_Volley's launch box\n")
		TEXT("gives them a slight fan so they don't fly as one particle. Drives User.Count."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarVolleyLifetime(
		TEXT("Volley.Lifetime"),
		0.75f,
		TEXT("Arrow flight time, seconds. Drives NS_Volley's User.Lifetime, and is also half of\n")
		TEXT("how far the arc travels -- see Volley.AuthoredSpeed. NS_Volley's spawn-time modules\n")
		TEXT("are configured off interpolated spawning for the same reason NS_Blood's are\n")
		TEXT("(interpolated spawn silently kills particles that die within their first partial\n")
		TEXT("frame), so short values here still render."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarVolleySize(
		TEXT("Volley.Size"),
		5.f,
		TEXT("Arrow sprite half-size in uu -- kept pixel-scale to match the sprites, not a\n")
		TEXT("fx-scale streak. Drives NS_Volley's User.Size."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarVolleyHeightOffset(
		TEXT("Volley.HeightOffset"),
		25.f,
		TEXT("Z lift, uu, above the raw published (ground-plane) position the arrow launches\n")
		TEXT("from. That raw position is where NS_Swarm's sprite pivot renders the archer's feet\n")
		TEXT("(task-110), so this lifts the launch up the body to roughly bow height. A little\n")
		TEXT("higher than Blood.HeightOffset on purpose -- a drawn bow sits above where a blow\n")
		TEXT("lands. 0 = arrows leave the archer's feet."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarVolleyAuthoredSpeed(
		TEXT("Volley.AuthoredSpeed"),
		1000.f,
		TEXT("The forward launch speed NS_Volley is AUTHORED at, uu/s -- the midpoint of its\n")
		TEXT("AddVelocity box (900..1100 forward). Not a tuning dial for how fast arrows fly:\n")
		TEXT("it is how this code converts a wanted RANGE into NS_Volley's User.SpeedScale,\n")
		TEXT("as Range / (AuthoredSpeed * Volley.Lifetime). Change it only to match an actual\n")
		TEXT("edit to the asset's launch box, or every arc lands at the wrong distance.\n")
		TEXT("\n")
		TEXT("Gravity (-980, from the asset's GravityForce module) is what bends the shot into\n")
		TEXT("an arc, and it does NOT scale with SpeedScale -- so retuning Volley.Lifetime or\n")
		TEXT("Swarm.ArchersEngageRange far off their defaults flattens or exaggerates the arc\n")
		TEXT("even though the arrows still land in the right place. Shipped defaults\n")
		TEXT("(0.75s, 750uu) are where the arc was shaped."),
		ECVF_Default);
}

TStatId UVolleySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVolleySubsystem, STATGROUP_Tickables);
}

bool UVolleySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Game and PIE only -- same reasoning as UBloodSubsystem: no reason to scan the render
	// arrays and spawn cues in an editor preview world.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UNiagaraSystem* UVolleySubsystem::GetVolleySystem()
{
	if (!bTriedLoadVolleySystem)
	{
		bTriedLoadVolleySystem = true;
		VolleySystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Gore/NS_Volley.NS_Volley"));
		if (!VolleySystem)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("VolleySubsystem: /Game/Gore/NS_Volley not found -- volley cues disabled."));
		}
	}
	return VolleySystem;
}

void UVolleySubsystem::SpawnCue(UWorld* World, UNiagaraSystem* System, const FVector& Location,
	const FRotator& Rotation, int32 ArrowCount, float Lifetime, float SpeedScale)
{
	// bAutoActivate=false / bAutoDestroy=true for exactly UBloodSubsystem's reasons: the cue
	// needs its User.* parameters written before the sim first ticks, and the subsystem owns
	// nothing about the component past this call.
	//
	// Rotation is the whole aiming mechanism. NS_Volley's AddVelocity module reads its launch
	// box in LOCAL space (bound to Engine.Owner.SystemLocalToWorld), and that box points down
	// local +X, so a pure yaw here swings the arc onto the brood. Inherited from NS_Blood's
	// spray, which works the same way -- this is why aiming needed no new user parameter.
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, System, Location, Rotation, FVector(1.f),
		/*bAutoDestroy=*/true, /*bAutoActivate=*/false);
	if (!Comp)
	{
		return;
	}

	Comp->SetVariableInt(FName("User.Count"), ArrowCount);
	Comp->SetVariableFloat(FName("User.Lifetime"), Lifetime);
	Comp->SetVariableFloat(FName("User.Size"), FMath::Max(CVarVolleySize.GetValueOnGameThread(), 0.1f));
	Comp->SetVariableFloat(FName("User.SpeedScale"), SpeedScale);

	// Match the units' own flame-lift exemption (Swarm.UnitStencil, SwarmRenderActor.cpp)
	// rather than adding a second, independent dial that could drift from it -- same call
	// UBloodSubsystem makes, same reasoning: an arrow leaving an exempted archer must not be
	// graded differently from the archer that loosed it. Read live (not cached at startup) so
	// toggling the swarm's stencil CVar for an A/B moves the arrows with it.
	static IConsoleVariable* UnitStencilCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("Swarm.UnitStencil"));
	const int32 StencilValue = UnitStencilCVar ? UnitStencilCVar->GetInt() : 0;
	Comp->SetRenderCustomDepth(StencilValue != 0);
	Comp->SetCustomDepthStencilValue(StencilValue);

	Comp->Activate(true);
}

void UVolleySubsystem::Tick(float DeltaSeconds)
{
	if (CVarVolleyEnable.GetValueOnGameThread() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	UNiagaraSystem* System = GetVolleySystem();
	if (!System)
	{
		return;
	}

	const TArray<FVector>& Positions = Swarm->GetRenderPositions();
	const TArray<int32>& RenderAnimBits = Swarm->GetRenderAnimBits();
	const int32 Count = FMath::Min(Positions.Num(), RenderAnimBits.Num());

	const int32 MaxPerFrame = FMath::Max(CVarVolleyMaxPerFrame.GetValueOnGameThread(), 0);
	const float CueChance = FMath::Clamp(CVarVolleyCueRate.GetValueOnGameThread() * DeltaSeconds, 0.f, 1.f);

	// ONE pass over the render arrays does both jobs: sums the brood so their centroid can be
	// averaged out afterwards, and collects the archers that rolled a cue. They cannot be done
	// in the same step because a cue's direction depends on a centroid the scan hasn't finished
	// computing yet -- hence the scratch array rather than a second walk over 40k entries.
	FVector BroodSum = FVector::ZeroVector;
	int32 BroodCount = 0;
	FiringScratch.Reset();

	for (int32 i = 0; i < Count; ++i)
	{
		// Symbolic unpack only -- SwarmRenderPack::Anim masks to the low 8 bits before anyone
		// reads a bit, so this never risks reading the size/facing/squad/variant payload riding
		// in the same int32. Never hand-roll this shift; see SwarmFragments.h's own warning.
		const uint8 Anim = SwarmRenderPack::Anim(RenderAnimBits[i]);

		if ((Anim & SwarmAnim::TeamBit) == 0)
		{
			// Brood. Every one of them counts toward the centroid, including the ones beyond
			// any archer's reach -- this is deliberately the whole horde's centre of mass, not
			// a per-archer neighbourhood, because a neighbourhood query per archer is exactly
			// the per-entity targeting cost this cue is scoped out of.
			BroodSum += Positions[i];
			++BroodCount;
			continue;
		}

		// Retinue. The cap is tested here and not at the top of the loop on purpose: hitting
		// the cue budget must not truncate the centroid, or the last cues of a frame would aim
		// at the average of only the brood that happened to be published early.
		if (FiringScratch.Num() >= MaxPerFrame)
		{
			continue;
		}
		if ((Anim & SwarmAnim::SwingBit) == 0)
		{
			continue;
		}
		if (SwarmSquad::UnitType(SwarmRenderPack::Squad(RenderAnimBits[i])) != EUnitType::Archers)
		{
			continue;
		}
		// See Volley.CueRate: SwingBit is a pose window ~19 frames wide, so without this roll
		// one shot would fire a cue on every one of those frames.
		if (FMath::FRand() >= CueChance)
		{
			continue;
		}

		FiringScratch.Add(Positions[i]);
	}

	if (BroodCount == 0 || FiringScratch.Num() == 0)
	{
		return;
	}

	const FVector Centroid = BroodSum / (double)BroodCount;

	// Read live rather than caching: the arcs then follow Swarm.ArchersEngageRange the same
	// frame it is dragged, and this subsystem never owns a copy of a combat number it does not
	// set. Found by name because that CVar lives in an anonymous namespace in the sim module.
	static IConsoleVariable* EngageRangeCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("Swarm.ArchersEngageRange"));
	const float EngageRange = EngageRangeCVar ? EngageRangeCVar->GetFloat() : 750.f;

	const float HeightOffset = CVarVolleyHeightOffset.GetValueOnGameThread();
	const int32 ArrowsPerCue = FMath::Max(CVarVolleyArrowsPerCue.GetValueOnGameThread(), 1);
	const float Lifetime = FMath::Max(CVarVolleyLifetime.GetValueOnGameThread(), 0.01f);
	const float AuthoredSpeed = FMath::Max(CVarVolleyAuthoredSpeed.GetValueOnGameThread(), 1.f);
	const float AuthoredRange = AuthoredSpeed * Lifetime;

	for (const FVector& From : FiringScratch)
	{
		FVector ToCentroid = Centroid - From;
		ToCentroid.Z = 0.0;	// ground plane only: the arc's rise is authored into the asset
		const double Distance = ToCentroid.Size();
		if (Distance < KINDA_SMALL_NUMBER)
		{
			continue;	// standing on the centroid; there is no direction to fire in
		}

		// Yaw only. Pitching the component toward the target would fight the launch box's own
		// +Z component and flatten the arc into a straight line at exactly the moment the
		// brood get close, which is when the arc reads best.
		const FRotator Rotation(0.f, ToCentroid.Rotation().Yaw, 0.f);

		// Land the arc on the brood, but never past what the archer can actually reach -- a
		// cue that flies further than Swarm.ArchersEngageRange would show the player a shot
		// the sim never took.
		const float Range = FMath::Min((float)Distance, EngageRange);
		SpawnCue(World, System, From + FVector(0.0, 0.0, HeightOffset), Rotation,
			ArrowsPerCue, Lifetime, Range / AuthoredRange);
	}
}
