using UnrealBuildTool;

public class KataCamera : ModuleRules
{
    public KataCamera(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 엔진 타입, FGameplayTag, 카메라 StateTree의 FStateTreeReference·노드 기반 타입을 노출하므로 Public에 둔다.
        // 코어 Kata는 이를 실제로 쓰는 단계(카메라 태스크)에서 추가한다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "StateTreeModule"
        });

        // Status 태그를 읽고 구독할 때만 ASC를 쓴다. 공개 헤더는 전방 선언만 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities"
        });

        // 디버그 카테고리는 GameplayDebugger를 쓸 수 있는 대상에서만 빌드된다. Shipping 등에서는 WITH_GAMEPLAY_DEBUGGER가 0이다.
        SetupGameplayDebuggerSupport(Target);
    }
}
