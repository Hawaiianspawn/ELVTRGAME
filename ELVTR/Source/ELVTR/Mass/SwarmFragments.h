#pragma once

#include "MassEntityTypes.h"
#include "SwarmCombat.h" // EUnitType
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
 * T_Team_2bit / T_Enemy_2bit layout — the contract between the anim byte and the sprite
 * sheets. task-085 split what used to be ONE shared atlas (T_Swarm_2bit) into two,
 * one per side, because a single grid meant retargeting the enemy roster could never
 * avoid touching the team's rows too (SwarmFragments.h has said so since 2026-07-26).
 *
 * Lives here, next to the bits it decodes, because FOUR things have to agree about
 * EACH grid and they are in four different places — and there are now TWO such grids,
 * independent of each other:
 *
 *   | # | Team                                      | Enemy                            |
 *   |---|-------------------------------------------|-----------------------------------|
 *   | 1 | these constants (SwarmSheet::Team)         | SwarmSheet::Enemy                 |
 *   | 2 | docs/data/art/requests/team-units.json     | .../enemy-units.json output.grid  |
 *   | 3 | imported texture T_Team_2bit               | imported texture T_Enemy_2bit     |
 *   | 4 | NS_Swarm emitter "Team" Sub UV              | NS_Swarm emitter "Enemy" Sub UV   |
 *
 * Row 4 lives in a .uasset and cannot be grepped. Change a Columns/Rows constant and you
 * MUST change that side's Sub UV to match; the failure is silent and looks like every
 * unit on that side wearing the wrong frame. Scripts/art/check_brood_variants.py checks
 * 1 against 2, for BOTH sides, and prints what each Sub UV field has to hold. Full table
 * in docs/perf/niagara-sprite-path.md §1.
 *
 * r2 (2026-07-26) turned the COLUMN axis into facing; both sheets keep that shape —
 * eight columns of direction, two rows per look (walk0/walk1):
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
 *
 *   ENEMY (T_Enemy_2bit, 8x18): rows 0-17, the NINE brood variants, unchanged since
 *   2026-07-29 apart from losing the retinue rows that used to trail them —
 *   Row = Variant*2 + WalkFrame. docs/data/art/brood-variants.json still owns the mix.
 *
 *   TEAM (T_Team_2bit, 8x34): rows 0-33, SEVENTEEN variants in TWO BLOCKS —
 *     - rows 0-21, variants 0-10, the SPEARMAN block: index 0 is the retinue base look
 *       (same art, same index the pre-split sheet used, so the default appearance did
 *       not move), indices 1-10 are the ten knight keeps task-085 landed (see
 *       docs/data/art/team-variants.json for which index is which silhouette —
 *       task-095 binds combat stats to this exact ordering, so it is not a free re-sort).
 *     - rows 22-47, variants 11-23, the ARCHER block: the twelve owner-supplied
 *       pathfinder-line looks plus the one surviving knight-armored archer
 *       (archer-medieval v2_bowextended). task-126 landed this block so a unit that
 *       spawns as EUnitType::Archers stops drawing as a knight; task-128 REPLACED its
 *       contents, because all six of task-126's looks were steel-helmeted knights
 *       holding a bow and an archer therefore still read as a spearman at a 56px cell.
 *       Thirteen is the ceiling on this block, not an arbitrary count — see the
 *       four-bit note below.
 *   Row = Variant*2 + WalkFrame, same formula as Enemy, different variant count.
 *
 * WHICH variant an entity wears is not this namespace's business — it comes in on bits
 * 21-24 of the render int32 (SwarmRenderPack::Variant), chosen by a display-weight table
 * — Swarm.BroodVariantWeights for the enemy side, Swarm.TeamVariantWeights (spearmen) or
 * Swarm.ArcherVariantWeights (archers) for the team side. TeamBit picks the side and the
 * squad byte's unit type picks the team sub-table. A look is retired by setting its
 * weight to 0, which is why every variant on a side is packed rather than a chosen
 * subset: retiring one costs no repack.
 *
 * The variant field is FOUR BITS and stops at 15, so twenty-four team looks do not fit a
 * flat index. They do not have to: the field carries the WITHIN-BLOCK index (0-10 for
 * spearmen, 0-12 for archers) and SwarmRenderActor.cpp's pack loop adds
 * Team::ArcherVariantBase when the entity is an archer. The atlas can therefore grow
 * past sixteen looks per side without ever repacking the render int32.
 *
 * NOTE walk0 and walk1 are IDENTICAL images for every row today — no source character has
 * a generated walk. The axis is kept anyway (double the rows either sheet strictly needs)
 * because collapsing it would force a SECOND layout change the day an animate_character
 * walk lands, and this comment block is the thing that has to be right in three places
 * at once, now times two.
 *
 * Column order is south-first and counter-clockwise on screen, matching the direction
 * order PixelLab returns rotations in and the frame_map in both request files. Changing
 * it means changing all of them.
 */
