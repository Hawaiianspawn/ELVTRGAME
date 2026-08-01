#pragma once

#include "CoreMinimal.h"
#include "Mass/SwarmCombat.h"
#include "KindledUITypes.generated.h"

/**
 * One squad's state, as the UI reads it. The UI binds to squad-level state only — never a
 * per-soldier loop (menu spec §5a: squad-as-entity). `Columns` = files; the muster grid
 * lays Size pips in Columns-wide rows, Standing of them lit.
 */
USTRUCT(BlueprintType)
struct FKindledSquad
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FText DisplayName = FText::FromString(TEXT("Shield"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0"))
	int32 Size = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0"))
	int32 Standing = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "1"))
	int32 Columns = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	ESwarmStance Stance = ESwarmStance::Follow;

	/** Wide variant (spans 2) for compacted large squads — e.g. 50-strong spearmen at 10 files. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bWide = false;
};
