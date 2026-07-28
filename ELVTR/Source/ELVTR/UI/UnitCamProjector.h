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

	/**
	 * Which sprite SHEET this body draws from — index into SUnitCamCanvas's BrushSets, one
	 * TArray<FSlateBrush> per texture. 0 is always the shared T_Swarm_2bit atlas (brood, and
	 * retinue when SoldierVariants is off); the rest are named in UnitCamSprite
	 * (UnitCamProjector.cpp) — the bearer's own T_Hero_Vanguard, the high-resolution knight,
	 * and the high-resolution archer. Added so a billboard is no longer forced to share one
	 * atlas with every other body. Task-050 originally drew retinue from six 48px
	 * soldier-roster variants (docs/art/soldier-roster-v1.md); the owner then rejected that
	 * as a resolution downgrade ("we degraded with the units again") and named exactly two
	 * high-resolution units to replace it — the six variant .uassets/sheets still exist in
	 * Content/RawArt, just unreferenced by this widget now.
	 */
	uint8 SpriteSet = 0;

	int32 Cell = INDEX_NONE;                   // cell within that sheet, or INDEX_NONE for a flat quad
	bool bHero = false;                        // the bearer: bigger, never dimmed, drawn over everyone

	/** Army View only (empty otherwise): the squad's live standing count, drawn over its block. */
	FString Label;
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
	 * One brush array per sprite sheet (T_Swarm_2bit, T_Hero_Vanguard, the knight, the
	 * archer — see UnitCamSprite in UnitCamProjector.cpp), each pre-sliced into one brush
	 * per cell. FUnitCamBillboard::SpriteSet picks the array, ::Cell the brush within it.
	 * Replaces the single shared-atlas SetCellBrushes now that not every body draws from
	 * the same texture.
	 */
	void SetBrushSets(TArray<TArray<FSlateBrush>>&& InSets);

	/** Live soldier-size multiple (framing dial); pushed each tick. */
	void SetSoldierScale(float InScale);

	/** Extra size multiple applied to the hero billboard on top of SoldierScale. */
	void SetHeroScale(float InScale);

	/** Live width-only stretch on every billboard, applied after the sheet's own cell aspect —
	 *  see CVarProjSoldierAspect's doc comment (UnitCamProjector.cpp) for the "more broad"
	 *  request this answers and why it's a taste dial layered on a packing fix, not instead
	 *  of one. */
	void SetSoldierAspect(float InAspect);

	/**
	 * Where a body sits relative to its projected point: 1 = standing on it, 0.5-worth of
	 * body centred on it at 0. The projected point is ground contact, so anything below 1
	 * draws units partly buried and makes every size dial dig downward.
	 */
	void SetFootAnchor(float InAnchor);

	/** The reticle marks the perspective virtual camera's aim point — meaningless in Army View's
	 *  fixed top-down block layout (there is no perspective camera in that mode), so it's hidden
	 *  there rather than left drawing over the blocks. */
	void SetShowReticle(bool bInShow);

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

	/**
	 * Army View (docs/design/squad-group-system.md §4.1) — the resting state (no squad
	 * selected): <=8 per-squad aggregate blocks instead of individual billboards. Does not touch
	 * Director or the perspective camera at all; a fixed, bearer-centred top-down read, same as
	 * an RTS group overview. FAKE, disclosed: block position is a placeholder ring around the
	 * bearer (USwarmSubsystem has no real per-squad centroid yet); standing count and the live
	 * label ARE real (GetSquadStanding); tint is the one global GetStance(), not a real
	 * per-squad stance (also not built yet) — every block tints identically for now.
	 */
	void BuildArmyView(const class USwarmSubsystem& Swarm);

	UPROPERTY(Transient) TObjectPtr<UUnitCamCanvasWidget> CanvasWidget = nullptr;

	// Resized every tick by total bodies on the field (individual <-> mass), and the frame is
	// tinted toward the reserved red as the tide outnumbers the host (threat read).
	UPROPERTY(Transient) TObjectPtr<USizeBox> RootBox = nullptr;
	UPROPERTY(Transient) TObjectPtr<UBorder> FrameBorder = nullptr;

	/** The 8x4 T_Swarm_2bit atlas (brood rows 0-1) that brood billboards, and retinue when
	 *  SoldierVariants is off, slice their frame out of. Loaded from Content once and held so
	 *  it isn't GC'd. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> SwarmAtlas = nullptr;

	/** T_Hero_Vanguard (Content/Sprites/Heroes) — the bearer's own dedicated sheet, replacing
	 *  the placeholder cell of SwarmAtlas his billboard used to draw (task-050). */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> HeroTexture = nullptr;

	/** T_Soldier_Knight (Content/Sprites/Units) — the owner-chosen high-resolution melee
	 *  retinue look (88x88 native, PixelLab character 1c935515-...). See provenance.json. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> KnightTexture = nullptr;

	/** T_Soldier_Archer — a temporary PixelLab proxy (RawArt/Renders/archer-proxy/), owner
	 *  flagged for a later swap, now high-resolution (92x92 native). See provenance.json. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> ArcherTexture = nullptr;

	bool bAtlasLoadAttempted = false;  // load once, even on failure, so we don't retry every frame
	bool bBrushSetsPushed = false;     // the slices never change; build them once, not per tick

	/** The camera manager seed — resolves which world point the virtual camera follows. */
	FUnitCamDirector Director;

	FVector2D PanelSizePx = FVector2D::ZeroVector;
	float FrameThickness = 4.f;
	bool bHostSized = false;
};
