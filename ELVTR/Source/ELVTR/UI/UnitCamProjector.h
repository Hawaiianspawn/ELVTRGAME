#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/EmberkeepWidget.h"
#include "UI/UnitCamDirector.h"
#include "UnitCamProjector.generated.h"

/**
 * One projected unit in panel-normalised space (0..1, y-down), ready to blit.
 *
 * POD on purpose: the projection reads live Mass UObjects on the game thread in
 * NativeTick and hands the Slate canvas nothing but numbers, so OnPaint touches no
 * UObjects and there is no lifetime hazard across the UMG/Slate seam.
 */
struct FUnitCamBillboard
{
	FVector2f Center = FVector2f::ZeroVector; // 0..1 within the panel, y-down
	float HalfSize = 0.f;                      // half-width as a fraction of the panel width
	float Depth = 0.f;                         // camera-space depth, for the far->near sort
	FLinearColor Color = FLinearColor::White;  // for sprites this is a light-only tint (Lit,Lit,Lit,1)
	int32 Cell = INDEX_NONE;                   // T_Swarm_2bit atlas cell, or INDEX_NONE for a flat quad
	bool bHero = false;                        // the bearer: bigger, never dimmed, drawn over everyone
};

class SUnitCamCanvas;
struct FSlateBrush;
class UTexture2D;
class USizeBox;
class UBorder;

/**
 * Thin UMG wrapper around the Slate billboard canvas, so it can sit inside a
 * WidgetTree (and therefore inside a normal UUserWidget that still gets NativeTick).
 */
UCLASS()
class ELVTR_API UUnitCamCanvasWidget : public UWidget
{
	GENERATED_BODY()

public:
	void SetBillboards(TArray<FUnitCamBillboard>&& InBillboards);

	/**
	 * One pre-sliced brush per cell of the T_Swarm_2bit atlas, indexed by FUnitCamBillboard::Cell.
	 * Every drawn body — brood, retinue and the bearer alike — picks its frame out of this array,
	 * so the panel can show per-unit walk/attack/hit exactly as the world view does.
	 */
	void SetCellBrushes(TArray<FSlateBrush>&& InBrushes);

	/** Live soldier-size multiple (framing dial); pushed each tick. */
	void SetSoldierScale(float InScale);

	/** Extra size multiple applied to the hero billboard on top of SoldierScale. */
	void SetHeroScale(float InScale);

	/**
	 * Where a body sits relative to its projected point: 1 = standing on it, 0.5-worth of
	 * body centred on it at 0. The projected point is ground contact, so anything below 1
	 * draws units partly buried and makes every size dial dig downward.
	 */
	void SetFootAnchor(float InAnchor);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	TSharedPtr<SUnitCamCanvas> Canvas;
};

/**
 * Projection-prototype Unit Cam (docs/RENDERING-LIGHTING.md §4d).
 *
 * Emulates a second camera WITHOUT a second scene render. A virtual camera is a
 * pure math construct; each swarm unit is projected and scaled by 1/depth
 * (Doom-sprite forced perspective) and drawn as a billboard in this panel. It
 * reads the same live Mass buffers the Niagara render bridge reads, so it costs
 * one projection loop over the units in frame — not a full-scene SceneCapture.
 *
 * Units draw their frame from the shared T_Swarm_2bit atlas. Lighting is the world's
 * flame falloff plus a close-up-specific model: a directional term (how much of the
 * lit hemisphere the virtual lens can see), per-team value ranges, and banded light
 * tiers — see RENDERING-LIGHTING.md §4d "Panel shading" for why distance alone read
 * as a flat grey crowd. The per-direction facing-bucket sheet is still later.
 *
 * Toggle in a play session with the console command `Emberkeep.UI.UnitCamProj`.
 * Virtual-camera dials: `Emberkeep.UnitCamProj.*` (Fov, Dist, Height, Yaw, Range, Scale).
 * Named apart from the capture-based unit cam's `Emberkeep.UI.UnitCam.*` on purpose:
 * this is a different, capture-free approach living alongside it, not a replacement yet.
 */
UCLASS()
class ELVTR_API UUnitCamProjector : public UEmberkeepWidget
{
	GENERATED_BODY()

public:
	/** Live panel size in px (width, height), recomputed each tick from the body count. The HUD
	 *  reads this to size the retinue wings, so the whole command rectangle scales as one. */
	FVector2D GetPanelSizePx() const { return PanelSizePx; }

	/** Frame thickness in px. The embedded HUD cam drops to a hairline (the band owns the
	 *  rectangle's edge); standalone it keeps the fat boxy frame. */
	void SetFrameThickness(float InPx);

	/**
	 * Stop overriding our own SizeBox and fill the slot the host gives us instead. The body-
	 * count panel size is still computed and published via GetPanelSizePx() — the host applies
	 * it to whatever container it wants. Used when the cam is one half of a split column.
	 */
	void SetHostSized(bool bInHostSized);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<UUnitCamCanvasWidget> CanvasWidget = nullptr;

	// Resized every tick by total bodies on the field (individual <-> mass), and the frame is
	// tinted toward the reserved red as the tide outnumbers the host (threat read).
	UPROPERTY(Transient) TObjectPtr<USizeBox> RootBox = nullptr;
	UPROPERTY(Transient) TObjectPtr<UBorder> FrameBorder = nullptr;

	/** The 4x2 T_Swarm_2bit atlas (brood row 0, retinue row 1) that every billboard slices its
	 *  frame out of. Loaded from Content once and held so it isn't GC'd. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> SwarmAtlas = nullptr;
	bool bAtlasLoadAttempted = false;  // load once, even on failure, so we don't retry every frame
	bool bCellBrushesPushed = false;   // the slices never change; build them once, not per tick

	/** The camera manager seed — resolves which world point the virtual camera follows. */
	FUnitCamDirector Director;

	FVector2D PanelSizePx = FVector2D::ZeroVector;
	float FrameThickness = 4.f;
	bool bHostSized = false;
};
