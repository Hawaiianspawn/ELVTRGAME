#include "StressWarSide.h"

#include "Battleground/BattlegroundCommander.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"

void UStressWarSide::Muster(USwarmSubsystem& Swarm, uint8 InTeamId, int32 FirstHandle, int32 Companies_,
	int32 BodiesPerHandle, int32 InReserve, int32 MeleeLook, int32 ArcherLook,
	const FVector& InHomeZone, const FVector& EnemyZone)
{
	TeamId = InTeamId;
	HomeZone = InHomeZone;
	Reserve = InReserve;
	StartPerHandle = BodiesPerHandle;
	Companies.Reset();
	Handles.Reset();

	UWorld* World = Swarm.GetWorld();
	Swarm.SetAttractor(HomeZone); // SpawnUnit lands bodies around the Attractor
	int32 Handle = FirstHandle;
	for (int32 c = 0; c < Companies_; ++c)
	{
		TArray<int32> CompanyHandles;
		for (EUnitType Type : { EUnitType::Spearmen, EUnitType::Archers })
		{
			if (Handle >= USwarmSubsystem::MaxSquads) { break; }
			// Rung BEFORE spawn: HP is baked from it (SwarmCommands.cpp). Tier INDEX_NONE keeps
			// stats on the look's own weapon row.
			Swarm.SetSquadRung(Handle, Type == EUnitType::Archers ? ArcherLook : MeleeLook, INDEX_NONE);
			SwarmSpawn::SpawnUnit(World, Handle, Type, BodiesPerHandle, TeamId);
			Swarm.SetUnitStance(Handle, ESwarmStance::Charge, EnemyZone);
			CompanyHandles.Add(Handle);
			Handles.Add(Handle++);
		}
		UBattlegroundCommander* Company = NewObject<UBattlegroundCommander>(this);
		Company->Initialize(TeamId, CompanyHandles, HomeZone, EnemyZone);
		Companies.Add(Company);
	}
}

void UStressWarSide::Decide(USwarmSubsystem& Swarm, const FVector& EnemyCentroid, int32 EnemyStanding, float Overshoot, float ReinforceFloor)
{
	// Orders: charge through the enemy while there is one; hold where we stand once there is
	// not. Field-battle Charge is slot-tethered (SwarmProcessors.cpp: anchor + slot, engage
	// within BlockEngageRange*2), so a block that REACHES the enemy centroid parks there --
	// aiming Overshoot past it keeps both blocks moving through each other, which is the
	// contact this level exists to measure.
	for (UBattlegroundCommander* Company : Companies)
	{
		if (EnemyStanding > 0)
		{
			const FVector Advance = (EnemyCentroid - Company->GetLiveCentroid(Swarm)).GetSafeNormal2D();
			Company->Order(Swarm, ESwarmStance::Charge, EnemyCentroid + Advance * Overshoot);
		}
		else { Company->Order(Swarm, ESwarmStance::Hold, Company->GetLiveCentroid(Swarm)); }
	}

	// Reinforcement: any handle under the floor is refilled from the reserve, at home.
	if (Reserve <= 0) { return; }
	const int32 Floor = FMath::CeilToInt(StartPerHandle * ReinforceFloor);
	const FVector SavedAttractor = Swarm.GetAttractor();
	Swarm.SetAttractor(HomeZone);
	for (int32 Handle : Handles)
	{
		const int32 Deficit = StartPerHandle - Swarm.GetSquadStanding(Handle);
		if (Swarm.GetSquadStanding(Handle) >= Floor || Deficit <= 0 || Reserve <= 0) { continue; }
		const int32 Count = FMath::Min(Deficit, Reserve);
		SwarmSpawn::SpawnUnit(Swarm.GetWorld(), Handle, Swarm.GetSquadType(Handle), Count, TeamId);
		Reserve -= Count;
		UE_LOG(LogTemp, Display, TEXT("StressWar: team %d reinforces handle %d +%d (reserve %d)"), TeamId, Handle, Count, Reserve);
	}
	Swarm.SetAttractor(SavedAttractor);
}

int32 UStressWarSide::LiveStanding(const USwarmSubsystem& Swarm) const
{
	int32 N = 0;
	for (int32 Handle : Handles) { N += Swarm.GetSquadStanding(Handle); }
	return N;
}

FVector UStressWarSide::LiveCentroid(const USwarmSubsystem& Swarm) const
{
	FVector Sum = FVector::ZeroVector;
	int32 N = 0;
	for (int32 Handle : Handles)
	{
		const int32 S = Swarm.GetSquadStanding(Handle);
		if (S <= 0) { continue; }
		Sum += Swarm.GetSquadCentroid(Handle) * (float)S;
		N += S;
	}
	return N > 0 ? Sum / (float)N : HomeZone;
}
