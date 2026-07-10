using UnrealBuildTool;

public class ELVTR : ModuleRules
{
	public ELVTR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
			"Niagara"
		});
	}
}
