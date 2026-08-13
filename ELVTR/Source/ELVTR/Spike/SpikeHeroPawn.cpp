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
		TEXT("Kindled.Cam.HudBias"), 1.f,
		TEXT("How far the game camera slides to compensate for the combat HUD covering the\n")
		TEXT("bottom of the screen, as a multiple of the exact correction. 1 = the hero sits\n")
		TEXT("dead centre of the strip you can actually see (default). 0 = off, hero centred in\n")
		TEXT("the full viewport and therefore crowded up against the top by the HUD. Values above\n")
		TEXT("1 overshoot, pushing him further up into open ground ahead of the fight."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamHudBiasLerp(
		TEXT("Kindled.Cam.HudBiasLerp"), 6.f,
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
		TEXT("Kindled.Cam.Ortho"), 1,
		TEXT("1 = orthographic (the shipped look: no perspective convergence, so the horde reads\n")
		TEXT("as a flat tactical field). 0 = perspective, which brings back foreshortening and\n")
		TEXT("makes Fov and Dist meaningful as separate dials."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOrthoWidth(
		TEXT("Kindled.Cam.OrthoWidth"), 2400.f,
		TEXT("Ortho view WIDTH in world units — the zoom dial while Ortho is 1. This is the\n")
		TEXT("framing number to compare against the retinue bbox in the spacing report when\n")
		TEXT("deciding whether the army is too small or the camera is too wide."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamFov(
		TEXT("Kindled.Cam.Fov"), 60.f,
		TEXT("Horizontal field of view in degrees, used only while Ortho is 0."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamDist(
		TEXT("Kindled.Cam.Dist"), 1200.f,
		TEXT("How far the camera sits from the hero along its own view axis, in uu. At the\n")
		TEXT("default Pitch of -90 this is literally height above him. Under ortho it does NOT\n")
		TEXT("zoom (that is OrthoWidth) — it only moves the near/far planes through the world."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamPitch(
		TEXT("Kindled.Cam.Pitch"), -90.f,
		TEXT("Camera pitch in degrees. -90 = straight down (default). Raising it toward about\n")
		TEXT("-50 gives an angled RTS shot — note the units are camera-facing billboards, so a\n")
		TEXT("shallow pitch is what makes their facing artwork read at all."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamYaw(
		TEXT("Kindled.Cam.Yaw"), 0.f,
		TEXT("Orbit the whole view around the hero, in degrees. At Pitch -90 this spins the map\n")
		TEXT("under you. See Kindled.Cam.YawInput — without it WASD keeps pushing along world\n")
		TEXT("axes and stops matching the screen as soon as this leaves 0."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamYawInput(
		TEXT("Kindled.Cam.YawInput"), 1,
		TEXT("1 = rotate WASD by the camera yaw so 'W' always means up-screen (default). 0 = raw\n")
		TEXT("world-axis movement, which is what the spike always did and is only correct while\n")
		TEXT("Yaw is 0. Turn this off only if you want to feel the world axes directly."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOffsetX(
		TEXT("Kindled.Cam.OffsetX"), 0.f,
		TEXT("World-space X offset applied to what the camera centres on. Shifts the framing\n")
		TEXT("without moving the hero — lead room ahead of the fight, for instance."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOffsetY(
		TEXT("Kindled.Cam.OffsetY"), 0.f,
		TEXT("World-space Y offset applied to the camera's focus point."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamOffsetZ(
		TEXT("Kindled.Cam.OffsetZ"), 0.f,
		TEXT("World-space Z offset applied to the camera's focus point. Useful to aim above the\n")
		TEXT("hero's feet once Pitch is shallow enough for ground level to matter."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamLerp(
		TEXT("Kindled.Cam.Lerp"), 0.f,
		TEXT("Smoothing on camera moves (FInterpTo speed), so dragging a dial glides instead of\n")
		TEXT("teleporting. 0 = snap, which is the honest setting while tuning: smoothing hides\n")
		TEXT("how far a value actually moved the shot."),
		ECVF_Default);

	// --- army-scale camera (docs/design/CAMERA-SCALE.md) -------------------------
	// "The camera tells you whether you are an army or a man." One scalar — how much army
	// you still have — drives width, pitch, distance and projection together. While Scale is
	// 0 every dial below is inert and the camera behaves exactly as it always has.

	TAutoConsoleVariable<int32> CVarCamScale(
		TEXT("Kindled.Cam.Scale"), 0,
		TEXT("Which driver decides how zoomed out the shot is.\n")
		TEXT("  0 = off (default). The Cam.OrthoWidth / Pitch / Dist / Ortho dials drive the shot\n")
		TEXT("      by hand — this is also the CLOSE shot: set OrthoWidth small and go in.\n")
		TEXT("  1 = army-weighted. Width follows how much army you have left (ScaleRetinueWeight,\n")
		TEXT("      ScaleBroodWeight, ScaleBodies). A proxy for where the bodies are, not a\n")
		TEXT("      measurement of it, so it CANNOT promise anything stays on screen.\n")
		TEXT("  2 = STRATEGIC. Mode 1's scalar becomes a FLOOR, and the shot additionally widens\n")
		TEXT("      until the live retinue bounding box actually fits (Cam.FitWidthMax,\n")
		TEXT("      Cam.FitMargin). Your units are contained and stay small; the brood is left to\n")
		TEXT("      run off every edge on purpose — a frame wide enough to hold the horde turns a\n")
		TEXT("      soldier into a smudge.\n")
		TEXT("While 1 or 2, this OVERRIDES the four hand dials — they stop responding, by design."),
		ECVF_Default);

	// Weights deliberately mirror Kindled.UnitCamProj.Size* — the Unit Cam already solved
	// "how much army is this" and the two views must not disagree about it.
	TAutoConsoleVariable<float> CVarCamScaleRetinueWeight(
		TEXT("Kindled.Cam.ScaleRetinueWeight"), 10.f,
		TEXT("Weight of one live retinue soldier in the army-scale total. Heavily outweighs\n")
		TEXT("ScaleBroodWeight on purpose: attrition of YOUR army is what the shot is about."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleBroodWeight(
		TEXT("Kindled.Cam.ScaleBroodWeight"), 0.25f,
		TEXT("Weight of one live brood in the army-scale total. Small but non-zero, so being\n")
		TEXT("swarmed widens the shot a little — the tide is part of the picture."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleBodies(
		TEXT("Kindled.Cam.ScaleBodies"), 1200.f,
		TEXT("WEIGHTED body count at which the camera is fully at its 'army' end. Not a raw\n")
		TEXT("headcount — see ScaleRetinueWeight / ScaleBroodWeight.\n")
		TEXT("Calibrated to RetinueCap * ScaleRetinueWeight (120 * 10), so a FULL RETINUE ON ITS\n")
		TEXT("OWN pins the scalar at 1 and you get the true top-down map — exactly the shipped\n")
		TEXT("framing. Brood then only holds you at the wide end, it can never push past it.\n")
		TEXT("Raise this above the retinue's own weight and full strength no longer reaches the\n")
		TEXT("top of its own axis: the map view becomes unreachable in normal play."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleCurve(
		TEXT("Kindled.Cam.ScaleCurve"), 1.f,
		TEXT("Shaping exponent on the 0..1 army scalar. 1 = linear. >1 holds the wide shot\n")
		TEXT("longer and collapses late (attrition feels survivable, then sudden); <1 descends\n")
		TEXT("early and eases out."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleWidthFull(
		TEXT("Kindled.Cam.ScaleWidthFull"), 2400.f,
		TEXT("World units across the view at full army. Matches the shipped OrthoWidth, so a\n")
		TEXT("full-strength run looks exactly like today."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleWidthAlone(
		TEXT("Kindled.Cam.ScaleWidthAlone"), 700.f,
		TEXT("World units across the view with nobody left — the character-camera end."),
		ECVF_Default);

	// --- strategic fit (Cam.Scale 2 only) ---------------------------------------
	// ScaleWidthFull is calibrated to look like the shipped shot, which makes it the wrong
	// CEILING: a retinue spread wider than 2400uu would silently clip against it and the mode
	// would quietly fail at the one thing it promises. Strategic swaps in its own far end.
	TAutoConsoleVariable<float> CVarCamFitWidthMax(
		TEXT("Kindled.Cam.FitWidthMax"), 7000.f,
		TEXT("Widest the STRATEGIC shot may open to, in world units. This is a hard clamp, not a\n")
		TEXT("target: a view that can grow without bound turns soldiers into pixels. Replaces\n")
		TEXT("ScaleWidthFull as the far end of the width lerp while Cam.Scale is 2 — pitch and\n")
		TEXT("distance still travel between their Alone/Full poles.\n")
		TEXT("7000 comes from the arena the brood arrives across at BroodSpawnRadiusMax 4000; at\n")
		TEXT("1920px wide that leaves a 48px soldier drawing ~13px, which still reads."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamFitLog(
		TEXT("Kindled.Cam.FitLog"), 0,
		TEXT("1 = log what the strategic solve decided, roughly once a second: the measured\n")
		TEXT("retinue extents, the width they demand, and the width the shot is actually at.\n")
		TEXT("Off by default. This exists because the solve has NO other observable — PIE\n")
		TEXT("screenshots come back as the editor viewport, so a picture cannot confirm the\n")
		TEXT("numbers. Turn it on to tune FitMargin/FitWidthMax against a real fight."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamFitMargin(
		TEXT("Kindled.Cam.FitMargin"), 1.15f,
		TEXT("Slack multiplier on the solved strategic width. 1.0 fits the bounding box exactly,\n")
		TEXT("which puts the outermost soldiers half-off the screen edge — the box bounds their\n")
		TEXT("CENTRES, and it is a frame behind. Below ~1.05 the edge of the line will clip."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamFitFloor(
		// 0 since 2026-08-04 (owner call: "the units can be tighter together"). Was 1: the
		// weighted body count floored the strategic scalar, so a full retinue pinned the shot
		// at ~FitWidthMax even when the line measured ~2100uu wide — containment held, but the
		// army read as specks on black ground. The measurement alone is the floor now.
		TEXT("Kindled.Cam.FitFloor"), 0.f,
		TEXT("How much of the weighted body-count scalar still floors the STRATEGIC shot, 0..1.\n")
		TEXT("0 = the width is solved from the measured retinue bbox alone (default): a tight\n")
		TEXT("huddle gets a tight, low, close shot however many bodies are in it, and the frame\n")
		TEXT("only opens when the line actually spreads. 1 = the old behaviour: full retinue\n")
		TEXT("pins the width at FitWidthMax regardless of spread. Values between blend.\n")
		TEXT("Note pitch and distance travel with the same scalar, so 0 also means a huddle\n")
		TEXT("gets the shallow, near end of the shot."),
		ECVF_Default);

	// --- movement lead --------------------------------------------------------
	// "Moves with you as you move": the focus runs a little ahead of the hero along his
	// velocity, so the frame shows where you are GOING instead of centering where you ARE.
	// Applies in every mode — it is folded into the focus point, which all paths share.
	TAutoConsoleVariable<float> CVarCamLead(
		TEXT("Kindled.Cam.Lead"), 0.35f,
		TEXT("Seconds of hero velocity to lead the camera focus by. 0.35 means the focus sits\n")
		TEXT("about a third of a second of travel ahead of the hero while he moves. 0 = off,\n")
		TEXT("the focus stays on the hero."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamLeadMax(
		TEXT("Kindled.Cam.LeadMax"), 400.f,
		TEXT("Cap on the movement-lead offset in uu, so a fast hero cannot push himself to the\n")
		TEXT("screen edge. The lead is a nudge, not a pan."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamLeadLerp(
		TEXT("Kindled.Cam.LeadLerp"), 5.f,
		TEXT("Easing speed on the lead offset (VInterpTo). WASD velocity starts and stops\n")
		TEXT("instantly, so the raw lead would jump; this glides it in and out. 0 = snap."),
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
		TEXT("Kindled.Cam.ScalePitchFull"), -55.f,
		TEXT("Camera pitch at full army. -90 = straight down (the old battlefield map), -55 =\n")
		TEXT("tilted so the arena reads as ground you look ACROSS rather than down at.\n")
		TEXT("Shallower sees further down-field — raise Swarm.SimLOD.NearRadius to match, or the\n")
		TEXT("LOD will begin striding units that are on screen. Ground depth is roughly\n")
		TEXT("(OrthoWidth / aspect) / sin(pitch)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScalePitchAlone(
		TEXT("Kindled.Cam.ScalePitchAlone"), -22.f,
		TEXT("Camera pitch with nobody left. Shallow enough to read as standing behind a person\n")
		TEXT("rather than looking down at a token."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleDistFull(
		TEXT("Kindled.Cam.ScaleDistFull"), 1200.f,
		TEXT("Camera distance back along the view axis at full army."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleDistAlone(
		TEXT("Kindled.Cam.ScaleDistAlone"), 420.f,
		TEXT("Camera distance with nobody left. Short enough that perspective actually bites."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleSwapAt(
		TEXT("Kindled.Cam.ScaleSwapAt"), 0.35f,
		TEXT("Army scalar (0..1) below which the projection switches to perspective. You cannot\n")
		TEXT("lerp an ortho matrix into a perspective one, so this is a HARD swap — but the FOV\n")
		TEXT("is solved from the same framing width, so the frame is identical across the cut and\n")
		TEXT("only parallax changes. Set 0 to stay orthographic for the whole run."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamScaleStages(
		TEXT("Kindled.Cam.ScaleStages"), 0,
		TEXT("0 = continuous (the camera is always subtly moving). N > 0 quantises the army\n")
		TEXT("scalar into N steps, so the shot SETTLES and only re-frames when a step is\n")
		TEXT("crossed. This is CAMERA-SCALE.md's open question 2 as a live dial — try 4 or 5\n")
		TEXT("against 0 and judge, rather than deciding it on paper."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCamScaleRatchet(
		TEXT("Kindled.Cam.ScaleRatchet"), 0,
		TEXT("1 = one-way: the camera descends as the army dies and never rises again, even if a\n")
		TEXT("breather refills the retinue. 0 = reversible. CAMERA-SCALE.md's open question 3 —\n")
		TEXT("one-way is the stronger dramatic statement and the worse feedback loop. Resets each\n")
		TEXT("time the camera is (re)placed, i.e. per run."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleLerp(
		TEXT("Kindled.Cam.ScaleLerp"), 1.5f,
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
		TEXT("Kindled.Cam.ScaleStiffness"), 3.f,
		TEXT("Spring stiffness (natural frequency, rad/s) pulling the army scalar toward its\n")
		TEXT("target. Higher = the camera commits to a new scale sooner. 0 disables the spring\n")
		TEXT("entirely and falls back to Cam.ScaleLerp's plain FInterpTo.\n")
		TEXT("Roughly: settle time to ~5%% is about 3/Stiffness seconds at Damping 1, so the\n")
		TEXT("default 3 settles in ~1s."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarCamScaleDamping(
		TEXT("Kindled.Cam.ScaleDamping"), 1.f,
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
	double GCamFitLogTime = 0.0; // world seconds of the last Cam.FitLog line

	// Movement-lead state, file-static for the same Live Coding reason as the army scalar.
	FVector GPrevHeroLoc = FVector::ZeroVector;
	FVector GCamLead = FVector::ZeroVector;

	// Edge-detect state for the numpad camera-mode keys. File-static rather than pawn members
	// on purpose: adding a member to ASpikeHeroPawn is a class-layout change, which cannot go
	// in over Live Coding and costs an editor-closed rebuild — and this is a tuning hotkey.
	// ponytail: one shared set of flags, fine while there is exactly one local player pawn;
	// move them onto the pawn if split-screen ever lands.
	bool GWasDownCamHand = false;
	bool GWasDownCamArmy = false;
	bool GWasDownCamStrategic = false;
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

	// --- camera modes on the numpad --------------------------------------
	// Deliberately the NUMPAD and not the number row: 1-4 up there are the stance orders and
	// are worth muscle memory, while these are a view toggle you flick during a fight.
	// They write the CVar rather than shadowing it in a second piece of state, so the console,
	// SwarmExecOnPlay.txt and these keys can never disagree about which mode is live.
	auto SetCamMode = [](int32 Mode)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Kindled.Cam.Scale")))
		{
			CVar->Set(Mode, ECVF_SetByConsole);
		}
	};
	if (ConsumeKeyPress(PC, EKeys::NumPadOne, GWasDownCamHand))
	{
		SetCamMode(0);	// hand dials — the close shot
	}
	else if (ConsumeKeyPress(PC, EKeys::NumPadTwo, GWasDownCamArmy))
	{
		SetCamMode(1);	// army-weighted
	}
	else if (ConsumeKeyPress(PC, EKeys::NumPadThree, GWasDownCamStrategic))
	{
		SetCamMode(2);	// strategic — fits your army, lets the horde overflow
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

	// Fetched once: the strategic fit and the HUD bias both need it, and it is a virtual call
	// into the viewport either way.
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	const bool bHaveViewport = ViewportSize.X > 1.0 && ViewportSize.Y > 1.0;

	// --- army scale ---------------------------------------------------------
	// One scalar, 0 (alone) .. 1 (full army), drives width, pitch, distance and projection
	// together so they can never disagree about what the shot is saying.
	const int32 ScaleMode = CVarCamScale.GetValueOnGameThread();
	const bool bScale = ScaleMode != 0;
	const bool bStrategic = ScaleMode >= 2;

	// Pawn-relative nudge that re-centres the shot on the retinue in strategic mode. Stays
	// zero everywhere else, so it can be added unconditionally at the focus below.
	FVector FitFocus = FVector::ZeroVector;

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
			GCamFitLogTime = 0.0; // world time restarts each run; the throttle must not straddle it
		}
		GCamArmyScaleLow = FMath::Min(GCamArmyScaleLow, Army);
		float TargetArmy = CVarCamScaleRatchet.GetValueOnGameThread() != 0
			? GCamArmyScaleLow : Army;

		// --- strategic fit (Cam.Scale 2) -------------------------------------
		// Everything above is a PROXY for where the bodies are. This measures where they
		// actually are and opens the shot until they fit. Deliberately applied AFTER stages
		// and ratchet: containment is a guarantee, and those two are stylistic dials that
		// would otherwise quantise it away or pin the shot narrow while the line spreads.
		// The weighted scalar only floors the fit as far as Cam.FitFloor lets it — at the
		// default 0 the measurement alone sets the width, so a tight huddle gets a tight
		// shot instead of a full-army-width frame around specks.
		if (bStrategic && Swarm)
		{
			const FBox& Bounds = Swarm->GetRetinueBounds();
			if (Bounds.IsValid != 0)
			{
				// Extents in CAMERA-YAW space, not world: the shot is yawed, so a world-axis
				// span measures the wrong rectangle. Four ground corners is exact for a yaw
				// of an AABB — the AABB itself is the only conservative step, and erring wide
				// is the safe direction here.
				const double YawRad = FMath::DegreesToRadians((double)CVarCamYaw.GetValueOnGameThread());
				const FVector2D Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad));
				const FVector2D Right(-Fwd.Y, Fwd.X);

				double FwdMin = 0.0, FwdMax = 0.0, RightMin = 0.0, RightMax = 0.0;
				for (int32 Corner = 0; Corner < 4; ++Corner)
				{
					const FVector2D P(
						(Corner & 1) ? Bounds.Max.X : Bounds.Min.X,
						(Corner & 2) ? Bounds.Max.Y : Bounds.Min.Y);
					const double F = FVector2D::DotProduct(P, Fwd);
					const double R = FVector2D::DotProduct(P, Right);
					if (Corner == 0)
					{
						FwdMin = FwdMax = F;
						RightMin = RightMax = R;
						continue;
					}
					FwdMin = FMath::Min(FwdMin, F); FwdMax = FMath::Max(FwdMax, F);
					RightMin = FMath::Min(RightMin, R); RightMax = FMath::Max(RightMax, R);
				}

				// This frame's pitch is not solved yet — it comes out of the very scalar being
				// computed. Use last frame's smoothed one: the shot is spring-damped anyway, so
				// a frame of lag on the depth term is invisible. Same argument the alive counts
				// already make for themselves one file over.
				const float PitchNow = FMath::Lerp(
					CVarCamScalePitchAlone.GetValueOnGameThread(),
					CVarCamScalePitchFull.GetValueOnGameThread(),
					FMath::Clamp(GCamArmyScale, 0.f, 1.f));
				// Ground depth visible at width W is (W / aspect) / sin(pitch) — invert it for
				// the width a depth span needs. Floored so a near-horizontal pitch cannot send
				// the solve to infinity.
				const float SinPitch = FMath::Max(
					FMath::Abs(FMath::Sin(FMath::DegreesToRadians(PitchNow))), 0.05f);
				const float AspectX = bHaveViewport ? (float)(ViewportSize.X / ViewportSize.Y) : 1.7778f;

				// BOTH axes, take the max. Screen width is the ortho width outright; screen
				// depth is foreshortened by the pitch. Solving depth alone under-frames badly.
				const float Needed =
					FMath::Max((float)(RightMax - RightMin), (float)(FwdMax - FwdMin) * AspectX * SinPitch)
					* FMath::Max(CVarCamFitMargin.GetValueOnGameThread(), 1.f);

				const float FitAlone = CVarCamScaleWidthAlone.GetValueOnGameThread();
				const float FitMax = FMath::Max(CVarCamFitWidthMax.GetValueOnGameThread(), FitAlone + 1.f);
				// FitFloor scales how much the weighted count still gets to say: 0 hands
				// the width to the measurement outright, 1 is the old "full army = full
				// width" floor.
				TargetArmy = FMath::Max(
					TargetArmy * FMath::Clamp(CVarCamFitFloor.GetValueOnGameThread(), 0.f, 1.f),
					FMath::Clamp((Needed - FitAlone) / (FitMax - FitAlone), 0.f, 1.f));

				// Centre on the box, not on the hero: a width that fits a box the view is not
				// centred on still does not contain it. This is also the forward lead the shot
				// needs — the brood arrives from the front, so the line sits ahead of you — and
				// it is SOLVED from where the army is rather than a hand-typed guess. Folded on
				// top of Cam.OffsetX/Y rather than replacing them, so those stay as trim.
				const FVector Centre = Bounds.GetCenter();
				const FVector Loc = GetActorLocation();
				FitFocus = FVector(Centre.X - Loc.X, Centre.Y - Loc.Y, 0.f);

				if (CVarCamFitLog.GetValueOnGameThread() != 0)
				{
					// Throttled off world time rather than a frame counter, so the cadence does
					// not change with frame rate while you are tuning.
					const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
					if (Now - GCamFitLogTime >= 1.0)
					{
						GCamFitLogTime = Now;
						UE_LOG(LogTemp, Display,
							TEXT("CamFit: retinue extents lateral=%.0f depth=%.0f uu  needs=%.0f  at=%.0f  (A=%.3f pitch=%.1f aspect=%.3f)"),
							(float)(RightMax - RightMin), (float)(FwdMax - FwdMin), Needed,
							FMath::Lerp(FitAlone, FitMax, FMath::Clamp(GCamArmyScale, 0.f, 1.f)),
							GCamArmyScale, PitchNow, AspectX);
					}
				}
			}
		}

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
		// Strategic swaps the FAR end of the width lerp only. ScaleWidthFull is calibrated to
		// look like the shipped shot, which makes it a ceiling the fit would silently clip
		// against; pitch and distance still travel between their own Alone/Full poles, so the
		// two modes keep saying the same thing about how the shot is angled.
		Width = FMath::Lerp(
			CVarCamScaleWidthAlone.GetValueOnGameThread(),
			bStrategic ? CVarCamFitWidthMax.GetValueOnGameThread()
					   : CVarCamScaleWidthFull.GetValueOnGameThread(), A);
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

	// --- movement lead --------------------------------------------------------
	// Velocity from position delta — the pawn moves by AddActorWorldOffset, so there is
	// no movement component to ask. Eased in and out because WASD speed is a step
	// function; a raw lead would jump the frame on every key press and release.
	const FVector HeroLoc = GetActorLocation();
	if (!bCameraPlaced)
	{
		GPrevHeroLoc = HeroLoc;
		GCamLead = FVector::ZeroVector;
	}
	FVector TargetLead = FVector::ZeroVector;
	const float LeadSecs = CVarCamLead.GetValueOnGameThread();
	if (LeadSecs > 0.f && DeltaSeconds > KINDA_SMALL_NUMBER)
	{
		const FVector Velocity = (HeroLoc - GPrevHeroLoc) / DeltaSeconds;
		TargetLead = Velocity * LeadSecs;
		const float LeadMax = FMath::Max(CVarCamLeadMax.GetValueOnGameThread(), 0.f);
		if (TargetLead.SizeSquared() > LeadMax * LeadMax)
		{
			TargetLead = TargetLead.GetSafeNormal() * LeadMax;
		}
	}
	GPrevHeroLoc = HeroLoc;
	const float LeadSpeed = FMath::Max(CVarCamLeadLerp.GetValueOnGameThread(), 0.f);
	GCamLead = LeadSpeed > 0.f
		? FMath::VInterpTo(GCamLead, TargetLead, DeltaSeconds, LeadSpeed)
		: TargetLead;

	const FVector Focus = FVector(
		CVarCamOffsetX.GetValueOnGameThread(),
		CVarCamOffsetY.GetValueOnGameThread(),
		CVarCamOffsetZ.GetValueOnGameThread()) + FitFocus + GCamLead;

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
			if (bHaveViewport)
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

	GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Silver,
		FString::Printf(TEXT("CAMERA  [Num1] close  [Num2] army  [Num3] strategic      (mode %d)"),
			IConsoleManager::Get().FindConsoleVariable(TEXT("Kindled.Cam.Scale"))
				? IConsoleManager::Get().FindConsoleVariable(TEXT("Kindled.Cam.Scale"))->GetInt()
				: 0));
}
