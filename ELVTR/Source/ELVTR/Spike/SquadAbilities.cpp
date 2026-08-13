#include "SquadAbilities.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Mass/SwarmSubsystem.h"
#include "SevenRoster.h"

namespace
{
	/** The seven's live centroids are one body each, so a centroid IS a position. */
	FVector SoldierAt(const USwarmSubsystem& Swarm, int32 UnitIndex)
	{
		return Swarm.GetSquadCentroid(UnitIndex);
	}

	bool IsStanding(const USwarmSubsystem& Swarm, int32 UnitIndex)
	{
		return USwarmSubsystem::IsNamedUnit(UnitIndex) && Swarm.GetSquadStanding(UnitIndex) > 0;
	}

	const TCHAR* SchemeName(SquadAbilities::EOrderScheme S)
	{
		switch (S)
		{
		case SquadAbilities::EOrderScheme::Wheel:			return TEXT("Q26=D wheel");
		case SquadAbilities::EOrderScheme::SelectThenOrder:	return TEXT("Q26=B select-then-order");
		case SquadAbilities::EOrderScheme::DirectTarget:	return TEXT("Q26=A direct-target");
		default:											return TEXT("console");
		}
	}

	/** Refusals are counted and logged, never swallowed — ability-kit.md §5 asks to watch for
	 *  wrong-verb activations and for a player reaching for something the shape cannot express,
	 *  and both of those arrive as a refusal before they arrive as a feeling. */
	bool Refuse(USwarmSubsystem& Swarm, SquadAbilities::EOrderScheme Scheme, const FString& Why)
	{
		const int32 Slot = FMath::Clamp((int32)Scheme, 0, USwarmSubsystem::FAbilityState::NumOrderSchemes - 1);
		Swarm.GetMutableAbilities().RefusalsByScheme[Slot]++;
		UE_LOG(LogTemp, Warning, TEXT("Ability REFUSED (%s): %s"), SchemeName(Scheme), *Why);
		return false;
	}
}

