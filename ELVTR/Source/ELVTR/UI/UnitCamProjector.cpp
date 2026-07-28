#include "UI/UnitCamProjector.h"

#include "UI/EmberkeepPalette.h"
#include "Mass/SwarmSubsystem.h"
#include "Mass/SwarmFragments.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Math/Box2D.h"
#include "Math/RotationMatrix.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	// --- per-body sprite sheets (task-050) -----------------------------------
	// Every billboard used to slice its frame out of ONE shared atlas (T_Swarm_2bit). Now a
	// body can draw from any of several textures, each with its own grid — the bearer's own
	// portrait sheet, the knight, the archer. SUnitCamCanvas keeps one TArray<FSlateBrush>
	// per sheet (BrushSets); a billboard names which one via SpriteSet.
	//
	// History worth knowing if you're reading this after a git blame: task-050 originally drew
	// retinue from six 48px soldier-roster variants (docs/art/soldier-roster-v1.md,
	// T_Soldier_01..06). The owner rejected that on sight — "we degraded with the units again.
	// I only want my high resolution units for the retinue" — and named exactly two PixelLab
	// characters (88-92px native) to replace it. The six variant textures still exist in
	// Content/Sprites/Units and RawArt/Sheets; this file just no longer loads or draws them.
	//
	// task-046 revision: sprite choice is now TWO axes, not one. TYPE (Spearmen/Archers, from
	// the sim's real per-soldier type) picks which of the two lists below a soldier draws
	// from; STATE (a stable per-soldier hash — the same SizeBucket-derived hash task-050
	// used, kept for exactly this) picks WHICH ENTRY within that list. Each entry is a
	// PixelLab "state": the same character group re-rendered with a variation (a face
	// shield, a different helmet) — the owner's own framing, "variants I mean states...
	// variety of the character type." A rank of spearmen reads as visibly varied while every
	// one of them is unambiguously a spearman, because state selection happens INSIDE the
	// type, not instead of it. SpearmenStateBase is not a compile-time constant — how many
	// Spearmen states are actually loaded is DATA (UUnitCamProjector::SpearmenStateTextures),
	// so Archer states start wherever Spearmen's happen to end; see NativeTick.
	namespace UnitCamSprite
	{
		constexpr uint8 SwarmAtlas = 0;      // T_Swarm_2bit — brood, and retinue when RetinueHighRes is off
		constexpr uint8 Hero = 1;            // T_Hero_Vanguard
		constexpr uint8 SpearmenStateBase = 2;
	}

	/**
	 * Data-driven state lists — adding a state is adding a path here, nothing else. Element 0
	 * is task-050's original texture for each type; this is intentionally the ONLY place that
	 * needs to change to add a second state once its sheet is packed (a /sprite pass, not a
	 * C++ change — see the doc comment on UUnitCamProjector::SpearmenStateTextures). Every
	 * asset listed here MUST share the RetinueSheetColumns x RetinueSheetRows grid (5x2,
	 * below) — the DirCol / south-walk-toggle logic reads every state through that one grid.
	 */
	const TArray<FString>& SpearmenStatePaths()
	{
		static const TArray<FString> Paths = { TEXT("/Game/Sprites/Units/T_Soldier_Knight.T_Soldier_Knight") };
		return Paths;
	}
	const TArray<FString>& ArcherStatePaths()
	{
		static const TArray<FString> Paths = { TEXT("/Game/Sprites/Units/T_Soldier_Archer.T_Soldier_Archer") };
		return Paths;
	}

	// docs/data/art/requests/hero-vanguard.json output.grid: 4x4, 16 cells, (idle, walk1) pairs
	// per direction. Order is S,SW,W,NW,N,NE,E,SE — the OPPOSITE rotation sense from
	// SwarmFacing's south-first-COUNTER-clockwise convention (S,SE,E,NE,N,NW,W,SW), which is
	// what the knight/archer sheets use (see RetinueSheetColumns below). That mismatch is why
	// the hero billboard below does NOT attempt per-view facing yet — mapping a resolved
	// SwarmFacing column onto this sheet would silently mirror every non-south direction. Fixed
	// at cell 0 (south idle) until someone builds that column remap deliberately.
	constexpr int32 HeroSheetColumns = 4;
	constexpr int32 HeroSheetRows = 4;

	// T_Soldier_Knight and T_Soldier_Archer share one grid, 5 cols x 2 rows = 10 cells, on
	// purpose — one code path for both rather than a per-unit special case:
	//   cells 0-7: the 8 real rotations, south-first counter-clockwise (S,SE,E,NE,N,NW,W,SW) —
	//     the SAME order and sense SwarmFacing already uses, so SwarmFacing::ColumnFor(..., 8)
	//     hands back a value that IS the flat cell index here. This identity (Row*Columns+Col
	//     recovers the direction index) holds for ANY column count, not just a divisor of 8 —
	//     it is plain row-major integer div/mod, so 5 columns works exactly like the old
	//     4-column militia sheets did.
	//   cells 8-9: a SOUTH-ONLY two-frame walk toggle (see the retinue billboard loop below,
	//     which switches to these only when the resolved facing is south). The knight has two
	//     real frames from its generated 4-frame walk cycle; the archer has no animation source
	//     at all, so its two cells duplicate its south idle — a frozen "toggle", disclosed in
	//     provenance.json, not a real animation. Kept the SAME grid shape for both anyway so
	//     this file has one selection path instead of an if-knight/if-archer branch.
	// Cell size is native pixel resolution (88-92px) centred with margin in a 96px cell, NOT
	// downscaled — the owner's whole complaint was units reading as low-resolution, so nothing
	// in this pipeline may resample them down. Niagara's SubUV power-of-two constraint does not
	// apply here; these sheets are sliced by UV fraction in Slate, not read by any Niagara emitter.
	constexpr int32 RetinueSheetColumns = 5;
	constexpr int32 RetinueSheetRows = 2;
	constexpr int32 RetinueSouthWalkCellA = 8; // FrameBit 0
	constexpr int32 RetinueSouthWalkCellB = 9; // FrameBit 1

	// Fallback desired size only — NativeTick overrides the SizeBox from the live body count.
	constexpr float PanelSize = 480.f; // the Unit Cam is the focal point of the HUD now

	// Soldiers stand taller than their footprint quad; draw the sprite at this multiple of
	// the projected box height, bottom-anchored on the ground point.
	constexpr float SoldierHeightScale = 3.f;

	// Virtual-camera dials. Named Emberkeep.UnitCamProj.* to sit clearly apart from the
	// capture-based cam's Emberkeep.UI.UnitCam.* (they are two different approaches).
	TAutoConsoleVariable<float> CVarProjFov(
		TEXT("Emberkeep.UnitCamProj.Fov"), 40.f,
		TEXT("Virtual-camera horizontal FOV in degrees for the projection Unit Cam."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjDist(
		TEXT("Emberkeep.UnitCamProj.Dist"), 320.f,
		TEXT("How far behind the focus (the hero) the virtual camera sits, in uu."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjHeight(
		TEXT("Emberkeep.UnitCamProj.Height"), 200.f,
		TEXT("Virtual-camera height above the focus, in uu."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjPitch(
		TEXT("Emberkeep.UnitCamProj.Pitch"), -20.f,
		TEXT("Extra tilt of the camera, in degrees, on top of the look-at. Positive angles the\n")
		TEXT("lens DOWN (more top-down); negative angles it UP toward eye level, for a\n")
		TEXT("character's-eye view of the fight ahead. 0 = look straight at the focus."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjRange(
		TEXT("Emberkeep.UnitCamProj.Range"), 1400.f,
		TEXT("Only units within this XY distance of the focus are considered for the panel. Acts\n")
		TEXT("as a FLOOR in Unit/Squad View: FrameFraction/FrameFloor may widen the EFFECTIVE range\n")
		TEXT("(and pull the camera back to match) beyond this, but never shrink below it."),
		ECVF_Default);

	// --- group-framing target (docs/design/squad-group-system.md §4.2) ------
	// "The majority of the army visible" translated into numbers: how much of the framed
	// population must fit before the shot is allowed to crop it, and the floor that overrides
	// the fraction at low counts. Protects squad cohesion by pulling the camera back (raising the
	// effective Range/Dist) rather than cropping — the bearer may drift off-centre first.
	TAutoConsoleVariable<float> CVarProjFrameFraction(
		TEXT("Emberkeep.UnitCamProj.FrameFraction"), 0.6f,
		TEXT("Minimum fraction of the framed population (the selected squad's standing, or the\n")
		TEXT("whole retinue with nothing selected) that must fit in Unit/Squad View before the\n")
		TEXT("camera is allowed to widen no further. Spec default 60% (squad-group-system.md §4.2's\n")
		TEXT("UnitCamProjector row; ViewFeed's 80% is a different panel, not this one)."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarProjFrameFloor(
		TEXT("Emberkeep.UnitCamProj.FrameFloor"), 6,
		TEXT("Body-count floor that overrides FrameFraction at low counts — a 5-soldier squad\n")
		TEXT("shows all 5, not 60% of 5 rounded down. Also the point below which the population is\n")
		TEXT("small enough that FrameFraction can't apply at all."),
		ECVF_Default);

	// --- Army View (docs/design/squad-group-system.md §4.1) ------------------
	// Placeholder centroid layout: a ring around the bearer, evenly split among the live squads.
	// USwarmSubsystem has no real per-squad centroid yet (SquadCentroidSum is proposed, not
	// built — Mass/**, out of this task's scope), so this is the disclosed stand-in.
	TAutoConsoleVariable<float> CVarProjArmyRingRadius(
		TEXT("Emberkeep.UnitCamProj.ArmyRingRadius"), 700.f,
		TEXT("World-space radius (uu) of the placeholder ring Army View arranges its <=8 squad\n")
		TEXT("blocks around the bearer on. FAKE positioning — replace with real per-squad\n")
		TEXT("centroids (SquadCentroidSum) once the Mass layer publishes them."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjArmyBlockScale(
		TEXT("Emberkeep.UnitCamProj.ArmyBlockScale"), 1.f,
		TEXT("Multiplier on Army View block size. Each block already scales with its own standing\n")
		TEXT("relative to SquadTargetSize; this is the overall bigness dial on top of that."),
		ECVF_Default);

	// --- dynamic panel size by total bodies (individual <-> mass) -----------
	TAutoConsoleVariable<float> CVarProjSizeMax(
		TEXT("Emberkeep.UnitCamProj.SizeMax"), 620.f,
		TEXT("Unit Cam panel HEIGHT (px) when the field is nearly empty — the individual is big\n")
		TEXT("and matters. The panel shrinks from here toward SizeMin as bodies pile up.\n")
		TEXT("Width = this * Aspect. Sized for the cam as the HUD's centrepiece."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjSizeMin(
		TEXT("Emberkeep.UnitCamProj.SizeMin"), 300.f,
		TEXT("Unit Cam panel height (px) at SizeBodies total bodies and beyond — the mass has\n")
		TEXT("taken over and any one soldier is a pixel."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjAspect(
		TEXT("Emberkeep.UnitCamProj.Aspect"), 1.35f,
		TEXT("Panel width as a multiple of its height. >1 gives the letterboxed viewport the\n")
		TEXT("command rectangle wants; 1 = the old square."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjSizeBodies(
		TEXT("Emberkeep.UnitCamProj.SizeBodies"), 1500.f,
		TEXT("WEIGHTED body count at which the Unit Cam reaches SizeMin. The panel scales from\n")
		TEXT("SizeMax down to SizeMin across 0..this many weighted bodies, where the weights are\n")
		TEXT("SizeRetinueWeight and SizeBroodWeight — NOT a raw headcount."),
		ECVF_Default);

	// The panel is sized by YOUR force, not by how crowded the field is. A soldier counts for
	// several brood, so a big retinue shrinks the cam (you are commanding a mass and no one
	// body matters) and losing soldiers grows it back — the cam becomes the primary view exactly
	// as the army stops being one. Weighting brood equally would invert that: a big enemy wave
	// would shrink the cam at the moment the last of your men needed watching.
	TAutoConsoleVariable<float> CVarProjSizeRetinueWeight(
		TEXT("Emberkeep.UnitCamProj.SizeRetinueWeight"), 10.f,
		TEXT("How much each of YOUR soldiers counts toward shrinking the Unit Cam. High relative\n")
		TEXT("to SizeBroodWeight on purpose: your own headcount is what should drive the panel,\n")
		TEXT("so the cam grows as you take losses regardless of how many brood are on the field."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjSizeBroodWeight(
		TEXT("Emberkeep.UnitCamProj.SizeBroodWeight"), 0.25f,
		TEXT("How much each enemy counts toward shrinking the Unit Cam. Deliberately a fraction\n")
		TEXT("of a soldier: at 1.0 a 700-strong wave alone shrinks the panel to mid-size even\n")
		TEXT("with your whole retinue dead, which is the opposite of the intent. Set 0 to make\n")
		TEXT("the panel depend purely on your own headcount."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjSizeCurve(
		TEXT("Emberkeep.UnitCamProj.SizeCurve"), 1.f,
		TEXT("Shaping exponent on the shrink ramp. 1 = linear. Above 1 holds the cam LARGE\n")
		TEXT("through the middle of the range and collapses it only once the army is near full\n")
		TEXT("strength; below 1 shrinks it early, so the last survivors produce a dramatic\n")
		TEXT("late swell. The dial for how sudden 'the cam becomes the next step' feels."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarProjThreatTint(
		TEXT("Emberkeep.UnitCamProj.ThreatTint"), 1,
		TEXT("Bleed the Unit Cam frame toward the reserved red as the brood outnumber the\n")
		TEXT("retinue — a small cam then reads as a big threat, not just a big army. 0 = off."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarProjDebugFrustum(
		TEXT("Emberkeep.UnitCamProj.DebugFrustum"), 0,
		TEXT("Draw the virtual camera in the MAIN world view: its position, FOV pyramid\n")
		TEXT("(near->far), the aim line to the focus, and the Range ring on the ground — so you\n")
		TEXT("can see where the Unit Cam is looking and what it covers. 1 = on."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjScale(
		TEXT("Emberkeep.UnitCamProj.Scale"), 1.f,
		TEXT("Multiplier on the billboard world half-size (~40uu base). Tune sprite bigness."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjSoldierScale(
		TEXT("Emberkeep.UnitCamProj.SoldierScale"), 0.75f,
		TEXT("Unit sprite size as a multiple of the projected footprint box. The main\n")
		TEXT("framing-size dial for the units themselves."),
		ECVF_Default);

	// task-050: owner feedback on the knight/archer capture — "the units they are a little
	// scaled oddly. Can they be more broad?" Diagnosis: the real cause was the PACKING, not the
	// draw math — the two sheets' cells were 96x96, but every rotation's actual alpha content
	// only measures ~23-46px per side (PixelLab's "low top-down" characters ship with a large,
	// roughly-symmetric transparent margin on all sides), so ~50%+ of every cell was blank
	// padding drawn at the same on-screen size as the content. Fixed at the source: rev 5's
	// pack_fullcolor_retinue.py cells are 56x60, sized to the measured content with a real
	// margin, not the source canvas — see RawArt/Renders/knight and archer-proxy provenance.
	// This dial is NOT a patch over that packing bug (a dial compensating for a mistake would
	// be worse than fixing it, and the mistake is fixed) — it is a live width-only knob left in
	// place because "more broad" is a taste call, not just a bug, and the owner should be able
	// to keep tuning it without a rebuild. Multiplies DrawW only, after Aspect: height and the
	// foot anchor are untouched, so widening never changes how tall a unit reads or where it
	// stands. Above ~1.2 this starts visibly stretching already-packed pixel art sideways
	// rather than just filling more of the cell, since (unlike the packing fix) it IS a resize,
	// done live at draw time on an already-fixed sheet — worth knowing before pushing it far.
	TAutoConsoleVariable<float> CVarProjSoldierAspect(
		TEXT("Emberkeep.UnitCamProj.SoldierAspect"), 1.f,
		TEXT("Width-only multiplier on every billboard (brood, retinue, hero alike), applied\n")
		TEXT("after the sheet's own cell aspect. >1 = broader, <1 = narrower. 1 = exactly as\n")
		TEXT("packed. This is a LIVE STRETCH, not a repack — past ~1.2 it reads as distortion,\n")
		TEXT("not as 'more content filling the box'. If a sheet's own padding is the problem,\n")
		TEXT("fix the packing (tighter cell) first; use this for taste on top of that."),
		ECVF_Default);

	// SoldierScale sizes every body at once; this is the per-team split. The panel is where
	// the size difference between the tide and your line is actually legible — in the world
	// view they are boxes seen from above — so a horde size change has to be answerable here
	// too or the two views disagree about what a brood is.
	TAutoConsoleVariable<float> CVarProjBroodScale(
		TEXT("Emberkeep.UnitCamProj.BroodScale"), 1.f,
		TEXT("Brood billboard size as a multiple of a soldier's, in the Unit Cam panel only.\n")
		TEXT("The panel counterpart to Swarm.BroodSize; mirrors HeroScale. 1 = same size as\n")
		TEXT("your soldiers. Set alongside Swarm.BroodSize so the world and the panel agree."),
		ECVF_Default);

	// The projected point for a body is its GROUND position — the sim is 2D, every entity's
	// transform sits on the floor plane. So a sprite centred on that point is drawn half
	// buried, and every size multiplier (SoldierScale, HeroScale, BroodScale, the size roll)
	// grows it downward through the floor exactly as much as upward. Anchoring by the feet
	// is what makes size a one-directional thing: a bigger unit is TALLER, not deeper.
	//
	// A dial rather than a hard switch because 1.0 re-frames the whole panel upward by half
	// a body — the Pitch/Height/Dist shot was composed against the old half-sunk look, and
	// 0 reproduces it exactly for an A/B.
	TAutoConsoleVariable<float> CVarProjFootAnchor(
		TEXT("Emberkeep.UnitCamProj.FootAnchor"), 1.f,
		TEXT("Where a body's sprite sits relative to its projected ground point.\n")
		TEXT("1 = standing ON it (feet planted, grows upward only) — the correct read.\n")
		TEXT("0 = centred on it, the pre-2026-07-26 look, where scaling sinks a unit through\n")
		TEXT("the floor. Values between slide the anchor. Raising this lifts every body half\n")
		TEXT("its height up the panel, so expect to re-check Pitch/Height after."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjNearFade(
		TEXT("Emberkeep.UnitCamProj.NearFade"), 150.f,
		TEXT("Depth band (uu) just in front of the near plane over which a unit fades in\n")
		TEXT("instead of popping. A unit at the near plane is transparent; NearFade uu deeper\n")
		TEXT("it is fully solid. 0 = hard pop (old behaviour)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjNearPlane(
		TEXT("Emberkeep.UnitCamProj.NearPlane"), 10.f,
		TEXT("Near clip distance of the fake camera, in uu. Units closer than this are\n")
		TEXT("dropped; NearFade is the fade band just beyond it."),
		ECVF_Default);

	// --- hero proxy (the bearer, drawn in the panel) ------------------------
	TAutoConsoleVariable<int32> CVarProjHero(
		TEXT("Emberkeep.UnitCamProj.Hero"), 1,
		TEXT("Draw the bearer himself in the Unit Cam. He is a pawn, not a Mass entity, so he\n")
		TEXT("is not in the render buffers — this injects one extra billboard at his location.\n")
		TEXT("0 = off (the panel shows only the swarm, as before)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjHeroScale(
		TEXT("Emberkeep.UnitCamProj.HeroScale"), 1.6f,
		TEXT("Hero billboard size as a multiple of a soldier's. He is the subject of the shot,\n")
		TEXT("so he reads bigger than the retinue around him."),
		ECVF_Default);

	// Cell index below is into T_Hero_Vanguard's OWN 4x4 grid now (see HeroSheetColumns/Rows
	// above) — the hero no longer shares SwarmAtlas at all (task-050). The real sprite this
	// dial used to be a placeholder FOR now exists; the dial survives as "which of its 16
	// cells" rather than being retired, since the sheet still has an idle+walk1 pair per
	// direction and nothing currently animates the hero's pose or turns him to face the cam.

	TAutoConsoleVariable<int32> CVarProjHeroCell(
		TEXT("Emberkeep.UnitCamProj.HeroCell"), 0,
		TEXT("Which cell of T_Hero_Vanguard's own 4x4 sheet the hero draws from (0-15).\n")
		TEXT("Defaults to 0 = south idle — a portrait pose turned toward the viewer, matching\n")
		TEXT("what the old shared-atlas default (retinue walk0 facing south) was standing in\n")
		TEXT("for. Odd cells are each direction's walk1 frame; see the doc comment on\n")
		TEXT("HeroSheetColumns/Rows in the anonymous namespace above for the direction order\n")
		TEXT("and why this does not yet resolve per-view like the retinue variants do."),
		ECVF_Default);

	// --- high-resolution retinue: knight + archer (task-050) ------------------
	TAutoConsoleVariable<int32> CVarProjSoldierVariants(
		TEXT("Emberkeep.UnitCamProj.SoldierVariants"), 1,
		TEXT("Draw retinue billboards from the two owner-chosen high-resolution units\n")
		TEXT("(T_Soldier_Knight, T_Soldier_Archer) instead of one shared cell of T_Swarm_2bit\n")
		TEXT("for every soldier. 0 = old behaviour (A/B against this change, or a fallback if a\n")
		TEXT("texture is missing from Content). Brood are unaffected either way — this dial is\n")
		TEXT("retinue-only. Name kept from the six-variant era (task-050 rev 1) rather than\n")
		TEXT("renamed, since it still means exactly the same thing: 'is retinue high-res or not'."),
		ECVF_Default);

	// RETIRED (task-046): retinue sprite choice now follows the real Spearmen/Archers type
	// (FSwarmAnimFragment::SquadId, decoded via SwarmSquad::UnitType — see the sprite-
	// selection block below), not a hash. This CVar is left registered, inert, only so an
	// old exec file or saved preset that still names it doesn't error; it is no longer read
	// anywhere in this file. PickSoldierLook, which used to compute the hash, is deleted —
	// see its old doc comment (git history) for why SquadId/SlotIndex used to be the WRONG
	// thing to key this on: both renumbered on any casualty anywhere before this task's
	// sticky-SquadId fix (docs/design/squad-group-system.md §1.3) landed.
	TAutoConsoleVariable<float> CVarProjArcherFraction(
		TEXT("Emberkeep.UnitCamProj.ArcherFraction"), 0.35f,
		TEXT("RETIRED, inert — see the comment above this CVar's registration in\n")
		TEXT("UnitCamProjector.cpp. Sprite choice now follows the real per-soldier type."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarProjBroodTint(
		TEXT("Emberkeep.UnitCamProj.BroodTint"), 0,
		TEXT("Multiply the reserved red through the brood sprites in the Unit Cam. Before they\n")
		TEXT("had art the brood WERE that red — it was the only thing telling them from the\n")
		TEXT("retinue. Now the silhouette does that job and the panel matches the world view,\n")
		TEXT("so this defaults OFF; set 1 to get the old high-contrast threat read back."),
		ECVF_Default);

	// --- panel lighting (see the block comment in the projection loop) -------

	TAutoConsoleVariable<int32> CVarProjDirShade(
		TEXT("Emberkeep.UnitCamProj.DirShade"), 1,
		TEXT("Shade each unit by WHICH SIDE of it the lens can see. The world renderer splits a\n")
		TEXT("unit into a flame-lit half and a Swarm.UnitBackShade half; a billboard can't be\n")
		TEXT("split, so this resolves the same geometry against the VIRTUAL CAMERA instead: a\n")
		TEXT("unit advancing on the bearer shows its lit face, one between the lens and the\n")
		TEXT("flame is a backlit silhouette. This is what makes the panel read as a close-up of\n")
		TEXT("bodies walking INTO the light rather than a flat grey crowd. 0 = distance only."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjBroodFloor(
		TEXT("Emberkeep.UnitCamProj.BroodFloor"), 0.05f,
		TEXT("Minimum brightness for BROOD, replacing Swarm.UnitLightFloor in this panel only.\n")
		TEXT("Deliberately far below it: the shared floor (0.28) exists so units never vanish at\n")
		TEXT("gameplay zoom, but in a close-up it pins every distant brood at one flat mid-grey —\n")
		TEXT("they read as fog, not as something coming out of the dark. Their own floor lets them\n")
		TEXT("start near-black at the edge and be LIFTED by the approach, which is the whole shot,\n")
		TEXT("and it matches the world's deliberate 'brood sit low in the value range, the flame\n")
		TEXT("lifts them only as they close' rule (SwarmRenderActor.cpp). Retinue keep the floor."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjBroodCeil(
		TEXT("Emberkeep.UnitCamProj.BroodCeil"), 0.7f,
		TEXT("Brightest a BROOD is ever drawn, even standing in the flame. Below 1 on purpose, and\n")
		TEXT("the direct fix for brood reading as flat grey up close: the atlas authors them as\n")
		TEXT("mid-grey hooded figures, and a Slate tint can only ever darken the art (the tint is\n")
		TEXT("packed to an 8-bit vertex colour and clamped, so there is no over-brightening to be\n")
		TEXT("had here — the sprite as authored IS the ceiling). Holding their ceiling down instead\n")
		TEXT("keeps them below the retinue in value at every distance, so a soldier beside a brood\n")
		TEXT("always reads as the lit one. Mirrors the world's rule that brood sit low in the value\n")
		TEXT("range and the flame only lifts them as they close (SwarmRenderActor.cpp).\n")
		TEXT("1 = brood may reach full sprite brightness (the old flat-grey look)."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarProjLightSteps(
		TEXT("Emberkeep.UnitCamProj.LightSteps"), 5,
		TEXT("Quantize the light into this many discrete tiers. The panel is UMG — it draws AFTER\n")
		TEXT("the demichrome pass, so nothing downstream posterises it and a continuous multiplier\n")
		TEXT("smears the 2-bit art across every intermediate grey there is. Note this steps the\n")
		TEXT("LIGHT, not the pixels: snapping the sprite itself to palette entries is exactly what\n")
		TEXT("collapsed the old SceneCapture close-up to one flat value (docs/UNIT-CAM-HANDOFF.md\n")
		TEXT("wall #3), so the body keeps its internal values and only its lighting tier is banded.\n")
		TEXT("0 or 1 = continuous (smooth, off-style). 4-6 reads as 2-bit."),
		ECVF_Default);

	// --- full-colour retinue lighting (task-050) ------------------------------
	// LightFloor/BroodFloor above were tuned against FLAT 4-value sprites, which have no
	// internal shadow of their own to lose — dimming one just steps it down a palette rung and
	// it stays legible. The knight and archer are full-colour PixelLab renders with their OWN
	// hood shadows and armour recesses already baked in; multiplying that by the SAME model
	// compounds two darkenings into one (source shadow x panel tint), and measured captures
	// showed it crushing toward near-black well short of the source's real saturation — see
	// docs/data/art/provenance.json's in_panel_lighting_finding. These two CVars scope a
	// gentler model to JUST the knight/archer sprite sets (PickSoldierLook's output) — brood,
	// the hero, and everything the world renderer draws are computed exactly as before,
	// untouched, because their art was never the problem.
	//
	// Two independent knobs, not one, because a floor alone can't fix this: raising ONLY the
	// floor (like BroodFloor above) still lets a unit slide from bright to dim across its
	// whole visible range, and a full-colour body slides through a lot more visible detail on
	// the way down than a flat one did. FullColorDimStrength instead scales back HOW FAR the
	// existing distance/facing falloff (Atten, computed once above and shared with brood) is
	// allowed to pull a body down from full brightness, before the floor even applies —
	// preserving the FALLOFF'S SHAPE (a unit further from the bearer or facing away still
	// reads dimmer than one close and lit; the flame still means something) while shrinking
	// its amplitude. 1 = the shared falloff, full strength, same as brood; 0 = full colour
	// exempt entirely (always max brightness — NOT the goal, kills the flame read for these
	// two units, don't set this). FullColorFloor is the same kind of floor as LightFloor/
	// BroodFloor, just a separate, higher number so it doesn't also raise brood or retire the
	// shared retinue floor other sprite sets still use.
	TAutoConsoleVariable<float> CVarProjFullColorFloor(
		TEXT("Emberkeep.UnitCamProj.FullColorFloor"), 0.55f,
		TEXT("Minimum brightness for the knight/archer sprite sets only (Swarm.UnitLightFloor's\n")
		TEXT("0.28 still applies to brood, hero and anything else). Chosen by looking at a\n")
		TEXT("typical-distance capture, not guessed — 0.28 crushed full-colour detail toward\n")
		TEXT("black; 0.55 keeps a unit at the edge of the pool clearly a lit colour, not a\n")
		TEXT("silhouette. Combine with FullColorDimStrength below, which controls how much of\n")
		TEXT("the gap between this floor and full brightness the flame falloff actually uses."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjFullColorDimStrength(
		TEXT("Emberkeep.UnitCamProj.FullColorDimStrength"), 0.5f,
		TEXT("How much of the shared distance/facing falloff (Atten) is applied to the knight/\n")
		TEXT("archer sprite sets before FullColorFloor takes over. 1 = full strength, identical\n")
		TEXT("proportional dimming to brood/retinue. 0 = falloff has no effect at all (constant\n")
		TEXT("max brightness) — DON'T use 0, it deletes the flame-distance read for these two\n")
		TEXT("units, which is the one thing this change must not do. 0.5 (default) means a body\n")
		TEXT("at the far edge of the flame's radius sits roughly halfway between\n")
		TEXT("FullColorFloor and full brightness rather than pinned at the floor — dimmer=\n")
		TEXT("further still reads, it just never reads as crushed."),
		ECVF_Default);

	// Read the live flame falloff so the panel shades units the same way the world does,
	// rather than duplicating the numbers. Flame position ~= the attractor for the
	// prototype (the smoothed spring pos lives privately on ASwarmRenderActor).
	float ReadCVarFloat(const TCHAR* Name, float Fallback)
	{
		if (const IConsoleVariable* CV = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return CV->GetFloat();
		}
		return Fallback;
	}

	// Same base albedos as the debug renderer's shaded path (SwarmRenderActor.cpp).
	const FColor RetinueAlbedo(232, 232, 238);
	const FColor BroodAlbedo(170, 44, 36);

	/**
	 * Real per-soldier look: TYPE decoded straight from the render buffer's squad byte
	 * (SwarmRenderPack::Squad -> SwarmSquad::UnitType), now that task-046 made SquadId+Type a
	 * permanent, sticky per-soldier tag (docs/design/squad-group-system.md §1.3, §1.5)
	 * instead of something re-derived from a repackable formation slot index. A soldier
	 * draws from the Spearmen list because it IS a spearman, and the Archer list because it
	 * IS an archer — that part retires the old SizeBucket-hash-decides-EVERYTHING workaround
	 * (PickSoldierLook, task-050 rev 2) that existed only because SquadId used to be unsafe
	 * to key anything on.
	 *
	 * STATE is where that hash comes back, repurposed rather than deleted (owner: "variants
	 * I mean states... variety of the character type"): SwarmRenderPack::SizeBucket, stable
	 * per soldier for the same reason PickSoldierLook relied on it (fixed at spawn from
	 * FSwarmJitterFragment::Phase, never touched again), re-hashed by a DIFFERENT irrational
	 * than the size roll so state doesn't correlate with physical size, then folded into
	 * however many states THIS soldier's type actually has loaded. With one state per type
	 * (today), NumStates is always 1 and every hash maps to state 0 — the mechanism is live,
	 * the variety just isn't authored yet. Adding a second state to SpearmenStatePaths above
	 * is the entire cost of turning it on.
	 */
	uint8 SpriteSetForSoldier(int32 PackedAnimBits, int32 NumSpearmenStates, int32 NumArcherStates)
	{
		const EUnitType Type = SwarmSquad::UnitType(SwarmRenderPack::Squad(PackedAnimBits));
		const int32 NumStates = FMath::Max(Type == EUnitType::Archers ? NumArcherStates : NumSpearmenStates, 1);

		const int32 Bucket = SwarmRenderPack::SizeBucket(PackedAnimBits); // 0-15, stable per soldier
		const float U = FMath::Frac((float)Bucket * 1.41421356f); // sqrt(2) -- not the size roll's golden ratio
		const int32 StateIdx = FMath::Clamp((int32)(U * (float)NumStates), 0, NumStates - 1);

		const uint8 SpearmenBase = UnitCamSprite::SpearmenStateBase;
		const uint8 ArcherBase = SpearmenBase + (uint8)FMath::Max(NumSpearmenStates, 0);
		return (Type == EUnitType::Archers ? ArcherBase : SpearmenBase) + (uint8)StateIdx;
	}

	/** Slice Tex into Columns x Rows cells of one FSlateBrush each, UV-mapped like the existing
	 *  SwarmAtlas slicing did. Empty (not a null-filled array) if Tex is null, so a missing
	 *  texture degrades to SUnitCamCanvas's existing untextured-quad fallback rather than a
	 *  crash or a wrong-sized array. */
	TArray<FSlateBrush> BuildBrushSet(UTexture2D* Tex, int32 Columns, int32 Rows)
	{
		TArray<FSlateBrush> Brushes;
		if (!Tex || Columns <= 0 || Rows <= 0)
		{
			return Brushes;
		}
		const float CellW = 1.f / (float)Columns;
		const float CellH = 1.f / (float)Rows;
		Brushes.Reserve(Columns * Rows);
		for (int32 Cell = 0; Cell < Columns * Rows; ++Cell)
		{
			const float U = (Cell % Columns) * CellW;
			const float V = (Cell / Columns) * CellH;
			FSlateBrush Brush;
			Brush.SetResourceObject(Tex);
			Brush.ImageSize = FVector2D(Tex->GetSizeX() * CellW, Tex->GetSizeY() * CellH);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.SetUVRegion(FBox2f(FVector2f(U, V), FVector2f(U + CellW, V + CellH)));
			Brushes.Add(MoveTemp(Brush));
		}
		return Brushes;
	}
}

// ---------------------------------------------------------------------------
// SUnitCamCanvas — the leaf that blits pre-projected billboards. Draws only POD;
// all UObject access happened up in UUnitCamProjector::NativeTick.
// ---------------------------------------------------------------------------
class SUnitCamCanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUnitCamCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& /*InArgs*/) {}

	void SetBillboards(TArray<FUnitCamBillboard>&& InBillboards)
	{
		Billboards = MoveTemp(InBillboards);
	}

	void SetBrushSets(TArray<TArray<FSlateBrush>>&& InSets)
	{
		BrushSets = MoveTemp(InSets);
	}

	void SetSoldierScale(float InScale) { SoldierScale = InScale; }

	void SetHeroScale(float InScale) { HeroScale = InScale; }

	void SetSoldierAspect(float InAspect) { SoldierAspect = InAspect; }

	void SetFootAnchor(float InAnchor) { FootAnchor = InAnchor; }

	void SetShowReticle(bool bInShow) { bShowReticle = bInShow; }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FSlateColorBrush Brush(FLinearColor::White); // tinted per element below
		const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());

		// Fraction of a body's drawn height that sits ABOVE its projected point: 0.5 centres
		// the sprite on it, 1.0 stands the sprite on it. Computed once, applied to every body
		// including the bearer, so nothing can be anchored differently from its neighbours.
		const float AnchorY = 0.5f + 0.5f * FMath::Clamp(FootAnchor, 0.f, 1.f);

		// The dark world behind the units — heavy midnight, never pure black.
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			&Brush, ESlateDrawEffect::None, Demichrome::Dark());

		// Billboards, already sorted far->near by the projector (painter's order).
		for (const FUnitCamBillboard& B : Billboards)
		{
			const float Half = B.HalfSize * Size.X;
			const FVector2f Centre(B.Center.X * Size.X, B.Center.Y * Size.Y);

			// Every body — brood, retinue, bearer — draws the atlas cell the sim picked for it,
			// FOOT-anchored on the projected point (that point is the unit's ground contact) and
			// sized to the cell's aspect. B.Color is a light-only tint (flame distance dims the
			// sprite). The bearer is scaled up and sits a layer above the swarm so he is never
			// lost behind whoever is in front of him.
			const TArray<FSlateBrush>* Brushes = BrushSets.IsValidIndex(B.SpriteSet) ? &BrushSets[B.SpriteSet] : nullptr;
			if (Brushes && Brushes->IsValidIndex(B.Cell))
			{
				const FSlateBrush& CellBrush = (*Brushes)[B.Cell];
				const float ImgX = (float)CellBrush.ImageSize.X;
				const float ImgY = (float)CellBrush.ImageSize.Y;
				const float Aspect = ImgY > 0.f ? ImgX / ImgY : 1.f;
				const float DrawH = 2.f * Half * SoldierScale * (B.bHero ? HeroScale : 1.f);
				// SoldierAspect: a live width-only stretch on top of the cell's own aspect — see
				// CVarProjSoldierAspect's doc comment for why this is a taste dial layered on a
				// packing fix, not a substitute for one.
				const float DrawW = DrawH * Aspect * SoldierAspect;
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + (B.bHero ? 2 : 1),
					AllottedGeometry.ToPaintGeometry(
						FVector2f(DrawW, DrawH),
						FSlateLayoutTransform(FVector2f(Centre.X - DrawW * 0.5f, Centre.Y - DrawH * AnchorY))),
					&CellBrush, ESlateDrawEffect::None, B.Color);
				continue;
			}

			// Untextured fallback (atlas still loading). Anchored the same way, or a body
			// would jump vertically the frame its brush becomes available.
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(2.f * Half, 2.f * Half),
					FSlateLayoutTransform(FVector2f(Centre.X - Half, Centre.Y - 2.f * Half * AnchorY))),
				&Brush, ESlateDrawEffect::None, B.Color);

			// Army View blocks carry a live-count label; nothing else sets one.
			if (!B.Label.IsEmpty())
			{
				const FVector2f LabelSize(48.f, 14.f);
				const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 9);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
					AllottedGeometry.ToPaintGeometry(LabelSize,
						FSlateLayoutTransform(Centre - LabelSize * 0.5f)),
					FText::FromString(B.Label), Font, ESlateDrawEffect::None, Demichrome::Pale());
			}
		}

		// Focus reticle (panel centre = where the virtual camera is aimed), so orbiting the Yaw
		// dial reads clearly as the world turning around the hero. Hidden in Army View: that mode
		// has no perspective camera to mark a reticle for (see SetShowReticle's doc comment).
		if (bShowReticle)
		{
			const FVector2f C = Size * 0.5f;
			const float R = 5.f;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(2.f * R, 1.f), FSlateLayoutTransform(C - FVector2f(R, 0.f))),
				&Brush, ESlateDrawEffect::None, Demichrome::Steel());
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(1.f, 2.f * R), FSlateLayoutTransform(C - FVector2f(0.f, R))),
				&Brush, ESlateDrawEffect::None, Demichrome::Steel());
		}

		return LayerId + 3;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(PanelSize, PanelSize);
	}

private:
	TArray<FUnitCamBillboard> Billboards;
	TArray<TArray<FSlateBrush>> BrushSets; // one array per sprite sheet; empty until textures load
	float SoldierScale = SoldierHeightScale;
	float HeroScale = 1.6f;
	float SoldierAspect = 1.f; // live width-only stretch on top of the cell's own aspect
	float FootAnchor = 1.f;   // 1 = bodies stand on their projected ground point
	bool bShowReticle = true; // off in Army View — see SetShowReticle
};

// ---------------------------------------------------------------------------
// UUnitCamCanvasWidget
// ---------------------------------------------------------------------------
void UUnitCamCanvasWidget::SetBillboards(TArray<FUnitCamBillboard>&& InBillboards)
{
	if (Canvas.IsValid())
	{
		Canvas->SetBillboards(MoveTemp(InBillboards));
	}
}

void UUnitCamCanvasWidget::SetBrushSets(TArray<TArray<FSlateBrush>>&& InSets)
{
	if (Canvas.IsValid())
	{
		Canvas->SetBrushSets(MoveTemp(InSets));
	}
}

void UUnitCamCanvasWidget::SetSoldierScale(float InScale)
{
	if (Canvas.IsValid())
	{
		Canvas->SetSoldierScale(InScale);
	}
}

void UUnitCamCanvasWidget::SetHeroScale(float InScale)
{
	if (Canvas.IsValid())
	{
		Canvas->SetHeroScale(InScale);
	}
}

void UUnitCamCanvasWidget::SetSoldierAspect(float InAspect)
{
	if (Canvas.IsValid())
	{
		Canvas->SetSoldierAspect(InAspect);
	}
}

void UUnitCamCanvasWidget::SetFootAnchor(float InAnchor)
{
	if (Canvas.IsValid())
	{
		Canvas->SetFootAnchor(InAnchor);
	}
}

void UUnitCamCanvasWidget::SetShowReticle(bool bInShow)
{
	if (Canvas.IsValid())
	{
		Canvas->SetShowReticle(bInShow);
	}
}

TSharedRef<SWidget> UUnitCamCanvasWidget::RebuildWidget()
{
	Canvas = SNew(SUnitCamCanvas);
	return Canvas.ToSharedRef();
}

void UUnitCamCanvasWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Canvas.Reset();
}

// ---------------------------------------------------------------------------
// UUnitCamProjector
// ---------------------------------------------------------------------------
void UUnitCamProjector::SetHostSized(bool bInHostSized)
{
	bHostSized = bInHostSized;
	if (bHostSized && RootBox)
	{
		// Drop the overrides once, here — NativeTick stops writing them, so a stale override
		// would otherwise pin the widget at whatever size it happened to be on the switch.
		RootBox->ClearWidthOverride();
		RootBox->ClearHeightOverride();
	}
}

void UUnitCamProjector::SetFrameThickness(float InPx)
{
	FrameThickness = FMath::Max(InPx, 0.f);
	if (FrameBorder)
	{
		FrameBorder->SetPadding(FMargin(FrameThickness));
	}
}

TSharedRef<SWidget> UUnitCamProjector::RebuildWidget()
{
	// The framed panel is the ROOT so this widget can be EMBEDDED — the combat HUD hosts
	// it in the band's right bookend as the default Unit Cam (UEmberkeepHud::RebuildBand),
	// which is why it shows in PIE without a console command and can't be occluded by the
	// HUD. Standalone via AddToViewport (the Emberkeep.UI.UnitCamProj toggle) it just lands
	// top-left — that path is now only for isolated testing.
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box"));
	WidgetTree->RootWidget = Box;
	Box->SetWidthOverride(PanelSize);
	Box->SetHeightOverride(PanelSize);
	RootBox = Box; // NativeTick resizes it by total body count

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
	Frame->SetPadding(FMargin(FrameThickness));    // boxy standalone; a hairline when embedded
	Frame->SetBrushColor(Demichrome::Steel());
	Box->SetContent(Frame);
	FrameBorder = Frame; // NativeTick tints it toward red as the tide outnumbers the host

	UOverlay* Inner = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Inner"));
	Frame->SetContent(Inner);

	CanvasWidget = WidgetTree->ConstructWidget<UUnitCamCanvasWidget>(UUnitCamCanvasWidget::StaticClass(), TEXT("Canvas"));
	if (UOverlaySlot* CanvasSlot = Inner->AddChildToOverlay(CanvasWidget))
	{
		CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
		CanvasSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* Tag = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Tag"));
	Tag->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	Tag->SetPadding(FMargin(4.f, 1.f));
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
	Label->SetText(FText::FromString(TEXT("UNIT CAM · proj")));
	Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 7));
	Label->SetColorAndOpacity(FSlateColor(Demichrome::Pale()));
	Tag->SetContent(Label);
	if (UOverlaySlot* TagSlot = Inner->AddChildToOverlay(Tag))
	{
		TagSlot->SetHorizontalAlignment(HAlign_Left);
		TagSlot->SetVerticalAlignment(VAlign_Top);
		TagSlot->SetPadding(FMargin(4.f));
	}

	return Super::RebuildWidget();
}

void UUnitCamProjector::BuildArmyView(const USwarmSubsystem& Swarm)
{
	// task-046: block position (SquadCentroidSum), per-block stance tint (UnitStance), and
	// type (SquadType) are all real now — see the class-header doc comment for what's still
	// a simplification (no yaw/camera-facing alignment on this fixed top-down layout yet).
	TArray<FUnitCamBillboard> Out;

	int32 LiveSquads = 0;
	for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
	{
		if (Swarm.GetSquadStanding(i) > 0) { ++LiveSquads; }
	}
	if (LiveSquads == 0)
	{
		CanvasWidget->SetBillboards({});
		return;
	}

	// Value-only stance tint, on-ramp (Demichrome is grayscale by design — no new hue). Real
	// per-unit stance now (task-046's UnitStance[8]) — each block tints by ITS OWN unit's
	// order, not the one global stance every block used to share.
	auto TintForStance = [](ESwarmStance Stance) -> FLinearColor
	{
		switch (Stance)
		{
		case ESwarmStance::Charge: return FMath::Lerp(Demichrome::Steel(), Demichrome::Pale(), 0.6f);
		case ESwarmStance::Hold:   return FMath::Lerp(Demichrome::Steel(), Demichrome::Dark(), 0.5f);
		default:                   return Demichrome::Steel(); // Follow, Rally — neutral
		}
	};

	const float RingFrac = 0.42f; // block layout radius as a fraction of the panel's half-size
	const float BlockScale = FMath::Max(CVarProjArmyBlockScale.GetValueOnGameThread(), 0.05f);
	const float RefSize = FMath::Max((float)USwarmSubsystem::SquadTargetSize, 1.f);

	// Real per-unit centroid (task-046's SquadCentroidSum), not a fake evenly-spaced ring —
	// a fixed top-down layout around the bearer, world offset scaled so a unit sitting at the
	// leash radius lands at the same visual radius the old placeholder ring drew at. No
	// yaw/camera-facing alignment yet (this mode has no "up-screen is forward" convention of
	// its own established) — flagged as a follow-up, not required for what this block needs
	// to show (real position, real stance, real type).
	const FVector HeroPos = Swarm.GetAttractor();
	const float WorldToPanel = RingFrac / FMath::Max(SwarmLeash::Radius, 1.f);

	for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
	{
		const int32 Standing = Swarm.GetSquadStanding(i);
		if (Standing <= 0) { continue; }

		const FVector Centroid = Swarm.GetSquadCentroid(i);
		const FVector2f PanelOffset(
			FMath::Clamp((float)(Centroid.X - HeroPos.X) * WorldToPanel, -RingFrac, RingFrac),
			FMath::Clamp((float)(Centroid.Y - HeroPos.Y) * WorldToPanel, -RingFrac, RingFrac));

		FUnitCamBillboard B;
		B.Center = FVector2f(0.5f + PanelOffset.X, 0.5f + PanelOffset.Y);
		const float Fill = FMath::Clamp((float)Standing / RefSize, 0.f, 1.f); // understrength reads smaller
		B.HalfSize = FMath::Lerp(0.05f, 0.16f, Fill) * BlockScale;
		B.Depth = 0.f;
		B.Color = TintForStance(Swarm.GetUnitStance(i));
		B.Cell = INDEX_NONE; // flat block — no atlas sprite, this is an aggregate icon, not a soldier
		// §5.4's block_type_marker: "S·34" / "A·12" — which blocks are Spearmen vs Archers is
		// now a real question Army View has to answer (both types share the same 8-block budget).
		const TCHAR* TypeLetter = (Swarm.GetSquadType(i) == EUnitType::Archers) ? TEXT("A") : TEXT("S");
		B.Label = FString::Printf(TEXT("%s·%d"), TypeLetter, Standing);
		Out.Add(MoveTemp(B));
	}

	CanvasWidget->SetBillboards(MoveTemp(Out));
}

void UUnitCamProjector::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (!CanvasWidget)
	{
		return;
	}

	// Unit sprites: the shared brood atlas plus, since task-050, one dedicated texture each for
	// the bearer, the knight and the archer. All loaded once from Content on first tick;
	// SwarmAtlas stays the source for brood (and for retinue if SoldierVariants is switched
	// off) so the panel and the Niagara bridge can never show a brood in two different poses.
	if (!SwarmAtlas && !bAtlasLoadAttempted)
	{
		bAtlasLoadAttempted = true;
		SwarmAtlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Spike1/T_Swarm_2bit.T_Swarm_2bit"));
		if (!SwarmAtlas)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UnitCamProj: T_Swarm_2bit not found in Content — units fall back to quads."));
		}
		HeroTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Sprites/Heroes/T_Hero_Vanguard.T_Hero_Vanguard"));
		if (!HeroTexture)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UnitCamProj: T_Hero_Vanguard not found in Content — hero falls back to a quad."));
		}

		// Type/state textures (task-046): every path in SpearmenStatePaths()/ArcherStatePaths()
		// is loaded and held, in order — element 0 is task-050's original, later elements are
		// additional PixelLab "states" of the same character group once packed. A missing path
		// logs and is skipped rather than aborting the rest.
		auto LoadStates = [](const TArray<FString>& Paths, TArray<TObjectPtr<UTexture2D>>& OutTextures, const TCHAR* TypeLabel)
		{
			for (const FString& Path : Paths)
			{
				UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);
				if (!Tex)
				{
					UE_LOG(LogTemp, Warning, TEXT("UnitCamProj: %s state '%s' not found in Content — skipped."), TypeLabel, *Path);
					continue;
				}
				OutTextures.Add(Tex);
			}
		};
		LoadStates(SpearmenStatePaths(), SpearmenStateTextures, TEXT("Spearmen"));
		LoadStates(ArcherStatePaths(), ArcherStateTextures, TEXT("Archer"));
	}
	if (bAtlasLoadAttempted && !bBrushSetsPushed)
	{
		// Grid for SwarmAtlas comes from SwarmSheet so the panel's UVs can't drift from the
		// Niagara bridge's; the rest use their own sheet's own grid (see the doc comments on
		// HeroSheetColumns/Rows and RetinueSheetColumns/Rows above). Built once — the slices
		// never change — even if a texture is still missing, so a late-loading asset doesn't
		// retry every frame; SUnitCamCanvas falls back to an untextured quad per-body regardless.
		bBrushSetsPushed = true;
		NumSpearmenStatesLoaded = SpearmenStateTextures.Num();
		NumArcherStatesLoaded = ArcherStateTextures.Num();

		TArray<TArray<FSlateBrush>> Sets;
		Sets.SetNum(UnitCamSprite::SpearmenStateBase + NumSpearmenStatesLoaded + NumArcherStatesLoaded);
		Sets[UnitCamSprite::SwarmAtlas] = BuildBrushSet(SwarmAtlas, SwarmSheet::Columns, SwarmSheet::Rows);
		Sets[UnitCamSprite::Hero] = BuildBrushSet(HeroTexture, HeroSheetColumns, HeroSheetRows);
		for (int32 i = 0; i < NumSpearmenStatesLoaded; ++i)
		{
			Sets[UnitCamSprite::SpearmenStateBase + i] = BuildBrushSet(SpearmenStateTextures[i], RetinueSheetColumns, RetinueSheetRows);
		}
		const uint8 ArcherBase = UnitCamSprite::SpearmenStateBase + (uint8)NumSpearmenStatesLoaded;
		for (int32 i = 0; i < NumArcherStatesLoaded; ++i)
		{
			Sets[ArcherBase + i] = BuildBrushSet(ArcherStateTextures[i], RetinueSheetColumns, RetinueSheetRows);
		}
		CanvasWidget->SetBrushSets(MoveTemp(Sets));
	}
	CanvasWidget->SetSoldierScale(CVarProjSoldierScale.GetValueOnGameThread());
	CanvasWidget->SetHeroScale(FMath::Max(CVarProjHeroScale.GetValueOnGameThread(), 0.f));
	CanvasWidget->SetSoldierAspect(FMath::Max(CVarProjSoldierAspect.GetValueOnGameThread(), 0.1f));
	CanvasWidget->SetFootAnchor(CVarProjFootAnchor.GetValueOnGameThread());

	const UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		CanvasWidget->SetBillboards({});
		return;
	}

	// Dynamic panel size + threat tint: the more bodies on the field, the smaller the
	// individual view; the more the brood outnumber the host, the redder the frame.
	if (RootBox)
	{
		const int32 Retinue = Swarm->GetAliveRetinue();
		const int32 Brood = Swarm->GetAliveBrood();

		// Weighted, not a headcount: your soldiers drive the framing and the tide only nudges it,
		// so attrition — the thing the run is actually about — is what grows the cam.
		const float Weighted =
			(float)Retinue * CVarProjSizeRetinueWeight.GetValueOnGameThread() +
			(float)Brood * CVarProjSizeBroodWeight.GetValueOnGameThread();
		const float Full = FMath::Max(CVarProjSizeBodies.GetValueOnGameThread(), 1.f);
		float T = FMath::Clamp(Weighted / Full, 0.f, 1.f);

		const float Curve = CVarProjSizeCurve.GetValueOnGameThread();
		if (Curve > 0.f && !FMath::IsNearlyEqual(Curve, 1.f))
		{
			T = FMath::Pow(T, Curve);
		}
		const float Height = FMath::Lerp(
			CVarProjSizeMax.GetValueOnGameThread(), CVarProjSizeMin.GetValueOnGameThread(), T);
		const float Width = Height * FMath::Max(CVarProjAspect.GetValueOnGameThread(), 0.1f);
		PanelSizePx = FVector2D(Width, Height); // the HUD sizes the retinue wings off this
		if (!bHostSized)
		{
			RootBox->SetWidthOverride(Width);
			RootBox->SetHeightOverride(Height);
		}

		if (FrameBorder)
		{
			FLinearColor FrameCol = Demichrome::Steel();
			if (CVarProjThreatTint.GetValueOnGameThread() != 0 && Retinue > 0)
			{
				// 0 at parity, 1 once outnumbered ~4:1.
				const float Threat = FMath::Clamp(((float)Brood / (float)Retinue - 1.f) / 3.f, 0.f, 1.f);
				FrameCol = FMath::Lerp(Demichrome::Steel(), FLinearColor(FColor(178, 58, 44)), Threat);
			}
			FrameBorder->SetBrushColor(FrameCol);
		}
	}

	// Mode gate (docs/design/squad-group-system.md §4.3): -1 (default) is the resting state,
	// Army View — <=8 squad blocks, no perspective camera, no yaw (§5 scopes the yaw clamp to
	// Unit/Squad View only, and there's nothing else here for it to do). >=0 is Unit/Squad View,
	// the existing per-soldier billboard path below, now framed by FrameFraction/FrameFloor.
	const int32 SelectedSquad = FUnitCamDirector::SelectedSquad();
	CanvasWidget->SetShowReticle(SelectedSquad >= 0);
	if (SelectedSquad < 0)
	{
		BuildArmyView(*Swarm);
		return;
	}

	const TArray<FVector>& Positions = Swarm->GetRenderPositions();
	const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();
	const int32 Num = FMath::Min(Positions.Num(), AnimBits.Num());

	// Two distinct points, deliberately: the flame/shading origin is always the bearer, but
	// the CAMERA follows whatever the director resolves (a soldier in follow mode). Splitting
	// them lets the cam ride a unit while the light still radiates from the hero.
	const FVector FlamePos = Swarm->GetAttractor();
	const double NowSeconds = World->GetTimeSeconds();
	const bool bCastFocus = Swarm->IsCastFocusActive(NowSeconds);
	const FVector CastPos = Swarm->GetCastFocusPos();
	// The director owns the whole camera decision — where to aim, from what angle, how close.
	// Everything below is projection: turn the shot into a basis and blit.
	const FUnitCamShot Shot = Director.Tick(Positions, AnimBits, FlamePos, DeltaTime, bCastFocus, CastPos);
	const FVector CamFocus = Shot.Focus;

	// Lens geometry, computed BEFORE the camera position: the group-framing pull-back below
	// needs TanHalf to solve for how far back Dist has to go, so it can no longer wait until
	// after CamPos is built the way it used to.
	const float FovRad = FMath::DegreesToRadians(FMath::Clamp(CVarProjFov.GetValueOnGameThread(), 10.f, 120.f));
	const float TanHalf = FMath::Tan(FovRad * 0.5f); // HORIZONTAL half-angle (Fov is horizontal)
	// Vertical half-angle follows the panel's aspect, so a wide panel shows a wider swath of
	// world rather than horizontally stretching a square image. Prefer the geometry we were
	// ACTUALLY allotted over the Aspect CVar: once the cam is one half of a split column its
	// real shape is set by the host, and trusting the CVar would stretch the image.
	const FVector2D Allotted = MyGeometry.GetLocalSize();
	const float Aspect = (Allotted.X > 1.0 && Allotted.Y > 1.0)
		? (float)(Allotted.X / Allotted.Y)
		: FMath::Max(CVarProjAspect.GetValueOnGameThread(), 0.1f);
	const float TanHalfV = TanHalf / FMath::Max(Aspect, 0.1f);

	// --- group-framing pull-back (docs/design/squad-group-system.md §4.2) -----------------
	// "The majority of the army visible" as a real dial: find the world-space radius R around
	// CamFocus that already contains FrameFraction (floor FrameFloor) of the framed population,
	// then make sure BOTH the cull radius (Range) and the camera distance (Dist) cover it — Dist
	// alone would leave the extra units culled before they're ever tested against the frustum;
	// Range alone would cull them in without actually being far enough back to fit them in frame.
	// Protects cohesion by widening coverage, never by cropping — the bearer may drift off-centre
	// first (Design Law 6 over Design Law 4, scoped to this one panel per the spec).
	//
	// Population is the selected squad's REAL standing count (GetSquadStanding) AND, since
	// task-046 piped a real squad byte into the render buffer, the units drawn below (and
	// counted here) are now actually filtered to that squad's real members — retiring the
	// "whole visible retinue" approximation this comment used to disclose.
	float RequiredRadius = 0.f;
	{
		const int32 ClampedSquad = FMath::Clamp(SelectedSquad, 0, USwarmSubsystem::MaxSquads - 1);
		const int32 TargetCount = Swarm->GetSquadStanding(ClampedSquad);
		if (TargetCount > 0)
		{
			const int32 Floor = FMath::Max(CVarProjFrameFloor.GetValueOnGameThread(), 0);
			const int32 Required = FMath::Min(TargetCount, FMath::Max(Floor,
				FMath::CeilToInt(CVarProjFrameFraction.GetValueOnGameThread() * (float)TargetCount)));

			TArray<float> RetinueDistSq;
			RetinueDistSq.Reserve(TargetCount);
			for (int32 i = 0; i < Num; ++i)
			{
				if ((AnimBits[i] & SwarmAnim::TeamBit) != 0
					&& SwarmSquad::UnitIndex(SwarmRenderPack::Squad(AnimBits[i])) == ClampedSquad)
				{
					RetinueDistSq.Add((float)FVector::DistSquaredXY(Positions[i], CamFocus));
				}
			}
			if (RetinueDistSq.Num() >= Required && Required > 0)
			{
				RetinueDistSq.Sort();
				RequiredRadius = FMath::Sqrt(RetinueDistSq[Required - 1]);
			}
			else if (RetinueDistSq.Num() > 0)
			{
				RetinueDistSq.Sort();
				RequiredRadius = FMath::Sqrt(RetinueDistSq.Last()); // fewer on screen than asked for — show them all
			}
		}
	}
	// 15% headroom so the Required-th unit sits just inside the edge, not exactly on it.
	const float CoverageMargin = 1.15f;
	const float NeededRange = RequiredRadius * CoverageMargin;
	const float NeededDist = TanHalf > KINDA_SMALL_NUMBER ? NeededRange / TanHalf : 0.f;

	// Build the virtual camera: orbit the focus by the shot's yaw, sit Dist behind and Height
	// above, look back at it. Pure math — no component, no second scene render.
	const float Yaw = FMath::DegreesToRadians(Shot.YawDeg);
	const FVector Behind(-FMath::Cos(Yaw), -FMath::Sin(Yaw), 0.f);
	const float DistVal = FMath::Max(CVarProjDist.GetValueOnGameThread(), NeededDist) * Shot.DistScale;
	const FVector CamPos = CamFocus + Behind * DistVal
		+ FVector(0.f, 0.f, CVarProjHeight.GetValueOnGameThread());
	FVector Forward = (CamFocus - CamPos).GetSafeNormal();

	// Extra downward tilt on top of the look-at: rotate the aim around the horizontal right
	// axis. Positive Pitch points the lens below the focus (so the focus rides higher).
	const float Pitch = CVarProjPitch.GetValueOnGameThread();
	if (FMath::Abs(Pitch) > KINDA_SMALL_NUMBER)
	{
		const FVector TiltAxis = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
		if (!TiltAxis.IsNearlyZero())
		{
			Forward = Forward.RotateAngleAxis(Pitch, TiltAxis).GetSafeNormal();
		}
	}
	const FMatrix Basis = FRotationMatrix::MakeFromX(Forward);
	const FVector Right = Basis.GetUnitAxis(EAxis::Y);
	const FVector Up = Basis.GetUnitAxis(EAxis::Z);

	// This panel's yaw in the same convention SwarmFacing uses, so a unit turned toward
	// THIS camera draws its south (facing-the-viewer) column. A unit facing the camera has
	// a world direction of -Forward, and SwarmFacing measures atan2(Y, -X) — so the yaw
	// that maps that unit onto column 0 is atan2(-Forward.Y, Forward.X). Ground plane only:
	// the sheet has no pitch axis, and the cam's tilt must not rotate the sprite.
	const float ViewYaw = FMath::RadiansToDegrees(FMath::Atan2(-Forward.Y, Forward.X));

	const float NearPlane = FMath::Max(CVarProjNearPlane.GetValueOnGameThread(), 1.f);
	const float RangeSq = FMath::Square(FMath::Max(CVarProjRange.GetValueOnGameThread(), NeededRange));
	const float UnitHalf = 40.f * CVarProjScale.GetValueOnGameThread();
	const float NearFade = FMath::Max(CVarProjNearFade.GetValueOnGameThread(), 0.f);
	const float BroodScale = FMath::Max(CVarProjBroodScale.GetValueOnGameThread(), 0.f);

	// Size variation is a WORLD dial, not a panel one — read the same Swarm.* CVars the
	// world renderer uses rather than mirroring them here, so the close-up can never
	// disagree with the wide shot about how varied the horde is.
	const float BroodJitter = FMath::Clamp(ReadCVarFloat(TEXT("Swarm.BroodSizeJitter"), 0.2f), 0.f, 0.95f);
	const float RetinueJitter = FMath::Clamp(ReadCVarFloat(TEXT("Swarm.RetinueSizeJitter"), 0.f), 0.f, 0.95f);

	// Debug: draw the virtual camera in the MAIN world view so its coverage is visible.
	if (CVarProjDebugFrustum.GetValueOnGameThread() != 0 && World)
	{
		const float FocusDist = (float)FVector::Dist(CamFocus, CamPos);
		const float Far = FocusDist + FMath::Sqrt(RangeSq); // effective range, incl. group-framing pull-back
		const FColor FrustumCol(80, 160, 255);
		const FColor AimCol(255, 200, 60);

		auto Corner = [&](float D, float Sx, float Sy) -> FVector
		{
			return CamPos + Forward * D + Right * (Sx * D * TanHalf) + Up * (Sy * D * TanHalfV);
		};

		// Near and far rectangles.
		for (int32 Plane = 0; Plane < 2; ++Plane)
		{
			const float D = (Plane == 0) ? NearPlane : Far;
			const FVector C00 = Corner(D, -1.f, -1.f), C10 = Corner(D, 1.f, -1.f);
			const FVector C11 = Corner(D, 1.f, 1.f), C01 = Corner(D, -1.f, 1.f);
			DrawDebugLine(World, C00, C10, FrustumCol, false, -1.f, 0, 1.5f);
			DrawDebugLine(World, C10, C11, FrustumCol, false, -1.f, 0, 1.5f);
			DrawDebugLine(World, C11, C01, FrustumCol, false, -1.f, 0, 1.5f);
			DrawDebugLine(World, C01, C00, FrustumCol, false, -1.f, 0, 1.5f);
		}
		// Edges from the camera to the far corners.
		DrawDebugLine(World, CamPos, Corner(Far, -1.f, -1.f), FrustumCol, false, -1.f, 0, 1.5f);
		DrawDebugLine(World, CamPos, Corner(Far, 1.f, -1.f), FrustumCol, false, -1.f, 0, 1.5f);
		DrawDebugLine(World, CamPos, Corner(Far, 1.f, 1.f), FrustumCol, false, -1.f, 0, 1.5f);
		DrawDebugLine(World, CamPos, Corner(Far, -1.f, 1.f), FrustumCol, false, -1.f, 0, 1.5f);

		// Camera marker, aim line, focus marker, and the Range ring on the ground.
		DrawDebugSphere(World, CamPos, 22.f, 10, FrustumCol, false, -1.f, 0, 1.5f);
		DrawDebugLine(World, CamPos, CamFocus, AimCol, false, -1.f, 0, 1.f);
		DrawDebugSphere(World, CamFocus, 26.f, 10, AimCol, false, -1.f, 0, 1.5f);
		DrawDebugCircle(World, CamFocus, FMath::Sqrt(RangeSq), 48,
			AimCol, false, -1.f, 0, 1.5f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}

	const float FlameRadius = FMath::Max(ReadCVarFloat(TEXT("Swarm.FlameRadius"), 900.f), 1.f);
	const float FlameFalloff = FMath::Max(ReadCVarFloat(TEXT("Swarm.FlameFalloff"), 2.f), 0.001f);
	const float LightFloor = FMath::Clamp(ReadCVarFloat(TEXT("Swarm.UnitLightFloor"), 0.28f), 0.f, 1.f);
	// Borrowed, not duplicated: the panel's back-facing shade IS Swarm.UnitBackShade, so tuning
	// the world's front/back split retunes the close-up with it and the two can't drift.
	const float BackShade = FMath::Clamp(ReadCVarFloat(TEXT("Swarm.UnitBackShade"), 0.32f), 0.f, 1.f);
	const float BroodFloor = FMath::Clamp(CVarProjBroodFloor.GetValueOnGameThread(), 0.f, 1.f);
	// Ceiling can't fall below the floor, or a brood would brighten as it walked AWAY.
	const float BroodCeil = FMath::Clamp(CVarProjBroodCeil.GetValueOnGameThread(), BroodFloor, 1.f);
	const bool bDirShade = CVarProjDirShade.GetValueOnGameThread() != 0;
	const int32 LightSteps = CVarProjLightSteps.GetValueOnGameThread();
	const float FullColorFloor = FMath::Clamp(CVarProjFullColorFloor.GetValueOnGameThread(), 0.f, 1.f);
	const float FullColorDimStrength = FMath::Clamp(CVarProjFullColorDimStrength.GetValueOnGameThread(), 0.f, 1.f);
	const bool bSoldierVariants = CVarProjSoldierVariants.GetValueOnGameThread() != 0; // read once, not per-body

	TArray<FUnitCamBillboard> Out;
	Out.Reserve(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		const FVector P = Positions[i];
		if (FVector::DistSquaredXY(P, CamFocus) > RangeSq)
		{
			continue; // not near the camera's focus — outside what this close-up frames
		}

		// Real per-unit membership filter (task-046): SelectedSquad is guaranteed >= 0 here
		// (Army View already returned above), so a retinue body only draws in THIS panel if
		// it's actually one of the selected unit's own soldiers — decoded straight from the
		// squad byte task-046 finally piped into the render buffer (SwarmRenderPack::Squad).
		// Retires the "framed toward the whole visible retinue, not filtered to one squad's
		// real members" approximation this loop used to carry (see UnitCamDirector.cpp's
		// own comment on the same gap). Brood are never filtered — the enemy read is unscoped.
		if ((AnimBits[i] & SwarmAnim::TeamBit) != 0
			&& SwarmSquad::UnitIndex(SwarmRenderPack::Squad(AnimBits[i])) != SelectedSquad)
		{
			continue;
		}

		const FVector V = P - CamPos;
		const float CamFwd = (float)FVector::DotProduct(V, Forward);
		if (CamFwd < NearPlane)
		{
			continue; // behind the camera / too close
		}

		// Forced perspective: divide the view-space offset by depth, normalise by the
		// FOV so the frustum maps to [-1,1], and cull anything outside it.
		const float NX = ((float)FVector::DotProduct(V, Right) / CamFwd) / TanHalf;
		const float NY = ((float)FVector::DotProduct(V, Up) / CamFwd) / TanHalfV;
		if (FMath::Abs(NX) > 1.1f || FMath::Abs(NY) > 1.1f)
		{
			continue;
		}

		const bool bRetinue = (AnimBits[i] & SwarmAnim::TeamBit) != 0;
		// Decided once, up front, and reused both for lighting (below) and sprite selection
		// (further down) so the two can never disagree about which bodies are full-colour.
		const bool bFullColorRetinue = bRetinue && bSoldierVariants;

		FUnitCamBillboard B;
		B.Center = FVector2f(0.5f + 0.5f * NX, 0.5f - 0.5f * NY); // NDC -> panel, y down
		// 1/depth scaling, then the team's size multiplier, then this body's own size roll
		// (packed in the high bits of the anim int32 — SwarmRenderPack). All folded into
		// HalfSize rather than carried as billboard fields: nothing downstream needs to
		// know WHY a body is the size it is, and the sort is on depth.
		B.HalfSize = (UnitHalf / CamFwd) / TanHalf * 0.5f
			* (bRetinue ? 1.f : BroodScale)
			* SwarmRenderPack::SizeScale(AnimBits[i], bRetinue ? RetinueJitter : BroodJitter);
		B.Depth = CamFwd;

		// --- flame shading, close-up edition -------------------------------
		// Shading uses the BEARER's position (FlamePos), never the camera focus: the cam may be
		// riding a soldier off to one side, but the light still radiates from the bearer.
		//
		// Built as one attenuation term in 0..1 — "how much of the flame is on this body" — that
		// every effect below multiplies into, with the team's value range applied ONCE at the end.
		// Same shape as the world renderer's Atten -> Lerp(Floor, 1, Atten), so no stack of terms
		// can drive a unit to pure black and the floor stays a guarantee rather than a suggestion.
		const FVector2D ToFlame(FlamePos.X - P.X, FlamePos.Y - P.Y);
		const float FlameDist = (float)ToFlame.Size();

		// 1. Distance falloff — identical to Swarm.UnitShading's, so the panel and the world
		//    agree about how far the light carries.
		const float T = FMath::Clamp(FlameDist / FlameRadius, 0.f, 1.f);
		float Atten = 1.f - FMath::Pow(T, FlameFalloff);

		// 2. Which side we can see. Two ground-plane directions: unit->flame (where the lit
		//    hemisphere points) and unit->lens. Their dot is how much of that lit hemisphere is
		//    turned toward us: +1 = flame behind the lens, we see the lit face; -1 = flame behind
		//    the unit, we see its shadowed back and it reads as a silhouette against the pool.
		//    The camera sits behind the bearer looking out, so brood walking in on him resolve to
		//    +1 and brighten as they arrive — that is the "walking toward the light" read, and it
		//    falls out of the geometry rather than being faked per-unit.
		if (bDirShade)
		{
			const FVector2D FlameDir = FlameDist > 1.f ? ToFlame / FlameDist : FVector2D(1.f, 0.f);
			const FVector2D CamDir = FVector2D(CamPos.X - P.X, CamPos.Y - P.Y).GetSafeNormal();
			const float Facing = CamDir.IsNearlyZero()
				? 1.f
				: (float)FVector2D::DotProduct(FlameDir, CamDir);
			Atten *= FMath::Lerp(BackShade, 1.f, 0.5f + 0.5f * Facing);
		}

		// 3. Band it, so the light steps like 2-bit art instead of sliding through every grey.
		if (LightSteps > 1)
		{
			Atten = FMath::RoundToFloat(Atten * (float)LightSteps) / (float)LightSteps;
		}

		// The two teams travel different value ranges. Retinue span the shared silhouette-rescue
		// floor up to the full sprite — they are yours and must stay legible even out at the
		// leash. Brood start near-black at the edge of the pool and are lifted by the approach,
		// but never all the way: they stay under the retinue at every distance, so a soldier
		// standing next to a brood is always the brighter of the two.
		//
		// Full-colour retinue (the knight/archer sprite sets) get a THIRD range, gentler than
		// the flat-art one above — see CVarProjFullColorFloor/DimStrength's doc comment for why
		// the same model crushed them. DimStrength softens Atten itself (how far the falloff is
		// allowed to pull toward dark) before the floor/ceiling lerp runs, so the flame-distance
		// read still survives, just scaled down — it does not touch LightFloor/BroodFloor or
		// anything brood/hero/world-side.
		const float Floor = bFullColorRetinue ? FullColorFloor : (bRetinue ? LightFloor : BroodFloor);
		const float Ceil = bRetinue ? 1.f : BroodCeil; // full-colour retinue still caps at 1 — full brightness is the authored art, untinted
		const float ClampedAtten = FMath::Clamp(Atten, 0.f, 1.f);
		const float LitAtten = bFullColorRetinue ? FMath::Lerp(1.f, ClampedAtten, FullColorDimStrength) : ClampedAtten;
		const float Lit = FMath::Lerp(Floor, Ceil, LitAtten);

		// Near-plane fade: a unit right at the near plane is transparent and ramps to solid
		// NearFade uu deeper, so a unit entering close to the fake camera fades in, not pops.
		const float FadeAlpha = NearFade > 0.f
			? FMath::Clamp((CamFwd - NearPlane) / NearFade, 0.f, 1.f)
			: 1.f;

		// The COLUMN is deliberately not the world view's. This camera looks from somewhere
		// else, so a unit the main view sees from the front is seen from the side here;
		// resolving facing per-view against the same stored world angle is the whole reason
		// SwarmFacing stores 32 world steps instead of a baked column. 8 buckets regardless of
		// which sheet ends up drawn below — both SwarmSheet and the variant sheets are
		// authored to the same 8-direction, south-first-counter-clockwise convention.
		const int32 DirCol = SwarmFacing::ColumnFor(SwarmRenderPack::Facing(AnimBits[i]), ViewYaw, 8);

		if (bFullColorRetinue)
		{
			// TYPE (which list) is the soldier's REAL, permanent type, decoded from the squad
			// byte task-046 piped into the render buffer — not a hash standing in for a type
			// that didn't exist yet. STATE (which entry in that list) is that same stable hash,
			// repurposed rather than deleted — see SpriteSetForSoldier's doc comment. DirCol IS
			// the flat cell index into any state's 5x2 grid (RetinueSheetColumns divides 8
			// evenly for the direction cells), so no further row math is needed the way
			// SwarmSheet::CellFor needs for the 8x4 atlas.
			B.SpriteSet = SpriteSetForSoldier(AnimBits[i], NumSpearmenStatesLoaded, NumArcherStatesLoaded);
			// South-only walk toggle (RetinueSouthWalkCellA/B): the two high-res sheets carry a
			// two-frame walk cycle ONLY for south (the knight's real generated walk frames 0
			// and 2; the archer has no animation source, so its two cells are a duplicated idle
			// — see the grid doc comment above and provenance.json). Reuses the sim's existing
			// walk-cycle bit (the same one that already toggled which row of the old shared
			// atlas a body drew from), so this costs no new per-entity data. Every OTHER
			// direction still gets its real idle rotation — this does not fake a walk for them.
			B.Cell = (DirCol == 0)
				? ((AnimBits[i] & SwarmAnim::FrameBit) ? RetinueSouthWalkCellB : RetinueSouthWalkCellA)
				: DirCol;
		}
		else
		{
			// Brood, or SoldierVariants forced off: the shared atlas, as before. Same row the
			// world view picks for this unit, from the same bits, so the panel plays each
			// body's own walk instead of freezing everyone on one global frame. Brood live on
			// rows 0-1 of the atlas, retinue (when drawn from here) on rows 2-3.
			B.SpriteSet = UnitCamSprite::SwarmAtlas;
			B.Cell = SwarmAtlas ? SwarmSheet::CellFor((uint8)AnimBits[i], DirCol) : INDEX_NONE;
		}
		const bool bSprite = B.Cell != INDEX_NONE;
		if ((AnimBits[i] & SwarmAnim::HitFlashBit) != 0)
		{
			// Hit flash, light-exempt for the same reasons as the world renderer. This
			// is the close-up, so it is where a flinch reads best — the panel would look
			// oddly serene if the only place hits didn't register were the shot framed
			// to show them.
			//
			// Demichrome::Pale(), NOT literal (1,1,1) (docs/art/palette-exceptions.md, task-040
			// ruling): unlike the world renderer's DrawDebugSolidBox hit-flash, THIS panel is UMG
			// drawn after post-processing, so nothing downstream quantizes it — literal white
			// would render as a real fifth palette value. Pale is already the ramp's "brightest a
			// unit can be" register and there's no "must outshine Pale" requirement here, so it
			// doesn't qualify for the flame-core-style exception; the perceptual difference over
			// Swarm.HitFlashTime (0.10s default) isn't detectable in play.
			const FLinearColor Flash = Demichrome::Pale();
			B.Color = FLinearColor(Flash.R, Flash.G, Flash.B, FadeAlpha);
		}
		else if (bSprite)
		{
			// Light-only tint: colour comes from the atlas, so the panel reads the same as the
			// world. Brood may optionally keep the reserved red they had as flat quads — the
			// team read used to come entirely from that colour, and now it comes from the art.
			B.Color = FLinearColor(Lit, Lit, Lit, FadeAlpha);
			if (!bRetinue && CVarProjBroodTint.GetValueOnGameThread() != 0)
			{
				B.Color *= FLinearColor(BroodAlbedo);
				B.Color.A = FadeAlpha;
			}
		}
		else
		{
			B.Color = FLinearColor(bRetinue ? RetinueAlbedo : BroodAlbedo) * Lit;
			B.Color.A = FadeAlpha;
		}

		Out.Add(MoveTemp(B));
	}

	// --- the hero proxy ----------------------------------------------------
	// The bearer is a pawn (ASpikeHeroPawn), not a Mass entity, so he is absent from the
	// render buffers the loop above walks — without this the panel frames a hero-shaped hole.
	// One billboard, projected through the same camera, at the attractor he publishes. No
	// longer gated on SwarmAtlas — he draws from his own T_Hero_Vanguard now (task-050); if
	// that texture is missing SUnitCamCanvas's untextured-quad fallback still marks his spot.
	if (CVarProjHero.GetValueOnGameThread() != 0 && Swarm->IsHeroAlive())
	{
		const FVector V = FlamePos - CamPos;
		const float CamFwd = (float)FVector::DotProduct(V, Forward);
		if (CamFwd >= NearPlane)
		{
			const float NX = ((float)FVector::DotProduct(V, Right) / CamFwd) / TanHalf;
			const float NY = ((float)FVector::DotProduct(V, Up) / CamFwd) / TanHalfV;
			if (FMath::Abs(NX) <= 1.1f && FMath::Abs(NY) <= 1.1f)
			{
				FUnitCamBillboard H;
				H.Center = FVector2f(0.5f + 0.5f * NX, 0.5f - 0.5f * NY);
				H.HalfSize = (UnitHalf / CamFwd) / TanHalf * 0.5f;
				H.Depth = CamFwd;
				H.bHero = true;
				H.SpriteSet = UnitCamSprite::Hero;
				H.Cell = FMath::Clamp(CVarProjHeroCell.GetValueOnGameThread(),
					0, HeroSheetColumns * HeroSheetRows - 1);
				// Never dimmed by the flame falloff: he IS the flame, so distance-to-light is
				// zero by definition. Only the near-plane fade applies.
				const float FadeAlpha = NearFade > 0.f
					? FMath::Clamp((CamFwd - NearPlane) / NearFade, 0.f, 1.f)
					: 1.f;
				// Pale, not literal white — same palette-exceptions.md reasoning as the hit-flash
				// fix above: this panel is UMG, drawn after post-processing, so nothing downstream
				// quantizes (1,1,1) down to the ramp. Pale is already "brightest a body gets" here.
				const FLinearColor HeroTint = Demichrome::Pale();
				H.Color = FLinearColor(HeroTint.R, HeroTint.G, HeroTint.B, FadeAlpha);
				Out.Add(MoveTemp(H));
			}
		}
	}

	// Painter's algorithm — no depth buffer in a Slate composite, so draw far first. The hero
	// sorts last regardless of depth: he is the subject of the shot and must never be hidden
	// behind whichever soldier happens to be between him and the lens.
	Out.Sort([](const FUnitCamBillboard& A, const FUnitCamBillboard& B)
	{
		if (A.bHero != B.bHero) { return B.bHero; }
		return A.Depth > B.Depth;
	});

	CanvasWidget->SetBillboards(MoveTemp(Out));
}

// ---------------------------------------------------------------------------
// Console toggle: Emberkeep.UI.UnitCamProj
// ---------------------------------------------------------------------------
namespace
{
	TWeakObjectPtr<UUnitCamProjector> GUnitCamProjWidget;

	UWorld* FindProjPlayWorld(UWorld* Passed)
	{
		if (Passed && Passed->GetFirstPlayerController())
		{
			return Passed;
		}
		if (GEngine)
		{
			UWorld* GameFallback = nullptr;
			for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
			{
				if (!Ctx.World())
				{
					continue;
				}
				if (Ctx.WorldType == EWorldType::PIE)
				{
					return Ctx.World();
				}
				if (Ctx.WorldType == EWorldType::Game)
				{
					GameFallback = Ctx.World();
				}
			}
			return GameFallback;
		}
		return nullptr;
	}

	void ToggleUnitCamProj(UWorld* Passed)
	{
		if (GUnitCamProjWidget.IsValid())
		{
			GUnitCamProjWidget->RemoveFromParent();
			GUnitCamProjWidget.Reset();
			UE_LOG(LogTemp, Display, TEXT("Emberkeep.UI.UnitCamProj: removed."));
			return;
		}

		UWorld* World = FindProjPlayWorld(Passed);
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Emberkeep.UI.UnitCamProj: no active play world. Press Play, then run this."));
			return;
		}

		UUnitCamProjector* Widget = CreateWidget<UUnitCamProjector>(PC, UUnitCamProjector::StaticClass());
		if (!Widget)
		{
			return;
		}
		Widget->AddToViewport(60);
		GUnitCamProjWidget = Widget;
		UE_LOG(LogTemp, Display,
			TEXT("Emberkeep.UI.UnitCamProj: shown (bottom-right). Dials: Emberkeep.UnitCamProj.*"));
	}

	FAutoConsoleCommandWithWorld GUnitCamProjCmd(
		TEXT("Emberkeep.UI.UnitCamProj"),
		TEXT("Toggle the projection-prototype Unit Cam: billboards the swarm into a bottom-right "
			 "panel from the live sim positions, no SceneCapture. Requires an active play session."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ToggleUnitCamProj));

	// Test hookup: simulate the bearer casting a spell, so the cast-focus camera behaviour can
	// be driven before a real spell system exists. The spell system will call
	// USwarmSubsystem::SetCastFocus(point, endTime) directly at cast time.
	void TestCastFocus(const TArray<FString>& Args, UWorld* Passed)
	{
		UWorld* World = FindProjPlayWorld(Passed);
		USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!Swarm)
		{
			UE_LOG(LogTemp, Warning, TEXT("Emberkeep.UnitCamProj.TestCast: no play world / swarm subsystem."));
			return;
		}
		const float Duration = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 2.f;
		Swarm->SetCastFocus(Swarm->GetAttractor(), World->GetTimeSeconds() + Duration);
		UE_LOG(LogTemp, Display,
			TEXT("Emberkeep.UnitCamProj.TestCast: focus-punch on the bearer for %.1fs."), Duration);
	}

	FAutoConsoleCommandWithWorldAndArgs GTestCastCmd(
		TEXT("Emberkeep.UnitCamProj.TestCast"),
		TEXT("Simulate a spell cast: punch the Unit Cam focus onto the bearer for N seconds "
			 "(default 2). Usage: Emberkeep.UnitCamProj.TestCast 2"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TestCastFocus));
}
