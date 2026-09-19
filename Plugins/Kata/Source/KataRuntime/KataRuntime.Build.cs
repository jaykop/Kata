using UnrealBuildTool;

public class KataRuntime : ModuleRules
{
    public KataRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "KataConditions",
            "GameplayTags",
            "GameplayTasks",
            "GameplayAbilities"
        });
    }
}