namespace SwarmSheet
{
	/** Shared by every sheet in this namespace — eight-direction facing never varies. */
	constexpr int32 Columns = 8;

	/**
	 * ENEMY atlas (T_Enemy_2bit) — the nine brood-ooze looks, no retinue. Independent
	 * of Team below by design: adding a tenth enemy touches only these rows, this
	 * texture, this emitter's Sub UV, and never the team's.
	 */
	namespace Enemy
	{
		constexpr int32 Variants = 9;
		constexpr int32 Rows = Variants * 2;	// 18

		/**
		 * Sheet cell for an anim byte, an already-resolved direction column, and an
		 * enemy variant. See the doc comment on Team::CellFor below for why the column
		 * is passed in rather than decoded here — same reasoning, both sheets.
		 */
		FORCEINLINE int32 CellFor(uint8 Bits, int32 DirColumn, int32 Variant = 0)
		{
			const int32 Col = FMath::Clamp(DirColumn, 0, Columns - 1);
			const int32 Frame1 = (Bits & SwarmAnim::FrameBit) != 0 ? 1 : 0;
			const int32 Row = FMath::Clamp(Variant, 0, Variants - 1) * 2 + Frame1;
			return Col + Row * Columns;
		}
	}

	/**
	 * TEAM atlas (T_Team_2bit) — the retinue base look (variant 0), the ten judged knight
	 * keeps task-085 landed (variants 1-10), and the thirteen archer looks task-128 put in
	 * the block task-126 opened (variants 11-23). Independent of Enemy above: this grid
	 * never moves because the enemy roster churns.
	 */
	namespace Team
	{
		// Two sub-tables, one grid. SpearVariants is the cap on Swarm.TeamVariantWeights
		// (and the index task-095's stat rows are keyed on, so it must not drift);
		// ArcherVariants is the cap on Swarm.ArcherVariantWeights. ArcherVariantBase is
		// the row-pair offset the render bridge adds for an archer — the only place the
		// two blocks ever share an index space, because the 4-bit render field cannot
		// hold a flat 0-16.
		constexpr int32 SpearVariants = 11;
		constexpr int32 ArcherVariants = 13;
		constexpr int32 ArcherVariantBase = SpearVariants;	// 11
		constexpr int32 Variants = SpearVariants + ArcherVariants;	// 24
		constexpr int32 Rows = Variants * 2;	// 48

		/**
		 * Sheet cell for an anim byte, an already-resolved direction column, and a team
		 * variant.
		 *
		 * The column is passed in rather than decoded from the anim byte because facing
		 * is VIEW-relative and the anim byte is not: the same unit is column 0 to the
		 * main camera and some other column to the Unit Cam, whose yaw differs. Each
		 * consumer resolves its own column via SwarmFacing::ColumnFor and hands it here,
		 * so the two cannot drift on what a row means even though they disagree about
		 * direction.
		 *
		 * Variant defaults to 0 (the retinue base look) so a caller that does not care
		 * about team variety keeps compiling and simply draws every soldier as variant 0.
		 */
		FORCEINLINE int32 CellFor(uint8 Bits, int32 DirColumn, int32 Variant = 0)
		{
			const int32 Col = FMath::Clamp(DirColumn, 0, Columns - 1);
			const int32 Frame1 = (Bits & SwarmAnim::FrameBit) != 0 ? 1 : 0;
			const int32 Row = FMath::Clamp(Variant, 0, Variants - 1) * 2 + Frame1;
			return Col + Row * Columns;
		}
	}

}

