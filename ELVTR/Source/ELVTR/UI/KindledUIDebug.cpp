// Preview + auto-show for the M1 UI over the running Spike map.
//   Kindled.UI.Hud       — spawn the bottom muster band
//   Kindled.UI.Clear     — remove UI added by these helpers
//   Kindled.UI.AutoShow  — cvar (default 1): show the HUD automatically on Play
//
// The helpers find the active PIE/Game world themselves, so the console commands work
// from any console as long as a play session is running.

#include "UI/KindledUIDebug.h"
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "UI/KindledHud.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

static TAutoConsoleVariable<int32> CVarUIAutoShow(
	TEXT("Kindled.UI.AutoShow"), 1,
	TEXT("Auto-show the combat HUD when a play session starts. 1=on (default), 0=off."),
	ECVF_Default);

namespace
{
	TArray<TWeakObjectPtr<UUserWidget>> GSpawnedDebugWidgets;
	TArray<TWeakObjectPtr<AActor>> GSpawnedDebugActors;

	// Prefer a running PIE world; fall back to a standalone Game world.
	UWorld* FindPlayWorld(UWorld* Passed)
	{
		if (Passed && Passed->GetFirstPlayerController())
		{
			return Passed;
		}
		if (GEngine)
		{
			UWorld* GameFallback = nullptr;
			for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
			{
				if (!Ctx.World())
				{
					continue;
				}
				if (Ctx.WorldType == EWorldType::PIE)
				{
					return Ctx.World();
				}
				if (Ctx.WorldType == EWorldType::Game)
				{
					GameFallback = Ctx.World();
				}
			}
			return GameFallback;
		}
		return nullptr;
	}

}

void KindledUI::ShowCombatHud(UWorld* Passed)
{
	UWorld* World = FindPlayWorld(Passed);
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Kindled.UI: no active play world. Press Play (PIE), then run this."));
		return;
	}

	UKindledHud* Hud = CreateWidget<UKindledHud>(PC, UKindledHud::StaticClass());
	if (!Hud)
	{
		return;
	}
	Hud->AddToViewport(100);
	GSpawnedDebugWidgets.Add(Hud);

	UE_LOG(LogTemp, Display, TEXT("Kindled.UI: muster band added."));
}

void KindledUI::ClearHud()
{
	for (const TWeakObjectPtr<UUserWidget>& Weak : GSpawnedDebugWidgets)
	{
		if (UUserWidget* Widget = Weak.Get())
		{
			Widget->RemoveFromParent();
		}
	}
	GSpawnedDebugWidgets.Reset();
}

void KindledUI::AutoShowIfEnabled(UWorld* World)
{
	if (CVarUIAutoShow.GetValueOnGameThread() != 0)
	{
		ShowCombatHud(World);
	}
}

// --- console commands --------------------------------------------------------

static FAutoConsoleCommandWithWorld GShowHudCmd(
	TEXT("Kindled.UI.Hud"),
	TEXT("Spawn the combat-HUD bottom muster band. Requires an active play session."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { KindledUI::ShowCombatHud(World); }));

static FAutoConsoleCommandWithWorld GClearUICmd(
	TEXT("Kindled.UI.Clear"),
	TEXT("Remove UI added by the Kindled.UI helpers."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* /*World*/) { KindledUI::ClearHud(); }));
