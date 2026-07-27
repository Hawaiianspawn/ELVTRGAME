#pragma once

#include "MassEntityTypes.h"
#include "SwarmFragments.generated.h"

// Anim byte layout (mirrored in NS_Swarm / M_Swarm):
//   bit 0: walk frame (0/1)
//   bit 1: attacking (a target is in reach — "engaged", not "swinging")
//   bit 2: flip X
//   bit 3: team (0 = brood, 1 = retinue)
//   bit 4: leash warning (retinue only, debug/HUD — not decoded by the SubUV bridge)
//   bit 5: mid-swing (windup -> strike -> recover; the attack pose)
//   bit 6: hit flash (struck this instant)
namespace SwarmAnim
{
	constexpr uint8 FrameBit = 1 << 0;
	constexpr uint8 AttackBit = 1 << 1;
	constexpr uint8 FlipBit = 1 << 2;
	constexpr uint8 TeamBit = 1 << 3;
	constexpr uint8 LeashWarnBit = 1 << 4;

	// Bits 5-6 are the hit-reaction channel. Written by the strike pass (see
	// FSwarmStrikeFragment), consumed by the debug renderer today and reserved for
	// the SubUV attack frame once the sprite sheet is widened
	// (docs/RENDERING-LIGHTING.md §4a).
	constexpr uint8 SwingBit = 1 << 5;
	constexpr uint8 HitFlashBit = 1 << 6;

	/**
	 * Bits NOT owned by the per-frame walk-cycle rebuild in UpdateAnimBits.
	 * Anything set by another pass has to be listed here or it is wiped every tick.
	 */
	constexpr uint8 PreservedBits = TeamBit | LeashWarnBit | SwingBit | HitFlashBit;
}

/**
 * T_Swarm_2bit layout — the contract between the anim byte and the sprite sheet.
 *
 * Lives here, next to the bits it decodes, because THREE things have to agree about
 * this grid and they are in three different places: the Niagara Sprite Renderer's
 * "Sub UV" field (an editor asset, ELVTR/SETUP-EDITOR.md §3.5), the SubImage index the
 * render bridge computes, and the atlas UVs the Unit Cam brush slices. The asset can't
 * read this header, so if you change Columns/Rows you MUST change Sub UV to match — the
 * failure is silent and looks like every unit wearing the wrong frame.
 *
 * r2 (2026-07-26) turned the COLUMN axis into facing. The old layout spent its four
 * columns on walk0/walk1/attack/hit; the new one spends eight on direction and keeps
 * only the two walk frames, on the owner's call:
 *
 *   - ATTACK is not lost — it was always also a lunge (the render position leads the
 *     true position during the swing pose, see USwarmIntegrateProcessor), and that
 *     lunge is now the whole tell.
 *   - HIT *is* lost on the sprite path. The debug renderer still flashes, but Niagara
 *     has no per-particle colour array here, so restoring a hit tell means either a new
 *     array + graph edit or reinstating a cell axis. SwingBit/HitFlashBit are still
 *     written by the strike pass and still read by the debug renderer — they are simply
 *     no longer decoded into a cell. Don't delete them.
 *
 *      col:    0      1      2      3      4      5      6      7
 *              S     SE      E     NE      N     NW      W     SW
 *   row 0:  brood walk0, all eight columns (the brood has no rotations yet — every
 *   row 1:  brood walk1,  column packs the same south frame, so the decode below is
 *                         already correct when it gains real ones)
 *   row 2:  retinue walk0, eight real facings
 *   row 3:  retinue walk1
 *
 * Column order is south-first and counter-clockwise on screen, matching the direction
 * order PixelLab returns rotations in and the frame_map in
 * docs/data/art/requests/swarm-units.json. Changing it means changing both.
 */
namespace SwarmSheet
{
	constexpr int32 Columns = 8;
	constexpr int32 Rows = 4;

