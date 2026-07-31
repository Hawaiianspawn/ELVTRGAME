#include "UI/MusterGrid.h"
#include "UI/KindledPalette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"

void UMusterGrid::SetMuster(int32 InSize, int32 InStanding, int32 InColumns)
{
	Size = FMath::Max(0, InSize);
	Standing = FMath::Clamp(InStanding, 0, Size);
	Columns = FMath::Max(1, InColumns);
	Rebuild();
}

TSharedRef<SWidget> UMusterGrid::RebuildWidget()
{
	if (!Panel)
	{
		Panel = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Panel"));
		WidgetTree->RootWidget = Panel;
	}
	Rebuild();
	return Super::RebuildWidget();
}

void UMusterGrid::Rebuild()
{
	if (!Panel)
	{
		return;
	}

	Panel->ClearChildren();
	Panel->SetSlotPadding(FMargin(1.f));

	const int32 Cols = FMath::Max(1, Columns);
	const int32 Standing_ = FMath::Clamp(Standing, 0, Size);

	for (int32 i = 0; i < Size; ++i)
	{
		UImage* Pip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Pip->SetBrush(Demichrome::SolidBrush(i < Standing_ ? Demichrome::Pale() : Demichrome::Steel()));
		Pip->SetDesiredSizeOverride(FVector2D(PipSize, PipSize));

		if (UUniformGridSlot* GridSlot = Panel->AddChildToUniformGrid(Pip, i / Cols, i % Cols))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}
