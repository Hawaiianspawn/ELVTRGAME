// Spawn implementation (SwarmSpawn.h) + the console commands that drive it:
//   Swarm.SpawnBrood <N>    - spawn N brood in a ring around the hero
//   Swarm.SpawnRetinue <N>  - spawn N retinue in formation slots around the hero
//   Swarm.Clear             - destroy all swarm entities
//   Swarm.Stance <name>     - Follow | Charge | Hold | Rally

#include "SwarmSpawn.h"

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "MassMovementFragments.h"
#include "SwarmCombat.h"
#include "SwarmFormation.h"
#include "SwarmFragments.h"
#include "SwarmSubsystem.h"
#include "TimerManager.h"

namespace
{
	// How far out the brood tide appears and converges from. Under the flame
	// (a ~900uu light pool) these spawn deep in the dark and emerge as they close.
	// There is no distance culling on the renderer, so this ring is the only
	// "how far out do enemies appear" number.
	TAutoConsoleVariable<float> CVarBroodSpawnRadiusMin(
		TEXT("Swarm.BroodSpawnRadiusMin"), 2500.f,
		TEXT("Inner radius (uu) of the ring brood spawn in, measured from the hero. Also the\n")
		TEXT("radius of the FRONT rank in the brood formation below (Swarm.BroodFormation.*)\n")
		TEXT("— the line that leads the wave and arrives first."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodSpawnRadiusMax(
		TEXT("Swarm.BroodSpawnRadiusMax"), 4000.f,
		TEXT("Outer radius (uu) of the brood spawn ring — kept as the REFERENCE depth a\n")
		TEXT("typical wave is sized to fit within (see BroodFormation.RankSpacing), not a\n")
		TEXT("hard bound: a bigger spawn's back ranks simply run past it, deeper into the\n")
		TEXT("dark, which reads fine since there is no distance culling on the renderer.\n")
		TEXT("Keep >= BroodSpawnRadiusMin."), ECVF_Default);

	// A full 360 ring is the "surrounded" case and it is the only one the spike could
	// stage. An arc is what lets a wave arrive as a FRONT — which is the situation the
	// stances are actually about, since Hold only means something if there is a
	// direction to hold against.
	//
	// 120 (2026-07-27, was 360) is the owner's "mostly from the front" width, not a
	// guess: at the BroodFormation defaults below (Columns 60, RankSpacing 140) the front
	// rank sits at RadiusMin (2500uu), and 120 degrees there puts ~90uu between
	// neighbouring columns — comfortably above the 60uu separation radius (Swarm.
	// BroodSeparation) so the front rank doesn't arrive already shoving itself apart —
	// while still leaving the retinue's flanks and rear (the other 240 degrees) clear of
	// spawns entirely.
	TAutoConsoleVariable<float> CVarBroodSpawnArc(
		TEXT("Swarm.BroodSpawnArc"), 120.f,
		TEXT("Width in DEGREES of arrival, AND the sweep the brood rank formation\n")
		TEXT("(Swarm.BroodFormation.*) fans its columns across — one dial, so the envelope\n")
		TEXT("brood spawn in and the ranks they spawn IN can't disagree. 360 = surrounded on\n")
		TEXT("all sides (the old default); ~90 = one flank; ~30 = a column down one\n")
		TEXT("approach; 120 (default) = a broad front. [0..360]"), ECVF_Default);

	// The retinue's own "forward" (Swarm.Formation.FaceCamera/Yaw) tracks the camera so
	// the line always stands broadside to the viewer. The brood's arc has to track the
	// SAME heading for the same reason: a fixed world bearing stops being "the front" the
	// instant the camera turns, and the owner would see brood spawning behind them. This
	// mirrors that composition exactly (same idea, same shape) rather than sharing its
	// CVars, so a scripted encounter can pin the brood's arrival direction independently
	// of wherever the retinue happens to be facing.
	TAutoConsoleVariable<int32> CVarBroodSpawnFaceCamera(
		TEXT("Swarm.BroodSpawnFaceCamera"), 1,
		TEXT("1 = the arc's centre bearing tracks the same heading the retinue faces\n")
		TEXT("(Kindled.Cam.Yaw, via the same composition Swarm.Formation.FaceCamera uses),\n")
		TEXT("so 'the front' keeps meaning the direction your line stands broadside to,\n")
		TEXT("however the camera turns (default). 0 = BroodSpawnArcCenter below is a\n")
		TEXT("literal fixed world bearing, for a scripted encounter that should always\n")
		TEXT("attack from one compass direction regardless of facing."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodSpawnArcCenter(
		TEXT("Swarm.BroodSpawnArcCenter"), 0.f,
		TEXT("Bearing in degrees, world +X = 0, CCW positive. While BroodSpawnFaceCamera is\n")
		TEXT("1 (default) this is EXTRA bearing added on top of the tracked camera yaw —\n")
		TEXT("mirrors Swarm.Formation.Yaw, for angling the front off-square without\n")
		TEXT("touching the camera. While 0, this IS the bearing, fixed. Ignored while Arc\n")
		TEXT("is 360 (no front to aim). [-180..180]"), ECVF_Default);

	// Same vocabulary as Swarm.Formation.Columns/RankSpacing (SwarmFormation.h), kept as
	// separate dials rather than shared ones: a wide shallow line of attackers against a
	// tight block of defenders is the readable case, and that needs the two formations to
	// be independently tunable, not locked together.
	TAutoConsoleVariable<int32> CVarBroodFormationColumns(
		TEXT("Swarm.BroodFormation.Columns"), 60,
		TEXT("Brood per rank in the spawn-time wave formation. THE framing dial, same idea\n")
		TEXT("as Formation.Columns: wide (default) reads as a broad tide-front advancing in\n")
		TEXT("rows; narrow and deep reads as a column punching down one approach. [1..200]"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarBroodFormationRankSpacing(
		TEXT("Swarm.BroodFormation.RankSpacing"), 140.f,
		TEXT("Radial gap between ranks, uu. Ranks step OUTWARD from BroodSpawnRadiusMin (the\n")
		TEXT("front rank, which leads the wave and arrives first) rather than inward like\n")
		TEXT("the retinue's Arc shape — the brood is still closing on the anchor, not\n")
		TEXT("standing at rest around it. At the defaults this fits a ~700-strong wave\n")
		TEXT("roughly inside the old RadiusMin..RadiusMax band; a bigger spawn just runs\n")
		TEXT("more ranks further back into the dark. [20..400]"), ECVF_Default);

	// Lowered 2026-07-26 -> 2026-07-27 (0.15 -> 0.06) once the brood got ranks to arrive
	// in. The two dials were fighting: over the ~10s crossing from RadiusMin to contact,
	// even a small per-unit speed gap compounds with distance and time into hundreds of uu
	// of drift between the fastest and slowest brood in a rank — at 0.15 that is enough to
	// blur the ranks together well before they reach the retinue, which defeats the entire
	// point of spawning them in rows. 0.06 keeps a rank's edge legible for longer while
	// still staggering the wave off one rigid, frame-perfect wall. What is lost: some of
	// the loose, unsynchronised trickle within a single rank — the wave now reads as
	// crisper advancing bars for most of the approach. That reads as MORE on-brand for the
	// Still Legion (G9), not less: an administrating enemy that closes in unsettlingly
	// even rows, not a rabble. Ranks still break up into ordinary per-unit steering at
	// contact regardless of this dial — see BroodFormation.RankSpacing and SwarmSteering.
	TAutoConsoleVariable<float> CVarBroodSpeedJitter(
		TEXT("Swarm.BroodSpeedJitter"), 0.06f,
		TEXT("Per-brood speed variation, +/- this fraction of Swarm.BroodSpeed, rolled at\n")
		TEXT("spawn. Strings the tide's arrival off one rigid frame; 0 makes the whole wave\n")
		TEXT("land at once. Lowered from 0.15 once the brood got a rank formation to arrive\n")
		TEXT("IN — see the comment above CVarBroodSpeedJitter in SwarmCommands.cpp for the\n")
		TEXT("full reconciliation. [0..0.8]"), ECVF_Default);

	struct FSwarmSpawnParams
	{
		bool bBrood = true;
		int32 Count = 0;
		// Vestigial for retinue as of task-046: formation slots are now TYPE-LOCAL, derived
		// per-recruit from USwarmSubsystem::GetAliveByType (see the recruitment block in
		// SpawnSwarm below), not from a flat cursor the caller has to track. Kept as a
		// public parameter so SpawnRetinue's signature — and every existing call site
		// (Spike1GameMode, the Swarm.SpawnRetinue console command) — doesn't have to churn.
		int32 SlotBase = 0;
	};

	void SpawnSwarm(UWorld* World, const FSwarmSpawnParams& Params)
	{
		if (!World || Params.Count <= 0)
		{
			return;
		}
		UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
		USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>();
		if (!MassSubsystem || !Swarm)
		{
			return;
		}

		FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

		TArray<const UScriptStruct*> Composition = {
			FTransformFragment::StaticStruct(),
			FMassVelocityFragment::StaticStruct(),
			FSwarmAnimFragment::StaticStruct(),
			FSwarmJitterFragment::StaticStruct(),
			FSwarmHealthFragment::StaticStruct(),
			FSwarmStrikeFragment::StaticStruct(),
			FSwarmTag::StaticStruct()
		};
		if (Params.bBrood)
		{
			Composition.Add(FBroodTag::StaticStruct());
		}
		else
		{
			Composition.Add(FRetinueFollowFragment::StaticStruct());
			Composition.Add(FRetinueTag::StaticStruct());
		}

		const FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(Composition);

		TArray<FMassEntityHandle> Entities;
		Entities.Reserve(Params.Count);
		const TSharedRef<FMassEntityManager::FEntityCreationContext> CreationContext =
			EntityManager.BatchCreateEntities(Archetype, Params.Count, Entities);

		// Flattened to the ground plane, and that Z=0 is load-bearing. The attractor is
		// SpikeHeroPawn's GetActorLocation(), which is a CHARACTER CAPSULE CENTRE sitting
		// 92uu above the floor -- not its feet. Both spawn branches below add only an XY
		// offset, and steering never touches Z, so an inherited 92 would float the entire
		// horde for its whole life, uniformly and independently of sprite size. That float
		// is what Swarm.SpriteGroundOffset's -72 was really cancelling (task-110): two
		// errors hiding each other, which is why removing one made the other visible.
		const FVector Center = FVector(Swarm->GetAttractor().X, Swarm->GetAttractor().Y, 0.f);
		FRandomStream Rand(FPlatformTime::Cycles());

		// Spearmen's formation (== today's retinue). Archers read their own independent
		// params (Swarm.Formation.Archers.*) — see SwarmFormation::ReadParamsForType.
		const SwarmFormation::FParams Formation = SwarmFormation::ReadParamsForType(EUnitType::Spearmen);
		const SwarmFormation::FParams ArchersFormation = SwarmFormation::ReadParamsForType(EUnitType::Archers);

		// Brood formation: SwarmFormation::FParams/BroodSlotOffset again (SwarmFormation.h)
		// — the retinue's own vocabulary, laid out on the brood's spawn arc instead of the
		// retinue's block. Built here, not via a ReadParams-style helper living next to
		// ReadParams, because every dial it needs (Arc, RadiusMin, ArcCenter, FaceCamera)
		// is already a spawn-owned CVar in this file; SwarmFormation.CameraYawDegrees()
		// is the one piece that has to be shared rather than re-looked-up, so the brood
		// front tracks the exact same camera reading the retinue formation does.
		SwarmFormation::FParams BroodFormation;
		BroodFormation.Columns = FMath::Clamp(CVarBroodFormationColumns.GetValueOnGameThread(), 1, 200);
		BroodFormation.RankSpacing = FMath::Max(CVarBroodFormationRankSpacing.GetValueOnGameThread(), 1.f);
		BroodFormation.ArcDegrees = FMath::Clamp(CVarBroodSpawnArc.GetValueOnGameThread(), 0.f, 360.f);
		BroodFormation.ArcRadius = FMath::Max(CVarBroodSpawnRadiusMin.GetValueOnGameThread(), 0.f);
		{
			const float BroodBearingDeg = CVarBroodSpawnArcCenter.GetValueOnGameThread()
				+ (CVarBroodSpawnFaceCamera.GetValueOnGameThread() != 0 ? SwarmFormation::CameraYawDegrees() : 0.f);
			BroodFormation.YawRadians = FMath::DegreesToRadians(BroodBearingDeg);
		}

		const float BroodMaxHP = SwarmCombatTuning::BroodMaxHP();

		// task-095: a Spearman's HP now comes from the knight sub-type row its own
		// team-atlas variant maps to, not one flat Swarm.RetinueMaxHP for the whole line —
		// baked in once here (below) from the SAME phase that later decides its look
		// (SwarmProcessors.cpp), so HP and look can never disagree. Snapshotted once per
		// spawn batch, same as everything else in this function.
		const SwarmCombatTuning::FKnightSubtypeTables KnightTables = SwarmCombatTuning::GetKnightSubtypeTables();

		// Retinue keeps the original fixed +/-15%: your line's raggedness isn't a dial
		// anyone has asked to move, and the formation slots already stagger it.
		constexpr float RetinueSpeedJitter = 0.15f;
		const float SpeedJitter = Params.bBrood
			? FMath::Clamp(CVarBroodSpeedJitter.GetValueOnGameThread(), 0.f, 0.95f)
			: RetinueSpeedJitter;

		// --- typed-unit recruitment (docs/design/squad-group-system.md §1.4) --------------
		// Every new soldier rolls its type independently against Swarm.ArcherGrowthWeight
		// (stand-in for a generator-tagged growth site — v1 has no real site system, §1.4).
		// SlotIndex is now TYPE-LOCAL (0..Pool(type)-1, not 0..AliveRetinue-1): each type
		// gets its own dense formation-slot space (§1.2/§8), so the cursor for each type
		// starts from that type's OWN current live count, not the flat Params.SlotBase this
		// used to share across both. AssignRecruit is also handed how many of that type
		// THIS BATCH has already produced, so a big single batch (e.g. StartingRetinue)
		// opens however many new units its own recruits justify, not just last frame's.
		const float ArcherWeight = SwarmCombatTuning::ArcherGrowthWeight();
		int32 SlotCursorByType[NumUnitTypes] = {
			Swarm->GetAliveByType(EUnitType::Spearmen), Swarm->GetAliveByType(EUnitType::Archers)
		};
		int32 RecruitedThisBatch[NumUnitTypes] = {};

		for (int32 Index = 0; Index < Entities.Num(); ++Index)
		{
			FMassEntityView View(EntityManager, Entities[Index]);

			FVector SpawnLocation;
			float MaxHP = BroodMaxHP;
			float TypeSpeedScale = 1.f;

			// Rolled here, once, rather than inline where JitterFragment.Phase used to be
			// set below — a Spearman's HP (right below) needs the SAME phase its look will
			// later resolve from, not a second independent draw that could pick a
			// different knight for the HP than the one it ends up wearing.
			const float Phase = Rand.FRandRange(0.f, 10.f);

			if (Params.bBrood)
			{
				// Rank/column slot on the spawn arc (SwarmFormation::BroodSlotOffset).
				// Rank 0 sits at BroodSpawnRadiusMin and leads the wave; later ranks step
				// outward and arrive later — this IS the front-as-rows arrangement, not a
				// scatter. No position jitter added on top: BroodSpeedJitter is what
				// unsettles this over the march, the same way the retinue's own grid-exact
				// slots only move once steering (not spawn) touches them.
				const FVector2D Offset = SwarmFormation::BroodSlotOffset(Index, BroodFormation);
				SpawnLocation = Center + FVector(Offset.X, Offset.Y, 0.f);
			}
			else
			{
				const EUnitType Type = (Rand.FRand() < ArcherWeight) ? EUnitType::Archers : EUnitType::Spearmen;
				const int32 TypeIdx = (int32)Type;
				const uint8 SquadByte = Swarm->AssignRecruit(Type, RecruitedThisBatch[TypeIdx]++);

				if (Type == EUnitType::Archers)
				{
					MaxHP = SwarmCombatTuning::ArchersMaxHP();
					TypeSpeedScale = SwarmCombatTuning::ArchersMoveSpeedScale();
				}
				else
				{
					const int32 Variant = SwarmRenderPack::VariantFromPhase(
						Phase, KnightTables.TeamVariantCum, KnightTables.NumTeamVariants);
					const int32 Row = SwarmCombatTuning::KnightSubtypeRowFor(KnightTables, Variant);
					MaxHP = KnightTables.HP[Row];
					TypeSpeedScale = 1.f;
				}

				// Only the index is stored — the offset below is just where to drop the
				// unit on frame one. The steering pass re-derives its place every frame
				// from the live formation dials, per this soldier's own type (SwarmFormation.h).
				const int32 Slot = SlotCursorByType[TypeIdx]++;
				View.GetFragmentData<FRetinueFollowFragment>().SlotIndex = Slot;
				const SwarmFormation::FParams& MyFormation = (Type == EUnitType::Archers) ? ArchersFormation : Formation;
				const FVector2D Offset = SwarmFormation::SlotOffset(Slot, MyFormation);
				SpawnLocation = Center + FVector(Offset.X, Offset.Y, 0.f);

				FSwarmAnimFragment& AnimFragment = View.GetFragmentData<FSwarmAnimFragment>();
				AnimFragment.Bits = SwarmAnim::TeamBit;
				AnimFragment.SquadId = SquadByte;
			}

			View.GetFragmentData<FTransformFragment>().GetMutableTransform().SetTranslation(SpawnLocation);

			FSwarmJitterFragment& JitterFragment = View.GetFragmentData<FSwarmJitterFragment>();
			JitterFragment.SpeedScale = TypeSpeedScale * Rand.FRandRange(1.f - SpeedJitter, 1.f + SpeedJitter);
			JitterFragment.Phase = Phase;

			FSwarmHealthFragment& HealthFragment = View.GetFragmentData<FSwarmHealthFragment>();
			HealthFragment.MaxHP = MaxHP;
			HealthFragment.HP = MaxHP;

			// Desynchronise the swing clocks. Reuse the jitter phase that already
			// staggers the walk cycle — spawning a wave with every unit's swing at
			// zero would make the whole front line strike on the same frame, which
			// reads as one pulsing organism rather than a mêlée.
			View.GetFragmentData<FSwarmStrikeFragment>().SwingTime =
				FMath::Fmod(Phase, SwarmCombatTuning::SwingInterval());
		}

		Swarm->TrackSpawned(Entities);
		UE_LOG(LogTemp, Display, TEXT("Swarm: spawned %d %s (total tracked: %d)"),
			Entities.Num(),
			Params.bBrood ? TEXT("brood") : TEXT("retinue"),
			Swarm->GetTrackedEntities().Num());
	}

	int32 ParseCount(const TArray<FString>& Args, int32 Default)
	{
		return Args.Num() > 0 ? FCString::Atoi(*Args[0]) : Default;
	}

	FAutoConsoleCommandWithWorldAndArgs GSpawnBroodCmd(
		TEXT("Swarm.SpawnBrood"),
		TEXT("Spawn N brood entities in a ring around the hero. Usage: Swarm.SpawnBrood 1000"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SwarmSpawn::SpawnBrood(World, ParseCount(Args, 1000));
			}));

	FAutoConsoleCommandWithWorldAndArgs GSpawnRetinueCmd(
		TEXT("Swarm.SpawnRetinue"),
		TEXT("Spawn N retinue entities in formation around the hero. Usage: Swarm.SpawnRetinue 100"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				// Base the new slots past the standing line rather than at 0. Two batches
				// both starting at 0 hand the same slot index to two soldiers, and since
				// the formation repack ranks by index it cannot tell them apart — they
				// would stand inside each other for the rest of the run. The game mode
				// already passes its own cursor (Spike1GameMode); this is the console
				// path catching up with it.
				const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				const int32 SlotBase = Swarm ? Swarm->GetAliveRetinue() : 0;
				SwarmSpawn::SpawnRetinue(World, ParseCount(Args, 100), SlotBase);
			}));

	FAutoConsoleCommandWithWorldAndArgs GClearCmd(
		TEXT("Swarm.Clear"),
		TEXT("Destroy all swarm entities."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				SwarmSpawn::ClearAll(World);
			}));

	// Evidence tool (task-046): a one-shot delayed console command, so an exec-on-play
	// sequence (Saved/SwarmExecOnPlay.txt — the only scripted-scenario surface this repo
	// has; MCP has no live console-command injection, see docs/AGENT-TEAMS.md's MCP CVar
	// notes) can stage a SECOND action after combat has had time to produce casualties —
	// e.g. re-running Swarm.LogSquadRoster to compare against a T=0 snapshot. Mirrors the
	// timer pattern Swarm.DebugShotAfter already uses (ASwarmRenderActor, out of this
	// task's owned files) instead of duplicating it there.
	FAutoConsoleCommandWithWorldAndArgs GRunAfterCmd(
		TEXT("Swarm.RunAfter"),
		TEXT("Run a console command once, N seconds from now. Usage:\n")
		TEXT("Swarm.RunAfter <seconds> <command> [args...]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (!World || Args.Num() < 2)
				{
					UE_LOG(LogTemp, Warning, TEXT("Swarm.RunAfter: usage <seconds> <command> [args...]"));
					return;
				}
				const float Delay = FMath::Max(FCString::Atof(*Args[0]), 0.f);
				FString Command = Args[1];
				for (int32 i = 2; i < Args.Num(); ++i)
				{
					Command += TEXT(" ");
					Command += Args[i];
				}
				TWeakObjectPtr<UWorld> WeakWorld(World);
				FTimerHandle Handle;
				World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda(
					[WeakWorld, Command]()
					{
						if (UWorld* W = WeakWorld.Get())
						{
							GEngine->Exec(W, *Command);
						}
					}), Delay, false);
				UE_LOG(LogTemp, Display, TEXT("Swarm.RunAfter: will run '%s' in %.1fs"), *Command, Delay);
			}));

	FAutoConsoleCommandWithWorldAndArgs GStanceCmd(
		TEXT("Swarm.Stance"),
		TEXT("Set the retinue stance. Usage: Swarm.Stance Follow|Charge|Hold|Rally"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm || Args.Num() == 0)
				{
					return;
				}
				const FString& Name = Args[0];
				ESwarmStance Stance = ESwarmStance::Follow;
				if (Name.Equals(TEXT("Charge"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Charge; }
				else if (Name.Equals(TEXT("Hold"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Hold; }
				else if (Name.Equals(TEXT("Rally"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Rally; }

				Swarm->SetStance(Stance, Swarm->GetAttractor());
				UE_LOG(LogTemp, Display, TEXT("Swarm: stance = %s"), LexToString(Stance));
			}));

	// Addressed order (docs/design/squad-group-system.md §3): one named unit instead of
	// "all". Stand-in input surface — the real driver is a muster-card click/hotkey, owned
	// elsewhere (mirrors Kindled.UnitCamProj.SelectedSquad's own stand-in precedent,
	// UnitCamDirector.cpp). Does NOT touch the global Stance/StanceAnchor or any other
	// unit's order — see USwarmSubsystem::SetUnitStance.
	FAutoConsoleCommandWithWorldAndArgs GUnitStanceCmd(
		TEXT("Swarm.UnitStance"),
		TEXT("Set ONE unit's stance, leaving every other unit (and the 'all units' order)\n")
		TEXT("untouched. Usage: Swarm.UnitStance <0-7> Follow|Charge|Hold|Rally"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm || Args.Num() < 2)
				{
					UE_LOG(LogTemp, Warning, TEXT("Swarm.UnitStance: usage <0-7> Follow|Charge|Hold|Rally"));
					return;
				}
				const int32 UnitIndex = FCString::Atoi(*Args[0]);
				const FString& Name = Args[1];
				ESwarmStance Stance = ESwarmStance::Follow;
				if (Name.Equals(TEXT("Charge"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Charge; }
				else if (Name.Equals(TEXT("Hold"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Hold; }
				else if (Name.Equals(TEXT("Rally"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Rally; }

				Swarm->SetUnitStance(UnitIndex, Stance, Swarm->GetAttractor());
				UE_LOG(LogTemp, Display, TEXT("Swarm: unit %d stance = %s (type %s)"),
					UnitIndex, LexToString(Stance), LexToString(Swarm->GetSquadType(UnitIndex)));
			}));

	// Evidence tool (task-046): dump every tracked entity's PERMANENT (unit, type) pair —
	// squad-group-system.md §1.3's sticky-SquadId fix should hold no matter how many
	// casualties land elsewhere. Log this before and after a wave of casualties and the
	// same entities must report the same unit + type both times; a repack may re-densify
	// FRetinueFollowFragment::SlotIndex (their spot in the line) but never SquadId.
	FAutoConsoleCommandWithWorldAndArgs GLogSquadRosterCmd(
		TEXT("Swarm.LogSquadRoster"),
		TEXT("Log every tracked retinue entity's (EntityIndex, unit, type). Usage:\n")
		TEXT("Swarm.LogSquadRoster — logs everyone. Swarm.LogSquadRoster <0-7> — one unit only."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				UMassEntitySubsystem* MassSubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
				if (!Swarm || !MassSubsystem)
				{
					return;
				}
				const bool bFilterUnit = Args.Num() > 0;
				const int32 FilterUnit = bFilterUnit ? FCString::Atoi(*Args[0]) : 0;

				const FMassEntityManager& EntityManager = MassSubsystem->GetEntityManager();
				int32 Logged = 0;
				for (const FMassEntityHandle& Handle : Swarm->GetTrackedEntities())
				{
					if (!EntityManager.IsEntityValid(Handle))
					{
						continue;
					}
					const FMassEntityView View(EntityManager, Handle);
					const FSwarmAnimFragment* Anim = View.GetFragmentDataPtr<FSwarmAnimFragment>();
					if (!Anim || (Anim->Bits & SwarmAnim::TeamBit) == 0)
					{
						continue; // brood carry no unit/type
					}
					const int32 UnitIdx = SwarmSquad::UnitIndex(Anim->SquadId);
					if (bFilterUnit && UnitIdx != FilterUnit)
					{
						continue;
					}
					UE_LOG(LogTemp, Display, TEXT("  Entity[idx=%d,serial=%d]: unit=%d type=%s"),
						Handle.Index, Handle.SerialNumber, UnitIdx, LexToString(SwarmSquad::UnitType(Anim->SquadId)));
					++Logged;
				}
				UE_LOG(LogTemp, Display, TEXT("Swarm.LogSquadRoster: %d entit%s logged."),
					Logged, Logged == 1 ? TEXT("y") : TEXT("ies"));
			}));
}

namespace SwarmSpawn
{
	void SpawnBrood(UWorld* World, int32 Count)
	{
		SpawnSwarm(World, FSwarmSpawnParams{ true, Count, 0 });
	}

	void SpawnRetinue(UWorld* World, int32 Count, int32 SlotBase)
	{
		SpawnSwarm(World, FSwarmSpawnParams{ false, Count, SlotBase });
	}

	void ClearAll(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
		USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>();
		if (!MassSubsystem || !Swarm)
		{
			return;
		}
		FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
		for (const FMassEntityHandle& Handle : Swarm->GetTrackedEntities())
		{
			if (EntityManager.IsEntityValid(Handle))
			{
				EntityManager.DestroyEntity(Handle);
			}
		}
		Swarm->GetTrackedEntities().Reset();
		UE_LOG(LogTemp, Display, TEXT("Swarm: cleared."));
	}

	void CompactTracked(UWorld* World)
	{
		UMassEntitySubsystem* MassSubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
		USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!MassSubsystem || !Swarm)
		{
			return;
		}
		const FMassEntityManager& EntityManager = MassSubsystem->GetEntityManager();
		Swarm->GetTrackedEntities().RemoveAllSwap(
			[&EntityManager](const FMassEntityHandle& Handle)
			{
				return !EntityManager.IsEntityValid(Handle);
			},
			EAllowShrinking::No);
	}
}
