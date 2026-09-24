using UnrealBuildTool;

public class KataTargeting : ModuleRules
{
    public KataTargeting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 FGameplayTag, FGenericTeamId, UDeveloperSettings를 노출하므로 Public에 둔다.
        // Kata 코어와 TargetingSystem은 이를 실제로 쓰는 타게팅 컴포넌트를 추가할 때 의존에 넣는다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "AIModule",
            "DeveloperSettings"
        });
    }
}
