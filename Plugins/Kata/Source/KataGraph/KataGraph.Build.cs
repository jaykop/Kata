using UnrealBuildTool;

public class KataGraph : ModuleRules
{
    public KataGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "KataConditions",
            "KataRuntime"
        });
    }
}
