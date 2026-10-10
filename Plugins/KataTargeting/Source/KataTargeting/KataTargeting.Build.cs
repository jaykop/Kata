using UnrealBuildTool;

public class KataTargeting : ModuleRules
{
    public KataTargeting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 FGameplayTag, FGenericTeamId, UDeveloperSettings, TargetingSystem 태스크 기반 클래스와
        // Kata 코어의 UKataCommand, UKataCondition을 노출하므로 Public에 둔다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "AIModule",
            "DeveloperSettings",
            "TargetingSystem",
            "KataConditions",
            "KataRuntime"
        });

        // 락온 해제 태그와 Owned Tags 필터가 대상 ASC의 태그를 조회할 때만 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities"
        });

        // 디버그 카테고리는 GameplayDebugger를 쓸 수 있는 대상에서만 빌드된다. Shipping 등에서는 WITH_GAMEPLAY_DEBUGGER가 0이다.
        SetupGameplayDebuggerSupport(Target);
    }
}
