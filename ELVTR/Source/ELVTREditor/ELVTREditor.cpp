#include "ELVTREditor.h"

#include "Breadboard/SBreadboardPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "Toolsets/KindledToolsets.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "ELVTREditor"

const FName FELVTREditorModule::BreadboardTabName(TEXT("ELVTRBreadboard"));

static TSharedRef<SDockTab> SpawnBreadboardTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBreadboardPanel)
		];
}

void FELVTREditorModule::OpenBreadboard()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(BreadboardTabName));
}

void FELVTREditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(BreadboardTabName,
			FOnSpawnTab::CreateStatic(&SpawnBreadboardTab))
		.SetDisplayName(LOCTEXT("BreadboardTabTitle", "Breadboard"))
		.SetTooltipText(LOCTEXT("BreadboardTabTooltip",
			"ELVTR tuning dials: every CVar in Saved/SwarmExecOnPlay.txt as a live field."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	// MCP surface: console write access and a live swarm read, both of which the
	// engine's own toolsets leave out.
	UToolsetRegistry::RegisterToolsetClass(UKindledConsoleToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UKindledSwarmToolset::StaticClass());
}

void FELVTREditorModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UKindledSwarmToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UKindledConsoleToolset::StaticClass());

	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BreadboardTabName);
	}
}

/** Typed in the editor console so the panel can be opened without hunting through menus —
 *  this is also how an automated session (MCP / Claude) pops the breadboard open. */
static FAutoConsoleCommand GOpenBreadboardCommand(
	TEXT("Kindled.Breadboard"),
	TEXT("Open the ELVTR breadboard: every tuning CVar as a live field, with Save to file."),
	FConsoleCommandDelegate::CreateStatic(&FELVTREditorModule::OpenBreadboard));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FELVTREditorModule, ELVTREditor);