/**
 * World facing, quantised to 32 steps, and the view-relative column it becomes.
 *
 * Why 32 and not 8: the sheet has eight columns, but the SIM must not be the thing that
 * knows that. Camera yaw is a live CVar (Kindled.Cam.Yaw) that spins the map under the
 * player, so a facing baked to eight screen-relative steps in the sim would snap visibly
 * when the view rotated. Storing a WORLD angle at 4x the sheet's resolution lets the view
 * apply its own yaw and quantise independently, and leaves headroom for a 16-column sheet
 * without touching the sim or the packing.
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
 * What FSwarmAnimFragment::SquadId actually packs (docs/design/squad-group-system.md §1.3,
 * §8). A soldier's unit membership AND type are both assigned exactly once, at recruit
 * time (USwarmSubsystem::AssignRecruit), and are permanent for that soldier's lifetime —
 * neither is ever re-derived from a live, repackable quantity like a formation slot index.
 *
 * Packed into the SAME byte the field already was, rather than a new field, for the exact
 * reason squad-group-system.md §8 gives for avoiding a class-layout change on a hot-path
 * fragment: only 3 bits were ever live (0..MaxSquads-1 == 0..7), so a 4th bit for Type
 * costs nothing new to store. §8's OWN proposal — deriving Type by comparing SquadId
 * against a live-recomputed range boundary (Units(Spearmen), recomputed every repack from
 * §4.1's ceil(pool/80) formula) — does not survive contact with §1.5's "type never changes
 * through combat": that boundary is NOT monotonic (Spearmen casualties shrink
 * Units(Spearmen)), so a soldier sitting near the boundary would flip type the instant
 * enough of its own type-mates died. Tagging the bit directly at assignment time is the
 * same "no new field" cost with none of that hazard — see the task-046 handback for the
 * full reasoning; this is a deliberate, disclosed deviation from §8's literal mechanism,
 * not a rejection of its goal. Declared here, ahead of SwarmRenderPack below, because that
 * namespace's own squad bits reuse these same masks.
 */
namespace SwarmSquad
{
	constexpr uint8 IdMask = 0x07;   // bits 0-2: which of MaxSquads (8) unit slots
	constexpr uint8 TypeShift = 3;
	constexpr uint8 TypeBit = 1 << TypeShift; // bit 3: 0 = Spearmen, 1 = Archers

	FORCEINLINE uint8 Pack(uint8 UnitIndex, EUnitType Type)
	{
		return (UnitIndex & IdMask) | (Type == EUnitType::Archers ? TypeBit : 0);
	}
	FORCEINLINE uint8 UnitIndex(uint8 Packed) { return Packed & IdMask; }
	FORCEINLINE EUnitType UnitType(uint8 Packed) { return (Packed & TypeBit) ? EUnitType::Archers : EUnitType::Spearmen; }
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
 *   bits 17-20 squad byte (SwarmSquad) — unit index (0-2) + type (bit 3), retinue only
 *   bits 21-24 variant: which atlas look this body wears, as a WITHIN-SUB-TABLE index.
 *              Shared field for all three tables -- TeamBit (already in bits 0-7) picks
 *              the side and, on the team side, the squad byte's unit type picks the
 *              sub-table: SwarmSheet::Enemy (0-8), team spearmen (0-10), team archers
 *              (0-5). task-126 kept this four bits wide by putting the archer ROW OFFSET
 *              in the render bridge instead of in this field -- see SwarmSheet::Team.
 *   bits 25-31 free
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

	// Bits 17-20: the squad byte (SwarmSquad::Pack — unit index + type), so a consumer of
	// the render buffer (the Unit Cam) can finally tell WHICH unit and WHICH type a body is
	// without touching SquadStanding or re-deriving anything from the formation slot. Same
	// "free bits in an already-existing TArray<int32>" reasoning as Size/Facing above — this
	// was the exact gap docs/design/squad-group-system.md §1.3 and UnitCamDirector.cpp's own
	// comment named ("bits 17-31 are free but nothing writes a squad id into them yet").
	constexpr int32 SquadShift = 17;
	constexpr int32 SquadMask = SwarmSquad::IdMask | SwarmSquad::TypeBit; // 4 bits, 0-15

