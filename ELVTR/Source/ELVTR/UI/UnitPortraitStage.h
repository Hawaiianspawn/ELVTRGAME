#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnitPortraitStage.generated.h"

class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UTextureRenderTarget2D;

/**
 * Close-up "action cam" for the Unit Cam, built as a detail-swap.
 *
 * The capture films the REAL battlefield (the swarm, the fight — full scene), but a
 * higher-detail stand-in of the focused unit is composited in that is flagged
 * `bVisibleInSceneCaptureOnly` — so it is INVISIBLE in the stylised main game view and
 * appears ONLY inside this capture. The main render keeps the cheap swarm sprites and pays
 * nothing for the detailed unit; the detail exists only where the close-up is looking.
 *
 * Attach the actor to the unit/hero you want to follow. The stand-in is a placeholder mesh
 * for now — swap it for a detailed unit sprite/mesh (with combat anims) later without
 * touching the capture setup.
 */
UCLASS()
class ELVTR_API AUnitPortraitStage : public AActor
{
	GENERATED_BODY()

public:
	AUnitPortraitStage();

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** The capture-only stand-in primitive — hide this from other captures (e.g. the hero cam). */
	UPrimitiveComponent* GetSubjectComponent() const;

	UPROPERTY(EditAnywhere, Category = "UnitCam", meta = (ClampMin = "64"))
	int32 Resolution = 384;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "UnitCam") TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnitCam") TObjectPtr<UStaticMeshComponent> Subject = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "UnitCam") TObjectPtr<USceneCaptureComponent2D> Capture = nullptr;

	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;
};
