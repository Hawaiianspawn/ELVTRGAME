// Spawn implementation (SwarmSpawn.h) + the console commands that drive it:
//   Swarm.SpawnBrood <N>    - spawn N brood in a ring around the hero
//   Swarm.SpawnRetinue <N>  - spawn N retinue in formation slots around the hero
//   Swarm.Clear             - destroy all swarm entities
//   Swarm.Stance <name>     - Follow | Charge | Hold | Rally
//   Kindled.Adapt <u> <ladder> <rung> - put one unit on an Adaptation rung
//                             (docs/design/adaptation.md)

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

		// --- recruitment override (docs/design/slice-a7.md) ------------------------------
		// INDEX_NONE keeps the shipped path exactly: roll a type, hand it to AssignRecruit,
		// let it grow units out of the stream. Set, it means "this batch belongs to THIS
		// handle" — which is what the garrison (one handle, a hundred bodies) and each of the
		// seven (one handle, one body) both need and neither can express through a policy
		// whose whole job is to spread recruits across handles by fullness.
		int32 ForceUnit = INDEX_NONE;
		bool bForceType = false;        // when ForceUnit is set, pin the type too rather than rolling
		EUnitType ForcedType = EUnitType::Spearmen;
		float HPScale = 1.f;
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
		// Adaptation: a recruit joining an already-adapted unit is BORN on that rung — the
		// alternative (only new units adapt) would leave a unit permanently mixed, and
		// adaptation.md §6 rules that a rung belongs to the branch, not to the body.
		const SwarmCombatTuning::FTierTable Tiers = SwarmCombatTuning::GetTierTable();

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
				// ForceUnit bypasses AssignRecruit entirely rather than teaching it a second
				// policy — see FSwarmSpawnParams. ClaimSquad still has to run, or the handle
				// reads unclaimed and Kindled.Adapt refuses to touch it.
				const EUnitType Type = Params.bForceType
					? Params.ForcedType
					: ((Rand.FRand() < ArcherWeight) ? EUnitType::Archers : EUnitType::Spearmen);
				const int32 TypeIdx = (int32)Type;
				uint8 SquadByte;
				if (Params.ForceUnit != INDEX_NONE)
				{
					Swarm->ClaimSquad(Params.ForceUnit, Type);
					SquadByte = SwarmSquad::Pack((uint8)Params.ForceUnit, Type);
				}
				else
				{
					SquadByte = Swarm->AssignRecruit(Type, RecruitedThisBatch[TypeIdx]++);
				}
				const int32 MyUnit = SwarmSquad::UnitIndex(SquadByte);
				const int32 MyTier = Swarm->GetSquadTier(MyUnit);

				if (Type == EUnitType::Archers)
				{
					MaxHP = SwarmCombatTuning::TierHPOr(Tiers, MyTier, SwarmCombatTuning::ArchersMaxHP());
					TypeSpeedScale = SwarmCombatTuning::ArchersMoveSpeedScale();
				}
				else
				{
					const int32 Variant = SwarmRenderPack::VariantFor(Swarm->GetSquadVariant(MyUnit),
						Phase, KnightTables.TeamVariantCum, KnightTables.NumTeamVariants);
					const int32 Row = SwarmCombatTuning::KnightSubtypeRowFor(KnightTables, Variant);
					MaxHP = SwarmCombatTuning::TierHPOr(Tiers, MyTier, KnightTables.HP[Row]);
					TypeSpeedScale = 1.f;
				}

				// Only the index is stored — the offset below is just where to drop the
				// unit on frame one. The steering pass re-derives its place every frame
				// from the live formation dials, per this soldier's own type (SwarmFormation.h).
				const int32 Slot = SlotCursorByType[TypeIdx]++;
				View.GetFragmentData<FRetinueFollowFragment>().SlotIndex = Slot;
				const SwarmFormation::FParams& MyFormation = (Type == EUnitType::Archers) ? ArchersFormation : Formation;
				// Group 0: the recruit's detachment isn't known until the repack ranks the
				// looks next frame, and this offset is only where to drop him on frame one.
				const FVector2D Offset = SwarmFormation::SlotOffset(Slot, 0, MyFormation);
				SpawnLocation = Center + FVector(Offset.X, Offset.Y, 0.f);

				FSwarmAnimFragment& AnimFragment = View.GetFragmentData<FSwarmAnimFragment>();
				AnimFragment.Bits = SwarmAnim::TeamBit;
				AnimFragment.SquadId = SquadByte;
			}

			View.GetFragmentData<FTransformFragment>().GetMutableTransform().SetTranslation(SpawnLocation);

			FSwarmJitterFragment& JitterFragment = View.GetFragmentData<FSwarmJitterFragment>();
			JitterFragment.SpeedScale = TypeSpeedScale * Rand.FRandRange(1.f - SpeedJitter, 1.f + SpeedJitter);
			JitterFragment.Phase = Phase;

			// HPScale is 1 for everything except a named soldier — see SwarmSpawn.h for why
			// the seven need one and why it is an expedient rather than a stat block.
			MaxHP *= Params.bBrood ? 1.f : FMath::Max(Params.HPScale, 0.01f);

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
		TEXT("untouched. Usage: Swarm.UnitStance <0-6> Follow|Charge|Hold|Rally\n")
		TEXT("\n")
		TEXT("0-6 are the SEVEN named soldiers (docs/design/slice-a7.md). 7 is the autonomous\n")
		TEXT("garrison and ordering it contradicts the whole point of it holding on its own —\n")
		TEXT("the command still goes through, deliberately, because pulling the war off its\n")
		TEXT("anchor by hand is a useful thing to be able to do while judging whether the line\n")
		TEXT("reads as a front. It warns so it can never happen by accident.\n")
		TEXT("\n")
		TEXT("How orders are ACTUALLY issued is docs/OPEN-DECISIONS.md Q26 and is still open;\n")
		TEXT("this is the stand-in surface, not a proposal."),
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

				if (UnitIndex == USwarmSubsystem::GarrisonUnit)
				{
					UE_LOG(LogTemp, Warning, TEXT("Swarm.UnitStance: unit %d is THE GARRISON — the war, "
						"not your squad. Ordering it pulls the front off its anchor. The seven are 0-6."),
						UnitIndex);
				}
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

	// --- Adaptation (docs/design/adaptation.md) ---------------------------------------
	// The ladder table, transcribed from docs/data/unit-types.json `adaptation.ladders[]`.
	// A CVar and not a JSON read, for the same reason every other table in this module is
	// one: nothing in the runtime loads docs/data (the SIM owns that file), and a CVar can
	// be retuned mid-PIE without a rebuild — which matters more than usual here, because
	// this module cannot be Live Coded at all. Keep the two in step BY HAND; the numbers
	// below are the FLAT 0-23 team-atlas indices the data speaks, and rung order IS tier
	// order (adaptation.md §3), so a rung index and a Swarm.TierHP row index are one number.
	TAutoConsoleVariable<FString> CVarAdaptationLadders(
		TEXT("Kindled.Adaptation.Ladders"),
		TEXT("spearmen-line:0,1,3,7;archer-scout:11,13,15,14;archer-gunner:11,20,18,21;archer-siege:11,23,22,19"),
		TEXT("Adaptation ladders: <id>:<flat atlas index per rung, comma-separated>, with\n")
		TEXT("ladders separated by ';'. Rung order is freed, militia, veteran, bannerman —\n")
		TEXT("the same order as Swarm.TierHP / Swarm.TierDPS, because a rung index IS a tier\n")
		TEXT("index (adaptation.md §3: rank is array order, never the atlas index).\n")
		TEXT("Spearman ladders must use indices 0-10 and archer ladders 11-23; Kindled.Adapt\n")
		TEXT("refuses a rung whose atlas block disagrees with the unit's own type, because\n")
		TEXT("the render bridge picks the sub-table off that type and would draw the wrong\n")
		TEXT("row. Transcribed from docs/data/unit-types.json adaptation.ladders[]."),
		ECVF_Default);

	struct FAdaptLadder
	{
		FString Id;
		TArray<int32> Rungs;	// FLAT team-atlas indices, one per rung, in rung order
	};

	TArray<FAdaptLadder> ParseAdaptationLadders()
	{
		TArray<FAdaptLadder> Out;
		TArray<FString> Entries;
		CVarAdaptationLadders.GetValueOnGameThread().ParseIntoArray(Entries, TEXT(";"), true);
		for (const FString& Entry : Entries)
		{
			FString Id, Csv;
			if (!Entry.Split(TEXT(":"), &Id, &Csv))
			{
				continue;
			}
			FAdaptLadder Ladder;
			Ladder.Id = Id.TrimStartAndEnd();
			TArray<FString> Parts;
			Csv.ParseIntoArray(Parts, TEXT(","), true);
			for (const FString& Part : Parts)
			{
				Ladder.Rungs.Add(FCString::Atoi(*Part.TrimStartAndEnd()));
			}
			if (!Ladder.Id.IsEmpty() && Ladder.Rungs.Num() > 0)
			{
				Out.Add(MoveTemp(Ladder));
			}
		}
		return Out;
	}

	/**
	 * Re-stat the bodies already standing in a unit.
	 *
	 * DPS, cleave and reach need none of this — they are read live off the tier table and
	 * the look every pass, so they move the instant the rung is assigned. HP does not:
	 * MaxHP is baked into FSwarmHealthFragment once at spawn (it is a running total combat
	 * decrements, not a per-frame lookup), so without this pass a unit would adapt into a
	 * new look and new damage while keeping the old tier's health, and the difference
	 * would show up as a wrong-feeling fight nobody could trace to a stale field.
	 *
	 * CURRENT HP is scaled by the same ratio rather than refilled: adapting is a promotion,
	 * not a heal, and a wounded veteran should stay wounded. Returns bodies touched.
	 */
	int32 ReStatUnit(UWorld* World, int32 UnitIndex, float NewMaxHP)
	{
		UMassEntitySubsystem* MassSubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
		USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!MassSubsystem || !Swarm || NewMaxHP <= 0.f)
		{
			return 0;
		}
		FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
		int32 Touched = 0;
		for (const FMassEntityHandle& Handle : Swarm->GetTrackedEntities())
		{
			if (!EntityManager.IsEntityValid(Handle))
			{
				continue;
			}
			FMassEntityView View(EntityManager, Handle);
			const FSwarmAnimFragment* Anim = View.GetFragmentDataPtr<FSwarmAnimFragment>();
			if (!Anim || (Anim->Bits & SwarmAnim::TeamBit) == 0
				|| SwarmSquad::UnitIndex(Anim->SquadId) != UnitIndex)
			{
				continue;	// brood carry no unit; other units are not this order's business
			}
			FSwarmHealthFragment& Health = View.GetFragmentData<FSwarmHealthFragment>();
			const float Fraction = Health.MaxHP > 0.f ? FMath::Clamp(Health.HP / Health.MaxHP, 0.f, 1.f) : 1.f;
			Health.MaxHP = NewMaxHP;
			Health.HP = NewMaxHP * Fraction;
			++Touched;
		}
		return Touched;
	}

	/**
	 * Assign one unit a rung on a ladder — the input surface for Adaptation, and a
	 * stand-in one, exactly like Swarm.UnitStance is for addressed orders. The player-
	 * facing branch pick and the shop (adaptation.md §7) are a separate pass; what this
	 * proves is that a rung actually lands: the unit re-skins, re-forms as one detachment,
	 * and fights on its tier's numbers.
	 *
	 * Scope, stated so it is not mistaken for the whole system: this is items 4-and-a-half
	 * of adaptation.md §6. A branch here is an EXISTING command handle, not a new
	 * EUnitType — §6 items 1, 2 and 3 (the one-bit type, the eight consumed handles, D14's
	 * unimplemented dispatch) are untouched and still open. The captain rung assigns
	 * bannerman's look and stats; it does NOT field a retinue (A4), which is its own pass.
	 */
	FAutoConsoleCommandWithWorldAndArgs GAdaptCmd(
		TEXT("Kindled.Adapt"),
		TEXT("Assign a unit an Adaptation rung. Usage:\n")
		TEXT("  Kindled.Adapt                       — list ladders and current assignments\n")
		TEXT("  Kindled.Adapt <0-7> <ladder> <rung> — e.g. Kindled.Adapt 0 spearmen-line 2\n")
		TEXT("  Kindled.Adapt <0-7> clear           — back to the phase roll and stock stats"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm)
				{
					return;
				}
				const TArray<FAdaptLadder> Ladders = ParseAdaptationLadders();
				const SwarmCombatTuning::FTierTable Tiers = SwarmCombatTuning::GetTierTable();

				if (Args.Num() == 0)
				{
					UE_LOG(LogTemp, Display, TEXT("Kindled.Adapt: %d ladder(s), %d tier row(s)."),
						Ladders.Num(), Tiers.NumRows);
					for (const FAdaptLadder& Ladder : Ladders)
					{
						FString Rungs;
						for (int32 r = 0; r < Ladder.Rungs.Num(); ++r)
						{
							Rungs += FString::Printf(TEXT("%s%d:atlas%d"), r ? TEXT(" ") : TEXT(""), r, Ladder.Rungs[r]);
						}
						UE_LOG(LogTemp, Display, TEXT("  %s -> %s"), *Ladder.Id, *Rungs);
					}
					for (int32 u = 0; u < USwarmSubsystem::MaxSquads; ++u)
					{
						if (!Swarm->IsSquadClaimed(u))
						{
							continue;
						}
						const int32 Tier = Swarm->GetSquadTier(u);
						UE_LOG(LogTemp, Display, TEXT("  unit %d (%s, %d standing): %s"),
							u, LexToString(Swarm->GetSquadType(u)), Swarm->GetSquadStanding(u),
							Tier < 0 ? TEXT("unadapted")
								: *FString::Printf(TEXT("rung %d, within-block look %d, %.0f HP / %.0f DPS"),
									Tier, Swarm->GetSquadVariant(u),
									SwarmCombatTuning::TierHPOr(Tiers, Tier, 0.f),
									SwarmCombatTuning::TierDPSOr(Tiers, Tier, 0.f)));
					}
					return;
				}

				const int32 UnitIndex = FCString::Atoi(*Args[0]);
				if (UnitIndex < 0 || UnitIndex >= USwarmSubsystem::MaxSquads)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: unit must be 0-%d."), USwarmSubsystem::MaxSquads - 1);
					return;
				}
				if (Args.Num() >= 2 && Args[1].Equals(TEXT("clear"), ESearchCase::IgnoreCase))
				{
					Swarm->ClearSquadRung(UnitIndex);
					UE_LOG(LogTemp, Display, TEXT("Kindled.Adapt: unit %d back to the phase roll. "
						"Standing bodies keep the HP they were adapted to — respawn to reset it."), UnitIndex);
					return;
				}
				if (Args.Num() < 3)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: usage <0-7> <ladder> <rung>, or <0-7> clear."));
					return;
				}
				if (!Swarm->IsSquadClaimed(UnitIndex))
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: unit %d has no soldiers — "
						"AssignRecruit has never opened it, so it has no type to check the rung against."), UnitIndex);
					return;
				}

				const FAdaptLadder* Ladder = Ladders.FindByPredicate(
					[&Args](const FAdaptLadder& L) { return L.Id.Equals(Args[1], ESearchCase::IgnoreCase); });
				if (!Ladder)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: no ladder '%s'. Run Kindled.Adapt with no args to list them."), *Args[1]);
					return;
				}
				const int32 Rung = FCString::Atoi(*Args[2]);
				if (Rung < 0 || Rung >= Ladder->Rungs.Num())
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: ladder '%s' has rungs 0-%d."),
						*Ladder->Id, Ladder->Rungs.Num() - 1);
					return;
				}
				if (Rung >= Tiers.NumRows)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: rung %d has no tier row — "
						"Swarm.TierHP/Swarm.TierDPS parse to %d row(s)."), Rung, Tiers.NumRows);
					return;
				}

				// The atlas block has to match the unit's own type or the render bridge draws
				// the wrong sheet rows: it picks the spearman or archer sub-table off the
				// squad byte's type and then adds ArcherVariantBase itself, so a flat archer
				// index handed to a spearman unit would land 11 rows early and silently.
				const int32 Flat = Ladder->Rungs[Rung];
				const EUnitType Type = Swarm->GetSquadType(UnitIndex);
				int32 WithinBlock = INDEX_NONE;
				if (Type == EUnitType::Archers)
				{
					if (Flat >= SwarmSheet::Team::ArcherVariantBase && Flat < SwarmSheet::Team::Variants)
					{
						WithinBlock = Flat - SwarmSheet::Team::ArcherVariantBase;
					}
				}
				else if (Flat >= 0 && Flat < SwarmSheet::Team::SpearVariants)
				{
					WithinBlock = Flat;
				}
				if (WithinBlock == INDEX_NONE)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Adapt: ladder '%s' rung %d is atlas index %d, "
						"which is not in the %s block (spearmen 0-%d, archers %d-%d). Unit %d is %s."),
						*Ladder->Id, Rung, Flat, LexToString(Type),
						SwarmSheet::Team::SpearVariants - 1, SwarmSheet::Team::ArcherVariantBase,
						SwarmSheet::Team::Variants - 1, UnitIndex, LexToString(Type));
					return;
				}

				Swarm->SetSquadRung(UnitIndex, WithinBlock, Rung);
				const float NewMaxHP = SwarmCombatTuning::TierHPOr(Tiers, Rung, 0.f);
				const int32 Touched = ReStatUnit(World, UnitIndex, NewMaxHP);
				UE_LOG(LogTemp, Display, TEXT("Kindled.Adapt: unit %d (%s) -> %s rung %d — "
					"atlas %d (block index %d), %.0f HP / %.0f DPS, %d standing body(s) re-stated."),
					UnitIndex, LexToString(Type), *Ladder->Id, Rung, Flat, WithinBlock,
					NewMaxHP, SwarmCombatTuning::TierDPSOr(Tiers, Rung, 0.f), Touched);
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

	FVector TideBearingPoint(UWorld* World, float Distance)
	{
		const USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		const FVector Bearer = Swarm ? Swarm->GetAttractor() : FVector::ZeroVector;

		// Built the same way SpawnSwarm builds the tide's own BroodFormation, and resolved
		// through the same BroodSlotOffset — a one-column, zero-degree arc's single slot IS
		// the arc's centre line. Borrowing the real function rather than re-deriving the trig
		// is what stops this drifting from the tide the day the convention changes.
		SwarmFormation::FParams Bearing;
		Bearing.Columns = 1;
		Bearing.ArcDegrees = 0.f;
		Bearing.ArcRadius = FMath::Max(Distance, 0.f);
		Bearing.RankSpacing = 1.f;
		const float BearingDeg = CVarBroodSpawnArcCenter.GetValueOnGameThread()
			+ (CVarBroodSpawnFaceCamera.GetValueOnGameThread() != 0 ? SwarmFormation::CameraYawDegrees() : 0.f);
		Bearing.YawRadians = FMath::DegreesToRadians(BearingDeg);

		const FVector2D Offset = SwarmFormation::BroodSlotOffset(0, Bearing);
		return FVector(Bearer.X + Offset.X, Bearer.Y + Offset.Y, 0.f);
	}

	void SpawnGarrison(UWorld* World, int32 Count)
	{
		FSwarmSpawnParams Params;
		Params.bBrood = false;
		Params.Count = Count;
		Params.ForceUnit = USwarmSubsystem::GarrisonUnit;
		SpawnSwarm(World, Params);
	}

	void SpawnNamed(UWorld* World, int32 UnitIndex, EUnitType Type, float HPScale)
	{
		FSwarmSpawnParams Params;
		Params.bBrood = false;
		Params.Count = 1;
		Params.ForceUnit = UnitIndex;
		Params.bForceType = true;
		Params.ForcedType = Type;
		Params.HPScale = HPScale;
		SpawnSwarm(World, Params);
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