	// Bits 21-24: which atlas look this body wears -- SwarmSheet::Enemy (0-8), team
	// spearmen (0-10) or team archers (0-12), picked by TeamBit plus the squad byte's unit
	// type (task-085 split one variant field into two independent tables sharing the same
	// bits, since the two sides are never both live for a given entity; task-126 added a
	// third table the same way, WITHIN-BLOCK, with the +11 archer row offset applied in
	// SwarmRenderActor.cpp so twenty-four team looks still index through four bits).
	// Same "free bits in an array that already exists" reasoning as Size/Facing/Squad above,
	// and the reason this feature needed no new fragment field, no new array, and no
	// class-layout change — which matters more than usual here, because a layout change on
	// this module cannot be applied by Live Coding at all (it reports success and then
	// crashes the next PIE, see the ponytail: note in SwarmRenderActor.cpp).
	constexpr int32 VariantShift = 21;
	constexpr int32 VariantMask = 0xF;						// 4 bits, 0-15: 11 spear, 6 archer, 9 enemy

	FORCEINLINE int32 Pack(uint8 AnimBits, int32 SizeBucket, int32 FacingIndex = 0, uint8 SquadByte = 0,
		int32 VariantIndex = 0)
	{
		return (int32)AnimBits
			| ((SizeBucket & SizeIndexMask) << SizeShift)
			| ((FacingIndex & FacingIndexMask) << FacingShift)
			| (((int32)SquadByte & SquadMask) << SquadShift)
			| ((VariantIndex & VariantMask) << VariantShift);
	}

	FORCEINLINE uint8 Anim(int32 Packed) { return (uint8)(Packed & AnimMask); }
	FORCEINLINE int32 SizeBucket(int32 Packed) { return (Packed >> SizeShift) & SizeIndexMask; }
	FORCEINLINE int32 Facing(int32 Packed) { return (Packed >> FacingShift) & FacingIndexMask; }
	FORCEINLINE uint8 Squad(int32 Packed) { return (uint8)((Packed >> SquadShift) & SquadMask); }
	FORCEINLINE int32 Variant(int32 Packed) { return (Packed >> VariantShift) & VariantMask; }

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

	/**
	 * Which atlas look an entity wears, from the same walk-cycle phase, mapped through a
	 * CUMULATIVE display-weight table.
	 *
	 * Shared by all three tables — the caller passes the brood table
	 * (Swarm.BroodVariantWeights, 9 entries) for enemies, the spearman table
	 * (Swarm.TeamVariantWeights, 11 entries) for retinue/knights, or the archer table
	 * (Swarm.ArcherVariantWeights, 13 entries, task-126/128); nothing here cares which, it
	 * just walks whatever Cum/Num it is handed. What it returns is always a WITHIN-TABLE
	 * index — the archer row offset is the render bridge's business, not this function's.
	 *
	 * Cumulative rather than uniform because the mix is the point: the owner's ask was
	 * "they will be match to display weight", so a common base look and a couple of rare
	 * ones is the shipped default and a look is RETIRED by giving it weight 0. The table
	 * itself is not baked in here — the caller passes a live one — for exactly the reason
	 * SizeScale takes its amplitude as an argument: the weights CVar then reskins a
	 * standing horde on the frame you change it, with no respawn, because the sim never
	 * stored a variant in the first place.
	 *
	 * A DIFFERENT irrational multiplier from BucketFromPhase, and that is load-bearing:
	 * sharing 0.618 would make variant a pure function of size bucket, so the largest body
	 * would always be the same skin and the variety would read as one big and one small
	 * creature instead of spread across the whole table.
	 *
	 * Cum must be non-decreasing with Cum[Num-1] == total weight. Num <= 0, or an all-zero
	 * table, falls back to variant 0 rather than dividing by nothing.
	 */
	FORCEINLINE int32 VariantFromPhase(float Phase, const int32* Cum, int32 Num)
	{
		if (Num <= 0 || Cum[Num - 1] <= 0)
		{
			return 0;
		}
		const float U = FMath::Frac(Phase * 0.3819660887f);	// 1 - golden ratio conjugate
		const int32 Roll = FMath::Min((int32)(U * (float)Cum[Num - 1]), Cum[Num - 1] - 1);
		// Linear scan: Num is 6, 9 or 11.
		for (int32 i = 0; i < Num; ++i)
		{
			if (Roll < Cum[i])
			{
				return i;
			}
		}
		return Num - 1;
	}

