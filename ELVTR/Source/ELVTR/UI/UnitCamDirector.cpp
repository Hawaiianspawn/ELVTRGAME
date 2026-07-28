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
		TEXT("Emberkeep.UnitCamProj.LookLerp"), 1.5f,
		TEXT("How fast the auto-look rotation eases toward its target direction. Higher = snappier,\n")
		TEXT("lower = lazier pans. Keeps the camera from whipping around as the fight shifts.\n")
		TEXT("Was 3.0; halved (docs/design/squad-group-system.md §5) so a swing across the full\n")
		TEXT("clamp arc reads as a deliberate pan rather than a whip — roughly doubles settle time."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjYawClamp(
		TEXT("Emberkeep.UnitCamProj.YawClampDeg"), 30.f,
		TEXT("Hard envelope, in degrees either side of the base heading (bearer->focus, or the\n")
		TEXT("bearer's own last movement heading when that's degenerate — see BaseDir in Tick()),\n")
		TEXT("that AutoLook's enemy-cluster bias may swing the shot within. This is the actual fix\n")
		TEXT("for 'the camera swings left/right too much': AutoLook 2's pull toward the enemy\n")
		TEXT("centroid used to have damping (LookLerp) but no ceiling, so it could drift through a\n")
		TEXT("wide arc over several seconds. The target is clamped BEFORE it reaches LookLerp's\n")
		TEXT("ease, so the boundary reads as the pan running out of steam, not a hard stop."),
		ECVF_Default);

	// --- squad selection (docs/design/squad-group-system.md §4.3) -----------
	// Stand-in input surface for the muster-card click / hotkey 1-8 the spec describes as the
	// real driver: selecting a squad for orders and selecting it for the camera are meant to be
	// the SAME action. That wiring lives in whoever owns the Muster widget / HUD input, outside
	// this task's six files — until it lands, this CVar IS the selection surface. -1 (default)
	// is the resting state (Army View, per spec); >=0 is treated as "a squad is selected."
	TAutoConsoleVariable<int32> CVarProjSelectedSquad(
		TEXT("Emberkeep.UnitCamProj.SelectedSquad"), -1,
		TEXT("Which squad (0-7) the Unit Cam is framed on. -1 = none selected -> Army View (the\n")
		TEXT("resting state: <=8 aggregate squad blocks, not individual billboards). >=0 switches\n")
		TEXT("the SAME panel to Unit/Squad View, framed toward that squad. PLACEHOLDER input\n")
		TEXT("surface: the real driver is a muster-card click/hotkey (owned elsewhere); this CVar\n")
		TEXT("stands in for it until that's wired. Latches — does not decay back to Army View."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarProjSelectSpeed(
		TEXT("Emberkeep.UnitCamProj.SelectSpeed"), 10.f,
		TEXT("VInterpTo speed the camera travels at when a squad selection changes (docs/design/\n")
		TEXT("squad-group-system.md §4.3). Faster than ambient FollowSpeed (6, ~0.5s settle) so a\n")
		TEXT("selection reads as decisive, a notch under CastFocusSpeed (12, ~0.25s settle) so a\n")
		TEXT("spell-cast focus-punch still visibly wins the moment it lands. ~0.3s settle at 10."),
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

	/** Clamp Desired's angle to within +/-ClampDeg of Base. Both must already be normalised
	 *  and non-zero; the caller checks that (see the yaw-envelope block in Tick()). */
	FVector2D ClampDirToBase(const FVector2D& Desired, const FVector2D& Base, float ClampDeg)
	{
		const float BaseDeg = FMath::RadiansToDegrees(FMath::Atan2(Base.Y, Base.X));
		const float DesiredDeg = FMath::RadiansToDegrees(FMath::Atan2(Desired.Y, Desired.X));
		const float Delta = FMath::Clamp(
			FMath::FindDeltaAngleDegrees(BaseDeg, DesiredDeg), -ClampDeg, ClampDeg);
		const float ClampedRad = FMath::DegreesToRadians(BaseDeg + Delta);
		return FVector2D(FMath::Cos(ClampedRad), FMath::Sin(ClampedRad));
	}
}

EUnitCamFocus FUnitCamDirector::CurrentFocusMode()
{
	return CVarProjFocus.GetValueOnGameThread() == 0 ? EUnitCamFocus::Hero : EUnitCamFocus::FollowSoldier;
}

int32 FUnitCamDirector::SelectedSquad()
{
	return CVarProjSelectedSquad.GetValueOnGameThread();
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

	// Track the bearer's own movement heading every tick — cast active or not, whatever focus
	// mode is running — so it's never stale for the yaw-envelope's fallback base heading below.
	// A tiny per-frame delta is noise, not a heading change, so only commit a new direction once
	// the bearer has actually covered a couple of uu since last frame; otherwise hold the last one.
	if (bHeroPosInit)
	{
		const FVector2D HeroDelta(HeroPos.X - LastHeroPos.X, HeroPos.Y - LastHeroPos.Y);
		if (HeroDelta.SizeSquared() > 4.0)
		{
			HeroMoveDir = HeroDelta.GetSafeNormal();
		}
	}
	else
	{
		bHeroPosInit = true;
	}
	LastHeroPos = HeroPos;

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
	const int32 SelectedSquad = CVarProjSelectedSquad.GetValueOnGameThread();
	const bool bSquadSelected = SelectedSquad >= 0;

	// One pass over the buffers serves three jobs: pick the follow target (nearest retinue to
	// where we were looking last frame), accumulate the pull toward nearby enemies for the
	// auto-look, and — while a squad is selected — accumulate the SELECTED unit's own centroid.
	//
	// task-046 piped a real squad byte into the render buffer (SwarmRenderPack::Squad ->
	// SwarmSquad::UnitIndex), so this is the selected unit's REAL members now, not the whole
	// visible retinue's centroid — retiring the stand-in this comment used to disclose.
	const FVector Anchor = bInitialized ? FocusPos : HeroPos;
	const double ScanSq = FMath::Square((double)FMath::Max(CVarProjCombatScan.GetValueOnGameThread(), 1.f));
	const bool bWantFollow = Mode == EUnitCamFocus::FollowSoldier;
	const bool bWantEnemyDir = AutoLook == 2;
	int32 Best = INDEX_NONE;
	double BestSq = TNumericLimits<double>::Max();
	FVector2D EnemyDir(0.0, 0.0);
	FVector RetinueSum = FVector::ZeroVector;
	int32 RetinueCount = 0;

	for (int32 i = 0; i < Num; ++i)
	{
		if ((AnimBits[i] & SwarmAnim::TeamBit) != 0)
		{
			if (bWantFollow)
			{
				const double DSq = FVector::DistSquared(Positions[i], Anchor);
				if (DSq < BestSq) { BestSq = DSq; Best = i; }
			}
			if (bSquadSelected && SwarmSquad::UnitIndex(SwarmRenderPack::Squad(AnimBits[i])) == SelectedSquad)
			{
				RetinueSum += Positions[i];
				++RetinueCount;
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
	if (Mode == EUnitCamFocus::Hero || Best == INDEX_NONE)
	{
		// No selection: Hero mode holds the bearer dead centre, unchanged from today — no
		// smoothing, because the hero IS the frame's subject and lag would slide him off the
		// middle exactly when he moves. A selection retargets this to the SELECTED unit's own
		// real centroid (see the comment above the loop) and travels there deliberately instead.
		const FVector HeroTarget = (bSquadSelected && RetinueCount > 0)
			? (RetinueSum / (float)RetinueCount)
			: HeroPos;
		if (!bInitialized)
		{
			FocusPos = HeroTarget;
			bInitialized = true;
		}
		else if (bSquadSelected)
		{
			FocusPos = FMath::VInterpTo(FocusPos, HeroTarget, DeltaSeconds,
				FMath::Max(CVarProjSelectSpeed.GetValueOnGameThread(), 0.f));
		}
		else
		{
			FocusPos = HeroTarget;
		}
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
		// Outward from the hero through the focus — that points at the enemy front, since brood
		// pour in from outside. In Hero focus with nothing selected the two coincide (Outward is
		// zero), so BaseDir falls back to the bearer's own last movement heading instead — this
		// is the fix for the reported swinging (docs/design/squad-group-system.md §5): AutoLook's
		// enemy-cluster bias used to be the ONLY signal, with damping but no ceiling, so it could
		// drift through a wide arc. Now it can only nudge the shot within YawClampDeg of BaseDir.
		const FVector2D Outward(FocusPos.X - HeroPos.X, FocusPos.Y - HeroPos.Y);
		FVector2D BaseDir = FVector2D::ZeroVector;
		if (Outward.SizeSquared() > 1.0)
		{
			BaseDir = Outward.GetSafeNormal();
		}
		else if (!HeroMoveDir.IsNearlyZero())
		{
			BaseDir = HeroMoveDir;
		}

		FVector2D Desired = BaseDir;
		if (bWantEnemyDir && !EnemyDir.IsNearlyZero())
		{
			const FVector2D Toward = EnemyDir.GetSafeNormal();
			const FVector2D Blended = Desired.IsNearlyZero() ? Toward : (Desired + Toward);
			if (!Blended.IsNearlyZero())
			{
				Desired = Blended.GetSafeNormal();
			}
		}

		// The envelope itself: clamp the TARGET before it reaches LookLerp's ease (not LookDir's
		// output after), so hitting the edge reads as the pan running out of steam rather than a
		// wall. No BaseDir yet (very first frames, bearer hasn't moved and nothing's focused away
		// from him) means nothing to clamp around — leave Desired as whatever AutoLook produced.
		if (!BaseDir.IsNearlyZero() && !Desired.IsNearlyZero())
		{
			Desired = ClampDirToBase(Desired, BaseDir, CVarProjYawClamp.GetValueOnGameThread());
		}

		EaseLookToward(Desired, DeltaSeconds);
	}

	Shot.Focus = FocusPos;
	Shot.YawDeg = (AutoLook != 0 && bLookInit) ? LookYawDeg() : CVarProjYaw.GetValueOnGameThread();
	Shot.DistScale = 1.f;
	return Shot;
}
