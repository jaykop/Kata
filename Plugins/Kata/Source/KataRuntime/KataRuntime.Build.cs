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

        // 실행 이력 디버그 카테고리는 GameplayDebugger를 쓸 수 있는 대상에서만 빌드된다. Shipping 등에서는 WITH_GAMEPLAY_DEBUGGER가 0이다.
        // GameplayDebugger는 엔진 모듈이라 코어에 엔진 플러그인 의존을 더하지 않는다.
        SetupGameplayDebuggerSupport(Target);
    }
}
