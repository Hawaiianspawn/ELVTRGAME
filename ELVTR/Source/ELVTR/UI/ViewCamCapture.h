#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ViewCamCapture.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Mirrors the active player camera into a render target, so the LIVE game view can be shown
 * inside a UMG panel (the top half of the HUD command rectangle).
 *
 * There is no cheaper way to do this: the main view is not a widget, and UMG cannot sample
 * the back buffer, so putting the game view in a panel means rendering the scene a second
 * time from the same POV. That is the cost this class exists to pay, and it is why the
 * capture is rate-limited (Emberkeep.UI.ViewCam.Rate) and rendered at panel resolution
 * rather than screen resolution.
 *
 * Contrast with UUnitCamProjector, which fakes its camera with pure math over the sim
 * buffers and costs one projection loop. This one is a real render.
 *
 * Capture source is SCS_FinalColorLDR, so the panel picks up the demichrome post-process
 * and matches the surrounding view instead of showing a raw un-styled scene.
 */
UCLASS()
class ELVTR_API AViewCamCapture : public AActor
{
	GENERATED_BODY()

public:
	AViewCamCapture();

	/** The one capture for this world, spawned on first use. Null if the feature is off. */
	static AViewCamCapture* FindOrSpawn(UWorld* World);

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/**
	 * Panel width/height the feed is drawn at. The render target is rebuilt to match this
	 * aspect so the mirrored view isn't stretched — the panel is a wide letterbox, the
	 * screen usually isn't.
	 */
	void SetPanelAspect(float InAspect);

	/** Panel caption for the current mode, so the tag reads MINIMAP vs VIEW CAM honestly. */
	static FText ModeLabel();

	/** True while showing the pulled-back minimap rather than mirroring the player camera. */
	static bool IsMinimapMode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** (Re)create the render target for the current resolution/aspect. */
	void RebuildRenderTarget();

	UPROPERTY(VisibleAnywhere, Category = "ViewCam")
	TObjectPtr<USceneCaptureComponent2D> Capture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;

	float PanelAspect = 2.0f;
	int32 BuiltWidth = 0;
	int32 BuiltHeight = 0;
	int32 FrameCounter = 0;
};
