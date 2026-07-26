#pragma once

#include "CoreMinimal.h"
#include "UI/EmberkeepWidget.h"
#include "EmberkeepCamFeed.generated.h"

class UImage;
class UBorder;
class USizeBox;
class UTextBlock;
class UTextureRenderTarget2D;

/**
 * A framed live subcamera feed (combat-HUD mockup): a square UImage showing a
 * SceneCapture render target, with a coloured frame and a corner label tag.
 * Hero = thin Pale frame; unit = thicker Steel "boxy" frame. The fuller cinematic
 * chrome (corner brackets, focus reticle, scanline) is a later pass.
 */
UCLASS()
class ELVTR_API UEmberkeepCamFeed : public UEmberkeepWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cam", meta = (ClampMin = "32"))
	float FeedSize = 190.f;

	void Setup(UTextureRenderTarget2D* InRenderTarget, const FText& InLabel, bool bInBoxy);

	/** Fill whatever slot the host gives us instead of forcing the square FeedSize — used when
	 *  the feed is one half of the HUD rectangle's centre column. */
	void SetHostSized(bool bInHostSized);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void Refresh();

	UPROPERTY(Transient) TObjectPtr<USizeBox> SizeRoot = nullptr;
	UPROPERTY(Transient) TObjectPtr<UImage> FeedImage = nullptr;
	UPROPERTY(Transient) TObjectPtr<UBorder> Frame = nullptr;
	UPROPERTY(Transient) TObjectPtr<UBorder> Tag = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LabelText = nullptr;

	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;
	UPROPERTY() FText CamLabel;
	UPROPERTY() bool bBoxy = false;
	bool bHostSized = false;
};
