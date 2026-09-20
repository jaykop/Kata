using UnrealBuildTool;

public class ProjectKataTesting : ModuleRules
{
    public ProjectKataTesting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "GameplayAbilities",
            "KataRuntime"
        });

        PrivateDependencyModuleNames.Add("KataConditions");
    }
}
