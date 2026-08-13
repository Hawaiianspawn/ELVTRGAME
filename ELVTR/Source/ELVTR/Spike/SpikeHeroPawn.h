#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Mass/SwarmCombat.h" // ESquadVerb
#include "SpikeHeroPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;

/**
 * The bearer. Owns four jobs beyond movement:
 *  - publishes its location to USwarmSubsystem as the attractor
 *  - issues stance orders (1 Follow / 2 Charge / 3 Hold / 4 Rally, R restart)
 *  - issues the squad-channelled ability kit, both addressings (task-144, SquadAbilities.h)
 *  - applies incoming brood damage and draws the prototype HUD
 *
 * INPUT IS ENHANCED INPUT AS OF task-144, which is the task-137 migration off raw key polling.
 * Every key below is a UInputAction inside one UInputMappingContext, bound with lambdas in
 * SetupPlayerInputComponent — so a hold is a real Started/Completed pair rather than an
 * up/down latch compared by hand, which is what the Q26 = D verb wheel needs to exist at all.
 *
 * THE ASSETS ARE BUILT IN C++, NOT IN ELVTR/Content/Input. NewObject'd into this pawn at
 * possession, so there is no .uasset to open in the editor and no editor round-trip to change
 * a binding. That is the cheap end of the migration and it is a deliberate stopping point, not
 * an oversight: content assets buy designer-editable rebinding, which nothing has asked for,
 * and cost an editor session per change. Moving to real assets later replaces BuildInputMap()
 * and touches nothing else, because every binding already goes through a UInputAction*.
 */
UCLASS()
class ASpikeHeroPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpikeHeroPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Spike")
	float MoveSpeed = 600.f;

	/**
	 * Seeds the camera's constructor placement only. At runtime TickCamera owns the transform
	 * from Kindled.Cam.Dist, so editing this in the details panel will not move the in-game
	 * shot — change the CVar (or its line in Saved/SwarmExecOnPlay.txt) instead.
	 */
	UPROPERTY(EditAnywhere, Category = "Spike")
	float CameraHeight = 1200.f;

private:
	/** Ground point under the cursor — Charge, every ability target and the wheel aim here. */
	bool GetCursorGroundLocation(FVector& OutLocation) const;

	/** Cursor if the deproject worked, else straight ahead of the bearer. Every ability path
	 *  needs a point and none of them may silently do nothing because the mouse was off-screen. */
	FVector CursorOrAhead() const;

	/** Build the UInputActions + the one UInputMappingContext. See the class comment for why
	 *  these are NewObject'd rather than assets under ELVTR/Content/Input. */
	void BuildInputMap();

	void TickHeroCombat(float DeltaSeconds);

	/** Q26 = D. Track the hovered sector while Q is held, and draw the wheel in the world. */
	void TickVerbWheel();

	/** Fire whatever the live Q23 shape says this input means. Shared by every scheme. */
	void CastArmed(int32 Caster, ESquadVerb Verb, uint8 Scheme);

	/**
	 * Drive the whole camera transform from the Kindled.Cam.* dials — projection, pitch/yaw,
	 * distance, focus offset — plus the up-axis bias that keeps the HUD from pushing the hero
	 * out of the visible strip. Owns the camera every frame, so the constructor's values only
	 * survive as the editor preview before BeginPlay.
	 */
	void TickCamera(float DeltaSeconds, const class USwarmSubsystem* Swarm);

	void DrawHUD() const;

	/**
	 * Every action, in one array indexed by the EHeroAction enum in the .cpp — a table rather
	 * than twenty near-identical UPROPERTY fields. Adding a key is one row in that table and
	 * one lambda, which is the whole reason the migration was worth doing here.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputMap;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> InputActions;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spike")
	TObjectPtr<UCameraComponent> Camera;

	/**
	 * The hero's own swing clock, in seconds, wrapping on Swarm.SwingInterval.
	 *
	 * The hero is a pawn, not a Mass entity, so it needs its own copy of the cadence
	 * every unit runs — otherwise the one attack the player actually feels would be
	 * the only one still bleeding damage continuously, with no blow to see land.
	 * Plain runtime state, not a UPROPERTY: the pawn is recreated each PIE.
	 */
	float HeroSwingTime = 0.f;

	/** Smoothed camera offset along its up-axis compensating for the HUD's occluded band. */
	float CameraHudBias = 0.f;

	/** Where the camera actually is, so Kindled.Cam.Lerp has something to ease from. */
	FVector CameraLoc = FVector::ZeroVector;
	FRotator CameraRot = FRotator::ZeroRotator;
	/** False until the first TickCamera places it — stops Lerp swooping in from the origin. */
	bool bCameraPlaced = false;

	/** Written by the Move action, spent by Tick. Plain runtime state; the pawn is recreated
	 *  each PIE, same as HeroSwingTime above. */
	FVector2D MoveInput = FVector2D::ZeroVector;

	// --- Q26 = D, the verb wheel (Q23 = A's coherent pairing) -----------------------------
	// Anchored where Q went down, NOT at the live cursor: a wheel that follows the mouse has no
	// origin to measure a direction FROM, which is the whole of how a radial works.
	bool bWheelOpen = false;
	FVector WheelAnchor = FVector::ZeroVector;
	ESquadVerb WheelHover = ESquadVerb::None;

	/** What the wheel last armed. Survives the wheel closing — pick the verb, then take your
	 *  time choosing where it goes, which is the two-stage input ability-kit.md §5 says to time. */
	ESquadVerb ArmedVerb = ESquadVerb::None;
};
