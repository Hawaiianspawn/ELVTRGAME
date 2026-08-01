#include "UI/KindledHud.h"
#include "UI/MusterPanel.h"
#include "UI/KindledUITypes.h"
#include "UI/KindledPalette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/World.h"
#include "Mass/SwarmSubsystem.h"

namespace
{
	TAutoConsoleVariable<float> CVarBandHeight(
		TEXT("Kindled.UI.BandHeight"), 190.f,
		TEXT("Height in px of the muster shelf along the bottom of the screen.\n")
		TEXT("\n")
		TEXT("This exists because the shelf has nothing else to size it: the cards render at\n")
		TEXT("natural size, which is roughly 40%% of the screen — the tallest squad card is as\n")
		TEXT("tall as its member count makes it.\n")
		TEXT("\n")
		TEXT("Scaling is DOWN-ONLY, so a small roster sits at natural size and only a big one is\n")
		TEXT("shrunk to fit. The camera compensates automatically: PublishHudOcclusion measures\n")
		TEXT("the band's real height, so raising this pushes the view up to match. [80..600]"),
		ECVF_Default);
}

void UKindledHud::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Kindled.UI.BandHeight is a live dial, so drive the shelf every frame rather than only
	// at build time.
	SyncShelf();

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

void UKindledHud::PushLiveMuster()
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

	// Nothing has ever spawned: the shelf sits empty rather than churning a rebuild per tick
	// over a roster that doesn't exist yet. Goes live the moment real retinue do.
	if (Alive <= 0 && PeakRetinue <= 0)
	{
		return;
	}

	PeakRetinue = FMath::Max(PeakRetinue, Alive);
	const ESwarmStance Stance = Swarm->GetStance();
	const int32 StanceInt = (int32)Stance;

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
	static const TCHAR* const SquadNames[] = {
		TEXT("Shield"), TEXT("Vets"), TEXT("Spearmen"), TEXT("Banner"), TEXT("Reserve") };

	const int32 SquadCount = FMath::Min<int32>(USwarmSubsystem::MaxSquads, UE_ARRAY_COUNT(SquadPeak));
	TArray<FKindledSquad> Live;
	Live.Reserve(SquadCount);
	for (int32 i = 0; i < SquadCount; ++i)
	{
		const int32 Standing = Swarm->GetSquadStanding(i);
		SquadPeak[i] = FMath::Max(SquadPeak[i], Standing);
		if (SquadPeak[i] <= 0)
		{
			continue; // squad never populated
		}
		FKindledSquad S;
		S.Size = SquadPeak[i];
		S.Standing = FMath::Min(Standing, S.Size);
		S.Columns = FMath::Clamp(FMath::RoundToInt(FMath::Sqrt((float)FMath::Max(S.Size, 1)) * 1.3f), 3, 10);
		S.bWide = S.Size > 30;
		S.Stance = Stance;
		S.DisplayName = FText::FromString(SquadNames[i % UE_ARRAY_COUNT(SquadNames)]);
		Live.Add(S);
	}

	Muster->SetSquads(Live);
}

TSharedRef<SWidget> UKindledHud::RebuildWidget()
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

		BuildBand();
	}

	return Super::RebuildWidget();
}

void UKindledHud::BuildBand()
{
	if (!Band || !Muster)
	{
		return;
	}

	// THE rectangle: one Steel frame over one Dark ground, holding the muster shelf. Everything
	// inside drops its own chrome so this is the only visible edge — the motif surround will
	// eventually wrap this border and nothing else.
	Rect = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rect"));
	Rect->SetBrushColor(Demichrome::Steel());
	Rect->SetPadding(FMargin(2.f));

	UBorder* Ground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RectGround"));
	Ground->SetBrushColor(Demichrome::Dark());
	Ground->SetPadding(FMargin(3.f));
	Rect->SetContent(Ground);

	if (UOverlaySlot* RectSlot = Band->AddChildToOverlay(Rect))
	{
		RectSlot->SetHorizontalAlignment(HAlign_Center);
		RectSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	// One centred shelf carrying the whole roster: the player's viewport is the only camera, so
	// the band is just the muster. The panel carries its own company readout ("Vanguard Company
	// N/M · S squads") — the band has no separate strip to hoist it into.
	Muster->SetChrome(false);
	Muster->SetFlow(EKindledMusterFlow::Row);
	Muster->SetContentAlignment(HAlign_Left);
	Muster->SetShowCompany(true);

	// Scale-to-fit bounded by Kindled.UI.BandHeight. DownOnly so a thin roster keeps its natural
	// size and only a full company gets shrunk.
	UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	Scale->SetStretch(EStretch::ScaleToFit);
	Scale->SetStretchDirection(EStretchDirection::DownOnly);
	Scale->SetContent(Muster);

	ShelfBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ShelfBox"));
	ShelfBox->SetHeightOverride(FMath::Clamp(CVarBandHeight.GetValueOnGameThread(), 80.f, 600.f));
	ShelfBox->SetContent(Scale);
	Ground->SetContent(ShelfBox);
}

void UKindledHud::PublishHudOcclusion(const FGeometry& MyGeometry)
{
	UWorld* World = GetWorld();
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}

	// Measured, not derived: the rectangle's height is the sum of the shelf, the paddings and
	// the frame, and re-deriving that arithmetic here would rot the moment the layout changes.
	// Ask the widget how tall it actually ended up.
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

void UKindledHud::SyncShelf()
{
	// Both axes are overridden, and that matters: a UScaleBox reports its child's UNSCALED
	// desired size, so a box given only a height reserves the full-size panel's WIDTH while
	// painting shrunk content — leaving dead ground inside the band's frame. So measure the
	// panel, work out the scale the height forces, and size the box to the result.
	if (!ShelfBox || !Muster)
	{
		return;
	}

	const float Want = FMath::Clamp(CVarBandHeight.GetValueOnGameThread(), 80.f, 600.f);
	const FVector2D Natural = Muster->GetDesiredSize();
	if (Natural.Y > 1.0 && Natural.X > 1.0)
	{
		const float S = FMath::Min(1.f, Want / (float)Natural.Y); // DownOnly, matching the ScaleBox
		ShelfBox->SetHeightOverride((float)Natural.Y * S);
		ShelfBox->SetWidthOverride((float)Natural.X * S);
	}
}
