using UnrealBuildTool;
using System.Collections.Generic;

public class EverwardBenchmarkTarget : TargetRules
{
    public EverwardBenchmarkTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EverwardBenchmark");
    }
}
