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

		// verb chip (task-144) — "what this one can currently do", under the order it is under.
		// A second chip rather than a second line in the existing one: the stance is an ORDER
		// and the verb is a CAPABILITY, and a card that runs them together makes a spent verb
		// look like a stance the player did not give.
		VerbChip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VerbChip"));
		VerbChip->SetPadding(FMargin(5.f, 2.f));
		VerbChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VerbChipText"));
		VerbChipText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 8));
		VerbChip->SetContent(VerbChipText);
		if (UVerticalBoxSlot* VerbSlot = Body->AddChildToVerticalBox(VerbChip))
		{
			VerbSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
			VerbSlot->SetHorizontalAlignment(HAlign_Left);
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

	// A squad's chip reads as its active order (Pale fill / Dark text).
	Chip->SetBrushColor(Demichrome::Pale());
	ChipText->SetColorAndOpacity(FSlateColor(Demichrome::Dark()));
	ChipText->SetText(FText::FromString(LexToString(Squad.Stance)));

	// The verb chip inverts against the stance chip when the verb is spent — Steel ground with
	// Pale text instead of a Pale ground — so "ready" and "not ready" are a fill difference at
	// card size rather than a word you have to read.
	if (!VerbChip)
	{
		return;
	}
	const bool bHasVerb = !Squad.Verb.IsEmpty();
	VerbChip->SetVisibility(bHasVerb ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bHasVerb)
	{
		VerbChip->SetBrushColor(Squad.bVerbReady ? Demichrome::Bone() : Demichrome::Steel());
		VerbChipText->SetColorAndOpacity(FSlateColor(
			Squad.bVerbReady ? Demichrome::Dark() : Demichrome::Pale()));
		VerbChipText->SetText(Squad.bVerbReady
			? Squad.Verb
			: FText::FromString(FString::Printf(TEXT("%s %.0fs"),
				*Squad.Verb.ToString(), FMath::CeilToFloat(Squad.VerbCooldown))));
	}
}
