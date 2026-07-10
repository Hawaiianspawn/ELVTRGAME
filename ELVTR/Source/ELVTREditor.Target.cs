using UnrealBuildTool;
using System.Collections.Generic;

public class ELVTREditorTarget : TargetRules
{
	public ELVTREditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ELVTR");
	}
}