namespace SquadAbilities
{

ESquadVerb VerbFor(int32 UnitIndex)
{
	return USwarmSubsystem::IsNamedUnit(UnitIndex) ? SevenRoster::Get(UnitIndex).Verb : ESquadVerb::None;
}

int32 NearestNamed(const USwarmSubsystem& Swarm, const FVector& P, float MaxDist)
{
	int32 Best = INDEX_NONE;
	float BestSq = (MaxDist > 0.f) ? MaxDist * MaxDist : TNumericLimits<float>::Max();
	for (int32 i = 0; i < SevenRoster::Num; ++i)
	{
		if (!IsStanding(Swarm, i))
		{
			continue;
		}
		const float DistSq = (float)FVector::DistSquared2D(SoldierAt(Swarm, i), P);
		if (DistSq <= BestSq)
		{
			BestSq = DistSq;
			Best = i;
		}
	}
	return Best;
}

float ReadyIn(const USwarmSubsystem& Swarm, ESquadVerb Verb, int32 Caster, float Now)
{
	const USwarmSubsystem::FAbilityState& A = Swarm.GetAbilities();
	const float At = USwarmSubsystem::IsNamedUnit(Caster)
		? A.SoldierReadyAt[Caster]
		: A.VerbReadyAt[FMath::Clamp((int32)Verb, 0, NumSquadVerbs - 1)];
	return FMath::Max(At - Now, 0.f);
}

int32 ResolveDirectTarget(const USwarmSubsystem& Swarm, const FVector& CursorPoint, ESquadVerb& OutVerb)
{
	// THE WHOLE RULE, in four lines, in priority order. Small on purpose: ability-kit.md §5
	// says the thing to watch under Q26 = A is wrong-verb activations from an ambiguous click,
	// and a rule elaborate enough never to misfire would be answering Q26 by deleting the
	// question. Widen or narrow Kindled.Ability.PickRadius to make it guess more or less.
	//
	//   1. cursor on the boss           -> FOCUS   (you clicked the thing you want dead)
	//   2. cursor on a HURT soldier     -> RAISE   (you clicked someone who needs it)
	//   3. cursor on a healthy soldier  -> SCREEN  (you clicked ground your line is holding)
	//   4. cursor on open ground        -> RALLY   (you clicked somewhere to plant a banner)
	//
	// Rule 3 is the ambiguous one and is known to be: "near a soldier" and "on open ground" are
	// a hair apart at any pick radius, so screen and rally trade places under small cursor
	// movements. That is the scheme's own risk surface, left visible rather than smoothed.
	const float Pick = SwarmCombatTuning::AbilityPickRadius();

	OutVerb = ESquadVerb::Rally;
	if (Swarm.IsBossAlive()
		&& FVector::DistSquared2D(Swarm.GetBoss().Location, CursorPoint) <= Pick * Pick)
	{
		OutVerb = ESquadVerb::Focus;
	}
	else if (const int32 Near = NearestNamed(Swarm, CursorPoint, Pick); Near != INDEX_NONE)
	{
		const float Max = Swarm.GetSquadMaxHP(Near);
		const bool bHurt = Max > 0.f && (Swarm.GetSquadHP(Near) / Max) < 0.9f;
		OutVerb = bHurt ? ESquadVerb::Raise : ESquadVerb::Screen;
	}

	// Who answers: the carrier of that verb nearest the cursor, PREFERRING one that is ready.
	// That preference is the single concession made to playability — three of the four verbs
	// are doubled on the roster, and refusing because the nearer of two carriers happened to be
	// spent would make the scheme read as broken rather than as ambiguous, and ambiguity is the
	// thing under test. Everything else about the resolution is left rough on purpose.
	const float Now = Swarm.GetWorld() ? Swarm.GetWorld()->GetTimeSeconds() : 0.f;
	int32 Best = INDEX_NONE;
	float BestSq = TNumericLimits<float>::Max();
	bool bBestReady = false;
	for (int32 i = 0; i < SevenRoster::Num; ++i)
	{
		if (!IsStanding(Swarm, i) || SevenRoster::Get(i).Verb != OutVerb)
		{
			continue;
		}
		const bool bReady = Swarm.GetAbilities().SoldierReadyAt[i] <= Now;
		const float DistSq = (float)FVector::DistSquared2D(SoldierAt(Swarm, i), CursorPoint);
		if (Best == INDEX_NONE || (bReady && !bBestReady) || (bReady == bBestReady && DistSq < BestSq))
		{
			BestSq = DistSq;
			bBestReady = bReady;
			Best = i;
		}
	}
	return Best;
}

bool Cast(UWorld* World, ESquadVerb Verb, int32 Caster, const FVector& CursorPoint, EOrderScheme Scheme)
{
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return false;
	}
	const float Now = World->GetTimeSeconds();
	USwarmSubsystem::FAbilityState& A = Swarm->GetMutableAbilities();

	// Read the mode LIVE rather than taking it as an argument: Kindled.Ability.Mode is meant to
	// be flipped mid-fight, and an already-standing effect must keep behaving as the shape that
	// cast it intended (see FAbilityState's own comment). Only NEW casts see the new mode.
	const bool bModeB = SwarmCombatTuning::AbilityMode() == 1;

	// --- who is allowed to cast, and what -------------------------------------------------
	if (bModeB)
	{
		if (!USwarmSubsystem::IsNamedUnit(Caster))
		{
			return Refuse(*Swarm, Scheme, TEXT("Q23 = B is live — the bearer has no kit of his "
				"own. Address one of the seven (ZXCVBNM + LMB, or E), or flip with F."));
		}
		if (!IsStanding(*Swarm, Caster))
		{
			return Refuse(*Swarm, Scheme, FString::Printf(TEXT("%s is down — that verb is gone "
				"for the run. Nothing revives (Q15 open)."), SevenRoster::Get(Caster).Name));
		}
		Verb = VerbFor(Caster);
	}
	else
	{
		// Q23 = A: the bearer's kit, so nobody in particular casts it — but it has to land on
		// SOMEBODY, and "whichever soldiers are in range" is the register's own wording.
		Caster = INDEX_NONE;
		if (Verb == ESquadVerb::None)
		{
			return Refuse(*Swarm, Scheme, TEXT("no verb armed — hold Q, point, release."));
		}
	}

