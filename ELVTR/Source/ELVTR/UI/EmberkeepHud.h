#pragma once

#include "CoreMinimal.h"
#include "UI/EmberkeepWidget.h"
#include "UI/EmberkeepUITypes.h"
#include "EmberkeepHud.generated.h"

class UBorder;
class UEmberkeepCamFeed;
class UMusterPanel;
class UOverlay;
class USizeBox;
class UVerticalBox;
class UStitchMeter;
class UTextBlock;
class UTextureRenderTarget2D;
class UUnitCamProjector;
class UVerticalBoxSlot;

/**
 * The combat-HUD bottom command band: ONE rectangle — the retinue muster split into a left and
 * a right wing flanking the projection Unit Cam, all inside a single frame (the motif surround
 * hangs off that frame later). The wings are re-sized every tick from the cam's live panel size,
 * so the whole rectangle grows and shrinks as one object as the cam scales with the body count.
 * bWithCams=false gives a muster-only preview (single centred shelf, no split).
 */
UCLASS()
class ELVTR_API UEmberkeepHud : public UEmberkeepWidget
{
	GENERATED_BODY()

public:
	void Setup(bool bWithCams);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UseMockData();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void RebuildBand();

	/** Read live retinue count + stance from USwarmSubsystem and push them to the muster
	 *  (M2 step 1). Once real units exist this replaces the mock; before then it no-ops so
	 *  the muster-only preview still shows something. */
	void PushLiveMuster();

	/** Push the given squads into the wings (split left/right) or the single centred shelf. */
	void ApplyMusterSquads(const TArray<FEmberkeepSquad>& InSquads);

	/** Match the wing boxes and the company strip to the cam's live panel size, so the whole
	 *  rectangle stays one object as the cam scales. */
	void SyncWingsToCam();

	/** Build the company strip — the standing bar in its own compartment above the cam. */
	UBorder* BuildCompanyStrip();

	/** Tell the sim how much of the viewport we cover, so the hero camera can bias around us. */
	void PublishHudOcclusion(const FGeometry& MyGeometry);

	/** Inset of the band from the screen edge; counts toward the occluded height. */
	static constexpr float BandPadding = 12.f;

	/** Gap between a retinue wing and the Unit Cam. Deliberately tight — the three columns
	 *  should read as one object, not as three panels sharing a background. */
	static constexpr float WingGutter = 1.f;

	UPROPERTY(Transient) TObjectPtr<UOverlay> Band = nullptr;   // dock host for the rectangle
	UPROPERTY(Transient) TObjectPtr<UBorder> Rect = nullptr;    // THE rectangle — motifs frame this
	UPROPERTY(Transient) TObjectPtr<UMusterPanel> Muster = nullptr;      // left wing (or the whole shelf)
	UPROPERTY(Transient) TObjectPtr<UMusterPanel> MusterRight = nullptr; // right wing
	UPROPERTY(Transient) TObjectPtr<USizeBox> WingLeftBox = nullptr;
	UPROPERTY(Transient) TObjectPtr<USizeBox> WingRightBox = nullptr;
	UPROPERTY(Transient) TObjectPtr<UUnitCamProjector> UnitCam = nullptr;

	// The company strip: its own compartment above the cam, hoisted out of the wings so its
	// width can't drive the wings' scale-to-fit down.
	UPROPERTY(Transient) TObjectPtr<USizeBox> CompanyBox = nullptr;
	UPROPERTY(Transient) TObjectPtr<UStitchMeter> CompanyMeter = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CompanyLabel = nullptr;

	// The split centre column: the mirrored game view stacked with the Unit Cam in one
	// VerticalBox, dividing the column's height by Emberkeep.UI.Rect.Split.
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CentreColumn = nullptr; // cam + squad-size bar
	UPROPERTY(Transient) TObjectPtr<USizeBox> CentreBox = nullptr;
	UPROPERTY(Transient) TObjectPtr<UEmberkeepCamFeed> ViewFeed = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> ViewFeedTarget = nullptr;
	UPROPERTY(Transient) TObjectPtr<UVerticalBoxSlot> TopSlot = nullptr;
	UPROPERTY(Transient) TObjectPtr<UVerticalBoxSlot> BottomSlot = nullptr;

	bool bSplitColumn = false;   // the column is currently built as two panels
	bool bLastViewOnTop = true;  // watched so a flip of the CVar triggers one rebuild
	bool bLastMinimap = true;    // watched so the panel's caption follows the capture mode

	bool bShowCams = false; // show the Unit Cam (false = muster-only preview)

	// Live-muster tracking. PeakRetinue is the high-water headcount (the company denominator);
	// SquadPeak is the per-squad high-water (each card's Size); the rest gate rebuilds so cards
	// only re-lay when something visible changed.
	int32 PeakRetinue = 0;
	int32 SquadPeak[8] = {}; // sized to USwarmSubsystem::MaxSquads
	int32 LastAlive = -1;
	int32 LastStance = -1;
	float RefreshTimer = 0.f;
};
