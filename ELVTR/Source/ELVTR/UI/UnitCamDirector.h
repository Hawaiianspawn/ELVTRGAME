#pragma once

#include "CoreMinimal.h"

/**
 * What the virtual camera centres on. Mirrors Kindled.UnitCamProj.Focus.
 *
 * Cast focus is NOT a mode here: a cast is a transient override that punches focus off
 * whatever mode is running and hands it back when the spell ends, so it composes with
 * both rather than replacing them.
 */
enum class EUnitCamFocus : uint8
{
	Hero = 0,          // the bearer — the one holding the light, centre frame (default)
	FollowSoldier = 1, // ride the nearest retinue unit, tracked with continuity
};

/**
 * One frame's camera decision. The director answers "where is the camera aimed, from what
 * angle, and how close" — the projector turns that into a matrix and paints. Keeping the
 * decision in one struct is what lets focus modes multiply without the projector caring.
 */
struct FUnitCamShot
{
	FVector Focus = FVector::ZeroVector; // world point the camera looks at
	float YawDeg = 0.f;                  // azimuth of the camera around the focus
	float DistScale = 1.f;               // multiplier on the camera distance (cast punch-in)
};

/**
 * The Unit Cam camera manager (docs/RENDERING-LIGHTING.md §4d).
 *
 * Owns which world point the virtual camera follows and which way it faces: the focus
 * modes, the smoothed auto-look, and the spell-cast focus punch. Lives apart from
 * UUnitCamProjector on purpose — the projector is projection math and paint, this is
 * direction. New modes (click-to-select, cycle, scripted shots, transitions) go here.
 *
 * Holds smoothing state across frames, so one instance belongs to one panel.
 *
 * Dials: Kindled.UnitCamProj.{Focus,FollowSpeed,Yaw,AutoLook,LookLerp,CombatScan,
 * CastFocusSpeed,CastZoom,YawClampDeg,SelectedSquad,SelectSpeed} — defined in
 * UnitCamDirector.cpp alongside the logic they drive.
 */
struct FUnitCamDirector
{
	FVector FocusPos = FVector::ZeroVector;
	bool bInitialized = false;

	// Smoothed horizontal direction the camera should look toward (auto-look): outward from
	// the hero through the followed unit, optionally biased to the nearest enemy cluster.
	FVector2D LookDir = FVector2D(1.0, 0.0);
	bool bLookInit = false;

	// The bearer's own last-known movement heading (docs/design/squad-group-system.md §5). In
	// Hero focus, Outward (FocusPos - HeroPos) is ZERO by definition, so AutoLook's yaw clamp
	// needs a base heading that ISN'T derived from the same unstable enemy-cluster signal it's
	// meant to bound — this is that independent anchor. Updated every Tick regardless of focus
	// mode or cast state, so it's never stale when a cast ends or the mode changes.
	FVector LastHeroPos = FVector::ZeroVector;
	bool bHeroPosInit = false;
	FVector2D HeroMoveDir = FVector2D::ZeroVector; // last NONZERO movement heading; holds while stationary

	/** The auto-look azimuth in degrees, derived from the smoothed LookDir. */
	float LookYawDeg() const { return FMath::RadiansToDegrees(FMath::Atan2(LookDir.Y, LookDir.X)); }

	/** The focus mode currently selected by the CVar. */
	static EUnitCamFocus CurrentFocusMode();

	/**
	 * Which squad (0-7) the panel is framed on, or -1 for none (Army View's resting state).
	 * Exposed here — not as a raw CVar read in UnitCamProjector.cpp — because the CVar itself
	 * (Kindled.UnitCamProj.SelectedSquad) is registered in this translation unit; a second
	 * TAutoConsoleVariable of the same console name in another .cpp would collide with it.
	 * Also keeps with the file split this class already documents: the director owns the
	 * camera/selection decision, the projector only asks it what to draw.
	 */
	static int32 SelectedSquad();

	/**
	 * Resolve this frame's shot.
	 *
	 * Positions/AnimBits are the live sim render buffers; HeroPos is the bearer (also the
	 * flame). When bCastFocus is set, focus punches to CastPos and the camera pulls in.
	 */
	FUnitCamShot Tick(const TArray<FVector>& Positions, const TArray<int32>& AnimBits,
		const FVector& HeroPos, float DeltaSeconds, bool bCastFocus, const FVector& CastPos);

private:
	/** Ease LookDir toward Desired (already normalised) and mark it initialised. */
	void EaseLookToward(const FVector2D& Desired, float DeltaSeconds);
};
