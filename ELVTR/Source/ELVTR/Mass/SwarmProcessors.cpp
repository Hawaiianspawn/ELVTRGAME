#include "SwarmProcessors.h"

#include "HAL/IConsoleManager.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "Algo/BinarySearch.h"
#include "SwarmCombat.h"
#include "SwarmFormation.h"
#include "SwarmFragments.h"
#include "SwarmStats.h"
#include "SwarmSubsystem.h"

namespace SwarmTuning
{
	// Retinue-side steering. Still compile-time: these describe YOUR line, and the
	// stance system is the thing that's meant to move it. The brood equivalents below
	// are CVars because the horde is what gets re-shaped between test runs.
	constexpr float RetinueSpeed = 450.f;
	constexpr float ChargeSpeedMul = 1.25f;
	constexpr float SeparationRadius = 60.f;
	constexpr float SeparationWeight = 1.4f;
	constexpr int32 SeparationNeighborCap = 6;

	/** How far a unit will step off its anchor to hit something, per stance. */
	constexpr float FollowEngageRange = 250.f;
	constexpr float ChargeEngageRange = 400.f;
	constexpr float LineEngageRange = 160.f;	// Hold / Rally — keep the line

	constexpr float SlotArriveRadius = 40.f;
	constexpr float RallySlotScale = 0.45f;		// collapse tight onto the hero
	constexpr float WalkAnimHz = 6.f;
}

namespace
{
	// --- horde dials (live via console) ----------------------------------
	// The brood's movement was constexpr until 2026-07-26, which meant the only thing
	// tunable about the tide was how hard it hit. Everything that decides how it LOOKS
	// arriving — pace, packing, what it goes for, where it comes from — is here now.
	// Read once per processor pass, never per entity.

