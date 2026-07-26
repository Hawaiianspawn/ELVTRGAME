#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "EmberkeepWidget.generated.h"

/**
 * Base for all Emberkeep UI widgets. Inherits CommonUI's UCommonUserWidget so the widget
 * family participates in CommonUI input/focus routing (screens use UCommonActivatableWidget
 * on top of this contract). Widgets are defined in C++ (docs/ui/UI-PROTOTYPE-PLAN.md §2).
 */
UCLASS(Abstract)
class ELVTR_API UEmberkeepWidget : public UCommonUserWidget
{
	GENERATED_BODY()
};
