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

	// --- attractor -------------------------------------------------------
	void SetAttractor(const FVector& InLocation) { Attractor = InLocation; }
	FVector GetAttractor() const { return Attractor; }

	// --- hero ------------------------------------------------------------
	void SetHeroAlive(bool bInAlive) { bHeroAlive = bInAlive; }
	bool IsHeroAlive() const { return bHeroAlive; }

	float GetHeroHP() const { return HeroHP; }
	float GetHeroMaxHP() const { return SwarmCombatTuning::HeroMaxHP; }
	void SetHeroHP(float InHP) { HeroHP = FMath::Clamp(InHP, 0.f, SwarmCombatTuning::HeroMaxHP); }

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
	};

	void ResetGrid(int32 ExpectedCount)
	{
		Grid.Reset();
		Grid.Reserve(ExpectedCount / 4 + 16);
	}

	void AddToGrid(const FVector& Location, bool bRetinue)
	{
		const FIntPoint Cell = ToCell(Location);
		Grid.FindOrAdd(Cell).Add(FGridEntry{ Location, bRetinue });
	}

	static FIntPoint ToCell(const FVector& Location)
	{
		return FIntPoint(
			FMath::FloorToInt(Location.X / GridCellSize),
			FMath::FloorToInt(Location.Y / GridCellSize));
	}

	int32 GetGridCellCount() const { return Grid.Num(); }

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
	}

	void PushRenderEntry(const FVector& Location, uint8 AnimBits)
	{
		RenderPositions.Add(Location);
		RenderAnimBits.Add(static_cast<int32>(AnimBits));
		((AnimBits & SwarmAnim::TeamBit) != 0 ? AliveRetinue : AliveBrood)++;
	}

	const TArray<FVector>& GetRenderPositions() const { return RenderPositions; }
	const TArray<int32>& GetRenderAnimBits() const { return RenderAnimBits; }

	// --- live counts (valid from the end of the integrate pass) -----------
	int32 GetAliveRetinue() const { return AliveRetinue; }
	int32 GetAliveBrood() const { return AliveBrood; }
	int32 GetLeashBrokenCount() const { return LeashBroken; }
	void AddLeashBroken(int32 Count) { LeashBroken += Count; }

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
		HeroHP = SwarmCombatTuning::HeroMaxHP;
		PendingHeroDamage = 0.f;
		bHeroAlive = true;
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

	float HeroHP = SwarmCombatTuning::HeroMaxHP;
	float PendingHeroDamage = 0.f;
	bool bHeroAlive = true;

	TMap<FIntPoint, TArray<FGridEntry>> Grid;
	TArray<FVector> RenderPositions;
	TArray<int32> RenderAnimBits;
	TArray<FMassEntityHandle> AllEntities;

	int32 AliveRetinue = 0;
	int32 AliveBrood = 0;
	int32 LeashBroken = 0;
	int64 HeroContacts = 0;
	int32 HeroContactsThisFrame = 0;

	double TotalDamageToRetinue = 0.0;
	double TotalDamageToBrood = 0.0;
	double TotalHeroDamage = 0.0;
	int64 TotalKilledRetinue = 0;
	int64 TotalKilledBrood = 0;
};
