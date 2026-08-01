#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/KindledUITypes.h"
#include "MusterPanel.generated.h"

class UBorder;
class UPanelWidget;
class UStitchMeter;
class UTextBlock;
class UVerticalBox;

/** How the squad cards run. Row = the menu's wide shelf; Column = a HUD wing flanking the Unit Cam. */
UENUM()
enum class EKindledMusterFlow : uint8
{
	Row,
	Column
};

/**
 * The Muster (menu spec §2/§5a): the company readout — a standing meter over a row of
 * square squad cards. This is the well content shared by the menu and the combat HUD,
 * fed from USwarmSubsystem squad state via UKindledHud::PushLiveMuster.
 *
 * The combat HUD hosts one of these as the band's centred shelf, with its own chrome
 * suppressed (SetChrome) so the band's frame is the only visible edge.
 */
UCLASS()
class ELVTR_API UMusterPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Selected card index (Pale border). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Muster")
	int32 SelectedIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Muster")
	void SetSquads(const TArray<FKindledSquad>& InSquads);

	/** Row (menu shelf) vs Column (a tall, narrow host slot). */
	void SetFlow(EKindledMusterFlow InFlow);

	/** Draw our own Steel border + Dark ground (standalone), or nothing (embedded in a band
	 *  that already owns the rectangle). */
	void SetChrome(bool bInChrome);

	/** Which edge the content packs against — the wings mirror inward toward the cam. */
	void SetContentAlignment(EHorizontalAlignment InAlign);

	/** Show the company label + standing meter. Off for the HUD wings — the band hoists the
	 *  company readout into its own section above the cam, since the bar is wide enough to
	 *  force a wing's scale-to-fit down and shrink its cards. */
	void SetShowCompany(bool bInShow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void Rebuild();
	void ApplyChrome();

	UPROPERTY()
	TArray<FKindledSquad> Squads;

	EKindledMusterFlow Flow = EKindledMusterFlow::Row;
	bool bChrome = true;
	bool bShowCompany = true;
	EHorizontalAlignment ContentAlign = HAlign_Left;

	UPROPERTY(Transient) TObjectPtr<UBorder> Panel = nullptr;       // outer border colour
	UPROPERTY(Transient) TObjectPtr<UBorder> Ground = nullptr;      // inner dark fill
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Column = nullptr; // label / meter / cards
	UPROPERTY(Transient) TObjectPtr<UStitchMeter> CompanyMeter = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CompanyLabel = nullptr;
	UPROPERTY(Transient) TObjectPtr<UPanelWidget> CardBox = nullptr; // horizontal or vertical, per Flow
};
