#include "StressWarSide.h"

#include "Battleground/BattlegroundCommander.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"

void UStressWarSide::Muster(USwarmSubsystem& Swarm, uint8 InTeamId, int32 FirstHandle, int32 Companies_,
	int32 BodiesPerHandle, int32 InReserve, int32 MeleeLook, int32 ArcherLook, float LeadHPScale,
	const FVector& InHomeZone, const FVector& EnemyZone)
{
	TeamId = InTeamId;
	HomeZone = InHomeZone;
	EnemyHome = EnemyZone;
	Reserve = InReserve;
	StartPerHandle = BodiesPerHandle;
	Companies.Reset();
	Handles.Reset();
	TroopHandles.Reset();

	UWorld* World = Swarm.GetWorld();
	Swarm.SetAttractor(HomeZone); // SpawnUnit/SpawnNamed land bodies around the Attractor
	int32 Handle = FirstHandle;
	for (int32 c = 0; c < Companies_; ++c)
	{
		if (Handle + HandlesPerCompany > USwarmSubsystem::MaxSquads) { break; }
		TArray<int32> CompanyHandles;

		// Company lead: one body on its own handle (the seven's own path), HP-scaled the same
		// prototype way SpawnNamed documents. Rung BEFORE spawn: HP is baked from it.
		Swarm.SetSquadRung(Handle, MeleeLook, INDEX_NONE);
		SwarmSpawn::SpawnNamed(World, Handle, EUnitType::Spearmen, LeadHPScale, TeamId);
		Swarm.SetUnitStance(Handle, ESwarmStance::Charge, EnemyZone);
		CompanyHandles.Add(Handle);
		Handles.Add(Handle++);

		for (EUnitType Type : { EUnitType::Spearmen, EUnitType::Archers })
		{
			Swarm.SetSquadRung(Handle, Type == EUnitType::Archers ? ArcherLook : MeleeLook, INDEX_NONE);
			SwarmSpawn::SpawnUnit(World, Handle, Type, BodiesPerHandle, TeamId);
			Swarm.SetUnitStance(Handle, ESwarmStance::Charge, EnemyZone);
			CompanyHandles.Add(Handle);
			TroopHandles.Add(Handle);
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
			Company->Order(Swarm, ESwarmStance::Charge, ClampToCorridor(EnemyCentroid + Advance * Overshoot));
		}
		else { Company->Order(Swarm, ESwarmStance::Hold, Company->GetLiveCentroid(Swarm)); }
	}

	// Reinforcement: any handle under the floor is refilled from the reserve, at home.
	if (Reserve <= 0) { return; }
	const int32 Floor = FMath::CeilToInt(StartPerHandle * ReinforceFloor);
	const FVector SavedAttractor = Swarm.GetAttractor();
	Swarm.SetAttractor(HomeZone);
	for (int32 Handle : TroopHandles)
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

FVector UStressWarSide::ClampToCorridor(const FVector& Aim) const
{
	// Charge is slot-tethered (anchor + formation slot) and every slot offset points the SAME
	// world way for both teams -- no per-army formation yaw exists. So "enemy centroid +
	// overshoot" feeds each side's own slot offset back into the next order and BOTH armies
	// walk off the map together, side B outrunning side A (measured 2026-08-19, 5000v5000:
	// centroids at X ~+27000 from an 8000uu field, gap growing, A shot in the back). Bounding
	// the aim to the corridor between the two home zones kills the feedback: the aim can lead
	// the enemy, never leave the field.
	const FVector Axis = EnemyHome - HomeZone;
	const float Length = Axis.Size2D();
	if (Length <= KINDA_SMALL_NUMBER) { return Aim; }
	const FVector Dir = Axis / Length;
	const float Along = FVector::DotProduct(Aim - HomeZone, Dir);
	return Aim + Dir * (FMath::Clamp(Along, 0.f, Length) - Along);
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
