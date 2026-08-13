#include "KindledToolsets.h"

#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmSubsystem.h"
#include "Mass/SwarmTelemetry.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KindledToolsets)

namespace
{
	/**
	 * PIE first, editor world second. Tuning a live fight is the whole point, and a
	 * CVar set against the editor world while PIE is up would land in the wrong
	 * place — GEngine->Exec routes some commands through the world it is handed.
	 */
	UWorld* ResolveWorld()
	{
		if (!GEditor)
		{
			return GWorld;
		}
		if (UWorld* PlayWorld = GEditor->PlayWorld)
		{
			return PlayWorld;
		}
		return GEditor->GetEditorWorldContext().World();
	}

	/** Captures whatever a command prints, one line per Serialize call. */
	struct FCapture : public FOutputDevice
	{
		FString Text;

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type, const FName&) override
		{
			if (!Text.IsEmpty())
			{
				Text.AppendChar(TEXT('\n'));
			}
			Text.Append(V);
		}
	};

	void WriteVector(const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& Json,
		const TCHAR* Key, const FVector& V)
	{
		Json->WriteArrayStart(Key);
		Json->WriteValue(V.X);
		Json->WriteValue(V.Y);
		Json->WriteValue(V.Z);
		Json->WriteArrayEnd();
	}
}

FString UKindledConsoleToolset::Exec(const FString& Command)
{
	const FString Trimmed = Command.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Command cannot be empty."));
		return FString();
	}

	UWorld* World = ResolveWorld();
	FCapture Capture;

	// ProcessUserConsoleInput first: it is what handles CVar reads and assignments,
	// and it reports "Command not recognized" itself for anything else, which would
	// bury a real exec command. Only fall through to Exec when it declines.
	if (!IConsoleManager::Get().ProcessUserConsoleInput(*Trimmed, Capture, World))
	{
		if (!GEngine->Exec(World, *Trimmed, Capture) && Capture.Text.IsEmpty())
		{
			Capture.Text = FString::Printf(TEXT("Command not recognized: %s"), *Trimmed);
		}
	}

	return Capture.Text;
}

FString UKindledConsoleToolset::SetCVar(const FString& Name, const FString& Value)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Name);
	if (!CVar)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(
			TEXT("No console variable named '%s'. Use EditorAppToolset.SearchCVars to find the "
				 "real name."), *Name));
		return FString();
	}

	// SetByConsole, not SetByCode: same precedence a human typing into the console
	// gets, so a later console edit can still override this and vice versa.
	CVar->Set(*Value, ECVF_SetByConsole);
	return CVar->GetString();
}

FString UKindledSwarmToolset::Snapshot()
{
	UWorld* World = ResolveWorld();
	if (!World)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("No world to query."));
		return FString();
	}

	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Json =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);

	Json->WriteObjectStart();
	Json->WriteValue(TEXT("pie"), GEditor && GEditor->PlayWorld != nullptr);
	Json->WriteValue(TEXT("worldTime"), World->GetTimeSeconds());

	const USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		// Not an error: an editor world with no swarm actor placed has no subsystem.
		Json->WriteValue(TEXT("swarm"), TEXT("no USwarmSubsystem in this world"));
		Json->WriteObjectEnd();
		Json->Close();
		return Out;
	}

	Json->WriteObjectStart(TEXT("alive"));
	Json->WriteValue(TEXT("retinue"), Swarm->GetAliveRetinue());
	Json->WriteValue(TEXT("brood"), Swarm->GetAliveBrood());
	Json->WriteValue(TEXT("spearmen"), Swarm->GetAliveByType(EUnitType::Spearmen));
	Json->WriteValue(TEXT("archers"), Swarm->GetAliveByType(EUnitType::Archers));
	Json->WriteValue(TEXT("leashBroken"), Swarm->GetLeashBrokenCount());
	Json->WriteObjectEnd();

	Json->WriteValue(TEXT("renderCount"), Swarm->GetRenderPositions().Num());
	Json->WriteValue(TEXT("gridCells"), Swarm->GetGridCellCount());

	Json->WriteObjectStart(TEXT("hero"));
	Json->WriteValue(TEXT("alive"), Swarm->IsHeroAlive());
	Json->WriteValue(TEXT("hp"), Swarm->GetHeroHP());
	Json->WriteValue(TEXT("maxHp"), Swarm->GetHeroMaxHP());
	Json->WriteValue(TEXT("striking"), Swarm->IsHeroStriking());
	Json->WriteObjectEnd();

	Json->WriteValue(TEXT("stance"), LexToString(Swarm->GetStance()));
	WriteVector(Json, TEXT("stanceAnchor"), Swarm->GetStanceAnchor());
	WriteVector(Json, TEXT("attractor"), Swarm->GetAttractor());

	Json->WriteObjectStart(TEXT("kills"));
	Json->WriteValue(TEXT("retinue"), (double)Swarm->GetTotalKilledRetinue());
	Json->WriteValue(TEXT("brood"), (double)Swarm->GetTotalKilledBrood());
	Json->WriteValue(TEXT("heroWave"), Swarm->GetHeroWaveKills());
	Json->WriteValue(TEXT("heroRun"), Swarm->GetHeroRunKills());
	Json->WriteObjectEnd();

	Json->WriteObjectStart(TEXT("damage"));
	Json->WriteValue(TEXT("toRetinue"), Swarm->GetTotalDamageToRetinue());
	Json->WriteValue(TEXT("toBrood"), Swarm->GetTotalDamageToBrood());
	Json->WriteValue(TEXT("hero"), Swarm->GetTotalHeroDamage());
	Json->WriteObjectEnd();

	// Claimed slots only. Eight always-present entries of zeroes would be eight
	// lines of nothing in every reply.
	Json->WriteArrayStart(TEXT("squads"));
	for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
	{
		if (!Swarm->IsSquadClaimed(i))
		{
			continue;
		}
		Json->WriteObjectStart();
		Json->WriteValue(TEXT("index"), i);
		Json->WriteValue(TEXT("type"), LexToString(Swarm->GetSquadType(i)));
		Json->WriteValue(TEXT("standing"), Swarm->GetSquadStanding(i));
		Json->WriteValue(TEXT("stance"), LexToString(Swarm->GetUnitStance(i)));
		WriteVector(Json, TEXT("centroid"), Swarm->GetSquadCentroid(i));
		Json->WriteValue(TEXT("waveKills"), Swarm->GetWaveKilledBySquad(i));
		Json->WriteValue(TEXT("runKills"), Swarm->GetRunKilledBySquad(i));
		Json->WriteObjectEnd();
	}
	Json->WriteArrayEnd();

	// The fight recorder's running totals, so a balance question can be answered
	// mid-fight instead of waiting for fights.csv to be written at the end.
	const USwarmTelemetrySubsystem* Telemetry = World->GetSubsystem<USwarmTelemetrySubsystem>();
	if (Telemetry && Telemetry->IsRecording())
	{
		const FSwarmFightRecord& Fight = Telemetry->GetCurrentRecord();
		Json->WriteObjectStart(TEXT("fight"));
		Json->WriteValue(TEXT("index"), Fight.Index);
		Json->WriteValue(TEXT("duration"), World->GetTimeSeconds() - Fight.StartTime);
		Json->WriteValue(TEXT("killedRetinue"), (double)Fight.KilledRetinue);
		Json->WriteValue(TEXT("killedBrood"), (double)Fight.KilledBrood);
		Json->WriteValue(TEXT("exchangeRate"), Fight.ExchangeRate());
		Json->WriteObjectEnd();
	}

	Json->WriteObjectEnd();
	Json->Close();
	return Out;
}
