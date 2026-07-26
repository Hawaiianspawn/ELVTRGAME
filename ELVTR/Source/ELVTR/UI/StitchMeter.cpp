#include "UI/StitchMeter.h"
#include "UI/EmberkeepPalette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"

void UStitchMeter::SetValues(int32 InTotal, int32 InFull)
{
	Total = FMath::Max(1, InTotal);
	Full = FMath::Clamp(InFull, 0, Total);
	Rebuild();
}

TSharedRef<SWidget> UStitchMeter::RebuildWidget()
{
	if (!PipBox)
	{
		PipBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PipBox"));
		WidgetTree->RootWidget = PipBox;
	}
	Rebuild();
	return Super::RebuildWidget();
}

void UStitchMeter::Rebuild()
{
	if (!PipBox)
	{
		return;
	}

	PipBox->ClearChildren();

	const int32 T = FMath::Max(1, Total);
	const int32 F = FMath::Clamp(Full, 0, T);

	for (int32 i = 0; i < T; ++i)
	{
		UImage* Pip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Pip->SetBrush(Demichrome::SolidBrush(i < F ? Demichrome::Pale() : Demichrome::Dark()));

		if (UHorizontalBoxSlot* PipSlot = PipBox->AddChildToHorizontalBox(Pip))
		{
			PipSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			PipSlot->SetPadding(FMargin(1.f, 0.f));
		}
	}
}