	/**
	 * The look this body ACTUALLY wears — an ASSIGNED one if its unit has adapted
	 * (docs/design/adaptation.md), otherwise the phase roll above.
	 *
	 * This function is the whole of adaptation.md §6 item 4 ("a unit's look cannot be
	 * assigned — the hardest one"). The problem was never storage size: it was that the
	 * variant was recomputed from spawn phase in FIVE places (spawn HP, the grid publish,
	 * both halves of the formation repack, the render push, and the combat row), so
	 * "this soldier adapted into look X" had five independent things to convince. Every
	 * one of them now routes through here, which is what makes a single assignment move
	 * a unit's sprite, its detachment, its stats and its reach together — the same
	 * can't-disagree guarantee task-095 built between a knight's look and its fight.
	 *
	 * Assignment lives on the SUBSYSTEM, per unit (USwarmSubsystem::SetSquadRung), not on
	 * a fragment: adaptation.md §6 is explicit that a branch is a command handle and a
	 * rung is a look-and-stat move inside one, so there are eight of these numbers, not
	 * thirty thousand. That also keeps FSwarmAnimFragment's layout untouched — the one
	 * thing Live Coding cannot apply to this module at all (see the note above
	 * FSwarmStrikeFragment).
	 *
	 * AssignedVariant is a WITHIN-BLOCK index, same space VariantFromPhase returns and
	 * the same space the render bridge adds SwarmSheet::Team::ArcherVariantBase to — the
	 * flat 0-23 atlas index the ladder data speaks is converted once, at the assignment
	 * boundary. Negative means "not adapted", which is the shipped behaviour verbatim.
	 */
	FORCEINLINE int32 VariantFor(int32 AssignedVariant, float Phase, const int32* Cum, int32 Num)
	{
		return AssignedVariant >= 0
			? FMath::Min(AssignedVariant, VariantMask)	// 4-bit render field; assignment already validated
			: VariantFromPhase(Phase, Cum, Num);
	}
}

USTRUCT()
struct FSwarmAnimFragment : public FMassFragment
{
	GENERATED_BODY()

	uint8 Bits = 0;

	// Retinue only: unit membership (bits 0-2) + type (bit 3) — see SwarmSquad above for the
	// packing. Not part of the anim byte sent to Niagara — the SubUV bridge reads Bits only.
	// Assigned once at recruit time and never rewritten (SwarmSquad's own doc comment).
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
	 *
	 * Under the Block shape this counts WITHIN the unit's detachment (see GroupIndex),
	 * not across the whole type — every detachment starts again at 0.
	 */
	int32 SlotIndex = 0;

	/**
	 * Which DETACHMENT this soldier stands in — one per unique sprite WITHIN this
	 * soldier's own command unit (SquadId), ranked densely so a look nobody in that unit
	 * wears costs no ground, and capped at Columns * GroupDepthCap soldiers deep (a look
	 * that outgrows the cap opens a sibling detachment instead of deepening forever). The
	 * repack writes it; SwarmFormation::SlotOffset turns it into the block's lateral band.
	 * A fresh command unit's detachments always start a new depth row, so a detachment
	 * never straddles two different units.
	 *
	 * Zero for every unit under the non-Block shapes, which have no lateral band to give
	 * a detachment (URetinueFormationProcessor::Execute keeps SlotIndex type-wide dense
	 * in that case, so those shapes are untouched by any of this).
	 *
	 * A look is NOT a command handle — command is by type, docs/design/DIRECTION-2026-07-31.md
	 * D14. This is where a soldier stands, nothing more.
	 */
	int32 GroupIndex = 0;

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
