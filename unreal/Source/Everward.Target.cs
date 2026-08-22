using UnrealBuildTool;
using System.Collections.Generic;

public class EverwardTarget : TargetRules
{
    public EverwardTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Everward");
    }
}
