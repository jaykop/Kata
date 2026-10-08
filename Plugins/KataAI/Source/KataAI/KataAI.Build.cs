using UnrealBuildTool;

public class KataAI : ModuleRules
{
    public KataAI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "AIModule", "StateTreeModule", "KataTargeting", "TargetingSystem",
            "KataConditions", "KataRuntime", "KataGraph", "GameplayTags"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayAbilities", "GameplayStateTreeModule"
        });

        // 디버그 카테고리는 GameplayDebugger를 쓸 수 있는 대상에서만 빌드된다. Shipping 등에서는 WITH_GAMEPLAY_DEBUGGER가 0이다.
        SetupGameplayDebuggerSupport(Target);
    }
}
