#pragma once

#include "CoreMinimal.h"
#include "UI/EmberkeepWidget.h"
#include "UI/EmberkeepUITypes.h"
#include "SquadCard.generated.h"

class UBorder;
class UTextBlock;
class UMusterGrid;

/**
 * One square squad card (menu spec §7): a bordered card whose body is the rank-and-file
 * muster grid, with a header (name + standing/size) and a stance chip. Default border =
 * Steel; selected jumps to Pale. Bound to one squad's state only. Built entirely in C++.
 */
UCLASS()
class ELVTR_API USquadCard : public UEmberkeepWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FEmberkeepSquad Squad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bSelected = false;

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void SetSquad(const FEmberkeepSquad& InSquad, bool bInSelected);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void Refresh();

	UPROPERTY(Transient) TObjectPtr<UBorder> Frame = nullptr;   // outer: border colour
	UPROPERTY(Transient) TObjectPtr<UBorder> Fill = nullptr;    // inner: dark ground
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SizeText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UMusterGrid> Grid = nullptr;
	UPROPERTY(Transient) TObjectPtr<UBorder> Chip = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ChipText = nullptr;
};
