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
		TEXT("Swarm.Formation.Columns"), 8,
		TEXT("Slots per rank, for Block and Arc. THE framing dial: wide and shallow puts\n")
		TEXT("the most bodies across the screen and makes losses read as the line getting\n")
		TEXT("shorter; narrow and deep reads as a column. Note the army-scale camera pulls\n")
		TEXT("back as you lose people, so a very wide line stays framed. [1..64]"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarGroupGap(
		TEXT("Swarm.Formation.GroupGap"), 80.f,
		TEXT("Block shape only: clear ground between DETACHMENTS, uu. One detachment per\n")
		TEXT("unique sprite in the standing army — each look gets its own Columns-wide block,\n")
		TEXT("deepening as that look recruits. 0 closes the gaps and the detachments touch,\n")
		TEXT("which is the old one-continuous-block reading with the looks still in bands.\n")
		TEXT("[0..2000]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarGroupsPerRow(
		TEXT("Swarm.Formation.GroupsPerRow"), 4,
		TEXT("Block shape only: how many DETACHMENTS stand abreast before the next one wraps\n")
		TEXT("to a second row behind them. THE LEASH DIAL: soldiers past Swarm's 2000uu leash\n")
		TEXT("radius abandon their slot and run back, so a line of thirteen looks laid out in\n")
		TEXT("one row (measured 2026-08-01: 35 of 120 latched broken) never forms at all.\n")
		TEXT("Keep GroupsPerRow * (Columns * Spacing + GroupGap) / 2 well under 2000.\n")
		TEXT("0 = never wrap, one row however wide it gets. [0..32]"), ECVF_Default);

	TAutoConsoleVariable<float> CVarGroupRowPitch(
		TEXT("Swarm.Formation.GroupRowPitch"), 300.f,
		TEXT("Block shape only: depth between wrapped detachment rows, uu. Fixed, not derived\n")
		TEXT("from how deep the blocks in front actually are, so a look that recruits past\n")
		TEXT("roughly GroupRowPitch / RankSpacing ranks will grow into the row behind it —\n")
		TEXT("raise this or Columns when that shows up. Ignored when GroupsPerRow is 0.\n")
		TEXT("[0..4000]"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarGroupDepthCap(
		TEXT("Swarm.Formation.GroupDepthCap"), 2,
		TEXT("Block shape only: ranks a look fills (Columns * GroupDepthCap soldiers) before\n")
		TEXT("opening a sibling detachment of the same look, rather than deepening forever.\n")
		TEXT("4x2 at the default Columns 4 / GroupDepthCap 2; raise to 4 for 4x4 later, no\n")
		TEXT("code change needed. Shared across types, same reasoning as GroupGap/GroupsPerRow.\n")
		TEXT("Note: a command unit that has absorbed unbounded overflow recruits past\n")
		TEXT("AssignRecruit's soft 16-soldier ceiling (once all 8 unit handles are claimed)\n")
		TEXT("can still open more detachments than one row comfortably holds — this dial caps\n")
		TEXT("detachment DEPTH, not how many detachments a unit can have. [1..16]"), ECVF_Default);

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
		TEXT("1 = the formation's 'forward' is up-screen, tracking Kindled.Cam.Yaw, so the\n")
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
		TEXT("Swarm.Formation.Archers.Columns"), 8,
		TEXT("Archer slots per rank. Was 20, when this was the frontage of the WHOLE archer\n")
		TEXT("line and wide-and-shallow read as 'the line behind the wall'. Under Block it is\n")
		TEXT("now the frontage of ONE DETACHMENT, and thirteen archer looks at 20 wide put the\n")
		TEXT("outer blocks past the 2000uu leash, where soldiers drop their slot and run back.\n")
		TEXT("See Swarm.Formation.GroupsPerRow for the width budget. [1..64]"),
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

	/**
	 * Rectangle: rank 0 nearest the bearer, ranks stacking away from camera.
	 *
	 * Group is a DETACHMENT — one per unique sprite, Index counts within it. Every
	 * detachment is the same Columns wide and grows in DEPTH as its look recruits, so
	 * the frontage a look occupies is fixed and its block visibly thickens: an army of
	 * specific units that gets bigger unit by unit, which is the whole readout.
	 *
	 * Groups alternate right/left of centre (0, +1, -1, +2, -2 ...) rather than running
	 * left to right. This function is PURE — it sees one index, never the army's total —
	 * so it cannot centre a left-to-right row it doesn't know the length of, and a row
	 * that grew rightward would slide the whole army off the anchor as it recruited.
	 * Alternating self-centres at every count.
	 *
	 * Formation only, same as the repack: a detachment is not a command handle. Command
	 * is by type (docs/design/DIRECTION-2026-07-31.md D14).
	 */
	FVector2D BlockSlot(int32 Index, int32 Group, const SwarmFormation::FParams& P)
	{
		const int32 Columns = FMath::Max(P.Columns, 1);
		Group = FMath::Max(Group, 0);

		const int32 Rank = Index / Columns;
		const int32 Column = Index % Columns;

		// Centre the rank on the anchor so the bearer sits under the middle of his line
		// rather than off its left end.
		const float Right = (Column - (Columns - 1) * 0.5f) * P.Spacing;

		// Columns * Spacing, not (Columns - 1) * Spacing: the block is (Columns-1) gaps
		// wide, and the extra Spacing is the one slot of pitch that keeps GroupGap the
		// actual clear ground between two blocks rather than centre-to-centre distance.
		const float GroupPitch = Columns * P.Spacing + FMath::Max(P.GroupGap, 0.f);

		// Wrap to a second row of detachments rather than letting one row grow without
		// bound: past the 2000uu leash a soldier drops his slot and runs back to the
		// bearer, so a wide enough row is a formation that never forms (measured
		// 2026-08-01 at thirteen archer looks — 35 of 120 latched broken).
		// ponytail: fixed row pitch, not derived from the depth of the blocks in front.
		// Derive it if a look ever recruits deep enough to grow into the row behind.
		const int32 PerRow = (P.GroupsPerRow > 0) ? P.GroupsPerRow : MAX_int32;
		const int32 GroupRow = Group / PerRow;
		const int32 GroupCol = Group % PerRow;

		const float Side = (GroupCol % 2) ? 1.f : -1.f;
		const float GroupRight = Side * (float)((GroupCol + 1) / 2) * GroupPitch;

		return FVector2D(Rank * P.RankSpacing + GroupRow * P.GroupRowPitch, Right + GroupRight);
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
		P.GroupGap = CVarGroupGap.GetValueOnAnyThread();
		P.GroupsPerRow = FMath::Clamp(CVarGroupsPerRow.GetValueOnAnyThread(), 0, 32);
		P.GroupRowPitch = CVarGroupRowPitch.GetValueOnAnyThread();
		P.GroupDepthCap = FMath::Clamp(CVarGroupDepthCap.GetValueOnAnyThread(), 1, 16);
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
		// Detachment gap is shared with Spearmen, same reasoning as the arc dials below:
		// how far apart the blocks stand is an army-wide reading, not a per-type shape.
		P.GroupGap = CVarGroupGap.GetValueOnAnyThread();
		P.GroupsPerRow = FMath::Clamp(CVarGroupsPerRow.GetValueOnAnyThread(), 0, 32);
		P.GroupRowPitch = CVarGroupRowPitch.GetValueOnAnyThread();
		P.GroupDepthCap = FMath::Clamp(CVarGroupDepthCap.GetValueOnAnyThread(), 1, 16);
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

	FVector2D SlotOffset(int32 Index, int32 GroupIndex, const FParams& P)
	{
		Index = FMath::Max(Index, 0);

		FVector2D Local;
		switch (P.Shape)
		{
		case EShape::Block: Local = BlockSlot(Index, GroupIndex, P); break;
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
	 * Kindled.Cam.Yaw is owned by SpikeHeroPawn.cpp — a spike actor the Mass side has
	 * no business depending on, and which may not exist at all in a later shipping mode.
	 * Finding it by name costs one lookup per pass and degrades to "world axes" if the
	 * pawn's translation unit never registered it, which is exactly the right failure.
	 */
	float CameraYawDegrees()
	{
		static IConsoleVariable* CamYaw = nullptr;
		if (!CamYaw)
		{
			CamYaw = IConsoleManager::Get().FindConsoleVariable(TEXT("Kindled.Cam.Yaw"));
		}
		return CamYaw ? CamYaw->GetFloat() : 0.f;
	}
}