	constexpr int32 RowBroodWalk0 = 0;
	constexpr int32 RowBroodWalk1 = 1;
	constexpr int32 RowRetinueWalk0 = 2;
	constexpr int32 RowRetinueWalk1 = 3;

	/**
	 * Sheet cell for an anim byte and an already-resolved direction column.
	 *
	 * The column is passed in rather than decoded from the anim byte because facing is
	 * VIEW-relative and the anim byte is not: the same unit is column 0 to the main
	 * camera and some other column to the Unit Cam, whose yaw differs. Each consumer
	 * resolves its own column via SwarmFacing::ColumnFor and hands it here, so the two
	 * cannot drift on what a row means even though they disagree about direction.
	 */
	FORCEINLINE int32 CellFor(uint8 Bits, int32 DirColumn)
	{
		const int32 Col = FMath::Clamp(DirColumn, 0, Columns - 1);
		const bool bRetinue = (Bits & SwarmAnim::TeamBit) != 0;
		const bool bFrame1 = (Bits & SwarmAnim::FrameBit) != 0;
		const int32 Row = bRetinue ? (bFrame1 ? RowRetinueWalk1 : RowRetinueWalk0)
								   : (bFrame1 ? RowBroodWalk1 : RowBroodWalk0);
		return Col + Row * Columns;
	}
}

/**
 * World facing, quantised to 32 steps, and the view-relative column it becomes.
 *
 * Why 32 and not 8: the sheet has eight columns, but the SIM must not be the thing that
 * knows that. Camera yaw is a live CVar (Emberkeep.Cam.Yaw) that spins the map under the
 * player, and the Unit Cam looks from somewhere else entirely — so a facing baked to
 * eight screen-relative steps in the sim would be wrong for one of them and would snap
 * visibly when the view rotated. Storing a WORLD angle at 4x the sheet's resolution lets
 * each view apply its own yaw and quantise independently, and leaves headroom for a
 * 16-column sheet without touching the sim or the packing.
 *
 * Screen convention at the default top-down camera (Pitch -90, Yaw 0): screen-up is +X
 * and screen-right is +Y, so "south" — the unit facing the player — is -X.
 */
namespace SwarmFacing
{
	constexpr int32 Steps = 32;						// world quantisation
	constexpr int32 StepMask = Steps - 1;
	constexpr float DegreesPerStep = 360.f / (float)Steps;

	/** World ground-plane direction -> 0..31, south-first and counter-clockwise. */
	FORCEINLINE int32 IndexFromDir(const FVector2f& Dir)
	{
		if (Dir.IsNearlyZero())
		{
			return 0;
		}
		// atan2(Y, -X) puts south (-X) at 0 and rises through south-east, east, north-east.
		const float Deg = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, -Dir.X));
		const int32 Step = FMath::RoundToInt(Deg / DegreesPerStep);
		return Step & StepMask;						// mask, not modulo: handles negatives
	}

	/** 0..31 world facing -> 0..7 sheet column, for a view rotated by ViewYawDegrees. */
	FORCEINLINE int32 ColumnFor(int32 WorldIndex, float ViewYawDegrees, int32 SheetColumns)
	{
		const float Deg = (float)WorldIndex * DegreesPerStep - ViewYawDegrees;
		const float PerColumn = 360.f / (float)SheetColumns;
		const int32 Col = FMath::RoundToInt(Deg / PerColumn);
		return ((Col % SheetColumns) + SheetColumns) % SheetColumns;
	}
}

