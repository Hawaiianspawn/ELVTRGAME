using UnrealBuildTool;

public class ELVTREditor : ModuleRules
{
	public ELVTREditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Flat module layout (no Public/Private), same as the runtime module.
		PrivateIncludePaths.Add(ModuleDirectory);

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"UnrealEd",              // editor module boilerplate + asset/editor services
			"WorkspaceMenuStructure", // puts the breadboard tab under Window > Tools
			"UMG"                     // so the panel can also be hosted inside an EditorUtilityWidget
		});
	}
}
