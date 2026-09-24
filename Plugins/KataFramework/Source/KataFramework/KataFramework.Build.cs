using UnrealBuildTool;

public class KataFramework : ModuleRules
{
    public KataFramework(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 ACharacter, IAbilitySystemInterface, UKataComponent를 노출하므로 Public에 둔다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayAbilities",
            "KataRuntime"
        });
    }
}