/**
 * How the render buffer's int32 is laid out.
 *
 * USwarmSubsystem::RenderAnimBits has always been a TArray<int32> carrying a 7-bit anim
 * byte, so 24 bits were going to the renderer empty every frame. Per-unit size variation
 * rides in them.
 *
 * The alternative was the obvious one — a size float on FSwarmJitterFragment plus a
 * TArray<float> on the subsystem — and it was rejected on cost, not taste: both are
 * class-layout changes, so both force a full editor-closed rebuild every time they are
 * touched, and the whole point of this surface is that it moves while the game runs.
 *
 * The seam that makes this safe is that every consumer either masks a single bit
 * (`& TeamBit`) or casts to uint8 before decoding a sheet cell, so nothing below bit 8
 * can see what is above it. KEEP IT THAT WAY: a consumer that compares the whole int32,
 * or drops the (uint8) cast on CellFor, silently reads a size or a facing as anim state.
 *
 *   bits 0-7   anim byte (SwarmAnim)
 *   bits 8-11  size bucket: a per-entity uniform random, 0-15
 *   bits 12-16 world facing, 32 steps (SwarmFacing)
 *   bits 17-31 free
 */
namespace SwarmRenderPack
{
	constexpr int32 AnimMask = 0xFF;
	constexpr int32 SizeShift = 8;
	constexpr int32 SizeSteps = 16;					// 4 bits
	constexpr int32 SizeIndexMask = SizeSteps - 1;

	// Bits 12-16: world facing, 32 steps (SwarmFacing). Rides here for exactly the
	// reason the size roll does — RenderAnimBits was already a TArray<int32> carrying a
	// 7-bit byte, so this costs no new array, no new fragment field on the render path,
	// and no class-layout change on the subsystem.
	constexpr int32 FacingShift = 12;
	constexpr int32 FacingIndexMask = SwarmFacing::StepMask;

	FORCEINLINE int32 Pack(uint8 AnimBits, int32 SizeBucket, int32 FacingIndex = 0)
	{
		return (int32)AnimBits
			| ((SizeBucket & SizeIndexMask) << SizeShift)
			| ((FacingIndex & FacingIndexMask) << FacingShift);
	}

	FORCEINLINE uint8 Anim(int32 Packed) { return (uint8)(Packed & AnimMask); }
	FORCEINLINE int32 SizeBucket(int32 Packed) { return (Packed >> SizeShift) & SizeIndexMask; }
	FORCEINLINE int32 Facing(int32 Packed) { return (Packed >> FacingShift) & FacingIndexMask; }

	/**
	 * Per-entity size multiplier, 1 +/- Amplitude.
	 *
	 * The AMPLITUDE is not baked into the buffer — only the raw 0-15 roll is. That is
	 * deliberate: it means Swarm.BroodSizeJitter retunes the whole horde on the frame you
	 * drag it, with no respawn, because the sim never knew the number in the first place.
	 */
	FORCEINLINE float SizeScale(int32 Packed, float Amplitude)
	{
		const float U = (float)SizeBucket(Packed) / (float)SizeIndexMask;	// 0..1
		return 1.f + Amplitude * (2.f * U - 1.f);
	}

	/**
	 * Stable bucket for an entity, derived from the walk-cycle phase it already carries.
	 *
	 * Phase is the only per-entity random the swarm has and it costs nothing to reuse:
	 * it is fixed at spawn, so a unit's size never shimmers, and no fragment grows a
	 * field. Multiplied by the golden ratio before taking the fraction so size does not
	 * correlate with walk phase or with the swing clock (also seeded from Phase) — three
	 * uses of one random is fine, three uses in lockstep would show.
	 */
	FORCEINLINE int32 BucketFromPhase(float Phase)
	{
		const float U = FMath::Frac(Phase * 0.6180339887f);
		return FMath::Clamp((int32)(U * (float)SizeSteps), 0, SizeIndexMask);
	}
}

USTRUCT()
struct FSwarmAnimFragment : public FMassFragment
{
	GENERATED_BODY()

	uint8 Bits = 0;

	// Retinue only: cosmetic squad grouping for the muster UI (assigned at spawn by formation
	// slot). Not part of the anim byte sent to Niagara — the SubUV bridge reads Bits only.
	uint8 SquadId = 0;