	const float Cooldown = SwarmCombatTuning::AbilityCooldown();
	if (const float Wait = ReadyIn(*Swarm, Verb, Caster, Now); Wait > 0.f)
	{
		return Refuse(*Swarm, Scheme, bModeB
			? FString::Printf(TEXT("%s is spent — %.1fs"), SevenRoster::Get(Caster).Name, Wait)
			: FString::Printf(TEXT("%s is on cooldown — %.1fs"), LexToString(Verb), Wait));
	}

	// Which of the seven this cast reaches. Under Q23 = B the caster is the answer for a zone
	// and the PACK is the answer for a mark; under Q23 = A it is whoever is in the bearer's
	// reach, for everything. This one mask is where most of the difference between the two
	// shapes actually lives.
	const FVector Bearer = Swarm->GetAttractor();
	const float PlayerRange = SwarmCombatTuning::AbilityPlayerRange();
	uint8 InReach = 0;
	int32 InReachCount = 0;
	for (int32 i = 0; i < SevenRoster::Num; ++i)
	{
		if (!IsStanding(*Swarm, i))
		{
			continue;
		}
		const bool bCounts = bModeB
			|| FVector::DistSquared2D(SoldierAt(*Swarm, i), Bearer) <= PlayerRange * PlayerRange;
		if (bCounts)
		{
			InReach |= (uint8)(1u << i);
			++InReachCount;
		}
	}
	if (!bModeB && InReachCount == 0)
	{
		return Refuse(*Swarm, Scheme, FString::Printf(
			TEXT("no soldiers within %.0fuu of the bearer — a fixed kit has nobody to act "
				"through. (Q23 = A's stated cost, on screen.)"), PlayerRange));
	}

