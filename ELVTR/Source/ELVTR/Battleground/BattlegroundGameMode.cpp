#include "BattlegroundGameMode.h"

#include "BattlegroundCommander.h"
#include "BattlegroundDirector.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "HAL/IConsoleManager.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"

namespace
{
	FString UnitsToString(const TArray<int32>& Units)
	{
		FString Out;
		for (int32 i = 0; i < Units.Num(); ++i)
		{
			if (i > 0) { Out += TEXT(","); }
			Out += FString::FromInt(Units[i]);
		}
		return Out.IsEmpty() ? TEXT("(none)") : Out;
	}

	int32 CountByType(const USwarmSubsystem& Swarm, const TArray<int32>& Units, EUnitType Type)
	{
		int32 Count = 0;
		for (int32 Unit : Units)
		{
			if (Swarm.GetSquadType(Unit) == Type) { ++Count; }
		}
		return Count;
	}

	TArray<int32> CollectTeamUnits(const USwarmSubsystem& Swarm, uint8 TeamId)
	{
		TArray<int32> Units;
		for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
		{
			if (Swarm.IsSquadClaimed(i) && Swarm.GetSquadArmy(i) == TeamId)
			{
				Units.Add(i);
			}
		}
		return Units;
	}

	// The player's own order path (docs/design/battleground.md §4): "zero new input
	// surface... the exact same stance-wheel and muster-card path the seven already use."
	// This level has no hero pawn and no muster-card widget, so — same precedent
	// Swarm.UnitStance already sets in SwarmCommands.cpp ("the stand-in console command
	// until [muster card] lands") — a console command is that path's prototype surface
	// here too, addressed to the player's own team only. It NEVER touches the enemy
	// commander's handles, which is what keeps §3.3's "never overrides the player's own
	// side" true in the other direction as well: nothing the player types can order the AI.
	// Unattended evidence: with Battleground.AutoShots 1 (pass -ExecCmds="Battleground.AutoShots 1"
	// on a -game launch) the game mode saves a screenshot at fixed match times to
	// Saved/Screenshots/ -- no window focus, no editor tools required.
	TAutoConsoleVariable<int32> CVarBattlegroundAutoShots(
		TEXT("Battleground.AutoShots"), 0,
		TEXT("1 = save screenshots at 8s, 30s and 45s of the match to Saved/Screenshots/."));

	FAutoConsoleCommandWithWorldAndArgs GBattlegroundOrderCmd(
		TEXT("Battleground.Order"),
		TEXT("Issue a stance order to the PLAYER's own Battleground army only -- the AI\n")
		TEXT("commander is untouched (docs/design/battleground.md §3.3, §4).\n")
		TEXT("Usage: Battleground.Order Follow|Charge|Hold|Rally"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				ABattlegroundGameMode* GameMode = World ? World->GetAuthGameMode<ABattlegroundGameMode>() : nullptr;
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				UBattlegroundCommander* Player = GameMode ? GameMode->GetPlayerCommander() : nullptr;
				UBattlegroundCommander* Enemy = GameMode ? GameMode->GetEnemyCommander() : nullptr;
				if (!Swarm || !Player || !Enemy || Args.Num() == 0)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("Battleground.Order: usage Follow|Charge|Hold|Rally (is L_Battleground loaded and playing?)"));
					return;
				}

				const FString& Name = Args[0];
				ESwarmStance Stance = ESwarmStance::Follow;
				if (Name.Equals(TEXT("Charge"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Charge; }
				else if (Name.Equals(TEXT("Hold"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Hold; }
				else if (Name.Equals(TEXT("Rally"), ESearchCase::IgnoreCase)) { Stance = ESwarmStance::Rally; }

				// Charge aims at wherever the enemy line actually stands RIGHT NOW, captured
				// once at issue time (SetStance's own doc comment: "a one-shot intent, not a
				// thing that drags behind the hero"). Every other verb anchors on the
				// player's own deployment zone.
				const FVector Anchor = (Stance == ESwarmStance::Charge)
					? Enemy->GetLiveCentroid(*Swarm)
					: Player->GetHomeZone();

				UE_LOG(LogTemp, Display, TEXT("Battleground.Order: player orders %s"), LexToString(Stance));
				Player->SetManual(true); // autopilot off from the first manual order
				for (int32 Unit : Player->GetUnitHandles())
				{
					Swarm->SetUnitStance(Unit, Stance, Anchor);
				}
			}));
}

ABattlegroundGameMode::ABattlegroundGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	// No hero pawn: this testbed's player input is the Battleground.Order console command
	// above, not a possessed character (§4 asks for zero new input surface, and building
	// a stance-wheel pawn here would be exactly that on a level that doesn't need one).
	DefaultPawnClass = nullptr;
}

void ABattlegroundGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartMatch();
}

void ABattlegroundGameMode::StartMatch()
{
	USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		UE_LOG(LogTemp, Error, TEXT("Battleground: no USwarmSubsystem -- is this level's world set up?"));
		return;
	}

	SwarmSpawn::ClearAll(GetWorld());
	Swarm->ResetRunState();
	Swarm->SetFieldBattle(true);

	const FVector FieldCenter = FVector::ZeroVector;
	const FVector PlayerZone = FieldCenter - FVector(DeploymentZoneDistance * 0.5f, 0.f, 0.f);
	const FVector EnemyZone = FieldCenter + FVector(DeploymentZoneDistance * 0.5f, 0.f, 0.f);

	// Formation + look dials for this level (owner, 2026-08-16): every unit is a 6x4 block,
	// the line faces +X (toward the Ooze) regardless of camera, and only the looks the owner
	// filed under the MELEE / RANGED palettes on the official page are drawn. Team-atlas
	// order is retinue,v1,v2,v3,v4,v6,v7,v8,v10,v11,v13 -> MELEE = v8,v10,v13; archer order
	// starts hoodedbow,v2_bowextended -> RANGED = those two. (Four more MELEE/RANGED looks
	// exist but are not in the packed atlas yet -- needs a repack + editor import.)
	auto SetCVar = [](const TCHAR* Name, const FString& Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) { V->Set(*Value, ECVF_SetByConsole); }
		else { UE_LOG(LogTemp, Warning, TEXT("Battleground: no CVar %s"), Name); }
	};
	SetCVar(TEXT("Swarm.Formation.Shape"), TEXT("1"));
	SetCVar(TEXT("Swarm.Formation.Columns"), FString::FromInt(UnitWidth));
	SetCVar(TEXT("Swarm.Formation.Archers.Shape"), TEXT("1"));
	SetCVar(TEXT("Swarm.Formation.Archers.Columns"), FString::FromInt(UnitWidth));
	// Square blocks: ranks as close as files, big gap between units.
	SetCVar(TEXT("Swarm.Formation.Spacing"), TEXT("48"));
	SetCVar(TEXT("Swarm.Formation.RankSpacing"), TEXT("52"));
	SetCVar(TEXT("Swarm.Formation.Archers.Spacing"), TEXT("48"));
	SetCVar(TEXT("Swarm.Formation.Archers.RankSpacing"), TEXT("52"));
	SetCVar(TEXT("Swarm.Formation.GroupGap"), TEXT("110"));
	SetCVar(TEXT("Swarm.Formation.GroupsPerRow"), TEXT("0"));
	SetCVar(TEXT("Swarm.Formation.GroupDepthCap"), TEXT("16"));
	SetCVar(TEXT("Swarm.Formation.GroupRowPitch"), TEXT("650"));
	SetCVar(TEXT("Swarm.Formation.FaceCamera"), TEXT("0"));
	SetCVar(TEXT("Swarm.Formation.Yaw"), TEXT("0"));
	// Team atlas (docs/data/art/requests/team-units.json), 2026-08-16 swap: idx 1 gs1_seed
	// (Sword), 2 gs2_sweep (Sword), 3 mass_v4_pike (Spear), 7 v8_heavycloak (Sword),
	// 8 v10_bracedstaff (Spear), 10 v13_maceraised (Mace) = the owner's MELEE palette.
	// Archer block: 0 hoodedbow (Merle), 1 v2_bowextended, 2 v1_narrowstrung = RANGED.
	SetCVar(TEXT("Swarm.TeamVariantWeights"), TEXT("0,1,1,1,0,0,0,1,1,0,1"));
	SetCVar(TEXT("Swarm.ArcherVariantWeights"), TEXT("1,1,1,0,0,0,0,0,0,0,0,0,0"));

	// Per-WEAPON stats (owner: "each of the weapon units should have differing stats for
	// damage"), through task-095's look->row binding. Rows: 0 Sword, 1 Spear, 2 Mace.
	// Sword = the baseline knight; Spear = less damage, more reach; Mace = heavy hitter,
	// short reach, fewer targets. Map is per team-atlas index (order above).
	SetCVar(TEXT("Swarm.KnightSubtypeMap"),     TEXT("0,0,0,1,0,0,0,0,1,0,2"));
	SetCVar(TEXT("Swarm.KnightSubtypeHP"),      TEXT("140,130,150"));
	SetCVar(TEXT("Swarm.KnightSubtypeDPS"),     TEXT("36,28,44"));
	SetCVar(TEXT("Swarm.KnightSubtypeEngage"),  TEXT("95,125,85"));
	SetCVar(TEXT("Swarm.KnightSubtypeTargets"), TEXT("8,8,6"));
	// Uniform blocks (owner, 2026-08-16): no per-soldier size roll, archers same size as
	// spearmen. SwarmExecOnPlay.txt re-enables jitter for the castle; this level overrides.
	SetCVar(TEXT("Swarm.RetinueSizeJitter"), TEXT("0"));
	SetCVar(TEXT("Swarm.ArcherSizeScale"), TEXT("1"));
	SetCVar(TEXT("Swarm.BroodSpawnFaceCamera"), TEXT("0"));
	SetCVar(TEXT("Swarm.BroodSpawnArcCenter"), TEXT("0"));
	SetCVar(TEXT("Swarm.BroodSpawnArc"), TEXT("30"));
	SetCVar(TEXT("Swarm.BroodSpawnRadiusMin"), FString::SanitizeFloat(DeploymentZoneDistance));
	SetCVar(TEXT("Swarm.BroodFormation.Columns"), FString::FromInt(UnitWidth * 4));

	// Ally army: one squad handle per formation, exactly UnitWidth*UnitDepth bodies each
	// (ForceUnit bypasses AssignRecruit's legibility split -- SpawnNamed/SpawnGarrison
	// precedent). Bodies spawn around the Attractor, so park it on the deployment zone.
	Swarm->SetAttractor(PlayerZone);
	TArray<int32> PlayerUnits;
	{
		// One block per sprite: unit u wears exactly one look (SetSquadRung pins the render
		// variant; tier INDEX_NONE keeps HP/DPS on the look's weapon row).
		int32 u = 0;
		for (int32 Look : MeleeLooks)
		{
			if (u >= USwarmSubsystem::MaxSquads) { break; }
			Swarm->SetSquadRung(u, Look, INDEX_NONE);
			SwarmSpawn::SpawnUnit(GetWorld(), u, EUnitType::Spearmen, FMath::Max(1, UnitWidth * UnitDepth), PlayerTeamId);
			PlayerUnits.Add(u++);
		}
		for (int32 Look : RangedLooks)
		{
			if (u >= USwarmSubsystem::MaxSquads) { UE_LOG(LogTemp, Warning, TEXT("Battleground: out of squad handles, ranged look %d skipped"), Look); continue; }
			Swarm->SetSquadRung(u, Look, INDEX_NONE);
			SwarmSpawn::SpawnUnit(GetWorld(), u, EUnitType::Archers, FMath::Max(1, UnitWidth * UnitDepth), PlayerTeamId);
			PlayerUnits.Add(u++);
		}
	}

	// Ooze: the brood tide, in ranks on the +X arc DeploymentZoneDistance out from the
	// Attractor (i.e. on the enemy zone), hunting the Attractor -- which Tick keeps on the
	// ally army's live centroid, so "find and start a fight" is the brood's own steering.
	SwarmSpawn::SpawnBrood(GetWorld(), OozeCount);

	// Formations hold their zone until the director's break; each one is its own unit and
	// answers its own handle (Battleground.Order addresses all of the player's at once).
	for (int32 Unit : PlayerUnits) { Swarm->SetUnitStance(Unit, ESwarmStance::Hold, PlayerZone); }
	const TArray<int32> EnemyUnits; // brood: no handles

	PlayerCommander = NewObject<UBattlegroundCommander>(this);
	PlayerCommander->Initialize(PlayerTeamId, PlayerUnits, PlayerZone, EnemyZone);

	EnemyCommander = NewObject<UBattlegroundCommander>(this);
	EnemyCommander->InitializeBrood(EnemyTeamId, EnemyZone, PlayerZone);

	Director = NewObject<UBattlegroundDirector>(this);
	const double Now = GetWorld()->GetTimeSeconds();
	Director->StartMatch(Now, PlayerCommander->GetLiveStanding(*Swarm), EnemyCommander->GetLiveStanding(*Swarm));

	DecisionCountdown = CommanderDecisionIntervalSeconds;
	MatchStartTime = Now;
	ShotsTaken = 0;

	// No hero pawn on this level, so give the player a fixed top-down orthographic view
	// wide enough to hold both deployment zones (same pitch as SpikeHeroPawn's default).
	if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>((PlayerZone + FieldCenter) * 0.5f + FVector(0.f, 0.f, 1200.f), FRotator(-90.f, 90.f, 0.f)))
	{
		Cam->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
		Cam->GetCameraComponent()->SetOrthoWidth(DeploymentZoneDistance * 2.0f);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			PC->SetViewTarget(Cam);
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("Battleground: BeginPlay -- army: %d formations of %dx%d on handles [%s] (%d melee + %d ranged); Ooze: %d in ranks"),
		PlayerUnits.Num(), UnitWidth, UnitDepth, *UnitsToString(PlayerUnits),
		CountByType(*Swarm, PlayerUnits, EUnitType::Spearmen), CountByType(*Swarm, PlayerUnits, EUnitType::Archers),
		OozeCount);
}

void ABattlegroundGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || !PlayerCommander || !EnemyCommander || !Director)
	{
		return;
	}

	// The brood hunts the Attractor; keep it on the army so the Ooze finds the fight.
	Swarm->SetAttractor(PlayerCommander->GetLiveCentroid(*Swarm));

	// Decision cadence, not per-frame (§2.1: "recommend 1.5-2s, matching the LookLerp/
	// pacing cadence elsewhere in this codebase rather than inventing a new one").
	DecisionCountdown -= DeltaSeconds;
	if (DecisionCountdown > 0.f)
	{
		return;
	}
	DecisionCountdown = CommanderDecisionIntervalSeconds;

	const double Now = GetWorld()->GetTimeSeconds();

	if (CVarBattlegroundAutoShots.GetValueOnGameThread() != 0)
	{
		static const float ShotTimes[] = { 8.f, 30.f, 45.f };
		const float Elapsed = static_cast<float>(Now - MatchStartTime);
		for (int32 i = 0; i < 3; ++i)
		{
			if (!(ShotsTaken & (1 << i)) && Elapsed >= ShotTimes[i])
			{
				ShotsTaken |= (1 << i);
				FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("battleground_%02ds.png"), (int32)ShotTimes[i]), false, false);
			}
		}
	}

	// Only the AI team runs a decision loop (§4) -- the player's own team's stance comes
	// from the Battleground.Order console command instead, never from this tick.
	EnemyCommander->Decide(*Swarm, *Director, PlayerCommander->GetLiveCentroid(*Swarm));
	// Player side runs the same autopilot until the first Battleground.Order arrives, so an
	// unattended run still produces a battle; SetManual(true) silences this permanently.
	PlayerCommander->Decide(*Swarm, *Director, EnemyCommander->GetLiveCentroid(*Swarm));

	const int32 PlayerStanding = PlayerCommander->GetLiveStanding(*Swarm);
	const int32 EnemyStanding = EnemyCommander->GetLiveStanding(*Swarm);
	{
		const FVector PC0 = PlayerCommander->GetLiveCentroid(*Swarm);
		const FVector PC1 = EnemyCommander->GetLiveCentroid(*Swarm);
		UE_LOG(LogTemp, Display, TEXT("Battleground: t=%.1f standing %d vs %d, centroids (%.0f,%.0f) vs (%.0f,%.0f)"),
			Now - MatchStartTime, PlayerStanding, EnemyStanding, PC0.X, PC0.Y, PC1.X, PC1.Y);
	}
	const bool bBreakNow = Director->Update(Now, PlayerStanding, EnemyStanding);
	if (bBreakNow)
	{
		// §3.3: the back-channel governs the AI commander's own pacing only -- it must
		// never intercept or veto a stance order the player issues, so ONLY the enemy
		// commander is ever forced here. The player's line sees the enemy suddenly commit
		// to a push (a diegetic "the enemy just decided something", per the spec's
		// narrative request) and can answer with their own Battleground.Order Charge --
		// that player answer, not this call, is what makes both formations visibly close
		// together for the §5 evidence screenshot.
		EnemyCommander->ForceBreakCharge(*Swarm, PlayerCommander->GetLiveCentroid(*Swarm));
		if (!PlayerCommander->IsManual())
		{
			PlayerCommander->ForceBreakCharge(*Swarm, EnemyCommander->GetLiveCentroid(*Swarm));
		}
	}
}
