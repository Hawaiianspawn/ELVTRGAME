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
	static constexpr float GridCellSize = 200.f;

	// --- cosmetic squads (muster UI) -------------------------------------
	static constexpr int32 MaxSquads = 8;        // hard cap; slots beyond fold into the last squad
	static constexpr int32 SquadTargetSize = 20; // formation slots per squad

	/** Squad a formation slot belongs to — contiguous chunks, capped at MaxSquads-1. */
	static int32 SquadIdForSlot(int32 Slot)
	{
		return FMath::Clamp(Slot / SquadTargetSize, 0, MaxSquads - 1);
	}

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

	// --- stance ----------------------------------------------------------
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
	}
	FVector GetStanceAnchor() const { return StanceAnchor; }

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
	};

	void ResetGrid(int32 ExpectedCount)
	{
		Grid.Reset();
		Grid.Reserve(ExpectedCount / 4 + 16);
	}

	void AddToGrid(const FVector& Location, bool bRetinue, bool bStriking = false,
		float StrikeReachSq = 0.f, int32 TargetsPerHit = 0)
	{
		const FIntPoint Cell = ToCell(Location);
		Grid.FindOrAdd(Cell).Add(FGridEntry{ Location, bRetinue, bStriking, StrikeReachSq, TargetsPerHit });
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
	 */
	void PushRenderEntry(const FVector& Location, uint8 AnimBits, uint8 SquadId = 0, int32 SizeBucket = 0,
		int32 FacingIndex = 0)
	{
		RenderPositions.Add(Location);
		RenderAnimBits.Add(SwarmRenderPack::Pack(AnimBits, SizeBucket, FacingIndex));
		if ((AnimBits & SwarmAnim::TeamBit) != 0)
		{
			++AliveRetinue;
			SquadStanding[FMath::Min<int32>(SquadId, MaxSquads - 1)]++;
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

	// --- live counts (valid from the end of the integrate pass) -----------
	int32 GetAliveRetinue() const { return AliveRetinue; }
	int32 GetAliveBrood() const { return AliveBrood; }
	int32 GetLeashBrokenCount() const { return LeashBroken; }
	void AddLeashBroken(int32 Count) { LeashBroken += Count; }

	// --- formation repack --------------------------------------------------
	// The formation slot index has to stay DENSE for the shape to mean anything: with a
	// baked index, casualties punch holes through a block and its outline never shrinks,
	// so the formation stops reporting how much army is left. Repacking is O(N log N),
	// so it is gated on the standing count actually having moved rather than run per
	// frame. AliveRetinue is last frame's count when the steering pass reads it, which is
	// exactly right — one frame of lag on re-forming a line is invisible.

	bool NeedsFormationRepack() const { return AliveRetinue != PackedRetinueCount; }
	void MarkFormationPacked() { PackedRetinueCount = AliveRetinue; }
	/** Force a repack next pass — for changes the count alone would not reveal. */
	void MarkFormationDirty() { PackedRetinueCount = -1; }

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
	int32 SquadStanding[MaxSquads] = {}; // live retinue count per cosmetic squad, refilled each frame
	TArray<FMassEntityHandle> AllEntities;

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
