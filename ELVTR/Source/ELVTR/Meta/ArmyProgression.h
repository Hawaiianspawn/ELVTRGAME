#pragma once

#include "CoreMinimal.h"

/**
 * Pure lifetime-kills -> army level curve (persistent army meta-progression).
 * Deliberately just this one function: ArmyLevel is a read-time fold over lifetime
 * kills, not a separately stored counter, so it can never drift out of sync with the
 * kills that earned it. Sqrt curve = cheap early levels, flattening growth later, no
 * tuning table to maintain.
 */
namespace ArmyProgression
{
	/**
	 * LifetimeKills only ever grows (see UArmyProgressionSubsystem::AddLifetimeKills),
	 * and this curve is monotonic non-decreasing in its input, so the level it returns
	 * can never fall out from under a player -- no separate clamp needed to guarantee
	 * that, the math already does.
	 */
	inline int32 ComputeLevelFromLifetimeKills(int64 LifetimeKills)
	{
		const int64 Kills = FMath::Max<int64>(0, LifetimeKills);
		return 1 + FMath::FloorToInt(FMath::Sqrt(static_cast<double>(Kills) / 50.0));
	}
}