	// --- the effect ------------------------------------------------------------------------
	FString What;
	switch (Verb)
	{
	case ESquadVerb::Focus:
	{
		// Mark Quarry targets an ENEMY, and this slice has exactly one enemy worth marking.
		// Marking a brood is not implemented and not stubbed: the boss is the only body in the
		// sim with an identity to point at, and a mark on an interchangeable brood would be a
		// damage buff wearing a verb's name.
		if (!Swarm->IsBossAlive())
		{
			return Refuse(*Swarm, Scheme, TEXT("nothing to mark — no boss standing. "
				"(Kindled.Boss.Spawn quilled+ram+sated)"));
		}
		A.FocusUntil = Now + SwarmCombatTuning::AbilityFocusSeconds();
		A.FocusUnits = InReach;

		// "The entire pack focus-fires it" is a statement about attention, not only about
		// damage — so the markers are also ORDERED onto it, through the shipped stance handle
		// rather than a second movement system. Charge at the boss is exactly what the words
		// describe and costs no new code.
		const FVector At = Swarm->GetBoss().Location;
		for (int32 i = 0; i < SevenRoster::Num; ++i)
		{
			if ((InReach & (uint8)(1u << i)) != 0)
			{
				Swarm->SetUnitStance(i, ESwarmStance::Charge, At);
			}
		}
		Swarm->SetCastFocus(At, World->GetTimeSeconds() + 1.5);
		What = FString::Printf(TEXT("%d marking %s"), InReachCount,
			*FString(Swarm->GetBoss().Marks ? TEXT("the marked boss") : TEXT("the boss")));
		break;
	}
	case ESquadVerb::Screen:
	{
		A.ScreenCentre = bModeB ? SoldierAt(*Swarm, Caster) : CursorPoint;
		A.ScreenUntil = Now + SwarmCombatTuning::AbilityScreenSeconds();
		Swarm->SetCastFocus(A.ScreenCentre, World->GetTimeSeconds() + 1.5);
		What = FString::Printf(TEXT("circle at (%.0f, %.0f), damage x%.2f for %.0fs"),
			A.ScreenCentre.X, A.ScreenCentre.Y, SwarmCombatTuning::AbilityScreenScale(),
			SwarmCombatTuning::AbilityScreenSeconds());
		break;
	}
	case ESquadVerb::Raise:
	{
		// Kindle targets an ALLY, in both shapes, by the same rule: the soldier nearest the
		// cursor. Keeping one rule across both modes is deliberate — the comparison is supposed
		// to be about the two ADDRESSINGS, so a third difference smuggled in here would muddy it.
		const int32 Target = NearestNamed(*Swarm, CursorPoint);
		if (Target == INDEX_NONE)
		{
			return Refuse(*Swarm, Scheme, TEXT("nobody left to raise."));
		}
		A.RaiseUnit = Target;
		A.RaiseUntil = Now + SwarmCombatTuning::AbilityRaiseSeconds();
		Swarm->SetCastFocus(SoldierAt(*Swarm, Target), World->GetTimeSeconds() + 1.5);
		What = FString::Printf(TEXT("%s, %.0f HP/s for %.0fs"), SevenRoster::Get(Target).Name,
			SwarmCombatTuning::AbilityRaiseRate(), SwarmCombatTuning::AbilityRaiseSeconds());
		break;
	}
	case ESquadVerb::Rally:
	{
		A.RallyCentre = bModeB ? SoldierAt(*Swarm, Caster) : CursorPoint;
		A.RallyUntil = Now + SwarmCombatTuning::AbilityRallySeconds();
		Swarm->SetCastFocus(A.RallyCentre, World->GetTimeSeconds() + 1.5);
		What = FString::Printf(TEXT("banner at (%.0f, %.0f), swing x%.2f + no leash for %.0fs"),
			A.RallyCentre.X, A.RallyCentre.Y, SwarmCombatTuning::AbilityRallyHaste(),
			SwarmCombatTuning::AbilityRallySeconds());
		break;
	}
	default:
		return Refuse(*Swarm, Scheme, TEXT("no verb."));
	}

	// One clock advances, never both — see FAbilityState for why both sets exist.
	if (bModeB)
	{
		A.SoldierReadyAt[Caster] = Now + Cooldown;
	}
	else
	{
		A.VerbReadyAt[(int32)Verb] = Now + Cooldown;
	}
	A.CastsByScheme[FMath::Clamp((int32)Scheme, 0, USwarmSubsystem::FAbilityState::NumOrderSchemes - 1)]++;

	UE_LOG(LogTemp, Display, TEXT("Ability: %s (%s) — Q23=%s via %s, by %s — %s"),
		LexToString(Verb), SquadVerbSource(Verb), bModeB ? TEXT("B") : TEXT("A"),
		SchemeName(Scheme),
		bModeB ? SevenRoster::Get(Caster).Name : TEXT("the bearer"), *What);
	return true;
}

