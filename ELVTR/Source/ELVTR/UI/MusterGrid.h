#pragma once

#include "CoreMinimal.h"
#include "UI/EmberkeepWidget.h"
#include "MusterGrid.generated.h"

class UUniformGridPanel;

/**
 * The rank-and-file muster (menu spec §5a/§7): a grid of pips that is the squad's health AND
 * formation in one control — Pale = standing soldier/file, Steel = fallen. `Columns` = files.
 * Reads squad-level counts only; never iterates per-soldier actors. Built entirely in C++.
 */
UCLASS()
class ELVTR_API UMusterGrid : public UEmberkeepWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muster", meta = (ClampMin = "0"))
	int32 Size = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muster", meta = (ClampMin = "0"))
	int32 Standing = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muster", meta = (ClampMin = "1"))
	int32 Columns = 6;

	/** Pixel size of each pip (square). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muster", meta = (ClampMin = "1"))
	float PipSize = 8.f;

	UFUNCTION(BlueprintCallable, Category = "Muster")
	void SetMuster(int32 InSize, int32 InStanding, int32 InColumns);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void Rebuild();

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> Panel = nullptr;
};
