using UnrealBuildTool;

public class KataTargeting : ModuleRules
{
    public KataTargeting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 FGameplayTag, FGenericTeamId, UDeveloperSettings와 TargetingSystem 태스크 기반 클래스를 노출하므로 Public에 둔다.
        // Kata 코어는 이를 실제로 쓰는 대상 결정 Command를 추가할 때 의존에 넣는다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "AIModule",
            "DeveloperSettings",
            "TargetingSystem"
        });

        // 락온 해제 태그를 대상 ASC에서 조회할 때만 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities"
        });
    }
}
