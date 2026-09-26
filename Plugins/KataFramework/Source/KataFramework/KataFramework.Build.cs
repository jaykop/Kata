using UnrealBuildTool;

public class KataFramework : ModuleRules
{
    public KataFramework(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 ACharacter, IAbilitySystemInterface, IGenericTeamAgentInterface, UKataActionComponent, UKataGraphComponent,
        // UKataTargetingComponent, UKataTask, FGameplayTag, UDeveloperSettings를 노출하므로 Public에 둔다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "AIModule",
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "GameplayAbilities",
            "GameplayTags",
            "KataGraph",
            "KataRuntime",
            "KataTargeting"
        });

        // Hit Trace의 대상 필터가 UTargetingPreset의 Filter 태스크를 실행한다. 공개 헤더는 전방 선언만 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "TargetingSystem"
        });
    }
}
