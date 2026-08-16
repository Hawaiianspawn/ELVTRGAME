#include "BattlegroundCommander.h"

#include "BattlegroundDirector.h"
#include "Mass/SwarmSubsystem.h"
#include "Mass/SwarmFragments.h"

void UBattlegroundCommander::Initialize(uint8 InTeamId, const TArray<int32>& InUnitHandles,
	const FVector& InHomeZone, const FVector& InEnemyZone)
{
	TeamId = InTeamId;
	UnitHandles = InUnitHandles;
	HomeZone = InHomeZone;
	EnemyZone = InEnemyZone;
}

void UBattlegroundCommander::InitializeBrood(uint8 InTeamId, const FVector& InHomeZone, const FVector& InEnemyZone)
{
	TeamId = InTeamId;
	UnitHandles.Reset();
	HomeZone = InHomeZone;
	EnemyZone = InEnemyZone;
	bBrood = true;
}

int32 UBattlegroundCommander::GetLiveStanding(const USwarmSubsystem& Swarm) const
{
	if (bBrood)
	{
		return Swarm.GetAliveBrood();
	}
	int32 Total = 0;
	for (int32 Unit : UnitHandles)
	{
		Total += Swarm.GetSquadStanding(Unit);
	}
	return Total;
}

FVector UBattlegroundCommander::GetLiveCentroid(const USwarmSubsystem& Swarm) const
{
	if (bBrood)
	{
		// Brood have no handles; average the render positions that carry no TeamBit.
		const TArray<FVector>& Pos = Swarm.GetRenderPositions();
		const TArray<int32>& Bits = Swarm.GetRenderAnimBits();
		FVector BSum = FVector::ZeroVector;
		int32 N = 0;
		for (int32 i = 0; i < Pos.Num() && i < Bits.Num(); ++i)
		{
			if ((Bits[i] & SwarmAnim::TeamBit) == 0) { BSum += Pos[i]; ++N; }
		}
		return N > 0 ? BSum / (float)N : HomeZone;
	}
	// Standing-weighted average of each unit's own centroid, not a plain mean of centroids
	// — a 2-body archer unit must not pull the point as hard as a 20-body spearman block.
	FVector Sum = FVector::ZeroVector;
	int32 TotalStanding = 0;
	for (int32 Unit : UnitHandles)
	{
		const int32 Standing = Swarm.GetSquadStanding(Unit);
		if (Standing <= 0)
		{
			continue;
		}
		Sum += Swarm.GetSquadCentroid(Unit) * (float)Standing;
		TotalStanding += Standing;
	}
	return TotalStanding > 0 ? (Sum / (float)TotalStanding) : EnemyZone;
}

void UBattlegroundCommander::Decide(USwarmSubsystem& Swarm, UBattlegroundDirector& Director, const FVector& EnemyCentroid)
{
	if (bManual || bBrood) { return; }
	// §3.2's no-snowball floor: a losing commander is nudged toward Rally (fall back,
	// consolidate, buy time) instead of grinding in place on Hold — an order CHOICE
	// already in this army's vocabulary, not a new mechanic or a stat change. Outside
	// that, the default posture is Hold: the one scripted Charge belongs to the director's
	// break (ForceBreakCharge), not to this commander deciding to advance on its own —
	// §3.1's finding is that a mirror matchup barely produces a spontaneous swing at all,
	// so nothing here needs to invent independent aggression to avoid one.
	// Once the director's break has committed this side to a Charge, it stays committed
	// (retargeting the live enemy centroid) until the no-snowball guard calls it back.
	const ESwarmStance Stance = Director.ShouldRally(TeamId) ? ESwarmStance::Rally
		: (bCommitted ? ESwarmStance::Charge : ESwarmStance::Hold);
	const FVector Anchor = (Stance == ESwarmStance::Charge) ? EnemyCentroid : HomeZone;

	if (Stance != LastStance)
	{
		UE_LOG(LogTemp, Display,
			TEXT("Battleground: commander %d orders %s (standing %d, tension %.2f)"),
			TeamId, LexToString(Stance), GetLiveStanding(Swarm), Director.GetTensionCurveTarget());
	}
	LastStance = Stance;

	for (int32 Unit : UnitHandles)
	{
		Swarm.SetUnitStance(Unit, Stance, Anchor);
	}
}

void UBattlegroundCommander::ForceBreakCharge(USwarmSubsystem& Swarm, const FVector& EnemyCentroid)
{
	if (bBrood) { return; } // the tide is already coming
	LastStance = ESwarmStance::Charge;
	bCommitted = true;
	UE_LOG(LogTemp, Display,
		TEXT("Battleground: commander %d BREAK — coordinated Charge on (%.0f, %.0f)"),
		TeamId, EnemyCentroid.X, EnemyCentroid.Y);
	for (int32 Unit : UnitHandles)
	{
		Swarm.SetUnitStance(Unit, ESwarmStance::Charge, EnemyCentroid);
	}
}
