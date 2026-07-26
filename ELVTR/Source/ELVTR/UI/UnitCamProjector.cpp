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
		TEXT("Only units within this XY distance of the focus are considered for the panel."),
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

	// Cell indices below are into the 8x4 T_Swarm_2bit grid (SwarmSheet, SwarmFragments.h).
	// Columns are the eight FACINGS (south first, counter-clockwise); rows are
	//   0 = brood walk0, 1 = brood walk1, 2 = retinue walk0, 3 = retinue walk1.
	// These defaults have now MOVED TWICE: 2x2 -> 4x2 (retinue row started at 4) ->
	// 8x4 (retinue row starts at 16). The attack and hit columns no longer exist.

	TAutoConsoleVariable<int32> CVarProjHeroCell(
		TEXT("Emberkeep.UnitCamProj.HeroCell"), 16,
		TEXT("Which cell of the 8x4 T_Swarm_2bit sheet the hero draws from (0-31).\n")
		TEXT("Defaults to 16 = retinue walk0 facing SOUTH, i.e. a soldier turned toward the\n")
		TEXT("viewer — the closest thing to a portrait the atlas has, and never a brood\n")
		TEXT("frame. Was 6 (retinue ATTACK) until the sheet gained a facing axis and lost\n")
		TEXT("its attack column. Placeholder until the real hero sprite exists."),
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

	void SetCellBrushes(TArray<FSlateBrush>&& InBrushes)
	{
		CellBrushes = MoveTemp(InBrushes);
	}

	void SetSoldierScale(float InScale) { SoldierScale = InScale; }

	void SetHeroScale(float InScale) { HeroScale = InScale; }

	void SetFootAnchor(float InAnchor) { FootAnchor = InAnchor; }

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
			if (CellBrushes.IsValidIndex(B.Cell))
			{
				const FSlateBrush& CellBrush = CellBrushes[B.Cell];
				const float ImgX = (float)CellBrush.ImageSize.X;
				const float ImgY = (float)CellBrush.ImageSize.Y;
				const float Aspect = ImgY > 0.f ? ImgX / ImgY : 1.f;
				const float DrawH = 2.f * Half * SoldierScale * (B.bHero ? HeroScale : 1.f);
				const float DrawW = DrawH * Aspect;
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
		}

		// Focus reticle (panel centre = where the virtual camera is aimed), so orbiting
		// the Yaw dial reads clearly as the world turning around the hero.
		const FVector2f C = Size * 0.5f;
		const float R = 5.f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(2.f * R, 1.f), FSlateLayoutTransform(C - FVector2f(R, 0.f))),
			&Brush, ESlateDrawEffect::None, Demichrome::Steel());
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(1.f, 2.f * R), FSlateLayoutTransform(C - FVector2f(0.f, R))),
			&Brush, ESlateDrawEffect::None, Demichrome::Steel());

		return LayerId + 3;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(PanelSize, PanelSize);
	}

private:
	TArray<FUnitCamBillboard> Billboards;
	TArray<FSlateBrush> CellBrushes; // one per atlas cell; empty until the texture loads
	float SoldierScale = SoldierHeightScale;
	float HeroScale = 1.6f;
	float FootAnchor = 1.f;   // 1 = bodies stand on their projected ground point
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

