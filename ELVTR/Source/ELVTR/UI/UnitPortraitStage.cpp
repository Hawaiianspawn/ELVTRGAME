#include "UI/UnitPortraitStage.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarUnitCamDebug(
	TEXT("Emberkeep.UI.UnitCamDebug"), 1,
	TEXT("Draw red debug markers for the unit close-up cam: stand-in location, capture frustum, aim line. 1=on (default), 0=off."),
	ECVF_Default);

// --- behind-the-unit framing (owner 2026-07-23) --------------------------------
// The unit cam sits BEHIND the focused unit and looks forward (local +X) into the
// dark, so friendly units read in the foreground and the brood are seen emerging
// from the black ahead. The hero pawn never rotates (translation-only movement),
// so local +X is a fixed world direction; aiming toward the live enemy front is a
// later pass. All tunable live via these CVars — no rebuild to re-frame.
static TAutoConsoleVariable<float> CVarUnitCamSubjectFwd(
	TEXT("Emberkeep.UI.UnitCam.SubjectFwd"), 250.f,
	TEXT("How far ahead of the unit (local +X, uu) the capture-only stand-in sits."),
	ECVF_Default);
static TAutoConsoleVariable<float> CVarUnitCamDist(
	TEXT("Emberkeep.UI.UnitCam.Dist"), 320.f,
	TEXT("How far BEHIND the stand-in the camera sits (uu). Larger = more foreground unit."),
	ECVF_Default);
static TAutoConsoleVariable<float> CVarUnitCamHeight(
	TEXT("Emberkeep.UI.UnitCam.Height"), 150.f,
	TEXT("Camera height above the unit (uu)."),
	ECVF_Default);
static TAutoConsoleVariable<float> CVarUnitCamPitch(
	TEXT("Emberkeep.UI.UnitCam.Pitch"), -12.f,
	TEXT("Camera look-down pitch in degrees (negative looks down)."),
	ECVF_Default);
static TAutoConsoleVariable<float> CVarUnitCamSide(
	TEXT("Emberkeep.UI.UnitCam.Side"), 0.f,
	TEXT("Over-the-shoulder side offset (uu). 0 = directly behind."),
	ECVF_Default);
static TAutoConsoleVariable<float> CVarUnitCamFOV(
	TEXT("Emberkeep.UI.UnitCam.FOV"), 55.f,
	TEXT("Capture field of view in degrees."),
	ECVF_Default);

AUnitPortraitStage::AUnitPortraitStage()
{
	PrimaryActorTick.bCanEverTick = true; // for the debug draw

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Higher-detail stand-in for the focused unit, placed out in front (in the fight).
	// Rendered ONLY in scene captures (invisible in the stylised main view) — the detail-swap.
	// Placeholder mesh for now; swap for a detailed unit sprite/mesh with combat anims later.
	Subject = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Subject"));
	Subject->SetupAttachment(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MeshFinder.Succeeded())
	{
		Subject->SetStaticMesh(MeshFinder.Object);
	}
	Subject->SetRelativeLocation(FVector(250.f, 0.f, 60.f));
	Subject->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.4f));
	Subject->SetCastShadow(false);
	Subject->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Subject->bVisibleInSceneCaptureOnly = true; // appears only inside the capture, not the main game

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	// Behind-and-above the stand-in, looking forward at it with the battle beyond.
	Capture->SetRelativeLocation(FVector(70.f, 90.f, 150.f));
	Capture->SetRelativeRotation(FRotator(-16.f, -28.f, 0.f));
	Capture->FOVAngle = 40.f;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR; // demichrome PPV -> looks like the game
	Capture->bCaptureEveryFrame = true;  // the battle is live
	Capture->bCaptureOnMovement = false;
	// Default PrimitiveRenderMode renders the whole scene, so the battlefield is visible,
	// with the capture-only stand-in composited in.
}

void AUnitPortraitStage::BeginPlay()
{
	Super::BeginPlay();

	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->InitAutoFormat(Resolution, Resolution);
	RenderTarget->UpdateResourceImmediate(true);
	Capture->TextureTarget = RenderTarget;
}

void AUnitPortraitStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Re-frame every tick from the CVars so the shot can be tuned live (no rebuild).
	if (Subject && Capture)
	{
		const float SubjFwd = CVarUnitCamSubjectFwd.GetValueOnGameThread();
		const float Dist = CVarUnitCamDist.GetValueOnGameThread();
		const float Height = CVarUnitCamHeight.GetValueOnGameThread();
		const float Pitch = CVarUnitCamPitch.GetValueOnGameThread();
		const float Side = CVarUnitCamSide.GetValueOnGameThread();

		Subject->SetRelativeLocation(FVector(SubjFwd, 0.f, 60.f));
		// Behind the stand-in along local -X, raised, looking forward (+X) and down.
		Capture->SetRelativeLocation(FVector(SubjFwd - Dist, Side, Height));
		Capture->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
		Capture->FOVAngle = CVarUnitCamFOV.GetValueOnGameThread();
	}

	if (CVarUnitCamDebug.GetValueOnGameThread() == 0 || !Subject || !Capture)
	{
		return;
	}

	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	// The stand-in is invisible in the main view (bVisibleInSceneCaptureOnly), so draw where
	// it actually is, and where the capture is looking, in red — all visible in the main viewport.
	const FVector SubLoc = Subject->Bounds.Origin;
	const FVector SubExt = Subject->Bounds.BoxExtent;
	const FVector CapLoc = Capture->GetComponentLocation();
	const FRotator CapRot = Capture->GetComponentRotation();

	DrawDebugBox(W, SubLoc, SubExt, FColor::Red, false, -1.f, 0, 2.f);
	DrawDebugSphere(W, SubLoc, 24.f, 10, FColor::Red, false, -1.f, 0, 1.5f);
	DrawDebugString(W, SubLoc + FVector(0.f, 0.f, SubExt.Z + 30.f), TEXT("UNIT STAND-IN"),
		nullptr, FColor::Red, 0.f, true);
	DrawDebugCamera(W, CapLoc, CapRot, Capture->FOVAngle, 1.f, FColor::Red);
	DrawDebugLine(W, CapLoc, SubLoc, FColor::Red, false, -1.f, 0, 1.f);
}

UPrimitiveComponent* AUnitPortraitStage::GetSubjectComponent() const
{
	return Subject;
}
