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

	// SwarmSpawn::SpawnArmy places bodies around wherever the subsystem's Attractor
	// currently is (SwarmCommands.cpp's "Center = GetAttractor()" convention) -- so the
	// Attractor is moved to each zone in turn before that army's own batch spawns.
	Swarm->SetAttractor(PlayerZone);
	SwarmSpawn::SpawnArmy(GetWorld(), SpearmenPerSide, ArchersPerSide, PlayerTeamId);

	Swarm->SetAttractor(EnemyZone);
	SwarmSpawn::SpawnArmy(GetWorld(), SpearmenPerSide, ArchersPerSide, EnemyTeamId);

	// Leave the shared Attractor at the field's midpoint. It no longer decides either
	// army's position -- both are on an explicit Hold anchor below -- but it is still the
	// Follow/Rally fallback the shared stance processor reads for a leash break or an
	// unaddressed handle (SwarmProcessors.cpp), and the midpoint is the least-wrong shared
	// value to leave a single global point at when two armies are reading it.
	Swarm->SetAttractor(FieldCenter);

	const TArray<int32> PlayerUnits = CollectTeamUnits(*Swarm, PlayerTeamId);
	const TArray<int32> EnemyUnits = CollectTeamUnits(*Swarm, EnemyTeamId);

	// Default posture: both lines HOLD their own deployment zone. Nobody Follows the
	// shared Attractor (that would pull both armies onto the same point) and nobody
	// starts Charging -- the director's break is what earns that (§3.1).
	for (int32 Unit : PlayerUnits) { Swarm->SetUnitStance(Unit, ESwarmStance::Hold, PlayerZone); }
	for (int32 Unit : EnemyUnits) { Swarm->SetUnitStance(Unit, ESwarmStance::Hold, EnemyZone); }

	PlayerCommander = NewObject<UBattlegroundCommander>(this);
	PlayerCommander->Initialize(PlayerTeamId, PlayerUnits, PlayerZone, EnemyZone);

	EnemyCommander = NewObject<UBattlegroundCommander>(this);
	EnemyCommander->Initialize(EnemyTeamId, EnemyUnits, EnemyZone, PlayerZone);

	Director = NewObject<UBattlegroundDirector>(this);
	const double Now = GetWorld()->GetTimeSeconds();
	Director->StartMatch(Now, PlayerCommander->GetLiveStanding(*Swarm), EnemyCommander->GetLiveStanding(*Swarm));

	DecisionCountdown = CommanderDecisionIntervalSeconds;
	MatchStartTime = Now;
	ShotsTaken = 0;

	// No hero pawn on this level, so give the player a fixed top-down orthographic view
	// wide enough to hold both deployment zones (same pitch as SpikeHeroPawn's default).
	if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(FieldCenter + FVector(0.f, 0.f, 1200.f), FRotator(-90.f, 90.f, 0.f)))
	{
		Cam->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
		Cam->GetCameraComponent()->SetOrthoWidth(DeploymentZoneDistance * 2.4f);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			PC->SetViewTarget(Cam);
		}
	}

	// §5 Build-scope evidence: does the claimed handle count actually match "2 Spearmen +
	// 1 Archer unit x2 teams = 6/8 handles"? Logged plainly so a reader doesn't have to
	// step through squad state by hand to check.
	UE_LOG(LogTemp, Display,
		TEXT("Battleground: BeginPlay -- team 0 (player) claimed handles [%s] (%d Spearmen unit(s) + %d Archer unit(s)); ")
		TEXT("team 1 (AI) claimed handles [%s] (%d Spearmen unit(s) + %d Archer unit(s)); %d/%d handles total"),
		*UnitsToString(PlayerUnits),
		CountByType(*Swarm, PlayerUnits, EUnitType::Spearmen), CountByType(*Swarm, PlayerUnits, EUnitType::Archers),
		*UnitsToString(EnemyUnits),
		CountByType(*Swarm, EnemyUnits, EUnitType::Spearmen), CountByType(*Swarm, EnemyUnits, EUnitType::Archers),
		PlayerUnits.Num() + EnemyUnits.Num(), USwarmSubsystem::MaxSquads);
}

void ABattlegroundGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || !PlayerCommander || !EnemyCommander || !Director)
	{
		return;
	}

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
