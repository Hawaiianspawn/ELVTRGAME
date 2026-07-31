#pragma once

#include "CoreMinimal.h"
#include "UI/KindledWidget.h"
#include "StitchMeter.generated.h"

class UHorizontalBox;

/**
 * Segmented meter (menu spec §7): a horizontal row of pips, NOT a smooth UProgressBar.
 * Full = Pale, empty = Dark. Used for hero Vigor and the company standing readouts.
 * Built entirely in C++.
 */
UCLASS()
class ELVTR_API UStitchMeter : public UKindledWidget
{
	GENERATED_BODY()

public:
	/** Total segments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter", meta = (ClampMin = "1"))
	int32 Total = 14;

	/** Filled (Pale) segments; the rest read Dark. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter", meta = (ClampMin = "0"))
	int32 Full = 14;

	UFUNCTION(BlueprintCallable, Category = "Meter")
	void SetValues(int32 InTotal, int32 InFull);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void Rebuild();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PipBox = nullptr;
};
