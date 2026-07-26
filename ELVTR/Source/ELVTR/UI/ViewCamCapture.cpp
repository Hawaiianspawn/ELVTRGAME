#include "UI/ViewCamCapture.h"

#include "Mass/SwarmSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarViewCamRes(
		TEXT("Emberkeep.UI.ViewCam.Res"), 640,
		TEXT("Longest edge of the view-camera render target, in px. This is a SECOND full\n")
		TEXT("render of the scene, so keep it near the panel's real pixel size — rendering it\n")
		TEXT("at screen resolution buys nothing a 800px-wide panel can show."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarViewCamRate(
		TEXT("Emberkeep.UI.ViewCam.Rate"), 1,
		TEXT("Capture the view camera every N frames. 1 = every frame (smoothest, most\n")
		TEXT("expensive). 2-3 visibly cheapens a heavy wave at the cost of a choppier panel.\n")
		TEXT("The main cost dial for the mirrored game view."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarViewCamMode(
		TEXT("Emberkeep.UI.ViewCam.Mode"), 1,
		TEXT("What the view panel shows. 0 = MIRROR the player camera exactly (the panel then\n")
		TEXT("duplicates the screen behind it — mostly a debug mode). 1 = MINIMAP: the same\n")
		TEXT("top-down axis but pulled back to its own zoom, so it shows the ground the main\n")
		TEXT("2400uu view cannot — including the ring the brood spawn on."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarViewCamMapWidth(
		TEXT("Emberkeep.UI.ViewCam.MapWidth"), 9000.f,
		TEXT("Minimap ortho width in uu — how much ground the panel covers. The main view is\n")
		TEXT("2400uu; brood spawn between 2500 and 4000uu out, so anything under ~8000 hides\n")
		TEXT("the spawn ring the minimap exists to show."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarViewCamMapHeight(
		TEXT("Emberkeep.UI.ViewCam.MapHeight"), 5000.f,
		TEXT("Minimap camera altitude above the hero, in uu. Ortho projection means this does\n")
		TEXT("not change framing (MapWidth does) — it only sets what gets clipped by the near\n")
		TEXT("plane, so keep it comfortably above the tallest thing on the field."),
		ECVF_Default);
}

AViewCamCapture::AViewCamCapture()
{
	PrimaryActorTick.bCanEverTick = true;
	// Tick late so the camera manager has already resolved this frame's POV — otherwise the
	// panel mirrors the camera one frame stale and lags the real view while moving.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	RootComponent = Capture;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR; // demichrome PPV applies
	Capture->bCaptureEveryFrame = false; // driven manually so Rate can throttle it
	Capture->bCaptureOnMovement = false;
}

AViewCamCapture* AViewCamCapture::FindOrSpawn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AViewCamCapture> It(World); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient; // a view mirror is never saved into the level
	return World->SpawnActor<AViewCamCapture>(AViewCamCapture::StaticClass(),
		FTransform::Identity, Params);
}

bool AViewCamCapture::IsMinimapMode()
{
	return CVarViewCamMode.GetValueOnGameThread() != 0;
}

FText AViewCamCapture::ModeLabel()
{
	return FText::FromString(IsMinimapMode() ? TEXT("MINIMAP · live") : TEXT("VIEW CAM · mirror"));
}

void AViewCamCapture::SetPanelAspect(float InAspect)
{
	const float Clamped = FMath::Clamp(InAspect, 0.2f, 8.f);
	if (!FMath::IsNearlyEqual(Clamped, PanelAspect, 0.01f))
	{
		PanelAspect = Clamped;
		RebuildRenderTarget();
	}
}

void AViewCamCapture::BeginPlay()
{
	Super::BeginPlay();
	RebuildRenderTarget();
}

void AViewCamCapture::RebuildRenderTarget()
{
	// Match the panel's aspect so the mirrored view is letterboxed the way the panel is,
	// rather than squashed into a square target and stretched back out by the UImage.
	const int32 Longest = FMath::Clamp(CVarViewCamRes.GetValueOnGameThread(), 64, 4096);
	const int32 W = PanelAspect >= 1.f ? Longest : FMath::Max(64, FMath::RoundToInt(Longest * PanelAspect));
	const int32 H = PanelAspect >= 1.f ? FMath::Max(64, FMath::RoundToInt(Longest / PanelAspect)) : Longest;
	if (W == BuiltWidth && H == BuiltHeight && RenderTarget)
	{
		return;
	}

	BuiltWidth = W;
	BuiltHeight = H;
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->InitAutoFormat(W, H);
	RenderTarget->UpdateResourceImmediate(true);
	Capture->TextureTarget = RenderTarget;
}

void AViewCamCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Capture)
	{
		return;
	}

	// Resolution is a live dial; pick up a change without needing a restart.
	const int32 Longest = FMath::Clamp(CVarViewCamRes.GetValueOnGameThread(), 64, 4096);
	const int32 WantLongest = FMath::Max(BuiltWidth, BuiltHeight);
	if (Longest != WantLongest)
	{
		RebuildRenderTarget();
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (CVarViewCamMode.GetValueOnGameThread() != 0)
	{
		// MINIMAP: the main view's own top-down axis, pulled back to its own zoom. Centred on
		// the bearer (the attractor he publishes), so the map is always "the ground around the
		// light" — which is the only ground that matters in a game about carrying it.
		const USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>();
		const FVector Centre = Swarm ? Swarm->GetAttractor() : FVector::ZeroVector;
		const float Altitude = FMath::Max(CVarViewCamMapHeight.GetValueOnGameThread(), 100.f);

		Capture->SetWorldLocationAndRotation(
			FVector(Centre.X, Centre.Y, Centre.Z + Altitude),
			FRotator(-90.f, 0.f, 0.f)); // straight down, matching the main camera's axis
		Capture->ProjectionType = ECameraProjectionMode::Orthographic;
		Capture->OrthoWidth = FMath::Max(CVarViewCamMapWidth.GetValueOnGameThread(), 100.f);
	}
	else
	{
		// MIRROR: same POV, same projection as the player camera. Duplicates the screen behind
		// the panel, so this is a debug/compare mode rather than a useful second view.
		const APlayerController* PC = World->GetFirstPlayerController();
		const APlayerCameraManager* PCM = PC ? PC->PlayerCameraManager : nullptr;
		if (!PCM)
		{
			return;
		}
		const FMinimalViewInfo& POV = PCM->GetCameraCacheView();
		Capture->SetWorldLocationAndRotation(POV.Location, POV.Rotation);
		Capture->ProjectionType = POV.ProjectionMode;
		Capture->FOVAngle = POV.FOV;
		Capture->OrthoWidth = POV.OrthoWidth;
	}

	const int32 Rate = FMath::Max(CVarViewCamRate.GetValueOnGameThread(), 1);
	if (++FrameCounter >= Rate)
	{
		FrameCounter = 0;
		Capture->CaptureScene();
	}
}
