#include "UI/EmberkeepHud.h"
#include "UI/MusterPanel.h"
#include "UI/EmberkeepUITypes.h"
#include "UI/UnitCamProjector.h"
#include "UI/EmberkeepCamFeed.h"
#include "UI/EmberkeepPalette.h"
#include "UI/StitchMeter.h"
#include "UI/ViewCamCapture.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Mass/SwarmSubsystem.h"
#include "Styling/CoreStyle.h"

namespace
{
	TAutoConsoleVariable<float> CVarMusterWingRatio(
		TEXT("Emberkeep.UI.Muster.WingRatio"), 0.5f,
		TEXT("Width of each retinue wing as a fraction of the Unit Cam's width. The wings flank\n")
		TEXT("the cam inside one rectangle and track its height exactly, so 0.5 makes the whole\n")
		TEXT("command rectangle twice the cam's width."),
		ECVF_Default);

	// --- the split centre column (view camera over unit cam) ----------------
	TAutoConsoleVariable<int32> CVarViewCam(
		TEXT("Emberkeep.UI.ViewCam"), 0,
		TEXT("Split the rectangle's centre column into two stacked panels: a second camera feed\n")
		TEXT("above the Unit Cam. 0 = UNIT CAM ALONE, full height — the primary layout (default).\n")
		TEXT("\n")
		TEXT("1 revives the split. The minimap that lived up there was tried and rejected\n")
		TEXT("(2026-07-26): a top-down capture wide enough to show the brood spawn ring left the\n")
		TEXT("lit pool a dot in a black field, so it read as empty. The machinery is kept because\n")
		TEXT("the split itself is sound — it is the minimap CONTENT that failed. Anything worth\n")
		TEXT("putting in a second panel can reuse it via Emberkeep.UI.ViewCam.Mode.\n")
		TEXT("Note the feed is a real second render of the scene — see Emberkeep.UI.ViewCam.Rate."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarRectSplit(
		TEXT("Emberkeep.UI.Rect.Split"), 0.5f,
		TEXT("Fraction of the centre column's height given to the TOP panel. 0.5 = even halves.\n")
		TEXT("Applied live every tick — drag it while playing to re-balance the two views."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarRectViewOnTop(
		TEXT("Emberkeep.UI.Rect.ViewOnTop"), 1,
		TEXT("1 = view camera on top, Unit Cam below. 0 = swap them. Rebuilds the band when\n")
		TEXT("changed, so it takes effect immediately."),
		ECVF_Default);
}

void UEmberkeepHud::Setup(bool bWithCams)
{
	bShowCams = bWithCams;
	RebuildBand();
}

void UEmberkeepHud::UseMockData()
{
	if (!Muster)
	{
		return;
	}

	// Run the mock roster through the same split the live path uses, so the preview shows the
	// real two-wing layout rather than a shelf that only the preview ever produces.
	ApplyMusterSquads(UMusterPanel::MakeMockSquads());
}

void UEmberkeepHud::ApplyMusterSquads(const TArray<FEmberkeepSquad>& InSquads)
{
	if (!Muster)
	{
		return;
	}

	// Whole-company totals, computed before the split — the strip measures the army, not either
	// half of it.
	int32 TotalSize = 0;
	int32 TotalStanding = 0;
	for (const FEmberkeepSquad& S : InSquads)
	{
		TotalSize += S.Size;
		TotalStanding += S.Standing;
	}

	if (CompanyMeter)
	{
		// 24 segments reads as a coarse company bar regardless of exact headcount.
		const int32 Segments = 24;
		CompanyMeter->SetValues(Segments, TotalSize > 0
			? FMath::RoundToInt(Segments * (float)TotalStanding / (float)TotalSize)
			: 0);
	}
	if (CompanyLabel)
	{
		CompanyLabel->SetText(FText::FromString(FString::Printf(
			TEXT("Vanguard Company   %d / %d   ·   %d squads"),
			TotalStanding, TotalSize, InSquads.Num())));
	}

	if (!bShowCams || !MusterRight)
	{
		Muster->SetSquads(InSquads);
		return;
	}

	// Split down the middle, first half left. Order is stable (squad index), so a squad never
	// hops sides as others are wiped out — you learn where your Shield squad lives.
	const int32 Half = (InSquads.Num() + 1) / 2;
	TArray<FEmberkeepSquad> Left(InSquads.GetData(), Half);
	TArray<FEmberkeepSquad> Right(InSquads.GetData() + Half, InSquads.Num() - Half);

	Muster->SetSquads(Left);
	MusterRight->SetSquads(Right);
}

void UEmberkeepHud::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Layout-shape dials can't be applied in place — they change which widgets exist and in
	// what order — so watch them and rebuild the band only when one actually flips.
	const bool bWantSplit = bShowCams && CVarViewCam.GetValueOnGameThread() != 0;
	const bool bWantViewOnTop = CVarRectViewOnTop.GetValueOnGameThread() != 0;
	if (bShowCams && (bWantSplit != bSplitColumn || (bWantSplit && bWantViewOnTop != bLastViewOnTop)))
	{
		RebuildBand();
	}

	// The cam resizes itself every frame from the body count; the wings follow it every frame so
	// the rectangle never visibly comes apart at the seams.
	SyncWingsToCam();

	PublishHudOcclusion(MyGeometry);

	// Refresh the live muster a few times a second, not every frame — SetSquads rebuilds the
	// card row, so we don't want to churn it on every kill during a heavy wave.
	RefreshTimer += InDeltaTime;
	if (RefreshTimer >= 0.15f)
	{
		RefreshTimer = 0.f;
		PushLiveMuster();
	}
}

void UEmberkeepHud::PushLiveMuster()
{
	if (!Muster)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	const int32 Alive = Swarm->GetAliveRetinue();

	// Nothing spawned yet: leave the mock preview intact (the muster-only command previews
	// the layout without a live army). Go live the moment real retinue exist.
	if (Alive <= 0 && PeakRetinue <= 0)
	{
		return;
	}

	PeakRetinue = FMath::Max(PeakRetinue, Alive);
	const ESwarmStance SwarmStance = Swarm->GetStance();
	const int32 StanceInt = (int32)SwarmStance;

	// Only rebuild when the visible state changed.
	if (Alive == LastAlive && StanceInt == LastStance)
	{
		return;
	}
	LastAlive = Alive;
	LastStance = StanceInt;

	// Real per-squad standing (cosmetic squads, tracked in the sim). Each card's SIZE is that
	// squad's high-water headcount, STANDING its live count — so cards show honest per-squad
	// attrition. Squads that never had members are skipped, so the row matches the live army.
	const EEmberkeepStance Stance = (EEmberkeepStance)(uint8)SwarmStance;
	static const TCHAR* const SquadNames[] = {
		TEXT("Shield"), TEXT("Vets"), TEXT("Spearmen"), TEXT("Banner"), TEXT("Reserve") };

	const int32 SquadCount = FMath::Min<int32>(USwarmSubsystem::MaxSquads, UE_ARRAY_COUNT(SquadPeak));
	TArray<FEmberkeepSquad> Live;
	Live.Reserve(SquadCount);
	for (int32 i = 0; i < SquadCount; ++i)
	{
		const int32 Standing = Swarm->GetSquadStanding(i);
		SquadPeak[i] = FMath::Max(SquadPeak[i], Standing);
		if (SquadPeak[i] <= 0)
		{
			continue; // squad never populated
		}
		FEmberkeepSquad S;
		S.Size = SquadPeak[i];
		S.Standing = FMath::Min(Standing, S.Size);
		S.Columns = FMath::Clamp(FMath::RoundToInt(FMath::Sqrt((float)FMath::Max(S.Size, 1)) * 1.3f), 3, 10);
		S.bWide = S.Size > 30;
		S.Stance = Stance;
		S.DisplayName = FText::FromString(SquadNames[i % UE_ARRAY_COUNT(SquadNames)]);
		Live.Add(S);
	}
	ApplyMusterSquads(Live);
}

TSharedRef<SWidget> UEmberkeepHud::RebuildWidget()
{
	if (!Band)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		Band = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Band"));
		if (UOverlaySlot* BandSlot = Root->AddChildToOverlay(Band))
		{
			// Content-sized and centred: the rectangle is an object on the screen, not a bar
			// stretched to the window — motifs need a known edge to hang off.
			BandSlot->SetHorizontalAlignment(HAlign_Center);
			BandSlot->SetVerticalAlignment(VAlign_Bottom);
			BandSlot->SetPadding(FMargin(BandPadding));
		}

		Muster = WidgetTree->ConstructWidget<UMusterPanel>(UMusterPanel::StaticClass(), TEXT("Muster"));
		MusterRight = WidgetTree->ConstructWidget<UMusterPanel>(UMusterPanel::StaticClass(), TEXT("MusterRight"));
	}

	RebuildBand();
	return Super::RebuildWidget();
}

void UEmberkeepHud::RebuildBand()
{
	if (!Band)
	{
		return;
	}

	Band->ClearChildren();
	WingLeftBox = nullptr;
	WingRightBox = nullptr;
	UnitCam = nullptr;
	CompanyBox = nullptr;
	CompanyMeter = nullptr;
	CompanyLabel = nullptr;
	CentreColumn = nullptr;
	CentreBox = nullptr;
	ViewFeed = nullptr;
	ViewFeedTarget = nullptr;
	TopSlot = nullptr;
	BottomSlot = nullptr;
	bSplitColumn = false;

	// THE rectangle: one Steel frame over one Dark ground, holding wing | cam | wing. Everything
	// inside drops its own chrome so this is the only visible edge — the motif surround will
	// eventually wrap this border and nothing else.
	Rect = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rect"));
	Rect->SetBrushColor(Demichrome::Steel());
	Rect->SetPadding(FMargin(2.f));

	UBorder* Ground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RectGround"));
	Ground->SetBrushColor(Demichrome::Dark());
	Ground->SetPadding(FMargin(3.f));
	Rect->SetContent(Ground);

	// Three columns, exactly as the layout is specified:
	//   [1] left squadron | [2] VBox{ unit cam, squad-size bar } | [3] right squadron
	// The bar belongs to the cam, so it lives INSIDE the centre column rather than spanning the
	// whole rectangle — that is what lets the wings close right up against the cam.
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Ground->SetContent(Row);

	if (UOverlaySlot* RectSlot = Band->AddChildToOverlay(Rect))
	{
		RectSlot->SetHorizontalAlignment(HAlign_Center);
		RectSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	// A wing: the muster panel, stripped of chrome, stacked as a column and scaled DOWN to fit
	// whatever the cam's current height allows. SyncWingsToCam drives the box each tick.
	auto BuildWing = [this, Row](UMusterPanel* Panel, EHorizontalAlignment Align, const TCHAR* Name) -> USizeBox*
	{
		if (!Panel)
		{
			return nullptr;
		}
		Panel->SetChrome(false);
		Panel->SetFlow(EEmberkeepMusterFlow::Column);
		Panel->SetContentAlignment(Align);

		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::DownOnly);
		Scale->SetContent(Panel);

		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetContent(Scale);
		if (UHorizontalBoxSlot* WingSlot = Row->AddChildToHorizontalBox(Box))
		{
			WingSlot->SetVerticalAlignment(VAlign_Top); // hang from the cam's top edge
			WingSlot->SetPadding(FMargin(WingGutter, 0.f));
		}
		return Box;
	};

	// Muster-only preview (no cam): one centred shelf, the original row layout.
	if (!bShowCams)
	{
		if (Muster)
		{
			Muster->SetChrome(false);
			Muster->SetFlow(EEmberkeepMusterFlow::Row);
			Muster->SetContentAlignment(HAlign_Left);
			Muster->SetShowCompany(false); // the strip above carries it
			Row->AddChildToHorizontalBox(Muster);
		}
		if (MusterRight)
		{
			MusterRight->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (MusterRight)
	{
		MusterRight->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// Left wing — cards only; the company bar lives in the strip above, so nothing wide sits in
	// a wing to force its scale-to-fit down and shrink the cards.
	if (Muster)
	{
		Muster->SetShowCompany(false);
	}
	WingLeftBox = BuildWing(Muster, HAlign_Right, TEXT("WingLeft"));

	// Centre column. Hairline frames throughout: the rectangle's own border is the outer edge.
	UnitCam = WidgetTree->ConstructWidget<UUnitCamProjector>(
		UUnitCamProjector::StaticClass(), TEXT("UnitCamProj"));
	UnitCam->SetFrameThickness(1.f);

	// The centre column: the cam (or the split pair) with the squad-size bar directly beneath it.
	CentreColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Centre"));
	if (UHorizontalBoxSlot* CentreSlot = Row->AddChildToHorizontalBox(CentreColumn))
	{
		CentreSlot->SetVerticalAlignment(VAlign_Top);
	}

	bSplitColumn = CVarViewCam.GetValueOnGameThread() != 0;
	if (!bSplitColumn)
	{
		// Unit Cam alone, sizing itself from the body count as it always did.
		UnitCam->SetHostSized(false);
		CentreColumn->AddChildToVerticalBox(UnitCam);
	}
	else
	{
		// Two panels in one VerticalBox, splitting the column's height by Rect.Split. Both are
		// host-sized: the column's SizeBox carries the body-count size, the Fill slots divide it.
		UnitCam->SetHostSized(true);

		ViewFeed = WidgetTree->ConstructWidget<UEmberkeepCamFeed>(
			UEmberkeepCamFeed::StaticClass(), TEXT("ViewFeed"));
		ViewFeed->SetHostSized(true);
		ViewFeed->Setup(nullptr, AViewCamCapture::ModeLabel(), true); // target arrives in tick

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("CentreColumn"));

		const bool bViewOnTop = CVarRectViewOnTop.GetValueOnGameThread() != 0;
		UWidget* TopWidget = bViewOnTop ? (UWidget*)ViewFeed : (UWidget*)UnitCam;
		UWidget* BottomWidget = bViewOnTop ? (UWidget*)UnitCam : (UWidget*)ViewFeed;
		TopSlot = Cast<UVerticalBoxSlot>(Column->AddChild(TopWidget));
		BottomSlot = Cast<UVerticalBoxSlot>(Column->AddChild(BottomWidget));
		for (UVerticalBoxSlot* ColSlot : { TopSlot.Get(), BottomSlot.Get() })
		{
			if (ColSlot)
			{
				ColSlot->SetHorizontalAlignment(HAlign_Fill);
				ColSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		if (BottomSlot)
		{
			BottomSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f)); // hairline gutter between the two
		}

		CentreBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CentreBox"));
		CentreBox->SetContent(Column);
		CentreColumn->AddChildToVerticalBox(CentreBox);
	}
	bLastViewOnTop = CVarRectViewOnTop.GetValueOnGameThread() != 0;

	// The squad-size bar, directly under the cam and only as wide as it — SyncWingsToCam matches
	// its width to the cam every tick, so the two read as one stacked unit.
	CompanyBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CompanyBox"));
	CompanyBox->SetContent(BuildCompanyStrip());
	if (UVerticalBoxSlot* StripSlot = CentreColumn->AddChildToVerticalBox(CompanyBox))
	{
		StripSlot->SetHorizontalAlignment(HAlign_Center);
		StripSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}

	// Right wing — cards mirror inward toward the cam; no second company readout.
	if (MusterRight)
	{
		MusterRight->SetShowCompany(false);
	}
	WingRightBox = BuildWing(MusterRight, HAlign_Left, TEXT("WingRight"));

	SyncWingsToCam();
}

void UEmberkeepHud::PublishHudOcclusion(const FGeometry& MyGeometry)
{
	UWorld* World = GetWorld();
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	// Measured, not derived: the rectangle's height is the sum of the company strip, the cam,
	// the paddings and the frame, and re-deriving that arithmetic here would rot the moment the
	// layout changes. Ask the widget how tall it actually ended up.
	const float ScreenH = (float)MyGeometry.GetLocalSize().Y; // the HUD root fills the viewport
	if (ScreenH <= 1.f || !Rect)
	{
		return;
	}

	const float RectH = (float)Rect->GetCachedGeometry().GetLocalSize().Y;
	if (RectH <= 1.f)
	{
		return; // not laid out yet — publishing 0 here would jerk the camera for a frame
	}

	Swarm->SetHudOccludedFraction((RectH + BandPadding) / ScreenH);
}

UBorder* UEmberkeepHud::BuildCompanyStrip()
{
	// Its own compartment: Steel edge over Dark ground, matching the rectangle's own framing so
	// it reads as a section of the same object rather than a floating label.
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CompanyFrame"));
	Frame->SetBrushColor(Demichrome::Steel());
	Frame->SetPadding(FMargin(1.f));

	UBorder* Fill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CompanyFill"));
	Fill->SetBrushColor(Demichrome::Dark());
	Fill->SetPadding(FMargin(6.f, 3.f));
	Frame->SetContent(Fill);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CompanyCol"));
	Fill->SetContent(Col);

	CompanyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompanyLabel"));
	CompanyLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	CompanyLabel->SetColorAndOpacity(FSlateColor(Demichrome::Pale()));
	if (UVerticalBoxSlot* LabelSlot = Col->AddChildToVerticalBox(CompanyLabel))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
	}

	CompanyMeter = WidgetTree->ConstructWidget<UStitchMeter>(UStitchMeter::StaticClass(), TEXT("CompanyMeter"));
	if (UVerticalBoxSlot* MeterSlot = Col->AddChildToVerticalBox(CompanyMeter))
	{
		MeterSlot->SetHorizontalAlignment(HAlign_Center);
	}

	return Frame;
}

void UEmberkeepHud::SyncWingsToCam()
{
	if (!UnitCam)
	{
		return;
	}

	const FVector2D Cam = UnitCam->GetPanelSizePx();
	if (Cam.Y <= 0.0)
	{
		return; // cam hasn't ticked yet — leave the boxes content-sized
	}

	// The strip spans exactly the cam beneath it, so the two sections stack as one column.
	if (CompanyBox)
	{
		CompanyBox->SetWidthOverride((float)Cam.X);
	}

	// Split column: the column carries the body-count size, the two Fill slots divide its
	// height by Rect.Split. Both applied every tick so the dials are live.
	if (bSplitColumn && CentreBox)
	{
		CentreBox->SetWidthOverride((float)Cam.X);
		CentreBox->SetHeightOverride((float)Cam.Y);

		const float Split = FMath::Clamp(CVarRectSplit.GetValueOnGameThread(), 0.05f, 0.95f);
		if (TopSlot)
		{
			FSlateChildSize S(ESlateSizeRule::Fill);
			S.Value = Split;
			TopSlot->SetSize(S);
		}
		if (BottomSlot)
		{
			FSlateChildSize S(ESlateSizeRule::Fill);
			S.Value = 1.f - Split;
			BottomSlot->SetSize(S);
		}

		// Point the feed at the live capture, spawning it on first use. Push the panel's real
		// aspect so the mirrored view is letterboxed to match instead of stretched.
		if (ViewFeed)
		{
			if (AViewCamCapture* Capture = AViewCamCapture::FindOrSpawn(GetWorld()))
			{
				const float ViewHeight = (float)Cam.Y * (CVarRectViewOnTop.GetValueOnGameThread() != 0 ? Split : 1.f - Split);
				Capture->SetPanelAspect(ViewHeight > 1.f ? (float)Cam.X / ViewHeight : 2.f);
				const bool bMinimap = AViewCamCapture::IsMinimapMode();
				if (Capture->GetRenderTarget() != ViewFeedTarget || bMinimap != bLastMinimap)
				{
					ViewFeedTarget = Capture->GetRenderTarget();
					bLastMinimap = bMinimap;
					ViewFeed->Setup(ViewFeedTarget, AViewCamCapture::ModeLabel(), true);
				}
			}
		}
	}

	if (!WingLeftBox && !WingRightBox)
	{
		return;
	}

	// Wings are a fixed fraction of the cam's width, and as tall as the WHOLE centre column —
	// cam plus the squad-size bar under it — so all three columns end flush and the rectangle
	// reads as one object. Measured rather than derived: the bar is content-sized, so its height
	// is not something this can compute.
	const float WingWidth = (float)Cam.X * FMath::Max(CVarMusterWingRatio.GetValueOnGameThread(), 0.05f);
	float ColumnHeight = (float)Cam.Y;
	if (CentreColumn)
	{
		const float Measured = (float)CentreColumn->GetCachedGeometry().GetLocalSize().Y;
		if (Measured > 1.f)
		{
			ColumnHeight = Measured;
		}
	}

	for (USizeBox* Box : { WingLeftBox.Get(), WingRightBox.Get() })
	{
		if (Box)
		{
			Box->SetWidthOverride(WingWidth);
			Box->SetHeightOverride(ColumnHeight);
		}
	}
}
