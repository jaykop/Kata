using UnrealBuildTool;

public class KataTargeting : ModuleRules
{
    public KataTargeting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 FGameplayTag, FGenericTeamId, UDeveloperSettings, TargetingSystem 태스크 기반 클래스와
        // Kata 코어의 UKataCommand를 노출하므로 Public에 둔다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "AIModule",
            "DeveloperSettings",
            "TargetingSystem",
            "KataRuntime"
        });

        // 락온 해제 태그를 대상 ASC에서 조회할 때만 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities"
        });
    }
}
