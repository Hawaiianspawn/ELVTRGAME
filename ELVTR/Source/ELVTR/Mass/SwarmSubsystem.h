#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Mass/EntityHandle.h"
#include "SwarmCombat.h"
#include "SwarmFragments.h" // SwarmAnim::TeamBit, used to bucket the live counts
#include "SwarmStats.h"
#include "SwarmSubsystem.generated.h"

/**
 * Shared state for the swarm:
 *  - attractor (hero) position that brood seeks / retinue orbits
 *  - hero HP + the retinue's stance intent (Follow/Charge/Hold/Rally)
 *  - a uniform spatial grid rebuilt each frame (separation, targeting, melee)
 *  - packed render buffers the Niagara bridge reads (positions + anim bits)
 *  - live team counts + benchmark counters
 *
 * Deliberately simple. If profiling says the grid or the buffer packing is the
 * bottleneck, replace with measurement in hand — not before.
 */
UCLASS()
class USwarmSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * World-space side length of one spatial-hash bucket. QueryNeighbors walks a 3x3
	 * neighbourhood around an entity's own cell, so the true search radius this buys is
	 * GridCellSize * 3 — 750uu at this value. Raised from 200 (task-052) specifically so
	 * Swarm.BroodAggroRange / a future ranged-combat EngageRange of 750uu (squad-group-
	 * system.md §2.2) is physically reachable; at 200 the same 3x3 walk topped out at
	 * 600uu no matter what any CVar asked for. Nothing else in the codebase derives a
	 * value from this constant except FIntPoint bucketing in ToCell() below — confirmed
	 * by grep before this change. Cost tradeoff (fewer, denser buckets = cheaper TMap
	 * overhead but more entries walked per QueryNeighbors call) measured in
	 * docs/perf/grid-cell-size.md; re-check that doc before raising this again.
	 */
	static constexpr float GridCellSize = 250.f;

	// --- typed units (muster UI + command layer) --------------------------
	// docs/design/squad-group-system.md §1.0: "squad" and "unit" are the same thing from
	// here on — SquadId/MaxSquads/squads.json all keep their existing names (a terminology
	// merge, not a code rename) but now name a TYPED thing (§1.1: Spearmen, Archers).
	static constexpr int32 MaxSquads = 8;        // hard cap; the shared command-handle budget
	static constexpr int32 SquadTargetSize = 20; // legacy per-unit formation-slot reference size

	/**
	 * Per-type legibility ceiling (§4.1's `squad_size_legibility_ceiling`, docs/data/
	 * squads.json). A type's derived unit COUNT is ceil(Pool(type) / this), recomputed at
	 * every recruit and every formation repack — see AssignRecruit and §4.1's formula.
	 */
	static constexpr int32 TypeLegibilityCeiling = 80;

	/**
	 * SquadIdForSlot is GONE (docs/design/squad-group-system.md §1.3): deriving a unit id
	 * from the dense, repackable formation slot index is exactly the defect this task
	 * fixes — that index silently renumbers on any casualty anywhere, which used to be
	 * cosmetic and would not stay cosmetic once a unit owns a stance or a type. SquadId
	 * (now packed with type — see SwarmSquad in SwarmFragments.h) is assigned ONCE, at
	 * recruit time, by AssignRecruit below, and never re-derived from anything that moves.
	 */

	// --- attractor -------------------------------------------------------
	void SetAttractor(const FVector& InLocation) { Attractor = InLocation; }
	FVector GetAttractor() const { return Attractor; }

	// --- Unit Cam spell-cast focus (hookup for the flame-bearer casting) --
	// When the bearer casts a spell FROM the flame, the caster calls SetCastFocus with a world
	// point + an absolute end time; the Unit Cam camera manager pulls focus to it for a "focus
	// punch" until then. Absolute world seconds so the subsystem needs no tick of its own.
	void SetCastFocus(const FVector& Pos, double EndTimeSeconds) { CastFocusPos = Pos; CastFocusEndTime = EndTimeSeconds; }
	bool IsCastFocusActive(double NowSeconds) const { return NowSeconds < CastFocusEndTime; }
	FVector GetCastFocusPos() const { return CastFocusPos; }

	// --- HUD occlusion ----------------------------------------------------
	// Fraction of the viewport height the combat HUD covers along the BOTTOM edge, published
	// by UEmberkeepHud each tick. The hero camera reads it to bias itself so the bearer sits
	// in the middle of the ground you can still see, rather than the middle of the viewport —
	// the rectangle scales with the body count, so this moves under you and can't be a constant.
	void SetHudOccludedFraction(float InFraction) { HudOccludedFraction = FMath::Clamp(InFraction, 0.f, 0.9f); }
	float GetHudOccludedFraction() const { return HudOccludedFraction; }

	// --- hero ------------------------------------------------------------
	void SetHeroAlive(bool bInAlive) { bHeroAlive = bInAlive; }
	bool IsHeroAlive() const { return bHeroAlive; }

	/**
	 * The hero's blow lands this frame. Published by the pawn's own swing clock,
	 * read by the combat pass — so the player's hits land on the same discrete
	 * cadence as everyone else's rather than bleeding continuously. Without this the
	 * one attack the player actually feels would be the one with no impact.
	 */
	void SetHeroStriking(bool bInStriking) { bHeroStriking = bInStriking; }
	bool IsHeroStriking() const { return bHeroStriking; }

	float GetHeroHP() const { return HeroHP; }
	float GetHeroMaxHP() const { return SwarmCombatTuning::HeroMaxHP(); }
	void SetHeroHP(float InHP) { HeroHP = FMath::Clamp(InHP, 0.f, SwarmCombatTuning::HeroMaxHP()); }

	/** Written by the combat pass, consumed + cleared by the hero pawn each tick. */
	void AddPendingHeroDamage(float Damage)
	{
		PendingHeroDamage += Damage;
		TotalHeroDamage += Damage;
	}
	float ConsumePendingHeroDamage()
	{
		const float Damage = PendingHeroDamage;
		PendingHeroDamage = 0.f;
		return Damage;
	}

	// --- balance telemetry ------------------------------------------------
	// Monotonic run totals, never consumed. The telemetry subsystem samples them
	// by differencing, so any number of readers can watch without racing each
	// other over a consume-and-clear. Cost is a handful of adds per frame.

	/** Combat pass: HP actually removed this frame, bucketed by victim team. */
	void AddDamageDealt(double ToRetinue, double ToBrood)
	{
		TotalDamageToRetinue += ToRetinue;
		TotalDamageToBrood += ToBrood;
	}
	double GetTotalDamageToRetinue() const { return TotalDamageToRetinue; }
	double GetTotalDamageToBrood() const { return TotalDamageToBrood; }
	double GetTotalHeroDamage() const { return TotalHeroDamage; }

	/** Death pass: entities destroyed this frame, bucketed by team. */
	void AddKills(int32 Retinue, int32 Brood)
	{
		TotalKilledRetinue += Retinue;
		TotalKilledBrood += Brood;
	}
	int64 GetTotalKilledRetinue() const { return TotalKilledRetinue; }
	int64 GetTotalKilledBrood() const { return TotalKilledBrood; }

	// --- stance ------------------------------------------------------------
	// docs/design/squad-group-system.md §3: an order now targets an ADDRESS — "all units"
	// (default) or one named unit. GetStance/SetStance/GetStanceAnchor keep their EXACT
	// existing meaning and every existing call site (SpikeHeroPawn's four stance hotkeys,
	// EmberkeepHud, SwarmTelemetry, UnitCamProjector's Army View tint) is untouched — this
	// is the main way this task could regress today's only behavior, so nothing about the
	// "all units" path changed: SetStance still records the single last-commanded whole-
	// retinue order AND (new) fans it out to every per-unit slot below, so a soldier's own
	// per-unit lookup reads the identical value it would have read from the old single
	// global Stance/StanceAnchor when no unit has ever been individually addressed.
	ESwarmStance GetStance() const { return Stance; }

	/**
	 * Hold anchors where the retinue stood when the order was issued; Charge
	 * aims at a world point. Both are captured at issue time so the order is a
	 * one-shot intent, not a thing that drags behind the hero.
	 */
	void SetStance(ESwarmStance InStance, const FVector& WorldPoint)
	{
		Stance = InStance;
		StanceAnchor = WorldPoint;
		// "All" writes every slot, both types — unchanged from today's only behavior (§3).
		for (int32 i = 0; i < MaxSquads; ++i)
		{
			UnitStance[i] = InStance;
			UnitStanceAnchor[i] = WorldPoint;
		}
	}
	FVector GetStanceAnchor() const { return StanceAnchor; }

	/**
	 * Address ONE unit (0..MaxSquads-1) with an order, leaving every other unit's stance —
	 * and the global "all units" Stance/StanceAnchor above — untouched. New in this task;
	 * the real input surface (muster-card click / hotkey) is owned elsewhere (per
	 * UnitCamDirector.cpp's own precedent for CVar-as-placeholder-surface) — Swarm.
	 * UnitStance in SwarmCommands.cpp is the stand-in console command until that lands.
	 */
	void SetUnitStance(int32 UnitIndex, ESwarmStance InStance, const FVector& WorldPoint)
	{
		if (UnitIndex >= 0 && UnitIndex < MaxSquads)
		{
			UnitStance[UnitIndex] = InStance;
			UnitStanceAnchor[UnitIndex] = WorldPoint;
		}
	}
	ESwarmStance GetUnitStance(int32 UnitIndex) const
	{
		return (UnitIndex >= 0 && UnitIndex < MaxSquads) ? UnitStance[UnitIndex] : Stance;
	}
	FVector GetUnitStanceAnchor(int32 UnitIndex) const
	{
		return (UnitIndex >= 0 && UnitIndex < MaxSquads) ? UnitStanceAnchor[UnitIndex] : StanceAnchor;
	}

	// --- spatial grid (rebuilt by USwarmGridBuildProcessor) ---------------
	struct FGridEntry
	{
		FVector Location;
		bool bRetinue = false;

		/**
		 * This entry's blow lands THIS frame. Published by the grid build pass at the
		 * top of the frame, read by the combat pass a few groups later.
		 *
		 * This one bool is what lets discrete blows exist without breaking the
		 * victim-pull rule: a victim can tell which of its neighbours are actually
		 * connecting right now, so it still applies its own damage. Nothing ever
		 * reaches across entities to write.
		 */
		bool bStriking = false;

		/**
		 * Squared distance to this unit's Kth-nearest enemy, where K is
		 * Swarm.TargetsPerHit. A victim within this radius is a CANDIDATE for this
		 * entry's blow — necessary, but (fixed 2026-07-26) no longer sufficient by
		 * itself. See BlowsClaimed for why and how the real cap is enforced.
		 *
		 * This is a radius computed from LAST frame's neighbour set, published for
		 * THIS frame's victims to test against. Positions move between the two
		 * (steering, separation, knockback), so the number of things that fall inside
		 * a fixed radius one frame later is not reliably K — it can be more (several
		 * victims jostle into the same stale circle) or fewer (the true K nearest
		 * scattered back out). The radius alone therefore does NOT conserve damage;
		 * BlowsClaimed is what actually makes "a striker delivers exactly K blows"
		 * true regardless of how the frame's positions moved.
		 */
		float StrikeReachSq = 0.f;

		/**
		 * The other half of the cap StrikeReachSq alone can't provide: how many blows
		 * this entry's owner is allowed to hand out this swing (its own K —
		 * Swarm.RetinueTargetsPerHit or Swarm.BroodTargetsPerHit, snapshotted at grid
		 * build time), and how many it has actually handed out so far this frame.
		 *
		 * BlowsClaimed is `mutable` so a victim can increment it through the `const
		 * FGridEntry&` QueryNeighbors hands out — the entry itself is still logically
		 * read-only to everyone except this one shared counter. Every successful claim
		 * anywhere this frame (the general neighbour-loop path in
		 * USwarmCombatProcessor, and the hero's own direct check via
		 * FindOwnGridEntry) increments the SAME counter on the SAME entry, so once
		 * BlowsClaimed reaches TargetsPerHit no further victim — no matter how many
		 * still sit inside the stale StrikeReachSq radius — can claim this swing.
		 *
		 * Safe without synchronisation: the whole combat pass is one sequential
		 * ForEachEntityChunk (see USwarmCombatProcessor::Execute), so victims are
		 * visited one at a time, never in parallel — an ordinary read-modify-write,
		 * not a race. Which victim wins a contested last blow is arbitrary (whichever
		 * chunk/entity is iterated first this frame), not a meaningful unfairness over
		 * the course of a run.
		 */
		int32 TargetsPerHit = 0;
		mutable int32 BlowsClaimed = 0;

		/**
		 * How much ONE blow from this entry deals, snapshotted at grid-build time from its
		 * own team/type + DPS (SwarmCombatTuning). Replaces the old "one shared blow value
		 * per TEAM" assumption (Strikers-count * flat TeamBlow) now that retinue strikers
		 * aren't all one type — a Spearman and an Archer striking the same victim this frame
		 * must not deal the same damage. Read by every victim that claims this entry's blow,
		 * same as StrikeReachSq/TargetsPerHit above.
		 */
		float BlowDamage = 0.f;
	};

	void ResetGrid(int32 ExpectedCount)
	{
		Grid.Reset();
		Grid.Reserve(ExpectedCount / 4 + 16);
	}

	void AddToGrid(const FVector& Location, bool bRetinue, bool bStriking = false,
		float StrikeReachSq = 0.f, int32 TargetsPerHit = 0, float BlowDamage = 0.f)
	{
		const FIntPoint Cell = ToCell(Location);
		Grid.FindOrAdd(Cell).Add(FGridEntry{ Location, bRetinue, bStriking, StrikeReachSq, TargetsPerHit, 0, BlowDamage });
	}

	static FIntPoint ToCell(const FVector& Location)
	{
		return FIntPoint(
			FMath::FloorToInt(Location.X / GridCellSize),
			FMath::FloorToInt(Location.Y / GridCellSize));
	}

	int32 GetGridCellCount() const { return Grid.Num(); }

	/**
	 * Find an entity's own just-published grid entry by value match, so a brood's
	 * direct hero-strike check (SwarmCombatProcessors.cpp) can spend from the SAME
	 * BlowsClaimed budget that retinue victims draw from via QueryNeighbors, instead
	 * of bypassing the cap entirely. Matches on Location + bRetinue + bStriking +
	 * StrikeReachSq, all copied verbatim (no arithmetic) from the same fragment read
	 * the caller already did, so exact float equality is reliable here.
	 *
	 * Only called for the bounded set of brood inside Swarm.HeroMeleeRange — not the
	 * general per-pair melee hot path — so the bucket scan this does is cheap in
	 * practice. A same-frame, same-cell, exact-position tie between two entities in
	 * identical state would misattribute the claim to whichever is found first; the
	 * existing Swarm.MaxAttackersPerUnit safety cap still bounds how bad that could
	 * ever get, and two entities occupying the identical FVector is not a state combat
	 * positioning produces.
	 */
	FGridEntry* FindOwnGridEntry(const FVector& Location, bool bRetinue, bool bStriking, float StrikeReachSq)
	{
		if (TArray<FGridEntry>* Bucket = Grid.Find(ToCell(Location)))
		{
			for (FGridEntry& Entry : *Bucket)
			{
				if (Entry.bRetinue == bRetinue && Entry.bStriking == bStriking
					&& Entry.StrikeReachSq == StrikeReachSq && Entry.Location == Location)
				{
					return &Entry;
				}
			}
		}
		return nullptr;
	}

	/** Visit entries in the 3x3 cells around Location. */
	template <typename TFunc>
	void QueryNeighbors(const FVector& Location, TFunc&& Func) const
	{
		const FIntPoint Center = ToCell(Location);
		for (int32 dY = -1; dY <= 1; ++dY)
		{
			for (int32 dX = -1; dX <= 1; ++dX)
			{
				if (const TArray<FGridEntry>* Bucket = Grid.Find(Center + FIntPoint(dX, dY)))
				{
					// Total callback invocations per frame — the N x M number that
					// says whether a pass is slow from work or from bad bucketing.
					// Compiles out entirely when STATS=0.
					INC_DWORD_STAT_BY(STAT_SwarmNeighborVisits, Bucket->Num());

					for (const FGridEntry& Entry : *Bucket)
					{
						Func(Entry);
					}
				}
			}
		}
	}

	// --- render buffers (written by integrate, read by render bridge) -----
	void ResetRenderBuffers(int32 ExpectedCount)
	{
		RenderPositions.Reset(ExpectedCount);
		RenderAnimBits.Reset(ExpectedCount);
		AliveRetinue = 0;
		AliveBrood = 0;
		LeashBroken = 0;
		for (int32& S : SquadStanding) { S = 0; }
		for (FVector& C : SquadCentroidSum) { C = FVector::ZeroVector; }
		for (int32& P : AlivePoolByType) { P = 0; }
	}

	/**
	 * SizeBucket is a raw 0-15 per-entity roll, packed into the spare high bits of the
	 * anim int32 (SwarmRenderPack). The renderers turn it into a size multiplier; the
	 * amplitude is theirs, not ours.
	 *
	 * FacingIndex is the same deal one field over: a raw 0-31 WORLD facing step, not a
	 * sheet column. Which column that becomes depends on the yaw of whoever is looking
	 * (main camera vs. Unit Cam), so the conversion belongs to each renderer and the
	 * sim stays ignorant of the sheet — see SwarmFacing.
	 *
	 * SquadId is now the SwarmSquad-packed byte (unit index + type, see SwarmFragments.h).
	 * It rides into the render int32 too (SwarmRenderPack::Squad) — the piece
	 * docs/design/squad-group-system.md §1.3 and UnitCamDirector.cpp's own comment named
	 * as missing: a render-buffer consumer (the Unit Cam) can now decode a body's real
	 * unit AND type instead of approximating off the whole visible retinue.
	 */
	void PushRenderEntry(const FVector& Location, uint8 AnimBits, uint8 SquadId = 0, int32 SizeBucket = 0,
		int32 FacingIndex = 0)
	{
		RenderPositions.Add(Location);
		RenderAnimBits.Add(SwarmRenderPack::Pack(AnimBits, SizeBucket, FacingIndex, SquadId));
		if ((AnimBits & SwarmAnim::TeamBit) != 0)
		{
			++AliveRetinue;
			const int32 UnitIndex = FMath::Min<int32>(SwarmSquad::UnitIndex(SquadId), MaxSquads - 1);
			SquadStanding[UnitIndex]++;
			SquadCentroidSum[UnitIndex] += Location;
			AlivePoolByType[(int32)SwarmSquad::UnitType(SquadId)]++;
		}
		else
		{
			++AliveBrood;
		}
	}

	const TArray<FVector>& GetRenderPositions() const { return RenderPositions; }
	const TArray<int32>& GetRenderAnimBits() const { return RenderAnimBits; }

	/** Live count of standing retinue in squad Index (0..MaxSquads-1). Valid after integrate. */
	int32 GetSquadStanding(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? SquadStanding[Index] : 0;
	}

	/**
	 * Real per-unit centroid (docs/design/squad-group-system.md §1.2's SquadCentroidSum),
	 * accumulated on the SAME PushRenderEntry pass that already receives Location and
	 * SquadId per unit — no second O(N) walk. Zero vector for an empty/unclaimed unit;
	 * callers already gate on GetSquadStanding(Index) > 0 (BuildArmyView, UnitCamDirector).
	 * Valid after integrate, same lifetime as GetSquadStanding.
	 */
	FVector GetSquadCentroid(int32 Index) const
	{
		if (Index < 0 || Index >= MaxSquads || SquadStanding[Index] <= 0) { return FVector::ZeroVector; }
		return SquadCentroidSum[Index] / (float)SquadStanding[Index];
	}

	/** Which type unit Index (0..MaxSquads-1) is — valid only once AssignRecruit has claimed
	 *  it (IsSquadClaimed). Sticky: set once by AssignRecruit, never rewritten. */
	EUnitType GetSquadType(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? SquadType[Index] : EUnitType::Spearmen;
	}
	bool IsSquadClaimed(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) && SquadClaimed[Index];
	}

	// --- live counts (valid from the end of the integrate pass) -----------
	int32 GetAliveRetinue() const { return AliveRetinue; }
	int32 GetAliveBrood() const { return AliveBrood; }
	int32 GetLeashBrokenCount() const { return LeashBroken; }
	void AddLeashBroken(int32 Count) { LeashBroken += Count; }

	/** Live standing pool of one type, valid after integrate (§4.1's Pool(type)). */
	int32 GetAliveByType(EUnitType Type) const { return AlivePoolByType[(int32)Type]; }

	// --- formation repack --------------------------------------------------
	// The formation slot index has to stay DENSE for the shape to mean anything: with a
	// baked index, casualties punch holes through a block and its outline never shrinks,
	// so the formation stops reporting how much army is left. Repacking is O(N log N),
	// so it is gated on the standing count actually having moved rather than run per
	// frame. AliveRetinue is last frame's count when the steering pass reads it, which is
	// exactly right — one frame of lag on re-forming a line is invisible.
	//
	// TWO independent repacks, one per TYPE, not one retinue-wide sort and not eight
	// per-unit-of-8 sorts (docs/design/squad-group-system.md §1.2, §8: "a type's units
	// still share ONE dense index space that subdivides into unit-sized chunks... scoped
	// to the type's own pool instead of the whole retinue" / "TWO independent dense
	// repacks (one per type) instead of one retinue-wide repack"). A stable type skips its
	// repack while the other type reforms — cheaper in aggregate than one big sort, and
	// this is the actual unit of "cheaper" the spec asks for, not a per-unit-of-8 grain.

	bool NeedsFormationRepack() const { return AliveRetinue != PackedRetinueCount; }
	void MarkFormationPacked() { PackedRetinueCount = AliveRetinue; }
	/** Force a repack next pass — for changes the count alone would not reveal. */
	void MarkFormationDirty() { PackedRetinueCount = -1; }

	bool NeedsFormationRepack(EUnitType Type) const
	{
		return AlivePoolByType[(int32)Type] != PackedPoolByType[(int32)Type];
	}
	void MarkFormationPacked(EUnitType Type) { PackedPoolByType[(int32)Type] = AlivePoolByType[(int32)Type]; }

	// --- recruitment (docs/design/squad-group-system.md §1.4, §4.1) -------------------
	/**
	 * Assign a newly recruited soldier's permanent (unit, type) pair. Fill-lowest-first
	 * into the least-full EXISTING unit of Type; opens a new unit — Spearmen claiming the
	 * lowest free global id, Archers the highest (§4.1's "Spearmen claim first", made
	 * concrete as a topology: the two types grow the shared 0..MaxSquads-1 budget from
	 * opposite ends instead of needing to renumber to avoid colliding in the middle) —
	 * once §4.1's derived want (ceil(Pool/TypeLegibilityCeiling)) exceeds the type's
	 * current live unit count and a global slot remains. Falls back to folding into the
	 * type's own existing units (or, if the type owns nothing yet and the whole 8-slot
	 * budget is already claimed by the other type, unit 0) once the budget is exhausted —
	 * §4.2's own honestly-flagged breaking point, not a crash.
	 *
	 * Reads last frame's SquadStanding (the same one-frame lag the formation repack
	 * already tolerates) plus a caller-supplied count of how many of this type this same
	 * spawn batch has already produced, so a big batch (e.g. StartingRetinue) opens
	 * however many new units its own recruits actually justify instead of only ever
	 * reacting to last frame's snapshot.
	 */
	uint8 AssignRecruit(EUnitType Type, int32 AlreadyRecruitedThisBatch = 0)
	{
		int32 BestUnit = INDEX_NONE;
		int32 BestStanding = TNumericLimits<int32>::Max();
		int32 TypeUnitCount = 0;
		int32 TypePool = AlreadyRecruitedThisBatch;
		for (int32 i = 0; i < MaxSquads; ++i)
		{
			if (!SquadClaimed[i] || SquadType[i] != Type) { continue; }
			++TypeUnitCount;
			TypePool += SquadStanding[i];
			if (SquadStanding[i] < BestStanding) { BestStanding = SquadStanding[i]; BestUnit = i; }
		}

		// +1: as-if this recruit already counted, so the soldier that TIPS a unit over the
		// ceiling starts the new one instead of being asked to join the one it just filled.
		const int32 WantedUnits = FMath::Max(1, (TypePool + 1 + TypeLegibilityCeiling - 1) / TypeLegibilityCeiling);
		const bool bWantNewUnit = (TypeUnitCount == 0) || (WantedUnits > TypeUnitCount);

		if (bWantNewUnit)
		{
			int32 NewId = INDEX_NONE;
			if (Type == EUnitType::Spearmen)
			{
				for (int32 i = 0; i < MaxSquads; ++i) { if (!SquadClaimed[i]) { NewId = i; break; } }
			}
			else
			{
				for (int32 i = MaxSquads - 1; i >= 0; --i) { if (!SquadClaimed[i]) { NewId = i; break; } }
			}
			if (NewId != INDEX_NONE)
			{
				SquadClaimed[NewId] = true;
				SquadType[NewId] = Type;
				return SwarmSquad::Pack((uint8)NewId, Type);
			}
			// No free global slot anywhere — §4.2's own named breaking point (the 8-handle
			// budget is fully claimed by the two types combined). Fold into this type's own
			// units below rather than crash.
		}

		if (BestUnit == INDEX_NONE)
		{
			// This type owns nothing yet AND no free slot exists (the other type claimed
			// all 8) — degrade to unit 0 rather than lose the recruit. Same §4.2 edge.
			BestUnit = 0;
			if (!SquadClaimed[0]) { SquadClaimed[0] = true; SquadType[0] = Type; }
		}
		return SwarmSquad::Pack((uint8)BestUnit, Type);
	}

	// --- bookkeeping -------------------------------------------------------
	void TrackSpawned(const TArray<FMassEntityHandle>& Handles) { AllEntities.Append(Handles); }
	TArray<FMassEntityHandle>& GetTrackedEntities() { return AllEntities; }

	void AddHeroContacts(int32 Count)
	{
		HeroContacts += Count;
		HeroContactsThisFrame = Count;	// instantaneous: how mobbed he is *right now*
	}
	int64 GetHeroContacts() const { return HeroContacts; }
	int32 GetHeroContactsThisFrame() const { return HeroContactsThisFrame; }

	/** Reset everything except tracked handles (the Clear command owns those). */
	void ResetRunState()
	{
		HeroHP = SwarmCombatTuning::HeroMaxHP();
		PendingHeroDamage = 0.f;
		bHeroAlive = true;
		bHeroStriking = false;
		Stance = ESwarmStance::Follow;
		StanceAnchor = FVector::ZeroVector;
		for (int32 i = 0; i < MaxSquads; ++i)
		{
			UnitStance[i] = ESwarmStance::Follow;
			UnitStanceAnchor[i] = FVector::ZeroVector;
			SquadClaimed[i] = false;
			SquadType[i] = EUnitType::Spearmen;
		}
		for (int32& P : PackedPoolByType) { P = -1; }
		HeroContacts = 0;
		HeroContactsThisFrame = 0;
		TotalDamageToRetinue = 0.0;
		TotalDamageToBrood = 0.0;
		TotalHeroDamage = 0.0;
		TotalKilledRetinue = 0;
		TotalKilledBrood = 0;
	}