void DrawActiveZones(UWorld* World)
{
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm || SwarmCombatTuning::AbilityDraw() == 0)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const USwarmSubsystem::FAbilityState& A = Swarm->GetAbilities();
	const FVector Up(0.f, 0.f, 30.f);

	// ponytail: debug draw, not VFX. No capture path in this project can film Slate or debug
	// draws (slice-a7.md §8), so a whole-window screenshot is the only evidence channel there
	// is — and a debug circle is in it. Upgrade path is a decal or a Niagara ribbon; nothing
	// about the sim changes when that lands, since none of it is read by anything.
	if (Now < A.ScreenUntil)
	{
		DrawDebugCircle(World, A.ScreenCentre + Up, SwarmCombatTuning::AbilityScreenRadius(), 48,
			FColor(120, 190, 255), false, -1.f, 0, 8.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
	if (Now < A.RallyUntil)
	{
		DrawDebugCircle(World, A.RallyCentre + Up, SwarmCombatTuning::AbilityRallyRadius(), 48,
			FColor(255, 190, 90), false, -1.f, 0, 8.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		// The banner itself, so the zone has a thing at its centre and not just an outline.
		DrawDebugLine(World, A.RallyCentre, A.RallyCentre + FVector(0.f, 0.f, 320.f),
			FColor(255, 190, 90), false, -1.f, 0, 10.f);
	}

	// Focus and raise are TETHERS, not zones: both are relationships between two bodies, and a
	// line between them is the only drawing that says which two.
	if (Now < A.FocusUntil && Swarm->IsBossAlive())
	{
		for (int32 i = 0; i < SevenRoster::Num; ++i)
		{
			if ((A.FocusUnits & (uint8)(1u << i)) != 0 && IsStanding(*Swarm, i))
			{
				DrawDebugLine(World, SoldierAt(*Swarm, i) + Up, Swarm->GetBoss().Location + Up,
					FColor(255, 80, 80), false, -1.f, 0, 4.f);
			}
		}
	}
	if (Now < A.RaiseUntil && IsStanding(*Swarm, A.RaiseUnit))
	{
		DrawDebugLine(World, Swarm->GetAttractor() + Up, SoldierAt(*Swarm, A.RaiseUnit) + Up,
			FColor(255, 245, 190), false, -1.f, 0, 7.f);
	}
}

void LogReport(UWorld* World)
{
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const USwarmSubsystem::FAbilityState& A = Swarm->GetAbilities();
	const bool bModeB = SwarmCombatTuning::AbilityMode() == 1;

	UE_LOG(LogTemp, Display, TEXT("Kit: Q23 = %s — %s"), bModeB ? TEXT("B") : TEXT("A"),
		bModeB ? TEXT("the verb lives in the soldier; the bearer chooses WHO acts")
			   : TEXT("a fixed kit on the bearer, applied to whoever is in range"));

	for (int32 i = 0; i < SevenRoster::Num; ++i)
	{
		const SevenRoster::FSoldier& S = SevenRoster::Get(i);
		if (!IsStanding(*Swarm, i))
		{
			UE_LOG(LogTemp, Display, TEXT("  [%d] %-6s %-12s DOWN   %s lost for the run"),
				i, S.Name, S.Archetype, LexToString(S.Verb));
			continue;
		}
		const float Wait = FMath::Max(A.SoldierReadyAt[i] - Now, 0.f);
		UE_LOG(LogTemp, Display, TEXT("  [%d] %-6s %-12s %4.0f/%-4.0f  %-6s (%s)  %s"),
			i, S.Name, S.Archetype, Swarm->GetSquadHP(i), Swarm->GetSquadMaxHP(i),
			LexToString(S.Verb), SquadVerbSource(S.Verb),
			bModeB ? (Wait > 0.f ? *FString::Printf(TEXT("spent %.1fs"), Wait) : TEXT("ready"))
				   : TEXT("(inert — Q23 = A)"));
	}

	if (!bModeB)
	{
		FString Clocks;
		for (int32 V = 1; V < NumSquadVerbs; ++V)
		{
			const float Wait = FMath::Max(A.VerbReadyAt[V] - Now, 0.f);
			Clocks += FString::Printf(TEXT("%s %s  "), LexToString((ESquadVerb)V),
				Wait > 0.f ? *FString::Printf(TEXT("%.1fs"), Wait) : TEXT("ready"));
		}
		UE_LOG(LogTemp, Display, TEXT("  bearer's kit: %s"), *Clocks);
	}

	UE_LOG(LogTemp, Display, TEXT("  live: FOCUS %s | SCREEN %s | RAISE %s | RALLY %s"),
		Now < A.FocusUntil ? *FString::Printf(TEXT("%.1fs, %d marking"), A.FocusUntil - Now,
			(int32)FMath::CountBits(A.FocusUnits)) : TEXT("—"),
		Now < A.ScreenUntil ? *FString::Printf(TEXT("%.1fs"), A.ScreenUntil - Now) : TEXT("—"),
		Now < A.RaiseUntil && USwarmSubsystem::IsNamedUnit(A.RaiseUnit)
			? *FString::Printf(TEXT("%s %.1fs"), SevenRoster::Get(A.RaiseUnit).Name, A.RaiseUntil - Now)
			: TEXT("—"),
		Now < A.RallyUntil ? *FString::Printf(TEXT("%.1fs"), A.RallyUntil - Now) : TEXT("—"));

	// The one number an owner cannot get from a screenshot or from memory: once both Q26
	// schemes are on the same keyboard, which one did the hand actually reach for.
	UE_LOG(LogTemp, Display,
		TEXT("  casts/refusals by scheme — wheel(Q26=D) %d/%d | select-then-order(Q26=B) %d/%d | "
			"direct-target(Q26=A) %d/%d | console %d/%d"),
		A.CastsByScheme[0], A.RefusalsByScheme[0],
		A.CastsByScheme[1], A.RefusalsByScheme[1],
		A.CastsByScheme[2], A.RefusalsByScheme[2],
		A.CastsByScheme[3], A.RefusalsByScheme[3]);
}

} // namespace SquadAbilities

//----------------------------------------------------------------------
// Console surface — the headless half of the input scheme.
//
// The keyboard is the real surface (SpikeHeroPawn's Enhanced Input map), but no capture path
// in this project can film a HUD or a debug draw (slice-a7.md §8), so a log line is the only
// evidence a headless run can produce. These two commands exist so a verb can be fired and
// read back without a hand on the keyboard, not as a second design.
//----------------------------------------------------------------------
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GAbilityUseCmd(
		TEXT("Kindled.Ability.Use"),
		TEXT("Fire one verb. Usage:\n")
		TEXT("  Kindled.Ability.Use <0-6> [X Y]        — Q23 = B: that soldier acts (their verb)\n")
		TEXT("  Kindled.Ability.Use <verb> [X Y]       — Q23 = A: the bearer casts it\n")
		TEXT("      verb = focus|mark, screen|ward, raise|kindle, rally|banner\n")
		TEXT("X Y is the ground point the order is aimed at; omitted, the bearer's own position\n")
		TEXT("stands in for the cursor. Whichever form does not match Kindled.Ability.Mode is\n")
		TEXT("refused with the reason, which is itself the readout — see Kindled.Ability.Report."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm || Args.Num() < 1)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Ability.Use: usage <0-6|verb> [X Y]"));
					return;
				}
				FVector At = Swarm->GetAttractor();
				if (Args.Num() >= 3)
				{
					At.X = FCString::Atof(*Args[1]);
					At.Y = FCString::Atof(*Args[2]);
				}
				if (Args[0].IsNumeric())
				{
					SquadAbilities::Cast(World, ESquadVerb::None, FCString::Atoi(*Args[0]), At,
						SquadAbilities::EOrderScheme::Console);
					return;
				}
				const ESquadVerb Verb = SquadVerbFromToken(Args[0]);
				if (Verb == ESquadVerb::None)
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Ability.Use: no verb '%s' — "
						"focus, screen, raise, rally."), *Args[0]);
					return;
				}
				SquadAbilities::Cast(World, Verb, INDEX_NONE, At, SquadAbilities::EOrderScheme::Console);
			}));

	FAutoConsoleCommandWithWorld GAbilityReportCmd(
		TEXT("Kindled.Ability.Report"),
		TEXT("Log which shape of Q23 is live, each of the seven with their verb and cooldown,\n")
		TEXT("every standing effect, and how many casts each Q26 scheme actually delivered."),
		FConsoleCommandWithWorldDelegate::CreateStatic(
			[](UWorld* World) { SquadAbilities::LogReport(World); }));
}