	TAutoConsoleVariable<float> CVarBroodSpeed(
		TEXT("Swarm.BroodSpeed"), 320.f,
		TEXT("March speed of a brood, uu/s. Against RetinueSpeed 450 this is what decides\n")
		TEXT("whether the tide can be outrun or has to be fought. [0..1200]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSeparation(
		TEXT("Swarm.BroodSeparation"), 60.f,
		TEXT("Personal space of a brood, uu — the radius it pushes neighbours out of.\n")
		TEXT("The density dial: low packs the tide into a solid mass that arrives all at\n")
		TEXT("once, high spreads it into a loose skirmish line. [0..300]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSeparationWeight(
		TEXT("Swarm.BroodSeparationWeight"), 1.4f,
		TEXT("How hard the separation push competes with the urge to close. Above ~2 the\n")
		TEXT("horde mills instead of charging; at 0 it stacks into one column. [0..4]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarBroodSeparationCap(
		TEXT("Swarm.BroodSeparationCap"), 6,
		TEXT("Neighbours one brood will push against per frame. Also the cost dial for the\n")
		TEXT("steering pass — this bounds its inner loop. [1..16]"), ECVF_Default);

	// --- simulation LOD (docs/perf/one-camera-bench.md, 2026-07-28) --------------------
	// Measured finding this exists to act on: with the Niagara sprite path, RENDERING IS FREE
	// (within noise of a sim-only baseline at every count from 500 to 20,000). 100% of the frame
	// cost is this sim on the game thread, ~0.75ms per 1,000 entities. So the only lever that
	// moves the entity ceiling is doing less sim work — not drawing fewer things.
	//
	// What's expensive per brood per frame is TWO spatial grid queries (FindNearestEnemy and
	// SeparationForce). What's cheap is integration, which stays at full rate for everyone.
	// A brood far from the fight is marching in a near-straight line at constant velocity, so
	// re-deriving that velocity 60 times a second buys nothing a player can see: it keeps
	// gliding on its last velocity between steers and integration still moves it every frame.
	TAutoConsoleVariable<int32> CVarSimLodStride(
		// Defaults to 4 (measured 2026-07-28): −32% frame time at 20,000 entities, raising the
		// 60fps ceiling from ~21,000 to ~34,000, for a change nothing on screen can see — at
		// NearRadius 1600 every unit it touches is off-camera. Set 1 to disable.
		TEXT("Swarm.SimLOD.Stride"), 4,
		TEXT("Re-steer a FAR brood only once every N frames (near brood always steer every\n")
		TEXT("frame). 1 = off, every unit steers every frame. 4 means a far brood does its two\n")
		TEXT("grid queries on one frame in four, cutting the dominant sim pass roughly to\n")
		TEXT("(near + far/N) of its cost.\n")
		TEXT("\n")
		TEXT("Skipped frames do NOT freeze the unit — velocity persists and integration still\n")
		TEXT("runs at full rate, so movement stays smooth. What lags is the RESPONSE to a new\n")
		TEXT("neighbour, by up to N frames (67ms at N=4, 60fps). That is invisible out in the\n")
		TEXT("march and would be very visible in a melee, which is exactly what NearRadius\n")
		TEXT("protects. [1..8]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarSimLodNearRadius(
		// 2200 since 2026-07-28, was 1600. Kindled.Cam.ScalePitchFull went -90 -> -55 the same
		// day; a tilted camera sees ~1650uu of ground rather than ~1350uu, which put the furthest
		// visible corner within a hair of the old radius. Raised with margin rather than tuned to
		// the edge, because the failure mode (units striding on screen) is worse than the cost.
		TEXT("Swarm.SimLOD.NearRadius"), 2200.f,
		TEXT("Distance from the bearer, uu, inside which a brood is 'near' and always steers at\n")
		TEXT("full rate regardless of Stride. A CORRECTNESS boundary, not a taste dial:\n")
		TEXT("everything the player can watch resolve — the front line, the pile against the\n")
		TEXT("retinue, avoidance in the melee — has to sit inside it. The top-down view is at\n")
		TEXT("most 2400uu wide, putting its furthest corner ~1470uu out, so at full army this\n")
		TEXT("only strides brood that are off-screen walking in from the spawn ring. Exception:\n")
		TEXT("below Kindled.Cam.ScaleSwapAt the camera goes shallow-perspective and sees\n")
		TEXT("further down-field, so a late-run shot CAN have strided units in frame — raise\n")
		TEXT("this if that shot ever becomes a moment the game cares about. [0..8000]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodAggroRange(
		TEXT("Swarm.BroodAggroRange"), 600.f,
		TEXT("How far a brood will divert from the bearer to bite a soldier, uu. THE\n")
		TEXT("horde-behaviour dial: high = the tide is stopped by your line and a front\n")
		TEXT("forms; low = it flows past the line and dives for the flame, and holding\n")
		TEXT("ground stops being enough. Capped in practice by the 3x3 grid reach\n")
		TEXT("(750uu at GridCellSize 250, task-052; was ~600uu at GridCellSize 200), so\n")
		TEXT("values beyond that read the same. [0..750]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodContactRange(
		TEXT("Swarm.BroodContactRange"), 120.f,
		TEXT("Radius, uu, inside which a brood counts as mobbing the bearer. Telemetry and\n")
		TEXT("HUD only — damage is Swarm.HeroMeleeRange. [0..600]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodWalkHz(
		TEXT("Swarm.BroodWalkHz"), 6.f,
		TEXT("Brood walk-cycle rate, full frames/sec. Independent of the retinue's so the\n")
		TEXT("two teams read as different creatures at a glance. [0..20]"), ECVF_Default);

	// --- brood variety (2026-07-29) --------------------------------------
	// Owner: "can we get npc variety into our system... the Oozes have some variance in
	// appearence. They will be match to display weight."

	TAutoConsoleVariable<FString> CVarBroodVariantWeights(
		TEXT("Swarm.BroodVariantWeights"), TEXT("14,40,12,8,10,4,2,6,4"),
		TEXT("How often each of the nine ENEMY-atlas looks appears, comma-separated integer\n")
		TEXT("weights in atlas order: base,sump,bell,stalk,wedge,ridge,crown,twin,slug. Only the\n")
		TEXT("RATIOS matter — the pick maps each unit's spawn phase through the cumulative sum.\n")
		TEXT("A weight of 0 RETIRES a look with no repack and no rebuild, which is why all nine\n")
		TEXT("are in the sheet. Takes effect on the frame you set it, on units already standing,\n")
		TEXT("because the variant is derived from the phase every pass and never stored.\n")
		TEXT("Fewer than nine entries leaves the rest at 0. Defaults documented in\n")
		TEXT("docs/data/art/brood-variants.json — keep the two in step (there is a check:\n")
		TEXT("Scripts/art/check_brood_variants.py). Try 0,0,0,0,0,0,100,0,0 to see it work."),
		ECVF_Default);

	// task-085: the team-side twin of the CVar above, same mechanism, other atlas. Owner,
	// 2026-07-29: "can we implement the knight family into the group... use that for
	// materials as the enemies will change from time to time."
	TAutoConsoleVariable<FString> CVarTeamVariantWeights(
		// 2026-07-30, owner: "i want at most 5% of the army to be super small like this. They
		// are outliers and not the average or part of a bell curve."
		//
		// So this is a BELL CURVE OVER BODY SIZE, not the flat mix it shipped as. Sizes
		// re-measured across all EIGHT rotations — the family manifests record ONE frame each,
		// which ranks these differently and misleadingly. Median mass across the eleven is 953:
		//
		//    822 v1_narrowguard  26w |  839 v7_barestance 28w |  891 v6_simplecolumn 29w  <- tail
		//    904 v10_bracedstaff 46w |  911 v4_overhead   40w |  953 v11_midguard    34w  <- floor
		//    998 v2_lanceout     47w | 1016 v13_maceraised 36w| 1040 v3_shieldbreak  40w
		//   1108 retinue         37w | 1242 v8_heavycloak 46w                            <- tail
		//
		// v11_midguard is the REALISTIC SMALLEST normal soldier: narrow, but mass dead on the
		// median. Below it the break is clean — the three tail looks are the only ones under
		// 30px wide AND under 0.95x mass, against midguard's 34px, with nothing in between.
		// They get 2 each = 4.9% combined. v8_heavycloak gets the same at the large end, so
		// both tails are rare rather than only the one that got complained about.
		//
		// NOT judged on mass alone: v10_bracedstaff is light (0.95x) but 46px wide, so it reads
		// as a big footprint, not a small unit. Mass alone would have wrongly demoted it.
		//
		// Since task-095 this table also drives STATS. Checked: the reweight moves average
		// retinue HP 132.7 -> 132.6 and DPS 30.71 -> 31.13, so the look fix is very nearly
		// combat-neutral. Re-check that if these weights move again.
		TEXT("Swarm.TeamVariantWeights"), TEXT("20,2,18,14,14,2,2,6,10,18,16"),
		TEXT("How often each of the eleven TEAM-atlas looks appears, comma-separated integer\n")
		TEXT("weights in atlas order: retinue base, then the ten judged knight keeps (v1, v2,\n")
		TEXT("v3, v4, v6, v7, v8, v10, v11, v13 — see docs/data/art/team-variants.json for which\n")
		TEXT("index is which silhouette). Only the RATIOS matter, same cumulative-sum pick as\n")
		TEXT("Swarm.BroodVariantWeights. A weight of 0 retires a look with no repack. Defaults\n")
		TEXT("documented in docs/data/art/team-variants.json — keep the two in step (checked by\n")
		TEXT("Scripts/art/check_brood_variants.py). Try 0,100,0,0,0,0,0,0,0,0,0 to see one knight\n")
		TEXT("dominate the army."),
		ECVF_Default);

	// task-126: the third table, over the ARCHER block of the same team atlas. Archers were
	// fully simulated and completely invisible -- they wore whichever of the eleven spearman
	// looks their phase happened to land on, so the player could not see their own ranged
	// line. Flat weights to start: unlike the team table this one has no owner verdict on
	// the mix yet, and six looks that all read as "archer" is the point.
	TAutoConsoleVariable<FString> CVarArcherVariantWeights(
		TEXT("Swarm.ArcherVariantWeights"), TEXT("16,16,16,16,16,16"),
		TEXT("How often each of the six ARCHER looks appears, comma-separated integer weights\n")
		TEXT("in archer-block order: v1_narrowstrung, v2_bowextended, v3_loosingarm,\n")
		TEXT("v4_quiverreach, v5_crossbowbrace, v6_slingwhirl. Same cumulative-sum pick as\n")
		TEXT("Swarm.TeamVariantWeights, over the archer half of the SAME team atlas (rows\n")
		TEXT("22-33; the +11 row offset is applied in the render bridge, not stored). A weight\n")
		TEXT("of 0 retires a look with no repack. Does NOT touch combat: archers read the\n")
		TEXT("Swarm.Archers* stats and never the knight sub-type table, so skewing this is a\n")
		TEXT("pure look change. Defaults documented in docs/data/art/team-variants.json.\n")
		TEXT("Try 0,0,0,0,0,100 to put the whole ranged line on slings."),
		ECVF_Default);

	/** Cumulative display weights, so a per-entity roll becomes a variant with one scan. */
	struct FVariantTable
	{
		int32 Cum[SwarmSheet::Team::Variants] = {};	// sized for the largest of the three tables
		int32 Num = 0;
	};

	/**
	 * Parse a comma-separated weights CVar into cumulative form, capped at MaxVariants
	 * entries (SwarmSheet::Enemy::Variants for the brood table, ::Team::SpearVariants for
	 * the spearman table, ::Team::ArcherVariants for the archer table — one parser, three
	 * caps and three CVars).
	 *
	 * ponytail: reparsed once per pass rather than cached against the last string — a
	 * handful of Atoi calls next to a 30k-entity sim, and it buys a live CVar with no
	 * change-detection machinery and no way for a cache to go stale. Cache on a string
	 * compare if it ever shows up in a profile.
	 */
	FVariantTable ParseVariantTable(const FString& Csv, int32 MaxVariants)
	{
		FVariantTable Table;
		TArray<FString> Parts;
		Csv.ParseIntoArray(Parts, TEXT(","), false);
		int32 Running = 0;
		for (const FString& Part : Parts)
		{
			if (Table.Num >= MaxVariants)
			{
				break;
			}
			Running += FMath::Max(FCString::Atoi(*Part.TrimStartAndEnd()), 0);
			Table.Cum[Table.Num++] = Running;
		}
		return Table;
	}

	// --- facing (2026-07-26) ---------------------------------------------
	// "They face outwards from you but sometimes look back at you." The resting posture
	// of your army is a ring staring into the dark; the glance is what keeps the flame
	// premise legible — the light is the thing they are all here for, so their eyes keep
	// returning to it.

	TAutoConsoleVariable<float> CVarFacingMoveSpeed(
		TEXT("Swarm.FacingMoveSpeed"), 40.f,
		TEXT("Speed, uu/s, above which a unit faces where it is GOING rather than outward.\n")
		TEXT("Below it the unit is 'at rest' and turns its back to the bearer to watch the\n")
		TEXT("dark. Set very high to force the outward ring on at all times; set to 0 to\n")
		TEXT("get pure velocity facing and no ring at all. [0..600]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarFacingTurnHysteresis(
		TEXT("Swarm.FacingTurnHysteresis"), 1.4f,
		TEXT("How far, in world facing steps (32 per turn), the desired angle must move\n")
		TEXT("before a unit commits to it. This is the anti-strobe dial: at 0 a unit\n")
		TEXT("steering along a column boundary flickers between two sprites every tick.\n")
		TEXT("Above ~4 turning visibly lags the movement. [0..8]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarGlancePeriod(
		TEXT("Swarm.GlancePeriod"), 6.5f,
		TEXT("Average seconds between one unit glancing back at the bearer. Per-unit and\n")
		TEXT("de-synced by the spawn jitter phase, so the army never turns in unison.\n")
		TEXT("[1..30]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarGlanceDuration(
		TEXT("Swarm.GlanceDuration"), 1.1f,
		TEXT("Seconds a glance back is held. Long enough to read at gameplay zoom, short\n")
		TEXT("enough that the ring still reads as facing outward. [0..5]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarGlanceEnabled(
		TEXT("Swarm.Glance"), 1,
		TEXT("Master switch for the look-back-at-the-flame tic. 0 freezes every resting\n")
		TEXT("unit facing outward."), ECVF_Default);

	/** Everything the facing pass reads, snapshotted once per processor pass. */
	struct FFacingParams
	{
		float MoveSpeedSq = 0.f;
		float Hysteresis = 0.f;
		float GlancePeriod = 0.f;
		float GlanceDuration = 0.f;
		bool bGlanceEnabled = false;
	};

	/**
	 * A unit's world facing step for this frame.
	 *
	 * The rule, in priority order:
	 *   1. MOVING  — face where you are going. A soldier crossing the field looking
	 *      sideways reads as broken no matter how good the fiction is.
	 *   2. GLANCING — a resting soldier periodically turns and looks at the bearer.
	 *   3. AT REST — face radially AWAY from the bearer. The army standing in the only
	 *      light in the world forms a ring watching the dark it came out of.
	 *
	 * Outward-and-glance are retinue-only. The brood is not gathered around the flame,
	 * it is coming for it, so its facing is simply its heading and it holds the last one
	 * when it stops. (Its sheet row ignores the column today anyway — every brood
	 * direction packs the same frame — but the value is computed honestly so the day it
	 * gains rotations nothing here changes.)
	 */
	FORCEINLINE uint8 ResolveFacing(uint8 Current, const FVector& Location, const FVector& Velocity,
		const FVector& HeroLocation, bool bRetinue, float Phase, float TimeSeconds,
		const FFacingParams& P)
	{
		const FVector2f Vel2((float)Velocity.X, (float)Velocity.Y);
		FVector2f Dir = Vel2;

		if (Vel2.SizeSquared() <= P.MoveSpeedSq && bRetinue)
		{
			const FVector2f Outward((float)(Location.X - HeroLocation.X),
									(float)(Location.Y - HeroLocation.Y));

			// Standing ON the bearer has no outward direction; keep the last facing
			// rather than snapping to an arbitrary one.
			if (Outward.IsNearlyZero())
			{
				return Current;
			}

			// De-synced from the spawn jitter phase, and scaled by a different irrational
			// than the size roll (SwarmRenderPack uses 0.618) so a unit's glance clock
			// does not correlate with its size or its walk cycle. Without this the whole
			// line turns its head on the same frame, which reads as a cutscene.
			bool bGlancing = false;
			if (P.bGlanceEnabled && P.GlanceDuration > 0.f)
			{
				const float Cycle = FMath::Max(P.GlancePeriod, 0.1f);
				const float Offset = FMath::Frac(Phase * 0.3819660113f) * Cycle;
				bGlancing = FMath::Fmod(TimeSeconds + Offset, Cycle) < P.GlanceDuration;
			}

			Dir = bGlancing ? -Outward : Outward;
		}

		if (Dir.IsNearlyZero())
		{
			return Current;
		}

		const int32 Desired = SwarmFacing::IndexFromDir(Dir);

		// Shortest signed step delta, wrapped. Steps is a power of two, so the mask does
		// the wrap for free and for negatives too.
		constexpr int32 Half = SwarmFacing::Steps / 2;
		const int32 Delta = ((Desired - (int32)Current + Half) & SwarmFacing::StepMask) - Half;

		return (FMath::Abs(Delta) > P.Hysteresis) ? (uint8)Desired : Current;
	}

	FORCEINLINE FVector SeparationForce(const USwarmSubsystem& Swarm, const FVector& Location,
		float Radius, int32 NeighborCap)
	{
		const float RadiusSq = Radius * Radius;
		FVector Push = FVector::ZeroVector;
		int32 Considered = 0;
		Swarm.QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (Considered >= NeighborCap)
			{
				return;
			}
			const FVector Delta = Location - Entry.Location;
			const float DistSq = Delta.SizeSquared2D();
			if (DistSq > UE_KINDA_SMALL_NUMBER && DistSq < RadiusSq)
			{
				Push += Delta.GetSafeNormal2D() * (1.f - FMath::Sqrt(DistSq) / Radius);
				++Considered;
			}
		});
		return Push;
	}

	/** Nearest entry of the opposing team inside the 3x3 grid neighbourhood. */
	FORCEINLINE bool FindNearestEnemy(const USwarmSubsystem& Swarm, const FVector& Location, bool bWantRetinue, FVector& OutLocation, float& OutDistSq)
	{
		bool bFound = false;
		float BestSq = TNumericLimits<float>::Max();
		FVector Best = FVector::ZeroVector;

		Swarm.QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (Entry.bRetinue != bWantRetinue)
			{
				return;
			}
			const float DistSq = FVector::DistSquared2D(Entry.Location, Location);
			if (DistSq < BestSq)
			{
				BestSq = DistSq;
				Best = Entry.Location;
				bFound = true;
			}
		});

		OutLocation = Best;
		OutDistSq = BestSq;
		return bFound;
	}

	/**
	 * Nearest entry of the opposing team inside the 3x3 grid neighbourhood, preferring one
	 * within [MinRangeSq, MaxRangeSq) but falling back to the nearest inside [0, MinRangeSq)
	 * if — and only if — no in-band candidate exists at all.
	 *
	 * task-073: Swarm.ArchersMinEngageRange (SwarmCombatProcessors.cpp) makes an Archer
	 * PREFER not to engage anything closer than its own min range — the cheap approximation
	 * of "don't shoot into your own scrum" (squad-group-system.md §2.2). But when nothing
	 * else is standing between the archer and a brood that close, there is no scrum to
	 * shoot into, and nothing else can kill it either: Swarm.MeleeRange (95uu) is short of
	 * MinEngageRange's default (150uu), so a brood stuck there was permanently unkillable —
	 * measured task-064, 7 brood held at an exact unchanged count for 135+s. Reaching into
	 * the dead zone only when the normal band is completely EMPTY means a real scrum, which
	 * still has in-band candidates, never loses the archer's preference for standing off;
	 * this only fires when the archer would otherwise stand idle next to something nobody
	 * else can reach.
	 *
	 * MinRangeSq == 0 (Spearmen, via URetinueFollowProcessor below) collapses this to
	 * FindNearestEnemy's plain nearest-in-range search — the dead-zone branch below can
	 * never contribute a closer candidate than the in-band one, so behaviour is unchanged.
	 */
	FORCEINLINE bool FindNearestEnemyBanded(const USwarmSubsystem& Swarm, const FVector& Location, bool bWantRetinue,
		float MinRangeSq, float MaxRangeSq, FVector& OutLocation, float& OutDistSq)
	{
		bool bFoundInBand = false;
		float BestInBandSq = TNumericLimits<float>::Max();
		FVector BestInBand = FVector::ZeroVector;

		bool bFoundDeadZone = false;
		float BestDeadZoneSq = TNumericLimits<float>::Max();
		FVector BestDeadZone = FVector::ZeroVector;

		Swarm.QueryNeighbors(Location, [&](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (Entry.bRetinue != bWantRetinue)
			{
				return;
			}
			const float DistSq = FVector::DistSquared2D(Entry.Location, Location);
			if (DistSq >= MaxRangeSq)
			{
				return;
			}
			if (DistSq >= MinRangeSq)
			{
				if (DistSq < BestInBandSq)
				{
					BestInBandSq = DistSq;
					BestInBand = Entry.Location;
					bFoundInBand = true;
				}
			}
			else if (MinRangeSq > 0.f && DistSq < BestDeadZoneSq)
			{
				BestDeadZoneSq = DistSq;
				BestDeadZone = Entry.Location;
				bFoundDeadZone = true;
			}
		});

		if (bFoundInBand)
		{
			OutLocation = BestInBand;
			OutDistSq = BestInBandSq;
			return true;
		}
		if (bFoundDeadZone)
		{
			OutLocation = BestDeadZone;
			OutDistSq = BestDeadZoneSq;
			return true;
		}
		return false;
	}

	FORCEINLINE void UpdateAnimBits(uint8& Bits, const FVector& Velocity, float TimeSeconds, float Phase, bool bAttacking, float WalkHz)
	{
		const bool bMoving = Velocity.SizeSquared2D() > 100.f;
		const bool bFrame = bMoving && (FMath::Fmod((TimeSeconds + Phase) * WalkHz, 2.f) >= 1.f);

		// Preserve the bits other passes own (team, leash warning, swing, hit flash);
		// rebuild the rest. Anything left off SwarmAnim::PreservedBits is wiped here
		// every single tick, which is a silent way to lose a feature.
		Bits &= SwarmAnim::PreservedBits;
		Bits |= bFrame ? SwarmAnim::FrameBit : 0;
		Bits |= bAttacking ? SwarmAnim::AttackBit : 0;
		Bits |= (Velocity.X < 0.f) ? SwarmAnim::FlipBit : 0;
	}
}

//----------------------------------------------------------------------
// Grid build
//----------------------------------------------------------------------
USwarmGridBuildProcessor::USwarmGridBuildProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmGrid"));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmGridBuildProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmStrikeFragment>(EMassFragmentAccess::ReadOnly);
	// Read-only, already exists on every swarm entity (SwarmSpawn.cpp) — no class-layout
	// change. A Spearman's own walk-cycle phase resolves which knight sub-type row it
	// strikes for (task-095), the same phase VariantFromPhase below already uses to pick
	// its look, so the two can never disagree.
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmGridBuildProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmGridBuild, SwarmGridBuild);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	const int32 ExpectedCount = Swarm->GetTrackedEntities().Num();
	Swarm->ResetGrid(ExpectedCount);
	Swarm->ResetRenderBuffers(ExpectedCount);

	// Each entity's own K (how many blows it may hand out this swing) rides into the
	// grid alongside its reach, so the combat pass can cap claims against it instead
	// of trusting the radius alone — see FGridEntry::BlowsClaimed.
	const int32 ArchersTargets = SwarmCombatTuning::ArchersTargetsPerHit();
	const int32 BroodTargets = SwarmCombatTuning::BroodTargetsPerHit();

	// Per-blow damage also rides along now (FGridEntry::BlowDamage) — retinue strikers
	// aren't all one type any more, so "one shared blow value per TEAM" (the old
	// assumption) can't stand; a Spearman and an Archer striking the same victim this
	// frame must not deal the same damage. Snapshotted once per pass, same as the K's.
	const float SwingInterval = SwarmCombatTuning::SwingInterval();
	const float ArchersBlow = SwarmCombatTuning::ArchersDPS() * SwingInterval;
	const float BroodBlow = SwarmCombatTuning::BroodDPS() * SwingInterval;

	// A Spearman's own K and blow value now come from the knight sub-type row its team-
	// atlas variant (SwarmSheet::Team) maps to (task-095), instead of one flat
	// Swarm.RetinueTargetsPerHit / Swarm.RetinueDPS for the whole line — see
	// SwarmCombatTuning::FKnightSubtypeTables (SwarmCombat.h).
	const SwarmCombatTuning::FKnightSubtypeTables KnightTables = SwarmCombatTuning::GetKnightSubtypeTables();

	EntityQuery.ForEachEntityChunk(Context, [Swarm, ArchersTargets, BroodTargets, ArchersBlow, BroodBlow, SwingInterval, KnightTables](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetFragmentView<FSwarmAnimFragment>();
		const TConstArrayView<FSwarmStrikeFragment> Strike = ChunkContext.GetFragmentView<FSwarmStrikeFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			// Strike state rides along with position so the combat pass can tell
			// which neighbours are actually connecting this frame. Published here,
			// at the top of the frame, because the swing clocks were advanced at the
			// end of the last one — so what combat reads is current, not stale.
			const bool bRetinue = (Anim[i].Bits & SwarmAnim::TeamBit) != 0;
			const bool bArcher = bRetinue && SwarmSquad::UnitType(Anim[i].SquadId) == EUnitType::Archers;

			int32 MyTargets;
			float MyBlow;
			if (!bRetinue)
			{
				MyTargets = BroodTargets;
				MyBlow = BroodBlow;
			}
			else if (bArcher)
			{
				MyTargets = ArchersTargets;
				MyBlow = ArchersBlow;
			}
			else
			{
				const int32 Variant = SwarmRenderPack::VariantFromPhase(
					Jitter[i].Phase, KnightTables.TeamVariantCum, KnightTables.NumTeamVariants);
				const int32 Row = SwarmCombatTuning::KnightSubtypeRowFor(KnightTables, Variant);
				MyTargets = KnightTables.Targets[Row];
				MyBlow = KnightTables.DPS[Row] * SwingInterval;
			}

			Swarm->AddToGrid(
				Transforms[i].GetTransform().GetLocation(),
				bRetinue,
				Strike[i].bStrikeFrame,
				Strike[i].StrikeReachSq,
				MyTargets,
				MyBlow,
				// Attacker identity for squad kill credit (docs/ui/end-of-wave-showcase.md
				// §5.3) — the combat pass has no other way to know who landed a blow.
				Anim[i].SquadId);
		}
	});

	// Occupancy is the grid's own health metric: cells vs. entities tells you
	// whether GridCellSize is bucketing sensibly or degenerating toward a list.
	SET_DWORD_STAT(STAT_SwarmGridCells, Swarm->GetGridCellCount());
}

//----------------------------------------------------------------------
// Brood steering: seek attractor + separation
//----------------------------------------------------------------------
// RANKS UNDER CONTACT (task-047): brood ranks (SwarmCommands.cpp, Swarm.BroodFormation.*)
// are a SPAWN-TIME arrangement only — there is no brood equivalent of
// URetinueFollowProcessor holding a slot fragment, and deliberately so. The moment a rank
// starts marching, THIS pass (seek the attractor or whichever soldier is in AggroRange,
// plus separation) is already what moves it, individually, every frame. So the degrade
// from "neat rows" to "per-unit melee" isn't a state transition anything decides — it's
// just what these two forces do to a grid of exact starting positions once
// Swarm.BroodSpeedJitter and per-target divergence stop agreeing unit-to-unit. A rank
// holding rigid through a melee (matching the retinue's compaction) would need every
// brood to share a target and a pace, which is precisely the "one pulsing organism"
// look the game is trying to avoid on the retinue's own attack cadence (see the swing-
// phase desync comment in SwarmCommands.cpp) — a rigid enemy line reads as scripted, not
// implacable. Letting ordinary steering own the whole lifecycle costs nothing extra: it
// is the same per-unit seek+separation math regardless of whether a unit spawned in a
// row five seconds ago or is mid-swing, so there is no separate bookkeeping to pay for.
//----------------------------------------------------------------------
UBroodSteeringProcessor::UBroodSteeringProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmSteering"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmGrid")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UBroodSteeringProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FBroodTag>(EMassFragmentPresence::All);
}

void UBroodSteeringProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmBroodSteering, SwarmBroodSteering);

