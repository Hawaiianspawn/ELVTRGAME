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

	TickCamera(DeltaSeconds, Swarm);

	DrawHUD();
}

void ASpikeHeroPawn::TickCamera(float DeltaSeconds, const USwarmSubsystem* Swarm)
{
	if (!Camera)
	{
		return;
	}

	// --- projection ---------------------------------------------------------
	// Only touched on a change: both setters dirty the component's render state.
	const bool bOrtho = CVarCamOrtho.GetValueOnGameThread() != 0;
	const ECameraProjectionMode::Type Mode = bOrtho
		? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
	if (Camera->ProjectionMode != Mode)
	{
		Camera->SetProjectionMode(Mode);
	}
	const float OrthoWidth = FMath::Max(CVarCamOrthoWidth.GetValueOnGameThread(), 1.f);
	if (bOrtho && !FMath::IsNearlyEqual(Camera->OrthoWidth, OrthoWidth))
	{
		Camera->SetOrthoWidth(OrthoWidth);
	}
	const float Fov = FMath::Clamp(CVarCamFov.GetValueOnGameThread(), 5.f, 170.f);
	if (!bOrtho && !FMath::IsNearlyEqual(Camera->FieldOfView, Fov))
	{
		Camera->SetFieldOfView(Fov);
	}

	// --- orientation and the point it centres on ----------------------------
	// The pawn only ever translates (AddActorWorldOffset), so its component space is world-
	// aligned and a relative rotation here is a world rotation.
	const FRotator Rot(
		CVarCamPitch.GetValueOnGameThread(),
		CVarCamYaw.GetValueOnGameThread(),
		0.f);
	const FRotationMatrix Basis(Rot);
	const FVector Forward = Basis.GetUnitAxis(EAxis::X);
	const FVector Up = Basis.GetUnitAxis(EAxis::Z);

	const FVector Focus(
		CVarCamOffsetX.GetValueOnGameThread(),
		CVarCamOffsetY.GetValueOnGameThread(),
		CVarCamOffsetZ.GetValueOnGameThread());
	const float Dist = CVarCamDist.GetValueOnGameThread();

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
