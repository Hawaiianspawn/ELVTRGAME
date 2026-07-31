#include "UI/SquadCard.h"
#include "UI/MusterGrid.h"
#include "UI/KindledPalette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void USquadCard::SetSquad(const FKindledSquad& InSquad, bool bInSelected)
{
	Squad = InSquad;
	bSelected = bInSelected;
	Refresh();
}

TSharedRef<SWidget> USquadCard::RebuildWidget()
{
	if (!Frame)
	{
		Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		WidgetTree->RootWidget = Frame;
		Frame->SetPadding(FMargin(2.f)); // the 2px reveal becomes the visible border

		Fill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Fill"));
		Fill->SetPadding(FMargin(5.f));
		Frame->SetContent(Fill);

		UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
		Fill->SetContent(Body);

		// header: name (Bone, fills) + standing/size (Pale, right)
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9));
		NameText->SetColorAndOpacity(FSlateColor(Demichrome::Bone()));
		SizeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SizeText"));
		SizeText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9));
		SizeText->SetColorAndOpacity(FSlateColor(Demichrome::Pale()));
		if (UHorizontalBoxSlot* NameSlot = Header->AddChildToHorizontalBox(NameText))
		{
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* SizeSlot = Header->AddChildToHorizontalBox(SizeText))
		{
			SizeSlot->SetHorizontalAlignment(HAlign_Right);
			SizeSlot->SetVerticalAlignment(VAlign_Center);
			SizeSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
		}
		if (UVerticalBoxSlot* HeaderSlot = Body->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		// muster grid
		Grid = WidgetTree->ConstructWidget<UMusterGrid>(UMusterGrid::StaticClass(), TEXT("Grid"));
		Body->AddChildToVerticalBox(Grid);

		// stance chip
		Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Chip"));
		Chip->SetPadding(FMargin(5.f, 2.f));
		ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChipText"));
		ChipText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 8));
		Chip->SetContent(ChipText);
		if (UVerticalBoxSlot* ChipSlot = Body->AddChildToVerticalBox(Chip))
		{
			ChipSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			ChipSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}

	Refresh();
	return Super::RebuildWidget();
}

void USquadCard::Refresh()
{
	if (!Frame)
	{
		return;
	}

	Frame->SetBrushColor(bSelected ? Demichrome::Pale() : Demichrome::Steel());
	Fill->SetBrushColor(Demichrome::Dark());

	NameText->SetText(Squad.DisplayName);
	SizeText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Squad.Standing, Squad.Size)));

	Grid->SetMuster(Squad.Size, Squad.Standing, Squad.Columns);

	// In the M1 mock, a squad's chip always reads as its active order (Pale fill / Dark text).
	Chip->SetBrushColor(Demichrome::Pale());
	ChipText->SetColorAndOpacity(FSlateColor(Demichrome::Dark()));
	ChipText->SetText(FText::FromString(KindledStanceToString(Squad.Stance)));
}
