using UnrealBuildTool;

public class KataAI : ModuleRules
{
    public KataAI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "AIModule", "StateTreeModule", "KataTargeting", "TargetingSystem",
            "KataConditions", "KataRuntime", "KataGraph", "GameplayTags"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities", "GameplayStateTreeModule"
        });
    }
}
