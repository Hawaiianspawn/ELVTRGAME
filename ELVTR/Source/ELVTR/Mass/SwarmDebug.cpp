#include "SwarmDebug.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "SwarmFragments.h"
#include "SwarmSubsystem.h"

namespace
{
	struct FSpacingStats
	{
		int32 Count = 0;
		float Min = 0.f;
		float P05 = 0.f;
		float Median = 0.f;
		float Mean = 0.f;
		float Max = 0.f;
		int32 CloserThan60 = 0;	// inside the separation radius
	};

	/**
	 * O(n^2) nearest-neighbour. Debug-only and capped, so the simple thing is
	 * the right thing — a grid query would silently miss neighbours further
	 * than one cell, which is exactly the case we need to detect.
	 */
	bool ComputeSpacing(const TArray<FVector>& Points, FSpacingStats& Out)
	{
		const int32 Num = Points.Num();
		if (Num < 2)
		{
			Out.Count = Num;
			return false;
		}

		TArray<float> Nearest;
		Nearest.Reserve(Num);

		for (int32 i = 0; i < Num; ++i)
		{
			float BestSq = TNumericLimits<float>::Max();
			for (int32 j = 0; j < Num; ++j)
			{
				if (i == j)
				{
					continue;
				}
				BestSq = FMath::Min(BestSq, static_cast<float>(FVector::DistSquared2D(Points[i], Points[j])));
			}
			Nearest.Add(FMath::Sqrt(BestSq));
		}

		Nearest.Sort();

		double Sum = 0.0;
		for (const float D : Nearest)
		{
			Sum += D;
		}

		Out.Count = Num;
		Out.Min = Nearest[0];
		Out.P05 = Nearest[FMath::Clamp(FMath::FloorToInt(Num * 0.05f), 0, Num - 1)];
		Out.Median = Nearest[Num / 2];
		Out.Mean = static_cast<float>(Sum / Num);
		Out.Max = Nearest.Last();

		for (const float D : Nearest)
		{
			if (D < 60.f)
			{
				++Out.CloserThan60;
			}
		}
		return true;
	}

	void ReportTeam(const TArray<FVector>& Points, const TCHAR* Label)
	{
		FSpacingStats Stats;
		if (!ComputeSpacing(Points, Stats))
		{
			UE_LOG(LogTemp, Display, TEXT("SwarmSpacing: %s — %d unit(s), nothing to measure"), Label, Stats.Count);
			return;
		}

		// Bounding box too: spacing alone can't distinguish "units are spread out
		// correctly" from "the camera is framed far too wide to see it".
		FBox Bounds(ForceInit);
		for (const FVector& P : Points)
		{
			Bounds += P;
		}
		const FVector Extent = Bounds.GetSize();

		UE_LOG(LogTemp, Display,
			TEXT("SwarmSpacing: %s n=%d  nearest-neighbour min=%.0f p05=%.0f median=%.0f mean=%.0f max=%.0f  (%d within 60uu)  bbox=%.0f x %.0f uu"),
			Label, Stats.Count, Stats.Min, Stats.P05, Stats.Median, Stats.Mean, Stats.Max, Stats.CloserThan60,
			Extent.X, Extent.Y);
	}

	FAutoConsoleCommandWithWorldAndArgs GSpacingReportCmd(
		TEXT("Swarm.SpacingReport"),
		TEXT("Log nearest-neighbour spacing stats per team, measured on the render buffers."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SwarmDebug::LogSpacingReport(World);
			}));
}

namespace SwarmDebug
{
	void LogSpacingReport(UWorld* World)
	{
		const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!Swarm)
		{
			return;
		}

		const TArray<FVector>& Positions = Swarm->GetRenderPositions();
		const TArray<int32>& AnimBits = Swarm->GetRenderAnimBits();
		if (Positions.Num() != AnimBits.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("SwarmSpacing: buffer mismatch (%d positions, %d anim bits)"),
				Positions.Num(), AnimBits.Num());
			return;
		}

		TArray<FVector> Retinue;
		TArray<FVector> Brood;
		for (int32 i = 0; i < Positions.Num(); ++i)
		{
			((AnimBits[i] & SwarmAnim::TeamBit) != 0 ? Retinue : Brood).Add(Positions[i]);
		}

		// Distinct positions matter more than count here: if the renderer shows
		// one unit but the sim has 120 spread out, these numbers prove it.
		TSet<FVector> UniqueRetinue(Retinue);
		UE_LOG(LogTemp, Display, TEXT("SwarmSpacing: --- report --- (%d distinct retinue positions of %d)"),
			UniqueRetinue.Num(), Retinue.Num());

		ReportTeam(Retinue, TEXT("retinue"));
		ReportTeam(Brood, TEXT("brood  "));
	}
}
