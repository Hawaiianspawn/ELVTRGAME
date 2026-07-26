#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "BreadboardHostWidget.generated.h"

class SBreadboardPanel;

/**
 * UMG wrapper around the breadboard panel, so the same tool can be dropped into an
 * EditorUtilityWidget blueprint (Content Browser > Editor Utilities > Editor Utility Widget,
 * then drag "ELVTR Breadboard" onto the canvas) as well as opened as the standalone tab.
 * All the behaviour lives in the Slate widget; this is just a host.
 */
UCLASS(meta = (DisplayName = "ELVTR Breadboard"))
class UBreadboardHostWidget : public UWidget
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SBreadboardPanel> Panel;
};
