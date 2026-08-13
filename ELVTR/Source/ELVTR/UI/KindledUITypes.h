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

	/**
	 * WHAT THIS ONE CAN CURRENTLY DO — castle-layout.md §6.4's HUD change, added by task-144.
	 *
	 * The doc's wording is that the HUD's job moves "from describing an army to describing seven
	 * individuals and what each can currently do", and the three fields below are that last
	 * clause. Everything above them still describes a squad, because the garrison is still a
	 * card and is still a hundred bodies — the panel now carries both kinds of thing, which is
	 * what the pivot actually produced.
	 *
	 * Empty Verb = this card has no verb to report (the garrison, or Q23 = A, where a soldier
	 * carries nothing and the kit is the bearer's). The card then draws no verb chip at all
	 * rather than an empty one, so "no verb" reads as absence and not as a blank readout.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FText Verb;

	/** Seconds until this card's verb is castable again; 0 = now. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	float VerbCooldown = 0.f;

	/** False greys the verb chip — spent, or its holder is down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bVerbReady = true;
};