void UUnitCamCanvasWidget::SetCellBrushes(TArray<FSlateBrush>&& InBrushes)
{
	if (Canvas.IsValid())
	{
		Canvas->SetCellBrushes(MoveTemp(InBrushes));
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

void UUnitCamCanvasWidget::SetFootAnchor(float InAnchor)
{
	if (Canvas.IsValid())
	{
		Canvas->SetFootAnchor(InAnchor);
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

void UUnitCamProjector::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (!CanvasWidget)
	{
		return;
	}

	// Unit sprites = the 2-bit swarm atlas — the same texture the main view uses, so the panel
	// and the world can never show a unit in two different poses. Loaded once from Content and
	// sliced into one brush per cell; the slices are static, so this happens on first tick only.
	if (!SwarmAtlas && !bAtlasLoadAttempted)
	{
		bAtlasLoadAttempted = true;
		SwarmAtlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Spike1/T_Swarm_2bit.T_Swarm_2bit"));
		if (!SwarmAtlas)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("UnitCamProj: T_Swarm_2bit not found in Content — units fall back to quads."));
		}
	}
	if (SwarmAtlas && !bCellBrushesPushed)
	{
		// Grid comes from SwarmSheet so the panel's UVs can't drift from the Niagara bridge's.
		bCellBrushesPushed = true;
		const int32 CellCount = SwarmSheet::Columns * SwarmSheet::Rows;
		const float CellW = 1.f / (float)SwarmSheet::Columns;
		const float CellH = 1.f / (float)SwarmSheet::Rows;
		TArray<FSlateBrush> Brushes;
		Brushes.Reserve(CellCount);
		for (int32 Cell = 0; Cell < CellCount; ++Cell)
		{
			const float U = (Cell % SwarmSheet::Columns) * CellW;
			const float V = (Cell / SwarmSheet::Columns) * CellH;
			FSlateBrush Brush;
			Brush.SetResourceObject(SwarmAtlas);
			// The cell, not the whole sheet — this is what the draw aspect is computed from,
			// and a 4x2 sheet's full size would stretch every unit to twice its width.
			Brush.ImageSize = FVector2D(
				SwarmAtlas->GetSizeX() * CellW, SwarmAtlas->GetSizeY() * CellH);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.SetUVRegion(FBox2f(FVector2f(U, V), FVector2f(U + CellW, V + CellH)));
			Brushes.Add(MoveTemp(Brush));
		}
		CanvasWidget->SetCellBrushes(MoveTemp(Brushes));
	}
	CanvasWidget->SetSoldierScale(CVarProjSoldierScale.GetValueOnGameThread());
	CanvasWidget->SetHeroScale(FMath::Max(CVarProjHeroScale.GetValueOnGameThread(), 0.f));
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

	// Build the virtual camera: orbit the focus by the shot's yaw, sit Dist behind and Height
	// above, look back at it. Pure math — no component, no second scene render.
	const float Yaw = FMath::DegreesToRadians(Shot.YawDeg);
	const FVector Behind(-FMath::Cos(Yaw), -FMath::Sin(Yaw), 0.f);
	const float DistVal = CVarProjDist.GetValueOnGameThread() * Shot.DistScale;
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
	const float NearPlane = FMath::Max(CVarProjNearPlane.GetValueOnGameThread(), 1.f);
	const float RangeSq = FMath::Square(CVarProjRange.GetValueOnGameThread());
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
		const float Far = FocusDist + FMath::Max(CVarProjRange.GetValueOnGameThread(), 0.f);
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
		DrawDebugCircle(World, CamFocus, FMath::Max(CVarProjRange.GetValueOnGameThread(), 1.f), 48,
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

	TArray<FUnitCamBillboard> Out;
	Out.Reserve(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		const FVector P = Positions[i];
		if (FVector::DistSquaredXY(P, CamFocus) > RangeSq)
		{
			continue; // not near the camera's focus — outside what this close-up frames
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
		const float Floor = bRetinue ? LightFloor : BroodFloor;
		const float Ceil = bRetinue ? 1.f : BroodCeil;
		const float Lit = FMath::Lerp(Floor, Ceil, FMath::Clamp(Atten, 0.f, 1.f));

		// Near-plane fade: a unit right at the near plane is transparent and ramps to solid
		// NearFade uu deeper, so a unit entering close to the fake camera fades in, not pops.
		const float FadeAlpha = NearFade > 0.f
			? FMath::Clamp((CamFwd - NearPlane) / NearFade, 0.f, 1.f)
			: 1.f;

		// Same row the world view picks for this unit, from the same bits — so the panel shows
		// the brood's own art and plays its walk per body instead of freezing every unit on
		// one global frame. Brood live on rows 0-1 of the atlas, retinue on rows 2-3.
		//
		// The COLUMN, though, is deliberately not the world view's. This camera looks from
		// somewhere else, so a unit the main view sees from the front is seen from the side
		// here; resolving facing per-view against the same stored world angle is the whole
		// reason SwarmFacing stores 32 world steps instead of a baked column.
		B.Cell = SwarmAtlas
			? SwarmSheet::CellFor((uint8)AnimBits[i],
				SwarmFacing::ColumnFor(SwarmRenderPack::Facing(AnimBits[i]), ViewYaw, SwarmSheet::Columns))
			: INDEX_NONE;
		const bool bSprite = B.Cell != INDEX_NONE;
		if ((AnimBits[i] & SwarmAnim::HitFlashBit) != 0)
		{
			// Hit flash, light-exempt for the same reasons as the world renderer. This
			// is the close-up, so it is where a flinch reads best — the panel would look
			// oddly serene if the only place hits didn't register were the shot framed
			// to show them.
			B.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha);
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
	// One billboard, projected through the same camera, at the attractor he publishes.
	if (CVarProjHero.GetValueOnGameThread() != 0 && SwarmAtlas && Swarm->IsHeroAlive())
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
				H.Cell = FMath::Clamp(CVarProjHeroCell.GetValueOnGameThread(),
					0, SwarmSheet::Columns * SwarmSheet::Rows - 1);
				// Never dimmed by the flame falloff: he IS the flame, so distance-to-light is
				// zero by definition. Only the near-plane fade applies.
				const float FadeAlpha = NearFade > 0.f
					? FMath::Clamp((CamFwd - NearPlane) / NearFade, 0.f, 1.f)
					: 1.f;
				H.Color = FLinearColor(1.f, 1.f, 1.f, FadeAlpha);
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