	const USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();

	// Snapshotted once per pass — the hot loop does zero CVar reads.
	const float Speed = FMath::Max(CVarBroodSpeed.GetValueOnAnyThread(), 0.f);
	const float SepRadius = FMath::Max(CVarBroodSeparation.GetValueOnAnyThread(), 0.f);
	const float SepWeight = CVarBroodSeparationWeight.GetValueOnAnyThread();
	const int32 SepCap = FMath::Clamp(CVarBroodSeparationCap.GetValueOnAnyThread(), 1, 16);
	const float AggroSq = FMath::Square(FMath::Max(CVarBroodAggroRange.GetValueOnAnyThread(), 0.f));

	// Simulation LOD, snapshotted with everything else so the hot loop reads no CVars.
	const int32 LodStride = FMath::Clamp(CVarSimLodStride.GetValueOnAnyThread(), 1, 8);
	const float LodNearSq = FMath::Square(FMath::Max(CVarSimLodNearRadius.GetValueOnAnyThread(), 0.f));
	// Which phase re-steers this frame. GFrameNumber (not a wall clock) so the split is exact
	// and reproducible in a benchmark rather than drifting with frame time.
	const uint32 LodPhase = LodStride > 1 ? (GFrameNumber % (uint32)LodStride) : 0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor, Speed, SepRadius, SepWeight, SepCap, AggroSq, LodStride, LodNearSq, LodPhase](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const FVector Location = Transforms[i].GetTransform().GetLocation();

			// --- simulation LOD gate ---------------------------------------------------
			// Cheap test first (one DistSquared2D) so the skip actually saves the two grid
			// queries below rather than trading them for something comparable.
			//
			// Phase is the chunk-local index, which spreads the far population evenly across
			// the N frames instead of re-steering all of them on the same one — a synchronised
			// stride would show up as a periodic hitch, which is the failure this avoids. It
			// reshuffles as entities die and chunks repack; that is harmless and mildly good,
			// since no unit can get stuck permanently on an unlucky phase.
			if (LodStride > 1
				&& (uint32)(i % LodStride) != LodPhase
				&& FVector::DistSquared2D(Location, Attractor) > LodNearSq)
			{
				continue; // keep last frame's velocity; integration still moves it
			}

			// Bite whatever is in front of you; otherwise keep marching on the hero.
			// Whether a front line forms at all is decided here: a brood only diverts to
			// a soldier inside AggroRange, so shrinking it lets the tide flow PAST your
			// line toward the flame instead of piling against it.
			FVector EnemyLocation;
			float EnemyDistSq = 0.f;
			const bool bDivert = FindNearestEnemy(*Swarm, Location, /*bWantRetinue=*/true, EnemyLocation, EnemyDistSq)
				&& EnemyDistSq <= AggroSq;
			const FVector Target = bDivert ? EnemyLocation : Attractor;

			const FVector Seek = (Target - Location).GetSafeNormal2D();
			const FVector Push = SeparationForce(*Swarm, Location, SepRadius, SepCap) * SepWeight;

			FVector Desired = Seek + Push;
			Desired.Z = 0.f;
			Velocities[i].Value = Desired.GetSafeNormal2D() * Speed * Jitter[i].SpeedScale;
		}
	});
}

