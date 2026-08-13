#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpikeBossActor.generated.h"

/**
 * One marked boss — docs/design/castle-layout.md §6, stat block from
 * docs/design/entity-tiers.md §3, promoted-Actor pattern from that doc's §5.
 *
 * §5 says to reuse the hero grid-bridge rather than invent a second Actor-vs-Mass path, and
 * this is that, with one addition that turns out to carry most of the weight: the boss keeps
 * its authoritative state on USwarmSubsystem (FBossState) exactly as the hero does, AND is
 * published into the spatial grid each frame as an ordinary enemy entry. Everything the
 * grid already does then applies to it for free — the line finds it, closes on it, swings at
 * it on the shared cadence, takes its blow victim-side out of the same conserved
 * BlowsClaimed budget, gets knocked back by it and turns to face it. Not one line of that
 * had to be written. What IS written by hand is damage IN (SwarmCombatProcessors.cpp's
 * retinue claim) and the blow against the bearer below, and both are near-verbatim copies of
 * the existing brood-vs-hero branch.
 *
 * NO PHASES, NO ADDS, NO ARENA. entity-tiers.md §3 is explicit that it specs the boss's
 * baseline stat block only and that "full fight design (phases, arena, mechanics) is out of
 * scope"; this actor stays inside that line. What it adds on top is §6.1's marks, because
 * marks are the pivot's actual idea and a boss without them is just a big brood.
 *
 * MARKS ARE SET BY CONSOLE AND NOTHING ACCRETES THEM. §6.2's whole claim — that a boss is a
 * report on a war you were not in — needs the offscreen war simulation, which is out of this
 * slice's scope. `Kindled.Boss.Marks` is a prototype surface for reading and judging the
 * marks, not a position on how a boss earns one.
 */
UCLASS()
class ASpikeBossActor : public AActor
{
	GENERATED_BODY()

public:
	ASpikeBossActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Spawn the boss into World at the bearing the tide arrives on, carrying Marks, and seed
	 * USwarmSubsystem's FBossState. Destroys any boss already standing — one boss, this
	 * slice. Returns nullptr if the world has no swarm subsystem.
	 */
	static ASpikeBossActor* SpawnBoss(UWorld* World, uint8 Marks);

	/** Destroy the standing boss, if any, and clear the subsystem's state. */
	static void ClearBoss(UWorld* World);

	/** "quilled,ram,sated" / "none" -> an EBossMark bitmask. Unknown tokens warn and are
	 *  ignored, so a typo in an exec file cannot silently produce an unmarked boss. */
	static uint8 ParseMarks(const FString& CommaSeparated);

	/** The bitmask back as "QUILLED+RAM" / "unmarked", for logs and the HUD. */
	static FString MarksToString(uint8 Marks);

private:
	/** Where this boss wants to be, which is the whole of what a mark changes about movement. */
	FVector ResolveTarget(const class USwarmSubsystem& Swarm) const;

	/** Body + mark decorations, drawn so each mark reads BEFORE it acts (§6.1). */
	void Draw(const class USwarmSubsystem& Swarm) const;

	/** Its own swing clock, in seconds, wrapping on Kindled.Boss.SwingInterval — the same
	 *  cadence every other body in the sim runs, at a slower, telegraphed beat. */
	float SwingTime = 0.f;

	/** Seconds since it last took a blow. Sated regenerates once this passes its calm window. */
	float CalmSeconds = 0.f;
};
