#pragma once

#include "CoreMinimal.h"
#include "KindledUITypes.generated.h"

/**
 * Retinue stance (GDD §4). UI-facing mirror kept decoupled from the gameplay ESwarmStance
 * for the M1 mock prototype; M2 maps this to USwarmSubsystem::GetStance().
 */
UENUM(BlueprintType)
enum class EKindledStance : uint8
{
	Follow UMETA(DisplayName = "Follow"),
	Charge UMETA(DisplayName = "Charge"),
	Hold   UMETA(DisplayName = "Hold"),
	Rally  UMETA(DisplayName = "Rally")
};

inline FString KindledStanceToString(EKindledStance Stance)
{
	switch (Stance)
	{
		case EKindledStance::Charge: return TEXT("Charge");
		case EKindledStance::Hold:   return TEXT("Hold");
		case EKindledStance::Rally:  return TEXT("Rally");
		case EKindledStance::Follow:
		default:                       return TEXT("Follow");
	}
}

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
	EKindledStance Stance = EKindledStance::Follow;

	/** Wide variant (spans 2) for compacted large squads — e.g. 50-strong spearmen at 10 files. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bWide = false;
};
