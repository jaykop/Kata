using UnrealBuildTool;

public class KataGraph : ModuleRules
{
    public KataGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "KataConditions",
            "KataRuntime"
        });

        // 자동화 테스트(Private/Tests)가 실행 주체의 ASC를 직접 만든다. 헤더 경로는 KataRuntime을 거쳐 들어오지만
        // 링크하려면 이 모듈이 직접 의존해야 한다. 런타임 그래프 코드는 GAS 심볼을 쓰지 않는다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities"
        });
    }
}