//----------------------------------------------------------------------
// Retinue: close ranks over the dead
//----------------------------------------------------------------------
URetinueFormationProcessor::URetinueFormationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmSteering"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmGrid")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	// Deliberately NOT ordered against RetinueFollow. If the follow pass runs first it
	// steers one frame against the pre-repack slots, which is a single frame of a line
	// that was about to re-form anyway — not worth a serialisation point on the group.
}

void URetinueFormationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRetinueFollowFragment>(EMassFragmentAccess::ReadWrite);
	// Read-only: which of the two dense index spaces (§1.2/§8) this soldier's SlotIndex
	// belongs to. Never written here — type is sticky, assigned once at recruit time.
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRetinueTag>(EMassFragmentPresence::All);
}

void URetinueFormationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmRetinueFormation, SwarmRetinueFormation);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm || !SwarmFormation::ReadParams().bCompact)
	{
		return;
	}
	const bool bNeedSpearmen = Swarm->NeedsFormationRepack(EUnitType::Spearmen);
	const bool bNeedArchers = Swarm->NeedsFormationRepack(EUnitType::Archers);
	if (!bNeedSpearmen && !bNeedArchers)
	{
		return;
	}

	// TWO independent dense repacks, one per TYPE (docs/design/squad-group-system.md §1.2,
	// §8) — not one retinue-wide sort (the old single-pool behavior), and NOT a separate
	// sort per each of the up to 8 command units: a type's units still share ONE dense
	// index space that subdivides into unit-sized chunks, just scoped to that type's own
	// pool instead of the whole retinue. A stable type skips its own repack while the
	// other reforms — cheaper in aggregate than one big sort. Compaction itself is still a
	// RANKING, not a reassignment, exactly as before: gather every live slot index (within
	// this type), sort, and each unit's new index is where its old one lands in that order.
	TArray<int32> LiveSpearmen, LiveArchers;
	LiveSpearmen.Reserve(FMath::Max(Swarm->GetAliveByType(EUnitType::Spearmen), 64));
	LiveArchers.Reserve(FMath::Max(Swarm->GetAliveByType(EUnitType::Archers), 64));

	EntityQuery.ForEachEntityChunk(Context, [&LiveSpearmen, &LiveArchers](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetFragmentView<FRetinueFollowFragment>();
		const TConstArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetFragmentView<FSwarmAnimFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			TArray<int32>& Bucket = (SwarmSquad::UnitType(Anim[i].SquadId) == EUnitType::Archers) ? LiveArchers : LiveSpearmen;
			Bucket.Add(Follow[i].SlotIndex);
		}
	});

	if (bNeedSpearmen) { LiveSpearmen.Sort(); }
	if (bNeedArchers) { LiveArchers.Sort(); }

	EntityQuery.ForEachEntityChunk(Context, [&LiveSpearmen, &LiveArchers, bNeedSpearmen, bNeedArchers](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetMutableFragmentView<FRetinueFollowFragment>();
		const TConstArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetFragmentView<FSwarmAnimFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const bool bArcher = SwarmSquad::UnitType(Anim[i].SquadId) == EUnitType::Archers;
			if (bArcher && bNeedArchers)
			{
				Follow[i].SlotIndex = Algo::LowerBound(LiveArchers, Follow[i].SlotIndex);
			}
			else if (!bArcher && bNeedSpearmen)
			{
				Follow[i].SlotIndex = Algo::LowerBound(LiveSpearmen, Follow[i].SlotIndex);
			}
		}
	});

	// Mark each type against the pool the repack was DERIVED from, not the one we just
	// produced, so a death landing mid-pass is picked up next frame instead of swallowed.
	if (bNeedSpearmen) { Swarm->MarkFormationPacked(EUnitType::Spearmen); }
	if (bNeedArchers) { Swarm->MarkFormationPacked(EUnitType::Archers); }
}

