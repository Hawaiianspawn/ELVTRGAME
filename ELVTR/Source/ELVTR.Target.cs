using UnrealBuildTool;
using System.Collections.Generic;

public class ELVTRTarget : TargetRules
{
	public ELVTRTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ELVTR");
	}
}