private:
	FVector Attractor = FVector::ZeroVector;
	FVector StanceAnchor = FVector::ZeroVector;
	ESwarmStance Stance = ESwarmStance::Follow;

	FVector CastFocusPos = FVector::ZeroVector; // Unit Cam spell-cast focus point
	double CastFocusEndTime = 0.0;              // absolute world seconds the focus lasts until

	float HudOccludedFraction = 0.f; // 0 until the HUD reports in, so no HUD = no camera bias

	float HeroHP = SwarmCombatTuning::HeroMaxHP();
	float PendingHeroDamage = 0.f;
	bool bHeroAlive = true;
	bool bHeroStriking = false;

	TMap<FIntPoint, TArray<FGridEntry>> Grid;
	TArray<FVector> RenderPositions;
	TArray<int32> RenderAnimBits;
	int32 SquadStanding[MaxSquads] = {}; // live retinue count per unit, refilled each frame
	FVector SquadCentroidSum[MaxSquads] = {}; // sum of member locations per unit, refilled each frame
	TArray<FMassEntityHandle> AllEntities;

	// --- typed-unit command layer (docs/design/squad-group-system.md §1) --------------
	bool SquadClaimed[MaxSquads] = {};                         // has AssignRecruit ever opened this unit id
	EUnitType SquadType[MaxSquads] = {};                        // sticky — set once by AssignRecruit
	ESwarmStance UnitStance[MaxSquads] = {};                    // per-unit order; "all" writes every slot
	FVector UnitStanceAnchor[MaxSquads] = {};
	int32 AlivePoolByType[NumUnitTypes] = {};                   // live standing per type, refilled each frame
	int32 PackedPoolByType[NumUnitTypes] = { -1, -1 };          // pool as of each type's last formation repack

	int32 AliveRetinue = 0;
	int32 AliveBrood = 0;
	int32 LeashBroken = 0;
	int32 PackedRetinueCount = -1;	// standing count as of the last formation repack
	int64 HeroContacts = 0;
	int32 HeroContactsThisFrame = 0;

	double TotalDamageToRetinue = 0.0;
	double TotalDamageToBrood = 0.0;
	double TotalHeroDamage = 0.0;
	int64 TotalKilledRetinue = 0;
	int64 TotalKilledBrood = 0;
};
