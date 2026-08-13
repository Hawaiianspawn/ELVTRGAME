#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ArmyProgressionSaveGame.generated.h"

/**
 * Everything meta-progression persists to disk. Just the one number -- ArmyLevel is
 * derived from it at read time (ArmyProgression::ComputeLevelFromLifetimeKills), so
 * there is nothing else here that could get out of sync with it.
 */
UCLASS()
class ELVTR_API UArmyProgressionSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int64 LifetimeKills = 0;
};
