#include "Breadboard/BreadboardHostWidget.h"

#include "Breadboard/SBreadboardPanel.h"

#define LOCTEXT_NAMESPACE "Breadboard"

TSharedRef<SWidget> UBreadboardHostWidget::RebuildWidget()
{
	Panel = SNew(SBreadboardPanel);
	return Panel.ToSharedRef();
}

void UBreadboardHostWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Panel.Reset();
}

#if WITH_EDITOR
const FText UBreadboardHostWidget::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "ELVTR");
}
#endif

#undef LOCTEXT_NAMESPACE
