#include "UI/UnitCamDirector.h"

#include "Mass/SwarmFragments.h"

#include "HAL/IConsoleManager.h"

// ---------------------------------------------------------------------------
// Camera-direction dials. These live here rather than in UnitCamProjector.cpp
// because they steer the decision this file makes; the projector's own CVars
// (Fov/Dist/Height/Pitch/Range/Scale/Size*) describe the lens and the panel.
// ---------------------------------------------------------------------------
namespace
{
	TAutoConsoleVariable<int32> CVarProjFocus(
		TEXT("Emberkeep.UnitCamProj.Focus"), 0,
		TEXT("What the virtual camera centres on. 0 = the HERO, the bearer holding the light,\n")
		TEXT("framed dead centre (default). 1 = follow a soldier (the nearest retinue unit,\n")
		TEXT("tracked with continuity so the camera moves WITH that unit)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjFollowSpeed(
		TEXT("Emberkeep.UnitCamProj.FollowSpeed"), 6.f,
		TEXT("How quickly the followed focus catches its target (VInterpTo speed). Higher =\n")
		TEXT("snappier/stiffer; lower = looser, more lag. 0 = frozen once acquired."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjYaw(
		TEXT("Emberkeep.UnitCamProj.Yaw"), 35.f,
		TEXT("Azimuth of the virtual camera around the focus, in degrees. Orbit it to\n")
		TEXT("watch the billboards swing — the cheapest proof the projection is real 3D.\n")
		TEXT("Used only while AutoLook is 0."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarProjAutoLook(
		TEXT("Emberkeep.UnitCamProj.AutoLook"), 2,
		TEXT("Camera rotation, driven by the director. 0 = manual (use Yaw). 1 = look OUTWARD\n")
		TEXT("(from the hero through the followed unit, i.e. toward the enemy front). 2 = look\n")
		TEXT("outward biased toward the nearest enemy cluster (where the fighting is). In Hero\n")
		TEXT("focus there is no unit to look through, so 1 holds the last heading and 2 turns\n")
		TEXT("toward the fight. Overrides Yaw while active."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjLookLerp(
		TEXT("Emberkeep.UnitCamProj.LookLerp"), 3.f,
		TEXT("How fast the auto-look rotation eases toward its target direction. Higher = snappier,\n")
		TEXT("lower = lazier pans. Keeps the camera from whipping around as the fight shifts."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjCombatScan(
		TEXT("Emberkeep.UnitCamProj.CombatScan"), 1200.f,
		TEXT("Radius (uu) around the focus that AutoLook 2 scans for enemies (brood) to aim\n")
		TEXT("toward. Larger = considers a wider swath of the battle when choosing where to look."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjCastFocusSpeed(
		TEXT("Emberkeep.UnitCamProj.CastFocusSpeed"), 12.f,
		TEXT("How fast the camera punches focus onto the spell when the bearer casts (VInterpTo).\n")
		TEXT("Higher than the normal FollowSpeed so a cast grabs the shot decisively."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjCastZoom(
		TEXT("Emberkeep.UnitCamProj.CastZoom"), 0.7f,
		TEXT("Camera distance multiplier while a spell cast is focused — <1 punches in for\n")
		TEXT("emphasis, 1 = no zoom. Applied to Dist only during the cast window."),
		ECVF_Default);
}

EUnitCamFocus FUnitCamDirector::CurrentFocusMode()
{
	return CVarProjFocus.GetValueOnGameThread() == 0 ? EUnitCamFocus::Hero : EUnitCamFocus::FollowSoldier;
}

void FUnitCamDirector::EaseLookToward(const FVector2D& Desired, float DeltaSeconds)
{
	if (Desired.IsNearlyZero())
	{
		return; // nothing to aim at — hold the last heading rather than snapping to a default
	}

	if (!bLookInit)
	{
		LookDir = Desired;
		bLookInit = true;
		return;
	}

	const double A = FMath::Clamp((double)CVarProjLookLerp.GetValueOnGameThread() * DeltaSeconds, 0.0, 1.0);
	const FVector2D Eased = LookDir + (Desired - LookDir) * A;
	if (!Eased.IsNearlyZero())
	{
		LookDir = Eased.GetSafeNormal();
	}
}

FUnitCamShot FUnitCamDirector::Tick(const TArray<FVector>& Positions, const TArray<int32>& AnimBits,
	const FVector& HeroPos, float DeltaSeconds, bool bCastFocus, const FVector& CastPos)
{
	FUnitCamShot Shot;
	const int32 AutoLook = CVarProjAutoLook.GetValueOnGameThread();

	// --- cast override: outranks whatever focus mode is running ------------
	if (bCastFocus)
	{
		const float PullSpeed = FMath::Max(CVarProjCastFocusSpeed.GetValueOnGameThread(), 0.f);
		if (!bInitialized) { FocusPos = CastPos; bInitialized = true; }
		else { FocusPos = FMath::VInterpTo(FocusPos, CastPos, DeltaSeconds, PullSpeed); }

		// Look from the hero toward the cast point (the spell). Self-cast (cast ~ hero) keeps
		// the last look direction rather than snapping to a degenerate one.
		FVector2D Desired(CastPos.X - HeroPos.X, CastPos.Y - HeroPos.Y);
		EaseLookToward(Desired.SizeSquared() > 1.0 ? Desired.GetSafeNormal() : FVector2D::ZeroVector, DeltaSeconds);

		Shot.Focus = FocusPos;
		Shot.YawDeg = bLookInit ? LookYawDeg() : CVarProjYaw.GetValueOnGameThread();
		Shot.DistScale = FMath::Max(CVarProjCastZoom.GetValueOnGameThread(), 0.05f);
		return Shot;
	}

	const int32 Num = FMath::Min(Positions.Num(), AnimBits.Num());
	const EUnitCamFocus Mode = CurrentFocusMode();

	// One pass over the buffers serves both jobs: pick the follow target (nearest retinue to
	// where we were looking last frame — a unit barely moves per frame, so "nearest to last
	// focus" stays the SAME unit and the camera rides along) and accumulate the pull toward
	// nearby enemies for the auto-look.
	const FVector Anchor = bInitialized ? FocusPos : HeroPos;
	const double ScanSq = FMath::Square((double)FMath::Max(CVarProjCombatScan.GetValueOnGameThread(), 1.f));
	const bool bWantFollow = Mode == EUnitCamFocus::FollowSoldier;
	const bool bWantEnemyDir = AutoLook == 2;
	int32 Best = INDEX_NONE;
	double BestSq = TNumericLimits<double>::Max();
	FVector2D EnemyDir(0.0, 0.0);

	for (int32 i = 0; i < Num; ++i)
	{
		if ((AnimBits[i] & SwarmAnim::TeamBit) != 0)
		{
			if (bWantFollow)
			{
				const double DSq = FVector::DistSquared(Positions[i], Anchor);
				if (DSq < BestSq) { BestSq = DSq; Best = i; }
			}
		}
		else if (bWantEnemyDir)
		{
			// Brood near the focus: accumulate the offset toward it, so the sum points at the
			// centre of mass of the local fight, weighted by how many enemies are there.
			const double DSq = FVector::DistSquaredXY(Positions[i], Anchor);
			if (DSq < ScanSq)
			{
				EnemyDir += FVector2D(Positions[i].X - Anchor.X, Positions[i].Y - Anchor.Y);
			}
		}
	}

	// --- resolve the focus point ------------------------------------------
	// Hero mode holds the bearer dead centre: no smoothing, because the hero IS the frame's
	// subject and lag would slide him off the middle exactly when he moves.
	if (Mode == EUnitCamFocus::Hero || Best == INDEX_NONE)
	{
		FocusPos = HeroPos;
		bInitialized = true;
	}
	else if (!bInitialized)
	{
		FocusPos = Positions[Best]; // snap on first acquire so we don't pan in from the origin
		bInitialized = true;
	}
	else
	{
		FocusPos = FMath::VInterpTo(FocusPos, Positions[Best], DeltaSeconds,
			FMath::Max(CVarProjFollowSpeed.GetValueOnGameThread(), 0.f));
	}

	// --- resolve the heading ----------------------------------------------
	if (AutoLook != 0)
	{
		FVector2D Desired(0.0, 0.0);

		// Outward from the hero through the focus — that points at the enemy front, since brood
		// pour in from outside. In Hero focus the two coincide, so there is no outward vector
		// and the enemy cluster (AutoLook 2) is the only heading available.
		const FVector2D Outward(FocusPos.X - HeroPos.X, FocusPos.Y - HeroPos.Y);
		if (Outward.SizeSquared() > 1.0)
		{
			Desired = Outward.GetSafeNormal();
		}

		if (bWantEnemyDir && !EnemyDir.IsNearlyZero())
		{
			const FVector2D Toward = EnemyDir.GetSafeNormal();
			const FVector2D Blended = Desired.IsNearlyZero() ? Toward : (Desired + Toward);
			if (!Blended.IsNearlyZero())
			{
				Desired = Blended.GetSafeNormal();
			}
		}

		EaseLookToward(Desired, DeltaSeconds);
	}

	Shot.Focus = FocusPos;
	Shot.YawDeg = (AutoLook != 0 && bLookInit) ? LookYawDeg() : CVarProjYaw.GetValueOnGameThread();
	Shot.DistScale = 1.f;
	return Shot;
}
