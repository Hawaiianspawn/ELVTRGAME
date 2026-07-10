#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntityTypes.h"
#include "SwarmSubsystem.generated.h"

/**
 * Shared state for the Spike 1 swarm:
 *  - attractor (hero) position that brood seeks / retinue orbits
 *  - a uniform spatial grid rebuilt each frame (separation + contact queries)
 *  - packed render buffers the Niagara bridge reads (positions + anim bits)
 *  - benchmark counters (entity count, hero contacts this session)
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

	// --- spatial grid (rebuilt by USwarmGridBuildProcessor) ---------------
	struct FGridEntry
	{
		FVector Location;
	};

	void ResetGrid(int32 ExpectedCount)
	{
		Grid.Reset();
		Grid.Reserve(ExpectedCount / 4 + 16);
	}

	void AddToGrid(const FVector& Location)
	{
		const FIntPoint Cell = ToCell(Location);
		Grid.FindOrAdd(Cell).Add(FGridEntry{ Location });
	}

	static FIntPoint ToCell(const FVector& Location)
	{
		return FIntPoint(
			FMath::FloorToInt(Location.X / GridCellSize),
			FMath::FloorToInt(Location.Y / GridCellSize));
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
	}

	void PushRenderEntry(const FVector& Location, uint8 AnimBits)
	{
		RenderPositions.Add(Location);
		RenderAnimBits.Add(static_cast<int32>(AnimBits));
	}

	const TArray<FVector>& GetRenderPositions() const { return RenderPositions; }
	const TArray<int32>& GetRenderAnimBits() const { return RenderAnimBits; }

	// --- bookkeeping -------------------------------------------------------
	void TrackSpawned(const TArray<FMassEntityHandle>& Handles) { AllEntities.Append(Handles); }
	TArray<FMassEntityHandle>& GetTrackedEntities() { return AllEntities; }

	void AddHeroContacts(int32 Count) { HeroContacts += Count; }
	int64 GetHeroContacts() const { return HeroContacts; }

private:
	FVector Attractor = FVector::ZeroVector;
	TMap<FIntPoint, TArray<FGridEntry>> Grid;
	TArray<FVector> RenderPositions;
	TArray<int32> RenderAnimBits;
	TArray<FMassEntityHandle> AllEntities;
	int64 HeroContacts = 0;
};
