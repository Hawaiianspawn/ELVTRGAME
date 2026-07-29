#include "SpikeHeroPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmSubsystem.h"
#include "Spike1GameMode.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TAutoConsoleVariable<float> CVarCamHudBias(
		TEXT("Emberkeep.Cam.HudBias"), 1.f,
		TEXT("How far the game camera slides to compensate for the combat HUD covering the\n")
		TEXT("bottom of the screen, as a multiple of the exact correction. 1 = the hero sits\n")
		TEXT("dead centre of the strip you can actually see (default). 0 = off, hero centred in\n")
		TEXT("the full viewport and therefore crowded up against the top by the HUD. Values above\n")
		TEXT("1 overshoot, pushing him further up into open ground ahead of the fight."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamHudBiasLerp(
		TEXT("Emberkeep.Cam.HudBiasLerp"), 6.f,
		TEXT("How fast the camera eases to a new HUD bias (FInterpTo speed). The HUD resizes in\n")
		TEXT("steps as the body count changes, and snapping on those steps reads as the world\n")
		TEXT("twitching. 0 = snap instantly."),
		ECVF_Default);

	// --- the shot itself ----------------------------------------------------
	// The camera used to be frozen by the constructor (ortho, pitch -90, height 1200) with only
	// the HUD bias moving at runtime, so none of the framing could be judged without a rebuild.
	// These own the whole transform every frame; the constructor's values now only matter for
	// the editor preview before BeginPlay. Defaults reproduce the old shot exactly.

	TAutoConsoleVariable<int32> CVarCamOrtho(
		TEXT("Emberkeep.Cam.Ortho"), 1,
		TEXT("1 = orthographic (the shipped look: no perspective convergence, so the horde reads\n")
		TEXT("as a flat tactical field). 0 = perspective, which brings back foreshortening and\n")
		TEXT("makes Fov and Dist meaningful as separate dials."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOrthoWidth(
		TEXT("Emberkeep.Cam.OrthoWidth"), 2400.f,
		TEXT("Ortho view WIDTH in world units — the zoom dial while Ortho is 1. This is the\n")
		TEXT("framing number to compare against the retinue bbox in the spacing report when\n")
		TEXT("deciding whether the army is too small or the camera is too wide."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamFov(
		TEXT("Emberkeep.Cam.Fov"), 60.f,
		TEXT("Horizontal field of view in degrees, used only while Ortho is 0."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamDist(
		TEXT("Emberkeep.Cam.Dist"), 1200.f,
		TEXT("How far the camera sits from the hero along its own view axis, in uu. At the\n")
		TEXT("default Pitch of -90 this is literally height above him. Under ortho it does NOT\n")
		TEXT("zoom (that is OrthoWidth) — it only moves the near/far planes through the world."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamPitch(
		TEXT("Emberkeep.Cam.Pitch"), -90.f,
		TEXT("Camera pitch in degrees. -90 = straight down (default). Raising it toward about\n")
		TEXT("-50 gives an angled RTS shot — note the units are camera-facing billboards, so a\n")
		TEXT("shallow pitch is what makes their facing artwork read at all."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamYaw(
		TEXT("Emberkeep.Cam.Yaw"), 0.f,
		TEXT("Orbit the whole view around the hero, in degrees. At Pitch -90 this spins the map\n")
		TEXT("under you. See Emberkeep.Cam.YawInput — without it WASD keeps pushing along world\n")
		TEXT("axes and stops matching the screen as soon as this leaves 0."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamYawInput(
		TEXT("Emberkeep.Cam.YawInput"), 1,
		TEXT("1 = rotate WASD by the camera yaw so 'W' always means up-screen (default). 0 = raw\n")
		TEXT("world-axis movement, which is what the spike always did and is only correct while\n")
		TEXT("Yaw is 0. Turn this off only if you want to feel the world axes directly."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOffsetX(
		TEXT("Emberkeep.Cam.OffsetX"), 0.f,
		TEXT("World-space X offset applied to what the camera centres on. Shifts the framing\n")
		TEXT("without moving the hero — lead room ahead of the fight, for instance."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOffsetY(
		TEXT("Emberkeep.Cam.OffsetY"), 0.f,
		TEXT("World-space Y offset applied to the camera's focus point."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOffsetZ(
		TEXT("Emberkeep.Cam.OffsetZ"), 0.f,
		TEXT("World-space Z offset applied to the camera's focus point. Useful to aim above the\n")
		TEXT("hero's feet once Pitch is shallow enough for ground level to matter."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamLerp(
		TEXT("Emberkeep.Cam.Lerp"), 0.f,
		TEXT("Smoothing on camera moves (FInterpTo speed), so dragging a dial glides instead of\n")
		TEXT("teleporting. 0 = snap, which is the honest setting while tuning: smoothing hides\n")
		TEXT("how far a value actually moved the shot."),
		ECVF_Default);

	// --- army-scale camera (docs/design/CAMERA-SCALE.md) -------------------------
	// "The camera tells you whether you are an army or a man." One scalar — how much army
	// you still have — drives width, pitch, distance and projection together. While Scale is
	// 0 every dial below is inert and the camera behaves exactly as it always has.

	TAutoConsoleVariable<int32> CVarCamScale(
		TEXT("Emberkeep.Cam.Scale"), 0,
		TEXT("1 = the camera scales with how much army you have left; 0 = off (default), and the\n")
		TEXT("Cam.OrthoWidth / Pitch / Dist / Ortho dials drive the shot by hand as before.\n")
		TEXT("While on, this OVERRIDES those four — they stop responding, by design."),
		ECVF_Default);

	// Weights deliberately mirror Emberkeep.UnitCamProj.Size* — the Unit Cam already solved
	// "how much army is this" and the two views must not disagree about it.
	TAutoConsoleVariable<float> CVarCamScaleRetinueWeight(
		TEXT("Emberkeep.Cam.ScaleRetinueWeight"), 10.f,
		TEXT("Weight of one live retinue soldier in the army-scale total. Heavily outweighs\n")
		TEXT("ScaleBroodWeight on purpose: attrition of YOUR army is what the shot is about."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleBroodWeight(
		TEXT("Emberkeep.Cam.ScaleBroodWeight"), 0.25f,
		TEXT("Weight of one live brood in the army-scale total. Small but non-zero, so being\n")
		TEXT("swarmed widens the shot a little — the tide is part of the picture."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleBodies(
		TEXT("Emberkeep.Cam.ScaleBodies"), 1200.f,
		TEXT("WEIGHTED body count at which the camera is fully at its 'army' end. Not a raw\n")
		TEXT("headcount — see ScaleRetinueWeight / ScaleBroodWeight.\n")
		TEXT("Calibrated to RetinueCap * ScaleRetinueWeight (120 * 10), so a FULL RETINUE ON ITS\n")
		TEXT("OWN pins the scalar at 1 and you get the true top-down map — exactly the shipped\n")
		TEXT("framing. Brood then only holds you at the wide end, it can never push past it.\n")
		TEXT("Raise this above the retinue's own weight and full strength no longer reaches the\n")
		TEXT("top of its own axis: the map view becomes unreachable in normal play."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleCurve(
		TEXT("Emberkeep.Cam.ScaleCurve"), 1.f,
		TEXT("Shaping exponent on the 0..1 army scalar. 1 = linear. >1 holds the wide shot\n")
		TEXT("longer and collapses late (attrition feels survivable, then sudden); <1 descends\n")
		TEXT("early and eases out."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleWidthFull(
		TEXT("Emberkeep.Cam.ScaleWidthFull"), 2400.f,
		TEXT("World units across the view at full army. Matches the shipped OrthoWidth, so a\n")
		TEXT("full-strength run looks exactly like today."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleWidthAlone(
		TEXT("Emberkeep.Cam.ScaleWidthAlone"), 700.f,
		TEXT("World units across the view with nobody left — the character-camera end."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScalePitchFull(
		// -55 since 2026-07-28 (owner call), was -90. One-camera mode made this the only shot
		// the game has, and a straight-down map cannot show the arena you are fighting across —
		// it also shows every sprite from directly above, which is the one angle the 8-direction
		// sheet has nothing authored for. -55 keeps the RTS read while letting the horde arrive
		// out of depth rather than slide in from a screen edge.
		//
		// TIED TO Swarm.SimLOD.NearRadius. Tilting the camera stretches how far down-field you
		// can see: ortho height is OrthoWidth/aspect on SCREEN, but that maps to
		// (OrthoWidth/aspect)/sin(pitch) of GROUND. At -90 that is ~1350uu, at -55 it is ~1650uu.
		// Lower this further without raising NearRadius and the sim LOD starts striding units the
		// player can watch. The pairing is the constraint, not either value alone.
		TEXT("Emberkeep.Cam.ScalePitchFull"), -55.f,
		TEXT("Camera pitch at full army. -90 = straight down (the old battlefield map), -55 =\n")
		TEXT("tilted so the arena reads as ground you look ACROSS rather than down at.\n")
		TEXT("Shallower sees further down-field — raise Swarm.SimLOD.NearRadius to match, or the\n")
		TEXT("LOD will begin striding units that are on screen. Ground depth is roughly\n")
		TEXT("(OrthoWidth / aspect) / sin(pitch)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScalePitchAlone(
		TEXT("Emberkeep.Cam.ScalePitchAlone"), -22.f,
		TEXT("Camera pitch with nobody left. Shallow enough to read as standing behind a person\n")
		TEXT("rather than looking down at a token."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleDistFull(
		TEXT("Emberkeep.Cam.ScaleDistFull"), 1200.f,
		TEXT("Camera distance back along the view axis at full army."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleDistAlone(
		TEXT("Emberkeep.Cam.ScaleDistAlone"), 420.f,
		TEXT("Camera distance with nobody left. Short enough that perspective actually bites."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleSwapAt(
		TEXT("Emberkeep.Cam.ScaleSwapAt"), 0.35f,
		TEXT("Army scalar (0..1) below which the projection switches to perspective. You cannot\n")
		TEXT("lerp an ortho matrix into a perspective one, so this is a HARD swap — but the FOV\n")
		TEXT("is solved from the same framing width, so the frame is identical across the cut and\n")
		TEXT("only parallax changes. Set 0 to stay orthographic for the whole run."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamScaleStages(
		TEXT("Emberkeep.Cam.ScaleStages"), 0,
		TEXT("0 = continuous (the camera is always subtly moving). N > 0 quantises the army\n")
		TEXT("scalar into N steps, so the shot SETTLES and only re-frames when a step is\n")
		TEXT("crossed. This is CAMERA-SCALE.md's open question 2 as a live dial — try 4 or 5\n")
		TEXT("against 0 and judge, rather than deciding it on paper."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamScaleRatchet(
		TEXT("Emberkeep.Cam.ScaleRatchet"), 0,
		TEXT("1 = one-way: the camera descends as the army dies and never rises again, even if a\n")
		TEXT("breather refills the retinue. 0 = reversible. CAMERA-SCALE.md's open question 3 —\n")
		TEXT("one-way is the stronger dramatic statement and the worse feedback loop. Resets each\n")
		TEXT("time the camera is (re)placed, i.e. per run."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleLerp(
		TEXT("Emberkeep.Cam.ScaleLerp"), 1.5f,
		TEXT("Easing speed on the army scalar itself (FInterpTo). Deliberately separate from\n")
		TEXT("Cam.Lerp: this smooths the DRIVER so a squad wiping doesn't jolt the shot, while\n")
		TEXT("Cam.Lerp smooths the resulting move. 0 = react instantly.\n")
		TEXT("ONLY USED when Cam.ScaleStiffness is 0 — the spring below supersedes it."),
		ECVF_Default);

	// A spring, not a lerp, because of what the two actually feel like. FInterpTo eases OUT
	// only: it moves fastest on the first frame after the army changes and decelerates into
	// the target, so every casualty reads as a small shove. A damped spring starts from rest,
	// accelerates, and settles — the camera slows INTO the new scale at both ends. That is the
	// motion the owner asked for, and it matches how the flame already moves (TickFlame's
	// stiffness/damping pair), so the two big continuous motions in the game share a feel.
	TAutoConsoleVariable<float> CVarCamScaleStiffness(
		TEXT("Emberkeep.Cam.ScaleStiffness"), 3.f,
		TEXT("Spring stiffness (natural frequency, rad/s) pulling the army scalar toward its\n")
		TEXT("target. Higher = the camera commits to a new scale sooner. 0 disables the spring\n")
		TEXT("entirely and falls back to Cam.ScaleLerp's plain FInterpTo.\n")
		TEXT("Roughly: settle time to ~5%% is about 3/Stiffness seconds at Damping 1, so the\n")
		TEXT("default 3 settles in ~1s."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleDamping(
		TEXT("Emberkeep.Cam.ScaleDamping"), 1.f,
		TEXT("Spring damping RATIO. 1 = critically damped: fastest approach with NO overshoot,\n")
		TEXT("and the right default here because overshoot on this scalar means the camera\n")
		TEXT("visibly rebounds past the framing and comes back, which reads as a mistake rather\n")
		TEXT("than as weight. Below 1 overshoots and oscillates; above 1 is sluggish but always\n")
		TEXT("monotonic. Try 0.7 only if the descent wants to feel like it lurches."),
		ECVF_Default);

	// --- the bearer's placeholder body --------------------------------------
	// BodyMesh is a 0.6 x 0.6 x 1.2 engine cube standing in for the hero until he has art.
	// It was invisible in practice from 1200uu straight down — a few pixels under the flame.
	// The pinned eye-level shot puts the lens 323uu behind him at -8.2 deg, which stands that
	// white block squarely between the camera and the line it exists to show.
	TAutoConsoleVariable<int32> CVarHeroProxy(
		// DEFAULT 0 since 2026-07-28 (owner: "get rid of the hero white block we dont need it").
		// Was 1 for the few hours between this dial existing and that call. Defaulting it off
		// rather than deleting BodyMesh, because BodyMesh is the pawn's RootComponent — removing
		// the component would take the actor's transform with it, and every system that reads the
		// bearer (flame position, retinue attractor, camera focus) hangs off that transform.
		// Hiding is the correct way to "get rid of" this; deleting is not.
		TEXT("Swarm.HeroProxy"), 0,
		TEXT("Draw the bearer's placeholder cube body. 0 hides it: the pawn still moves, still\n")
		TEXT("attracts the retinue, still fights and still carries the flame — only the mesh\n")
		TEXT("stops rendering. Safe to hide because nothing reads the mesh: the camera hangs off\n")
		TEXT("the PAWN (TickCamera builds its transform from the actor location), the flame is\n")
		TEXT("published from GetActorLocation, and the mesh has collision off already.\n")
		TEXT("Set 0 whenever the camera is low enough that the proxy blocks the shot; set 1 to\n")
		TEXT("confirm where he actually is standing."),
		ECVF_Default);
}

namespace
{
	/**
	 * Eased army scalar, and the low-water mark used by ScaleRatchet.
	 *
	 * File-static rather than members on purpose: adding a member would change the class
	 * layout, which Live Coding cannot apply — it reports success and then crashes the next
	 * PIE. Keeping this out of the header means the whole army-scale feature is a
	 * function-body change and stays hot-reloadable while it is being tuned. One hero pawn
	 * exists in the prototype, so a single static is sound; it becomes wrong the moment
	 * there are two (4-player co-op), and should move onto the pawn then.
	 */
	float GCamArmyScale = 0.f;
	float GCamArmyScaleLow = 1.f;
	float GCamArmyScaleVel = 0.f;
}

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
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(2.f * CameraHeight); // matches prior perspective framing at 90-deg FOV
}

void ASpikeHeroPawn::BeginPlay()
{
	Super::BeginPlay();

	// Charge aims at the cursor, so the cursor has to be visible.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
	}

	if (USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->SetAttractor(GetActorLocation());
	}

	// Framing check: compare against the retinue bbox in the spacing report to
	// tell "army is too small" apart from "camera is too wide".
	if (Camera)
	{
		UE_LOG(LogTemp, Display, TEXT("SwarmDebug: camera ortho=%.0fuu height=%.0fuu projection=%s"),
			Camera->OrthoWidth, CameraHeight,
			Camera->ProjectionMode == ECameraProjectionMode::Orthographic ? TEXT("ortho") : TEXT("perspective"));
	}
}

bool ASpikeHeroPawn::ConsumeKeyPress(const APlayerController& PC, const FKey& Key, bool& bWasDown) const
{
	const bool bIsDown = PC.IsInputKeyDown(Key);
	const bool bPressed = bIsDown && !bWasDown;
	bWasDown = bIsDown;
	return bPressed;
}

bool ASpikeHeroPawn::GetCursorGroundLocation(FVector& OutLocation) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	FVector WorldOrigin, WorldDirection;
	if (!PC || !PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection)
		|| FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	// Everything in the prototype lives on the Z = hero plane.
	const float PlaneZ = GetActorLocation().Z;
	const float T = (PlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (T < 0.f)
	{
		return false;
	}

	OutLocation = WorldOrigin + WorldDirection * T;
	return true;
}

void ASpikeHeroPawn::TickStanceInput(const APlayerController& PC)
{
	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	if (ConsumeKeyPress(PC, EKeys::One, bWasDownFollow))
	{
		Swarm->SetStance(ESwarmStance::Follow, GetActorLocation());
	}
	else if (ConsumeKeyPress(PC, EKeys::Two, bWasDownCharge))
	{
		// Aim at the cursor; fall back to straight ahead if the deproject fails.
		FVector Target;
		if (!GetCursorGroundLocation(Target))
		{
			Target = GetActorLocation() + FVector(1000.f, 0.f, 0.f);
		}
		Swarm->SetStance(ESwarmStance::Charge, Target);
	}
	else if (ConsumeKeyPress(PC, EKeys::Three, bWasDownHold))
	{
		// Hold anchors where the hero stands when the order is given.
		Swarm->SetStance(ESwarmStance::Hold, GetActorLocation());
	}
	else if (ConsumeKeyPress(PC, EKeys::Four, bWasDownRally))
	{
		Swarm->SetStance(ESwarmStance::Rally, GetActorLocation());
	}

	if (ConsumeKeyPress(PC, EKeys::R, bWasDownRestart))
	{
		if (ASpike1GameMode* GameMode = GetWorld()->GetAuthGameMode<ASpike1GameMode>())
		{
			GameMode->RestartRun();
		}
	}
}

void ASpikeHeroPawn::TickHeroCombat(float DeltaSeconds)
{
	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	if (!Swarm->IsHeroAlive())
	{
		Swarm->SetHeroStriking(false);
		return;
	}

	// --- the hero's swing cadence ---------------------------------------
	// Runs unconditionally rather than only when brood are in reach: the combat pass
	// already checks HeroMeleeRange before a blow does anything, so an idle hero just
	// swings at air harmlessly, and the pawn doesn't need a contact signal it has no
	// clean way to compute. Published as a one-frame flag, so it self-clears.
	const float SwingInterval = SwarmCombatTuning::SwingInterval();
	const float StrikeAt = SwingInterval * SwarmCombatTuning::SwingStrikeAt();
	const float PreviousSwing = HeroSwingTime;
	HeroSwingTime += DeltaSeconds;
	Swarm->SetHeroStriking(PreviousSwing < StrikeAt && HeroSwingTime >= StrikeAt);
	if (HeroSwingTime >= SwingInterval)
	{
		HeroSwingTime = FMath::Fmod(HeroSwingTime, SwingInterval);
	}

	const float Damage = Swarm->ConsumePendingHeroDamage();
	if (Damage > 0.f)
	{
		Swarm->SetHeroHP(Swarm->GetHeroHP() - Damage);
		if (Swarm->GetHeroHP() <= 0.f)
		{
			Swarm->SetHeroAlive(false);
		}
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

	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	const bool bAlive = Swarm && Swarm->IsHeroAlive();

	if (bAlive)
	{
		FVector Input = FVector::ZeroVector;
		if (PC->IsInputKeyDown(EKeys::W)) { Input.X += 1.f; }
		if (PC->IsInputKeyDown(EKeys::S)) { Input.X -= 1.f; }
		if (PC->IsInputKeyDown(EKeys::D)) { Input.Y += 1.f; }
		if (PC->IsInputKeyDown(EKeys::A)) { Input.Y -= 1.f; }

		if (!Input.IsNearlyZero())
		{
			// Turn WASD into screen directions rather than world axes. Without this, yawing the
			// camera leaves 'W' pushing along world +X while the screen has rotated under it, so
			// the movement dials and the camera dials fight each other.
			FVector Move = Input.GetSafeNormal();
			if (CVarCamYawInput.GetValueOnGameThread() != 0)
			{
				Move = FRotator(0.f, CVarCamYaw.GetValueOnGameThread(), 0.f).RotateVector(Move);
			}
			AddActorWorldOffset(Move * MoveSpeed * DeltaSeconds);
		}
	}

	TickStanceInput(*PC);
	TickHeroCombat(DeltaSeconds);

	if (Swarm)
	{
		Swarm->SetAttractor(GetActorLocation());
	}

	// Live, not BeginPlay-only, so the dial can be dragged against the running shot — the
	// whole point of it is judging whether the proxy is in the way at a given camera angle.
	if (BodyMesh)
	{
		const bool bShowProxy = CVarHeroProxy.GetValueOnGameThread() != 0;
		if (BodyMesh->IsVisible() != bShowProxy)
		{
			BodyMesh->SetVisibility(bShowProxy);
		}
	}

	TickCamera(DeltaSeconds, Swarm);

	DrawHUD();
}

void ASpikeHeroPawn::TickCamera(float DeltaSeconds, const USwarmSubsystem* Swarm)
{
	if (!Camera)
	{
		return;
	}

	// --- army scale ---------------------------------------------------------
	// One scalar, 0 (alone) .. 1 (full army), drives width, pitch, distance and projection
	// together so they can never disagree about what the shot is saying.
	const bool bScale = CVarCamScale.GetValueOnGameThread() != 0;

	// Hand-driven defaults. Each is replaced below when army scale is on.
	bool bOrtho = CVarCamOrtho.GetValueOnGameThread() != 0;
	float Width = FMath::Max(CVarCamOrthoWidth.GetValueOnGameThread(), 1.f);
	float Fov = FMath::Clamp(CVarCamFov.GetValueOnGameThread(), 5.f, 170.f);
	float Pitch = CVarCamPitch.GetValueOnGameThread();
	float Dist = CVarCamDist.GetValueOnGameThread();

	if (bScale)
	{
		// Weighted, not a headcount — the same total the Unit Cam sizes itself from
		// (UnitCamProjector.cpp). Your soldiers drive the framing; the tide only nudges it.
		float Army = 0.f;
		if (Swarm)
		{
			const float Weighted =
				(float)Swarm->GetAliveRetinue() * CVarCamScaleRetinueWeight.GetValueOnGameThread() +
				(float)Swarm->GetAliveBrood() * CVarCamScaleBroodWeight.GetValueOnGameThread();
			const float Full = FMath::Max(CVarCamScaleBodies.GetValueOnGameThread(), 1.f);
			Army = FMath::Clamp(Weighted / Full, 0.f, 1.f);

			const float Curve = CVarCamScaleCurve.GetValueOnGameThread();
			if (Curve > 0.f && !FMath::IsNearlyEqual(Curve, 1.f))
			{
				Army = FMath::Pow(Army, Curve);
			}
		}

		// Quantise BEFORE easing, so a stage change still glides into place rather than
		// snapping — stages decide when the shot re-frames, not how abruptly.
		const int32 Stages = CVarCamScaleStages.GetValueOnGameThread();
		if (Stages > 0)
		{
			Army = FMath::RoundToFloat(Army * (float)Stages) / (float)Stages;
		}

		// Ratchet: track the low-water mark so a breather that refills the retinue does not
		// lift the camera back up. Reset with the camera placement, i.e. once per run.
		if (!bCameraPlaced)
		{
			GCamArmyScale = Army;
			GCamArmyScaleLow = Army;
			GCamArmyScaleVel = 0.f;
		}
		GCamArmyScaleLow = FMath::Min(GCamArmyScaleLow, Army);
		const float TargetArmy = CVarCamScaleRatchet.GetValueOnGameThread() != 0
			? GCamArmyScaleLow : Army;

		const float Stiffness = FMath::Max(CVarCamScaleStiffness.GetValueOnGameThread(), 0.f);
		if (Stiffness > 0.f && bCameraPlaced)
		{
			// Critically-dampable spring: accel = w^2 * (target - x) - 2*zeta*w * v.
			// Explicit Euler is fine at this stiffness, but a frame hitch (or a PIE
			// breakpoint) can hand us a dt large enough to make it blow up, so clamp the
			// step rather than trusting DeltaSeconds. A camera that explodes on a hitch is
			// worse than one that eases a fraction slower through it.
			const float Zeta = FMath::Max(CVarCamScaleDamping.GetValueOnGameThread(), 0.f);
			const float Dt = FMath::Min(DeltaSeconds, 1.f / 30.f);
			const float Accel = Stiffness * Stiffness * (TargetArmy - GCamArmyScale)
							  - 2.f * Zeta * Stiffness * GCamArmyScaleVel;
			GCamArmyScaleVel += Accel * Dt;
			GCamArmyScale += GCamArmyScaleVel * Dt;
		}
		else
		{
			// Legacy path: plain FInterpTo, kept so Stiffness 0 is a true A/B against the
			// spring rather than just "no smoothing at all".
			const float ScaleSpeed = FMath::Max(CVarCamScaleLerp.GetValueOnGameThread(), 0.f);
			GCamArmyScale = (ScaleSpeed > 0.f && bCameraPlaced)
				? FMath::FInterpTo(GCamArmyScale, TargetArmy, DeltaSeconds, ScaleSpeed)
				: TargetArmy;
			GCamArmyScaleVel = 0.f;
		}

		const float A = FMath::Clamp(GCamArmyScale, 0.f, 1.f);
		Width = FMath::Lerp(
			CVarCamScaleWidthAlone.GetValueOnGameThread(),
			CVarCamScaleWidthFull.GetValueOnGameThread(), A);
		Pitch = FMath::Lerp(
			CVarCamScalePitchAlone.GetValueOnGameThread(),
			CVarCamScalePitchFull.GetValueOnGameThread(), A);
		Dist = FMath::Lerp(
			CVarCamScaleDistAlone.GetValueOnGameThread(),
			CVarCamScaleDistFull.GetValueOnGameThread(), A);
		Width = FMath::Max(Width, 1.f);

		// The hard swap. An ortho matrix cannot be blended into a perspective one, so instead
		// of interpolating projections we CUT between them at a frame where both frame the
		// same world width: ortho spans Width; perspective spans 2*Dist*tan(Fov/2). Solving
		// that for Fov makes the two images agree at the seam, so the only thing that changes
		// across the cut is parallax — and at this pitch and distance that is nearly nil.
		bOrtho = A >= CVarCamScaleSwapAt.GetValueOnGameThread();
		if (!bOrtho)
		{
			const float SafeDist = FMath::Max(FMath::Abs(Dist), 1.f);
			Fov = FMath::RadiansToDegrees(2.f * FMath::Atan(Width / (2.f * SafeDist)));
			Fov = FMath::Clamp(Fov, 5.f, 170.f);
		}
	}

	// --- projection ---------------------------------------------------------
	// Only touched on a change: both setters dirty the component's render state.
	const ECameraProjectionMode::Type Mode = bOrtho
		? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	if (Camera->ProjectionMode != Mode)
	{
		Camera->SetProjectionMode(Mode);
	}
	const float OrthoWidth = Width;
	if (bOrtho && !FMath::IsNearlyEqual(Camera->OrthoWidth, OrthoWidth))
	{
		Camera->SetOrthoWidth(OrthoWidth);
	}
	if (!bOrtho && !FMath::IsNearlyEqual(Camera->FieldOfView, Fov))
	{
		Camera->SetFieldOfView(Fov);
	}

	// --- orientation and the point it centres on ----------------------------
	// The pawn only ever translates (AddActorWorldOffset), so its component space is world-
	// aligned and a relative rotation here is a world rotation.
	const FRotator Rot(
		Pitch,
		CVarCamYaw.GetValueOnGameThread(),
		0.f);
	const FRotationMatrix Basis(Rot);
	const FVector Forward = Basis.GetUnitAxis(EAxis::X);
	const FVector Up = Basis.GetUnitAxis(EAxis::Z);

	const FVector Focus(
		CVarCamOffsetX.GetValueOnGameThread(),
		CVarCamOffsetY.GetValueOnGameThread(),
		CVarCamOffsetZ.GetValueOnGameThread());

	// --- HUD bias -----------------------------------------------------------
	// The combat HUD covers the bottom of the screen, so the middle of the VIEWPORT is not the
	// middle of what you can see. Slide the camera along its own up-axis until the hero sits in
	// the centre of the unobstructed strip instead. The HUD scales with the body count, so the
	// correction has to track it every frame rather than be baked in.
	float TargetOffset = 0.f;
	const float Strength = CVarCamHudBias.GetValueOnGameThread();
	if (Swarm && Strength > 0.f)
	{
		const float Occluded = Swarm->GetHudOccludedFraction();
		if (Occluded > 0.f)
		{
			FVector2D ViewportSize = FVector2D::ZeroVector;
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(ViewportSize);
			}
			if (ViewportSize.X > 1.0 && ViewportSize.Y > 1.0)
			{
				const float AspectY = (float)(ViewportSize.Y / ViewportSize.X);
				// World height of the view at the focus. Ortho pixels are square so the height
				// falls out of OrthoWidth; under perspective it depends on how far away we are,
				// which is why this can't just read OrthoWidth any more.
				const float VerticalExtent = bOrtho
					? OrthoWidth * AspectY
					: 2.f * FMath::Abs(Dist) * FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5f) * AspectY;
				// Half the occluded band. Negative pushes the camera DOWN its own up-axis, which
				// lifts the hero UP the screen, out from behind the HUD.
				TargetOffset = -0.5f * Occluded * VerticalExtent * Strength;
			}
		}
	}

	// Ease toward it: the HUD resizes in steps as squads die, and snapping the camera on those
	// steps would read as the world twitching.
	const float BiasSpeed = FMath::Max(CVarCamHudBiasLerp.GetValueOnGameThread(), 0.f);
	CameraHudBias = BiasSpeed > 0.f
		? FMath::FInterpTo(CameraHudBias, TargetOffset, DeltaSeconds, BiasSpeed)
		: TargetOffset;

	// --- commit -------------------------------------------------------------
	// Sit Dist back along the view axis from the focus, then apply the bias along up.
	const FVector Target = Focus - Forward * Dist + Up * CameraHudBias;

	const float Lerp = FMath::Max(CVarCamLerp.GetValueOnGameThread(), 0.f);
	if (Lerp > 0.f && bCameraPlaced)
	{
		CameraLoc = FMath::VInterpTo(CameraLoc, Target, DeltaSeconds, Lerp);
		CameraRot = FMath::RInterpTo(CameraRot, Rot, DeltaSeconds, Lerp);
	}
	else
	{
		// First frame has nothing to interpolate from — easing in from the origin would swoop
		// the camera across the map on BeginPlay.
		CameraLoc = Target;
		CameraRot = Rot;
		bCameraPlaced = true;
	}

	Camera->SetRelativeLocationAndRotation(CameraLoc, CameraRot);
}

void ASpikeHeroPawn::DrawHUD() const
{
	const USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!GEngine || !Swarm)
	{
		return;
	}

	const ASpike1GameMode* GameMode = GetWorld()->GetAuthGameMode<ASpike1GameMode>();

	// --- run banner ------------------------------------------------------
	if (GameMode)
	{
		FString Banner;
		FColor BannerColor = FColor::White;

		switch (GameMode->GetPhase())
		{
		case ERunPhase::Deploying:
			Banner = TEXT("DEPLOYING — form up");
			BannerColor = FColor::Cyan;
			break;
		case ERunPhase::WaveActive:
			Banner = FString::Printf(TEXT("WAVE %d / %d"), GameMode->GetWaveIndex() + 1, GameMode->GetWaveCount());
			BannerColor = FColor::Orange;
			break;
		case ERunPhase::Breather:
			Banner = FString::Printf(TEXT("WAVE CLEARED — reinforcements in %.0fs"),
				FMath::Max(0.f, GameMode->BreatherSeconds - GameMode->GetPhaseTimer()));
			BannerColor = FColor::Green;
			break;
		case ERunPhase::Won:
			Banner = TEXT("*** THE LINE HELD — VICTORY ***   [R] restart");
			BannerColor = FColor::Green;
			break;
		case ERunPhase::Lost:
			Banner = TEXT("*** THE HERO HAS FALLEN ***   [R] restart");
			BannerColor = FColor::Red;
			break;
		}
		GEngine->AddOnScreenDebugMessage(1, 0.f, BannerColor, Banner);
	}

	// --- hero ------------------------------------------------------------
	const float HPFraction = Swarm->GetHeroHP() / FMath::Max(1.f, Swarm->GetHeroMaxHP());
	GEngine->AddOnScreenDebugMessage(2, 0.f,
		HPFraction > 0.5f ? FColor::Green : (HPFraction > 0.25f ? FColor::Yellow : FColor::Red),
		FString::Printf(TEXT("HERO  %3.0f / %3.0f"), Swarm->GetHeroHP(), Swarm->GetHeroMaxHP()));

	// --- army ------------------------------------------------------------
	GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Cyan,
		FString::Printf(TEXT("RETINUE %4d      BROOD %4d"), Swarm->GetAliveRetinue(), Swarm->GetAliveBrood()));

	// --- stance + leash ---------------------------------------------------
	const int32 Broken = Swarm->GetLeashBrokenCount();
	GEngine->AddOnScreenDebugMessage(4, 0.f, Broken > 0 ? FColor::Yellow : FColor::White,
		FString::Printf(TEXT("STANCE  %s%s"),
			LexToString(Swarm->GetStance()),
			Broken > 0 ? *FString::Printf(TEXT("   (%d leashed back to Follow)"), Broken) : TEXT("")));

	GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Silver,
		TEXT("[WASD] move   [1] Follow  [2] Charge (at cursor)  [3] Hold  [4] Rally   [R] restart"));
}
