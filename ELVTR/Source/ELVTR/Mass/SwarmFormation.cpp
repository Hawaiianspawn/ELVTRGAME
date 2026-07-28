#include "SwarmFormation.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarShape(
		TEXT("Swarm.Formation.Shape"), 1,
		TEXT("How the retinue arranges itself around you.\n")
		TEXT("  0 Ring   - concentric rings, the original. Surrounds you, and under a camera\n")
		TEXT("             that does not rotate it parks half your army off the bottom of frame.\n")
		TEXT("  1 Block  - rectangle, Columns wide, ranks stacking away from camera (default).\n")
		TEXT("  2 Wedge  - V pointing away from camera, wings trailing back past you.\n")
		TEXT("  3 Arc    - shield wall bowed around the far side of you.\n")
		TEXT("Live: changing this re-forms the standing army, it is not a spawn property."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarSpacing(
		TEXT("Swarm.Formation.Spacing"), 42.4f,
		TEXT("Gap between neighbours WITHIN a rank, uu. Owner-tuned to 42.4 (was 110): tight\n")
		TEXT("ranks are what make the line read as ROWS in the Unit Cam panel rather than a\n")
		TEXT("loose crowd. This sits BELOW the ~70 threshold where the separation force\n")
		TEXT("(Swarm steering, 60uu personal space) starts fighting the slots and the line\n")
		TEXT("can seethe instead of standing — kept because it reads correctly on screen.\n")
		TEXT("If seething shows up in play, lower the separation force, not this. [40..400]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarRankSpacing(
		TEXT("Swarm.Formation.RankSpacing"), 110.f,
		TEXT("Gap BETWEEN ranks, uu — the depth dial. Under a shallow camera pitch this is\n")
		TEXT("what decides whether the back ranks are legible or hidden behind the front\n")
		TEXT("one, so it is worth setting larger than Spacing once Cam.Pitch leaves -90.\n")
		TEXT("[40..400]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarColumns(
		TEXT("Swarm.Formation.Columns"), 12,
		TEXT("Slots per rank, for Block and Arc. THE framing dial: wide and shallow puts\n")
		TEXT("the most bodies across the screen and makes losses read as the line getting\n")
		TEXT("shorter; narrow and deep reads as a column. Note the army-scale camera pulls\n")
		TEXT("back as you lose people, so a very wide line stays framed. [1..64]"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarForward(
		TEXT("Swarm.Formation.Forward"), 150.f,
		TEXT("Push the whole formation away from the camera, uu, so the bearer stands behind\n")
		TEXT("his line rather than inside it. Positive = deeper into frame. Negative puts\n")
		TEXT("the army between you and the viewer. [-2000..2000]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarArcDegrees(
		TEXT("Swarm.Formation.Arc"), 140.f,
		TEXT("Arc shape only: how many degrees of circle the front rank subtends. 360 is a\n")
		TEXT("closed ring at fixed radius; ~140 is a shield wall that still wraps your\n")
		TEXT("flanks; ~60 is nearly a straight line. [10..360]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarArcRadius(
		TEXT("Swarm.Formation.ArcRadius"), 700.f,
		TEXT("Arc shape only: radius of the FRONT rank, uu. Ranks behind it step inward by\n")
		TEXT("RankSpacing, so keep this comfortably above Columns * RankSpacing or the inner\n")
		TEXT("ranks collapse through the centre. [100..4000]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarYaw(
		TEXT("Swarm.Formation.Yaw"), 0.f,
		TEXT("Extra bearing, degrees, added on top of whatever FaceCamera resolves. Use it to\n")
		TEXT("angle the line off-square to the viewer without touching the camera. [-180..180]"),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarFaceCamera(
		TEXT("Swarm.Formation.FaceCamera"), 1,
		TEXT("1 = the formation's 'forward' is up-screen, tracking Emberkeep.Cam.Yaw, so the\n")
		TEXT("line stays broadside to the viewer however the camera is set (default).\n")
		TEXT("0 = forward is world +X regardless of where the camera is looking."),
		ECVF_Default);

	// --- per-type formation (docs/design/squad-group-system.md §1.7) -------------------
	// Spearmen keep the shared Swarm.Formation.* CVars above unchanged (they ARE today's
	// retinue). Archers get their own independent set, mirroring the precedent Swarm.
	// BroodFormation.* already set. Defaults match docs/data/unit-types.json's shipped
	// values — UNMEASURED placeholder dials, same status as everything else in that file.
	TAutoConsoleVariable<int32> CVarArchersShape(
		TEXT("Swarm.Formation.Archers.Shape"), 1,
		TEXT("Archer line shape. Same {0 Ring, 1 Block, 2 Wedge, 3 Arc} vocabulary as\n")
		TEXT("Swarm.Formation.Shape. Default Block — reads as ranks, same as Spearmen."),
		ECVF_Default);
	TAutoConsoleVariable<int32> CVarArchersColumns(
		TEXT("Swarm.Formation.Archers.Columns"), 20,
		TEXT("Archer slots per rank. Wider than Spearmen's 12 — a wide, shallow firing line\n")
		TEXT("reads as 'the line behind the wall' rather than a second block. [1..64]"),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersSpacing(
		TEXT("Swarm.Formation.Archers.Spacing"), 55.f,
		TEXT("Lateral gap within an archer rank, uu. Looser than Spearmen's 42.4 — a firing\n")
		TEXT("line doesn't need shoulder-to-shoulder density. [40..400]"), ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersRankSpacing(
		TEXT("Swarm.Formation.Archers.RankSpacing"), 70.f,
		TEXT("Gap between archer ranks, uu. Shallower than Spearmen's 110 — an archer line is\n")
		TEXT("1-2 ranks deep at v1 counts, not stacked. [40..400]"), ECVF_Default);
	TAutoConsoleVariable<float> CVarArchersForward(
		TEXT("Swarm.Formation.Archers.Forward"), 40.f,
		TEXT("THE load-bearing number (spec §1.7): push archers only slightly away from the\n")
		TEXT("bearer, well short of Spearmen's 250 — this is what makes 'archers behind\n")
		TEXT("spearmen' true on the ground, since the spear block physically screens them.\n")
		TEXT("[-2000..2000]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarCompact(
		TEXT("Swarm.Formation.Compact"), 1,
		TEXT("1 = slots re-densify as people die, so the formation visibly SHRINKS and its\n")
		TEXT("outline is a readout of your remaining strength (default).\n")
		TEXT("0 = each unit keeps the slot it spawned into, and casualties leave holes where\n")
		TEXT("they fell — the block stays full size and stops reporting anything.\n")
		TEXT("Caveat: turning this off does not restore spawn slots that have already been\n")
		TEXT("compacted, it only stops further repacking. Respawn to get them back."),
		ECVF_Default);

	/** Rotate a formation-space (local) offset onto the world ground plane by a bearing. */
	FVector2D RotateToWorld(const FVector2D& Local, float YawRadians)
	{
		const float S = FMath::Sin(YawRadians);
		const float C = FMath::Cos(YawRadians);
		return FVector2D(Local.X * C - Local.Y * S, Local.X * S + Local.Y * C);
	}

	/** Ring slot in formation space. Ring r holds 8r slots at r * Spacing. */
	FVector2D RingSlot(int32 Index, const SwarmFormation::FParams& P)
	{
		int32 Ring = 1;
		int32 SlotsBefore = 0;
		while (Index >= SlotsBefore + Ring * 8)
		{
			SlotsBefore += Ring * 8;
			++Ring;
		}
		const int32 SlotInRing = Index - SlotsBefore;
		const float Angle = (2.f * PI * SlotInRing) / (Ring * 8);
		const float Radius = Ring * P.Spacing;
		return FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
	}

	/** Rectangle: rank 0 nearest the bearer, ranks stacking away from camera. */
	FVector2D BlockSlot(int32 Index, const SwarmFormation::FParams& P)
	{
		const int32 Columns = FMath::Max(P.Columns, 1);
		const int32 Rank = Index / Columns;
		const int32 Column = Index % Columns;

		// Centre the rank on the anchor so the bearer sits under the middle of his line
		// rather than off its left end.
		const float Right = (Column - (Columns - 1) * 0.5f) * P.Spacing;
		return FVector2D(Rank * P.RankSpacing, Right);
	}

	/**
	 * V, apex furthest from camera. Rank r holds 2r+1 slots, so the wings widen by one
	 * either side per rank while stepping back toward the bearer — a shape that reads as
	 * pointed at whatever you are walking into.
	 */
	FVector2D WedgeSlot(int32 Index, const SwarmFormation::FParams& P)
	{
		int32 Rank = 0;
		int32 SlotsBefore = 0;
		while (Index >= SlotsBefore + (2 * Rank + 1))
		{
			SlotsBefore += 2 * Rank + 1;
			++Rank;
		}
		const int32 SlotInRank = Index - SlotsBefore;
		const float Right = (SlotInRank - Rank) * P.Spacing;
		return FVector2D(-Rank * P.RankSpacing, Right);
	}

	/** Shield wall: Columns-wide ranks bowed on an arc, each rank one step inward. */
	FVector2D ArcSlot(int32 Index, const SwarmFormation::FParams& P)
	{
		const int32 Columns = FMath::Max(P.Columns, 1);
		const int32 Rank = Index / Columns;
		const int32 Column = Index % Columns;

		const float Sweep = FMath::DegreesToRadians(FMath::Clamp(P.ArcDegrees, 1.f, 360.f));
		const float T = (Columns > 1) ? ((float)Column / (float)(Columns - 1) - 0.5f) : 0.f;
		const float Angle = T * Sweep;

		// Floor the radius rather than letting deep formations invert through the centre,
		// which would fold the back ranks out the far side pointing the wrong way.
		const float Radius = FMath::Max(P.ArcRadius - Rank * P.RankSpacing, P.Spacing);
		return FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
	}

	/**
	 * Brood's arc slot — see SwarmFormation::BroodSlotOffset for why this isn't ArcSlot
	 * with a sign flipped. Rank 0 sits at ArcRadius (the front, closest to the anchor,
	 * arriving first); later ranks step OUTWARD, away from the anchor, the opposite sense
	 * from the retinue's shield wall stepping inward toward a bearer standing still.
	 */
	FVector2D BroodArcSlot(int32 Index, const SwarmFormation::FParams& P)
	{
		const int32 Columns = FMath::Max(P.Columns, 1);
		const int32 Rank = Index / Columns;
		const int32 Column = Index % Columns;

		const float Sweep = FMath::DegreesToRadians(FMath::Clamp(P.ArcDegrees, 0.f, 360.f));
		const float T = (Columns > 1) ? ((float)Column / (float)(Columns - 1) - 0.5f) : 0.f;
		const float Angle = T * Sweep;

		const float Radius = FMath::Max(P.ArcRadius + Rank * P.RankSpacing, 1.f);
		return FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
	}
}

namespace SwarmFormation
{
	FParams ReadParams()
	{
		FParams P;
		P.Shape = (EShape)FMath::Clamp(CVarShape.GetValueOnAnyThread(), 0, 3);
		P.Spacing = FMath::Max(CVarSpacing.GetValueOnAnyThread(), 1.f);
		P.RankSpacing = FMath::Max(CVarRankSpacing.GetValueOnAnyThread(), 1.f);
		P.Columns = FMath::Clamp(CVarColumns.GetValueOnAnyThread(), 1, 64);
		P.Forward = CVarForward.GetValueOnAnyThread();
		P.ArcDegrees = CVarArcDegrees.GetValueOnAnyThread();
		P.ArcRadius = CVarArcRadius.GetValueOnAnyThread();
		P.bCompact = CVarCompact.GetValueOnAnyThread() != 0;

		const float Bearing = CVarYaw.GetValueOnAnyThread()
			+ (CVarFaceCamera.GetValueOnAnyThread() != 0 ? CameraYawDegrees() : 0.f);
		P.YawRadians = FMath::DegreesToRadians(Bearing);
		return P;
	}

	FParams ReadParamsForType(EUnitType Type)
	{
		if (Type == EUnitType::Spearmen)
		{
			return ReadParams(); // Spearmen ARE today's retinue, unchanged.
		}

		FParams P;
		P.Shape = (EShape)FMath::Clamp(CVarArchersShape.GetValueOnAnyThread(), 0, 3);
		P.Spacing = FMath::Max(CVarArchersSpacing.GetValueOnAnyThread(), 1.f);
		P.RankSpacing = FMath::Max(CVarArchersRankSpacing.GetValueOnAnyThread(), 1.f);
		P.Columns = FMath::Clamp(CVarArchersColumns.GetValueOnAnyThread(), 1, 64);
		P.Forward = CVarArchersForward.GetValueOnAnyThread();
		// Shared across types on purpose — see ReadParamsForType's doc comment in
		// SwarmFormation.h for why (unit-types.json ships identical arc dials for both).
		P.ArcDegrees = CVarArcDegrees.GetValueOnAnyThread();
		P.ArcRadius = CVarArcRadius.GetValueOnAnyThread();
		P.bCompact = CVarCompact.GetValueOnAnyThread() != 0;

		const float Bearing = CVarYaw.GetValueOnAnyThread()
			+ (CVarFaceCamera.GetValueOnAnyThread() != 0 ? CameraYawDegrees() : 0.f);
		P.YawRadians = FMath::DegreesToRadians(Bearing);
		return P;
	}

	FVector2D SlotOffset(int32 Index, const FParams& P)
	{
		Index = FMath::Max(Index, 0);

		FVector2D Local;
		switch (P.Shape)
		{
		case EShape::Block: Local = BlockSlot(Index, P); break;
		case EShape::Wedge: Local = WedgeSlot(Index, P); break;
		case EShape::Arc:   Local = ArcSlot(Index, P);   break;
		case EShape::Ring:
		default:            Local = RingSlot(Index, P);  break;
		}

		// Ring is the one shape with no front, so a forward shove would just slide the
		// bearer out of his own circle rather than framing anything. Left alone.
		if (P.Shape != EShape::Ring)
		{
			Local.X += P.Forward;
		}

		// Formation space is (forward, right); rotate it onto the world ground plane.
		// At Yaw 0 forward is world +X, which is what 'W' pushes along and what the
		// camera treats as up-screen — the three agree by construction.
		return RotateToWorld(Local, P.YawRadians);
	}

	FVector2D BroodSlotOffset(int32 Index, const FParams& P)
	{
		Index = FMath::Max(Index, 0);
		return RotateToWorld(BroodArcSlot(Index, P), P.YawRadians);
	}

	/**
	 * The camera's bearing, read by name rather than by linking to the pawn.
	 *
	 * Emberkeep.Cam.Yaw is owned by SpikeHeroPawn.cpp — a spike actor the Mass side has
	 * no business depending on, and which may not exist at all in a later shipping mode.
	 * Finding it by name costs one lookup per pass and degrades to "world axes" if the
	 * pawn's translation unit never registered it, which is exactly the right failure.
	 */
	float CameraYawDegrees()
	{
		static IConsoleVariable* CamYaw = nullptr;
		if (!CamYaw)
		{
			CamYaw = IConsoleManager::Get().FindConsoleVariable(TEXT("Emberkeep.Cam.Yaw"));
		}
		return CamYaw ? CamYaw->GetFloat() : 0.f;
	}
}
