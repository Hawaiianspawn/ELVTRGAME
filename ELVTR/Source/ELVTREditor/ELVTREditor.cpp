#include "ELVTREditor.h"

#include "Breadboard/SBreadboardPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
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
}

void FELVTREditorModule::ShutdownModule()
{
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
