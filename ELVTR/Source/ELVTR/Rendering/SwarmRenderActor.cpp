#include "SwarmRenderActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmDebug.h"
#include "Materials/MaterialParameterCollection.h"
#include "Mass/SwarmFragments.h"
#include "Mass/SwarmStats.h"
#include "Mass/SwarmSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "RenderTimer.h"
#include "RHI.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * Emberkeep.Cam.Yaw, by name.
	 *
	 * It is owned by ASpikeHeroPawn's translation unit, and duplicating the
	 * TAutoConsoleVariable here would register the same name twice. Looked up once and
	 * cached — the console manager hands back a stable pointer for the process lifetime,
	 * and a null (pawn module not loaded, e.g. a commandlet) just means yaw 0, which is
	 * the default anyway.
	 */
	FORCEINLINE float GetCameraYawDegrees()
	{
		static IConsoleVariable* CamYaw =
			IConsoleManager::Get().FindConsoleVariable(TEXT("Emberkeep.Cam.Yaw"));
		return CamYaw ? CamYaw->GetFloat() : 0.f;
	}

	/**
	 * World units across the live view at a given world-space focus point's depth, or 0.f
	 * if it can't be measured (no viewport, no camera yet). Ortho spans OrthoWidth outright;
	 * perspective spans 2*Dist*tan(FOV/2) at the focus point's distance from the camera.
	 *
	 * Shared rather than duplicated on purpose: Swarm.DitherZoomCompensate and
	 * Swarm.FlameScaleWithView both need this exact "how wide is the screen, in world
	 * units, at this depth" measurement, and SpikeHeroPawn::TickCamera's HUD-bias extent
	 * derives the same quantity a third time for its own purpose (there, at the camera's
	 * own focus point rather than an arbitrary world position). Do not add a fourth copy —
	 * if a caller needs this from outside this translation unit, promote it instead.
	 */
	float GetLiveViewWidthUU(UWorld* World, const FVector& FocusPoint, FVector2D& OutViewportSize)
	{
		OutViewportSize = FVector2D::ZeroVector;
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(OutViewportSize);
		}
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const APlayerCameraManager* CamMgr = PC ? PC->PlayerCameraManager : nullptr;
		if (!CamMgr || OutViewportSize.X <= 1.0)
		{
			return 0.f;
		}

		const FMinimalViewInfo& View = CamMgr->GetCameraCacheView();
		if (View.ProjectionMode == ECameraProjectionMode::Perspective)
		{
			const float Dist = FMath::Max((float)FVector::Dist(View.Location, FocusPoint), 1.f);
			return 2.f * Dist * FMath::Tan(FMath::DegreesToRadians(View.FOV) * 0.5f);
		}
		return View.OrthoWidth;
	}

	TAutoConsoleVariable<int32> CVarSwarmDebugRender(
		TEXT("Swarm.DebugRender"),
		1,
		TEXT("Render the swarm as mesh instances instead of Niagara sprites.\n")
		TEXT("1 = debug meshes (default while the sprite pipeline is unverified), 0 = Niagara."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarSwarmDebugPlainView(
		TEXT("Swarm.DebugPlainView"),
		0,
		TEXT("Opt-in: disable post-process materials (drops the demichrome dither) for an\n")
		TEXT("unfiltered read of the debug view. Default 0 keeps the game's look intact."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDebugShot(
		TEXT("Swarm.DebugShotAfter"),
		0.f,
		TEXT("Take one screenshot this many seconds after BeginPlay. 0 disables.\n")
		TEXT("Lets a headless/scripted run capture what the renderer is actually drawing.\n")
		TEXT("\n")
		TEXT("task-048: this is a SceneCaptureComponent2D capture (DebugCaptureComponent), NOT\n")
		TEXT("the engine's FScreenshotRequest — that path is fulfilled inside the game viewport's\n")
		TEXT("Slate Draw() call, which simply never runs on an unfocused/occluded PIE window, so\n")
		TEXT("the request can sit queued through an entire run and nothing lands on disk (verified\n")
		TEXT("empirically, task-047). The scene capture issues its own render command and needs no\n")
		TEXT("window paint, so it works the same whether or not the PIE window has OS focus.\n")
		TEXT("\n")
		TEXT("If Swarm.DebugRender is 1 (debug-box mode) the shot will show the flame pool but NO\n")
		TEXT("units: DrawDebugSolidBox primitives are not visible to a scene capture (verified,\n")
		TEXT("see docs/AGENT-TEAMS.md capture recipe). Set Swarm.DebugRender 0 (Niagara) before\n")
		TEXT("shooting if the swarm itself needs to be in frame. The log line this prints says\n")
		TEXT("which mode was active so a blank-looking shot is diagnosable after the fact.\n")
		TEXT("\n")
		TEXT("NOT YET VALID FOR JUDGING ART: this capture came back RAW, not the styled game view —\n")
		TEXT("the flame pool showed as blown-out flat facets, not the dithered 4-value demichrome\n")
		TEXT("ramp, despite SCS_FinalColorLDR. Use it for geometry/formation/counts/framing only,\n")
		TEXT("never for palette or dither judgement, until that gap is closed. See docs/AGENT-\n")
		TEXT("TEAMS.md §8 for the two untested candidate causes (capture exposure convergence,\n")
		TEXT("double-gamma on export) before assuming it's architectural."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarSwarmDebugShotWidth(
		TEXT("Swarm.DebugShotWidth"),
		1920,
		TEXT("Render target width, px, for Swarm.DebugShotAfter's capture. Independent of the\n")
		TEXT("actual PIE window/viewport size — task-048's fix for units landing at 8-16px in a\n")
		TEXT("desktop screenshot squeezed into part of the editor. Clamped to [64, 7680]."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarSwarmDebugShotHeight(
		TEXT("Swarm.DebugShotHeight"),
		1080,
		TEXT("Render target height, px, for Swarm.DebugShotAfter's capture. See DebugShotWidth.\n")
		TEXT("Clamped to [64, 4320]."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmSpacingLog(
		TEXT("Swarm.SpacingLogInterval"),
		0.f,
		TEXT("Seconds between automatic nearest-neighbour spacing reports. 0 disables."),
		ECVF_Default);

	// --- the bearer's spotlight (docs/RENDERING-LIGHTING.md §4b) -------------

	TAutoConsoleVariable<int32> CVarSwarmFlame(
		TEXT("Swarm.Flame"),
		1,
		TEXT("Drive MPC_Flame from the hero each tick (the bearer's spotlight).\n")
		TEXT("0 stops writing the collection, which leaves the light frozen wherever it\n")
		TEXT("last was — useful for checking that it really is tracking the hero."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameRadius(
		TEXT("Swarm.FlameRadius"),
		900.f,
		TEXT("How far the flame reaches, in uu — the outer edge of the pool.\n")
		TEXT("DECOUPLED from SwarmLeash::Radius (owner call 2026-07-23). The spec had these\n")
		TEXT("wired together so the edge of the light would read as the leash, but the leash\n")
		TEXT("(2000uu) is wider than the camera can see (~1200uu half-width), so that edge was\n")
		TEXT("never on screen. This is the dial to tune the pool by; expect it to move."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameCoreRadius(
		TEXT("Swarm.FlameCoreRadius"),
		330.f,
		TEXT("Radius in uu of the pure-white focusing core at the flame itself.\n")
		TEXT("The core sits outside the 4-value palette on purpose; its edge is cut with the\n")
		TEXT("same Bayer threshold as the pool so it dissolves rather than ending on a clean\n")
		TEXT("circle. Colour is MPC_Flame's FlameCoreColor (white by default)."),
		ECVF_Default);

	// --- flame pool vs. the army-scale camera (CAMERA-SCALE-HANDOFF.md #1/#4.5) ---
	// FlameRadius/FlameCoreRadius are dialed against the SHIPPED framing (2400uu). At the
	// close end of Emberkeep.Cam.Scale (~700uu) FlameCoreRadius alone (330uu, ~660uu across)
	// exceeds the frame and the whole view sits inside the pure-white core — a genuine
	// blowout, not a tuning miss (evidence: SwarmDebugShot00027.png). Two coherent answers,
	// neither ruled: keep the pool WORLD-FIXED (physically honest, but then the close
	// framing must never be narrower than the pool) or make it SCREEN-PROPORTIONAL (the
	// pool keeps a constant screen fraction at every zoom, at the cost of the light's world
	// reach now depending on army size). Same idiom as Emberkeep.Cam.ScaleStages/Ratchet —
	// both live behind a CVar so the owner judges it live rather than it being baked in.
	TAutoConsoleVariable<int32> CVarSwarmFlameScaleWithView(
		TEXT("Swarm.FlameScaleWithView"),
		0,
		TEXT("0 = world-fixed (default, today's behaviour): FlameRadius/FlameCoreRadius are\n")
		TEXT("literal world-unit values, so the pool's reach never changes but its SCREEN size\n")
		TEXT("grows as Emberkeep.Cam.Scale narrows the view — at the close end this blows the\n")
		TEXT("whole frame out to the white core. 1 = screen-proportional: both radii scale by\n")
		TEXT("the live view width over Swarm.FlameScaleReferenceWidth, so the pool keeps a\n")
		TEXT("constant fraction of the screen at every zoom (the 'carried light, darkness\n")
		TEXT("beyond' read never breaks), at the cost that the light's actual reach in the\n")
		TEXT("world now changes with army size. Undecided which is right — this is the A/B."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameScaleReferenceWidth(
		TEXT("Swarm.FlameScaleReferenceWidth"), 2400.f,
		TEXT("View width in uu at which FlameRadius/FlameCoreRadius render exactly as dialed,\n")
		TEXT("used only while Swarm.FlameScaleWithView is 1. Matches the shipped OrthoWidth /\n")
		TEXT("Emberkeep.Cam.ScaleWidthFull default, so a full-army run looks identical to\n")
		TEXT("today; the radii only start scaling once the camera narrows past this."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameFalloff(
		TEXT("Swarm.FlameFalloff"),
		2.f,
		TEXT("Falloff exponent. 1 = linear; higher makes the dark heavier and the pool\n")
		TEXT("edge arrive more suddenly."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameIntensity(
		TEXT("Swarm.FlameIntensity"),
		0.55f,
		TEXT("Base brightness of the flame before flicker. This is the channel the\n")
		TEXT("upkeep/fuel economy would eventually drive.\n")
		TEXT("Well under 1.0 because the light LIFTS the palette value: at 1.0 the whole\n")
		TEXT("visible area saturates to the brightest value and the falloff ramp disappears.\n")
		TEXT("0.55 specifically because Threshold3 is 0.75 — it keeps the body of the pool\n")
		TEXT("at Demichrome Bone so the pure-white core reads as the focusing point instead\n")
		TEXT("of blending into a field of Pale. Measured, see docs/RENDERING-LIGHTING.md §4b.7."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameFlicker(
		TEXT("Swarm.FlameFlicker"),
		0.06f,
		TEXT("Flicker amplitude, 0-1. Mandatory anti-vignette mechanism (§4b.1): a lens\n")
		TEXT("vignette is perfectly steady, a carried fire is not. 0 disables."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameStiffness(
		TEXT("Swarm.FlameStiffness"),
		55.f,
		TEXT("Spring stiffness pulling the flame toward the bearer — the responsiveness dial.\n")
		TEXT("Higher = snappier / catches up faster. Replaces the old VInterpTo lag, which\n")
		TEXT("could never overshoot; this is a real damped spring so a fast 180 lets the\n")
		TEXT("light's momentum sail past the hero before the spring reels it back. <=0 snaps\n")
		TEXT("instantly. Only the LIGHT springs — steering and the leash still read the true\n")
		TEXT("hero position, so the retinue math stays exact."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameDamping(
		TEXT("Swarm.FlameDamping"),
		0.6f,
		TEXT("Damping ratio, as a fraction of critical. 1.0 = critically damped (eases in,\n")
		TEXT("NEVER overshoots — the old VInterpTo feel). Below 1.0 overshoots on a fast\n")
		TEXT("direction change; lower = more overshoot and a longer settle. Above 1.0 is\n")
		TEXT("overdamped (sluggish). 0.6 gives a clear overshoot on a 180 that settles fast.\n")
		TEXT("Independent of stiffness — the critical coefficient is derived from it."),
		ECVF_Default);

	// --- soldier shadows on the flame (docs/RENDERING-LIGHTING.md §4b Phase C) --
	// There is no real light, so there is no real shadow: the flame is a luminance
	// trick in M_PP_Demichrome, and the shadow is the same trick. Each frame the
	// nearest retinue are published to MPC_Flame as occluders; the post pass casts
	// a radial wedge outward from each and dims the light there. Cost is flat —
	// N occluders in a per-pixel pass, independent of total swarm count.

	TAutoConsoleVariable<int32> CVarSwarmFlameShadows(
		TEXT("Swarm.FlameShadows"),
		0,
		TEXT("Soldiers cast shadows into the flame pool. 0 = off (the material is a no-op,\n")
		TEXT("identical to before). 1 = publish the nearest retinue as occluders each frame."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameShadowStrength(
		TEXT("Swarm.FlameShadowStrength"),
		0.6f,
		TEXT("How dark a soldier's shadow wedge gets, 0-1. 0 = no dimming, 1 = the wedge\n")
		TEXT("kills the light entirely (dithers to the outer dark)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameShadowSoft(
		TEXT("Swarm.FlameShadowSoft"),
		0.5f,
		TEXT("Penumbra width as a fraction of the occluder's angular size. 0 = hard edge,\n")
		TEXT("higher = softer, wider fade at the wedge sides."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmFlameShadowRadius(
		TEXT("Swarm.FlameShadowRadius"),
		45.f,
		TEXT("Caster radius in uu — sets each soldier's angular shadow width. Bigger =\n")
		TEXT("fatter wedges. A soldier's debug box is ~22uu; 45 casts a readable wedge."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarSwarmFlameShadowCount(
		TEXT("Swarm.FlameShadowCount"),
		8,
		TEXT("Max soldiers that cast a shadow at once (the nearest to the flame win).\n")
		TEXT("Hard-capped at 8 by the number of occluder slots in MPC_Flame."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherWorldAnchor(
		TEXT("Swarm.DitherWorldAnchor"),
		1.f,
		TEXT("0 = screen-anchored dither (pattern fixed to the display), 1 = world-anchored\n")
		TEXT("(pattern fixed to the ground and scrolling with it). World anchoring is the\n")
		TEXT("other mandatory anti-vignette mechanism — it is what proves the world is\n")
		TEXT("moving through the light. This CVar is the A/B for open decision L5."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmWorldDitherScale(
		TEXT("Swarm.WorldDitherScale"),
		12.f,
		TEXT("World units per dither texel when world-anchored. Lower = finer pattern.\n")
		TEXT("The camera shows ~2400uu across ~860px, i.e. ~2.8uu per pixel, so 5uu put a\n")
		TEXT("Bayer texel under 2px and the pattern read as noise. 12uu keeps texels at\n")
		TEXT("the 2x2-pixel minimum that docs/art/aesthetic-direction.md §2.4 requires of\n")
		TEXT("anything that moves."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherZoomCompensate(
		TEXT("Swarm.DitherZoomCompensate"),
		0.f,
		TEXT("1 = derive WorldDitherScale from the LIVE view width each frame so a Bayer texel\n")
		TEXT("keeps a constant size in PIXELS; 0 = off (default), WorldDitherScale is used as a\n")
		TEXT("fixed world-unit value exactly as before.\n")
		TEXT("\n")
		TEXT("Why this exists: WorldDitherScale is calibrated in WORLD units against a FIXED\n")
		TEXT("2400uu framing, but the constraint it encodes ('a texel must be at least 2 screen\n")
		TEXT("pixels', aesthetic-direction.md §2.4) is a SCREEN-space one. The moment the camera\n")
		TEXT("zooms — which is the whole point of Emberkeep.Cam.Scale — the two disagree: at a\n")
		TEXT("700uu framing an 8uu texel becomes ~10px instead of ~3px and the ground reads as a\n")
		TEXT("giant checkerboard. This keeps world ANCHORING (the anti-vignette mechanism, and\n")
		TEXT("the thing that proves the world moves through the light) while removing the zoom\n")
		TEXT("coupling.\n")
		TEXT("\n")
		TEXT("Cost, stated honestly: the pattern's world scale now breathes as the camera scales,\n")
		TEXT("so it is no longer rigidly welded to the ground. Camera scale tracks attrition and\n")
		TEXT("moves slowly, so this is invisible frame to frame — whereas the wrong texel size is\n")
		TEXT("glaring. That trade is the reason to prefer it, not an oversight."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherTexelPixels(
		TEXT("Swarm.DitherTexelPixels"),
		4.35f,
		TEXT("Target size of one Bayer texel in SCREEN PIXELS while DitherZoomCompensate is 1.\n")
		TEXT("Floor is 2 (aesthetic-direction.md §2.4, for anything that moves); below that the\n")
		TEXT("pattern reads as noise. This replaces WorldDitherScale as the dial you tune once\n")
		TEXT("compensation is on.\n")
		TEXT("\n")
		TEXT("The default is not a taste call — it is derived to REPRODUCE the owner-tuned\n")
		TEXT("WorldDitherScale of 8uu at the shipped 2400uu framing: a ~1305px game viewport puts\n")
		TEXT("2400uu at ~1.84uu/px, and 8 / 1.84 = 4.35. So switching compensation on leaves the\n")
		TEXT("wide shot looking as it does today and only changes what happens when you zoom.\n")
		TEXT("Note the honest caveat: because this is now measured in pixels, the world-unit\n")
		TEXT("figure it lands on will differ at a different resolution. That is the intended\n")
		TEXT("behaviour — the §2.4 constraint was always about pixels — but it does mean the old\n")
		TEXT("8uu number was only ever correct for one window size."),
		ECVF_Default);

	// --- demichrome dither fine dials (M_PP_Demichrome, via MPC_Flame) -------
	// The post-process quantises scene luminance to the 4 locked Demichrome values
	// using 3 thresholds; each threshold's step is softened by a Bayer dither band.
	// These used to be bake-time-only material scalars — now routed through MPC_Flame
	// so they can be tuned live like the flame dials.

	TAutoConsoleVariable<float> CVarSwarmDitherBandWidth(
		TEXT("Swarm.DitherBandWidth"),
		0.326f,
		TEXT("Width of the Bayer dither transition zone straddling each palette threshold.\n")
		TEXT("Wider = the two neighbouring values interleave over a bigger luminance range,\n")
		TEXT("so steps read softer and grainier; narrower = harder, cleaner banding with a\n")
		TEXT("more sudden value change. 0 = no dithering (pure hard posterise)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherThreshold1(
		TEXT("Swarm.DitherThreshold1"),
		0.40f,
		TEXT("Luminance where the palette steps from value 0 (darkest) to value 1. Lower\n")
		TEXT("pushes more of the image into the darker value. Keep 1 < 2 < 3 ordered."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherThreshold2(
		TEXT("Swarm.DitherThreshold2"),
		0.50f,
		TEXT("Luminance where the palette steps from value 1 to value 2 (the mid split).\n")
		TEXT("Keep 1 < 2 < 3 ordered."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmDitherThreshold3(
		TEXT("Swarm.DitherThreshold3"),
		0.75f,
		TEXT("Luminance where the palette steps from value 2 to value 3 (brightest). Raising\n")
		TEXT("it keeps the body of the lit pool at Bone so the white core reads (§4b.8).\n")
		TEXT("Keep 1 < 2 < 3 ordered."),
		ECVF_Default);

	// --- palette presets (docs/art/palette.json, task-043) -------------------
	//
	// The four output colours of the demichrome pass, driven live so a candidate
	// ramp can be judged in PIE instead of on a swatch. They ride MPC_Flame as
	// Palette0..3 and land on M_PP_Demichrome's Custom node inputs C0..C3 — the
	// same route Threshold1/2/3 already take.
	//
	// Sprites recolour too, and that is the point: the pass quantises scene
	// LUMINANCE into four buckets, so a sprite baked at Bone (luma 0.622) lands in
	// bucket 2 and emerges as whatever entry 2 currently is. Baked sheets do not
	// need regenerating to preview a ramp.
	//
	// NOT recoloured: UMG (EmberkeepPalette.h) draws after post-processing.
	//
	// TO ADD A CANDIDATE: one row here, darkest to brightest, plus the matching
	// entry in docs/data/art/palette.json. That is the whole cost, by design —
	// palettes are meant to be shopped.
	struct FSwarmPalettePreset
	{
		const TCHAR* Name;
		FColor Values[4];   // sRGB bytes, darkest -> brightest
	};

	const FSwarmPalettePreset GSwarmPalettePresets[] =
	{
		// name            dark                    steel/low-mid           bone/high-mid           pale/bright
		{ TEXT("demichrome"),  { FColor(0x21,0x1E,0x20), FColor(0x55,0x55,0x68), FColor(0xA0,0xA0,0x8B), FColor(0xE9,0xEF,0xEC) } },
		{ TEXT("eulbink-4"),   { FColor(0x25,0x24,0x46), FColor(0x00,0x98,0xDB), FColor(0x0C,0xE6,0xF2), FColor(0xFF,0xFF,0xFF) } },
		{ TEXT("rust-gold-4"), { FColor(0x33,0x1C,0x17), FColor(0x72,0x59,0x56), FColor(0xBB,0x7F,0x57), FColor(0xF6,0xCD,0x26) } },
	};

	const int32 GSwarmPaletteCount = UE_ARRAY_COUNT(GSwarmPalettePresets);

	TAutoConsoleVariable<int32> CVarSwarmPalette(
		TEXT("Emberkeep.Palette"),
		0,
		TEXT("Which palette the demichrome pass outputs. Live — no rebuild, no sprite regen.\n")
		TEXT("  0 = demichrome  (LOCKED direction, min luma gap 0.218)\n")
		TEXT("  1 = eulbink-4   (cold blue->white, 0.235 — separates better than demichrome)\n")
		TEXT("  2 = rust-gold-4 (warm rust->gold, 0.168)\n")
		TEXT("Out-of-range clamps to 0. UI does NOT follow — UMG draws after post.\n")
		TEXT("Judge candidates at horde density, not on a swatch: the minimum luma gap is\n")
		TEXT("what carries friend/foe readability at 700 units."),
		ECVF_Default);

	// --- per-unit flame shading (docs/RENDERING-LIGHTING.md §4a) -------------

	TAutoConsoleVariable<int32> CVarSwarmUnitShading(
		TEXT("Swarm.UnitShading"),
		0,
		TEXT("Light each unit from the flame: the hemisphere facing the flame draws\n")
		TEXT("brighter, the hemisphere turned to the dark draws dimmer, and both dim with\n")
		TEXT("distance from the flame. Grounds the units in the world as a point light\n")
		TEXT("would. 0 = flat single box (one colour, no direction).\n")
		TEXT("\n")
		TEXT("DEFAULTS TO 0 since 2026-07-26. It draws TWO DrawDebugSolidBox calls per\n")
		TEXT("unit per frame instead of one, on the path that is currently the whole\n")
		TEXT("shipping picture. Measured (-SwarmBench, clean A/B, retinue=100):\n")
		TEXT("  2000 brood  40.66 -> 18.43 ms frame  (25 -> 54 fps)  2.21x\n")
		TEXT(" 10000 brood 350.02 -> 135.47 ms frame ( 2.9 -> 7.4)   2.58x\n")
		TEXT("The ratio worsens with count as batching overhead compounds. It also\n")
		TEXT("rotates each unit's shading boundary to face the flame, which is a\n")
		TEXT("contributor to the 'offset overlapping grids' dither artifact against the\n")
		TEXT("world-anchored dither. Set 1 to see the directional read; expect the cost.\n")
		TEXT("Both go away with the Niagara sprite path (docs/perf/niagara-sprite-refactor.md)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmUnitBackShade(
		TEXT("Swarm.UnitBackShade"),
		0.32f,
		TEXT("How dark the dark-facing hemisphere is, as a fraction of the lit side.\n")
		TEXT("0 = black back (hard shading), 1 = no front/back difference."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmUnitLightFloor(
		TEXT("Swarm.UnitLightFloor"),
		0.28f,
		TEXT("Minimum brightness a unit keeps at the very edge of the light, so the horde\n")
		TEXT("never sinks fully into the dark and vanishes (the silhouette-rescue rule,\n")
		TEXT("docs/RENDERING-LIGHTING.md §2.5 / gate G5). 0 lets edge units disappear.\n")
		TEXT("RETINUE ONLY since 2026-07-26 — brood use BroodLightFloor/BroodLightCeil."),
		ECVF_Default);

	// --- brood-only exposure window (owner 2026-07-26) ----------------------
	// The silhouette-rescue floor is a promise about YOUR line, not about the tide.
	// Applied to both teams it did two things the owner called out: it pinned every
	// distant brood at one flat mid-value so they popped into being at the pool edge
	// as fully-formed shapes, and it let the near ones ride the full albedo into the
	// top of the ramp and blow out. Brood now get their own window — near-black at
	// spawn distance, held below the retinue at contact — which is the same split
	// Emberkeep.UnitCamProj.BroodFloor/BroodCeil already makes in the close-up panel
	// (docs/RENDERING-LIGHTING.md §4d finding 2/3). Keep the two roughly in step.
	TAutoConsoleVariable<float> CVarSwarmBroodLightFloor(
		TEXT("Swarm.BroodLightFloor"),
		0.f,
		TEXT("Minimum brightness for BROOD, replacing Swarm.UnitLightFloor for that team.\n")
		TEXT("0 means a brood at the outer edge of the pool is drawn black, which the\n")
		TEXT("demichrome pass quantises to Palette[0] — the same value as the ground, so\n")
		TEXT("it is invisible and FADES IN as it walks toward the flame. Raise it toward\n")
		TEXT("Swarm.UnitLightFloor to buy back the old always-visible tide."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmBroodLightCeil(
		TEXT("Swarm.BroodLightCeil"),
		0.7f,
		TEXT("Brightest a brood is ever drawn, as a fraction of its albedo — the anti\n")
		TEXT("blow-out dial for units standing in the flame. Clamped to >= BroodLightFloor.\n")
		TEXT("1 = the pre-2026-07-26 look (brood reach full albedo at the hero's feet).\n")
		TEXT("Holding it under 1 keeps brood a step below retinue at every distance, so a\n")
		TEXT("soldier beside a brood always reads as the lit one."),
		ECVF_Default);

	// --- body size, per team (added 2026-07-26) ---------------------------
	// Until now the only way to resize a unit was to select the placed ASwarmRenderActor
	// and edit its BroodDebugPointSize/RetinueDebugPointSize UPROPERTY, which is not a
	// thing you can do mid-fight. These override those at runtime.
	//
	// 0 deliberately means "don't override" rather than "zero-sized", so the level's
	// placed actor stays the design-time default and these stay a live experiment on top
	// of it. The exec file writes explicit positive values, so the breadboard row shows a
	// real number rather than a sentinel.
	//
	// Size is only half a decision: a brood grown past Swarm.BroodSeparation (60uu) will
	// visibly interpenetrate its neighbours, because separation is what actually holds
	// bodies apart and it has no idea how big they are. Move the two together.
	TAutoConsoleVariable<float> CVarSwarmBroodSize(
		TEXT("Swarm.BroodSize"),
		0.f,
		TEXT("Brood body half-extent in uu, overriding the render actor's BroodDebugPointSize\n")
		TEXT("(14). 0 = don't override. Read against RetinueSize: the size difference is how a\n")
		TEXT("player tells the tide from their own line before either resolves into a sprite.\n")
		TEXT("Grow it past Swarm.BroodSeparation and bodies interpenetrate — raise both."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmRetinueSize(
		TEXT("Swarm.RetinueSize"),
		0.f,
		TEXT("Retinue body half-extent in uu, overriding RetinueDebugPointSize (22).\n")
		TEXT("0 = don't override. Here because a size dial is only meaningful next to the\n")
		TEXT("thing it is compared against. Formation spacing is ~86uu, so keep under ~40."),
		ECVF_Default);

	// --- per-unit size variation -----------------------------------------
	// A horde whose every member is the exact same box reads as one texture, not as a
	// crowd. These are amplitudes on a per-entity roll the sim publishes in the spare
	// high bits of the anim int32 (SwarmRenderPack), so they retune live with no respawn.
	//
	// NOT honoured by the Niagara sprite path, which would need its own per-particle size
	// array and a graph edit. Only the debug-box renderer and the Unit Cam vary. That is
	// currently the whole shipping picture (Swarm.DebugRender is 1 because the emitter
	// draws nothing) but it is a trap waiting for the day it isn't.
	TAutoConsoleVariable<float> CVarSwarmBroodSizeJitter(
		TEXT("Swarm.BroodSizeJitter"),
		0.2f,
		TEXT("Per-brood size variation: each body draws at 1 +/- this fraction of\n")
		TEXT("Swarm.BroodSize, fixed for its lifetime. The dial that stops the tide reading\n")
		TEXT("as one repeated stamp. 0 = every brood identical (the pre-2026-07-26 look).\n")
		TEXT("Quantised to 16 steps, so beyond ~0.5 the banding starts to show."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmRetinueSizeJitter(
		TEXT("Swarm.RetinueSizeJitter"),
		0.f,
		TEXT("Same for your soldiers, and 0 on purpose: a drilled line that is uniform\n")
		TEXT("against a tide that is not is a free read on which team a body belongs to.\n")
		TEXT("Raise it if the retinue should look conscripted rather than trained."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSwarmBodyHeight(
		TEXT("Swarm.BodyHeight"),
		0.5f,
		TEXT("Body height as a multiple of half-extent, both teams. 0.5 is the shipped flat\n")
		TEXT("slab; ~1.5 stands the boxes up into figures. A placeholder-geometry dial — it\n")
		TEXT("goes away the day the boxes become sprites."),
		ECVF_Default);

	// Base albedos for the shaded path — brought down from pure white/bright so the
	// flame has room to modulate them; the light does the brightening, not the sprite.
	const FColor RetinueBaseAlbedo(232, 232, 238);
	const FColor BroodBaseAlbedo(170, 44, 36);

	// Retinue pure white, brood dark red — the flat fallback when UnitShading is off.
	const FColor RetinueDebugColor(255, 255, 255);
	// Brood sit low in the value range on purpose (owner 2026-07-23): they read as
	// the dark made flesh, and they are darkest at the edge of the pool where they
	// enter — the flame lifts them only as they close on the hero. Was (190,45,35).
	const FColor BroodDebugColor(130, 32, 26);

	/**
	 * Hit flash. Pure white, and deliberately NOT shaded by the flame.
	 *
	 * Two reasons it has to be light-exempt. A unit struck at the edge of the pool has
	 * its brightness floored by Swarm.UnitLightFloor (0.28), which the demichrome pass
	 * then quantises well below Threshold3 — so an attenuated "white" flash out there
	 * would not even reach the brightest palette value and the hit would be invisible
	 * exactly where the fighting starts. And retinue are *already* near-white, so a
	 * tint alone does nothing for them; what makes a soldier's flash read is the
	 * two-tone shading dropping away for an instant (see TickDebugRender).
	 *
	 * Same class of deliberate palette exception as the flame's white core.
	 */
	const FColor HitFlashColor(255, 255, 255);

	constexpr float DebugPointZOffset = 30.f;
}

ASwarmRenderActor::ASwarmRenderActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after Mass PrePhysics processing

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	RootComponent = NiagaraComponent;
	NiagaraComponent->SetAutoActivate(true);

	// task-048: on-demand capture for Swarm.DebugShotAfter. bCaptureEveryFrame/OnMovement are
	// both false — this only renders when TakeDebugShot() calls CaptureScene() explicitly, so it
	// costs nothing the rest of the time. SCS_FinalColorLDR (not SceneColorHDR) is what pulls the
	// demichrome post-process into the capture, same as the live game view — a shot that skipped
	// post-processing would prove nothing about what the game actually looks like.
	DebugCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DebugCaptureComponent"));
	DebugCaptureComponent->SetupAttachment(RootComponent);
	DebugCaptureComponent->bCaptureEveryFrame = false;
	DebugCaptureComponent->bCaptureOnMovement = false;
	DebugCaptureComponent->CaptureSource = SCS_FinalColorLDR;

	// Resolved here rather than left for a designer to assign: the light is not
	// optional set dressing, it is the only light in the game, and an actor
	// placed without it would render a black level with no obvious cause.
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> FlameMPC(
		TEXT("/Game/PostProcess/MPC_Flame.MPC_Flame"));
	if (FlameMPC.Succeeded())
	{
		FlameCollection = FlameMPC.Object;
	}
}

void ASwarmRenderActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	// Per-run seed so two sessions don't flicker in lockstep.
	FlameSeed = FMath::FRandRange(0.f, 1000.f);

	// Debug hook for MCP-driven sessions (no interactive console): run any
	// console commands listed in Saved/SwarmExecOnPlay.txt at BeginPlay.
	FString ExecFileContents;
	if (FFileHelper::LoadFileToString(ExecFileContents, *(FPaths::ProjectSavedDir() / TEXT("SwarmExecOnPlay.txt"))))
	{
		TArray<FString> ExecLines;
		ExecFileContents.ParseIntoArrayLines(ExecLines);
		for (const FString& Line : ExecLines)
		{
			// Strip inline comments (# or ;) so the file can be self-documenting;
			// what remains is the bare console command. Whole-line comments and
			// blank lines fall out here too.
			FString Command = Line;
			int32 CommentAt;
			if (Command.FindChar(TEXT('#'), CommentAt) || Command.FindChar(TEXT(';'), CommentAt))
			{
				Command = Command.Left(CommentAt);
			}
			Command.TrimStartAndEndInline();
			if (!Command.IsEmpty())
			{
				BenchExec(Command);
			}
		}
	}

	const bool bArmed = bRunBenchmark || FParse::Param(FCommandLine::Get(), TEXT("SwarmBench"));
	if (World && World->IsGameWorld() && bArmed)
	{
		if (GEngine)
		{
			GEngine->bSmoothFrameRate = false; // uncapped frame rate so timings are real
		}
		BenchExec(TEXT("t.MaxFPS 0"));
		BenchExec(TEXT("r.VSync 0"));
		BenchStep = 0;
		BenchStartStep();
	}
}

void ASwarmRenderActor::BenchExec(const FString& Cmd)
{
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), *Cmd);
	}
}

void ASwarmRenderActor::BenchStartStep()
{
	if (BenchStep >= BenchmarkBroodCounts.Num())
	{
		BenchExec(TEXT("Swarm.Clear"));
		BenchPhase = EBenchPhase::Off;
		UE_LOG(LogTemp, Display, TEXT("SwarmBench: DONE"));
		return;
	}

	BenchExec(TEXT("Swarm.Clear"));
	BenchExec(FString::Printf(TEXT("Swarm.SpawnRetinue %d"), BenchmarkRetinueCount));
	BenchExec(FString::Printf(TEXT("Swarm.SpawnBrood %d"), BenchmarkBroodCounts[BenchStep]));
	BenchPhase = EBenchPhase::Settle;
	BenchTimer = 0.f;
}

void ASwarmRenderActor::BenchTick(float DeltaSeconds)
{
	if (BenchPhase == EBenchPhase::Off)
	{
		return;
	}

	BenchTimer += DeltaSeconds;

	if (BenchPhase == EBenchPhase::Settle)
	{
		if (BenchTimer >= BenchmarkSettleSeconds)
		{
			BenchPhase = EBenchPhase::Sample;
			BenchTimer = 0.f;
			BenchFrames = 0;
			BenchFrameMs = BenchGameMs = BenchRenderMs = BenchGpuMs = 0.0;
		}
		return;
	}

	// Sample phase: GGameThreadTime / GRenderThreadTime / GPU cycles are the
	// previous frame's thread times, updated once per frame by the engine.
	++BenchFrames;
	BenchFrameMs += DeltaSeconds * 1000.0;
	BenchGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
	BenchRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
	BenchGpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());

	if (BenchTimer >= BenchmarkSampleSeconds && BenchFrames > 0)
	{
		const double Inv = 1.0 / BenchFrames;
		UE_LOG(LogTemp, Display,
			TEXT("SwarmBench: brood=%d retinue=%d frame=%.2fms game=%.2fms draw=%.2fms gpu=%.2fms fps=%.1f"),
			BenchmarkBroodCounts[BenchStep], BenchmarkRetinueCount,
			BenchFrameMs * Inv, BenchGameMs * Inv, BenchRenderMs * Inv, BenchGpuMs * Inv,
			1000.0 / (BenchFrameMs * Inv));
		++BenchStep;
		BenchStartStep();
	}
}

void ASwarmRenderActor::TickSpacingLog(float DeltaSeconds)
{
	PlayTime += DeltaSeconds;

	const float ShotAfter = CVarSwarmDebugShot.GetValueOnGameThread();
	if (!bDebugShotTaken && ShotAfter > 0.f && PlayTime >= ShotAfter)
	{
		bDebugShotTaken = true;
		TakeDebugShot();
	}

	const float Interval = CVarSwarmSpacingLog.GetValueOnGameThread();
	if (Interval <= 0.f)
	{
		SpacingLogTimer = 0.f;
		return;
	}

	SpacingLogTimer += DeltaSeconds;
	if (SpacingLogTimer >= Interval)
	{
		SpacingLogTimer = 0.f;
		SwarmDebug::LogSpacingReport(GetWorld());

		UE_LOG(LogTemp, Display, TEXT("SwarmDebug: debugRender=%d niagaraVisible=%d"),
			CVarSwarmDebugRender.GetValueOnGameThread(),
			NiagaraComponent ? (int32)NiagaraComponent->IsVisible() : -1);
	}
}

void ASwarmRenderActor::TakeDebugShot()
{
	UWorld* World = GetWorld();
	if (!World || !DebugCaptureComponent)
	{
		return;
	}

	// Mirror the live game camera exactly rather than re-deriving ASpikeHeroPawn's Ortho/Pitch/
	// Yaw/Dist/HudBias math a second time here — see the doc comment on DebugCaptureComponent.
	const APlayerController* PC = World->GetFirstPlayerController();
	const APlayerCameraManager* CamMgr = PC ? PC->PlayerCameraManager : nullptr;
	if (!CamMgr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwarmDebug: no active player camera to shoot from — skipping capture."));
		return;
	}
	const FMinimalViewInfo& View = CamMgr->GetCameraCacheView();

	const int32 Width = FMath::Clamp(CVarSwarmDebugShotWidth.GetValueOnGameThread(), 64, 7680);
	const int32 Height = FMath::Clamp(CVarSwarmDebugShotHeight.GetValueOnGameThread(), 64, 4320);
	if (!DebugCaptureRT || DebugCaptureRT->SizeX != Width || DebugCaptureRT->SizeY != Height)
	{
		// Routed through the same Blueprint-facing helper the "Create Render Target 2D" node
		// uses, rather than hand-rolling NewObject+InitAutoFormat, so this doesn't have to track
		// engine-version init-order quirks separately.
		DebugCaptureRT = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, Width, Height, RTF_RGBA8_SRGB, FLinearColor::Black, /*bAutoGenerateMipMaps=*/false);
	}
	DebugCaptureComponent->TextureTarget = DebugCaptureRT;

	DebugCaptureComponent->SetWorldLocationAndRotation(View.Location, View.Rotation);
	if (View.ProjectionMode == ECameraProjectionMode::Orthographic)
	{
		DebugCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		DebugCaptureComponent->OrthoWidth = FMath::Max(View.OrthoWidth, 1.f);
	}
	else
	{
		DebugCaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
		DebugCaptureComponent->FOVAngle = View.FOV;
	}

	DebugCaptureComponent->CaptureScene();

	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots");
	const FString FileName = FString::Printf(TEXT("SwarmDebugShot_%s.png"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	UKismetRenderingLibrary::ExportRenderTarget(this, DebugCaptureRT, Dir, FileName);

	// DrawDebugSolidBox primitives are not visible to a scene capture (verified, task-048 /
	// docs/AGENT-TEAMS.md) — flag it plainly so a blank-looking shot is diagnosable without
	// re-deriving this from scratch.
	const bool bDebugBoxMode = CVarSwarmDebugRender.GetValueOnGameThread() != 0;
	UE_LOG(LogTemp, Display,
		TEXT("SwarmDebug: capture written to %s (%dx%d, %s)%s"),
		*(Dir / FileName), Width, Height,
		bDebugBoxMode ? TEXT("debug-box mode") : TEXT("Niagara sprite mode"),
		bDebugBoxMode ? TEXT(" — WARNING: debug boxes do not render into scene captures; set Swarm.DebugRender 0 to see the swarm in this shot") : TEXT(""));
}

void ASwarmRenderActor::TickFlame(float DeltaSeconds)
{
	if (CVarSwarmFlame.GetValueOnGameThread() == 0 || !FlameCollection)
	{
		return;
	}

	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	// The attractor is already the hero's published position, updated every tick
	// by the pawn. Reading it here rather than finding the pawn keeps one source
	// of truth for "where the bearer is" — the swarm and the light can't disagree.
	const FVector Target = Swarm->GetAttractor();

	// Damped spring pulling the flame toward the bearer (owner 2026-07-23), so it
	// trails, and — unlike the old VInterpTo ease — carries momentum: a fast 180
	// lets it overshoot before the spring reels it back. Snap on the first tick so
	// it doesn't streak in from the world origin; snap thereafter only if disabled.
	const float Stiffness = CVarSwarmFlameStiffness.GetValueOnGameThread();
	if (!bFlameInitialized || Stiffness <= 0.f)
	{
		SmoothedFlamePos = Target;
		FlameVel = FVector::ZeroVector;
		bFlameInitialized = true;
	}
	else
	{
		// accel = k·(target−pos) − c·vel, semi-implicit Euler (update vel then pos).
		// c is derived from the damping RATIO so overshoot is independent of
		// stiffness: critical is 2·sqrt(k); ratio<1 underdamps and overshoots.
		// dt is clamped so a frame spike can't make the explicit integrator blow up.
		const float SpringDt = FMath::Min(DeltaSeconds, 1.f / 30.f);
		const float Ratio = FMath::Max(CVarSwarmFlameDamping.GetValueOnGameThread(), 0.f);
		const float Damping = Ratio * 2.f * FMath::Sqrt(Stiffness);
		const FVector ToTarget = Target - SmoothedFlamePos;
		FlameVel += (ToTarget * Stiffness - FlameVel * Damping) * SpringDt;
		SmoothedFlamePos += FlameVel * SpringDt;
	}
	const FVector Flame = SmoothedFlamePos;

	// Flicker: two incommensurate sines so the period never reads as a loop.
	// Cheap, and it is doing real work — see §4b.1, a steady radial gradient on a
	// hero-locked camera reads as a lens vignette rather than a carried fire.
	const float Amplitude = FMath::Clamp(CVarSwarmFlameFlicker.GetValueOnGameThread(), 0.f, 1.f);
	const float Time = World->GetTimeSeconds();
	const float Wobble = 0.6f * FMath::Sin(Time * 11.3f + FlameSeed)
					   + 0.4f * FMath::Sin(Time * 17.7f + FlameSeed * 2.1f);
	const float Intensity = CVarSwarmFlameIntensity.GetValueOnGameThread() * (1.f + Amplitude * Wobble);

	UKismetMaterialLibrary::SetVectorParameterValue(
		World, FlameCollection, FName(TEXT("FlamePosition")),
		FLinearColor(Flame.X, Flame.Y, Flame.Z, 0.f));

	const auto SetScalar = [this, World](const TCHAR* Name, float Value)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(World, FlameCollection, FName(Name), Value);
	};

	// Pool radii, optionally re-derived from the live framing so the pool keeps a constant
	// SCREEN fraction while the camera zooms (Swarm.FlameScaleWithView) instead of blowing
	// out at the close end of Emberkeep.Cam.Scale. Off by default — see the CVar comment.
	float EffectiveFlameRadius = CVarSwarmFlameRadius.GetValueOnGameThread();
	float EffectiveFlameCoreRadius = CVarSwarmFlameCoreRadius.GetValueOnGameThread();
	FVector2D ViewportSize = FVector2D::ZeroVector;
	const float ViewWidthUU = GetLiveViewWidthUU(World, Flame, ViewportSize);
	if (CVarSwarmFlameScaleWithView.GetValueOnGameThread() != 0 && ViewWidthUU > 1.f)
	{
		const float RefWidth = FMath::Max(CVarSwarmFlameScaleReferenceWidth.GetValueOnGameThread(), 1.f);
		const float ViewScale = ViewWidthUU / RefWidth;
		EffectiveFlameRadius *= ViewScale;
		EffectiveFlameCoreRadius *= ViewScale;
	}

	SetScalar(TEXT("FlameRadius"), EffectiveFlameRadius);
	SetScalar(TEXT("FlameCoreRadius"), EffectiveFlameCoreRadius);
	SetScalar(TEXT("FlameFalloff"), CVarSwarmFlameFalloff.GetValueOnGameThread());
	SetScalar(TEXT("FlameIntensity"), FMath::Max(Intensity, 0.f));
	SetScalar(TEXT("DitherWorldAnchor"), CVarSwarmDitherWorldAnchor.GetValueOnGameThread());

	// World dither scale, optionally re-derived from the live framing so a Bayer texel keeps a
	// constant PIXEL size while the camera zooms (Swarm.DitherZoomCompensate). Reuses the same
	// live-view-width measurement as the pool radii above.
	float WorldDitherScale = CVarSwarmWorldDitherScale.GetValueOnGameThread();
	if (CVarSwarmDitherZoomCompensate.GetValueOnGameThread() != 0.f && ViewWidthUU > 1.f)
	{
		const float UUPerPixel = ViewWidthUU / (float)ViewportSize.X;
		WorldDitherScale = FMath::Max(
			UUPerPixel * CVarSwarmDitherTexelPixels.GetValueOnGameThread(), 0.01f);
	}
	SetScalar(TEXT("WorldDitherScale"), WorldDitherScale);
	SetScalar(TEXT("DitherBandWidth"), CVarSwarmDitherBandWidth.GetValueOnGameThread());
	SetScalar(TEXT("Threshold1"), CVarSwarmDitherThreshold1.GetValueOnGameThread());
	SetScalar(TEXT("Threshold2"), CVarSwarmDitherThreshold2.GetValueOnGameThread());
	SetScalar(TEXT("Threshold3"), CVarSwarmDitherThreshold3.GetValueOnGameThread());

	// Palette: push the selected preset's four colours to MPC_Flame every tick, so
	// dragging Emberkeep.Palette in the Breadboard recolours the world immediately.
	//
	// Deliberately NOT FLinearColor::FromSRGBColor. palette.json's luma_model is
	// "gamma sRGB (no linearization)" and the material's existing Color_* defaults
	// store the raw byte/255 (0x21 -> 0.129412). Linearizing here would darken every
	// value and silently break the threshold calibration the whole look is tuned to.
	{
		const int32 Index = FMath::Clamp(
			CVarSwarmPalette.GetValueOnGameThread(), 0, GSwarmPaletteCount - 1);
		const FSwarmPalettePreset& Preset = GSwarmPalettePresets[Index];
		for (int32 v = 0; v < 4; ++v)
		{
			const FColor& C = Preset.Values[v];
			UKismetMaterialLibrary::SetVectorParameterValue(
				World, FlameCollection,
				FName(*FString::Printf(TEXT("Palette%d"), v)),
				FLinearColor(C.R / 255.f, C.G / 255.f, C.B / 255.f, 1.f));
		}
	}

	// --- radial shadow buffer: ALL soldiers occlude the flame (§4b Phase C, faked).
	// 64 angular bins around the flame, each holding the nearest retinue distance in
	// that direction (0 = none). Every soldier splats itself across the bins it
	// subtends — a closer soldier wins its bins. Packed into FlameOcc0..15 (4 bins
	// per vector); the post pass samples one bin per pixel. No real light, no shadow
	// maps. Shadows off → an all-zero buffer, so the shader is a no-op.
	constexpr int32 NumBins = 64; // must match the shader (16 occluder vectors x 4)
	float Bins[NumBins];
	for (int32 b = 0; b < NumBins; ++b)
	{
		Bins[b] = 0.f;
	}

	if (CVarSwarmFlameShadows.GetValueOnGameThread() != 0)
	{
		const TArray<FVector>& Positions = Swarm->GetRenderPositions();
		const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();
		const int32 Num = FMath::Min(Positions.Num(), AnimBits.Num());
		// Effective (possibly view-scaled) radius, so a soldier's shadow wedge switches off
		// at the same visual pool edge the post-process is actually drawing.
		const float PoolR = FMath::Max(EffectiveFlameRadius, 1.f);
		const float CasterR = FMath::Max(CVarSwarmFlameShadowRadius.GetValueOnGameThread(), 1.f);
		const float TwoPi = 2.f * PI;

		for (int32 i = 0; i < Num; ++i)
		{
			if ((AnimBits[i] & SwarmAnim::TeamBit) == 0)
			{
				continue; // retinue only — brood don't carry the light's story
			}
			const float Dx = Positions[i].X - Flame.X;
			const float Dy = Positions[i].Y - Flame.Y;
			const float D = FMath::Sqrt(Dx * Dx + Dy * Dy);
			if (D < 1.f || D > PoolR)
			{
				continue; // outside the pool casts nothing visible
			}
			// Bin range this soldier subtends, at its distance. Half-angle grows as
			// the soldier nears the flame, so close soldiers throw wider wedges.
			const float Centre = (FMath::Atan2(Dy, Dx) / TwoPi + 0.5f) * NumBins;
			const float HalfBins = (FMath::Atan(CasterR / D) / TwoPi) * NumBins + 0.5f;
			const int32 Lo = FMath::FloorToInt(Centre - HalfBins);
			const int32 Hi = FMath::CeilToInt(Centre + HalfBins);
			for (int32 b = Lo; b <= Hi; ++b)
			{
				const int32 Bi = ((b % NumBins) + NumBins) % NumBins;
				Bins[Bi] = (Bins[Bi] <= 0.f) ? D : FMath::Min(Bins[Bi], D);
			}
		}
	}

	// Pack the 64 bins into FlameOcc0..15 (xyzw = four consecutive bins).
	for (int32 v = 0; v < NumBins / 4; ++v)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			World, FlameCollection, FName(*FString::Printf(TEXT("FlameOcc%d"), v)),
			FLinearColor(Bins[v * 4 + 0], Bins[v * 4 + 1], Bins[v * 4 + 2], Bins[v * 4 + 3]));
	}
	SetScalar(TEXT("FlameShadowStrength"), CVarSwarmFlameShadowStrength.GetValueOnGameThread());
	SetScalar(TEXT("FlameShadowSoft"), CVarSwarmFlameShadowSoft.GetValueOnGameThread());
}

void ASwarmRenderActor::TickDebugRender()
{
	SWARM_SCOPE(STAT_SwarmDebugDraw, SwarmDebugDraw);

	UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	const TArray<FVector>& Positions = Swarm->GetRenderPositions();
	const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();

	const bool bShade = CVarSwarmUnitShading.GetValueOnGameThread() != 0;

	// Shade from the same point the light springs to, so the units and the pool
	// agree about where the flame is. Falls back to the true hero position if the
	// flame writer is disabled and the smoothed value was never seeded.
	const FVector FlamePos = bFlameInitialized ? SmoothedFlamePos : Swarm->GetAttractor();
	const float Radius = FMath::Max(CVarSwarmFlameRadius.GetValueOnGameThread(), 1.f);
	const float Falloff = FMath::Max(CVarSwarmFlameFalloff.GetValueOnGameThread(), 0.001f);
	const float BackShade = FMath::Clamp(CVarSwarmUnitBackShade.GetValueOnGameThread(), 0.f, 1.f);
	const float LightFloor = FMath::Clamp(CVarSwarmUnitLightFloor.GetValueOnGameThread(), 0.f, 1.f);
	const float BroodFloor = FMath::Clamp(CVarSwarmBroodLightFloor.GetValueOnGameThread(), 0.f, 1.f);
	const float BroodCeil = FMath::Clamp(CVarSwarmBroodLightCeil.GetValueOnGameThread(), BroodFloor, 1.f);

	// Per-team size. A CVar of 0 falls back to the placed actor's UPROPERTY, so the level
	// keeps its design-time default and these stay a live override on top of it.
	const float BroodOverride = CVarSwarmBroodSize.GetValueOnGameThread();
	const float RetinueOverride = CVarSwarmRetinueSize.GetValueOnGameThread();
	const float BroodHalf = BroodOverride > 0.f ? BroodOverride : BroodDebugPointSize;
	const float RetinueHalf = RetinueOverride > 0.f ? RetinueOverride : RetinueDebugPointSize;
	const float HeightRatio = FMath::Max(CVarSwarmBodyHeight.GetValueOnGameThread(), 0.01f);
	const float BroodJitter = FMath::Clamp(CVarSwarmBroodSizeJitter.GetValueOnGameThread(), 0.f, 0.95f);
	const float RetinueJitter = FMath::Clamp(CVarSwarmRetinueSizeJitter.GetValueOnGameThread(), 0.f, 0.95f);

	const auto Shade = [](const FColor& C, float M) -> FColor
	{
		return FColor(
			(uint8)FMath::Clamp(FMath::RoundToInt(C.R * M), 0, 255),
			(uint8)FMath::Clamp(FMath::RoundToInt(C.G * M), 0, 255),
			(uint8)FMath::Clamp(FMath::RoundToInt(C.B * M), 0, 255));
	};

	const int32 Num = FMath::Min(Positions.Num(), AnimBits.Num());
	for (int32 i = 0; i < Num; ++i)
	{
		const bool bRetinue = (AnimBits[i] & SwarmAnim::TeamBit) != 0;
		const float HalfSize = (bRetinue ? RetinueHalf : BroodHalf)
			* SwarmRenderPack::SizeScale(AnimBits[i], bRetinue ? RetinueJitter : BroodJitter);
		const float HalfZ = HalfSize * HeightRatio;
		// Lift the box off the floor by at least its own half-height, so a body can never
		// sink through the ground plane as it scales. DebugPointZOffset (30) is the tuned
		// resting lift and still wins at every default size (HalfZ is 7 brood / 11 retinue),
		// so this changes nothing until a size dial makes a body taller than the lift —
		// which is exactly the case it exists for.
		const FVector Centre = Positions[i] + FVector(0.f, 0.f, FMath::Max(DebugPointZOffset, HalfZ));

		// Struck this instant: one solid white box, full size, no directional split and
		// no distance falloff. Collapsing the two half-boxes is what sells it — the dark
		// half vanishing for a tenth of a second is a much louder change than a colour
		// shift, and it works on retinue, who are already almost white.
		if ((AnimBits[i] & SwarmAnim::HitFlashBit) != 0)
		{
			DrawDebugSolidBox(
				World, Centre, FVector(HalfSize, HalfSize, HalfZ), HitFlashColor,
				/*bPersistent=*/false, /*LifeTime=*/-1.f, /*DepthPriority=*/0);
			continue;
		}

		if (!bShade)
		{
			DrawDebugSolidBox(
				World, Centre, FVector(HalfSize, HalfSize, HalfZ),
				bRetinue ? RetinueDebugColor : BroodDebugColor,
				/*bPersistent=*/false, /*LifeTime=*/-1.f, /*DepthPriority=*/0);
			continue;
		}

		// Direction to the flame in the ground plane. The lit hemisphere is the one
		// this points at; the far hemisphere is turned to the outer dark.
		const FVector2D ToFlame(FlamePos.X - Centre.X, FlamePos.Y - Centre.Y);
		const float Dist = ToFlame.Size();
		const FVector2D Dir = Dist > 1.f ? ToFlame / Dist : FVector2D(1.f, 0.f);

		// Distance attenuation mapped into a per-team exposure window. Retinue keep the
		// shared silhouette-rescue floor and the full range above it; brood ride their own
		// narrower window, so they surface out of the dark on the approach instead of
		// arriving already visible, and stop short of the top of the ramp when they get here.
		const float T = FMath::Clamp(Dist / Radius, 0.f, 1.f);
		const float Atten = 1.f - FMath::Pow(T, Falloff);
		const float Floor = bRetinue ? LightFloor : BroodFloor;
		const float Ceil = bRetinue ? 1.f : BroodCeil;
		const float Lit = FMath::Lerp(Floor, Ceil, Atten);

		const FColor& Base = bRetinue ? RetinueBaseAlbedo : BroodBaseAlbedo;
		const FColor FrontCol = Shade(Base, Lit);
		const FColor BackCol = Shade(Base, Lit * BackShade);

		// Two half-boxes split along the flame direction: near half lit, far half dark.
		const FQuat Rot(FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), 0.f));
		const FVector HalfExtent(HalfSize * 0.5f, HalfSize, HalfZ);
		const FVector Offset = Rot.RotateVector(FVector(HalfSize * 0.5f, 0.f, 0.f));

		DrawDebugSolidBox(World, Centre - Offset, HalfExtent, Rot, BackCol,
			/*bPersistent=*/false, /*LifeTime=*/-1.f, /*DepthPriority=*/0);
		DrawDebugSolidBox(World, Centre + Offset, HalfExtent, Rot, FrontCol,
			/*bPersistent=*/false, /*LifeTime=*/-1.f, /*DepthPriority=*/0);
	}
}

void ASwarmRenderActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SWARM_SCOPE(STAT_SwarmRenderBridge, SwarmRenderBridge);

	BenchTick(DeltaSeconds);
	TickSpacingLog(DeltaSeconds);
	TickFlame(DeltaSeconds);

	const USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || !NiagaraComponent)
	{
		return;
	}

	// One renderer at a time, so a broken sprite setup can't be mistaken for a
	// broken sim (or vice versa).
	const bool bDebugRender = CVarSwarmDebugRender.GetValueOnGameThread() != 0;
	NiagaraComponent->SetVisibility(!bDebugRender);

	// The demichrome post-process stays on by default — the debug points are
	// bright enough to read straight through the dither, and the whole point of
	// judging the game is judging it as it actually looks. Only an explicit
	// opt-in drops it. Driven on change so toggling either way self-heals.
	const int32 PlainView = CVarSwarmDebugPlainView.GetValueOnGameThread();
	if (LastPlainViewState != PlainView)
	{
		LastPlainViewState = PlainView;
		BenchExec(PlainView != 0 ? TEXT("r.PostProcessing.DisableMaterials 1")
								 : TEXT("r.PostProcessing.DisableMaterials 0"));
	}

	if (bDebugRender)
	{
		TickDebugRender();
		return;
	}

	SWARM_SCOPE(STAT_SwarmNiagaraPush, SwarmNiagaraPush);

	const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();
	SubImageScratch.Reset(AnimBits.Num());

	// The sim stores a WORLD facing; this camera turns it into a column. Reading the
	// live orbit CVar here rather than baking a column in the sim is what keeps sprites
	// correct while Emberkeep.Cam.Yaw spins the map — otherwise every unit would keep
	// facing its old screen direction as the view rotated under it.
	const float ViewYaw = GetCameraYawDegrees();

	for (const int32 Bits : AnimBits)
	{
		// One shared decode (SwarmSheet::CellFor) rather than the old inline
		// `frame + 2*team`, so the Unit Cam and this bridge cannot drift apart on what
		// cell a given anim byte means.
		//
		// The (uint8) cast is load-bearing, not tidiness: bits 8-11 carry the per-entity
		// size roll and bits 12-16 the facing (SwarmRenderPack); both would decode as
		// garbage anim state without it. This path also IGNORES the size roll —
		// Swarm.BroodSizeJitter does nothing to Niagara sprites, which need a
		// per-particle size array and a graph edit to honour it. Every brood sprite is
		// the same size until that exists.
		const int32 Column = SwarmFacing::ColumnFor(
			SwarmRenderPack::Facing(Bits), ViewYaw, SwarmSheet::Columns);
		SubImageScratch.Add((float)SwarmSheet::CellFor((uint8)Bits, Column));
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
		NiagaraComponent, FName(TEXT("Positions")), Swarm->GetRenderPositions());
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		NiagaraComponent, FName(TEXT("SubImages")), SubImageScratch);
	NiagaraComponent->SetVariableInt(FName(TEXT("Count")), Swarm->GetRenderPositions().Num());
}
