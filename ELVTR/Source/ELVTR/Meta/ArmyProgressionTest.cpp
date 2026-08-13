#include "Meta/ArmyProgression.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Pure-function coverage for the lifetime-kills -> ArmyLevel curve. This is the one
 * piece of the meta-progression system testable without a live World/GameInstance --
 * see UArmyProgressionSubsystem for the save/tick side, which needs PIE to exercise.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArmyProgressionLevelCurveTest, "Kindled.ArmyProgression.LevelCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FArmyProgressionLevelCurveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Zero kills is level 1, not level 0"),
		ArmyProgression::ComputeLevelFromLifetimeKills(0), 1);

	TestEqual(TEXT("Negative input clamps to zero kills"),
		ArmyProgression::ComputeLevelFromLifetimeKills(-500), 1);

	// Monotonic non-decreasing: never falls as lifetime kills climb, across a wide
	// sweep including plenty of non-round boundary points sqrt() rounding could trip on.
	int32 PrevLevel = ArmyProgression::ComputeLevelFromLifetimeKills(0);
	for (int64 Kills = 1; Kills <= 200000; Kills += 37)
	{
		const int32 Level = ArmyProgression::ComputeLevelFromLifetimeKills(Kills);
		if (!TestTrue(FString::Printf(TEXT("Level never falls (kills=%lld)"), Kills), Level >= PrevLevel))
		{
			break;
		}
		PrevLevel = Level;
	}

	// A concrete known point, so a future curve change is a deliberate edit, not silent drift.
	TestEqual(TEXT("50 kills is exactly level 2 (curve's own unit)"),
		ArmyProgression::ComputeLevelFromLifetimeKills(50), 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