	/**
	 * Current world facing, 0-31 (SwarmFacing::Steps). Stored rather than recomputed
	 * fresh each frame purely for HYSTERESIS: a unit steering along a boundary between
	 * two sheet columns recomputes an angle that dithers across it every tick, and the
	 * sprite would strobe between two facings while the unit walks in a straight line.
	 * Holding the last committed step and only moving off it past a threshold makes
	 * facing sticky, which is also how it should feel — soldiers turn, they don't snap.
	 */
	uint8 Facing = 0;
};

/**
 * Discrete melee cadence + hit reaction.
 *
 * Combat used to be a pure per-tick bleed, which meant nothing ever "landed" —
 * there was no instant for an attack pose, a flash, or a shove to belong to. This
 * fragment is that instant. Each unit runs its own swing clock; on the frame the
 * blow lands it raises bStrikeFrame, which the grid publishes so victims can pull
 * their own damage. The model stays victim-pull: no pass writes another entity.
 */
USTRUCT()
struct FSwarmStrikeFragment : public FMassFragment
{
	GENERATED_BODY()

	/** Seconds into the current swing. Only advances while a target is in reach. */
	float SwingTime = 0.f;

	/** Seconds of hit flash remaining. */
	float FlashTime = 0.f;

	/**
	 * Knockback velocity in uu/s, ground plane, decaying toward zero.
	 *
	 * A separate channel from FMassVelocityFragment on purpose: steering
	 * *overwrites* velocity outright every frame (SwarmProcessors.cpp), so an
	 * impulse stored there would be stomped before it moved anything. The
	 * integrate pass adds this on top of steering velocity.
	 */
	FVector2f Impulse = FVector2f::ZeroVector;

	/**
	 * Unit vector toward whatever this unit is currently fighting, zero if nothing.
	 *
	 * Not the same as velocity direction and can't be replaced by it: in contact the
	 * separation force pushes a unit *away* from the enemy it is attacking, so
	 * velocity points backwards exactly when the lunge needs to point forwards.
	 * Costs nothing — accumulated in the neighbour walk the combat pass already does.
	 */
	FVector2f Facing = FVector2f::ZeroVector;

	/**
	 * Squared distance to this unit's Kth-nearest enemy (K = Swarm.TargetsPerHit),
	 * recomputed every frame and published into the grid for victims to test against.
	 * See FGridEntry::StrikeReachSq for why one float replaced the attacker cap.
	 * 0 means "nothing in reach" — the blow lands on nobody.
	 */
	float StrikeReachSq = 0.f;

	/** True for exactly one frame, on the frame this unit's blow lands. */
	bool bStrikeFrame = false;
};

/** Per-entity variation so the brood doesn't move as one rigid mass. */
USTRUCT()
struct FSwarmJitterFragment : public FMassFragment
{
	GENERATED_BODY()

	float SpeedScale = 1.f;
	float Phase = 0.f; // walk-cycle offset, seconds
};

/** Retinue only: this unit's place in the formation. */
USTRUCT()
struct FRetinueFollowFragment : public FMassFragment
{
	GENERATED_BODY()

	/**
	 * Slot INDEX, not an offset — the offset is derived from it every frame by
	 * SwarmFormation::SlotOffset. Storing the resolved position here (as this did until
	 * 2026-07-26) made the shape a property of a spawn that had already happened, so no
	 * formation dial could move a standing army.
	 *
	 * Also not the spawn order: while Swarm.Formation.Compact is on, the repack pass
	 * re-densifies these as units die so the formation shrinks instead of going holey.
	 */
	int32 SlotIndex = 0;

	/**
	 * Leash hysteresis latch (docs/RTS-VERTICAL-SLICE.md §2). Set when the unit
	 * exceeds LeashRadius, cleared only once it is well back inside
	 * ReanchorRadius. While set the unit ignores the global stance and behaves
	 * as Follow — "you must stay in the fight with your troops".
	 */
	bool bLeashBroken = false;
};

USTRUCT()
struct FSwarmTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FBroodTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct FRetinueTag : public FMassTag { GENERATED_BODY() };
