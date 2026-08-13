#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/KindledUITypes.h"
#include "KindledHud.generated.h"

class UBorder;
class UMusterPanel;
class UOverlay;
class USizeBox;

/**
 * The combat-HUD bottom command band: ONE rectangle holding the retinue muster as a single
 * centred shelf. The player's viewport is the only camera (owner call 2026-07-28), so the
 * band is just the muster; the motif surround hangs off the rectangle's frame later.
 */
UCLASS()
class ELVTR_API UKindledHud : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Build the rectangle and the shelf inside it. Called once, from RebuildWidget. */
	void BuildBand();

	/** Read live retinue count + stance from USwarmSubsystem and push them to the muster. */
	void PushLiveMuster();

	/** Size the shelf box to the panel scaled by Kindled.UI.BandHeight. */
	void SyncShelf();

	/** Tell the sim how much of the viewport we cover, so the hero camera can bias around us. */
	void PublishHudOcclusion(const FGeometry& MyGeometry);

	/** Inset of the band from the screen edge; counts toward the occluded height. */
	static constexpr float BandPadding = 12.f;

	UPROPERTY(Transient) TObjectPtr<UOverlay> Band = nullptr;   // dock host for the rectangle
	UPROPERTY(Transient) TObjectPtr<UBorder> Rect = nullptr;    // THE rectangle — motifs frame this
	UPROPERTY(Transient) TObjectPtr<UMusterPanel> Muster = nullptr;
	/** The shelf's size box — what Kindled.UI.BandHeight actually drives. */
	UPROPERTY(Transient) TObjectPtr<USizeBox> ShelfBox = nullptr;

	// Live-muster tracking. PeakRetinue is the high-water headcount (the company denominator);
	// SquadPeak is the per-unit high-water (each card's Size).
	//
	// LastAlive is a SIGNATURE now, not a headcount (task-144): a card shows a verb cooldown
	// counting down, which is a visible change with no body count behind it, so the gate folds
	// in each of the seven's own order and whole-second readiness. The separate LastStance is
	// gone with the global stance it tracked — cards read their unit's own order since the
	// pivot, and one number for eight of them was never going to be right.
	int32 PeakRetinue = 0;
	int32 SquadPeak[8] = {}; // sized to USwarmSubsystem::MaxSquads
	int32 LastAlive = -1;
	float RefreshTimer = 0.f;
};
