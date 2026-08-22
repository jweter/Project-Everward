using UnrealBuildTool;
using System.Collections.Generic;

public class EverwardEditorTarget : TargetRules
{
    public EverwardEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Everward");
    }
}
