#include "UI/MusterPanel.h"
#include "UI/SquadCard.h"
#include "UI/StitchMeter.h"
#include "UI/KindledPalette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Styling/CoreStyle.h"

void UMusterPanel::SetSquads(const TArray<FKindledSquad>& InSquads)
{
	Squads = InSquads;
	Rebuild();
}

void UMusterPanel::SetFlow(EKindledMusterFlow InFlow)
{
	if (Flow != InFlow)
	{
		Flow = InFlow;
		Rebuild(); // Rebuild swaps the card container to match
	}
}

void UMusterPanel::SetChrome(bool bInChrome)
{
	bChrome = bInChrome;
	ApplyChrome();
}

void UMusterPanel::SetContentAlignment(EHorizontalAlignment InAlign)
{
	if (ContentAlign != InAlign)
	{
		ContentAlign = InAlign;
		ApplyChrome();
		Rebuild();
	}
}

void UMusterPanel::SetShowCompany(bool bInShow)
{
	if (bShowCompany != bInShow)
	{
		bShowCompany = bInShow;
		ApplyChrome();
	}
}

TSharedRef<SWidget> UMusterPanel::RebuildWidget()
{
	if (!Panel)
	{
		// Content-sized root: the Steel border IS the panel edge. Placement (corner / band slot)
		// is owned by whatever hosts this panel — the HUD band, or a preview command's overlay.
		Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		WidgetTree->RootWidget = Panel;
		Panel->SetPadding(FMargin(1.f));
		Panel->SetBrushColor(Demichrome::Steel());

		Ground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Ground"));
		Ground->SetPadding(FMargin(8.f));
		Ground->SetBrushColor(Demichrome::Dark());
		Panel->SetContent(Ground);

		Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		Ground->SetContent(Column);

		// company header: label + standing meter
		CompanyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompanyLabel"));
		CompanyLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
		CompanyLabel->SetColorAndOpacity(FSlateColor(Demichrome::Pale()));
		if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(CompanyLabel))
		{
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		CompanyMeter = WidgetTree->ConstructWidget<UStitchMeter>(UStitchMeter::StaticClass(), TEXT("CompanyMeter"));
		if (UVerticalBoxSlot* MeterSlot = Column->AddChildToVerticalBox(CompanyMeter))
		{
			MeterSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		ApplyChrome();
	}

	Rebuild();
	return Super::RebuildWidget();
}

void UMusterPanel::ApplyChrome()
{
	if (!Panel || !Ground || !Column)
	{
		return;
	}

	// Embedded (wing) mode: no border, no ground, no inset — the host band's frame is the only
	// edge, so the cam and the wings read as one solid rectangle rather than three panels.
	const FLinearColor Clear(0.f, 0.f, 0.f, 0.f);
	Panel->SetBrushColor(bChrome ? Demichrome::Steel() : Clear);
	Panel->SetPadding(bChrome ? FMargin(1.f) : FMargin(0.f));
	Ground->SetBrushColor(bChrome ? Demichrome::Dark() : Clear);
	Ground->SetPadding(bChrome ? FMargin(8.f) : FMargin(0.f));

	const ESlateVisibility CompanyVis = bShowCompany ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	if (CompanyLabel) { CompanyLabel->SetVisibility(CompanyVis); }
	if (CompanyMeter) { CompanyMeter->SetVisibility(CompanyVis); }

	// Column children pack against the same edge as the cards, so a right wing mirrors the left.
	for (int32 i = 0; i < Column->GetChildrenCount(); ++i)
	{
		if (UVerticalBoxSlot* ColumnSlot = Cast<UVerticalBoxSlot>(Column->GetChildAt(i)->Slot))
		{
			ColumnSlot->SetHorizontalAlignment(ContentAlign);
		}
	}
}

void UMusterPanel::Rebuild()
{
	if (!Column)
	{
		return;
	}

	// The card container follows Flow: a horizontal shelf for the menu, a vertical stack for a
	// HUD wing (a wing is tall and narrow, so cards run down it, not across).
	const bool bColumnFlow = Flow == EKindledMusterFlow::Column;
	const TSubclassOf<UPanelWidget> WantClass = bColumnFlow
		? TSubclassOf<UPanelWidget>(UVerticalBox::StaticClass())
		: TSubclassOf<UPanelWidget>(UHorizontalBox::StaticClass());
	if (!CardBox || CardBox->GetClass() != WantClass.Get())
	{
		if (CardBox)
		{
			Column->RemoveChild(CardBox);
		}
		// Unnamed on purpose: a flow flip constructs a second box while the first is still alive
		// in the widget tree's outer, and an explicit name would collide.
		CardBox = WidgetTree->ConstructWidget<UPanelWidget>(WantClass);
		Column->AddChildToVerticalBox(CardBox);
		ApplyChrome(); // the new slot needs the shared alignment
	}

	CardBox->ClearChildren();

	int32 TotalSize = 0;
	int32 TotalStanding = 0;

	for (int32 i = 0; i < Squads.Num(); ++i)
	{
		const FKindledSquad& S = Squads[i];
		TotalSize += S.Size;
		TotalStanding += S.Standing;

		USquadCard* Card = WidgetTree->ConstructWidget<USquadCard>(USquadCard::StaticClass());
		Card->SetSquad(S, i == SelectedIndex);

		if (bColumnFlow)
		{
			if (UVerticalBoxSlot* CardSlot = Cast<UVerticalBoxSlot>(CardBox->AddChild(Card)))
			{
				CardSlot->SetPadding(FMargin(0.f, 3.f));
				CardSlot->SetHorizontalAlignment(ContentAlign);
			}
		}
		else if (UHorizontalBoxSlot* CardSlot = Cast<UHorizontalBoxSlot>(CardBox->AddChild(Card)))
		{
			CardSlot->SetPadding(FMargin(3.f, 0.f));
			CardSlot->SetVerticalAlignment(VAlign_Bottom);
		}
	}

	if (CompanyMeter)
	{
		// 24 segments reads as a coarse company bar regardless of exact headcount.
		const int32 Segments = 24;
		const int32 FullSegments = (TotalSize > 0)
			? FMath::RoundToInt(Segments * (float)TotalStanding / (float)TotalSize)
			: 0;
		CompanyMeter->SetValues(Segments, FullSegments);
	}

	if (CompanyLabel)
	{
		CompanyLabel->SetText(FText::FromString(FString::Printf(
			TEXT("Vanguard Company   %d / %d   ·   %d squads"),
			TotalStanding, TotalSize, Squads.Num())));
	}
}
