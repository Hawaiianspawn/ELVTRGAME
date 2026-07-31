#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Editor-only module for ELVTR's in-engine tooling. Its first tenant is the breadboard —
 * the tuning-dial panel (Window > Tools > Breadboard, or the `Kindled.Breadboard` console
 * command). Tools live here rather than in the runtime module so nothing editor-only has to be
 * `WITH_EDITOR`-guarded inside shipping game code.
 */
class FELVTREditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Nomad tab id for the breadboard. */
	static const FName BreadboardTabName;

	/** Open (or focus) the breadboard tab. Backs the `Kindled.Breadboard` console command. */
	static void OpenBreadboard();
};
