using UnrealBuildTool;

public class ELVTR : ModuleRules
{
	public ELVTR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Flat module layout (no Public/Private): allow module-root-relative includes.
		PrivateIncludePaths.Add(ModuleDirectory);

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
			"RHI"
		});
	}
}
