#include "BattlegroundDirector.h"

void UBattlegroundDirector::StartMatch(double NowSeconds, int32 StartingA, int32 StartingB)
{
	MatchStartSeconds = NowSeconds;
	StartingStandingByTeam[0] = LastStandingByTeam[0] = StartingA;
	StartingStandingByTeam[1] = LastStandingByTeam[1] = StartingB;
	bBreakFired = false;
	TensionCurveTarget = 0.f;

	UE_LOG(LogTemp, Display,
		TEXT("Battleground: director armed — %d vs %d starting, break at %.0f%% of %.0fs"),
		StartingA, StartingB, BreakAtFraction * 100.f, ExpectedDurationSeconds);
}

bool UBattlegroundDirector::Update(double NowSeconds, int32 StandingA, int32 StandingB)
{
	const float Elapsed = (float)(NowSeconds - MatchStartSeconds);
	TensionCurveTarget = ExpectedDurationSeconds > 0.f
		? FMath::Clamp(Elapsed / ExpectedDurationSeconds, 0.f, 1.f)
		: 1.f;

	LastStandingByTeam[0] = StandingA;
	LastStandingByTeam[1] = StandingB;

	UE_LOG(LogTemp, Verbose,
		TEXT("Battleground: director tick — tension %.2f, standing %d vs %d"),
		TensionCurveTarget, StandingA, StandingB);

	const bool bShouldBreakNow = !bBreakFired && TensionCurveTarget >= BreakAtFraction;
	if (bShouldBreakNow)
	{
		bBreakFired = true;
		UE_LOG(LogTemp, Display,
			TEXT("Battleground: director BREAK — tension %.2f crossed %.2f at %.1fs, standing %d vs %d"),
			TensionCurveTarget, BreakAtFraction, Elapsed, StandingA, StandingB);
	}
	return bShouldBreakNow;
}

bool UBattlegroundDirector::ShouldRally(uint8 TeamId) const
{
	const int32 Mine = LastStandingByTeam[TeamId & 1];
	const int32 Theirs = LastStandingByTeam[(TeamId & 1) ^ 1];
	if (Mine <= 0 || Theirs <= 0)
	{
		return false;
	}
	const float MyShare = (float)Mine / (float)(Mine + Theirs);
	const bool bRally = MyShare < (1.f - NoSnowballWinnerShare);
	if (bRally)
	{
		UE_LOG(LogTemp, Display,
			TEXT("Battleground: director no-snowball guard — team %d holds only %.0f%% of the standing line, nudging Rally"),
			TeamId, MyShare * 100.f);
	}
	return bRally;
}