//----------------------------------------------------------------------
// Retinue: hold formation slot around the hero
//----------------------------------------------------------------------
URetinueFollowProcessor::URetinueFollowProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmSteering"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmGrid")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void URetinueFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRetinueFollowFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRetinueTag>(EMassFragmentPresence::All);
}

namespace
{
	/**
	 * Nearest LIVE Spearmen unit's centroid to Location, for Archers' Rally reflavor
	 * ("Fall Back: collapses behind the nearest Spearmen unit if one exists" — §1.8).
	 * O(MaxSquads) = O(8) per archer, the same fixed-small-constant class as Army View's
	 * own per-unit walk — not a per-soldier cross-entity query, so it stays Design-Law-5
	 * compliant. Falls back to the hero position (bFound=false) when no Spearmen stand.
	 */
	FVector NearestSpearmenCentroid(const USwarmSubsystem& Swarm, const FVector& Location, bool& bOutFound)
	{
		bOutFound = false;
		float BestSq = TNumericLimits<float>::Max();
		FVector Best = FVector::ZeroVector;
		for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
		{
			if (Swarm.GetSquadType(i) != EUnitType::Spearmen || Swarm.GetSquadStanding(i) <= 0)
			{
				continue;
			}
			const FVector Centroid = Swarm.GetSquadCentroid(i);
			const float DistSq = FVector::DistSquared2D(Centroid, Location);
			if (DistSq < BestSq)
			{
				BestSq = DistSq;
				Best = Centroid;
				bOutFound = true;
			}
		}
		return Best;
	}
}

void URetinueFollowProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmRetinueFollow, SwarmRetinueFollow);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();

	// Shape, spacing and bearing, read once per pass rather than per unit — one FParams
	// per TYPE now (docs/design/squad-group-system.md §1.7), resolved here rather than
	// baked at spawn so every dial in SwarmFormation.h re-forms the standing army the
	// instant it moves.
	const SwarmFormation::FParams SpearmenFormation = SwarmFormation::ReadParamsForType(EUnitType::Spearmen);
	const SwarmFormation::FParams ArchersFormation = SwarmFormation::ReadParamsForType(EUnitType::Archers);

	// Archer combat dials, snapshotted once per pass — see SwarmCombat.h for why these
	// exist (§2.2's minimum-viable ranged model) and §1.8 for the per-stance reflavor.
	const float ArchersRange = SwarmCombatTuning::ArchersEngageRange();
	const float ArchersMinRange = SwarmCombatTuning::ArchersMinEngageRange();

	int32 BrokenThisFrame = 0;

	EntityQuery.ForEachEntityChunk(Context, [Swarm, Attractor, SpearmenFormation, ArchersFormation, ArchersRange, ArchersMinRange, &BrokenThisFrame](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FRetinueFollowFragment> Follow = ChunkContext.GetMutableFragmentView<FRetinueFollowFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			const uint8 SquadByte = Anim[i].SquadId;
			const int32 UnitIndex = SwarmSquad::UnitIndex(SquadByte);
			const EUnitType Type = SwarmSquad::UnitType(SquadByte);
			const bool bArcher = Type == EUnitType::Archers;
			const SwarmFormation::FParams& Formation = bArcher ? ArchersFormation : SpearmenFormation;

			const FVector Location = Transforms[i].GetTransform().GetLocation();
			const FVector2D Slot = SwarmFormation::SlotOffset(Follow[i].SlotIndex, Formation);
			const float HeroDistSq = FVector::DistSquared2D(Location, Attractor);

			// --- leash (docs/RTS-VERTICAL-SLICE.md §2) -------------------
			// Latch out past Radius, latch back in only inside ReanchorRadius,
			// so a unit sitting exactly on the boundary can't flicker.
			if (Follow[i].bLeashBroken)
			{
				if (HeroDistSq < SwarmLeash::ReanchorRadiusSq)
				{
					Follow[i].bLeashBroken = false;
				}
			}
			else if (HeroDistSq > SwarmLeash::RadiusSq)
			{
				Follow[i].bLeashBroken = true;
			}

			// A broken unit drops to Follow and paths back — it never detaches. Reads its
			// OWN unit's order otherwise: docs/design/squad-group-system.md §3 — an order
			// now targets an address ("all units", the default, or one named unit); "all"
			// writes every per-unit slot identically (USwarmSubsystem::SetStance), so this
			// reproduces today's only behavior exactly until a unit is individually addressed.
			const ESwarmStance Stance = Follow[i].bLeashBroken ? ESwarmStance::Follow : Swarm->GetUnitStance(UnitIndex);
			const FVector StanceAnchor = Swarm->GetUnitStanceAnchor(UnitIndex);

			if (Follow[i].bLeashBroken)
			{
				++BrokenThisFrame;
			}

			// Warn before breaking: breaking stance must never feel random.
			const bool bWarn = !Follow[i].bLeashBroken && HeroDistSq > SwarmLeash::WarnRadiusSq;
			Anim[i].Bits = bWarn ? (Anim[i].Bits | SwarmAnim::LeashWarnBit)
								 : (Anim[i].Bits & ~SwarmAnim::LeashWarnBit);

			// --- where this stance wants the unit to stand (per-type reflavor, §1.8) -----
			FVector Anchor;
			float EngageRange;
			float MinEngageRange = 0.f;
			float SpeedMul = 1.f;
			// Archers NEVER close distance to melee — the one invariant every §1.8 row for
			// them shares ("hold formation... without closing distance"). Spearmen keep
			// today's exact behavior: step off the anchor toward whatever they're engaging.
			bool bMayCloseDistance = !bArcher;

			if (!bArcher)
			{
				switch (Stance)
				{
				case ESwarmStance::Hold:
					// Shield Wall: anchored to the world point where the order was issued.
					Anchor = StanceAnchor + FVector(Slot.X, Slot.Y, 0.f);
					EngageRange = SwarmTuning::LineEngageRange;
					break;
				case ESwarmStance::Charge:
					// Advance the Line.
					Anchor = StanceAnchor;
					EngageRange = SwarmTuning::ChargeEngageRange;
					SpeedMul = SwarmTuning::ChargeSpeedMul;
					break;
				case ESwarmStance::Rally:
					// To the Banner.
					Anchor = Attractor + FVector(Slot.X, Slot.Y, 0.f) * SwarmTuning::RallySlotScale;
					EngageRange = SwarmTuning::LineEngageRange;
					break;
				case ESwarmStance::Follow:
				default:
					Anchor = Attractor + FVector(Slot.X, Slot.Y, 0.f);
					EngageRange = SwarmTuning::FollowEngageRange;
					break;
				}
			}
			else
			{
				EngageRange = ArchersRange;
				MinEngageRange = ArchersMinRange;
				switch (Stance)
				{
				case ESwarmStance::Hold:
					// Loose from Cover: anchors IDENTICALLY to Follow — behaviorally almost
					// indistinguishable (archers are already static shooters); the value is
					// in ADDRESSING one unit to hold a firing position while others move.
				case ESwarmStance::Follow:
				default:
					Anchor = Attractor + FVector(Slot.X, Slot.Y, 0.f);
					break;
				case ESwarmStance::Charge:
					// Volley Advance: hold ground (never Charge's melee-advance), engaging at
					// the same reach — the "aggressive" read without breaking never-melee.
					Anchor = Attractor + FVector(Slot.X, Slot.Y, 0.f);
					break;
				case ESwarmStance::Rally:
				{
					// Fall Back: collapses behind the nearest Spearmen unit if one exists,
					// the hero otherwise — archers retreating behind the shield wall.
					bool bFoundSpearmen = false;
					const FVector FallBackTo = NearestSpearmenCentroid(*Swarm, Location, bFoundSpearmen);
					const FVector Base = bFoundSpearmen ? FallBackTo : Attractor;
					Anchor = Base + FVector(Slot.X, Slot.Y, 0.f) * SwarmTuning::RallySlotScale;
					break;
				}
				}
			}

			// --- auto-fight: step off the anchor for anything in reach ----
			// task-073: banded so an Archer's own MinEngageRange dead zone doesn't hide a
			// valid in-band target behind a closer dead-zone one (FindNearestEnemy only
			// ever returns the single closest entry, so a naive nearest-then-filter here
			// would reject engagement outright whenever the closest brood happened to be
			// the dead-zone one) — see FindNearestEnemyBanded's own comment for the fuller
			// escape-hatch rationale. Spearmen pass MinEngageRange == 0, which collapses
			// this back to exactly today's plain nearest-in-range search.
			FVector EnemyLocation;
			float EnemyDistSq = 0.f;
			const bool bEngaging = FindNearestEnemyBanded(*Swarm, Location, /*bWantRetinue=*/false,
				FMath::Square(MinEngageRange), FMath::Square(EngageRange), EnemyLocation, EnemyDistSq);

			const FVector Target = (bEngaging && bMayCloseDistance) ? EnemyLocation : Anchor;
			const FVector ToTarget = Target - Location;
			const float Dist = ToTarget.Size2D();

			// Arrive: full speed far out, ease onto the anchor.
			const float Arrive = FMath::Clamp(Dist / SwarmTuning::SlotArriveRadius, 0.f, 1.f);
			const FVector Push = SeparationForce(*Swarm, Location,
					SwarmTuning::SeparationRadius, SwarmTuning::SeparationNeighborCap) * SwarmTuning::SeparationWeight;

			// Don't renormalise: the blended vector's own length is the throttle.
			// Sitting on the anchor with no crowding => zero, so the line settles
			// instead of vibrating between seek and separation.
			FVector Desired = ToTarget.GetSafeNormal2D() * Arrive + Push;
			Desired.Z = 0.f;
			const float Throttle = FMath::Min(Desired.Size2D(), 1.f);
			// Jitter[i].SpeedScale already carries this soldier's TYPE speed multiplier
			// (baked in at recruit time, SwarmCommands.cpp) on top of its random jitter, so
			// Archers' slower march (Swarm.ArchersMoveSpeedScale) falls out here for free.
			Velocities[i].Value = Desired.GetSafeNormal2D()
				* SwarmTuning::RetinueSpeed * SpeedMul * Throttle * Jitter[i].SpeedScale;

			if (bEngaging)
			{
				Anim[i].Bits |= SwarmAnim::AttackBit;
			}
		}
	});

	Swarm->AddLeashBroken(BrokenThisFrame);
}

