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

			// task-137's migration off raw key polling, landed by task-144: the bearer's input
			// is one C++-built UInputMappingContext (SpikeHeroPawn::BuildInputMap). Needed for
			// the Q26 = D verb wheel, which is a HOLD — a real Started/Completed pair, not two
			// IsInputKeyDown reads compared against last frame's bool.
			"EnhancedInput",
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
