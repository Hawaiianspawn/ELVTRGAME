using UnrealBuildTool;

public class ELVTR : ModuleRules
{
	public ELVTR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Flat module layout (no Public/Private): allow module-root-relative includes.
		// Public, not private: ELVTREditor's MCP toolsets include Mass/SwarmSubsystem.h.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"MassCore",
			"MassEntity",
			"MassCommon",
			"MassMovement",
			"Niagara",
			"RenderCore",
			"RHI",

			// UI (M1 prototype — docs/ui/UI-PROTOTYPE-PLAN.md). Plain UUserWidget: nothing here
			// uses CommonUI's activatable stacks or input routing, so CommonUI/CommonInput/
			// GameplayTags are not linked. Re-add them with the base class when a screen stack
			// actually needs them.
			"UMG",
			"Slate",
			"SlateCore"
		});
	}
}
