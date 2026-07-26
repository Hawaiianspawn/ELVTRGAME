// Preview + auto-show for the M1 UI over the running Spike map.
//   Emberkeep.UI.Hud       — bottom band: Muster (centre) + Unit Cam (projection, right)
//   Emberkeep.UI.Muster    — muster band only (no cam)
//   Emberkeep.UI.Clear     — remove UI added by these helpers
//   Emberkeep.UI.AutoShow  — cvar (default 1): show the HUD automatically on Play
//
// The helpers find the active PIE/Game world themselves, so the console commands work
// from any console as long as a play session is running.

#include "UI/EmberkeepUIDebug.h"
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "UI/EmberkeepHud.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

static TAutoConsoleVariable<int32> CVarUIAutoShow(
	TEXT("Emberkeep.UI.AutoShow"), 1,
	TEXT("Auto-show the combat HUD (with cams) when a play session starts. 1=on (default), 0=off."),
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

void EmberkeepUI::ShowCombatHud(UWorld* Passed, bool bWithCams)
{
	UWorld* World = FindPlayWorld(Passed);
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Emberkeep.UI: no active play world. Press Play (PIE), then run this."));
		return;
	}

	UEmberkeepHud* Hud = CreateWidget<UEmberkeepHud>(PC, UEmberkeepHud::StaticClass());
	if (!Hud)
	{
		return;
	}
	Hud->AddToViewport(100); // build the widget tree first, then feed it

	// No captures any more: the hero cam is gone and the Unit Cam is the capture-free projection
	// (UUnitCamProjector, docs/RENDERING-LIGHTING.md §4d), hosted directly in the band. bWithCams
	// just decides whether the Unit Cam shows (muster-only preview passes false).
	Hud->Setup(bWithCams);
	Hud->UseMockData();
	GSpawnedDebugWidgets.Add(Hud);

	UE_LOG(LogTemp, Display, TEXT("Emberkeep.UI: band added (%s)."),
		bWithCams ? TEXT("with cams") : TEXT("muster only"));
}

void EmberkeepUI::ClearHud()
{
	for (const TWeakObjectPtr<UUserWidget>& Weak : GSpawnedDebugWidgets)
	{
		if (UUserWidget* Widget = Weak.Get())
		{
			Widget->RemoveFromParent();
		}
	}
	GSpawnedDebugWidgets.Reset();

	for (const TWeakObjectPtr<AActor>& Weak : GSpawnedDebugActors)
	{
		if (AActor* Actor = Weak.Get())
		{
			Actor->Destroy();
		}
	}
	GSpawnedDebugActors.Reset();
}

void EmberkeepUI::AutoShowIfEnabled(UWorld* World)
{
	if (CVarUIAutoShow.GetValueOnGameThread() != 0)
	{
		ShowCombatHud(World, /*bWithCams*/ true);
	}
}

// --- console commands --------------------------------------------------------

static FAutoConsoleCommandWithWorld GShowHudCmd(
	TEXT("Emberkeep.UI.Hud"),
	TEXT("Spawn the combat-HUD bottom band with live Hero/Unit cam feeds. Requires an active play session."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { EmberkeepUI::ShowCombatHud(World, true); }));

static FAutoConsoleCommandWithWorld GShowMusterCmd(
	TEXT("Emberkeep.UI.Muster"),
	TEXT("Spawn the muster band only, no cams. Requires an active play session."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { EmberkeepUI::ShowCombatHud(World, false); }));

static FAutoConsoleCommandWithWorld GClearUICmd(
	TEXT("Emberkeep.UI.Clear"),
	TEXT("Remove UI and capture actors added by the Emberkeep.UI helpers."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* /*World*/) { EmberkeepUI::ClearHud(); }));