//----------------------------------------------------------------------
// Integrate + fill render buffers
//----------------------------------------------------------------------
USwarmIntegrateProcessor::USwarmIntegrateProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = FName(TEXT("SwarmIntegrate"));
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmCombat")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmIntegrateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmAnimFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSwarmJitterFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSwarmStrikeFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FSwarmTag>(EMassFragmentPresence::All);
}

void USwarmIntegrateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmIntegrate, SwarmIntegrate);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	const float TimeSeconds = Context.GetWorld()->GetTimeSeconds();

	// Swing / hit-reaction tunables, snapshotted once per pass.
	const float SwingInterval = SwarmCombatTuning::SwingInterval();
	const float StrikeAt = SwingInterval * SwarmCombatTuning::SwingStrikeAt();
	const float Lunge = SwarmCombatTuning::SwingLunge();

	// The attack POSE window, not the whole interval. A single attack frame held for a
	// short beat either side of the blow reads as a jab — lean in, connect, snap back.
	// Holding it for the full interval would just look like a unit permanently leaning.
	const float PoseStart = StrikeAt * 0.5f;
	const float PoseEnd = FMath::Min(StrikeAt + SwingInterval * 0.18f, SwingInterval);

	// Knockback decays exponentially with time constant KnockbackTime, which is what
	// makes the total displacement exactly Speed x KnockbackTime (see the combat pass)
	// and lets repeated hits stack additively without any special-casing. One Exp per
	// pass, not per entity.
	const float KnockDecay = FMath::Exp(-DeltaTime / SwarmCombatTuning::KnockbackTime());

	// Walk rate is per team so the tide and the line don't step in lockstep. Picked off
	// TeamBit below, which the entity already carries — no extra query or branch on tags.
	const float BroodWalkHz = FMath::Max(CVarBroodWalkHz.GetValueOnAnyThread(), 0.f);

	// Facing dials, snapshotted once. The hero position is the same one the retinue
	// already anchors to, so the ring cannot drift from the formation it is standing in.
	FFacingParams Facing;
	Facing.MoveSpeedSq = FMath::Square(FMath::Max(CVarFacingMoveSpeed.GetValueOnAnyThread(), 0.f));
	Facing.Hysteresis = FMath::Max(CVarFacingTurnHysteresis.GetValueOnAnyThread(), 0.f);
	Facing.GlancePeriod = CVarGlancePeriod.GetValueOnAnyThread();
	Facing.GlanceDuration = CVarGlanceDuration.GetValueOnAnyThread();
	Facing.bGlanceEnabled = CVarGlanceEnabled.GetValueOnAnyThread() != 0;
	const FVector HeroLocation = Swarm->GetAttractor();

	// Which look each body wears. Snapshotted here, once, exactly like the facing dials —
	// so dragging any weights CVar reskins the standing horde on the next frame. THREE
	// tables since task-126: brood read EnemyVariants, spearmen read TeamVariants, archers
	// read ArcherVariants, picked per-entity below on TeamBit plus the squad byte's unit
	// type. The team pair index the same atlas — the archer block's +11 row offset is added
	// in SwarmRenderActor.cpp's pack loop, not here, because the render int32's variant
	// field is four bits and seventeen looks do not fit a flat index (SwarmSheet::Team).
	const FVariantTable EnemyVariants = ParseVariantTable(
		CVarBroodVariantWeights.GetValueOnGameThread(), SwarmSheet::Enemy::Variants);
	const FVariantTable TeamVariants = ParseVariantTable(
		CVarTeamVariantWeights.GetValueOnGameThread(), SwarmSheet::Team::SpearVariants);
	const FVariantTable ArcherVariants = ParseVariantTable(
		CVarArcherVariantWeights.GetValueOnGameThread(), SwarmSheet::Team::ArcherVariants);

	EntityQuery.ForEachEntityChunk(Context, [Swarm, DeltaTime, TimeSeconds, SwingInterval, StrikeAt, PoseStart, PoseEnd, Lunge, KnockDecay, BroodWalkHz, Facing, HeroLocation, EnemyVariants, TeamVariants, ArcherVariants](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetFragmentView<FMassVelocityFragment>();
		const TArrayView<FSwarmAnimFragment> Anim = ChunkContext.GetMutableFragmentView<FSwarmAnimFragment>();
		const TConstArrayView<FSwarmJitterFragment> Jitter = ChunkContext.GetFragmentView<FSwarmJitterFragment>();
		const TArrayView<FSwarmStrikeFragment> Strike = ChunkContext.GetMutableFragmentView<FSwarmStrikeFragment>();

		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			FTransform& Transform = Transforms[i].GetMutableTransform();
			const FVector Velocity = Velocities[i].Value;
			FSwarmStrikeFragment& S = Strike[i];

			// --- knockback ------------------------------------------------
			// Added to steering velocity here rather than written into it, because
			// steering OVERWRITES FMassVelocityFragment outright every frame — an
			// impulse stored there would be gone before it moved anything.
			const FVector Push(S.Impulse.X, S.Impulse.Y, 0.f);
			Transform.SetTranslation(Transform.GetTranslation() + (Velocity + Push) * DeltaTime);
			S.Impulse *= KnockDecay;

			// --- swing clock ----------------------------------------------
			// Advances only while something is in reach (AttackBit, set by the combat
			// pass and by retinue steering), so idle units don't swing at air and a
			// unit that just made contact starts its windup from where it left off.
			const bool bWasAttacking = (Anim[i].Bits & SwarmAnim::AttackBit) != 0;
			bool bStrike = false;
			bool bSwinging = false;

			if (bWasAttacking)
			{
				const float Previous = S.SwingTime;
				S.SwingTime += DeltaTime;

				// Edge-triggered: true on exactly the one frame the clock crosses the
				// strike point, so a blow lands once no matter the frame rate.
				bStrike = (Previous < StrikeAt && S.SwingTime >= StrikeAt);

				bSwinging = (S.SwingTime >= PoseStart && S.SwingTime < PoseEnd);

				if (S.SwingTime >= SwingInterval)
				{
					S.SwingTime = FMath::Fmod(S.SwingTime, SwingInterval);
				}
			}
			else
			{
				S.SwingTime = 0.f;
			}
			S.bStrikeFrame = bStrike;

			// --- flash ----------------------------------------------------
			S.FlashTime = FMath::Max(S.FlashTime - DeltaTime, 0.f);

			const float WalkHz = (Anim[i].Bits & SwarmAnim::TeamBit) ? SwarmTuning::WalkAnimHz : BroodWalkHz;
				UpdateAnimBits(Anim[i].Bits, Velocity, TimeSeconds, Jitter[i].Phase, bWasAttacking, WalkHz);

			// Owned here, so they have to survive UpdateAnimBits — see
			// SwarmAnim::PreservedBits.
			Anim[i].Bits = bSwinging ? (Anim[i].Bits | SwarmAnim::SwingBit)
									 : (Anim[i].Bits & ~SwarmAnim::SwingBit);
			Anim[i].Bits = (S.FlashTime > 0.f) ? (Anim[i].Bits | SwarmAnim::HitFlashBit)
											   : (Anim[i].Bits & ~SwarmAnim::HitFlashBit);

			// The lunge is applied to the PUBLISHED position, not the transform: it is
			// a pose, and the sim must not be able to reach further or shove a neighbour
			// because a unit happens to be mid-swing. So the render position leads the
			// true position by a few uu during the pose, and nothing else notices.
			// Deliberately keeps the renderers dumb — they have no idea what a unit is
			// fighting, and now they don't need to.
			FVector Published = Transform.GetTranslation();
			if (bSwinging)
			{
				Published += FVector(S.Facing.X, S.Facing.Y, 0.f) * Lunge;
			}

			// Facing is resolved from the TRUE velocity and the TRUE position, not the
			// published ones: the lunge is a pose offset, and letting it feed back into
			// the direction would make a unit appear to turn every time it swung.
			Anim[i].Facing = ResolveFacing(Anim[i].Facing, Transform.GetTranslation(),
				Velocity, HeroLocation, (Anim[i].Bits & SwarmAnim::TeamBit) != 0,
				Jitter[i].Phase, TimeSeconds, Facing);

			// Size roll and atlas variant both ride along in the same int32, and both are
				// DERIVED from the jitter phase rather than stored, so no fragment grows a
				// field — see SwarmRenderPack. Which table picks the variant is TeamBit
				// plus, on the team side, the unit type this soldier was recruited as:
				// task-085 gave the team side its own eleven-look table instead of the
				// one-look-forever retinue used to have, and task-126 gave ARCHERS their own
				// six-look table on top, because until then an archer rolled a KNIGHT look
				// and the player's ranged line was invisible as a unit type.
				//
				// What goes into the int32 is the within-table index; the archer block's row
				// offset is added by the render bridge (SwarmSheet::Team::ArcherVariantBase).
				const bool bTeamEntity = (Anim[i].Bits & SwarmAnim::TeamBit) != 0;
				const bool bArcherEntity = bTeamEntity
					&& SwarmSquad::UnitType(Anim[i].SquadId) == EUnitType::Archers;
				const FVariantTable& MyVariants = bTeamEntity
					? (bArcherEntity ? ArcherVariants : TeamVariants)
					: EnemyVariants;
				Swarm->PushRenderEntry(Published, Anim[i].Bits, Anim[i].SquadId,
					SwarmRenderPack::BucketFromPhase(Jitter[i].Phase), Anim[i].Facing,
					SwarmRenderPack::VariantFromPhase(Jitter[i].Phase, MyVariants.Cum, MyVariants.Num));

			// Consume AttackBit: it is an observation made THIS frame by the steering
			// and combat passes, and it has to be re-observed next frame. Left set it
			// would latch permanently — UpdateAnimBits re-derives the bit from its own
			// previous value, so a unit that engaged once would read as attacking
			// forever. Harmless while nothing decoded the bit; not harmless now that it
			// gates the swing clock, which would leave veterans swinging at nothing.
			Anim[i].Bits &= ~SwarmAnim::AttackBit;
		}
	});

	// The N every ms number in this group is measured at. Live counts are only
	// valid once integrate has repacked the buffers, so they belong here.
	SET_DWORD_STAT(STAT_SwarmAliveRetinue, Swarm->GetAliveRetinue());
	SET_DWORD_STAT(STAT_SwarmAliveBrood, Swarm->GetAliveBrood());
}

//----------------------------------------------------------------------
// Contact: brood within range of hero
//----------------------------------------------------------------------
USwarmContactProcessor::USwarmContactProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteAfter.Add(FName(TEXT("SwarmIntegrate")));
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void USwarmContactProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FBroodTag>(EMassFragmentPresence::All);
}

void USwarmContactProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	SWARM_SCOPE(STAT_SwarmContact, SwarmContact);

	USwarmSubsystem* Swarm = Context.GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}
	const FVector Attractor = Swarm->GetAttractor();
	const float ContactRangeSq = FMath::Square(FMath::Max(CVarBroodContactRange.GetValueOnAnyThread(), 0.f));
	int32 Contacts = 0;

	EntityQuery.ForEachEntityChunk(Context, [&Contacts, Attractor, ContactRangeSq](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		for (int32 i = 0; i < ChunkContext.GetNumEntities(); ++i)
		{
			if (FVector::DistSquared2D(Transforms[i].GetTransform().GetLocation(), Attractor) < ContactRangeSq)
			{
				++Contacts;
			}
		}
	});

	Swarm->AddHeroContacts(Contacts);
}
